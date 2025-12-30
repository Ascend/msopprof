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

#include "plugin/memory_chart/record_mov_align.h"

using namespace Common;

// #374
// ASM: MOV_OUT_TO_UB_ALIGN.b16	[dst], [src], config, gapConfig
MSOPPROF_REPORT(copy_gm_to_ubuf_align_b16, __ubuf__ void *dst, __gm__ void *src, uint64_t config, uint64_t gapConfig)
{
    RecordMovAlignEvent<MemType::GM, MemType::UB, DataType::DATA_B16>({EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst),
                                                                       reinterpret_cast<uint64_t>(src)},
                                                                      config, gapConfig);
}

// #376
// ASM: MOV_OUT_TO_UB_ALIGN.b32	[dst], [src], config, gapConfig
MSOPPROF_REPORT(copy_gm_to_ubuf_align_b32, __ubuf__ void *dst, __gm__ void *src, uint64_t config, uint64_t gapConfig)
{
    RecordMovAlignEvent<MemType::GM, MemType::UB, DataType::DATA_B32>({EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst),
                                                                       reinterpret_cast<uint64_t>(src)},
                                                                      config, gapConfig);
}

// #378
// ASM: MOV_OUT_TO_UB_ALIGN.b8	[dst], [src], config, gapConfig
MSOPPROF_REPORT(copy_gm_to_ubuf_align_b8, __ubuf__ void *dst, __gm__ void *src, uint64_t config, uint64_t gapConfig)
{
    RecordMovAlignEvent<MemType::GM, MemType::UB, DataType::DATA_B8>({EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst),
                                                                      reinterpret_cast<uint64_t>(src)},
                                                                     config, gapConfig);
}

// #966
// ASM: MOV_UB_TO_OUT_ALIGN.b16	[dst], [src], config, gapConfig
MSOPPROF_REPORT(copy_ubuf_to_gm_align_b16, __gm__ void *dst, __ubuf__ void *src, uint64_t config, uint64_t gapConfig)
{
    RecordMovAlignEvent<MemType::UB, MemType::GM, DataType::DATA_B16>({EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst),
                                                                       reinterpret_cast<uint64_t>(src)},
                                                                      config, gapConfig);
}

// #968
// ASM: MOV_UB_TO_OUT_ALIGN.b32	[dst], [src], config, gapConfig
MSOPPROF_REPORT(copy_ubuf_to_gm_align_b32, __gm__ void *dst, __ubuf__ void *src, uint64_t config, uint64_t gapConfig)
{
    RecordMovAlignEvent<MemType::UB, MemType::GM, DataType::DATA_B32>({EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst),
                                                                       reinterpret_cast<uint64_t>(src)},
                                                                      config, gapConfig);
}

// #970
// ASM: MOV_UB_TO_OUT_ALIGN.b8	[dst], [src], config, gapConfig
MSOPPROF_REPORT(copy_ubuf_to_gm_align_b8, __gm__ void *dst, __ubuf__ void *src, uint64_t config, uint64_t gapConfig)
{
    RecordMovAlignEvent<MemType::UB, MemType::GM, DataType::DATA_B8>({EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst),
                                                                      reinterpret_cast<uint64_t>(src)},
                                                                     config, gapConfig);
}

// #342
// ASM: MOV_OUT_TO_L1_ALIGN_V2.b16	[dst], [src], config0, config1
MSOPPROF_REPORT(copy_gm_to_cbuf_align_v2_b16, __cbuf__ void *dst, __gm__ void *src, uint64_t config0, uint64_t config1)
{
    RecordMovAlignV2Event<MemType::GM, MemType::L1, DataType::DATA_B16>({EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst),
                                                                        reinterpret_cast<uint64_t>(src)}, config0, config1);
}

// #362
// ASM: MOV_OUT_TO_L1_ALIGN_V2.b32	[dst], [src], config0, config1
MSOPPROF_REPORT(copy_gm_to_cbuf_align_v2_b32, __cbuf__ void *dst, __gm__ void *src, uint64_t config0, uint64_t config1)
{
    RecordMovAlignV2Event<MemType::GM, MemType::L1, DataType::DATA_B32>({EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst),
                                                                        reinterpret_cast<uint64_t>(src)}, config0, config1);
}

