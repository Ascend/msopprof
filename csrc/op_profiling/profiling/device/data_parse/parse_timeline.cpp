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

#include "parse_timeline.h"
#include <algorithm>
#include <array>
#include "log.h"
#include "filesystem.h"
#include "common/hal_helper.h"


using namespace Utility;

namespace Profiling {


bool ParseTimeline::GenerateBiuTimeStamps(const std::string &outputPath)
{
    bool ret = false;
    std::string dumpPath = Utility::JoinPath({outputPath, "dump"});
    std::vector<std::string> filenames;
    if (!Utility::ListDir(dumpPath, std::back_inserter(filenames))) {
        return false;
    }
    // channel id -> this channel's timeline cycle
    std::unordered_map<uint32_t, uint64_t> totalCyclesMap;

    for (auto &file : filenames) {
        if (StartsWith(file, "timeline.bin")) {
            std::string filePath = JoinPath({dumpPath, file});
            ret |= GenerateBiuTimeStampsOneBin(filePath, totalCyclesMap);
        }
    }
    // 解析完所有timeline.bin后，一次性截断各channel中endMark起点之后的所有点
    for (const auto &it : endMarks_) {
        uint32_t channelId = it.first;
        size_t eraseStart = it.second;
        if (eraseStart < timelineVec_[channelId].size()) {
            timelineVec_[channelId].erase(timelineVec_[channelId].begin() + eraseStart,
                                           timelineVec_[channelId].end());
        }
    }
    return ret;
}

bool ParseTimeline::GenerateBiuTimeStampsOneBin(const std::string &filePath,
                                                std::unordered_map<uint32_t, uint64_t> &totalCyclesMap)
{
    // read timeline.bin
    if (!IsReadable(filePath)) {
        LogWarn("File %s is not exists or not readable.", filePath.c_str());
        return false;
    }
    size_t fileSize = GetFileSize(filePath);
    std::vector<char> totalBin(fileSize);
    size_t structSize = sizeof(InstrProfHeadInfo) + DATA_BUFFER_SIZE;
    size_t biuSize = sizeof(BiuPerfInfo);
    if (!ReadBinaryFile(filePath, totalBin)) {
        LogError("File %s failed to be read.", filePath.c_str());
        return false;
    }
    for (size_t i = 0; i < fileSize; i += structSize) {
        InstrProfHeadInfo instrProfHeadInfo;
        if (memcpy_s(&instrProfHeadInfo, sizeof(InstrProfHeadInfo), &totalBin[i], sizeof(InstrProfHeadInfo)) != EOK) {
            continue;
        }
        std::vector<char> splitData{&totalBin[i] + sizeof(InstrProfHeadInfo), &totalBin[i] + structSize};
        for (size_t j = 0; j < instrProfHeadInfo.validLen && j < splitData.size(); j += biuSize) {
            BiuPerfInfo originInfo;
            if (memcpy_s(&originInfo, sizeof(originInfo), &splitData[j], sizeof(originInfo)) != EOK) {
                continue;
            }
            uint16_t high4Bit = (originInfo.biuInfo >> PIPE_STATE_BIT_OFFSET) & 0xf;
            // 高四位标识数据类型，高四位是0xf，则表示有效biu数据
            if (high4Bit == 0xf) {
                ParseOriginInfo(originInfo, instrProfHeadInfo, totalCyclesMap);
            }

            if ((high4Bit >= 0 && high4Bit <= 5) || high4Bit == 10) {
                GenMark(pipeMap_.at(high4Bit), originInfo, instrProfHeadInfo, totalCyclesMap);
            }
        }
    }
    return true;
}

void ParseTimeline::GenMark(const std::string &pipe, const BiuPerfInfo &originInfo,
                            const InstrProfHeadInfo &instrProfHeadInfo,
                            std::unordered_map<uint32_t, uint64_t> &totalCyclesMap)
{
    uint16_t markId = originInfo.biuInfo & 0xfff;
    uint32_t channelId = instrProfHeadInfo.coreId * 3 + instrProfHeadInfo.coreType;  // 3 表示2vec + 1cube
    if (channelId >= INSTR_PROF_CHANNEL_NUM) {
        Utility::LogDebug("InstrProf channelId %u exceeds limit %d", channelId, INSTR_PROF_CHANNEL_NUM);
        return;
    }
    UpdateEndMarks(markId, channelId);
    totalCyclesMap[channelId] += originInfo.cycles;
    std::string coreName = "core" + std::to_string(instrProfHeadInfo.coreId)
        + "." + subCore_[instrProfHeadInfo.coreType];
    std::string markName = "MarkStamp" + std::to_string(markId);
    timelineVec_[channelId].emplace_back(TimelineInfo(pipe, coreName, markName, totalCyclesMap[channelId], 1));
    return;
}

// 刷新endMark表
// instrprof_end中插入了连续的8个标签，需要按顺序过滤这些多余的end标记
void ParseTimeline::UpdateEndMarks(uint16_t markId, uint32_t channelId)
{
    static const std::array<uint16_t, 8> endMarkSequence = {
        0x888, 0x999, 0xaaa, 0xbbb, 0xccc, 0xddd, 0xeee, 0xfff
    };

    auto it = std::find(endMarkSequence.begin(), endMarkSequence.end(), markId);
    if (it == endMarkSequence.end()) {
        // markId不在end标记序列中，说明是用户的mark，删除这条通道上EndMark
        endMarks_.erase(channelId);
        return;
    }

    uint64_t expectedIdx = std::distance(endMarkSequence.begin(), it);
    if (expectedIdx == 0) {
        // 序列起始标记，记录起始位置（该channel在timelineVec_中的索引）
        endMarks_[channelId] = timelineVec_[channelId].size();
    } else if (endMarks_.find(channelId) == endMarks_.end()) {
        // 顺序不连续，删除这条通道上EndMark
        endMarks_.erase(channelId);
    }
    // 后续标记（expectedIdx > 0）且已存在起点，无需额外操作
}

void ParseTimeline::ParseOriginInfo(const BiuPerfInfo &originInfo, const InstrProfHeadInfo &instrProfHeadInfo,
                                    std::unordered_map<uint32_t, uint64_t> &totalCyclesMap)
{
    if (instrProfHeadInfo.coreType >= subCore_.size()) {
        Utility::LogDebug("InstrProf core type is %d, the limit is 2", instrProfHeadInfo.coreType);
        return;
    }
    uint16_t pipeInfo = originInfo.biuInfo & 0xfff;
    size_t pipeId = 0;
    uint32_t channelId = instrProfHeadInfo.coreId * 3 + instrProfHeadInfo.coreType;  // 3 表示2vec + 1cube
    if (channelId >= INSTR_PROF_CHANNEL_NUM) {
        Utility::LogDebug("InstrProf channelId %u exceeds limit %d", channelId, INSTR_PROF_CHANNEL_NUM);
        return;
    }
    totalCyclesMap[channelId] += originInfo.cycles;
    while (channelId < INSTR_PROF_CHANNEL_NUM && pipeId < TIMELINE_PIPE_NUM && pipeId < pipe_.size()) {
        // pipeInfo & 0x1 获取当前pipe状态，0 表示 idle，1 表示 busy
        // 如果当前状态为0，且当前流水状态表中start不是0，则说明这是一条完整 timeline
        if ((pipeInfo & 0x1) == 0) {
            if (timelineStatesVec_[channelId][pipeId].start != 0) {
                timelineStatesVec_[channelId][pipeId].duration =
                    totalCyclesMap[channelId] - timelineStatesVec_[channelId][pipeId].start;
                timelineVec_[channelId].emplace_back(timelineStatesVec_[channelId][pipeId]);
                timelineStatesVec_[channelId][pipeId] = TimelineInfo();
            }
            pipeId++;
            pipeInfo = pipeInfo >> 1;
            continue;
        }
        // 如果当前状态表中 start 为0，说明首次 active
        if (timelineStatesVec_[channelId][pipeId].start == 0) {
            timelineStatesVec_[channelId][pipeId].start = totalCyclesMap[channelId];
            timelineStatesVec_[channelId][pipeId].pipeName = pipe_[pipeId];
            timelineStatesVec_[channelId][pipeId].lineName = pipe_[pipeId];
            std::string coreName = "core" + std::to_string(instrProfHeadInfo.coreId)
                + "." + subCore_[instrProfHeadInfo.coreType];
            timelineStatesVec_[channelId][pipeId].coreName = coreName;
        }
        pipeId++;
        pipeInfo = pipeInfo >> 1;
    }
    return;
}

}
