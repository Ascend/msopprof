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

#include "data_visualize.h"

#include "json.hpp"
#include "log.h"
#include "common/visualize.h"
#include "roofline.h"
#include "warp_timeline_visualize.h"

using namespace Common;
using namespace Profiling;

namespace Visualize {
namespace {
bool HasTraceEvents(const nlohmann::json &traceJson) {
    return traceJson.contains("traceEvents") && traceJson["traceEvents"].is_array() &&
        !traceJson["traceEvents"].empty();
}
}

void DataVisualize::GenerateVisualizeData(Parse::DataCenter &dataCenter, const std::string &outputPath,
                                          const Common::ProfMetricsAbilityConfig &metrics) {
    GenerateAllVisualizeData(dataCenter, outputPath, metrics);
    CleanupAndLog(dataCenter, outputPath);
}

void DataVisualize::MergeTraceJson(nlohmann::json &dst, nlohmann::json src) {
    if (!HasTraceEvents(src)) {
        return;
    }
    if (!HasTraceEvents(dst)) {
        dst = std::move(src);
        return;
    }
    for (auto &event : src["traceEvents"]) {
        dst["traceEvents"].emplace_back(std::move(event));
    }
}

void DataVisualize::GenerateAllVisualizeData(Profiling::Parse::DataCenter &dataCenter, const std::string &outputPath,
    const Common::ProfMetricsAbilityConfig &metrics) {
    using VT = Utility::VisualizeType;
    opBasicInfo_->OpBasicInfoToJson();
    Utility::Visualize::WriteBin<VT::OP_BASIC_INFO>(outputPath, opBasicInfo_->opBasicFileJson_);

    std::shared_ptr<Parse::ComputeLoadInfo> computeLoadInfoPtr = dataCenter.GetDbPtr<Parse::ComputeLoadInfo>();
    if (computeLoadInfoPtr != nullptr) {
        Utility::Visualize::WriteBin<VT::COMPUTE_LOAD_FIGURE>(outputPath, computeLoadInfoPtr->figure);
        Utility::Visualize::WriteBin<VT::COMPUTE_LOAD_TABLE>(outputPath, computeLoadInfoPtr->table);
    } else {
        Utility::LogDebug("computeLoadInfoPtr is null, skipping writing compute load data");
    }

    storageAccess_->StorageAccessToJson(metrics.isKernelScale, metrics.isMemoryDetail);
    Utility::Visualize::WriteBin<VT::STORAGE_ACCESS_HEAT_MAP>(outputPath,
                                                              storageAccess_->GetStorageAccessHeatMap());
    Utility::Visualize::WriteBin<VT::STORAGE_ACCESS_TABLE>(outputPath,
                                                           storageAccess_->GetStorageAccessTable());

    if (metrics.occupancyEnable) {
        nlohmann::json occupancyMapJson;
        if (occupancy_->GetOccupancyMap(occupancyMapJson)) {
            Utility::Visualize::WriteBin<VT::OCCUPANCY_MAP>(outputPath, occupancyMapJson);
        }
    }

    if (metrics.roofline) {
        auto res = storageAccess_->GetPipeLineRatio();
        for (auto &pair : res) {
            roofLine_->SetPipeLineRatio(pair.first, pair.second);
        }
        roofLine_->RoofLineToJson();
        Utility::Visualize::WriteBin<VT::ROOF_LINE>(outputPath, roofLine_->visualRoofLineJson_);
    }

    nlohmann::json traceJson;
    if ((metrics.pipeTimelineEnable || metrics.instrTimelineEnable) && biuTimeline_->TimelineToJson(outputPath)) {
        traceJson = biuTimeline_->GetTimelineJson();
    } else if (timelineParser_->TimelineToJson(outputPath)) {
        traceJson = timelineParser_->GetTimelineJson();
    }
    WarpTimelineVisualize warpTimelineVisualize;
    if (warpTimelineVisualize.TimelineToJson(outputPath)) {
        MergeTraceJson(traceJson, warpTimelineVisualize.GetTimelineJson());
    }
    if (HasTraceEvents(traceJson)) {
        std::string tracePath = Utility::JoinPath({outputPath, "trace.json"});
        if (!Utility::WriteFileByStream(tracePath, traceJson)) {
            Utility::LogWarn("Generate %s failed", tracePath.c_str());
        }
        Utility::Visualize::WriteBin<VT::TRACE>(outputPath, traceJson);
    }
    if (cachelineHeatMapParser_->ToJson(outputPath)) {
        Utility::Visualize::WriteBin<VT::CACHELINE_HEAT_MAP>(outputPath,
                                                             cachelineHeatMapParser_->GetCacheLineHeatMapJson());
    }
}

void DataVisualize::CleanupAndLog(Profiling::Parse::DataCenter &dataCenter, const std::string &outputPath) {
    occupancy_->ClearOccupancyJson();
    opBasicInfo_->ClearOpBasicFileJson();
    storageAccess_->ClearStorageAccessJson();
    roofLine_->ClearRoofLineJson();
    dataCenter.DataTableUnRegister({typeid(Parse::ComputeLoadInfo)});
    Utility::LogDebug("Profiling file [%s] saved in %s.", outputPath.c_str(), Utility::VISUALIZE_DATA_BIN);
}

}
