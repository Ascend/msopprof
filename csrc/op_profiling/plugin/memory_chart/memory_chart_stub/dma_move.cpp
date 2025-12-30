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

#include "plugin/memory_chart/record_dma_mov.h"

using namespace Common;

// #329
// ASM: MOV_OUT_TO_UB 	[dst], [src], config
MSOPPROF_REPORT(copy_gm_to_ubuf, __ubuf__ void *dst, __gm__ void *src, uint64_t config)
{
    RecordDmaMovEvent<MemType::GM, MemType::UB>({EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst),
                                                 reinterpret_cast<uint64_t>(src)}, config,
                                                PadMode::PAD_NONE, ByteMode::BM_DISABLE);
}

// #931
// ASM: MOV_UB_TO_OUT 	[dst], [src], config, #0
MSOPPROF_REPORT(copy_ubuf_to_gm, __gm__ void *dst, __ubuf__ void *src, uint64_t config)
{
    RecordDmaMovEvent<MemType::UB, MemType::GM>({EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst),
                                                 reinterpret_cast<uint64_t>(src)}, config,
                                                PadMode::PAD_NONE, ByteMode::BM_DISABLE);
}

// #930
// ASM: MOV_UB_TO_OUT 	[dst], [src], config, #byteMode
MSOPPROF_REPORT(copy_ubuf_to_gm_byte, __gm__ void *dst, __ubuf__ void *src, uint64_t config, bm_t byteMode)
{
    RecordDmaMovEvent<MemType::UB, MemType::GM>({EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst),
                                                 reinterpret_cast<uint64_t>(src)}, config,
                                                PadMode::PAD_NONE, static_cast<ByteMode>(byteMode));
}

// #197
// ASM: MOV_OUT_TO_L1 	[dst], [src], config, #padMode
MSOPPROF_REPORT(copy_gm_to_cbuf, __cbuf__ void *dst, __gm__ void *src, uint64_t config, pad_t padMode)
{
    RecordDmaMovEvent<MemType::GM, MemType::L1>({EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst),
                                                 reinterpret_cast<uint64_t>(src)}, config,
                                                static_cast<PadMode>(padMode), ByteMode::BM_DISABLE);
}

// #94
// ASM: MOV_L1_TO_OUT 	[dst], [src], config
MSOPPROF_REPORT(copy_cbuf_to_gm, __gm__ void *dst, __cbuf__ void *src, uint64_t config)
{
    RecordDmaMovEvent<MemType::L1, MemType::GM>({EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst),
                                                 reinterpret_cast<uint64_t>(src)}, config,
                                                PadMode::PAD_NONE, ByteMode::BM_DISABLE);
}

// #319
// ASM: MOV_OUT_TO_L1_MULTI_ND2NZ.b8 	[dst], [src], xm, xt
MSOPPROF_REPORT(copy_gm_to_cbuf_multi_nd2nz_b8, __cbuf__ void *dst, __gm__ void *src, uint64_t xm, uint64_t xt)
{
    RecordDmaMovNd2nzEvent<MemType::GM, MemType::L1, DataType::DATA_B8>({EXTRA_PARAMS,
                                                                         reinterpret_cast<uint64_t>(dst),
                                                                         reinterpret_cast<uint64_t>(src)},
                                                                        xm, xt);
}

// #305
// ASM: MOV_OUT_TO_L1_MULTI_ND2NZ.b16 	[dst], [src], xm, xt
MSOPPROF_REPORT(copy_gm_to_cbuf_multi_nd2nz_b16, __cbuf__ void *dst, __gm__ void *src, uint64_t xm, uint64_t xt)
{
    RecordDmaMovNd2nzEvent<MemType::GM, MemType::L1, DataType::DATA_B16>({EXTRA_PARAMS,
                                                                          reinterpret_cast<uint64_t>(dst),
                                                                          reinterpret_cast<uint64_t>(src)},
                                                                         xm, xt);
}

// #313
// ASM: MOV_OUT_TO_L1_MULTI_ND2NZ.b32s 	[dst], [src], xm, xt
MSOPPROF_REPORT(copy_gm_to_cbuf_multi_nd2nz_b32s, __cbuf__ void *dst, __gm__ void *src, uint64_t xm, uint64_t xt)
{
    RecordDmaMovNd2nzEvent<MemType::GM, MemType::L1, DataType::DATA_B32>({EXTRA_PARAMS,
                                                                          reinterpret_cast<uint64_t>(dst),
                                                                          reinterpret_cast<uint64_t>(src)},
                                                                         xm, xt);
}

