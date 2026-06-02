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

#include "plugin/defs.h"
#include "plugin/utils.h"
using namespace Common;

MSOPPROF_REPORT(before_kernel_end)
{
    asm volatile("bar.all");
    asm volatile(".rept 32\n\tNOP \n\t.endr");     // 确保MTE变为IDLE
    asm("DFX_REGION.S %0" :: "l" (0xd88));         // 8个DFX_REGION指令确保刷满32BYTE
    asm volatile("nop");
    asm("DFX_REGION.S %0" :: "l" (0xd99));
    asm volatile("nop");
    asm("DFX_REGION.S %0" :: "l" (0xdaa));
    asm volatile("nop");
    asm("DFX_REGION.S %0" :: "l" (0xdbb));
    asm volatile("nop");
    asm("DFX_REGION.S %0" :: "l" (0xdcc));
    asm volatile("nop");
    asm("DFX_REGION.S %0" :: "l" (0xddd));
    asm volatile("nop");
    asm("DFX_REGION.S %0" :: "l" (0xdee));
    asm volatile("nop");
    asm("DFX_REGION.S %0" :: "l" (0xdff));
    asm volatile(".rept 3500\n\tNOP \n\t.endr");     // 增加时延，以确保buffer刷入总线
    asm("DFX_REGION.S %0" :: "l" (0xdff));
    asm volatile("nop");
    return;
}
