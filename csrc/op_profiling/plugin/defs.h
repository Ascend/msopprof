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
__aicore__ inline uint64_t GetBlockIdx()
{
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1 // AICORE
    #if (defined(__DAV_C220__) || defined(__DAV_C220_VEC__) || defined(__DAV_C220_CUBE__))
    return get_block_idx() * get_subblockdim() + get_subblockid();
#elif defined(__NPU_ARCH__) && __NPU_ARCH__ == 3101
    #if defined(__DAV_VEC__) && defined(SIMT_MODE) // c310-simt
        return bisheng::cce::simt::get_block_idx();
    #else
        return get_block_idx() * get_subblockdim() + get_subblockid();
    #endif // __DAV_VEC__
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
    #if defined(__NPU_ARCH__) && __NPU_ARCH__ == 3101 && defined(__DAV_VEC__)
    return __cce_simt_get_TID_X();
#endif // AICORE
#endif // SIMT_MODE
    return 0;
}

__aicore__ __inline__ __attribute__((always_inline)) uint16_t GetThreadIdY()
{
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1
    #if defined(__NPU_ARCH__) && __NPU_ARCH__ == 3101 && defined(__DAV_VEC__)
    return __cce_simt_get_TID_Y();
#endif // AICORE
#endif // SIMT_MODE
    return 0;
}

__aicore__ __inline__ __attribute__((always_inline)) uint16_t GetThreadIdZ()
{
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1
    #if defined(__NPU_ARCH__) && __NPU_ARCH__ == 3101 && defined(__DAV_VEC__)
    return __cce_simt_get_TID_Z();
#endif // AICORE
#endif // SIMT_MODE
    return 0;
}

/// x/y/z一维展开，从0开始计数
__aicore__ __inline__ __attribute__((always_inline)) uint16_t GetThreadId()
{
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1
    #if defined(__NPU_ARCH__) && __NPU_ARCH__ == 3101 && defined(__DAV_VEC__)
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
