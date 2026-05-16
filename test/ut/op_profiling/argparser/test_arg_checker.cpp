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


#include <cstdlib>
#include <unistd.h>
#include <gtest/gtest.h>
#include "mockcpp/mockcpp.hpp"

#include "filesystem.h"
#include "ascend_helper.h"
#include "common/prof_args.h"
#include "common/hal_helper.h"
#include "filesystem.h"
#define private public
#include "argparser/arg_checker.h"
#undef private

using namespace Common;
using namespace Parser;

class AscendEnvSetter {
public:
    AscendEnvSetter(std::string const &value)
    {
        char const *env = getenv("ASCEND_HOME_PATH");
        origin_ = env == nullptr ? "" : env;
        setenv("ASCEND_HOME_PATH", value.c_str(), 1);
    }
    ~AscendEnvSetter(void)
    {
        setenv("ASCEND_HOME_PATH", origin_.c_str(), 1);
    }
private:
    std::string origin_;
};

TEST(ArgChecker, args_with_onboard_run_mode_expect_check_success)
{
    ProfArgs args;
    args.runMode = "device";
    std::string msg;
    ArgChecker checker("device");

    MOCKER(&Common::HalHelper::GetPlatformType)
        .stubs()
        .will(returnValue(Common::ChipType::ASCEND910B));
    ASSERT_TRUE(checker.CheckRunModeValid(args, msg));
    GlobalMockObject::verify();
}

TEST(ArgChecker, args_with_onboard_run_mode_expect_check_fail)
{
    ProfArgs args;
    args.runMode = "device";
    std::string msg;
    ArgChecker checker("device");

    MOCKER(&Common::HalHelper::GetPlatformType)
        .stubs()
        .will(returnValue(Common::ChipType::END_TYPE));
    ASSERT_FALSE(checker.CheckRunModeValid(args, msg));
    GlobalMockObject::verify();
}

TEST(ArgChecker, args_with_unknown_run_mode_expect_unexpected_run_mode_error)
{
    ProfArgs args;
    args.runMode = "unknown";
    std::string msg;
    ArgChecker checker("unknown");
    ASSERT_FALSE(checker.CheckRunModeValid(args, msg));
    ASSERT_NE(msg.find("unexpected run mode"), std::string::npos);
}

TEST(ArgChecker, args_with_no_ASCEND_HOME_PATH_expect_get_simulators_failed)
{
    ProfArgs args;
    args.runMode = "simulator";
    std::string msg;
    ArgChecker checker("simulator");

    AscendEnvSetter setter("");
    ASSERT_FALSE(checker.CheckRunModeValid(args, msg));
    ASSERT_NE(msg.find("get simulators from ascend path failed"), std::string::npos);
}

TEST(ArgChecker, args_with_no_simulator_path_expect_get_simulators_failed)
{
    ProfArgs args;
    args.runMode = "simulator";
    std::string msg;
    ArgChecker checker("simulator");

    AscendEnvSetter setter("./");
    ASSERT_FALSE(checker.CheckRunModeValid(args, msg));
    ASSERT_NE(msg.find("get simulators from ascend path failed"), std::string::npos);
}

TEST(ArgChecker, args_with_recorgnized_simulator_type_expect_check_success)
{
    ProfArgs args;
    args.runMode = "simulator";
    std::string msg;
    ArgChecker checker("simulator");

    mkdir("/tmp/tools", 0755);
    mkdir("/tmp/tools/simulator", 0755);
    mkdir("/tmp/tools/simulator/Ascend910", 0755);

    AscendEnvSetter setter("/tmp");
    ASSERT_TRUE(checker.CheckRunModeValid(args, msg));

    rmdir("/tmp/tools/simulator/Ascend910");
    rmdir("/tmp/tools/simulator");
    rmdir("/tmp/tools");
}

