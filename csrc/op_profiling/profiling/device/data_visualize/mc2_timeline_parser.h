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
#include "profiling/simulator/data_parse/sim_defs.h"

namespace Visualize {
using json = nlohmann::json;

// 用于临时存储同一操作类型的start和end信息
struct OperationInfo {
    uint64_t startSyscyc;
    uint32_t blockId;
    uint64_t startCurPc;
    uint64_t endSyscyc;
    bool startFound;
    bool endFound;
};

class MC2TimelineParser {
public:
    MC2TimelineParser(bool isMC2, AcsqTimeMapType acsqTimeMap, uint64_t minMc2TimeCyc,
                      std::shared_ptr<OpBasicInfo> &opBasicInfoObj, std::shared_ptr<BasicPmu> &basicPmuObj)
        : isMC2_(isMC2), acsqTimeMap_(std::move(acsqTimeMap)), minSysCyc_(minMc2TimeCyc),
        opBasicInfoObj_(opBasicInfoObj), basicPmuObj_(basicPmuObj) {}
    bool MC2TimelineToJson(const std::string &outputPath);
    json GetMC2TimelineJson() { return mc2TimelineJson_; }

private:
    void PreProcessData(BlockSystemTimeType &blockSystemTimes,
                        std::vector<MsprofAicTimeStampInfo> &aicoreTimeStamps,
                        std::vector<AicpuKfcProfCommTurn> &aicpuTurns,
                        std::vector<MsprofAicpuHcclTaskInfo> &aicpuHcclTasks);
    void ProcessJsonData(const BlockSystemTimeType &blockSystemTimes,
                         const std::vector<MsprofAicTimeStampInfo> &aicoreTimeStamps,
                         const std::vector<AicpuKfcProfCommTurn> &aicpuTurns,
                         const std::vector<MsprofAicpuHcclTaskInfo> &aicpuHcclTasks);
    void SortTimelineByIds(const std::set<uint16_t> &sortIds, const std::string &pidName, const std::string &tidName);
    // PreProcess
    void GetAicoreTimeStamps(std::vector<MsprofAicTimeStampInfo> &aicoreTimeStamps);
    void GetAicpuDatas(std::vector<AicpuKfcProfCommTurn> &aicpuTurns,
                       std::vector<MsprofAicpuHcclTaskInfo> &aicpuHcclTasks);
    // ProcessAicore
    void ProcessAicoreData(const BlockSystemTimeType &blockSystemTimes,
                           const std::vector<MsprofAicTimeStampInfo> &aicoreTimeStamps);
    void AddAicoreBlockDur(const BlockSystemTimeType &blockSystemTimes, std::set<uint16_t> &dotBlockIds);
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

uint16_t GetAicoreDotBlockId(const std::string &opType, uint16_t blockIndex, uint16_t subBlockIndex,
                             uint16_t subBlockNum);
inline std::string GetAicoreBlockName(uint16_t dotBlockId)
{
    if (dotBlockId >= CUBE_BLOCK_START_INDEX) {
        return "AIC_BLOCK" + std::to_string(dotBlockId - CUBE_BLOCK_START_INDEX);
    }
    return "AIV_BLOCK" + std::to_string(dotBlockId);
}

inline std::string GetAicoreTimeLinePid(uint16_t dotBlockId)
{
    return (dotBlockId >= CUBE_BLOCK_START_INDEX) ? "AIC BLOCK" : "AIV BLOCK";
}
}

#endif // MC2_H