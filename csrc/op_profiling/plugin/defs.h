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

#ifndef PLUGIN_DEFS_H
#define PLUGIN_DEFS_H
#include "ccec/ccec_defines.h"

// mix算子运行时，blockDim = 1对应1个cube + 2个vec
/// 因某些硬件的特殊架构，一个 AICore 中包含若干个 AIVEC 核和 AICUBE 核（c220架构 为 1 个 AICUBE 核和 2 个 AIVEC 核）
/// 当算子以 MIX 模式运行时，blockDim 代表参与运算的 AICore 逻辑核数，因此实际参与运算的 subBlockDim 分别为
/// blockDim * VEC_SUB_BLOCKDIM 和 blockDim * CUBE_SUB_BLOCKDIM。
// 最终申请的GM内存为：cache_size * blockDim * (VEC_SUB_BLOCKDIM + CUBE_SUB_BLOCKDIM)
// 排布顺序为：vec/vec/cube/vec/vec/cube ......
constexpr uint8_t VEC_SUB_BLOCKDIM = 2;
constexpr uint8_t CUBE_SUB_BLOCKDIM = 1;
// A2、A3、A5在Mix模式下1个aicore均包含3个小核
constexpr uint8_t MIX_SUB_BLOCKDIM = VEC_SUB_BLOCKDIM + CUBE_SUB_BLOCKDIM;

/// c220芯片架构，
/// A2芯片： vec核对应的物理核编号范围：[25, 74]，cube核编号范围：[0, 24]
/// A3芯片： 偶数卡：和A2芯片一致;奇数卡: vec核对应的物理核编号范围:[32793,32842],cube核编号范围:[32768,32792]
constexpr int64_t C220_A2_OR_A3_EVEN_DEVICE_VEC_PHYS_CORE_START_IDS = 25;
constexpr int64_t C220_A2_OR_A3_EVEN_DEVICE_VEC_PHYS_CORE_END_IDS = 74;
constexpr int64_t C220_A3_ODD_DEVICE_VEC_PHYS_CORE_START_IDS = 32793;

/// c310架构A5芯片：vec核对应的物理核编号范围:[18, 51]和>= 72
constexpr int64_t C310_A5_DEVICE_VEC_PHYS_SMALL_BOUND_CORE_START_IDS = 18;
constexpr int64_t C310_A5_DEVICE_VEC_PHYS_SMALL_BOUND_CORE_END_IDS = 51;
constexpr int64_t C310_A5_DEVICE_VEC_PHYS_GREAT_BOUND_CORE_START_IDS = 72;

constexpr uint16_t FRACTAL_SIZE16 = 16;


__aicore__ inline uint64_t GetBlockIdx()
{
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1 // AICORE

#if (defined(__DAV_C220__) || defined(__DAV_C220_VEC__) || defined(__DAV_C220_CUBE__))
    return get_block_idx() * get_subblockdim() + get_subblockid();
#elif defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510)
    #if defined(__DAV_VEC__) && defined(SIMT_MODE) // c310-simt
        return bisheng::cce::simt::get_block_idx();
    #else
        int64_t coreId = get_coreid();
        if ((coreId >= C310_A5_DEVICE_VEC_PHYS_SMALL_BOUND_CORE_START_IDS &&
            coreId <= C310_A5_DEVICE_VEC_PHYS_SMALL_BOUND_CORE_END_IDS) ||
            coreId >= C310_A5_DEVICE_VEC_PHYS_GREAT_BOUND_CORE_START_IDS) {    // c310-vec
            return get_block_idx() * get_subblockdim() + get_subblockid();
        } else {                                                               // c310-cube
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
__aicore__ __inline__ __attribute__((always_inline)) uint16_t GetThreadIdX()
{
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1
    #if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(__DAV_VEC__)
    return __cce_simt_get_TID_X();
#endif // AICORE
#endif // SIMT_MODE
    return 0;
}

__aicore__ __inline__ __attribute__((always_inline)) uint16_t GetThreadIdY()
{
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1
    #if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(__DAV_VEC__)
    return __cce_simt_get_TID_Y();
#endif // AICORE
#endif // SIMT_MODE
    return 0;
}

__aicore__ __inline__ __attribute__((always_inline)) uint16_t GetThreadIdZ()
{
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1
    #if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(__DAV_VEC__)
    return __cce_simt_get_TID_Z();
#endif // AICORE
#endif // SIMT_MODE
    return 0;
}

/// x/y/z一维展开，从0开始计数
__aicore__ __inline__ __attribute__((always_inline)) uint16_t GetThreadId()
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
#endif  // PLUGIN_DEFS_H
