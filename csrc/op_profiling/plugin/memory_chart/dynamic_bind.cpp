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

#include <string>
#include <vector>
#include <cstdint>
#include "kernel_injection/include/MSBit.h"

namespace {
struct InstBindInfo {
    InstrType instrType;
    std::string funcName;
    std::vector<uint16_t> argsIndex;
    InstrForCore type;
};

std::vector<InstBindInfo> BIND_FUNCTION{
    // ccec type, stub function name, stub function args index
    // DMA_MOV
    {InstrType::COPY_GM_TO_UBUF, "__msopprof_report_copy_gm_to_ubuf", {0, 1, 2}},
    {InstrType::COPY_UBUF_TO_GM, "__msopprof_report_copy_ubuf_to_gm", {0, 1, 2}},
    {InstrType::COPY_UBUF_TO_GM_BYTE, "__msopprof_report_copy_ubuf_to_gm_byte", {0, 1, 2, 3}},
    {InstrType::COPY_GM_TO_CBUF, "__msopprof_report_copy_gm_to_cbuf", {0, 1, 2, 3}},
    {InstrType::COPY_CBUF_TO_GM, "__msopprof_report_copy_cbuf_to_gm", {0, 1, 2}},
    // MOV_ALIGN
    {InstrType::COPY_GM_TO_UBUF_ALIGN_B16, "__msopprof_report_copy_gm_to_ubuf_align_b16", {0, 1, 2, 3}},
    {InstrType::COPY_GM_TO_UBUF_ALIGN_B32, "__msopprof_report_copy_gm_to_ubuf_align_b32", {0, 1, 2, 3}},
    {InstrType::COPY_GM_TO_UBUF_ALIGN_B8, "__msopprof_report_copy_gm_to_ubuf_align_b8", {0, 1, 2, 3}},
    {InstrType::COPY_UBUF_TO_GM_ALIGN_B16, "__msopprof_report_copy_ubuf_to_gm_align_b16", {0, 1, 2, 3}},
    {InstrType::COPY_UBUF_TO_GM_ALIGN_B32, "__msopprof_report_copy_ubuf_to_gm_align_b32", {0, 1, 2, 3}},
    {InstrType::COPY_UBUF_TO_GM_ALIGN_B8, "__msopprof_report_copy_ubuf_to_gm_align_b8", {0, 1, 2, 3}},
    // MOV_ALIGN_V2
    {InstrType::COPY_UBUF_TO_GM_ALIGN_V2, "__msopprof_report_copy_ubuf_to_gm_align_v2", {0, 1, 2, 3}},
    {InstrType::LOOP_SIZE_UBTOOUT, "__msopprof_report_set_loop_size_ubtoout", {0}},
    {InstrType::LOOP1_STRIDE_UBTOOUT, "__msopprof_report_set_loop1_stride_ubtoout", {0}},
    {InstrType::LOOP2_STRIDE_UBTOOUT, "__msopprof_report_set_loop2_stride_ubtoout", {0}},
    {InstrType::COPY_GM_TO_CBUF_ALIGN_V2_B16, "__msopprof_report_copy_gm_to_cbuf_align_v2_b16", {0, 1, 2, 3}},
    {InstrType::COPY_GM_TO_CBUF_ALIGN_V2_B32, "__msopprof_report_copy_gm_to_cbuf_align_v2_b32", {0, 1, 2, 3}},
    {InstrType::COPY_GM_TO_CBUF_ALIGN_V2_B8, "__msopprof_report_copy_gm_to_cbuf_align_v2_b8", {0, 1, 2, 3}},
    {InstrType::SET_LOOP_SIZE_OUTTOL1, "__msopprof_report_set_loop_size_outtol1", {0}},
    {InstrType::SET_LOOP1_STRIDE_OUTTOL1, "__msopprof_report_set_loop1_stride_outtol1", {0}},
    {InstrType::SET_LOOP2_STRIDE_OUTTOL1, "__msopprof_report_set_loop2_stride_outtol1", {0}},
    {InstrType::COPY_GM_TO_UBUF_ALIGN_V2_B8, "__msopprof_report_copy_gm_to_ubuf_align_v2_b8", {0, 1, 2, 3}},
    {InstrType::COPY_GM_TO_UBUF_ALIGN_V2_B16, "__msopprof_report_copy_gm_to_ubuf_align_v2_b16", {0, 1, 2, 3}},
    {InstrType::COPY_GM_TO_UBUF_ALIGN_V2_B32, "__msopprof_report_copy_gm_to_ubuf_align_v2_b32", {0, 1, 2, 3}},
    {InstrType::LOOP_SIZE_OUTTOUB, "__msopprof_report_set_loop_size_outtoub", {0}},
    {InstrType::LOOP1_STRIDE_OUTTOUB, "__msopprof_report_set_loop1_stride_outtoub", {0}},
    {InstrType::LOOP2_STRIDE_OUTTOUB, "__msopprof_report_set_loop2_stride_outtoub", {0}},
    // LOAD_2D
    {InstrType::LOAD_GM_TO_CA, "__msopprof_report_load_gm_to_ca", {0, 1, 2, 3}},
    {InstrType::LOAD_GM_TO_CB, "__msopprof_report_load_gm_to_cb", {0, 1, 2, 3}},
    {InstrType::LOAD_GM_TO_CBUF, "__msopprof_report_load_gm_to_cbuf", {0, 1, 2, 3}},
    {InstrType::LOAD_GM_TO_CBUF_2DV2, "__msopprof_report_load_gm_to_cbuf_2dv2", {0, 1, 2, 3}},
    {InstrType::MTE2_SRC_PARA, "__msopprof_report_set_mte2_src_para", {0}},
    // DMA_MOV_ND2NZ
    {InstrType::COPY_GM_TO_CBUF_MULTI_ND2NZ_B8, "__msopprof_report_copy_gm_to_cbuf_multi_nd2nz_b8", {0, 1, 2, 3}},
    {InstrType::COPY_GM_TO_CBUF_MULTI_ND2NZ_B16, "__msopprof_report_copy_gm_to_cbuf_multi_nd2nz_b16", {0, 1, 2, 3}},
    {InstrType::COPY_GM_TO_CBUF_MULTI_ND2NZ_B32S, "__msopprof_report_copy_gm_to_cbuf_multi_nd2nz_b32s", {0, 1, 2, 3}},
    // DMA_MOV_DN2NZ_D
    {InstrType::COPY_GM_TO_CBUF_MULTI_DN2NZ_D_B8, "__msopprof_report_copy_gm_to_cbuf_multi_dn2nz_d_b8", {0, 1, 2, 3}},
    {InstrType::COPY_GM_TO_CBUF_MULTI_DN2NZ_D_B16, "__msopprof_report_copy_gm_to_cbuf_multi_dn2nz_d_b16", {0, 1, 2, 3}},
    {InstrType::COPY_GM_TO_CBUF_MULTI_DN2NZ_D_B32, "__msopprof_report_copy_gm_to_cbuf_multi_dn2nz_d_b32", {0, 1, 2, 3}},
    // DMA_MOV_ND2NZ_D
    {InstrType::COPY_GM_TO_CBUF_MULTI_ND2NZ_D_B8, "__msopprof_report_copy_gm_to_cbuf_multi_nd2nz_d_b8", {0, 1, 2, 3}},
    {InstrType::COPY_GM_TO_CBUF_MULTI_ND2NZ_D_B16, "__msopprof_report_copy_gm_to_cbuf_multi_nd2nz_d_b16", {0, 1, 2, 3}},
    {InstrType::COPY_GM_TO_CBUF_MULTI_ND2NZ_D_B32, "__msopprof_report_copy_gm_to_cbuf_multi_nd2nz_d_b32", {0, 1, 2, 3}},
    {InstrType::SET_MTE2_NZ_PARA, "__msopprof_report_set_mte2_nz_para", {0}},
    // MOV_FP
    {InstrType::COPY_MATRIX_CC_TO_GM_F32, "__msopprof_report_copy_matrix_cc_to_gm_f32", {0, 1, 2, 3}},
    {InstrType::COPY_MATRIX_CC_TO_GM_S32, "__msopprof_report_copy_matrix_cc_to_gm_s32", {0, 1, 2, 3}},
    {InstrType::SET_ND_PARA, "__msopprof_report_set_nd_para", {0}},
    // LOAD_2D_TRANSPOSE
    {InstrType::LOAD_CBUF_TO_CB_TRANSPOSE_B4, "__msopprof_report_load_cbuf_to_cb_transpose_b4", {0, 1, 2, 3}},
    {InstrType::LOAD_CBUF_TO_CB_TRANSPOSE_B8, "__msopprof_report_load_cbuf_to_cb_transpose_b8", {0, 1, 2, 3}},
    {InstrType::LOAD_CBUF_TO_CB_TRANSPOSE_B16, "__msopprof_report_load_cbuf_to_cb_transpose_b16", {0, 1, 2, 3}},
    {InstrType::LOAD_CBUF_TO_CB_TRANSPOSE_B32, "__msopprof_report_load_cbuf_to_cb_transpose_b32", {0, 1, 2, 3}},
    // LOAD_2Dv2
    {InstrType::LOAD_CBUF_TO_CA_B4, "__msopprof_report_load_cbuf_to_ca_b4", {0, 1, 2, 3, 4}},
    {InstrType::LOAD_CBUF_TO_CA_B8, "__msopprof_report_load_cbuf_to_ca_b8", {0, 1, 2, 3, 4}},
    {InstrType::LOAD_CBUF_TO_CA_B16, "__msopprof_report_load_cbuf_to_ca_b16", {0, 1, 2, 3, 4}},
    {InstrType::LOAD_CBUF_TO_CA_B32, "__msopprof_report_load_cbuf_to_ca_b32", {0, 1, 2, 3, 4}},
    {InstrType::LOAD_CBUF_TO_CB_B4, "__msopprof_report_load_cbuf_to_cb_b4", {0, 1, 2, 3, 4}},
    {InstrType::LOAD_CBUF_TO_CB_B8, "__msopprof_report_load_cbuf_to_cb_b8", {0, 1, 2, 3, 4}},
    {InstrType::LOAD_CBUF_TO_CB_B16, "__msopprof_report_load_cbuf_to_cb_b16", {0, 1, 2, 3, 4}},
    {InstrType::LOAD_CBUF_TO_CB_B32, "__msopprof_report_load_cbuf_to_cb_b32", {0, 1, 2, 3, 4}},
    // LOAD_MX2Dv2
    {InstrType::LOAD_CBUF_TO_CA_MX, "__msopprof_report_load_cbuf_to_ca_mx", {0, 1, 2, 3}},
    {InstrType::LOAD_CBUF_TO_CB_MX, "__msopprof_report_load_cbuf_to_cb_mx", {0, 1, 2, 3}},
    // SET_L1_2D
    {InstrType::SET_L1_2D_B16, "__msopprof_report_set_l1_2d_b16", {0, 1}},
    {InstrType::SET_L1_2D_B32, "__msopprof_report_set_l1_2d_b32", {0, 1}},
    // MOV
    {InstrType::COPY_CBUF_TO_UBUF, "__msopprof_report_copy_cbuf_to_ubuf", {0, 1, 2}},
    {InstrType::COPY_UBUF_TO_CBUF, "__msopprof_report_copy_ubuf_to_cbuf", {0, 1, 2}},
    {InstrType::COPY_CBUF_TO_BT_B16, "__msopprof_report_copy_cbuf_to_bt_b16", {0, 1, 2, 3}},
    {InstrType::COPY_CBUF_TO_BT_F32, "__msopprof_report_copy_cbuf_to_bt_f32", {0, 1, 2, 3}},
    // MOV_V2
    {InstrType::COPY_GM_TO_CBUF_V2, "__msopprof_report_copy_gm_to_cbuf_v2", {0, 1, 2, 3}},
    {InstrType::COPY_MATRIX_CC_TO_GM_F32_A5, "__msopprof_report_copy_matrix_cc_to_gm_f32_a5", {0, 1, 2, 3}},
    {InstrType::COPY_MATRIX_CC_TO_GM_S32_A5, "__msopprof_report_copy_matrix_cc_to_gm_s32_a5", {0, 1, 2, 3}},
    {InstrType::LOOP3_PARA, "__msopprof_report_set_loop3_para", {0}},
    {InstrType::CHANNEL_PARA, "__msopprof_report_set_channel_para", {0}},
    // ND_DMA_OUT_TO_UB
    {InstrType::NDDMA_OUT_TO_UB_B8, "__msopprof_report_nd_copy_gm_to_ubuf_b8", {0, 1, 2, 3}},
    {InstrType::NDDMA_OUT_TO_UB_B16, "__msopprof_report_nd_copy_gm_to_ubuf_b16", {0, 1, 2, 3}},
    {InstrType::NDDMA_OUT_TO_UB_B32, "__msopprof_report_nd_copy_gm_to_ubuf_b32", {0, 1, 2, 3}},
    {InstrType::PAD_CNT_NDDMA, "__msopprof_report_set_pad_cnt_nddma", {0}},
    {InstrType::LOOP0_STRIDE_NDDMA, "__msopprof_report_set_loop0_stride_nddma", {0}},
    {InstrType::LOOP1_STRIDE_NDDMA, "__msopprof_report_set_loop1_stride_nddma", {0}},
    {InstrType::LOOP2_STRIDE_NDDMA, "__msopprof_report_set_loop2_stride_nddma", {0}},
    {InstrType::LOOP3_STRIDE_NDDMA, "__msopprof_report_set_loop3_stride_nddma", {0}},
    {InstrType::LOOP4_STRIDE_NDDMA, "__msopprof_report_set_loop4_stride_nddma", {0}},
    {InstrType::SIMT_LDG_U8, "__msopprof_report_simt_ldg_u8", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_LDG_U16, "__msopprof_report_simt_ldg_u16", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_LDG_S8, "__msopprof_report_simt_ldg_s8", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_LDG_S16, "__msopprof_report_simt_ldg_s16", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_LDG_B32, "__msopprof_report_simt_ldg_b32", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_LDG_B64, "__msopprof_report_simt_ldg_b64", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_LDG_B128, "__msopprof_report_simt_ldg_b128", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_STG_B8, "__msopprof_report_simt_stg_b8", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_STG_B16, "__msopprof_report_simt_stg_b16", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_STG_B32, "__msopprof_report_simt_stg_b32", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_STG_B64, "__msopprof_report_simt_stg_b64", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_STG_B128, "__msopprof_report_simt_stg_b128", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_LDS_U8, "__msopprof_report_simt_lds_u8", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_LDS_S8, "__msopprof_report_simt_lds_s8", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_LDS_U16, "__msopprof_report_simt_lds_u16", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_LDS_S16, "__msopprof_report_simt_lds_s16", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_LDS_B32, "__msopprof_report_simt_lds_b32", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_LDS_B64, "__msopprof_report_simt_lds_b64", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_LDS_B128, "__msopprof_report_simt_lds_b128", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_STS_B8, "__msopprof_report_simt_sts_b8", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_STS_B16, "__msopprof_report_simt_sts_b16", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_STS_B32, "__msopprof_report_simt_sts_b32", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_STS_B64, "__msopprof_report_simt_sts_b64", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_STS_B128, "__msopprof_report_simt_sts_b128", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_LDK_U8, "__msopprof_report_simt_ldk_u8", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_LDK_S8, "__msopprof_report_simt_ldk_s8", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_LDK_U16, "__msopprof_report_simt_ldk_u16", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_LDK_S16, "__msopprof_report_simt_ldk_s16", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_LDK_B32, "__msopprof_report_simt_ldk_b32", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_STK_B8, "__msopprof_report_simt_stk_b8", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_STK_B16, "__msopprof_report_simt_stk_b16", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_STK_B32, "__msopprof_report_simt_stk_b32", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_LD_U8, "__msopprof_report_simt_ld_u8", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_LD_S8, "__msopprof_report_simt_ld_s8", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_LD_U16, "__msopprof_report_simt_ld_u16", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_LD_S16, "__msopprof_report_simt_ld_s16", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_LD_B32, "__msopprof_report_simt_ld_b32", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_ST_B8, "__msopprof_report_simt_st_b8", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_ST_B16, "__msopprof_report_simt_st_b16", {0, 1, 2}, InstrForCore::VEC},
    {InstrType::SIMT_ST_B32, "__msopprof_report_simt_st_b32", {0, 1, 2}, InstrForCore::VEC},
};
}

extern "C" {
void MSBitAtInit()
{
    for (auto it : BIND_FUNCTION) {
        Bind(it.instrType, it.funcName, it.argsIndex, it.type);
    }
}
}