TEST(ArgChecker, args_with_empty_cmd_expect_no_specific_user_program_error)
{
    ProfArgs args;
    std::string msg;
    ArgChecker checker("");
    ASSERT_FALSE(checker.CheckApplicationValid(args, msg));
    ASSERT_NE(msg.find("Input parameter config, export and application can not be used together or empty at the same time"), std::string::npos);
}

TEST(ArgChecker, args_with_no_application_cmd_expect_error)
{
    ProfArgs args;
    std::string msg;
    ArgChecker checker("");
    args.cmd = {"./app="};
    ASSERT_FALSE(checker.CheckApplicationValid(args, msg));
    ASSERT_FALSE(msg.empty());
}

TEST(ArgChecker, args_with_args_path_and_cmd_expect_can_not_specified_together_error)
{
    ProfArgs args;
    args.argConfig = { "unknown" };
    args.cmd = { "unknown" };
    std::string msg;
    ArgChecker checker("unknown");

    ASSERT_FALSE(checker.CheckApplicationValid(args, msg));
    ASSERT_NE(msg.find("Input parameter config, export and application can not be used together or empty at the same time"), std::string::npos);
}

TEST(ArgChecker, export_with_application_and_confg_expect_false)
{
    mkdir("/tmp/dump1", 0664);
    ProfArgs args;
    args.argConfig = "/tmp/dump1";
    std::string msg;
    ArgChecker checker("");
    args.argExport = "/tmp/dump1";
    ASSERT_FALSE(checker.CheckApplicationValid(args, msg));

    args.argConfig = "";
    args.cmd = {"test"};
    ASSERT_FALSE(checker.CheckApplicationValid(args, msg));
    rmdir("/tmp/dump1");
}

TEST(ArgChecker, args_with_kernel_name_expect_check_failed)
{
    ProfArgs args;
    args.argConfig = { "unknown" };
    args.argKernelName = { "unknown" };
    std::string msg;
    ArgChecker checker("unknown");
    ASSERT_FALSE(checker.CheckKernelNameValid(args, msg));
}

TEST(ArgChecker, application_with_kernel_name_expect_check_success)
{
    ProfArgs args;
    args.argApplication = { "unknown" };
    args.argKernelName = { "unknown" };
    std::string msg;
    ArgChecker checker("unknown");
    ASSERT_TRUE(checker.CheckKernelNameValid(args, msg));
}

TEST(ArgChecker, application_with_invalid_kernel_name_expect_check_failed)
{
    ProfArgs args;
    args.argApplication = { "unknown" };
    args.argKernelName = "a=a";
    std::string msg;
    ArgChecker checker("unknown");
    ASSERT_FALSE(checker.CheckKernelNameValid(args, msg));
}

TEST(ArgChecker, application_with_too_long_kernel_name_expect_check_failed)
{
    ProfArgs args;
    args.argApplication = { "unknown" };
    args.argKernelName = std::string(1024, '|');
    std::string msg;
    ArgChecker checker("unknown");
    ASSERT_FALSE(checker.CheckKernelNameValid(args, msg));
}

TEST(ArgChecker, test_CheckMstxInclude_with_only_mstx_include_expect_check_failed)
{
    ProfArgs args;
    args.argMstxInclude = "message1|message2";
    args.argMstx = "off";
    std::string msg;
    ArgChecker checker("");
    ASSERT_FALSE(checker.CheckMstxInclude(args, msg));
    ASSERT_TRUE(msg.find("--mstx-include only support when --mstx=on") != std::string::npos);
}

TEST(ArgChecker, test_CheckMstxInclude_with_normal_mstx_usage_expect_check_success)
{
    ProfArgs args;
    args.argMstxInclude = "message1|message2";
    args.argMstx = "on";
    std::string msg;
    ArgChecker checker("");
    ASSERT_TRUE(checker.CheckMstxInclude(args, msg));
}

