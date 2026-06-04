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

#ifndef MSOPT_WARP_TIMELINE_VISUALIZE_H
#define MSOPT_WARP_TIMELINE_VISUALIZE_H

#include <cstdint>
#include <string>
#include <vector>
#include "json.hpp"
#include "common/dbi_defs.h"

namespace Visualize {
class WarpTimelineVisualize {
public:
    bool TimelineToJson(const std::string &outputPath);
    nlohmann::json GetTimelineJson() const { return timelineJson_; }

private:
    bool LoadRecords(const std::string &outputPath, std::vector<char> &binData) const;
    void AppendBlockEvents(
        const char *blockData, uint32_t blockId, int64_t aicoreFreq, std::vector<nlohmann::json> &traceEvents) const;
    nlohmann::json timelineJson_;
};
}

#endif // MSOPT_WARP_TIMELINE_VISUALIZE_H
