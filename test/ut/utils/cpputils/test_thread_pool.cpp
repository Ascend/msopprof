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
#include <atomic>
#include <future>
#include <stdexcept>

#include "thread_safe_unordered_map.h"
#include "thread_pool.h"
#include "log.h"

using namespace Utility;

/**
 * |  用例集  | ThreadPool
 * | 测试函数 | ThreadPool::WaitAllTasks、ThreadPool::Stop
 * | 测试场景 | 单线程已领取唯一任务，但任务被同步信号阻塞；先WaitAllTasks，再从另一线程Stop。
 * | 预期结果 | WaitAllTasks返回时队列为空但任务未完成；释放任务前Stop不返回，释放后任务完成、线程数为0。
 */
TEST(ThreadPool, wait_on_empty_queue_expect_active_task_unfinished_until_stop_joins) {
    std::promise<void> releaseTask;
    auto release = releaseTask.get_future().share();
    std::promise<void> taskStarted;
    auto started = taskStarted.get_future();
    std::atomic<bool> taskCompleted{false};
    std::atomic<bool> releaseTimedOut{false};
    ThreadPool pool(1);
    ASSERT_TRUE(pool.Start());
    pool.AddTask([&] {
        taskStarted.set_value();
        // Bounded even when the controller fails; never strand a worker on a failed assertion.
        releaseTimedOut = release.wait_for(std::chrono::seconds(5)) != std::future_status::ready;
        taskCompleted = true;
    });
    EXPECT_EQ(started.wait_for(std::chrono::seconds(2)), std::future_status::ready);
    pool.WaitAllTasks();
    EXPECT_EQ(pool.GetUnassignedTasksNum(), 0);
    EXPECT_FALSE(taskCompleted.load());

    std::promise<void> stopEntered;
    auto stopping = stopEntered.get_future();
    std::promise<bool> stopResult;
    auto stopped = stopResult.get_future();
    std::thread controller([&] {
        stopEntered.set_value();
        stopResult.set_value(pool.Stop());
    });
    EXPECT_EQ(stopping.wait_for(std::chrono::seconds(2)), std::future_status::ready);
    EXPECT_EQ(stopped.wait_for(std::chrono::milliseconds(50)), std::future_status::timeout);
    releaseTask.set_value();
    controller.join();
    EXPECT_TRUE(stopped.get());
    EXPECT_TRUE(taskCompleted.load());
    EXPECT_FALSE(releaseTimedOut.load());
    EXPECT_EQ(pool.GetThreadsNum(), 0);
}

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

/**
 * |  用例集  | ThreadPool
 * | 测试函数 | ThreadPool::AddTask、ThreadPool::Stop
 * | 测试场景 | 单线程池先执行抛出runtime_error("boom")的任务，再执行设置完成标记的任务。
 * | 预期结果 | 后续任务正常完成，Stop成功，父捕获缓冲包含caught exception: boom。
 */
TEST(ThreadPool, standard_exception_expect_error_log_and_next_task_completed) {
    std::string captured;
    LogCaptureGuard capture(&captured);
    std::atomic<bool> siblingCompleted{false};
    ThreadPool pool(1);
    ASSERT_TRUE(pool.Start());
    pool.AddTask([] { throw std::runtime_error("boom"); });
    pool.AddTask([&] { siblingCompleted = true; });
    pool.WaitAllTasks();
    EXPECT_TRUE(pool.Stop());
    EXPECT_TRUE(siblingCompleted.load());
    EXPECT_NE(captured.find("caught exception: boom"), std::string::npos);
}

/**
 * |  用例集  | ThreadPool
 * | 测试函数 | ThreadPool::AddTask、ThreadPool::Stop
 * | 测试场景 | 单线程池先执行抛出整数42的任务，再执行设置完成标记的任务。
 * | 预期结果 | 后续任务正常完成，Stop成功，父捕获缓冲包含caught unknown exception。
 */
TEST(ThreadPool, nonstandard_exception_expect_error_log_and_next_task_completed) {
    std::string captured;
    LogCaptureGuard capture(&captured);
    std::atomic<bool> siblingCompleted{false};
    ThreadPool pool(1);
    ASSERT_TRUE(pool.Start());
    pool.AddTask([] { throw 42; });
    pool.AddTask([&] { siblingCompleted = true; });
    pool.WaitAllTasks();
    EXPECT_TRUE(pool.Stop());
    EXPECT_TRUE(siblingCompleted.load());
    EXPECT_NE(captured.find("caught unknown exception"), std::string::npos);
}

/**
 * |  用例集  | ThreadPool
 * | 测试函数 | ThreadPool::Stop、Log::FlushCapture
 * | 测试场景 | 父线程捕获时向双线程池先提交first、再提交second；first等待second写完日志后才写日志。
 * | 预期结果 | Stop后父缓冲包含两条日志，顺序仍为first、second，与提交顺序一致而非任务写日志顺序。
 */
TEST(ThreadPool, tasks_log_in_reverse_order_expect_stop_replays_in_submission_order) {
    std::string parent;
    std::promise<void> secondFinished;
    auto finished = secondFinished.get_future();
    std::atomic<bool> timedOut{false};
    ThreadPool pool(2);
    ASSERT_TRUE(pool.Start());
    {
        LogCaptureGuard guard(&parent);
        pool.AddTask([&] {
            timedOut = finished.wait_for(std::chrono::seconds(5)) != std::future_status::ready;
            LogInfo("worker-first");
        });
        pool.AddTask([&] {
            LogInfo("worker-second");
            secondFinished.set_value();
        });
        pool.WaitAllTasks();
        ASSERT_TRUE(pool.Stop());
    }
    const auto first = parent.find("worker-first");
    const auto second = parent.find("worker-second");
    EXPECT_NE(first, std::string::npos);
    EXPECT_NE(second, std::string::npos);
    EXPECT_LT(first, second);
    EXPECT_FALSE(timedOut.load());
}
