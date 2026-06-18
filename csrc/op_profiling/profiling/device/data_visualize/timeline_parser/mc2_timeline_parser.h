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

#include "profiling/device/data_visualize/basic_op_and_pmu.h"
#include "profiling/device/data_visualize/data_visualize_const.h"
#include "profapi/profapi.h"
#include "aicore_timeline_parser.h"
#include "profiling/simulator/data_parse/sim_defs.h"

namespace Visualize {
using json = nlohmann::json;

class MC2TimelineParser : public AicoreTimelineParser {
public:
    MC2TimelineParser(AcsqTimeMapType acsqTimeMap, uint64_t minMc2TimeCyc,
                      std::shared_ptr<OpBasicInfo> &opBasicInfoObj, std::shared_ptr<BasicPmu> &basicPmuObj)
        : AicoreTimelineParser(minMc2TimeCyc, opBasicInfoObj, basicPmuObj),
        acsqTimeMap_(std::move(acsqTimeMap)) {}
    bool TimelineToJson(const std::string &outputPath) override;
private:
    void PreProcessData(std::vector<AicpuKfcProfCommTurn> &aicpuTurns,
                        std::vector<MsprofAicpuHcclTaskInfo> &aicpuHcclTasks,
                        std::vector<MsprofAicTimeStampInfoUpdate> &aicoreTimeStamps);
    void ProcessJsonData(const std::vector<AicpuKfcProfCommTurn> &aicpuTurns,
                         const std::vector<MsprofAicpuHcclTaskInfo> &aicpuHcclTasks,
                         std::vector<MsprofAicTimeStampInfoUpdate> &aicoreTimeStamps);
    void SortTimelineByIds(const std::set<uint16_t> &sortIds, const std::string &pidName, const std::string &tidName);
    // PreProcess
    void GetAicpuDatas(std::vector<AicpuKfcProfCommTurn> &aicpuTurns,
                       std::vector<MsprofAicpuHcclTaskInfo> &aicpuHcclTasks);
    // ProcessAicore
    void ProcessMC2AicoreData(const std::vector<MsprofAicTimeStampInfoUpdate> &aicoreTimeStamps);
    json BuildAicoreDot(const OperationInfo& info, const std::string& operationType);
    // ProcessHccl, total aicore/aicpu time
    void ProcessHcclData();
    void ProcessHcclTaskData(const std::vector<MsprofAicpuHcclTaskInfo> &aicpuHcclTasks);
    void ProcessAicpuTurnData(const std::vector<AicpuKfcProfCommTurn> &aicpuTurns);
    void AddAicpuTurnJson(const AicpuKfcProfCommTurn &turn, const std::string &turnName, uint64_t startTime,
                          uint64_t endTime);
    AcsqTimeMapType acsqTimeMap_;
};
}

#endif // MC2_H
