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


#include "subcore_timeline_visualizer.h"
#include "parse/data_visualizer/utility.h"
#include "parse/data_visualizer/timeline_visualizer/timeline_defs.h"
#include "ustring.h"
#include "filesystem.h"
#include "thread_pool.h"

using namespace Utility;

namespace Profiling {
namespace Parse {

struct MetaDataEvent {
    std::string name;
    std::string ph = "M";
    int pid;
    int tid = 0;
};

struct MetaDataNameEvent {
    inline void ToJson(nlohmann::json &jsonData) const
    {
        jsonData["name"] = this->metaDataEvent.name;
        jsonData["ph"] = this->metaDataEvent.ph;
        jsonData["pid"] = this->metaDataEvent.pid;
        jsonData["tid"] = this->metaDataEvent.tid;
        jsonData["args"] = this->args;
    }
    MetaDataEvent metaDataEvent;
    std::map<std::string, std::string> args;
};

struct MetaDataIndexEvent {
    inline void ToJson(nlohmann::json &jsonData) const
    {
        jsonData["name"] = this->metaDataEvent.name;
        jsonData["ph"] = this->metaDataEvent.ph;
        jsonData["pid"] = this->metaDataEvent.pid;
        jsonData["tid"] = this->metaDataEvent.tid;
        jsonData["args"] = this->args;
    }
    MetaDataEvent metaDataEvent;
    std::map<std::string, int> args;
};

PluginErrorCode SubcoreTimelineVisualizer::Entry()
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
        if (data.instrs == nullptr) {
            LogError("Failed to generate split core timeline for %s", coreName.c_str());
            continue;
        }
        pool.AddTask([this, &coreNameWithData]() {
            ParseByCore(coreNameWithData.first, coreNameWithData.second);
        });
    }
    pool.WaitAllTasks();
    pool.Stop();
    return PluginErrorCode::SUCCESS;
}

bool SubcoreTimelineVisualizer::ParseByCore(const std::string &coreName, SimData &data)
{
    std::set<std::string> pipeSet;
    std::vector<nlohmann::json> singleSplitCoreJsonList;
    std::string coreTraceFilePath = Utility::JoinPath({dataVisualizerConfig_.GetOutputPath(), coreName, "trace.json"});

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
    CollectInstrEvents4SepTrace(instrs, singleSplitCoreJsonList, pipeSet);

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
        CollectInstrEvents4SepTrace(cache, singleSplitCoreJsonList, pipeSet);
    }
    CollectUserMarkEvents(data, singleSplitCoreJsonList);

    // write single core json
    if (WriteSepJson(coreTraceFilePath, singleSplitCoreJsonList)) {
        LogDebug("Save trace json of %s in %s", coreName.c_str(), coreTraceFilePath.c_str());
        return true;
    } else {
        LogError("write json of %s failed", coreName.c_str());
        return false;
    }
}

