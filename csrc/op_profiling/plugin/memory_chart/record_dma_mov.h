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


#ifndef MSOPT_RECORD_DMA_MOV_H
#define MSOPT_RECORD_DMA_MOV_H

#include "plugin/utils.h"
#include "plugin/defs.h"
#include "recoder.h"

namespace Common {

template<MemType srcMemType, MemType dstMemType>
__aicore__ inline void RecordDmaMovEvent(RecordFixParams &&fixParams,
                                         uint64_t config, PadMode padMode, ByteMode byteMode)
{
    uint64_t block;
    if (!RecordPreCheck<DmaMovRecord>(fixParams.memInfo, block)) {
        return;
    }

    DmaMovRecord record{};
    record.dst = fixParams.dst;
    record.src = fixParams.src;
    record.pc = fixParams.pc;
    record.nBurst = (config >> 4) & 0xfff;
    record.lenBurst = (config >> 16) & 0xffff;
    record.srcStride = (config >> 32) & 0xffff;
    record.dstStride = (config >> 48) & 0xffff;
    record.coreID = static_cast<uint16_t>(block);
    record.dstMemType = dstMemType;
    record.srcMemType = srcMemType;
    record.padMode = padMode;
    record.byteMode = byteMode;
    DumpRecord<DmaMovRecord, RecordType::DMA_MOV>(fixParams.memInfo, record);
}

template<MemType srcMemType, MemType dstMemType, DataType dataType>
__aicore__ inline void RecordDmaMovNd2nzEvent(RecordFixParams &&fixParams, uint64_t xm, uint64_t xt)
{
    uint64_t block;
    if (!RecordPreCheck<DmaMovNd2nzRecord>(fixParams.memInfo, block)) {
        return;
    }

    DmaMovNd2nzRecord record{};
    record.dst = fixParams.dst;
    record.src = fixParams.src;
    record.pc = fixParams.pc;
    record.ndNum = (xm >> 4) & 0xfff;
    record.nValue = (xm >> 16) & 0xffff;
    record.dValue = (xm >> 32) & 0xffff;
    record.srcNdMatrixStride = (xm >> 48) & 0xffff;
    record.srcDValue = xt & 0xffff;
    record.dstNzC0Stride = (xt >> 16) & 0xffff;
    record.dstNzNStride = (xt >> 32) & 0xffff;
    record.dstNzMatrixStride = (xt >> 48) & 0xffff;
    record.coreID = static_cast<uint16_t>(block);
    record.srcMemType = srcMemType;
    record.dstMemType = dstMemType;
    record.dataType = dataType;
    DumpRecord<DmaMovNd2nzRecord, RecordType::DMA_MOV_ND2NZ>(fixParams.memInfo, record);
}

__aicore__ inline void SetMte2NzPara(EXTRA_PARAMS_DEC, uint64_t config)
{
    uint64_t block;
    if (!RecordPreCheck<DmaMovNd2nzDavRecord>(memInfo, block)) {
        return;
    }

    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ BlockHeader*>(memInfo + offset);
    header->mte2NzPara = config;
    Flush(memInfo);
}

__aicore__ inline uint64_t GetMte2NzPara(__gm__ const uint8_t *memInfo, uint64_t block)
{
    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ const BlockHeader*>(memInfo + offset);
    return header->mte2NzPara;
}

template<MemType srcMemType, MemType dstMemType, DataType dataType, uint8_t recordId>
__aicore__ inline void RecordDmaMovNdOrDn2nzDavEvent(RecordFixParams &&fixParams, uint64_t xm, uint64_t xt)
{
    uint64_t block;
    if (!RecordPreCheck<DmaMovNd2nzDavRecord>(fixParams.memInfo, block)) {
        return;
    }

    uint64_t mte2NzPara = GetMte2NzPara(fixParams.memInfo, block);
    DmaMovNd2nzDavRecord record{};
    record.dst = fixParams.dst;
    record.src = fixParams.src;
    record.pc = fixParams.pc;
    record.loop1SrcStride = GetUintFromConf<43, 4>(xm);
    record.nValue = GetUintFromConf<63, 48>(xm);
    record.dValue = GetUintFromConf<20, 0>(xt);
    record.loop4SrcStride = GetUintFromConf<60, 21>(xt);
    bool smallC0 = GetUintFromConf<61, 61>(xt);
    record.smallC0 = smallC0 && record.dValue <= 4;
    record.recordId = recordId;
    record.ndNum = GetUintFromConf<15, 0>(mte2NzPara);
    record.loop2DstStride = GetUintFromConf<31, 16>(mte2NzPara);
    record.loop3DstStride = GetUintFromConf<47, 32>(mte2NzPara);
    record.loop4DstStride = GetUintFromConf<63, 48>(mte2NzPara);
    record.coreID = static_cast<uint16_t>(block);
    record.srcMemType = srcMemType;
    record.dstMemType = dstMemType;
    record.dataType = dataType;
    DumpRecord<DmaMovNd2nzDavRecord, RecordType::DMA_MOV_ND2NZ_DAV>(fixParams.memInfo, record);
}

template<MemType srcMemType, MemType dstMemType, DataType dataType>
__aicore__ inline void RecordMovEvent(RecordFixParams &&fixParams, uint64_t config)
{
    uint64_t block;
    if (!RecordPreCheck<MovRecord>(fixParams.memInfo, block)) {
        return;
    }

    MovRecord record{};
    record.dst = fixParams.dst;
    record.src = fixParams.src;
    record.pc = fixParams.pc;
    record.srcGap = GetUintFromConf<47, 32, uint64_t>(config);
    record.dstGap = GetUintFromConf<63, 48, uint64_t>(config);
    record.lenBurst = GetUintFromConf<31, 16, uint64_t>(config);
    record.nBurst = GetUintFromConf<15, 4, uint64_t>(config);
    record.coreID = static_cast<uint16_t>(block);
    record.cvtEn = GetUintFromConf<3, uint64_t>(config);
    record.dataType = dataType;
    record.srcMemType = srcMemType;
    record.dstMemType = dstMemType;

    DumpRecord<MovRecord, RecordType::MOV_A5>(fixParams.memInfo, record);
}

template<MemType srcMemType, MemType dstMemType, DataType dataType>
__aicore__ inline void RecordMovV2Event(RecordFixParams &&fixParams, uint64_t config0, uint64_t config1)
{
    uint64_t block;
    if (!RecordPreCheck<MovV2Record>(fixParams.memInfo, block)) {
        return;
    }

    MovV2Record record{};
    record.dst = fixParams.dst;
    record.src = fixParams.src;
    record.pc = fixParams.pc;
    record.lenBurst = GetUintFromConf<41, 25, uint64_t>(config0);
    record.nBurst = GetUintFromConf<20, 4, uint64_t>(config0);
    record.sid = GetUintFromConf<3, 0, uint64_t>(config0);
    record.coreID = static_cast<uint16_t>(block);
    record.padMode = static_cast<PadMode>(GetUintFromConf<59, 56, uint64_t>(config0));
    record.dataType = dataType;
    record.srcStride = GetUintFromConf<35, 0, uint64_t>(config1);
    record.dstStride = GetUintFromConf<56, 40, uint64_t>(config1);
    record.srcMemType = srcMemType;
    record.dstMemType = dstMemType;

    DumpRecord<MovV2Record, RecordType::MOV_V2_A5>(fixParams.memInfo, record);
}

__aicore__ inline void SetPadCntNddma(EXTRA_PARAMS_DEC, uint64_t config)
{
    uint64_t block;
    if (!RecordPreCheck<NdDMAOut2UbRecord>(memInfo, block)) {
        return;
    }

    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ BlockHeader*>(memInfo + offset);
    header->padCntNdDma = config;
    Flush(memInfo);
}

__aicore__ inline uint64_t GetPadCntNddma(__gm__ const uint8_t *memInfo, uint64_t block)
{
    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ const BlockHeader*>(memInfo + offset);
    return header->padCntNdDma;
}

__aicore__ inline void SetLoop0StrideNddma(EXTRA_PARAMS_DEC, uint64_t config)
{
    uint64_t block;
    if (!RecordPreCheck<NdDMAOut2UbRecord>(memInfo, block)) {
        return;
    }

    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ BlockHeader*>(memInfo + offset);
    header->loop0StrideNdDma = config;
    Flush(memInfo);
}

__aicore__ inline uint64_t GetLoop0StrideNddma(__gm__ const uint8_t *memInfo, uint64_t block)
{
    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ const BlockHeader*>(memInfo + offset);
    return header->loop0StrideNdDma;
}

__aicore__ inline void SetLoop1StrideNddma(EXTRA_PARAMS_DEC, uint64_t config)
{
    uint64_t block;
    if (!RecordPreCheck<NdDMAOut2UbRecord>(memInfo, block)) {
        return;
    }

    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ BlockHeader*>(memInfo + offset);
    header->loop1StrideNdDma = config;
    Flush(memInfo);
}

__aicore__ inline uint64_t GetLoop1StrideNddma(__gm__ const uint8_t *memInfo, uint64_t block)
{
    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ const BlockHeader*>(memInfo + offset);
    return header->loop1StrideNdDma;
}
__aicore__ inline void SetLoop2StrideNddma(EXTRA_PARAMS_DEC, uint64_t config)
{
    uint64_t block;
    if (!RecordPreCheck<NdDMAOut2UbRecord>(memInfo, block)) {
        return;
    }

    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ BlockHeader*>(memInfo + offset);
    header->loop2StrideNdDma = config;
    Flush(memInfo);
}

__aicore__ inline uint64_t GetLoop2StrideNddma(__gm__ const uint8_t *memInfo, uint64_t block)
{
    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ const BlockHeader*>(memInfo + offset);
    return header->loop2StrideNdDma;
}
__aicore__ inline void SetLoop3StrideNddma(EXTRA_PARAMS_DEC, uint64_t config)
{
    uint64_t block;
    if (!RecordPreCheck<NdDMAOut2UbRecord>(memInfo, block)) {
        return;
    }

    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ BlockHeader*>(memInfo + offset);
    header->loop3StrideNdDma = config;
    Flush(memInfo);
}

__aicore__ inline uint64_t GetLoop3StrideNddma(__gm__ const uint8_t *memInfo, uint64_t block)
{
    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ const BlockHeader*>(memInfo + offset);
    return header->loop3StrideNdDma;
}
__aicore__ inline void SetLoop4StrideNddma(EXTRA_PARAMS_DEC, uint64_t config)
{
    uint64_t block;
    if (!RecordPreCheck<NdDMAOut2UbRecord>(memInfo, block)) {
        return;
    }

    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ BlockHeader*>(memInfo + offset);
    header->loop4StrideNdDma = config;
    Flush(memInfo);
}

__aicore__ inline uint64_t GetLoop4StrideNddma(__gm__ const uint8_t *memInfo, uint64_t block)
{
    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ const BlockHeader*>(memInfo + offset);
    return header->loop4StrideNdDma;
}

template<DataType dataType>
__aicore__ inline void RecordNdDMAOut2Ub(RecordFixParams &&fixParams, uint64_t config0, uint64_t config1)
{
    uint64_t block;
    if (!RecordPreCheck<NdDMAOut2UbRecord>(fixParams.memInfo, block)) {
        return;
    }

    NdDMAOut2UbRecord record{};
    record.dst = fixParams.dst;
    record.src = fixParams.src;
    record.dataType = dataType;
    uint64_t sprPadCntNdDma = GetPadCntNddma(fixParams.memInfo, block);
    uint64_t sprLoopStrideNdDma[5] = {GetLoop0StrideNddma(fixParams.memInfo, block),
                                      GetLoop1StrideNddma(fixParams.memInfo, block),
                                      GetLoop2StrideNddma(fixParams.memInfo, block),
                                      GetLoop3StrideNddma(fixParams.memInfo, block),
                                      GetLoop4StrideNddma(fixParams.memInfo, block)};

    record.loop[0].loopSize = GetUintFromConf<23, 4>(config0);
    record.loop[1].loopSize = GetUintFromConf<43, 24>(config0);
    record.loop[2].loopSize = GetUintFromConf<63, 44>(config0);
    record.loop[3].loopSize = GetUintFromConf<19, 0>(config1);
    record.loop[4].loopSize = GetUintFromConf<39, 20>(config1);
    record.loop[0].loopLpSize = GetUintFromConf<47, 40>(config1);
    record.loop[0].loopRpSize = GetUintFromConf<55, 48>(config1);
    record.loop[1].loopLpSize = GetUintFromConf<7, 0>(sprPadCntNdDma);
    record.loop[1].loopRpSize = GetUintFromConf<15, 8>(sprPadCntNdDma);
    record.loop[2].loopLpSize = GetUintFromConf<23, 16>(sprPadCntNdDma);
    record.loop[2].loopRpSize = GetUintFromConf<31, 24>(sprPadCntNdDma);
    record.loop[3].loopLpSize = GetUintFromConf<39, 32>(sprPadCntNdDma);
    record.loop[3].loopRpSize = GetUintFromConf<47, 40>(sprPadCntNdDma);
    record.loop[4].loopLpSize = GetUintFromConf<55, 48>(sprPadCntNdDma);
    record.loop[4].loopRpSize = GetUintFromConf<63, 56>(sprPadCntNdDma);
    for (size_t i = 0; i < NdDMAOut2UbRecord::LOOP; ++i) {
        record.loop[i].loopDstStride = GetUintFromConf<19, 0>(sprLoopStrideNdDma[i]);
        record.loop[i].loopSrcStride = GetUintFromConf<59, 20>(sprLoopStrideNdDma[i]);
    }
    DumpRecord<NdDMAOut2UbRecord,  RecordType::ND_DMA_OUT_TO_UB>(fixParams.memInfo, record);
}
} // namespace Common
#endif // MSOPT_RECORD_DMA_MOV_H
