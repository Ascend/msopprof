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


#include <gtest/gtest.h>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <limits.h>
#include <sys/wait.h>
#include <unistd.h>

#include "argparser/options.h"
#include "argparser/arg_normalize.h"
#include "common/prof_args.h"
#include "common/hal_helper.h"
#include "common/runtime_helper.h"
#include "filesystem.h"
#include "json_defs.h"

using namespace Common;
using namespace Utility;

TEST(ProfArgs, on_board_metrics_config_default_construct_expect_all_false)
{
    ProfMetricsAbilityConfig metrics;
    ASSERT_TRUE(std::all_of(std::begin(metrics.metricsConfig), std::end(metrics.metricsConfig),
                             [=](Parser::OnOff f) { return f.isOn; }));
}

TEST(ProfArgs, on_board_metrics_config_subscript_oprerator_expect_correct_get_and_set)
{
    ProfMetricsAbilityConfig metrics;
    metrics[ProfMetrics::MEMORY] = {true};
    ASSERT_TRUE(metrics[ProfMetrics::MEMORY].isOn);
}

TEST(ProfArgs, on_board_metrics_config_set_some_true_expect_get_correct_metrics)
{
    ProfMetricsAbilityConfig metrics;
    metrics[ProfMetrics::ARITHMETIC_UTILIZATION] = {false};
    metrics[ProfMetrics::MEMORY] = {false};
    auto enabled = metrics.Enabled();
    ASSERT_EQ(enabled.size(), 7);
    ASSERT_EQ(enabled[0], ProfMetrics::PIPE_UTILIZATION);
    ASSERT_EQ(enabled[1], ProfMetrics::L2_CACHE);
    ASSERT_EQ(enabled[2], ProfMetrics::RESOURCE_CONFLICT_RATIO);
    ASSERT_EQ(enabled[3], ProfMetrics::MEMORY_L0);
    ASSERT_EQ(enabled[4], ProfMetrics::MEMORY_UB);
    ASSERT_EQ(enabled[5], ProfMetrics::OCCUPANCY);
    ASSERT_EQ(enabled[6], ProfMetrics::ROOFLINE);
}

TEST(ProfArgs, dlopen_failed)
{
    int64_t num;
    ASSERT_FALSE(HalHelper::Instance().GetAicoreFreq(num));
    ASSERT_FALSE(HalHelper::Instance().GetTaskSchedulerFreq(num));
}

TEST(RuntimeHelper, get_soc_version_failed_repeatedly_expect_warn_once) {
    constexpr char childProcessEnv[] = "MSOPPROF_RUNTIME_HELPER_WARNING_CHILD";
    if (std::getenv(childProcessEnv) != nullptr) {
        RuntimeHelper &helper = RuntimeHelper::Instance();
        EXPECT_TRUE(helper.GetSocVersion().empty());
        EXPECT_TRUE(helper.GetSocVersion().empty());
        EXPECT_TRUE(helper.GetSocVersion().empty());
        return;
    }

    std::array<char, PATH_MAX> executablePath{};
    ssize_t pathLength = readlink("/proc/self/exe", executablePath.data(), executablePath.size() - 1);
    ASSERT_GT(pathLength, 0);
    executablePath[static_cast<size_t>(pathLength)] = '\0';
    std::string command = "env ASCEND_HOME_PATH=/__msopprof_runtime_helper_ut_missing__ " +
        std::string(childProcessEnv) + "=1 '" + executablePath.data() +
        "' --gtest_filter=RuntimeHelper.get_soc_version_failed_repeatedly_expect_warn_once --gtest_color=no";
    FILE *childPipe = popen(command.c_str(), "r");
    ASSERT_NE(childPipe, nullptr);

    std::string childOutput;
    std::array<char, 1024> outputBuffer{};
    while (fgets(outputBuffer.data(), outputBuffer.size(), childPipe) != nullptr) {
        childOutput += outputBuffer.data();
    }
    int childStatus = pclose(childPipe);
    ASSERT_NE(childStatus, -1);
    ASSERT_TRUE(WIFEXITED(childStatus)) << childOutput;
    ASSERT_EQ(WEXITSTATUS(childStatus), 0) << childOutput;

    const std::string warning = "Get soc version from runtime failed";
    size_t warningCount = 0;
    size_t warningPos = 0;
    while ((warningPos = childOutput.find(warning, warningPos)) != std::string::npos) {
        ++warningCount;
        warningPos += warning.size();
    }
    EXPECT_EQ(warningCount, 1U) << childOutput;
}

TEST(DeviceTask, LoadPmuVec_App_910B_replay)
{
    ProfMetricsAbilityConfig metrics;
    PmuEventsId pmuEventsId;
    pmuEventsId.LoadPmuVec(metrics, ChipType::ASCEND910B, "application");
    ASSERT_EQ(pmuEventsId.aicPmu[2], 1280);
    ASSERT_EQ(pmuEventsId.aivPmu[11], 1288);
}

