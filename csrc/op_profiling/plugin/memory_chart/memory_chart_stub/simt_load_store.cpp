/* -------------------------------------------------------------------------
 *  This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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

#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510)
#define SIMT_MODE  // 开启simt模式
#include "plugin/memory_chart/record_simt.h"
#include "plugin/operand_record/recorder.h"
#include "plugin/utils.h"
#include "kernel_injection/include/MSBit.h"
using namespace Common;
// LD ST 系列桩函数
MSOPPROF_REPORT(simt_ldg_u8, uint64_t reg, uint64_t addr, int64_t offset)
{
    RecordLoadStoreEvent<OperandType::DATA_U8, SimtType::LDG>({EXTRA_PARAMS, reg, addr}, MemType::GM,
                                                                         offset);
}

MSOPPROF_REPORT(simt_ldg_s8, uint64_t reg, uint64_t addr, int64_t offset)
{
    RecordLoadStoreEvent<OperandType::DATA_S8, SimtType::LDG>({EXTRA_PARAMS, reg, addr}, MemType::GM,
                                                                         offset);
}

MSOPPROF_REPORT(simt_ldg_u16, uint64_t reg, uint64_t addr, int64_t offset)
{
    RecordLoadStoreEvent<OperandType::DATA_U16, SimtType::LDG>({EXTRA_PARAMS, reg, addr}, MemType::GM,
                                                                         offset);
}

MSOPPROF_REPORT(simt_ldg_s16, uint64_t reg, uint64_t addr, int64_t offset)
{
    RecordLoadStoreEvent<OperandType::DATA_S16, SimtType::LDG>({EXTRA_PARAMS, reg, addr}, MemType::GM,
                                                                          offset);
}

MSOPPROF_REPORT(simt_ldg_b32, uint64_t reg, uint64_t addr, int64_t offset)
{
    RecordLoadStoreEvent<OperandType::DATA_B32, SimtType::LDG>({EXTRA_PARAMS, reg, addr}, MemType::GM,
                                                                          offset);
}

MSOPPROF_REPORT(simt_ldg_b64, uint64_t reg, uint64_t addr, int64_t offset)
{
    RecordLoadStoreEvent<OperandType::DATA_B64, SimtType::LDG>({EXTRA_PARAMS, reg, addr}, MemType::GM,
                                                                          offset);
}

MSOPPROF_REPORT(simt_ldg_b128, uint64_t reg, uint64_t addr, int64_t offset)
{
    RecordLoadStoreEvent<OperandType::DATA_B128, SimtType::LDG>({EXTRA_PARAMS, reg, addr}, MemType::GM,
                                                                          offset);
}

MSOPPROF_REPORT(simt_stg_b8, uint64_t reg, uint64_t addr, int64_t offset)
{
    RecordLoadStoreEvent<OperandType::DATA_B8, SimtType::STG>({EXTRA_PARAMS, reg, addr}, MemType::GM,
                                                                         offset);
}

MSOPPROF_REPORT(simt_stg_b16, uint64_t reg, uint64_t addr, int64_t offset)
{
    RecordLoadStoreEvent<OperandType::DATA_B16, SimtType::STG>({EXTRA_PARAMS, reg, addr}, MemType::GM,
                                                                          offset);
}

MSOPPROF_REPORT(simt_stg_b32, uint64_t reg, uint64_t addr, int64_t offset)
{
    RecordLoadStoreEvent<OperandType::DATA_B32, SimtType::STG>({EXTRA_PARAMS, reg, addr}, MemType::GM,
                                                                          offset);
}

MSOPPROF_REPORT(simt_stg_b64, uint64_t reg, uint64_t addr, int64_t offset)
{
    RecordLoadStoreEvent<OperandType::DATA_B64, SimtType::STG>({EXTRA_PARAMS, reg, addr}, MemType::GM,
                                                                          offset);
}

MSOPPROF_REPORT(simt_stg_b128, uint64_t reg, uint64_t addr, int64_t offset)
{
    RecordLoadStoreEvent<OperandType::DATA_B128, SimtType::STG>({EXTRA_PARAMS, reg, addr}, MemType::GM,
                                                                           offset);
}

MSOPPROF_REPORT(simt_ld_u8, uint64_t reg, uint64_t addr, int64_t offset)
{
    MemType space = SimtRemapAddress(addr, offset, sizeof(uint8_t));
    RecordLoadStoreEvent<OperandType::DATA_U8, SimtType::LD>({EXTRA_PARAMS, reg, addr}, space,
                                                                           offset);
}

MSOPPROF_REPORT(simt_ld_s8, uint64_t reg, uint64_t addr, int64_t offset)
{
    MemType space = SimtRemapAddress(addr, offset, sizeof(int8_t));
    RecordLoadStoreEvent<OperandType::DATA_S8, SimtType::LD>({EXTRA_PARAMS, reg, addr}, space,
                                                                           offset);
}

MSOPPROF_REPORT(simt_ld_u16, uint64_t reg, uint64_t addr, int64_t offset)
{
    MemType space = SimtRemapAddress(addr, offset, sizeof(uint16_t));
    RecordLoadStoreEvent<OperandType::DATA_U16, SimtType::LD>({EXTRA_PARAMS, reg, addr}, space,
                                                                           offset);
}

MSOPPROF_REPORT(simt_ld_s16, uint64_t reg, uint64_t addr, int64_t offset)
{
    MemType space = SimtRemapAddress(addr, offset, sizeof(int16_t));
    RecordLoadStoreEvent<OperandType::DATA_S16, SimtType::LD>({EXTRA_PARAMS, reg, addr}, space,
                                                                           offset);
}

MSOPPROF_REPORT(simt_ld_b32, uint64_t reg, uint64_t addr, int64_t offset)
{
    MemType space = SimtRemapAddress(addr, offset, sizeof(int32_t));
    RecordLoadStoreEvent<OperandType::DATA_B32, SimtType::LD>({EXTRA_PARAMS, reg, addr}, space,
                                                                           offset);
}

MSOPPROF_REPORT(simt_st_b8, uint64_t reg, uint64_t addr, int64_t offset)
{
    MemType space = SimtRemapAddress(addr, offset, sizeof(int8_t));
    RecordLoadStoreEvent<OperandType::DATA_B8, SimtType::ST>({EXTRA_PARAMS, reg, addr}, space,
                                                                           offset);
}

MSOPPROF_REPORT(simt_st_b16, uint64_t reg, uint64_t addr, int64_t offset)
{
    MemType space = SimtRemapAddress(addr, offset, sizeof(int16_t));
    RecordLoadStoreEvent<OperandType::DATA_B16, SimtType::ST>({EXTRA_PARAMS, reg, addr}, space,
                                                                           offset);
}

MSOPPROF_REPORT(simt_st_b32, uint64_t reg, uint64_t addr, int64_t offset)
{
    MemType space = SimtRemapAddress(addr, offset, sizeof(int32_t));
    RecordLoadStoreEvent<OperandType::DATA_B32, SimtType::ST>({EXTRA_PARAMS, reg, addr}, space,
                                                                           offset);
}

MSOPPROF_REPORT(simt_lds_u8, uint64_t reg, uint64_t addr, int64_t offset)
{
    RecordLoadStoreEvent<OperandType::DATA_U8, SimtType::LDS>({EXTRA_PARAMS, reg, addr}, MemType::UB,
                                                                           offset);
}

MSOPPROF_REPORT(simt_lds_s8, uint64_t reg, uint64_t addr, int64_t offset)
{
    RecordLoadStoreEvent<OperandType::DATA_S8, SimtType::LDS>({EXTRA_PARAMS, reg, addr}, MemType::UB,
                                                                           offset);
}

MSOPPROF_REPORT(simt_lds_u16, uint64_t reg, uint64_t addr, int64_t offset)
{
    RecordLoadStoreEvent<OperandType::DATA_U16, SimtType::LDS>({EXTRA_PARAMS, reg, addr}, MemType::UB,
                                                                           offset);
}

MSOPPROF_REPORT(simt_lds_s16, uint64_t reg, uint64_t addr, int64_t offset)
{
    RecordLoadStoreEvent<OperandType::DATA_S16, SimtType::LDS>({EXTRA_PARAMS, reg, addr}, MemType::UB,
                                                                           offset);
}

MSOPPROF_REPORT(simt_lds_b32, uint64_t reg, uint64_t addr, int64_t offset)
{
    RecordLoadStoreEvent<OperandType::DATA_B32, SimtType::LDS>({EXTRA_PARAMS, reg, addr}, MemType::UB,
                                                                           offset);
}

MSOPPROF_REPORT(simt_lds_b64, uint64_t reg, uint64_t addr, int64_t offset)
{
    RecordLoadStoreEvent<OperandType::DATA_B64, SimtType::LDS>({EXTRA_PARAMS, reg, addr}, MemType::UB,
                                                                           offset);
}

MSOPPROF_REPORT(simt_lds_b128, uint64_t reg, uint64_t addr, int64_t offset)
{
    RecordLoadStoreEvent<OperandType::DATA_B128, SimtType::LDS>({EXTRA_PARAMS, reg, addr}, MemType::UB,
                                                                           offset);
}

MSOPPROF_REPORT(simt_sts_b8, uint64_t reg, uint64_t addr, int64_t offset)
{
    RecordLoadStoreEvent<OperandType::DATA_B8, SimtType::STS>({EXTRA_PARAMS, reg, addr}, MemType::UB,
                                                                           offset);
}

MSOPPROF_REPORT(simt_sts_b16, uint64_t reg, uint64_t addr, int64_t offset)
{
    RecordLoadStoreEvent<OperandType::DATA_B16, SimtType::STS>({EXTRA_PARAMS, reg, addr}, MemType::UB,
                                                                           offset);
}

MSOPPROF_REPORT(simt_sts_b32, uint64_t reg, uint64_t addr, int64_t offset)
{
    RecordLoadStoreEvent<OperandType::DATA_B32, SimtType::STS>({EXTRA_PARAMS, reg, addr}, MemType::UB,
                                                                           offset);
}

MSOPPROF_REPORT(simt_sts_b64, uint64_t reg, uint64_t addr, int64_t offset)
{
    RecordLoadStoreEvent<OperandType::DATA_B64, SimtType::STS>({EXTRA_PARAMS, reg, addr}, MemType::UB,
                                                                          offset);
}

MSOPPROF_REPORT(simt_sts_b128, uint64_t reg, uint64_t addr, int64_t offset)
{
    RecordLoadStoreEvent<OperandType::DATA_B128, SimtType::STS>({EXTRA_PARAMS, reg, addr}, MemType::UB,
                                                                           offset);
}

MSOPPROF_REPORT(simt_ldk_u8, uint64_t reg, uint64_t addr, int64_t offset)
{
    RecordLoadStoreEvent<OperandType::DATA_U8, SimtType::LDK>({EXTRA_PARAMS, reg, addr}, MemType::PRIVATE,
                                                                           offset);
}

MSOPPROF_REPORT(simt_ldk_s8, uint64_t reg, uint64_t addr, int64_t offset)
{
    RecordLoadStoreEvent<OperandType::DATA_S8, SimtType::LDK>({EXTRA_PARAMS, reg, addr}, MemType::PRIVATE,
                                                                           offset);
}

MSOPPROF_REPORT(simt_ldk_u16, uint64_t reg, uint64_t addr, int64_t offset)
{
    RecordLoadStoreEvent<OperandType::DATA_U16, SimtType::LDK>({EXTRA_PARAMS, reg, addr}, MemType::PRIVATE,
                                                                           offset);
}

MSOPPROF_REPORT(simt_ldk_s16, uint64_t reg, uint64_t addr, int64_t offset)
{
    RecordLoadStoreEvent<OperandType::DATA_S16, SimtType::LDK>({EXTRA_PARAMS, reg, addr}, MemType::PRIVATE,
                                                                           offset);
}

MSOPPROF_REPORT(simt_ldk_b32, uint64_t reg, uint64_t addr, int64_t offset)
{
    RecordLoadStoreEvent<OperandType::DATA_B32, SimtType::LDK>({EXTRA_PARAMS, reg, addr}, MemType::PRIVATE,
                                                                           offset);
}

MSOPPROF_REPORT(simt_stk_b8, uint64_t reg, uint64_t addr, int64_t offset)
{
    RecordLoadStoreEvent<OperandType::DATA_B8, SimtType::STK>({EXTRA_PARAMS, reg, addr}, MemType::PRIVATE,
                                                                           offset);
}

MSOPPROF_REPORT(simt_stk_b16, uint64_t reg, uint64_t addr, int64_t offset)
{
    RecordLoadStoreEvent<OperandType::DATA_B16, SimtType::STK>({EXTRA_PARAMS, reg, addr}, MemType::PRIVATE,
                                                                           offset);
}

MSOPPROF_REPORT(simt_stk_b32, uint64_t reg, uint64_t addr, int64_t offset)
{
    RecordLoadStoreEvent<OperandType::DATA_B32, SimtType::STK>({EXTRA_PARAMS, reg, addr}, MemType::PRIVATE,
                                                                           offset);
}
#endif
