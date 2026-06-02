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

#ifndef BIU_TIMELINE_H
#define BIU_TIMELINE_H
#include <string>
#include <vector>
#include <unordered_map>
#include "json.hpp"


namespace Visualize {
constexpr int FREQ_DEFAULT = 1650;                          // MHz
constexpr int STATE_BIT_OFFSET = 12;                        // biuInfo位移量，低12位标识dfx-region id或pipe状态
constexpr int DATA_BUFFER_SIZE = 1024 * 1024 * 2;           // InstrProf数据量长度为2M
constexpr int PIPE_TIMELINE_PIPE_NUM = 7;                   // pipetimeline pipe有7类
constexpr int INSTR_PROF_CHANNEL_NUM = 18;                  // instr profiling 通道共有18个
constexpr char const *SU_PIPE = "SCALAR";                   // bit 0，biuInfo：1111000000000001，表示当前SCALAR繁忙
constexpr char const *VEC_PIPE = "VECTOR";                  // bit 1，biuInfo：1111000000000010，表示当前VECTOR繁忙
constexpr char const *CUBE_PIPE = "CUBE";                   // bit 2，biuInfo：1111000000000100，表示当前CUBE繁忙
constexpr char const *MTE1_PIPE = "MTE1";                   // bit 3，biuInfo：1111000000001000，表示当前MTE1繁忙
constexpr char const *MTE2_PIPE = "MTE2";                   // bit 4，biuInfo：1111000000010000，表示当前MTE2繁忙
constexpr char const *MTE3_PIPE = "MTE3";                   // bit 5，biuInfo：1111000000100000，表示当前MTE3繁忙
constexpr char const *FIXP_PIPE = "FIXP";                   // bit 6，biuInfo：1111000001000000，表示当前Fixpipe繁忙
constexpr char const *CUBE_CORE = "cubecore";
constexpr char const *VECTOR_CORE_0 = "veccore0";
constexpr char const *VECTOR_CORE_1 = "veccore1";

struct BiuTimelineInfo {
    BiuTimelineInfo() = default;
    BiuTimelineInfo(const std::string &pipe, const std::string &core, const std::string &line, uint64_t st, uint64_t dur, std::map<std::string, std::string> args={})
        : pipeName(pipe), coreName(core), lineName(line), start(st), duration(dur), args(args) {}
    std::string pipeName;
    std::string coreName;
    std::string lineName;
    uint64_t start = 0;
    uint64_t duration = 0;
    std::map<std::string, std::string> args;
};

struct InstrProfHeadInfo {
    uint16_t coreId = 0;
    uint16_t coreType = 0;   // 标识core类型，0：CUBE，1：VECTOR，2：SCALAR
    uint32_t validLen = 0;
};

struct BiuPerfInfo {
    uint16_t cycles;
    uint16_t biuInfo;       // 高4位表示数据类型，低12位表示dfx-region id或pipe状态（空闲/繁忙）
};

struct EndMarkState {
    size_t startIndex = 0;
    size_t nextExpectedIdx = 0;
};

struct DfxRegionInfo {
    uint64_t pc;
    std::string InstrName;
};

class BiuTimeline {
public:
    explicit BiuTimeline() = default;
    virtual ~BiuTimeline() = default;
    bool TimelineToJson(const std::string &outputPath);
    nlohmann::json GetTimelineJson() { return timelineJson_; }
protected:
    virtual void ParsePipeState(uint16_t pipeMask, uint32_t channelId, const std::string &coreName) {};
    virtual void ParseDfxRegion(uint16_t dfxRegionId, uint32_t channelId, const std::string &coreName, const std::string &pipe) {};
    virtual void PrintMissData() {};
    void UpdateEndMarks(uint16_t dfxRegionId, uint32_t channelId);
    // 各个通道当前cycle{channelId, totalCycle}
    std::unordered_map<uint32_t, uint64_t> channelCycleMap_;
    // 各个通道workaround连续打点结束标志{channelId, EndMarkState}
    std::unordered_map<uint32_t, EndMarkState> channelEndMarkMap_;
    std::vector<std::vector<BiuTimelineInfo>> timelineVec_{INSTR_PROF_CHANNEL_NUM};
    std::string outputPath_;

private:
    bool ParseBiuTimeStamps();
    bool ParseSingleBiuTimeStamps(const std::string &filePath);
    std::unordered_map<std::string, std::string> cnames_{
        {SU_PIPE, "startup"}, {VEC_PIPE, "rail_idle"}, {CUBE_PIPE, "rail_response"},
        {MTE1_PIPE, "thread_state_iowait"}, {MTE2_PIPE, "yellow"},
        {MTE3_PIPE, "rail_animation"}, {FIXP_PIPE, "thread_state_unknown"}
    };
    const std::array<std::string, 3> subCore_ = {CUBE_CORE, VECTOR_CORE_0, VECTOR_CORE_1};
    const std::array<uint16_t, 8> endMarkSequence_ = {0xd88, 0xd99, 0xdaa, 0xdbb, 0xdcc, 0xddd, 0xdee, 0xdff};
    const std::unordered_map<uint16_t, std::string> dfxPipeMap_ = {
        {0, SU_PIPE}, {1, VEC_PIPE}, {2, CUBE_PIPE}, {3, MTE1_PIPE}, {4, MTE2_PIPE}, {5, MTE3_PIPE}, {10, FIXP_PIPE},
    };
    nlohmann::json timelineJson_;
};

class PipeBiuTimeline : public BiuTimeline {
private:
    void ParseDfxRegion(uint16_t dfxRegionId, uint32_t channelId, const std::string &coreName, const std::string &pipe) override;
    void ParsePipeState(uint16_t pipeMask, uint32_t channelId, const std::string &coreName) override;
    const std::array<std::string, PIPE_TIMELINE_PIPE_NUM> statePipe_ = {SU_PIPE, VEC_PIPE, CUBE_PIPE, MTE1_PIPE, MTE2_PIPE, MTE3_PIPE, FIXP_PIPE};
    BiuTimelineInfo pipeStateVec_[INSTR_PROF_CHANNEL_NUM][PIPE_TIMELINE_PIPE_NUM] = {};
};

class InstrBiuTimeline : public BiuTimeline {
private:
    void ParseDfxRegion(uint16_t dfxRegionId, uint32_t channelId, const std::string &coreName, const std::string &pipe) override;
    void PrintMissData() override;
    void PrintPipeIdFull();
    // 指令打点起始数据 Key: {pipeName, channelId, dfxRealId}, Value: startCycle
    std::map<std::tuple<const std::string, uint32_t, uint16_t>, uint64_t> startCache_;
    // 编译器提供dfx打点详细信息 Key: {pipeName, dfxStartId}, Value: DfxRegionInfo
    std::map<std::pair<std::string, uint16_t>, DfxRegionInfo> dfxRegionInfoMap_ = {};
};
}
#endif // BIU_TIMELINE_H
