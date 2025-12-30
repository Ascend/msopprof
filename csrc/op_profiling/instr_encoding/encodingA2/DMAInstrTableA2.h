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


#ifndef MSOPT_DMA_INSTR_TABLE_A2_H
#define MSOPT_DMA_INSTR_TABLE_A2_H

#include <unordered_map>
#include <string>


namespace Encode {
const std::unordered_map<uint32_t, std::string> INSTR2_TABLE4_A2 = {
    {0x61400000, "SET_RAWHEADER_TO_OUT"},
    {0x61800000, "DECOMPRESS_HEADER"},
    {0x61c00000, "UNZIP_INDEX"},
    {0x62000000, "LOAD_OUT_TO_L1_IMAGE"},
    {0x62400000, "COMPRESS_UB_TO_OUT"},
    {0x62800000, "LOAD_L1_TO_L0B_WINOGRAD"},
    {0x62c00000, "LOAD_L1_TO_L0A_WINOGRAD"},
    {0x6e000000, "LOAD_L1_TO_L0B_2D_SP"}
};

const std::unordered_map<uint32_t, std::string> INSTR2_TABLE5_A2 = {
    {0x68000000, "HEBCD_OUT_TO_UB"},
    {0x68400000, "HEBCE_{SRC}_TO_OUT"},
    {0x68800000, "LOAD_L1_TO_L0A_WINOGRAD_V2"},
    {0x68c00000, "LOAD_L1_TO_L0B_WINOGRAD_V2"},
};

const std::unordered_map<uint32_t, std::string> INSTR2_TABLE6_A2 = {
    {0x6b800000, "MOV_OUT_TO_UB_ALIGN"},
    {0x6bc00000, "MOV_UB_TO_OUT_ALIGN"}
};

const std::unordered_map<uint32_t, std::string> INSTR9_TABLE2_A2 = {
    {0xc0000000, "MOV_L1_TO_FB"},
    {0xc2000000, "FIX_L0C_TO_OUT"},
    {0xc3000000, "FIX_L0C_TO_L1"}
};

const std::unordered_map<uint32_t, std::string> INSTR19_TABLE0_A2 = {
    {0x60000000, "LOAD_L1_TO_L0A_2D"},
    {0x60000001, "LOAD_L1_TO_L0B_2D"},
    {0x60000002, "LOAD_L1_TO_L1_2D"},
    {0x60000010, "LOAD_OUT_TO_L0A_2D"},
    {0x60000011, "LOAD_OUT_TO_L0B_2D"},
    {0x60000012, "LOAD_OUT_TO_L1_2D"}
};

const std::unordered_map<uint32_t, std::string> INSTR20_TABLE0_A2 = {
    {0x60c00000, "BRC_UB_TO_L0C16"},
    {0x60c00001, "BRC_UB_TO_L0C32"},
    {0x60c00004, "BRC_L1_TO_L0C16"},
    {0x60c00005, "BRC_L1_TO_L0C32"}
};

const std::unordered_map<uint32_t, std::string> INSTR22_TABLE1_A2 = {
    {0x61000000, "DECOMPRESS_OUT_TO_UB"},
    {0x61000001, "DECOMPRESS_OUT_TO_L1"},
    {0x6d000001, "LOAD_L1_TO_L0B_2D_TRANSPOSE.b8"},
    {0x6d400001, "LOAD_L1_TO_L0B_2D_TRANSPOSE.f16"},
    {0x6d800001, "LOAD_L1_TO_L0B_2D_TRANSPOSE.b4"},
    {0x6dc00001, "LOAD_L1_TO_L0B_2D_TRANSPOSE.b32"},
    {0x6d000000, "LOAD_L1_TO_L0A_2D_TRANSPOSE.b8"},
    {0x6d400000, "LOAD_L1_TO_L0A_2D_TRANSPOSE.f16"},
    {0x6d800000, "LOAD_L1_TO_L0A_2D_TRANSPOSE.b4"},
    {0x6dc00000, "LOAD_L1_TO_L0A_2D_TRANSPOSE.b32"},
};

const std::unordered_map<uint32_t, std::string> INSTR23_TABLE0_A2 = {
    {0x63000000, "LOAD_L1_TO_L0A_3D"},
    {0x63000002, "LOAD_L1_TO_L0B_3D"},
    {0x63800000, "LOAD_L1_TO_UB_3D"}
};

const std::unordered_map<uint32_t, std::string> INSTR23_TABLE1_A2 = {
    {0x65000000, "LOAD_L1_TO_L0A_3Dv2"},
    {0x65000002, "LOAD_L1_TO_L0B_3Dv2"},
    {0x65800000, "LOAD_L1_TO_UB_3Dv2"},
    {0x67000000, "LOAD_L1_TO_L0A_3Dv2"},
    {0x67000002, "LOAD_L1_TO_L0B_3Dv2"},
    {0x67800000, "LOAD_L1_TO_UB_3Dv2"}
};

const std::unordered_map<uint32_t, std::string> INSTR24_TABLE0_A2 = {
    {0x64000000, "LOAD_L1_TO_L0B_B2"}
};

const std::unordered_map<uint32_t, std::string> INSTR25_TABLE0_A2 = {
    {0x64400000, "LOAD_OUT_TO_SMASK"},
    {0x64400001, "LOAD_UB_TO_SMASK"},
    {0x64400002, "LOAD_L1_TO_SMASK"}
};

const std::unordered_map<uint32_t, std::string> INSTR26_TABLE0_A2 = {
    {0x70000000, "MOV_L0C16_TO_L0C16"},
    {0x70000008, "MOV_L0C16_TO_UB"},
    {0x70000010, "MOV_L0C16_TO_OUT"},
    {0x70000018, "MOV_L0C16_TO_L0C32"},
    {0x70000020, "MOV_L0C16_TO_L1"},
    {0x70000028, "MOV_L0C16_TO_BT"},
    {0x70800000, "MOV_UB_TO_L0C16"},
    {0x70800008, "MOV_UB_TO_UB"},
    {0x70800010, "MOV_UB_TO_OUT"},
    {0x70800018, "MOV_UB_TO_L0C32"},
    {0x70800020, "MOV_UB_TO_L1"},
    {0x70800028, "MOV_UB_TO_BT"},
    {0x71000000, "MOV_OUT_TO_L0C16"},
    {0x71000008, "MOV_OUT_TO_UB"},
    {0x71000010, "MOV_OUT_TO_OUT"},
    {0x71000018, "MOV_OUT_TO_L0C32"},
    {0x71000020, "MOV_OUT_TO_L1"},
    {0x71000028, "MOV_OUT_TO_BT"},
    {0x71800000, "MOV_L0C32_TO_L0C16"},
    {0x71800008, "MOV_L0C32_TO_UB"},
    {0x71800010, "MOV_L0C32_TO_OUT"},
    {0x71800018, "MOV_L0C32_TO_L0C32"},
    {0x71800020, "MOV_L0C32_TO_L1"},
    {0x71800028, "MOV_L0C32_TO_BT"},
    {0x72000000, "MOV_L0C32_TO_L0C16"},
    {0x72000008, "MOV_L1_TO_UB"},
    {0x72000010, "MOV_L1_TO_OUT"},
    {0x72000018, "MOV_L1_TO_L0C32"},
    {0x72000020, "MOV_L1_TO_L1"},
    {0x72000028, "MOV_L1_TO_BT"},
};

const std::unordered_map<uint32_t, std::string> INSTR27_TABLE0_A2 = {
    {0x69000000, "STORE_L1_TO_OUT_IMAGE"},
    {0x69800000, "MOV_OUT_TO_L1_PAD"},
    {0x6a000000, "MVF_OUT_TO_UB"},
    {0x6a800000, "MVF_DCI"},
    {0x6b000000, "MOV_OUT_TO_L1_MULTI_ND2NZ"}
};
const std::unordered_map<uint32_t, std::string> INSTR28_TABLE0_A2 = {
    {0x60400000, "SET_L0A_2D.b16"},
    {0x60400004, "SET_L0A_2D.b32"},
    {0x60400001, "SET_L0B_2D.b16"},
    {0x60400005, "SET_L0B_2D.b32"},
    {0x60400002, "SET_L1_2D.b16"},
    {0x60400006, "SET_L1_2D.b32"},
    {0x60800000, "LOAD_OUT_TO_L0A_unzip"},
    {0x60800001, "LOAD_OUT_TO_L0B_unzip"},
    {0x60800002, "LOAD_OUT_TO_L1_unzip"}
};
}
#endif // MSOPT_DMA_INSTR_TABLE_A2_H