TEST(ArgChecker, test_CheckMstxInclude_with_invalid_mstx_include_expect_check_failed)
{
    ProfArgs args;
    args.argMstxInclude = "message1,message2";
    args.argMstx = "on";
    std::string msg;
    ArgChecker checker("");
    ASSERT_FALSE(checker.CheckMstxInclude(args, msg));
    ASSERT_TRUE(msg.find("Support characters in one message are: A-Z a-z 0-9 _") != std::string::npos);
}

TEST(ArgChecker, test_CheckMstxInclude_with_too_long_kernel_name_expect_check_failed)
{
    ProfArgs args;
    args.argMstx = "on";
    args.argMstxInclude = std::string(1024, '|');
    std::string msg;
    ArgChecker checker("");
    ASSERT_FALSE(checker.CheckMstxInclude(args, msg));
    ASSERT_TRUE(msg.find("--mstx-include input length exceeds limitation") != std::string::npos);
}

TEST(ArgChecker, replay_mode_return_should_be_true)
{
    ProfArgs args;
    args.argReplayMode = "application";
    std::string msg;
    ArgChecker checker("");
    ASSERT_TRUE(checker.CheckReplayMode(args, msg));

    args.argReplayMode = "kernel";
    ASSERT_TRUE(checker.CheckReplayMode(args, msg));

    MOCKER(&Common::HalHelper::GetPlatformType)
            .stubs()
            .will(returnValue(Common::ChipType::ASCEND910B));
    args.argReplayMode = "range";
    args.argMstx = "on";
    ASSERT_TRUE(checker.CheckReplayMode(args, msg));
    GlobalMockObject::verify();
}

TEST(ArgChecker, replay_mode_return_should_be_false)
{
    ProfArgs args;
    args.argReplayMode = "app";
    std::string msg;
    ArgChecker checker("");
    ASSERT_FALSE(checker.CheckReplayMode(args, msg));

    args.argReplayMode = "application";
    args.argAicMetrics.isDeviceToSimulator = true;
    ASSERT_FALSE(checker.CheckReplayMode(args, msg));

    MOCKER(&Common::HalHelper::GetPlatformType)
            .stubs()
            .will(returnValue(Common::ChipType::ASCEND910B));
    args.argReplayMode = "range";
    ASSERT_FALSE(checker.CheckReplayMode(args, msg));

    args.argReplayMode = "range";
    args.argMstx = "on";
    args.argAicMetrics.isDeviceToSimulator = true;
    ASSERT_FALSE(checker.CheckReplayMode(args, msg));

    args.argReplayMode = "range";
    args.argMstx = "on";
    args.argAicMetrics.isDeviceToSimulator = false;
    args.argAicMetrics.isSource = true;
    ASSERT_FALSE(checker.CheckReplayMode(args, msg));

    args.argReplayMode = "range";
    args.argMstx = "on";
    args.argAicMetrics.isDeviceToSimulator = false;
    args.argAicMetrics.isSource = false;
    args.argAicMetrics.isMemoryDetail = true;
    ASSERT_FALSE(checker.CheckReplayMode(args, msg));
    GlobalMockObject::verify();
}

TEST(ArgChecker, config_with_launch_count_check)
{
    ProfArgs args;
    args.cmd = { "unknown" };
    std::string msg;
    ArgChecker checker("unknown");
    // launch count should in range [1, 5000]
    args.argLaunchCount = { "100" };
    ASSERT_TRUE(checker.CheckLaunchCount(args, msg));
    args.argLaunchCount = { "1" };
    ASSERT_TRUE(checker.CheckLaunchCount(args, msg));
    // launch count can not be 0
    args.argLaunchCount = { "0" };
    ASSERT_FALSE(checker.CheckLaunchCount(args, msg));
    // launch count string length cannot more than 4
    args.argLaunchCount = { "11111" };
    ASSERT_FALSE(checker.CheckLaunchCount(args, msg));
    // launch count illegal number
    args.argLaunchCount = { "-1" };
    ASSERT_FALSE(checker.CheckLaunchCount(args, msg));
    args.argLaunchCount = { "5001" };
    ASSERT_FALSE(checker.CheckLaunchCount(args, msg));
    // launch count not a number
    args.argLaunchCount = { "10a" };
    ASSERT_FALSE(checker.CheckLaunchCount(args, msg));
}