TEST(DeviceTask, LoadPmuVec_App_910B_ArithmeticUtilization_Memory_replay)
{
    ProfArgs args;
    ProfMetricsAbilityConfig metrics = ProfMetricsAbilityConfig(false);
    metrics[ProfMetrics::ARITHMETIC_UTILIZATION] = {true};
    metrics[ProfMetrics::MEMORY] = {true};
    args.argAicMetrics = metrics;
    PmuEventsId pmuEventsId;
    pmuEventsId.LoadPmuVec(metrics, Common::ChipType::ASCEND910B, "application");
    std::vector<uint16_t> aic_res = {1032,1033,1292,1293,1294,0,0,0,19,49,50,518,524,0,0,0,3,4,5,6,526,13,73,74,10,12,770,0,0,0,0,0};
    std::vector<uint16_t> aiv_res = {1292,1293,1294,61,62,0,0,0,1,75,76,77,78,79,174,186,5,6,184,185,4,526,13,0,8,12,0,0,0,0,0,0};
    ASSERT_EQ(aic_res, pmuEventsId.aicPmu);
    ASSERT_EQ(aiv_res, pmuEventsId.aivPmu);
}

TEST(DeviceTask, LoadPmuVec_App_910B_PipeUtilization_MemoryL0_MemoryUB_replay)
{
    ProfArgs args;
    ProfMetricsAbilityConfig metrics = ProfMetricsAbilityConfig(false);
    metrics[ProfMetrics::PIPE_UTILIZATION] = {true};
    metrics[ProfMetrics::MEMORY_L0] = {true};
    metrics[ProfMetrics::MEMORY_UB] = {true};
    args.argAicMetrics = metrics;
    PmuEventsId pmuEventsId;
    pmuEventsId.LoadPmuVec(metrics, ChipType::ASCEND910B, "application");
    std::vector<uint16_t> aic_res = {27,28,33,34,40,42,84,85,1792,1793,1794,1795,1796,1797,1798,1799,1780,1781,1782,1783,1784,1785,1786,1787,107,108,109,110,112,113,0,0,13,87,114,9,10,12,770,771};
    std::vector<uint16_t> aiv_res = {55,56,67,68,0,0,0,0,1792,1793,1794,1795,1796,1797,1798,1799,1780,1781,1782,1783,1784,1785,1786,1787,106,108,109,111,112,113,0,0,84,85,13,87,114,0,0,0,8,9,12,770,771,0,0,0};
    ASSERT_EQ(aic_res, pmuEventsId.aicPmu);
    ASSERT_EQ(aiv_res, pmuEventsId.aivPmu);
}

TEST(DeviceTask, LoadPmuVec_App_910B_Memory_ResourceConflictRatio_replay)
{
    ProfArgs args;
    ProfMetricsAbilityConfig metrics = ProfMetricsAbilityConfig(false);
    metrics[ProfMetrics::MEMORY] = {true};
    metrics[ProfMetrics::RESOURCE_CONFLICT_RATIO] = {true};
    args.argAicMetrics = metrics;
    PmuEventsId pmuEventsId;
    pmuEventsId.LoadPmuVec(metrics, ChipType::ASCEND910B, "application");
    std::vector<uint16_t> aic_res = {1292,1293,1294,19,49,50,518,524,4,5,6,526,13,92,0,0,12,88,90,91,770,0,0,0};
    std::vector<uint16_t> aiv_res = {1292,1293,1294,61,62,0,0,0,5,6,100,101,102,103,0,0,4,526,13,92,12,89,90,91};
    ASSERT_EQ(aic_res, pmuEventsId.aicPmu);
    ASSERT_EQ(aiv_res, pmuEventsId.aivPmu);
}

TEST(DeviceTask, LoadPmuVec_App_910B_Memory_MemoryL0_replay)
{
    ProfArgs args;
    ProfMetricsAbilityConfig metrics = ProfMetricsAbilityConfig(false);
    metrics[ProfMetrics::MEMORY] = {true};
    metrics[ProfMetrics::MEMORY_L0] = {true};
    args.argAicMetrics = metrics;
    PmuEventsId pmuEventsId;
    pmuEventsId.LoadPmuVec(metrics, ChipType::ASCEND910B, "application");
    std::vector<uint16_t> aic_res = {1292,1293,1294,19,49,50,518,524,4,5,6,526,0,0,0,0,27,28,33,34,40,42,13,0,12,770,0,0,0,0,0,0};
    std::vector<uint16_t> aiv_res = {1292,1293,1294,61,62,5,6,0,4,526,13,12,0,0,0,0};
    ASSERT_EQ(aic_res, pmuEventsId.aicPmu);
    ASSERT_EQ(aiv_res, pmuEventsId.aivPmu);
}

