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


#ifndef MSOPT_LCCL_TIMELINE_PARSER_H
#define MSOPT_LCCL_TIMELINE_PARSER_H
#include <vector>
#include "json.hpp"

#include "basic_op_and_pmu.h"
#include "data_visualize_const.h"
#include "profapi/profapi.h"

namespace Visualize {
struct LcclInfo {
    uint32_t logId;
    uint32_t blockId;
    uint32_t operationType;
    uint32_t rsv;
    uint64_t startSyscyc;
    uint64_t endSyscyc;
    std::string curPc;
};

class LcclTimelineParser {
public:
    LcclTimelineParser(bool isLccl, uint64_t minLcclTimeCyc, std::shared_ptr<OpBasicInfo> &opBasicInfoObj,
                       std::shared_ptr<BasicPmu> &basicPmuObj)
        : isLccl_(isLccl), minSysCyc_(minLcclTimeCyc), aicoreStartCyc_(minLcclTimeCyc),
        opBasicInfoObj_(opBasicInfoObj), basicPmuObj_(basicPmuObj) {}
    bool TimelineToJson(const std::string &outputPath);
    nlohmann::json GetTimelineJson() { return timelineJson_; }

private:
    void PreProcessData(BlockSystemTimeType &blockSystemTimes, std::vector<LcclDumpLogInfo> &aicoreTimeStamps);
    void ProcessJsonData(const BlockSystemTimeType &blockSystemTimes,
                         const std::vector<LcclDumpLogInfo> &aicoreTimeStamps);
    void GetAicoreTimeStamps(std::vector<LcclDumpLogInfo> &aicoreTimeStamps);
    // ProcessAicore
    void ProcessAicoreData(const BlockSystemTimeType &blockSystemTimes,
                           const std::vector<LcclDumpLogInfo> &aicoreTimeStamps);
    void AddAicoreBlockDur(const BlockSystemTimeType &blockSystemTimes, std::set<uint16_t> &dotBlockIds);
    nlohmann::json AddAicoreApiDot(const LcclInfo& info) const;

    nlohmann::json timelineJson_;
    nlohmann::json traceEvents_;

    std::string outputPath_;
    bool isLccl_ = false;
    uint64_t minSysCyc_ = UINT64_MAX;
    uint64_t aicoreStartCyc_ = 0;
    std::shared_ptr<OpBasicInfo> &opBasicInfoObj_;
    std::shared_ptr<BasicPmu> &basicPmuObj_;
    int64_t aicpuFreq_ = 0;
};
}
#endif // MSOPT_LCCL_TIMELINE_PARSER_H
