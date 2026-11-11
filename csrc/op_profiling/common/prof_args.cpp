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


#include "prof_args.h"
#include <cmath>
#include <cctype>
#include <regex>
#include "include/opprof/Protocol.h"
#include "json_parser.h"
#include "log.h"

using namespace Utility;
namespace Common {

const std::map<std::string, ProfMetricsSet> SET_DEFINITIONS_A2A3{
    {"default", {{"Default"}, "Suitable for first-round fast analysis."}},
    {"detailed",
        {{"Default", "Occupancy", "Roofline"}, "Adds core-level throughput and compute/memory bound analysis."}},
    {"full",
        {{"Default", "Occupancy", "Roofline", "MemoryDetail", "Source"},
            "Enables full data path analysis and source hotspot."}},
};

const std::map<std::string, ProfMetricsSet> SET_DEFINITIONS_A5{
    {"default", {{"Default"}, "Suitable for first-round fast analysis."}},
    {"detailed",
        {{"Default", "Occupancy", "Roofline", "PipeTimeline"},
            "Adds core-level throughput and pipeline timeline analysis."}},
    {"full",
        {{"Default", "Occupancy", "Roofline", "MemoryDetail", "Source", "PCSampling", "InstrTimeline"},
            "Enables full data path and instruction timeline analysis."}},
};

bool ResolveSet(const std::string &setName, ProfMetricsAbilityConfig &config,
                ChipType chipType, std::string &errMsg)
{
    const std::map<std::string, ProfMetricsSet> *setDefs = &SET_DEFINITIONS_A2A3;
    if (chipType == ChipType::ASCEND950) {
        setDefs = &SET_DEFINITIONS_A5;
    }

    std::string lowerName;
    for (char c : setName) {
        lowerName += static_cast<char>(tolower(static_cast<unsigned char>(c)));
    }

    auto it = setDefs->find(lowerName);
    if (it == setDefs->end()) {
        errMsg = "--set must be one of: default, detailed, full";
        return false;
    }

    std::string metricStr;
    for (const auto &m : it->second.metrics) {
        if (!metricStr.empty()) {
            metricStr += ",";
        }
        metricStr += m;
    }
    return ParseValue(metricStr, config);
}

constexpr size_t MAX_DESC_ID_DISPLAY_COUNT = 1024;
constexpr size_t MAX_DESC_ID_DISPLAY_LEN = 64;
constexpr uint32_t MIN_TIMELINE_MERGE_RANK_COUNT = 2;
constexpr uint32_t MAX_TIMELINE_MERGE_RANK_COUNT = 64;
constexpr uint32_t MAX_TIMELINE_MERGE_LAUNCH_COUNT = 50;
constexpr char const *TIMELINE_MERGE_CONFIG = "timeline_merge_config";
constexpr char const *MC2_TRACE_CONFIG = "mc2_trace_config";
constexpr char const *ENABLE_BLOCK_TIME = "enable_block_time";
constexpr char const *DESC_ID_DISPLAY = "descid_display";
constexpr char const *MSTX_NAME = "mstx_name";
constexpr char const *RANK_COUNT = "rank_count";
constexpr char const *LAUNCH_COUNT = "launch_count";

static bool CheckTimelineMergeNames(const nlohmann::json &configJson, const std::string &prefix, std::string &msg) {
    static const std::regex kernelNamePattern("^[A-Za-z0-9_*]+$");
    static const std::regex mstxNamePattern("^[A-Za-z0-9_]+$");
    for (const std::string fieldName : {KERNEL_NAME, MSTX_NAME}) {
        const bool isKernelName = fieldName == KERNEL_NAME;
        const auto maxLength = isKernelName ? MAX_KERNEL_NAME_LENGTH : MAX_MSTX_INCLUDE_NAME_LENGTH;
        if (!configJson.contains(fieldName) || !configJson[fieldName].is_string()) {
            msg = prefix + "." + fieldName + " must be a string.";
            return false;
        }
        const auto &configName = configJson[fieldName].get_ref<const std::string &>();
        if (configName.empty() || configName.size() >= maxLength) {
            msg = prefix + "." + fieldName + " must be a string with length in [1, " + std::to_string(maxLength - 1) +
                "].";
            return false;
        }
        if (!std::regex_match(configName, isKernelName ? kernelNamePattern : mstxNamePattern)) {
            msg = prefix + "." + fieldName + " contains unsupported characters. Support characters are: " +
                (isKernelName ? "A-Za-z0-9_*" : "A-Za-z0-9_");
            return false;
        }
    }
    return true;
}

static bool ParseTimelineMergeConfigs(
    const nlohmann::json &customInputJson, std::vector<TimelineMergeConfig> &configs, std::string &msg) {
    if (!customInputJson.contains(TIMELINE_MERGE_CONFIG)) {
        return true;
    }
    const auto &input = customInputJson[TIMELINE_MERGE_CONFIG];
    if (!input.is_array()) {
        msg = "timeline_merge_config must be an array of objects.";
        return false;
    }
    if (input.size() > MAX_TIMELINE_MERGE_CONFIG_COUNT) {
        msg = "timeline_merge_config supports at most " + std::to_string(MAX_TIMELINE_MERGE_CONFIG_COUNT) +
            " selections.";
        return false;
    }
    for (size_t index = 0; index < input.size(); ++index) {
        const auto &value = input[index];
        const auto prefix = std::string(TIMELINE_MERGE_CONFIG) + "[" + std::to_string(index) + "]";
        if (!value.is_object()) {
            msg = prefix + " must be an object.";
            return false;
        }
        if (!CheckTimelineMergeNames(value, prefix, msg)) {
            return false;
        }
        for (const std::string fieldName : {RANK_COUNT, LAUNCH_COUNT}) {
            const auto minCount = fieldName == RANK_COUNT ? MIN_TIMELINE_MERGE_RANK_COUNT : 1U;
            const auto maxCount =
                fieldName == RANK_COUNT ? MAX_TIMELINE_MERGE_RANK_COUNT : MAX_TIMELINE_MERGE_LAUNCH_COUNT;
            if (!value.contains(fieldName) || !value[fieldName].is_number_integer() || value[fieldName] < minCount ||
                value[fieldName] > maxCount) {
                msg = prefix + "." + fieldName + " must be an integer within [" + std::to_string(minCount) + ", " +
                    std::to_string(maxCount) + "].";
                return false;
            }
        }
        TimelineMergeConfig config;
        config.kernelName = value[KERNEL_NAME].get<std::string>();
        config.mstxName = value[MSTX_NAME].get<std::string>();
        config.rankCount = value[RANK_COUNT].get<uint32_t>();
        config.launchCount = value[LAUNCH_COUNT].get<uint32_t>();
        configs.push_back(std::move(config));
    }
    return true;
}

static bool ParseDescIdDisplay(const nlohmann::json &descIdDisplayJson, Mc2TraceConfig &config, std::string &msg) {
    if (!descIdDisplayJson.is_object() || descIdDisplayJson.size() > MAX_DESC_ID_DISPLAY_COUNT) {
        msg = "mc2_trace_config.descid_display must be an object with at most " +
            std::to_string(MAX_DESC_ID_DISPLAY_COUNT) + " entries.";
        return false;
    }
    const static std::regex valuePattern("^[a-zA-Z0-9-_ ]+$");
    for (const auto &item : descIdDisplayJson.items()) {
        std::string descIdStr = item.key();
        auto radix = RADIX_10;
        uint64_t descIdNum = 0;
        if (StartsWith(descIdStr, "0x") || StartsWith(descIdStr, "0X")) {
            radix = RADIX_16;
            descIdStr = descIdStr.substr(2);
        }
        if (!StoullConverter(descIdStr, descIdNum, radix)) {
            msg = "mc2_trace_config.descid_display key is not a valid number.";
            return false;
        }
        if (!item.value().is_string()) {
            msg = "mc2_trace_config.descid_display value must be a string.";
            return false;
        }
        std::string valueStr = item.value().get<std::string>();
        if (valueStr.length() > MAX_DESC_ID_DISPLAY_LEN) {
            msg = "mc2_trace_config.descid_display value exceeds max length " +
                std::to_string(MAX_DESC_ID_DISPLAY_LEN) + ".";
            return false;
        }
        if (!std::regex_match(valueStr, valuePattern)) {
            msg = "mc2_trace_config.descid_display value contains invalid characters, 'a-zA-Z0-9-_' and space are "
                  "allowed.";
            return false;
        }
        config.descIdDisplay[descIdNum] = valueStr;
    }
    return true;
}

static bool ParseMc2TraceConfig(const nlohmann::json &customInputJson, Mc2TraceConfig &config, std::string &msg) {
    if (!customInputJson.contains(MC2_TRACE_CONFIG)) {
        return true;
    }
    const auto &configJson = customInputJson[MC2_TRACE_CONFIG];
    if (!configJson.is_object()) {
        msg = "mc2_trace_config must be an object.";
        return false;
    }
    if (configJson.contains(ENABLE_BLOCK_TIME)) {
        if (!configJson[ENABLE_BLOCK_TIME].is_boolean()) {
            msg = "mc2_trace_config.enable_block_time must be a boolean.";
            return false;
        }
        config.enableBlockTime = configJson[ENABLE_BLOCK_TIME].get<bool>();
    }
    if (!configJson.contains(DESC_ID_DISPLAY)) {
        return true;
    }
    return ParseDescIdDisplay(configJson[DESC_ID_DISPLAY], config, msg);
}

CustomInputConfig &CustomInputConfig::Instance() {
    static CustomInputConfig config;
    return config;
}

void CustomInputConfig::Reset() {
    mc2TraceConfig_ = Mc2TraceConfig{};
    timelineMergeConfigs_.clear();
}

bool CustomInputConfig::Load(const std::string &path, std::string &msg) {
    Reset();
    msg.clear();
    if (!path.empty()) {
        nlohmann::json customInputJson;
        Mc2TraceConfig trace;
        std::vector<TimelineMergeConfig> merge;
        if (!GetJsonData(path, customInputJson)) {
            msg = "Cannot read --custom-input json file.";
        } else if (!customInputJson.is_object()) {
            msg = "--custom-input json file must be an object.";
        } else if (ParseMc2TraceConfig(customInputJson, trace, msg) &&
            ParseTimelineMergeConfigs(customInputJson, merge, msg)) {
            mc2TraceConfig_ = std::move(trace);
            timelineMergeConfigs_ = std::move(merge);
        }
    }
    return msg.empty();
}

const std::vector<uint16_t> GetEventsByType(const ChipType &chipType, const std::string &blockType)
{
    if (chipType == ChipType::ASCEND310P) {
        return REPLAY_AIC_EVENTS_FOR_310P;
    }
    if (chipType == ChipType::ASCEND910B) {
        return blockType.find("cube") != std::string::npos ? REPLAY_AIC_EVENTS_FOR_910B : REPLAY_AIV_EVENTS_FOR_910B;
    }
    if (chipType == ChipType::ASCEND950) {
        return blockType.find("cube") != std::string::npos ? REPLAY_AIC_EVENTS_FOR_A5 : REPLAY_AIV_EVENTS_FOR_A5;
    }
    return {};
}

uint32_t GetUpNumCanDivisibleByX(uint16_t number, uint16_t x)
{
    if (x == 0) {
        LogDebug("Division by zero error in GetUpNumCanDivisibleByX");
        return 0;
    }
    return static_cast<uint32_t>(std::ceil(static_cast<double>(number) / static_cast<double>(x))) * x;
}

std::map<ProfMetrics, std::string> Mapping::onBoardMetricsToMsprof = {
    {ProfMetrics::PIPE_UTILIZATION,        std::string(MsprofMetrics::PIPE_UTILIZATION)},
    {ProfMetrics::ARITHMETIC_UTILIZATION,  std::string(MsprofMetrics::ARITHMETIC_UTILIZATION)},
    {ProfMetrics::L2_CACHE,                std::string(MsprofMetrics::L2_CACHE)},
    {ProfMetrics::MEMORY,                  std::string(MsprofMetrics::MEMORY)},
    {ProfMetrics::RESOURCE_CONFLICT_RATIO, std::string(MsprofMetrics::RESOURCE_CONFLICT_RATIO)},
    {ProfMetrics::MEMORY_L0,               std::string(MsprofMetrics::MEMORY_L0)},
    {ProfMetrics::MEMORY_UB,               std::string(MsprofMetrics::MEMORY_UB)},
    {ProfMetrics::OCCUPANCY,               std::string(MsprofMetrics::OCCUPANCY)},
    {ProfMetrics::ROOFLINE,                std::string(MsprofMetrics::ROOFLINE)}
};

std::vector<ProfMetrics> ProfMetricsAbilityConfig::Enabled() const
{
    std::vector<ProfMetrics> enabled;
    using ul = typename std::underlying_type<ProfMetrics>::type;
    for (auto it = static_cast<ul>(ProfMetrics::PIPE_UTILIZATION);
            it < static_cast<ul>(ProfMetrics::COUNT); ++it) {
        if (metricsConfig[it].isOn) {
            enabled.push_back(static_cast<ProfMetrics>(it));
        }
    }
    return enabled;
}

bool ProfMetricsAbilityConfig::HasEnabledPmu() const
{
    for (uint16_t i = 0; i < static_cast<uint16_t>(ProfMetrics::COUNT); i++) {
        if (metricsConfig[i].isOn) {
            return true;
        }
    }
    return false;
}

bool ProfMetricsAbilityConfig::HasEnabledAllPmu() const
{
    uint32_t enableMetrics = 0;
    for (uint32_t i = 0; i < static_cast<uint32_t>(ProfMetrics::COUNT); i++) {
        if (metricsConfig[i].isOn) {
            enableMetrics++;
        }
    }
    return enableMetrics == static_cast<uint32_t>(ProfMetrics::COUNT);
}

bool ParseMetricBool(std::string const &metric, ProfMetricsAbilityConfig &value)
{
    std::unordered_map<std::string, std::reference_wrapper<bool>> metricMap = {
        {std::string(MsprofMetrics::OCCUPANCY), std::ref(value.occupancyEnable)},
        {std::string(MsprofMetrics::ROOFLINE), std::ref(value.roofline)},
        {std::string(MsprofMetrics::TIMELINE_DETAIL), std::ref(value.isDeviceToSimulator)},
        {std::string(MsprofMetrics::SOURCE), std::ref(value.isSource)},
        {std::string(MsprofMetrics::PMSAMPLING), std::ref(value.pmSamplingEnable)},
        {std::string(MsprofMetrics::MEMORYDETAIL), std::ref(value.isMemoryDetail)},
        {std::string(MsprofMetrics::PCSAMPLING), std::ref(value.pcSamplingEnable)},
        {std::string(MsprofMetrics::PIPE_TIMELINE), std::ref(value.pipeTimelineEnable)},
        {std::string(MsprofMetrics::INSTR_TIMELINE), std::ref(value.instrTimelineEnable)},
        {std::string(MsprofMetrics::OVERHEAD), std::ref(value.overHead)},
    };
    if (metricMap.count(metric) == 0) {
        return false;
    }
    metricMap.at(metric).get() = true;
    static std::unordered_map<std::string, ProfMetrics> metricsNeedPmu{
        {std::string(MsprofMetrics::OCCUPANCY),     ProfMetrics::OCCUPANCY},
        {std::string(MsprofMetrics::ROOFLINE),      ProfMetrics::ROOFLINE}
    };
    if (metricsNeedPmu.find(metric) != metricsNeedPmu.end()) {
        value[metricsNeedPmu[metric]].isOn = true;
    }
    if (metric == std::string(MsprofMetrics::ROOFLINE) || metric == std::string(MsprofMetrics::MEMORYDETAIL)) {
        SetAllAbility(value);
    }
    if (metric == std::string(MsprofMetrics::PCSAMPLING)) {
        value.isSource = true;
    }
    return true;
}

bool ParseValue(std::string const &str, ProfMetricsAbilityConfig &value)
{
    if (str.length() > MAX_INPUT_STR_LENGTH) {
        LogError("Invalid --aic-metrics options!");
        return false;
    }
    if (str.empty()) {
        value = ProfMetricsAbilityConfig(true);
        return true;
    }
    value = ProfMetricsAbilityConfig(false);
    std::vector<std::string> argMetrics;
    SplitString(str, ',', argMetrics);
    for (auto it = argMetrics.begin(); it != argMetrics.end(); ) {
        for (auto& c : *it) {
            c = tolower(static_cast<unsigned char>(c));
        }
        it = it->empty() ? argMetrics.erase(it) : ++it;
    }
    value.metricVec = argMetrics;
    for (std::string const &metric : argMetrics) {
        if (metric == MsprofMetrics::DEFAULT) {
            SetAllAbility(value);
            continue;
        }
        if (metric == MsprofMetrics::BASIC_INFO) {
            value.isBasicInfo = true;
            continue;
        }
        if (metric == MsprofMetrics::KERNEL_SCALE) {
            value.isKernelScale = true;
            if (argMetrics.size() == 1) {
                SetAllAbility(value);
            }
            continue;
        } else if (ParseMetricBool(metric, value)) {
            continue;
        }
        auto it = StringToOnBoardMetrics.find(metric);
        if (it == StringToOnBoardMetrics.end()) {
            LogError("Invalid metric options!");
            return false;
        }
        value[it->second].isOn = true;
    }
    return true;
}

std::vector<uint16_t> PmuEventsId::GetEventsByMetrics(const ProfMetricsAbilityConfig &metrics,
                                                      const MetricEventsMapType &metricEventsMap) const
{
    std::set<uint16_t> eventSet;
    std::vector<uint16_t> eventVec;
    for (ProfMetrics metric : metrics.Enabled()) {
        auto it = metricEventsMap.find(Common::Mapping::onBoardMetricsToMsprof[metric]);
        if (it == metricEventsMap.end()) {
            continue;
        }
        for (uint16_t event: it->second) {
            if (eventSet.count(event) == 0) {
                eventVec.emplace_back(event);
                eventSet.insert(event);
            }
        }
    }
    return eventVec;
}

std::map<uint16_t, uint16_t> PmuEventsId::GetEventIndexMap(const std::vector<uint16_t> &sortedPmuDataVec,
                                                           std::vector<uint16_t> &eventVec) const
{
    std::map<uint16_t, uint16_t> indexMap;  // map<pum numbers, pum index in sortedPmuDataVec>
    for (size_t index = 0; index < sortedPmuDataVec.size(); index++) {
        if (indexMap.count(sortedPmuDataVec[index]) != 0) {
            LogWarn("Pmu data vector has repeat data, index is %d, number is %d", index, sortedPmuDataVec[index]);
            break;
        }
        if (sortedPmuDataVec[index] == 0) {
            continue;
        }
        indexMap[sortedPmuDataVec[index]] = index;
    }
    std::sort(eventVec.begin(), eventVec.end(), [&indexMap](uint16_t pre, uint16_t next) {
        if (indexMap.find(pre) == indexMap.end() || indexMap.find(next) == indexMap.end()) {
            LogDebug("indexMap does not has num key: %d/%d", pre, next);
            return false;
        }
        return indexMap[pre] < indexMap[next];
    });
    return indexMap;
}

std::vector<uint16_t> PmuEventsId::GetSortedEventsByMetrics(
    const ProfMetricsAbilityConfig &metrics, const MetricEventsMapType &metricEventsMap,
    const std::vector<uint16_t> &sortedPmuDataVec, const ChipType &chipType) const
{
    std::vector<uint16_t> eventVec = GetEventsByMetrics(metrics, metricEventsMap);
    bool isAddZero = false;
    // default ASCEND910B
    uint32_t eventMaxNum = EVENT_MAX_NUM;
    uint32_t pmuMaxNum = PMU_EVENT_MAX_NUM;
    if (chipType == ChipType::ASCEND950) {
        eventMaxNum = EVENT_MAX_NUM_A5;
        pmuMaxNum = PMU_EVENT_MAX_NUM_A5;
        // A5下pmu=0为有效数据，当出现时不能放置每轮重放的第一个pmu
        auto it = std::find(eventVec.begin(), eventVec.end(), 0);
        if (it != eventVec.end()) {
            isAddZero = true;
            eventVec.erase(it);
        }
    }
    std::map<uint16_t, uint16_t> indexMap = GetEventIndexMap(sortedPmuDataVec, eventVec);
    std::vector<uint16_t> resVec (eventMaxNum, 0U);
    uint16_t start = 0U;
    uint16_t end = 1U;
    uint32_t resVecIndex = 0U;
    while (start < eventVec.size() && !eventVec.empty()) {
        while (end < eventVec.size() && (indexMap[eventVec[end]] / pmuMaxNum ==
                                         indexMap[eventVec[end - 1]] / pmuMaxNum)) {
            end++;
        }
        uint32_t vacantNum = GetUpNumCanDivisibleByX(resVecIndex, pmuMaxNum) - resVecIndex;
        if (vacantNum + start < end) {
            resVecIndex = GetUpNumCanDivisibleByX(resVecIndex, pmuMaxNum);
        }
        while (start < end) {
            resVec[resVecIndex++] = eventVec[start++];
        }
        end++;
    }
    resVecIndex = GetUpNumCanDivisibleByX(resVecIndex, pmuMaxNum);
    resVec.resize(resVecIndex);
    if (isAddZero && !std::count(resVec.begin(), resVec.end(), 0) && !resVec.empty() && resVecIndex + pmuMaxNum <= eventMaxNum) {
        resVec.insert(resVec.end() - 1, 0);
        resVec.resize(resVecIndex + pmuMaxNum);
    }
    return resVec;
}

void PmuEventsId::LoadPmuVec(const ProfMetricsAbilityConfig &metrics, ChipType chipType, const std::string &replayMode)
{
    // 仅使能basicInfo时，kernel和application不进行pmu采集，range模式需要额外处理进行1轮pmu采集
    if (!metrics.HasEnabledPmu() && metrics.isBasicInfo) {
        aicPmu = {};
        aivPmu = {};
        return;
    }
    if (chipType == ChipType::ASCEND310P) {
        aicPmu = GetEventsByType(chipType, "");
        return;
    }

    std::vector<uint16_t> aicTotalEvents = GetEventsByType(chipType, "cube");
    std::vector<uint16_t> aivTotalEvents = GetEventsByType(chipType, "vector");
    // default ASCEND910B
    MetricEventsMapType aicMetricEventsMap = AIC_EVENTS_FOR_910B;
    MetricEventsMapType aivMetricEventsMap = AIV_EVENTS_FOR_910B;
    if (chipType == ChipType::ASCEND950) {
        aicMetricEventsMap = AIC_EVENTS_FOR_A5;
        aivMetricEventsMap = AIV_EVENTS_FOR_A5;
    }
    if (replayMode == "kernel" || replayMode == "range" || metrics.HasEnabledAllPmu()) {
        aicPmu = aicTotalEvents;
        aivPmu = aivTotalEvents;
    } else {
        aicPmu = GetSortedEventsByMetrics(metrics, aicMetricEventsMap, aicTotalEvents, chipType);
        aivPmu = GetSortedEventsByMetrics(metrics, aivMetricEventsMap, aivTotalEvents, chipType);
    }
}
}  // namespace Common