TEST(DeviceTask, LoadPmuVec_App_910B_Memory_MemoryL0_L2Cache_replay)
{
    ProfArgs args;
    ProfMetricsAbilityConfig metrics = ProfMetricsAbilityConfig(false);
    metrics[ProfMetrics::MEMORY] = {true};
    metrics[ProfMetrics::MEMORY_L0] = {true};
    metrics[ProfMetrics::L2_CACHE] = {true};
    args.argAicMetrics = metrics;
    PmuEventsId pmuEventsId;
    pmuEventsId.LoadPmuVec(metrics, ChipType::ASCEND910B, "application");
    std::vector<uint16_t> aic_res = {1280,1282,1283,1292,0,0,0,0,1284,1286,1287,1288,1290,1291,1293,1294,19,49,50,518,524,0,0,0,4,5,6,526,0,0,0,0,27,28,33,34,40,42,13,0,12,770,0,0,0,0,0,0};
    std::vector<uint16_t> aiv_res = {1280,1282,1283,1292,0,0,0,0,1284,1286,1287,1288,1290,1291,1293,1294,61,62,5,6,4,526,13,12};
    ASSERT_EQ(aic_res, pmuEventsId.aicPmu);
    ASSERT_EQ(aiv_res, pmuEventsId.aivPmu);
}

TEST(DeviceTask, LoadPmuVec_App_910B_Memory_MemoryL0_L2Cache_ArithmeticUtilization_replay)
{
    ProfArgs args;
    ProfMetricsAbilityConfig metrics = ProfMetricsAbilityConfig(false);
    metrics[ProfMetrics::MEMORY] = {true};
    metrics[ProfMetrics::MEMORY_L0] = {true};
    metrics[ProfMetrics::L2_CACHE] = {true};
    metrics[ProfMetrics::ARITHMETIC_UTILIZATION] = {true};
    args.argAicMetrics = metrics;
    PmuEventsId pmuEventsId;
    pmuEventsId.LoadPmuVec(metrics, ChipType::ASCEND910B, "application");
    std::vector<uint16_t> aic_res = {1032,1033,1280,1282,1283,1292,0,0,1284,1286,1287,1288,1290,1291,1293,1294,19,49,50,518,524,0,0,0,3,4,5,6,526,0,0,0,27,28,33,34,40,42,0,0,13,73,74,10,12,770,0,0};
    std::vector<uint16_t> aiv_res = {1280,1282,1283,1292,0,0,0,0,1284,1286,1287,1288,1290,1291,1293,1294,61,62,0,0,0,0,0,0,1,75,76,77,78,79,174,186,5,6,184,185,4,526,13,0,8,12,0,0,0,0,0,0};
    ASSERT_EQ(aic_res, pmuEventsId.aicPmu);
    ASSERT_EQ(aiv_res, pmuEventsId.aivPmu);
}

TEST(DeviceTask, LoadPmuVec_App_910B_L2Cache_ArithmeticUtilization_replay)
{
    ProfArgs args;
    ProfMetricsAbilityConfig metrics = ProfMetricsAbilityConfig(false);
    metrics[ProfMetrics::L2_CACHE] = {true};
    metrics[ProfMetrics::ARITHMETIC_UTILIZATION] = {true};
    args.argAicMetrics = metrics;
    PmuEventsId pmuEventsId;
    pmuEventsId.LoadPmuVec(metrics, ChipType::ASCEND910B, "application");
    std::vector<uint16_t> aic_res = {1032,1033,1280,1282,1283,0,0,0,1284,1286,1287,1288,1290,1291,3,0,73,74,10,0,0,0,0,0 };
    std::vector<uint16_t> aiv_res = {1280,1282,1283,0,0,0,0,0,1284,1286,1287,1288,1290,1291,0,0,1,75,76,77,78,79,174,186,184,185,8,0,0,0,0,0};
    ASSERT_EQ(aic_res, pmuEventsId.aicPmu);
    ASSERT_EQ(aiv_res, pmuEventsId.aivPmu);
}

TEST(DeviceTask, LoadPmuVec_App_A5_Pipe_Memoryl0_replay)
{
    ProfArgs args;
    ProfMetricsAbilityConfig metrics = ProfMetricsAbilityConfig(false);
    metrics[ProfMetrics::PIPE_UTILIZATION] = {true};
    metrics[ProfMetrics::MEMORY_L0] = {true};
    args.argAicMetrics = metrics;
    PmuEventsId pmuEventsId;
    pmuEventsId.LoadPmuVec(metrics, Common::ChipType::ASCEND950, "application");
    std::vector<uint16_t> aic_res = {1813, 52, 53, 772, 774, 776, 778, 1795, 1797, 0, 10, 36, 1, 514, 515, 810, 1794, 1812, 0, 0};
    std::vector<uint16_t> aiv_res = {52, 53, 10, 1, 514, 515, 1281, 0, 0, 0};
    ASSERT_EQ(aic_res, pmuEventsId.aicPmu);
    ASSERT_EQ(aiv_res, pmuEventsId.aivPmu);
}

