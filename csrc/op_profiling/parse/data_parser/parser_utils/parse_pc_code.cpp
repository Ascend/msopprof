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


#include "parse_pc_code.h"
#include <set>
#include <algorithm>
#include <iterator>
#include "filesystem.h"
#include "number_operation.h"
#include "parse/data_table/instr_detail_table.h"
#include "cmd_execute.h"

using namespace std;
using namespace Utility;

namespace Profiling {
bool ParsePcCode::Parse()
{
    std::vector<std::string> pcOffset;
    uint64_t startPc;
    if (isGetPcOffsetByKernelName_) {
        startPc = GetStartPcFromTxt();
        pcOffset = Int2Hex(pcSet_);
    } else {
        startPc = GetStartPc();
        if (startPc == UINT64_MAX) {
            LogDebug("Failed to get start pc.");
            return false;
        }
        pcOffset = GetPcOffset(startPc);
    }
    SymbolizerParser symbolPar;
    std::string aiCorePath = JoinPath({dumpPath_, Common::AICORE_KERNEL_NAME});
    if (!symbolPar.Parse(aiCorePath, pcOffset)) {
        LogDebug("Failed to get offset2code");
        return false;
    }
    std::vector<Offset2Line> addr2Lines = symbolPar.offset2Lines_;
    if (!GenPc2Code(addr2Lines, startPc)) {
        LogWarn("Failed to get code and pc info, most likely no debug info");
        return false;
    }
    return true;
}

bool ParsePcCode::GenPc2Code(const std::vector<Offset2Line>& addr2Lines, uint64_t startPc)
{
    for (const Offset2Line &addr2line:addr2Lines) {
        uint64_t offset;
        if (!StoullConverter(addr2line.offset, offset, RADIX_16)) {
            continue;
        }
        uint64_t pc = AddStartPc2Offset(startPc, offset);
        if (pc == UINT64_MAX) {
            LogWarn("Failed to get pc by start pc and offset");
            return false;
        }
        if (addr2line.symbols.size() > pc2code_[pc].size()) {
            pc2code_[pc] ={};
            GenPc2CodeBySymbol(addr2line.symbols, pc);
        }
    }
    return true;
}

void ParsePcCode::GenPc2CodeBySymbol(const std::vector<Symbol> &symbols, uint64_t pc)
{
    std::set<std::string> removeDuplicates;
    for (const auto &symbol : symbols) {
        std::string fileLine = symbol.file + ":" + to_string(symbol.line);
        if (removeDuplicates.find(fileLine) != removeDuplicates.end()) {
            continue;
        }
        removeDuplicates.insert(fileLine);
        pc2code_[pc].emplace_back(fileLine);
    }
}

uint64_t ParsePcCode::AddStartPc2Offset(uint64_t startPC, uint64_t offset) const
{
    std::string location = "addOffset";
    uint64_t resInt = SafeAdd(startPC, offset, location);
    return resInt;
}

std::vector<std::string> ParsePcCode::GetPcOffset(uint64_t startPc)
{
    std::set<uint64_t> pcOffset;
    for (const auto &temp : pcSet_) {
        if (temp < startPc) {
            continue;
        }
        pcOffset.insert(temp - startPc);
    }
    return Int2Hex(pcOffset);
}

std::vector<std::string> ParsePcCode::Int2Hex(const std::set<uint64_t> &inSet) const
{
    std::vector<std::string> res;
    for (const uint64_t &it : inSet) {
        stringstream ss;
        std::string out;
        ss << std::hex << it;
        ss >> out;
        res.emplace_back("0x" + out);
    }
    return res;
}

uint64_t ParsePcCode::GetStartPcFromTxt() const
{
    uint64_t startPc = UINT64_MAX;
    std::string pcStartDumpFile = Utility::JoinPath({dumpPath_, Common::PC_START_PATH});
    char buf[PATH_MAX];
    if (realpath(pcStartDumpFile.c_str(), buf) != nullptr && Utility::IsReadable(std::string{buf})) {
        std::ifstream file(buf);
        if (file.is_open()) {
            std::string line;
            getline(file, line);
            file.close();
            // Default value of start pc in stub is 0x0.
            if (line != "0x0" && StoullConverter(line, startPc, RADIX_16)) {
                LogDebug("Success get pc start %s from dump file", line.c_str());
            }
        }
    }
    if (startPc == UINT64_MAX) {
        LogWarn("Failed to get pc start from %s", pcStartDumpFile.c_str());
    }
    return startPc;
}

uint64_t ParsePcCode::GetStartPc() const
{
    uint64_t startPc = GetStartPcFromTxt();
    if (startPc != UINT64_MAX) {
        return startPc;
    }
    if (!pcSet_.empty()) {
        return *pcSet_.begin();
    }
    return startPc;
}

void ParsePcCode::GetAllPc(const string &start, const string &size)
{
    std::stringstream ssStart(start);
    std::stringstream ssSize(size);

    uint64_t kernelStartPc {};
    ssStart >> std::hex >> kernelStartPc;
    uint64_t kernelSize {};
    ssSize >> std::hex >> kernelSize;
    // pc间隔为4
    kernelSize = kernelSize / 4;

    std::generate_n(std::inserter(pcSet_, pcSet_.end()), kernelSize + 1, [&kernelStartPc]() {
        uint64_t val = kernelStartPc;
        kernelStartPc += 4;  // 每次增加4
        return val;
    });
}

bool ParsePcCode::GetPcSetByKernelName(const std::string &kernelName)
{
    using namespace Common;
    if (kernelName.empty()) {
        LogDebug("Failed to get pc, kernel name is null");
        return false;
    }
    if (GetStartPcFromTxt() == UINT64_MAX) {
        LogDebug("Failed to get start pc info in dump.");
        return false;
    }
    // kernel Name for mix kernel
    std::set<std::string> kernelNames;
    kernelNames.insert(kernelName);
    size_t pos = 0;
    if ((pos = kernelName.find(MIX_AIC_TAIL)) != std::string::npos) {
        std::string aivName = kernelName;
        aivName.replace(pos, std::string(MIX_AIC_TAIL).length(), MIX_AIV_TAIL);
        kernelNames.insert(aivName);
    } else if ((pos = kernelName.find(MIX_AIV_TAIL)) != std::string::npos) {
        std::string aiCName = kernelName;
        aiCName.replace(pos, std::string(MIX_AIV_TAIL).length(), MIX_AIC_TAIL);
        kernelNames.insert(aiCName);
    }
    std::string aicoreFilePath = Utility::JoinPath({dumpPath_, Common::AICORE_KERNEL_NAME});
    std::string output;
    std::map<std::string, std::string> envs;
    bool result = Utility::CmdExecute({"llvm-objdump", "-t", aicoreFilePath}, envs, output);
    if (!result) {
        LogWarn("Get pc set failed");
        return false;
    }
    std::vector<std::string> lines;
    Split(output, std::back_inserter(lines), "\n");
    for (std::string &line: lines) {
        std::istringstream iss(line);
        auto lineVec = std::vector<std::string>(std::istream_iterator<std::string>{iss},
                                                std::istream_iterator<std::string>());
        // dump info like this:
        // address      symbol   segment   type    size     kernelname
        // 0                1    2         3       4        5
        // 0000000000000000 g F .text 00000000000000a8 Abs_d2db1a80c523e7e59a032c95969880af_high_performance_2147483647
        if (lineVec.size() == 6 && lineVec.at(1) == "g" && lineVec.at(2) == "F" && lineVec.at(3) == ".text" &&
            kernelNames.find(lineVec.at(5)) != kernelNames.end()) { // index 5 means kernel name
            string start = lineVec.at(0);
            string size = lineVec.at(4);
            GetAllPc(start, size);
            LogDebug("Get kernel pc set, pc start is %s, pc size is %s, kernel name is %s, pc set size is %d",
                     start.c_str(), size.c_str(), lineVec.at(5).c_str(), pcSet_.size()); // index 5 means kernel name
        }
    }
    if (!pcSet_.empty()) {
        isGetPcOffsetByKernelName_ = true;
    } else if (kernelName.find(MIX_AIC_TAIL) == std::string::npos && kernelName.find(MIX_AIV_TAIL) == std::string::npos) {
        LogDebug("PcSet is empty, try to get pcSet by kernel name %s append tail", kernelName.c_str());
        GetPcSetByKernelName(kernelName + MIX_AIC_TAIL);
    } else {
        LogDebug("Failed to get kernel name %s", kernelName.c_str());
        return false;
    }
    
    return true;
}
}