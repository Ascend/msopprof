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

#include "plugin/memory_chart/record_mov_fp.h"

using namespace Common;

// #609
// ASM: FIX_L0C_TO_OUT.f32 	[dst], [src], xm, xt
MSOPPROF_REPORT(copy_matrix_cc_to_gm_f32, __gm__ float *dst, __cc__ float *src, uint64_t xm, uint64_t xt)
{
    RecordMovFpEvent<AtomicMode::F32>({EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src)},
                                      xm, xt);
}

// #651
// ASM: FIX_L0C_TO_OUT.s32 	[dst], [src], xm, xt
MSOPPROF_REPORT(copy_matrix_cc_to_gm_s32, __gm__ int32_t *dst, __cc__ int32_t *src, uint64_t xm, uint64_t xt)
{
    RecordMovFpEvent<AtomicMode::S32>({EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src)},
                                      xm, xt);
}

// #609
// ASM: FIX_L0C_TO_OUT.f32 	[dst], [src], xm, xt
MSOPPROF_REPORT(set_nd_para, uint64_t config)
{
    SetNdPara(EXTRA_PARAMS, config);
}

MSOPPROF_REPORT(set_loop3_para, uint64_t config)
{
    SetLoop3Para(EXTRA_PARAMS, config);
}

MSOPPROF_REPORT(set_channel_para, uint64_t config)
{
    SetChannelPara(EXTRA_PARAMS, config);
}

// ASM: FIX_L0C_TO_OUT.f32 	[dst], [src], xm, xt
MSOPPROF_REPORT(copy_matrix_cc_to_gm_f32_a5, __gm__ void *dst, __cc__ void *src, uint64_t xm, uint64_t xt)
{
    RecordFixL0CGMEvent<MemType::L0C, MemType::GM, DataType::DATA_F32>({EXTRA_PARAMS,
                                                                        reinterpret_cast<uint64_t>(dst),
                                                                        reinterpret_cast<uint64_t>(src)},
                                                                        xm, xt);
}

// #651
// ASM: FIX_L0C_TO_OUT.s32 	[dst], [src], xm, xt
MSOPPROF_REPORT(copy_matrix_cc_to_gm_s32_a5, __gm__ void *dst, __cc__ void *src, uint64_t xm, uint64_t xt)
{
    RecordFixL0CGMEvent<MemType::L0C, MemType::GM, DataType::DATA_S32>({EXTRA_PARAMS,
                                                                        reinterpret_cast<uint64_t>(dst),
                                                                        reinterpret_cast<uint64_t>(src)},
                                                                        xm, xt);
}