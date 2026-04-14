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

#include "profiling/device/data_visualize/basic_op_and_pmu.h"
#include "profiling/device/data_visualize/data_visualize_const.h"
#include "profapi/profapi.h"
#include "timeline_parser.h"

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

uint16_t GetAicoreDotBlockId(const std::string &opType, uint16_t blockIndex, uint16_t subBlockIndex,
                             uint16_t subBlockNum);

class LcclTimelineParser : public TimelineParser{
public:
    LcclTimelineParser(uint64_t minLcclTimeCyc, std::shared_ptr<OpBasicInfo> &opBasicInfoObj,
                       std::shared_ptr<BasicPmu> &basicPmuObj)
        : TimelineParser(minLcclTimeCyc, opBasicInfoObj, basicPmuObj), aicoreStartCyc_(minLcclTimeCyc)
        {}
    bool TimelineToJson(const std::string &outputPath);

private:
    void PreProcessData(std::vector<LcclDumpLogInfo> &aicoreTimeStamps);
    void ProcessJsonData(const std::vector<LcclDumpLogInfo> &aicoreTimeStamps);
    // ProcessAicore
    void ProcessAicoreData(const std::vector<LcclDumpLogInfo> &aicoreTimeStamps);
    nlohmann::json AddAicoreApiDot(const LcclInfo& info) const;
    uint64_t aicoreStartCyc_ = 0;
};
}
#endif // MSOPT_LCCL_TIMELINE_PARSER_H