// #475
// ASM: MOV_OUT_TO_L1_MULTI_ND2NZ.b8 	[dst], [src], xm, xt
MSOPPROF_REPORT(copy_gm_to_cbuf_multi_nd2nz_d_b8, __cbuf__ void *dst, __gm__ void *src, uint64_t xm, uint64_t xt)
{
    RecordDmaMovNdOrDn2nzDavEvent<MemType::GM, MemType::L1, DataType::DATA_B8, 1>({EXTRA_PARAMS,
                                                                         reinterpret_cast<uint64_t>(dst),
                                                                         reinterpret_cast<uint64_t>(src)},
                                                                        xm, xt);
}

// #469
// ASM: MOV_OUT_TO_L1_MULTI_ND2NZ.b16 	[dst], [src], xm, xt
MSOPPROF_REPORT(copy_gm_to_cbuf_multi_nd2nz_d_b16, __cbuf__ void *dst, __gm__ void *src, uint64_t xm, uint64_t xt)
{
    RecordDmaMovNdOrDn2nzDavEvent<MemType::GM, MemType::L1, DataType::DATA_B16, 1>({EXTRA_PARAMS,
                                                                          reinterpret_cast<uint64_t>(dst),
                                                                          reinterpret_cast<uint64_t>(src)},
                                                                         xm, xt);
}

// #485
// ASM: MOV_OUT_TO_L1_MULTI_ND2NZ.b32 	[dst], [src], xm, xt
MSOPPROF_REPORT(copy_gm_to_cbuf_multi_nd2nz_d_b32, __cbuf__ void *dst, __gm__ void *src, uint64_t xm, uint64_t xt)
{
    RecordDmaMovNdOrDn2nzDavEvent<MemType::GM, MemType::L1, DataType::DATA_B32, 1>({EXTRA_PARAMS,
                                                                          reinterpret_cast<uint64_t>(dst),
                                                                          reinterpret_cast<uint64_t>(src)},
                                                                         xm, xt);
}

// ASM: MOV_L1_TO_UB 	[dst_addr], [src_addr], config
MSOPPROF_REPORT(copy_cbuf_to_ubuf, __ubuf__ void *dst, __cbuf__ void *src, uint64_t config)
{
    RecordMovEvent<MemType::L1, MemType::UB, DataType::DATA_B32>(
        {EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src)}, config);
}

// ASM: MOV_UB_TO_L1 	[dst_addr], [src_addr], config
MSOPPROF_REPORT(copy_ubuf_to_cbuf, __cbuf__ void *dst, __ubuf__ void *src, uint64_t config)
{
    RecordMovEvent<MemType::UB, MemType::L1, DataType::DATA_B32>(
        {EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src)}, config);
}

// ASM: MOV_L1_TO_BT.f16 	[dst], [src], config
MSOPPROF_REPORT(copy_cbuf_to_bt_b16, uint64_t dst, __cbuf__ void *src, uint64_t config)
{
    RecordMovEvent<MemType::L1, MemType::BT, DataType::DATA_F16>(
        {EXTRA_PARAMS, dst, reinterpret_cast<uint64_t>(src)}, config);
}

// ASM: MOV_L1_TO_BT.f32 	[dst], [src], config
MSOPPROF_REPORT(copy_cbuf_to_bt_f32, uint64_t dst, __cbuf__ void *src, uint64_t config)
{
    RecordMovEvent<MemType::L1, MemType::BT, DataType::DATA_F32>(
        {EXTRA_PARAMS, dst, reinterpret_cast<uint64_t>(src)}, config);
}

// ASM: MOV_OUT_TO_L1_V2 	[dst], [src], config0, config1
MSOPPROF_REPORT(copy_gm_to_cbuf_v2, __cbuf__ void *dst, __gm__ void *src, uint64_t config0, uint64_t config1)
{
    RecordMovV2Event<MemType::GM, MemType::L1, DataType::DATA_F32>(
        {EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src)}, config0, config1);
}

