/* -------------------------------------------------------------------------
 *  This file is part of the MindStudio project.
 * Copyright (c) 2025 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * ------------------------------------------------------------------------- */


#include "vector_utilization_calculator.h"

#include <bitset>
#include <algorithm>

#include "number_operation.h"
#include "common/defs.h"
#include "instr_detail_utils.h"
#include "instr_in_camodel/camodel_dtype_defs.h"

namespace Profiling {
namespace Parse {

constexpr uint32_t START_BIT = 56;
constexpr uint32_t BIT_LEN = 8;
constexpr float DEFAULT_VEC_UTILIZATION = -1;

uint8_t RepeatCalculateCommon(const uint64_t &config)
{
    return static_cast<uint8_t>(Utility::ExtractKBits(config, START_BIT, BIT_LEN));
}

const std::unordered_map<VecInstrTemplate, std::function<uint8_t(uint64_t)>> SpecialVecRepeatCalculate = {
    {VecInstrTemplate::VREDUCEV2, [](const uint64_t &config) {
        return static_cast<uint16_t>((Utility::ExtractKBits(config, START_BIT, BIT_LEN) << 8U)
        + Utility::ExtractKBits(config, 0, BIT_LEN));}},
    {VecInstrTemplate::VMS4, [](const uint64_t &config) {
        return Utility::ExtractKBits(config, 0, BIT_LEN);}},
    {VecInstrTemplate::VMOVMASK_CMPMASK_XN, [](const uint64_t &config) {
        (void)config;
        return 1;}}
};

PluginErrorCode VectorUtilizationCalculator::Entry()
{
    AttributeMapInit();
    std::shared_ptr<InstrDetailTable> instrDetailTable = dataCenter_.GetDbPtr<InstrDetailTable>();
    if (instrDetailTable == nullptr) {
        return PluginErrorCode::NONBLOCKING_ERROR;
    }

    InstrLogStr2Template mapping;
    if (!GetInstrLogStr2TemplateMap(instrDetailConfig_.GetProductSeriesType(),
                                    mapping)) {
        Utility::LogDebug("Get instr name to template map failed.");
        return PluginErrorCode::NONBLOCKING_ERROR;
    }
    SpReg spStatus = {0, 0, 0, MaskMode::REPEAT_MODE};
    for (uint32_t i = 0; i < instrDetailTable->GetSize(); i++) {
        MergeInfo* mergeInfo = instrDetailTable->QueryColumnValue<MergeInfo>(InstrDetailTable::MERGE_INFO, i);
        if (mergeInfo == nullptr) {
            instrDetailTable->UpdateColumnValue(InstrDetailTable::VEC_UTILIZATION, i, DEFAULT_VEC_UTILIZATION);
            continue;
        }
        UpdateSpReg(mergeInfo->name, mergeInfo->detail, spStatus);
        auto it = mapping.find(mergeInfo->name);
        if (it == mapping.end()) {
            instrDetailTable->UpdateColumnValue(InstrDetailTable::VEC_UTILIZATION, i, DEFAULT_VEC_UTILIZATION);
        } else {
            instrDetailTable->UpdateColumnValue(InstrDetailTable::VEC_UTILIZATION, i,
                                                CalVecUtilization(it->second, *mergeInfo, spStatus));
        }
    }
    return PluginErrorCode::SUCCESS;
}

float VectorUtilizationCalculator::CalVecUtilization(VecInstrTemplate vecType,
                                                     const MergeInfo &mergeInfo, const SpReg &spRegStatus)
{
    constexpr float percentage = 100;
    float vecCalElementsNum = 0;
    uint64_t repeatTimes = 0;
    if (IsChipSeriesTypeValid(instrDetailConfig_.GetChipType(), ChipProductType::ASCEND310P_SERIES)) {
        // 310P provide direct result
        uint64_t dtypeValue = static_cast<uint32_t>(VectorDtype310P::VEC_NA);
        if (!GetRegValue(regDetailRegexMap_.at(RegNameKey::VEC_REPEAT), mergeInfo.detail, repeatTimes,
                         Utility::RADIX_10) ||
            !GetRegValue(regDetailRegexMap_.at(RegNameKey::VEC_DTYPE), mergeInfo.detail, dtypeValue,
                         Utility::RADIX_10) ||
            dtypeValue >= static_cast<uint64_t>(VectorDtype310P::VEC_NA)) {
            return DEFAULT_VEC_UTILIZATION;
        }
        InstrDataType instrDataType = VEC_DTYPE_BYTES_310P_MAP.at(static_cast<VectorDtype310P>(dtypeValue));
        uint8_t dtypeSize = static_cast<uint8_t>(instrDataType);
        vecCalElementsNum = static_cast<float>(ElementCount(repeatTimes, spRegStatus));
        float vecUtil = vecCalElementsNum * static_cast<float>(dtypeSize) / MAX_VEC_PROCESSED_BYTES * percentage;
        return std::min(vecUtil, 100.0f);
    }
    // normal calculate flow: A2 A3
    uint64_t config = 0;
    std::string dtypeStr;
    if (!GetRegValue(regDetailRegexMap_.at(RegNameKey::XT_VALUE), mergeInfo.detail, config) ||
        !GetRegString(regDetailRegexMap_.at(RegNameKey::DTYPE), mergeInfo.detail, dtypeStr) ||
        VEC_DTYPE_BYTES_A2A3_MAP.find(dtypeStr) == VEC_DTYPE_BYTES_A2A3_MAP.end()) {
        return DEFAULT_VEC_UTILIZATION;
    }
    auto it = SpecialVecRepeatCalculate.find(vecType);
    if (it == SpecialVecRepeatCalculate.end()) {
        repeatTimes = RepeatCalculateCommon(config);
    } else {
        repeatTimes = it->second(config);
    }
    vecCalElementsNum = static_cast<float>(ElementCount(repeatTimes, spRegStatus));
    uint8_t dtypeSize = static_cast<uint8_t>(VEC_DTYPE_BYTES_A2A3_MAP.at(dtypeStr));
    float vecUtil = vecCalElementsNum * static_cast<float>(dtypeSize) / MAX_VEC_PROCESSED_BYTES * percentage;
    return std::min(vecUtil, 100.0f);
}

uint32_t VectorUtilizationCalculator::ElementCount(const uint64_t repeatTimes, const SpReg &spRegStatus) const
{
    constexpr size_t size64 = 64;
    if (spRegStatus.maskMode == MaskMode::REPEAT_MODE) {
        std::bitset<size64> bits0(spRegStatus.vectorMask0);
        std::bitset<size64> bits1(spRegStatus.vectorMask1);
        return (bits0.count() + bits1.count()) * repeatTimes;
    } else if (spRegStatus.maskMode == MaskMode::ELEMENT_COUNT_MODE) {
        if (spRegStatus.vectorMask1 == 0) {
            return spRegStatus.vectorMask0;
        }
        return spRegStatus.vectorMask1;
    }
    return 0;
}

}
}