TEST(DeviceTask, LoadPmuVec_App_310P_replay)
{
    ProfArgs args;
    ProfMetricsAbilityConfig metrics;
    PmuEventsId pmuEventsId;
    pmuEventsId.LoadPmuVec(metrics, ChipType::ASCEND310P);
    std::vector<uint16_t> aic_res = {3,    73,   74,   75,   76,   77,   78,   79,
        174,  184,  185,  186,  4,    5,    6,    18,
        19,   44,   49,   50,   61,   62,   27,   28,
        33,   34,   39,   40,   41,   42,   55,   56,
        67,   68,   1,    84,   85,   87,   100,  101,
        102,  103,  9,    13,   92,   0,    0,    0,
        8,    10,   11,   12,   88,   89,   90,   91};
    ASSERT_EQ(pmuEventsId.aicPmu, aic_res);
}

TEST(DeviceTask, LoadPmuVec_range_replay)
{
    ProfArgs args;
    ProfMetricsAbilityConfig metrics;
    PmuEventsId pmuEventsId;
    pmuEventsId.LoadPmuVec(metrics, ChipType::ASCEND910B, "range");
    ASSERT_EQ(pmuEventsId.aicPmu, REPLAY_AIC_EVENTS_FOR_910B);
    ASSERT_EQ(pmuEventsId.aivPmu, REPLAY_AIV_EVENTS_FOR_910B);
}

TEST(DeviceTask, LoadPmuVec_kernel_replay)
{
    ProfArgs args;
    ProfMetricsAbilityConfig metrics;
    PmuEventsId pmuEventsId;
    pmuEventsId.LoadPmuVec(metrics, ChipType::ASCEND910B, "kernel");
    ASSERT_EQ(pmuEventsId.aicPmu, REPLAY_AIC_EVENTS_FOR_910B);
    ASSERT_EQ(pmuEventsId.aivPmu, REPLAY_AIV_EVENTS_FOR_910B);
}

TEST(ParseValue, aic_metrics_default_expect_success)
{
    ProfMetricsAbilityConfig metrics;
    ASSERT_TRUE(ParseValue("Default", metrics));
    ASSERT_TRUE(metrics.metricsConfig[static_cast<uint32_t>(ProfMetrics::PIPE_UTILIZATION)].isOn);
    ASSERT_TRUE(metrics.metricsConfig[static_cast<uint32_t>(ProfMetrics::ARITHMETIC_UTILIZATION)].isOn);
    ASSERT_TRUE(metrics.metricsConfig[static_cast<uint32_t>(ProfMetrics::L2_CACHE)].isOn);
    ASSERT_TRUE(metrics.metricsConfig[static_cast<uint32_t>(ProfMetrics::MEMORY)].isOn);
    ASSERT_TRUE(metrics.metricsConfig[static_cast<uint32_t>(ProfMetrics::RESOURCE_CONFLICT_RATIO)].isOn);
    ASSERT_TRUE(metrics.metricsConfig[static_cast<uint32_t>(ProfMetrics::MEMORY_L0)].isOn);
    ASSERT_TRUE(metrics.metricsConfig[static_cast<uint32_t>(ProfMetrics::MEMORY_UB)].isOn);
    ASSERT_FALSE(metrics.occupancyEnable);
    ASSERT_FALSE(metrics.roofline);
    ASSERT_FALSE(metrics.isDeviceToSimulator);
}

TEST(ParseValue, aic_metrics_partial_expect_success)
{
    ProfMetricsAbilityConfig metrics;
    ASSERT_TRUE(ParseValue("ArithmeticUtilization,Memory,Occupancy,TimelineDetail", metrics));
    ASSERT_FALSE(metrics.metricsConfig[static_cast<uint32_t>(ProfMetrics::PIPE_UTILIZATION)].isOn);
    ASSERT_TRUE(metrics.metricsConfig[static_cast<uint32_t>(ProfMetrics::ARITHMETIC_UTILIZATION)].isOn);
    ASSERT_FALSE(metrics.metricsConfig[static_cast<uint32_t>(ProfMetrics::L2_CACHE)].isOn);
    ASSERT_TRUE(metrics.metricsConfig[static_cast<uint32_t>(ProfMetrics::MEMORY)].isOn);
    ASSERT_FALSE(metrics.metricsConfig[static_cast<uint32_t>(ProfMetrics::RESOURCE_CONFLICT_RATIO)].isOn);
    ASSERT_FALSE(metrics.metricsConfig[static_cast<uint32_t>(ProfMetrics::MEMORY_L0)].isOn);
    ASSERT_FALSE(metrics.metricsConfig[static_cast<uint32_t>(ProfMetrics::MEMORY_UB)].isOn);
    ASSERT_TRUE(metrics.occupancyEnable);
    ASSERT_TRUE(metrics.isDeviceToSimulator);
}

