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

#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510)
#define SIMT_MODE
#include "plugin/warp_timeline/recorder.h"

using namespace Common;

extern __attribute__((noinline)) __attribute__((weak)) __simt_callee__ __aicore__ void __msopprof_report_simt_start(
    __gm__ uint8_t *memInfo, uint64_t pc, uint32_t bid) {
    DumpWarpTimestamp(memInfo, true);
}

extern __attribute__((noinline)) __attribute__((weak)) __simt_callee__ __aicore__ void __msopprof_report_simt_end(
    __gm__ uint8_t *memInfo, uint64_t pc, uint32_t bid) {
    DumpWarpTimestamp(memInfo, false);
}
#endif
