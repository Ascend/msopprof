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


#ifndef MSOPT_SIMD_INSTR_TABLE_A2_H
#define MSOPT_SIMD_INSTR_TABLE_A2_H

#include <unordered_map>
#include <string>

namespace Encode {
const std::unordered_map<uint32_t, std::string> INSTR1_TABLE1_A2 = {
    {0x80000002, "VCSSPLIT"},
    {0x8000000a, "VCSSPLIT"},
    {0x80000042, "VGATHER"},
    {0x8000004a, "VGATHER"},
    {0x80000022, "VPADDING"},
    {0x8000002a, "VPADDING"},
    {0x80000062, "VSCATTER"},
    {0x8000006a, "VSCATTER"},
    {0x80000012, "L0C_LOCK_SET"},
    {0x8000001a, "L0C_LOCK_RELEASE"}
};

const std::unordered_map<uint32_t, std::string> INSTR9_TABLE0_A2 = {
    {0x8b000000, "VDIV"}
};

const std::unordered_map<uint32_t, std::string> INSTR9_TABLE1_A2 = {
    {0x95000000, "VAXPY"}
};

const std::unordered_map<uint32_t, std::string> INSTR2_TABLE7_A2 = {
    {0x80400000, "MOVEMASK"},
    {0x80800000, "LD VAd"},
    {0x80c00000, "VCBD"}
};

const std::unordered_map<uint32_t, std::string> INSTR28_TABLE1_A2 = {
    {0x80000000, "MOVEVA"},
    {0x80000001, "VSCMERGE"},
};

const std::unordered_map<uint32_t, std::string> INSTR28_TABLE2_A2 = {
    {0x80000003, "VGATHERB"},
    {0x80000004, "VBRCB"},
    {0x80000007, "VCSHUFFLE"}
};

const std::unordered_map<uint32_t, std::string> INSTR29_TABLE0_A2 = {
    {0x82000000, "MOVEV"},
    {0x82000001, "VCI"},
    {0x82000080, "VEXP"},
    {0x82000100, "VRSQRT"},
    {0x82000180, "VRELU"},
    {0x82000200, "VREC"},
    {0x82000280, "VLN"},
    {0x82000300, "VABS"},
    {0x82000380, "VCADD"},
    {0x82000400, "VCMAX"},
    {0x82000480, "VCMIN"},
    {0x82000500, "VCGMAX"},
    {0x82000580, "VCGMIN"},
    {0x82000600, "VCGADD"},
    {0x82000680, "VNCHWCONV"},
    {0x82000700, "VCOPY"},
    {0x82000780, "VCPADD"},
    {0x82000800, "VNOT"},
    {0x82000880, "VRPAC"},
    {0x82000900, "VBS16"},
    {0x82000980, "VMS4"},
    {0x82000a00, "VEXTRACT"},
    {0x82000a80, "VCONCAT"},
    {0x82000b00, "VMERGECH"},
    {0x82000b80, "RPN_COR_DIAG"}
};

const std::unordered_map<uint32_t, std::string> INSTR29_TABLE1_A2 = {
    {0x82000c00, "VTRANSPOSE"},
    {0x82000c01, "V4DTRANS"}
};

const std::unordered_map<uint32_t, std::string> INSTR30_TABLE0_A2 = {
    {0x82000c80, "VSQRT"}
};

const std::unordered_map<uint32_t, std::string> INSTR31_TABLE0_A2 = {
    {0x84000000, "VADD"},
    {0x84000001, "VSUB"},
    {0x84000002, "VBS32"},
    {0x84000003, "VMS4v2"}
};

const std::unordered_map<uint32_t, std::string> INSTR31_TABLE1_A2 = {
    {0x9c000000, "VSEL"},
    {0x9c000001, "VIOU"},
    {0x9c000002, "VREDUCEV2"}
};

const std::unordered_map<uint32_t, std::string> INSTR31_TABLE2_A2 = {
    {0x9e000000, "VAADD"}
};

const std::unordered_map<uint32_t, std::string> INSTR31_TABLE3_A2 = {
    {0x9e000002, "VMOVMASK [Xd], CMPMASK"}
};

const std::unordered_map<uint32_t, std::string> INSTR32_TABLE0_A2 = {
    {0x86000000, "VMAX"},
    {0x86000001, "VMIN"},
    {0x88000000, "VMUL"},
    {0x88000001, "VMLA"}
};

const std::unordered_map<uint32_t, std::string> INSTR32_TABLE1_A2 = {
    {0x92000001, "VMADDRELU"}
};

const std::unordered_map<uint32_t, std::string> INSTR32_TABLE2_A2 = {
    {0x98000001, "VCMPV"}
};

const std::unordered_map<uint32_t, std::string> INSTR33_TABLE0_A2 = {
    {0x8a000000, "VADDRELUCONV"},
    {0x8a000002, "VSUBRELUCONV"}
};

const std::unordered_map<uint32_t, std::string> INSTR33_TABLE1_A2 = {
    {0x9b000000, "VSHR"}
};

const std::unordered_map<uint32_t, std::string> INSTR34_TABLE0_A2 = {
    {0x8c000000, "VCONV"}
};

const std::unordered_map<uint32_t, std::string> INSTR35_TABLE0_A2 = {
    {0x92000000, "VLRELU"},
    {0x93000000, "VMADD"}
};

const std::unordered_map<uint32_t, std::string> INSTR35_TABLE1_A2 = {
    {0x94000000, "VADDRELU"},
    {0x94000001, "VSUBRELU"},
    {0x96000000, "VMAXS"},
    {0x97000000, "VADDS"},
    {0x96000001, "VMINS"},
    {0x97000001, "VMULS"}
};

const std::unordered_map<uint32_t, std::string> INSTR35_TABLE2_A2 = {
    {0x98000000, "VCMPVEQ"},
    {0x99000000, "VCMP"},
    {0x99000002, "VCMPVSEQ"}
};

const std::unordered_map<uint32_t, std::string> INSTR36_TABLE0_A2 = {
    {0x9a000002, "VCMPVS"}
};

const std::unordered_map<uint32_t, std::string> INSTR37_TABLE0_A2 = {
    {0x9a000000, "VOR"},
    {0x9a000001, "VAND"}
};

const std::unordered_map<uint32_t, std::string> INSTR38_TABLE0_A2 = {
    {0x9c000003, "VDP"},
    {0x9c800003, "VSHL"},
    {0x9d000003, "VREDUCE"},
    {0x9d800003, "VBI"}
};

const std::unordered_map<uint32_t, std::string> INSTR39_TABLE0_A2 = {
    {0x9e000001, "RPN_COR"},
    {0x9e400001, "VMULCONV"},
    {0x9e800001, "VMULCONV"}
};

const std::unordered_map<uint32_t, std::string> INSTR40_TABLE0_A2 = {
    {0x9e000003, "VMOVMASK CMPMASK, [Xn]"},
    {0x9e400003, "VADDDEQRELU"}
};
}
#endif // MSOPT_SIMD_INSTR_TABLE_A2_H
