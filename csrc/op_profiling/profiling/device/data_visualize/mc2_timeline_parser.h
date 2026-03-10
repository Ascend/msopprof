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

#ifndef MC2_H
#define MC2_H

#include <fstream>
#include <utility>
#include <vector>
#include <cstdint>
#include "json.hpp"

#include "basic_op_and_pmu.h"
#include "data_visualize_const.h"
#include "profapi/profapi.h"
#include "aicore_timeline_parser.h"
#include "profiling/simulator/data_parse/sim_defs.h"

namespace Visualize {
using json = nlohmann::json;

class MC2TimelineParser {
public:
    MC2TimelineParser(bool isMC2, AcsqTimeMapType acsqTimeMap, uint64_t minMc2TimeCyc,
                      std::shared_ptr<OpBasicInfo> &opBasicInfoObj, std::shared_ptr<BasicPmu> &basicPmuObj)
        : isMC2_(isMC2), acsqTimeMap_(std::move(acsqTimeMap)), minSysCyc_(minMc2TimeCyc),
        opBasicInfoObj_(opBasicInfoObj), basicPmuObj_(basicPmuObj) {}
    bool MC2TimelineToJson(const std::string &outputPath);
    json GetMC2TimelineJson() { return mc2TimelineJson_; }

private:
    void PreProcessData(AicoreTimelineParser &timelineParser,
                        std::vector<MsprofAicTimeStampInfo> &aicoreTimeStamps,
                        std::vector<AicpuKfcProfCommTurn> &aicpuTurns,
                        std::vector<MsprofAicpuHcclTaskInfo> &aicpuHcclTasks,
                        std::vector<std::string> &type);
    void ProcessJsonData(AicoreTimelineParser &timelineParser,
                         const std::vector<MsprofAicTimeStampInfo> &aicoreTimeStamps,
                         const std::vector<AicpuKfcProfCommTurn> &aicpuTurns,
                         const std::vector<MsprofAicpuHcclTaskInfo> &aicpuHcclTasks,
                         std::vector<std::string> &type);
    void SortTimelineByIds(const std::set<uint16_t> &sortIds, const std::string &pidName, const std::string &tidName);
    // PreProcess
    void GetAicpuDatas(std::vector<AicpuKfcProfCommTurn> &aicpuTurns,
                       std::vector<MsprofAicpuHcclTaskInfo> &aicpuHcclTasks);
    // ProcessAicore
    void ProcessAicoreData(AicoreTimelineParser &timelineParser, const std::vector<MsprofAicTimeStampInfo> &aicoreTimeStamps, std::vector<std::string> &type);
    json BuildAicoreDot(const OperationInfo& info, const std::string& operationType);
    // ProcessHccl, total aicore/aicpu time
    void ProcessHcclData();
    void ProcessHcclTaskData(const std::vector<MsprofAicpuHcclTaskInfo> &aicpuHcclTasks);
    void ProcessAicpuTurnData(const std::vector<AicpuKfcProfCommTurn> &aicpuTurns);
    void AddAicpuTurnJson(const AicpuKfcProfCommTurn &turn, const std::string &turnName, uint64_t startTime,
                          uint64_t endTime);

    nlohmann::json mc2TimelineJson_;
    nlohmann::json traceEvents_;

    std::string outputPath_;
    bool isMC2_ = false;
    AcsqTimeMapType acsqTimeMap_;
    uint64_t minSysCyc_ = UINT64_MAX;
    std::shared_ptr<OpBasicInfo> &opBasicInfoObj_;
    std::shared_ptr<BasicPmu> &basicPmuObj_;

    int64_t aicpuFreq_ = 0;
    Profiling::Pc2CodeMap pc2code_;
};
}

#endif // MC2_H