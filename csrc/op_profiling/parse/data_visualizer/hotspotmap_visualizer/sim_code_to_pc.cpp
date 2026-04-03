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


#include "sim_code_to_pc.h"
#include <algorithm>
#include <vector>
#include "parse/data_visualizer/utility.h"
#include "ustring.h"
#include "filesystem.h"
#include "thread_pool.h"
#include "pc_process.h"

using namespace Utility;
using namespace Serialization;
namespace Profiling {
namespace Parse {

struct UpdateCode {
    int callCount;
    int cycles;
    int gprCount;
    int processBytes;
    std::string code;
    std::set<uint64_t> pcSet;
};

void SimCodeToPc::CalCulate()
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

void SimCodeToPc::Statistic(const std::string &coreName, SimData &data)
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
    std::vector<CodeExe> exeStatList;
    std::map<std::string, std::vector<MergeInfo>> codeMap = !pc2code_.Empty() ?
        MergeInstr(instrs) : std::map<std::string, std::vector<MergeInfo>> {};
    for (auto &instrList: codeMap) {
        std::set<uint64_t> pcSet;
        int callCount = static_cast<int>(instrList.second.size());
        int cycles = CalCycles(instrList.second);
        int maxRegister = -1;
        int processBytes = 0;
        for (size_t i = 0; i < instrList.second.size(); i++) {
            // user mark类指令不参与计算GPR使用等数据，固定展示NA
            if (instrList.second[i].pipe == USER_MARK) {
                continue;
            }
            maxRegister = std::max(maxRegister, instrList.second[i].gprCount);
            processBytes += instrList.second[i].processBytes;
        }
        float runningTime = CalculateRunningTime(instrList.second, simConfig_.GetChipType());
        std::for_each(instrList.second.begin(), instrList.second.end(), [&](MergeInfo &i) {
            pcSet.insert(i.pc);
        });
        struct UpdateCode updateCode = {callCount, cycles, maxRegister, processBytes, instrList.first, pcSet};
        UpdateCodeStat(updateCode, coreName);
        CodeExe codeExe = {callCount, cycles, runningTime, instrList.first};
        exeStatList.emplace_back(codeExe);
    }
    if (!WriteExeStat(coreName, exeStatList)) {
        LogError("Failed to write csv of %s", coreName.c_str());
    }
    CalAddrRange();
}

std::map <std::string, std::vector<MergeInfo>> SimCodeToPc::MergeInstr(const std::vector <MergeInfo> &instrs)
{
    std::map<std::string, std::vector<MergeInfo>> codeMap;
    for (const auto &instr: instrs) {
        std::vector<std::string> codeStack = pc2code_[instr.pc]; // get code according to pc
        for (const std::string &code: codeStack) {
            if (code.find("ascend-toolkit") == std::string::npos) {
                codeMap[code].emplace_back(instr);
            }
        }
    }
    return codeMap;
}

bool SimCodeToPc::UpdateCodeStat(const struct UpdateCode &updateCode, const std::string &coreName)
{
    std::vector<std::string> fileLines;
    SplitString(updateCode.code, ':', fileLines);
    std::string codeFile = fileLines.front();
    std::string line = fileLines.back();
    {
        std::lock_guard<std::mutex> lock(codeLineMappingMutex_);
        if (codeInfoMap_.count(codeFile) == 0) {
            if (!VisualizeKernelFiles(simConfig_.GetOutputPath(), codeFile)) { return false; }
            codeInfoMap_[codeFile] = {};
        }
        if (codeInfoMap_[codeFile].find(line) == codeInfoMap_[codeFile].end()) {
            codeInfoMap_[codeFile][line].callCount.resize(cores_.size(), 0);
            codeInfoMap_[codeFile][line].cycles.resize(cores_.size(), 0);
            codeInfoMap_[codeFile][line].gprCount.resize(cores_.size(), 0);
            codeInfoMap_[codeFile][line].processBytes.resize(cores_.size(), -1);
            if (!StoiConverter(line, codeInfoMap_[codeFile][line].line)) {
                LogWarn("Failed to convert line [%d] str to int", codeInfoMap_[codeFile][line].line);
                return false;
            }
        }
        auto index = distance(cores_.begin(), find(cores_.begin(), cores_.end(), coreName));
        if (static_cast<size_t>(index) >= cores_.size()) {
            Utility::LogWarn("Core index error, core name:%s", coreName.c_str());
            return false;
        }
        codeInfoMap_[codeFile][line].callCount[index] = updateCode.callCount;
        codeInfoMap_[codeFile][line].cycles[index] = updateCode.cycles;
        codeInfoMap_[codeFile][line].gprCount[index] = updateCode.gprCount;
        if (updateCode.processBytes != 0) {
            // updateCode.processBytes=0 should be displayed as NA
            codeInfoMap_[codeFile][line].processBytes[index] = updateCode.processBytes;
        }
        lineAdrrMap_[codeFile][line].insert(updateCode.pcSet.begin(), updateCode.pcSet.end());
    }
    return true;
}

bool SimCodeToPc::CalAddrRange()
{
    for (auto &codeInfo: codeInfoMap_) {
        for (auto &lineCode: codeInfo.second) {
            std::set<uint64_t> codeSet = lineAdrrMap_[codeInfo.first][lineCode.first];
            std::vector<std::vector<std::string>> addrRangeList = MergeAddrRange(codeSet);
            if (addrRangeList.empty()) {return false;}
            {
                std::lock_guard<std::mutex> lock(codeLineMappingMutex_);
                lineCode.second.addrRange = addrRangeList;
            }
        }
    }
    return true;
}

bool SimCodeToPc::WriteExeStat(const std::string &coreName, std::vector<CodeExe> &exeStatList)
{
    std::string timeStamp;
    GenerateTimeStamp(timeStamp, TimeAccuracy::MILLISECOND);
    std::string csvPath = JoinPath({simConfig_.GetOutputPath(), coreName, coreName + "_code_exe_" + timeStamp +".csv"});
    std::ofstream writeFile(csvPath, std::fstream::out | std::fstream::trunc);
    if (!writeFile.is_open()) {
        LogWarn("Can not create file [%s]", csvPath.c_str());
        return false;
    }
    std::vector<std::string> CodeHeads = {"code", "call_count", "cycles", "running_time(us)"};
    std::string heads = Join(CodeHeads.begin(), CodeHeads.end(), ",");
    writeFile << heads << std::endl;
    sort(exeStatList.begin(), exeStatList.end(),
         [](CodeExe &code1, CodeExe &code2) { return code1.cycles > code2.cycles; });
    for (const CodeExe &exeStat: exeStatList) {
        std::string eventStr = exeStat.code + "," + std::to_string(exeStat.callCount) + "," +
        std::to_string(exeStat.cycles) + "," + std::to_string(exeStat.runningTimeUS) + "\n";
        writeFile << eventStr;
    }
    writeFile.close(); // finish writing of cores[i]

    chmod(csvPath.c_str(), Utility::SAVE_DATA_FILE_AUTHORITY);
    LogDebug("Save code execution statistic info in [%s]", csvPath.c_str());
    return true;
}

bool SimCodeToPc::VisualizeKernelFiles(const std::string &outputPath, const std::string &filePath) const
{
    auto fileContent = CheckAndReadFile(filePath, "cpp", INPUT_CPP_FILE_MAX_SIZE, "", Utility::LogLv::DEBUG);
    if (fileContent.empty()) {
        return false;
    }
    Visualize::WriteBin<VisualizeType::CODE>(outputPath, fileContent, filePath);
    return true;
}

std::vector<CodeFile> SimCodeToPc::GetCodeFile()
{
    std::vector<CodeFile> files;
    for (const auto &codeInfo: codeInfoMap_) {
        CodeFile codeFile;
        codeFile.file = codeInfo.first;
        for (const auto &codeLines: codeInfo.second) {codeFile.lines.emplace_back(codeLines.second);}
        files.emplace_back(codeFile);
    }
    return files;
}
}
}