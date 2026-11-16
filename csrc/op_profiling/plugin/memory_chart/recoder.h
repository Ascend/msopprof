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

    static_assert(sizeof(RecordHeader) + sizeof(Record) <=
            (MC_SEGMENT_AREA / (MAX_THREAD_NUM + 1) & ~0x7ULL) - MC_SEGMENT_HEADER_SIZE,
            "size of record type must be smaller than the smallest thread segment capacity");
    return true;
}

// 分区槽位与 OperandRecord 一致：0 号槽固定给 SIMD（非 SIMT）记录，SIMT 线程 threadId 用第 threadId + 1 个槽。
// 布局标识由 host 在 launch 前写入 BlockHeader.threadNum（0 或 MAX_THREAD_NUM），整个采集期间只读，
// 因此同一 block 内不会出现布局漂移；同一分区只有一个写者，count/length 直接用普通读改写，无需原子操作。
// 纯 SIMD 布局（layoutId == 0）时只有 0 号槽，独占整个 block。
template<typename Record, RecordType recordType>
AICORE_FUNC_HEAD void DumpRecord(__gm__ uint8_t *memInfo, const Record &record)
{
    constexpr uint64_t recordBytes = sizeof(RecordHeader) + sizeof(Record);
    __gm__ uint8_t *blockBase = memInfo + static_cast<uint64_t>(record.coreID) * BLOCK_MEM_SIZE;
    const uint64_t layoutId = reinterpret_cast<__gm__ BlockHeader *>(blockBase)->threadNum;
    if (!MemoryChartLayoutIsValid(layoutId)) {
        return; // 布局非法（host 未预写或写入异常），丢弃本条记录
    }

    uint64_t segIdx = 0;
#ifdef SIMT_MODE
    // 纯 SIMD 布局没有 SIMT 槽位；按 slot 数判断可避免 0 号槽被 SIMT 记录误用
    uint64_t threadId = 0;
    if (MemoryChartSegmentNum(layoutId) <= 1U || !TryGetThreadId(threadId) || threadId >= MAX_THREAD_NUM) {
        return; // 线程号非法时丢弃本条记录，避免写到其他线程的分区
    }
    segIdx = threadId + 1;
#endif

    const uint64_t capacity = MemoryChartSegmentCapacity(MC_SEGMENT_AREA, layoutId);
    const uint64_t segOffset = MemoryChartSegmentOffsetInBlock(MC_SEGMENT_AREA, segIdx, layoutId);
    auto header = reinterpret_cast<__gm__ MemoryChartSegmentHeader *>(blockBase + segOffset);

    if (header->length > capacity || recordBytes > capacity - header->length) {
        header->dropped++;
        Flush(memInfo);
        return;
    }

    uint64_t offset = MC_SEGMENT_HEADER_SIZE + header->length;
    RecordHeader rh;
    rh.recordType = recordType;
    WriteData<RecordHeader>(blockBase + segOffset + offset, rh);

    offset += sizeof(RecordHeader);
    WriteData<Record>(blockBase + segOffset + offset, record);

    header->count++;
    header->length += recordBytes;
    Flush(memInfo);
}

} // namespace Common
#endif // MSOPT_MEMORY_RECODER_H
