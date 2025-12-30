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

