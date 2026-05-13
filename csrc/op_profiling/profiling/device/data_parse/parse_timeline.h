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

#ifndef PARSE_TIMELINE_H
#define PARSE_TIMELINE_H
#include <string>
#include <vector>
#include <unordered_map>
#include "json.hpp"


namespace Profiling {
constexpr int PIPE_STATE_BIT_OFFSET = 12;                   // biuInfo 位移量，低12位标识不同 pipe 的状态
constexpr int DATA_BUFFER_SIZE = 1024 * 1024 * 2;           // InstrProf 数据量长度为2M
constexpr int TIMELINE_PIPE_NUM = 7;                        // timeline pipe有7类
constexpr int INSTR_PROF_CHANNEL_NUM = 18;                  // instr profiling 通道共有18个
constexpr char const *SU_PIPE = "SCALAR";                   // bit 0，biuInfo：1111000000000001，表示当前SCALAR繁忙
constexpr char const *VEC_PIPE = "VECTOR";                  // bit 1，biuInfo：1111000000000010，表示当前VECTOR繁忙
constexpr char const *CUBE_PIPE = "CUBE";                   // bit 2，biuInfo：1111000000000100，表示当前CUBE繁忙
constexpr char const *MTE1_PIPE = "MTE1";                   // bit 3，biuInfo：1111000000001000，表示当前MTE1繁忙
constexpr char const *MTE2_PIPE = "MTE2";                   // bit 4，biuInfo：1111000000010000，表示当前MTE2繁忙
constexpr char const *MTE3_PIPE = "MTE3";                   // bit 5，biuInfo：1111000000100000，表示当前MTE3繁忙
constexpr char const *FIXP_PIPE = "Fixpipe";                // bit 6，biuInfo：1111000001000000，表示当前Fixpipe繁忙
constexpr char const *CUBE_CORE = "cubecore";
constexpr char const *VECTOR_CORE_0 = "veccore0";
constexpr char const *VECTOR_CORE_1 = "veccore1";

struct TimelineInfo {
    TimelineInfo() = default;
    TimelineInfo(const std::string &pipe, const std::string &core, const std::string &line, uint64_t st, uint64_t dur)
        : pipeName(pipe), coreName(core), lineName(line), start(st), duration(dur) {}
    std::string pipeName;
    std::string coreName;
    std::string lineName;
    uint64_t start = 0;
    uint64_t duration = 0;      // 和上一条数据相差的cycle数量
};

struct InstrProfHeadInfo {
    uint16_t coreId = 0;
    uint16_t coreType = 0;   // 标识core类型，0：CUBE，1：VECTOR，2：SCALAR
    uint32_t validLen = 0;
};

struct BiuPerfInfo {
    uint16_t cycles;
    uint16_t biuInfo;           // 高4位表示数据类型，低12位表示不同 pipe 的状态（空闲或者繁忙）
};

class ParseTimeline {
public:
    inline std::vector<TimelineInfo> GetTimeline()
    {
        std::vector<TimelineInfo> result;
        for (const auto& channelVec : timelineVec_) {
            result.insert(result.end(), channelVec.begin(), channelVec.end());
        }
        return result;
    }
    bool GenerateBiuTimeStamps(const std::string &outputPath);

private:
    bool GenerateBiuTimeStampsOneBin(const std::string &filePath,
                                     std::unordered_map<uint32_t, uint64_t> &totalCyclesMap);
    void ParseOriginInfo(const BiuPerfInfo &originInfo, const InstrProfHeadInfo &instrProfHeadInfo,
                         std::unordered_map<uint32_t, uint64_t> &totalCyclesMap);
    void ParseMarkStamp(const BiuPerfInfo &originInfo, const InstrProfHeadInfo &instrProfHeadInfo,
                        std::unordered_map<uint32_t, uint64_t> &totalCyclesMap);
    void GenMark(const std::string &pipe, const BiuPerfInfo &originInfo, const InstrProfHeadInfo &instrProfHeadInfo,
                 std::unordered_map<uint32_t, uint64_t> &totalCyclesMap);
    void UpdateEndMarks(uint16_t markId, uint32_t channelId);
    std::vector<std::string> subCore_ {CUBE_CORE, VECTOR_CORE_0, VECTOR_CORE_1};
    std::vector<std::string> pipe_ = {SU_PIPE, VEC_PIPE, CUBE_PIPE, MTE1_PIPE, MTE2_PIPE, MTE3_PIPE, FIXP_PIPE};
    TimelineInfo timelineStatesVec_[INSTR_PROF_CHANNEL_NUM][TIMELINE_PIPE_NUM] = {};
    std::vector<std::vector<TimelineInfo>> timelineVec_{INSTR_PROF_CHANNEL_NUM};
    const std::unordered_map<uint16_t, std::string> pipeMap_ = {
    {0, SU_PIPE}, {1, VEC_PIPE}, {2, CUBE_PIPE}, {3, MTE1_PIPE}, {4, MTE2_PIPE}, {5, MTE3_PIPE}, {10, FIXP_PIPE},
    };
    // endMarks用来屏蔽workaround打点 channelId -> endMark起点在该channel vector中的索引
    std::unordered_map<uint32_t, size_t> endMarks_;
};
}
#endif