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

#include "plugin/operand_record/recorder.h"
#include "plugin/utils.h"
#include "kernel_injection/include/MSBit.h"
using namespace Common;

MSOPPROF_REPORT(mad_s8, __cc__ void *c, __ca__ void *a, __cb__ void *b, uint64_t config)
{
    RecordMmadA5<InstrType::MAD_S8, OperandType::DATA_S8>(memInfo, config);
}

MSOPPROF_REPORT(mad_f16_f32, __cc__ void *c, __ca__ void *a, __cb__ void *b, uint64_t config)
{
    RecordMmadA5<InstrType::MAD_F16_F32, OperandType::DATA_F16>(memInfo, config);
}

MSOPPROF_REPORT(mad_bf16_f32, __cc__ void *c, __ca__ void *a, __cb__ void *b, uint64_t config)
{
    RecordMmadA5<InstrType::MAD_BF16_F32, OperandType::DATA_BF16>(memInfo, config);
}

MSOPPROF_REPORT(mad_f32_f32, __cc__ void *c, __ca__ void *a, __cb__ void *b, uint64_t config)
{
    RecordMmadA5<InstrType::MAD_F32_F32, OperandType::DATA_F32>(memInfo, config);
}

MSOPPROF_REPORT(mad_e4m3_e4m3, __cc__ void *c, __ca__ void *a, __cb__ void *b, uint64_t config)
{
    RecordMmadA5<InstrType::MAD_E4M3_E4M3, OperandType::DATA_E4M3>(memInfo, config);
}

MSOPPROF_REPORT(mad_e4m3_e5m2, __cc__ void *c, __ca__ void *a, __cb__ void *b, uint64_t config)
{
    RecordMmadA5<InstrType::MAD_E4M3_E5M2, OperandType::DATA_E4M3>(memInfo, config);
}

MSOPPROF_REPORT(mad_e5m2_e4m3, __cc__ void *c, __ca__ void *a, __cb__ void *b, uint64_t config)
{
    RecordMmadA5<InstrType::MAD_E5M2_E4M3, OperandType::DATA_E5M2>(memInfo, config);
}

MSOPPROF_REPORT(mad_e5m2_e5m2, __cc__ void *c, __ca__ void *a, __cb__ void *b, uint64_t config)
{
    RecordMmadA5<InstrType::MAD_E5M2_E5M2, OperandType::DATA_E5M2>(memInfo, config);
}

MSOPPROF_REPORT(mad_mx_e1m2_e1m2, __cc__ void *c, __ca__ void *a, __cb__ void *b, uint64_t config)
{
    RecordMmadA5<InstrType::MAD_MX_E1M2X2_E1M2X2, OperandType::DATA_E1M2>(memInfo, config, 1);
}

MSOPPROF_REPORT(mad_mx_e1m2_e2m1, __cc__ void *c, __ca__ void *a, __cb__ void *b, uint64_t config)
{
    RecordMmadA5<InstrType::MAD_MX_E1M2X2_E2M1X2, OperandType::DATA_E1M2>(memInfo, config, 1);
}

MSOPPROF_REPORT(mad_mx_e2m1_e1m2, __cc__ void *c, __ca__ void *a, __cb__ void *b, uint64_t config)
{
    RecordMmadA5<InstrType::MAD_MX_E2M1X2_E1M2X2, OperandType::DATA_E2M1>(memInfo, config, 1);
}

MSOPPROF_REPORT(mad_mx_e2m1_e2m1, __cc__ void *c, __ca__ void *a, __cb__ void *b, uint64_t config)
{
    RecordMmadA5<InstrType::MAD_MX_E2M1X2_E2M1X2, OperandType::DATA_E2M1>(memInfo, config, 1);
}

MSOPPROF_REPORT(mad_mx_e4m3_e4m3, __cc__ void *c, __ca__ void *a, __cb__ void *b, uint64_t config)
{
    RecordMmadA5<InstrType::MAD_MX_E4M3_E4M3, OperandType::DATA_E4M3>(memInfo, config, 1);
}

MSOPPROF_REPORT(mad_mx_e4m3_e5m2, __cc__ void *c, __ca__ void *a, __cb__ void *b, uint64_t config)
{
    RecordMmadA5<InstrType::MAD_MX_E4M3_E5M2, OperandType::DATA_E4M3>(memInfo, config, 1);
}

MSOPPROF_REPORT(mad_mx_e5m2_e4m3, __cc__ void *c, __ca__ void *a, __cb__ void *b, uint64_t config)
{
    RecordMmadA5<InstrType::MAD_MX_E5M2_E4M3, OperandType::DATA_E5M2>(memInfo, config, 1);
}

MSOPPROF_REPORT(mad_mx_e5m2_e5m2, __cc__ void *c, __ca__ void *a, __cb__ void *b, uint64_t config)
{
    RecordMmadA5<InstrType::MAD_MX_E5M2_E5M2, OperandType::DATA_E5M2>(memInfo, config, 1);
}
