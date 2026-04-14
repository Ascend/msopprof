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

void TimelineParser::ProcessAicoreBlockDur()
{
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
            resultItem["dur"] = GetRunTime(aicpuFreq_, SafeSub(timeVec[i].second, timeVec[i].first, location, false));
            resultItem["name"] = recordType + std::to_string(dots);
            resultItem["ph"] = "X";
            resultItem["pid"] = recordType;
            resultItem["tid"] = dots;
            resultItem["ts"] = GetRunTime(aicpuFreq_, SafeSub(timeVec[i].first, minSysCyc_, location, false));
            timelineJson_.push_back(resultItem);

        }
    }
    SortTimelineByIds(aicDotBlockIds, aivDotBlockIds);
}

void TimelineParser::SortTimelineByIds(const std::set<uint16_t> &aicDotBlockIds, const std::set<uint16_t> &aivDotBlockIds)
{
    // sort aicore timeline
    auto OrderDotsId = [this] (const std::string &type, const std::set<uint16_t> &dots) {
        for (const auto &dotBlockId: dots) {
                json nameItem;
                nameItem["ph"] = "M";
                nameItem["name"] = "thread_name";
                nameItem["pid"] = type;
                nameItem["tid"] = dotBlockId;
                nameItem["args"]["name"] = type + std::to_string(dotBlockId);
                timelineJson_.push_back(nameItem);
                json sortItem;
                sortItem["ph"] = "M";
                sortItem["name"] = "thread_sort_index";
                sortItem["pid"] = type;
                sortItem["tid"] = dotBlockId;
                sortItem["args"]["sort_index"] = dotBlockId;
                timelineJson_.push_back(sortItem);
        }
    };
    OrderDotsId(AIC_BLOCK, aicDotBlockIds);
    OrderDotsId(AIV_BLOCK, aivDotBlockIds);
}
}