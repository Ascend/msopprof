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

#include "data_bean.h"
#include "securec.h"

using namespace std;
namespace Profiling {

void GetStreamAndTaskId(uint16_t &streamId, uint16_t &taskId)
{
    // when the 13th bit of the stream id is set, the task id need to be exchanged.
    // when the 14th bit of the stream id is set, the lower 12 bits of the stream id and task id need to be exchanged.
    uint16_t hardwareStreamId = streamId;
    uint16_t hardwareTaskId = taskId;
    constexpr uint16_t streamIdOffset = 11;
    if ((hardwareStreamId & 0x1000) != 0) {
        taskId = (hardwareTaskId & 0x1FFF) | (hardwareStreamId & 0xE000);
        streamId = hardwareStreamId % (1 << streamIdOffset);
    } else if ((hardwareStreamId & 0x2000) != 0) {
        taskId = (hardwareTaskId & 0xF000) | (hardwareStreamId & 0x0FFF);
        streamId = hardwareTaskId & 0x0FFF;
    } else {
        taskId = hardwareTaskId;
        streamId = hardwareStreamId % (1 << streamIdOffset);
    }
}

FftsBlockBean::FftsBlockBean(const Common::ChipProductType &chip, const vector<char> &bin)
{
    if (IsChipSeriesTypeValid(chip, Common::ChipProductType::ASCEND950_SERIES)) {
        InitA5(bin);
    } else {
        InitA2(bin);
    }
}

void FftsBlockBean::InitA5(const vector<char> &bin)
{
    if (memcpy_s(&fftsConstructA5_, sizeof(fftsConstructA5_), &bin[0], bin.size()) != EOK) {
        return;
    }
    constexpr uint16_t blockIdIndex = 1;
    constexpr uint16_t subBlockIdIndex = 0;
    constexpr uint16_t typeIndex = 2;
    constexpr uint16_t coreIdOffset = 8;
    constexpr uint16_t coreIdAndOperation = 0xff;
    constexpr uint16_t coreTypeAndOperation = 1;
    constexpr uint16_t totalCyclesIndex = 0;
    constexpr uint16_t pmuBeginIndex = 0;
    constexpr uint16_t pmuTotalNum = 10;
    constexpr uint16_t startTimeIndex = 10;
    constexpr uint16_t endTimeIndex = 11;
    constexpr uint16_t funcTypeMask = 0x3f;
    constexpr uint16_t funcTypeIndex = 0;
    constexpr uint16_t streamIdIndex = 2;
    constexpr uint16_t taskIdIndex = 3;
    fftsBlockData_.funcType = fftsConstructA5_.shortNums1[funcTypeIndex] & funcTypeMask;
    fftsBlockData_.blockId = fftsConstructA5_.shortNums3[blockIdIndex];
    fftsBlockData_.subBlockId = fftsConstructA5_.shortNums3[subBlockIdIndex];
    fftsBlockData_.coreId = (fftsConstructA5_.shortNums2[typeIndex] >> coreIdOffset) & coreIdAndOperation;
    fftsBlockData_.coreType = fftsConstructA5_.shortNums2[typeIndex] & coreTypeAndOperation;
    fftsBlockData_.totalCycles = fftsConstructA5_.longlongNums1[totalCyclesIndex];
    fftsBlockData_.streamId = fftsConstructA5_.shortNums1[streamIdIndex];
    fftsBlockData_.taskId = fftsConstructA5_.shortNums1[taskIdIndex];
    GetStreamAndTaskId(fftsBlockData_.streamId, fftsBlockData_.taskId);
    for (uint16_t i = pmuBeginIndex; i < pmuBeginIndex + pmuTotalNum; i++) {
        fftsBlockData_.pmuValues.emplace_back(fftsConstructA5_.longlongNums2[i]);
    }
    fftsBlockData_.startSystemTime = fftsConstructA5_.longlongNums2[startTimeIndex];
    fftsBlockData_.endSystemTime = fftsConstructA5_.longlongNums2[endTimeIndex];
}

void FftsBlockBean::InitA2(const vector<char> &bin)
{
    if (memcpy_s(&fftsConstruct_, sizeof(fftsConstruct_), &bin[0], bin.size()) != EOK) {
        return;
    }
    constexpr uint16_t blockIdIndex = 1;
    constexpr uint16_t subBlockIdIndex = 0;
    constexpr uint16_t typeIndex = 2;
    constexpr uint16_t coreIdOffset = 1;
    constexpr uint16_t coreIdAndOperation = 0x7f;
    constexpr uint16_t coreTypeAndOperation = 1;
    constexpr uint16_t totalCyclesIndex = 0;
    constexpr uint16_t pmuBeginIndex = 2;
    constexpr uint16_t pmuTotalNum = 8;
    constexpr uint16_t startTimeIndex = 10;
    constexpr uint16_t endTimeIndex = 11;
    constexpr uint16_t streamIdIndex = 2;
    constexpr uint16_t taskIdIndex = 3;
    fftsBlockData_.blockId = fftsConstruct_.shortNums3[blockIdIndex];
    fftsBlockData_.subBlockId = fftsConstruct_.shortNums3[subBlockIdIndex];
    fftsBlockData_.coreId = (fftsConstruct_.shortNums2[typeIndex] >> coreIdOffset) & coreIdAndOperation;
    fftsBlockData_.coreType = fftsConstruct_.shortNums2[typeIndex] & coreTypeAndOperation;
    fftsBlockData_.streamId = fftsConstruct_.shortNums1[streamIdIndex];
    fftsBlockData_.taskId = fftsConstruct_.shortNums1[taskIdIndex];
    GetStreamAndTaskId(fftsBlockData_.streamId, fftsBlockData_.taskId);
    fftsBlockData_.totalCycles = fftsConstruct_.longlongNums2[totalCyclesIndex];
    for (uint16_t i = pmuBeginIndex; i < pmuBeginIndex + pmuTotalNum; i++) {
        fftsBlockData_.pmuValues.emplace_back(fftsConstruct_.longlongNums2[i]);
    }
    fftsBlockData_.startSystemTime = fftsConstruct_.longlongNums2[startTimeIndex];
    fftsBlockData_.endSystemTime = fftsConstruct_.longlongNums2[endTimeIndex];
}

SplitBlockPmuData FftsBlockBean::GetBlockData(const vector<uint16_t> &aicEvents,
                                              const vector<uint16_t> &aivEvents) const
{
    PmuEventValueMapType pmuEventValueMap = {};
    vector<uint64_t> pmuValues = GetPmuValues();
    string blockType = Common::OpType::VECTOR;
    vector<uint16_t> events = aivEvents;
    if (GetCoreType() == 0) {
        events = aicEvents;
        blockType = Common::OpType::CUBE;
    }
    for (size_t i = 0; i < events.size(); i++) {
        pmuEventValueMap[events[i]] = pmuValues[i];
    }
    return {pmuEventValueMap, blockType, GetTotalCycles(), GetBlockId(), blockType + to_string(GetSubBlockId()),
            GetCoreId(), GetStartSystemTime(), GetEndSystemTime()};
}

Common::TimeType CommonGetTimeType(uint16_t funcType)
{
    if (funcType == 0) {
        return Common::TimeType::START;
    } else if (funcType == 1) {
        return Common::TimeType::END;
    }
    return Common::TimeType::OTHERS;
}

AcsqBean::AcsqBean(const Common::ChipProductType &chip, const vector<char> &bin)
{
    constexpr uint16_t typeIndex = 0;
    constexpr uint16_t streamIdIndex = 2;
    constexpr uint16_t taskIdIndex = 3;
    constexpr uint16_t taskTypeOffset = 10;
    constexpr uint16_t funcTypeAndOperation = 63;
    constexpr uint16_t systemTimeIndex = 0;
    if (IsChipSeriesTypeValid(chip, Common::ChipProductType::ASCEND950_SERIES)) {
        if (memcpy_s(&acsqA5Construct_, sizeof(acsqA5Construct_), &bin[0], bin.size()) != EOK) {
            return;
        }
        acsqData_.taskType = acsqA5Construct_.shortNums1[typeIndex] >> taskTypeOffset;
        acsqData_.funcType = acsqA5Construct_.shortNums1[typeIndex] & funcTypeAndOperation;
        acsqData_.systemTime = acsqA5Construct_.longlongNums[systemTimeIndex];
        acsqData_.streamId = acsqA5Construct_.shortNums1[streamIdIndex];
        acsqData_.taskId = acsqA5Construct_.shortNums1[taskIdIndex];
        GetStreamAndTaskId(acsqData_.streamId, acsqData_.taskId);
    } else {
        if (memcpy_s(&acsqConstruct_, sizeof(acsqConstruct_), &bin[0], bin.size()) != EOK) {
            return;
        }
        acsqData_.taskType = acsqConstruct_.shortNums1[typeIndex] >> taskTypeOffset;
        acsqData_.funcType = acsqConstruct_.shortNums1[typeIndex] & funcTypeAndOperation;
        acsqData_.systemTime = acsqConstruct_.longlongNums[systemTimeIndex];
        acsqData_.streamId = acsqConstruct_.shortNums1[streamIdIndex];
        acsqData_.taskId = acsqConstruct_.shortNums1[taskIdIndex];
        GetStreamAndTaskId(acsqData_.streamId, acsqData_.taskId);
    }
}

uint16_t AcsqBean::GetTaskType() const
{
    return acsqData_.taskType;
}

Common::TimeType AcsqBean::GetTimeType() const
{
    return CommonGetTimeType(acsqData_.funcType);
}

uint16_t AcsqBean::GetStreamId() const
{
    return acsqData_.streamId;
}

uint16_t AcsqBean::GetTaskId() const
{
    return acsqData_.taskId;
}

uint64_t AcsqBean::GetSystemTime() const
{
    return acsqData_.systemTime;
}

AiCoreBean::AiCoreBean(const vector<char> &bin)
{
    constexpr uint16_t totalCyclesIndex = 0;
    constexpr uint16_t pmuBeginIndex = 2;
    constexpr uint16_t pmuTotalNum = 8;
    if (memcpy_s(&aiCoreConstruct_, sizeof(aiCoreConstruct_),
                 &bin[0], bin.size()) != EOK) {
        return;
    }

    aiCoreData_.totalCycles = aiCoreConstruct_.longlongNums[totalCyclesIndex];
    for (uint16_t i = pmuBeginIndex; i < pmuBeginIndex + pmuTotalNum; i++) {
        aiCoreData_.pmuValues.emplace_back(aiCoreConstruct_.longlongNums[i]);
    }
}

SplitBlockPmuData AiCoreBean::GetAiCoreData(const vector<uint16_t> &events) const
{
    PmuEventValueMapType pmuEventValueMap = {};
    vector<uint64_t> pmuValues = GetPmuValues();
    for (size_t i = 0; i < events.size(); i++) {
        pmuEventValueMap[events[i]] = pmuValues[i];
    }
    return {pmuEventValueMap,
            Common::OpType::CUBE,
            GetTotalCycles()};
}

uint64_t AiCoreBean::GetTotalCycles() const
{
    return aiCoreData_.totalCycles;
}

vector<uint64_t> AiCoreBean::GetPmuValues() const
{
    return aiCoreData_.pmuValues;
}

L2CacheBean::L2CacheBean(const vector<char> &bin)
{
    constexpr uint16_t pmuBeginIndex = 0;
    constexpr uint16_t pmuTotalNum = 8;
    if (memcpy_s(&l2CacheConstruct_, sizeof(l2CacheConstruct_),
                 &bin[0], bin.size()) != EOK) {
        return;
    }

    for (uint16_t i = pmuBeginIndex; i < pmuBeginIndex + pmuTotalNum; i++) {
        l2CacheData_.pmuValues.emplace_back(l2CacheConstruct_.longlongNums[i]);
    }
}

SplitBlockPmuData L2CacheBean::GetL2CacheData(const vector<uint16_t> &events) const
{
    PmuEventValueMapType pmuEventValueMap = {};
    vector<uint64_t> pmuValues = GetPmuValues();
    for (size_t i = 0; i < events.size(); i++) {
        pmuEventValueMap[events[i]] = pmuValues[i];
    }
    return {pmuEventValueMap, Common::OpType::CUBE};
}

HwtsBean::HwtsBean(const vector<char> &bin)
{
    constexpr uint16_t typeIndex = 0;
    constexpr uint16_t funcTypeAndOperation = 7;
    constexpr uint16_t systemTimeIndex = 0;
    if (memcpy_s(&hwtsConstruct_, sizeof(hwtsConstruct_),
                 &bin[0], bin.size()) != EOK) {
        return;
    }
    hwtsData_.funcType = hwtsConstruct_.charNums[typeIndex] & funcTypeAndOperation;
    hwtsData_.systemTime = hwtsConstruct_.longlongNums[systemTimeIndex];
}

vector<uint64_t> L2CacheBean::GetPmuValues() const
{
    return l2CacheData_.pmuValues;
}

Common::TimeType HwtsBean::GetTimeType() const
{
    return CommonGetTimeType(hwtsData_.funcType);
}

uint64_t HwtsBean::GetSystemTime() const
{
    return hwtsData_.systemTime;
}
}