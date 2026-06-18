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

#include "lccl_timeline_parser.h"

#include "filesystem.h"
#include "log.h"
#include "number_operation.h"
#include "ustring.h"
#include "common/hal_helper.h"
#include "common/visualize.h"
#include "mc2_timeline_parser.h"

using namespace Utility;
using namespace std;

namespace Visualize {

struct LcalApi {
    static constexpr char const *OVERALL = "OVERALL";
    static constexpr char const *INIT = "INIT";
    static constexpr char const *PROCESS = "PROCESS";
};

const map<string, string> API_CNAME_MAP = {
    {string(LcalApi::OVERALL),      string(VISUALIZE_COLOR_NAME::PINK)},
    {string(LcalApi::INIT),         string(VISUALIZE_COLOR_NAME::YELLOW)},
    {string(LcalApi::PROCESS),      string(VISUALIZE_COLOR_NAME::PURPLE)},
};

const map<uint32_t, string> LOG_ID_MAP = {
    {0, string(LcalApi::OVERALL)},
    {1, string(LcalApi::INIT)},
    {2, string(LcalApi::PROCESS)},
};

nlohmann::json LcclTimelineParser::AddAicoreApiDot(const LcclInfo& info) const
{
    string location = "Lccl Api";
    JsonEvent event;
    // for safe, name and cname key already in map
    event.name = LOG_ID_MAP.at(info.logId);
    event.cName = API_CNAME_MAP.at(event.name);
    event.ph = "X";
    event.pid = GetAicoreTimeLinePid(info.blockId);
    event.tid = to_string(info.blockId);
    event.ts = static_cast<float>(SafeSub(info.startSyscyc, minSysCyc_, location, false)) / aicpuFreq_
               * TIME_CONVERSION;
    event.dur = static_cast<float>(SafeSub(info.endSyscyc, info.startSyscyc, location, false)) / aicpuFreq_
                * TIME_CONVERSION;
    std::stringstream ss;
    ss << std::hex << info.curPc;
    event.args = {
        {"curPc", "0x" + ss.str()},
        {"operationType", to_string(info.operationType)},
        {"rsv", to_string(info.rsv)},
    };
    nlohmann::json resultItem;
    event.ToJson(resultItem);
    return resultItem;
}

void LcclTimelineParser::ProcessAicoreData(const vector<LcclDumpLogInfo> &aicoreTimeStamps)
{
    // add aicore block duration
    ProcessAicoreBlockDur();
    AddAicoreDuration(aicoreStartCyc_);
    set<uint16_t> dotBlockIds;

    // add aicore api dot
    map<pair<uint32_t, uint32_t>, LcclInfo> operationMap;
    for (const auto& item: aicoreTimeStamps) {
        dotBlockIds.insert(item.blockId);
        pair<uint32_t, uint32_t> logBlockPair = {item.logId, item.blockId};
        auto it = operationMap.find(logBlockPair);
        if (it == operationMap.end()) {
            operationMap[logBlockPair] = {item.logId, item.blockId, item.operationType, item.rsv, item.syscyc, 0,
                                          NumToHexString(item.curPc)};
        } else {
            if (item.syscyc < it->second.startSyscyc) {
                LogDebug("Lccl data not regular, logId: %u, blockId: %u, syscyc: %llu, curPc: %llu, operationType: %u, "
                    "rsv: %u.", item.logId, item.blockId, item.syscyc, item.curPc, item.operationType, item.rsv);
                continue;
            }
            LcclInfo& info = it->second;
            std::string pid = GetAicoreTimeLinePid(info.blockId);
            if (blockDuration_[{pid, info.blockId}].first > info.startSyscyc || blockDuration_[{pid, info.blockId}].second < info.endSyscyc) {
                continue;
            }
            it->second.endSyscyc = item.syscyc;
            nlohmann::json resultItem = AddAicoreApiDot(it->second);
            timelineJson_.push_back(resultItem);
            operationMap.erase(logBlockPair);
        }
    }
    for (const auto& iter: operationMap) {
        LogDebug("Lccl data not match, logId: %u, blockId: %u, syscyc: %llu, curPc: %s, operationType: %u, "
            "rsv: %u.", iter.second.logId, iter.second.blockId, iter.second.startSyscyc, iter.second.curPc.c_str(),
            iter.second.operationType, iter.second.rsv);
    }
}

bool LcclTimelineParser::TimelineToJson(const string &outputPath)
{
    ChipProductType chipType = GetChipTypeSeries();
    if (chipType != ChipProductType::ASCEND910B_SERIES && chipType != ChipProductType::ASCEND910_93_SERIES) {
        return false;
    }

    outputPath_ = outputPath;
    vector<LcclDumpLogInfo> aicoreTimeStamps;
    // get aicore dot timestamp from aic_timestamp.bin
    if (!GetTimeStamp(aicoreTimeStamps)) {
        LogDebug("Failed to parser data from aic_timestamp.bin");
        return false;
    }
    if (aicpuFreq_ == 0) {
        LogDebug("Failed to calculate data becuase freq is 0");
        return false;
    }
    ProcessAicoreData(aicoreTimeStamps);
    json result;
    result["profilingType"] = "op";
    result["displayTimeUnit"] = "ns";
    result["schemaVersion"] = 1;
    result["traceEvents"] = timelineJson_;
    timelineJson_ = result;
    return true;
}
}