TEST(ParseValue, aic_metrics_roofline_expect_success)
{
    ProfMetricsAbilityConfig metrics;
    ASSERT_TRUE(ParseValue("roofline", metrics));
    ASSERT_TRUE(metrics.metricsConfig[static_cast<uint32_t>(ProfMetrics::PIPE_UTILIZATION)].isOn);
    ASSERT_TRUE(metrics.metricsConfig[static_cast<uint32_t>(ProfMetrics::ARITHMETIC_UTILIZATION)].isOn);
    ASSERT_TRUE(metrics.metricsConfig[static_cast<uint32_t>(ProfMetrics::L2_CACHE)].isOn);
    ASSERT_TRUE(metrics.metricsConfig[static_cast<uint32_t>(ProfMetrics::MEMORY)].isOn);
    ASSERT_TRUE(metrics.metricsConfig[static_cast<uint32_t>(ProfMetrics::RESOURCE_CONFLICT_RATIO)].isOn);
    ASSERT_TRUE(metrics.metricsConfig[static_cast<uint32_t>(ProfMetrics::MEMORY_L0)].isOn);
    ASSERT_TRUE(metrics.metricsConfig[static_cast<uint32_t>(ProfMetrics::MEMORY_UB)].isOn);
    ASSERT_TRUE(metrics.roofline);
}


TEST(ParseValue, aic_metrics_only_kernel_scale_expect_success)
{
    ProfMetricsAbilityConfig metrics;
    ASSERT_TRUE(ParseValue("KernelScale", metrics));
    ASSERT_TRUE(metrics.isKernelScale);
    ASSERT_TRUE(metrics.metricsConfig[static_cast<uint32_t>(ProfMetrics::PIPE_UTILIZATION)].isOn);
    ASSERT_TRUE(metrics.metricsConfig[static_cast<uint32_t>(ProfMetrics::ARITHMETIC_UTILIZATION)].isOn);
    ASSERT_TRUE(metrics.metricsConfig[static_cast<uint32_t>(ProfMetrics::L2_CACHE)].isOn);
    ASSERT_TRUE(metrics.metricsConfig[static_cast<uint32_t>(ProfMetrics::MEMORY)].isOn);
    ASSERT_TRUE(metrics.metricsConfig[static_cast<uint32_t>(ProfMetrics::RESOURCE_CONFLICT_RATIO)].isOn);
    ASSERT_TRUE(metrics.metricsConfig[static_cast<uint32_t>(ProfMetrics::MEMORY_L0)].isOn);
    ASSERT_TRUE(metrics.metricsConfig[static_cast<uint32_t>(ProfMetrics::MEMORY_UB)].isOn);
}

TEST(ParseValue, aic_metrics_input_str_expect_failed)
{
    const std::string str1 = "aaaaa";
    ProfMetricsAbilityConfig value;
    ASSERT_FALSE(ParseValue(str1, value));
}

TEST(ParseValue, aic_metrics_empty_expect_default_seven_metrics_on)
{
    ProfMetricsAbilityConfig metrics;
    ASSERT_TRUE(ParseValue("", metrics));
    ASSERT_TRUE(metrics.metricsConfig[static_cast<uint32_t>(ProfMetrics::PIPE_UTILIZATION)].isOn);
    ASSERT_TRUE(metrics.metricsConfig[static_cast<uint32_t>(ProfMetrics::ARITHMETIC_UTILIZATION)].isOn);
    ASSERT_TRUE(metrics.metricsConfig[static_cast<uint32_t>(ProfMetrics::L2_CACHE)].isOn);
    ASSERT_TRUE(metrics.metricsConfig[static_cast<uint32_t>(ProfMetrics::MEMORY)].isOn);
    ASSERT_TRUE(metrics.metricsConfig[static_cast<uint32_t>(ProfMetrics::RESOURCE_CONFLICT_RATIO)].isOn);
    ASSERT_TRUE(metrics.metricsConfig[static_cast<uint32_t>(ProfMetrics::MEMORY_L0)].isOn);
    ASSERT_TRUE(metrics.metricsConfig[static_cast<uint32_t>(ProfMetrics::MEMORY_UB)].isOn);
}

TEST(ParseValue, aic_metrics_basicInfo_expect_success)
{
    ProfMetricsAbilityConfig metrics;
    ASSERT_TRUE(ParseValue("BasicInfo", metrics));
    ASSERT_TRUE(metrics.isBasicInfo);
}

TEST(ResolveSet, set_default_expect_all_seven_pmu_enabled)
{
    ProfMetricsAbilityConfig metrics;
    std::string errMsg;
    ASSERT_TRUE(ResolveSet("default", metrics, ChipType::ASCEND910B, errMsg));
    ASSERT_TRUE(metrics.IsOn(ProfMetrics::PIPE_UTILIZATION));
    ASSERT_TRUE(metrics.IsOn(ProfMetrics::ARITHMETIC_UTILIZATION));
    ASSERT_TRUE(metrics.IsOn(ProfMetrics::L2_CACHE));
    ASSERT_TRUE(metrics.IsOn(ProfMetrics::MEMORY));
    ASSERT_TRUE(metrics.IsOn(ProfMetrics::RESOURCE_CONFLICT_RATIO));
    ASSERT_TRUE(metrics.IsOn(ProfMetrics::MEMORY_L0));
    ASSERT_TRUE(metrics.IsOn(ProfMetrics::MEMORY_UB));
    ASSERT_FALSE(metrics.occupancyEnable);
    ASSERT_FALSE(metrics.roofline);
    ASSERT_FALSE(metrics.isMemoryDetail);
    ASSERT_FALSE(metrics.isSource);
}

