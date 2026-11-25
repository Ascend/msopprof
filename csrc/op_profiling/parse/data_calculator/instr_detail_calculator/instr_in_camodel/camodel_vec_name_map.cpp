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


#include "camodel_vec_name_map.h"
#include <unordered_set>

namespace Profiling {
namespace Parse {

namespace {
// Ascend950 computation instructions. All entries use the same processed-bytes formula.
const std::unordered_set<std::string> SIMD_VF_COMPUTE_INSTR_A5 = {"RV_VADD", "RV_VSUB", "RV_VMUL", "RV_VMAX", "RV_VMIN",
    "RV_VABS", "RV_VNEG", "RV_VABSDIF", "RV_VADD3", "RV_VAVG", "RV_VSADD", "RV_VSSUB", "RV_VADDS", "RV_VMULS",
    "RV_VMAXS", "RV_VMINS", "RV_VMAXRELU", "RV_VMINRELU", "RV_VRELU", "RV_VPRELU", "RV_VLRELU", "RV_VAXPY", "RV_VMULA",
    "RV_VMADD", "RV_VAND", "RV_VOR", "RV_VXOR", "RV_VNOT", "RV_VSHLS", "RV_VSHRS", "RV_VSHLI", "RV_VSHRI", "RV_VSQRT",
    "RV_VLN", "RV_VEXP", "RV_VEXP2", "RV_VDIV", "RV_VDIVF", "RV_VREC", "RV_VRSQRT", "RV_VTRC", "RV_VCVTFI", "RV_VCVTFF",
    "RV_VCVTIF", "RV_VCVTII", "RV_VCVTFF2", "RV_VADDC", "RV_VSUBC", "RV_VADDCS", "RV_VSUBCS", "RV_WMUL", "RV_WMULA",
    "RV_VMULL", "RV_VRNDI", "RV_VRND", "RV_VCBMAX", "RV_VCBMIN", "RV_VCADD", "RV_VCMAX", "RV_VCMIN", "RV_VCGADD",
    "RV_VCGMAX", "RV_VCGMIN", "RV_VCPADD", "RV_VADIF", "RV_VSAD", "RV_WADD", "RV_WADDA", "RV_WSUB", "RV_WSUBA",
    "RV_WADDSUB", "RV_WCVT48", "RV_VEXPDIF", "RV_VMULSCVT", "RV_VRNDS", "RV_VCLS", "RV_VBCNT"};
}

bool IsSimdVfComputeInstrA5(const std::string &name) { return SIMD_VF_COMPUTE_INSTR_A5.count(name) != 0; }

InstrLogStr2Template instrLogStr2TemplateA2A3 = {
    {"LD_VAD", VecInstrTemplate::LD_VAD},
    {"VAADD", VecInstrTemplate::VAADD},
    {"VABS", VecInstrTemplate::VABS},
    {"VADD", VecInstrTemplate::VADD},
    {"VADDDEQRELU", VecInstrTemplate::VADDDEQRELU },
    {"VADDRELU", VecInstrTemplate::VADDRELU},
    {"VADDRELUCONV", VecInstrTemplate::VADDRELUCONV},
    {"VADDS", VecInstrTemplate::VADDS},
    {"VAND", VecInstrTemplate::VAND},
    {"VAXPY", VecInstrTemplate::VAXPY},
    {"VBS16", VecInstrTemplate::VBS16},
    {"VCADD", VecInstrTemplate::VCADD},
    {"VCBD", VecInstrTemplate::VCBD},
    {"VCGADD", VecInstrTemplate::VCGADD},
    {"VCGMAX", VecInstrTemplate::VCGMAX},
    {"VCGMIN", VecInstrTemplate::VCGMIN},
    {"VCMAX", VecInstrTemplate::VCMAX},
    {"VCMIN", VecInstrTemplate::VCMIN},
    {"VCMP", VecInstrTemplate::VCMP},
    {"VCONV", VecInstrTemplate::VCONV},
    {"VCPADD", VecInstrTemplate::VCPADD},
    {"VDIV", VecInstrTemplate::VDIV},
    {"VDP", VecInstrTemplate::VDP},
    {"VEXP", VecInstrTemplate::VEXP},
    {"VEXTRACT", VecInstrTemplate::VEXTRACT},
    {"VGATHER", VecInstrTemplate::VGATHER},
    {"VIOU", VecInstrTemplate::VIOU},
    {"VLN", VecInstrTemplate::VLN},
    {"VLRELU", VecInstrTemplate::VLRELU},
    {"VMADD", VecInstrTemplate::VMADD},
    {"VMADDRELU", VecInstrTemplate::VMADDRELU},
    {"VMAX", VecInstrTemplate::VMAX},
    {"VMAXS", VecInstrTemplate::VMAXS},
    {"VMERGECH", VecInstrTemplate::VMERGECH},
    {"VMIN", VecInstrTemplate::VMIN},
    {"VMINS", VecInstrTemplate::VMINS},
    {"VMLA", VecInstrTemplate::VMLA},
    {"VMOVMASK_XN", VecInstrTemplate::VMOVMASK_CMPMASK_XN},
    {"VMS4", VecInstrTemplate::VMS4},
    {"VMUL", VecInstrTemplate::VMUL},
    {"VMULCONV", VecInstrTemplate::VMULCONV},
    {"VMULS", VecInstrTemplate::VMULS},
    {"VNOT", VecInstrTemplate::VNOT},
    {"VOR", VecInstrTemplate::VOR},
    {"VPADDING", VecInstrTemplate::VPADDING},
    {"VREC", VecInstrTemplate::VREC},
    {"VREDUCE", VecInstrTemplate::VREDUCE},
    {"VRELU", VecInstrTemplate::VRELU},
    {"VRPAC", VecInstrTemplate::VRPAC},
    {"VRSQRT", VecInstrTemplate::VRSQRT},
    {"VSCATTER", VecInstrTemplate::VSCATTER},
    {"VSEL", VecInstrTemplate::VSEL},
    {"VSHL", VecInstrTemplate::VSHL},
    {"VSHR", VecInstrTemplate::VSHR},
    {"VSQRT", VecInstrTemplate::VSQRT},
    {"VSUB", VecInstrTemplate::VSUB},
    {"VSUBRELU", VecInstrTemplate::VSUBRELU},
    {"VSUBRELUCONV", VecInstrTemplate::VSUBRELUCONV},
    {"VTRANSPOSE", VecInstrTemplate::VTRANSPOSE}
};

const std::unordered_map<ChipProductType, InstrLogStr2Template> CamodelVecInstrNameMap = {
    {ChipProductType::ASCEND910B_SERIES, instrLogStr2TemplateA2A3},
    {ChipProductType::ASCEND910_93_SERIES, instrLogStr2TemplateA2A3},
    {ChipProductType::ASCEND310P_SERIES, {
        {"ldvad", VecInstrTemplate::LD_VAD},
        {"vaadd", VecInstrTemplate::VAADD},
        {"vabs", VecInstrTemplate::VABS},
        {"vadd", VecInstrTemplate::VADD},
        {"vadddeqrelu", VecInstrTemplate::VADDDEQRELU },
        {"vaddrelu", VecInstrTemplate::VADDRELU},
        {"vaddreluconv", VecInstrTemplate::VADDRELUCONV},
        {"vadds", VecInstrTemplate::VADDS},
        {"vand", VecInstrTemplate::VAND},
        {"vaxpy", VecInstrTemplate::VAXPY},
        {"vbs16", VecInstrTemplate::VBS16},
        {"vcadd", VecInstrTemplate::VCADD},
        {"vcbd", VecInstrTemplate::VCBD},
        {"vcgadd", VecInstrTemplate::VCGADD},
        {"vcgmax", VecInstrTemplate::VCGMAX},
        {"vcgmin", VecInstrTemplate::VCGMIN},
        {"vcmax", VecInstrTemplate::VCMAX},
        {"vcmin", VecInstrTemplate::VCMIN},
        {"vcmp", VecInstrTemplate::VCMP},
        {"vconv", VecInstrTemplate::VCONV},
        {"vcpadd", VecInstrTemplate::VCPADD},
        {"vdiv", VecInstrTemplate::VDIV},
        {"vdp", VecInstrTemplate::VDP},
        {"vexp", VecInstrTemplate::VEXP},
        {"vextract", VecInstrTemplate::VEXTRACT},
        {"vgather", VecInstrTemplate::VGATHER},
        {"viou", VecInstrTemplate::VIOU},
        {"vln", VecInstrTemplate::VLN},
        {"vlrelu", VecInstrTemplate::VLRELU},
        {"vmadd", VecInstrTemplate::VMADD},
        {"vmaddrelu", VecInstrTemplate::VMADDRELU},
        {"vmax", VecInstrTemplate::VMAX},
        {"vmaxs", VecInstrTemplate::VMAXS},
        {"vmergech", VecInstrTemplate::VMERGECH},
        {"vmin", VecInstrTemplate::VMIN},
        {"vmins", VecInstrTemplate::VMINS},
        {"vmla", VecInstrTemplate::VMLA},
        {"vmovemask_cmp_xn", VecInstrTemplate::VMOVMASK_CMPMASK_XN},
        {"vms4", VecInstrTemplate::VMS4},
        {"vmul", VecInstrTemplate::VMUL},
        {"vmulconv", VecInstrTemplate::VMULCONV},
        {"vmuls", VecInstrTemplate::VMULS},
        {"vnot", VecInstrTemplate::VNOT},
        {"vor", VecInstrTemplate::VOR},
        {"vpadding", VecInstrTemplate::VPADDING},
        {"vrec", VecInstrTemplate::VREC},
        {"vreduce", VecInstrTemplate::VREDUCE},
        {"vrelu", VecInstrTemplate::VRELU},
        {"vrpac", VecInstrTemplate::VRPAC},
        {"vrsqrt", VecInstrTemplate::VRSQRT},
        {"vscatter", VecInstrTemplate::VSCATTER},
        {"vsel", VecInstrTemplate::VSEL},
        {"vshl", VecInstrTemplate::VSHL},
        {"vshr", VecInstrTemplate::VSHR},
        {"vsqrt", VecInstrTemplate::VSQRT},
        {"vsub", VecInstrTemplate::VSUB},
        {"vsubrelu", VecInstrTemplate::VSUBRELU},
        {"vsubreluconv", VecInstrTemplate::VSUBRELUCONV},
        {"vtranspose", VecInstrTemplate::VTRANSPOSE}
    }},
};

bool GetInstrLogStr2TemplateMap(ChipProductType seriesChipType, InstrLogStr2Template &resMap)
{
    auto it = CamodelVecInstrNameMap.find(seriesChipType);
    if (it == CamodelVecInstrNameMap.end()) {
        resMap = {};
        return false;
    }
    resMap = it->second;
    return true;
}

}
}
