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

#ifndef MSOPT_OPERAND_RECODER_H
#define MSOPT_OPERAND_RECODER_H

#include "plugin/defs.h"
#include "plugin/utils.h"
#include "common/dbi_defs.h"
#include "kernel_injection/include/MSBit.h"
namespace Common {
struct BasicInfo {
    __gm__ uint8_t *memInfo;
    uint64_t blockIdx;
    uint64_t threadId;
};
// 要写入gm的结构体保证4/8字节对齐(这里用了8字节对齐)，写入单字节数据大概率导致所有数据丢失
__aicore__ inline void AccumulateData(__gm__ uint8_t *memInfo, const OperandRecord &record)
{
    auto dst = reinterpret_cast<__gm__ uint64_t*>(memInfo);
    uint64_t instructions = *dst + record.instructions;
    uint64_t operands = *(dst + 1) + record.operands;
    *dst = instructions;
    *(dst + 1) = operands;
    *(dst + 2) = record.funcType;
}

__aicore__ inline void DumpRecord(const BasicInfo &basicInfo, OperandType type, const OperandRecord &record, bool isSimt)
{
    uint64_t blockIdx = basicInfo.blockIdx;
    uint64_t threadId = basicInfo.threadId;
    uint64_t sizePerAllType = static_cast<uint32_t>(OperandType::END) * sizeof(OperandRecord) + SIMT_THREAD_GAP;
    uint64_t offset;
    if (isSimt) {
        offset = sizeof(OperandHeader) + blockIdx * ((MAX_THREAD_NUM + 1) * sizePerAllType + BLOCK_GAP) +
                 (threadId + 1) * sizePerAllType + static_cast<uint32_t>(type) * sizeof(OperandRecord);
    } else {
        offset = sizeof(OperandHeader) + blockIdx * ((MAX_THREAD_NUM + 1) * sizePerAllType + BLOCK_GAP) +
                 static_cast<uint32_t>(type) * sizeof(OperandRecord);
    }
    __gm__ uint8_t *objMem = reinterpret_cast<__gm__ uint8_t*>(basicInfo.memInfo + offset);
    AccumulateData(objMem, record);
    Flush(basicInfo.memInfo);
}

__aicore__ inline bool CheckAndGetBasicInfo(__gm__ uint8_t *memInfo, uint64_t &blockIdx,
                                            uint64_t &threadId, bool isSimt)
{
    if (!CheckMemInfo(memInfo)) {
        return false;
    }
    if (!TryGetBlockIdx(blockIdx)) {
        return false;
    }
    if (isSimt && !TryGetThreadId(threadId)) {
        return false;
    }
    return true;
}

template<InstrType instrType, OperandType operandType>
__aicore__ inline void RecordOperandOperation(__gm__ uint8_t *memInfo, uint8_t operandNums, bool isSimt)
{
    OperandRecord record {1, operandNums, static_cast<uint64_t>(instrType)};
    uint64_t blockIdx = 0;
    uint64_t threadId = 0;
    if (!CheckAndGetBasicInfo(memInfo, blockIdx, threadId, isSimt)) {
        return;
    }
    DumpRecord({memInfo, blockIdx, threadId}, operandType, record, isSimt);
}

template<InstrType instrType, OperandType operandType>
__aicore__ inline void RecordMmadA5(__gm__ uint8_t *memInfo, uint64_t config, bool isMx = 0)
{
    uint64_t blockIdx = 0;
    uint64_t threadId = 0;
    if (!CheckAndGetBasicInfo(memInfo, blockIdx, threadId, 0)) {
        return;
    }
    uint32_t m = GetUintFromConf<11, 0>(config);
    uint32_t k = GetUintFromConf<23, 12>(config);
    uint32_t n = GetUintFromConf<35, 24>(config);

    bool enGEMV = GetUintFromConf<61, 61>(config) == 0;
    uint16_t k0 = 256 / GetDataBits(operandType); // C0 固定32 * 8 = 256 bit
    uint32_t mCnt = CeilToFractal(m, FRACTAL_SIZE16);
    uint32_t kCnt = CeilToFractal(k, k0); // k is 32B align
    uint32_t nCnt = CeilToFractal(n, FRACTAL_SIZE16);

    if (enGEMV && m == 1) {
        // GEMV 模式下 src0 会从 1*k 的矩阵变为 16*(k/16) 的矩阵；dst矩阵会从 1*n 变为 16*n
        mCnt = 1;
        kCnt = 16;
        k0 = CeilToFractal(k, FRACTAL_SIZE16);
    }

    // A=[M,K] B=[K,N] C=[M,N]
    // [A * B + C] FLOP计算公式 = 2 * M * N * K - M * N + M * N
    uint64_t operands = 2 * static_cast<uint64_t>(mCnt) * FRACTAL_SIZE16 * nCnt * FRACTAL_SIZE16 * kCnt * k0;

    if (isMx) {
        operands += kCnt * k0 * FRACTAL_SIZE16 * (mCnt + nCnt);
    }

    OperandRecord record {1, operands, static_cast<uint64_t>(instrType)};
    DumpRecord({memInfo, blockIdx, threadId}, operandType, record, 0);
}
}
#endif // MSOPT_OPERAND_RECODER_H
