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

#include "warp_timeline_visualize.h"

#include <algorithm>
#include <utility>
#include <vector>
#include "securec.h"
#include "common/hal_helper.h"
#include "common/visualize.h"
#include "filesystem.h"

using namespace Common;
using namespace Utility;

namespace Visualize {
namespace {
// 小核类型
constexpr uint32_t CORE_TYPE_VEC0 = 0U;
constexpr uint32_t CORE_TYPE_VEC1 = 1U;
constexpr uint32_t CORE_TYPE_CUBE0 = 2U;
constexpr int64_t DEFAULT_AICORE_FREQ = 1650; // MHz

std::string GetWarpTimelinePid(uint32_t coreId, uint32_t coreType)
{
    std::string subCoreName;
    switch (coreType) {
        case CORE_TYPE_VEC0:
            subCoreName = "veccore0";
            break;
        case CORE_TYPE_VEC1:
            subCoreName = "veccore1";
            break;
        case CORE_TYPE_CUBE0:
            subCoreName = "cubecore0";
            break;
        default:
            subCoreName = "unknowncore" + std::to_string(coreType);
            break;
    }
    return "core" + std::to_string(coreId) + "." + subCoreName + ".warp";
}
}

bool WarpTimelineVisualize::LoadRecords(const std::string &outputPath, std::vector<char> &binData) const
{
    std::string binPath = JoinPath({outputPath, "dump", WARP_TIMELINE});
    if (!IsReadable(Realpath(binPath))) {
        LogDebug("%s is not exist or readable.", binPath.c_str());
        return false;
    }
    return ReadBinFileByMultiStruct(binPath, GetFileSize(binPath), sizeof(BlockWarpRecords), binData);
}

void WarpTimelineVisualize::AppendBlockEvents(const char *blockData, uint32_t blockId, int64_t aicoreFreq,
                                              std::vector<nlohmann::json> &traceEvents) const
{
    WarpHeader header{};
    if (memcpy_s(&header, sizeof(header), blockData, sizeof(header)) != EOK) {
        return;
    }
    if (header.magicWords != DBI_RECORD_MAGIC_WORDS) {
        return;
    }

    uint32_t warpCount = std::min(header.warpCount, WARP_NUM_PER_BLOCK);
    auto records = reinterpret_cast<const WarpRecord*>(blockData + sizeof(WarpHeader));
    for (uint32_t warpId = 0; warpId < warpCount; ++warpId) {
        const WarpRecord &record = records[warpId];
        if (record.startTime == 0 || record.endTime <= record.startTime) {
            continue;
        }
        nlohmann::json event;
        event["name"] = "Warp_" + std::to_string(warpId);
        event["cname"] = std::string(Utility::VISUALIZE_COLOR_NAME::BLUE);
        event["ph"] = "X";
        event["pid"] = GetWarpTimelinePid(header.coreId, header.coreType);
        event["tid"] = "Warp " + std::to_string(warpId);
        event["ts"] = static_cast<double>(record.startTime) / static_cast<double>(aicoreFreq);
        event["dur"] = static_cast<double>(record.endTime - record.startTime) / static_cast<double>(aicoreFreq);
        event["args"]["block_id"] = blockId;
        event["args"]["core_id"] = header.coreId;
        event["args"]["core_type"] = header.coreType;
        event["args"]["warp_id"] = warpId;
        traceEvents.emplace_back(std::move(event));
    }
}

bool WarpTimelineVisualize::TimelineToJson(const std::string &outputPath)
{
    timelineJson_ = nlohmann::json::object();

    std::vector<char> binData;
    if (!LoadRecords(outputPath, binData)) {
        return false;
    }

    int64_t aicoreFreq = 0;
    if (!Common::HalHelper::Instance().GetAicoreFreq(aicoreFreq)) {
        LogWarn("Get aicore frequency failed. Use default value instead.");
        aicoreFreq = DEFAULT_AICORE_FREQ;
    }

    size_t blockSize = sizeof(BlockWarpRecords);
    std::vector<nlohmann::json> traceEvents;
    for (size_t offset = 0, blockId = 0; offset + blockSize <= binData.size(); offset += blockSize, ++blockId) {
        AppendBlockEvents(binData.data() + offset, static_cast<uint32_t>(blockId), aicoreFreq, traceEvents);
    }
    if (traceEvents.empty()) {
        LogDebug("No valid warp timeline records found.");
        return false;
    }

    timelineJson_["profilingType"] = "op";
    timelineJson_["displayTimeUnit"] = "ns";
    timelineJson_["schemaVersion"] = 1;
    timelineJson_["traceEvents"] = std::move(traceEvents);
    return true;
}
}
