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


#include "core_timeline_visualizer.h"
#include "parse/data_visualizer/utility.h"
#include "common/visualize.h"
#include "ustring.h"
#include "thread_pool.h"
#include "filesystem.h"

using namespace Utility;
namespace Profiling {
namespace Parse {

PluginErrorCode CoreTimeLineVisualizer::Entry()
{
    auto dataMapPtr = dataCenter_.GetDbPtr<std::map<std::string, SimData>>();
    if (dataMapPtr == nullptr) {
        return PluginErrorCode::NONBLOCKING_ERROR;
    }
    ThreadPool pool(dataVisualizerConfig_.GetThreads());
    pool.Start();
    for (auto &coreNameWithData: *dataMapPtr) {
        const std::string &coreName = coreNameWithData.first;
        SimData data = coreNameWithData.second;
        AddCoreNameOrder(coreName);
        if (data.instrs == nullptr) {
            LogError("Failed to generate core timeline for %s", coreName.c_str());
            continue;
        }
        pool.AddTask([this, &coreNameWithData]() {
            ParseByCore(coreNameWithData.first, coreNameWithData.second);
        });
    }
    pool.WaitAllTasks();
    pool.Stop();

    // 写入MTE Throughput json
    std::shared_ptr<std::vector<nlohmann::json>> mteJsonListPtr = dataCenter_.GetDbPtr<std::vector<nlohmann::json>>();
    if (mteJsonListPtr != nullptr) {
        coresJsonList_.insert(coresJsonList_.end(), mteJsonListPtr->begin(), mteJsonListPtr->end());
    }

    // 将泳道排序事件写入到json
    if (laneJsonList_.size() != 0) {
        coresJsonList_.insert(coresJsonList_.begin(), laneJsonList_.begin(), laneJsonList_.end());
    }

    // write all cores json
    std::string outputPath = dataVisualizerConfig_.GetOutputPath();
    WriteFile(outputPath);
    LogDebug("Save trace json of all cores in %s", outputPath.c_str());
    return PluginErrorCode::SUCCESS;
}

uint32_t CoreTimeLineVisualizer::ExtraNumAfterKey(const std::string &str, const std::string &key)
{
    size_t keyPos = str.find(key);
    if (keyPos == std::string::npos) {
        return 0;
    }
    size_t numStart = keyPos + key.length();
    std::string numStr;
    while (numStart < str.length() && isdigit(str[numStart])) {
        numStr += str[numStart];
        numStart++;
    }
    return numStr.empty() ? 0 : std::stoul(numStr);
}

void CoreTimeLineVisualizer::AddCoreNameOrder(const std::string &coreName)
{
    uint32_t result = 0;
    if (coreName.find("core") != std::string::npos) {
        uint32_t coreIndex = ExtraNumAfterKey(coreName, "core");
        if (coreName.find("veccore") != std::string::npos) {
            uint32_t veccoreIndex = ExtraNumAfterKey(coreName, "veccore");
            result = coreIndex * 3 + 2 + veccoreIndex;
        } else {
            result = coreIndex * 3 + 1;
        }
    }
    LaneEvent event = {"process_sort_index", "M", coreName, "NA", {}};
    event.args["sort_index"] = result;
    nlohmann::json jsonData;
    event.ToJson(jsonData);
    laneJsonList_.emplace_back(jsonData);
}

bool CoreTimeLineVisualizer::ParseByCore(const std::string &coreName, SimData &data)
{
    std::vector<nlohmann::json> coreJson;
    // get instr events
    std::shared_ptr<InstrDetailTable> instrDetailTable = data.instrs;
    if (instrDetailTable == nullptr) {
        return false;
    }
    std::shared_ptr<std::vector<MergeInfo>> instrList =
            instrDetailTable->GetColumnData<MergeInfo>(InstrDetailTable::MERGE_INFO);
    if (instrList == nullptr) {
        return false;
    }
    std::vector<MergeInfo> instrs = *instrList;
    sort(instrs.begin(), instrs.end(),
        [](MergeInfo &instr1, MergeInfo &instr2) {
            if (instr1.endTick != instr2.endTick) {
                return instr1.endTick < instr2.endTick;
            }
        return instr1.pc < instr2.pc;
    });
    CollectLaneOrderEvents(coreName, instrs, coreJson);
    CollectInstrEvents(coreName, instrs, coreJson);

    // get cache events in this place
    std::shared_ptr<CacheDetailTable> cacheDetailTable = data.caches;
    if (cacheDetailTable) {
        std::vector<MergeInfo> cache = cacheDetailTable->GetCache();
        sort(cache.begin(), cache.end(),
            [](MergeInfo &instr1, MergeInfo &instr2) {
                if (instr1.endTick != instr2.endTick) {
                    return instr1.endTick < instr2.endTick;
                }
            return instr1.pc < instr2.pc;
        });
        CollectInstrEvents(coreName, cache, coreJson);
    }
    CollectUserMarkEvents(coreName, data, coreJson);
    std::lock_guard<std::mutex> lock(timeLineLock_);
    coresJsonList_.insert(coresJsonList_.end(), coreJson.begin(), coreJson.end());
    return true;
}

void CoreTimeLineVisualizer::WriteFile(const std::string &filePath)
{
    constexpr int writeJsonThreadPoolSize = 2;
    nlohmann::json fileJson;

    // 泳道排序后的可视化文件schemaVersion修改为2，未排序文件schemaVersion为1
    fileJson["schemaVersion"] = 2;
    fileJson["displayTimeUnit"] = "ns";
    fileJson["profilingType"] = "op";
    fileJson["traceEvents"] = coresJsonList_;
    std::string jsonPath = Utility::JoinPath({dataVisualizerConfig_.GetOutputPath(), "trace.json"});
    ThreadPool pool(writeJsonThreadPoolSize);
    pool.Start();

    pool.AddTask([this, &fileJson, &filePath] {
        Visualize::WriteBin<VisualizeType::TRACE>(filePath, fileJson);
    });

    pool.AddTask([&] {
        WriteFileByStream(jsonPath, fileJson);
    });

    pool.WaitAllTasks();
    pool.Stop();
}

void CoreTimeLineVisualizer::CollectLaneOrderEvents(const std::string &coreName, std::vector<MergeInfo> &instrs,
    std::vector<nlohmann::json> &coreJson)
{
    std::map <std::string, uint32_t> laneMap = Profiling::Parse::getLaneOrderMap();
    for (auto &instr: instrs) {
        std::string laneName = "WARP_" + std::to_string(instr.warpId);
        if (laneMap.find(laneName) == laneMap.end()) {
            laneMap[laneName] = static_cast<uint32_t>(100 + instr.warpId);
        }
    }
    for (auto &record: laneMap) {
        LaneEvent event = {"thread_sort_index", "M", coreName, record.first, {}};
        event.args["sort_index"] = record.second;
        nlohmann::json jsonData;
        event.ToJson(jsonData);
        coreJson.emplace_back(jsonData);
    }
}

void CoreTimeLineVisualizer::CollectInstrEvents(const std::string &coreName, std::vector<MergeInfo> &instrs,
    std::vector<nlohmann::json> &coreJson)
{
    std::map<std::string, uint64_t> pipeEndTime;
    std::map<std::string, uint32_t> markCnt;
    std::map<std::string, std::vector<SetWaitFlag>> setFlagRecord;
    std::map<std::string, std::vector<SetWaitFlag>> waitFlagRecord;
    int count = 0;
    for (auto &instr: instrs) {
        std::vector<std::string> codeStack {};
        if (pc2code_.Find(instr.pc)) {
            codeStack = pc2code_[instr.pc];
        }
        EventArgs evtArgs = {GetPc2String(instr.pc), Utility::Join(codeStack.begin(), codeStack.end(), "\n"),
            instr.detail};
        if (instr.name == waitFlagName_ && pipeEndTime.count(instr.pipe) > 0) {
            uint64_t endTime = pipeEndTime.at(instr.pipe);
            if (instr.startTick < endTime && instr.endTick > endTime) {
                instr.startTick = endTime;
            }
        }
        if (instr.name == setFlagName_) {
            setFlagRecord[instr.detail].emplace_back(SetWaitFlag {instr, evtArgs, coreName});
            continue;
        }
        if (instr.name == waitFlagName_) {
            waitFlagRecord[instr.detail].emplace_back(SetWaitFlag {instr, evtArgs, coreName});
            continue;
        }
        if (pipeEndTime.count(instr.pipe) == 0 || instr.endTick > pipeEndTime.at(instr.pipe)) {
            pipeEndTime[instr.pipe] = instr.endTick;
        }
        GenerateEvent(instr, evtArgs, coreName, instr.name + std::to_string(count), coreJson);
        count++;
    }
    CollectFlowEvents(setFlagRecord, waitFlagRecord, coreJson);
}

void CoreTimeLineVisualizer::CollectFlowEvents(std::map<std::string, std::vector<SetWaitFlag>> &setFlagRecord,
    std::map<std::string, std::vector<SetWaitFlag>> &waitFlagRecord, std::vector<nlohmann::json> &coreJson)
{
    int flagMatchId = 0;
    for (const auto &waitFlagMap: waitFlagRecord) {
        // Key is described the flow direction of the pipe.
        std::string detail = waitFlagMap.first;
        // The waitFlagVector stores all wait_flag according to the sequence of instructions.
        auto waitFlagVector = waitFlagMap.second;
        while (!waitFlagVector.empty()) {
            auto wait = waitFlagVector.front();
            std::string id = wait.coreName + "_" + std::to_string(flagMatchId);
            if (setFlagRecord.find(detail) != setFlagRecord.end() && !setFlagRecord[detail].empty()) {
                // collect set_flag and wait_flag instr and flow when can make a pair
                auto set = setFlagRecord[detail].front();
                AddFlag(set, id, coreJson);
                AddFlag(wait, id, coreJson);
                GetFlowEvents(set, wait, id, coreJson);
                setFlagRecord[detail].erase(setFlagRecord[detail].begin());
                waitFlagVector.erase(waitFlagVector.begin());
                flagMatchId++;
                continue;
            }
            // only collect wait_flag instr
            LogDebug("No corresponding set_flag info, flag detail is %s, flag id is %s", detail.c_str(), id.c_str());
            AddFlag(wait, id, coreJson);
            waitFlagVector.erase(waitFlagVector.begin());
            flagMatchId++;
        }
    }
    for (const auto &setFlagMap : setFlagRecord) {
        auto setFlagVector = setFlagMap.second;
        while (!setFlagVector.empty()) {
            // only collect set_flag instr
            auto set = setFlagVector.front();
            std::string id = set.coreName + "_" + std::to_string(flagMatchId);
            LogDebug("No corresponding wait_flag info, flag detail is %s, flag id is %s", setFlagMap.first.c_str(),
                     id.c_str());
            AddFlag(set, id, coreJson);
            setFlagVector.erase(setFlagVector.begin());
            flagMatchId++;
        }
    }
}


void CoreTimeLineVisualizer::AddFlag(const SetWaitFlag &flag, const std::string &id,
    std::vector<nlohmann::json> &coreJson)
{
    int startCycle = flag.instr.startTick;
    if (flag.instr.name == setFlagName_) {
        startCycle = flag.instr.endTick - 1;
    }
    FlagEvent flagBeginEvent = {flag.instr.name, GetCNameByPipe(flag.instr.name), "B",
                                GetMicrosecond(chipType_, startCycle, FLOW_AND_EVENT_ROUND_PARAM),
                                flag.coreName, flag.instr.pipe, {}, id};
    flagBeginEvent.args["pc_addr"] = flag.evtArgs.pcAddr;
    flagBeginEvent.args["code"] = flag.evtArgs.code;
    flagBeginEvent.args["detail"] = flag.evtArgs.detail;

    FlagEvent flagEndEvent = {flag.instr.name, GetCNameByPipe(flag.instr.name), "E",
                              GetMicrosecond(chipType_, flag.instr.endTick, FLOW_AND_EVENT_ROUND_PARAM),
                              flag.coreName, flag.instr.pipe, {}, id};
    nlohmann::json setFlagBeginJson;
    nlohmann::json setFlagEndJson;
    flagBeginEvent.ToJson(setFlagBeginJson);
    flagEndEvent.ToJson(setFlagEndJson);
    if (flag.instr.name == waitFlagName_ && AddScalarHeadEvents(flag, id, coreJson)) {
        setFlagBeginJson["group_id"] = id;
        setFlagEndJson["group_id"] = id;
    }
    coreJson.emplace_back(setFlagBeginJson);
    coreJson.emplace_back(setFlagEndJson);
}

void CoreTimeLineVisualizer::GetFlowEvents(SetWaitFlag &begin, SetWaitFlag &end, std::string &id,
    std::vector<nlohmann::json> &coreJson) const
{
    // Obtains the specific pipe flow direction for classified display.
    std::string cat;
    std::string flowStartTid = begin.instr.pipe;
    std::string flowEndTid = end.instr.pipe;
    cat = flowStartTid + "To" + flowEndTid;

    // Get begin and end flow event
    float beginTime = GetMicrosecond(chipType_, begin.instr.endTick, FLOW_AND_EVENT_ROUND_PARAM);
    float endTime = GetMicrosecond(chipType_, end.instr.endTick, FLOW_AND_EVENT_ROUND_PARAM);
    FlowEvent event4FlowBegin = {"flow", id, "s", beginTime, begin.coreName, flowStartTid, cat};
    FlowEvent event4FlowEnd = {"flow", id, "t", endTime, end.coreName, flowEndTid, cat};

    nlohmann::json flowBegin;
    nlohmann::json flowEnd;
    event4FlowBegin.ToJson(flowBegin);
    event4FlowEnd.ToJson(flowEnd);
    coreJson.emplace_back(flowBegin);
    coreJson.emplace_back(flowEnd);
}

bool CoreTimeLineVisualizer::AddScalarHeadEvents(
    const SetWaitFlag &flag, const std::string &groupId, std::vector<nlohmann::json> &json) const {
    if (flag.instr.icacheTick == UINT64_MAX) {
        return false;
    }
    int durationCycle = flag.instr.ccuTick - flag.instr.icacheTick;
    Event scalarEvent = {"cache_time", GetCNameByPipe(flag.instr.name), "X",
        GetMicrosecond(chipType_, flag.instr.icacheTick, -1), GetMicrosecond(chipType_, durationCycle, -1),
        flag.coreName, flag.instr.pipe, {}};
    scalarEvent.args["pc_addr"] = flag.evtArgs.pcAddr;
    scalarEvent.args["code"] = flag.evtArgs.code;
    scalarEvent.args["detail"] = flag.evtArgs.detail;

    nlohmann::json jsonData;
    scalarEvent.ToJson(jsonData);
    jsonData["group_id"] = groupId;
    json.emplace_back(jsonData);

    durationCycle = flag.instr.startTick - flag.instr.ccuTick;
    Event ccuEvent = {"ccu_time", GetCNameByPipe(flag.instr.name), "X",
        GetMicrosecond(chipType_, flag.instr.ccuTick, -1), GetMicrosecond(chipType_, durationCycle, -1), flag.coreName,
        flag.instr.pipe, {}};
    ccuEvent.args["pc_addr"] = flag.evtArgs.pcAddr;
    ccuEvent.args["code"] = flag.evtArgs.code;
    ccuEvent.args["detail"] = flag.evtArgs.detail;

    ccuEvent.ToJson(jsonData);
    jsonData["group_id"] = groupId;
    json.emplace_back(jsonData);
    return true;
}

bool CoreTimeLineVisualizer::AddScalarHeadEvents(
    const MergeInfo &instr, const std::string &groupId, const Event &event, std::vector<nlohmann::json> &json) const {
    if (instr.icacheTick == UINT64_MAX || instr.name == "BAR" || instr.pipe == "CACHEMISS") {
        return false;
    }
    int durationCycle = instr.ccuTick - instr.icacheTick;
    Event scalarEvent = {"cache_time", event.cName, "X", GetMicrosecond(chipType_, instr.icacheTick, -1),
        GetMicrosecond(chipType_, durationCycle, -1), event.pid, event.tid, {}};
    scalarEvent.args = event.args;
    nlohmann::json jsonData;
    scalarEvent.ToJson(jsonData);
    jsonData["group_id"] = groupId;
    json.emplace_back(jsonData);

    durationCycle = instr.startTick - instr.ccuTick;
    Event ccuEvent = {"ccu_time", event.cName, "X", GetMicrosecond(chipType_, instr.ccuTick, -1),
        GetMicrosecond(chipType_, durationCycle, -1), event.pid, event.tid, {}};
    ccuEvent.args = event.args;
    ccuEvent.ToJson(jsonData);
    jsonData["group_id"] = groupId;
    json.emplace_back(jsonData);
    return true;
}

void CoreTimeLineVisualizer::GenerateEvent(const MergeInfo &instr, const EventArgs &evtArgs,
    const std::string &coreName, const std::string &groupId, std::vector<nlohmann::json> &json) const {
    uint64_t startCycle = instr.startTick;
    uint64_t durationCycle = (instr.endTick >= instr.startTick) ? instr.endTick - instr.startTick : 0;
    if (instr.name == "BAR") {
        startCycle = instr.endTick >= 1 ? instr.endTick - 1 : 0;
        durationCycle = 1;
    }
    Event event = {instr.name, "", "X", GetMicrosecond(chipType_, startCycle, -1),
        GetMicrosecond(chipType_, durationCycle, -1), coreName, instr.pipe, {}};
    if (instr.warpId != DEFAULT_INT_VALUE && instr.schId != DEFAULT_INT_VALUE) {
        event.tid = "WARP_" + std::to_string(instr.warpId);
        event.cName = GetCNameByInstrName(instr.name);
    } else {
        event.cName = GetCNameByPipe(instr.pipe);
    }
    event.args["pc_addr"] = evtArgs.pcAddr;
    event.args["code"] = evtArgs.code;
    event.args["detail"] = evtArgs.detail;
    if (instr.processBytes != -1 && instr.processBytes != 0) {
        event.args["process_bytes"] = std::to_string(instr.processBytes);
    }
    nlohmann::json jsonData;
    event.ToJson(jsonData);
    if (AddScalarHeadEvents(instr, groupId, event, json)) {
        jsonData["group_id"] = groupId;
    }
    json.emplace_back(jsonData);
}

void CoreTimeLineVisualizer::CollectUserMarkEvents(const std::string &coreName, const SimData &data,
    std::vector<nlohmann::json> &coreJson)
{
    auto userMarkPtr = data.userMarks;
    if (userMarkPtr == nullptr) {
        return;
    }
    const std::vector<MergeInfo> &instrs = userMarkPtr->userMarkInstrs;
    std::map<std::string, std::vector<UserMarkInfo>> &markInfos = userMarkPtr->userMarkInfos;
    std::map<std::string, uint32_t> markCnt;
    for (size_t i = 0; i < instrs.size(); ++i) {
        const MergeInfo& tmpInstr = instrs[i];
        size_t markId = markCnt[tmpInstr.name];
        if (markInfos.count(tmpInstr.name) == 0 || markId > markInfos[tmpInstr.name].size()) {
            LogError("USERMARK id is error. id:%zu, size:%lld", markId,
                     markInfos[tmpInstr.name].size());
            continue;
        }
        Profiling::UserMarkInfo markInfo = markInfos[tmpInstr.name][markId];
        std::string startCode = "";
        std::string stopCode = "";
        if (pc2code_.Find(markInfo.startPc) && pc2code_.Find(markInfo.endPc)) {
            const std::vector<std::string> &startCodeStack = pc2code_[markInfo.startPc];
            const std::vector<std::string> &stopCodeStack = pc2code_[markInfo.endPc];
            startCode = Utility::Join(startCodeStack.begin(), startCodeStack.end(), "\n");
            stopCode = Utility::Join(stopCodeStack.begin(), stopCodeStack.end(), "\n");
        }

        // get x event instr info
        uint64_t startCycle = tmpInstr.startTick;
        uint64_t durationCycle = (tmpInstr.endTick >= tmpInstr.startTick) ? tmpInstr.endTick - tmpInstr.startTick : 0;
        Event event = {instrs[i].name, GetCNameByPipe(instrs[i].pipe), "X",
                       GetMicrosecond(chipType_, startCycle),
                       GetMicrosecond(chipType_, durationCycle), coreName, instrs[i].pipe, {}};
        event.args["trace_start"] = startCode;
        event.args["trace_stop"] = stopCode;

        nlohmann::json jsonData;
        event.ToJson(jsonData);
        coreJson.emplace_back(jsonData);
        markCnt[tmpInstr.name]++;
    }
}
}
}
