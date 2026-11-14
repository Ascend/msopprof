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

#include "device_data_parse.h"
#include <atomic>
#include <algorithm>
#include <tuple>
#include <exception>
#include <functional>
#include <chrono>
#include <thread>
#include <sstream>
#include "thread_pool.h"
#include "data_save.h"
#include "smart_pointer.h"
#include "log.h"
#include "filesystem.h"
#include "common/visualize.h"
#include "parse/plugin/plugin_manager.h"
#include "parse/data_calculator/compute_load_calculator.h"
#include "include/opprof/DbiDefs.h"

using namespace Visualize;
using namespace Utility;
using namespace Common;
using namespace std;

namespace Profiling {
DeviceDataParse::DeviceDataParse(Common::ChipType chipType, PmuEventsId pmuEventsId, ProfMetricsAbilityConfig metrics,
    const std::string &kernelNameFilter)
    : DataParse(std::move(metrics), kernelNameFilter), chipType_(chipType), pmuEventsId_(pmuEventsId) {
    // register different profiling data handler
    chipInfoMap_ = {{ChipType::ASCEND950, {MetricHeaderForA5, Common::AIC_EVENTS_FOR_A5,
                        Common::AIV_EVENTS_FOR_A5}},
                    {ChipType::ASCEND910B, {MetricHeaderFor910B, Common::AIC_EVENTS_FOR_910B,
                        Common::AIV_EVENTS_FOR_910B}},
                    {ChipType::ASCEND310P, {MetricHeaderFor310P, Common::AIC_EVENTS_FOR_310P, {}}},
                    // 310b is supported temporarily. Data is obtained through the 910b channel.
                    // The configuration will be modified later.
                    {ChipType::ASCEND310B, {MetricHeaderFor910B, Common::AIC_EVENTS_FOR_910B,
                        Common::AIV_EVENTS_FOR_910B}}};
    if (CHIP_ARCHITECTURE_TO_PRODUCT_SERIES.find(chipType_) == CHIP_ARCHITECTURE_TO_PRODUCT_SERIES.end()) {
        LogError("Analyzing profiling data failed for this chip type is in support series list");
        chipProductType_ = ChipProductType::UNKNOWN_PRODUCT_TYPE;
    } else {
        chipProductType_ = CHIP_ARCHITECTURE_TO_PRODUCT_SERIES.at(chipType);
    }
}

unique_ptr<DataHandler> GetHandle(ChipType chipType) {
    unique_ptr<DataHandler> handlePtr;
    ChipProductType type = ChipProductType::ASCEND910B_SERIES;
    if (CHIP_ARCHITECTURE_TO_PRODUCT_SERIES.find(chipType) != CHIP_ARCHITECTURE_TO_PRODUCT_SERIES.end()) {
        type = CHIP_ARCHITECTURE_TO_PRODUCT_SERIES.at(chipType);
    }
    auto createInstance = [](ChipProductType type) -> std::unique_ptr<DataHandler> {
        switch (type) {
            case ChipProductType::ASCEND910B_SERIES:
            case ChipProductType::ASCEND310B_SERIES:
                return Utility::MakeUnique<DataHandlerOf910B>();
            case ChipProductType::ASCEND310P_SERIES:
                return Utility::MakeUnique<DataHandlerOf310P>();
            case ChipProductType::ASCEND950_SERIES:
                return Utility::MakeUnique<DataHandlerOf91095>();
            default:
                return Utility::MakeUnique<DataHandlerOf910B>();
        }
    };
    handlePtr = createInstance(type);
    return handlePtr;
}

unique_ptr<::Visualize::PmuCalculator> GetPmuCalculatorObj(ChipType chipType, shared_ptr<BasicPmu> &basicPmuObj) {
    unique_ptr<::Visualize::PmuCalculator> pmuCalculatorPtr;
    if (chipType == ChipType::ASCEND910B) {
        pmuCalculatorPtr = Utility::MakeUnique<::Visualize::PmuCalculator910B>();
    } else if (chipType == ChipType::ASCEND310P) {
        pmuCalculatorPtr = Utility::MakeUnique<::Visualize::PmuCalculator310P>();
    } else {
        pmuCalculatorPtr = Utility::MakeUnique<::Visualize::PmuCalculatorA5>();
    }
    if (pmuCalculatorPtr == nullptr) {
        return pmuCalculatorPtr;
    }
    pmuCalculatorPtr->Init(basicPmuObj);
    return pmuCalculatorPtr;
}

unique_ptr<::Visualize::StorageAccess> GetStorageAccessObj(ChipType chipType,
    shared_ptr<::Visualize::OpBasicInfo> &opBasicInfoObj, shared_ptr<::Visualize::BasicPmu> &basicPmuObj,
    unique_ptr<::Visualize::PmuCalculator> &pmuCalculatorObj) {
    unique_ptr<::Visualize::StorageAccess> storageAccessPtr;
    if (chipType == ChipType::ASCEND910B) {
        storageAccessPtr = Utility::MakeUnique<StorageAccess910B>(opBasicInfoObj, basicPmuObj, pmuCalculatorObj);
    } else if (chipType == ChipType::ASCEND310P) {
        storageAccessPtr = Utility::MakeUnique<StorageAccess310P>(opBasicInfoObj, basicPmuObj, pmuCalculatorObj);
    } else {
        storageAccessPtr = Utility::MakeUnique<StorageAccessA5>(opBasicInfoObj, basicPmuObj, pmuCalculatorObj);
    }
    return storageAccessPtr;
}

unique_ptr<::Visualize::Occupancy> GetOccupancyObj(ChipType chipType,
    shared_ptr<::Visualize::OpBasicInfo> &opBasicInfoObj, shared_ptr<::Visualize::BasicPmu> &basicPmuObj) {
    unique_ptr<::Visualize::Occupancy> occupancyPtr;
    if (chipType == ChipType::ASCEND910B) {
        occupancyPtr = Utility::MakeUnique<Occupancy910B>(opBasicInfoObj, basicPmuObj);
    } else if (chipType == ChipType::ASCEND950) {
        occupancyPtr = Utility::MakeUnique<OccupancyA5>(opBasicInfoObj, basicPmuObj);
    } else {
        occupancyPtr = Utility::MakeUnique<Occupancy>(opBasicInfoObj, basicPmuObj);
    }
    return occupancyPtr;
}

unique_ptr<RoofLine> GetRoofLineObj(unique_ptr<DataHandler> &handler, shared_ptr<OpBasicInfo> &opBasicInfoObj,
    shared_ptr<BasicPmu> &basicPmuObj, unique_ptr<PmuCalculator> &pmuCalculatorObj) {
    unique_ptr<::Visualize::RoofLine> roofLinePtr;
    if (handler == nullptr) {
        LogWarn("Get roofline failed because of nullptr");
        return roofLinePtr;
    }
    if (handler->chipType_ == ChipType::ASCEND910B) {
        roofLinePtr = MakeUnique<RoofLineOf910B>(handler->aicoreFreq_, handler->aiCoreNum_,
                                                 opBasicInfoObj, basicPmuObj, pmuCalculatorObj);
    } else if (handler->chipType_ == ChipType::ASCEND310P) {
        roofLinePtr = MakeUnique<RoofLineOf310P>(handler->aicoreFreq_, handler->aiCoreNum_,
                                                 opBasicInfoObj, basicPmuObj, pmuCalculatorObj);
    } else {
        roofLinePtr = MakeUnique<RoofLineOfA5>(handler->aicoreFreq_, handler->aiCoreNum_,
                                               opBasicInfoObj, basicPmuObj, pmuCalculatorObj);
    }
    return roofLinePtr;
}

unique_ptr<::Visualize::TimelineParser> GetTimelineObj(
    unique_ptr<DataHandler> &handler, shared_ptr<OpBasicInfo> &opBasicInfoObj, shared_ptr<BasicPmu> &basicPmuObj) {
    unique_ptr<::Visualize::TimelineParser> parserPtr;
    if (handler == nullptr || opBasicInfoObj == nullptr || basicPmuObj == nullptr) {
        LogWarn("Get MC2 timeline failed because of nullptr");
        return parserPtr;
    }
    if (handler->GetMC2Flag()) {
        parserPtr = Utility::MakeUnique<::Visualize::MC2TimelineParser>(handler->GetAcsqTimeMap(),
        handler->GetMinMc2TimeCyc(), opBasicInfoObj, basicPmuObj);
    } else if (handler->GetLcclFlag()) {
        parserPtr = Utility::MakeUnique<::Visualize::LcclTimelineParser>(
        handler->GetMinLcclTimeCyc(), opBasicInfoObj, basicPmuObj);
    } else {
        parserPtr = Utility::MakeUnique<::Visualize::AicoreTimelineParser>(
            handler->GetMinTimeCyc(), opBasicInfoObj, basicPmuObj);
    }
    return parserPtr;
}

unique_ptr<::Visualize::CachelineHeatMap> GetCachelineHeatMapObj(unique_ptr<DataHandler> &handler)
{
    unique_ptr<::Visualize::CachelineHeatMap> parserPtr =
        Utility::MakeUnique<::Visualize::CachelineHeatMap>(handler->GetL2CacheObj());
    return parserPtr;
}

unique_ptr<::Visualize::BiuTimeline> GetBiuTimelineObj(const Common::ProfMetricsAbilityConfig &metrics) {
    unique_ptr<::Visualize::BiuTimeline> biuTimelinePtr;
    if (metrics.instrTimelineEnable) {
        biuTimelinePtr = MakeUnique<InstrBiuTimeline>();
    } else if (metrics.pipeTimelineEnable) {
        biuTimelinePtr = MakeUnique<PipeBiuTimeline>();
    } else {
        biuTimelinePtr = MakeUnique<BiuTimeline>();
    }
    return biuTimelinePtr;
}

bool DeviceDataParse::ParseExactKernelData(const string &path, const string &kernelName)
{
    Parse::DataCenter dataCenter;
    string timeStamp;
    GenerateTimeStamp(timeStamp, TimeAccuracy::MILLISECOND);
    ParserConfig parserConfig = {kernelName, path, aicCalMetricItems_, aivCalMetricItems_, metrics_.isKernelScale };
    auto handler = GetHandle(chipType_);
    if (!handler) {
        LogError("Get handle failed because of nullptr");
        return false;
    }
    bool res = handler->ParseDeviceData(parserConfig, eventMap_, metrics_, timeStamp);

    std::shared_ptr<::Visualize::OpBasicInfo> opBasicInfoObj =  Utility::MakeShared<::Visualize::OpBasicInfo>(handler);
    std::shared_ptr<::Visualize::BasicPmu> basicPmuObj = Utility::MakeShared<::Visualize::BasicPmu>(handler);
    if (!dataCenter.DataTableRegister(opBasicInfoObj) || !dataCenter.DataTableRegister(basicPmuObj)) {
        LogError("Get op basic info or basic pmu failed because of nullptr");
        return false;
    }
    auto pmuCalculatorObj = GetPmuCalculatorObj(chipType_, basicPmuObj);
    if (pmuCalculatorObj == nullptr) {
        LogError("Get pmuCalculatorObj failed because of nullptr");
        return false;
    }

    auto storageAccessObj = GetStorageAccessObj(chipType_, opBasicInfoObj, basicPmuObj, pmuCalculatorObj);
    auto occupancyObj = GetOccupancyObj(chipType_, opBasicInfoObj, basicPmuObj);
    auto roofLineObj = GetRoofLineObj(handler, opBasicInfoObj, basicPmuObj, pmuCalculatorObj);
    auto timelineObj = GetTimelineObj(handler, opBasicInfoObj, basicPmuObj);
    auto biuTimelineObj = GetBiuTimelineObj(metrics_);
    auto cachelineHeatMapObj = GetCachelineHeatMapObj(handler);
    if (!storageAccessObj || !occupancyObj || !roofLineObj || !timelineObj || !biuTimelineObj || !cachelineHeatMapObj) {
        LogError("Get visualize data failed because of nullptr");
        return false;
    }

    Parse::PluginManager calculatorPluginManager(1);
    std::vector<Parse::PluginErrorCode> results;
    calculatorPluginManager.AddPlugin<Parse::ComputeLoadCalculator>(dataCenter, chipProductType_);
    calculatorPluginManager.RunAllPlugins(results);

    DataVisualizePtr ptr = { opBasicInfoObj, storageAccessObj, occupancyObj, roofLineObj, timelineObj, biuTimelineObj, cachelineHeatMapObj};
    auto dataVisualize = Utility::MakeShared<DataVisualize>(ptr);
    if (!dataVisualize || !storageAccessObj) {
        LogError("Get visualize data failed because of nullptr");
        return false;
    }
    dataVisualize->GenerateVisualizeData(dataCenter, parserConfig.outputPath, metrics_);
    RecordSummary(*handler, path, dataVisualize->GetMergeTimelineData());
    storageAccessObj->ShowAdviceInfo();    // 展示调优建议
    opBasicInfoObj->ShowOpBasicInfo();    // 展示算子基础信息
    return res;
}

void DeviceDataParse::ParseKernelFile(const string &kernelDir, const string &kernelName, const string &deviceId)
{
    vector<string> fileNames;
    if (!GetFileNames(kernelDir, fileNames)) {
        LogWarn("Parsing %s failed, which maybe empty or too many kernels.", kernelDir.c_str());
        return;
    }
    sort(fileNames.begin(), fileNames.end());

    regex pattern("[0-9]{1,3}");
    auto subFilesChecker = std::bind(&DeviceDataParse::CheckKernelFiles, this, std::placeholders::_1,
                                     std::placeholders::_2, std::placeholders::_3);
    CopyDfxRegionFiles(kernelDir, fileNames, pattern);
    for (const string &fileName : fileNames) {
        string orderDir = JoinPath({kernelDir, fileName});
        if (!regex_match(fileName, pattern)) {
            continue;
        }
        string errorMsg;
        if (!CheckFolder(orderDir, errorMsg, false, subFilesChecker)) {
            LogError("%s", errorMsg.c_str());
            continue;
        }

        kernelParseTasks_.push_back({orderDir, kernelName, deviceId});

        if (!theOnlyKernelPath_.empty()) {
            isTheOnlyKernel_ = false;
        }
        if (isTheOnlyKernel_) {
            theOnlyKernelPath_ = orderDir;
        }
        totalKernelNum_++;
    }
}

bool DeviceDataParse::RunKernelParseTask(const KernelParseTask &task) {
    LogInfo("Start analyze kernel on device: %s, kernel: %s, name is: %s", task.deviceId.c_str(),
        task.kernelName.c_str(), GetLastFile(task.path).c_str());
    try {
        if (ParseExactKernelData(task.path, task.kernelName)) {
            return true;
        }
        LogError("Analyzing kernel data failed, device: %s, kernel: %s, name is: %s", task.deviceId.c_str(),
            task.kernelName.c_str(), GetLastFile(task.path).c_str());
    } catch (const std::exception &ex) {
        LogError("Analyzing kernel data raised an exception, device: %s, kernel: %s, path: %s, reason: %s",
            task.deviceId.c_str(), task.kernelName.c_str(), task.path.c_str(), ex.what());
    } catch (...) {
        LogError("Analyzing kernel data raised an unknown exception, device: %s, kernel: %s, path: %s",
            task.deviceId.c_str(), task.kernelName.c_str(), task.path.c_str());
    }
    return false;
}

void DeviceDataParse::ParseKernelTasks() {
    if (kernelParseTasks_.empty()) {
        return;
    }
    auto start = std::chrono::steady_clock::now();
    constexpr uint32_t maxParseThreads = 8;
    const uint32_t hardwareThreads = std::max(1u, static_cast<uint32_t>(std::thread::hardware_concurrency()));
    const uint32_t threads =
        std::min({maxParseThreads, hardwareThreads, static_cast<uint32_t>(kernelParseTasks_.size())});
    LogDebug("Device kernel parsing started: %zu tasks, %u threads.", kernelParseTasks_.size(), threads);
    std::atomic<uint32_t> failedCount{0};
    std::vector<std::string> taskLogs(kernelParseTasks_.size());
    auto runTask = [this, &failedCount, &taskLogs](size_t i) {
        Utility::LogCaptureGuard guard(&taskLogs[i]);
        if (!RunKernelParseTask(kernelParseTasks_[i])) {
            failedCount.fetch_add(1, std::memory_order_relaxed);
        }
    };
    Utility::ThreadPool pool(threads);
    if (threads > 1 && pool.Start()) {
        for (size_t i = 0; i < kernelParseTasks_.size(); ++i) {
            pool.AddTask([&runTask, i] { runTask(i); });
        }
        pool.WaitAllTasks();
        pool.Stop();
    } else {
        // 单算子或多线程启动失败，走原串行解析
        for (size_t i = 0; i < kernelParseTasks_.size(); ++i) {
            runTask(i);
        }
    }
    failedKernelNum_ += failedCount.load(std::memory_order_relaxed);
    double elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    for (const auto &taskLog : taskLogs) {
        Utility::Log::FlushCapture(taskLog);
    }
    LogDebug(
        "Device kernel parsing finished: %zu tasks, %u threads, %.3f ms.", kernelParseTasks_.size(), threads, elapsed);
    kernelParseTasks_.clear();
}

void DeviceDataParse::CopyDfxRegionFiles(
    const string &kernelDir, const vector<string> &fileNames, const regex &pattern) {
    // 多launch场景下，dfx交付件只由第一次launch生成，需要在每个算子解析前，先将交付件复制到对应的dump目录下
    if (!metrics_.instrTimelineEnable) {
        return;
    }
    const vector<string> dfxFiles = {DFX_TUNE_LOG, DFX_REGION_MAP};
    for (const auto &fileName : fileNames) {
        if (!regex_match(fileName, pattern) || fileName == "0") {
            continue;
        }
        for (const auto &file : dfxFiles) {
            string destPath = JoinPath({kernelDir, fileName, "dump", file});
            if (IsExist(destPath)) {
                continue;
            }
            CopyFile(JoinPath({kernelDir, "0/dump", file}), destPath);
        }
    }
}

bool DeviceDataParse::ParserInit()
{
    auto it = chipInfoMap_.find(chipType_);
    if (it == chipInfoMap_.end()) {
        LogError("Analyzing profiling data failed for this chip type is not supported.");
        return false;
    }
    chipInfo_ = it->second;
    GetCalMetricItems(metrics_);
    eventMap_ = GetProfBinInfo(metrics_, pmuEventsId_);
    return true;
}

void DeviceDataParse::CopyTimeStamp(const std::string &filePath) const
{
    using namespace std::experimental::filesystem;
    std::experimental::filesystem::path outputPath {filePath};
    for (const auto &dirs : directory_iterator(outputPath)) {
        std::string curpath = dirs.path().string();
        std::string deviceId = dirs.path().filename();
        std::string timeStampPath = JoinPath({curpath, "aic_timestamp.bin"});
        if (IsExist(timeStampPath)) {
            RollbackPath(curpath, 2);
            std::string searchPath = JoinPath({curpath, deviceId});
            std::vector<std::string> results;
            SearchDirRecursive(searchPath, "dump", results, 0);
            for (const auto &res : results) {
                CopyFile(timeStampPath, res);
            }
        }
    }
}

bool DeviceDataParse::Execute(std::string dataPath)
{
    if (!ParserInit()) {
        return false;
    }
    std::string tmpPath = JoinPath({dataPath, "tmp_dump"});
    ParseTmpDump(tmpPath);
    if (!ParserDeviceIdDir(dataPath)) {
        return false;
    }
    ParseKernelTasks();
    bool summarySaved = SaveSummary(dataPath);
    if (IsExist(tmpPath) && (Utility::Log::GetLog().GetLogLv() > Utility::LogLv::DEBUG)) {
        RemoveAll(tmpPath);
    }
    return ExecuteSummary(dataPath) && summarySaved;
}

void DeviceDataParse::RecordSummary(
    const DataHandler &handler, const std::string &path, const MergeTimelineData &timeline) {
    const auto &info = handler.GetOpInfo();
    const auto device = info.find("Device Id");
    uint32_t deviceId = 0;
    if (device == info.end() || !StringToNum(device->second, deviceId)) {
        LogWarn("Skipping summary row without valid device id: %s", path.c_str());
        return;
    }
    std::lock_guard<std::mutex> lock(summaryMutex_);
    if (summaryHeader_.empty()) {
        summaryHeader_ = handler.opBasicHeader;
        // "Device Id"放首列
        const auto column = std::find(summaryHeader_.begin(), summaryHeader_.end(), "Device Id");
        if (column != summaryHeader_.end()) {
            std::rotate(summaryHeader_.begin(), column, column + 1);
        }
    }
    summaryRows_.push_back({deviceId, handler.GetStartTime(), CsvParser::AddRow(summaryHeader_, info), path});
    uint64_t executionIndex = 0;
    const auto name = info.find("Op Name");
    if (name == info.end() || timeline.timelineJson.empty()) {
        return;
    }
    // 有custom_input合并配置时优先按记录目录配组，否则只自动合并AICPU通信的MC2和LCCL算子
    const int64_t cycleFreq = timeline.isCommunication ? handler.taskSchedulerFreq_ : handler.aicoreFreq_;
    if (!CustomInputConfig::Instance().GetTimelineMergeConfigs().empty()) {
        timelineInputsByPath_[path] = {name->second, deviceId, 0, timeline, cycleFreq};
    } else if (timeline.isCommunication && (handler.GetMC2Flag() || handler.GetLcclFlag()) &&
        StringToNum(GetLastFile(path), executionIndex)) {
        timelineInputs_.push_back({name->second, deviceId, executionIndex, timeline, cycleFreq});
    }
}

static bool GetStartTime(const std::vector<TimelineInput> &inputs, uint64_t &startTime) {
    startTime = UINT64_MAX;
    for (const auto &input : inputs) {
        const auto &trace = input.timeline.timelineJson;
        if ((input.timeline.isCommunication && input.timeline.minCycle == UINT64_MAX) || input.cycleFreq <= 0 ||
            input.timeline.isCommunication != inputs.front().timeline.isCommunication || !trace.is_object() ||
            !trace.contains("traceEvents") || !trace["traceEvents"].is_array() || trace["traceEvents"].empty()) {
            return false;
        }
        startTime = input.timeline.isCommunication ? std::min(startTime, input.timeline.minCycle) : 0;
    }
    return startTime != UINT64_MAX;
}

static std::string GetTimelineDirectoryName(const std::string &name, const std::string &mstxName = "") {
    // 各组成保留前120字符，避免合并后再截断时 mstx_name 被整体丢弃；序号后缀由调用方追加。
    constexpr size_t maxComponentLength = 120;
    auto kernelName = name;
    kernelName.erase(std::remove(kernelName.begin(), kernelName.end(), '*'), kernelName.end());
    const auto truncatedName = Utility::TruncateString(kernelName, maxComponentLength);
    if (mstxName.empty()) {
        return truncatedName;
    }
    const auto truncatedMstxName = Utility::TruncateString(mstxName, maxComponentLength);
    return truncatedMstxName + "_" + truncatedName;
}

json DeviceDataParse::MergeTimelineGroup(std::vector<TimelineInput> inputs) {
    uint64_t startTime = 0;
    if (inputs.size() < 2 || !GetStartTime(inputs, startTime)) {
        return json();
    }
    std::sort(inputs.begin(), inputs.end(),
        [](const TimelineInput &a, const TimelineInput &b) { return a.deviceId < b.deviceId; });
    json events = json::array();
    for (auto &input : inputs) {
        const std::string prefix = "Device " + std::to_string(input.deviceId);
        // 通算流水按系统cycle对齐；BIU流水已按通道累计cycle，不再叠加卡间基准。
        const double timeOffset = input.timeline.isCommunication
            ? static_cast<double>(input.timeline.minCycle - startTime) / input.cycleFreq * TIME_CONVERSION
            : 0.0;
        for (auto &event : input.timeline.timelineJson["traceEvents"]) {
            if (!event.is_object()) {
                continue;
            }
            const bool metadata = event.value("ph", json()) == "M";
            if (!metadata && event.contains("ts")) {
                if (!event["ts"].is_number()) {
                    continue;
                }
                const double ts = event["ts"].get<double>() + timeOffset;
                event["ts"] = ts;
            }
            event["pid"] = prefix + " | " + event["pid"].get<std::string>();
            events.push_back(std::move(event));
        }
    }
    // 复用首卡根字段
    json result = std::move(inputs.front().timeline.timelineJson);
    result["traceEvents"] = std::move(events);
    return result;
}

static TimelineGroups GroupTimelines(std::vector<TimelineInput> inputs) {
    // 按算子名称和执行编号对各卡流水自动配组
    std::map<std::pair<std::string, uint64_t>, std::vector<TimelineInput>> groups;
    for (auto &input : inputs) {
        groups[{input.opName, input.executionIndex}].push_back(std::move(input));
    }
    TimelineGroups result;
    for (auto &group : groups) {
        result.push_back(std::move(group.second));
    }
    return result;
}

bool DeviceDataParse::MergeTimelines(TimelineGroups groups, const std::string &summaryPath) {
    // 根据分组合并流水图，兼容通算、pipe、instr流水
    bool res = true;
    size_t mergedCount = 0;
    for (auto &members : groups) {
        if (members.size() < 2) {
            continue;
        }
        const auto name = members.front().opName;
        const auto executionIndex = members.front().executionIndex;
        const auto directoryName =
            members.front().mergeDirectory.empty() ? GetTimelineDirectoryName(name) : members.front().mergeDirectory;
        const auto mergedTimelineJson = MergeTimelineGroup(std::move(members));
        if (mergedTimelineJson.is_null()) {
            LogWarn("Failed to merge timeline for operator [%s], execution [%llu].", name.c_str(),
                static_cast<unsigned long long>(executionIndex));
            res = false;
            continue;
        }
        const auto directory = JoinPath({summaryPath, directoryName, std::to_string(executionIndex)});
        if (!MkdirRecusively(directory)) {
            res = false;
            continue;
        }
        const auto tracePath = JoinPath({directory, "trace.json"});
        if (!WriteFileByStream(tracePath, mergedTimelineJson.dump())) {
            res = false;
            continue;
        }
        Utility::Visualize::WriteBin<Utility::VisualizeType::TRACE>(directory, mergedTimelineJson);
        ++mergedCount;
    }
    if (mergedCount != 0) {
        LogInfo("Saved %zu merged timelines in %s.", mergedCount, summaryPath.c_str());
    }
    return res;
}

void DeviceDataParse::ReadTimelineMstxPaths(const std::string &dataPath, std::vector<TimelineMstxPaths> &recorded) {
    const auto path = JoinPath({dataPath, TMP_DUMP, "timeline_mstx.json"});
    std::ifstream stream(path);
    if (!stream.is_open()) {
        LogWarn("Cannot open timeline mstx records: %s, maybe matched empty.", path.c_str());
        return;
    }
    std::string line;
    while (std::getline(stream, line)) {
        // 文件每行一条JSON记录，字段无效时告警并跳过，继续读取后续记录
        const auto record = json::parse(line, nullptr, false);
        if (!record.is_object() || !record.contains("config_index") || !record.contains("device_id") ||
            !record.contains("output_path")) {
            LogWarn("Invalid timeline mstx record in %s: missing required fields.", path.c_str());
            continue;
        }
        const auto &configIndex = record["config_index"];
        const auto &deviceId = record["device_id"];
        if (!configIndex.is_number_integer() || configIndex < 0 || configIndex >= recorded.size()) {
            LogWarn("Invalid timeline mstx record in %s: invalid config_index.", path.c_str());
            continue;
        }
        if (!deviceId.is_number_integer() || deviceId < 0 || deviceId > UINT32_MAX) {
            LogWarn("Invalid timeline mstx record in %s: invalid device_id.", path.c_str());
            continue;
        }
        if (!record["output_path"].is_string()) {
            LogWarn("Invalid timeline mstx record in %s: output_path must be a string.", path.c_str());
            continue;
        }
        auto output = record["output_path"].get<std::string>();
        if (output.empty()) {
            continue;
        }
        if (GetLastFile(output) == "dump") {
            RollbackPath(output, 1);
        }
        // 按配置、设备保存算子目录，保留每卡记录顺序，供后续按相同位置配组
        recorded[configIndex.get<size_t>()][deviceId.get<uint32_t>()].push_back(std::move(output));
    }
}

static std::vector<TimelineInput> MatchRecordedTimelines(
    const TimelineMstxPaths &devicePaths, size_t index, const TimelineInputsByPath &inputs, const std::string &prefix) {
    std::vector<TimelineInput> matched;
    // 取各卡目录列表的第index项，按目录查找流水并核对设备号
    for (const auto &device : devicePaths) {
        const auto output = index < device.second.size() ? device.second[index] : "";
        const auto input = inputs.find(output);
        if (output.empty() || input == inputs.end() || input->second.deviceId != device.first) {
            LogWarn("%slaunch %zu: missing valid single-card timeline for device%u [%s].", prefix.c_str(), index,
                device.first, output.c_str());
        } else {
            matched.push_back(input->second);
        }
    }
    return matched;
}

void DeviceDataParse::AddTimelineMstxGroups(const TimelineMergeConfig &config, const TimelineMstxPaths &devicePaths,
    const TimelineInputsByPath &inputs, const std::string &prefix, const std::string &directoryName,
    TimelineGroups &groups) {
    if (devicePaths.empty()) {
        LogWarn("%sno timeline mstx records matched mstx_name=%s, kernel_name=%s.", prefix.c_str(),
            config.mstxName.c_str(), config.kernelName.c_str());
        return;
    }
    if (devicePaths.size() > config.rankCount) {
        LogWarn("%srank_count=%u, actual devices=%zu exceed the limit, skip this configuration.", prefix.c_str(),
            config.rankCount, devicePaths.size());
        return;
    }
    // 取各卡记录数的最大值，检查配置次数上限内的缺卡位置
    size_t launchCount = 0;
    std::vector<std::string> counts;
    for (const auto &device : devicePaths) {
        launchCount = std::max(launchCount, device.second.size());
        counts.push_back("device" + std::to_string(device.first) + "=" + std::to_string(device.second.size()));
    }
    // 按各卡列表的相同位置配组，仅保留所有设备流水齐全的组，执行编号统一为列表位置
    size_t preparedCount = 0;
    for (size_t index = 0; index < std::min(launchCount, static_cast<size_t>(config.launchCount)); ++index) {
        auto matched = MatchRecordedTimelines(devicePaths, index, inputs, prefix);
        if (matched.size() != devicePaths.size()) {
            continue;
        }
        for (auto &input : matched) {
            input.executionIndex = index;
            input.mergeDirectory = directoryName;
        }
        groups.push_back(std::move(matched));
        ++preparedCount;
    }
    if (preparedCount < config.launchCount) {
        LogInfo("%slaunch_count=%u, prepared=%zu, captured records [%s].", prefix.c_str(), config.launchCount,
            preparedCount, Utility::Join(counts.begin(), counts.end()).c_str());
    }
}

TimelineGroups DeviceDataParse::GetTimelineGroups(const std::string &dataPath) {
    const auto &configs = CustomInputConfig::Instance().GetTimelineMergeConfigs();
    if (configs.empty()) {
        // AICPU通信的MC2或LCCL算子
        return GroupTimelines(std::move(timelineInputs_));
    }
    std::vector<TimelineMstxPaths> recorded(configs.size());
    ReadTimelineMstxPaths(dataPath, recorded);
    TimelineGroups groups;
    std::map<std::string, size_t> directoryCounts;
    for (const auto &config : configs) {
        ++directoryCounts[GetTimelineDirectoryName(config.kernelName, config.mstxName)];
    }
    std::map<std::string, size_t> directoryIndices;
    for (size_t configIndex = 0; configIndex < configs.size(); ++configIndex) {
        const auto prefix = "timeline_merge_config[" + std::to_string(configIndex) + "] ";
        const auto &config = configs[configIndex];
        const auto baseName = GetTimelineDirectoryName(config.kernelName, config.mstxName);
        std::string suffix;
        // 同名配置按其出现顺序加_0、_1等后缀，各配置的所有launch共用同一个一级目录。
        if (directoryCounts[baseName] > 1) {
            suffix = "_" + std::to_string(directoryIndices[baseName]++);
        }
        const auto directoryName = baseName + suffix;
        AddTimelineMstxGroups(config, recorded[configIndex], timelineInputsByPath_, prefix, directoryName, groups);
    }
    return groups;
}

bool DeviceDataParse::SaveSummary(const std::string &dataPath) {
    // 单条算子记录无需汇总或多卡合并，直接使用原有单算子产物
    if (summaryRows_.size() <= 1) {
        return true;
    }
    const auto directory = JoinPath({dataPath, "summary"});
    if (!MkdirRecusively(directory)) {
        return false;
    }
    if (!MergeTimelines(GetTimelineGroups(dataPath), directory)) {
        LogWarn("Failed to save some merged timelines in %s.", directory.c_str());
    }
    auto rows = summaryRows_;
    // 按"Device Id"和算子执行时间排序
    std::stable_sort(rows.begin(), rows.end(), [](const SummaryRow &a, const SummaryRow &b) {
        return std::make_tuple(a.deviceId, a.startTime == 0, a.startTime) <
            std::make_tuple(b.deviceId, b.startTime == 0, b.startTime);
    });
    CsvData values;
    for (const auto &row : rows) {
        values.push_back(row.values);
    }
    return DataSave().CsvSave(JoinPath({directory, "summary.csv"}), summaryHeader_, values);
}

// classify all metric items to aic or aiv
void DeviceDataParse::GetCalMetricItems(const ProfMetricsAbilityConfig &metrics)
{
    for (const ProfMetrics &selectMetric: metrics.Enabled()) {
        string metricName = Mapping::onBoardMetricsToMsprof[selectMetric];
        // 若不在metricHeader中，说明此指标不需要写入csv文件
        if (chipInfo_.metricHeader.count(metricName) == 0) {
            LogDebug("%s does not have to write csv.", metricName.c_str());
            continue;
        }
        const vector<string> &headers = chipInfo_.metricHeader.at(metricName);
        for (const string &h : headers) {
            if (StartsWith(h, "aic") || count(ExtraAicMetricItems.begin(), ExtraAicMetricItems.end(), h) > 0) {
                aicCalMetricItems_.insert(h);
            }
            if (StartsWith(h, "aiv") || count(ExtraAivMetricItems.begin(), ExtraAivMetricItems.end(), h) > 0) {
                aivCalMetricItems_.insert(h);
            }
        }
    }
}

// get aicEvents, aivEvents and related metrics of DeviceProfX.bin
std::map<std::string, ProfBinInfo, FileNameCompare> DeviceDataParse::GetProfBinInfo(const ProfMetricsAbilityConfig &metrics,
                                                         const PmuEventsId &pmuEventsId) const
{
    std::map<std::string, ProfBinInfo, FileNameCompare> res;
    // 不采集pmu时仅仅完成1轮空的DeviceProf1.bin的解析，用于获取算子类型
    if (!metrics.HasEnabledPmu()) {
        ProfBinInfo info;
        info.aicEvents = chipProductType_ == ChipProductType::ASCEND950_SERIES ? vector<uint16_t>(PMU_EVENT_MAX_NUM_A5, 0) : vector<uint16_t>(PMU_EVENT_MAX_NUM, 0);
        info.aivEvents = chipProductType_ == ChipProductType::ASCEND950_SERIES ? vector<uint16_t>(PMU_EVENT_MAX_NUM_A5, 0) : vector<uint16_t>(PMU_EVENT_MAX_NUM, 0);
        res.insert({std::string {MSPROF_DUMPFILE_PREFIX} + "1.bin", info});
        return res;
    }
    // aicRelatedMap/aivRelatedMap:{{pmu event, metric names vector}} eg:{1280, {"memory", "pipeutilization"}}
    map<uint16_t, vector<string>> aicRelatedMap;
    map<uint16_t, vector<string>> aivRelatedMap;
    vector<uint16_t> aicEvents = pmuEventsId.aicPmu;
    vector<uint16_t> aivEvents = pmuEventsId.aivPmu;
    GetRelatedMap(metrics, chipInfo_.aicMetricEventsMap, aicRelatedMap);
    GetRelatedMap(metrics, chipInfo_.aivMetricEventsMap, aivRelatedMap);
    uint16_t maxSize = max(aicEvents.size(), aivEvents.size());
    int size = PMU_EVENT_MAX_NUM;
    if (chipProductType_ == ChipProductType::ASCEND950_SERIES) {
        size = PMU_EVENT_MAX_NUM_A5;
    }
    int profBinNum = static_cast<int>((maxSize + size - 1) / size);
    aicEvents.resize(profBinNum * size, 0);
    aivEvents.resize(profBinNum * size, 0);

    for (int i = 0; i < profBinNum; i++) {
        ProfBinInfo info;
        info.aicEvents = vector<uint16_t>{&aicEvents[i * size], &aicEvents[(i + 1) * size]};
        info.aivEvents = vector<uint16_t>{&aivEvents[i * size], &aivEvents[(i + 1) * size]};
        for (int idx = 0; idx < size; ++idx) {
            info.relatedMetrics.insert(aicRelatedMap[info.aicEvents[idx]].begin(),
                                       aicRelatedMap[info.aicEvents[idx]].end());
            info.relatedMetrics.insert(aivRelatedMap[info.aivEvents[idx]].begin(),
                                       aivRelatedMap[info.aivEvents[idx]].end());
        }
        res.insert({MSPROF_DUMPFILE_PREFIX + to_string(i + 1) + ".bin", info});
    }
    return res;
}

void DeviceDataParse::GetRelatedMap(const ProfMetricsAbilityConfig &metrics,
                                    const MetricEventsMapType &metricEvents,
                                    map<uint16_t, vector<string>> &relatedMap) const
{
    for (const ProfMetrics &metric : metrics.Enabled()) {
        if (metricEvents.count(Mapping::onBoardMetricsToMsprof[metric]) == 0) {
            continue;
        }
        for (const uint16_t &eventId : metricEvents.at(Mapping::onBoardMetricsToMsprof[metric])) {
            relatedMap[eventId].emplace_back(Mapping::onBoardMetricsToMsprof[metric]);
        }
    }
}

bool DeviceDataParse::CheckKernelFiles(const std::string &path, vector<std::string> &fileNames, string &errorMsg) const
{
    for (const std::string &fileName : fileNames) {
        std ::string dumpFilePath = Utility::JoinPath({path, fileName});
        if (IsDir(dumpFilePath)) { continue; }
        Utility::CheckPermission(dumpFilePath);
        if (!Utility::CheckInputFileValid(dumpFilePath, "bin", INPUT_BINARY_FILE_MAX_SIZE)) {
            errorMsg = "file in dir is invalid: " + dumpFilePath;
            return false;
        }
    }
    return true;
}

void DeviceDataParse::GetRangeFreq(const string &path, vector<string> &freqs) const
{
    string currentFreqStr = "Current Freq=NA";
    string ratedFreqStr = "Rated Freq=NA";
    freqs = {currentFreqStr, ratedFreqStr};
    string freqFilePath = Realpath(JoinPath({path, "freq.txt"}));
    if (!IsReadable(freqFilePath)) {
        LogWarn("Access [%s] failed, maybe is not exist or readable.", freqFilePath.c_str());
        return;
    }
    ifstream file(freqFilePath);
    if (!file.is_open()) {
        LogWarn("Failed to open file %s", freqFilePath.c_str());
        return;
    }
    string line;
    while (getline(file, line)) {
        if (StartsWith(line, "Current Freq")) {
            currentFreqStr = line;
        } else {
            if (StartsWith(line, "Rated Freq")) {
                ratedFreqStr = line;
            }
        }
    }
    freqs = {currentFreqStr, ratedFreqStr};
    file.close();
}

void DeviceDataParse::GetRangeKernelDurBin(const string &path)
{
    uint16_t acsqLength = ACSQ_LENGTH;
    if (chipProductType_ == ChipProductType::ASCEND950_SERIES) {
        acsqLength = ACSQ_LENGTH_A5;
    }
    string durFilePath = JoinPath({path, "duration.bin"});
    size_t fileSize = GetFileSize(durFilePath);
    vector<char> binData;
    if (fileSize < acsqLength || !ReadBinFileByMultiStruct(durFilePath, fileSize, acsqLength, binData)) {
        LogWarn("Access [%s] failed, maybe is not exist, readable or complete.", durFilePath.c_str());
        return;
    }
    for (size_t i = 0; i < fileSize; i = i + acsqLength) {
        vector<char> splitBinData{&binData[i], &binData[i] + acsqLength};
        AcsqBean acsqBean(chipProductType_, splitBinData);
        uint16_t streamId = acsqBean.GetStreamId();
        uint16_t taskId = acsqBean.GetTaskId();
        rangeReplayDurBinMap_[{streamId, taskId}].insert(rangeReplayDurBinMap_[{streamId, taskId}].end(),
                                                         splitBinData.begin(), splitBinData.end());
    }
}

void DeviceDataParse::GenerateRangeKernelBin(const vector<string> &outputVec, const vector<char> &profBinData,
    const pair<uint16_t, uint16_t> &streamAndTaskId, const std::string &path, int round, size_t kernelIndex) {
    if (kernelIndex > outputVec.size() - 1) {
        LogWarn("Cannot find output path, replay count is %d, kernel index is %d.", round, kernelIndex);
        return;
    }
    string outputPath = outputVec[kernelIndex];
    if (outputPath == "-1") {
        // invalid output path
        return;
    }

    string profBinPath = JoinPath({outputPath, "DeviceProf" + to_string(round) + ".bin"});
    WriteBinaryFile(profBinPath, profBinData.data(), profBinData.size());

    // duration.bin only generate when replay count is 1, use streamId and taskId to match kernel
    if (round != 1) {
        return;
    }
    auto iter = rangeReplayDurBinMap_.find(streamAndTaskId);
    if (iter == rangeReplayDurBinMap_.end()) {
        return;
    }
    string durBinPath = JoinPath({outputPath, "duration.bin"});
    WriteBinaryFile(durBinPath, iter->second.data(), iter->second.size());
    rangeReplayDurBinMap_.erase(streamAndTaskId);
}

void DeviceDataParse::ParseRangeKernelProfBin(const string &path, const vector<string> &outputVec, int round)
{
    string fileName = "DeviceProf" + to_string(round) + ".bin";
    string binFilePath = JoinPath({path, fileName});
    size_t fileSize = GetFileSize(binFilePath);
    vector<char> binData;
    if (fileSize < FFTS_LENGTH || !ReadBinFileByMultiStruct(binFilePath, fileSize, FFTS_LENGTH, binData)) {
        LogWarn("Access [%s] failed, maybe is not exist, readable or complete.", binFilePath.c_str());
        return;
    }
    vector<char> profBinData;
    int32_t currentTaskId = -1;
    int32_t currentStreamId = -1;
    int kernelIndex = -1;
    for (size_t i = 0; i < fileSize; i = i + FFTS_LENGTH) {
        vector<char> splitBinData{&binData[i], &binData[i] + FFTS_LENGTH};
        FftsBlockBean fftsBlockBean(chipProductType_, splitBinData);
        uint16_t taskId = fftsBlockBean.GetTaskId();
        uint16_t streamId = fftsBlockBean.GetStreamId();
        if (currentTaskId != taskId || currentStreamId != streamId) {
            if (currentTaskId != -1 && currentStreamId != -1) {
                GenerateRangeKernelBin(outputVec, profBinData, {static_cast<uint16_t>(currentStreamId), static_cast<uint16_t>(currentTaskId)}, path,
                                       round, static_cast<size_t>(kernelIndex));
                profBinData = {};
            }
            currentTaskId = taskId;
            currentStreamId = streamId;
            kernelIndex += 1;
        }
        profBinData.insert(profBinData.end(), splitBinData.begin(), splitBinData.end());
    }
    if (kernelIndex != -1) {
        GenerateRangeKernelBin(outputVec, profBinData, {static_cast<uint16_t>(currentStreamId), static_cast<uint16_t>(currentTaskId)}, path,
                               round, static_cast<size_t>(kernelIndex));
    }
}

void DeviceDataParse::ParseSingleRangeData(const string &path)
{
    // get range replay operators output paths, one line for an operator, output path will be "-1" if no need to parse
    string outputFilePath = Realpath(JoinPath({path, "output.txt"}));
    if (!IsReadable(outputFilePath)) {
        LogWarn("Access [%s] failed, maybe is not exist or readable.", outputFilePath.c_str());
        return;
    }
    ifstream file(outputFilePath);
    if (!file.is_open()) {
        LogWarn("Failed to open file %s", outputFilePath.c_str());
        return;
    }
    // get range replay current and rated frequency
    vector<string> freqs;
    GetRangeFreq(path, freqs);

    vector<string> outputVec;
    int validPathCount = 0;
    string output;
    while (getline(file, output)) {
        if (output != "-1") {
            if (IsDir(output) && IsWritable(output)) {
                validPathCount += 1;
                // Add "Current Freq/Rated Freq" to op_basic_info.txt
                AppendLinesToFile(JoinPath({output, "op_basic_info.txt"}), freqs);
            } else {
                LogWarn("Kernel dump path [%s] is not exist or not writeable.", output.c_str());
                output = "-1";
            }
        }
        outputVec.emplace_back(output);
    }
    file.close();
    if (validPathCount == 0) {
        LogDebug("No need to parse range replay data in [%s].", path.c_str());
        return;
    }
    // get range replay duration
    GetRangeKernelDurBin(path);
    int profBinCount = 1;
    if (!metrics_.isBasicInfo || metrics_.HasEnabledPmu()) {
        if (chipProductType_ == ChipProductType::ASCEND950_SERIES) {
            profBinCount = static_cast<int>(max(REPLAY_AIC_EVENTS_FOR_A5.size(), REPLAY_AIV_EVENTS_FOR_A5.size()) / PMU_EVENT_MAX_NUM_A5);
        } else {
            profBinCount = static_cast<int>(max(REPLAY_AIC_EVENTS_FOR_910B.size(), REPLAY_AIV_EVENTS_FOR_910B.size()) / PMU_EVENT_MAX_NUM);
        }
    }
    for (int i = 1; i <= profBinCount; i++) {
        ParseRangeKernelProfBin(path, outputVec, i);
    }
}

void DeviceDataParse::ParseTmpDump(const string &tmpPath)
{
    if (!IsDir(tmpPath)) {
        return;
    }
    CopyTimeStamp(tmpPath);
    vector<string> deviceIds;
    if (!GetFileNames(tmpPath, deviceIds)) {
        LogDebug("No file in tmp_dump.");
        return;
    }
    // device dir
    for (const string &deviceId : deviceIds) {
        string deviceIdDir = JoinPath({tmpPath, deviceId});
        if (!StartsWith(deviceId, "device") || !IsDir(deviceIdDir)) {
            continue;
        }
        // pid dir
        vector<string> pids;
        if (!GetFileNames(deviceIdDir, pids)) {
            continue;
        }
        regex pidPattern("[0-9]{1,32}");
        for (const auto &pid : pids) {
            string pidsDir = JoinPath({deviceIdDir, pid});
            if (!regex_match(pid, pidPattern) || !IsDir(pidsDir)) {
                continue;
            }
            vector<string> orders;
            if (!GetFileNames(pidsDir, orders)) {
                continue;
            }
            // order dir
            regex orderPattern("[0-9]{1,3}");
            for (const auto &order : orders) {
                string orderDir = JoinPath({pidsDir, order});
                if (!regex_match(order, orderPattern) || !IsDir(orderDir)) {
                    continue;
                }
                ParseSingleRangeData(orderDir);
            }
        }
    }
}
}
