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


#ifndef __MSOPPROF_PROFILING_OP_PROF_DATA_PARSE_H__
#define __MSOPPROF_PROFILING_OP_PROF_DATA_PARSE_H__

#include <atomic>
#include <mutex>
#include <utility>
#include "common/prof_args.h"
#include "json_parser.h"
namespace Profiling {

class DataParse {
public:
    explicit DataParse(Common::ProfMetricsAbilityConfig metrics): metrics_(std::move(metrics)) {};
    virtual ~DataParse() = default;
    virtual bool Execute(std::string dataPath) = 0;
    void SingleKernelOutputReorganize(const std::string &outputPath);

    static std::atomic<bool> inExitMode;

protected:
    static void RenameCsvFileAndModifyKernelData(const std::string &fromPath, const std::string &outputRootPath);
    static void ModifyKernelData(const std::string &destPath, const std::string &outputRootPath);
    virtual void ParseKernelFile(const std::string &kernelDir, const std::string &kernelName,
                                 const std::string &deviceId) = 0;
    virtual void DisposeTmp(const std::string &dumpPath) {}
    bool ParserDeviceIdDir(const std::string &dataPath);
    bool ExecuteSummary(const std::string &outputPath) const;

    Common::ProfMetricsAbilityConfig metrics_;
    bool isTheOnlyKernel_ {true};
    std::string theOnlyKernelPath_;
    bool isTheOnlyDevice_ {true};
    std::string theOnlyDevicePath_;
    int16_t totalKernelNum_ {0};
    int16_t failedKernelNum_ {0};
};

struct Symbol {
    std::string file;
    std::int64_t line;
    std::string func;
    std::int64_t col;
    std::int64_t discriminator;
    std::string startFile;
    std::int64_t startLine;
    std::string startAddr;
};

struct Offset2Line {
    std::string offset;
    std::string file;
    std::int64_t line;
    std::string func;
    std::vector<Symbol> symbols;
};

class SymbolizerParser {
public:
    bool Parse(std::string relocFilePath, std::vector<std::string>pcOffsets);
    std::vector<Offset2Line> offset2Lines_;

private:
    bool Symbolizer();
    bool SymbolizerPartAddr(const std::vector<std::string> &partCmd);
    bool GetSymbolsFromAddr(nlohmann::json addr2Line, std::vector<Symbol> &symbols, std::int32_t index) const;
    size_t GetSymbolizerCount();
    bool ParseAddr2Offset(const nlohmann::json &addr2Lines);
    bool SaveOffset2Lines(const nlohmann::json &addr2Line, const nlohmann::json &tempSymbol,
                          std::int32_t index, std::vector<Offset2Line> &partOffset2Lines);

    std::string relocFilePath_;
    std::vector<std::string> pcOffsets_;
    std::mutex writeAddrMutex_;
    std::atomic<int> relations_ = {0};
};

std::string GetStartPcFromDump(const std::string &outputPath);
}

#endif // __MSOPPROF_PROFILING_OP_PROF_DATA_PARSE_H__
