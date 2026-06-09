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
#include <set>
#include <unordered_map>
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
constexpr uint32_t VECTOR_CORE_TYPE_COUNT = CORE_TYPE_CUBE0;
constexpr int64_t DEFAULT_AICORE_FREQ = 1650; // MHz

bool IsVectorCore(uint32_t coreType) { return coreType < VECTOR_CORE_TYPE_COUNT; }

void NormalizeTraceEventStart(std::vector<nlohmann::json> &traceEvents, double alignStartTs) {
    if (traceEvents.empty()) {
        return;
    }
    std::unordered_map<std::string, double> minTsByPid;
    for (const auto &event : traceEvents) {
        std::string pid = event["pid"].get<std::string>();
        double ts = event["ts"].get<double>();
        auto iter = minTsByPid.find(pid);
        if (iter == minTsByPid.end()) {
            minTsByPid.emplace(pid, ts);
        } else {
            iter->second = std::min(iter->second, ts);
        }
    }
    for (auto &event : traceEvents) {
        std::string pid = event["pid"].get<std::string>();
        event["ts"] = event["ts"].get<double>() - minTsByPid[pid] + alignStartTs;
    }
}

void AppendWarpSortMetadata(std::vector<nlohmann::json> &traceEvents) {
    std::set<std::pair<std::string, uint32_t>> sortedWarpIds;
    for (const auto &event : traceEvents) {
        if (!event.contains("pid") || !event.contains("args") || !event["args"].contains("warp_id")) {
            continue;
        }
        sortedWarpIds.emplace(event["pid"].get<std::string>(), event["args"]["warp_id"].get<uint32_t>());
    }

    for (const auto &warpKey : sortedWarpIds) {
        const std::string &pid = warpKey.first;
        uint32_t warpId = warpKey.second;
        std::string tid = "Warp " + std::to_string(warpId);
        nlohmann::json sortItem;
        sortItem["ph"] = "M";
        sortItem["name"] = "thread_sort_index";
        sortItem["pid"] = pid;
        sortItem["tid"] = tid;
        sortItem["args"]["sort_index"] = warpId;
        traceEvents.emplace_back(std::move(sortItem));
    }
}

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
    return "warp.core" + std::to_string(coreId) + "." + subCoreName;
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
    if (!IsVectorCore(header.coreType)) {
        LogDebug("Unexpected warp timeline record on non-vector core. blockId:%u coreId:%u coreType:%u", blockId,
            header.coreId, header.coreType);
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

bool WarpTimelineVisualize::TimelineToJson(const std::string &outputPath, double alignStartTs) {
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
    NormalizeTraceEventStart(traceEvents, alignStartTs);
    AppendWarpSortMetadata(traceEvents);

    timelineJson_["profilingType"] = "op";
    timelineJson_["displayTimeUnit"] = "ns";
    timelineJson_["schemaVersion"] = 1;
    timelineJson_["traceEvents"] = std::move(traceEvents);
    return true;
}
}
