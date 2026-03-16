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


#ifndef __MSOPPROF_PARSE_DATA_CALCULATOR_INSTR_DETAIL_CAMODEL_INSTR_DETAIL_MAP_H__
#define __MSOPPROF_PARSE_DATA_CALCULATOR_INSTR_DETAIL_CAMODEL_INSTR_DETAIL_MAP_H__

#include <unordered_map>

#include <regex>

#include "common/defs.h"

namespace Profiling {
namespace Parse {

enum class RegNameKey : uint32_t {
    XD_VALUE,
    XN_VALUE,
    XM_VALUE,
    XS_VALUE,
    XT_VALUE,
    SRC,
    DST,
    DTYPE,
    // A2 A3 only
    CONV_TYPE,
    // 310P only
    VEC_DTYPE,
    VEC_REPEAT,
    VEC_SRC_ADDR,
    VEC_SRC_STRIDE,
    VEC_SRC_REP_STRIDE,
    VEC_SRC1_ADDR,
    VEC_SRC1_STRIDE,
    VEC_SRC1_REP_STRIDE,
    VEC_DST_ADDR,
    VEC_DST_STRIDE,
    VEC_DST_REP_STRIDE,
};

using RegDetailRegexMap = std::unordered_map<RegNameKey, std::regex>;
bool GetRegDetailRegexMap(ChipProductType seriesChipType, RegDetailRegexMap &resMap);
}
}
#endif // __MSOPPROF_PARSE_DATA_CALCULATOR_INSTR_DETAIL_CAMODEL_INSTR_DETAIL_MAP_H__
