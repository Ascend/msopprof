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


#ifndef __MSOPPROF_CPPUTILS_THREAD_SAFE_UNORDERED_MAP_H__
#define __MSOPPROF_CPPUTILS_THREAD_SAFE_UNORDERED_MAP_H__

#include <unordered_map>
#include <functional>
#include <mutex>

namespace Utility {

template<typename KeyType, typename ValueType>
class ThreadSafeUnorderedMap {
public:
    using Iterator = typename std::unordered_map<KeyType, ValueType>::iterator;

    explicit ThreadSafeUnorderedMap()
    = default;

    explicit ThreadSafeUnorderedMap(const std::unordered_map<KeyType, ValueType> initMap)
    {
        safeMap_ = initMap;
    }

    ~ThreadSafeUnorderedMap()
    = default;

    ThreadSafeUnorderedMap(const ThreadSafeUnorderedMap &rhs)
    {
        safeMap_ = rhs.safeMap_;
    }

    // implement operator =
    ThreadSafeUnorderedMap &operator=(const ThreadSafeUnorderedMap &rhs)
    {
        if (&rhs != this) {
            safeMap_ = rhs.safeMap_;
        }
        return *this;
    }

    ThreadSafeUnorderedMap &operator=(const std::unordered_map<KeyType, ValueType> inputMap)
    {
        if (!inputMap.empty()) {
            safeMap_ = inputMap;
        }
        return *this;
    }

    // implement operator []
    ValueType &operator[](const KeyType &key)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return safeMap_[key];
    }

    // get map size
    int Size()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return safeMap_.size();
    }

    // check map is empty
    bool Empty()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return safeMap_.empty();
    }

    // count select key num
    int Count(const KeyType &key)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return safeMap_.count(key);
    }

    // implement erase function
    void Erase(const KeyType &key)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        safeMap_.erase(key);
    }

    // implement insert function. return ture when success
    bool Insert(const KeyType &key, const ValueType &value)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto res = safeMap_.insert(std::pair<KeyType, ValueType>(key, value));
        return res.second;
    }

    // implement insert function. return ture when success
    bool Insert(const std::pair<KeyType, ValueType> pair)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto res = safeMap_.insert(std::pair<KeyType, ValueType>(pair.first, pair.second));
        return res.second;
    }

    // implement find key function. return ture when key found and save target value into value
    bool Find(const KeyType &key, ValueType &value)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = safeMap_.find(key);
        if (it != safeMap_.end()) {
            value = it->second;
            return true;
        }
        return false;
    }

    // implement find key function, return ture when key found
    bool Find(const KeyType &key)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = safeMap_.find(key);
        if (it != safeMap_.end()) {
            return true;
        }
        return false;
    }

    bool FindAndInsertIfNotExist(const KeyType &key, const ValueType value = {})
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = safeMap_.find(key);
        if (it != safeMap_.end()) {
            return true;
        }
        safeMap_.insert(std::pair<KeyType, ValueType>(key, value));
        return false;
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        safeMap_.clear();
    }

    Iterator begin() noexcept
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return safeMap_.begin();
    }

    Iterator end() noexcept
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return safeMap_.end();
    }

private:
    std::unordered_map<KeyType, ValueType> safeMap_ {};
    std::mutex mutex_;
};

}
#endif // __MSOPPROF_CPPUTILS_THREAD_SAFE_UNORDERED_MAP_H__
