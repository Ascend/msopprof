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


#ifndef MSOPT_RECORD_MOV_ALIGN_H
#define MSOPT_RECORD_MOV_ALIGN_H

#include "plugin/utils.h"
#include "plugin/defs.h"
#include "recoder.h"

namespace Common {

template<MemType srcMemType, MemType dstMemType, DataType dataType>
AICORE_FUNC_HEAD void RecordMovAlignEvent(RecordFixParams &&fixParams, uint64_t config, uint64_t gapConfig)
{
    uint64_t block;
    if (!RecordPreCheck<MovAlignRecord>(fixParams.memInfo, block)) {
        return;
    }

    MovAlignRecord record{};
    record.dst = fixParams.dst;
    record.src = fixParams.src;
    record.pc = fixParams.pc;
    record.srcGap = gapConfig & 0xffffffff;
    record.dstGap = (gapConfig >> 32) & 0xffffffff;
    record.lenBurst = (config >> 16) & 0x1fffff;
    record.nBurst = (config >> 4) & 0xfff;
    record.coreID = static_cast<uint16_t>(block);
    record.dstMemType = dstMemType;
    record.srcMemType = srcMemType;
    record.dataType = dataType;
    record.leftPaddingNum = (config >> 48) & 0x3f;
    record.rightPaddingNum = (config >> 54) & 0x3f;
    DumpRecord<MovAlignRecord, RecordType::MOV_ALIGN>(fixParams.memInfo, record);
}

AICORE_FUNC_HEAD void SetLoopSizeOuttol1(EXTRA_PARAMS_DEC, uint64_t config)
{
    uint64_t block;
    if (!RecordPreCheck<MovAlignV2Record>(memInfo, block)) {
        return;
    }

    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ BlockHeader*>(memInfo + offset);
    header->loopSizeOuttol1 = config;
    Flush(memInfo);
}

AICORE_FUNC_HEAD uint64_t GetLoopSizeOuttol1(__gm__ const uint8_t *memInfo, uint64_t block)
{
    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ const BlockHeader*>(memInfo + offset);
    return header->loopSizeOuttol1;
}

AICORE_FUNC_HEAD void SetLoop1StrideOuttol1(EXTRA_PARAMS_DEC, uint64_t config)
{
    uint64_t block;
    if (!RecordPreCheck<MovAlignV2Record>(memInfo, block)) {
        return;
    }

    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ BlockHeader*>(memInfo + offset);
    header->loop1StrideOuttol1 = config;
    Flush(memInfo);
}

AICORE_FUNC_HEAD uint64_t GetLoop1StrideOuttol1(__gm__ const uint8_t *memInfo, uint64_t block)
{
    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ const BlockHeader*>(memInfo + offset);
    return header->loop1StrideOuttol1;
}

AICORE_FUNC_HEAD void SetLoop2StrideOuttol1(EXTRA_PARAMS_DEC, uint64_t config)
{
    uint64_t block;
    if (!RecordPreCheck<MovAlignV2Record>(memInfo, block)) {
        return;
    }

    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ BlockHeader*>(memInfo + offset);
    header->loop2StrideOuttol1 = config;
    Flush(memInfo);
}

AICORE_FUNC_HEAD uint64_t GetLoop2StrideOuttol1(__gm__ const uint8_t *memInfo, uint64_t block)
{
    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ const BlockHeader*>(memInfo + offset);
    return header->loop2StrideOuttol1;
}

AICORE_FUNC_HEAD void SetLoopSizeUbToOut(EXTRA_PARAMS_DEC, uint64_t config)
{
    uint64_t block;
    if (!RecordPreCheck<MovAlignV2Record>(memInfo, block)) {
        return;
    }

    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ BlockHeader*>(memInfo + offset);
    header->loopSizeUbToOut = config;
    Flush(memInfo);
}

AICORE_FUNC_HEAD uint64_t GetLoopSizeUbToOut(__gm__ const uint8_t *memInfo, uint64_t block)
{
    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ const BlockHeader*>(memInfo + offset);
    return header->loopSizeUbToOut;
}

AICORE_FUNC_HEAD void SetLoop1StrideUbToOut(EXTRA_PARAMS_DEC, uint64_t config)
{
    uint64_t block;
    if (!RecordPreCheck<MovAlignV2Record>(memInfo, block)) {
        return;
    }

    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ BlockHeader*>(memInfo + offset);
    header->loop1StrideUbToOut = config;
    Flush(memInfo);
}

AICORE_FUNC_HEAD uint64_t GetLoop1StrideUbToOut(__gm__ const uint8_t *memInfo, uint64_t block)
{
    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ const BlockHeader*>(memInfo + offset);
    return header->loop1StrideUbToOut;
}

AICORE_FUNC_HEAD void SetLoop2StrideUbToOut(EXTRA_PARAMS_DEC, uint64_t config)
{
    uint64_t block;
    if (!RecordPreCheck<MovAlignV2Record>(memInfo, block)) {
        return;
    }

    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ BlockHeader*>(memInfo + offset);
    header->loop2StrideUbToOut = config;
    Flush(memInfo);
}

AICORE_FUNC_HEAD uint64_t GetLoop2StrideUbToOut(__gm__ const uint8_t *memInfo, uint64_t block)
{
    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ const BlockHeader*>(memInfo + offset);
    return header->loop2StrideUbToOut;
}

AICORE_FUNC_HEAD void SetLoopSizeOuttoub(EXTRA_PARAMS_DEC, uint64_t config)
{
    uint64_t block;
    if (!RecordPreCheck<MovAlignV2Record>(memInfo, block)) {
        return;
    }

    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ BlockHeader*>(memInfo + offset);
    header->loopSizeOuttoub = config;
    Flush(memInfo);
}

AICORE_FUNC_HEAD uint64_t GetLoopSizeOuttoub(__gm__ const uint8_t *memInfo, uint64_t block)
{
    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ const BlockHeader*>(memInfo + offset);
    return header->loopSizeOuttoub;
}

AICORE_FUNC_HEAD void SetLoop1StrideOuttoub(EXTRA_PARAMS_DEC, uint64_t config)
{
    uint64_t block;
    if (!RecordPreCheck<MovAlignV2Record>(memInfo, block)) {
        return;
    }

    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ BlockHeader*>(memInfo + offset);
    header->loop1StrideOuttoub = config;
    Flush(memInfo);
}

AICORE_FUNC_HEAD uint64_t GetLoop1StrideOuttoub(__gm__ const uint8_t *memInfo, uint64_t block)
{
    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ const BlockHeader*>(memInfo + offset);
    return header->loop1StrideOuttoub;
}

AICORE_FUNC_HEAD void SetLoop2StrideOuttoub(EXTRA_PARAMS_DEC, uint64_t config)
{
    uint64_t block;
    if (!RecordPreCheck<MovAlignV2Record>(memInfo, block)) {
        return;
    }

    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ BlockHeader*>(memInfo + offset);
    header->loop2StrideOuttoub = config;
    Flush(memInfo);
}

AICORE_FUNC_HEAD uint64_t GetLoop2StrideOuttoub(__gm__ const uint8_t *memInfo, uint64_t block)
{
    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ const BlockHeader*>(memInfo + offset);
    return header->loop2StrideOuttoub;
}

template<MemType srcMemType, MemType dstMemType, DataType dataType>
AICORE_FUNC_HEAD void RecordMovAlignV2Event(RecordFixParams &&fixParams, uint64_t config, uint64_t strideConfig)
{
    uint64_t block;
    if (!RecordPreCheck<MovAlignV2Record>(fixParams.memInfo, block)) { return; }

    uint64_t loopSize = 0;
    uint64_t loop1Stride = 0;
    uint64_t loop2Stride = 0;
    if (dstMemType == MemType::GM) {
        // MOV_UB_TO_OUT_ALIGN_V2
        loopSize = GetLoopSizeUbToOut(fixParams.memInfo, block);
        loop1Stride = GetLoop1StrideUbToOut(fixParams.memInfo, block);
        loop2Stride = GetLoop2StrideUbToOut(fixParams.memInfo, block);
    }  else if (dstMemType == MemType::L1) {
        // MOV_OUT_TO_L1_ALIGN_V2
        loopSize = GetLoopSizeOuttol1(fixParams.memInfo, block);
        loop1Stride = GetLoop1StrideOuttol1(fixParams.memInfo, block);
        loop2Stride = GetLoop2StrideOuttol1(fixParams.memInfo, block);
    } else if (dstMemType == MemType::UB) {
        loopSize = GetLoopSizeOuttoub(fixParams.memInfo, block);
        loop1Stride = GetLoop1StrideOuttoub(fixParams.memInfo, block);
        loop2Stride = GetLoop2StrideOuttoub(fixParams.memInfo, block);
    }
    MovAlignV2Record record{};
    record.dst = fixParams.dst;
    record.src = fixParams.src;
    record.pc = fixParams.pc;
    record.nBurst = GetUintFromConf<24, 4>(config);
    record.lenBurst = GetUintFromConf<45, 25>(config);
    record.loop1Size = GetUintFromConf<20, 0>(loopSize);
    record.loop2Size = GetUintFromConf<42, 21>(loopSize);
    record.srcMemType = srcMemType;
    record.dstMemType = dstMemType;
    record.dataType = dataType;
    record.coreID = static_cast<uint16_t>(block);
    if (dstMemType == MemType::GM) {
        // mov ub -> gm
        record.dstStride = GetUintFromConf<39, 0>(strideConfig);
        record.srcStride = GetUintFromConf<60, 40>(strideConfig);
        record.loop1DstStride = GetUintFromConf<39, 0>(loop1Stride);
        record.loop1SrcStride = GetUintFromConf<60, 40>(loop1Stride);
        record.loop2DstStride = GetUintFromConf<39, 0>(loop2Stride);
        record.loop2SrcStride = GetUintFromConf<60, 40>(loop2Stride);
    } else {
        // mov gm -> ub or gm -> l1
        record.srcStride = GetUintFromConf<39, 0>(strideConfig);
        record.dstStride = GetUintFromConf<60, 40>(strideConfig);
        record.loop1SrcStride = GetUintFromConf<39, 0>(loop1Stride);
        record.loop1DstStride = GetUintFromConf<60, 40>(loop1Stride);
        record.loop2SrcStride = GetUintFromConf<39, 0>(loop2Stride);
        record.loop2DstStride = GetUintFromConf<60, 40>(loop2Stride);
        record.leftPaddingNum = GetUintFromConf<51, 46>(config);
        record.rightPaddingNum = GetUintFromConf<57, 52>(config);
    }
    DumpRecord<MovAlignV2Record, RecordType::MOV_ALIGN_V2>(fixParams.memInfo, record);
}

} // namespace Common
#endif // MSOPT_RECORD_MOV_ALIGN_H
