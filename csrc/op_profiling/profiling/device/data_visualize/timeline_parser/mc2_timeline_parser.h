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


struct AiCoreDot {
    static constexpr char const *INIT = "INIT";
    static constexpr char const *COMMIT = "COMMIT";
    static constexpr char const *WAIT = "WAIT";
    static constexpr char const *QUERY = "QUERY";
    static constexpr char const *FINALIZE = "FINALIZE";
    static constexpr char const *GROUP_SYNC = "GROUP_SYNC";
    static constexpr char const *GET_WIN_IN = "GET_WINDOW_IN";
    static constexpr char const *GET_WIN_OUT = "GET_WINDOW_OUT";
    static constexpr char const *GET_RANK_ID = "GET_RANK_ID";
    static constexpr char const *GET_RANK_DIM = "GET_RANK_DIM";
    static constexpr char const *SET_CCTILING = "SET_CCTILING";
    static constexpr char const *ALL_REDUCE = "ALL_REDUCE_PREPARE";
    static constexpr char const *ALL_GATHER = "ALL_GATHER_PREPARE";
    static constexpr char const *REDUCE_SCATTER = "REDUCE_SCATTER_PREPARE";
    static constexpr char const *ALL_TO_ALL = "ALL_TO_ALL_PREPARE";
    static constexpr char const *ALL_TO_ALL_V = "ALL_TO_ALL_V_PREPARE";
};

const std::map<uint32_t, std::string> AICORE_DOT_MAP = {
    // start都是偶数，end都是奇数（start+1）
    {0x1000, std::string(AiCoreDot::INIT)},
    {0x1001, std::string(AiCoreDot::INIT)},
    {0x1010, std::string(AiCoreDot::COMMIT)},
    {0x1011, std::string(AiCoreDot::COMMIT)},
    {0x1020, std::string(AiCoreDot::WAIT)},
    {0x1021, std::string(AiCoreDot::WAIT)},
    {0x1030, std::string(AiCoreDot::QUERY)},
    {0x1031, std::string(AiCoreDot::QUERY)},
    {0x1040, std::string(AiCoreDot::FINALIZE)},
    {0x1041, std::string(AiCoreDot::FINALIZE)},
    {0x1050, std::string(AiCoreDot::GROUP_SYNC)},
    {0x1051, std::string(AiCoreDot::GROUP_SYNC)},
    {0x1060, std::string(AiCoreDot::GET_WIN_IN)},
    {0x1061, std::string(AiCoreDot::GET_WIN_IN)},
    {0x1062, std::string(AiCoreDot::GET_WIN_OUT)},
    {0x1063, std::string(AiCoreDot::GET_WIN_OUT)},
    {0x1064, std::string(AiCoreDot::GET_RANK_ID)},
    {0x1065, std::string(AiCoreDot::GET_RANK_ID)},
    {0x1066, std::string(AiCoreDot::GET_RANK_DIM)},
    {0x1067, std::string(AiCoreDot::GET_RANK_DIM)},
    {0x1068, std::string(AiCoreDot::SET_CCTILING)},
    {0x1069, std::string(AiCoreDot::SET_CCTILING)},
    {0x1100, std::string(AiCoreDot::ALL_REDUCE)},
    {0x1101, std::string(AiCoreDot::ALL_REDUCE)},
    {0x1110, std::string(AiCoreDot::ALL_GATHER)},
    {0x1111, std::string(AiCoreDot::ALL_GATHER)},
    {0x1120, std::string(AiCoreDot::REDUCE_SCATTER)},
    {0x1121, std::string(AiCoreDot::REDUCE_SCATTER)},
    {0x1130, std::string(AiCoreDot::ALL_TO_ALL)},
    {0x1131, std::string(AiCoreDot::ALL_TO_ALL)},
    {0x1140, std::string(AiCoreDot::ALL_TO_ALL_V)},
    {0x1141, std::string(AiCoreDot::ALL_TO_ALL_V)}
};


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