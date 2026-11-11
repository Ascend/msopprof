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

#ifndef __MSOPPROF_COMMON_CONFIG_H__
#define __MSOPPROF_COMMON_CONFIG_H__

#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <set>

#include "argparser/options.h"
#include "defs.h"
#include "ustring.h"
#include "log.h"
#include "op_runner.h"
#include "include/opprof/BasicDefs.h"

namespace Common {
struct TimelineMergeConfig {
    std::string kernelName;
    std::string mstxName;
    uint32_t rankCount = 0;
    uint32_t launchCount = 0;
};

struct Mc2TraceConfig {
    bool enableBlockTime = true;
    std::map<uint64_t, std::string> descIdDisplay;
};

class CustomInputConfig {
public:
    static CustomInputConfig &Instance();
    bool Load(const std::string &path, std::string &msg);
    void Reset();
    const Mc2TraceConfig &GetMc2TraceConfig() const { return mc2TraceConfig_; }
    const std::vector<TimelineMergeConfig> &GetTimelineMergeConfigs() const { return timelineMergeConfigs_; }

private:
    CustomInputConfig() = default;
    CustomInputConfig(const CustomInputConfig &) = delete;
    CustomInputConfig &operator=(const CustomInputConfig &) = delete;
    Mc2TraceConfig mc2TraceConfig_;
    std::vector<TimelineMergeConfig> timelineMergeConfigs_;
};

const std::map<std::string, ReplayMode> ReplayModeMap {
    {"kernel",       ReplayMode::KERNEL},
    {"application",  ReplayMode::APPLICATION},
    {"range",        ReplayMode::RANGE},
};

const std::vector<uint16_t> GetEventsByType(const ChipType &chipType, const std::string &blockType);

/// map from device metrics to msprof metrics option
struct Mapping {
    static std::map<ProfMetrics, std::string> onBoardMetricsToMsprof;
};

struct ProfMetricsAbilityConfig {
    std::vector<std::string> metricVec;
    Parser::OnOff metricsConfig[static_cast<uint32_t>(ProfMetrics::COUNT)];
    bool isKernelScale = false;
    bool occupancyEnable = false;
    bool isDeviceToSimulator = false;
    bool roofline = false;
    bool isSource = false;
    bool isBasicInfo = false;
    bool isMemoryDetail = false;
    bool pmSamplingEnable = false;
    bool pipeTimelineEnable = false;
    bool instrTimelineEnable = false;
    bool pcSamplingEnable = false;
    bool overHead = false;
    ReplayMode replayMode = ReplayMode::KERNEL;
    inline explicit ProfMetricsAbilityConfig(bool defaultOptionsStatus = true)
    {
        std::fill(std::begin(metricsConfig), std::end(metricsConfig), Parser::OnOff{defaultOptionsStatus});
    }

    inline Parser::OnOff &operator[](ProfMetrics metrics)
    {
        return metricsConfig[static_cast<uint32_t>(metrics)];
    }

    inline Parser::OnOff Getvalue(ProfMetrics metrics) const
    {
        return metricsConfig[static_cast<uint32_t>(metrics)];
    }

    inline bool IsOn(ProfMetrics profMetrics) const
    {
        return metricsConfig[static_cast<uint32_t>(profMetrics)].isOn;
    }
    std::vector<ProfMetrics> Enabled(void) const;
    bool HasEnabledPmu() const;
    bool HasEnabledAllPmu() const;
};

inline void SetAllAbility(ProfMetricsAbilityConfig &ability) {
    std::fill(std::begin(ability.metricsConfig), std::end(ability.metricsConfig), Parser::OnOff{true});
}

const std::map<std::string, ProfMetrics> StringToOnBoardMetrics = {
    {std::string(MsprofMetrics::PIPE_UTILIZATION),        ProfMetrics::PIPE_UTILIZATION},
    {std::string(MsprofMetrics::ARITHMETIC_UTILIZATION),  ProfMetrics::ARITHMETIC_UTILIZATION},
    {std::string(MsprofMetrics::L2_CACHE),                ProfMetrics::L2_CACHE},
    {std::string(MsprofMetrics::MEMORY),                  ProfMetrics::MEMORY},
    {std::string(MsprofMetrics::MEMORY_L0),               ProfMetrics::MEMORY_L0},
    {std::string(MsprofMetrics::MEMORY_UB),               ProfMetrics::MEMORY_UB},
    {std::string(MsprofMetrics::RESOURCE_CONFLICT_RATIO), ProfMetrics::RESOURCE_CONFLICT_RATIO}
};

bool ParseValue(std::string const &str, ProfMetricsAbilityConfig &value);

struct ProfMetricsSet {
    std::vector<std::string> metrics;
    std::string description;
};

extern const std::map<std::string, ProfMetricsSet> SET_DEFINITIONS_A2A3;
extern const std::map<std::string, ProfMetricsSet> SET_DEFINITIONS_A5;

bool ResolveSet(const std::string &setName, ProfMetricsAbilityConfig &config,
              ChipType chipType, std::string &errMsg);

struct PmuEventsId {
    std::vector<uint16_t> aicPmu;
    std::vector<uint16_t> aivPmu;
    void LoadPmuVec(const ProfMetricsAbilityConfig &metrics,
                    ChipType chipType, const std::string &replayMode = "kernel");
private:
    std::vector<uint16_t> GetEventsByMetrics(const MetricEventsMapType &metricEventsMap) const;
    std::vector<uint16_t> GetEventsByMetrics(const ProfMetricsAbilityConfig &metrics,
                                             const MetricEventsMapType &metricEventsMap) const;
    std::vector<uint16_t> GetSortedEventsByMetrics(
        const ProfMetricsAbilityConfig &metrics, const MetricEventsMapType &metricEventsMap,
        const std::vector<uint16_t> &sortedPmuDataVec, const ChipType &chipType = ChipType::ASCEND910B) const;
    std::map<uint16_t, uint16_t> GetEventIndexMap(const std::vector<uint16_t> &sortedPmuDataVec,
                                                  std::vector<uint16_t> &eventVec) const;
};

struct ProfArgs {
    bool printHelp { false };
    bool printVersion { false };
    bool listSets { false };
    bool deprecatedVersion { false };
    std::string runMode { "device" };
    std::string argOutput {"./" };
    std::string argExport;
    std::string argConfig;
    std::string argApplication;  // input --application option
    std::string argApps;                // input apps with none option
    ProfMetricsAbilityConfig argAicMetrics { };
    std::string argKernelName;
    std::string argLaunchCount {"1" };
    std::string argLaunchSkipBeforeMatch {"0" };
    std::string argMstx {"false" };
    std::string argMstxInclude { };
    std::string argReplayMode {"kernel"};
    std::string argKill {"false"};
    std::string argSocVersion;
    std::string argCoreId;
    std::string argWarmUp {"5"};
    std::string argTimeout;
    std::string argDump {"false"};
    std::string argInstrTimelinePipe{};
    std::string argCustomInput;
    std::string argSet;
    bool isSetSpecified{false};
    std::vector<std::string> cmd { };
    OpRunner::KernelConfig kernelConfig;
};

} // namespace Common

#endif  // __MSOPPROF_COMMON_CONFIG_H__
