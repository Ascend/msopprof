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

#ifndef __MSOPPROF_DATA_VISUALIZE_H__
#define __MSOPPROF_DATA_VISUALIZE_H__

#include <string>
#include <map>
#include "json.hpp"
#include "profiling/device/data_parse/pmu_calculate.h"
#include "common/prof_args.h"
#include "filesystem.h"
#include "basic_op_and_pmu.h"
#include "storage_access.h"
#include "roofline.h"
#include "occupancy.h"
#include "mc2_timeline_parser.h"
#include "lccl_timeline_parser.h"
#include "cacheline_heat_map.h"
#include "parse/data_calculator/compute_load_calculator.h"

namespace Visualize {
struct DataVisualizePtr {
    std::shared_ptr<OpBasicInfo> &opBasicInfo;
    std::unique_ptr<StorageAccess> &storageAccess;
    std::unique_ptr<Occupancy> &occupancy;
    std::unique_ptr<RoofLine> &roofLine;
    std::unique_ptr<MC2TimelineParser> &mc2TimelineParser;
    std::unique_ptr<LcclTimelineParser> &lcclTimelineParser;
    std::unique_ptr<CachelineHeatMap> &cachelineHeatMapParser;
    std::unique_ptr<AicoreTimelineParser> &aicoreTimelineParser;
};

class DataVisualize {
public:
    explicit DataVisualize(DataVisualizePtr ptr)
        : opBasicInfo_(ptr.opBasicInfo),
          storageAccess_(ptr.storageAccess),
          occupancy_(ptr.occupancy),
          roofLine_(ptr.roofLine),
          mc2TimelineParser_(ptr.mc2TimelineParser),
          lcclTimelineParser_(ptr.lcclTimelineParser),
          cachelineHeatMapParser_(ptr.cachelineHeatMapParser),
          aicoreTimelineParser_(ptr.aicoreTimelineParser)
    {}

    void GenerateVisualizeData(Profiling::Parse::DataCenter &dataCenter, const std::string &outputPath,
                               const Common::ProfMetricsAbilityConfig &metrics);
    void GenerateAllVisualizeData(Profiling::Parse::DataCenter &dataCenter, const std::string &outputPath,
                               const Common::ProfMetricsAbilityConfig &metrics);
    void CleanupAndLog(Profiling::Parse::DataCenter &dataCenter, const std::string &outputPath);

private:
    std::shared_ptr<OpBasicInfo> &opBasicInfo_;
    std::unique_ptr<StorageAccess> &storageAccess_;
    std::unique_ptr<Occupancy> &occupancy_;
    std::unique_ptr<RoofLine> &roofLine_;
    std::unique_ptr<MC2TimelineParser> &mc2TimelineParser_;
    std::unique_ptr<LcclTimelineParser> &lcclTimelineParser_;
    std::unique_ptr<CachelineHeatMap> &cachelineHeatMapParser_;
    std::unique_ptr<AicoreTimelineParser> &aicoreTimelineParser_;
};
} // namespace Visualize


#endif // __MSOPPROF_DATA_VISUALIZE_H__
