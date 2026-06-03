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

#ifndef MSOPT_OPERAND_RECODER_H
#define MSOPT_OPERAND_RECODER_H

#include "plugin/defs.h"
#include "plugin/utils.h"
#include "recoder.h"
#include "common/dbi_defs.h"
#include "kernel_injection/include/MSBit.h"
namespace Common {
template<OperandType dataType, SimtType simtType>
AICORE_FUNC_HEAD void RecordAtomRedEvent(EXTRA_PARAMS_DEC, MemType memType, uint64_t addr)
{
    uint64_t block;
    if (!RecordPreCheck<AtomRedRecord>(memInfo, block)) {
        return;
    }

    AtomRedRecord record{};
    uint8_t dataSize = GetDataBits(dataType) / 8;
    record.pc = pc;
    record.memType = memType;
    record.coreID = static_cast<uint16_t>(block);
    record.addr = addr;
    record.size = dataSize;
    record.simtType = simtType;
    record.dataType = dataType;
    DumpRecord<AtomRedRecord, RecordType::SIMT_ATOM_RED>(memInfo, record);
}

template<OperandType dataType, SimtType simtType>
AICORE_FUNC_HEAD void RecordLoadStoreEvent(RecordFixParams &&fixParams, MemType memType, int64_t offset)
{
    uint64_t block;
    if (!RecordPreCheck<LoadStoreRecord>(fixParams.memInfo, block)) {
        return;
    }

    LoadStoreRecord record{};
    record.reg = fixParams.dst;
    uint8_t dataSize = GetDataBits(dataType) / 8;
    record.pc = fixParams.pc;
    record.memType = memType;
    record.coreID = static_cast<uint16_t>(block);
    record.addr = fixParams.src + offset * dataSize;
    record.size = dataSize;
    record.simtType = simtType;
    record.dataType = dataType;
    DumpRecord<LoadStoreRecord, RecordType::SIMT_LOAD_STORE>(fixParams.memInfo, record);
}

/**
 * @brief SIMT LD ST地址重映射函数，根据地址范围决定内存类型并调整地址偏移
 * @param Rn 源地址寄存器
 * @param offset #ofst
 * @param dataType size_of_data_type
 * @return MemType 地址所属的内存空间类型
 */
AICORE_FUNC_HEAD MemType SimtRemapAddress(uint64_t &Rn, int64_t offset, uint8_t dataType) {
    uint64_t addr = Rn + offset * dataType;
    uint64_t sysVaBase = GetSysVaBase();
    constexpr uint64_t SHARED_MEM_START = 0x80000;      // Shared Memory 起始偏移
    constexpr uint64_t SHARED_MEM_END = 0x100000;       // Shared Memory 结束偏移
    constexpr uint64_t STACK_MEM_START = 0x100000;      // Stack Memory 起始偏移
    constexpr uint64_t STACK_MEM_END = 0x1000000;       // Stack Memory 结束偏移

    // 检查是否属于 Shared Memory（LDS STS访问）
    if (addr >= (sysVaBase + SHARED_MEM_START) && addr < (sysVaBase + SHARED_MEM_END)) {
        Rn -= (sysVaBase + SHARED_MEM_START);  // 调整为 LDS 偏移地址
        return MemType::UB;
    }

    // 检查是否属于 Stack Memory（LDK STK访问）
    if (addr >= (sysVaBase + STACK_MEM_START) && addr < (sysVaBase + STACK_MEM_END)) {
        Rn -= (sysVaBase + STACK_MEM_START);   // 调整为 LDK 偏移地址
        return MemType::PRIVATE;
    }

    // 检查是否属于异常访问
    if (addr >= sysVaBase && addr < (sysVaBase + SHARED_MEM_START)) {
        return MemType::INVALID;
    }

    // 其余情况属于 Global Memory（LDG STG访问）
    return MemType::GM;
}
}
#endif // MSOPT_OPERAND_RECODER_H
