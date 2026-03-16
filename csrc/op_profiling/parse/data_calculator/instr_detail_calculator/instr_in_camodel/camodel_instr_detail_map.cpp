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


#include "camodel_instr_detail_map.h"

#include <regex>

namespace Profiling {
namespace Parse {

const RegDetailRegexMap A2A3RegDetailRegexMap = {
    {RegNameKey::XD_VALUE, std::regex("XD:X[0-9]{1,2}=(?:0x)?([0-9a-f]+)")},
    {RegNameKey::XN_VALUE, std::regex("XN:X[0-9]{1,2}=(?:0x)?([0-9a-f]+)")},
    {RegNameKey::XM_VALUE, std::regex("XM:X[0-9]{1,2}=(?:0x)?([0-9a-f]+)")},
    {RegNameKey::XS_VALUE, std::regex("XS:X[0-9]{1,2}=(?:0x)?([0-9a-f]+)")},
    {RegNameKey::XT_VALUE, std::regex("XT:X[0-9]{1,2}=(?:0x)?([0-9a-f]+)")},
    {RegNameKey::SRC,      std::regex("Src:([A-Za-z]+)")},
    {RegNameKey::DST,      std::regex("Dst:([A-Za-z]+)")},
    {RegNameKey::DTYPE,    std::regex("Dtype:([A-Za-z0-9]+)")},
    {RegNameKey::CONV_TYPE,    std::regex("Conv_type:([A-Za-z0-9]+)")},
};

const std::unordered_map<ChipProductType, RegDetailRegexMap> RegDetailRegexChipBase = {
    // current support 310P A2 A3
    {ChipProductType::ASCEND910B_SERIES, A2A3RegDetailRegexMap},
    {ChipProductType::ASCEND910_93_SERIES, A2A3RegDetailRegexMap},
    {ChipProductType::ASCEND310P_SERIES, {
        {RegNameKey::XD_VALUE, std::regex("xdValue:(?:0x)?([0-9a-f]+)")},
        {RegNameKey::XN_VALUE, std::regex("xnValue:(?:0x)?([0-9a-f]+)")},
        {RegNameKey::XM_VALUE, std::regex("xmValue:(?:0x)?([0-9a-f]+)")},
        {RegNameKey::XS_VALUE, std::regex("xsValue:(?:0x)?([0-9a-f]+)")},
        {RegNameKey::XT_VALUE, std::regex("xtValue:(?:0x)?([0-9a-f]+)")},
        {RegNameKey::SRC, std::regex("srcIDValue:([0-9]+)")},
        {RegNameKey::DST, std::regex("destIDValue:([0-9]+)")},
        {RegNameKey::DTYPE, std::regex("dTypeValue:([0-9]+)")},
        {RegNameKey::VEC_DTYPE, std::regex("[Tt]ype(?:[:\\s]{0,3})([0-9]+)")}, // radix 10
        {RegNameKey::VEC_REPEAT, std::regex("repeat:([0-9]+)")}, // radix 10
        {RegNameKey::VEC_SRC_ADDR, std::regex("src_addr:(?:0x)?([0-9a-f]+)")}, // radix 16
        {RegNameKey::VEC_SRC_STRIDE, std::regex("src_stride:([0-9a-fx]+)")}, // radix 16 / 10
        {RegNameKey::VEC_SRC_REP_STRIDE, std::regex("src_rep_stride:([0-9a-fx]+)")}, // radix 16 / 10
        {RegNameKey::VEC_SRC1_ADDR, std::regex("src1_addr:(?:0x)?([0-9a-f]+)")}, // radix 16
        {RegNameKey::VEC_SRC1_STRIDE, std::regex("src1_stride:([0-9a-fx]+)")}, // radix 16 / 10
        {RegNameKey::VEC_SRC1_REP_STRIDE, std::regex("src1_rep_stride:([0-9a-fx]+)")}, // radix 16 / 10
        {RegNameKey::VEC_DST_ADDR, std::regex("dest_addr:(?:0x)?([0-9a-f]+)")}, // radix 16
        {RegNameKey::VEC_DST_STRIDE, std::regex("dst_stride:([0-9a-fx]+)")}, // radix 16 / 10
        {RegNameKey::VEC_DST_REP_STRIDE, std::regex("dst_rep_stride:([0-9a-fx]+)")}, // radix 16 / 10
    }},
};

bool GetRegDetailRegexMap(ChipProductType seriesChipType, RegDetailRegexMap &resMap)
{
    auto it = RegDetailRegexChipBase.find(seriesChipType);
    if (it == RegDetailRegexChipBase.end()) {
        resMap = {};
        return false;
    }
    resMap = it->second;
    return true;
}
}
}
