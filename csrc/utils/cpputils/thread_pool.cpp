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


#include "log.h"

#include "thread_pool.h"

namespace Utility {

ThreadPool::ThreadPool(uint32_t threadsNum)
    : threadsNum_(threadsNum)
{}

ThreadPool::~ThreadPool()
{
    if (running_) {
        Stop();
    }
}

bool ThreadPool::Start()
{
    std::unique_lock<std::mutex> lock(poolRunningStatusMutex_);
    if (running_) {
        LogError("Do not call thread pool Start multiple times");
        return false;
    }

    if (threadsNum_ == 0) {
        LogError("thread pool thread number should greater than 0");
        return false;
    }

    try {
        threads_.reserve(threadsNum_);
    } catch (...) {
        LogError("Reserve threads memory failed");
        return false;
    }

    running_ = true;
    for (uint32_t i = 0; i < threadsNum_; ++i) {
        std::thread emptyTaskLoopThread(&ThreadPool::Loop, this);
        threads_.emplace_back(std::move(emptyTaskLoopThread));
    }
    return true;
}

bool ThreadPool::Stop()
{
    {
        std::unique_lock<std::mutex> lock(poolRunningStatusMutex_);
        if (!running_) {
            LogWarn("Do not call thread pool Stop multiple times");
            return false;
        }

        // stop loop
        running_ = false;
        // Release all thread that is blocked in FetchTask.
        hasTaskToDo_.notify_all();
    }

    // wait all remaining thread until finished.
    for (auto &singleThread: threads_) {
        if (singleThread.joinable()) {
            singleThread.join();
        }
    }
    threads_.clear();
    std::vector<std::shared_ptr<std::string>> taskLogs;
    {
        std::lock_guard<std::mutex> lock(poolRunningStatusMutex_);
        taskLogs.swap(taskLogs_);
    }
    for (const auto &taskLog : taskLogs) {
        Log::FlushCapture(*taskLog);
    }
    return true;
}

void ThreadPool::AddTask(const Task &task)
{
    std::unique_lock<std::mutex> lock(poolRunningStatusMutex_);
    // 只继承父线程的日志捕获状态，每个任务使用独立缓冲，避免并发写入父缓冲。
    // 使用 shared_ptr 保证后续任务提交时，已有任务的缓冲地址和生命周期保持有效。
    auto taskLog = Log::IsCapturing() ? std::make_shared<std::string>() : nullptr;
    if (taskLog != nullptr) {
        taskLogs_.push_back(taskLog);
    }
    taskQueue_.emplace_back([task, taskLog] {
        LogCaptureGuard guard(taskLog.get());
        try {
            task();
        } catch (const std::exception &ex) {
            LogError("Thread[%zu] in Pool caught exception: %s",  std::hash<std::thread::id>{}(std::this_thread::get_id()), ex.what());
        } catch (...) {
            LogError("Thread[%zu] in Pool caught unknown exception", std::hash<std::thread::id>{}(std::this_thread::get_id()));
        }
    });
    // Release one thread that is blocked in FetchTask.
    hasTaskToDo_.notify_one();
}

void ThreadPool::WaitAllTasks()
{
    // This function does not ensure that the task is complete.
    // To implement all tasks, you need to invoke Stop again after all tasks are complete.
    std::unique_lock<std::mutex> lock(poolRunningStatusMutex_);
    waitTaskDone_.wait(lock, [this] { return taskQueue_.empty(); });
}

uint32_t ThreadPool::GetUnassignedTasksNum()
{
    std::unique_lock<std::mutex> lock(poolRunningStatusMutex_);
    return taskQueue_.size();
}

uint32_t ThreadPool::GetThreadsNum() const
{
    return threads_.size();
}

bool ThreadPool::FetchTask(Task &task)
{
    std::unique_lock<std::mutex> lock(poolRunningStatusMutex_);
    while (taskQueue_.empty() && running_) {
        // blocked until new task is allocated
        hasTaskToDo_.wait(lock);
    }

    if (!taskQueue_.empty() && running_) {
        task = taskQueue_.front();
        taskQueue_.pop_front();
        waitTaskDone_.notify_one();
        return true;
    }
    return false;
}

void ThreadPool::Loop()
{
    Task task;
    while (running_) {
        if (FetchTask(task)) {
            task();
        }
    }
}
} // namespace Utility
