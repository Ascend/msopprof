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
#include <sys/wait.h>
#include <functional>
#include <unistd.h>
#include "timer.h"
using namespace Utility;

int testNum = 1;

void callBackFunc()
{
    testNum = 20;
}

TEST(Timer, test_child_process_exit_advance_when_already_reached_wait_time)
{
    testNum = 1;
    uint32_t waitTime = 1;
    pid_t pid = fork();
    if (pid >= 0) {
        if (pid == 0) {
            sleep(2);
        } else {
            Timer timer;
            timer.Start(waitTime, [this] { callBackFunc(); });
            int status;
            waitpid(pid, &status, 0);
            timer.Stop();
            ASSERT_EQ(testNum, 20);
        }
    }
}

TEST(Timer, test_child_process_exit_normal_when_not_reach_wait_time)
{
    testNum = 1;
    uint32_t waitTime = 3;
    pid_t pid = fork();
    if (pid >= 0) {
        if (pid == 0) {
            sleep(2);
        } else {
            Timer timer;
            timer.Start(waitTime, [this] { callBackFunc(); });
            int status;
            waitpid(pid, &status, 0);
            timer.Stop();
            ASSERT_EQ(testNum, 1);
        }
    }
}