void SubcoreTimelineVisualizer::CollectInstrEvents4SepTrace(std::vector<MergeInfo> &instrs,
    std::vector<nlohmann::json> &coreJsonList, std::set<std::string> &pipeSet)
{
    std::map<int, std::vector<MergeInfo>> instrsByPipe;
    std::map<int, std::vector<uint64_t>> preInstrOfPipes;
    std::set<int> warpSet;
    std::map<int, std::vector<MergeInfo>> simtInstrByWarp;
    // setFlagRecord_ store flags in the time sequence of instructions based on the pipe flow direction.
    // setFlagRecord_ key describes the flow direction of the pipe.eg. "PIPE:SCALAR,TRIGGERPIPE:MTE3,FLAGID:0,"
    std::map<std::string, std::vector<XEvent>> setFlagRecord;
    // waitFlagRecord_ store flags in the time sequence of instructions based on the pipe flow direction.
    std::map<std::string, std::vector<XEvent>> waitFlagRecord;
    std::map<std::string, uint64_t> pipeEndTime;
    for (auto &instr: instrs) {
        if (instr.warpId != DEFAULT_INT_VALUE && instr.schId != DEFAULT_INT_VALUE) {
            int tmpPid = pidMap_["WARP"] + instr.warpId;
            if (warpSet.find(instr.warpId) == warpSet.end()) {
                warpSet.insert(instr.warpId);
                AddProcessMetaData(coreJsonList, "WARP_" + std::to_string(instr.warpId), tmpPid);
            }
            instr.pipe = "WARP_" + std::to_string(instr.warpId);
            simtInstrByWarp[tmpPid].emplace_back(instr);
            continue;
        }
        if (instr.pipe == "SCALAR" && instr.detail.find("XD:") != std::string::npos) {
            instr.pipe = "SCALARLDST";
        }
        auto endPipe = pipeEndTime.find(instr.pipe);
        if (instr.name == waitFlagName_ && endPipe != pipeEndTime.end()) {
            uint64_t endTime = pipeEndTime.at(instr.pipe);
            if (instr.startTick < endTime && instr.endTick > endTime) {
                instr.startTick = endTime;
            }
        }
        if (endPipe == pipeEndTime.end() || instr.endTick > endPipe->second) {
            pipeEndTime[instr.pipe] = instr.endTick;
        }
        int tmpPid = pidMap_[instr.pipe];
        if (pipeSet.find(instr.pipe) == pipeSet.end()) {
            pipeSet.insert(instr.pipe);
            AddProcessMetaData(coreJsonList, instr.pipe, pidMap_[instr.pipe]);
            preInstrOfPipes[tmpPid] = {0, 0};
        }
        if (instr.endTick == preInstrOfPipes[tmpPid][1] && instr.startTick > preInstrOfPipes[tmpPid][0]) {
            instrsByPipe[tmpPid].insert(instrsByPipe[tmpPid].end() - 1, instr);
        } else {
            instrsByPipe[tmpPid].emplace_back(instr);
        }
        preInstrOfPipes[tmpPid] = {instr.startTick, instr.endTick};
    }
    CollectEvents(instrsByPipe, coreJsonList, setFlagRecord, waitFlagRecord);
    CollectSIMTEvents(simtInstrByWarp, coreJsonList);
    CollectFlagAndFlowEventsForSep(coreJsonList, setFlagRecord, waitFlagRecord);
}

void SubcoreTimelineVisualizer::CollectSIMTEvents(std::map<int, std::vector<MergeInfo>> &instrsGroup,
    std::vector<nlohmann::json> &coreJsonList)
{
    for (const auto &instrPair : instrsGroup) {
        const std::vector<MergeInfo> &pipeInstrs = instrPair.second;
        std::vector<std::vector<int>> warpOccupy;
        for (size_t i = 0; i < pipeInstrs.size(); ++i) {
            MergeInfo tmpInstr = pipeInstrs[i];
            std::vector<std::string> codeStack {};
            if (pc2code_.Find(tmpInstr.pc)) {
                codeStack = pc2code_[tmpInstr.pc];
            }
            tmpInstr.pipe = "WARP_" + std::to_string(tmpInstr.warpId);
            // get tid
            uint64_t maxCycle = pipeInstrs.back().endTick;
            int tid = GetTid(tmpInstr, maxCycle, warpOccupy, coreJsonList, pidMap_["WARP"] + tmpInstr.warpId);
            // get x event instr info
            uint64_t startCycle = tmpInstr.startTick;
            uint64_t durationCycle = tmpInstr.endTick > tmpInstr.startTick ? tmpInstr.endTick - tmpInstr.startTick : 0;
            XEvent xEvent = {tmpInstr.name, "X", pidMap_["WARP"] + tmpInstr.warpId, tid,
                             GetMicrosecond(chipType_, startCycle),
                             GetMicrosecond(chipType_, durationCycle), GetCNameByInstrName(tmpInstr.name), {}};
            EventArgs evtArgs = {GetPc2String(tmpInstr.pc), Utility::Join(codeStack.begin(), codeStack.end(), "\n"),
                                 tmpInstr.detail};
            xEvent.args["pc_addr"] = evtArgs.pcAddr;
            xEvent.args["code"] = evtArgs.code;
            xEvent.args["detail"] = evtArgs.detail;
            nlohmann::json jsonData;
            xEvent.ToJson(jsonData);
            coreJsonList.emplace_back(jsonData);
        }
    }
}

