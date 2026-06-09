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
    #define AICORE_FUNC_HEAD __simt_callee__ __aicore__ inline
#else
#define MSOPPROF_REPORT_AICORE(func, ...) \
    extern __attribute__((noinline)) __attribute__((weak)) __aicore__ void \
    MACRO_CONCAT(INSTR_STUB_PREFIX, func)(EXTRA_PARAMS_DEC, ## __VA_ARGS__)
    #define AICORE_FUNC_HEAD __aicore__ inline
#endif
#define MSOPPROF_REPORT MSOPPROF_REPORT_AICORE


// transform config[leftBit, rightBit] into an unsigned integer
template<uint8_t leftBit, uint8_t rightBit, typename confT>
AICORE_FUNC_HEAD uint64_t GetUintFromConf(confT config)
{
    constexpr uint8_t bitsPerByte= 8U;
    constexpr uint8_t maxBit = sizeof(config) * bitsPerByte - 1;
    constexpr uint64_t mask = ~0x0ULL;
    static_assert(leftBit >= rightBit);
    static_assert(leftBit <= maxBit);
    return (config >> rightBit) & (mask >> (63 - leftBit + rightBit));
}

AICORE_FUNC_HEAD uint64_t GetSysVaBase()
{
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1 // AICORE

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__) || defined(__DAV_C220_CUBE__) || \
    (defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510))
    using namespace __cce_scalar;
    return get_sys_va_base();
#else
    return 0;
#endif

#else // NOT AICORE
    return 0;
#endif
}

AICORE_FUNC_HEAD uint64_t GetBlockIdx()
{
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1 // AICORE
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__) || defined(__DAV_C220_CUBE__)
    return get_block_idx() * get_subblockdim() + get_subblockid();
#elif defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510)
#if defined(__DAV_VEC__) && defined(SIMT_MODE) // c310-simt
    return __cce_simt::get_block_idx();
#else
    int64_t coreId = get_coreid();
    if ((coreId >= C310_A5_DEVICE_VEC_PHYS_SMALL_BOUND_CORE_START_IDS &&
            coreId <= C310_A5_DEVICE_VEC_PHYS_SMALL_BOUND_CORE_END_IDS) ||
        coreId >= C310_A5_DEVICE_VEC_PHYS_GREAT_BOUND_CORE_START_IDS) { // c310-vec
        return get_block_idx() * get_subblockdim() + get_subblockid();
    } else { // c310-cube
        return get_block_idx();
    }
#endif
#else // NOT C220 C310
    return get_block_idx();
#endif // __DAV
#else // NOT AICORE
    return 0;
#endif
}
AICORE_FUNC_HEAD __attribute__((always_inline)) uint16_t GetThreadIdX()
{
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1
    #if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(__DAV_VEC__)
    return __cce_simt_get_TID_X();
#endif // AICORE
#endif // SIMT_MODE
    return 0;
}

AICORE_FUNC_HEAD __attribute__((always_inline)) uint16_t GetThreadIdY()
{
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1
    #if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(__DAV_VEC__)
    return __cce_simt_get_TID_Y();
#endif // AICORE
#endif // SIMT_MODE
    return 0;
}

AICORE_FUNC_HEAD __attribute__((always_inline)) uint16_t GetThreadIdZ()
{
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1
    #if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(__DAV_VEC__)
    return __cce_simt_get_TID_Z();
#endif // AICORE
#endif // SIMT_MODE
    return 0;
}

/// x/y/z一维展开，从0开始计数
AICORE_FUNC_HEAD __attribute__((always_inline)) uint16_t GetThreadId()
{
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1
    #if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(__DAV_VEC__)
    int32_t blockDimX = __cce_simt_get_BLOCK_DIM_X();
    int32_t blockDimY = __cce_simt_get_BLOCK_DIM_Y();
    int32_t threadIdX = GetThreadIdX();
    int32_t threadIdY = GetThreadIdY();
    int32_t threadIdZ = GetThreadIdZ();
    return threadIdX + blockDimX * threadIdY + blockDimX * blockDimY * threadIdZ;
#endif // AICORE
#endif // SIMT_MODE
    return 0;
}

// get a single bit
template<uint8_t leftBit, typename confT>
AICORE_FUNC_HEAD uint64_t GetUintFromConf(confT config)
{
    return GetUintFromConf<leftBit, leftBit>(config);
}

// config[leftBit, rightBit] = val
template<uint8_t leftBit, uint8_t rightBit, typename confT, typename uintT>
AICORE_FUNC_HEAD void SetConfByUint(confT &config, uintT val)
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

AICORE_FUNC_HEAD uint64_t GetDataBits(OperandType type)
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

AICORE_FUNC_HEAD uint16_t CeilToFractal(uint16_t realSize, uint16_t fractalSize)
{
    if (fractalSize == 0) {
        return fractalSize;
    }
    return (realSize + fractalSize - 1) / fractalSize;
}

AICORE_FUNC_HEAD bool CheckMemInfo(__gm__ uint8_t *memInfo)
{
    return (memInfo != nullptr);
}

AICORE_FUNC_HEAD bool TryGetBlockIdx(uint64_t &blockIdx)
{
    uint64_t block = GetBlockIdx();
    int64_t coreId{};
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__) || defined(__DAV_C220_CUBE__) || \
    (defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510))
#ifdef SIMT_MODE
    coreId = __cce_simt_get_COREID();
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

AICORE_FUNC_HEAD bool TryGetThreadId(uint64_t &threadId)
{
    uint64_t thread = GetThreadId();
    if (thread >= MAX_THREAD_NUM) {
        return false;
    }
    threadId = thread;
    return true;
}

AICORE_FUNC_HEAD uint64_t GetWarpTimelineClock() {
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1
#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(__DAV_VEC__)
    return static_cast<uint64_t>(__cce_simt_get_CLOCK64());
#endif
#endif
    return 0;
}

AICORE_FUNC_HEAD uint32_t GetLaneId() {
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1
#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(__DAV_VEC__)
    return static_cast<uint32_t>(__cce_simt_get_laneID());
#endif
#endif
    return 0;
}

AICORE_FUNC_HEAD bool GetWarpBasicInfo(
    __gm__ uint8_t *memInfo, uint64_t &blockIdx, uint32_t &warpId, uint32_t &coreId, uint32_t &coreType)
{
    if (!CheckMemInfo(memInfo)) {
        return false;
    }
    uint64_t threadId = 0;
    constexpr uint32_t warpThreadNum = 32U;
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1
#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(__DAV_VEC__)
    if (!TryGetBlockIdx(blockIdx) || !TryGetThreadId(threadId)) {
        return false;
    }
#else
    return false;
#endif
#else
    return false;
#endif
    warpId = static_cast<uint32_t>(threadId / warpThreadNum);
    coreId = static_cast<uint32_t>(blockIdx / MIX_SUB_BLOCKDIM);
    coreType = static_cast<uint32_t>(blockIdx % MIX_SUB_BLOCKDIM);
    return blockIdx < MAX_BLOCK && warpId < WARP_NUM_PER_BLOCK;
}

AICORE_FUNC_HEAD void Flush(__gm__ uint8_t *gm)
{
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1
    using namespace __cce_scalar;
#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510)
    dcci((__gm__ uint64_t*)gm, ENTIRE_DATA_CACHE, CACHELINE_ALL);
#else
    dcci(gm, ENTIRE_DATA_CACHE);
#endif
#endif
}
#endif // MSOPT_UTILS_H
