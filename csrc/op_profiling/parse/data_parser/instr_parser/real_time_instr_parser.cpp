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


#include "real_time_instr_parser.h"

#include <utility>
#include "smart_pointer.h"
#include "log.h"
namespace Profiling {
namespace Parse {
using namespace Utility;
PluginErrorCode RealTimeInstrParserPlugin::Entry()
{
    instrLogParsers_.clear();
    auto ptr = dataCenter_.GetStreamPtr<InstrParseInfoForRealTime>();
    LogDebug("Instr real time plugin entry");
    SetEntry(true);
    while (ptr != nullptr) {
        InstrParseInfoForRealTime instrInfo = ptr->Pop();
        if (ptr->IsStop()) {
            LogDebug("Instr parser plugin stopped");
            break;
        }
        auto iter = instrLogParsers_.find(instrInfo.coreName);
        if (iter == instrLogParsers_.end()) {
            SimDataParserConfig config = dataParserConfig_;
            config.SetCoreName(instrInfo.coreName);
            InstrLogParser instrLogParser(config);
            instrLogParsers_.insert({instrInfo.coreName, instrLogParser});
            iter = instrLogParsers_.find(instrInfo.coreName);
        }
        iter->second.ParseRealTimeDumpLog(instrInfo);
    }
    SetEntry(false);
    return PluginErrorCode::SUCCESS;
}

PluginErrorCode RealTimePopParserPlugin::Entry()
{
    popLogParsers_.clear();
    auto ptr = dataCenter_.GetStreamPtr<PoppedInstrParseInfoForRealTime>();
    LogDebug("PopInstr real time plugin entry");
    SetEntry(true);
    while (ptr != nullptr) {
        PoppedInstrParseInfoForRealTime instrInfo = ptr->Pop();
        if (ptr->IsStop()) {
            LogDebug("Pop Instr parser plugin stopped");
            break;
        }
        std::lock_guard<std::mutex> lock(mtx_);
        auto iter = popLogParsers_.find(instrInfo.coreName);
        if (iter == popLogParsers_.end()) {
            SimDataParserConfig config = dataParserConfig_;
            config.SetCoreName(instrInfo.coreName);
            PopLogParser popLogParser(config);
            popLogParsers_.insert({instrInfo.coreName, popLogParser});
            iter = popLogParsers_.find(instrInfo.coreName);
        }
        iter->second.ParseRealTimeDumpLog(instrInfo);
    }
    SetEntry(false);
    return PluginErrorCode::SUCCESS;
}

bool RealTimeInstrParser::IsSkipSetLog(const std::string &coreName)
{
    if (realTimePopParserPlugin_ == nullptr) {
        LogWarn("Pop parser plugin is null, skip add log");
        return true;
    }
    std::lock_guard<std::mutex> lock(realTimePopParserPlugin_->mtx_);
    auto iter = realTimePopParserPlugin_->popLogParsers_.find(coreName);
    if (iter != realTimePopParserPlugin_->popLogParsers_.end() && iter->second.IsGetCoreId()) {
        std::set<int> parseIds = context_.parseCoreId;
        int coreId = iter->second.GetCoreId();
        if (!parseIds.empty() && parseIds.find(coreId) == parseIds.end()) {
            return true;
        }
    }
    return false;
}

void RealTimeInstrParser::SetInstrLog(const Profiling::InstrParseInfoForRealTime &instrParseInfo)
{
    auto ptr = dataCenter_.GetStreamPtr<InstrParseInfoForRealTime>();
    if (ptr == nullptr) {
        LogWarn("cannot find InstrParseInfoForRealTime in data center");
        return;
    }
    if (IsSkipSetLog(instrParseInfo.coreName)) {
        return;
    }
    ptr->Push(instrParseInfo);
}

void RealTimeInstrParser::SetPopInstrLog(const Profiling::PoppedInstrParseInfoForRealTime &popParseInfo)
{
    auto ptr = dataCenter_.GetStreamPtr<PoppedInstrParseInfoForRealTime>();
    if (ptr == nullptr) {
        LogWarn("Cannot find PoppedInstrParseInfoForRealTime in data center");
        return;
    }
    if (IsSkipSetLog(popParseInfo.coreName)) {
        return;
    }
    ptr->Push(popParseInfo);
}

// thread number is 2
RealTimeInstrParser::RealTimeInstrParser(RealTimeSimParseContext context) : RealTimeLogParer(std::move(context), 2)
{
    realTimeInstrParserPlugin_ = std::make_shared<RealTimeInstrParserPlugin>(dataCenter_,
        SimDataParserConfig("", context_.parseCoreId, context_.enableResourceConflictRatio));
    realTimePopParserPlugin_ = std::make_shared<RealTimePopParserPlugin>(dataCenter_,
        SimDataParserConfig("", context_.parseCoreId, context_.enableResourceConflictRatio));
    pluginManager_.AddPlugin(realTimePopParserPlugin_);
    pluginManager_.AddPlugin(realTimeInstrParserPlugin_);
}

void RealTimeInstrParser::Start()
{
    dataCenter_.Clear();
    dataCenter_.DataStreamRegister<InstrParseInfoForRealTime>();
    dataCenter_.DataStreamRegister<PoppedInstrParseInfoForRealTime>();
    pluginManager_.RunAllPluginsNoBlock();
}

void RealTimeInstrParser::Merge(std::map<std::string, std::shared_ptr<Profiling::Parse::DataCenter>> &dateCenterMap)
{
    if (realTimePopParserPlugin_ == nullptr || realTimeInstrParserPlugin_ == nullptr) { return; }
    auto &instrLogParser = realTimeInstrParserPlugin_->instrLogParsers_;
    for (const auto &popParser : realTimePopParserPlugin_->popLogParsers_) {
        const std::set<int> &parseIds = context_.parseCoreId;
        if (!parseIds.empty() && parseIds.find(popParser.second.GetCoreId()) == parseIds.end()) {
            continue;
        }
        if (instrLogParser.find(popParser.first) == instrLogParser.end()) {
            LogWarn("Cannot find instr log parser, core name is %s", popParser.first.c_str());
            continue;
        }
        std::string logiName = popParser.second.GetLogicalName();
        // 逻辑核获取不到时，需要使用物理核代替
        if (logiName.empty()) { logiName = popParser.first; }
        instrLogParser.at(popParser.first).SetCoreName(logiName);
        auto popPtr = MakeShared<PopLogParser>(popParser.second);
        auto instrPtr = MakeShared<InstrLogParser>(instrLogParser.at(popParser.first));
        if (popPtr == nullptr || instrPtr == nullptr) {
            LogError("Merge failed, instr or pop is null");
            return;
        }
        auto dataCenter = Utility::MakeShared<Profiling::Parse::DataCenter>();
        if (dataCenter == nullptr) {
            LogWarn("Failed to create datacenter for %s", logiName.c_str());
            continue;
        }
        dataCenter->DataTableRegister<InstrLogParser>(instrPtr);
        dataCenter->DataTableRegister<PopLogParser>(popPtr);
        auto coreNamePtr = Utility::MakeShared<PhysicalAndLogicalPair>(popParser.first, logiName);
        if (coreNamePtr == nullptr) {
            LogWarn("Failed to create coreNamePtr");
            return;
        }
        dataCenter->DataTableRegister<PhysicalAndLogicalPair>(coreNamePtr);
        dateCenterMap.emplace(logiName, dataCenter);
    }
    if (dateCenterMap.empty()) {
        LogWarn("Merge finish, but data center is null");
        return;
    }
    auto poolSize = static_cast<uint32_t>(std::thread::hardware_concurrency() * MAX_THREAD_USAGE_RATIO);
    auto threadNum = (dateCenterMap.size() < poolSize) ? dateCenterMap.size() : poolSize;
    Profiling::Parse::PluginManager pluginManager(threadNum);
    for (auto& iter : dateCenterMap) {
        SimDataParserConfig dataParserConfig {iter.first, context_.parseCoreId, context_.enableResourceConflictRatio};
        pluginManager.AddPlugin<Profiling::Parse::RealTimeInstrMergeParser>(*iter.second, dataParserConfig);
    }
    std::vector<PluginErrorCode> res;
    pluginManager.RunAllPlugins(res);
}

PluginErrorCode RealTimeInstrMergeParser::Entry()
{
    auto popLogParserPtr = dataCenter_.GetDbPtr<PopLogParser>();
    auto instrLogParserPtr = dataCenter_.GetDbPtr<InstrLogParser>();
    if (popLogParserPtr == nullptr || instrLogParserPtr == nullptr) {
        LogWarn("Merge failed in core: %s", coreName_.c_str());
        return PluginErrorCode::NONBLOCKING_ERROR;
    }
    instrLogParserPtr->DisposeUserMark();
    MergeLog(*instrLogParserPtr, *popLogParserPtr);
    return PluginErrorCode::SUCCESS;
}
}
}