// #417
// ASM_D: MOV_OUT_TO_L1_MULTI_DN2NZ.b8    [dst], [src], xm, xt
MSOPPROF_REPORT(copy_gm_to_cbuf_multi_dn2nz_d_b8, __cbuf__ void *dst, __gm__ void *src, uint64_t xm, uint64_t xt)
{
    RecordDmaMovNdOrDn2nzDavEvent<MemType::GM, MemType::L1, DataType::DATA_B8, 0>(
        {EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src)}, xm, xt);
}

// #425
// ASM_D: MOV_OUT_TO_L1_MULTI_DN2NZ.b16   [dst], [src], xm, xt
MSOPPROF_REPORT(copy_gm_to_cbuf_multi_dn2nz_d_b16, __cbuf__ void *dst, __gm__ void *src, uint64_t xm, uint64_t xt)
{
    RecordDmaMovNdOrDn2nzDavEvent<MemType::GM, MemType::L1, DataType::DATA_B16, 0>(
        {EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src)}, xm, xt);
}

// #431
// ASM_D: MOV_OUT_TO_L1_MULTI_ND2NZ.b32   [dst], [src], xm, xt
MSOPPROF_REPORT(copy_gm_to_cbuf_multi_dn2nz_d_b32, __cbuf__ void *dst, __gm__ void *src, uint64_t xm, uint64_t xt)
{
    RecordDmaMovNdOrDn2nzDavEvent<MemType::GM, MemType::L1, DataType::DATA_B32, 0>(
        {EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src)}, xm, xt);
}

// #2438
// ASM: ND_DMA_OUT_TO_UB.b8 	[dst], [src], config, secConfig
MSOPPROF_REPORT(nd_copy_gm_to_ubuf_b8, __ubuf__ void *dst, __gm__ void *src, uint64_t config0, uint64_t config1)
{
    RecordNdDMAOut2Ub<DataType::DATA_B8>(
        {EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src)}, config0, config1);
}

// #2436
// ASM: ND_DMA_OUT_TO_UB.b16 	[dst], [src], config, secConfig
MSOPPROF_REPORT(nd_copy_gm_to_ubuf_b16, __ubuf__ void *dst, __gm__ void *src, uint64_t config0, uint64_t config1)
{
    RecordNdDMAOut2Ub<DataType::DATA_B16>(
     {EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src)}, config0, config1);
}

// #2437
// ASM: ND_DMA_OUT_TO_UB.b32 	[dst], [src], config, secConfig
MSOPPROF_REPORT(nd_copy_gm_to_ubuf_b32, __ubuf__ void *dst, __gm__ void *src, uint64_t config0, uint64_t config1)
{
    RecordNdDMAOut2Ub<DataType::DATA_B32>(
        {EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src)}, config0, config1);
}

// #3024
// ASM: MOV		PAD_CNT_NDDMA, config
MSOPPROF_REPORT(set_pad_cnt_nddma, uint64_t config)
{
    SetPadCntNddma(EXTRA_PARAMS, config);
}

// #2987
// ASM: MOV		LOOP0_STRIDE_NDDMA, config
MSOPPROF_REPORT(set_loop0_stride_nddma, uint64_t config)
{
    SetLoop0StrideNddma(EXTRA_PARAMS, config);
}

// #2989
// ASM: MOV		LOOP1_STRIDE_NDDMA, config
MSOPPROF_REPORT(set_loop1_stride_nddma, uint64_t config)
{
    SetLoop1StrideNddma(EXTRA_PARAMS, config);
}

// #2994
// ASM: MOV		LOOP2_STRIDE_NDDMA, config
MSOPPROF_REPORT(set_loop2_stride_nddma, uint64_t config)
{
    SetLoop2StrideNddma(EXTRA_PARAMS, config);
}

// #2999
// ASM: MOV		LOOP3_STRIDE_NDDMA, config
MSOPPROF_REPORT(set_loop3_stride_nddma, uint64_t config)
{
    SetLoop3StrideNddma(EXTRA_PARAMS, config);
}

// #3001
// ASM: MOV		LOOP4_STRIDE_NDDMA, config
MSOPPROF_REPORT(set_loop4_stride_nddma, uint64_t config)
{
    SetLoop4StrideNddma(EXTRA_PARAMS, config);
}

// #3735
// ASM: MOV 	MTE2_NZ_PARA, config
MSOPPROF_REPORT(set_mte2_nz_para, uint64_t config)
{
    SetMte2NzPara(EXTRA_PARAMS, config);
}