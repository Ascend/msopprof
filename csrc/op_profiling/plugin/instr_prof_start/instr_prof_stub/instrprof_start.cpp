/* -------------------------------------------------------------------------
 *  This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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

#include "plugin/defs.h"
#include "plugin/utils.h"
using namespace Common;

inline __attribute__((weak)) __simt_vf__ LAUNCH_BOUND(2048)[aicore] void SimtCall(__gm__ uint8_t *memInfo);

MSOPPROF_REPORT(after_kernel_start, uint64_t pcOffset)
{
    cce::async_invoke<SimtCall>(cce::dim3{2048, 1, 1}, memInfo);
    __gm__ uint64_t *pcOffsetAddr = reinterpret_cast<__gm__ uint64_t*>(memInfo);
    *pcOffsetAddr = pcOffset;
    Flush(memInfo);
    return;
}

inline __attribute__((weak)) __simt_vf__ LAUNCH_BOUND(2048)[aicore] void SimtCall(__gm__ uint8_t *memInfo)
{
    for (uint32_t i = 0; i < 10; ++i) asm volatile("NOP          wait:0b0000000 stall:15");
    for (uint32_t i = 0; i < 10; ++i) asm volatile("NOP          wait:0b0000000 stall:15");
    for (uint32_t i = 0; i < 10; ++i) asm volatile("NOP          wait:0b0000000 stall:15");
    for (uint32_t i = 0; i < 10; ++i) asm volatile("NOP          wait:0b0000000 stall:15");
    for (uint32_t i = 0; i < 10; ++i) asm volatile("NOP          wait:0b0000000 stall:15");
    for (uint32_t i = 0; i < 10; ++i) asm volatile("NOP          wait:0b0000000 stall:15");
    for (uint32_t i = 0; i < 10; ++i) asm volatile("NOP          wait:0b0000000 stall:15");
    for (uint32_t i = 0; i < 10; ++i) asm volatile("NOP          wait:0b0000000 stall:15");
    for (uint32_t i = 0; i < 10; ++i) asm volatile("NOP          wait:0b0000000 stall:15");
    for (uint32_t i = 0; i < 10; ++i) asm volatile("NOP          wait:0b0000000 stall:15");
    for (uint32_t i = 0; i < 10; ++i) asm volatile("NOP          wait:0b0000000 stall:15");
    for (uint32_t i = 0; i < 10; ++i) asm volatile("NOP          wait:0b0000000 stall:15");
    for (uint32_t i = 0; i < 10; ++i) asm volatile("NOP          wait:0b0000000 stall:15");
    for (uint32_t i = 0; i < 10; ++i) asm volatile("NOP          wait:0b0000000 stall:15");
    for (uint32_t i = 0; i < 10; ++i) asm volatile("NOP          wait:0b0000000 stall:15");
    for (uint32_t i = 0; i < 10; ++i) asm volatile("NOP          wait:0b0000000 stall:15");
    for (uint32_t i = 0; i < 10; ++i) asm volatile("NOP          wait:0b0000000 stall:15");
    for (uint32_t i = 0; i < 10; ++i) asm volatile("NOP          wait:0b0000000 stall:15");
    return;
}