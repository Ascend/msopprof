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


#ifndef MSOPT_INSTRDETAILTABLE_H
#define MSOPT_INSTRDETAILTABLE_H

#include <memory>
#include <vector>
#include <mutex>
#include <set>

#include "profiling/simulator/data_parse/sim_defs.h"

namespace Profiling {
namespace Parse {

class InstrDetailTable {
public:
    // Column Index Start
    static constexpr uint32_t START_TICK = 0;
    static constexpr uint32_t END_TICK = 1;
    static constexpr uint32_t THEO_STALL_CYC = 2;
    static constexpr uint32_t REAL_STALL_CYC = 3;
    static constexpr uint32_t END_OF_STRING_TYPE = 4; // string type end
    static constexpr uint32_t GPR_COUNT = 5;
    static constexpr uint32_t PROCESS_BYTES = 6;
    static constexpr uint32_t UB_READ_CONFLICT = 7;
    static constexpr uint32_t UB_WRITE_CONFLICT = 8;
    static constexpr uint32_t END_OF_INT_TYPE = 9; // int type end
    static constexpr uint32_t VEC_UTILIZATION = 10;
    static constexpr uint32_t END_OF_FLOAT_TYPE = 11; // float type end
    static constexpr uint32_t MERGE_INFO = 12;       // row
    // Column Index End
    explicit InstrDetailTable(std::vector<MergeInfo>& data);
    ~InstrDetailTable() = default;

    template<typename T>
    bool UpdateColumnValue(uint32_t columnIndex, uint32_t valueIndex, T value) const
    {
        std::shared_ptr<std::vector<T>> column = GetColumn<T>(columnIndex);
        if (column == nullptr || column->size() <= valueIndex) {
            return false;
        }
        (*column)[valueIndex] = value;
        return true;
    }

    template<typename T>
    bool UpdateColumnValueInRange(uint32_t columnIndex, uint32_t begin, uint32_t end,
                                  std::function<T (T)> const &function) const
    {
        if (begin >= end) {
            return false;
        }
        std::shared_ptr<std::vector<T>> column = GetColumn<T>(columnIndex);
        if (column == nullptr) {
            return false;
        }
        uint32_t size = column->size() < end ? column->size() : end;
        for (uint32_t i = begin; i < size; i++) {
            (*column)[i] = function((*column)[i]);
        }
        return true;
    }

    template<typename T>
    bool UpdateColumnAllValue(uint32_t columnIndex, std::vector<T>& value) const
    {
        std::shared_ptr<std::vector<T>> column = GetColumn<T>(columnIndex);
        if (column == nullptr || column->size() != value.size()) {
            return false;
        }
        column->assign(value.begin(), value.end());
        return true;
    }

    template<typename T>
    T* QueryColumnValue(uint32_t columnIndex, uint32_t valueIndex) const
    {
        std::shared_ptr<std::vector<T>> column = GetColumn<T>(columnIndex);
        if (column == nullptr || column->size() <= valueIndex) {
            return nullptr;
        }
        return &(*column)[valueIndex];
    }

    // temporary func
    template<typename T>
    std::shared_ptr<std::vector<T>> GetColumnData(uint32_t columnIndex) const
    {
        return GetColumn<T>(columnIndex);
    }
    // temporary func
    void UpdateRow();

    uint32_t GetSize() const
    {
        return tableSize_;
    }
private:
    template<typename T>
    typename std::enable_if<std::is_base_of<std::string, T>::value, std::shared_ptr<std::vector<std::string>>>::type
    GetColumn(uint32_t columnIndex) const
    {
        if (columnIndex >= stringDataVec_.size()) {
            return nullptr;
        }
        std::shared_ptr<std::vector<std::string>> column = stringDataVec_[columnIndex];
        if (column->empty()) {
            column->resize(tableSize_);
        }
        return column;
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, std::shared_ptr<std::vector<int>>>::type
    GetColumn(uint32_t columnIndex) const
    {
        if (columnIndex <= END_OF_STRING_TYPE || columnIndex - (END_OF_STRING_TYPE + 1) >= intDataVec_.size()) {
            return nullptr;
        }
        std::shared_ptr<std::vector<int>> column = intDataVec_[columnIndex - (END_OF_STRING_TYPE + 1)];
        if (column->empty()) {
            column->resize(tableSize_);
            std::fill(column->begin(), column->end(), -1);
        }
        return column;
    }

    template<typename T>
    typename std::enable_if<std::is_floating_point<T>::value, std::shared_ptr<std::vector<float>>>::type
    GetColumn(uint32_t columnIndex) const
    {
        if (columnIndex <= END_OF_INT_TYPE || columnIndex - (END_OF_INT_TYPE + 1) >= floatDataVec_.size()) {
            return nullptr;
        }
        std::shared_ptr<std::vector<float>> column = floatDataVec_[columnIndex - (END_OF_INT_TYPE + 1)];
        if (column->empty()) {
            column->resize(tableSize_);
            std::fill(column->begin(), column->end(), -1.0f);
        }
        return column;
    }

    template<typename T>
    typename std::enable_if<std::is_base_of<MergeInfo, T>::value, std::shared_ptr<std::vector<MergeInfo>>>::type
    GetColumn(uint32_t columnIndex) const
    {
        if (columnIndex != MERGE_INFO) {
            return nullptr;
        }
        return mergeInfo_;
    }
private:
    std::vector<std::shared_ptr<std::vector<std::string>>> stringDataVec_;
    std::vector<std::shared_ptr<std::vector<int>>> intDataVec_;
    std::vector<std::shared_ptr<std::vector<float>>> floatDataVec_;
    std::shared_ptr<std::vector<MergeInfo>> mergeInfo_;
    uint32_t tableSize_;
};

} // Parse
} // Profiling

#endif // MSOPT_INSTRDETAILTABLE_H
