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
#include "log.h"

using namespace Utility;
namespace Common {
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
        {std::string(MsprofMetrics::TIMELINE), std::ref(value.timelineEnable)},
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
