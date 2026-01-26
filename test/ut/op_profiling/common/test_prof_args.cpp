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
#include <functional>

#include "argparser/options.h"
#include "common/prof_args.h"
#include "common/hal_helper.h"

using namespace Common;

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
    pmuEventsId.LoadPmuVec(metrics, Common::ChipType::ASCEND910_95, "application");
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