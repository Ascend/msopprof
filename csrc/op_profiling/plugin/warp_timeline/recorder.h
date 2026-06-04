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

#ifndef MSOPT_WARP_TIMELINE_RECORDER_H
#define MSOPT_WARP_TIMELINE_RECORDER_H

#include "plugin/utils.h"

#ifndef AICORE_FUNC_HEAD
#ifdef SIMT_MODE
#define AICORE_FUNC_HEAD __simt_callee__ __aicore__ inline
#else
#define AICORE_FUNC_HEAD __aicore__ inline
#endif
#endif

namespace Common {
AICORE_FUNC_HEAD void DumpWarpTimestamp(__gm__ uint8_t *memInfo, bool isStart)
{
    // Only one lane writes a warp record. This avoids 32 lanes racing on the same start/end fields.
    if (GetLaneId() != 0) {
        return;
    }
    uint64_t blockIdx = 0;
    uint32_t warpId = 0;
    uint32_t coreId = 0;
    uint32_t coreType = 0;
    if (!GetWarpBasicInfo(memInfo, blockIdx, warpId, coreId, coreType)) {
        return;
    }

    constexpr uint64_t blockSize = sizeof(WarpHeader) + WARP_NUM_PER_BLOCK * sizeof(WarpRecord) + BLOCK_GAP;
    uint64_t offset = blockIdx * blockSize;
    __gm__ WarpHeader *header = reinterpret_cast<__gm__ WarpHeader*>(memInfo + offset);
    header->magicWords = DBI_RECORD_MAGIC_WORDS;
    header->warpCount = WARP_NUM_PER_BLOCK;
    header->coreId = coreId;
    header->coreType = coreType;

    __gm__ WarpRecord *record = reinterpret_cast<__gm__ WarpRecord*>(
        memInfo + offset + sizeof(WarpHeader) + static_cast<uint64_t>(warpId) * sizeof(WarpRecord));
    uint64_t timestamp = GetWarpTimelineClock();
    if (isStart) {
        record->startTime = timestamp;
    } else {
        record->endTime = timestamp;
    }
    Flush(memInfo);
}
}

#endif // MSOPT_WARP_TIMELINE_RECORDER_H
