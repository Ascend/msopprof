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


#ifndef MSOPT_SIMD_INSTR_TABLE_A5_H
#define MSOPT_SIMD_INSTR_TABLE_A5_H

#include <string>
#include <unordered_map>
namespace Encode {
const std::unordered_map<uint32_t, std::string> INSTR29_TABLE0_A5 = {
    {0xc000001e, "VLOOPv2"},
    {0x00000008, "VLD"},
    {0x00000009, "VLDA"},
    {0x0000000a, "VLDU"},
    {0x0000000b, "VSLD"},
    {0x0000000c, "PLD"},
    {0x00000010, "VLDS"},
    {0x00000013, "VSLDB"},
    {0x00000014, "PLDS"},
    {0x00000018, "VLDI"},
    {0x0000001c, "PLDI"},
};

const std::unordered_map<uint32_t, std::string> INSTR30_TABLE0_A5 = {
    {0xc000001d, "VAG"},
    {0x4000004a, "VSTAI"},
    {0x40000052, "SPRSTI"},
    {0x40000049, "VSTAS"},
    {0x40000051, "VSSTB"},
    {0x40000055, "SPRSTS"},
    {0x4000004f, "VSTUR"},
    {0x8000002b, "VSADDS"},
    {0x8000002c, "VCI"},
    {0x8000002d, "WPACKST"},
    {0x8000002e, "WPACKS"},
    {0x8000002f, "WPACKSA"},
    {0x80000031, "WPACKIT"},
    {0x80000032, "WPACKI"},
    {0x80000033, "WPACKIA"},
    {0x80000034, "WDUPS"},
    {0x80000048, "VMULS"},
    {0x80000049, "VAXPY"},
    {0x8000004a, "VLRELU"},
    {0x8000004b, "WMULS"},
    {0x8000004c, "WMULAS"},
    {0x8000004d, "VSHLS"},
    {0x8000004e, "VSHRS"},
};

const std::unordered_map<uint32_t, std::string> INSTR31_TABLE0_A5 = {
    {0xc0000020, "SADD"},
    {0xc0000000, "SADDI"},
    {0xc0000021, "SSUB"},
    {0xc0000001, "SSUBI"},
    {0xc0000022, "SMUL"},
    {0xc0000002, "SMULI"},
    {0xc0000023, "SMULL"},
    {0xc0000003, "SMULLI"},
    {0xc0000024, "SMAX"},
    {0xc0000025, "SMIN"},
    {0xc0000006, "SABS"},
    {0xc0000027, "SAND"},
    {0xc0000028, "SOR"},
    {0xc0000029, "SXOR"},
    {0xc000000a, "SNOT"},
    {0xc000002b, "SSHL"},
    {0xc000000b, "SSHLI"},
    {0xc000002c, "SSHR"},
    {0xc000000c, "SSHRI"},
    {0xc000000e, "SMOVK"},
    {0xc000002f, "SLD"},
    {0xc000000f, "SLDI"},
    {0xc0000030, "SST"},
    {0xc0000010, "SSTI"},
    {0xc0000011, "SZEROEXT"},
    {0xc0000012, "SSIGNEXT"},
    {0xc0000034, "SCBZ"},
    {0xc0000014, "SCBZI"},
    {0xc0000035, "SCMP"},
    {0xc0000016, "SNOP"},
    {0xc0000017, "SEND"},
    {0xc0000018, "SMEM_BAR"},
    {0xc0000039, "SEXT"},
    {0xc000003a, "FORK"},
    {0x00000032, "VLDUS"},
    {0x0000003a, "VLDUI"},
    {0x00000019, "VLDAS"},
    {0x00000000, "VGATHER2"},
    {0x00000001, "VGATHER2_BC"},
    {0x00000002, "VGATHERB"},

};

const std::unordered_map<uint32_t, std::string> INSTR32_TABLE0_A5 = {
    {0xc000000d, "SMOV"},
    {0xc000010d, "SMOVI"},
    {0xc0000013, "SJUMP"},
    {0xc0000113, "SJUMPI"},
};

const std::unordered_map<uint32_t, std::string> INSTR33_TABLE0_A5 = {
    {0x40000000, "VST"},
};

const std::unordered_map<uint32_t, std::string> INSTR34_TABLE0_A5 = {
    {0x40000040, "PST"},
};

const std::unordered_map<uint32_t, std::string> INSTR35_TABLE0_A5 = {
    {0x40000048, "VSTA"},
    {0x400000cc, "VSTU"},
    {0x400000ce, "VSTUI"},
    {0x400000cd, "VSTUS"},
    {0x4000004b, "VSTAR"},
    {0x40000013, "SPRCLR"},
};

const std::unordered_map<uint32_t, std::string> INSTR36_TABLE0_A5 = {
    {0x40000060, "VSST"},
};

const std::unordered_map<uint32_t, std::string> INSTR38_TABLE0_A5 = {
    {0x40000002, "VSTI"},
    {0x40000001, "VSTS"},
};

const std::unordered_map<uint32_t, std::string> INSTR39_TABLE0_A5 = {
    {0x40000042, "PSTI"},
    {0x40000041, "PSTS"},
};

const std::unordered_map<uint32_t, std::string> INSTR40_TABLE0_A5 = {
    {0x400000e2, "PSTU"},
    {0x40000003, "VSCATTER"},
};

const std::unordered_map<uint32_t, std::string> INSTR41_TABLE0_A5 = {
    {0x80080010, "VCVTFI"},
    {0x80080011, "VCVTFF"},
    {0x80080012, "VCVTIF"},
};

const std::unordered_map<uint32_t, std::string> INSTR48_TABLE0_A5 = {
    {0x80080000, "VADD"},
    {0x80080001, "VSUB"},
    {0x80080002, "VMAX"},
    {0x80080003, "VMIN"},
    {0x8008000c, "VTRC"},
    {0x80080013, "VCVTII"},
    {0x8008001b, "VMAXRELU"},
    {0x8008001f, "VMINRELU"},
    {0x80000010, "VADDC"},
    {0x80000011, "VSUBC"},
    {0x80000012, "VADDCS"},
    {0x80000013, "VSUBCS"},
    {0x80080030, "VCVTFF2"},
    {0x80000040, "VMUL"},
    {0x80000041, "VMULA"},
    {0x80000042, "VMADD"},
    {0x80000043, "VPRELU"},
    {0x80000044, "WMUL"},
    {0x80000045, "WMULA"},
    {0x80000046, "VMULL"},
    {0x80080042, "VRNDI"},
    {0x80080045, "VRND"},
    {0x80080040, "VSHRI"},
    {0x80080041, "VSHLI"},
    {0x80080043, "VSHLI"},
    {0x80080044, "VSHLI"},
    {0x80000060, "VINTLV"},
    {0x80000061, "VDINTLV"},
    {0x80000062, "VSELR"},
    {0x80080063, "VSLIDE"},
    {0x80000064, "VDUP"},
    {0x80000065, "VDUPM"},
    {0x80000066, "VPACK"},
    {0x80000067, "MOVVP"},
    {0x80000068, "VZUNPACK"},
    {0x80000069, "VSUNPACK"},
    {0x8000006a, "PPACK"},
    {0x8000006b, "PUNPACK"},
    {0x8008006c, "PRPNSET"},
    {0x8000006d, "PINTLV"},
    {0x8000006e, "PDINTLV"},
    {0x8008006f, "PSLIDE"},
    {0x80000070, "VCBMAX"},
    {0x80000071, "VCBMIN"},
    {0x80000072, "VCADD"},
    {0x80000073, "VCMAX"},
    {0x80000074, "VCMIN"},
    {0x80000075, "VCGADD"},
    {0x80000076, "VCGMAX"},
    {0x80000077, "VCGMIN"},
    {0x80000078, "VCPADD"},
    {0x80000079, "VSQZ"},
    {0x8000007a, "VUSQZ"},
};

const std::unordered_map<uint32_t, std::string> INSTR49_TABLE0_A5 = {
    {0x80080004, "VABSDIF"},
    {0x80080005, "VAVG"},
    {0x80080006, "VADD3"},
    {0x80080007, "VADIF"},
    {0x80080008, "VSAD"},
    {0x80080009, "VABS"},
    {0x8008000a, "VSADD"},
    {0x8008000b, "VSSUB"},
    {0x8008000d, "WPACKT"},
    {0x8008000e, "WPACK"},
    {0x8008000f, "WPACKA"},
    {0x80080017, "VNEG"},
    {0x80000000, "WADD"},
    {0x80000001, "WADDA"},
    {0x80000002, "WSUB"},
    {0x80000003, "WSUBA"},
    {0x80000004, "WADDSUB"},
    {0x80000005, "WCVT48"},
    {0x80000006, "WMOVT"},
    {0x80000007, "WMOV"},
    {0x80080035, "VCP"},
    {0x80000050, "VAND"},
    {0x80000051, "VOR"},
    {0x80040050, "VXOR"},
    {0x80040051, "VNOT"},
    {0x80000052, "VSEL"},
    {0x80000053, "VMOV"},
    {0x80040052, "PAND"},
    {0x80040053, "POR"},
    {0x80000054, "PXOR"},
    {0x80000055, "PNOT"},
    {0x80040054, "PSEL"},
    {0x80040055, "PSET"},
    {0x80000056, "VDUPI"},
    {0x80040056, "MOVP"},
    {0x80080050, "VDUPS"},
    {0x80080051, "VBR"},
    {0x800c0050, "PLT"},
    {0x800c0051, "PLTM"},
    {0x8000005a, "VSQRT"},
    {0x8000005b, "VLN"},
    {0x8004005a, "VEXP"},
    {0x8004005b, "VDIV"},
    {0x8008005a, "VDIVF"},
    {0x800c005a, "VEXPDIF"},
    {0x800c005b, "VEXP2"},
    {0x8000005e, "DHISTv2"},
    {0x8004005e, "CHISTv2"},
    {0x8004005f, "VINTEGRALv2"},
    {0x8008005e, "WFIFR2"},
    {0x8008005f, "WFIFR2A"},
    {0x800c005e, "WFIFR2S"},
};

const std::unordered_map<uint32_t, std::string> INSTR50_TABLE0_A5 = {
    {0x80040000, "VCMP"},
};

const std::unordered_map<uint32_t, std::string> INSTR51_TABLE0_A5 = {
    {0x80000020, "VCMPS"},
};

const std::unordered_map<uint32_t, std::string> INSTR52_TABLE0_A5 = {
    {0x80000028, "VADDS"},
    {0x80000029, "VMAXS"},
    {0x8000002a, "VMINS"},
};

const std::unordered_map<uint32_t, std::string> INSTR53_TABLE0_A5 = {
    {0x8000014f, "VMULSCVT"},
    {0x8000004f, "VRNDS"},
};

const std::unordered_map<uint32_t, std::string> INSTR54_TABLE0_A5 = {
    {0x8000005c, "VABS"},
    {0x8000005d, "VCLS"},
    {0x8004005c, "VNEG"},
    {0x8004005d, "VBCNT"},
    {0x8000205c, "VREC"},
    {0x8000205d, "VRSQRT"},
    {0x8004205c, "VRELU"},
};

}
#endif // MSOPT_SIMD_INSTR_TABLE_A5_H