TEST(ArgChecker, args_with_launch_skip_before_match_check)
{
    ProfArgs args;
    args.cmd = { "unknown" };
    std::string msg;
    ArgChecker checker("unknown");
    args.argLaunchSkipBeforeMatch = { "0" };
    ASSERT_TRUE(checker.CheckLaunchSkipBeforeMatch(args, msg));
    args.argLaunchSkipBeforeMatch = { "1000" };
    ASSERT_TRUE(checker.CheckLaunchSkipBeforeMatch(args, msg));
    args.argLaunchSkipBeforeMatch = { "-1" };
    ASSERT_FALSE(checker.CheckLaunchSkipBeforeMatch(args, msg));
    args.argLaunchSkipBeforeMatch = { "10a" };
    ASSERT_FALSE(checker.CheckLaunchSkipBeforeMatch(args, msg));
}

TEST(ArgChecker, args_with_kill_check)
{
    ProfArgs args;
    std::string msg;
    ArgChecker checker("");
    // kill should be in on/off or null
    args.argKill = { "on" };
    ASSERT_TRUE(checker.CheckKillAdvance(args, msg));
    args.argKill = { "off" };
    ASSERT_TRUE(checker.CheckKillAdvance(args, msg));
    args.argKill = { "off" };
    ASSERT_TRUE(checker.CheckKillAdvance(args, msg));
    // kill illegal
    args.argKill = { "off1" };
    ASSERT_FALSE(checker.CheckKillAdvance(args, msg));
}

TEST(ArgChecker, args_with_sim_soc_version_check_success)
{
    ProfArgs args;
    std::string msg;
    ArgChecker checker1("device");
    // no need to get simulators
    args.runMode = { "device" };
    ASSERT_TRUE(checker1.CheckSimSocVersion(args, msg));

    ArgChecker checker2("simulator");
    args.argSocVersion = { "" };
    args.runMode = { "simulator" };
    ASSERT_TRUE(checker2.CheckSimSocVersion(args, msg));
}

TEST(ArgChecker, args_with_sim_soc_version_in_chip_product_success)
{
    ProfArgs args;
    std::string msg;
    MOCKER(&Utility::GetAscendHomePath)
        .stubs()
        .will(returnValue(true));
    ArgChecker checker("simulator");
    args.cmd = { "./app" };
    args.runMode = { "simulator" };
    args.argSocVersion = { "Ascend950DT_9573" };
    ASSERT_TRUE(checker.CheckSimSocVersion(args, msg));
    GlobalMockObject::verify();
}

TEST(ArgChecker, args_with_sim_soc_version_get_ascend_home_path_failed)
{
    MOCKER(&Utility::GetAscendHomePath)
        .stubs()
        .will(returnValue(false));
    ProfArgs args;
    std::string msg;
    ArgChecker checker("simulator");
    args.cmd = { "./app" };
    args.runMode = { "simulator" };
    args.argSocVersion = { "Ascend910B1" };
    ASSERT_FALSE(checker.CheckSimSocVersion(args, msg));
    ASSERT_TRUE(msg.find("$ASCEND_HOME_PATH not found") != std::string::npos);
    GlobalMockObject::verify();
}

TEST(ArgChecker, args_with_sim_soc_version_get_file_names_failed)
{
    ProfArgs args;
    std::string msg;
    ArgChecker checker("simulator");
    args.cmd = { "./app" };
    args.runMode = { "simulator" };
    args.argSocVersion = { "Ascend910B1" };
    MOCKER(&Utility::GetAscendHomePath)
        .stubs()
        .will(returnValue(true));
    ASSERT_FALSE(checker.CheckSimSocVersion(args, msg));
    ASSERT_TRUE(msg.find("get simulator failed") != std::string::npos);
    GlobalMockObject::verify();
}

