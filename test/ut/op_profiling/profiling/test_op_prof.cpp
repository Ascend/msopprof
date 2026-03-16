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
#include "mockcpp/mockcpp.hpp"

#include "common/prof_args.h"
#include "profiling/simulator/op_sim_prof.h"
#include "op_runner.h"
#include "ascend_helper.h"
#include "profiling/device/data_parse/device_defs.h"
#include "profiling/device/data_parse/equation_utils.h"
using namespace Profiling;

TEST(OpProf, Run_with_OpSimProf_expect_success)
{
    GlobalMockObject::verify();
    MOCKER(&OpSimProf::RunTask)
        .stubs()
        .will(returnValue(true));
    MOCKER(&OpSimProf::RunDataParse)
        .stubs()
        .will(returnValue(true));

    Common::ProfArgs runConfig;
    OpSimProf opSimProf(runConfig);
    ASSERT_TRUE(opSimProf.Run());
}

TEST(OpProf, Run_with_OpSimProf_expect_failed)
{
    GlobalMockObject::verify();
    MOCKER(&OpSimProf::RunTask)
        .stubs()
        .will(returnValue(false));
    MOCKER(&OpSimProf::RunDataParse)
        .stubs()
        .will(returnValue(false));

    Common::ProfArgs runConfig;
    OpSimProf opSimProf(runConfig);
    ASSERT_FALSE(opSimProf.Run());
    ASSERT_FALSE(opSimProf.Run());
}

TEST(OpProf, Run_with_OpSimProf_DataParse_expect_failed)
{
    GlobalMockObject::verify();
    MOCKER(&OpSimProf::RunTask)
        .stubs()
        .will(returnValue(true));
    MOCKER(&OpSimProf::RunDataParse)
        .stubs()
        .will(returnValue(false));

    Common::ProfArgs runConfig;
    OpSimProf opSimProf(runConfig);;
    ASSERT_FALSE(opSimProf.Run());
    ASSERT_FALSE(opSimProf.Run());
}

TEST(OpProf, RunTask_with_OpSimProf_expect_failed)
{
    GlobalMockObject::verify();
    Common::ProfArgs runConfig;
    OpSimProf opSimProf(runConfig);;
    MOCKER(&OpRunner::RunOpBinary)
       .stubs()
       .will(returnValue(false));
    ASSERT_FALSE(opSimProf.RunTask());
}

TEST(OpProf, RunDataParse_with_OpSimProf_expect_failed)
{
    GlobalMockObject::verify();
    MOCKER(&Utility::GetAscendHomePath)
        .stubs()
        .will(returnValue(false));
    Common::ProfArgs runConfig;
    OpSimProf opSimProf(runConfig);
    ASSERT_FALSE(opSimProf.RunDataParse());
    const char *val = "test/ut/resources";
    setenv("ASCEND_HOME_PATH", val, 1);
    ASSERT_FALSE(opSimProf.RunDataParse());
    unsetenv("ASCEND_HOME_PATH");
}

/**
/* | 用例集 | OpProf
/* |测试函数| BandWidthUsage
/* | 用例名 | test_band_width_usage_when_duration_or_maxbw_is_zero_and_expect_no_throw
/* |用例描述| 执行测试函数，当duration或者maxbw为0时，不抛出异常
*/
TEST(OpProf, test_band_width_usage_when_duration_or_maxbw_is_zero_and_expect_no_throw)
{
    GlobalMockObject::verify();
    ASSERT_NO_THROW(BandWidthUsage(10.0, 0.0, TransportType::GM_TO_UB, ChipProductType::ASCEND910B_SERIES));
    float num = 0.0f;
    MOCKER(&GetMaxBandWidthByType)
        .stubs()
        .with(any(), any(), outBound(num))
        .will(returnValue(true));
    ASSERT_NO_THROW(BandWidthUsage(10.0, 1.1, TransportType::GM_TO_UB, ChipProductType::ASCEND910B_SERIES));
    GlobalMockObject::verify();
}