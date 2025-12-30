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

#include "compute_load_calculator.h"
#include "log.h"

using namespace Visualize;
using namespace Utility;
using namespace Common;

namespace Profiling {
namespace Parse {
static uint8_t VEC_INDEX = 0;

PluginErrorCode ComputeLoadCalculator::Entry()
{
    std::shared_ptr<ComputeLoadInfo> computeLoadInfoPtr = MakeShared<ComputeLoadInfo>();
    if (!dataCenter_.DataTableRegister(computeLoadInfoPtr)) {
        Utility::LogDebug("Failed to register compute load info.");
        return PluginErrorCode::NONBLOCKING_ERROR;
    }
    std::map<uint8_t, std::vector<nlohmann::json>> jsonMap = { {0, std::vector<nlohmann::json>()},
                                                               {1, std::vector<nlohmann::json>()} };
    if (Common::IsChipSeriesTypeValid(chipType_, Common::ChipProductType::ASCEND910B_SERIES)) {
        ComputeLoadInfoFor910B(jsonMap);
    } else if (Common::IsChipSeriesTypeValid(chipType_, Common::ChipProductType::ASCEND310P_SERIES)) {
        ComputeLoadInfoFor310P(jsonMap);
    } else if (Common::IsChipSeriesTypeValid(chipType_, Common::ChipProductType::ASCEND910_95_SERIES)) {
        ComputeLoadInfoFor91095(jsonMap);
    }
    computeLoadInfoPtr->figure["subblock_detail"] = jsonMap[0];
    computeLoadInfoPtr->table["subblock_detail"] = jsonMap[1];
    computeLoadInfoPtr->figure["advice"] = std::vector<nlohmann::json>();
    computeLoadInfoPtr->table["advice"] = std::vector<nlohmann::json>();
    return PluginErrorCode::SUCCESS;
}

void ComputeLoadCalculator::ComputeLoadInfoFor910B(std::map<uint8_t, std::vector<nlohmann::json>> &jsonMap)
{
    std::shared_ptr<BasicPmu> basicPmuPtr = dataCenter_.GetDbPtr<BasicPmu>();
    if (basicPmuPtr == nullptr) {
        Utility::LogDebug("Failed to get basic pmu.");
        return;
    }
    const std::vector<ComputeLoadBlockDetail>& detail = basicPmuPtr->GetComputeLoadBlockDetail();

    // detailInfoVecAic记录了aic下需要记录的详细信息，分别包括计算负载的名称，数据类型和存储的目的json
    // 其存储的值由于来自不同event id且存在部分结果需要计算得出，因此定义eventResVecAic记录detailInfoVecAic中对应负载的值
    // eventResVecAic与detailInfoVecAic的值一一对应
    for (size_t i = 0; i < detail.size(); ++i) {  // 保存了某个block上的所有数据
        std::map<uint64_t, uint64_t> eventMap = detail[i].eventMap;
        if (detail[i].blockType.find("cube") != std::string::npos) {
            // 生成cube数据
            auto cubeAllActive = SafeAdd(eventMap[73], eventMap[74], "cube all activate");
            std::vector<uint64_t> eventResVecAic = {cubeAllActive, eventMap[73], eventMap[74],
                eventMap[88], eventMap[3], eventMap[1032], eventMap[1033]};       // aic_88/3/1032/1033
            for (size_t j = 0; j < basicPmuPtr->detailInfoVecAic_.size(); ++j) {
                GenBlockDetail(detail[i], eventResVecAic[j], basicPmuPtr->detailInfoVecAic_[j], jsonMap);
            }
        } else {
            // 生成vec数据
            uint64_t fp16Value = SafeAddAll(std::vector<uint64_t>{eventMap[76], eventMap[77], eventMap[174]}, "fp 16 value");                  // aiv_76+aiv_77+aiv_174
            uint64_t allBlock = SafeAddAll(std::vector<uint64_t>{eventMap[100], eventMap[101], eventMap[102], eventMap[103]}, "all block value"); // aiv(100+101+102+103)
            uint64_t wait = (detail[i].freq <= 0) || eventMap[89] == UINT64_MAX ? 0 : static_cast<uint64_t>
                ((eventMap[89] / static_cast<float >(detail[i].freq))); // aiv_89
            uint64_t fp32Value = (eventMap[75] >= eventMap[76]) ? eventMap[75] - eventMap[76] : 0;  // aiv_75/76
            std::vector<uint64_t> eventResVecAiv = {eventMap[8], eventMap[1], eventMap[78],      // aiv_8/1/78
                fp32Value, fp16Value, eventMap[186], eventMap[79], eventMap[184], // aiv_75/76/186/79/184
                eventMap[185], allBlock, eventMap[100], eventMap[101], eventMap[102],           // aiv_185/100/101/102
                eventMap[103], wait};                                   // aiv_103
            for (size_t j = 0; j < basicPmuPtr->detailInfoVecAiv_.size(); ++j) {
                GenBlockDetail(detail[i], eventResVecAiv[j], basicPmuPtr->detailInfoVecAiv_[j], jsonMap);
            }
        }
    }
}

void ComputeLoadCalculator::ComputeLoadInfoFor310P(std::map<uint8_t, std::vector<nlohmann::json>> &jsonMap)
{
    std::shared_ptr<BasicPmu> basicPmuPtr = dataCenter_.GetDbPtr<BasicPmu>();
    if (basicPmuPtr == nullptr) {
        Utility::LogDebug("Failed to get basic pmu.");
        return;
    }
    const std::vector<ComputeLoadBlockDetail>& detail = basicPmuPtr->GetComputeLoadBlockDetail();
    // detailInfoVecAic记录了aic下需要记录的详细信息，分别包括计算负载的名称，数据类型和存储的目的json
    // 其存储的值由于来自不同event id且存在部分结果需要计算得出，因此定义eventResVecAic记录detailInfoVecAic中对应负载的值
    // eventResVecAic与detailInfoVecAic的值一一对应
    for (size_t i = 0; i < detail.size(); ++i) {  // 保存了某个block上的所有数据
        std::map<uint64_t, uint64_t> eventMap = detail[i].eventMap;
        // 生成cube数据
        std::vector<uint64_t> eventResVecAic = {eventMap[73] + eventMap[74], eventMap[73], eventMap[74],  // aic_3
            eventMap[88], eventMap[3], EMPTY_PMU_VALUE, EMPTY_PMU_VALUE};       // aic_88/3/1032/1033
        for (size_t j = 0; j < basicPmuPtr->detailInfoVecAic_.size(); ++j) {
            GenBlockDetail(detail[i], eventResVecAic[j], basicPmuPtr->detailInfoVecAic_[j], jsonMap);
        }
        // 生成vec数据
        uint64_t fp16Value = eventMap[76] + eventMap[77] + eventMap[174];                  // aiv_76+aiv_77+aiv_174
        uint64_t allBlock = eventMap[100] + eventMap[101] + eventMap[102] + eventMap[103]; // aiv(100+101+102+103)
        uint64_t wait = (detail[i].freq <= 0) ? 0 : static_cast<uint64_t>(
            (eventMap[89] / static_cast<float >(detail[i].freq))); // aiv_89
        uint64_t fp32Value = (eventMap[75] >= eventMap[76]) ? eventMap[75] - eventMap[76] : 0;  // aiv_75/76
        std::vector<uint64_t> eventResVecAiv = {eventMap[8], eventMap[1], eventMap[78],      // aiv_8/1/78
            fp32Value, fp16Value, eventMap[186], eventMap[79], eventMap[184], // aiv_75/76/186/79/184
            eventMap[185], allBlock, eventMap[100], eventMap[101], eventMap[102],               // aiv_185/100/101/102
            eventMap[103], wait};                                       // aiv_103
        for (size_t j = 0; j < basicPmuPtr->detailInfoVecAiv_.size(); ++j) {
            GenBlockDetail(detail[i], eventResVecAiv[j], basicPmuPtr->detailInfoVecAiv_[j], jsonMap);
        }
    }
}

void ComputeLoadCalculator::ComputeLoadInfoFor91095(std::map<uint8_t, std::vector<nlohmann::json>> &jsonMap)
{
    std::shared_ptr<BasicPmu> basicPmuPtr = dataCenter_.GetDbPtr<BasicPmu>();
    if (basicPmuPtr == nullptr) {
        Utility::LogDebug("Failed to get basic pmu.");
        return;
    }
    const std::vector<ComputeLoadBlockDetail>& details = basicPmuPtr->GetComputeLoadBlockDetail();
    for (const auto &detail : details) {
        std::map<uint64_t, uint64_t> eventMap = detail.eventMap;
        if (detail.blockType.find("cube") != std::string::npos) {
            // 生成cube数据
            uint64_t wait = (detail.freq <= 0 || eventMap[11] == UINT64_MAX) ? 0 : static_cast<uint64_t> (eventMap[11] / detail.freq);
            std::vector<uint64_t> eventResVecAic = {eventMap[810], eventMap[808], eventMap[809], eventMap[789], eventMap[790], eventMap[768], wait};
            for (size_t i = 0; i < detailInfoAicA5_.size(); ++i) {
                GenBlockDetail(detail, eventResVecAic[i], detailInfoAicA5_[i], jsonMap);
            }
            continue;
        }
        // 生成vec数据
        // 仅展示指令条数不为0的类型, simdMap和simtMap数据结构相同
        uint64_t totalInstrNum = 0;
        auto simdMap = detail.operandRecordMap.simdMap;
        auto simtMap = detail.operandRecordMap.simtMap;
        for (const auto &simd: simdMap) {
            auto simtInstr = simtMap.at(simd.first).instructions;
            if (simd.second.instructions == 0 && simtInstr == 0) {
                continue;
            }
            uint64_t instr = SafeAdd(simd.second.instructions, simtInstr, "vector instr");
            totalInstrNum = SafeAdd(totalInstrNum, instr, "vector total instr");
            Visualize::DetailInfo info{"Vector " + OperandTypeStrMap.at(simd.first), UnitType::INSTR, 1};
            GenBlockDetail(detail, instr, info, jsonMap);
        }
        Visualize::DetailInfo info{"Vector All Active", UnitType::INSTR, 1};
        GenBlockDetail(detail, totalInstrNum, info, jsonMap);

        uint64_t wait = (detail.freq <= 0 || eventMap[12] == UINT64_MAX) ?
            0 : static_cast<uint64_t>(eventMap[12] / detail.freq);
        std::vector<uint64_t> eventResVecAiv = {eventMap[1281], wait};
        for (size_t i = 0; i < detailInfoAivA5_.size(); ++i) {
            GenBlockDetail(detail, eventResVecAiv[i], detailInfoAivA5_[i], jsonMap);
        }
    }
}

void ComputeLoadCalculator::GenBlockDetail(const ComputeLoadBlockDetail &computeLoadBlockDetail, const uint64_t &value,
    const DetailInfo &detailInfo, std::map<uint8_t, std::vector<nlohmann::json>> &jsonMap) const
{
    nlohmann::json js;
    uint64_t cyc = computeLoadBlockDetail.totalCycles;
    if (detailInfo.type == UnitType::PER) {
        auto v = static_cast<float>(value);
        js["origin_value"] = v;
        v /= ((cyc != 0) ? static_cast<float>(cyc) : 1.0f);
        js["value"] = (value == EMPTY_PMU_VALUE) ? 0.0f : v * 100.0f;
    } else {
        js["value"] = (value == EMPTY_PMU_VALUE) ? "NA" : std::to_string(value);
    }

    js["block_id"] = (computeLoadBlockDetail.opType == "AiCore") ? "NA" :
        std::to_string(computeLoadBlockDetail.blockId);
    js["block_type"] = (computeLoadBlockDetail.opType == "mix") ? computeLoadBlockDetail.blockType :
        computeLoadBlockDetail.opType;
    std::string computeLoadName = detailInfo.name;   // 对于mix算子，两个vectorCore上面的Vector计算负载通过在名称之后加0/1
    if (computeLoadBlockDetail.opType == "mix" && detailInfo.name == "Vector All Active" &&
        detailInfo.type == UnitType::PER) {
        computeLoadName += std::to_string(VEC_INDEX);
        VEC_INDEX = (VEC_INDEX == 1) ? 0 : VEC_INDEX + 1;
    }
    js["name"] = computeLoadName;
    js["unit"] = detailInfo.type;
    jsonMap[detailInfo.jsonType].emplace_back(js);
}
}
}