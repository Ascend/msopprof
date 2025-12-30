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


#ifndef __MSOPPROF_CPPUTILS_TIMER_H__
#define __MSOPPROF_CPPUTILS_TIMER_H__

#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace Utility {
class Timer {
public:
    Timer() = default;
    ~Timer();
    // The unit of time is second, func is the callback function after the time arrives.
    void Start(uint32_t time, const std::function<void()> &func);
    void Stop();

private:
    void Wait(uint32_t time, const std::function<void()> &func);
    // thread for time countdown
    std::thread timeThread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    bool isStop_ = false;
};
}
// namespace Utility
#endif //__MSOPPROF_CPPUTILS_TIMER_H__