TEST(ResolveSet, set_detailed_expect_occupancy_roofline_A2A3)
{
    ProfMetricsAbilityConfig metrics;
    std::string errMsg;
    ASSERT_TRUE(ResolveSet("detailed", metrics, ChipType::ASCEND910B, errMsg));
    ASSERT_TRUE(metrics.HasEnabledAllPmu());
    ASSERT_TRUE(metrics.occupancyEnable);
    ASSERT_TRUE(metrics.roofline);
    ASSERT_FALSE(metrics.isMemoryDetail);
    ASSERT_FALSE(metrics.isSource);
}

TEST(ResolveSet, set_full_expect_memory_detail_source_A2A3)
{
    ProfMetricsAbilityConfig metrics;
    std::string errMsg;
    ASSERT_TRUE(ResolveSet("full", metrics, ChipType::ASCEND910B, errMsg));
    ASSERT_TRUE(metrics.HasEnabledAllPmu());
 	ASSERT_TRUE(metrics.occupancyEnable);
    ASSERT_TRUE(metrics.roofline);
    ASSERT_TRUE(metrics.isMemoryDetail);
    ASSERT_TRUE(metrics.isSource);
}

TEST(ResolveSet, set_detailed_A5_expect_pipe_timeline)
{
    ProfMetricsAbilityConfig metrics;
 	std::string errMsg;
    ASSERT_TRUE(ResolveSet("detailed", metrics, ChipType::ASCEND950, errMsg));
    ASSERT_TRUE(metrics.HasEnabledAllPmu());
    ASSERT_TRUE(metrics.occupancyEnable);
    ASSERT_TRUE(metrics.roofline);
    ASSERT_TRUE(metrics.pipeTimelineEnable);
    ASSERT_FALSE(metrics.isMemoryDetail);
    ASSERT_FALSE(metrics.isSource);
}

TEST(ResolveSet, set_full_A5_expect_pcsampling_instrtimeline)
{
    ProfMetricsAbilityConfig metrics;
    std::string errMsg;
    ASSERT_TRUE(ResolveSet("full", metrics, ChipType::ASCEND950, errMsg));
    ASSERT_TRUE(metrics.HasEnabledAllPmu());
    ASSERT_TRUE(metrics.occupancyEnable);
    ASSERT_TRUE(metrics.roofline);
    ASSERT_TRUE(metrics.isMemoryDetail);
    ASSERT_TRUE(metrics.isSource);
    ASSERT_TRUE(metrics.pcSamplingEnable);
    ASSERT_TRUE(metrics.instrTimelineEnable);
}

TEST(ResolveSet, set_invalid_expect_failure)
{
    ProfMetricsAbilityConfig metrics;
    std::string errMsg;
    ASSERT_FALSE(ResolveSet("extra", metrics, ChipType::ASCEND910B, errMsg));
    ASSERT_NE(errMsg.find("--set must be one of"), std::string::npos);
}

TEST(ResolveSet, set_case_insensitive_expect_success)
{
    ProfMetricsAbilityConfig metrics;
    std::string errMsg;
    ASSERT_TRUE(ResolveSet("DEFAULT", metrics, ChipType::ASCEND910B, errMsg));
    ASSERT_TRUE(metrics.HasEnabledAllPmu());
}

TEST(ResolveSet, set_detailed_case_mixed_expect_success)
{
    ProfMetricsAbilityConfig metrics;
    std::string errMsg;
    ASSERT_TRUE(ResolveSet("Detailed", metrics, ChipType::ASCEND910B, errMsg));
    ASSERT_TRUE(metrics.occupancyEnable);
    ASSERT_TRUE(metrics.roofline);
}

TEST(ResolveSet, set_comma_combination_expect_failure)
{
    ProfMetricsAbilityConfig metrics;
    std::string errMsg;
    ASSERT_FALSE(ResolveSet("default,detailed", metrics, ChipType::ASCEND910B, errMsg));
    ASSERT_NE(errMsg.find("--set must be one of"), std::string::npos);
}

TEST(ResolveSet, set_empty_value_expect_failure)
{
    ProfMetricsAbilityConfig metrics;
    std::string errMsg;
    ASSERT_FALSE(ResolveSet("", metrics, ChipType::ASCEND910B, errMsg));
    ASSERT_NE(errMsg.find("--set must be one of"), std::string::npos);
}

/**
 * | 用例集 | CustomInputConfigTest
 * |测试函数| CustomInputConfig::Load
 * | 用例名 | empty_path_expect_defaults
 * |用例描述| 测试空路径时使用默认显示配置，且不开启流水合并。
 */
