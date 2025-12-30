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

#include <string>
#include <sys/wait.h>
#include "op_runner.h"

#define private public
#include "runner_impl/exec_binary_runner.h"
#undef private
#include "filesystem.h"
#include "profiling/op_prof_task.h"
#include "profiling/op_prof_data_parse.h"

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

using namespace OpRunner;
using namespace Utility;
using namespace std;

/**
/* | 用例集 | OpRunner
/* |测试函数| RunOpBinary
/* | 用例名 | exec_binary_runner_normal_expect_success
/* |用例描述| 执行测试函数，输入正常时，结果不抛出异常
*/
TEST(OpRunner, exec_binary_runner_normal_expect_success) {
    std::vector<std::string> cmd = {"ls", "-l"};
    std::map<std::string, std::string> envs;
    envs["LD_LIBRARY_PATH"] = "VALUE1";
    envs["KEY2"] = "VALUE2";
    EXPECT_NO_THROW(RunOpBinary(cmd, envs));
}

/**
/* | 用例集 | OpRunner
/* |测试函数| ExecBinaryRunner::Run
/* | 用例名 | test_exec_binary_runner_run_given_executeCmd_is_empty_then_return_false
/* |用例描述| 执行测试函数，输入cmd为空时，返回false
*/
TEST(OpRunner, test_exec_binary_runner_run_given_executeCmd_is_empty_then_return_false)
{
    ExecBinaryRunner runner;
    std::vector<std::string> cmd = {};
    std::map<std::string, std::string> envs;
    envs["LD_LIBRARY_PATH"] = "VALUE1";
    EXPECT_FALSE(runner.Run(cmd, envs));
}

/**
/* | 用例集 | OpRunner
/* |测试函数| ExecBinaryRunner::Run
/* | 用例名 | test_exec_binary_runner_run_when_child_process_in_exit_mode_then_return_true
/* |用例描述| 执行测试函数，fork成功后，子进程收到消息正常退出，返回true
*/
TEST(OpRunner, test_exec_binary_runner_run_when_child_process_in_exit_mode_then_return_true)
{
    ExecBinaryRunner runner;
    Profiling::Task::inExitMode = true;
    std::vector<std::string> cmd = {"ls"};
    std::map<std::string, std::string> envs;
    envs["LD_LIBRARY_PATH"] = "VALUE1";
    int status = 1;
    MOCKER(&fork)
        .stubs()
        .will(returnValue(pid_t(1)));
    MOCKER(&waitpid)
        .stubs()
        .with(any(), outBoundP(&status, sizeof(status)), any())
        .will(returnValue(__pid_t(0)));
    EXPECT_TRUE(runner.Run(cmd, envs));
    Profiling::Task::inExitMode = false;
    GlobalMockObject::verify();
}

/**
/* | 用例集 | OpRunner
/* |测试函数| ExecBinaryRunner::Run
/* | 用例名 | test_exec_binary_runner_run_when_child_process_status_abnormal_then_return_false
/* |用例描述| 执行测试函数，fork成功后，子进程异常退出，返回false
*/
TEST(OpRunner, test_exec_binary_runner_run_when_child_process_status_abnormal_then_return_false)
{
    ExecBinaryRunner runner;
    std::vector<std::string> cmd = {"ls"};
    std::map<std::string, std::string> envs;
    envs["LD_LIBRARY_PATH"] = "VALUE1";
    int status = 1;
    MOCKER(&fork)
        .stubs()
        .will(returnValue(pid_t(1)));
    MOCKER(&waitpid)
        .stubs()
        .with(any(), outBoundP(&status, sizeof(status)), any())
        .will(returnValue(__pid_t(0)));
    EXPECT_FALSE(runner.Run(cmd, envs));
    GlobalMockObject::verify();
}

/**
/* | 用例集 | OpRunner
/* |测试函数| ExecBinaryRunner::KillBinaryProcess
/* | 用例名 | test_kill_binary_process_except_success
/* |用例描述| 执行测试函数，不抛出异常
*/
TEST(OpRunner, test_kill_binary_process_except_success)
{
    ExecBinaryRunner runner;
    int status = 1;
    MOCKER(&kill)
        .stubs()
        .will(returnValue(int(0)));
    EXPECT_NO_THROW(runner.KillBinaryProcess());
    GlobalMockObject::verify();
}