TEST(ArgChecker, args_with_sim_soc_version_str_failed)
{
    ProfArgs args;
    std::string msg;
    ArgChecker checker("simulator");
    args.cmd = { "./app" };
    args.runMode = { "simulator" };
    args.argSocVersion = { "Ascend910xxx" };
    MOCKER(&Utility::GetAscendHomePath)
        .stubs()
        .will(returnValue(true));
    MOCKER(&Utility::GetFileNames)
        .stubs()
        .will(returnValue(true));
    ASSERT_FALSE(checker.CheckSimSocVersion(args, msg));
    ASSERT_TRUE(msg.find("--soc-version is invalid") != std::string::npos);
    GlobalMockObject::verify();
}

TEST(ArgChecker, args_with_core_id_expect_failed)
{
    ArgChecker checker("");
    Common::ProfArgs args;
    std::string msg;
    // test too long input
    auto str = std::string(201, '1');
    args.argCoreId = str;
    ASSERT_FALSE(checker.CheckCoreId(args, msg));
    ASSERT_TRUE(msg.find("--core-id input length exceeds limitation") != std::string::npos);
    // test StringToNum failed
    args.argCoreId = "12a";
    ASSERT_FALSE(checker.CheckCoreId(args, msg));
    ASSERT_TRUE(msg.find("should be an integer which within [0, 49]") != std::string::npos);
    // test core id smaller than 0
    args.argCoreId = "-1";
    ASSERT_FALSE(checker.CheckCoreId(args, msg));
    ASSERT_TRUE(msg.find("should be an integer which within [0, 49]") != std::string::npos);
    // test core id larger than 49
    args.argCoreId = "50";
    ASSERT_FALSE(checker.CheckCoreId(args, msg));
    ASSERT_TRUE(msg.find("should be an integer which within [0, 49]") != std::string::npos);
}

TEST(ArgChecker, args_with_core_id_expect_success)
{
    ArgChecker checker("");
    Common::ProfArgs args;
    std::string msg;
    // test empty input
    args.argCoreId = "";
    ASSERT_TRUE(checker.CheckCoreId(args, msg));
    // test normal input
    args.argCoreId = "0|2|49";
    ASSERT_TRUE(checker.CheckCoreId(args, msg));
}

TEST(ArgChecker, args_with_timeout)
{
    ArgChecker checker("simulator");
    Common::ProfArgs args;
    std::string msg;
    // test empty input
    args.argTimeout = "";
    ASSERT_TRUE(checker.CheckTimeout(args, msg));
    // test normal input
    args.argTimeout = "2880";
    ASSERT_TRUE(checker.CheckTimeout(args, msg));
    // test StringToNum failed
    args.argTimeout = "abc";
    ASSERT_FALSE(checker.CheckTimeout(args, msg));
    // test timeout smaller than 1
    args.argTimeout = "0";
    ASSERT_FALSE(checker.CheckTimeout(args, msg));
    // test timeout larger than 2880
    args.argTimeout = "2881";
    ASSERT_FALSE(checker.CheckTimeout(args, msg));
}

TEST(ArgChecker, mstx_args_check)
{
    ArgChecker checker("");
    Common::ProfArgs args;
    std::string msg;
    // test empty input
    args.argMstx = "";
    ASSERT_TRUE(checker.CheckMstx(args, msg));
    // test correct input
    args.argMstx = "on";
    ASSERT_TRUE(checker.CheckMstx(args, msg));
    args.argMstx = "off";
    ASSERT_TRUE(checker.CheckMstx(args, msg));

    // test error input
    args.argMstx = "ofn";
    ASSERT_FALSE(checker.CheckMstx(args, msg));
    ASSERT_TRUE(msg.find("--mstx should use on/off") != std::string::npos);
}

