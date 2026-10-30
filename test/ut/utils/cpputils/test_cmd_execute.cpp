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
#include "cmd_execute.h"
#include "filesystem.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <spawn.h>

using namespace Utility;

TEST(CmdExecute, JoinWithSystemEnv_expect_success) {
    std::vector<std::string> outEnv;
    JoinWithSystemEnv({{"test", "123"}}, outEnv, true);
    ASSERT_FALSE(std::find(outEnv.begin(), outEnv.end(), "test=123") == outEnv.end());
    std::vector<std::string> outEnv2;
    JoinWithSystemEnv({{"test", "123"}}, outEnv2, false);
    ASSERT_FALSE(std::find(outEnv2.begin(), outEnv2.end(), "test=123") == outEnv2.end());
}

TEST(CmdExecute, CmdExecute_expect_success) {
    std::string profilingDir = "test/ut/resources/op_profiling/";
    std::vector<std::string> cmd = {"ls", profilingDir};
    std::map<std::string, std::string> env;
    std::string output;
    ASSERT_TRUE(CmdExecute(cmd, env, output));
}

TEST(CmdExecute, ToRawCArgv_expect_success) {
    std::vector<std::string> const argv = {"test", "aaa"};
    std::vector<char *>  res = {};
    ASSERT_NE(ToRawCArgv(argv), res);
}

/**
 * |  用例集  | CmdExecute
 * | 测试函数 | JoinWithSystemEnv
 * |  用例名  | test_JoinWithSystemEnv_with_systemEnvs_return_without_equal_sign_expect_success
 * | 用例描述 | 测试当系统变量environ返回的结果异常，字符串存在没有=号的字符串时，函数会跳过盖变量并正常运行
 */
TEST(CmdExecute, test_JoinWithSystemEnv_with_systemEnvs_return_without_equal_sign_expect_success) {
    char *testSystemEnvs[] = {
            (char*)"LDtest",
            (char*)"LD_PRELOAD=345",
            nullptr
    };
    std::vector<std::string> outEnv;
    char **originSystemEnvs = environ;
    environ = testSystemEnvs;
    JoinWithSystemEnv({{"LD_PRELOAD", "123"}}, outEnv, true);
    ASSERT_FALSE(std::find(outEnv.begin(), outEnv.end(), "LD_PRELOAD=123:345") == outEnv.end());
    environ = originSystemEnvs;
}

// 覆盖 JoinWithSystemEnv 在待合并的系统环境变量值为空时直接替换（不追加冒号）的分支
TEST(CmdExecute, test_JoinWithSystemEnv_with_empty_env_value_expect_replace) {
    char *testSystemEnvs[] = {
            (char*)"LD_PRELOAD=",
            nullptr
    };
    std::vector<std::string> outEnv;
    char **originSystemEnvs = environ;
    environ = testSystemEnvs;
    JoinWithSystemEnv({{"LD_PRELOAD", "123"}}, outEnv, true);
    ASSERT_FALSE(std::find(outEnv.begin(), outEnv.end(), "LD_PRELOAD=123") == outEnv.end());
    environ = originSystemEnvs;
}

// 覆盖 CmdExecuteWithOutput 正常执行命令成功的主流程
TEST(CmdExecute, test_CmdExecuteWithOutput_expect_success) {
    std::string outputName = "/tmp/msopprof_cmd_execute_touch_xyz";
    std::vector<std::string> cmd = {"touch"};
    ASSERT_TRUE(CmdExecuteWithOutput(cmd, outputName));
    ASSERT_TRUE(IsExist(outputName));
    std::remove(outputName.c_str());
}

// 覆盖 CmdExecuteWithOutput 在子进程退出码非 0 时返回 false 的分支
TEST(CmdExecute, test_CmdExecuteWithOutput_with_failing_command_expect_false) {
    std::string outputName = "extra_arg";
    std::vector<std::string> cmd = {"false"};
    ASSERT_FALSE(CmdExecuteWithOutput(cmd, outputName));
}

// 覆盖 CmdExecuteWithOutput 在 posix_spawnp 失败时返回 false 的分支
TEST(CmdExecute, test_CmdExecuteWithOutput_with_spawn_failure_expect_false) {
    GlobalMockObject::verify();
    MOCKER(&posix_spawnp)
        .stubs()
        .will(returnValue(ENOENT));
    std::string outputName = "extra_arg";
    std::vector<std::string> cmd = {"any_cmd"};
    ASSERT_FALSE(CmdExecuteWithOutput(cmd, outputName));
    GlobalMockObject::verify();
}

// 覆盖 CmdExecute 在命令为空时返回 false 的分支
TEST(CmdExecute, test_CmdExecute_with_empty_command_expect_false) {
    std::vector<std::string> cmd;
    std::map<std::string, std::string> env;
    std::string output;
    ASSERT_FALSE(CmdExecute(cmd, env, output));
}

// 覆盖 CmdExecute 在 fork 失败时关闭管道并返回 false 的分支
TEST(CmdExecute, test_CmdExecute_with_fork_failure_expect_false) {
    GlobalMockObject::verify();
    MOCKER(&fork)
        .stubs()
        .will(returnValue(static_cast<pid_t>(-1)));
    std::vector<std::string> cmd = {"ls"};
    std::map<std::string, std::string> env;
    std::string output;
    ASSERT_FALSE(CmdExecute(cmd, env, output));
    GlobalMockObject::verify();
}

// 覆盖 CmdExecute 在子进程被信号杀死时返回 false 的分支
TEST(CmdExecute, test_CmdExecute_with_signaled_child_expect_false) {
    std::vector<std::string> cmd = {"sh", "-c", "kill -TERM $$"};
    std::map<std::string, std::string> env;
    std::string output;
    ASSERT_FALSE(CmdExecute(cmd, env, output));
}

