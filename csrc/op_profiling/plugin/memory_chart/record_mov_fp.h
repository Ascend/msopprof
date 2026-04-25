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


#ifndef MSOPT_RECORD_MOV_FP_H
#define MSOPT_RECORD_MOV_FP_H

#include "plugin/utils.h"
#include "plugin/defs.h"
#include "recoder.h"

namespace Common {

AICORE_FUNC_HEAD void SetNdPara(EXTRA_PARAMS_DEC, uint64_t config)
{
    uint64_t block;
    if (!RecordPreCheck<MovFpRecord>(memInfo, block)) {
        return;
    }

    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ BlockHeader*>(memInfo + offset);
    header->ndPara = config;
    Flush(memInfo);
}

AICORE_FUNC_HEAD uint64_t GetNdPara(__gm__ const uint8_t *memInfo, uint64_t block)
{
    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ const BlockHeader*>(memInfo + offset);
    return header->ndPara;
}

AICORE_FUNC_HEAD void SetLoop3Para(EXTRA_PARAMS_DEC, uint64_t config)
{
    uint64_t block;
    if (!RecordPreCheck<FixL0CGMRecord>(memInfo, block)) {
        return;
    }

    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ BlockHeader*>(memInfo + offset);
    header->loop3Para = config;
    Flush(memInfo);
}

AICORE_FUNC_HEAD uint64_t GetLoop3Para(__gm__ const uint8_t *memInfo, uint64_t block)
{
    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ const BlockHeader*>(memInfo + offset);
    return header->loop3Para;
}

AICORE_FUNC_HEAD void SetChannelPara(EXTRA_PARAMS_DEC, uint64_t config)
{
    uint64_t block;
    if (!RecordPreCheck<FixL0CGMRecord>(memInfo, block)) {
        return;
    }

    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ BlockHeader*>(memInfo + offset);
    header->channelPara = config;
    Flush(memInfo);
}

AICORE_FUNC_HEAD uint64_t GetChannelPara(__gm__ const uint8_t *memInfo, uint64_t block)
{
    uint64_t offset = BLOCK_MEM_SIZE * static_cast<uint64_t>(block);
    auto header = reinterpret_cast<__gm__ const BlockHeader*>(memInfo + offset);
    return header->channelPara;
}

// 根据配置的quantPRE，设置搬运目的数据类型长度
AICORE_FUNC_HEAD void ParseMovfpQuantBits(const uint64_t quantPRE, MovFpRecord& record)
{
    // 1:f32->f16; 10/11: s32->f16; 12/13: s32->s16; 16: f32->bf16;
    if (quantPRE == 1 || quantPRE == 10 || quantPRE == 11 || quantPRE == 12 || quantPRE == 13 || quantPRE == 16) {
        record.quantPreBits = 16;  // f16/s16/bf16
        // 8/9: s32->s8/u8; 23/24: f32->s8/u8
    } else if (quantPRE == 8 || quantPRE == 9 || quantPRE == 23 || quantPRE == 24) {
        record.int8ChannelMerge = !record.enNZ2ND;
        record.quantPreBits = 8;  // s8/u8
        // 21/22: s32->s4; 25/26: f32->s4
    } else if (quantPRE == 21 || quantPRE == 22 || quantPRE == 25 || quantPRE == 26) {
        record.int4ChannelMerge = !record.enNZ2ND;
        record.quantPreBits = 4;  // s4
    } else {
        record.quantPreBits = 32;  // f32/s32
    }
}

AICORE_FUNC_HEAD void ParseFixfpQuantBits(uint64_t quantPRE, bool enNDorDN, FixL0CGMRecord& record)
{
    // 1:f32->f16; 10/11: s32->f16; 12/13: s32->s16; 16: f32->bf16;
    if (quantPRE == 1 || quantPRE == 10 || quantPRE == 11 || quantPRE == 12 || quantPRE == 13 || quantPRE == 16) {
        record.quantPreBits = 16;  // f16/s16/bf16
        // 8/9: s32->s8/u8; 23/24: f32->s8/u8
    } else if (quantPRE == 8 || quantPRE == 9 || quantPRE == 23 || quantPRE == 24) {
        record.int8ChannelMerge = !record.enNZ2ND;
        record.int8ChannelMerge = enNDorDN ? false : true;
        // 21/22: s32->s4; 25/26: f32->s4
    } else if (quantPRE == 21 || quantPRE == 22 || quantPRE == 25 || quantPRE == 26) {
        record.int4ChannelMerge = enNDorDN ? false : true;
        record.quantPreBits = 4;  // s4
    } else {
        record.quantPreBits = 32;  // f32/s32
    }
}

template<AtomicMode atomicMode>
AICORE_FUNC_HEAD void RecordMovFpEvent(RecordFixParams &&fixParams, uint64_t xm, uint64_t xt)
{
    uint64_t block;
    if (!RecordPreCheck<MovFpRecord>(fixParams.memInfo, block)) {
        return;
    }
    uint8_t unitFlag = (xt >> 32) & 0x3;
    bool channelSplit = (xt >> 42) & 0x1;
    uint64_t ndPara = GetNdPara(fixParams.memInfo, block);

    MovFpRecord record{};
    record.dst = fixParams.dst;
    record.src = fixParams.src;
    record.pc = fixParams.pc;
    record.dstStride = (xm >> 32) & 0xffffffff;
    record.srcStride = xt & 0xffff;
    record.nSize = (xm >> 4) & 0xfff;
    record.mSize = (xm >> 16) & 0xffff;
    record.coreID = static_cast<uint16_t>(block);
    record.ndNum = ndPara & 0xffff;
    record.dstNdStride = (ndPara >> 16) & 0xffff;
    record.srcNdStride = (ndPara >> 32) & 0xffff;
    record.enUnitFlag = (unitFlag > 1);  // Mode2/Mode3表示使能unit-flag机制
    record.channelSplit = (atomicMode == AtomicMode::F32 && channelSplit);
    record.enNZ2ND = (xt >> 43) & 0x1;
    ParseMovfpQuantBits((xt >> 34) & 0x1f, record);
    DumpRecord<MovFpRecord, RecordType::MOV_FP>(fixParams.memInfo, record);
}

template<MemType srcMemType, MemType dstMemType, DataType dataType>
AICORE_FUNC_HEAD void RecordFixL0CGMEvent(RecordFixParams &&fixParams, uint64_t xm, uint64_t xt)
{
    uint64_t block;
    if (!RecordPreCheck<FixL0CGMRecord>(fixParams.memInfo, block)) {
        return;
    }
    FixL0CGMRecord record{};
    record.dst = fixParams.dst;
    record.src = fixParams.src;
    record.pc = fixParams.pc;
    record.coreID = static_cast<uint16_t>(block);

    // 从xm、xt中获取参数
    record.nSize = GetUintFromConf<15, 4, uint64_t>(xm);
    record.mSize = GetUintFromConf<31, 16, uint64_t>(xm);
    record.dstStride = GetUintFromConf<63, 32, uint64_t>(xm);
    record.srcStride = GetUintFromConf<15, 0, uint64_t>(xt);
    uint8_t unitFlag = GetUintFromConf<33, 32, uint64_t>(xt);
    bool channelSplit = GetUintFromConf<42, 42, uint64_t>(xt);

    // 查看是否使能ND模式
    record.enNZ2ND = GetUintFromConf<43, 43, uint64_t>(xt);
    record.enNZ2DN = GetUintFromConf<62, 62, uint64_t>(xt);
    bool enNDorDN = record.enNZ2ND || record.enNZ2DN;

    // ndNum/srcNdStride/dstNdStride来自LOOP3_PARA，从桩函数set_loop3_para中获取
    uint64_t sprLoop3Para = 0;
    uint64_t loop3Para = GetLoop3Para(fixParams.memInfo, block);
    record.ndNum = GetUintFromConf<15, 0, uint64_t>(loop3Para);
    record.srcNdStride = GetUintFromConf<31, 16, uint64_t>(loop3Para);
    record.dstNdStride = GetUintFromConf<63, 32, uint64_t>(loop3Para);

    record.enUnitFlag = (unitFlag > 1);  // Mode2/Mode3表示使能unit-flag机制
    record.channelSplit = (dataType == DataType::DATA_F32 && channelSplit && !enNDorDN);
    record.srcMemType = srcMemType;
    record.dstMemType = dstMemType;
    record.dataType = dataType;
    uint64_t lowBits = GetUintFromConf<38, 34, uint64_t>(xt);
    uint64_t highBits = GetUintFromConf<29, 29, uint64_t>(xt);
    if (highBits == 1) {
        lowBits += 32;
    }
    ParseFixfpQuantBits(lowBits, enNDorDN, record);

    if (record.enNZ2DN) {
        // enNZ2DN使能时，srcNzC0Stride(Loop0_src_stride)从桩函数set_channel_para中获取
        uint64_t channelPara = GetChannelPara(fixParams.memInfo, block);
        record.srcNzC0Stride = GetUintFromConf<63, 48, uint64_t>(channelPara);; // in unit of C0 SIZE
        if (record.srcNzC0Stride != 1) {
            // When NZ2DN is enable and loop0 src stride is not 1, unit-flag cannot be enable.
            record.enUnitFlag = false;
        }
    }
    DumpRecord<FixL0CGMRecord, RecordType::FIX_L0C_TO_OUT>(fixParams.memInfo, record);
}
}
#endif // MSOPT_RECORD_MOV_FP_H
