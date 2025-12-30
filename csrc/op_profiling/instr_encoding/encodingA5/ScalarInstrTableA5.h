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


#ifndef MSOPT_SCALAR_INSTR_TABLE_A5_H
#define MSOPT_SCALAR_INSTR_TABLE_A5_H

#include <string>
#include <unordered_map>

namespace Encode {
const std::unordered_map<uint32_t, std::string> INSTR0_TABLE0_A5 = {
    {0x00000001, "ADD.s64"},
    {0x00400001, "ADD.u64"},
    {0x00800001, "ADD.f32"},
    {0x00c00001, "ADD.f64"},
    {0x00000002, "SUB.s64"},
    {0x00400002, "SUB.u64"},
    {0x00800002, "SUB.f32"},
    {0x00c00002, "SUB.f64"},
    {0x00000003, "MUL.s64"},
    {0x00400003, "MUL.u64"},
    {0x00800003, "MUL.f32"},
    {0x00c00003, "MUL.f64"},
    {0x00000004, "MADD.s64"},
    {0x00400004, "MADD.u64"},
    {0x00800004, "MADD.f32"},
    {0x00c00004, "MADD.f64"},
    {0x00000005, "DIV.s64"},
    {0x00400005, "DIV.u64"},
    {0x00800005, "DIV.f32"},
    {0x00c00005, "DIV.f64"},
    {0x00000006, "REM.s64"},
    {0x00400006, "REM.u64"},
    {0x00800006, "REM.f32"},
    {0x00c00006, "REM.f64"},
    {0x00000007, "MAX.s64"},
    {0x00400007, "MAX.u64"},
    {0x00800007, "MAX.f32"},
    {0x00c00007, "MAX.f64"},
    {0x00000008, "MIN.s64"},
    {0x00400008, "MIN.u32"},
    {0x00800008, "MIN.f32"},
    {0x00c00008, "MIN.f64"},
    {0x00000009, "SEL.b8"},
    {0x00400009, "SEL.b16"},
    {0x00800009, "SEL.b32"},
    {0x00c00009, "SEL.b64"},
    {0x0000000a, "AND.b8"},
    {0x0040000a, "AND.b16"},
    {0x0080000a, "AND.b32"},
    {0x00c0000a, "AND.b64"},
    {0x0000000b, "OR.b8"},
    {0x0040000b, "OR.b16"},
    {0x0080000b, "OR.b32"},
    {0x00c0000b, "OR.b64"},
    {0x0000000c, "XOR.b8"},
    {0x0040000c, "XOR.b16"},
    {0x0080000c, "XOR.b32"},
    {0x00c0000c, "XOR.b64"}
};

const std::unordered_map<uint32_t, std::string> INSTR1_TABLE0_A5 = {
    {0x0000000e, "CMP.EQ.s64"},
    {0x0000001e, "CMP.NE.s64"},
    {0x0000002e, "CMP.LT.s64"},
    {0x0000003e, "CMP.GT.s64"},
    {0x0000004e, "CMP.GE.s64"},
    {0x0000005e, "CMP.LE.s64"},
    {0x0040000e, "CMP.EQ.u64"},
    {0x0040001e, "CMP.NE.u64"},
    {0x0040002e, "CMP.LT.u64"},
    {0x0040003e, "CMP.GT.u64"},
    {0x0040004e, "CMP.GE.u64"},
    {0x0040005e, "CMP.LE.u64"},
    {0x0080000e, "CMP.EQ.f32"},
    {0x0080001e, "CMP.NE.f32"},
    {0x0080002e, "CMP.LT.f32"},
    {0x0080003e, "CMP.GT.f32"},
    {0x0080004e, "CMP.GE.f32"},
    {0x0080005e, "CMP.LE.f32"},
    {0x00c0000e, "CMP.EQ.f64"},
    {0x00c0001e, "CMP.NE.f64"},
    {0x00c0002e, "CMP.LT.f64"},
    {0x00c0003e, "CMP.GT.f64"},
    {0x00c0004e, "CMP.GE.f64"},
    {0x00c0005e, "CMP.LE.f64"},
    {0x0000000f, "CMPN.EQ.s64"},
    {0x0000001f, "CMPN.NE.s64"},
    {0x0000002f, "CMPN.LT.s64"},
    {0x0000003f, "CMPN.GT.s64"},
    {0x0000004f, "CMPN.GE.s64"},
    {0x0000005f, "CMPN.LE.s64"},
    {0x0040000f, "CMPN.EQ.u64"},
    {0x0040001f, "CMPN.NE.u64"},
    {0x0040002f, "CMPN.LT.u64"},
    {0x0040003f, "CMPN.GT.u64"},
    {0x0040004f, "CMPN.GE.u64"},
    {0x0040005f, "CMPN.LE.u64"},
    {0x0080000f, "CMPN.EQ.f32"},
    {0x0080001f, "CMPN.NE.f32"},
    {0x0080002f, "CMPN.LT.f32"},
    {0x0080003f, "CMPN.GT.f32"},
    {0x0080004f, "CMPN.GE.f32"},
    {0x0080005f, "CMPN.LE.f32"},
    {0x00c0000f, "CMPN.EQ.f64"},
    {0x00c0001f, "CMPN.NE.f64"},
    {0x00c0002f, "CMPN.LT.f64"},
    {0x00c0003f, "CMPN.GT.f64"},
    {0x00c0004f, "CMPN.GE.f64"},
    {0x00c0005f, "CMPN.LE.f64"},
};

const std::unordered_map<uint32_t, std::string> INSTR2_TABLE0_A5 = {
    {0x0a000000, "CMPI.EQ"},
    {0x0a400000, "CMPI.NE"},
    {0x0a800000, "CMPI.LT"},
    {0x0ac00000, "CMPI.GT"},
    {0x0b000000, "CMPI.GE"},
    {0x0b400000, "CMPI.LE"},
    {0x0e000000, "STI.b8"},
    {0x0e400000, "STI.b16"},
    {0x0e800000, "STI.b32"},
    {0x0ec00000, "STI.b64"},
    {0x0f000000, "STI_IO.b8"},
    {0x0f400000, "STI_IO.b16"},
    {0x0f800000, "STI_IO.b32"},
    {0x0fc00000, "STI_IO.b64"},
    {0x03000000, "ST_IO.b8"},
    {0x13000000, "ST_IO.b8"},
    {0x03400000, "ST_IO.b16"},
    {0x13400000, "ST_IO.b16"},
    {0x03800000, "ST_IO.b32"},
    {0x13800000, "ST_IO.b32"},
    {0x03c00000, "ST_IO.b64"},
    {0x13c00000, "ST_IO.b64"},
    {0x08000000, "ADDI"},
    {0x08400000, "MULI"},
    {0x08800000, "SUBI"},
    {0x08c00000, "DC_PRELOADI"},
};

const std::unordered_map<uint32_t, std::string> INSTR3_TABLE0_A5 = {
    {0x01000000, "LD.b8"},
    {0x01400000, "LD.b16"},
    {0x01800000, "LD.b32"},
    {0x01c00000, "LD.b64"},
    {0x01000020, "ST.b8"},
    {0x01400020, "ST.b16"},
    {0x01800020, "ST.b32"},
    {0x01c00020, "ST.b64"},
    {0x01000050, "DC_PRELOAD"},
    {0x01000070, "ST_ATOMIC.b8"},
    {0x01400070, "ST_ATOMIC.b16"},
    {0x01800070, "ST_ATOMIC.b32"},
    {0x01c00070, "ST_ATOMIC.b64"},
};

const std::unordered_map<uint32_t, std::string> INSTR4_TABLE0_A5 = {
    {0x02000000, "SQRT.s64"},
    {0x02400000, "SQRT.u64"},
    {0x02800000, "SQRT.f32"},
    {0x02c00000, "SQRT.f64"},
    {0x02000080, "NEG.s64"},
    {0x02400080, "NEG.u64"},
    {0x02800080, "NEG.f32"},
    {0x02c00080, "NEG.f64"},
    {0x02000100, "ABS.s64"},
    {0x02400100, "ABS.u64"},
    {0x02800100, "ABS.f32"},
    {0x02c00100, "ABS.f64"},
    {0x02000180, "NOT.s64"},
    {0x02400180, "NOT.u64"},
    {0x02800180, "NOT.f32"},
    {0x02c00180, "NOT.f64"},
    {0x02000480, "CLZ.b8"},
    {0x02400480, "CLZ.b16"},
    {0x02800480, "CLZ.b32"},
    {0x02c00480, "CLZ.b64"},
    {0x02000500, "SFLBITS.s64"},
    {0x02400500, "SFLBITS.u64"},
    {0x02800500, "SFLBITS.f32"},
    {0x02c00500, "SFLBITS.f64"},
    {0x02000980, "SIGNEXT.s8"},
    {0x02400980, "SIGNEXT.s16"},
    {0x02800980, "SIGNEXT.s32"},
    {0x02000a00, "ZEROEXT.u8"},
    {0x02400a00, "ZEROEXT.u16"},
    {0x02800a00, "ZEROEXT.u32"}
};

const std::unordered_map<uint32_t, std::string> INSTR5_TABLE0_A5 = {
    {0x02000b00, "INSERTI"},
    {0x02000c00, "INSERT"},
    {0x02000d00, "INSERT"}
};

const std::unordered_map<uint32_t, std::string> INSTR6_TABLE0_A5 = {
    {0x02000580, "CONV.f322s32a"},
    {0x02000581, "CONV.f322s32f"},
    {0x02000582, "CONV.f322s32c"},
    {0x02000583, "CONV.f322s32z"},
    {0x02000584, "CONV.f322s32r"},
    {0x02000585, "CONV.s322f32"},
    {0x02000586, "CONV.f322f16"},
    {0x02000587, "CONV.f162f32"},
    {0x02000588, "CONV.f322f16o"},
    {0x02000589, "CONV.f642f32"},
    {0x0200058a, "CONV.f322f64"}
};

const std::unordered_map<uint32_t, std::string> INSTR7_TABLE0_A5 = {
    {0x02000200, "SHLI.b8"},
    {0x02400200, "SHLI.b16"},
    {0x02800200, "SHLI.b32"},
    {0x02c00200, "SHLI.b64"},
    {0x02000240, "SHL.b8"},
    {0x02400240, "SHL.b16"},
    {0x02800240, "SHL.b32"},
    {0x02c00240, "SHL.b64"},
    {0x02000280, "SHRI.s64"},
    {0x02400280, "SHRI.u64"},
    {0x02800280, "SHRI.f32"},
    {0x02c00280, "SHRI.f64"},
    {0x020002c0, "SHR.s64"},
    {0x024002c0, "SHR.u64"},
    {0x028002c0, "SHR.f32"},
    {0x02c002c0, "SHR.f64"},
    {0x02000300, "SBCNT0.b8"},
    {0x02400300, "SBCNT0.b16"},
    {0x02800300, "SBCNT0.b32"},
    {0x02c00300, "SBCNT0.b64"},
    {0x02000340, "SBCNT1.b8"},
    {0x02400340, "SBCNT1.b16"},
    {0x02800340, "SBCNT1.b32"},
    {0x02c00340, "SBCNT1.b64"},
    {0x02000380, "SFF0.b8"},
    {0x02400380, "SFF0.b16"},
    {0x02800380, "SFF0.b32"},
    {0x02c00380, "SFF0.b64"},
    {0x020003c0, "SFF1.b8"},
    {0x024003c0, "SFF1.b16"},
    {0x028003c0, "SFF1.b32"},
    {0x02c003c0, "SFF1.b64"},
    {0x02000400, "SBITSET0.b8"},
    {0x02400400, "SBITSET0.b16"},
    {0x02800400, "SBITSET0.b32"},
    {0x02c00400, "SBITSET0.b64"},
    {0x02000440, "SBITSET1.b8"},
    {0x02400440, "SBITSET1.b16"},
    {0x02800440, "SBITSET1.b32"},
    {0x02c00440, "SBITSET1.b64"}
};

const std::unordered_map<uint32_t, std::string> INSTR8_TABLE0_A5 = {
    {0x02000800, "MOV"},
    {0x02000880, "MOV_SPR2X"},
    {0x02000900, "MOV_X2SPR"},
};

const std::unordered_map<uint32_t, std::string> INSTR10_TABLE0_A5 = {
    {0x07000000, "MOVI"},
    {0x07010000, "MOVK"},
    {0x10000000, "MOVX8"},
};

const std::unordered_map<uint32_t, std::string> INSTR22_TABLE0_A5 = {

    {0x09000001, "STP.b8"},
    {0x09400001, "STP.b16"},
    {0x09800001, "STP.b32"},
    {0x09c00001, "STP.b64"},
};

const std::unordered_map<uint32_t, std::string> INSTR42_TABLE0_A5 = {
    {0x1c000000, "LD_IO.b8"},
    {0x1e000000, "LD_IO.b8"},
    {0x1c400000, "LD_IO.b16"},
    {0x1e400000, "LD_IO.b16"},
    {0x1c800000, "LD_IO.b32"},
    {0x1e800000, "LD_IO.b32"},
    {0x1cc00000, "LD_IO.b64"},
    {0x1ec00000, "LD_IO.b64"},
    {0x16000000, "ST_ATOMIC.b8"},
    {0x16400000, "ST_ATOMIC.b16"},
    {0x16800000, "ST_ATOMIC.b32"},
    {0x16c00000, "ST_ATOMIC.b64"},
    {0x1a000000, "LD_DEV.b8"},
    {0x1a400000, "LD_DEV.b16"},
    {0x1a800000, "LD_DEV.b32"},
    {0x1ac00000, "LD_DEV.b64"},
    {0x18000000, "ST_DEV.b8"},
    {0x18400000, "ST_DEV.b16"},
    {0x18800000, "ST_DEV.b32"},
    {0x18c00000, "ST_DEV.b64"},
};

const std::unordered_map<uint32_t, std::string> INSTR43_TABLE0_A5 = {
    {0x0c000000, "LDP.b8"},
    {0x0c400000, "LDP.b16"},
    {0x0c800000, "LDP.b32"},
    {0x0cc00000, "LDP.b64"},
};
}
#endif // MSOPT_SCALAR_INSTR_TABLE_A5_H
