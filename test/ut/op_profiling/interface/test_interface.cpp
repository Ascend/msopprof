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

#include <dlfcn.h>
#include "argparser/arg_checker.h"
#include "interface/ms_op_prof.h"
#include "profiling/op_prof.h"
#include "common/hal_helper.h"

using namespace Interface;

TEST(Interface, args_init_called_expect_return_false)
{
    Common::ProfArgs args;
    bool ret = ProfArgsInit(args, 0, nullptr, nullptr);
    ASSERT_EQ(ret, false);
}

TEST(Interface, print_device_help_called_expect_return_true)
{
    Common::ProfArgs args;
    char *argv[2];
    argv[0] = "msopprof";
    argv[1] = "--help";
    bool ret = ProfArgsInit(args, 2, argv, nullptr);
    PrintDeviceHelp(Common::ChipType::END_TYPE);
    ASSERT_EQ(ret, true);
}

TEST(Interface, print_simulator_help_called_expect_return_true)
{
    Common::ProfArgs args;
    char *argv[3];
    argv[0] = "msopprof";
    argv[1] = "simulator";
    argv[2] = "--help";
    bool ret = ProfArgsInit(args, 3, argv, nullptr);
    PrintSimulatorHelp();
    ASSERT_EQ(ret, true);
}

TEST(Interface, args_init_with_invalid_param_expect_return_false)
{
    Common::ProfArgs args;
    char *argv[2];
    argv[0] = "msopprof";
    argv[1] = "--ops";
    bool ret = ProfArgsInit(args, 2, argv, nullptr);
    ASSERT_FALSE(ret);
}

TEST(Interface, PlatformInit_failed)
{
    GlobalMockObject::verify();
    std::string so = "libascend_hal.so";
    void *open = &so;
    MOCKER(&dlopen)
            .stubs()
            .will(returnValue(open));
    void *sym = nullptr;
    MOCKER(&dlsym)
            .stubs()
            .will(returnValue(sym));
    Common::ProfArgs args;
    char *argv[3];
    argv[0] = "msopprof";
    argv[1] = "--application=./npu";
    argv[2] = "--output=./output";
    bool ret = ProfArgsInit(args, 3, argv, nullptr);
    ASSERT_EQ(ret, false);
    GlobalMockObject::verify();
}

TEST(Interface, ProfilingRun_expect_success)
{
    GlobalMockObject::verify();
    MOCKER(&Profiling::OpProf::RunTask)
        .stubs()
        .will(returnValue(true));
    MOCKER(&Profiling::OpProf::RunDataParse)
            .stubs()
            .will(returnValue(true));
    MOCKER(&Common::HalHelper::GetPlatformType)
        .stubs()
        .will(returnValue(Common::ChipType::ASCEND910B));

    Common::ProfArgs args;
    ASSERT_TRUE(ProfilingRun(args));
    args.runMode = "sim";
    ASSERT_TRUE(ProfilingRun(args));
}

TEST(Interface, ConfigInit_not_exist_json_parse_failed)
{
    Common::ProfArgs args;
    char *argv[2];
    argv[0] = "msopprof";
    argv[1] = "--config=./not_exist.json";
    bool ret = ProfArgsInit(args, 2, argv, nullptr);
    ASSERT_FALSE(ret);
}

TEST(Interface, IsProcessRunning)
{
    Common::ProfArgs args;
    SetExitMode();
    Profiling::DataParse::inExitMode = false;
    ASSERT_FALSE(IsProcessRunning());
}
