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
#include "common/visualize.h"

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
    ParseDfxMapInfo();
    if (!ParseBiuTimeStamps()) {
        return false;
    }
    std::vector<nlohmann::json> traceEvents;
    for (const auto& channelVec : timelineVec_) {
        for (const auto& timeline : channelVec) {
            nlohmann::json singleJson;
            singleJson["name"] = timeline.lineName;
            singleJson["cname"] = timeline.cName.empty() ? cnames_[timeline.pipeName] : timeline.cName;
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
        LogDebug("Biu timeline is empty.");
        return false;
    }
    PrintMissData();
    nlohmann::json result;
    result["profilingType"] = "op";
    result["displayTimeUnit"] = "ns";
    result["schemaVersion"] = 1;
    result["traceEvents"] = traceEvents;
    timelineJson_ = result;
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
    // 指令起始
    std::tuple<const std::string, uint32_t, uint16_t> key = {pipe, channelId, dfxRegionId};
    auto it = startCache_.find(key);
    if (it == startCache_.end()) {
        startCache_[key] = channelCycleMap_[channelId];
        return;
    }
    // 指令结束
    uint64_t startCycle = it->second;
    if (startCycle < channelCycleMap_[channelId]) {
        std::string instrName = "Instr" + std::to_string(dfxRegionId);
        std::string cName = "";
        std::map<std::string, std::string> args = {};
        auto infoIt = dfxRegionInfoMap_.find({pipe, dfxRegionId});
        if (infoIt != dfxRegionInfoMap_.end()) {
            auto dfxRegionInfo = infoIt->second;
            instrName = dfxRegionInfo.InstrName;
            cName = TOTAL_CNAME_MAP[GetInstrNameId(instrName) % TOTAL_CNAME_MAP.size()];
            args["pc_addr"] = NumToHexString(dfxRegionInfo.pc, 8);
            std::string codeAcc;
            if (pc2code_.Find(dfxRegionInfo.pc)) {
                codeAcc =
                    accumulate(pc2code_[dfxRegionInfo.pc].begin(), pc2code_[dfxRegionInfo.pc].end(), std::string(),
                        [](std::string acc, const std::string &s) { return acc.empty() ? s : acc + "\n" + s; });
                args["code"] = codeAcc;
            } else {
                args["code"] = "";
            }
        }
        timelineVec_[channelId].emplace_back(BiuTimelineInfo(pipe, coreName, instrName, startCycle, channelCycleMap_[channelId] - startCycle, cName, args));
        startCache_.erase(it);
    }
}

void InstrBiuTimeline::PrintMissData() {
    PrintPipeIdFull();
    // 按channel和pipe聚合打印未配对的dfx region
    std::map<std::pair<std::string, uint32_t>, std::vector<std::string>> groupedMissData;
    for (const auto& start : startCache_) {
        auto key = start.first;
        std::string dfxRegionIdStr = std::to_string(std::get<2>(key));
        groupedMissData[{std::get<0>(key), std::get<1>(key)}].emplace_back(dfxRegionIdStr);
    }
    for (const auto &data : groupedMissData) {
        auto dfxRegionIds = data.second;
        if (dfxRegionIds.empty()) {
            continue;
        }
        size_t total = dfxRegionIds.size();
        for (size_t i = 0; i < total; i += missBatchSize_) {
            size_t endIdx = std::min(i + missBatchSize_, total);
            std::string regionStr = Join(dfxRegionIds.begin() + i, dfxRegionIds.begin() + endIdx, ",");
            LogDebug("dfx point [%s] miss, channelId [%u], pipe [%s]", regionStr.c_str(), data.first.second,
                data.first.first.c_str());
        }
    }
}

void InstrBiuTimeline::PrintPipeIdFull() {
    // dfx-region id达到1024以上的pipe，提示给用户
    std::string dfxLogPath = JoinPath({outputPath_, "dump", "dfx_tune.log"});
    std::ifstream file(dfxLogPath);
    if (!file.is_open()) {
        LogWarn("Failed to open file %s", dfxLogPath.c_str());
        return;
    }
    std::set<std::string> fullPipes;
    std::string line;
    std::smatch match;
    size_t lineCount = 0;
    static const std::regex pattern(R"(\[pipe=([a-zA-Z0-9]{4,6})\])");
    while (std::getline(file, line)) {
        if (lineCount >= UINT64_MAX) {
            break;
        }
        lineCount++;
        if (line.find("TUNE-ERROR: all dfx ids consumed") == std::string::npos) {
            continue;
        }
        if (std::regex_search(line, match, pattern) && Common::DfxPipe::IsValidDfxPipe(match[1].str())) {
            fullPipes.insert(match[1].str());
        }
    }
    file.close();
    if (Utility::Log::GetLog().GetLogLv() > Utility::LogLv::DEBUG) {
        RemoveAll(dfxLogPath);
    }
    if (fullPipes.empty()) {
        LogDebug("InstrTimeline dfx id is enough.");
    } else {
        LogWarn("InstrTimeline of pipes[%s] is incomplete because these pipes have more than 1024 instructions.",
            Join(fullPipes.begin(), fullPipes.end(), ", ").c_str());
    }
}

void InstrBiuTimeline::ParseDfxMapInfo() {
    // 解析region_id、pipe、偏移地址、指令名称等信息
    std::string dumpPath = JoinPath({outputPath_, "dump"});
    std::string dfxMapPath = JoinPath({dumpPath, "dfx_region_map.txt"});
    std::ifstream file(dfxMapPath);
    if (!file.is_open()) {
        LogWarn("Failed to open file %s", dfxMapPath.c_str());
        return;
    }

    std::smatch match;
    std::string line;
    size_t lineCount = 0;
    std::set<uint64_t> pcSet = {};
    Profiling::ParsePcCode pc2Code(dumpPath, pcSet);
    uint64_t startPc = pc2Code.GetStartPc();
    static const std::regex dfxInfoPattern(
        R"(region_id=(\d{1,4}),pipe=([a-zA-Z0-9]{4,6}),.*?,pc=(0[xX][a-fA-F0-9]{1,8}),opcode=([a-zA-Z0-9_-]+))");
    while (std::getline(file, line)) {
        if (lineCount >= UINT64_MAX) {
            break;
        }
        lineCount++;
        if (!std::regex_search(line, match, dfxInfoPattern)) {
            continue;
        }
        uint16_t regionId;
        StringToNum<uint16_t>(match[1].str(), regionId);
        if (!Common::DfxPipe::IsValidDfxPipe(match[2].str())) {
            continue;
        }
        std::string pipe = Utility::ToUpper(match[2].str());
        uint64_t offsetPc = 0;
        StoullConverter(match[3].str(), offsetPc, RADIX_16);
        DfxRegionInfo info;
        info.pc = pc2Code.AddStartPc2Offset(startPc, offsetPc);
        info.InstrName = match[4].str();
        dfxRegionInfoMap_[{pipe, regionId}] = info;
        pcSet.insert(info.pc);
    }

    file.close();
    pc2Code.SetPcSet(pcSet);
    pc2Code.Parse();
    pc2code_ = pc2Code.GetPc2Code();
    if (Utility::Log::GetLog().GetLogLv() > Utility::LogLv::DEBUG) {
        RemoveAll(dfxMapPath);
    }
}
}