TEST(ArgChecker, warm_up_args_check)
{
    ArgChecker checker("");
    Common::ProfArgs args;
    std::string msg;
    // test Non-numeric input
    args.argWarmUp = "aa";
    ASSERT_FALSE(checker.CheckWarmUp(args, msg));
    ASSERT_TRUE(msg.find("should be number and within [0, 500]") != std::string::npos);
    // test numeric input
    args.argWarmUp = "400";
    ASSERT_TRUE(checker.CheckWarmUp(args, msg));

    // test Inputs that exceed the maximum value
    args.argWarmUp = "501";
    ASSERT_FALSE(checker.CheckWarmUp(args, msg));
    ASSERT_TRUE(msg.find("should within [0, 500]") != std::string::npos);
    // test inputs with a negative value
    args.argWarmUp = "-4294967294";
    ASSERT_FALSE(checker.CheckWarmUp(args, msg));
    ASSERT_TRUE(msg.find("should be number and within [0, 500]") != std::string::npos);
}

/**
* |  用例集  | ArgChecker
* | 测试函数 | ArgChecker
* |  用例名  | test_CheckExportPathValid_expect_return_true
* | 用例描述 | 测试不输入或输入路径权限正确CheckExportPathValid成功
*/
TEST(ArgChecker, test_CheckExportPathValid_expect_return_true)
{
    ProfArgs args;
    args.runMode = "simulator";
    std::string msg;
    ArgChecker checker("simulator");
    // 不输入
    args.argExport = "";
    ASSERT_TRUE(checker.CheckExportPathValid(args, msg));
    // 输入路径权限正确
    args.argExport = "test/ut/resources/op_profiling/simulator_sample/dump";
    ASSERT_TRUE(checker.CheckExportPathValid(args, msg));
}

/**
* |  用例集  | ArgChecker
* | 测试函数 | ArgChecker
* |  用例名  | test_CheckExportPathValid_expect_return_false
* | 用例描述 | 测试不输入或输入路径权限正确CheckExportPathValid失败
*/
TEST(ArgChecker, test_CheckExportPathValid_expect_return_false)
{
    ProfArgs args;
    args.runMode = "simulator";
    std::string msg;
    ArgChecker checker("simulator");
    // 输入路径没有写权限
    args.argExport = "test/ut/resources/op_profiling/simulator_sample/dump";
    MOCKER(&Utility::CheckPermission)
        .stubs()
        .will(returnValue(false));
    ASSERT_FALSE(checker.CheckExportPathValid(args, msg));
    ASSERT_TRUE(msg.find("cannot have write permission") != std::string::npos);
    // 输入路径父目录权限问题
    MOCKER(&Utility::CheckInputFileValid)
        .stubs()
        .will(returnValue(false));
    ASSERT_FALSE(checker.CheckExportPathValid(args, msg));
    ASSERT_TRUE(msg.find("parent dir permission wrong") != std::string::npos);
    GlobalMockObject::verify();
}

/**
* |  用例集  | ArgChecker
* | 测试函数 | ArgChecker
* |  用例名  | test_CheckOutputPathValid_expect_return_true
* | 用例描述 | 测试输出路径权限正确CheckOutputPathValid成功
*/
TEST(ArgChecker, test_CheckOutputPathValid_expect_return_true)
{
    mkdir("/tmp/dump1", 0640);
    ProfArgs args;
    args.runMode = "device";
    std::string msg;
    ArgChecker checker("device");
    args.argOutput = "/tmp/dump1";
    ASSERT_TRUE(checker.CheckOutputPathValid(args, msg));
    rmdir("/tmp/dump1");
}

