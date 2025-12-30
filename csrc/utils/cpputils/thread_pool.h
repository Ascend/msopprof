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


#ifndef __MSOPPROF_CPPUTILS_THREAD_POOL_H__
#define __MSOPPROF_CPPUTILS_THREAD_POOL_H__

#include <thread>
#include <mutex>
#include <functional>
#include <condition_variable>
#include <deque>
#include <vector>
#include <atomic>

namespace Utility {

// Task type is for task in thread pool
using Task = std::function<void()>;

class ThreadPool {
public:
    explicit ThreadPool(uint32_t threadsNum = 1);

    // All resources are automatically released during destructor.
    ~ThreadPool();

    // Start the thread pool and create threadsNum threads to wait for tasks.
    bool Start();
    // Stop the thread pool. (Stop task assignment, and wait current thread complete, then release all threads.
    bool Stop();
    // Wait for tasks in the task queue to be assigned to threads for execution.
    void WaitAllTasks();

    void AddTask(const Task &task);
    uint32_t GetUnassignedTasksNum();
    uint32_t GetThreadsNum();

private:
    // default loop for each thread
    void Loop();
    // Get one task from taskQueue_. Return true if success
    bool FetchTask(Task &task);

private:
    std::mutex poolRunningStatusMutex_;
    std::condition_variable hasTaskToDo_;
    std::condition_variable waitTaskDone_;
    std::vector<std::thread> threads_;
    std::deque<Task> taskQueue_;
    uint32_t threadsNum_ = 0;
    std::atomic<bool> running_ {false};
};

} // namespace Utility


#endif //__MSOPPROF_CPPUTILS_THREAD_POOL_H__
