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
constexpr int64_t FREQ = 50000; // kHz
constexpr uint16_t TIME_CONVERSION = 1000; // time in visualize.bin will be us, cyc/FREQ unit is ms
constexpr uint16_t SORT_BIAS = 10000;

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

struct LcclEvent {
    inline void ToJson(nlohmann::json &jsonData) const
    {
        jsonData["name"] = this->name;
        jsonData["cname"] = this->cName;
        jsonData["ph"] = this->ph;
        jsonData["pid"] = this->pid;
        jsonData["tid"] = this->tid;
        jsonData["ts"] = this->ts;
        jsonData["dur"] = this->dur;
        jsonData["args"] = this->args;
    }

    string name;
    string cName;
    string ph;
    string pid;
    string tid;
    float ts;
    float dur;
    map<string, string> args;
};

void LcclTimelineParser::GetAicoreTimeStamps(vector<LcclDumpLogInfo> &aicoreTimeStamps)
{
    string binFilePath = JoinPath({outputPath_, "dump/aic_timestamp.bin"});
    size_t fileSize = GetFileSize(binFilePath);
    size_t structSize = sizeof(LcclDumpLogInfo);
    vector<char> binData;
    if (fileSize < structSize || !ReadBinFileByMultiStruct(binFilePath, fileSize, structSize, binData)) {
        return;
    }
    for (size_t i = 0; i < fileSize; i = i + structSize) {
        LcclDumpLogInfo info{};
        if (memcpy_s(&info, structSize, &binData[i], structSize) != EOK) {
            continue;
        }
        if (info.syscyc < minSysCyc_ || LOG_ID_MAP.find(info.logId) == LOG_ID_MAP.end()) {
            continue;
        }
        aicoreTimeStamps.push_back(info);
        if (i > SIZE_MAX - structSize) {
            break;
        }
    }
}

void LcclTimelineParser::PreProcessData(BlockSystemTimeType &blockSystemTimes,
                                        vector<LcclDumpLogInfo> &aicoreTimeStamps)
{
    // get aicore system time of every block
    for (const auto &pair: basicPmuObj_->GetTotalPmuData()) {
        blockSystemTimes[pair.first.first].emplace_back(pair.second.systemTime);
        minSysCyc_ = min(pair.second.systemTime.first, minSysCyc_);
    }
    // get aicore dot timestamp from aic_timestamp.bin
    GetAicoreTimeStamps(aicoreTimeStamps);
}

void LcclTimelineParser::AddAicoreBlockDur(const BlockSystemTimeType &blockSystemTimes, set<uint16_t> &dotBlockIds)
{
    string opType = opBasicInfoObj_->GetOpType();
    string location = "Lccl Block Dur";
    for (const auto &pair: blockSystemTimes) {
        uint16_t blockIndex = pair.first;
        auto timeVec = pair.second;
        uint16_t subBlockNum = timeVec.size();
        for (uint16_t i = 0; i < subBlockNum; i++) {
            uint16_t dotBlockId = GetAicoreDotBlockId(opType, blockIndex, i, subBlockNum);
            dotBlockIds.insert(dotBlockId);
            string cName = (dotBlockId >= CUBE_BLOCK_START_INDEX) ? string(VISUALIZE_COLOR_NAME::GRASS_GREEN) :
                string(VISUALIZE_COLOR_NAME::GREEN);
            LcclEvent event = {
                GetAicoreBlockName(dotBlockId), cName, "X", GetAicoreTimeLinePid(dotBlockId), to_string(dotBlockId),
                static_cast<float>(SafeSub(timeVec[i].first, minSysCyc_, location, false)) / aicpuFreq_
                * TIME_CONVERSION,
                static_cast<float>(SafeSub(timeVec[i].second, timeVec[i].first, location, false)) / aicpuFreq_
                * TIME_CONVERSION,
            };
            nlohmann::json jsonData;
            event.ToJson(jsonData);
            traceEvents_.push_back(jsonData);
        }
    }
}

nlohmann::json LcclTimelineParser::AddAicoreApiDot(const LcclInfo& info) const
{
    string location = "Lccl Api";
    LcclEvent event;
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
    event.args = {
        {"curPc", info.curPc},
        {"operationType", to_string(info.operationType)},
        {"rsv", to_string(info.rsv)},
    };
    nlohmann::json resultItem;
    event.ToJson(resultItem);
    return resultItem;
}

