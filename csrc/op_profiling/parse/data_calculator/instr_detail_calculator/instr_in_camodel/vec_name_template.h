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


#ifndef __MSOPPROF_PARSE_DATA_CALCULATOR_INSTR_DETAIL_VEC_INSTR_TEMPLATE_MAP_H__
#define __MSOPPROF_PARSE_DATA_CALCULATOR_INSTR_DETAIL_VEC_INSTR_TEMPLATE_MAP_H__

#include <cstdint>

namespace Profiling {
namespace Parse {


enum class VecInstrTemplate : uint8_t {
    LD_VAD = 1,
    VAADD = 2,
    VABS = 1,
    VADD = 2,
    VADDDEQRELU  = 2,
    VADDRELU = 2,
    VADDRELUCONV = 2,
    VADDS = 1,
    VAND = 2,
    VAXPY = 2,
    VBRCB = 1,
    VBS16 = 1,
    VBS32 = 2,
    VCADD = 5,
    VCBD = 1,
    VCGADD = 5,
    VCGMAX = 5,
    VCGMIN = 5,
    VCMAX = 5,
    VCMIN = 5,
    VCMP = 2,
    VCONCAT = 10,
    VCONV = 1,
    VCOPY = 1,
    VCPADD = 5,
    VDIV = 2,
    VDP = 10,
    VEXP = 1,
    VEXTRACT = 10,
    VGATHER = 1,
    VGATHERB = 1,
    VIOU = 2,
    VLN = 1,
    VLRELU = 1,
    VMADD = 2,
    VMADDRELU = 2,
    VMAX = 2,
    VMAXS = 1,
    VMERGECH = 1,
    VMIN = 2,
    VMINS = 1,
    VMLA = 2,
    VMOVMASK_CMPMASK_XN = 10,
    VMS4 = 10,
    VMUL = 2,
    VMULCONV = 2,
    VMULS = 1,
    VNOT = 1,
    VOR = 2,
    VPADDING = 1,
    VREC = 1,
    VREDUCE = 10,
    VREDUCEV2 = 10,
    VRELU = 1,
    VRPAC = 1,
    VRSQRT = 1,
    VSCATTER = 1,
    VSEL = 2,
    VSHL = 1,
    VSHR = 1,
    VSQRT = 1,
    VSUB = 2,
    VSUBRELU = 2,
    VSUBRELUCONV = 2,
    VTRANSPOSE = 1,
};

}
}
#endif // __MSOPPROF_PARSE_DATA_CALCULATOR_INSTR_DETAIL_VEC_INSTR_TEMPLATE_MAP_H__
