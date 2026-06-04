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

#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(__DAV_VEC__)
#define SIMT_MODE  // 开启simt模式
#include "plugin/operand_record/recorder.h"
#include "plugin/utils.h"
#include "kernel_injection/include/MSBit.h"
using namespace Common;
MSOPPROF_REPORT(ffma_f16)
{
    RecordOperandOperation<InstrType::FFMA_F16, OperandType::DATA_F16>(memInfo, 1, true);
}

MSOPPROF_REPORT(ffma_f32)
{
    RecordOperandOperation<InstrType::FFMA_F32, OperandType::DATA_F32>(memInfo, 1, true);
}

MSOPPROF_REPORT(ffma_f16x2)
{
    RecordOperandOperation<InstrType::FFMA_V2F16, OperandType::DATA_F16X2>(memInfo, 1, true);
}

MSOPPROF_REPORT(ffma_bf16)
{
    RecordOperandOperation<InstrType::FFMA_BF16, OperandType::DATA_BF16>(memInfo, 1, true);
}

MSOPPROF_REPORT(ffma_bf16x2)
{
    RecordOperandOperation<InstrType::FFMA_V2BF16, OperandType::DATA_BF16X2>(memInfo, 1, true);
}

MSOPPROF_REPORT(ffma_imm_f16)
{
    RecordOperandOperation<InstrType::FFMA_F16_IMM, OperandType::DATA_F16>(memInfo, 1, true);
}

MSOPPROF_REPORT(ffma_imm_f32)
{
    RecordOperandOperation<InstrType::FFMA_F32_IMM, OperandType::DATA_F32>(memInfo, 1, true);
}

MSOPPROF_REPORT(ffma_imm_f16x2)
{
    RecordOperandOperation<InstrType::FFMA_V2F16_IMM, OperandType::DATA_F16X2>(memInfo, 1, true);
}

MSOPPROF_REPORT(ffma_imm_bf16)
{
    RecordOperandOperation<InstrType::FFMA_BF16_IMM, OperandType::DATA_BF16>(memInfo, 1, true);
}

MSOPPROF_REPORT(ffma_imm_bf16x2)
{
    RecordOperandOperation<InstrType::FFMA_V2BF16_IMM, OperandType::DATA_BF16X2>(memInfo, 1, true);
}

MSOPPROF_REPORT(atom_max_s_s32)
{
    RecordOperandOperation<InstrType::ATOM_MAX_S_S32, OperandType::DATA_S32>(memInfo, 1, true);
}

MSOPPROF_REPORT(atom_min_s_s32)
{
    RecordOperandOperation<InstrType::ATOM_MIN_S_S32, OperandType::DATA_S32>(memInfo, 1, true);
}

MSOPPROF_REPORT(atom_add_s_s32)
{
    RecordOperandOperation<InstrType::ATOM_ADD_S_S32, OperandType::DATA_S32>(memInfo, 1, true);
}

