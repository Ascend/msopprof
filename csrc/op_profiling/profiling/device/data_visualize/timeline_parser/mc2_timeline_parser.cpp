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

#include "mc2_timeline_parser.h"

#include <algorithm>

#include "filesystem.h"
#include "log.h"
#include "number_operation.h"
#include "ustring.h"
#include "common/hal_helper.h"
#include "common/visualize.h"
#include "profiling/op_prof_data_parse.h"
#include "parse/data_parser/parser_utils/parse_pc_code.h"

using namespace Utility;
using namespace std;

namespace Visualize {
constexpr uint64_t B_TO_GB = 1024 * 1024 * 1024;
constexpr uint16_t COLORNUM = 9; // total 9 color in cnames
constexpr uint16_t MSPROF_REPORT_AICPU_LEVEL = 6000;
constexpr uint16_t MSPROF_REPORT_AICPU_MC2_EXECUTE_COMM_TIME = 4; // communication time
constexpr uint16_t MSPROF_REPORT_AICPU_MC2_BATCH_HCCL_INFO = 13;  // hccl task info

constexpr char const *AICPU_PID = "AI CPU";
constexpr char const *AICPU_TURN_TID = "TURN";
constexpr char const *HCCL_TASK_PID = "HCCL TASK";
constexpr char const *HCCL_TASK_TID = "PLANE";
constexpr char const *HCCL_PID = "HCCL";
constexpr char const *HCCL_TID = "STREAM";

struct AicpuTurn {
    static constexpr char const *START_SERVER = "StartServer";
    static constexpr char const *TASK_WAIT_REQUEST = "TaskWaitRequest";
    static constexpr char const *TASK_ORCHESTRATION = "TaskOrchestration";
    static constexpr char const *TASK_LAUNCH = "TaskLaunch";
    static constexpr char const *TASK_EXECUTE = "TaskExecute";
    static constexpr char const *FINALIZE = "Finalize";
};

struct HcclTask {
    static constexpr char const *HCCL_INFO = "hccl_info";
    static constexpr char const *NOTIFY_RECORD = "Notify_Record";
    static constexpr char const *NOTIFY_WAIT = "Notify_Wait";
    static constexpr char const *MEM_CPY = "Memcpy";
    static constexpr char const *INTER_RANK_RECORD = "Inter_Rank_Record";
    static constexpr char const *INTER_PROCESSOR_SYNC = "Inter_Processor_Sync";
    static constexpr char const *REDUCE_INLINE = "Reduce_Inline";
    static constexpr char const *RDMA_SEND = "RDMASend";
    static constexpr char const *INVALID_TYPE = "INVALID_TYPE";
};

const vector<string> HCCL_CNAME_MAP = {
    string(VISUALIZE_COLOR_NAME::GRASS_GREEN), string(VISUALIZE_COLOR_NAME::GREEN),  string(VISUALIZE_COLOR_NAME::PINK),
    string(VISUALIZE_COLOR_NAME::YELLOW),      string(VISUALIZE_COLOR_NAME::PURPLE), string(VISUALIZE_COLOR_NAME::BLUE),
    string(VISUALIZE_COLOR_NAME::ORANGE),      string(VISUALIZE_COLOR_NAME::CYAN),   string(VISUALIZE_COLOR_NAME::RED)
};

const map<string, string> AICORE_CNAME_MAP = {
    {string(AiCoreDot::INIT),           string(VISUALIZE_COLOR_NAME::PINK)},
    {string(AiCoreDot::COMMIT),         string(VISUALIZE_COLOR_NAME::YELLOW)},
    {string(AiCoreDot::WAIT),           string(VISUALIZE_COLOR_NAME::PURPLE)},
    {string(AiCoreDot::QUERY),          string(VISUALIZE_COLOR_NAME::BLUE)},
    {string(AiCoreDot::FINALIZE),       string(VISUALIZE_COLOR_NAME::ORANGE)},
    {string(AiCoreDot::GROUP_SYNC),     string(VISUALIZE_COLOR_NAME::CYAN)},
    {string(AiCoreDot::GET_WIN_IN),     string(VISUALIZE_COLOR_NAME::RED)},
    {string(AiCoreDot::GET_WIN_OUT),    string(VISUALIZE_COLOR_NAME::GRASS_GREEN)},
    {string(AiCoreDot::GET_RANK_ID),    string(VISUALIZE_COLOR_NAME::GREEN)},
    {string(AiCoreDot::GET_RANK_DIM),   string(VISUALIZE_COLOR_NAME::PINK)},
    {string(AiCoreDot::SET_CCTILING),   string(VISUALIZE_COLOR_NAME::YELLOW)},
    {string(AiCoreDot::ALL_REDUCE),     string(VISUALIZE_COLOR_NAME::BLUE)},
    {string(AiCoreDot::ALL_GATHER),     string(VISUALIZE_COLOR_NAME::BLUE)},
    {string(AiCoreDot::REDUCE_SCATTER), string(VISUALIZE_COLOR_NAME::BLUE)},
    {string(AiCoreDot::ALL_TO_ALL),     string(VISUALIZE_COLOR_NAME::BLUE)},
    {string(AiCoreDot::ALL_TO_ALL_V),   string(VISUALIZE_COLOR_NAME::BLUE)}
};

const map<string, string> HCCL_TASK_CNAME_MAP = {
    {string(HcclTask::HCCL_INFO),            string(VISUALIZE_COLOR_NAME::GRASS_GREEN)},
    {string(HcclTask::NOTIFY_RECORD),        string(VISUALIZE_COLOR_NAME::GREEN)},
    {string(HcclTask::NOTIFY_WAIT),          string(VISUALIZE_COLOR_NAME::PINK)},
    {string(HcclTask::MEM_CPY),              string(VISUALIZE_COLOR_NAME::YELLOW)},
    {string(HcclTask::INTER_RANK_RECORD),    string(VISUALIZE_COLOR_NAME::PURPLE)},
    {string(HcclTask::INTER_PROCESSOR_SYNC), string(VISUALIZE_COLOR_NAME::BLUE)},
    {string(HcclTask::REDUCE_INLINE),        string(VISUALIZE_COLOR_NAME::ORANGE)},
    {string(HcclTask::RDMA_SEND),            string(VISUALIZE_COLOR_NAME::CYAN)},
    {string(HcclTask::INVALID_TYPE),         string(VISUALIZE_COLOR_NAME::RED)}
};

const map<string, string> AICPU_TURN_CNAME_MAP = {
    {string(AicpuTurn::START_SERVER),        string(VISUALIZE_COLOR_NAME::PINK)},
    {string(AicpuTurn::TASK_WAIT_REQUEST),   string(VISUALIZE_COLOR_NAME::YELLOW)},
    {string(AicpuTurn::TASK_ORCHESTRATION),  string(VISUALIZE_COLOR_NAME::PURPLE)},
    {string(AicpuTurn::TASK_LAUNCH),         string(VISUALIZE_COLOR_NAME::BLUE)},
    {string(AicpuTurn::TASK_EXECUTE),        string(VISUALIZE_COLOR_NAME::ORANGE)},
    {string(AicpuTurn::FINALIZE),            string(VISUALIZE_COLOR_NAME::CYAN)}
};

// MsprofAicpuHcclTaskInfo itemId, key is hash data
const map<uint64_t, string> HCCL_TASK_TYPE_MAP = {
    {0x448d1a5ea052639,  string(HcclTask::HCCL_INFO)},
    {0x9eb5886f0b5ef22f, string(HcclTask::NOTIFY_RECORD)},
    {0xab44b101f36f4b07, string(HcclTask::NOTIFY_WAIT)},
    {0x628f0043a268d0cf, string(HcclTask::MEM_CPY)},
    {0xfe1a130f3676775d, string(HcclTask::INTER_RANK_RECORD)},
    {0x23f07c59d5c54aaf, string(HcclTask::INTER_PROCESSOR_SYNC)},
    {0x2c384f8e83425f7a, string(HcclTask::REDUCE_INLINE)},
    {0x220e79fe6508b9ce, string(HcclTask::RDMA_SEND)},
};

const map<uint8_t, string> HCCL_TRANSPORT_TYPE_MAP = {
    {0, "SDMA"},
    {1, "RDMA"},
    {2, "LOCAL"},
};

const map<uint8_t, string> HCCL_DATA_TYPE_MAP = {
    {0, "INT8"},
    {1, "INT16"},
    {2, "INT32"},
    {3, "FP16"},
    {4, "FP32"},
    {5, "INT64"},
    {6, "UINT64"},
    {7, "UINT8"},
    {8, "UINT16"},
    {9, "UINT32"},
    {10, "FP64"},
    {11, "BFP16"},
    {12, "INT128"},
};

const map<uint8_t, string> HCCL_LINK_TYPE_MAP = {
    {0, "ON_CHIP"},
    {1, "HCCS"},
    {2, "PCIE"},
    {3, "ROCE"},
    {4, "SIO"},
    {5, "HCCS_SW"},
    {6, "STANDARD_ROCE"},
    {7, "RESERVED"},
};

const map<uint16_t, string> ACSQ_TASK_TYPE_MAP = {
    {0, "AI_CORE"},
    {1, "AI_CPU"},
    {2, "AIV_SQE"},
    {3, "PLACE_HOLDER_SQE"},
    {4, "EVENT_RECORD_SQE"},
    {5, "EVENT_WAIT_SQE"},
    {6, "NOTIFY_RECORD_SQE"},
    {7, "NOTIFY_WAIT_SQE"},
    {8, "WRITE_VALUE_SQE"},
    {9, "VQ6_SQE"},
    {10, "TOF_SQE"},
    {11, "SDMA_SQE"},
    {12, "VPC_SQE"},
    {13, "JPEGE_SQE"},
    {14, "JPEGD_SQE"},
    {15, "DSA_SQE"},
    {16, "ROCCE_SQE"},
    {17, "PCIE_DMA_SQE"},
    {18, "HOST_CPU_SQE"},
    {19, "CDQM_SQE"},
    {20, "C_CORE_SQE"},
};

void GetStreamIdAndTaskId(uint32_t &taskId, uint16_t &streamId)
{
    // when the 13th bit of the stream id is set, the task id need to be exchanged.
    // when the 14th bit of the stream id is set, the lower 12 bits of the stream id and task id need to be exchanged.
    constexpr uint16_t offset = 11;
    uint32_t hardwareTaskId = taskId;
    uint16_t hardwareStreamId = streamId;
    if ((hardwareStreamId & 0x1000) != 0) {
        taskId = (hardwareTaskId & 0x1FFF) | (hardwareStreamId & 0xE000);
        streamId = hardwareStreamId % (1 << offset);
    } else if ((hardwareStreamId & 0x2000) != 0) {
        taskId = (hardwareTaskId & 0xF000) | (hardwareStreamId & 0x0FFF);
        streamId = hardwareTaskId & 0x0FFF;
    } else {
        taskId = hardwareTaskId;
        streamId = hardwareStreamId % (1 << offset);
    }
}

void MC2TimelineParser::GetAicpuDatas(vector<AicpuKfcProfCommTurn> &aicpuTurns,
                                      vector<MsprofAicpuHcclTaskInfo> &aicpuHcclTasks)
{
    // aicpu.bin include aicpu turn data and hccl task data, use type and level to discriminate.
    string binFilePath = JoinPath({outputPath_, "dump/aicpu.bin"});
    size_t fileSize = GetFileSize(binFilePath);
    size_t additionalSize = sizeof(MsprofAdditionalInfo);
    vector<char> binData;
    if (fileSize < additionalSize || !ReadBinFileByMultiStruct(binFilePath, fileSize, additionalSize, binData)) {
        return;
    }

    size_t hcclSize = sizeof(MsprofAicpuHcclTaskInfo);
    size_t turnSize = sizeof(AicpuKfcProfCommTurn);
    for (size_t i = 0; i < fileSize; i = i + additionalSize) {
        MsprofAdditionalInfo additionalInfo{};
        if (memcpy_s(&additionalInfo, additionalSize, &binData[i], additionalSize) != EOK) {
            LogDebug("memcpy MsprofAdditionalInfo data from bin file failed.");
            continue;
        }
        if (additionalInfo.level != MSPROF_REPORT_AICPU_LEVEL) {
            continue;
        }
        // aicpu turn data, type=4, level=6000
        if (additionalInfo.type == MSPROF_REPORT_AICPU_MC2_EXECUTE_COMM_TIME) {
            AicpuKfcProfCommTurn turn{};
            if (memcpy_s(&turn, turnSize, &additionalInfo.data[0], turnSize) != EOK) {
                LogDebug("memcpy AicpuKfcProfCommTurn data from bin file failed.");
                continue;
            }
            aicpuTurns.push_back(turn);
        }
        // hccl task data, type=13, level=6000
        if (additionalInfo.type != MSPROF_REPORT_AICPU_MC2_BATCH_HCCL_INFO) {
            continue;
        }
        int beginIndex = 0;
        for (int j = 0; j < 2; j++) { // every additionalInfo contains 2 hccl task data
            MsprofAicpuHcclTaskInfo info{};
            if (memcpy_s(&info, hcclSize, &additionalInfo.data[beginIndex], hcclSize) != EOK) {
                LogDebug("memcpy MsprofAicpuHcclTaskInfo data from bin file failed.");
                continue;
            }
            if (info.groupName == 0) {
                continue;
            }
            GetStreamIdAndTaskId(info.taskId, info.streamId);
            aicpuHcclTasks.push_back(info);
            beginIndex += static_cast<int>(hcclSize);
        }
    }
}

void MC2TimelineParser::PreProcessData(vector<AicpuKfcProfCommTurn> &aicpuTurns,
                                       vector<MsprofAicpuHcclTaskInfo> &aicpuHcclTasks,
                                       vector<MsprofAicTimeStampInfoUpdate> &aicoreTimeStamps)
{
    // get aicpu data from aicpu.bin
    GetAicpuDatas(aicpuTurns, aicpuHcclTasks);
    if (!GetAicoreTimeStamps(aicoreTimeStamps)) {
        LogDebug("Can not parse aic_timestamp.bin, no need to generate timeline.");
    }
}

void ProcessOperationStart(const MsprofAicTimeStampInfoUpdate& item, const string& operationType, const std::string &type,
                           unordered_map<string, OperationInfo>& operationMap)
{
    uint32_t descId = item.descId;
    uint64_t syscyc = item.syscyc;
    uint32_t blockId = item.blockId;
    uint64_t curPc = item.curPc;

    bool isStart = descId % 2 == 0;
    if (isStart) {
        if (!operationMap[operationType].startFound) {
            operationMap[operationType].startSyscyc = syscyc;
            operationMap[operationType].blockId = blockId;
            operationMap[operationType].startCurPc = curPc;
            operationMap[operationType].startFound = true;
            operationMap[operationType].type = type;
        }
    }
}

void ProcessOperationEnd(const MsprofAicTimeStampInfoUpdate& item, const string& operationType,
                         unordered_map<string, OperationInfo>& operationMap)
{
    uint32_t descId = item.descId;
    uint64_t syscyc = item.syscyc;

    bool isStart = descId % 2 == 0;
    if (!isStart && operationMap[operationType].startFound) {
        operationMap[operationType].endSyscyc = syscyc;
        operationMap[operationType].endFound = true;
    }
}

json MC2TimelineParser::BuildAicoreDot(const OperationInfo& info, const string& operationType)
{
    json resultItem;
    string codeAcc;
    string location = "MC2 AICore";
    if (aicpuFreq_ == 0) {
        return resultItem;
    }
    if (AICORE_CNAME_MAP.count(operationType) != 0) {
        resultItem["cname"] = AICORE_CNAME_MAP.at(operationType);
    }
    resultItem["dur"] = static_cast<float>(SafeSub(info.endSyscyc, info.startSyscyc, location, false)) / aicpuFreq_
        * TIME_CONVERSION;
    resultItem["name"] = operationType;
    resultItem["ph"] = "X";
    resultItem["pid"] = info.type;
    resultItem["tid"] = info.blockId;
    resultItem["ts"] = static_cast<float>(SafeSub(info.startSyscyc, minSysCyc_, location, false)) / aicpuFreq_
        * TIME_CONVERSION;

    if (pc2code_.Find(info.startCurPc)) {
        codeAcc = accumulate(pc2code_[info.startCurPc].begin(),
                             pc2code_[info.startCurPc].end(),
                             string(),
                             [](string acc, const string &s) {
                                 return acc.empty() ? s : acc + "\n" + s;
                             });
        resultItem["args"]["code"] = codeAcc;
        resultItem["args"]["pc_addr"] = info.startCurPc;
    } else {
        resultItem["args"]["code"] = "";
        resultItem["args"]["pc_addr"] = "";
    }
    return resultItem;
}

void MC2TimelineParser::ProcessMC2AicoreData(const vector<MsprofAicTimeStampInfoUpdate> &aicoreTimeStamps)
{
    set<uint16_t> dotBlockIds;
    // add aicore dot
    unordered_map<string, OperationInfo> operationMap;
    // add aicore block duration
    ProcessAicoreBlockDur();
    for (uint32_t i = 0; i < aicoreTimeStamps.size(); i++) {
        auto &item = aicoreTimeStamps[i];
        if (AICORE_DOT_MAP.find(item.descId) == AICORE_DOT_MAP.end()) {
            continue;
        }
        dotBlockIds.insert(item.blockId);
        uint32_t descId = item.descId;
        string colorType = AICORE_DOT_MAP.at(descId); // descId has been verified, must be in AICORE_DOT_MAP
        std::string operationType =  AICORE_DOT_MAP.at(descId) + item.type + std::to_string(item.blockId);
        ProcessOperationStart(item, operationType, item.type, operationMap);
        ProcessOperationEnd(item, operationType, operationMap);
        if (!operationMap[operationType].startFound || !operationMap[operationType].endFound) {
           continue;
        }
        if (blockDuration_[{item.type, item.blockId}].first > operationMap[operationType].startSyscyc || blockDuration_[{item.type, item.blockId}].second < operationMap[operationType].endSyscyc) {
            continue;
        }
        json resultItem = BuildAicoreDot(operationMap[operationType], colorType);
        timelineJson_.push_back(resultItem);
        operationMap[operationType].startFound = false;
        operationMap[operationType].endFound = false;
    }
    ProcessAicoreData(aicoreTimeStamps);
}

void MC2TimelineParser::SortTimelineByIds(const set<uint16_t> &sortIds, const string &pidName, const string &tidName)
{
    // sort Timeline by id, use for Hccl, HcclTaskInfo and AicpuTurn, eg: STREAM10 should after STREAM3.
    for (const auto &id: sortIds) {
        string displayName = tidName + to_string(id);
        json nameItem;
        nameItem["ph"] = "M";
        nameItem["name"] = "thread_name";
        nameItem["pid"] = pidName;
        nameItem["tid"] = displayName;
        nameItem["args"]["name"] = displayName;
        timelineJson_.push_back(nameItem);
        json sortItem;
        sortItem["ph"] = "M";
        sortItem["name"] = "thread_sort_index";
        sortItem["pid"] = pidName;
        sortItem["tid"] = displayName;
        sortItem["args"]["sort_index"] = SORT_BIAS + id;
        timelineJson_.push_back(sortItem);
    }
}

template<typename MapType, typename KeyType>
inline string FindMap(const MapType &mapData, const KeyType &key)
{
    auto it = mapData.find(key);
    return it != mapData.end() ? it->second : "INVALID_TYPE";
}

void MC2TimelineParser::ProcessHcclTaskData(const vector<MsprofAicpuHcclTaskInfo> &aicpuHcclTasks)
{
    set<uint16_t> planeIds;
    if (aicpuFreq_ == 0) {
        return;
    }
    for (auto &info: aicpuHcclTasks) {
        auto iter = acsqTimeMap_.find({info.taskId, info.streamId});
        if (iter == acsqTimeMap_.end() || iter->second.startTime == 0 || iter->second.endTime == 0 ||
            iter->second.endTime < iter->second.startTime || iter->second.startTime < minSysCyc_) {
            continue;
        }

        planeIds.insert(info.planeID);
        string location = "MC2 Hccltask";
        string taskTypeStr = FindMap(HCCL_TASK_TYPE_MAP, info.itemId);
        float dur = static_cast<float>(SafeSub(iter->second.endTime, iter->second.startTime, location, false)) /
                    aicpuFreq_; // unit is ms
        float bandwidth = fabs(dur) < numeric_limits<float>::epsilon() ? 0.0f :
                          static_cast<float>(info.dataSize) / dur / B_TO_GB * TIME_CONVERSION; // unit is GB/s
        JsonEvent event = {
            taskTypeStr, HCCL_TASK_CNAME_MAP.at(taskTypeStr), "X", HCCL_TASK_PID,
            HCCL_TASK_TID + to_string(info.planeID),
            static_cast<float>(SafeSub(iter->second.startTime, minSysCyc_, location, false)) / aicpuFreq_ *
            TIME_CONVERSION,
            dur * TIME_CONVERSION,
            {
                {"plane id", to_string(info.planeID)},
                {"notify id", to_string(info.notifyID)},
                {"duration estimated(us)", to_string(info.durationEstimated)},
                {"stream id", to_string(info.streamId)},
                {"task id", to_string(info.taskId)},
                {"task type", taskTypeStr},
                {"src rank", to_string(info.localRank)},
                {"dst rank", to_string(info.remoteRank)},
                {"transport type", FindMap(HCCL_TRANSPORT_TYPE_MAP, info.transportType)},
                {"size(Byte)", to_string(info.dataSize)},
                {"data type", FindMap(HCCL_DATA_TYPE_MAP, info.dataType)},
                {"link type", FindMap(HCCL_LINK_TYPE_MAP, info.linkType)},
                {"bandwidth(GB/s)", to_string(bandwidth)}
            }
        };
        json jsonData;
        event.ToJson(jsonData);
        timelineJson_.emplace_back(jsonData);
    }
    SortTimelineByIds(planeIds, HCCL_TASK_PID, HCCL_TASK_TID);
}

void MC2TimelineParser::AddAicpuTurnJson(const AicpuKfcProfCommTurn &turn, const string &turnName, uint64_t startTime,
                                         uint64_t endTime)
{
    if (startTime < minSysCyc_ || endTime <= startTime) {
        return;
    }
    if (aicpuFreq_ == 0) {
        return;
    }
    JsonEvent event = {
        turnName,
        AICPU_TURN_CNAME_MAP.at(turnName),
        "X",
        AICPU_PID,
        AICPU_TURN_TID + to_string(turn.currentTurn),
        static_cast<float>(startTime - minSysCyc_) / aicpuFreq_ * TIME_CONVERSION,
        static_cast<float>(endTime - startTime) / aicpuFreq_ * TIME_CONVERSION,
        {
            {"stream id", to_string(turn.streamId)},
            {"task id", to_string(turn.taskId)},
        }
    };
    json jsonData;
    event.ToJson(jsonData);
    timelineJson_.emplace_back(jsonData);
}

void MC2TimelineParser::ProcessAicpuTurnData(const vector<AicpuKfcProfCommTurn> &aicpuTurns)
{
    set<uint16_t> turnIds;
    for (const auto &turn: aicpuTurns) {
        turnIds.insert(turn.currentTurn);
        // {{communication name, {startTime, endTime}}}
        map<string, pair<uint64_t, uint64_t>> commTimeMap = {
            {string(AicpuTurn::START_SERVER),       {turn.waitNotifyStartTime, turn.kfcAlgExeStartTime}},
            {string(AicpuTurn::TASK_WAIT_REQUEST),  {turn.kfcAlgExeStartTime,  turn.sendTaskStartTime}},
            {string(AicpuTurn::TASK_ORCHESTRATION), {turn.sendTaskStartTime,   turn.waitActiveStartTime}},
            {string(AicpuTurn::TASK_LAUNCH),        {turn.waitActiveStartTime, turn.activeStartTime}},
            {string(AicpuTurn::TASK_EXECUTE),       {turn.activeStartTime,     turn.waitExeEndStartTime}},
            {string(AicpuTurn::FINALIZE),           {turn.waitExeEndStartTime, turn.rtsqExeEndTime}},
        };
        for (const auto &comm: commTimeMap) {
            AddAicpuTurnJson(turn, comm.first, comm.second.first, comm.second.second);
        }
    }
    SortTimelineByIds(turnIds, AICPU_PID, AICPU_TURN_TID);
}

void MC2TimelineParser::ProcessHcclData()
{
    set<uint16_t> streamIds;
    for (const auto &taskPair: acsqTimeMap_) {
        AcsqTaskInfo info = taskPair.second;
        uint64_t startTime = info.startTime;
        uint64_t endTime = info.endTime;
        if (startTime == 0 || endTime == 0 || endTime < startTime || startTime < minSysCyc_) {
            continue;
        }
        uint16_t taskType = info.taskType;
        auto typeIter = ACSQ_TASK_TYPE_MAP.find(taskType);
        if (typeIter == ACSQ_TASK_TYPE_MAP.end()) {
            continue;
        }
        string taskTypeStr = typeIter->second;

        uint16_t streamId = taskPair.first.second;
        string location = "MC2 Hccl";
        if (aicpuFreq_ == 0) {
            return;
        }
        JsonEvent event = {
            taskTypeStr,
            HCCL_CNAME_MAP[taskType % COLORNUM],
            "X",
            HCCL_PID,
            HCCL_TID + to_string(streamId),
            static_cast<float>(SafeSub(startTime, minSysCyc_, location, false)) / aicpuFreq_ * TIME_CONVERSION,
            static_cast<float>(SafeSub(endTime, startTime, location, false)) / aicpuFreq_ * TIME_CONVERSION,
            {
                {"task id", to_string(taskPair.first.first)},
                {"stream id", to_string(streamId)}
            }
        };
        if ((taskTypeStr == "AI_CPU") || (taskTypeStr == "AI_CORE")) {
            event.pid = ReplaceSubStr(taskTypeStr, "_", " ");
            event.tid = taskTypeStr;
        } else {
            streamIds.insert(streamId);
        }
        json jsonData;
        event.ToJson(jsonData);
        timelineJson_.emplace_back(jsonData);
    }
    SortTimelineByIds(streamIds, HCCL_PID, HCCL_TID);
}

void MC2TimelineParser::ProcessJsonData(const vector<AicpuKfcProfCommTurn> &aicpuTurns,
                                        const vector<MsprofAicpuHcclTaskInfo> &aicpuHcclTasks,
                                        vector<MsprofAicTimeStampInfoUpdate> &aicoreTimeStamps)
{
    ProcessAicpuTurnData(aicpuTurns);
    ProcessHcclTaskData(aicpuHcclTasks);
    ProcessHcclData();
    ProcessMC2AicoreData(aicoreTimeStamps);
}

bool MC2TimelineParser::TimelineToJson(const string &outputPath)
{
    ChipProductType chipType = GetChipTypeSeries();
    if (chipType != ChipProductType::ASCEND910B_SERIES && chipType != ChipProductType::ASCEND910_93_SERIES) {
        return false;
    }
    if (!Common::HalHelper::Instance().GetTaskSchedulerFreq(aicpuFreq_)) {
        LogWarn("Get task scheduler frequency failed. Use default value instead");
        aicpuFreq_ = FREQ;
    }
    outputPath_ = outputPath;
    BlockSystemTimeType blockSystemTimes;
    vector<AicpuKfcProfCommTurn> aicpuTurns;
    vector<MsprofAicpuHcclTaskInfo> aicpuHcclTasks;
    vector<MsprofAicTimeStampInfoUpdate> aicoreTimeStamps;
    PreProcessData(aicpuTurns, aicpuHcclTasks, aicoreTimeStamps);
    ProcessJsonData(aicpuTurns, aicpuHcclTasks, aicoreTimeStamps);
    json result;
    result["profilingType"] = "op";
    result["displayTimeUnit"] = "ns";
    result["schemaVersion"] = 1;
    result["traceEvents"] = timelineJson_;
    timelineJson_ = result;
    return true;
}
}
