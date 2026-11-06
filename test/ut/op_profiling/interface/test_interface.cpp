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
#include <sstream>
#include <streambuf>
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

TEST(Interface, deprecated_version_option_warns_and_sets_print_version)
{
    char program[] = "msopprof";
    char simulator[] = "simulator";
    char version[] = "-v";
    char *deviceArgv[] = {program, version};
    char *simulatorArgv[] = {program, simulator, version};
    Common::ProfArgs deviceArgs;
    Common::ProfArgs simulatorArgs;

    testing::internal::CaptureStderr();
    bool deviceRet = ProfArgsInit(deviceArgs, 2, deviceArgv, nullptr);
    std::string deviceWarning = testing::internal::GetCapturedStderr();
    testing::internal::CaptureStderr();
    bool simulatorRet = ProfArgsInit(simulatorArgs, 3, simulatorArgv, nullptr);
    std::string simulatorWarning = testing::internal::GetCapturedStderr();

    ASSERT_TRUE(deviceRet);
    ASSERT_TRUE(simulatorRet);
    ASSERT_TRUE(deviceArgs.printVersion);
    ASSERT_TRUE(simulatorArgs.printVersion);
    ASSERT_EQ(deviceWarning, "WARNING: '-v' is deprecated; use '-V' instead.\n");
    ASSERT_EQ(simulatorWarning, "WARNING: '-v' is deprecated; use '-V' instead.\n");
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

TEST(Interface, args_init_with_device_dump_on_ascend950_with_timeline_detail_expect_return_false) {
    GlobalMockObject::verify();
    MOCKER(&Common::HalHelper::GetPlatformType).stubs().will(returnValue(Common::ChipType::ASCEND950));
    Common::ProfArgs args;
    char *argv[5];
    argv[0] = "msopprof";
    argv[1] = "--application=/bin/true";
    argv[2] = "--aic-metrics=TimelineDetail";
    argv[3] = "--dump=on";
    argv[4] = "--output=./output";
    bool ret = ProfArgsInit(args, 5, argv, nullptr);
    ASSERT_FALSE(ret);
    GlobalMockObject::verify();
}

TEST(Interface, args_init_with_device_core_id_on_ascend950_without_timeline_detail_expect_return_false) {
    GlobalMockObject::verify();
    MOCKER(&Common::HalHelper::GetPlatformType).stubs().will(returnValue(Common::ChipType::ASCEND950));
    Common::ProfArgs args;
    char *argv[4];
    argv[0] = "msopprof";
    argv[1] = "--application=/bin/true";
    argv[2] = "--core-id=0";
    argv[3] = "--output=./output";
    bool ret = ProfArgsInit(args, 4, argv, nullptr);
    ASSERT_FALSE(ret);
    GlobalMockObject::verify();
}

TEST(Interface, args_init_with_device_core_id_on_ascend950_with_timeline_detail_expect_return_true) {
    GlobalMockObject::verify();
    MOCKER(&Common::HalHelper::GetPlatformType).stubs().will(returnValue(Common::ChipType::ASCEND950));
    Common::ProfArgs args;
    char *argv[5];
    argv[0] = "msopprof";
    argv[1] = "--application=/bin/true";
    argv[2] = "--aic-metrics=TimelineDetail,Default";
    argv[3] = "--core-id=0";
    argv[4] = "--output=./output";
    bool ret = ProfArgsInit(args, 5, argv, nullptr);
    ASSERT_TRUE(ret);
    GlobalMockObject::verify();
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

TEST(Interface, args_init_with_list_sets_expect_return_true)
{
    Common::ProfArgs args;
    char *argv[2];
    argv[0] = "msopprof";
 	argv[1] = "--list-sets";
    bool ret = ProfArgsInit(args, 2, argv, nullptr);
    ASSERT_TRUE(ret);
    ASSERT_TRUE(args.listSets);
}

TEST(Interface, args_init_with_list_sets_and_simulator_expect_parse_error)
{
    Common::ProfArgs args;
    char *argv[3];
    argv[0] = "msopprof";
    argv[1] = "simulator";
    argv[2] = "--list-sets";
    bool ret = ProfArgsInit(args, 3, argv, nullptr);
    ASSERT_FALSE(ret);
}

TEST(Interface, args_init_with_set_and_aic_metrics_expect_error)
{
    GlobalMockObject::verify();
    Common::ProfArgs args;
    char *argv[4];
    argv[0] = "msopprof";
    argv[1] = "--set=default";
    argv[2] = "--aic-metrics=Memory";
    argv[3] = "--application=/bin/true";
    bool ret = ProfArgsInit(args, 4, argv, nullptr);
    ASSERT_FALSE(ret);
    GlobalMockObject::verify();
}

TEST(Interface, args_init_with_set_full_and_range_replay_expect_error)
{
    GlobalMockObject::verify();
    MOCKER(&Common::HalHelper::GetPlatformType).stubs().will(returnValue(Common::ChipType::ASCEND910B));
    Common::ProfArgs args;
    char *argv[5];
    argv[0] = "msopprof";
 	argv[1] = "--set=full";
 	argv[2] = "--replay-mode=range";
    argv[3] = "--mstx=on";
    argv[4] = "--application=/bin/true";
    bool ret = ProfArgsInit(args, 5, argv, nullptr);
    ASSERT_FALSE(ret);
    GlobalMockObject::verify();
}

class CoutRedirect {
public:
    CoutRedirect() : buf_(std::cout.rdbuf())
    {
        std::cout.rdbuf(oss_.rdbuf());
    }
    ~CoutRedirect() { std::cout.rdbuf(buf_); }
    std::string GetOutput() const { return oss_.str(); }
private:
    std::streambuf *buf_;
    std::ostringstream oss_;
};

TEST(Interface, print_list_sets_a2a3_expect_content)
{
    GlobalMockObject::verify();
    MOCKER(&Common::HalHelper::GetPlatformType).stubs().will(returnValue(Common::ChipType::ASCEND910B));
    CoutRedirect redirect;
    PrintListSets();
    std::string output = redirect.GetOutput();
    ASSERT_NE(output.find("Ascend910B (A2/A3)"), std::string::npos);
    ASSERT_EQ(output.find("Ascend950 (A5)"), std::string::npos);
    ASSERT_NE(output.find("default"), std::string::npos);
    ASSERT_NE(output.find("detailed"), std::string::npos);
    ASSERT_NE(output.find("full"), std::string::npos);
    ASSERT_NE(output.find("Default"), std::string::npos);
    ASSERT_NE(output.find("Occupancy"), std::string::npos);
    ASSERT_NE(output.find("Roofline"), std::string::npos);
    ASSERT_NE(output.find("MemoryDetail"), std::string::npos);
    ASSERT_NE(output.find("Source"), std::string::npos);
    ASSERT_NE(output.find("Incompatible with --replay-mode=range"), std::string::npos);
    GlobalMockObject::verify();
}

TEST(Interface, print_list_sets_a5_expect_content)
{
    GlobalMockObject::verify();
    MOCKER(&Common::HalHelper::GetPlatformType).stubs().will(returnValue(Common::ChipType::ASCEND950));
    CoutRedirect redirect;
    PrintListSets();
    std::string output = redirect.GetOutput();
    ASSERT_NE(output.find("Ascend950 (A5)"), std::string::npos);
    ASSERT_EQ(output.find("Ascend910B (A2/A3)"), std::string::npos);
    ASSERT_NE(output.find("PipeTimeline"), std::string::npos);
    ASSERT_NE(output.find("PCSampling"), std::string::npos);
    ASSERT_NE(output.find("InstrTimeline"), std::string::npos);
    ASSERT_NE(output.find("Incompatible with --replay-mode=range"), std::string::npos);
    GlobalMockObject::verify();
}

TEST(Interface, print_list_sets_unknown_chip_expect_show_all)
{
    GlobalMockObject::verify();
    MOCKER(&Common::HalHelper::GetPlatformType).stubs().will(returnValue(Common::ChipType::END_TYPE));
    CoutRedirect redirect;
    PrintListSets();
    std::string output = redirect.GetOutput();
    ASSERT_NE(output.find("Ascend910B (A2/A3)"), std::string::npos);
    ASSERT_NE(output.find("Ascend950 (A5)"), std::string::npos);
    GlobalMockObject::verify();
}
