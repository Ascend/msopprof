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


#include "sim_pc_to_code.h"
#include <algorithm>
#include "log.h"
#include "number_operation.h"
#include "parse/data_visualizer/utility.h"
#include "filesystem.h"
#include "profiling/simulator/data_parse/api_data.h"
#include "parse/data_visualizer/sim_visualizer_config.h"
#include "thread_pool.h"

using namespace Utility;
using namespace Serialization;
namespace Profiling {
namespace Parse {

std::map<uint64_t, std::vector<MergeInfo>> SimPcToCode::MergeInstr(const std::vector<MergeInfo> &instrs) const
{
    std::map <uint64_t, std::vector<MergeInfo>> pcMap;
    for (const auto &instr: instrs) {
        pcMap[instr.pc].emplace_back(instr);
    }
    return pcMap;
}

void SimPcToCode::UpdateInstrDetail(std::vector<MergeInfo> &instrList, MergeInfo &selectedInstr) const
{
    if (instrList.size() <= 1) {
        return ;
    }
    for (size_t i = 1; i < instrList.size(); i++) {
        selectedInstr.gprCount = std::max(selectedInstr.gprCount, instrList[i].gprCount);
        if (selectedInstr.processBytes > 0) {
            selectedInstr.processBytes += instrList[i].processBytes;
        }
        selectedInstr.vecUtilization = std::min(selectedInstr.vecUtilization, instrList[i].vecUtilization);
        selectedInstr.ubReadConflict = std::max(selectedInstr.ubReadConflict, instrList[i].ubReadConflict);
        selectedInstr.ubWriteConflict = std::max(selectedInstr.ubWriteConflict, instrList[i].ubWriteConflict);
    }
}

void SimPcToCode::InitNewInstrInfo(InstrInfo &instrInfo)
{
    instrInfo.cceCode = "";
    instrInfo.gprCount.resize(cores_.size(), 0);
    instrInfo.processBytes.resize(cores_.size(), -1);
    instrInfo.vecUtilization.resize(cores_.size(), -1);
    instrInfo.ubReadConflict.resize(cores_.size(), -1);
    instrInfo.ubWriteConflict.resize(cores_.size(), -1);
    instrInfo.cycles.resize(cores_.size(), 0);
    instrInfo.callCount.resize(cores_.size(), 0);
    instrInfo.realStallCyc.resize(cores_.size(), 0);
    instrInfo.iCacheCyc.resize(cores_.size(), 0);
    instrInfo.ccuCyc.resize(cores_.size(), 0);
    instrInfo.scalarCyc.resize(cores_.size(), 0);
}

void SimPcToCode::UpdatePCStat(const MergeInfo &selectedInstr, const std::string &coreName, const CycleInfo &cycleInfo,
    int callCount)
{
    auto coreIndex = distance(cores_.begin(), find(cores_.begin(), cores_.end(), coreName));
    if (static_cast<size_t>(coreIndex) >= cores_.size()) {
        Utility::LogWarn("Core index error, core name:%s", coreName.c_str());
        return;
    }
    // 多线程操作instrInfoMap_第一次创建数据可能因为key存在失败，需要另外处理
    bool res = false;
    if (instrInfoMap_.Count(selectedInstr.pc) == 0) {
        InstrInfo instrInfo;
        InitNewInstrInfo(instrInfo);
        instrInfo.addr = GetPc2String(selectedInstr.pc);
        instrInfo.instr = selectedInstr.name + " " + selectedInstr.detail;
        instrInfo.pipe = selectedInstr.pipe;
        instrInfo.cceCode = GetAscendCInnerCode(selectedInstr.pc);
        instrInfo.gprCount[coreIndex] = selectedInstr.gprCount;
        if (selectedInstr.processBytes != 0) {
            // updateCode.processBytes=0 should be displayed as NA
            instrInfo.processBytes[coreIndex] = selectedInstr.processBytes;
        }
        instrInfo.vecUtilization[coreIndex] = selectedInstr.vecUtilization;
        instrInfo.ubReadConflict[coreIndex] = selectedInstr.ubReadConflict;
        instrInfo.ubWriteConflict[coreIndex] = selectedInstr.ubWriteConflict;
        instrInfo.cycles[coreIndex] = cycleInfo.cycles;
        instrInfo.realStallCyc[coreIndex] = cycleInfo.realStallCyc;
        instrInfo.iCacheCyc[coreIndex] = cycleInfo.iCacheCyc;
        instrInfo.ccuCyc[coreIndex] = cycleInfo.ccuCyc;
        instrInfo.scalarCyc[coreIndex] = cycleInfo.scalarCyc;
        instrInfo.callCount[coreIndex] = callCount;
        res = instrInfoMap_.Insert(std::make_pair(selectedInstr.pc, instrInfo));
    }
    if (!res) {
        instrInfoMap_[selectedInstr.pc].gprCount[coreIndex] = selectedInstr.gprCount;
        if (selectedInstr.processBytes != 0) {
            // updateCode.processBytes=0 should be displayed as NA
            instrInfoMap_[selectedInstr.pc].processBytes[coreIndex] = selectedInstr.processBytes;
        }
        instrInfoMap_[selectedInstr.pc].vecUtilization[coreIndex] = selectedInstr.vecUtilization;
        instrInfoMap_[selectedInstr.pc].ubReadConflict[coreIndex] = selectedInstr.ubReadConflict;
        instrInfoMap_[selectedInstr.pc].ubWriteConflict[coreIndex] = selectedInstr.ubWriteConflict;
        instrInfoMap_[selectedInstr.pc].cycles[coreIndex] = cycleInfo.cycles;
        instrInfoMap_[selectedInstr.pc].realStallCyc[coreIndex] = cycleInfo.realStallCyc;
        instrInfoMap_[selectedInstr.pc].iCacheCyc[coreIndex] = cycleInfo.iCacheCyc;
        instrInfoMap_[selectedInstr.pc].ccuCyc[coreIndex] = cycleInfo.ccuCyc;
        instrInfoMap_[selectedInstr.pc].scalarCyc[coreIndex] = cycleInfo.scalarCyc;
        instrInfoMap_[selectedInstr.pc].callCount[coreIndex] = callCount;
    }
}


void SimPcToCode::CalCulate()
{
    auto simDataMapPtr = dataCenter_.GetDbPtr<std::map<std::string, SimData>>();
    if (simDataMapPtr == nullptr) {
        return;
    }
    ThreadPool pool(simConfig_.GetThreads());
    pool.Start();
    for (auto &coreNameWithData: *simDataMapPtr) {
        pool.AddTask([this, &coreNameWithData]() {
            Statistic(coreNameWithData.first, coreNameWithData.second);
        });
    }
    pool.WaitAllTasks();
    pool.Stop();
}

void SimPcToCode::CalCycles(const std::vector<MergeInfo> &instrList, Serialization::CycleInfo &cycleInfo)
{
    std::string location = "cycles";
    for (auto &instr: instrList) {
        int temp = static_cast<int>(SafeSub(instr.endTick, instr.startTick, location, false));
        cycleInfo.cycles = SafeAdd(cycleInfo.cycles, temp, location);
        cycleInfo.realStallCyc = SafeAdd(cycleInfo.realStallCyc, static_cast<int>(instr.realStallCyc), location);
        if (instr.icacheTick != UINT64_MAX) {
            uint64_t iCacheCyc = instr.ccuTick - instr.icacheTick;
            uint64_t ccuCyc = instr.startTick - instr.ccuTick;
            uint64_t scalarCyc = SafeAdd(iCacheCyc, ccuCyc, location, false);
            cycleInfo.iCacheCyc = SafeAdd(cycleInfo.iCacheCyc, static_cast<int>(iCacheCyc), location);
            cycleInfo.ccuCyc = SafeAdd(cycleInfo.ccuCyc, static_cast<int>(ccuCyc), location);
            cycleInfo.scalarCyc = SafeAdd(cycleInfo.scalarCyc, static_cast<int>(scalarCyc), location);
        }
    }
}

void SimPcToCode::Statistic(const std::string &coreName, SimData &data)
{
    auto instrPtr = data.instrs;
    auto userMarkPtr = data.userMarks;
    std::vector<MergeInfo> instrs = *instrPtr->GetColumnData<MergeInfo>(Parse::InstrDetailTable::MERGE_INFO);
    if (userMarkPtr != nullptr) {
        std::vector<MergeInfo> userMark = userMarkPtr->userMarkInstrs;
        instrs.insert(instrs.end(), userMark.begin(), userMark.end());
    }
    sort(instrs.begin(), instrs.end(),
        [](MergeInfo &instr1, MergeInfo &instr2) {
            if (instr1.endTick != instr2.endTick) {
                return instr1.endTick < instr2.endTick;
            }
            return instr1.pc < instr2.pc;
        });
    std::vector<InstrExe> exeStatList{};
    std::map<uint64_t, std::vector<MergeInfo>> pcMap = MergeInstr(instrs);
    for (auto &instrList: pcMap) {
        std::map<std::string, float> statValue;
        MergeInfo selectedInstr = instrList.second[0]; // get 1st instr in instrlist(contains all instrs of pc)
        UpdateInstrDetail(instrList.second, selectedInstr);
        int callCount = static_cast<int>(instrList.second.size());
        CycleInfo cycleInfo = {0, 0, 0, 0, 0};
        CalCycles(instrList.second, cycleInfo);
        if (cycleInfo.realStallCyc > 0) {
            hasStallCyc_ = true;
        }
        if (cycleInfo.scalarCyc > 0) {
            hasScalarCyc_ = true;
        }
        float runningTime = CalculateRunningTime(instrList.second, simConfig_.GetChipType());
        UpdatePCStat(selectedInstr, coreName, cycleInfo, callCount);
        InstrExe instrExe = {selectedInstr.name, selectedInstr.pipe, selectedInstr.pc, callCount,
                             cycleInfo.cycles, runningTime, "\"" + selectedInstr.detail + "\""};
        exeStatList.emplace_back(instrExe);
    }
    if (!WriteExeStat(coreName, exeStatList)) {
        LogError("Failed to write csv of %s", coreName.c_str());
    }
}

std::vector<InstrInfo> SimPcToCode::GetInstrInfo()
{
    std::vector<InstrInfo> instrInfoList;
    instrInfoList.reserve(instrInfoMap_.Size());

    // 910B series
    std::regex waitSetPattern{"PIPE:([A-Za-z0-9]+),TRIGGERPIPE:([A-Za-z0-9]+),FLAGID:([A-Za-z0-9]+)"};
    std::string waitFlagName{Profiling::WAIT_FLAG};
    std::string setFlagName{Profiling::SET_FLAG};

    if (simConfig_.GetChipTypeProduct() == ChipProductType::ASCEND310P_SERIES) {
        // 310P series
        waitSetPattern = "pipe_type:([A-Za-z0-9]+),tigger_pipe:([A-Za-z0-9]+),event_id:([A-Za-z0-9]+)";
        waitFlagName = Profiling::WAIT_EVENT;
        setFlagName = Profiling::SET_EVENT;
    }

    for (auto &instrInfo: instrInfoMap_) {
        instrInfo.second.instr = UpdateInstr(instrInfo.second.instr, waitSetPattern, waitFlagName, setFlagName);
        instrInfoList.emplace_back(instrInfo.second);
    }
    std::sort(instrInfoList.begin(), instrInfoList.end(), [](InstrInfo l, const InstrInfo &r) {
        return l.addr < r.addr;
    });
    return instrInfoList;
}

std::string SimPcToCode::UpdateInstr(const std::string &instr, const std::regex &waitSetPattern,
    const std::string &waitFlagName, const std::string &setFlagName) const
{
    std::string newInstr{instr};
    std::string waitPipe{};
    std::string setPipe{};
    std::string id{};
    const int waitPipeIndex = 1;
    const int setPipeIndex = 2;
    const int index = 3;

    std::istringstream iss(instr);
    // instr 格式为 "instrName detail" , 使用 istringstream 通过空格切分字符串
    std::string instrName;
    iss >> instrName;
    std::string waitSetDetail{};
    iss >> waitSetDetail;

    if (waitSetDetail.empty()) {  // 指令格式有问题，不处理
        return newInstr;
    }

    if (instrName == waitFlagName || instrName == setFlagName) {
        std::smatch waitSetMatches;
        if (!std::regex_search(waitSetDetail, waitSetMatches, waitSetPattern)) {
            return newInstr;
        }
        // 1 为 wait pipe，2 为 set pipe，3 为 flagid
        waitPipe = waitSetMatches[waitPipeIndex];
        setPipe = waitSetMatches[setPipeIndex];
        id = waitSetMatches[index];
        if (instrName == waitFlagName) {
            newInstr = instrName + " " + waitPipe + " " + "ID:" + id;
        } else {
            newInstr = instrName + " " + setPipe + " " + "ID:" + id;
        }
    }
    return newInstr;
}

bool SimPcToCode::WriteExeStat(const std::string &coreName, std::vector<InstrExe> &exeStatList)
{
    std::string timeStamp;
    GenerateTimeStamp(timeStamp, TimeAccuracy::MILLISECOND);
    std::string csvPath = JoinPath({simConfig_.GetOutputPath(), coreName,
        coreName + "_instr_exe_" + timeStamp + ".csv"});
    std::ofstream writeFile(csvPath, std::fstream::out | std::fstream::trunc);
    if (!writeFile.is_open()) {
        LogWarn("Can not create file [%s]", csvPath.c_str());
        return false;
    }
    std::vector<std::string> InstrHeads = {"instr", "addr", "pipe", "call_count", "cycles",
                                           "running_time(us)", "detail"};
    std::string heads = Join(InstrHeads.begin(), InstrHeads.end(), ","); // heads of csv
    writeFile << heads << std::endl;
    sort(exeStatList.begin(), exeStatList.end(),
         [](InstrExe &instr1, InstrExe &instr2) { return instr1.cycles > instr2.cycles; }); // sorted by cycles
    for (const auto &exeStat: exeStatList) {
        std::string eventStr =
            exeStat.instr + "," + std::to_string(exeStat.addr) + "," + exeStat.pipe + "," +
            std::to_string(exeStat.callCount) + "," + std::to_string(exeStat.cycles) + "," +
            std::to_string(exeStat.runningTime) + "," + exeStat.detail + "\n";
        writeFile << eventStr; // write all exeStats' parameters of cores[i] into csv in order
    }
    writeFile.close(); // finish writing of cores[i]
    chmod(csvPath.c_str(), Utility::SAVE_DATA_FILE_AUTHORITY);
    LogDebug("Save instr execution statistic info of %s in %s", coreName.c_str(), csvPath.c_str());
    return true;
}

std::string SimPcToCode::GetAscendCInnerCode(uint64_t pc)
{
    if (!pc2code_.Find(pc) || pc2code_[pc].empty()) {
        return "";
    }
    const std::vector<std::string> &lines = pc2code_[pc];
    for (auto &line : lines) {
        if (line.find("ascend-toolkit") == std::string::npos) {
            return line;
        }
    }
    return lines[0];
}

}
}
