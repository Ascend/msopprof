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

#include "timeline_visualize.h"
#include "log.h"
#include "filesystem.h"
#include "common/hal_helper.h"
#include "profiling/device/data_parse/pmu_calculate.h"

using namespace Utility;

namespace Visualize {
void TimelineVisualize::TimelineToJson()
{
    int64_t aicoreFreq = 0;
    if (!Common::HalHelper::Instance().GetAicoreFreq(aicoreFreq)) {
        LogWarn("Get task scheduler frequency failed. Use Default value instead.");
        aicoreFreq = FREQ_DEFAULT;
    }
    timelineJson_["profilingType"] = "op-biuperf";
    std::vector<nlohmann::json> traceEvents;
    for (const auto& timeline : timelineVec_) {
        nlohmann::json timelineJson;
        timelineJson["name"] = timeline.lineName;
        timelineJson["cname"] = cnames_[timeline.pipeName];
        timelineJson["ph"] = "X";
        timelineJson["ts"] = static_cast<float>(timeline.start) / aicoreFreq;
        timelineJson["dur"] = static_cast<float>(timeline.duration) / aicoreFreq;
        timelineJson["pid"] = timeline.coreName;
        timelineJson["tid"] = timeline.pipeName;
        timelineJson["args"] = "";
        traceEvents.emplace_back(timelineJson);
    }
    timelineJson_["traceEvents"] = traceEvents;
}

}
