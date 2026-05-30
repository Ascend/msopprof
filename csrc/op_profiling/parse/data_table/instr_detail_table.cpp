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


#include "instr_detail_table.h"
#include "smart_pointer.h"

namespace Profiling {
namespace Parse {
InstrDetailTable::InstrDetailTable(std::vector<MergeInfo>& data) : tableSize_(data.size())
{
    for (uint32_t i = 0; i < END_OF_UINT64_TYPE; i++) {
        auto vector = Utility::MakeShared<std::vector<uint64_t>>(0);
        if (vector != nullptr) { uint64DataVec_.emplace_back(vector); }
    }

    for (uint32_t i = 0; i < END_OF_INT_TYPE - END_OF_UINT64_TYPE - 1; i++) {
        auto vector = Utility::MakeShared<std::vector<int>>(0);
        if (vector != nullptr) { intDataVec_.emplace_back(vector); }
    }

    for (uint32_t i = 0; i < END_OF_FLOAT_TYPE - END_OF_INT_TYPE - 1; i++) {
        auto vector = Utility::MakeShared<std::vector<float>>(0);
        if (vector != nullptr) { floatDataVec_.emplace_back(vector); }
    }

    mergeInfo_ = Utility::MakeShared<std::vector<MergeInfo>>(data.size());
    if (mergeInfo_ != nullptr) { mergeInfo_->swap(data); }
}

void InstrDetailTable::UpdateRow()
{
    if (mergeInfo_ == nullptr) {
        Utility::LogDebug("Get mergeInfo failed because of nullptr");
        return;
    }
    std::vector<MergeInfo>& mergeInfo = *mergeInfo_;
    std::shared_ptr<std::vector<int>> processBytes = GetColumnData<int>(PROCESS_BYTES);
    if (processBytes == nullptr) {
        Utility::LogDebug("Get process byte data failed because of nullptr");
    }
    std::shared_ptr<std::vector<int>> gprCount = GetColumnData<int>(GPR_COUNT);
    if (gprCount == nullptr) {
        Utility::LogDebug("Get gpr count data failed because of nullptr");
    }
    std::shared_ptr<std::vector<float>> vecUtilization = GetColumnData<float>(VEC_UTILIZATION);
    if (vecUtilization == nullptr) {
        Utility::LogDebug("Get vec utilization data failed because of nullptr");
    }
    std::shared_ptr<std::vector<int>> ubWriteConflict = GetColumnData<int>(UB_WRITE_CONFLICT);
    if (ubWriteConflict == nullptr) {
        Utility::LogDebug("Get ub write conflict data failed because of nullptr");
    }
    std::shared_ptr<std::vector<int>> ubReadConflict = GetColumnData<int>(UB_READ_CONFLICT);
    if (ubReadConflict == nullptr) {
        Utility::LogDebug("Get ub read conflict data failed because of nullptr");
    }
    std::shared_ptr<std::vector<uint64_t>> icache = GetColumnData<uint64_t>(ICACHE_CYC);
    if (icache == nullptr) {
        Utility::LogDebug("Get icache data failed because of nullptr");
    }
    std::shared_ptr<std::vector<uint64_t>> ccu = GetColumnData<uint64_t>(CCU_CYC);
    if (ccu == nullptr) {
        Utility::LogDebug("Get ccu data failed because of nullptr");
    }
    for (uint32_t i = 0; i < tableSize_; i++) {
        mergeInfo[i].processBytes = processBytes == nullptr ? 0 : (*processBytes)[i];
        mergeInfo[i].gprCount = gprCount == nullptr ? 0 : (*gprCount)[i];
        mergeInfo[i].vecUtilization = vecUtilization == nullptr ? 0 : (*vecUtilization)[i];
        mergeInfo[i].ubReadConflict = ubReadConflict == nullptr ? 0 : (*ubReadConflict)[i];
        mergeInfo[i].ubWriteConflict = ubWriteConflict == nullptr ? 0 : (*ubWriteConflict)[i];
        mergeInfo[i].icacheTick = icache == nullptr ? UINT64_MAX : (*icache)[i];
        mergeInfo[i].ccuTick = ccu == nullptr ? UINT64_MAX : (*ccu)[i];
    }
    for (auto &vec : intDataVec_) {
        if (vec != nullptr) {
            std::vector<int>().swap(*vec);
        }
    }
    for (auto &vec : floatDataVec_) {
        if (vec != nullptr) {
            std::vector<float>().swap(*vec);
        }
    }
    for (auto &vec : uint64DataVec_) {
        if (vec != nullptr) {
            std::vector<uint64_t>().swap(*vec);
        }
    }
}
} // Parse
} // Profiling
