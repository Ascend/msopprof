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
#include <functional>
#include "smart_pointer.h"
#include "log.h"
#include "filesystem.h"
#include "parse/plugin/plugin_manager.h"
#include "parse/data_calculator/compute_load_calculator.h"

using namespace Visualize;
using namespace Utility;
using namespace Common;
using namespace std;

namespace Profiling {
DeviceDataParse::DeviceDataParse(Common::ChipType chipType, PmuEventsId pmuEventsId, ProfMetricsAbilityConfig metrics,
    const std::string &kernelNameFilter, const std::string &customDotJson)
    : DataParse(std::move(metrics), kernelNameFilter), chipType_(chipType), pmuEventsId_(pmuEventsId), customDotJson_(customDotJson) {
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

unique_ptr<DataHandler> &GetHandle(ChipType chipType)
{
    static unique_ptr<DataHandler> handlePtr;
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
    handlePtr = std::move(createInstance(type));
    return handlePtr;
}

unique_ptr<Visualize::PmuCalculator> &GetPmuCalculatorObj(ChipType chipType, shared_ptr<BasicPmu> &basicPmuObj)
{
    static unique_ptr<Visualize::PmuCalculator> pmuCalculatorPtr;
    if (chipType == ChipType::ASCEND910B) {
        pmuCalculatorPtr = Utility::MakeUnique<Visualize::PmuCalculator910B>();
    } else if (chipType == ChipType::ASCEND310P) {
        pmuCalculatorPtr = Utility::MakeUnique<Visualize::PmuCalculator310P>();
    } else {
        pmuCalculatorPtr = Utility::MakeUnique<Visualize::PmuCalculatorA5>();
    }
    if (pmuCalculatorPtr == nullptr) {
        return pmuCalculatorPtr;
    }
    pmuCalculatorPtr->Init(basicPmuObj);
    return pmuCalculatorPtr;
}

unique_ptr<Visualize::StorageAccess> &GetStorageAccessObj(ChipType chipType,
                                                          shared_ptr<Visualize::OpBasicInfo> &opBasicInfoObj,
                                                          shared_ptr<Visualize::BasicPmu> &basicPmuObj,
                                                          unique_ptr<Visualize::PmuCalculator> &pmuCalculatorObj)
{
    static unique_ptr<Visualize::StorageAccess> storageAccessPtr;
    if (chipType == ChipType::ASCEND910B) {
        storageAccessPtr = Utility::MakeUnique<StorageAccess910B>(opBasicInfoObj, basicPmuObj, pmuCalculatorObj);
    } else if (chipType == ChipType::ASCEND310P) {
        storageAccessPtr = Utility::MakeUnique<StorageAccess310P>(opBasicInfoObj, basicPmuObj, pmuCalculatorObj);
    } else {
        storageAccessPtr = Utility::MakeUnique<StorageAccessA5>(opBasicInfoObj, basicPmuObj, pmuCalculatorObj);
    }
    return storageAccessPtr;
}

unique_ptr<Visualize::Occupancy> &GetOccupancyObj(ChipType chipType,
                                                  shared_ptr<Visualize::OpBasicInfo> &opBasicInfoObj,
                                                  shared_ptr<Visualize::BasicPmu> &basicPmuObj)
{
    static unique_ptr<Visualize::Occupancy> occupancyPtr;
    if (chipType == ChipType::ASCEND910B) {
        occupancyPtr = Utility::MakeUnique<Occupancy910B>(opBasicInfoObj, basicPmuObj);
    } else if (chipType == ChipType::ASCEND950) {
        occupancyPtr = Utility::MakeUnique<OccupancyA5>(opBasicInfoObj, basicPmuObj);
    } else {
        occupancyPtr = Utility::MakeUnique<Occupancy>(opBasicInfoObj, basicPmuObj);
    }
    return occupancyPtr;
}

unique_ptr<RoofLine> &GetRoofLineObj(unique_ptr<DataHandler> &handler, shared_ptr<OpBasicInfo> &opBasicInfoObj,
                                     shared_ptr<BasicPmu> &basicPmuObj, unique_ptr<PmuCalculator> &pmuCalculatorObj)
{
    static unique_ptr<Visualize::RoofLine> roofLinePtr;
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

unique_ptr<Visualize::TimelineParser> &GetTimelineObj(unique_ptr<DataHandler> &handler,
                                                      shared_ptr<OpBasicInfo> &opBasicInfoObj,
                                                      shared_ptr<BasicPmu> &basicPmuObj,
                                                      const std::string &customDotJson)
{
    static unique_ptr<Visualize::TimelineParser> parserPtr;
    if (handler == nullptr || opBasicInfoObj == nullptr || basicPmuObj == nullptr) {
        LogWarn("Get MC2 timeline failed because of nullptr");
        return parserPtr;
    }
    if (handler->GetMC2Flag()) {
        parserPtr = Utility::MakeUnique<Visualize::MC2TimelineParser>(handler->GetAcsqTimeMap(),
        handler->GetMinMc2TimeCyc(), opBasicInfoObj, basicPmuObj);
    } else if (handler->GetLcclFlag()) {
        parserPtr = Utility::MakeUnique<Visualize::LcclTimelineParser>(
        handler->GetMinLcclTimeCyc(), opBasicInfoObj, basicPmuObj);
    } else {
        parserPtr = Utility::MakeUnique<Visualize::AicoreTimelineParser>(
        handler->GetMinTimeCyc(), opBasicInfoObj, basicPmuObj, customDotJson);
    }
    return parserPtr;
}

unique_ptr<Visualize::CachelineHeatMap> GetCachelineHeatMapObj(unique_ptr<DataHandler> &handler)
{
    unique_ptr<Visualize::CachelineHeatMap> parserPtr =
        Utility::MakeUnique<Visualize::CachelineHeatMap>(handler->GetL2CacheObj());
    return parserPtr;
}

unique_ptr<Visualize::BiuTimeline> &GetBiuTimelineObj(const Common::ProfMetricsAbilityConfig &metrics)
{
    static unique_ptr<Visualize::BiuTimeline> biuTimelinePtr;
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
    auto &handler = GetHandle(chipType_);
    if (!handler) {
        LogError("Get handle failed because of nullptr");
        return false;
    }
    bool res = handler->ParseDeviceData(parserConfig, eventMap_, metrics_, timeStamp);

    std::shared_ptr<Visualize::OpBasicInfo> opBasicInfoObj =  Utility::MakeShared<Visualize::OpBasicInfo>(handler);
    std::shared_ptr<Visualize::BasicPmu> basicPmuObj = Utility::MakeShared<Visualize::BasicPmu>(handler);
    if (!dataCenter.DataTableRegister(opBasicInfoObj) || !dataCenter.DataTableRegister(basicPmuObj)) {
        LogError("Get op basic info or basic pmu failed because of nullptr");
        return false;
    }
    auto &pmuCalculatorObj = GetPmuCalculatorObj(chipType_, basicPmuObj);
    if (pmuCalculatorObj == nullptr) {
        LogError("Get pmuCalculatorObj failed because of nullptr");
        return false;
    }

    auto &storageAccessObj = GetStorageAccessObj(chipType_, opBasicInfoObj, basicPmuObj, pmuCalculatorObj);
    auto &occupancyObj = GetOccupancyObj(chipType_, opBasicInfoObj, basicPmuObj);
    auto &roofLineObj = GetRoofLineObj(handler, opBasicInfoObj, basicPmuObj, pmuCalculatorObj);
    auto &timelineObj = GetTimelineObj(handler, opBasicInfoObj, basicPmuObj, customDotJson_);
    auto &biuTimelineObj = GetBiuTimelineObj(metrics_);
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

        LogInfo("Start analyze kernel on device: %s, kernel: %s, name is: %s", deviceId.c_str(), kernelName.c_str(),
                fileName.c_str());
        if (!ParseExactKernelData(orderDir, kernelName)) {
            failedKernelNum_++;
            LogError("Analyzing kernel data failed, device: %s, kernel: %s, name is: %s", deviceId.c_str(),
                     kernelName.c_str(), fileName.c_str());
        }

        if (!theOnlyKernelPath_.empty()) {
            isTheOnlyKernel_ = false;
        }
        if (isTheOnlyKernel_) {
            theOnlyKernelPath_ = orderDir;
        }
        totalKernelNum_++;
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
    if (IsExist(tmpPath) && (Utility::Log::GetLog().GetLogLv() > Utility::LogLv::DEBUG)) {
        RemoveAll(tmpPath);
    }
    return ExecuteSummary(dataPath);
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
        if (!Utility::CheckInputFileValid(dumpFilePath, "bin", INPUT_BINARY_FILE_MAX_SIZE) ||
            !Utility::CheckPermission(dumpFilePath)) {
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