TEST(CustomInputConfigTest, empty_path_expect_defaults) {
    auto &config = CustomInputConfig::Instance();
    config.Reset();
    std::string msg;
    ASSERT_TRUE(config.Load("", msg));
    EXPECT_TRUE(config.GetMc2TraceConfig().enableBlockTime);
    EXPECT_TRUE(config.GetMc2TraceConfig().descIdDisplay.empty());
    EXPECT_TRUE(config.GetTimelineMergeConfigs().empty());
    config.Reset();
}

/**
 * | 用例集 | CustomInputConfigTest
 * |测试函数| CustomInputConfig::Load
 * | 用例名 | decimal_and_hex_ids_expect_parsed
 * |用例描述| 测试descid_display解析十进制和十六进制key正确。
 */
TEST(CustomInputConfigTest, decimal_and_hex_ids_expect_parsed) {
    auto &config = CustomInputConfig::Instance();
    config.Reset();
    nlohmann::json descIdJson;
    descIdJson["65536"] = "NOP";
    descIdJson["0x10"] = "LOAD";
    descIdJson["0x10001"] = "STORE";
    descIdJson["0X10002"] = "COMPUTE";
    const std::string path = "test/ut/resources/custom_input_ids.json";
    const nlohmann::json input = {{"mc2_trace_config", {{"descid_display", descIdJson}}}};
    ASSERT_TRUE(WriteFileByStream(path, input.dump()));
    std::string msg;
    ASSERT_TRUE(config.Load(path, msg));
    const auto &names = config.GetMc2TraceConfig().descIdDisplay;
    EXPECT_EQ(names.size(), 4U);
    EXPECT_EQ(names.at(65536), "NOP");
    EXPECT_EQ(names.at(16), "LOAD");
    EXPECT_EQ(names.at(65537), "STORE");
    EXPECT_EQ(names.at(65538), "COMPUTE");
    RemoveAll(path);
    config.Reset();
}

/**
 * | 用例集 | CustomInputConfigTest
 * |测试函数| CustomInputConfig::Load
 * | 用例名 | invalid_key_expect_reject_whole_config
 * |用例描述| 测试无效key导致整份配置失败，不发布部分有效的显示名称。
 */
TEST(CustomInputConfigTest, invalid_key_expect_reject_whole_config) {
    auto &config = CustomInputConfig::Instance();
    config.Reset();
    nlohmann::json descIdJson;
    descIdJson["abc"] = "INVALID_KEY";
    descIdJson["0x"] = "EMPTY_HEX";
    descIdJson["1"] = "VALID_NUM";
    const std::string path = "test/ut/resources/custom_input_bad_key.json";
    const nlohmann::json input = {{"mc2_trace_config", {{"descid_display", descIdJson}}}};
    ASSERT_TRUE(WriteFileByStream(path, input.dump()));
    std::string msg;
    EXPECT_FALSE(config.Load(path, msg));
    EXPECT_NE(msg.find("key"), std::string::npos);
    EXPECT_TRUE(config.GetMc2TraceConfig().descIdDisplay.empty());
    RemoveAll(path);
    config.Reset();
}

/**
 * | 用例集 | CustomInputConfigTest
 * |测试函数| CustomInputConfig::Load
 * | 用例名 | invalid_value_expect_reject_whole_config
 * |用例描述| 测试非字符串、非法字符和超长名称均导致配置失败。
 */
TEST(CustomInputConfigTest, invalid_value_expect_reject_whole_config) {
    auto &config = CustomInputConfig::Instance();
    const std::string path = "test/ut/resources/custom_input_bad_value.json";
    for (const auto &value :
        {nlohmann::json(123), nlohmann::json("invalid@chars!"), nlohmann::json(std::string(65, 'a'))}) {
        config.Reset();
        const nlohmann::json input = {{"mc2_trace_config", {{"descid_display", {{"0", "VALID_VALUE"}, {"1", value}}}}}};
        ASSERT_TRUE(WriteFileByStream(path, input.dump()));
        std::string msg;
        EXPECT_FALSE(config.Load(path, msg));
        EXPECT_FALSE(msg.empty());
        EXPECT_TRUE(config.GetMc2TraceConfig().descIdDisplay.empty());
    }
    RemoveAll(path);
    config.Reset();
}

/**
 * | 用例集 | CustomInputConfigTest
 * |测试函数| CustomInputConfig::Load
 * | 用例名 | timeline_merge_config_expect_parsed
 * |用例描述| 测试合法timeline_merge_config按顺序解析，名称及计数边界值被正确保留。
 */
