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


#ifndef __MSOPPROF_PARSE_DATA_CALCULATOR_INSTR_DETAIL_CAMODEL_DTYPE_DEFS_H__
#define __MSOPPROF_PARSE_DATA_CALCULATOR_INSTR_DETAIL_CAMODEL_DTYPE_DEFS_H__

#include <cstdint>
#include <unordered_map>

#include "parse/data_calculator/instr_detail_calculator/instr_detail_defs.h"

namespace Profiling {
namespace Parse {

enum class VectorDtype310P : uint32_t {
    VEC_B8 = 0,
    VEC_B16 = 1,
    VEC_B32 = 2,
    VEC_S8,
    VEC_S32,
    VEC_FP16,
    VEC_FMIX,
    VEC_FP32,
    VEC_U32,
    VEC_S16,
    VEC_NA
};

const std::unordered_map<VectorDtype310P, InstrDataType> VEC_DTYPE_BYTES_310P_MAP = {
    {VectorDtype310P::VEC_B8,   InstrDataType::B8},
    {VectorDtype310P::VEC_B16,  InstrDataType::B16},
    {VectorDtype310P::VEC_B32,  InstrDataType::B32},
    {VectorDtype310P::VEC_S8,   InstrDataType::S8},
    {VectorDtype310P::VEC_S32,  InstrDataType::S32},
    {VectorDtype310P::VEC_FP16, InstrDataType::F16},
    {VectorDtype310P::VEC_FMIX, InstrDataType::FMIX},
    {VectorDtype310P::VEC_FP32, InstrDataType::F32},
    {VectorDtype310P::VEC_U32,  InstrDataType::U32},
    {VectorDtype310P::VEC_S16,  InstrDataType::S16},
    {VectorDtype310P::VEC_NA,   InstrDataType::INVALID},
};

const std::unordered_map<std::string, InstrDataType> VEC_DTYPE_BYTES_A2A3_MAP = {
    {"S4", InstrDataType::S4},
    {"B4", InstrDataType::B4},
    {"U8", InstrDataType::U8},
    {"S8", InstrDataType::S8},
    {"B8", InstrDataType::B8},
    {"U16", InstrDataType::U16},
    {"S16", InstrDataType::S16},
    {"B16", InstrDataType::B16},
    {"F16", InstrDataType::F16},
    {"U32", InstrDataType::U32},
    {"S32", InstrDataType::S32},
    {"B32", InstrDataType::B32},
    {"B32S", InstrDataType::B32S},
    {"B32L", InstrDataType::B32L},
    {"F32", InstrDataType::F32},
    {"U64", InstrDataType::U64},
    {"S64", InstrDataType::S64},
    {"B64", InstrDataType::B64},
    {"F64", InstrDataType::F64},
    {"FMIX", InstrDataType::FMIX},
    {"F16F16", InstrDataType::F16F16},
    {"F16F32", InstrDataType::F16F32},
    {"F16U2", InstrDataType::F16U2},
    {"U8S8", InstrDataType::U8S8},
    {"B8U2", InstrDataType::B8U2},
    {"DEQ", InstrDataType::DEQ},
    {"BF16F32", InstrDataType::BF16F32},
    {"F32F32", InstrDataType::F32F32},
    {"BF16", InstrDataType::BF16},
    {"UNDEF", InstrDataType::INVALID},
};

}
}

#endif // __MSOPPROF_PARSE_DATA_CALCULATOR_INSTR_DETAIL_CAMODEL_DTYPE_DEFS_H__