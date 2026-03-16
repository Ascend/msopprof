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


#include "ub_conflict_calculator.h"

#include <bitset>
#include <algorithm>

#include "number_operation.h"
#include "common/defs.h"
#include "instr_detail_utils.h"
#include "instr_in_camodel/camodel_dtype_defs.h"

namespace Profiling {
namespace Parse {

constexpr uint64_t VEC_BLOCK_SIZE = 32;
constexpr uint64_t VEC_BLOCK_NUM = 8;
constexpr float DEFAULT_CONFLICT = -1;

using VecParseFunc = std::function<void (const InstrVec &instrVec, InstrDetailEvent &event)>;

void ParseUnaryOp(const InstrVec &instrVec, InstrDetailEvent &event);
void ParseBinaryOp(const InstrVec &instrVec, InstrDetailEvent &event);
void ParseReduceOp(const InstrVec &instrVec, InstrDetailEvent &event);

const std::unordered_map<uint8_t, VecParseFunc> A2A3VecParseMap = {
    {static_cast<uint8_t>(VecInstrTemplate::VABS), ParseUnaryOp},
    {static_cast<uint8_t>(VecInstrTemplate::VADD), ParseBinaryOp},
    {static_cast<uint8_t>(VecInstrTemplate::VCADD), ParseReduceOp},
};


void ParseUnaryOp(const InstrVec &instrVec, InstrDetailEvent &event)
{
    BaseMemInfo dst {true, instrVec.xd, VEC_BLOCK_NUM, VEC_BLOCK_SIZE, 0, 0, 0};
    BaseMemInfo src {true, instrVec.xn, VEC_BLOCK_NUM, VEC_BLOCK_SIZE, 0, 0, 0};

    dst.blockStride = Utility::ExtractKBits(instrVec.xt, 0, BIT16);
    src.blockStride = Utility::ExtractKBits(instrVec.xt, BIT16, BIT16);
    if (instrVec.dType == InstrDataType::U8) {
        dst.repeatStride = Utility::ExtractKBits(instrVec.xt, BIT32, BIT8);
        src.repeatStride = Utility::ExtractKBits(instrVec.xt, BIT40, BIT8);
    } else {
        dst.repeatStride = Utility::ExtractKBits(instrVec.xt, BIT32, BIT8) |
            Utility::ExtractKBits(instrVec.xt, BIT52, BIT4);
        src.repeatStride = Utility::ExtractKBits(instrVec.xt, BIT40, BIT12);
    }
    event.isValid = true;
    event.repeat = static_cast<uint8_t>(Utility::ExtractKBits(instrVec.xt, BIT56, BIT8));
    dst.repeatTimes = event.repeat;
    src.repeatTimes = event.repeat;
    event.dType = instrVec.dType;
    event.srcAddrList.emplace_back(src);
    event.dstAddrList.emplace_back(dst);
}

void ParseBinaryOp(const InstrVec &instrVec, InstrDetailEvent &event)
{
    BaseMemInfo dst {true, instrVec.xd, VEC_BLOCK_NUM, VEC_BLOCK_SIZE, 0, 0, 0};
    BaseMemInfo src {true, instrVec.xn, VEC_BLOCK_NUM, VEC_BLOCK_SIZE, 0, 0, 0};
    BaseMemInfo src1 {true, instrVec.xm, VEC_BLOCK_NUM, VEC_BLOCK_SIZE, 0, 0, 0};

    dst.blockStride = Utility::ExtractKBits(instrVec.xt, 0, BIT8);
    src.blockStride = Utility::ExtractKBits(instrVec.xt, BIT8, BIT8);
    src1.blockStride = Utility::ExtractKBits(instrVec.xt, BIT16, BIT8);
    dst.repeatStride = Utility::ExtractKBits(instrVec.xt, BIT24, BIT8);
    src.repeatStride = Utility::ExtractKBits(instrVec.xt, BIT32, BIT8);
    src1.repeatStride = Utility::ExtractKBits(instrVec.xt, BIT40, BIT8);

    event.repeat = static_cast<uint8_t>(Utility::ExtractKBits(instrVec.xt, BIT56, BIT8));
    dst.repeatTimes = event.repeat;
    src.repeatTimes = event.repeat;
    src1.repeatTimes = event.repeat;
    event.dType = instrVec.dType;
    event.srcAddrList.emplace_back(src);
    event.srcAddrList.emplace_back(src1);
    event.dstAddrList.emplace_back(dst);
    event.isValid = true;
}

void ParseReduceOp(const InstrVec &instrVec, InstrDetailEvent &event)
{
    BaseMemInfo dst {true, instrVec.xd, VEC_BLOCK_NUM, VEC_BLOCK_SIZE, 0, 0, 0};
    BaseMemInfo src {true, instrVec.xn, VEC_BLOCK_NUM, VEC_BLOCK_SIZE, 0, 0, 0};

    dst.blockStride = 1;
    src.blockStride = Utility::ExtractKBits(instrVec.xt, BIT16, BIT16);
    dst.repeatStride = Utility::ExtractKBits(instrVec.xt, 0, BIT16);
    src.repeatStride = Utility::ExtractKBits(instrVec.xt, BIT32, BIT16);

    event.repeat = static_cast<uint8_t>(Utility::ExtractKBits(instrVec.xt, BIT56, BIT8));
    dst.repeatTimes = event.repeat;
    src.repeatTimes = event.repeat;
    event.dType = instrVec.dType;
    event.srcAddrList.emplace_back(src);
    event.dstAddrList.emplace_back(dst);
    event.isValid = true;
}

PluginErrorCode UbConflictCalculator::Entry()
{
    AttributeMapInit();
    std::shared_ptr<InstrDetailTable> instrDetailTable = dataCenter_.GetDbPtr<InstrDetailTable>();
    if (instrDetailTable == nullptr) {
        return PluginErrorCode::NONBLOCKING_ERROR;
    }

    InstrLogStr2Template mapping;
    ChipProductType seriesType = instrDetailConfig_.GetProductSeriesType();
    if (!GetInstrLogStr2TemplateMap(seriesType, mapping)) {
        Utility::LogDebug("Get instr name to template map failed.");
        return PluginErrorCode::NONBLOCKING_ERROR;
    }
    for (size_t i = 0; i < instrDetailTable->GetSize(); i++) {
        MergeInfo* mergeInfo = instrDetailTable->QueryColumnValue<MergeInfo>(InstrDetailTable::MERGE_INFO, i);
        if (mergeInfo == nullptr) {
            instrDetailTable->UpdateColumnValue(InstrDetailTable::UB_READ_CONFLICT, i, DEFAULT_CONFLICT);
            instrDetailTable->UpdateColumnValue(InstrDetailTable::UB_WRITE_CONFLICT, i, DEFAULT_CONFLICT);
            continue;
        }
        auto it = mapping.find(mergeInfo->name);
        if (it == mapping.end()) {
            instrDetailTable->UpdateColumnValue(InstrDetailTable::UB_READ_CONFLICT, i, DEFAULT_CONFLICT);
            instrDetailTable->UpdateColumnValue(InstrDetailTable::UB_WRITE_CONFLICT, i, DEFAULT_CONFLICT);
            continue;
        }
        InstrDetailEvent event;
        if (IsChipSeriesTypeValid(seriesType, ChipProductType::ASCEND310P_SERIES)) {
            // 310P ca model
            Get310PVecEvent(*mergeInfo, event);
        } else {
            GetA2A3VecEvent(*mergeInfo, it->second, event);
        }
        std::pair<int, int> conflictPair = ConflictCalculator(event);
        instrDetailTable->UpdateColumnValue(InstrDetailTable::UB_READ_CONFLICT, i, conflictPair.first);
        instrDetailTable->UpdateColumnValue(InstrDetailTable::UB_WRITE_CONFLICT, i, conflictPair.second);
    }
    return PluginErrorCode::SUCCESS;
}

std::pair<int, int> UbConflictCalculator::ConflictCalculator(const InstrDetailEvent &instrDetailEvent) const
{
    constexpr uint32_t bankGroupStartBit = 5;
    constexpr uint32_t bankGroupBitLen = 4;
    constexpr uint32_t bankIdStartBit = 16;
    constexpr uint32_t bankIdBitLen = 2;
    if (!instrDetailEvent.isValid) {
        return {DEFAULT_CONFLICT, DEFAULT_CONFLICT};
    }
    std::vector<uint64_t> srcBankGroupList;
    std::set<uint64_t> srcBankIdSet;
    std::vector<uint64_t> dstBankGroupList;
    std::set<uint64_t> dstBankIdSet;
    for (const auto &srcAddr : instrDetailEvent.srcAddrList) {
        for (uint64_t i = 0; i < srcAddr.blockNum; i++) {
            uint64_t blockAddr = i * srcAddr.blockStride * srcAddr.blockSize + srcAddr.addr;
            srcBankGroupList.emplace_back(Utility::ExtractKBits(blockAddr, bankGroupStartBit, bankGroupBitLen));
            srcBankIdSet.insert(Utility::ExtractKBits(blockAddr, bankIdStartBit, bankIdBitLen));
        }
    }
    for (const auto &dstAddr : instrDetailEvent.dstAddrList) {
        for (uint64_t i = 0; i < dstAddr.blockNum; i++) {
            uint64_t blockAddr = i * dstAddr.blockStride * dstAddr.blockSize + dstAddr.addr;
            dstBankGroupList.emplace_back(Utility::ExtractKBits(blockAddr, bankGroupStartBit, bankGroupBitLen));
            dstBankIdSet.insert(Utility::ExtractKBits(blockAddr, bankIdStartBit, bankIdBitLen));
        }
    }
    // read 2 read conflict
    int readConflict = FindSameTimesMax(srcBankGroupList);
    // write 2 write conflict
    int writeConflict = FindSameTimesMax(dstBankGroupList);
    return {readConflict, writeConflict};
}

int UbConflictCalculator::FindSameTimesMax(const std::vector<uint64_t> &idList) const
{
    int conflictCount = 0;
    std::unordered_map<uint64_t, int> elementCountMap;
    for (uint64_t id : idList) {
        auto it = elementCountMap.find(id);
        if (it == elementCountMap.end()) {
            elementCountMap[id] = 1;
            continue;
        }
        ++elementCountMap[id];
        conflictCount = std::max(conflictCount, elementCountMap[id] - 1);
    }
    return conflictCount;
}

void UbConflictCalculator::Get310PVecEvent(const MergeInfo &mergeInfo, InstrDetailEvent &event)
{
    BaseMemInfo tmpMemInfo {};
    tmpMemInfo.isValid = true;
    tmpMemInfo.blockNum = VEC_BLOCK_NUM;
    tmpMemInfo.blockSize = VEC_BLOCK_SIZE;

    if (!GetRegValue(regDetailRegexMap_.at(RegNameKey::VEC_REPEAT),
        mergeInfo.detail, tmpMemInfo.repeatTimes, Utility::RADIX_10)) {
        event.isValid = false;
        return;
    }

    if (!GetRegValue(regDetailRegexMap_.at(RegNameKey::VEC_DST_ADDR), mergeInfo.detail, tmpMemInfo.addr) ||
        !GetRegValueAuto(regDetailRegexMap_.at(RegNameKey::VEC_DST_STRIDE), mergeInfo.detail, tmpMemInfo.blockStride) ||
        !GetRegValueAuto(regDetailRegexMap_.at(RegNameKey::VEC_DST_REP_STRIDE),
            mergeInfo.detail, tmpMemInfo.repeatStride)) {
        event.isValid = false;
        return;
    }

    event.dstAddrList.push_back(tmpMemInfo);
    if (!GetRegValue(regDetailRegexMap_.at(RegNameKey::VEC_SRC_ADDR), mergeInfo.detail, tmpMemInfo.addr) ||
        !GetRegValueAuto(regDetailRegexMap_.at(RegNameKey::VEC_SRC_STRIDE), mergeInfo.detail, tmpMemInfo.blockStride) ||
        !GetRegValueAuto(regDetailRegexMap_.at(RegNameKey::VEC_SRC_REP_STRIDE),
            mergeInfo.detail, tmpMemInfo.repeatStride)) {
        event.isValid = false;
        return;
    }
    event.srcAddrList.push_back(tmpMemInfo);

    if (GetRegValue(regDetailRegexMap_.at(RegNameKey::VEC_SRC1_ADDR), mergeInfo.detail, tmpMemInfo.addr) &&
        GetRegValueAuto(regDetailRegexMap_.at(RegNameKey::VEC_SRC1_STRIDE), mergeInfo.detail, tmpMemInfo.blockStride) &&
        GetRegValueAuto(regDetailRegexMap_.at(RegNameKey::VEC_SRC1_REP_STRIDE),
            mergeInfo.detail, tmpMemInfo.repeatStride)) {
        // some vec does not contain src1
        event.srcAddrList.push_back(tmpMemInfo);
    }
    uint64_t dtypeValue = static_cast<uint64_t>(VectorDtype310P::VEC_NA);
    if (GetRegValue(regDetailRegexMap_.at(RegNameKey::VEC_DTYPE), mergeInfo.detail, dtypeValue, Utility::RADIX_10) &&
        dtypeValue < static_cast<uint64_t>(VectorDtype310P::VEC_NA)) {
        event.dType = VEC_DTYPE_BYTES_310P_MAP.at(static_cast<VectorDtype310P>(dtypeValue));
    }
    event.isValid = true;
}

void UbConflictCalculator::GetA2A3VecEvent(const MergeInfo &mergeInfo,
                                           VecInstrTemplate vecTemplate, InstrDetailEvent &event)
{
    InstrRegDetail instrRegDetail {};
    // non-strict extract, extract if have
    GetRegValue(regDetailRegexMap_.at(RegNameKey::XD_VALUE), mergeInfo.detail, instrRegDetail.instrVec.xd);
    GetRegValue(regDetailRegexMap_.at(RegNameKey::XN_VALUE), mergeInfo.detail, instrRegDetail.instrVec.xn);
    GetRegValue(regDetailRegexMap_.at(RegNameKey::XM_VALUE), mergeInfo.detail, instrRegDetail.instrVec.xm);
    GetRegValue(regDetailRegexMap_.at(RegNameKey::XT_VALUE), mergeInfo.detail, instrRegDetail.instrVec.xt);
    std::string dType;
    if (!GetRegString(regDetailRegexMap_.at(RegNameKey::DTYPE), mergeInfo.detail, dType) ||
        VEC_DTYPE_BYTES_A2A3_MAP.find(dType) == VEC_DTYPE_BYTES_A2A3_MAP.end()) {
        event.isValid = false;
        return;
    }
    instrRegDetail.instrVec.dType = VEC_DTYPE_BYTES_A2A3_MAP.at(dType);

    auto it = A2A3VecParseMap.find(static_cast<uint8_t>(vecTemplate));
    if (it == A2A3VecParseMap.end()) {
        event.isValid = false;
        return;
    }
    it->second(instrRegDetail.instrVec, event);
}

}
}