bool SubcoreTimelineVisualizer::AddScalarHeadEvents(const MergeInfo &instr, const std::string &groupId, const XEvent &event, std::vector<nlohmann::json> &json) const
{
    if (instr.icacheTick == UINT64_MAX || instr.name == "BAR" || instr.pipe == "CACHEMISS" || instr.name == setFlagName_) {
        return false;
    }
    int durationCycle = instr.ccuTick - instr.icacheTick;
    XEvent scalarEvent = {"cache_time", "X", event.pid, event.tid,
        GetMicrosecond(chipType_, instr.icacheTick, -1),
        GetMicrosecond(chipType_, durationCycle, -1), event.cName, event.args};

    nlohmann::json jsonData;
    scalarEvent.ToJson(jsonData);
    jsonData["group_id"] = groupId;
    json.emplace_back(jsonData);

    durationCycle = instr.startTick - instr.ccuTick;
    XEvent ccuEvent = {"ccu_time", "X", event.pid, event.tid,
        GetMicrosecond(chipType_, instr.ccuTick, -1),
        GetMicrosecond(chipType_, durationCycle, -1), event.cName, event.args};

    ccuEvent.ToJson(jsonData);
    jsonData["group_id"] = groupId;
    json.emplace_back(jsonData);
    return true;
}

void SubcoreTimelineVisualizer::CollectEvents(const std::map<int, std::vector<MergeInfo>> &instrsGroup,
    std::vector<nlohmann::json> &coreJsonList, std::map<std::string, std::vector<XEvent>> &setFlagRecord,
    std::map<std::string, std::vector<XEvent>> &waitFlagRecord)
{
    for (const auto &instrPair : instrsGroup) {
        const std::vector<MergeInfo> &pipeInstrs = instrPair.second;
        uint64_t maxCycle = pipeInstrs.back().endTick;
        std::vector<std::vector<int>> pipeOccupy;
        for (size_t i = 0; i < pipeInstrs.size(); ++i) {
            MergeInfo tmpInstr = pipeInstrs[i];
            std::vector<std::string> codeStack {};
            if (pc2code_.Find(tmpInstr.pc)) {
                codeStack = pc2code_[tmpInstr.pc];
            }
            if (tmpInstr.name == "BAR") {
                tmpInstr.startTick = tmpInstr.endTick - 1;
            }
            // get tid
            int tid = GetTid(tmpInstr, maxCycle, pipeOccupy, coreJsonList, pidMap_[tmpInstr.pipe]);
            // get x event instr info
            uint64_t startCycle = tmpInstr.startTick;
            uint64_t durationCycle = tmpInstr.endTick - tmpInstr.startTick;
            XEvent xEvent = {tmpInstr.name, "X", pidMap_[tmpInstr.pipe], tid, GetMicrosecond(chipType_, startCycle, -1),
                GetMicrosecond(chipType_, durationCycle, -1), GetCNameByPipe(tmpInstr.pipe), {}};
            EventArgs evtArgs = {GetPc2String(tmpInstr.pc), Utility::Join(codeStack.begin(), codeStack.end(), "\n"),
                tmpInstr.detail};
            xEvent.args["pc_addr"] = evtArgs.pcAddr;
            xEvent.args["code"] = evtArgs.code;
            xEvent.args["detail"] = evtArgs.detail;
            if (tmpInstr.processBytes != -1 && tmpInstr.processBytes != 0) {
                xEvent.args["process_bytes"] = std::to_string(tmpInstr.processBytes);
            }

            // storage all set_flag and wait_flag
            if (tmpInstr.name == setFlagName_) {
                // not round here for precision of flow event
                xEvent.ts = GetMicrosecond(chipType_, tmpInstr.endTick - 1, FLOW_AND_EVENT_ROUND_PARAM);
                xEvent.dur = GetMicrosecond(chipType_, 1, FLOW_AND_EVENT_ROUND_PARAM);
                xEvent.cName = GetCNameByPipe(setFlagName_);
                setFlagRecord[tmpInstr.detail].emplace_back(xEvent);
            }
            if (tmpInstr.name == waitFlagName_) {
                xEvent.ts = GetMicrosecond(chipType_, startCycle, FLOW_AND_EVENT_ROUND_PARAM);
                xEvent.dur = GetMicrosecond(chipType_, durationCycle, FLOW_AND_EVENT_ROUND_PARAM);
                xEvent.cName = GetCNameByPipe(waitFlagName_);
                waitFlagRecord[tmpInstr.detail].emplace_back(xEvent);
            }
            nlohmann::json jsonData;
            xEvent.ToJson(jsonData);
            std::string groupId = tmpInstr.pipe + std::to_string(i);
            if (AddScalarHeadEvents(tmpInstr, groupId, xEvent, coreJsonList)) {
                jsonData["group_id"] = groupId;
            }
            coreJsonList.emplace_back(jsonData);
        }
    }
}

