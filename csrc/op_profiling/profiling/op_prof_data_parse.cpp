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


#include "op_prof_data_parse.h"
#include <functional>
#include <thread>
#include <cmath>
#include "filesystem.h"
#include "log.h"
#include "ustring.h"
#include "ascend_helper.h"
#include "cmd_execute.h"
#include "thread_pool.h"

using namespace Utility;
namespace Profiling {
std::atomic<bool> DataParse::inExitMode {false};
void DataParse::SingleKernelOutputReorganize(const std::string &outputPath)
{
    if (isTheOnlyKernel_ && !theOnlyKernelPath_.empty()) {
        LogDebug("There's only one kernel running, path is %s", theOnlyKernelPath_.c_str());
        std::function<void(const std::string&, const std::string&)> NewCallable =
           bind(&DataParse::RenameCsvFileAndModifyKernelData, std::placeholders::_1, std::placeholders::_2);
        CopyFolder(theOnlyKernelPath_, outputPath, outputPath, NewCallable);
        uint32_t rollNum = 2;
        RollbackPath(theOnlyKernelPath_, rollNum);
        RemoveAll(theOnlyKernelPath_);
    } else if (isTheOnlyDevice_ && !theOnlyDevicePath_.empty()) {
        LogDebug("There's only one device running, path is %s", theOnlyDevicePath_.c_str());
        std::function<void(const std::string&, const std::string&)> NewCallable =
           bind(&DataParse::ModifyKernelData, std::placeholders::_1, std::placeholders::_2);
        CopyFolder(theOnlyDevicePath_, outputPath, outputPath, NewCallable);
        RemoveAll(theOnlyDevicePath_);
    }
}

void DataParse::ModifyKernelData(const std::string &destPath, const std::string &outputRootPath)
{
    // This func will replace kernel_config.bin content of old path to new Path
    std::string fileName = GetLastFile(destPath);
    if (fileName != "kernel_config.bin") {
        return;
    }
    std::ifstream in(destPath.c_str(), std::ios::in|std::ios::binary);
    if (!in.is_open()) {
        LogWarn("Modify kernel config read failed");
        return;
    }
    std::string buf;
    getline(in, buf);
    in.close();
    std::string newPath = destPath;
    RollbackPath(newPath, 1);
    auto start = buf.find(outputRootPath);
    auto end = buf.find("kernel_data", start + outputRootPath.size());
    if (start == std::string::npos || end == std::string::npos) {
        LogWarn("Modify kernel config failed");
        return;
    }
    // because end pos contains one more char and '/',so need to -2
    std::string oldPath = buf.substr(start, end - 2);
    std::string replaceContent = ReplaceSubStr(buf, oldPath, newPath);
    if (!WriteFileByStream(destPath, replaceContent)) {
        LogWarn("Modify kernel config write failed");
        return;
    }
}

void DataParse::RenameCsvFileAndModifyKernelData(const std::string &fromPath, const std::string &outputRootPath)
{
    ModifyKernelData(fromPath, outputRootPath);
    std::string fileName = GetLastFile(fromPath);
    std::string filePath = GetFileFaPath(fromPath, fileName);
    if (fileName.empty() || filePath.empty()) {
        return;
    }
    std::regex deviceMetricPattern("^[A-Za-z0-9]{0,30}_[0-9]{17}.csv$");
    std::regex codeExecPattern("^[.A-Za-z0-9]{0,20}_code_exe_[0-9]{17}.csv$");
    std::regex instrExecPattern("^[.A-Za-z0-9]{0,20}_instr_exe_[0-9]{17}.csv$");
    if (std::regex_match(fileName, codeExecPattern) || std::regex_match(fileName, instrExecPattern) ||
        std::regex_match(fileName, deviceMetricPattern)) {
        std::string prefix = fileName.substr(0, fileName.find_last_of('_'));
        size_t dotIndex = fileName.find_last_of('.');
        std::string suffix = fileName.substr(dotIndex, fileName.length() - dotIndex + 1);

        LogDebug("Rename csv file[%s] prefix[%s] suffix[%s]", filePath.c_str(), prefix.c_str(), suffix.c_str());
        std::string newName(prefix + suffix);
        std::string newPath = JoinPath({filePath, newName.c_str()});
        rename(fromPath.c_str(), newPath.c_str());
    }
}
bool DataParse::ExecuteSummary(const std::string &outputPath) const
{
    if (totalKernelNum_ == 0) {
        LogWarn("Profiling results are empty. No kernel profiling data was generated. Please check the dump output.");
        return false;
    }
    LogInfo("Profiling results saved in %s", outputPath.c_str());
    if (failedKernelNum_ > 0) {
        LogWarn("Profiling kernels result is: %s success, %s failed. Please check",
                std::to_string(totalKernelNum_ - failedKernelNum_).c_str(), std::to_string(failedKernelNum_).c_str());
        if (failedKernelNum_ == totalKernelNum_) {
            return false;
        }
    }
    return true;
}

bool DataParse::ParserDeviceIdDir(const std::string &dataPath)
{
    std::vector<std::string> deviceIds;
    bool kernelNameFilterMiss = false;
    bool parseSuccess = true;
    if (!GetFileNames(dataPath, deviceIds)) {
        kernelNameFilterMiss = !kernelNameFilter_.empty();
        parseSuccess = false;
    } else {
        // device id dir
        for (const std::string &deviceId : deviceIds) {
            std::string deviceIdDir = JoinPath({dataPath, deviceId});
            if (!IsDir(deviceIdDir)) {
                continue;
            }
            std::vector<std::string> kernelNames;
            DisposeTmp(deviceIdDir);
            if (!GetFileNames(deviceIdDir, kernelNames)) {
                continue;
            }
            // kernel name dir
            for (const auto &file : kernelNames) {
                std::string filePath = JoinPath({deviceIdDir, file});
                if (!IsDir(filePath)) {
                    continue;
                }
                ParseKernelFile(filePath, file, deviceId);
            }
            if (!theOnlyDevicePath_.empty()) {
                isTheOnlyDevice_ = false;
            }
            if (isTheOnlyDevice_) {
                theOnlyDevicePath_ = deviceIdDir;
            }
        }
        kernelNameFilterMiss = (totalKernelNum_ == 0 && !kernelNameFilter_.empty());
    }
    if (kernelNameFilterMiss) {
        LogWarn("No profiling data matched --kernel-name=%s. All kernels were filtered out or no matching dump was "
                "generated. Please confirm the kernel name or wildcard pattern.",
            kernelNameFilter_.c_str());
    }
    if (!parseSuccess) {
        LogWarn("No profiling data dumped.");
        return false;
    }
    return true;
}

bool SymbolizerParser::Parse(std::string relocFilePath, std::vector<std::string> pcOffsets)
{
    relocFilePath_ = std::move(relocFilePath);
    pcOffsets_ = std::move(pcOffsets);
    if (!Symbolizer()) {
        return false;
    }
    Utility::LogInfo("Parse %d addr2line relations", offset2Lines_.size());
    return true;
}

size_t SymbolizerParser::GetSymbolizerCount()
{
    size_t maxLength = 1;
    for (const auto &pc : pcOffsets_) {
        maxLength = std::max(maxLength, pc.size());
    }
    // calculate max actually params number limit by args max length
    size_t argMaxLength = static_cast<size_t>(sysconf(_SC_ARG_MAX)) / maxLength;
    // calculate max actually params number limit by args max number,usually pagesize is 32
    size_t argMaxNumber = static_cast<size_t>(sysconf(_SC_PAGESIZE)) * 32;
    float threshold = 0.8;
    size_t maxLimitCount = static_cast<size_t>(std::min(argMaxLength, argMaxNumber) * threshold);
    // Set the maximum instruction once process to prevent excessive pc parsing at a time from taking a long time.
    size_t maxExcuteAddr = 30000;
    maxExcuteAddr = std::min(maxExcuteAddr, maxLimitCount);
    return maxExcuteAddr;
}

bool SymbolizerParser::Symbolizer()
{
    std::string symbolizerPath;
    std::string opprofPath = Utility::GetMsopprofPath();
    if (!opprofPath.empty()) {
        symbolizerPath = Utility::JoinPath({opprofPath, "bin", "llvm-symbolizer"});
    } else {
        return false;
    }
    std::vector<std::string> cmd = {symbolizerPath, "-f", "-e", relocFilePath_,
                                    "--inlining=true", "--output-style=JSON"};
    // calculate the number of parameters than symbolizer parsed at a time
    size_t maxExcuteAddr = GetSymbolizerCount();
    if (maxExcuteAddr == 0) {
        Utility::LogDebug("There are something wrong with system, can't parse pc address");
        return false;
    }
    uint32_t count = static_cast<uint32_t>(ceil(static_cast<float>(pcOffsets_.size()) / maxExcuteAddr));
    static constexpr double maxThreadUsageRatio = 0.6;
    uint32_t systemCores = static_cast<uint32_t>(std::thread::hardware_concurrency() * maxThreadUsageRatio);
    uint32_t threadPoolSize = std::min(systemCores, count) == 0 ? 1 : std::min(systemCores, count);
    Utility::ThreadPool pool (threadPoolSize);
    pool.Start();
    for (size_t i = 0; i < count; i++) {
        std::vector<std::string> partCmd;
        partCmd.insert(partCmd.end(), cmd.begin(), cmd.end());
        size_t endPosition = std::min((i + 1) * maxExcuteAddr, pcOffsets_.size());
        partCmd.insert(partCmd.end(), pcOffsets_.begin() + i * maxExcuteAddr, pcOffsets_.begin() + endPosition);
        const auto executeCmd = partCmd;
        std::string partOutput;
        pool.AddTask([this, executeCmd]() {
            SymbolizerPartAddr(executeCmd);
        });
    }
    pool.WaitAllTasks();
    pool.Stop();
    Utility::LogInfo("Extract %d relations from kernel", relations_.load());
    if (offset2Lines_.empty()) {
        Utility::LogWarn("Failed to parse pc to code , maybe kernel missed debug_line information. If you need code "
                         "call stack, please recompile kernel with -g option");
        return false;
    }
    return true;
}

bool SymbolizerParser::SymbolizerPartAddr(const std::vector<std::string> &partCmd)
{
    std::string partOutput;
    std::map<std::string, std::string> env = {};
    if (!Utility::CmdExecute(partCmd, env, partOutput)) {
        Utility::LogDebug("Failed to extract addr2line info from: %s", relocFilePath_.c_str());
        return false;
    }
    std::string jsonContext = partOutput;
    std::vector<std::string> linesVec;
    SplitString(partOutput, '\n', linesVec);
    uint32_t endPoslength = 2;
    for (std::string &line:linesVec) {
        if ((line.rfind("[{", 0) == 0) && (line.rfind("}]") == (line.length() - endPoslength))) {
            jsonContext = line;
            break;
        }
    }
    nlohmann::json addr2Lines;
    try {
        addr2Lines = nlohmann::json::parse(jsonContext);
    } catch (const std::exception &e) {
        Utility::LogWarn("Failed to extract addr2line info from : %s", relocFilePath_.c_str());
        return false;
    }
    relations_.fetch_add(addr2Lines.size());
    if (!ParseAddr2Offset(addr2Lines)) {
        return false;
    }
    return true;
}

bool SymbolizerParser::GetSymbolsFromAddr(nlohmann::json addr2Line, std::vector<Symbol> &symbols,
    std::int32_t index) const
{
    if (!addr2Line.contains("Symbol")) {
        Utility::LogDebug("addr2Line do not contain Symbol");
        return false;
    }
    nlohmann::json symbolsData = addr2Line.at("Symbol");
    Symbol symbolStruct;
    for (std::int32_t i = 0; i < index + 1; i++) {
        if (i >= static_cast<int32_t>(symbolsData.size())) {
            continue;
        }
        auto symbol = symbolsData[i];
        try {
            symbol["FileName"].get_to(symbolStruct.file);
            symbol["Line"].get_to(symbolStruct.line);
            symbol["FunctionName"].get_to(symbolStruct.func);
            symbol["Column"].get_to(symbolStruct.col);
            symbol["Discriminator"].get_to(symbolStruct.discriminator);
            symbol["StartFileName"].get_to(symbolStruct.startFile);
            symbol["StartLine"].get_to(symbolStruct.startLine);
            symbol["StartAddress"].get_to(symbolStruct.startAddr);
        } catch (std::exception &e) {
            return false;
        }
        symbols.emplace_back(symbolStruct);
    }
    if (symbols.empty()) {
        Utility::LogDebug("Failed get symbols from addr2line");
        return false;
    }
    return true;
}

bool SymbolizerParser::ParseAddr2Offset(const nlohmann::json &addr2Lines)
{
    std::vector<Offset2Line> partOffset2Lines;
    for (auto addr2Line: addr2Lines) {
        if (!addr2Line.contains("Address") || !addr2Line.contains("Symbol")) { continue; }
        auto symbolsVec = addr2Line.at("Symbol");
        if (symbolsVec.size() == 0) { continue; }
        auto symbolEnd = symbolsVec[symbolsVec.size() - 1];
        if (!symbolEnd.contains("FileName")) { continue; }
        std::string kernelFile = symbolEnd.at("FileName");
        std::string ascendFlag = "ascend-toolkit";
        if (strstr(kernelFile.c_str(), ascendFlag.c_str()) != nullptr) {
            if (!SaveOffset2Lines(addr2Line, symbolEnd, 0, partOffset2Lines)) {
                return false;
            }
        }
        for (size_t i = 0; i < symbolsVec.size(); i++) {
            const auto symbolMap = symbolsVec[i];
            if (!symbolMap.contains("FileName") || std::int64_t(symbolMap.at("Line")) == 0) { continue; }
            std::string fileName = symbolMap.at("FileName");
            if (strstr(fileName.c_str(), ascendFlag.c_str()) != nullptr) { continue; }
            if (!SaveOffset2Lines(addr2Line, symbolMap, i, partOffset2Lines)) {
                return false;
            }
        }
    }
    {
        std::lock_guard<std::mutex> lock(writeAddrMutex_);
        offset2Lines_.insert(offset2Lines_.end(), partOffset2Lines.begin(), partOffset2Lines.end());
    }
    return true;
}

bool SymbolizerParser::SaveOffset2Lines(const nlohmann::json &addr2Line, const nlohmann::json &tempSymbol,
                                        std::int32_t index, std::vector<Offset2Line> &partOffset2Lines)
{
    std::vector<Symbol> symbols;
    if (!GetSymbolsFromAddr(addr2Line, symbols, index)) {
        Utility::LogWarn("Failed to parse addr2line from %s", relocFilePath_.c_str());
        return false;
    }
    Offset2Line offLine;
    addr2Line["Address"].get_to(offLine.offset);
    tempSymbol["FileName"].get_to(offLine.file);
    tempSymbol["Line"].get_to(offLine.line);
    tempSymbol["FunctionName"].get_to(offLine.func);
    offLine.symbols = symbols;
    partOffset2Lines.emplace_back(offLine);
    return true;
}

std::string GetStartPcFromDump(const std::string &outputPath, const std::string &pcStartText)
{
    std::string pcStartDumpFile = Utility::JoinPath({outputPath, pcStartText});
    if (Utility::IsReadable(pcStartDumpFile)) {
        std::ifstream file(pcStartDumpFile);
        if (file.is_open()) {
            std::string line;
            getline(file, line);
            file.close();
            // Default value of start pc in stub is 0x0.
            if (line != "0x0") {
                return line;
            }
        }
    }
    LogDebug("Failed to get start pc info in %s.", pcStartText.c_str());
    return "";
}
}  // namespace Profiling
