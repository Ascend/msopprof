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

void AicoreTimelineParser::GenPc2Code(std::vector<MsprofAicTimeStampInfoUpdate> &aicoreTimeStamps)
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
// A5上0~17,54~71是cube,18~53,72~108是vector的物理核
void AicoreTimelineParser::GetTimeStampType(std::vector<MsprofAicTimeStampInfo> &infos, std::vector<MsprofAicTimeStampInfoUpdate> &aicoreTimeStamps)
{
    bool isOldVersion = true;
    for (const auto &item : infos) {
        uint16_t physicalId = static_cast<uint32_t>(item.blockId >> 16);
        if (physicalId != 0) {
            isOldVersion = false;
            break;
        }
    }
    if (aicBlocKNum_ == 1 && aivBlockNum_ == 0) {
        for (const auto &item : infos) {
            if (item.blockId <= CUBE_BLOCK_START_INDEX) {
                isOldVersion = false;
                break;
            }
        }
    }
    if (isOldVersion) {
        for (auto &item : infos) {
            std::string type = GetAicoreTimeLinePid(item.blockId);
            if (item.blockId >= CUBE_BLOCK_START_INDEX) {
                item.blockId -= CUBE_BLOCK_START_INDEX;
            }
            aicoreTimeStamps.emplace_back(MsprofAicTimeStampInfoUpdate(item, type));
        }
        return;
    }
    for (auto &item : infos) {
        uint16_t physicalId = static_cast<uint32_t>(item.blockId >> 16);
        uint16_t logicalId = static_cast<uint32_t>(item.blockId & 0xFFFF);
        item.blockId = logicalId;
        if (chipSeries_ == ChipProductType::ASCEND950_SERIES) {
            std::string type = (physicalId < A5_VEC_START_RANGE_ONE) || (physicalId > A5_VEC_END_RANGE_ONE && physicalId < A5_VEC_START_RANGE_TWO) ? AIC_BLOCK : AIV_BLOCK;
            aicoreTimeStamps.emplace_back(MsprofAicTimeStampInfoUpdate(item, type));
            continue;
        }
        std::string type = physicalId < A2_VEC_START ? AIC_BLOCK : AIV_BLOCK;
        aicoreTimeStamps.emplace_back(MsprofAicTimeStampInfoUpdate(item, type));
    }
}

bool AicoreTimelineParser::TimelineToJson(const std::string &outputPath)
{
    ChipProductType chipType = GetChipTypeSeries();
    if (chipType != ChipProductType::ASCEND910B_SERIES && chipType != ChipProductType::ASCEND910_93_SERIES
        && chipType != ChipProductType::ASCEND950_SERIES) {
        return false;
    }
    outputPath_ = outputPath;
    std::vector<MsprofAicTimeStampInfoUpdate> aicoreTimeStamps;
    if (!GetAicoreTimeStamps(aicoreTimeStamps)) {
        return false;
    }
    ProcessAicoreBlockDur();
    AddAicoreDuration(minSysCyc_);
    ProcessAicoreData(aicoreTimeStamps);
    json result;
    result["profilingType"] = "op";
    result["displayTimeUnit"] = "ns";
    result["schemaVersion"] = 1;
    result["traceEvents"] = timelineJson_;
    timelineJson_ = result;
    std::string tracePath = JoinPath({outputPath_, "trace.json"});
    if (!WriteFileByStream(tracePath, timelineJson_)) {
        LogWarn("Generate %s failed", tracePath.c_str());
    }
    return true;
}

bool AicoreTimelineParser::GetAicoreTimeStamps(std::vector<MsprofAicTimeStampInfoUpdate> &aicoreTimeStamps)
{
    chipSeries_ = GetChipTypeSeries();
    std::vector<MsprofAicTimeStampInfo> infos;
    if (!GetTimeStamp(infos)) {
        return false;
    }
    GetTimeStampType(infos, aicoreTimeStamps);
    GenPc2Code(aicoreTimeStamps);
    return true;
}

void AicoreTimelineParser::ProcessAicoreData(const std::vector<MsprofAicTimeStampInfoUpdate> &aicoreTimeStamps)
{
    std::unordered_map<std::string, OperationInfo> operationMap;
    for (uint32_t i = 0; i < aicoreTimeStamps.size(); i++) {
        MsprofAicTimeStampInfoUpdate item = aicoreTimeStamps.at(i);
        if (item.descId < TIME_STAMP_START) {
            continue;
        }
        // 通过blockID + type + descId唯一锁定1条记录作为key，匹配更新结束时间
        std::tuple<uint32_t, uint32_t, std::string> key {item.blockId, item.descId, item.type};
        if (timeStampInfo_[key].empty() || timeStampInfo_[key].back().endFound) {
            OperationInfo info;
            info.startFound = true;
            info.blockId = item.blockId;
            info.startSyscyc = item.syscyc;
            info.endFound = false;
            info.startCurPc = item.curPc;
            info.type = item.type;
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
            timelineJson_.push_back(resultItem);
        }
    }
}

}

