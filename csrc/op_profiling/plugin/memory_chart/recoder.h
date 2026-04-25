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


#ifndef MSOPT_MEMORY_RECODER_H
#define MSOPT_MEMORY_RECODER_H

#include "plugin/defs.h"
#include "common/dbi_defs.h"

namespace Common {
struct RecordFixParams {
    __gm__ uint8_t *memInfo;
    uint64_t pc;
    uint32_t bid;
    uint64_t dst;
    uint64_t src;
};

// 要写入gm的结构体保证4/8字节对齐，写入单字节数据大概率导致所有数据丢失
template <typename Record>
AICORE_FUNC_HEAD void WriteData(__gm__ uint8_t *memInfo, const Record &record)
{
    static_assert(sizeof(record) % 4 == 0, "record type write into GM must 4/8B aligned");
    constexpr uint32_t step = sizeof(Record) / sizeof(uint32_t);
    constexpr uint32_t tail = sizeof(Record) - step * sizeof(uint32_t);

    auto dst = reinterpret_cast<__gm__ uint32_t *>(memInfo);
    auto src = reinterpret_cast<const uint32_t *>(static_cast<const Record*>(&record));
    for (uint32_t i = 0; i < step; ++i) {
        *dst++ = *src++;
    }
}

template<typename Record>
AICORE_FUNC_HEAD bool RecordPreCheck(__gm__ uint8_t *memInfo, uint64_t &blockIdx)
{
    if (!CheckMemInfo(memInfo) || !TryGetBlockIdx(blockIdx)) {
        return false;
    }

    static_assert(MAX_BLOCK_DATA_SIZE > sizeof(BlockHeader) + sizeof(RecordHeader) + sizeof(Record),
            "size of record type must slower than preset capacity");
    constexpr uint64_t maxLength = MAX_BLOCK_DATA_SIZE - sizeof(BlockHeader) - sizeof(RecordHeader) - sizeof(Record);
    uint64_t offset = blockIdx * BLOCK_MEM_SIZE;
    auto header = reinterpret_cast<__gm__ BlockHeader*>(memInfo + offset);
    uint64_t length = header->length;
    if ((length & DBI_RECORD_OVERFLOW_BIT) != 0) {
        length++;
    } else if (length > maxLength) {
        length = header->count + 1;
        length |= DBI_RECORD_OVERFLOW_BIT;
    } else {
        return true;
    }

    header->length = length;
    Flush(memInfo);
    return false;
}

template<typename Record, RecordType recordType>
AICORE_FUNC_HEAD void DumpRecord(__gm__ uint8_t *memInfo, const Record &record)
{
    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(record.coreID);
    auto header = reinterpret_cast<__gm__ BlockHeader*>(memInfo + offset);

    offset += sizeof(BlockHeader) + header->length;
    RecordHeader rh;
    rh.recordType = recordType;
    WriteData<RecordHeader>(memInfo + offset, rh);

    offset += sizeof(RecordHeader);
    WriteData<Record>(memInfo + offset, record);

    header->count++;
    header->length += sizeof(RecordHeader) + sizeof(Record);
    Flush(memInfo);
}

} // namespace Common
#endif // MSOPT_MEMORY_RECODER_H
