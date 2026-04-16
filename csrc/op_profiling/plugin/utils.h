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


#ifndef MSOPT_UTILS_H
#define MSOPT_UTILS_H
#include "defs.h"
#include "common/dbi_defs.h"
#define EXTRA_PARAMS memInfo, pc, bid
#define EXTRA_PARAMS_DEC __gm__ uint8_t *memInfo, uint64_t pc, uint32_t bid

#define CONCAT_IMPL(x, y) x##y
#define MACRO_CONCAT(x, y) CONCAT_IMPL(x, y)

#define INSTR_STUB_PREFIX __msopprof_report_
// AICORE 平台使用的 MSOPPROF_REPORT 宏，用于声明桩函数接口
#ifdef SIMT_MODE // simt
#define MSOPPROF_REPORT_AICORE(func, ...) \
    extern __attribute__((noinline)) __attribute__((weak)) __simt_callee__ __aicore__ void \
    MACRO_CONCAT(INSTR_STUB_PREFIX, func)(EXTRA_PARAMS_DEC, ## __VA_ARGS__)
#else
#define MSOPPROF_REPORT_AICORE(func, ...) \
    extern __attribute__((noinline)) __attribute__((weak)) __aicore__ void \
    MACRO_CONCAT(INSTR_STUB_PREFIX, func)(EXTRA_PARAMS_DEC, ## __VA_ARGS__)
#endif
#define MSOPPROF_REPORT MSOPPROF_REPORT_AICORE


// transform config[leftBit, rightBit] into an unsigned integer
template<uint8_t leftBit, uint8_t rightBit, typename confT>
__aicore__ inline uint64_t GetUintFromConf(confT config)
{
    constexpr uint8_t bitsPerByte= 8U;
    constexpr uint8_t maxBit = sizeof(config) * bitsPerByte - 1;
    constexpr uint64_t mask = ~0x0ULL;
    static_assert(leftBit >= rightBit);
    static_assert(leftBit <= maxBit);
    return (config >> rightBit) & (mask >> (63 - leftBit + rightBit));
}

// get a single bit
template<uint8_t leftBit, typename confT>
__aicore__ inline uint64_t GetUintFromConf(confT config)
{
    return GetUintFromConf<leftBit, leftBit>(config);
}

// config[leftBit, rightBit] = val
template<uint8_t leftBit, uint8_t rightBit, typename confT, typename uintT>
__aicore__ inline void SetConfByUint(confT &config, uintT val)
{
    constexpr uint8_t bitsPerByte = 8U;
    constexpr uint8_t maxBit = sizeof(config) * bitsPerByte - 1;
    static_assert(leftBit >= rightBit);
    static_assert(leftBit <= maxBit);

    uint64_t mask = ~0x0ULL;
    mask >>= 63 - leftBit + rightBit;
    uint64_t otherMask = ~(mask << rightBit);
    config &= otherMask;
    mask = val & mask;
    mask <<= rightBit;
    config |= mask;
}

__aicore__ inline uint64_t GetDataBits(OperandType type)
{
    switch (type) {
        case OperandType::DATA_B4:
        case OperandType::DATA_E1M2:
        case OperandType::DATA_E2M1:
            return 4UL;
        case OperandType::DATA_B8:
        case OperandType::DATA_S8:
        case OperandType::DATA_U8:
        case OperandType::DATA_E4M3:
        case OperandType::DATA_E5M2:
            return 8UL;
        case OperandType::DATA_B16:
        case OperandType::DATA_S16:
        case OperandType::DATA_U16:
        case OperandType::DATA_F16:
        case OperandType::DATA_F16X2:
        case OperandType::DATA_HALF:
        case OperandType::DATA_BF16:
        case OperandType::DATA_HIF8X2:
        case OperandType::DATA_F8E4M3X2:
        case OperandType::DATA_F8E5M2X2:
            return 16UL;
        case OperandType::DATA_B32:
        case OperandType::DATA_S32:
        case OperandType::DATA_U32:
        case OperandType::DATA_F32:
        case OperandType::DATA_SX32:
        case OperandType::DATA_ZX32:
        case OperandType::DATA_BF16X2:
        case OperandType::DATA_V2BF16:
        case OperandType::DATA_V2F16:
            return 32UL;
        case OperandType::DATA_B64:
        case OperandType::DATA_S64:
        case OperandType::DATA_U64:
        case OperandType::DATA_F32X2:
            return 64UL;
        case OperandType::DATA_B128:
        default:
            return 128UL;
    }
}

__aicore__ inline uint16_t CeilToFractal(uint16_t realSize, uint16_t fractalSize)
{
    if (fractalSize == 0) {
        return fractalSize;
    }
    return (realSize + fractalSize - 1) / fractalSize;
}

__aicore__ inline bool CheckMemInfo(__gm__ uint8_t *memInfo)
{
    return (memInfo != nullptr);
}

__aicore__ inline bool TryGetBlockIdx(uint64_t &blockIdx)
{
    uint64_t block = GetBlockIdx();
    int64_t coreId{};
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__) || defined(__DAV_C220_CUBE__) || \
    (defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510))
#ifdef SIMT_MODE
    coreId = bisheng::cce::simt::get_coreid();
#else
    coreId = get_coreid();
#endif // SIMT_MODE
#endif // DAV
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__) || defined(__DAV_C220_CUBE__)
    if ((coreId >= C220_A2_OR_A3_EVEN_DEVICE_VEC_PHYS_CORE_START_IDS &&
         coreId <= C220_A2_OR_A3_EVEN_DEVICE_VEC_PHYS_CORE_END_IDS) ||
        coreId >= C220_A3_ODD_DEVICE_VEC_PHYS_CORE_START_IDS) {
        blockIdx = block + block / (MIX_SUB_BLOCKDIM - 1);
    } else {
        blockIdx = block + (block + 1) * (MIX_SUB_BLOCKDIM - 1);
    }
#elif defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510)
    if ((coreId >= C310_A5_DEVICE_VEC_PHYS_SMALL_BOUND_CORE_START_IDS &&
        coreId <= C310_A5_DEVICE_VEC_PHYS_SMALL_BOUND_CORE_END_IDS) ||
        coreId >= C310_A5_DEVICE_VEC_PHYS_GREAT_BOUND_CORE_START_IDS) {
        blockIdx = block + block / (MIX_SUB_BLOCKDIM - 1);
    } else {
        blockIdx = block + (block + 1) * (MIX_SUB_BLOCKDIM - 1);
    }
#endif
    if (blockIdx > MAX_BLOCK) {
        return false;
    }
    return true;
}

__aicore__ inline bool TryGetThreadId(uint64_t &threadId)
{
    uint64_t thread = GetThreadId();
    if (thread >= MAX_THREAD_NUM) {
        return false;
    }
    threadId = thread;
    return true;
}
__aicore__ inline void Flush(__gm__ uint8_t *gm)
{
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1
#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510)
        dcci((__gm__ uint64_t*)gm, ENTIRE_DATA_CACHE, CACHELINE_ALL);
#else
        dcci(gm, ENTIRE_DATA_CACHE);
#endif
#endif
}
#endif // MSOPT_UTILS_H