MSOPPROF_REPORT(atom_add_s_u32)
{
    RecordOperandOperation<InstrType::ATOM_ADD_S_U32, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(atom_exch_s_u32)
{
    RecordOperandOperation<InstrType::ATOM_EXCH_S_U32, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(atom_min_s_u32)
{
    RecordOperandOperation<InstrType::ATOM_MIN_S_U32, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(atom_cas_s_u32)
{
    RecordOperandOperation<InstrType::ATOM_CAS_S_U32, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(atom_max_s_u32)
{
    RecordOperandOperation<InstrType::ATOM_MAX_S_U32, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(atom_exch_g_u64)
{
    RecordOperandOperation<InstrType::ATOM_EXCH_G_U64, OperandType::DATA_U64>(memInfo, 1, true);
}

MSOPPROF_REPORT(atom_min_g_u64)
{
    RecordOperandOperation<InstrType::ATOM_MIN_G_U64, OperandType::DATA_U64>(memInfo, 1, true);
}

MSOPPROF_REPORT(atom_add_g_u64)
{
    RecordOperandOperation<InstrType::ATOM_ADD_G_U64, OperandType::DATA_U64>(memInfo, 1, true);
}

MSOPPROF_REPORT(atom_cas_g_u64)
{
    RecordOperandOperation<InstrType::ATOM_CAS_G_U64, OperandType::DATA_U64>(memInfo, 1, true);
}

MSOPPROF_REPORT(atom_max_g_u64)
{
    RecordOperandOperation<InstrType::ATOM_MAX_G_U64, OperandType::DATA_U64>(memInfo, 1, true);
}

MSOPPROF_REPORT(atom_add_g_s64)
{
    RecordOperandOperation<InstrType::ATOM_ADD_G_S64, OperandType::DATA_S64>(memInfo, 1, true);
}

MSOPPROF_REPORT(atom_min_g_s64)
{
    RecordOperandOperation<InstrType::ATOM_MIN_G_S64, OperandType::DATA_S64>(memInfo, 1, true);
}

MSOPPROF_REPORT(atom_max_g_s64)
{
    RecordOperandOperation<InstrType::ATOM_MAX_G_S64, OperandType::DATA_S64>(memInfo, 1, true);
}

MSOPPROF_REPORT(atom_exch_g_u32)
{
    RecordOperandOperation<InstrType::ATOM_EXCH_G_U32, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(atom_min_g_u32)
{
    RecordOperandOperation<InstrType::ATOM_MIN_G_U32, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(atom_cas_g_u32)
{
    RecordOperandOperation<InstrType::ATOM_CAS_G_U32, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(atom_max_g_u32)
{
    RecordOperandOperation<InstrType::ATOM_MAX_G_U32, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(atom_add_g_u32)
{
    RecordOperandOperation<InstrType::ATOM_ADD_G_U32, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(atom_add_g_s32)
{
    RecordOperandOperation<InstrType::ATOM_ADD_G_S32, OperandType::DATA_S32>(memInfo, 1, true);
}

MSOPPROF_REPORT(atom_min_g_s32)
{
    RecordOperandOperation<InstrType::ATOM_MIN_G_S32, OperandType::DATA_S32>(memInfo, 1, true);
}

MSOPPROF_REPORT(atom_max_g_s32)
{
    RecordOperandOperation<InstrType::ATOM_MAX_G_S32, OperandType::DATA_S32>(memInfo, 1, true);
}


MSOPPROF_REPORT(red_s_u32)
{
    RecordOperandOperation<InstrType::RED_S_U32, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(red_s_bf16)
{
    RecordOperandOperation<InstrType::RED_S_BF16, OperandType::DATA_BF16>(memInfo, 1, true);
}

MSOPPROF_REPORT(red_s_s32)
{
    RecordOperandOperation<InstrType::RED_S_S32, OperandType::DATA_S32>(memInfo, 1, true);
}

MSOPPROF_REPORT(red_s_fp16)
{
    RecordOperandOperation<InstrType::RED_S_FP16, OperandType::DATA_F16>(memInfo, 1, true);
}

MSOPPROF_REPORT(red_s_fp32)
{
    RecordOperandOperation<InstrType::RED_S_FP32, OperandType::DATA_F32>(memInfo, 1, true);
}

MSOPPROF_REPORT(red_g_bf16)
{
    RecordOperandOperation<InstrType::RED_G_BF16, OperandType::DATA_BF16>(memInfo, 1, true);
}

MSOPPROF_REPORT(red_g_u32)
{
    RecordOperandOperation<InstrType::RED_G_U32, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(red_g_f16x2)
{
    RecordOperandOperation<InstrType::RED_G_F16X2, OperandType::DATA_F16X2>(memInfo, 1, true);
}

MSOPPROF_REPORT(red_g_s32)
{
    RecordOperandOperation<InstrType::RED_G_S32, OperandType::DATA_S32>(memInfo, 1, true);
}

MSOPPROF_REPORT(red_g_fp32)
{
    RecordOperandOperation<InstrType::RED_G_FP32, OperandType::DATA_F32>(memInfo, 1, true);
}

MSOPPROF_REPORT(red_g_bf16x2)
{
    RecordOperandOperation<InstrType::RED_G_BF16X2, OperandType::DATA_BF16X2>(memInfo, 1, true);
}

MSOPPROF_REPORT(red_g_fp16)
{
    RecordOperandOperation<InstrType::RED_G_FP16, OperandType::DATA_F16>(memInfo, 1, true);
}

MSOPPROF_REPORT(mufu_f16)
{
    RecordOperandOperation<InstrType::MUFU_F16, OperandType::DATA_F16>(memInfo, 1, true);
}

MSOPPROF_REPORT(mufu_f16x2)
{
    RecordOperandOperation<InstrType::MUFU_F16X2, OperandType::DATA_F16X2>(memInfo, 1, true);
}

MSOPPROF_REPORT(mufu_f32)
{
    RecordOperandOperation<InstrType::MUFU_F32, OperandType::DATA_F32>(memInfo, 1, true);
}

MSOPPROF_REPORT(fmul_f16)
{
    RecordOperandOperation<InstrType::FMUL_F16, OperandType::DATA_F16>(memInfo, 1, true);
}

MSOPPROF_REPORT(fmul_f16x2)
{
    RecordOperandOperation<InstrType::FMUL_F16X2, OperandType::DATA_F16X2>(memInfo, 1, true);
}

MSOPPROF_REPORT(fmul_f32)
{
    RecordOperandOperation<InstrType::FMUL_F32, OperandType::DATA_F32>(memInfo, 1, true);
}

MSOPPROF_REPORT(fmul_bf16)
{
    RecordOperandOperation<InstrType::FMUL_BF16, OperandType::DATA_BF16>(memInfo, 1, true);
}

MSOPPROF_REPORT(fmul_bf16x2)
{
    RecordOperandOperation<InstrType::FMUL_BF16X2, OperandType::DATA_BF16X2>(memInfo, 1, true);
}

MSOPPROF_REPORT(fmul_f16_imm)
{
    RecordOperandOperation<InstrType::FMUL_F16_IMM, OperandType::DATA_F16>(memInfo, 1, true);
}

MSOPPROF_REPORT(fmul_f16x2_imm)
{
    RecordOperandOperation<InstrType::FMUL_F16X2_IMM, OperandType::DATA_F16X2>(memInfo, 1, true);
}

MSOPPROF_REPORT(fmul_f32_imm)
{
    RecordOperandOperation<InstrType::FMUL_F32_IMM, OperandType::DATA_F32>(memInfo, 1, true);
}

MSOPPROF_REPORT(fmul_bf16_imm)
{
    RecordOperandOperation<InstrType::FMUL_BF16_IMM, OperandType::DATA_BF16>(memInfo, 1, true);
}

MSOPPROF_REPORT(fmul_bf16x2_imm)
{
    RecordOperandOperation<InstrType::FMUL_BF16X2_IMM, OperandType::DATA_BF16X2>(memInfo, 1, true);
}


MSOPPROF_REPORT(fadd_fp16)
{
    RecordOperandOperation<InstrType::FADD_FP16, OperandType::DATA_F16>(memInfo, 1, true);
}

MSOPPROF_REPORT(fadd_fp32)
{
    RecordOperandOperation<InstrType::FADD_FP32, OperandType::DATA_F32>(memInfo, 1, true);
}

MSOPPROF_REPORT(fadd_v2f16)
{
    RecordOperandOperation<InstrType::FADD_V2F16, OperandType::DATA_V2F16>(memInfo, 1, true);
}

MSOPPROF_REPORT(fadd_bf16)
{
    RecordOperandOperation<InstrType::FADD_BF16, OperandType::DATA_BF16>(memInfo, 1, true);
}

MSOPPROF_REPORT(fadd_v2bf16)
{
    RecordOperandOperation<InstrType::FADD_V2BF16, OperandType::DATA_V2BF16>(memInfo, 1, true);
}

MSOPPROF_REPORT(fadd_fp16_imm)
{
    RecordOperandOperation<InstrType::FADD_FP16_IMM, OperandType::DATA_F16>(memInfo, 1, true);
}

MSOPPROF_REPORT(fadd_fp32_imm)
{
    RecordOperandOperation<InstrType::FADD_FP32_IMM, OperandType::DATA_F32>(memInfo, 1, true);
}

MSOPPROF_REPORT(fadd_fp16x2_imm)
{
    RecordOperandOperation<InstrType::FADD_FP16X2_IMM, OperandType::DATA_F16X2>(memInfo, 1, true);
}

MSOPPROF_REPORT(fadd_bf16_imm)
{
    RecordOperandOperation<InstrType::FADD_BF16_IMM, OperandType::DATA_BF16>(memInfo, 1, true);
}

MSOPPROF_REPORT(fadd_bf16x2_imm)
{
    RecordOperandOperation<InstrType::FADD_BF16X2_IMM, OperandType::DATA_BF16X2>(memInfo, 1, true);
}

MSOPPROF_REPORT(fsetp_f16)
{
    RecordOperandOperation<InstrType::FSETP_F16, OperandType::DATA_F16>(memInfo, 1, true);
}

MSOPPROF_REPORT(fsetp_f32)
{
    RecordOperandOperation<InstrType::FSETP_F32, OperandType::DATA_F32>(memInfo, 1, true);
}

MSOPPROF_REPORT(fsetp_f16x2)
{
    RecordOperandOperation<InstrType::FSETP_F16X2, OperandType::DATA_F16X2>(memInfo, 1, true);
}

MSOPPROF_REPORT(fsetp_bf16)
{
    RecordOperandOperation<InstrType::FSETP_BF16, OperandType::DATA_BF16>(memInfo, 1, true);
}

MSOPPROF_REPORT(fsetp_bf16x2)
{
    RecordOperandOperation<InstrType::FSETP_BF16X2, OperandType::DATA_BF16X2>(memInfo, 1, true);
}

MSOPPROF_REPORT(fsetp_f16_imm)
{
    RecordOperandOperation<InstrType::FSETP_F16_IMM, OperandType::DATA_F16>(memInfo, 1, true);
}

MSOPPROF_REPORT(fsetp_f32_imm)
{
    RecordOperandOperation<InstrType::FSETP_F32_IMM, OperandType::DATA_F32>(memInfo, 1, true);
}

MSOPPROF_REPORT(fsetp_f16x2_imm)
{
    RecordOperandOperation<InstrType::FSETP_F16X2_IMM, OperandType::DATA_F16X2>(memInfo, 1, true);
}

MSOPPROF_REPORT(fsetp_bf16_imm)
{
    RecordOperandOperation<InstrType::FSETP_BF16_IMM, OperandType::DATA_BF16>(memInfo, 1, true);
}

MSOPPROF_REPORT(fsetp_bf16x2_imm)
{
    RecordOperandOperation<InstrType::FSETP_BF16X2_IMM, OperandType::DATA_BF16X2>(memInfo, 1, true);
}

MSOPPROF_REPORT(fmnmx_f16)
{
    RecordOperandOperation<InstrType::FMNMX_F16, OperandType::DATA_F16>(memInfo, 1, true);
}

MSOPPROF_REPORT(fmnmx_f32)
{
    RecordOperandOperation<InstrType::FMNMX_F32, OperandType::DATA_F32>(memInfo, 1, true);
}

MSOPPROF_REPORT(fmnmx_f16x2)
{
    RecordOperandOperation<InstrType::FMNMX_F16X2, OperandType::DATA_F16X2>(memInfo, 1, true);
}

MSOPPROF_REPORT(fmnmx_bf16)
{
    RecordOperandOperation<InstrType::FMNMX_BF16, OperandType::DATA_BF16>(memInfo, 1, true);
}

MSOPPROF_REPORT(fmnmx_bf16x2)
{
    RecordOperandOperation<InstrType::FMNMX_BF16X2, OperandType::DATA_BF16X2>(memInfo, 1, true);
}

MSOPPROF_REPORT(fmnmx_f16_imm)
{
    RecordOperandOperation<InstrType::FMNMX_F16_IMM, OperandType::DATA_F16>(memInfo, 1, true);
}

MSOPPROF_REPORT(fmnmx_f32_imm)
{
    RecordOperandOperation<InstrType::FMNMX_F32_IMM, OperandType::DATA_F32>(memInfo, 1, true);
}

MSOPPROF_REPORT(fmnmx_f16x2_imm)
{
    RecordOperandOperation<InstrType::FMNMX_F16X2_IMM, OperandType::DATA_F16X2>(memInfo, 1, true);
}

MSOPPROF_REPORT(fmnmx_bf16_imm)
{
    RecordOperandOperation<InstrType::FMNMX_BF16_IMM, OperandType::DATA_BF16>(memInfo, 1, true);
}

MSOPPROF_REPORT(fmnmx_bf16x2_imm)
{
    RecordOperandOperation<InstrType::FMNMX_BF16X2_IMM, OperandType::DATA_BF16X2>(memInfo, 1, true);
}

MSOPPROF_REPORT(f2i_f16_u32)
{
    RecordOperandOperation<InstrType::F2I_F16_U32, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(f2i_f16_s32)
{
    RecordOperandOperation<InstrType::F2I_F16_S32, OperandType::DATA_S32>(memInfo, 1, true);
}

MSOPPROF_REPORT(f2i_f32_u32)
{
    RecordOperandOperation<InstrType::F2I_F32_U32, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(f2i_f32_s32)
{
    RecordOperandOperation<InstrType::F2I_F32_S32, OperandType::DATA_S32>(memInfo, 1, true);
}

MSOPPROF_REPORT(f2i_f32_u64)
{
    RecordOperandOperation<InstrType::F2I_F32_U64, OperandType::DATA_U64>(memInfo, 1, true);
}

MSOPPROF_REPORT(f2i_f32_s64)
{
    RecordOperandOperation<InstrType::F2I_F32_S64, OperandType::DATA_S64>(memInfo, 1, true);
}

MSOPPROF_REPORT(f2i_bf16_u32)
{
    RecordOperandOperation<InstrType::F2I_BF16_U32, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(f2i_bf16_s32)
{
    RecordOperandOperation<InstrType::F2I_BF16_S32, OperandType::DATA_S32>(memInfo, 1, true);
}

MSOPPROF_REPORT(i2f_s32_bf16)
{
    RecordOperandOperation<InstrType::I2F_S32_BF16, OperandType::DATA_BF16>(memInfo, 1, true);
}

MSOPPROF_REPORT(i2f_s32_f16)
{
    RecordOperandOperation<InstrType::I2F_S32_F16, OperandType::DATA_V2F16>(memInfo, 1, true);
}

MSOPPROF_REPORT(i2f_s64_f32)
{
    RecordOperandOperation<InstrType::I2F_S64_F32, OperandType::DATA_F32>(memInfo, 1, true);
}

MSOPPROF_REPORT(i2f_u32_bf16)
{
    RecordOperandOperation<InstrType::I2F_U32_BF16, OperandType::DATA_BF16>(memInfo, 1, true);
}

MSOPPROF_REPORT(i2f_u32_f16)
{
    RecordOperandOperation<InstrType::I2F_U32_F16, OperandType::DATA_V2F16>(memInfo, 1, true);
}

MSOPPROF_REPORT(i2f_u32_f32)
{
    RecordOperandOperation<InstrType::I2F_U32_F32, OperandType::DATA_F32>(memInfo, 1, true);
}

MSOPPROF_REPORT(i2f_u64_f32)
{
    RecordOperandOperation<InstrType::I2F_U64_F32, OperandType::DATA_F32>(memInfo, 1, true);
}

MSOPPROF_REPORT(f2f_f8e5m2x2_f32x2)
{
    RecordOperandOperation<InstrType::F2F_F8E5M2X2_F32X2, OperandType::DATA_F32X2>(memInfo, 1, true);
}

MSOPPROF_REPORT(f2f_f16_f16)
{
    RecordOperandOperation<InstrType::F2F_F16_F16, OperandType::DATA_V2F16>(memInfo, 1, true);
}

MSOPPROF_REPORT(f2f_f16_f32)
{
    RecordOperandOperation<InstrType::F2F_F16_F32, OperandType::DATA_F32>(memInfo, 1, true);
}

MSOPPROF_REPORT(f2f_f16_bf16)
{
    RecordOperandOperation<InstrType::F2F_F16_BF16, OperandType::DATA_BF16>(memInfo, 1, true);
}

MSOPPROF_REPORT(f2f_f32_bf16)
{
    RecordOperandOperation<InstrType::F2F_F32_BF16, OperandType::DATA_BF16>(memInfo, 1, true);
}

MSOPPROF_REPORT(f2f_f32_f16)
{
    RecordOperandOperation<InstrType::F2F_F32_F16, OperandType::DATA_V2F16>(memInfo, 1, true);
}

MSOPPROF_REPORT(f2f_f32_f32)
{
    RecordOperandOperation<InstrType::F2F_F32_F32, OperandType::DATA_F32>(memInfo, 1, true);
}

MSOPPROF_REPORT(f2f_f32x2_bf16x2)
{
    RecordOperandOperation<InstrType::F2F_F32X2_BF16X2, OperandType::DATA_BF16X2>(memInfo, 1, true);
}

MSOPPROF_REPORT(f2f_f32x2_f16x2)
{
    RecordOperandOperation<InstrType::F2F_F32X2_F16X2, OperandType::DATA_F16X2>(memInfo, 1, true);
}

MSOPPROF_REPORT(f2f_f32x2_hif8x2)
{
    RecordOperandOperation<InstrType::F2F_F32X2_HIF8X2, OperandType::DATA_HIF8X2>(memInfo, 1, true);
}

MSOPPROF_REPORT(f2f_f32x2_f8e4m3x2)
{
    RecordOperandOperation<InstrType::F2F_F32X2_F8E4M3X2, OperandType::DATA_F8E4M3X2>(memInfo, 1, true);
}

MSOPPROF_REPORT(f2f_f32x2_f8e5m2x2)
{
    RecordOperandOperation<InstrType::F2F_F32X2_F8E5M2X2, OperandType::DATA_F8E5M2X2>(memInfo, 1, true);
}

MSOPPROF_REPORT(f2f_bf16_bf16)
{
    RecordOperandOperation<InstrType::F2F_BF16_BF16, OperandType::DATA_BF16>(memInfo, 1, true);
}

MSOPPROF_REPORT(f2f_bf16_fp16)
{
    RecordOperandOperation<InstrType::F2F_BF16_FP16, OperandType::DATA_V2F16>(memInfo, 1, true);
}

MSOPPROF_REPORT(f2f_bf16_fp32)
{
    RecordOperandOperation<InstrType::F2F_BF16_FP32, OperandType::DATA_F32>(memInfo, 1, true);
}

MSOPPROF_REPORT(f2f_f16x2_f32x2)
{
    RecordOperandOperation<InstrType::F2F_F16X2_F32X2, OperandType::DATA_F32X2>(memInfo, 1, true);
}

MSOPPROF_REPORT(f2f_f16x2_hif8x2)
{
    RecordOperandOperation<InstrType::F2F_F16X2_HIF8X2, OperandType::DATA_HIF8X2>(memInfo, 1, true);
}

MSOPPROF_REPORT(f2f_hif8x2_f16x2)
{
    RecordOperandOperation<InstrType::F2F_HIF8X2_F16X2, OperandType::DATA_F16X2>(memInfo, 1, true);
}

MSOPPROF_REPORT(f2f_hif8x2_f32x2)
{
    RecordOperandOperation<InstrType::F2F_HIF8X2_F32X2, OperandType::DATA_F32X2>(memInfo, 1, true);
}

MSOPPROF_REPORT(f2f_bf16x2_f32x2)
{
    RecordOperandOperation<InstrType::F2F_BF16X2_F32X2, OperandType::DATA_F32X2>(memInfo, 1, true);
}

MSOPPROF_REPORT(f2f_f8e4m3x2_f32x2)
{
    RecordOperandOperation<InstrType::F2F_F8E4M3X2_F32X2, OperandType::DATA_F32X2>(memInfo, 1, true);
}

MSOPPROF_REPORT(fdiv_f16)
{
    RecordOperandOperation<InstrType::FDIV_F16, OperandType::DATA_V2F16>(memInfo, 1, true);
}

MSOPPROF_REPORT(fdiv_f16x2)
{
    RecordOperandOperation<InstrType::FDIV_F16X2, OperandType::DATA_F16X2>(memInfo, 1, true);
}

MSOPPROF_REPORT(fdiv_f32)
{
    RecordOperandOperation<InstrType::FDIV_F32, OperandType::DATA_F32>(memInfo, 1, true);
}

MSOPPROF_REPORT(fdiv_f16_imm)
{
    RecordOperandOperation<InstrType::FDIV_F16_IMM, OperandType::DATA_V2F16>(memInfo, 1, true);
}

MSOPPROF_REPORT(fdiv_f16x2_imm)
{
    RecordOperandOperation<InstrType::FDIV_F16X2_IMM, OperandType::DATA_F16X2>(memInfo, 1, true);
}

MSOPPROF_REPORT(fdiv_f32_imm)
{
    RecordOperandOperation<InstrType::FDIV_F32_IMM, OperandType::DATA_F32>(memInfo, 1, true);
}

MSOPPROF_REPORT(fexpdif_fmix)
{
    RecordOperandOperation<InstrType::FEXPDIF_FMIX, OperandType::DATA_FMIX>(memInfo, 1, true);
}

MSOPPROF_REPORT(fexpdif_f32)
{
    RecordOperandOperation<InstrType::FEXPDIF_F32, OperandType::DATA_F32>(memInfo, 1, true);
}


MSOPPROF_REPORT(imad_s32)
{
    RecordOperandOperation<InstrType::IMAD_S32, OperandType::DATA_S32>(memInfo, 1, true);
}

MSOPPROF_REPORT(imad_u32)
{
    RecordOperandOperation<InstrType::IMAD_U32, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(imad_hi_u32)
{
    RecordOperandOperation<InstrType::IMAD_HI_U32, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(imad_hi_s32)
{
    RecordOperandOperation<InstrType::IMAD_HI_S32, OperandType::DATA_S32>(memInfo, 1, true);
}

MSOPPROF_REPORT(imad_wide_u32)
{
    RecordOperandOperation<InstrType::IMAD_WIDE_U32, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(imad_wide_s32)
{
    RecordOperandOperation<InstrType::IMAD_WIDE_S32, OperandType::DATA_S32>(memInfo, 1, true);
}

MSOPPROF_REPORT(imad_norm_u32_imm)
{
    RecordOperandOperation<InstrType::IMAD_NORM_U32_IMM, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(imad_norm_s32_imm)
{
    RecordOperandOperation<InstrType::IMAD_NORM_S32_IMM, OperandType::DATA_S32>(memInfo, 1, true);
}

MSOPPROF_REPORT(imad_hi_s32_imm)
{
    RecordOperandOperation<InstrType::IMAD_HI_S32_IMM, OperandType::DATA_S32>(memInfo, 1, true);
}

MSOPPROF_REPORT(imad_hi_u32_imm)
{
    RecordOperandOperation<InstrType::IMAD_HI_U32_IMM, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(imad_wide_s32_imm)
{
    RecordOperandOperation<InstrType::IMAD_WIDE_S32_IMM, OperandType::DATA_S32>(memInfo, 1, true);
}

MSOPPROF_REPORT(imad_wide_u32_imm)
{
    RecordOperandOperation<InstrType::IMAD_WIDE_U32_IMM, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(imad_x_s32)
{
    RecordOperandOperation<InstrType::IMAD_X_S32, OperandType::DATA_S32>(memInfo, 1, true);
}

MSOPPROF_REPORT(imad_x_u32)
{
    RecordOperandOperation<InstrType::IMAD_X_U32, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(imad_x_s32_imm)
{
    RecordOperandOperation<InstrType::IMAD_X_S32_IMM, OperandType::DATA_S32>(memInfo, 1, true);
}

MSOPPROF_REPORT(imad_x_u32_imm)
{
    RecordOperandOperation<InstrType::IMAD_X_U32_IMM, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(imul_norm_u32)
{
    RecordOperandOperation<InstrType::IMUL_NORM_U32, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(imul_norm_s32)
{
    RecordOperandOperation<InstrType::IMUL_NORM_S32, OperandType::DATA_S32>(memInfo, 1, true);
}

MSOPPROF_REPORT(imul_hi_s32)
{
    RecordOperandOperation<InstrType::IMUL_HI_S32, OperandType::DATA_S32>(memInfo, 1, true);
}

MSOPPROF_REPORT(imul_hi_u32)
{
    RecordOperandOperation<InstrType::IMUL_HI_U32, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(imul_wide_s32)
{
    RecordOperandOperation<InstrType::IMUL_WIDE_S32, OperandType::DATA_S32>(memInfo, 1, true);
}

MSOPPROF_REPORT(imul_wide_u32)
{
    RecordOperandOperation<InstrType::IMUL_WIDE_U32, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(imul_norm_u32_imm)
{
    RecordOperandOperation<InstrType::IMUL_NORM_U32_IMM, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(imul_norm_s32_imm)
{
    RecordOperandOperation<InstrType::IMUL_NORM_S32_IMM, OperandType::DATA_S32>(memInfo, 1, true);
}

MSOPPROF_REPORT(imul_hi_u32_imm)
{
    RecordOperandOperation<InstrType::IMUL_HI_U32_IMM, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(imul_hi_s32_imm)
{
    RecordOperandOperation<InstrType::IMUL_HI_S32_IMM, OperandType::DATA_S32>(memInfo, 1, true);
}

MSOPPROF_REPORT(imul_wide_s32_imm)
{
    RecordOperandOperation<InstrType::IMUL_WIDE_S32_IMM, OperandType::DATA_S32>(memInfo, 1, true);
}

MSOPPROF_REPORT(imul_wide_u32_imm)
{
    RecordOperandOperation<InstrType::IMUL_WIDE_U32_IMM, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(iadd)
{
    RecordOperandOperation<InstrType::IADD, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(iadd_imm)
{
    RecordOperandOperation<InstrType::IADD_IMM, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(iadd_x)
{
    RecordOperandOperation<InstrType::IADD_X, OperandType::DATA_BF16X2>(memInfo, 1, true);
}

MSOPPROF_REPORT(iadd_x_imm)
{
    RecordOperandOperation<InstrType::IADD_X_IMM, OperandType::DATA_BF16X2>(memInfo, 1, true);
}

MSOPPROF_REPORT(isetp_s32_imm)
{
    RecordOperandOperation<InstrType::ISETP_S32_IMM, OperandType::DATA_S32>(memInfo, 1, true);
}

MSOPPROF_REPORT(isetp_u32_imm)
{
    RecordOperandOperation<InstrType::ISETP_U32_IMM, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(isetp_s32)
{
    RecordOperandOperation<InstrType::ISETP_S32, OperandType::DATA_S32>(memInfo, 1, true);
}

MSOPPROF_REPORT(isetp_u32)
{
    RecordOperandOperation<InstrType::ISETP_U32, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(imnmx_u32)
{
    RecordOperandOperation<InstrType::IMNMX_U32, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(imnmx_s32)
{
    RecordOperandOperation<InstrType::IMNMX_S32, OperandType::DATA_S32>(memInfo, 1, true);
}

MSOPPROF_REPORT(imnmx_u32_imm)
{
    RecordOperandOperation<InstrType::IMNMX_U32_IMM, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(imnmx_s32_imm)
{
    RecordOperandOperation<InstrType::IMNMX_S32_IMM, OperandType::DATA_S32>(memInfo, 1, true);
}

MSOPPROF_REPORT(lea_hi_x_b32)
{
    RecordOperandOperation<InstrType::LEA_HI_X_B32, OperandType::DATA_B32>(memInfo, 1, true);
}

MSOPPROF_REPORT(lea_hi_x_sx32)
{
    RecordOperandOperation<InstrType::LEA_HI_X_SX32, OperandType::DATA_SX32>(memInfo, 1, true);
}

MSOPPROF_REPORT(lea_hi_x_zx32)
{
    RecordOperandOperation<InstrType::LEA_HI_X_ZX32, OperandType::DATA_ZX32>(memInfo, 1, true);
}

MSOPPROF_REPORT(shf_u32)
{
    RecordOperandOperation<InstrType::SHF_U32, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(shf_s32)
{
    RecordOperandOperation<InstrType::SHF_S32, OperandType::DATA_S32>(memInfo, 1, true);
}

MSOPPROF_REPORT(shf_u64)
{
    RecordOperandOperation<InstrType::SHF_U64, OperandType::DATA_U64>(memInfo, 1, true);
}

MSOPPROF_REPORT(shf_s64)
{
    RecordOperandOperation<InstrType::SHF_S64, OperandType::DATA_S64>(memInfo, 1, true);
}

MSOPPROF_REPORT(shfi_s32)
{
    RecordOperandOperation<InstrType::SHFI_S32, OperandType::DATA_S32>(memInfo, 1, true);
}

MSOPPROF_REPORT(shfi_u32)
{
    RecordOperandOperation<InstrType::SHFI_U32, OperandType::DATA_U32>(memInfo, 1, true);
}

MSOPPROF_REPORT(shfi_s64)
{
    RecordOperandOperation<InstrType::SHFI_S64, OperandType::DATA_S64>(memInfo, 1, true);
}

MSOPPROF_REPORT(shfi_u64)
{
    RecordOperandOperation<InstrType::SHFI_U64, OperandType::DATA_U64>(memInfo, 1, true);
}
#endif
