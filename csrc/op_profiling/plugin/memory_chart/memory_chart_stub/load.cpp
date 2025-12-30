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

#include "plugin/memory_chart/record_load.h"

using namespace Common;

MSOPPROF_REPORT(load_gm_to_ca, __ca__ void *dst, __gm__ void *src, uint64_t config, addr_cal_mode_t addr_cal_mode)
{
    RecordLoad2DEvent<MemType::GM, MemType::L0A>({EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst),
                                                  reinterpret_cast<uint64_t>(src)},
                                                 config, static_cast<AddrCalMode>(addr_cal_mode));
}

MSOPPROF_REPORT(load_gm_to_cb, __cb__ void *dst, __gm__ void *src, uint64_t config, addr_cal_mode_t addr_cal_mode)
{
    RecordLoad2DEvent<MemType::GM, MemType::L0B>({EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst),
                                                  reinterpret_cast<uint64_t>(src)},
                                                 config, static_cast<AddrCalMode>(addr_cal_mode));
}

MSOPPROF_REPORT(load_gm_to_cbuf, __cbuf__ void *dst, __gm__ void *src, uint64_t config, addr_cal_mode_t addr_cal_mode)
{
    RecordLoad2DEvent<MemType::GM, MemType::L0C>({EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst),
                                                  reinterpret_cast<uint64_t>(src)},
                                                 config, static_cast<AddrCalMode>(addr_cal_mode));
}

// ASM: LOAD_L1_TO_L0B_2D_TRANSPOSE.b4 	[dst], [src], config, fracStride
MSOPPROF_REPORT(load_cbuf_to_cb_transpose_b4,
                __cb__ void *dst, __cbuf__ void *src, uint64_t config, uint64_t fracStride)
{
    RecordLoadL1ToL0B2DTransposeEvent<MemType::L1, MemType::L0B, DataType::DATA_B4>(
        {EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src)}, config, fracStride);
}

// ASM: LOAD_L1_TO_L0B_2D_TRANSPOSE.b8 	[dst], [src], config, fracStride
MSOPPROF_REPORT(load_cbuf_to_cb_transpose_b8,
                __cb__ void *dst, __cbuf__ void *src, uint64_t config, uint64_t fracStride)
{
    RecordLoadL1ToL0B2DTransposeEvent<MemType::L1, MemType::L0B, DataType::DATA_B8>(
        {EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src)}, config, fracStride);
}

// ASM: LOAD_L1_TO_L0B_2D_TRANSPOSE.b16 	[dst], [src], config, fracStride
MSOPPROF_REPORT(load_cbuf_to_cb_transpose_b16,
                __cb__ void *dst, __cbuf__ void *src, uint64_t config, uint64_t fracStride)
{
    RecordLoadL1ToL0B2DTransposeEvent<MemType::L1, MemType::L0B, DataType::DATA_B16>(
        {EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src)}, config, fracStride);
}

// ASM: LOAD_L1_TO_L0B_2D_TRANSPOSE.b32 	[dst], [src], config, fracStride
MSOPPROF_REPORT(load_cbuf_to_cb_transpose_b32,
                __cb__ void *dst, __cbuf__ void *src, uint64_t config, uint64_t fracStride)
{
    RecordLoadL1ToL0B2DTransposeEvent<MemType::L1, MemType::L0B, DataType::DATA_B32>(
        {EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src)}, config, fracStride);
}

// ASM: LOAD_L1_TO_L0A_2DV2.b4 	[dst], [src], config0, config1, #transpose
MSOPPROF_REPORT(load_cbuf_to_ca_b4,
                __ca__ void *dst, __cbuf__ void *src, uint64_t config0, uint64_t config1, bool transpose)
{
    RecordLoad2DV2Event<MemType::L1, MemType::L0A, DataType::DATA_B4>(
        {EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src)}, config0, config1, transpose);
}

// ASM: LOAD_L1_TO_L0A_2DV2.b8 	[dst], [src], config0, config1, #transpose
MSOPPROF_REPORT(load_cbuf_to_ca_b8,
                __ca__ void *dst, __cbuf__ void *src, uint64_t config0, uint64_t config1, bool transpose)
{
    RecordLoad2DV2Event<MemType::L1, MemType::L0A, DataType::DATA_B8>(
        {EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src)}, config0, config1, transpose);
}

// ASM: LOAD_L1_TO_L0A_2DV2.b16 	[dst], [src], config0, config1, #transpose
MSOPPROF_REPORT(load_cbuf_to_ca_b16,
                __ca__ void *dst, __cbuf__ void *src, uint64_t config0, uint64_t config1, bool transpose)
{
    RecordLoad2DV2Event<MemType::L1, MemType::L0A, DataType::DATA_B16>(
        {EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src)}, config0, config1, transpose);
}

