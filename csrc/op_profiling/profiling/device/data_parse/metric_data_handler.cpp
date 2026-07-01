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

#include "metric_data_handler.h"

#include <string>
#include <cmath>
#include "filesystem.h"
#include "data_save.h"
#include "common/defs.h"
#include "common/hal_helper.h"
#include "securec.h"
#include "hotspot_function_generator.h"
#include "dbi_helper.h"
#include "common/runtime_helper.h"

using namespace std;
using namespace Common;
using namespace Utility;
using namespace Visualize;

constexpr uint16_t SUBLOCK_NUM = 3;
constexpr uint16_t VEC0_SUBBLOCK_INDEX = 0;
constexpr uint16_t VEC1_SUBBLOCK_INDEX = 1;
constexpr uint16_t CUBE_SUBBLOCK_INDEX = 2;

namespace Profiling {

// {src, dst} map to record direction
struct MemTypeHash {
    size_t operator()(const std::pair<MemType, MemType>& p) const
    {
        size_t h1 = std::hash<MemType>{}(p.first);
        size_t h2 = std::hash<MemType>{}(p.second);
        return h1 ^ (h2 << 8);
    }
};

// 内存传输规则结构体
struct MemTransferRule {
    std::string reqKey;
    std::string dataKey;
    TransportType transportType;
    bool needExtraUbIdMapping;  // 是否需要额外的 ubId 映射（L1 -> UB 特殊处理）
};

// 初始化内存传输规则映射表
const std::unordered_map<std::pair<MemType, MemType>, std::vector<MemTransferRule>, MemTypeHash>& GetMemTransferRules()
{
    static std::unordered_map<std::pair<MemType, MemType>, std::vector<MemTransferRule>, MemTypeHash> rules = {
        {std::make_pair(MemType::GM, MemType::REG), {
            {GM_TO_DCACHE, GM_TO_DCACHE_DATA, TransportType::GM_TO_DCACHE, false},
            {DCACHE_TO_VEC, DCACHE_TO_VEC_DATA, TransportType::DCACHE_TO_VEC, false}
        }},
        {std::make_pair(MemType::GM, MemType::UB), {
            {GM_TO_UB, GM_TO_UB_DATA, TransportType::GM_TO_UB, false}
        }},
        {std::make_pair(MemType::UB, MemType::L1), {
            {UB_TO_L1, UB_TO_L1_DATA, TransportType::UB_TO_L1, false}
        }},
        {std::make_pair(MemType::UB, MemType::GM), {
            {UB_TO_GM, UB_TO_GM_DATA, TransportType::UB_TO_GM, false}
        }},
        {std::make_pair(MemType::UB, MemType::REG), {
            {UB_TO_VEC, UB_TO_VEC_DATA, TransportType::UB_TO_VEC, false}
        }},
        {std::make_pair(MemType::L1, MemType::UB), {
            {L1_TO_UB, L1_TO_UB_DATA, TransportType::L1_TO_UB, true}
        }},
        {std::make_pair(MemType::REG, MemType::GM), {
            {VEC_TO_DCACHE, VEC_TO_DCACHE_DATA, TransportType::VEC_TO_DCACHE, false},
            {DCACHE_TO_GM, DCACHE_TO_GM_DATA, TransportType::DCACHE_TO_GM, false}
        }},
        {std::make_pair(MemType::REG, MemType::UB), {
            {VEC_TO_UB, VEC_TO_UB_DATA, TransportType::VEC_TO_UB, false}
        }},
        {std::make_pair(MemType::REG, MemType::PRIVATE), {
            {VEC_TO_DCACHE, VEC_TO_DCACHE_DATA, TransportType::VEC_TO_DCACHE, false}
        }},
        {std::make_pair(MemType::PRIVATE, MemType::REG), {
            {DCACHE_TO_VEC, DCACHE_TO_VEC_DATA, TransportType::DCACHE_TO_VEC, false}
        }}
    };
    return rules;
}

// 核间同步指标，仅在当前pmu存在时写入csv进行展示
const std::map<uint16_t, std::string> scalarPmuToIndex = {
    {1792, "scalar_wait_id0_time(us)"},
    {1793, "scalar_wait_id1_time(us)"},
    {1794, "scalar_wait_id2_time(us)"},
    {1795, "scalar_wait_id3_time(us)"},
    {1796, "scalar_wait_id4_time(us)"},
    {1797, "scalar_wait_id5_time(us)"},
    {1798, "scalar_wait_id6_time(us)"},
    {1799, "scalar_wait_id7_time(us)"},
    {1780, "scalar_wait_id8_time(us)"},
    {1781, "scalar_wait_id9_time(us)"},
    {1782, "scalar_wait_id10_time(us)"},
    {1783, "scalar_wait_id11_time(us)"},
    {1784, "scalar_wait_id12_time(us)"},
    {1785, "scalar_wait_id13_time(us)"},
    {1786, "scalar_wait_id14_time(us)"},
    {1787, "scalar_wait_id15_time(us)"}
};

// 通过[起始内存类型,目的内存类型]确定此次record的类型
using SrcToDstRecordMap = unordered_map<pair<MemType, MemType>, uint64_t, MemTypeHash>;

void DataHandler::InitPlatformInfo()
{
    if (!HalHelper::Instance().GetAicoreFreq(aicoreFreq_)) {
        LogWarn("Get aicore frequency failed. Use Default value instead.");
        aicoreFreq_ = defaultDeviceInfo_.aicoreFreq;
    }
    if (!HalHelper::Instance().GetTaskSchedulerFreq(taskSchedulerFreq_)) {
        LogWarn("Get task scheduler frequency failed. Use Default value instead.");
        taskSchedulerFreq_ = defaultDeviceInfo_.taskSchedulerFreq;
    }
    if (!HalHelper::Instance().GetAiCoreNum(aiCoreNum_)) {
        LogWarn("Get aicore number failed. Use Default value instead.");
        aiCoreNum_ = defaultDeviceInfo_.aiCoreNum;
    }
    soc_ = RuntimeHelper::Instance().GetSocVersion();
    if (soc_.empty()) {
        LogWarn("Get soc version failed.");
    }
}

bool DataHandler::ParseL2CacheBin(const std::string &filePath, const std::vector<uint16_t> &pmuVec, SplitBlockPmuData &blockData) const
{
    if (!Utility::IsReadable(filePath)) {
        if (chipType_ == Common::ChipType::ASCEND310P) {
            LogWarn("Access profiling data file [%s] failed, maybe do not contain this file or it is not readable. "
                    "Some indicators of [L2Cache] failed to be collected.", filePath.c_str());
        } else {
            LogWarn("Access profiling data file [%s] failed, maybe do not contain this file or it is not readable. "
                    "Failed to collect l2Cache evict.", filePath.c_str());
        }
        return false;
    }
    size_t fileSize = GetFileSize(filePath);
    if (fileSize == 0) {
        LogWarn("Profiling data file [%s] is empty. Some indicators of [L2Cache] failed to be collected.",
                filePath.c_str());
        return false;
    }
    vector<char> totalBin(fileSize);
    if ((fileSize != L2_CACHE_LENGTH) || !ReadBinaryFile(filePath, totalBin)) {
        LogWarn("Some indicators of [L2Cache] failed to be read.");
        return false;
    }
    L2CacheBean l2CacheBean(totalBin);
    blockData = l2CacheBean.GetL2CacheData(pmuVec);
    return true;
}

bool DataHandler::ParseDeviceData(ParserConfig &config, const std::map<std::string, ProfBinInfo, FileNameCompare> &profBinMap,
    const Common::ProfMetricsAbilityConfig &metrics, const std::string &timeStamp)
{
    profilingFileTimeStamp_ = timeStamp;
    bool res = true;
    if (!LoadOpBasicInfoTxtFile(config.outputPath)) {
        LogWarn("Failed to analyze op basic info");
        res = false;
    }
    string pcSamplingBinPath = JoinPath({config.outputPath, "dump", "PcOffset.bin"});
    ParsePcSamplingRecords(pcSamplingBinPath);
    vector<MemRecord> memoryRecords;
    ParseMemoryChartData(config.outputPath, metrics, memoryRecords, config.rawKernelName);
    string recordBinPath = JoinPath({config.outputPath, "dump", "OperandRecord.bin"});
    ParseOperandRecords(recordBinPath);
    if (IsExist(recordBinPath)) {
        RemoveAll(recordBinPath);
    }
    bool pmuRes = HandlePmuData(config.outputPath, profBinMap, metrics);
    if (!metrics.HasEnabledPmu()) {
        return res && SaveOpBasicInfo(config.outputPath);
    }
    if (!pmuRes) {
        LogWarn("Failed to analyze metric data.");
        res = false;
    }
    // 该函数必须在HandlePmuData执行初始化算子类型后运行，其生成的csv会影响后面可视化算子类型
    if (!SaveOpBasicInfo(config.outputPath)) {
        LogWarn("Failed to save OpBasicInfo.csv");
        res = false;
    }
    AddIndexToCsv(metrics, config);
    SetOpVisualizeData();
    std::map<uint64_t, std::map<std::string, uint64_t>> dbiMap;
    Statics(memoryRecords, dbiMap);
    if (pmuRes) {
        if (!CalMetrics(config, metrics, dbiMap)) {
            res = false;
        }
    }
    return res;
}

bool DataHandler::LoadOpBasicInfoTxtFile(const std::string &filePath)
{
    string infoFilePath = Utility::JoinPath({filePath, "dump", "op_basic_info.txt"});
    if (!Utility::IsReadable(infoFilePath)) {
        LogWarn("Access [%s] failed. Maybe do not contain this file or it is not readable. Please check.",
            infoFilePath.c_str());
        return false;
    }
    if (!ParseBasicInfoFile(infoFilePath)) {
        return false;
    }
    unsigned int currentDeviceId = 0;
    if (!StringToNum<unsigned int>(opInfoMap_["Device Id"], currentDeviceId)) {
        LogWarn("Device Id value invalid. Please check op_basic_info.txt");
        return false;
    }
    HalHelper::Instance().SetCurrentDeviceId(currentDeviceId);
    return true;
}

bool DataHandler::ParseBasicInfoFile(const string &filePath)
{
    ifstream file(filePath);
    if (!file.is_open()) {
        LogError("Failed to open file %s", filePath.c_str());
        return false;
    }
    string line;
    while (getline(file, line)) {
        vector<string> lineOpInfoList;
        size_t keyValuePairSize = 2;
        SplitString(line, '=', lineOpInfoList);
        if (lineOpInfoList.size() != keyValuePairSize) {
            continue;
        }
        if (lineOpInfoList[0] == "Is MC2" && lineOpInfoList[1] == "1") {
            isMC2_ = true;
            continue;
        }
        if (lineOpInfoList[0] == "Is Lccl" && lineOpInfoList[1] == "1") {
            isLccl_ = true;
            continue;
        }
        if (lineOpInfoList[0] == "Has Simt" && lineOpInfoList[1] == "1") {
            hasSimt_ = true;
            continue;
        }
        opInfoMap_[lineOpInfoList[0]] = lineOpInfoList[1];
        if (lineOpInfoList[0] == "Run Soc Version") {
            socVersion_ = lineOpInfoList[1];
            continue;
        }
        if (lineOpInfoList[0] == "Block Dim") {
            if (!StringToNum<uint64_t>(lineOpInfoList[1], blockDim_)) {
                LogWarn("Block dim value invalid. Please check op_basic_info.txt");
                continue;
            }
        }
        if (lineOpInfoList[0] == "Current Freq") {
            // Get cur freq from op_basic_info.txt.
            int64_t curFreq = -1;
            if (!StringToNum(lineOpInfoList[1], curFreq)) {
                LogWarn("Current Freq value invalid. Please check op_basic_info.txt");
            }
            curFreq_ = curFreq;
        }
        if (lineOpInfoList[0] == "Rated Freq") {
            // Get rated freq from op_basic_info.txt.
            int64_t ratedFreq;
            if (!StringToNum(lineOpInfoList[1], ratedFreq)) {
                LogWarn("Rated Freq value invalid. Please check op_basic_info.txt");
            }
        }
    }
    file.close();
    return true;
}

void DataHandler::HandleDuration(const std::string &outputPath)
{
    string timeFilePath = Utility::JoinPath({outputPath, "dump", "duration.bin"});
    if (!Utility::IsReadable(timeFilePath)) {
        LogWarn("Access [%s] failed. Maybe do not contain this file or it is not readable. Please check.",
                timeFilePath.c_str());
        return;
    }
    size_t fileSize = GetFileSize(timeFilePath);
    vector<char> totalBin(fileSize);
    if (!ReadBinaryFile(timeFilePath, totalBin)) {
        return;
    }

    uint64_t startTime = 0;
    uint64_t endTime = 0;
    ParseDurationBin(outputPath, totalBin, fileSize, startTime, endTime);
    if (startTime == 0 || endTime == 0 || taskSchedulerFreq_ == 0 || endTime < startTime) {
        LogWarn("Get op basic info [Task Duration] failed.");
    } else {
        duration_ = static_cast<float>((endTime - startTime) * TIME_CONVERSION) /
            static_cast<float>(taskSchedulerFreq_);
        SetDuration(duration_);
        opInfoMap_["Task Duration(us)"] = to_string(duration_);
    }
}

bool DataHandler::SaveOpBasicInfo(const std::string &outputPath)
{
    opType_ = GetOpType();
    opInfoMap_["Op Type"] = opType_;
    if (opInfoMap_["Op Type"] == Common::OpType::MIX) {
        // 此处的MixBlockDim指的是算子中vector的数量
        uint32_t vecBlockDim = 0;
        for (const auto &core : totalPmuData_) {
            if (core.second.blockType == OpType::VECTOR) {
                vecBlockDim++;
            }
        }
        SetMixBlockDim(to_string(vecBlockDim));
        opInfoMap_["Mix Block Dim"] = to_string(vecBlockDim);
    }
    HandleDuration(outputPath);
    std::string suffix = "_" + profilingFileTimeStamp_ +".csv";
    string fileName = Utility::JoinPath({outputPath, "OpBasicInfo" + suffix});
    CsvData csvData = {Utility::CsvParser::AddRow(opBasicHeader, opInfoMap_)};

    SetOpName(opInfoMap_["Op Name"]);
    SetSoc(opInfoMap_["Run Soc Version"]);
    SetDeviceId(opInfoMap_["Device Id"]);
    SetPid(opInfoMap_["Pid"]);
    SetBlockDim(opInfoMap_["Block Dim"]);
    SetCurFreq(opInfoMap_["Current Freq"]);
    SetRatedFreq(opInfoMap_["Rated Freq"]);

    DataSave dataSave;
    return dataSave.CsvSave(fileName, opBasicHeader, csvData);
}

bool DataHandler::CalMetrics(const ParserConfig &config, const Common::ProfMetricsAbilityConfig &metrics,
    std::map<uint64_t, std::map<std::string, uint64_t>> &dbiMap)
{
    vector<map<string, string>> totalCalValues;
    bool useA5Formula = SOC_STRING_TO_CHIP_PRODUCT.find(socVersion_) != SOC_STRING_TO_CHIP_PRODUCT.end() &&
                        IsChipSeriesTypeValid(SOC_STRING_TO_CHIP_PRODUCT.at(socVersion_),
                        ChipProductType::ASCEND950_SERIES);

    for (const auto &pair: totalPmuData_) {
        int64_t calFreq = (curFreq_ != -1) ? curFreq_ : aicoreFreq_;
        float calDurationA5 = calFreq <= 0 ? 0 : static_cast<float>(pair.second.totalCycles) / calFreq;
        uint64_t frequency = calFreq <= 0 ? 0 : static_cast<uint64_t>(calFreq);
        float calDuration = 0.0f;
        if (opType_ == OpType::MIX && !config.kernelScale) {
            calDuration = duration_;
            calDurationA5 = duration_;
        }
        Calculate cal(pair.second.pmuEventValueMap, pair.second.totalCycles, {chipType_, calFreq,
            aiCoreNum_, blockDim_, socVersion_}, calDuration);
        map<string, string> metricValues;
        PmuMap pmuMap(cal.pmuEventValueMap_);
        CalculateParams params;
        params.totalCycles = pair.second.totalCycles;
        params.frequency = frequency;
        params.duration = calDurationA5;
        params.socVersion = socVersion_;
        params.pmuMap = pmuMap;

        if (useA5Formula) {
            if (pair.second.blockType == OpType::CUBE) {
                metricValues = CalMetricItems(params, config.aicCalMetricItems, formulaOfA5_);
            } else {
                PrepareDbiParams(params, pair.first.first, pair.first.second);
                metricValues = CalMetricItems(params, config.aivCalMetricItems, formulaOfA5_);
            }
        } else {
            if (pair.second.blockType == OpType::CUBE) {
                metricValues = CalMetricItems(cal, config.aicCalMetricItems, formula, dbiMap[pair.first.first]);
            } else {
                metricValues = CalMetricItems(cal, config.aivCalMetricItems, formula, dbiMap[pair.first.first]);
            }
        }
        metricValues["block_id"] = to_string(pair.first.first);
        metricValues["sub_block_id"] = pair.first.second;
        totalCalValues.emplace_back(metricValues);
    }
    return SaveMetrics(config.outputPath, metrics, totalCalValues);
}

bool DataHandler::SaveMetrics(const std::string &outputPath, const Common::ProfMetricsAbilityConfig &metrics,
    std::vector<std::map<std::string, std::string>> &totalCalValues)
{
    DataSave dataSave;
    bool res = true;
    for (const ProfMetrics &selectMetric: metrics.Enabled()) {
        string metricName = Mapping::onBoardMetricsToMsprof[selectMetric];
        if (metricHeader.count(metricName) == 0 || metricName.empty()) {
            LogDebug("%s does not have to write csv.", metricName.c_str());
            continue;
        }
        const vector<string> &headers = metricHeader.at(metricName);
        std::string csvName;
        for (const auto &csvMap : METRICS_CSV_MAP) {
            if (std::strcmp(csvMap.first, metricName.c_str()) == 0) {
                csvName = csvMap.second;
                break;
            }
        }
        if (csvName.empty()) {
            LogDebug("%s does not need to write csv.", metricName.c_str());
            continue;
        }
        CsvData csvData = Utility::CsvParser::GenerateCsvData(headers, totalCalValues);
        std::string suffix = "_" + profilingFileTimeStamp_ +".csv";
        string metricFileName = Utility::JoinPath({outputPath, csvName + suffix});
        if (dataSave.CsvSave(metricFileName, headers, csvData)) {
            LogInfo("Save profiling data with metric [%s] success.", csvName.c_str());
        } else {
            LogWarn("Failed to save profiling data with metric [%s].", csvName.c_str());
            res = false;
        }
    }
    return res;
}

void DataHandler::MergeTotalPmuData(SplitBlockPmuData &blockData)
{
    BlockIDPairType key(blockData.blockId, blockData.subBlockId);
    totalPmuData_[key].blockType = blockData.blockType;
    // 使用最后1次重放的totalCycles计算aic_time/aiv_time
    totalPmuData_[key].totalCycles = blockData.totalCycles;
    totalPmuData_[key].pmuEventValueMap.insert(blockData.pmuEventValueMap.begin(), blockData.pmuEventValueMap.end());
    totalPmuData_[key].systemTime = {blockData.startSystemTime, blockData.endSystemTime};
}

bool DataHandler::HandlePmuData(const std::string &outputPath, const std::map<std::string, ProfBinInfo, FileNameCompare> &profBinMap,
                                Common::ProfMetricsAbilityConfig metrics)
{
    // parse DeviceProfX.bin and L2Cache.bin to get all pmu values: totalPmuData_
    ReadAndParseL2CacheBin(outputPath, metrics);
    for (const auto &pair: profBinMap) {
        string targetBinFilePath = Utility::JoinPath({outputPath, "dump", pair.first});
        if (!Utility::IsReadable(targetBinFilePath)) {
            LogWarn("Access profiling data file [%s] failed, maybe do not contain this file or it is not readable. "
                    "Some indicators of [%s] failed to be collected.", targetBinFilePath.c_str(),
                    pair.second.GetAllRelatedMetrics().c_str());
            continue;
        }
        size_t fileSize = GetFileSize(targetBinFilePath);
        if (fileSize == 0) {
            LogWarn("Profiling data file [%s] is empty. Some indicators of [%s] failed to be collected.",
                    targetBinFilePath.c_str(), pair.second.GetAllRelatedMetrics().c_str());
            continue;
        }
        vector<char> totalBin(fileSize);
        if (!ReadBinaryFile(targetBinFilePath, totalBin)) {
            LogWarn("Some indicators of [%s] failed to be read.", pair.second.GetAllRelatedMetrics().c_str());
            continue;
        }
        ParseProfBin(totalBin, fileSize, pair.second);
    }
    return (!totalPmuData_.empty());
}

void DataHandler::SetOpVisualizeData()
{
    auto opType = GetOpType();  // 对于910B表明是vec\cube\mix算子
    for (const auto &pair: totalPmuData_) {
        uint16_t blockId = pair.first.first;
        std::string subBlockId = pair.first.second;
        const BlockProfBinData &blockProfBinData = pair.second;
        uint64_t totalCycles = pair.second.totalCycles;
        float timeFactor = 1.0f; // 仅当310P时支持时间系数
        if ((blockDim_ != 0) && (aiCoreNum_ != 0) && (opType == Common::OpType::AI_CORE)) {
            auto blockDim = static_cast<float>(blockDim_);
            auto aiCoreNum = static_cast<float>(aiCoreNum_);
            timeFactor = static_cast<float>(std::floor((blockDim + aiCoreNum - 1) / aiCoreNum)) / blockDim;
        }
        // Check whether you get correct cur_freq
        int64_t calFreq = (curFreq_ != -1) ? curFreq_ : aicoreFreq_;
        float dur = (calFreq == 0) ? 0.0 : static_cast<float>((static_cast<float>(totalCycles) /
                calFreq * timeFactor));
        if (blockDetailMap_.count(blockId) == 0) {
            vector<float> durations;
            blockDetailMap_[blockId] = durations;
        }
        blockDetailMap_[blockId].emplace_back(dur);

        auto recordMap = GetOperandRecordMap(blockId, subBlockId, opType);
        AddComputeBlockDetailToVisualize(pair.first, blockProfBinData, recordMap, computeLoadBlockDetailVec_);
        AddMemMapDetailToVisualize(pair.first, blockProfBinData, memMapDetail_);
    }
}

void DataHandler::AddMemMapDetailToVisualize(const BlockIDPairType& blockIdPairType,
                                             const BlockProfBinData &blockProfBinData,
                                             std::vector<MemMapDetail> &memMapDetail)
{
    MemMapDetail memMap;
    memMap.opType = GetOpType();
    memMap.soc = GetSoc();
    memMap.blockId = blockIdPairType.first;
    memMap.blockType = blockIdPairType.second;
    memMap.freq = (curFreq_ != -1) ? curFreq_ : aicoreFreq_;
    memMap.totalCycles = blockProfBinData.totalCycles;
    memMap.aiCoreNum = aiCoreNum_;
    memMap.blockDim = blockDim_;
    memMap.eventMap = GetVisualizeEventMap(memMap.blockType, blockProfBinData.pmuEventValueMap);
    if (memMap.opType != Common::OpType::MIX) {
        memMapDetail.emplace_back(memMap);
        return;
    }
    if (memMap.blockType == "vector0" && !memMapDetail.empty()) {
        memMapDetail.back().eventMapVec0 = memMap.eventMap;
        memMapDetail.back().cycMap["Vector"] = memMap.totalCycles;
    }
    if (memMap.blockType == "vector1" && !memMapDetail.empty()) {
        memMapDetail.back().eventMapVec1 = memMap.eventMap;
        memMapDetail.back().cycMap["Vector1"] = memMap.totalCycles;
    }
    if (memMap.blockType == "cube0") {
        memMapDetail.emplace_back(memMap);
        memMapDetail.back().cycMap["Cube"] = memMap.totalCycles;
    }
}

void DataHandler::AddComputeBlockDetailToVisualize(const BlockIDPairType& blockIdPairType,
    const BlockProfBinData &blockProfBinData, const TypeOperandRecord &recordMap, vector<ComputeLoadBlockDetail> &blockDetailMap)
{
    ComputeLoadBlockDetail computeLoadBlockDetail;
    computeLoadBlockDetail.opType = GetOpType();  // 对于910B表明是vec\cube\mix算子
    computeLoadBlockDetail.blockId = blockIdPairType.first;     // 910B时为cube0\vector0\vector1
    computeLoadBlockDetail.blockType = blockIdPairType.second;
    computeLoadBlockDetail.freq = (curFreq_ != -1) ? curFreq_ : aicoreFreq_;
    computeLoadBlockDetail.totalCycles = blockProfBinData.totalCycles;
    computeLoadBlockDetail.eventMap = GetVisualizeEventMap(computeLoadBlockDetail.blockType, blockProfBinData.pmuEventValueMap);
    computeLoadBlockDetail.operandRecordMap = recordMap;
    blockDetailMap.emplace_back(computeLoadBlockDetail);
}

void DataHandlerOf910B::ParseProfBin(const std::vector<char> &binData, const size_t &fileSize,
                                     const ProfBinInfo &binInfo)
{
    blockIdCoreIdPairVec_.clear();
    for (size_t i = 0; i + FFTS_LENGTH <= fileSize; i = i + FFTS_LENGTH) {
        vector<char> splitBinData{&binData[i], &binData[i] + FFTS_LENGTH};
        FftsBlockBean fftsBlockBean(ChipProductType::ASCEND910B_SERIES, splitBinData);
        SplitBlockPmuData blockData = fftsBlockBean.GetBlockData(binInfo.aicEvents, binInfo.aivEvents);
        if (blockData.blockType == Common::OpType::CUBE) {
            isAic_ = true;
        }
        if (blockData.blockType == Common::OpType::VECTOR) {
            isAiv_ = true;
        }
        MergeTotalPmuData(blockData);
        // 记录最后一次重放的blockId和CoreId
        blockIdCoreIdPairVec_.push_back({blockData.blockId, blockData.coreId});
    }
}

void DataHandlerOf910B::ReadAndParseL2CacheBin(const std::string &outputPath, const Common::ProfMetricsAbilityConfig &metrics)
{
    string targetBinFilePath = Utility::JoinPath({outputPath, "dump", "L2Cache.bin"});
    if (!metrics.HasEnabledPmu()) {
        return;
    }
    if (metrics.replayMode == ReplayMode::RANGE) {
        LogInfo("GM to L2Cache data not calculate L2Cache evict in Range-Replay Mode.");
        return;
    }
    SplitBlockPmuData blockData;
    if (ParseL2CacheBin(targetBinFilePath, L2_CACHE_EVENTS_FOR_A2_A3, blockData)) {
        l2CacheEvict_ = static_cast<int64_t>(blockData.pmuEventValueMap[0x9c]);
    } else {
        LogWarn("Failed to parse L2Cache evict value");
    }
}

void DataHandlerOf910B::AddIndexToCsv(const Common::ProfMetricsAbilityConfig &metrics, ParserConfig &config)
{
    std::set<std::string> newAicMetrics {};
    std::set<std::string> newAivMetrics {};
    auto &pipeCsv = metricHeader[std::string(Common::MsprofMetrics::PIPE_UTILIZATION)];
    for (const auto &blockWithData : GetTotalPmuData()) {
        for (const auto &pmuMap : blockWithData.second.pmuEventValueMap) {
            auto it = scalarPmuToIndex.find(pmuMap.first);
            if (it == scalarPmuToIndex.end() || pmuMap.second == 0) {
                continue;
            }
            std::string metricValue = scalarPmuToIndex.at(pmuMap.first);
            if (blockWithData.second.blockType == OpType::CUBE) {
                metricValue = "aic_" + metricValue;
                config.aicCalMetricItems.emplace(metricValue);
                newAicMetrics.emplace(metricValue);
            } else {
                metricValue = "aiv_" + metricValue;
                config.aivCalMetricItems.emplace(metricValue);
                newAivMetrics.emplace(metricValue);
            }
        }
    }
    pipeCsv.insert(pipeCsv.end(), newAicMetrics.begin(), newAicMetrics.end());
    pipeCsv.insert(pipeCsv.end(), newAivMetrics.begin(), newAivMetrics.end());
    if (metrics.isMemoryDetail) {
        config.aicCalMetricItems.insert(pipeDbiFor910B_.begin(), pipeDbiFor910B_.end());
        pipeCsv.insert(pipeCsv.end(), pipeDbiFor910B_.begin(), pipeDbiFor910B_.end());
    }
}

void DataHandlerOf910B::ParseDurationBin(const std::string &outputPath, const std::vector<char> &binData,
                                         const size_t &fileSize, uint64_t &startTime, uint64_t &endTime)
{
    uint64_t aicpuStartTime = UINT64_MAX;
    for (size_t i = 0; i + ACSQ_LENGTH <= fileSize; i = i + ACSQ_LENGTH) { // 这里的start和end存的是aicore的起止时间
        vector<char> splitBinData{&binData[i], &binData[i] + ACSQ_LENGTH};
        AcsqBean acsqBean(ChipProductType::ASCEND910B_SERIES, splitBinData);
        uint16_t taskType = acsqBean.GetTaskType();
        Common::TimeType timeType = acsqBean.GetTimeType();
        uint16_t streamId = acsqBean.GetStreamId();
        uint16_t taskId = acsqBean.GetTaskId();
        uint64_t systemTime = acsqBean.GetSystemTime();
        // AI_CORE type = 0, save last group time
        if (taskType == 0) {
            if (timeType == Common::TimeType::START) {
                startTime = systemTime;
            } else if (timeType == Common::TimeType::END) {
                endTime = systemTime;
            }
        }
        if (!isMC2_) {
            continue;
        }
        // for MC2 operator timeline
        auto iter = acsqTimeMap_.find({taskId, streamId});
        if (timeType == Common::TimeType::START) {
            if (iter == acsqTimeMap_.end()) {
                acsqTimeMap_[{taskId, streamId}] = {taskType, systemTime, 0};
            } else {
                iter->second.taskType = taskType;
                iter->second.startTime = systemTime;
            }
        } else if (timeType == Common::TimeType::END) {
            if (iter == acsqTimeMap_.end()) {
                acsqTimeMap_[{taskId, streamId}] = {0, 0, systemTime};
            } else {
                iter->second.endTime = systemTime;
            }
        }
        // AI_CPU type = 1, save last group time
        if (taskType == 1 && timeType == Common::TimeType::START) {
            aicpuStartTime = systemTime;
        }
    }
    minTimeCyc_ = startTime;
    minMc2TimeCyc_ = min(aicpuStartTime, startTime);
    minLcclTimeCyc_ = startTime;
}

string DataHandlerOf910B::GetOpType()
{
    if (isAic_) {
        if (isAiv_) {
            return Common::OpType::MIX;
        }
        return Common::OpType::CUBE;
    }
    return Common::OpType::VECTOR;
}

vector<MemRecord> DataHandler::ParseMemoryChartBin(const string &memoryChartBin) const
{
    if (!IsExist(memoryChartBin)) {
        LogDebug("%s is not exist, please check dbi args.", memoryChartBin.c_str());
        return {};
    }
    size_t fileSize = GetFileSize(memoryChartBin);
    if (fileSize == 0 || fileSize % sizeof(MemRecord) != 0) {
        LogWarn("File size of %s is invalid: %zu.", memoryChartBin.c_str(), fileSize);
        return {};
    }
    vector<MemRecord> memRecord;
    memRecord.resize(fileSize / sizeof(MemRecord));
    if (!ReadFile(memoryChartBin, reinterpret_cast<uint8_t*>(memRecord.data()), fileSize)) {
        LogWarn("Read file %s failed.", memoryChartBin.c_str());
        return {};
    }
    return memRecord;
}

void DataHandlerOf910B::Statics(std::vector<MemRecord> &memoryRecords,
    std::map<uint64_t, std::map<std::string, uint64_t>> &dataSize)
{
    auto getReq = [](uint64_t x, uint64_t align) -> uint64_t {
        if (align == 0) {
            return 0;
        }
        return (x + align - 1) / align;
    };
    // blockId->{GM_TO_L0A, GM_TO_L0B,GM_TO_L1}
    unordered_map<uint16_t, std::map<std::string, uint64_t>> gmRequests;
    unordered_map<uint16_t, std::map<std::string, uint64_t>> gmDates;
    for (const auto &record : memoryRecords) {
        if (record.src == MemType::GM) {
            if (record.dst == MemType::L0A) {
                gmRequests[record.blockId][GM_TO_L0A] += getReq(record.srcMemSize,
                                                                REQ_DATA_OF_910B.at(TransportType::GM_TO_L0A));
                gmDates[record.blockId][GM_TO_L0A_DATA] += record.srcMemSize;
            } else if (record.dst == MemType::L0B) {
                gmRequests[record.blockId][GM_TO_L0B] += getReq(record.srcMemSize,
                                                                REQ_DATA_OF_910B.at(TransportType::GM_TO_L0B));
                gmDates[record.blockId][GM_TO_L0B_DATA] += record.srcMemSize;
            } else if (record.dst == MemType::L1) {
                gmRequests[record.blockId][GM_TO_L1] += getReq(record.srcMemSize,
                                                               REQ_DATA_OF_910B.at(TransportType::GM_TO_L1));
                gmDates[record.blockId][GM_TO_L1_DATA] += record.srcMemSize;
            }
        }
    }
    for (auto &memDetail : memMapDetail_) {
        memDetail.ApiDataTransVolume_[GM_TO_L0A] = gmRequests[memDetail.blockId][GM_TO_L0A];
        memDetail.ApiDataTransVolume_[GM_TO_L0B] = gmRequests[memDetail.blockId][GM_TO_L0B];
        memDetail.ApiDataTransVolume_[GM_TO_L1] = gmRequests[memDetail.blockId][GM_TO_L1];
        memDetail.ApiDataTransVolume_[GM_TO_L0A_DATA] = gmDates[memDetail.blockId][GM_TO_L0A_DATA];
        memDetail.ApiDataTransVolume_[GM_TO_L0B_DATA] = gmDates[memDetail.blockId][GM_TO_L0B_DATA];
        memDetail.ApiDataTransVolume_[GM_TO_L1_DATA] = gmDates[memDetail.blockId][GM_TO_L1_DATA];
        dataSize[memDetail.blockId][GM_TO_L0A_DATA] = gmDates[memDetail.blockId][GM_TO_L0A_DATA];
        dataSize[memDetail.blockId][GM_TO_L0B_DATA] = gmDates[memDetail.blockId][GM_TO_L0B_DATA];
        dataSize[memDetail.blockId][GM_TO_L1_DATA] = gmDates[memDetail.blockId][GM_TO_L1_DATA];
    }
}

void DataHandlerOf910B::ParseMemoryChartData(const std::string &outputPath,
    const Common::ProfMetricsAbilityConfig &metrics, vector<MemRecord> &memoryRecords, const std::string &kernelName)
{
    if (isMC2_ || isLccl_) { // mc2/lccl不支持动态插桩
        return;
    }
    string memoryChartBin = JoinPath({outputPath, "dump", MEMORY_CHART_BIN});
    memoryRecords = ParseMemoryChartBin(memoryChartBin);
    if (IsExist(memoryChartBin)) {
        RemoveAll(memoryChartBin);
    }
    l2Cache_ = GetDefaultL2Cache(GetSoc());
    if (l2Cache_ == nullptr) {
        LogWarn("L2cache ptr get failed");
        return;
    }
    if (metrics.isMemoryDetail) {
        l2Cache_->Modeling(memoryRecords);
    }
    if (metrics.isSource) {
        HotSpotFunctionGenerator hotSpotFunctionGenerator({GetSoc(), kernelName, 0, true, false,
                                                           metrics.isMemoryDetail});
        if (!hotSpotFunctionGenerator.Process(outputPath, memoryRecords, l2Cache_)) {
            LogWarn("Generate hot spot function failed");
        }
    }
}

void DataHandlerOf310P::ParseProfBin(const std::vector<char> &binData, const size_t &fileSize,
                                     const ProfBinInfo &binInfo)
{
    for (size_t i = 0; i + AICORE_LENGTH <= fileSize; i = i + AICORE_LENGTH) {
        vector<char> splitBinData{&binData[i], &binData[i] + AICORE_LENGTH};
        AiCoreBean aiCoreBean(splitBinData);
        SplitBlockPmuData blockData = aiCoreBean.GetAiCoreData(binInfo.aicEvents);
        MergeTotalPmuData(blockData);
    }
}

void DataHandlerOf310P::ParseDurationBin(const std::string &outputPath, const std::vector<char> &binData,
                                         const size_t &fileSize, uint64_t &startTime, uint64_t &endTime)
{
    for (size_t i = 0; i + HWTS_LENGTH <= fileSize; i = i + HWTS_LENGTH) {
        vector<char> splitBinData{&binData[i], &binData[i] + HWTS_LENGTH};
        HwtsBean hwtsBean(splitBinData);
        Common::TimeType timeType = hwtsBean.GetTimeType();
        if (timeType == Common::TimeType::START) {
            startTime = hwtsBean.GetSystemTime();
        } else if (timeType == Common::TimeType::END) {
            endTime = hwtsBean.GetSystemTime();
        }
    }
}

void DataHandlerOf310P::ReadAndParseL2CacheBin(const string &outputPath, const Common::ProfMetricsAbilityConfig &metrics)
{
    if (!metrics.Getvalue(ProfMetrics::L2_CACHE).isOn) {
        return;
    }
    string targetBinFilePath = Utility::JoinPath({outputPath, "dump", "L2Cache.bin"});
    SplitBlockPmuData blockData;
    if (ParseL2CacheBin(targetBinFilePath, L2_CACHE_EVENTS_FOR_310P, blockData)) {
        MergeTotalPmuData(blockData);
    } else {
        LogWarn("Failed to parse L2cache value");
    }
}

string DataHandlerOf310P::GetOpType()
{
    return Common::OpType::AI_CORE;
}

void DataHandlerOf91095::ParseMemoryChartData(const std::string &outputPath,
    const Common::ProfMetricsAbilityConfig &metrics, std::vector<Common::MemRecord> &memoryRecords,
    const std::string &kernelName)
{
    if (isMC2_ || isLccl_) { // mc2/lccl不支持动态插桩
        return;
    }
    string memoryChartBin = JoinPath({outputPath, "dump", MEMORY_CHART_BIN});
    memoryRecords = ParseMemoryChartBin(memoryChartBin);
    if (IsExist(memoryChartBin)) {
        RemoveAll(memoryChartBin);
    }

    const bool pcSamplingEnable = metrics.pcSamplingEnable && GetHasSimt();
    if (pcSamplingEnable) {
        LogInfo("Enable stall reason analysis for SIMT operator.");
    } else {
        LogDebug("Skip stall reason analysis because current operator is not SIMT.");
    }

    if (metrics.isSource || pcSamplingEnable) {
        HotSpotFunctionGenerator hotSpotFunctionGenerator({GetSoc(), "", pcOffsetByPcSampling_, metrics.isSource,
                                                           pcSamplingEnable,
                                                           metrics.isMemoryDetail});
        if (!hotSpotFunctionGenerator.Process(outputPath, memoryRecords, l2Cache_)) {
            LogWarn("Generate hot spot function failed");
            return;
        }
    }
}

void DataHandler::ParsePcSamplingRecords(const std::string &recordBinPath)
{
    constexpr size_t recordSize = 8;
    if (!IsReadable(Realpath(recordBinPath))) {
        LogDebug("%s is not exist or readable.", recordBinPath.c_str());
        return;
    }
    size_t fileSize = GetFileSize(recordBinPath);
    if (fileSize != recordSize) {
        LogWarn("File size of %s is invalid %zu.", recordBinPath.c_str(), fileSize);
    }
    vector<char> binData(fileSize);
    if (!ReadBinaryFile(recordBinPath, binData)) {
        return;
    }
    if (memcpy_s(&pcOffsetByPcSampling_, recordSize, binData.data(), recordSize) != EOK) {
        LogWarn("Pc sampling record memcpy failed.");
    }
    return;
}

void DataHandler::ParseOperandRecords(const std::string &recordBinPath)
{
    if (!IsReadable(Realpath(recordBinPath))) {
        LogDebug("%s is not exist or readable.", recordBinPath.c_str());
        return;
    }
    size_t fileSize = GetFileSize(recordBinPath);
    size_t headerSize = sizeof(OperandHeader);
    size_t blockSize = sizeof(BlockOperandRecords);
    if (fileSize < headerSize || (fileSize - headerSize) % blockSize != 0) {
        LogWarn("File size of %s is invalid: %zu.", recordBinPath.c_str(), fileSize);
        return;
    }
    LogDebug("OperandRecord.bin size is %zu.", fileSize);
    vector<char> binData(fileSize);
    if (!ReadBinaryFile(recordBinPath, binData)) {
        return;
    }

    auto statistical = [](const ThreadOperandRecords &origin, OperandRecordMap &res) {
        for (size_t typeIndex = 0; typeIndex < static_cast<size_t>(OperandType::END); typeIndex++) {
            auto type = static_cast<OperandType>(typeIndex);
            res[type].instructions = SafeAdd(res[type].instructions, origin.records[typeIndex].instructions, "instructions");
            res[type].operands = SafeAdd(res[type].operands, origin.records[typeIndex].operands, "operands");
        }
    };
    for (size_t i = headerSize; i < fileSize; i = i + blockSize) {
        auto blockRecord = MakeUnique<BlockOperandRecords>();
        if (!blockRecord) {
            continue;
        }
        if (memcpy_s(blockRecord.get(), blockSize, &binData[i], blockSize) != EOK) {
            continue;
        }
        TypeOperandRecord blockData;
        statistical(blockRecord->simdRecord, blockData.simdMap);
        for (const auto &threadRecord : blockRecord->simtRecord) {
            statistical(threadRecord, blockData.simtMap);
        }
        operandRecords_.push_back(blockData);
    }
}

TypeOperandRecord DataHandler::GetOperandRecordMap(uint16_t blockId, const string &subBlockId, const string &opType)
{
    TypeOperandRecord recordMap;
    // 一个AICore的数据分为3个block。
    // mix算子：    vector0，vector1，cube， vector0，vector1，cube
    // cube算子：   空，      空，     cube， 空，     空，     cube
    // vector算子： vector，  vector， 空，   vector， vector， 空
    size_t blockIndex = 0;
    constexpr uint8_t subBlockNum = 3;
    constexpr uint8_t vecBlockNum = 2;
    constexpr uint8_t vector0Index = 0;
    constexpr uint8_t vector1Index = 1;
    constexpr uint8_t cubeIndex = 2;
    if (subBlockId == "cube0") {
        blockIndex = blockId * subBlockNum + cubeIndex;
    }
    if (opType == Common::OpType::MIX) {
        if (subBlockId == "vector0") {
            blockIndex = blockId * subBlockNum + vector0Index;
        } else if (subBlockId == "vector1") {
            blockIndex = blockId * subBlockNum + vector1Index;
        }
    } else if (opType == Common::OpType::VECTOR) {
        blockIndex = (blockId / vecBlockNum) * subBlockNum + (blockId % vecBlockNum);
    }
    if (blockIndex < operandRecords_.size()) {
        recordMap = operandRecords_[blockIndex];
    }
    return recordMap;
}

void DataHandlerOf91095::Statics(std::vector<MemRecord> &memoryRecords,
    std::map<uint64_t, std::map<std::string, uint64_t>> &dataSize)
{
    std::map<uint16_t, std::map<std::string, uint64_t>> subblockStats;
    std::map<uint16_t, std::map<std::string, uint64_t>> aicoreStats;

    for (const auto &record : memoryRecords) {
        DBIHelper::Instance().PrintRecord(record);
        CollectStats(record, subblockStats, aicoreStats);
    }

    for (auto &memDetail : memMapDetail_) {
        uint16_t aicoreId = memDetail.blockId;
        if (memDetail.opType == Common::OpType::MIX) {
            memDetail.apiDataTransVolumeVec0.insert(subblockStats[aicoreId * SUBLOCK_NUM + VEC0_SUBBLOCK_INDEX].begin(), subblockStats[aicoreId * SUBLOCK_NUM + VEC0_SUBBLOCK_INDEX].end());
            memDetail.apiDataTransVolumeVec1.insert(subblockStats[aicoreId * SUBLOCK_NUM + VEC1_SUBBLOCK_INDEX].begin(), subblockStats[aicoreId * SUBLOCK_NUM + VEC1_SUBBLOCK_INDEX].end());
            memDetail.ApiDataTransVolume_.insert(subblockStats[aicoreId * SUBLOCK_NUM + CUBE_SUBBLOCK_INDEX].begin(), subblockStats[aicoreId * SUBLOCK_NUM + CUBE_SUBBLOCK_INDEX].end());
        } else {
            memDetail.ApiDataTransVolume_.insert(aicoreStats[aicoreId].begin(), aicoreStats[aicoreId].end());
        }
        if (!aicoreStats[aicoreId].empty()) {
            dataSize[aicoreId] = aicoreStats[aicoreId];
        }
    }
}

void DataHandlerOf91095::PrepareDbiParams(CalculateParams &params, uint64_t blockId, const std::string &subBlockId)
{
    const auto &memDetail = memMapDetail_[blockId];

    auto findUbToGmData = [&](const std::map<std::string, uint64_t> &dataMap) {
        auto it = dataMap.find("UB_TO_GM_DATA");
        if (it != dataMap.end()) {
            params.hasDbiUBToGMData = true;
            params.dbiUBToGMData = it->second;
        }
    };

    if (GetOpType() == Common::OpType::MIX) {
        if (subBlockId == "vector0") {
            findUbToGmData(memDetail.apiDataTransVolumeVec0);
        } else if (subBlockId == "vector1") {
            findUbToGmData(memDetail.apiDataTransVolumeVec1);
        }
    } else {
        findUbToGmData(memDetail.ApiDataTransVolume_);
    }
}

uint64_t DataHandlerOf91095::CalcRequest(uint64_t x, uint64_t align)
{
    return (align == 0) ? 0 : (x + align - 1) / align;
}

void DataHandlerOf91095::CollectStats(const Common::MemRecord &record,
    std::map<uint16_t, std::map<std::string, uint64_t>> &subblockStats,
    std::map<uint16_t, std::map<std::string, uint64_t>> &aicoreStats)
{
    uint16_t subblockId = record.blockId;
    uint16_t aicoreId = subblockId / SUBLOCK_NUM;

    auto addStats = [&](const std::string &reqKey, const std::string &dataKey, uint64_t req, uint64_t data) {
        subblockStats[subblockId][reqKey] += req;
        subblockStats[subblockId][dataKey] += data;
        aicoreStats[aicoreId][reqKey] += req;
        aicoreStats[aicoreId][dataKey] += data;
    };

    auto addUbIdStats = [&](const std::string &reqKey, const std::string &dataKey, uint64_t req, uint64_t data, uint16_t targetSubblockId) {
        subblockStats[targetSubblockId][reqKey] += req;
        subblockStats[targetSubblockId][dataKey] += data;
    };

    uint64_t data = record.srcMemSize;
    const auto &rules = GetMemTransferRules();
    auto it = rules.find(std::make_pair(record.src, record.dst));

    if (it != rules.end()) {
        for (const auto &rule : it->second) {
            uint64_t req = CalcRequest(data, REQ_DATA_OF_A5.at(rule.transportType));
            addStats(rule.reqKey, rule.dataKey, req, data);

            if (rule.needExtraUbIdMapping && subblockId % SUBLOCK_NUM == CUBE_SUBBLOCK_INDEX) {
                uint16_t ubId = subblockId - CUBE_SUBBLOCK_INDEX + record.subBlockID;
                addUbIdStats(rule.reqKey, rule.dataKey, req, data, ubId);
            }
        }
    }
}

void DataHandlerOf91095::ParseProfBin(const std::vector<char> &binData, const size_t &fileSize,
    const ProfBinInfo &binInfo)
{
    blockIdCoreIdPairVec_.clear();
    for (size_t i = 0; i + FFTS_LENGTH <= fileSize; i = i + FFTS_LENGTH) {
        vector<char> splitBinData{&binData[i], &binData[i] + FFTS_LENGTH};
        FftsBlockBean fftsBlockBean(ChipProductType::ASCEND950_SERIES, splitBinData);
        if (fftsBlockBean.GetFuncType() != FUNC_TYPE_BLOCK_PMU) {
            continue;
        }
        SplitBlockPmuData blockData = fftsBlockBean.GetBlockData(binInfo.aicEvents, binInfo.aivEvents);
        if (blockData.blockType == Common::OpType::CUBE) {
            isAic_ = true;
        }
        if (blockData.blockType == Common::OpType::VECTOR) {
            isAiv_ = true;
        }
        MergeTotalPmuData(blockData);
        // 记录最后一次重放的blockId和CoreId
        blockIdCoreIdPairVec_.push_back({blockData.blockId, blockData.coreId});
    }
}

void DataHandlerOf91095::ParseDurationBin(const std::string &outputPath, const std::vector<char> &binData,
    const size_t &fileSize, uint64_t &startTime, uint64_t &endTime)
{
    for (size_t i = 0; i + ACSQ_LENGTH_A5 <= fileSize;
         i = i + ACSQ_LENGTH_A5) { // 这里的start和end存的是aicore的起止时间
        vector<char> splitBinData{&binData[i], &binData[i] + ACSQ_LENGTH_A5};
        AcsqBean acsqBean(ChipProductType::ASCEND950_SERIES, splitBinData);
        SqeType taskType = static_cast<SqeType>(acsqBean.GetTaskType());
        Common::TimeType timeType = acsqBean.GetTimeType();
        uint64_t systemTime = acsqBean.GetSystemTime();
        if (taskType == SqeType::AIC_TYPE || taskType == SqeType::AIV_TYPE || taskType == SqeType::MIX_TYPE) {
            if (timeType == Common::TimeType::START) {
                startTime = systemTime;
            } else if (timeType == Common::TimeType::END) {
                endTime = systemTime;
            }
        }
    }
}

string DataHandlerOf91095::GetOpType()
{
    if (isAic_) {
        if (isAiv_) {
            return Common::OpType::MIX;
        }
        return Common::OpType::CUBE;
    }
    return Common::OpType::VECTOR;
}

map<uint64_t, uint64_t> DataHandler::GetVisualizeEventMap(const string &blockType, const PmuEventValueMapType &originEventMap)
{
    auto totalEvents = GetEventsByType(chipType_, blockType);
    map<uint64_t, uint64_t> eventMap;
    for (const auto &i: totalEvents) {
        eventMap[static_cast<uint64_t>(i)] = GetPmuValue(originEventMap, i);
    }
    return eventMap;
}
}
