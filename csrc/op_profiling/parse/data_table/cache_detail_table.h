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


#ifndef MSOPT_CACHEDETAILTABLE_H
#define MSOPT_CACHEDETAILTABLE_H
#include <memory>
#include <vector>
#include <mutex>
#include "profiling/simulator/data_parse/sim_defs.h"
#include "profiling/simulator/data_parse/sim_common_statistic.h"
#include "data_table.h"

namespace Profiling {
namespace Parse {

class CacheDetailTable : public DataTable {
public:
    static constexpr uint32_t PC = 0;
    static constexpr uint32_t PIPE = 1;
    static constexpr uint32_t NAME = 2;
    static constexpr uint32_t DETAIL = 3;
    static constexpr uint32_t END_OF_STRING_TYPE = 4; // string type end
    static constexpr uint32_t START = 5;
    static constexpr uint32_t END = 6;
    static constexpr uint32_t GPR_COUNT = 7;
    static constexpr uint32_t PROCESS_BYTES = 8;
    static constexpr uint32_t UB_READ_CONFLICT = 9;
    static constexpr uint32_t UB_WRITE_CONFLICT = 10;
    static constexpr uint32_t END_OF_INT_TYPE = 11; // int type end
    static constexpr uint32_t VEC_UTILIZATION = 12;
    static constexpr uint32_t END_OF_FLOAT_TYPE = 13; // float type end
    static constexpr uint32_t INSTR = 14;       // row
    explicit CacheDetailTable(const std::vector<MergeInfo> &data) : tableSize_(data.size()), cacheInstr_(data) {}
    CacheDetailTable() = default;

    std::vector<MergeInfo> &GetCache() { return cacheInstr_ ;}
    void SetCache(std::vector<MergeInfo> &cache) { cacheInstr_= cache; }
private:
    uint32_t tableSize_;
    std::vector<MergeInfo> cacheInstr_;
};
}
}
#endif // MSOPT_CACHEDETAILTABLE_H