TEST(CustomInputConfigTest, timeline_merge_config_expect_parsed) {
    auto &config = CustomInputConfig::Instance();
    config.Reset();
    const nlohmann::json selections = nlohmann::json::array(
        {{{"kernel_name", "kernel*"}, {"mstx_name", "range1"}, {"rank_count", 2}, {"launch_count", 1}},
            {{"kernel_name", "matmul_0"}, {"mstx_name", "range2"}, {"rank_count", 64}, {"launch_count", 50}}});
    const std::string path = "test/ut/resources/custom_input_timeline_merge.json";
    const nlohmann::json input = {{"timeline_merge_config", selections}};
    ASSERT_TRUE(WriteFileByStream(path, input.dump()));
    std::string msg;
    ASSERT_TRUE(config.Load(path, msg));
    const auto &configs = config.GetTimelineMergeConfigs();
    ASSERT_EQ(configs.size(), 2U);
    EXPECT_EQ(configs[0].kernelName, "kernel*");
    EXPECT_EQ(configs[0].mstxName, "range1");
    EXPECT_EQ(configs[0].rankCount, 2U);
    EXPECT_EQ(configs[0].launchCount, 1U);
    EXPECT_EQ(configs[1].kernelName, "matmul_0");
    EXPECT_EQ(configs[1].mstxName, "range2");
    EXPECT_EQ(configs[1].rankCount, 64U);
    EXPECT_EQ(configs[1].launchCount, 50U);
    RemoveAll(path);
    config.Reset();
}

/**
 * | 用例集 | CustomInputConfigTest
 * |测试函数| CustomInputConfig::Load
 * | 用例名 | timeline_merge_config_invalid_structure_expect_reject
 * |用例描述| 测试timeline_merge_config非数组、元素非对象及超过64条时整份配置失败。
 */
TEST(CustomInputConfigTest, timeline_merge_config_invalid_structure_expect_reject) {
    auto &config = CustomInputConfig::Instance();
    const std::string path = "test/ut/resources/custom_input_merge_structure.json";
    const nlohmann::json valid = {
        {"kernel_name", "kernel*"}, {"mstx_name", "range1"}, {"rank_count", 2}, {"launch_count", 1}};
    const std::vector<nlohmann::json> badInputs = {
        {{"timeline_merge_config", false}},
        {{"timeline_merge_config", nlohmann::json::object()}},
        {{"timeline_merge_config", nlohmann::json::array({1})}},
        {{"timeline_merge_config", nlohmann::json(std::vector<nlohmann::json>(65, valid))}},
    };
    for (const auto &input : badInputs) {
        config.Reset();
        ASSERT_TRUE(WriteFileByStream(path, input.dump()));
        std::string msg;
        EXPECT_FALSE(config.Load(path, msg));
        EXPECT_FALSE(msg.empty());
        EXPECT_TRUE(config.GetTimelineMergeConfigs().empty());
    }
    RemoveAll(path);
    config.Reset();
}

/**
 * | 用例集 | CustomInputConfigTest
 * |测试函数| CustomInputConfig::Load
 * | 用例名 | timeline_merge_config_invalid_field_expect_reject
 * |用例描述| 测试名称空串、超长、非法字符、缺失以及计数越界、非整数时整份配置失败。
 */
TEST(CustomInputConfigTest, timeline_merge_config_invalid_field_expect_reject) {
    auto &config = CustomInputConfig::Instance();
    const std::string path = "test/ut/resources/custom_input_merge_field.json";
    const auto selection = []() {
        return nlohmann::json{
            {"kernel_name", "kernel*"}, {"mstx_name", "range1"}, {"rank_count", 2}, {"launch_count", 1}};
    };
    std::vector<nlohmann::json> badItems;
    for (const std::string field : {"kernel_name", "mstx_name"}) {
        for (const auto &bad : {nlohmann::json(1), nlohmann::json(""), nlohmann::json("bad|name"),
                 nlohmann::json(std::string(1023, 'x'))}) {
            auto item = selection();
            item[field] = bad;
            badItems.push_back(item);
        }
        auto item = selection();
        item.erase(field);
        badItems.push_back(item);
    }
    auto starMstx = selection();
    starMstx["mstx_name"] = "range*";
    badItems.push_back(starMstx);
    for (const std::string field : {"rank_count", "launch_count"}) {
        for (const auto &bad :
            {nlohmann::json(0), nlohmann::json(-1), nlohmann::json(1.5), nlohmann::json("2"), nlohmann::json(true)}) {
            auto item = selection();
            item[field] = bad;
            badItems.push_back(item);
        }
        auto item = selection();
        item.erase(field);
        badItems.push_back(item);
    }
    auto rankTooLarge = selection();
    rankTooLarge["rank_count"] = 65;
    badItems.push_back(rankTooLarge);
    auto rankTooSmall = selection();
    rankTooSmall["rank_count"] = 1;
    badItems.push_back(rankTooSmall);
    auto launchTooLarge = selection();
    launchTooLarge["launch_count"] = 51;
    badItems.push_back(launchTooLarge);
    for (const auto &item : badItems) {
        config.Reset();
        const nlohmann::json input = {{"timeline_merge_config", nlohmann::json::array({item})}};
        ASSERT_TRUE(WriteFileByStream(path, input.dump()));
        std::string msg;
        EXPECT_FALSE(config.Load(path, msg)) << item.dump();
        EXPECT_TRUE(config.GetTimelineMergeConfigs().empty());
    }
    RemoveAll(path);
    config.Reset();
}
