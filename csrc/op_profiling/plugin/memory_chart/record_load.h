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


#ifndef MSOPT_RECORD_LOAD_H
#define MSOPT_RECORD_LOAD_H

#include "plugin/utils.h"
#include "plugin/defs.h"
#include "recoder.h"

namespace Common {

template<MemType srcMemType, MemType dstMemType>
AICORE_FUNC_HEAD void RecordLoad2DEvent(RecordFixParams &&fixParams, uint64_t config, AddrCalMode addrCalMode)
{
    uint64_t block;
    if (!RecordPreCheck<Load2DRecord>(fixParams.memInfo, block)) {
        return;
    }

    Load2DRecord record{};
    record.dst = fixParams.dst;
    record.src = fixParams.src;
    record.pc = fixParams.pc;
    record.coreID = static_cast<uint16_t>(block);
    record.baseIdx = config & 0xffff;
    record.srcStride = (config >> 24U) & 0xffff;
    record.dstStride = (config >> 44U) & 0xffff;
    record.repeat = (config >> 16U) & 0xff;
    record.dstMemType = dstMemType;
    record.srcMemType = srcMemType;
    record.addrCalMode = addrCalMode;
    DumpRecord<Load2DRecord, RecordType::LOAD_2D>(fixParams.memInfo, record);
}

template<MemType srcMemType, MemType dstMemType, DataType dataType>
AICORE_FUNC_HEAD void RecordLoadL1ToL0B2DTransposeEvent(RecordFixParams &&fixParams, uint64_t config,
                                                         uint64_t fracStride)
{
    uint64_t block;
    if (!RecordPreCheck<Load2DTransposeRecord>(fixParams.memInfo, block)) {
        return;
    }

    Load2DTransposeRecord record{};
    record.dst = fixParams.dst;
    record.src = fixParams.src;
    record.pc = fixParams.pc;
    record.coreID = static_cast<uint16_t>(block);
    record.baseIdx = GetUintFromConf<15, 0, uint64_t>(config);
    record.dstStride = GetUintFromConf<59, 44, uint64_t>(config) + 1;
    record.srcStride = GetUintFromConf<39, 24, uint64_t>(config);
    record.repeat = GetUintFromConf<23, 16, uint64_t>(config);
    record.dstMemType = dstMemType;
    record.srcMemType = srcMemType;
    record.dataType = dataType;
    DumpRecord<Load2DTransposeRecord, RecordType::LOAD_L1_TO_L0B_2D_TRANSPOSE>(fixParams.memInfo, record);
}

template<MemType srcMemType, MemType dstMemType, DataType dataType>
AICORE_FUNC_HEAD void RecordLoad2DV2Event(RecordFixParams &&fixParams, uint64_t config0, uint64_t config1,
                                                  bool transpose)
{
    uint64_t block;
    if (!RecordPreCheck<Load2DV2Record>(fixParams.memInfo, block)) {
        return;
    }

    Load2DV2Record record{};
    record.dst = fixParams.dst;
    record.src = fixParams.src;
    record.pc = fixParams.pc;
    record.coreID = static_cast<uint16_t>(block);
    record.mStartPos = GetUintFromConf<15, 0, uint64_t>(config0);
    record.kStartPos = GetUintFromConf<31, 16, uint64_t>(config0);
    record.mStep = GetUintFromConf<39, 32, uint64_t>(config0);
    record.kStep = GetUintFromConf<47, 40, uint64_t>(config0);
    record.srcStride = GetUintFromConf<15, 0, uint64_t>(config1);
    record.dstStride = GetUintFromConf<31, 16, uint64_t>(config1);
    record.dstMemType = dstMemType;
    record.srcMemType = srcMemType;
    record.dataType = dataType;
    record.transpose = transpose;
    DumpRecord<Load2DV2Record, RecordType::LOAD_2DV2>(fixParams.memInfo, record);
}

template<MemType srcMemType, MemType dstMemType>
AICORE_FUNC_HEAD void RecordLoadMX2DV2Event(RecordFixParams &&fixParams, uint64_t config0, uint64_t config1)
{
    uint64_t block;
    if (!RecordPreCheck<LoadMX2DV2Record>(fixParams.memInfo, block)) {
        return;
    }

    LoadMX2DV2Record record{};
    record.dst = fixParams.dst;
    record.src = fixParams.src;
    record.pc = fixParams.pc;
    record.coreID = static_cast<uint16_t>(block);
    record.xStartPos = GetUintFromConf<15, 0, uint64_t>(config0);
    record.yStartPos = GetUintFromConf<31, 16, uint64_t>(config0);
    record.xStep = GetUintFromConf<39, 32, uint64_t>(config0);
    record.yStep = GetUintFromConf<47, 40, uint64_t>(config0);
    record.srcStride = GetUintFromConf<15, 0, uint64_t>(config1);
    record.dstStride = GetUintFromConf<31, 16, uint64_t>(config1);
    record.dstMemType = dstMemType;
    record.srcMemType = srcMemType;
    DumpRecord<LoadMX2DV2Record, RecordType::LOAD_MX2DV2>(fixParams.memInfo, record);
}

AICORE_FUNC_HEAD void SetMte2SrcPara(EXTRA_PARAMS_DEC, uint64_t config)
{
    uint64_t block;
    if (!RecordPreCheck<Load2DV2DecRecord>(memInfo, block)) {
        return;
    }

    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ BlockHeader*>(memInfo + offset);
    header->mte2SrcPara = config;
    Flush(memInfo);
}

AICORE_FUNC_HEAD uint64_t GetMte2SrcPara(__gm__ const uint8_t *memInfo, uint64_t block)
{
    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ const BlockHeader*>(memInfo + offset);
    return header->mte2SrcPara;
}

template<MemType srcMemType, MemType dstMemType>
AICORE_FUNC_HEAD void RecordLoad2DV2DecEvent(RecordFixParams &&fixParams, uint64_t config0, uint64_t config1)
{
    uint64_t block;
    if (!RecordPreCheck<Load2DV2DecRecord>(fixParams.memInfo, block)) {
        return;
    }

    uint64_t mte2SrcPara = GetMte2SrcPara(fixParams.memInfo, block);
    Load2DV2DecRecord record{};
    record.dst = fixParams.dst;
    record.src = fixParams.src;
    record.pc = fixParams.pc;
    record.coreID = static_cast<uint16_t>(block);
    record.mStartPos = GetUintFromConf<31, 0, uint64_t>(config0);
    record.kStartPos = GetUintFromConf<63, 32, uint64_t>(config0);
    record.mStep = GetUintFromConf<23, 12, uint64_t>(config1);
    record.kStep = GetUintFromConf<35, 24, uint64_t>(config1);
    record.srcStride = GetUintFromConf<31, 0, uint64_t>(mte2SrcPara);
    record.dstStride = GetUintFromConf<11, 0, uint64_t>(config1);
    record.decompMode = GetUintFromConf<42, 40, uint64_t>(config1);
    record.dstMemType = dstMemType;
    record.srcMemType = srcMemType;
    DumpRecord<Load2DV2DecRecord, RecordType::LOAD_2DV2_DEC>(fixParams.memInfo, record);
}

template<MemType srcMemType, MemType dstMemType, DataType dataType>
AICORE_FUNC_HEAD void RecordSet2DEvent(RecordFixParams &&fixParams, int64_t config)
{
    uint64_t block;
    if (!RecordPreCheck<Set2DRecord>(fixParams.memInfo, block)) {
        return;
    }

    Set2DRecord record{};
    record.dst = fixParams.dst;
    record.src = fixParams.src;
    record.pc = fixParams.pc;
    record.coreID = static_cast<uint16_t>(block);
    record.blockNum = GetUintFromConf<30, 16, int64_t>(config);
    record.repeat = GetUintFromConf<14, 0, int64_t>(config);
    record.repeatGap = GetUintFromConf<46, 32, int64_t>(config);
    record.dstMemType = dstMemType;
    record.srcMemType = srcMemType;
    record.dataType = dataType;
    DumpRecord<Set2DRecord, RecordType::SET_2D>(fixParams.memInfo, record);
}

} // namespace Common

#endif // MSOPT_RECORD_LOAD_2D_H