/**
* |  用例集  | ArgChecker
* | 测试函数 | ArgChecker
* |  用例名  | test_CheckOutputPathValid_expect_return_false
* | 用例描述 | 测试输出路径非文件夹、权限问题、属主问题等CheckOutputPathValid失败
*/
TEST(ArgChecker, test_CheckOutputPathValid_expect_return_false)
{
    mkdir("/tmp/dump1", 0400);
    ProfArgs args;
    args.runMode = "device";
    std::string msg;
    ArgChecker checker("device");
    args.argOutput = "/tmp/dump1";
    // 输出路径没有写权限
    ASSERT_FALSE(checker.CheckOutputPathValid(args, msg));
    ASSERT_TRUE(msg.find("Output dir is not writable") != std::string::npos);
    // 输出路径属主问题
    MOCKER(&Utility::CheckOwnerPermission)
        .stubs()
        .will(returnValue(false));
    ASSERT_FALSE(checker.CheckOutputPathValid(args, msg));
    // 输出路径长度问题
    MOCKER(&Utility::PathLenCheckValid)
        .stubs()
        .will(returnValue(false));
    ASSERT_FALSE(checker.CheckOutputPathValid(args, msg));
    ASSERT_TRUE(msg.find("--output parameter length is larger than 200") != std::string::npos);
    // 输出路径字符不合规
    MOCKER(&Utility::IsStringCharValid)
        .stubs()
        .will(returnValue(false));
    ASSERT_FALSE(checker.CheckOutputPathValid(args, msg));
    ASSERT_TRUE(msg.find("--output parameter contains") != std::string::npos);
    // 输出路径存在但非文件夹
    MOCKER(&Utility::IsDir)
        .stubs()
        .will(returnValue(false));
    ASSERT_FALSE(checker.CheckOutputPathValid(args, msg));
    ASSERT_TRUE(msg.find("--output parameter is not a folder but already exist") != std::string::npos);
    rmdir("/tmp/dump1");
    GlobalMockObject::verify();
}

/**
* |  用例集  | ArgChecker
* | 测试函数 | ArgChecker
* |  用例名  | test_ArgChecker_size_expect_return_true
* | 用例描述 | 测试不同场景下按照runmode分类的argcheck的size大小满足预期
*/
TEST(ArgChecker, test_ArgChecker_size_expect_return_true)
{
    ArgChecker checker1("device");
    ASSERT_TRUE(checker1.checkers_.size() == 14);

    ArgChecker checker2("simulator");
    ASSERT_TRUE(checker2.checkers_.size() == 13);

    ArgChecker checker3("");
    ASSERT_TRUE(checker3.checkers_.size() == 14);
}

/**
* |  用例集  | ArgChecker
* | 测试函数 | CheckAicMetrics
* |  用例名  | test_CheckAicMetrics_device_platform_get_failed_expect_return_false
* | 用例描述 | 测试device场景芯片型号获取失败，CheckAicMetrics失败
*/
TEST(ArgChecker, test_CheckAicMetrics_device_platform_get_failed_expect_return_false)
{
    ArgChecker checker("");
    Common::ProfArgs args;
    args.runMode = "device";
    std::string msg;
    MOCKER(&Common::HalHelper::GetPlatformType)
        .stubs()
        .will(returnValue(Common::ChipType::END_TYPE));
    ASSERT_FALSE(checker.CheckAicMetrics(args, msg));
    ASSERT_TRUE(msg.find("not support") != std::string::npos);
    GlobalMockObject::verify();
}

/**
* |  用例集  | ArgChecker
* | 测试函数 | CheckAicMetrics
* |  用例名  | test_CheckAicMetrics_device_metric_failed_expect_return_false
* | 用例描述 | 测试device场景metric校验失败，CheckAicMetrics失败
*/
TEST(ArgChecker, test_CheckAicMetrics_device_metric_failed_expect_return_false)
{
    ArgChecker checker("");
    Common::ProfArgs args;
    args.runMode = "device";
    std::string msg;
    MOCKER(&Common::HalHelper::GetPlatformType)
        .stubs()
        .will(returnValue(Common::ChipType::ASCEND910B));
    // 测试错误metric
    args.argAicMetrics.metricVec = {{"test"}};
    ASSERT_FALSE(checker.CheckAicMetrics(args, msg));
    ASSERT_TRUE(msg.find("maybe in wrong run mode") != std::string::npos);
    // 测试芯片型号不支持的metric
    args.argAicMetrics.metricVec = {{"pipetimeline"}};
    ASSERT_FALSE(checker.CheckAicMetrics(args, msg));
    ASSERT_TRUE(msg.find("maybe in wrong soc platform") != std::string::npos);
    GlobalMockObject::verify();
}

