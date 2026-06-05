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


#include "real_time_data_parser.h"
#include <utility>
#include "sim_data_parser.h"
#include "sim_dump_parser.h"
namespace Profiling {
namespace Parse {
using namespace Utility;
const char * const CORE = "core";
const char * const CORE_CUBE = "cubecore";
const char * const CORE_VEC = "veccore";
const std::regex INSTR_PATTERN(
    R"(\(PC: 0x[0-9a-f]{1,16}\)\s*([A-Za-z0-3_]+)\s*\:\s*\(Binary\: 0x[0-9a-f]{8}\)\s*([0-9a-zA-Z_-]*))");
struct PairHash {
public:
    size_t operator()(const std::pair<uint32_t, uint32_t>& p) const
    {
        return (p.second << 5) | p.first; // 2^5 = 32 > 24
    }
};
PluginErrorCode RealTimeICacheParserPlugin::Entry()
{
    cacheInstrMap_.clear();
    auto ptr = dataCenter_.GetStreamPtr<IcacheParseInfoForRealTime>();
    SetEntry(true);
    LogDebug("Cache real time plugin entry");
    while (ptr != nullptr) {
        IcacheParseInfoForRealTime icacheInfo = ptr->Pop();
        if (ptr->IsStop()) {
            break;
        }
        if (icacheInfo.opType == "miss_read") {
            ParseLine(icacheInfo);
        } else {
            ParseScalar(icacheInfo);
        }
    }
    std::map<std::string, Parse::CacheDetailTable> cacheDetailTableMap;
    for (const auto &cacheInstr : cacheInstrMap_) {
        if (cacheInstr.second.empty()) {
            LogDebug("Parser %s cache info is empty because all hit", cacheInstr.first.c_str());
            continue;
        }
        Parse::CacheDetailTable cacheDetailTable(cacheInstr.second);
        cacheDetailTableMap[cacheInstr.first] = std::move(cacheDetailTable);
    }
    auto cacheDetailTableMapPtr =
        Utility::MakeShared<std::map<std::string, Parse::CacheDetailTable>>(cacheDetailTableMap);
    if (!dataCenter_.DataTableRegister(cacheDetailTableMapPtr)) {
        LogWarn("Failed to register cache table");
        return PluginErrorCode::NONBLOCKING_ERROR;
    }
    if (!scalarMap_.empty()) {
        auto scalarTable = Utility::MakeShared<std::map<std::string, scalarHeadCache>>(scalarMap_);
        if (scalarTable != nullptr && !dataCenter_.DataTableRegister(scalarTable)) {
            LogWarn("Failed to register icache scalar table");
        }
    }
    SetEntry(false);
    return PluginErrorCode::SUCCESS;
}

void RealTimeICacheParserPlugin::ParseScalar(const IcacheParseInfoForRealTime &info) {
    for (auto i = 0; i < 4; i++) {
        scalarMap_[info.coreName][info.pc + i * 4].insert(info.tick);
    }
}

void RealTimeICacheParserPlugin::ParseLine(const IcacheParseInfoForRealTime &info)
{
    MergeInfo merge{};
    merge.startTick = info.tick;
    merge.endTick = info.tick + 1;
    merge.pc = info.pc;
    merge.pipe = "CACHEMISS";
    std::ostringstream oss;
    // In hexadecimal format, 8 is reserved for display.
    oss << "0x" << std::hex << std::nouppercase << std::setw(8) << std::setfill('0') << info.pc;
    merge.name = oss.str();
    merge.detail = info.detail;
    Utility::TrimBlank(merge.detail);
    merge.gprCount = 0;
    merge.ubWriteConflict = DEFAULT_INT_VALUE;
    merge.ubReadConflict = DEFAULT_INT_VALUE;
    merge.vecUtilization = static_cast<float>(DEFAULT_INT_VALUE);
    merge.processBytes = DEFAULT_INT_VALUE;
    merge.warpId = DEFAULT_INT_VALUE;
    merge.schId = DEFAULT_INT_VALUE;
    cacheInstrMap_[info.coreName].emplace_back(merge);
}

RealTimeICacheParser::RealTimeICacheParser(RealTimeSimParseContext context) : RealTimeLogParer(std::move(context), 1)
{
    realTimeICacheParserPlugin_ = Utility::MakeShared<RealTimeICacheParserPlugin>(dataCenter_, context_.chipType);
    pluginManager_.AddPlugin(realTimeICacheParserPlugin_);
}

void RealTimeICacheParser::SetICacheLog(const IcacheParseInfoForRealTime &iCacheParseInfo)
{
    auto ptr = dataCenter_.GetStreamPtr<IcacheParseInfoForRealTime>();
    if (ptr == nullptr) {
        LogWarn("Set pop instr log failed, can not match object target format");
        return;
    }
    ptr->Push(iCacheParseInfo);
}

const std::string &GetCoreName(uint32_t coreId, uint32_t subCoreId)
{
    static std::unordered_map<std::pair<uint32_t, uint32_t>, std::string, PairHash> coreNameMap;
    auto iter = coreNameMap.find({coreId, subCoreId});
    if (iter != coreNameMap.end()) {
        return iter->second;
    }
    static std::string nullCore;
    static std::mutex mtx;
    std::lock_guard<std::mutex> lock(mtx);
    std::string subcoreNum;
    switch (subCoreId) {
        case 0: // 0 means vector0
            subcoreNum = std::string(CORE_CUBE) + "0";
            break;
        case 1: // 1 means cube0
            subcoreNum = std::string(CORE_VEC) + "0";
            break;
        case 2: // 2 means cube1
            subcoreNum = std::string(CORE_VEC) + "1";
            break;
        default:
            return nullCore;
    }
    std::string coreName = CORE + std::to_string(coreId) + "." + subcoreNum;
    coreNameMap[{coreId, subCoreId}] = coreName;
    return coreNameMap[{coreId, subCoreId}];
}

bool GetInstrDetail(const std::string &decodeDescr, std::smatch &lineMatch)
{
    return std::regex_match(decodeDescr, lineMatch, INSTR_PATTERN);
}

void RealTimeDataParser::SetInstrLog(const Common::DvcInstrLog &dvcInstrLog)
{
    std::smatch lineMatch;
    if (!GetInstrDetail(dvcInstrLog.decodeDescr, lineMatch)) {
        LogWarn("Set instr log failed, can not match object target format, describe is %s", dvcInstrLog.decodeDescr);
        return;
    }
    const std::string &coreName = GetCoreName(dvcInstrLog.coreId, dvcInstrLog.subCoreId);
    if (coreName.empty()) {
        LogWarn("Set instr log failed, core name is empty");
        return;
    }
    // index 1 is name, index 2 is detail
    InstrParseInfoForRealTime instrParseInfo = {{dvcInstrLog.time, dvcInstrLog.pc, 0, DEFAULT_INT_VALUE,
        DEFAULT_INT_VALUE, lineMatch[1].str(), lineMatch[2].str(), dvcInstrLog.execDescr, {}},
        coreName};
    realTimeInstrParser_.SetInstrLog(instrParseInfo);
}

void RealTimeDataParser::SetPopInstrLog(const Common::DvcInstrLog &dvcInstrLog)
{
    std::smatch lineMatch;
    if (!GetInstrDetail(dvcInstrLog.decodeDescr, lineMatch)) {
        LogWarn("Set pop instr log failed, can not match object target format, describe is %s",
                dvcInstrLog.decodeDescr);
        return;
    }
    const std::string &coreName = GetCoreName(dvcInstrLog.coreId, dvcInstrLog.subCoreId);
    if (coreName.empty()) {
        LogWarn("Set pop instr log failed, core name is empty");
        return;
    }
    PoppedInstrParseInfo poppedInstrParseInfo({dvcInstrLog.time, dvcInstrLog.pc, 0, DEFAULT_INT_VALUE,
        DEFAULT_INT_VALUE, lineMatch[1].str(), lineMatch[2].str(), dvcInstrLog.execDescr, {}});
    PoppedInstrParseInfoForRealTime instrParseInfo(poppedInstrParseInfo, coreName);
    realTimeInstrParser_.SetPopInstrLog(instrParseInfo);
}

void RealTimeDataParser::SetICacheLog(const Common::DvciCacheLog &iCacheLog)
{
    // need to test
    std::ostringstream oss;
    // In hexadecimal format, 8 is reserved for display.
    oss << "size is 0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') <<
        iCacheLog.size << ", type:" << iCacheLog.type << ", last:"
        << static_cast<uint32_t>(iCacheLog.last) << ",status is MISS";
    const std::string &coreName = GetCoreName(iCacheLog.coreId, iCacheLog.subCoreId);
    if (coreName.empty()) {
        LogWarn("Set iCache log failed, core name is empty");
        return;
    }
    IcacheParseInfoForRealTime iCacheParseInfo = {
        iCacheLog.time, iCacheLog.addr, coreName, oss.str(), iCacheLog.opType};
    realTimeICacheParser_.SetICacheLog(iCacheParseInfo);
}

void RealTimeDataParser::SetMteLog(const Common::DvcMteLog &dvcMteLog)
{
    realTimeMteParser_.SetMteLog(dvcMteLog);
}

void RealTimeDataParser::ProcessAfterKernelExit()
{
    std::map<std::string, std::shared_ptr<Profiling::Parse::DataCenter>> dateCenterMap;
    realTimeInstrParser_.Merge(dateCenterMap);
    std::shared_ptr<Profiling::Parse::DataCenter> InteDataCenterPtr = MakeShared<Profiling::Parse::DataCenter>();
    if (InteDataCenterPtr == nullptr) {
        LogWarn("Inter data center ptr create failed");
        return;
    }
    InsertCache(dateCenterMap);
    if (context_.metricsConfig.overHead) {
        InsertScalar(dateCenterMap);
    }
    if (context_.metricsConfig.pmSamplingEnable) {
        realTimeMteParser_.MteProcessAfterExit(InteDataCenterPtr);
    }
    std::string dumpPath = JoinPath({outputPath_, "dump"});
    bool isNeedGetPc2Code = true;
    if (pc2Code_ != nullptr && pc2CodeThr_.joinable()) {
        pc2CodeThr_.join();
        isNeedGetPc2Code = false;
    } else {
        pc2Code_ = Utility::MakeShared<ParsePcCode>(dumpPath);
        if (pc2Code_ == nullptr) {
            LogWarn("Pc2code create failed");
            return;
        }
    }
    CalCulate(dateCenterMap, pc2Code_, context_.metricsConfig, isNeedGetPc2Code, context_.chipType);

    auto pcToCode = pc2Code_->GetPc2Code();
    std::shared_ptr<Pc2CodeMap> pc2codePtr = MakeShared<Pc2CodeMap>(pcToCode);

    CombineCoreData(dateCenterMap, InteDataCenterPtr);
    auto systemCores = static_cast<uint32_t>(std::thread::hardware_concurrency() * MAX_THREAD_USAGE_RATIO);
    std::string simulatorPath = JoinPath({outputPath_, "simulator"});
    Mkdir(simulatorPath);
    if (pc2codePtr == nullptr) {
        Pc2CodeMap empty;
        LogWarn("Can not get kernel file of %s. code information will be missing", outputPath_.c_str());
        Visualizedata(simulatorPath, context_.chipType, empty, InteDataCenterPtr, systemCores);
    } else  {
        Visualizedata(simulatorPath, context_.chipType, *pc2codePtr, InteDataCenterPtr, systemCores);
    }
}

void RealTimeDataParser::GetPc2Code()
{
    pc2Code_ = Utility::MakeShared<ParsePcCode>(JoinPath({outputPath_, "dump"}));
    if (pc2Code_ == nullptr) {
        LogWarn("Pc2code create failed");
        return;
    }
    if (!pc2Code_->GetPcSetByKernelName(kernelName_)) {
        pc2Code_ = nullptr;
        return;
    }
    pc2CodeThr_ = std::thread([this] {
        pc2Code_->Parse();
    });
}

void RealTimeDataParser::Stop()
{
    if (isStop_) {
        return;
    }
    Utility::LogDebug("Real time all plugin will stop");
    isStop_ = true;
    realTimeICacheParser_.Stop();
    if (context_.metricsConfig.pmSamplingEnable) {
        realTimeMteParser_.Stop();
    }
    realTimeInstrParser_.Stop();
    realTimeCcuParser_.Stop();
    Utility::LogDebug("Real time all plugin stopped");
    {
        std::lock_guard<std::mutex> lock(mtx_);
        afterExitProcessThr_ = std::thread([this] {
            ProcessAfterKernelExit();
        });
    }
}

void RealTimeDataParser::Start(const std::string &outputPath, const std::string &kernelName)
{
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (afterExitProcessThr_.joinable()) {
            afterExitProcessThr_.join();
        }
        isStop_ = false;
    }
    outputPath_ = outputPath;
    kernelName_ = kernelName;
    Utility::RollbackPath(outputPath_, 1);
    GetPc2Code();
    realTimeInstrParser_.Start();
    realTimeICacheParser_.Start();
    realTimeCcuParser_.Start();
    if (context_.metricsConfig.pmSamplingEnable) {
        Utility::LogDebug("PMSampling is enabled. Start to dispose mte log");
        realTimeMteParser_.Start();
    }
}

PluginErrorCode RealTimeMteParserPlugin::Entry()
{
    auto ptr = dataCenter_.GetStreamPtr<Common::DvcMteLog>();
    SetEntry(true);
    LogDebug("Mte real time plugin entry");
    while (ptr != nullptr) {
        Common::DvcMteLog mteInfo = ptr->Pop();
        if (ptr->IsStop()) {
            break;
        }
        ParseMTELogLine(mteInfo, mteLogInstrVec_[mteInfo.coreId]);
    }
    auto MteLogInstrMapPtr = Utility::MakeShared<std::vector<Parse::MteLogInstrMap>>(mteLogInstrVec_);
    if (!dataCenter_.DataTableRegister(MteLogInstrMapPtr)) {
        LogDebug("DataCenter Register MteLogInstrMap failed");
        return PluginErrorCode::FATAL_ERROR;
    }
    SetEntry(false);
    return PluginErrorCode::SUCCESS;
}

void RealTimeMteParser::SetMteLog(const Common::DvcMteLog &mteLog)
{
    auto ptr = dataCenter_.GetStreamPtr<Common::DvcMteLog>();
    if (ptr == nullptr) {
        LogWarn("Can not find DvcMteLog type in data center");
        return;
    }
    ptr->Push(mteLog);
}

RealTimeMteParser::RealTimeMteParser(RealTimeSimParseContext context) : RealTimeLogParer(std::move(context), 1)
{
    realTimeMteParserPlugin_ = Utility::MakeShared<RealTimeMteParserPlugin>(dataCenter_, context_.chipType);
    pluginManager_.AddPlugin(realTimeMteParserPlugin_);
}

void RealTimeMteParser::MteProcessAfterExit(const std::shared_ptr<Profiling::Parse::DataCenter> &InteDataCenterPtr)
{
    std::vector<Parse::PluginErrorCode> results;
    // 计算
    Parse::PluginManager calPluginManager(1);
    calPluginManager.AddPlugin<Parse::MteLogCalculator>(dataCenter_, context_.chipType);
    calPluginManager.RunAllPlugins(results);

    // 可视化
    Parse::PluginManager visPluginManager(1);
    visPluginManager.AddPlugin<Parse::MteLogVisualizer>(dataCenter_, context_.chipType);
    visPluginManager.RunAllPlugins(results);

    InteDataCenterPtr->DataTableRegister(dataCenter_.GetDbPtr<std::vector<nlohmann::json>>());
}

PluginErrorCode RealTimeCcuParserPlugin::Entry() {
    ccuInstrMap_.clear();
    auto ptr = dataCenter_.GetStreamPtr<CcuParseInfoForRealTime>();
    SetEntry(true);
    LogDebug("Ccu real time plugin entry");
    while (ptr != nullptr) {
        CcuParseInfoForRealTime ccuInfo = ptr->Pop();
        if (ptr->IsStop()) {
            break;
        }
        ccuInstrMap_[ccuInfo.coreName][ccuInfo.pc].emplace_back(ScalarInstrInfo{UINT64_MAX, ccuInfo.tick, ccuInfo.pc});
    }
    auto ccuPtr = MakeShared<std::map<std::string, scalarHead>>(ccuInstrMap_);
    if (!dataCenter_.DataTableRegister(ccuPtr)) {
        LogDebug("Failed to register ccu table");
        return PluginErrorCode::NONBLOCKING_ERROR;
    }
    SetEntry(false);
    return PluginErrorCode::SUCCESS;
}

void RealTimeCcuParser::SetCcuLog(const CcuParseInfoForRealTime &ccuLog) {
    auto ptr = dataCenter_.GetStreamPtr<CcuParseInfoForRealTime>();
    if (ptr == nullptr) {
        LogWarn("Set ccu log failed, can not match object format");
        return;
    }
    ptr->Push(ccuLog);
}

RealTimeCcuParser::RealTimeCcuParser(RealTimeSimParseContext context) : RealTimeLogParer(std::move(context), 1) {
    realTimeCcuParserPlugin_ = Utility::MakeShared<RealTimeCcuParserPlugin>(dataCenter_, context_.chipType);
    pluginManager_.AddPlugin(realTimeCcuParserPlugin_);
}

void RealTimeDataParser::SetCcuLog(const Common::DvcCcuLog &ccuLog) {
    const std::string &coreName = GetCoreName(ccuLog.coreId, ccuLog.subCoreId);
    if (coreName.empty()) {
        LogDebug("Set iCache log failed, core name is empty");
        return;
    }
    CcuParseInfoForRealTime ccuParseInfo = {ccuLog.time, ccuLog.pc, coreName};
    realTimeCcuParser_.SetCcuLog(ccuParseInfo);
}

void RealTimeDataParser::InsertScalar(
    std::map<std::string, std::shared_ptr<Profiling::Parse::DataCenter>> &dataCenterMap) {
    auto ccuPtr = realTimeCcuParser_.dataCenter_.GetDbPtr<std::map<std::string, scalarHead>>();
    auto cacheScalarPtr = realTimeICacheParser_.dataCenter_.GetDbPtr<std::map<std::string, scalarHeadCache>>();
    if (ccuPtr == nullptr || cacheScalarPtr == nullptr) {
        LogDebug("Failed to register scalar info");
        return;
    }

    for (auto iter : dataCenterMap) {
        std::string logicName = iter.first;
        auto dataCenter = iter.second;
        auto physisToLogicalPtr = dataCenter->GetDbPtr<PhysicalAndLogicalPair>();
        if (physisToLogicalPtr == nullptr) {
            LogDebug("Failed to merge scalar because physical core name lost");
            continue;
        }
        std::string phyCoreName = physisToLogicalPtr->first;
        if ((*ccuPtr).find(phyCoreName) != (*ccuPtr).end() &&
            (*cacheScalarPtr).find(phyCoreName) != (*cacheScalarPtr).end()) {
            auto scalarCcuPtr = Utility::MakeShared<scalarHead>((*ccuPtr)[phyCoreName]);
            auto scalarCachePtr = Utility::MakeShared<scalarHeadCache>((*cacheScalarPtr)[phyCoreName]);
            if (!dataCenter->DataTableRegister(scalarCcuPtr) || !dataCenter->DataTableRegister(scalarCachePtr)) {
                LogDebug("Failed register ccu instr for core %s", phyCoreName.c_str());
            }
        }
    }
}

void RealTimeDataParser::InsertCache(
    std::map<std::string, std::shared_ptr<Profiling::Parse::DataCenter>> &dateCenterMap) {
    auto cacheMapPtr = realTimeICacheParser_.dataCenter_.GetDbPtr<std::map<std::string, Parse::CacheDetailTable>>();
    if (cacheMapPtr == nullptr) {
        return;
    }
    for (const auto &iter : dateCenterMap) {
        std::string logicName = iter.first;
        auto dataCenter = iter.second;
        auto physisToLogicalPtr = dataCenter->GetDbPtr<PhysicalAndLogicalPair>();
        if (physisToLogicalPtr == nullptr) {
            continue;
        }
        std::string phyCoreName = physisToLogicalPtr->first;
        if ((*cacheMapPtr).find(phyCoreName) != (*cacheMapPtr).end()) {
            auto cachePtr = Utility::MakeShared<Parse::CacheDetailTable>((*cacheMapPtr)[phyCoreName]);
            if (!dataCenter->DataTableRegister(cachePtr)) {
                LogDebug("Failed register cache instr for core %s", phyCoreName.c_str());
            }
        }
    }
}
}
}
