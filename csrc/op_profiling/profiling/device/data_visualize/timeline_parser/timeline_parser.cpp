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

#include "timeline_parser.h"
#include "common/visualize.h"

using namespace Utility;
using namespace std;

namespace Visualize {

uint16_t GetAicoreBlockIndexByType(const std::string &opType, const std::string &recordType, uint16_t subBlockNum, uint16_t blockIndex, uint16_t subBlockIndex) {
    if (opType == Common::OpType::VECTOR || opType == Common::OpType::CUBE) {
        return blockIndex;
    }
    if (recordType == AIC_BLOCK) {
        return blockIndex;
    }
     // for mix operator, index 0 is cube subblock, index 1/2 is vector subblock
    if (subBlockIndex == 0) {
        return blockIndex;
    }
    int vecNum = subBlockNum - 1; // cube subblock number is 1
    return blockIndex * vecNum + subBlockIndex - 1;
}

float TimelineParser::GetRunTime(uint64_t freq, int64_t cycles)
{
    if (cycles < 0 || freq == 0) {
        LogDebug("There is some value wrong when calculating time");
        return 0;
    }
    return static_cast<float>(cycles) / freq * TIME_CONVERSION;
}

void TimelineParser::AddAicoreDuration(uint64_t startTime)
{
    if (aicpuFreq_ == 0) {
        LogDebug("Faile to calculate ai_core duration.");
        return;
    }
    float duration =  opBasicInfoObj_->GetDuration();
    JsonEvent event = {
        "AI_CORE", string(VISUALIZE_COLOR_NAME::GRASS_GREEN), "X", "AI CORE", "AI_CORE",
        static_cast<float>(SafeSub(startTime, minSysCyc_, "Aicore Dur", false)) / aicpuFreq_ * TIME_CONVERSION,
        duration
    };
    nlohmann::json jsonData;
    event.ToJson(jsonData);
    timelineJson_.emplace_back(jsonData);
}

void TimelineParser::ProcessAicoreBlockDur(bool enableTimeDetail)
{
    // 默认按照cube类型初始化，1个core里面只有1个子cube core
    subCoreCubeNum_ = 1;
    subCoreVecNum_ = 0;
    std::set<uint16_t> aicDotBlockIds;
    std::set<uint16_t> aivDotBlockIds;
    std::string opType = opBasicInfoObj_->GetOpType();
    std::string recordType = AIV_BLOCK;
    std::string location = "AICore Dur";
    std::string cName = std::string(VISUALIZE_COLOR_NAME::GREEN);
    if (aicpuFreq_ == 0) {
        return;
    }
    if (opType == Common::OpType::CUBE) {
        cName = std::string(VISUALIZE_COLOR_NAME::GRASS_GREEN);
        recordType = AIC_BLOCK;
    }
    for (const auto &pair : blockSystemTimes_) {
        uint16_t blockIndex = pair.first;
        auto timeVec = pair.second;
        uint16_t subBlockNum = timeVec.size();
        if (opType == Common::OpType::MIX) {
            subCoreVecNum_ = subBlockNum - 1;
        }
        if (opType == Common::OpType::VECTOR) {
            subCoreCubeNum_ = 0;
            subCoreVecNum_ = subBlockNum;
        }
        for (uint16_t i = 0; i < subBlockNum; i++) {
            json resultItem;
            if (opType == Common::OpType::MIX) {
                // blockSystemTimes_中记录的第1个是cube
                cName = i == 0 ? std::string(VISUALIZE_COLOR_NAME::GRASS_GREEN) : std::string(VISUALIZE_COLOR_NAME::GREEN);
                recordType = i == 0 ? AIC_BLOCK : AIV_BLOCK;
            }
            auto dots = GetAicoreBlockIndexByType(opType, recordType, subBlockNum, blockIndex, i);
            if (recordType == AIC_BLOCK) {
                aicDotBlockIds.insert(dots);
            } else {
                aivDotBlockIds.insert(dots);
            }
            blockDuration_[{recordType, dots}] = timeVec[i];
            resultItem["cname"] = cName;
            resultItem["ph"] = "X";
            auto [core, name] = GetGroupName(recordType, dots);
            resultItem["pid"] = BLOCK;
            resultItem["tid"] = core;
            if (enableTimeDetail) {
                resultItem["name"] = name;
                resultItem["ts"] = GetRunTime(aicpuFreq_, SafeSub(timeVec[i].first, minSysCyc_, location, false));
                resultItem["dur"] = GetRunTime(aicpuFreq_, SafeSub(timeVec[i].second, timeVec[i].first, location, false));
                timelineJson_.push_back(resultItem);
            } else {
                resultItem["dur"] = 0;
                resultItem["name"] = recordType + std::to_string(dots) + " START";
                resultItem["ts"] = GetRunTime(aicpuFreq_, SafeSub(timeVec[i].first, minSysCyc_, location, false));
                timelineJson_.push_back(resultItem);
                resultItem["name"] = recordType + std::to_string(dots) + " END";
                resultItem["ts"] = GetRunTime(aicpuFreq_, SafeSub(timeVec[i].second, minSysCyc_, location, false));
                timelineJson_.push_back(resultItem);
            }
        }
    }
    SortTimelineByIds(blockSystemTimes_.size());
}

void TimelineParser::SortTimelineByIds(uint32_t coreNums) {
    int order = 1;
    for (uint16_t coreIdx = 0; coreIdx < coreNums; coreIdx++) {
        std::string pid = std::string("core") + std::to_string(coreIdx);
        for (uint16_t c = 0; c < subCoreCubeNum_; c++) {
            std::string tid = std::string("cubecore0");
            json nameItem;
            nameItem["ph"] = "M";
            nameItem["name"] = "thread_sort_index";
            nameItem["pid"] = BLOCK;
            nameItem["tid"] = "core" + std::to_string(coreIdx) + "." + tid;
            nameItem["args"]["sort_index"] = order++;
            timelineJson_.push_back(nameItem);
        }
        for (uint16_t v = 0; v < subCoreVecNum_; v++) {
            std::string tid = std::string("veccore") + std::to_string(v);
            json nameItem;
            nameItem["ph"] = "M";
            nameItem["name"] = "thread_sort_index";
            nameItem["pid"] = BLOCK;
            nameItem["tid"] = "core" + std::to_string(coreIdx) + "." + tid;
            nameItem["args"]["sort_index"] = order++;
            timelineJson_.push_back(nameItem);
        }
    }
}
}
