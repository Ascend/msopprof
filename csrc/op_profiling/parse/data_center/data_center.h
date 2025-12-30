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


#ifndef __MSOPPROF_PARSE_DATA_CENTER_H__
#define __MSOPPROF_PARSE_DATA_CENTER_H__

#include <typeindex>
#include <memory>
#include <vector>
#include <mutex>
#include <set>
#include "data_stream.h"

#include "smart_pointer.h"
#include "profiling/simulator/data_parse/sim_defs.h"

namespace Profiling {
namespace Parse {

/* 保存在DataCenter中的DataBase基础数据类型 */
struct BaseDataType {
    virtual ~BaseDataType() = default;
};

template <typename T>
struct CustomDataType : public BaseDataType {
    std::shared_ptr<T> data;
};

using BaseTypePtr = std::shared_ptr<BaseDataType>;

class DataCenter {
public:
    DataCenter() = default;
    ~DataCenter() = default;

    // 禁止拷贝构造和赋值操作，仅允许移动构造
    DataCenter(DataCenter&) = delete;
    DataCenter& operator=(DataCenter&) = delete;
    DataCenter(DataCenter&& dataCenter) : dataTablePtrMap_(dataCenter.dataTablePtrMap_),
                                          dataStreamPtrMap_(dataCenter.dataStreamPtrMap_) {}
    DataCenter& operator=(DataCenter&&) = delete;

    template<typename T>
    std::shared_ptr<T> GetDbPtr() const
    {
        return CastBase2CustomPtr<T>(GetDbPtrByClassIdx(typeid(T)));
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(dataCenterMutex_);
        dataTablePtrMap_.clear();
        dataStreamPtrMap_.clear();
    }

    /**
    * @brief 将数据存放入DataCenter
    *
    * @tparam T 自定义数据类型
    * @param ptr 数据实例共享指针
    * @return 成功或失败
    */
    template<typename T>
    bool DataTableRegister(std::shared_ptr<T> ptr)
    {
        if (ptr == nullptr) {
            return false;
        }

        std::type_index idx = typeid(T);
        if (GetDbPtr<T>() != nullptr) { // 支持注入唯一类型数据，即同个子类只能注入一次
            Utility::LogError("DataBase has been registered, type name: %s", idx.name());
            return false;
        }

        auto newDb = Utility::MakeShared<CustomDataType<T>>();
        if (newDb == nullptr) {
            Utility::LogError("Build new dataBase ptr failed, type name: %s", idx.name());
            return false;
        }
        newDb->data = ptr;
        std::lock_guard<std::mutex> lock(dataCenterMutex_);
        auto ret = dataTablePtrMap_.emplace(idx, newDb);
        return ret.second;
    }

    template<typename T>
    bool DataStreamRegister()
    {
        std::type_index idx = typeid(T);
        std::lock_guard<std::mutex> lock(dataCenterMutex_);
        auto ptr = Utility::MakeShared<DataStreamImpl<T>>();
        if (ptr == nullptr) {
            return false;
        }
        auto ret = dataStreamPtrMap_.emplace(idx, ptr);
        return ret.second;
    }

    template<typename T>
    std::shared_ptr<DataStreamImpl<T>> GetStreamPtr() const
    {
        std::type_index idx = typeid(T);
        std::lock_guard<std::mutex> lock(dataCenterMutex_);
        auto it = dataStreamPtrMap_.find(idx);
        if (it == dataStreamPtrMap_.end()) {
            return nullptr;
        }
        return std::static_pointer_cast<DataStreamImpl<T>>(it->second);
    }

    /**
     * @brief 输入需要保留的数据类型，删除其它数据
     *
     * @param keepingDataType 需要保留的数据类型列表
     * @return 释放的类型集合
     */
    std::set<std::type_index> DataTableUnRegister(const std::set<std::type_index>& activateDb);

    std::vector<bool> IsRequiredDbExist(const std::vector<std::type_index>& queryDbList) const;

private:
    template<typename T>
    std::shared_ptr<T> CastBase2CustomPtr(const BaseTypePtr& ptr) const
    {
        if (ptr == nullptr) {
            return nullptr;
        }
        auto customPtr = std::static_pointer_cast<CustomDataType<T>>(ptr);
        return customPtr->data;
    }
    BaseTypePtr GetDbPtrByClassIdx(std::type_index idx) const;

    std::unordered_map<std::type_index, BaseTypePtr> dataTablePtrMap_ = {};
    std::unordered_map<std::type_index, std::shared_ptr<DataStreamBase>> dataStreamPtrMap_ = {};
    mutable std::mutex dataCenterMutex_;
};

}
}

#endif // __MSOPPROF_PARSE_DATA_CENTER_H__
