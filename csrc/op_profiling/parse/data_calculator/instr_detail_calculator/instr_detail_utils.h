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


#ifndef __MSOPPROF_PARSE_DATA_CALCULATOR_INSTR_DETAIL_UTILS_H__
#define __MSOPPROF_PARSE_DATA_CALCULATOR_INSTR_DETAIL_UTILS_H__

#include <cstdint>
#include <regex>

#include "instr_detail_defs.h"
#include "ustring.h"

namespace Profiling {
namespace Parse {

inline uint32_t CeilByAlignSize(uint32_t v, uint32_t alignSize = UB_ALIGN_SIZE)
{
    if (alignSize == 0) {
        return 0;
    }
    return ((v + alignSize - 1) / alignSize) * alignSize;
}

template<typename T>
inline T RoundUpDivide(T v, T divisor)
{
    if (divisor == 0) {
        return 0;
    }
    return static_cast<T>((v + divisor - 1) / divisor);
}

bool GetRegValue(const std::regex &pattern, const std::string &instrDetail, uint64_t &resValue,
                 int32_t regValueRadix = Utility::RADIX_16);
bool GetRegString(const std::regex &pattern, const std::string &instrDetail, std::string &resString);
bool GetRegValueAuto(const std::regex &pattern, const std::string &instrDetail, uint64_t &resValue);
}
}

#endif // __MSOPPROF_PARSE_DATA_CALCULATOR_INSTR_DETAIL_UTILS_H__
