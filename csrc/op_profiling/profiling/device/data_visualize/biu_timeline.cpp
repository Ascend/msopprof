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

#include "biu_timeline.h"
#include <algorithm>
#include "log.h"
#include "filesystem.h"
#include "common/defs.h"
#include "common/hal_helper.h"


using namespace Utility;

namespace Visualize {
bool BiuTimeline::TimelineToJson(const std::string &outputPath)
{
    int64_t aicoreFreq = 0;
    if (!Common::HalHelper::Instance().GetAicoreFreq(aicoreFreq)) {
        LogWarn("Get task scheduler frequency failed. Use Default value instead.");
        aicoreFreq = FREQ_DEFAULT;
    }
    outputPath_ = outputPath;
    if (!ParseBiuTimeStamps()) {
        return false;
    }
    std::vector<nlohmann::json> traceEvents;
    for (const auto& channelVec : timelineVec_) {
        for (const auto& timeline : channelVec) {
            nlohmann::json singleJson;
            singleJson["name"] = timeline.lineName;
            singleJson["cname"] = cnames_[timeline.pipeName];
            singleJson["ph"] = "X";
            singleJson["ts"] = static_cast<float>(timeline.start) / aicoreFreq;
            singleJson["dur"] = static_cast<float>(timeline.duration) / aicoreFreq;
            singleJson["pid"] = timeline.coreName;
            singleJson["tid"] = timeline.pipeName;
            for (const auto& arg : timeline.args) {
                singleJson["args"][arg.first] = arg.second;
            }
            traceEvents.emplace_back(singleJson);
        }
    }
    if (traceEvents.empty()) {
        return false;
    }
    PrintMissData();
    timelineJson_["profilingType"] = "op-biuperf";
    timelineJson_["displayTimeUnit"] = "ns";
    timelineJson_["schemaVersion"] = 1;
    timelineJson_["traceEvents"] = traceEvents;
    std::string tracePath = JoinPath({outputPath_, "trace.json"});
    if (!WriteFileByStream(tracePath, timelineJson_)) {
        LogWarn("Generate %s failed.", tracePath.c_str());
    }
    return true;
}

bool BiuTimeline::ParseBiuTimeStamps()
{
    bool ret = false;
    std::string dumpPath = JoinPath({outputPath_, "dump"});
    std::vector<std::string> filenames;
    if (!ListDir(dumpPath, std::back_inserter(filenames))) {
        return false;
    }
    for (auto &file : filenames) {
        if (StartsWith(file, "timeline.bin")) {
            std::string filePath = JoinPath({dumpPath, file});
            ret |= ParseSingleBiuTimeStamps(filePath);
        }
    }
    // 解析完所有timeline.bin后，一次性截断各channel中endMark起点之后的所有点
    for (const auto &it : channelEndMarkMap_) {
        uint32_t channelId = it.first;
        const EndMarkState &state = it.second;
        auto &channelVec = timelineVec_[channelId];
        if (state.startIndex < channelVec.size()) {
            channelVec.erase(channelVec.begin() + state.startIndex, channelVec.end());
        }
    }
    return ret;
}

bool BiuTimeline::ParseSingleBiuTimeStamps(const std::string &filePath) {
    // 解析timeline.bin.X
    if (!IsReadable(filePath)) {
        LogWarn("File %s is not exists or not readable.", filePath.c_str());
        return false;
    }
    size_t fileSize = GetFileSize(filePath);
    std::vector<char> totalBin(fileSize);
    size_t headSize = sizeof(InstrProfHeadInfo);
    size_t structSize = headSize + DATA_BUFFER_SIZE;
    if (!ReadBinFileByMultiStruct(filePath, fileSize, structSize, totalBin)) {
        LogError("File %s failed to be read.", filePath.c_str());
        return false;
    }
    for (size_t i = 0; i < fileSize; i += structSize) {
        InstrProfHeadInfo instrProfHeadInfo;
        if (memcpy_s(&instrProfHeadInfo, headSize, &totalBin[i], headSize) != EOK) {
            continue;
        }
        if (instrProfHeadInfo.coreType >= subCore_.size()) {
            LogDebug("InstrProf core type is %d, the limit is 2", instrProfHeadInfo.coreType);
            continue;
        }
        std::string coreName = "core" + std::to_string(instrProfHeadInfo.coreId) + "." + subCore_[instrProfHeadInfo.coreType];
        uint32_t channelId = instrProfHeadInfo.coreId * subCore_.size() + instrProfHeadInfo.coreType;
        if (channelId >= INSTR_PROF_CHANNEL_NUM) {
            LogDebug("InstrProf channelId %u exceeds limit %d", channelId, INSTR_PROF_CHANNEL_NUM);
            continue;
        }

        std::vector<char> splitData{&totalBin[i] + headSize, &totalBin[i] + structSize};
        size_t maxSize = std::min<size_t>(instrProfHeadInfo.validLen, splitData.size());
        for (size_t j = 0; j + sizeof(BiuPerfInfo) <= maxSize; j += sizeof(BiuPerfInfo)) {
            BiuPerfInfo biuData;
            if (memcpy_s(&biuData, sizeof(biuData), &splitData[j], sizeof(biuData)) != EOK) {
                continue;
            }
            // 高4位ctrl标识数据类型，0b1111:pipe状态变化，0b0000至0b0101、0b1010:dfx-region打点;低12位表示dfx-region id或pipe状态（空闲/繁忙）
            uint16_t ctrl = (biuData.biuInfo >> STATE_BIT_OFFSET) & 0xf;
            uint16_t state = biuData.biuInfo & 0xfff;
            if (ctrl == 0b1111) {
                channelCycleMap_[channelId] += biuData.cycles;
                ParsePipeState(state, channelId, coreName);
            } else if ((ctrl >= 0b0000 && ctrl <= 0b0101) || ctrl == 0b1010) {
                channelCycleMap_[channelId] += biuData.cycles;
                UpdateEndMarks(state, channelId);
                ParseDfxRegion(state, channelId, coreName, dfxPipeMap_.at(ctrl));
            }
        }
    }
    return true;
}

// 刷新endMark表，instrprof_end中插入了连续的8个标签，需要按顺序过滤这些多余的end标记
void BiuTimeline::UpdateEndMarks(uint16_t dfxRegionId, uint32_t channelId) {
    auto it = std::find(endMarkSequence_.begin(), endMarkSequence_.end(), dfxRegionId);
    if (it == endMarkSequence_.end()) {
        // markId不在end标记序列中，说明仍是用户或指令打点的mark，删除这条通道上EndMark
        channelEndMarkMap_.erase(channelId);
        return;
    }
    if (dfxRegionId == endMarkSequence_.front()) {
        // workaround连续打点序列起始标记，开始一轮新的顺序匹配，记录起始位置（cycle）
        EndMarkState state;
        state.startIndex = timelineVec_[channelId].size();
        state.nextExpectedIdx = 1;
        channelEndMarkMap_[channelId] = state;
        return;
    }
    auto stateIt = channelEndMarkMap_.find(channelId);
    if (stateIt == channelEndMarkMap_.end()) {
        return;
    }
    if (stateIt->second.nextExpectedIdx == endMarkSequence_.size()) {
        return;
    }
    size_t currentIdx = static_cast<size_t>(std::distance(endMarkSequence_.begin(), it));
    if (stateIt->second.nextExpectedIdx != currentIdx) {
        // 打点序列不连续，mark记录作废
        channelEndMarkMap_.erase(stateIt);
        return;
    }
    stateIt->second.nextExpectedIdx++;
}

void PipeBiuTimeline::ParseDfxRegion(uint16_t dfxRegionId, uint32_t channelId, const std::string &coreName, const std::string &pipe) {
    std::string markName = "MarkStamp" + std::to_string(dfxRegionId);
    timelineVec_[channelId].emplace_back(BiuTimelineInfo(pipe, coreName, markName, channelCycleMap_[channelId], 1));
}

void PipeBiuTimeline::ParsePipeState(uint16_t pipeMask, uint32_t channelId, const std::string &coreName) {
    size_t pipeId = 0;
    while (pipeId < statePipe_.size()) {
        // pipeMask & 0x1 获取当前pipe状态，0 表示 idle，1 表示 busy
        // 如果当前状态为0，且当前流水状态表中start不是0，则说明这是一条完整timeline
        if ((pipeMask & 0x1) == 0) {
            if (pipeStateVec_[channelId][pipeId].start != 0) {
                pipeStateVec_[channelId][pipeId].duration =
                    channelCycleMap_[channelId] - pipeStateVec_[channelId][pipeId].start;
                timelineVec_[channelId].emplace_back(pipeStateVec_[channelId][pipeId]);
                pipeStateVec_[channelId][pipeId] = BiuTimelineInfo();
            }
            pipeId++;
            pipeMask = pipeMask >> 1;
            continue;
        }
        // 如果当前状态表中start为0，说明首次active
        if (pipeStateVec_[channelId][pipeId].start == 0) {
            pipeStateVec_[channelId][pipeId].start = channelCycleMap_[channelId];
            pipeStateVec_[channelId][pipeId].pipeName = statePipe_[pipeId];
            pipeStateVec_[channelId][pipeId].lineName = statePipe_[pipeId];
            pipeStateVec_[channelId][pipeId].coreName = coreName;
        }
        pipeId++;
        pipeMask = pipeMask >> 1;
    }
}

void InstrBiuTimeline::ParseDfxRegion(uint16_t dfxRegionId, uint32_t channelId, const std::string &coreName, const std::string &pipe) {
    // 不展示SCALAR PIPE
    if (pipe == SU_PIPE) {
        return;
    }
    // dfx-region指令级打点9-10位必须均为1
    if (((dfxRegionId >> 9) & 0x3) != 0x3) {
        return;
    }
    bool isStart = ((dfxRegionId >> 11) & 0x1) == 0; // dfx-region指令级打点11位:0-start，1-end
    uint16_t realId = dfxRegionId & 0x1ff; // 0~8位作为真正匹配id
    std::tuple<const std::string, uint32_t, uint16_t> key = {pipe, channelId, realId};
    if (isStart) {
        startCache_[key] = channelCycleMap_[channelId];
        return;
    }
    auto it = startCache_.find(key);
    if (it == startCache_.end()) {
        LogDebug("dfx end point [%d] miss start point, channelId [%lu], pipe [%s]", dfxRegionId, channelId, pipe.c_str());
        return;
    }
    uint64_t startCycle = it->second;
    if (startCycle < channelCycleMap_[channelId]) {
        uint16_t startId = realId | 0x600; // 9-10位填充1
        std::string instrName = "MarkStamp" + std::to_string(startId);
        std::map<std::string, std::string> args = {};
        auto infoIt = dfxRegionInfoMap_.find({pipe, realId});
        if (infoIt != dfxRegionInfoMap_.end()) {
            // 待补充调用栈解析
            auto dfxRegionInfo = infoIt->second;
            instrName = dfxRegionInfo.InstrName;
            args["PC"] = NumToHexString(dfxRegionInfo.pc, 8);
        }
        timelineVec_[channelId].emplace_back(BiuTimelineInfo(pipe, coreName, instrName, startCycle, channelCycleMap_[channelId] - startCycle, args));
        startCache_.erase(it);
    }
}

void InstrBiuTimeline::PrintMissData() {
    PrintPipeIdFull();
    for (const auto& start : startCache_) {
        auto key = start.first;
        uint16_t startId = std::get<2>(key) | 0x600; // 9-10位填充1
        LogDebug("dfx start point [%d] miss end point, channelId [%lu], pipe [%s]", startId, std::get<1>(key), std::get<0>(key).c_str());
    }
}

void InstrBiuTimeline::PrintPipeIdFull() {
    // dfx-region id达到512以上的pipe，提示给用户
    std::string dfxLogPath = JoinPath({outputPath_, "dump", "dfx_tune.log"});
    std::ifstream file(dfxLogPath);
    if (!file.is_open()) {
        LogError("Failed to open file %s", dfxLogPath.c_str());
        return;
    }
    std::set<std::string> fullPipes;
    std::regex pattern(R"(\[pipe=([a-zA-Z0-9]+)\])");
    std::string line;
    std::smatch match;
    size_t lineCount = 0;
    while (std::getline(file, line)) {
        if (lineCount >= UINT64_MAX) {
            break;
        }
        lineCount++;
        if (line.find("TUNE-ERROR:all dfx ids consumed") == std::string::npos) {
            continue;
        }
        if (std::regex_search(line, match, pattern) && Common::DfxPipe::IsValidDfxPipe(match[1].str())) {
            fullPipes.insert(match[1].str());
        }
    }
    file.close();
    RemoveAll(dfxLogPath);
    if (fullPipes.empty()) {
        LogDebug("InstrTimeline dfx id is enough.");
    } else {
        LogWarn("InstrTimeline of pipes[%s] is incomplete because these pipes have more than 512 instructions.",
            Join(fullPipes.begin(), fullPipes.end(), ", ").c_str());
    }
}
}