void LcclTimelineParser::ProcessAicoreData(const BlockSystemTimeType &blockSystemTimes,
                                           const vector<LcclDumpLogInfo> &aicoreTimeStamps)
{
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
            it->second.endSyscyc = item.syscyc;
            nlohmann::json resultItem = AddAicoreApiDot(it->second);
            traceEvents_.push_back(resultItem);
            operationMap.erase(logBlockPair);
        }
    }
    for (const auto& iter: operationMap) {
        LogDebug("Lccl data not match, logId: %u, blockId: %u, syscyc: %llu, curPc: %s, operationType: %u, "
            "rsv: %u.", iter.second.logId, iter.second.blockId, iter.second.startSyscyc, iter.second.curPc.c_str(),
            iter.second.operationType, iter.second.rsv);
    }
    // add aicore block duration
    AddAicoreBlockDur(blockSystemTimes, dotBlockIds);
    // sort aicore timeline
    for (const auto &dotBlockId: dotBlockIds) {
        nlohmann::json nameItem;
        nameItem["ph"] = "M";
        nameItem["name"] = "thread_name";
        nameItem["pid"] = GetAicoreTimeLinePid(dotBlockId);
        nameItem["tid"] = dotBlockId;
        nameItem["args"]["name"] = GetAicoreBlockName(dotBlockId);
        traceEvents_.push_back(nameItem);
        nlohmann::json sortItem;
        sortItem["ph"] = "M";
        sortItem["name"] = "thread_sort_index";
        sortItem["pid"] = GetAicoreTimeLinePid(dotBlockId);
        sortItem["tid"] = dotBlockId;
        sortItem["args"]["sort_index"] = SORT_BIAS + dotBlockId;
        traceEvents_.push_back(sortItem);
    }
}

void LcclTimelineParser::ProcessJsonData(const BlockSystemTimeType &blockSystemTimes,
                                         const vector<LcclDumpLogInfo> &aicoreTimeStamps)
{
    if (!Common::HalHelper::Instance().GetTaskSchedulerFreq(aicpuFreq_)) {
        LogWarn("Get task scheduler frequency failed. Use default value instead.");
        aicpuFreq_ = FREQ;
    }
    if (aicpuFreq_ == 0) {
        return;
    }

    ProcessAicoreData(blockSystemTimes, aicoreTimeStamps);
    string location = "Lccl Aicore Dur";
    LcclEvent event = {
        "AI_CORE", string(VISUALIZE_COLOR_NAME::GRASS_GREEN), "X", "AI CORE", "AI_CORE",
        static_cast<float>(SafeSub(aicoreStartCyc_, minSysCyc_, location, false)) / aicpuFreq_ * TIME_CONVERSION,
        opBasicInfoObj_->GetDuration()
    };
    nlohmann::json jsonData;
    event.ToJson(jsonData);
    traceEvents_.emplace_back(jsonData);
}

bool LcclTimelineParser::TimelineToJson(const string &outputPath)
{
    if (!isLccl_) {
        LogDebug("This is not a lccl operator, no need to generate lccl timeline.");
        return false;
    }
    auto socVersion = opBasicInfoObj_->GetSoc();
    if (Common::SOC_910B.count(socVersion) == 0 && Common::SOC_910_93.count(socVersion) == 0) {
        LogDebug("Only A2 and A3 need to generate lccl timeline.");
        return false;
    }

    outputPath_ = outputPath;
    BlockSystemTimeType blockSystemTimes;
    vector<LcclDumpLogInfo> aicoreTimeStamps;
    PreProcessData(blockSystemTimes, aicoreTimeStamps);
    ProcessJsonData(blockSystemTimes, aicoreTimeStamps);
    timelineJson_["profilingType"] = "op";
    timelineJson_["displayTimeUnit"] = "ns";
    timelineJson_["schemaVersion"] = 1;
    timelineJson_["traceEvents"] = traceEvents_;
    string tracePath = JoinPath({outputPath_, "trace.json"});
    if (!WriteFileByStream(tracePath, timelineJson_)) {
        LogWarn("Generate %s failed.", tracePath.c_str());
    }
    return true;
}
}
