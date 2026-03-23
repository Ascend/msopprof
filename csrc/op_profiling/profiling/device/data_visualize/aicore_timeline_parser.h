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

#ifndef AICORE_TIMELINE_H
#define AICORE_TIMELINE_H

#include <utility>
#include <vector>
#include <cstdint>
#include "json.hpp"
#include "basic_op_and_pmu.h"
#include "profapi/profapi.h"

namespace Visualize {
using json = nlohmann::json;

constexpr int64_t FREQ = 50000; // kHz
constexpr uint16_t TIME_CONVERSION = 1000; // time in visualize.bin will be us, cyc/FREQ unit is ms
constexpr uint16_t SORT_BIAS = 10000;
constexpr uint16_t VEC_START = 25;
constexpr uint32_t TIME_STAMP_START = 0xffff;
constexpr char const *AIC_BLOCK = "AIC BLOCK";
constexpr char const *AIV_BLOCK = "AIV BLOCK";

// 用于临时存储同一操作类型的start和end信息
struct OperationInfo {
    uint64_t startSyscyc;
    uint32_t blockId;
    uint64_t startCurPc;
    uint64_t endSyscyc;
    bool startFound;
    bool endFound;
    std::string type;
};


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


class AicoreTimelineParser {
public:
    AicoreTimelineParser(uint64_t minTimeCyc, std::shared_ptr<OpBasicInfo> &opBasicInfoObj, std::shared_ptr<BasicPmu> &basicPmuObj)
        : minSysCyc_(minTimeCyc), opBasicInfoObj_(opBasicInfoObj), basicPmuObj_(basicPmuObj)
        {
            for (const auto &pair: basicPmuObj_->GetTotalPmuData()) {
                blockSystemTimes_[pair.first.first].emplace_back(pair.second.systemTime);
                minSysCyc_ = std::min(pair.second.systemTime.first, minSysCyc_);
                if (pair.second.blockType == Common::OpType::VECTOR) {
                    aivBlockNum_ ++;
                }
                if (pair.second.blockType == Common::OpType::CUBE) {
                    aicBlocKNum_ ++;
                }
            }
        }
    bool AicoreTimelineToJson(const std::string &outputPath);
    json GetAicoreTimelineJson() { return aicoreTimelineJson_; }
    bool GetAicoreTimeStamps(std::vector<MsprofAicTimeStampInfo> &aicoreTimeStamps, std::vector<std::string> &type);
    void GenPc2Code(std::vector<MsprofAicTimeStampInfo> &aicoreTimeStamps);
    void ProcessBlockDur(const std::vector<MsprofAicTimeStampInfo> &aicoreTimeStamps, std::vector<std::string> &type);
    void SetOutputPath(const std::string &outputPath) { outputPath_ = outputPath; };
    uint64_t GetMinSysCycle() const { return minSysCyc_; }
    json GetTraceEvent() const {return traceEvents_; }
    void ProcessAicoreData(const std::vector<MsprofAicTimeStampInfo> &aicoreTimeStamps, std::vector<std::string> &type);
    std::map<std::pair<std::string, uint32_t>, std::pair<uint64_t, uint64_t>> GetBlockDurRange() { return  blockDuration_; };
private:
    void AddAicoreBlockDur(const BlockSystemTimeType &blockSystemTimes, std::set<uint16_t> &aicDotBlockIds, std::set<uint16_t> &aivDotBlockIds);
    void GetTimeStampType(std::vector<MsprofAicTimeStampInfo> &aicoreTimeStamps, std::vector<std::string> &type);
    // key {pid：类型aic/aiv, tid: block}, value:对应的aicore运行时间始末记录
    std::map<std::pair<std::string, uint32_t>, std::pair<uint64_t, uint64_t>> blockDuration_;
    BlockSystemTimeType blockSystemTimes_;
    uint64_t minSysCyc_ = UINT64_MAX;
    std::shared_ptr<OpBasicInfo> &opBasicInfoObj_;
    std::shared_ptr<BasicPmu> &basicPmuObj_;
    Profiling::Pc2CodeMap pc2code_;
    std::string outputPath_;
    uint32_t aicBlocKNum_ = 0;
    uint32_t aivBlockNum_ = 0;
    int64_t aicpuFreq_ = 0;
    std::map<std::tuple<uint32_t, uint32_t, std::string>, std::vector<OperationInfo>> timeStampInfo_;
    nlohmann::json aicoreTimelineJson_;
    nlohmann::json traceEvents_;
};

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

#endif // AICORE_TIMELINE_H