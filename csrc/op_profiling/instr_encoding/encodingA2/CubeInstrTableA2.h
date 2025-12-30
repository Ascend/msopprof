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


#ifndef MSOPT_CUBE_INSTR_TABLE_A2_H
#define MSOPT_CUBE_INSTR_TABLE_A2_H

#include <unordered_map>
#include <string>

namespace Encode {
const std::unordered_map<uint32_t, std::string> INSTR41_TABLE0_A2 = {
    {0xe0000000, "MMAD u8"},
    {0xe0400000, "MMAD s8"},
    {0xe0800000, "MMAD f16_to_f16"},
    {0xe0c00000, "MMAD f16_to_f32"},
    {0xe1400000, "MMAD u8s8"},
    {0xe1800000, "MMAD s4"},
    {0xe0400001, "MMAD bf16_2_f32"},
    {0xe0800001, "MMAD f32_2_f32"},
    {0xea000000, "MMAD_SP u8"},
    {0xea400000, "MMAD_SP s8"},
    {0xea800000, "MMAD_SP f16_to_f16"},
    {0xeac00000, "MMAD_SP f16_to_f32"},
    {0xeb400000, "MMAD_SP u8s8"},
    {0xeb800000, "MMAD_SP s4"},
    {0xea400001, "MMAD_SP bf16_2_f32"},
    {0xea800001, "MMAD_SP f32_2_f32"}
};
}
#endif // MSOPT_CUBE_INSTR_TABLE_A2_H