/**
* |  用例集  | ArgChecker
* | 测试函数 | CheckAicMetrics
* |  用例名  | test_CheckAicMetrics_device_expect_return_true
* | 用例描述 | 测试device场景CheckAicMetrics成功
*/
TEST(ArgChecker, test_CheckAicMetrics_device_expect_return_true)
{
    ArgChecker checker("");
    Common::ProfArgs args;
    args.runMode = "device";
    std::string msg;
    args.argAicMetrics.metricVec = {{"pipetimeline"}};
    MOCKER(&Common::HalHelper::GetPlatformType)
        .stubs()
        .will(returnValue(Common::ChipType::ASCEND950));
    ASSERT_TRUE(checker.CheckAicMetrics(args, msg));
    GlobalMockObject::verify();
}

/**
* |  用例集  | ArgChecker
* | 测试函数 | CheckAicMetrics
* |  用例名  | test_CheckAicMetrics_sim_expect_return_false
* | 用例描述 | 测试simulator场景CheckAicMetrics失败
*/
TEST(ArgChecker, test_CheckAicMetrics_sim_expect_return_false)
{
    ArgChecker checker("");
    Common::ProfArgs args;
    args.runMode = "simulator";
    std::string msg;
    args.argSocVersion = "Ascend310P3";
    // 测试错误metric
    args.argAicMetrics.metricVec = {{"test"}};
    ASSERT_FALSE(checker.CheckAicMetrics(args, msg));
    ASSERT_TRUE(msg.find("maybe in wrong run mode") != std::string::npos);
    // 测试芯片型号不支持的metric
    args.argAicMetrics.metricVec = {{"pmsampling"}};
    ASSERT_FALSE(checker.CheckAicMetrics(args, msg));
    ASSERT_TRUE(msg.find("maybe in wrong soc platform") != std::string::npos);
}

/**
* |  用例集  | ArgChecker
* | 测试函数 | CheckAicMetrics
* |  用例名  | test_CheckAicMetrics_sim_expect_return_true
* | 用例描述 | 测试simulator场景CheckAicMetrics成功
*/
TEST(ArgChecker, test_CheckAicMetrics_sim_expect_return_true)
{
    ArgChecker checker("");
    Common::ProfArgs args;
    args.runMode = "simulator";
    std::string msg;
    args.argAicMetrics.metricVec = {{"pipeutilization"}};
    ASSERT_TRUE(checker.CheckAicMetrics(args, msg));
}

/**
* |  用例集  | ArgChecker
* | 测试函数 | CheckDump
* |  用例名  | test_CheckDump_expect_return_false
* | 用例描述 | 测试异常输入CheckDump失败
*/
TEST(ArgChecker, test_CheckDump_expect_return_false)
{
    ArgChecker checker("simulator");
    Common::ProfArgs args;
    std::string msg;
    args.runMode = "simulator";
    args.argDump = "test";
    ASSERT_FALSE(checker.CheckDump(args, msg));
    ASSERT_TRUE(msg.find("--dump should be on/off") != std::string::npos);
}

/**
* |  用例集  | ArgChecker
* | 测试函数 | CheckDump
* |  用例名  | test_CheckDump_expect_return_true
* | 用例描述 | 测试输入on/off CheckDump均成功
*/
TEST(ArgChecker, test_CheckDump_expect_return_true)
{
    ArgChecker checker("simulator");
    Common::ProfArgs args;
    std::string msg;
    args.runMode = "simulator";
    args.argDump = "on";
    ASSERT_TRUE(checker.CheckDump(args, msg));
    args.argDump = "off";
    ASSERT_TRUE(checker.CheckDump(args, msg));
}