int SubcoreTimelineVisualizer::GetUserMarkTid(const MergeInfo &instr, std::map<std::string, bool> &userMarkTidMap,
    std::vector<nlohmann::json> &coreJsonList)
{
    int tid = instr.name.back() - '0';
    if (!userMarkTidMap.empty() && userMarkTidMap.count(instr.name)) {
        return tid;
    }
    userMarkTidMap[instr.name] = true;
    AddThreadMetaData(coreJsonList, tid, instr.pipe  + "_" + std::to_string(tid), pidMap_[instr.pipe]);
    return tid;
}

void SubcoreTimelineVisualizer::CollectUserMarkEvents(const SimData &data, std::vector<nlohmann::json> &coreJsonList)
{
    auto userMarkPtr = data.userMarks;
    if (userMarkPtr == nullptr) {
        return;
    }
    AddProcessMetaData(coreJsonList, Profiling::USER_MARK, pidMap_[Profiling::USER_MARK]);
    const std::vector<MergeInfo> &instrs = userMarkPtr->userMarkInstrs;
    std::map<std::string, std::vector<UserMarkInfo>> &markInfos = userMarkPtr->userMarkInfos;
    std::map<std::string, uint32_t> markCnt;
    std::map<std::string, bool> userMarkTidMap;
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
        XEvent xEvent;

        // get tid
        int tid = GetUserMarkTid(tmpInstr, userMarkTidMap, coreJsonList);

        // get x event instr info
        int startCycle = tmpInstr.startTick;
        int durationCycle = tmpInstr.endTick - tmpInstr.startTick;
        xEvent = {tmpInstr.name, "X", pidMap_[tmpInstr.pipe], tid, GetMicrosecond(chipType_, startCycle),
                  GetMicrosecond(chipType_, durationCycle), GetCNameByPipe(tmpInstr.pipe), {}};
        xEvent.args["trace_start"] = startCode;
        xEvent.args["trace_stop"] = stopCode;
        nlohmann::json jsonData;
        xEvent.ToJson(jsonData);
        coreJsonList.emplace_back(jsonData);
        markCnt[tmpInstr.name]++;
    }
}

int SubcoreTimelineVisualizer::GetTid(const MergeInfo &instr, uint64_t maxCycle,
    std::vector<std::vector<int>> &pipeOccupy, std::vector<nlohmann::json> &coreJsonList, int pid)
{
    int curThreadNum = static_cast<int>(pipeOccupy.size());
    int tid = 0;
    if (FindAvailableThread(curThreadNum, instr, pipeOccupy, tid)) {
        return tid;
    }
    // if not find or curThreadNum == 0, set additional thread
    tid = curThreadNum + 1;
    AddThreadMetaData(coreJsonList, tid, instr.pipe + "_" + std::to_string(tid), pid);
    if (maxCycle > INT32_MAX) {
        return tid;
    }
    std::vector<int> threadOccupyNew(maxCycle, 0);
    for (uint64_t i = instr.startTick; (i < instr.endTick + 1) && (i < threadOccupyNew.size()); ++i) {
        threadOccupyNew[i]++;
    }
    pipeOccupy.emplace_back(threadOccupyNew);
    return tid;
}

bool SubcoreTimelineVisualizer::FindAvailableThread(int curThreadNum, const MergeInfo &instr,
    std::vector<std::vector<int>> &pipeOccupy, int &tid) const
{
    for (int j = 0; j < curThreadNum; ++j) {
        auto &curThread = pipeOccupy[j];
        // find thread that is available
        if (instr.endTick < instr.startTick) {
            continue;
        }
        uint64_t start = std::min(static_cast<uint64_t>(instr.startTick), static_cast<uint64_t>(curThread.size()));
        uint64_t end = std::min(static_cast<uint64_t>(instr.endTick + 1), static_cast<uint64_t>(curThread.size()));
        if (std::accumulate(curThread.begin() + start, curThread.begin() + end, 0) == 0) {
            tid = j + 1;
            // occupy cur thread
            for_each(curThread.begin() + start, curThread.begin() + end, [](int &k) { k += 1; });
            return true;
        }
    }
    return false;
}