// ASM: LOAD_L1_TO_L0A_2DV2.b32 	[dst], [src], config0, config1, #transpose
MSOPPROF_REPORT(load_cbuf_to_ca_b32,
                __ca__ void *dst, __cbuf__ void *src, uint64_t config0, uint64_t config1, bool transpose)
{
    RecordLoad2DV2Event<MemType::L1, MemType::L0A, DataType::DATA_B32>(
        {EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src)}, config0, config1, transpose);
}

// ASM: LOAD_L1_TO_L0B_2DV2.b4 	[dst], [src], config0, config1, #transpose
MSOPPROF_REPORT(load_cbuf_to_cb_b4,
                __cb__ void *dst, __cbuf__ void *src, uint64_t config0, uint64_t config1, bool transpose)
{
    RecordLoad2DV2Event<MemType::L1, MemType::L0B, DataType::DATA_B4>(
        {EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src)}, config0, config1, transpose);
}

// ASM: LOAD_L1_TO_L0B_2DV2.b8 	[dst], [src], config0, config1, #transpose
MSOPPROF_REPORT(load_cbuf_to_cb_b8,
                __cb__ void *dst, __cbuf__ void *src, uint64_t config0, uint64_t config1, bool transpose)
{
    RecordLoad2DV2Event<MemType::L1, MemType::L0B, DataType::DATA_B8>(
        {EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src)}, config0, config1, transpose);
}

// ASM: LOAD_L1_TO_L0B_2DV2.b16 	[dst], [src], config0, config1, #transpose
MSOPPROF_REPORT(load_cbuf_to_cb_b16,
                __cb__ void *dst, __cbuf__ void *src, uint64_t config0, uint64_t config1, bool transpose)
{
    RecordLoad2DV2Event<MemType::L1, MemType::L0B, DataType::DATA_B16>(
        {EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src)}, config0, config1, transpose);
}

// ASM: LOAD_L1_TO_L0B_2DV2.b32 	[dst], [src], config0, config1, #transpose
MSOPPROF_REPORT(load_cbuf_to_cb_b32,
                __cb__ void *dst, __cbuf__ void *src, uint64_t config0, uint64_t config1, bool transpose)
{
    RecordLoad2DV2Event<MemType::L1, MemType::L0B, DataType::DATA_B32>(
        {EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src)}, config0, config1, transpose);
}

// ASM: LOAD_OUT_TO_L1_2Dv2 	[dst], [src], config0, config1
MSOPPROF_REPORT(load_gm_to_cbuf_2dv2, __cbuf__ void *dst, __gm__ void *src, uint64_t config0, uint64_t config1)
{
    RecordLoad2DV2DecEvent<MemType::GM, MemType::L1>({EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst),
                                                   reinterpret_cast<uint64_t>(src)},
                                                  config0, config1);
}

// ASM: LOAD_L1_TO_L0A_MX_2DV2 	[dst], [src], config0, config1
MSOPPROF_REPORT(load_cbuf_to_ca_mx,
                __ca__ void *dst, __cbuf__ void *src, uint64_t config0, uint64_t config1)
{
    RecordLoadMX2DV2Event<MemType::L1, MemType::L0A>(
        {EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src)}, config0, config1);
}

// ASM: LOAD_L1_TO_L0B_MX_2DV2 	[dst], [src], config0, config1
MSOPPROF_REPORT(load_cbuf_to_cb_mx,
                __cb__ void *dst, __cbuf__ void *src, uint64_t config0, uint64_t config1)
{
    RecordLoadMX2DV2Event<MemType::L1, MemType::L0B>(
        {EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src)}, config0, config1);
}

// ASM: SET_L1_2D.b16 	[dst], config
MSOPPROF_REPORT(set_l1_2d_b16, __cbuf__ void *dst, int64_t config)
{
    RecordSet2DEvent<MemType::REG, MemType::L1, DataType::DATA_B16>(
        {EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), UINT64_MAX}, config);
}

// ASM: SET_L1_2D.b32 	[dst], config
MSOPPROF_REPORT(set_l1_2d_b32, __cbuf__ void *dst, int64_t config)
{
    RecordSet2DEvent<MemType::REG, MemType::L1, DataType::DATA_B32>(
        {EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), UINT64_MAX}, config);
}

// ASM: MOV 		MTE2_SRC_PARA, config
MSOPPROF_REPORT(set_mte2_src_para, uint64_t config)
{
    SetMte2SrcPara(EXTRA_PARAMS, config);
}
