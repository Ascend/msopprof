
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


#ifndef MSOPT_VECTOR_INSTR_TABLE_A5_H
#define MSOPT_VECTOR_INSTR_TABLE_A5_H
#include <string>
#include <unordered_map>
#include "common/defs.h"
namespace Encode {
const std::unordered_map<uint32_t, std::string> INSTR9_TABLE1_A5 = {
    {0x43000000, "PUSH_PB"},
};

const std::unordered_map<uint32_t, std::string> INSTR27_TABLE1_A5 = {
    {0x15000000, "PUSHQS1"},
    {0x15200000, "PUSHQS1"},
    {0x15400000, "PUSHQS1"},
    {0x15600000, "PUSHQS1"},
    {0x15800000, "PUSHQS2"},
    {0x15a00000, "PUSHQS2"},
    {0x15c00000, "PUSHQS2"},
    {0x15e00000, "PUSHQS2"},
};

const std::unordered_map<uint32_t, std::string> INSTR44_TABLE0_A5 = {
    {0x15e08000, "SET_FLAG.V.triggerp #id"},
    {0x15e00000, "WAIT_FLAG.pipe.V #id"},
};

const std::unordered_map<uint32_t, std::string> INSTR45_TABLE0_A5 = {
    {0x15c08000, "SET_FLAG.V.triggerp Xt"},
    {0x15c00000, "WAIT_FLAG.pipe.V Xt"},
    {0x15e08001, "GET_BUFI_V #buf_id"},
    {0x15e00001, "RLS_BUFI_V #buf_id"},
    {0x15c08001, "GET_BUF_V Xt"},
    {0x15c00001, "RLS_BUF_V Xt"},
    {0x15e08002, "SET_INTRA_BLOCKI_V #id"},
    {0x15e00002, "WAIT_INTRA_BLOCKI_V #id"},
    {0x15c08002, "SET_INTRA_BLOCK_V Xt"},
    {0x15c00002, "WAIT_INTRA_BLOCK_V Xt"},
    {0x15e08003, "WAIT_FLAG_DEVI_V #uimm8"},
    {0x15c08003, "SET_CROSS_CORE_V Xt"},
    {0x15c00003, "WAIT_FLAG_DEV_V Xt"},
    {0x15c00004, "DFX_REGION_V Xt"},
};
const std::unordered_map<uint32_t, std::string> INSTR46_TABLE0_A5 = {
    {0x15c0000c, "RELEASE_PBID Xn"},
    {0x15c0000d, "MOV #v_spr_id, Xn"},
    {0x15a00011, "MOVEVA VAd[n], Xn, Xm"},
    {0x15c00012, "LD VAd, [Xn], #h"},
    {0x15c00013, "MOVEMASK MASK[n], Xn"},
    {0x15c00014, "VNCHWCONV.type [VAd], [VAn], Xt, #dstH, #srcH"},
    {0x15a00017, "RPN_COR_DIAG2.type [Xd], [Xn], RPN_COR_IR"},
    {0x15a00019, "VTRANSPOSE.type [Xd], [Xn]"},
    {0x1580001a, "MOV_UB_TO_UB [Xd], [Xn], Xm"},
};

const std::unordered_map<uint32_t, std::string> INSTR47_TABLE0_A5 = {
    {0x1580000f, "FMAX [Xd], [Xn], Xm"},
    {0x15800010, "FMIN [Xd], [Xn], Xm"},
};

const std::unordered_map<uint64_t, std::string> INSTR_VEC_TABLE0_A5 = {
    {0x15e0000415600000, "VFI #offset, #instr_num"},
    {0x15e0000515400000, "VF Xn, #instr_num"},
    {0x15e0000615400000, "VFI_RU Xm, #offset, #instr_num"},
    {0x15e0000715200000, "VF_RU Xn, Xm, #instr_num"},
    {0x15e0000815600000, "VFI_BC #vpc_offset, #instr_num, #temp, #blk_strd_en"},
    {0x15e0000915400000, "VF_BC Xn, #instr_num, #temp, #blk_strd_en"},
    {0x15e0000a15600000, "VFI_PREFETCH  #offset, #instr_num"},
    {0x15e0000b15400000, "VF_PREFETCH Xn, #instr_num"},
    {0x15c0000e15000000, "FIFR1.type [Xd], [Xn], Xm, Xt"},
    {0x15c0001515000000, "VBS32.type [Xd], [Xn], [Xm], Xt"},
    {0x15c0001615000000, "VMS4v2.type [Xd], [Xn], Xm, Xt"},
    {0x15e0001815000000, "V4DTRANS.type [Xd], [Xn], Xt"},
    {0x15e0001c15200000, "VF_SIMT Xn, Xm, #instr_num"},
};

const std::unordered_map<uint64_t, std::string> INSTR_VEC_TABLE1_A5 = {
    {0x0000000000000000, "LDG"},
    {0x0000000000040000, "LDS"},
    {0x0000000000080000, "LDK"},
    {0x0000000000200000, "LD"},
};

const std::unordered_map<uint64_t, std::string> INSTR_VEC_TABLE2_A5 = {
    {0x000000001c0c0000, "STG"},
    {0x000000001c100000, "STS"},
    {0x000000001c140000, "STK"},
    {0x000000001c1c0000, "DCCI"},
    {0x000000001c240000, "ST"},
    {0x000000001c180000, "RED"},
};

const std::unordered_map<Common::Enc128, std::string, Common::Enc128Hash> INSTR_VEC_TABLE3_A5 = {
    {{0x0000000000000001, 0x0000000000000001}, "ATOM"},
};

const std::unordered_map<Common::Enc128, std::string, Common::Enc128Hash> INSTR_VEC_TABLE4_A5 = {
    {{0x0000000000020001, 0x0000000000000001}, "FFMA.i"},
};

const std::unordered_map<uint64_t, std::string> INSTR_VEC_TABLE5_A5 = {
    {0x0000000000020000, "FFMA"},
};

const std::unordered_map<Common::Enc128, std::string, Common::Enc128Hash> INSTR_VEC_TABLE6_A5 = {
    {{0x00000000000a0001, 0x0000000000000001}, "FSETP.i"},
};

const std::unordered_map<uint64_t, std::string> INSTR_VEC_TABLE7_A5 = {
    {0x00000000000a0000, "FSETP"},
};

const std::unordered_map<Common::Enc128, std::string, Common::Enc128Hash> INSTR_VEC_TABLE8_A5 = {
    {{0x0000000000120001, 0x0000000000000001}, "FMUL.i"},
    {{0x00000000001a0001, 0x0000000000000001}, "FADD.i"},
    {{0x00000000002a0001, 0x0000000000000001}, "FMNMX.i"},
    {{0x00000000005a0001, 0x0000000000000001}, "FDIV.i"},
    {{0x0000000000860001, 0x0000000000000001}, "IMAD.X"},
    {{0x0000000001860001, 0x0000000000000001}, "IMAD.X.i"},
    {{0x0000000002860001, 0x0000000000000001}, "IMUL.i"},
    {{0x00000000000e0001, 0x0000000000000001}, "IADD.i"},
    {{0x00000000008e0001, 0x0000000000000001}, "IADD.X.i"},
    {{0x00000000018e0001, 0x0000000000000001}, "ISETP.i"},
};

const std::unordered_map<uint64_t, std::string> INSTR_VEC_TABLE9_A5 = {
    {0x0000000000120000, "FMUL"},
    {0x00000000001a0000, "FADD"},
    {0x00000000002a0000, "FMNMX"},
    {0x0000000000320000, "F2I"},
    {0x00000000003a0000, "I2F"},
    {0x00000000004a0000, "F2F"},
    {0x0000000000520000, "MUFU"},
    {0x00000000005a0000, "FDIV"},
    {0x00000000006a0000, "FEXPDIF"},
    {0x0000000000720000, "FMULSCVT"},
    {0x0000000002860000, "IMUL"},
    {0x00000000000e0000, "IADD"},
    {0x00000000008e0000, "IADD.X"},
    {0x00000000018e0000, "ISETP"},
};

const std::unordered_map<uint64_t, std::string> INSTR_VEC_TABLE10_A5 = {
    {0x0000000000060000, "IMAD"},
    {0x0000000000160000, "MOVI"},
};

const std::unordered_map<Common::Enc128, std::string, Common::Enc128Hash> INSTR_VEC_TABLE11_A5 = {
    {{0x0000000000060001, 0x0000000000000001}, "FMUL.i"},
};

const std::unordered_map<Common::Enc128, std::string, Common::Enc128Hash> INSTR_VEC_TABLE12_A5 = {
    {{0x00000000020e0001, 0x0000000000000001}, "IMNMX.i"},
    {{0x00000000160e0001, 0x0000000000000001}, "IDIV.i"},
};

const std::unordered_map<uint64_t, std::string> INSTR_VEC_TABLE13_A5 = {
    {0x00000000020e0000, "IMNMX"},
    {0x00000000060e0000, "LEA"},
    {0x000000000a0e0000, "LEA.HI.X"},
    {0x000000000e0e0000, "SHF"},
    {0x00000000120e0000, "SHFI"},
    {0x00000000160e0000, "IDIV"},
    {0x000000001a0e0000, "PLop3"},
};

const std::unordered_map<Common::Enc128, std::string, Common::Enc128Hash> INSTR_VEC_TABLE14_A5 = {
    {{0x00000000028e0001, 0x0000000000000001}, "Lop3"},
    {{0x00000000068e0001, 0x0000000000000001}, "Lop3.i"},
};

const std::unordered_map<uint64_t, std::string> INSTR_VEC_TABLE16_A5 = {
    {0x00000000001e0000, "PRmT"},
};

const std::unordered_map<uint64_t, std::string> INSTR_VEC_TABLE17_A5 = {
    {0x00000000009e0000, "MOV"},
    {0x00000000019e0000, "SEL"},
    {0x00000000029e0000, "S2R"},
    {0x00000000039e0000, "P2R"},
    {0x00000000049e0000, "R2P"},
    {0x00000000059e0000, "CLZ"},
    {0x00000000069e0000, "PopC"},
    {0x00000000079e0000, "BREV"},
    {0x00000000089e0000, "S2RL"},
    {0x0000000000260000, "START_DVG"},
    {0x0000000000a60000, "END_DVG"},
    {0x0000000001260000, "BRANCH"},
    {0x0000000001a60000, "BRANCH_IND"},
    {0x0000000002260000, "CALL"},
    {0x0000000006260000, "RET"},
    {0x0000000006a60000, "END"},
    {0x0000000007260000, "CALL_IND"},
    {0x0000000000360000, "VOTE"},
    {0x0000000000b60000, "VOTE.ballot"},
    {0x0000000001360000, "SHFL"},
    {0x0000000001b60000, "REDUX"},
    {0x00000000002e0000, "Nop"},
    {0x0000000000ae0000, "BAR.THREAD_BLOCK"},
    {0x00000000012e0000, "JOIN"},
    {0x0000000001ae0000, "MEMBAR"},
};

const std::unordered_map<Common::Enc128, std::string, Common::Enc128Hash> INSTR_VEC_TABLE18_A5 = {
    {{0x0000000005260001, 0x0000000000000001}, "CALLI"},
};
}
#endif // MSOPT_VECTOR_INSTR_TABLE_A5_H