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

#include "aicore_timeline_parser.h"
#include "common/visualize.h"
#include "common/hal_helper.h"

namespace Visualize {
using namespace Utility;

uint16_t GetAicoreBlockIndexByType(const std::string &opType, const std::string &recordType, uint16_t subBlockNum, uint16_t blockIndex, uint16_t subBlockIndex) {
    if (opType == Common::OpType::VECTOR || opType == Common::OpType::CUBE) {
        return blockIndex;
    }
    if (recordType == AIC_BLOCK) {
        return blockIndex;
    }
     // for mix operator, index 0 is cube subblock, index 1/2 is vector subblock
    if (subBlockIndex == 0) {
        return blockIndex;
    }
    int vecNum = subBlockNum - 1; // cube subblock number is 1
    return blockIndex * vecNum + subBlockIndex - 1;
}

float GetRunTime(uint64_t freq, int64_t cycles)
{
    if (cycles < 0 || freq == 0) {
        LogDebug("There is some value wrong when calculating time");
        return 0;
    }
    return static_cast<float>(cycles) / freq * TIME_CONVERSION;
}

void AicoreTimelineParser::GenPc2Code(std::vector<MsprofAicTimeStampInfo> &aicoreTimeStamps)
{
    if (!aicoreTimeStamps.empty()) {
        std::set<uint64_t> pcSet = {};
        for (const auto &item : aicoreTimeStamps) {
            pcSet.emplace(item.curPc);
        }
        std::string dumpPath = JoinPath({outputPath_, "dump"});
        Profiling::ParsePcCode pc2Code(dumpPath, pcSet);
        pc2Code.Parse();
        pc2code_ = pc2Code.GetPc2Code();
    } else {
        LogDebug("Can not parse aic_timestamp.bin,no need to generate Aicore timeline");
    }
}

// 当前descId前16为为物理核，后16位逻辑核，物理核0~24为cube，25~74为vector
// 在旧版本中runtime传递的descId为物理核，为兼容老版本，通过解析所有记录中物理核是否包含非0值判断新旧版本
// 旧版本中使用原逻辑获取当前记录类型，新版本使用物理核值区分记录类型
void AicoreTimelineParser::GetTimeStampType(std::vector<MsprofAicTimeStampInfo> &aicoreTimeStamps, std::vector<std::string> &type)
{
    if (aicBlocKNum_ + aivBlockNum_ == 1) {
        std::string allType = aicBlocKNum_ == 1 ? AIC_BLOCK : AIV_BLOCK;
        for (uint32_t i = 0; i< aicoreTimeStamps.size(); i++) {
            type.emplace_back(allType);
        }
        return;
    }
    bool isOldVersion = true;
    for (const auto &item : aicoreTimeStamps) {
        uint16_t physicalId = static_cast<uint32_t>(item.blockId >> 16);
        if (physicalId != 0) {
            isOldVersion = false;
            break;
        }
    }
    if (isOldVersion) {
        for (const auto &item : aicoreTimeStamps) {
            type.emplace_back(GetAicoreTimeLinePid(item.blockId));
        }
        return;
    }
    for (auto &item : aicoreTimeStamps) {
        uint16_t physicalId = static_cast<uint32_t>(item.blockId >> 16);
        uint16_t logicalId = static_cast<uint32_t>(item.blockId & 0xFFFF);
        item.blockId = logicalId;
        if (physicalId < VEC_START) {
            type.emplace_back(AIC_BLOCK);
        } else {
            type.emplace_back(AIV_BLOCK);
        }
    }
}

bool AicoreTimelineParser::AicoreTimelineToJson(const std::string &outputPath)
{
    outputPath_ = outputPath;
    auto socVersion = opBasicInfoObj_->GetSoc();
    if (Common::SOC_910B.count(socVersion) == 0 && Common::SOC_910_93.count(socVersion) == 0) {
        LogDebug("Only A2 and A3 support aicore timeline");
        return false;
    }
    std::vector<MsprofAicTimeStampInfo> aicoreTimeStamps;
    std::vector<std::string> type;
    GetAicoreTimeStamps(aicoreTimeStamps, type);
    ProcessBlockDur(aicoreTimeStamps, type);
    GenPc2Code(aicoreTimeStamps);
    ProcessAicoreData(aicoreTimeStamps, type);
    aicoreTimelineJson_["profilingType"] = "op";
    aicoreTimelineJson_["displayTimeUnit"] = "ns";
    aicoreTimelineJson_["schemaVersion"] = 1;
    aicoreTimelineJson_["traceEvents"] = traceEvents_;

    std::string tracePath = JoinPath({outputPath, "trace.json"});
    if (!WriteFileByStream(tracePath, aicoreTimelineJson_)) {
        LogWarn("Generate %s failed", tracePath.c_str());
    }
    return true;
}

void AicoreTimelineParser::GetAicoreTimeStamps(std::vector<MsprofAicTimeStampInfo> &aicoreTimeStamps, std::vector<std::string> &type)
{
    std::string binPath = JoinPath({outputPath_, "dump/aic_timestamp.bin"});
    size_t fileSize = GetFileSize(binPath);
    size_t structSize = sizeof(MsprofAicTimeStampInfo);
    std::vector<char> binData;
    if (fileSize < structSize || !ReadBinFileByMultiStruct(binPath, fileSize, structSize, binData)) {
        return;
    }
    for (size_t i = 0; i < fileSize; i = i + structSize) {
        MsprofAicTimeStampInfo info {};
        if (memcpy_s(&info, structSize, &binData[i], structSize) != EOK) {
            continue;
        }
        if (info.syscyc < minSysCyc_) {
            continue;
        }
        aicoreTimeStamps.push_back(info);
    }
    GetTimeStampType(aicoreTimeStamps, type);
}

void AicoreTimelineParser::ProcessBlockDur(const std::vector<MsprofAicTimeStampInfo> &aicoreTimeStamps, std::vector<std::string> &type)
{
    std::set<uint16_t> aicDotBlockIds;
    std::set<uint16_t> aivDotBlockIds;
    std::unordered_map<std::string, OperationInfo> operationMap;
    for (uint32_t i = 0; i < aicoreTimeStamps.size(); i++) {
        auto &item = aicoreTimeStamps[i];
        if (type.size() <= i) {
            break;
        }
        if (type[i] == AIV_BLOCK) {
            aivDotBlockIds.insert(item.blockId);
        } else {
            aicDotBlockIds.insert(item.blockId);
        }
    }

    // add aicore block duration
    AddAicoreBlockDur(blockSystemTimes_, aicDotBlockIds, aivDotBlockIds);
    // sort aicore timeline
    auto OrderDotsId = [this] (const std::string &type, std::set<uint16_t> &dots) {
        for (const auto &dotBlockId: dots) {
                json nameItem;
                nameItem["ph"] = "M";
                nameItem["name"] = "thread_name";
                nameItem["pid"] = type;
                nameItem["tid"] = dotBlockId;
                nameItem["args"]["name"] = type + std::to_string(dotBlockId);
                traceEvents_.push_back(nameItem);
                json sortItem;
                sortItem["ph"] = "M";
                sortItem["name"] = "thread_sort_index";
                sortItem["pid"] = type + std::to_string(dotBlockId);
                sortItem["tid"] = dotBlockId;
                sortItem["args"]["sort_index"] = SORT_BIAS + dotBlockId;
                traceEvents_.push_back(sortItem);
        }
    };
    OrderDotsId(AIC_BLOCK, aicDotBlockIds);
    OrderDotsId(AIV_BLOCK, aivDotBlockIds);
}

void AicoreTimelineParser::ProcessAicoreData(const std::vector<MsprofAicTimeStampInfo> &aicoreTimeStamps, std::vector<std::string> &type)
{
    std::unordered_map<std::string, OperationInfo> operationMap;
    for (uint32_t i = 0; i < aicoreTimeStamps.size(); i++) {
        MsprofAicTimeStampInfo item = aicoreTimeStamps.at(i);
        if (AICORE_DOT_MAP.find(item.descId) != AICORE_DOT_MAP.end() || item.descId < TIME_STAMP_START || type.size() < i) {
            continue;
        }
        std::tuple<uint32_t, uint32_t, std::string> key {item.blockId, item.descId, type[i]};
        if (timeStampInfo_[key].empty() || timeStampInfo_[key].back().endFound) {
            OperationInfo info;
            info.startFound = true;
            info.blockId = item.blockId;
            info.startSyscyc = item.syscyc;
            info.endFound = false;
            info.startCurPc = item.curPc;
            timeStampInfo_[key].emplace_back(info);
        } else {
            timeStampInfo_[key].back().endFound = true;
            timeStampInfo_[key].back().endSyscyc = item.syscyc;
        }
    }
    std::string codeAcc;
    for (const auto &infos : timeStampInfo_) {
        uint32_t blockId = std::get<0>(infos.first);
        auto pid = std::get<2>(infos.first);
        for (const auto &dot : infos.second) {
            if (!dot.endFound || dot.endSyscyc <= dot.startSyscyc || dot.startSyscyc < minSysCyc_) {
                continue;
            }
            json resultItem;
            if (blockDuration_[{pid, blockId}].first > dot.startSyscyc || blockDuration_[{pid, blockId}].second < dot.endSyscyc) {
                continue;
            }
            resultItem["ph"] = "X";
            resultItem["name"] = std::to_string(std::get<1>(infos.first));
            resultItem["pid"] = pid;
            resultItem["tid"] = blockId;
            resultItem["args"]["name"] = std::get<1>(infos.first);
            resultItem["ts"] = GetRunTime(aicpuFreq_, dot.startSyscyc - minSysCyc_);
            resultItem["dur"] = GetRunTime(aicpuFreq_, dot.endSyscyc - dot.startSyscyc);

            if (pc2code_.Find(dot.startCurPc)) {
                codeAcc = accumulate(pc2code_[dot.startCurPc].begin(),
                                    pc2code_[dot.startCurPc].end(),
                                    std::string(),
                                    [](std::string acc, const std::string &s) {
                                        return acc.empty() ? s : acc + "\n" + s;
                                    });
                resultItem["args"]["code"] = codeAcc;
                resultItem["args"]["pc_addr"] = dot.startCurPc;
            } else {
                resultItem["args"]["code"] = "";
                resultItem["args"]["pc_addr"] = "";
            }
            traceEvents_.push_back(resultItem);
        }
    }
}

void AicoreTimelineParser::AddAicoreBlockDur(const BlockSystemTimeType &blockSystemTimes, std::set<uint16_t> &aicDotBlockIds, std::set<uint16_t> &aivDotBlockIds)
{
    if (!Common::HalHelper::Instance().GetTaskSchedulerFreq(aicpuFreq_)) {
        LogWarn("Get task scheduler frequency failed. Use default value instead");
        aicpuFreq_ = FREQ;
    }
    std::string opType = opBasicInfoObj_->GetOpType();
    std::string recordType = AIV_BLOCK;
    std::string location = "AICore Dur";
    std::string cName = std::string(VISUALIZE_COLOR_NAME::GREEN);
    if (aicpuFreq_ == 0) {
        return;
    }
    if (opType == Common::OpType::CUBE) {
        cName = std::string(VISUALIZE_COLOR_NAME::GRASS_GREEN);
        recordType = AIC_BLOCK;
    }
    for (const auto &pair : blockSystemTimes) {
        uint16_t blockIndex = pair.first;
        auto timeVec = pair.second;
        uint16_t subBlockNum = timeVec.size();
        for (uint16_t i = 0; i < subBlockNum; i++) {
            json resultItem;
            if (opType == Common::OpType::MIX) {
                // blockSystemTimes中记录的第1个是cube
                cName = i == 0 ? std::string(VISUALIZE_COLOR_NAME::GRASS_GREEN) : std::string(VISUALIZE_COLOR_NAME::GREEN);
                recordType = i == 0 ? AIC_BLOCK : AIV_BLOCK;
            }
            auto dots = GetAicoreBlockIndexByType(opType, recordType, subBlockNum, blockIndex, i);
            if (recordType == AIC_BLOCK) {
                aicDotBlockIds.insert(dots);
            } else {
                aivDotBlockIds.insert(dots);
            }
            blockDuration_[{recordType, dots}] = {timeVec[i].first, timeVec[i].second};
            resultItem["cname"] = cName;
            resultItem["dur"] = GetRunTime(aicpuFreq_, SafeSub(timeVec[i].second, timeVec[i].first, location, false));
            resultItem["name"] = recordType + std::to_string(dots);
            resultItem["ph"] = "X";
            resultItem["pid"] = recordType;
            resultItem["tid"] = dots;
            resultItem["ts"] = GetRunTime(aicpuFreq_, SafeSub(timeVec[i].first, minSysCyc_, location, false));
            traceEvents_.push_back(resultItem);

        }
    }
}

}