void SubcoreTimelineVisualizer::AddProcessMetaData(std::vector<nlohmann::json> &coreJsonList, const std::string &pipe,
    int pid) const
{
    MetaDataEvent metaDataEvent;
    metaDataEvent.name = "process_name";
    metaDataEvent.pid = pid;
    MetaDataNameEvent processNameEvent;
    processNameEvent.metaDataEvent = metaDataEvent;
    processNameEvent.args["name"] = pipe;
    MetaDataIndexEvent processIndexEvent;
    metaDataEvent.name = "process_sort_index";
    processIndexEvent.metaDataEvent = metaDataEvent;
    processIndexEvent.args["sort_index"] = pid;
    nlohmann::json processNameJson;
    nlohmann::json processIndexJson;
    processNameEvent.ToJson(processNameJson);
    processIndexEvent.ToJson(processIndexJson);
    coreJsonList.emplace_back(processNameJson);
    coreJsonList.emplace_back(processIndexJson);
}

void SubcoreTimelineVisualizer::AddThreadMetaData(std::vector<nlohmann::json> &coreJsonList, int threadIndex,
    const std::string &threadName, int pid) const
{
    MetaDataEvent metaDataEvent;
    metaDataEvent.name = "thread_name";
    metaDataEvent.pid = pid;
    metaDataEvent.tid = threadIndex;
    MetaDataNameEvent threadNameEvent;
    threadNameEvent.metaDataEvent = metaDataEvent;
    threadNameEvent.args["name"] = threadName;
    MetaDataIndexEvent threadIndexEvent;
    metaDataEvent.name = "thread_sort_index";
    threadIndexEvent.metaDataEvent = metaDataEvent;
    threadIndexEvent.args["sort_index"] = threadIndex;
    nlohmann::json threadNameJson;
    nlohmann::json threadIndexJson;
    threadNameEvent.ToJson(threadNameJson);
    threadIndexEvent.ToJson(threadIndexJson);
    coreJsonList.emplace_back(threadNameJson);
    coreJsonList.emplace_back(threadIndexJson);
}

bool SubcoreTimelineVisualizer::WriteSepJson(const std::string &filePath,
    std::vector<nlohmann::json> &coreJsonList) const
{
    nlohmann::json fileJson;
    fileJson["schemaVersion"] = 1;
    fileJson["displayTimeUnit"] = "ns";
    fileJson["profilingType"] = "op";
    fileJson["traceEvents"] = coreJsonList;
    if (!WriteFileByStream(filePath, fileJson)) {
        return false;
    }
    return true;
}

void SubcoreTimelineVisualizer::CollectFlagAndFlowEventsForSep(std::vector<nlohmann::json> &coreJsonList,
    std::map<std::string, std::vector<XEvent>> &setFlagRecord,
    std::map<std::string, std::vector<XEvent>> &waitFlagRecord)
{
    int waitFlagId = 0;
    for (const auto &record: waitFlagRecord) {
        // Key is described the flow direction of the pipe.
        const std::string &key = record.first;
        // The waitFlagVector stores all wait_flag according to the sequence of instructions.
        const auto &waitFlagVector = record.second;
        for (uint32_t i = 0; i < waitFlagVector.size(); i++) {
            if (setFlagRecord.find(key) == setFlagRecord.end() || setFlagRecord[key].size() <= i) {
                LogDebug("No corresponding sepcore flag info, flag detail is %s", key.c_str());
                return ;
            }
            GetFlowEventsForSeq(setFlagRecord[key][i], waitFlagVector[i], coreJsonList, waitFlagId);
        }
    }
}

void SubcoreTimelineVisualizer::GetFlowEventsForSeq(const XEvent &begin, const XEvent &end,
    std::vector<nlohmann::json> &coreJsonList, int &waitFlagId)
{
    // Obtains the specific pipe flow direction for classified display.
    std::string cat = flagMap_[begin.pid] + "To" + flagMap_[end.pid];
    // Get begin and end flow event
    FlowEvent event4FlowBegin = {"flow", std::to_string(waitFlagId), "s",
        static_cast<float>(begin.ts + begin.dur / 2.0), std::to_string(begin.pid), std::to_string(begin.tid), cat};
    FlowEvent event4FlowEnd = {"flow", std::to_string(waitFlagId), "t", static_cast<float>(end.ts + end.dur),
        std::to_string(end.pid), std::to_string(end.tid), cat};
    waitFlagId++;

    nlohmann::json flowBegin;
    nlohmann::json flowEnd;
    event4FlowBegin.ToJson(flowBegin);
    event4FlowEnd.ToJson(flowEnd);
    coreJsonList.emplace_back(flowBegin);
    coreJsonList.emplace_back(flowEnd);
}
}
}
