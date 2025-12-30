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


#include "hotspotmap_visualizer.h"
#include <iostream>
#include "parse/data_visualizer/utility.h"

using namespace Utility;
using namespace Serialization;
namespace Profiling {
namespace Parse {

PluginErrorCode HotSpotMapVisualizer::Entry()
{
    auto dataMapPtr = dataCenter_.GetDbPtr<std::map<std::string, SimData>>();
    if (dataMapPtr == nullptr) {
        return PluginErrorCode::NONBLOCKING_ERROR;
    }
    std::vector<std::string> coreVec;
    for (auto &coreNameWithData: *dataMapPtr) {
        coreVec.emplace_back(coreNameWithData.first);
    }
    SimPcToCode simPcToCode(dataVisualizerConfig_, dataCenter_, coreVec);
    simPcToCode.CalCulate();
    std::vector<InstrInfo> instrInfoList = simPcToCode.GetInstrInfo();

    SimCodeToPc simCodeToPc(dataVisualizerConfig_, dataCenter_, coreVec);
    simCodeToPc.CalCulate();
    std::vector<CodeFile> files = simCodeToPc.GetCodeFile();

    CodeInstrData codeInstrData = {simPcToCode.GetStallCyc(), coreVec, files, instrInfoList};
    Serialization::ApiData::VisualizeApiData(dataVisualizerConfig_.GetOutputPath(), codeInstrData);
    PrintCoreInfo();
    return PluginErrorCode::SUCCESS;
}

void HotSpotMapVisualizer::PrintCoreInfo()
{
    auto simDataMapPtr = dataCenter_.GetDbPtr<std::map<std::string, SimData>>();
    if (simDataMapPtr == nullptr) {
        return;
    }
    for (auto &coreNameWithData: *simDataMapPtr) {
        std::string coreName = coreNameWithData.first;
        auto instrPtr = coreNameWithData.second.instrs;
        if (instrPtr == nullptr) {
            continue;
        }
        auto mergeInstrPtr = instrPtr->GetColumnData<MergeInfo>(Parse::InstrDetailTable::MERGE_INFO);
        if (mergeInstrPtr == nullptr) {
            continue;
        }
        ParseCoreInfo(coreName, *mergeInstrPtr);
    }
    // sorted by coreName
    sort(exeStatList_.begin(), exeStatList_.end(),
         [](CoreExe &coreExe1, CoreExe &coreExe2) { return coreExe1.coreName < coreExe2.coreName; });
    LogInfo("Core operator results run in simulator as follow:");
    ExeNames exeNames;
    std::vector<std::string> heads = exeNames.CoreHeads;
    int fieldWidth = 20; // 20: set width
    for (const std::string &head: heads) {
        std::cout << std::left << std::setw(fieldWidth) << std::setfill(' ') << head;
    }
    std::cout << "\n";
    for (const CoreExe &exeStat: exeStatList_) {
        std::cout << std::left << std::setw(fieldWidth) << std::setfill(' ') << exeStat.coreName;
        std::cout << std::left << std::setw(fieldWidth) << std::setfill(' ') << exeStat.durationTimeUs;
        std::cout << std::left << std::setw(fieldWidth) << std::setfill(' ') << exeStat.runningTime<< std::endl;
    }
}

bool HotSpotMapVisualizer::ParseCoreInfo(const std::string &coreName, std::vector<MergeInfo> instrs)
{
    uint64_t minStart = UINT64_MAX;
    uint64_t maxEnd = 0;
    for (const MergeInfo &instr: instrs) {
        if (instr.startTick < minStart || minStart == UINT64_MAX) {minStart = instr.startTick;}
        if (instr.endTick > maxEnd) {maxEnd = instr.endTick;}
    }
    if (minStart > maxEnd) {
        return false;
    }
    float runningTime = CalculateRunningTime(instrs, chipType_);
    float durationTime = GetMicrosecond(chipType_, (maxEnd - minStart), 2); // 2: Precise number of digits
    CoreExe coreExe = {coreName, durationTime, runningTime};
    exeStatList_.emplace_back(coreExe);
    return true;
}
}
}
