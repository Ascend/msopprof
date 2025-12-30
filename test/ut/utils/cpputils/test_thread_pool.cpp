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

#include <algorithm>

#include "thread_safe_unordered_map.h"
#include "thread_pool.h"

using namespace Utility;

TEST(ThreadPool, test_ThreadPool_execute_task_in_parallel_expect_success)
{
    const int tasksNum = 40;
    const int threadsNum = 20;

    ThreadPool pool(threadsNum);
    // build task with time consume 1s
    auto task = []() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    };
    auto err = pool.Start();
    EXPECT_TRUE(err);

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < tasksNum; ++i) {
        pool.AddTask(task);
    }
    pool.WaitAllTasks();
    pool.Stop();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    // 20 thread execute 40 tasks expect consumes 2s ideally
    std::chrono::duration<double> expectT{2.2};
    bool check = elapsed <= expectT;
    EXPECT_TRUE(check);
}

TEST(ThreadPool, test_ThreadPool_with_no_task_add_when_not_start_thread_pool_expect_zero_unassined_task_num)
{
    const int threadsNum = 20;
    ThreadPool pool(threadsNum);

    auto res = pool.Start();
    EXPECT_TRUE(res);

    auto checkZero = pool.GetUnassignedTasksNum();
    EXPECT_EQ(checkZero, 0);

    res = pool.Stop();
    EXPECT_TRUE(res);
}

TEST(ThreadPool, test_ThreadPool_with_thirty_task_add_when_not_start_thread_pool_expect_thirty_unassined_task_num)
{
    const int tasksNum = 20;
    const int threadsNum = 15;
    ThreadPool pool(threadsNum);

    auto task = []() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    };

    for (int i = 0; i < tasksNum; ++i) {
        pool.AddTask(task);
    }

    auto checkNum = pool.GetUnassignedTasksNum();
    const int expectNum = tasksNum;
    EXPECT_EQ(expectNum, checkNum);
}

TEST(ThreadPool, test_ThreadPool_when_not_start_thread_pool_expect_zero_thread_num)
{
    const int threadsNum = 20;
    ThreadPool pool(threadsNum);

    auto checkZero = pool.GetThreadsNum();
    EXPECT_EQ(checkZero, 0);
}

TEST(ThreadPool, test_ThreadPool_with_multi_start_expect_failed)
{
    const int threadsNum = 20;
    ThreadPool pool(threadsNum);

    auto checkTrue = pool.Start();
    EXPECT_TRUE(checkTrue);

    auto checkFalse = pool.Start();
    EXPECT_FALSE(checkFalse);

    checkTrue = pool.Stop();
    EXPECT_TRUE(checkTrue);
}

TEST(ThreadPool, test_ThreadPool_with_multi_stop_expect_failed)
{
    const int threadsNum = 20;
    ThreadPool pool(threadsNum);

    auto checkTrue = pool.Start();
    EXPECT_TRUE(checkTrue);

    auto checkStopTrue = pool.Stop();
    EXPECT_TRUE(checkStopTrue);

    auto checkStopFalse = pool.Stop();
    EXPECT_FALSE(checkStopFalse);
}

TEST(ThreadPool, test_ThreadPool_with_forty_init_thread_expect_forty_threads_num_after_thread_pool_start)
{
    const int threadsNum = 40;
    ThreadPool pool(threadsNum);

    auto err = pool.Start();
    EXPECT_TRUE(err);

    auto checkThirty = pool.GetThreadsNum();
    const int expectNum = threadsNum;
    EXPECT_EQ(checkThirty, expectNum);

    auto checkTrue = pool.Stop();
    EXPECT_TRUE(checkTrue);
}

TEST(ThreadPool, test_ThreadPool_with_100_thread_num_and_map_insert_expect_correct_map_result)
{
    ThreadSafeUnorderedMap<std::string, int> testMap;
    const int threadsNum = 50;
    const int tasksNum = 50;
    ThreadPool pool(threadsNum);
    std::vector<int> res;
    std::mutex resLock;

    auto err = pool.Start();
    EXPECT_TRUE(err);

    for (int i = 0; i < tasksNum; ++i) {
        // task for insert key
        auto taskWrite = [i, &testMap]() {
            testMap.Insert(std::to_string(i), i);
        };
        // task for read
        auto taskRead = [i, &testMap, &res, &resLock]() {
            thread_local int tmp = -1;
            while (!testMap.Find(std::to_string(i), tmp)) {
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
            {
                std::lock_guard<std::mutex> lock(resLock);
                res.emplace_back(tmp);
            }
        };

        pool.AddTask(taskRead);
        pool.AddTask(taskWrite);
    }

    pool.WaitAllTasks();
    pool.Stop();
    // check testMap size is required
    EXPECT_EQ(tasksNum, testMap.Size());

    // check if key is unique
    std::sort(res.begin(), res.end());
    std::set<int> resSet(res.begin(), res.end());
    EXPECT_EQ(tasksNum, resSet.size());

    // check if map is correct
    for (int i = 0; i < tasksNum; ++i) {
        EXPECT_EQ(i, res[i]);
    }
}

TEST(ThreadPool, test_thread_pool_with_wait_task_between_two_task_addtion_expect_success)
{
    ThreadSafeUnorderedMap<std::string, int> testMap;
    const int threadsNum = 25;
    const int tasksNum = 50;
    ThreadPool pool(threadsNum);
    std::vector<int> res;
    std::mutex resLock;
    pool.Start();

    for (int i = 0; i < tasksNum; ++i) {
        auto taskWrite = [i, &testMap]() {
            testMap.Insert(std::to_string(i), i);
        };
        auto taskRead = [i, &testMap, &res, &resLock]() {
            thread_local int tmp = -1;
            while (!testMap.Find(std::to_string(i), tmp)) {
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
            std::lock_guard<std::mutex> lock(resLock);
            res.emplace_back(tmp);
        };

        pool.AddTask(taskRead);
        pool.AddTask(taskWrite);
    }

    pool.WaitAllTasks();
    const int tasksNewNum = 100;
    for (int i = tasksNum; i < tasksNewNum; ++i) {
        auto taskWrite = [i, &testMap]() {
            testMap.Insert(std::to_string(i), i);
        };
        auto taskRead = [i, &testMap, &res, &resLock]() {
            thread_local int tmp = -1;
            while (!testMap.Find(std::to_string(i), tmp)) {
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
            std::lock_guard<std::mutex> lock(resLock);
            res.emplace_back(tmp);
        };
        pool.AddTask(taskRead);
        pool.AddTask(taskWrite);
    }
    pool.WaitAllTasks();
    pool.Stop();

    EXPECT_EQ(tasksNewNum, testMap.Size());

    std::sort(res.begin(), res.end());
    std::set<int> resSet(res.begin(), res.end());
    EXPECT_EQ(tasksNewNum, resSet.size());

    for (int i = 0; i < tasksNum; ++i) {
        EXPECT_EQ(i, res[i]);
    }
}
