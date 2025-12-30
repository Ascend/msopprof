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


#ifndef MSOPT_CUBE_INSTR_TABLE_A5_H
#define MSOPT_CUBE_INSTR_TABLE_A5_H

#include <string>
#include <unordered_map>

namespace Encode {
const std::unordered_map<uint32_t, std::string> INSTR19_TABLE1_A5 = {
    {0xf4000000, "DEPTHWISE_CONV_V2.u8"},
    {0xf4400000, "DEPTHWISE_CONV_V2.s8"},
    {0xf4800000, "DEPTHWISE_CONV_V2.f162f16"},
    {0xf4c00000, "DEPTHWISE_CONV_V2.f162f32"},
    {0xf5400000, "DEPTHWISE_CONV_V2.u8s8"},
    {0xf5800000, "DEPTHWISE_CONV_V2.s16s8"},
    {0xfc000000, "GROUP_CONV.u8"},
    {0xfc400000, "GROUP_CONV.s8"},
    {0xfc800000, "GROUP_CONV.f162f16"},
    {0xfcc00000, "GROUP_CONV.f162f32"},
    {0xfd400000, "GROUP_CONV.u8s8"},
    {0xfd800000, "GROUP_CONV.s16s8"},
};

const std::unordered_map<uint32_t, std::string> INSTR20_TABLE1_A5 = {
    {0xf0000000, "MMAD u8"},
    {0xf0400000, "MMAD s8"},
    {0xf0800000, "MMAD f16_to_f16"},
    {0xf0c00000, "MMAD f16_to_f32"},
    {0xf1400000, "MMAD u8s8"},
    {0xf1800000, "MMAD s4"},
    {0xf0000001, "MMAD s16s8"},
    {0xf0400001, "MMAD bf16_2_f32"},
    {0xf0800001, "MMAD f32_2_f32"},
    {0xf0c00001, "MMAD s8s4"},
    {0xf1000001, "MMAD e4m3e4m3"},
    {0xf1400001, "MMAD e4m3e5m2"},
    {0xf1800001, "MMAD e5m2e4m3"},
    {0xf1c00001, "MMAD e5m2e5m2"},
    {0xf2000000, "MMAD_MX.e1m2e1m2"},
    {0xf2400000, "MMAD_MX.e1m2e1m2"},
    {0xf2800000, "MMAD_MX.e1m2e1m2"},
    {0xf2c00000, "MMAD_MX.e1m2e1m2"},
    {0xf3000000, "MMAD_MX.e1m2e1m2"},
    {0xf3400000, "MMAD_MX.e1m2e1m2"},
    {0xf3800000, "MMAD_MX.e1m2e1m2"},
    {0xf3c00000, "MMAD_MX.e1m2e1m2"},
    {0xfa000000, "MMAD_SP u8"},
    {0xfa400000, "MMAD_SP s8"},
    {0xfa800000, "MMAD_SP f16_to_f16"},
    {0xfac00000, "MMAD_SP f16_to_f32"},
    {0xfb400000, "MMAD_SP u8s8"},
    {0xfb800000, "MMAD_SP s4"},
    {0xfa000001, "MMAD_SP s16s8"},
    {0xfa400001, "MMAD_SP bf16_2_f32"},
    {0xfa800001, "MMAD_SP f32_2_f32"},
    {0xfac00001, "MMAD_SP s8s4"},
    {0xfb000001, "MMAD_SP e4m3e4m3"},
    {0xfb400001, "MMAD_SP e4m3e5m2"},
    {0xfb800001, "MMAD_SP e5m2e4m3"},
    {0xfbc00001, "MMAD_SP e5m2e5m2"},
    {0xf6400000, "WINOGRAD_CONV.s8"},
    {0xf6c00000, "WINOGRAD_CONV.s16s8"},
    {0xe4400000, "WINOGRAD_TO_L1.s8"},
    {0xe4c00000, "WINOGRAD_TO_L1.s16s8"},
    {0xe0000000, "CONV_TO_L1.s8s8"},
    {0xe0400000, "CONV_TO_L1.s16s8"},
    {0xe0800000, "CONV_TO_L1.s8s4"},
    {0xe0c00000, "CONV_TO_L1.s4s4"},
    {0xe2000000, "MATMUL_TO_L1.s8s8"},
    {0xe2400000, "MATMUL_TO_L1.s16s8"},
    {0xe2800000, "MATMUL_TO_L1.s8s4"},
    {0xe2c00000, "MATMUL_TO_L1.s4s4"},
};
}
#endif // MSOPT_CUBE_INSTR_TABLE_A5_H