// #347
// ASM: MOV_OUT_TO_L1_ALIGN_V2.b8	[dst], [src], config0, config1
MSOPPROF_REPORT(copy_gm_to_cbuf_align_v2_b8, __cbuf__ void *dst, __gm__ void *src, uint64_t config0, uint64_t config1)
{
    RecordMovAlignV2Event<MemType::GM, MemType::L1, DataType::DATA_B8>({EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst),
                                                                       reinterpret_cast<uint64_t>(src)}, config0, config1);
}

// #3707
// ASM: MOV 	LOOP_SIZE_OUTTOL1, config
MSOPPROF_REPORT(set_loop_size_outtol1, uint64_t config)
{
    SetLoopSizeOuttol1(EXTRA_PARAMS, config);
}

// #3694
// ASM: MOV 	LOOP1_STRIDE_OUTTOL1, config
MSOPPROF_REPORT(set_loop1_stride_outtol1, uint64_t config)
{
    SetLoop1StrideOuttol1(EXTRA_PARAMS, config);
}

// #3699
// ASM: MOV 	LOOP2_STRIDE_OUTTOL1, config
MSOPPROF_REPORT(set_loop2_stride_outtol1, uint64_t config)
{
    SetLoop2StrideOuttol1(EXTRA_PARAMS, config);
}
// #1283
// ASM: MOV_UB_TO_OUT_ALIGN_V2	[dst_addr], [src_addr], config0, config1
MSOPPROF_REPORT(copy_ubuf_to_gm_align_v2, __gm__ void *dst, __ubuf__ void *src, uint64_t config0, uint64_t config1)
{
    RecordMovAlignV2Event<MemType::UB, MemType::GM, DataType::DATA_B32>(
        {EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src)}, config0, config1);
}

// #3709
// ASM: MOV		LOOP_SIZE_UBTOOUT, config
MSOPPROF_REPORT(set_loop_size_ubtoout, uint64_t config)
{
    SetLoopSizeUbToOut(EXTRA_PARAMS, config);
}

// #2991
// ASM: MOV		LOOP1_STRIDE_UBTOOUT, config
MSOPPROF_REPORT(set_loop1_stride_ubtoout, uint64_t config)
{
    SetLoop1StrideUbToOut(EXTRA_PARAMS, config);
}

// #2996
// ASM: MOV		LOOP2_STRIDE_UBTOOUT, config
MSOPPROF_REPORT(set_loop2_stride_ubtoout, uint64_t config)
{
    SetLoop2StrideUbToOut(EXTRA_PARAMS, config);
}


// #395
// ASM: MOV_OUT_TO_UB_ALIGN_V2.b8	[dst], [src], config, strideConfig
MSOPPROF_REPORT(copy_gm_to_ubuf_align_v2_b8, __ubuf__ void *dst, __gm__ void *src, uint64_t config0, uint64_t config1)
{
    RecordMovAlignV2Event<MemType::GM, MemType::UB, DataType::DATA_B8>(
        {EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src)}, config0, config1);
}

// #383
// ASM: MOV_OUT_TO_UB_ALIGN_V2.b16	[dst], [src], config, strideConfig
MSOPPROF_REPORT(copy_gm_to_ubuf_align_v2_b16, __ubuf__ void *dst, __gm__ void *src, uint64_t config0, uint64_t config1)
{
    RecordMovAlignV2Event<MemType::GM, MemType::UB, DataType::DATA_B16>(
        {EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src)}, config0, config1);
}

// #391
// ASM: MOV_OUT_TO_UB_ALIGN_V2.b32	[dst], [src], config0, config1
MSOPPROF_REPORT(copy_gm_to_ubuf_align_v2_b32, __ubuf__ void *dst, __gm__ void *src, uint64_t config0, uint64_t config1)
{
    RecordMovAlignV2Event<MemType::GM, MemType::UB, DataType::DATA_B32>(
        {EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src)}, config0, config1);
}

// #3004
// ASM: MOV		LOOP_SIZE_OUTTOUB, config
MSOPPROF_REPORT(set_loop_size_outtoub, uint64_t config)
{
    SetLoopSizeOuttoub(EXTRA_PARAMS, config);
}

// #2991
// ASM: MOV		LOOP1_STRIDE_OUTTOUB, config
MSOPPROF_REPORT(set_loop1_stride_outtoub, uint64_t config)
{
    SetLoop1StrideOuttoub(EXTRA_PARAMS, config);
}

// #2996
// ASM: MOV		LOOP2_STRIDE_OUTTOUB, config
MSOPPROF_REPORT(set_loop2_stride_outtoub, uint64_t config)
{
    SetLoop2StrideOuttoub(EXTRA_PARAMS, config);
}
