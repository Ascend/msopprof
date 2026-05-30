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

#include <iostream>
#include "ccu_log_parser.h"

using namespace Utility;
namespace Profiling {
namespace Parse {

PluginErrorCode CcuParser::Entry() {
    const std::string &corePrefix = dataParserConfig_.GetCoreInfo().second;
    std::vector<std::string> splitDumpFileVec = dataParserConfig_.GetSplitFilesVec(corePrefix + "ccu_log", DUMP_SUFFIX);
    reverse(splitDumpFileVec.begin(), splitDumpFileVec.end());
    ChipProductType chip = dataParserConfig_.GetProductSeriesType();
    if (splitDumpFileVec.empty()) {
        LogDebug("Failed to get ccu log");
        return PluginErrorCode::NONBLOCKING_ERROR;
    }
    for (std::string &splitFile : splitDumpFileVec) {
        std::vector<std::string> fileLines;
        ReadFileByMMap(splitFile, fileLines);
        for (const auto &line : fileLines) {
            ParseLine(line);
        }
    }
    if (ccuInstrMap_.empty()) {
        LogDebug("Failed to parse ccu log");
        return PluginErrorCode::NONBLOCKING_ERROR;
    }
    auto ccuTable = Utility::MakeShared<scalarHead>(ccuInstrMap_);
    if (ccuTable == nullptr) {
        LogDebug("Failed to register ccu message");
        return PluginErrorCode::NONBLOCKING_ERROR;
    }
    if (!dataCenter_.DataTableRegister(ccuTable)) {
        LogDebug("Failed to register ccu table");
        return PluginErrorCode::NONBLOCKING_ERROR;
    }
    return PluginErrorCode::SUCCESS;
}

void CcuParser::ParseLine(const std::string &line) {
    std::smatch lineMatch;
    if (std::regex_search(line, lineMatch, instrMatchPattern_)) {
        uint64_t tick;
        uint64_t pc;
        std::istringstream iss1(lineMatch[scalarRuleNamePos_["tick"]].str());
        std::istringstream iss2(lineMatch[scalarRuleNamePos_["pc"]].str());
        if ((iss1 >> tick) && (iss2 >> std::hex >> pc)) {
            ccuInstrMap_[pc].emplace_back(ScalarInstrInfo{UINT64_MAX, tick, pc});
        }
    }
}
}
}
