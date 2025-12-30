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

#ifndef TIMELINE_VISUALIZE_H
#define TIMELINE_VISUALIZE_H
#include <string>
#include <vector>
#include "profiling/device/data_parse/parse_timeline.h"

namespace Visualize {

constexpr int FREQ_DEFAULT = 1650;  // MHz
class TimelineVisualize {
public:
    explicit TimelineVisualize(std::vector<Profiling::TimelineInfo> timelineVec) : timelineVec_(timelineVec) {}
    void TimelineToJson();
    nlohmann::json timelineJson_;

private:

    std::unordered_map<std::string, std::string> cnames_{{Profiling::SU_PIPE, "startup"},
        {Profiling::VEC_PIPE, "rail_idle"}, {Profiling::CUBE_PIPE, "rail_response"},
        {Profiling::MTE1_PIPE, "thread_state_iowait"}, {Profiling::MTE2_PIPE, "yellow"},
        {Profiling::MTE3_PIPE, "rail_animation"}, {Profiling::FIXP_PIPE, "thread_state_unknown"}};
    std::vector<Profiling::TimelineInfo> timelineVec_;
};
}
#endif