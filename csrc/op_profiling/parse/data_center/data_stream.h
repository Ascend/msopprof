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


#ifndef MSOPT_DATA_STREAM_H
#define MSOPT_DATA_STREAM_H
#include <queue>
#include <mutex>
#include <atomic>
#include <condition_variable>

namespace Profiling {
namespace Parse {
class DataStreamBase {
public:
    virtual ~DataStreamBase() = default;
    virtual void Shutdown() = 0;
};

template <typename T>
class DataStreamImpl : public DataStreamBase {
public:
    ~DataStreamImpl() override = default;
    bool IsStop() const
    {
        return !isActive_;
    }
    // 优雅关闭
    void Shutdown() override
    {
        isActive_ = false;
        cv_.notify_all();
    }
    // 生产者插入数据
    void Push(const T& item)
    {
        if (!isActive_) {
            return;
        }
        std::lock_guard<std::mutex> lock(mtx_);
        dataQueue_.push(item);
        cv_.notify_one();  // 通知消费者
    }

    // 消费者获取数据（阻塞直到有数据）
    T Pop()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        if (dataQueue_.empty()) {
            cv_.wait(lock, [this] { return (!dataQueue_.empty() || !isActive_); });
        }
        if (!isActive_) {
            return T();
        }
        T item = dataQueue_.front();
        dataQueue_.pop();
        return item;
    }

    // 非阻塞版本（立即返回，无数据时返回空值）
    bool TryPop(T& item)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (dataQueue_.empty()) {
            return false;
        }
        item = dataQueue_.front();
        dataQueue_.pop();
        return true;
    }
private:
    std::queue<T> dataQueue_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::atomic<bool> isActive_ {true};
};
}
}

#endif // MSOPT_DATA_STREAM_H
