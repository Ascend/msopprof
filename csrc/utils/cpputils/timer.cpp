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

#include "timer.h"

#include "log.h"

using namespace std;
namespace Utility {
Timer::~Timer()
{
    if (timeThread_.joinable()) {
        timeThread_.join();
    }
}

void Timer::Start(uint32_t time, const function<void()> &func)
{
    timeThread_ = thread([this, time, func] { Wait(time, func); });
}

void Timer::Wait(uint32_t time, const function<void()> &func)
{
    // The unit of time is second, func is the callback function after the time arrives.
    auto startTime = chrono::steady_clock::now();
    while (!isStop_) {
        auto elapsedTime = chrono::steady_clock::now() - startTime;
        if (elapsedTime >= chrono::seconds(time)) {
            isStop_ = true;
            LogDebug("The timer has reached the waiting time %u seconds.", time);
            func();
            break;
        }
        unique_lock<mutex> lock(mutex_);
        cond_.wait_for(lock, chrono::seconds(time) - elapsedTime);
    }
}

void Timer::Stop()
{
    if (!isStop_) {
        LogDebug("The timer was interrupted.");
        unique_lock<mutex> lock(mutex_);
        isStop_ = true;
        cond_.notify_one();
    }
}
}  // namespace Utility