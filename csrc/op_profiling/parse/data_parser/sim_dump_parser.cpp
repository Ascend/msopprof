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


#include "sim_dump_parser.h"
#include "pc_process.h"
#include "parse/data_parser/instr_parser/instr_parser.h"
#include "parse/data_parser/ccu_parser/ccu_log_parser.h"
#include "parse/data_parser/cache_parser/icache_parser.h"
#include "parse/data_calculator/instr_detail_calculator/process_bytes_calculator.h"
#include "parse/data_calculator/instr_detail_calculator/vector_utilization_calculator.h"
#include "parse/data_calculator/instr_detail_calculator/ub_conflict_calculator.h"
#include "parse/data_calculator/instr_detail_calculator/gpr_live_register_calculator.h"
#include "parse/data_visualizer/hotspotmap_visualizer/hotspotmap_visualizer.h"
#include "parse/data_visualizer/timeline_visualizer/core_timeline/core_timeline_visualizer.h"
#include "parse/data_calculator/instr_detail_calculator/scalar_calculator.h"
#include "parse/data_visualizer/timeline_visualizer/subcore_timeline/subcore_timeline_visualizer.h"
#include "profiling/simulator/data_parse/api_data.h"

using namespace Serialization;
using namespace Utility;
namespace Profiling {
namespace Parse {

void AnalySisFileByCore(Profiling::Parse::DataCenter &dataCenter, const SimParseContext &simParseContext,
    const CoreNameAndPreFixPair &coresNamePair, uint32_t parseThread)
{
    bool enableResourceConflictRatio = simParseContext.metricsConfig.IsOn(Common::ProfMetrics::RESOURCE_CONFLICT_RATIO);
    bool enableOverhead = simParseContext.metricsConfig.overHead;
    Profiling::Parse::PluginManager pluginManager(parseThread);
    Profiling::Parse::SimDataParserConfig config {simParseContext.dumpPath, coresNamePair, simParseContext.parseCorIds,
        enableResourceConflictRatio, enableOverhead, simParseContext.chipType};
    pluginManager.AddPlugin<Profiling::Parse::InstrParser>(dataCenter, config);
    pluginManager.AddPlugin<Profiling::Parse::ICacheParser>(dataCenter, config);
    if (enableOverhead) {
        pluginManager.AddPlugin<Profiling::Parse::CcuParser>(dataCenter, config);
    }
    std::vector<PluginErrorCode> results;
    pluginManager.RunAllPlugins(results);
}

void VisualizeByPlugin(const std::string &outputPath, ChipProductType chipType, Pc2CodeMap &pc2Code,
                       std::shared_ptr<Profiling::Parse::DataCenter> &dataCenterPtr, uint32_t systemCores)
{
    constexpr uint32_t visualPlugin = 2;
    auto mapPtr = dataCenterPtr->GetDbPtr<std::map<std::string, SimData>>();
    if (mapPtr == nullptr) {
        LogError("Failed to get data from datacenter, please check dump file exists!");
        return;
    }
    const uint32_t coreSize = mapPtr->size();
    uint32_t visualThread = systemCores < coreSize ? 1 : (systemCores - coreSize + visualPlugin - 1) / visualPlugin;
    Profiling::Parse::PluginManager timelinePluginManager(visualPlugin);
    Profiling::Parse::SimVisualizerConfig config {outputPath, pc2Code, visualThread, chipType};
    timelinePluginManager.AddPlugin<Profiling::Parse::CoreTimeLineVisualizer>(*dataCenterPtr, config);
    timelinePluginManager.AddPlugin<Profiling::Parse::SubcoreTimelineVisualizer>(*dataCenterPtr, config);
    std::vector<PluginErrorCode> results;
    timelinePluginManager.RunAllPlugins(results);

    // 可视化存在依赖，必须现有流水图后有热点图，后续可视化修改后直接把热点图添加到上面manager即可
    Profiling::Parse::SimVisualizerConfig config2 {outputPath, pc2Code, systemCores, chipType};
    Profiling::Parse::PluginManager hotSpotPluginManager(visualPlugin);
    hotSpotPluginManager.AddPlugin<Profiling::Parse::HotSpotMapVisualizer>(*dataCenterPtr, config2);
    std::vector<PluginErrorCode> results2;
    hotSpotPluginManager.RunAllPlugins(results2);
}

void GetLogiCoreName(const Profiling::Parse::DataCenter &dataCenter, std::string &coreName)
{
    auto physisToLogicalPtr = dataCenter.GetDbPtr<PhysicalAndLogicalPair>();
    if (physisToLogicalPtr == nullptr) {
        return;
    }
    coreName = physisToLogicalPtr->second;
}

void GetphyCoreName(const Profiling::Parse::DataCenter &dataCenter, std::string &coreName)
{
    auto physisToLogicalPtr = dataCenter.GetDbPtr<PhysicalAndLogicalPair>();
    if (physisToLogicalPtr == nullptr) {
        return;
    }
    coreName = physisToLogicalPtr->first;
}

void Visualizedata(const std::string &outputPath, ChipProductType chipType, Pc2CodeMap &pc2code,
                   std::shared_ptr<Profiling::Parse::DataCenter> &dataCneterPtr, uint32_t systemCores)
{
    auto simDataPtr = dataCneterPtr->GetDbPtr<std::map<std::string, SimData>>();
    if (simDataPtr == nullptr) {
        LogError("Failed to generate trace and hotspotMap because get sim data failed");
        return;
    }
    for (const auto &coreWithData: *simDataPtr) {
        const std::string &coreName = coreWithData.first;
        Mkdir(JoinPath({outputPath, coreName}));
    }
    VisualizeByPlugin(outputPath, chipType, pc2code, dataCneterPtr, systemCores);
}

std::set<uint64_t> CollectPcFromCore(const std::shared_ptr<Profiling::Parse::DataCenter> &dataCenterPtr)
{
    std::set<uint64_t> pcSet;
    auto instrPtr = dataCenterPtr->GetDbPtr<InstrDetailTable>();
    auto cachePtr = dataCenterPtr->GetDbPtr<CacheDetailTable>();
    auto userMarkPtr = dataCenterPtr->GetDbPtr<UserMarkStruct>();
    if (instrPtr != nullptr) {
        auto &mergeData = *instrPtr->GetColumnData<MergeInfo>(Parse::InstrDetailTable::MERGE_INFO);
        for (const auto &temp : mergeData) {
            pcSet.emplace(temp.pc);
        }
    }
    if (cachePtr != nullptr) {
        auto &cacheInstr = cachePtr->GetCache();
        for (const auto &temp: cacheInstr) {
            pcSet.emplace(temp.pc);
        }
    }
    if (userMarkPtr == nullptr) {
        return pcSet;
    }
    auto &userMarkInfos = userMarkPtr->userMarkInfos;
    auto &userMark = userMarkPtr->userMarkInstrs;
    for (const auto &nameWithInfos : userMarkInfos) {
        for (const auto &temp : nameWithInfos.second) {
            if (temp.startPc != UINT64_MAX) {
                pcSet.emplace(temp.startPc);
            }
            if (temp.endPc != UINT64_MAX) {
                pcSet.emplace(temp.endPc);
            }
        }
    }
    for (const auto &temp : userMark) {
        pcSet.emplace(temp.pc);
    }
    return pcSet;
}

void CalCulateDetail(Parse::DataCenter &dataCenter, const ChipProductType &chipProductType, const Common::ProfMetricsAbilityConfig &config, uint32_t calThread)
{
    using namespace Common;
    Parse::PluginManager pluginManager(calThread);
    Parse::InstrDetailConfig instrDetailContext {chipProductType};
    ChipProductType chipTypeSeries = GetProductSeriesType(chipProductType);
    bool supportPart = IsChipSeriesTypeValid(chipTypeSeries, ChipProductType::ASCEND310P_SERIES) ||
        IsChipSeriesTypeValid(chipTypeSeries, ChipProductType::ASCEND910B_SERIES) ||
        IsChipSeriesTypeValid(chipTypeSeries, ChipProductType::ASCEND910_93_SERIES);
    bool supportGpr = supportPart || IsChipSeriesTypeValid(chipTypeSeries, ChipProductType::ASCEND950_SERIES);
    if (!supportGpr) {
        return;
    }
    if (supportPart) {
        pluginManager.AddPlugin<Parse::ProcessBytesCalculator>(dataCenter, instrDetailContext);
        pluginManager.AddPlugin<Parse::VectorUtilizationCalculator>(dataCenter, instrDetailContext);
        pluginManager.AddPlugin<Parse::UbConflictCalculator>(dataCenter, instrDetailContext);

    }
    if (config.overHead) {
        pluginManager.AddPlugin<Parse::ScalarCalculator>(dataCenter, instrDetailContext);
    }
    pluginManager.AddPlugin<Parse::GPRLiveRegisterCalculator>(dataCenter, instrDetailContext);
    std::vector<PluginErrorCode> results;
    pluginManager.RunAllPlugins(results);
    std::shared_ptr<Parse::InstrDetailTable> instrPtr = dataCenter.GetDbPtr<Parse::InstrDetailTable>();
    if (instrPtr != nullptr) {
        instrPtr->UpdateRow();
    }
}

void CalCulate(std::map<std::string, std::shared_ptr<Profiling::Parse::DataCenter>> &dataMap,
               const std::shared_ptr<ParsePcCode> &pc2code, const Common::ProfMetricsAbilityConfig &config, bool isNeedGetPc2Code, ChipProductType chipType)
{
    auto poolSize = static_cast<uint32_t>(std::thread::hardware_concurrency() * MAX_THREAD_USAGE_RATIO);
    auto coreSize = dataMap.size();
    constexpr uint32_t calculPlugin = 5;
    uint32_t calCulThread = poolSize < coreSize ? 1 : (poolSize - coreSize + calculPlugin - 1) / calculPlugin;
    std::set<uint64_t> pcSet;
    ThreadPool calPool(poolSize);
    calPool.Start();
    for (auto &coreWithDatacenter : dataMap) {
        auto &datacenter = coreWithDatacenter.second;
        if (datacenter->GetDbPtr<InstrDetailTable>() == nullptr) {
            continue;
        }
        if (isNeedGetPc2Code) {
            std::set<uint64_t> partPcSet = CollectPcFromCore(datacenter);
            pcSet.insert(partPcSet.begin(), partPcSet.end());
        }
        calPool.AddTask([&datacenter, &calCulThread, &config, chipType] {
            CalCulateDetail(*datacenter, chipType, config, calCulThread);
        });
    }
    if (isNeedGetPc2Code && pc2code != nullptr) {
        pc2code->SetPcSet(pcSet);
        pc2code->Parse();
    }
    calPool.WaitAllTasks();
    calPool.Stop();
}

bool CalCulate(const SimParseContext &simParseContext, std::map<std::string,
    std::shared_ptr<Profiling::Parse::DataCenter>> &dataMap,
    std::shared_ptr<Profiling::Parse::DataCenter> &dataCenterPtr,
    std::shared_ptr<Pc2CodeMap> &pc2codePtr, uint32_t poolSize)
{
    auto coreSize = simParseContext.coresNamePair.size();
    constexpr uint32_t calculPlugin = 5;
    uint32_t calCulThread = poolSize < coreSize ? 1 : (poolSize - coreSize + calculPlugin - 1) / calculPlugin;
    std::set<uint64_t> pcSet;
    for (auto &coreWithDatacenter : dataMap) {
        auto &tempDatacenter = coreWithDatacenter.second;
        std::set<uint64_t> partPcSet = CollectPcFromCore(tempDatacenter);
        pcSet.insert(partPcSet.begin(), partPcSet.end());
    }
    ParsePcCode pcCode(simParseContext.dumpPath, pcSet);

    ThreadPool calPool(poolSize);
    calPool.Start();
    for (auto &coreWithDatacenter : dataMap) {
        auto &datacenter = coreWithDatacenter.second;
        if (datacenter->GetDbPtr<InstrDetailTable>() == nullptr) {
            continue;
        }
        calPool.AddTask([&datacenter, &calCulThread, &simParseContext] {
            CalCulateDetail(*datacenter, simParseContext.chipType, simParseContext.metricsConfig, calCulThread);
        });
    }
    pcCode.Parse();
    calPool.WaitAllTasks();
    calPool.Stop();
    auto pcToCode = pcCode.GetPc2Code();
    pc2codePtr = Utility::MakeShared<Pc2CodeMap>(pcToCode);
    if (pc2codePtr == nullptr) {
        return false;
    }
    return CombineCoreData(dataMap, simParseContext.chipType, dataCenterPtr);
}


bool ParseDumpFile(const SimParseContext &simParseContext,
    std::shared_ptr<Profiling::Parse::DataCenter> &dataCenterPtr,
    std::shared_ptr<Pc2CodeMap> &pc2codePtr, uint32_t poolSize)
{
    constexpr uint32_t parsePlugin = 3;
    auto coreSize = simParseContext.coresNamePair.size();
    MteThroughput mteThroughput;
    if (simParseContext.metricsConfig.pmSamplingEnable) {
        mteThroughput.Process(simParseContext.dumpPath, simParseContext.chipType, poolSize);
        std::shared_ptr<std::vector<nlohmann::json>> mteJsonList =
                MakeShared<std::vector<nlohmann::json>>(std::move(mteThroughput.jsonList_));
        if (mteJsonList == nullptr) {
            LogWarn("Failed to create mteJsonList");
            return false;
        }
        dataCenterPtr->DataTableRegister(mteJsonList);
    }
    uint32_t parseLogThread = poolSize < coreSize ? 1 : (poolSize - coreSize + parsePlugin - 1) / parsePlugin;

    std::map<std::string, std::shared_ptr<Profiling::Parse::DataCenter>> dataMap;
    ThreadPool parsePool(poolSize);
    parsePool.Start();
    for (const auto &coreWithFix : simParseContext.coresNamePair) {
        auto dataCenter = Utility::MakeShared<Profiling::Parse::DataCenter>();
        if (dataCenter == nullptr) {
            LogWarn("Failed to create datacenter for %s", coreWithFix.first.c_str());
            continue;
        }
        dataMap[coreWithFix.first] = dataCenter;
        parsePool.AddTask([&dataMap, &simParseContext, &coreWithFix, parseLogThread]() {
            AnalySisFileByCore(*dataMap[coreWithFix.first], simParseContext, coreWithFix, parseLogThread);
        });
    }
    parsePool.WaitAllTasks();
    parsePool.Stop();
    return CalCulate(simParseContext, dataMap, dataCenterPtr, pc2codePtr, poolSize);
}

bool CombineCoreData(const std::map<std::string, std::shared_ptr<Profiling::Parse::DataCenter>> &dataCenterMap,
                     std::shared_ptr<Profiling::Parse::DataCenter> &dataCenterPtr)
{
    std::map<std::string, SimData> allDataMap;
    for (const auto &coreDataCenter : dataCenterMap) {
        std::string logiCoreName = coreDataCenter.first; // coreName is logi core name
        auto tempDataCenter = coreDataCenter.second;
        auto instrPtr = tempDataCenter->GetDbPtr<InstrDetailTable>();
        if (instrPtr == nullptr || instrPtr->GetColumnData<MergeInfo>(InstrDetailTable::MERGE_INFO) == nullptr) {
            continue;
        }
        auto cachePtr = tempDataCenter->GetDbPtr<Parse::CacheDetailTable>();
        auto userMarkPtr = tempDataCenter->GetDbPtr<UserMarkStruct>();
        SimData data = {instrPtr, cachePtr, userMarkPtr};
        allDataMap[logiCoreName] = data;
    }
    auto simDataMapPtr = Utility::MakeShared<std::map<std::string, SimData>>(allDataMap);
    if (simDataMapPtr == nullptr || !dataCenterPtr->DataTableRegister(simDataMapPtr)) {
        LogError("Failed to combine dump file data");
        return false;
    }
    return true;
}

bool CombineCoreData(const std::map<std::string, std::shared_ptr<Profiling::Parse::DataCenter>> &dataCenterMap,
    const ChipProductType &chipProductType, std::shared_ptr<Profiling::Parse::DataCenter> &dataCenterPtr)
{
    std::map<std::string, SimData> allDataMap;
    for (const auto &coreDataCenter : dataCenterMap) {
        std::string coreName = coreDataCenter.first;
        auto tempDataCenter = coreDataCenter.second;
        auto instrPtr = tempDataCenter->GetDbPtr<InstrDetailTable>();
        if (instrPtr == nullptr || instrPtr->GetColumnData<MergeInfo>(InstrDetailTable::MERGE_INFO) == nullptr) {
            continue;
        }
        if (GetProductSeriesType(chipProductType) == ChipProductType::ASCEND910_93_SERIES ||
            GetProductSeriesType(chipProductType) == ChipProductType::ASCEND910B_SERIES) {
            GetLogiCoreName(*tempDataCenter, coreName);
        }
        auto cachePtr = tempDataCenter->GetDbPtr<CacheDetailTable>();
        auto userMarkPtr = tempDataCenter->GetDbPtr<UserMarkStruct>();
        SimData data = {instrPtr, cachePtr, userMarkPtr};
        allDataMap[coreName] = data;
    }
    auto simDataMapPtr = Utility::MakeShared<std::map<std::string, SimData>>(allDataMap);
    if (simDataMapPtr == nullptr || !dataCenterPtr->DataTableRegister(simDataMapPtr)) {
        LogError("Failed to combine dump file data");
        return false;
    }
    return true;
}
}
}
