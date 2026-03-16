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


#include "sim_dump_parser_task.h"
#include <map>
#include <vector>
#include <string>
#include <regex>

#include "log.h"
#include "filesystem.h"
#include "data_format.h"
#include "thread_pool.h"
#include "common/defs.h"
#include "profiling/op_prof_data_parse.h"
#include "filesystem.h"
#include "parse/data_table/mte_throughput_table.h"
#include "parse/data_parser/mte_log_parser/mte_log_parser.h"
#include "parse/data_visualizer/mte_log_visualizer/mte_log_visualizer.h"
#include "parse/data_calculator/mte_log_calculator.h"
#include "parse/plugin/plugin_manager.h"

using namespace std;
using namespace Utility;

namespace Profiling {
void GetCoreNameFromDumpFile(std::map<CoreNameAndPreFixPair, std::vector<std::string>> &coresMap,
    const std::string &dumpFilePath)
{
    std::string dumpFile;
    if (!GetLastFile(dumpFilePath, dumpFile)) {
        LogDebug("Failed to get dump file from dump path: %s", dumpFilePath.c_str());
        return;
    }
    // coresMap: map<pair<coreName, corePrefix>, vector<dumpType>>,
    // eg: {{"core0.veccore0", "core0.veccore0."}, {"instr", "instr_popped"}}
    if (!EndsWith(dumpFile, "_log.dump")) {
        return;
    }
    constexpr size_t matchSize = 9;
    static const regex dumpPattern("((((core([0-9]+))(\\.((?:vec|cube|mix)core)[0-9]+)?)[_.])"
                                   "(icache|ifu.icache|instr|instr_popped)_log)");
    smatch dumpMatch;
    auto stripDumpFile = Strip(dumpFile.substr(0, dumpFile.size()), DUMP_SUFFIX);
    bool res = regex_match(stripDumpFile, dumpMatch, dumpPattern);
    if (!res || dumpMatch.size() < matchSize) {
        return;
    }
    string coreIdStr = dumpMatch[5];
    size_t dumpFileSize = GetFileSize(dumpFilePath);
    if (dumpFileSize == 0) {
        return;
    }
    string corePrefix = dumpMatch[2];
    string coreName = dumpMatch[3];
    string coreType = dumpMatch[7];
    string dumpType = dumpMatch[8];
    coresMap[{coreName, corePrefix}].emplace_back(dumpType);
}

bool GetCoresTuple(const string &dumpDir, std::set<CoreNameAndPreFixPair> &coresNamePair)
{
    vector<string> fileNames;
    if (!GetFileNames(dumpDir, fileNames)) {
        LogError("Failed to get dump file");
        return false;
    }
    map<CoreNameAndPreFixPair, vector<string>> coresMap;
    for (const string &dumpFile: fileNames) {
        auto dumpFilePath = JoinPath({dumpDir, dumpFile});
        // coresMap is physical
        GetCoreNameFromDumpFile(coresMap, dumpFilePath);
    }
    for (const auto &it : coresMap) {
        if (it.second.size() < 2) {  // at least 2 type, instr and instr_popped
            continue;
        }
        // A300 use physical core
        coresNamePair.insert(it.first);
    }
    bool res = !coresNamePair.empty();
    if (!res) {
        LogWarn("Failed to get any available dump file to parse, please check");
    }
    return res;
}

std::string PipeType::FindPipe(const std::string &pipeValue, const std::string &nameValue,
                               const std::string &detailValue)
{
    if (detailValue.find(LPCNT_FLAG) != detailValue.npos || detailValue.find(COND_FLAG) != detailValue.npos) {
        return USER_MARK;
    }

    if (std::find(pipeVec_.begin(), pipeVec_.end(), pipeValue) != pipeVec_.end()) {
        return pipeValue;
    }

    PipeStruct pipe;
    if (std::count(pipe.aiVector.begin(), pipe.aiVector.end(), pipeValue) != 0) {
        return "VECTOR";
    }
    if (pipeValue == pipe.flowctrl) {
        return "FLOWCTRL";
    }
    if (pipeValue == pipe.fixp) {
        return "FIXP";
    }
    if (pipeValue == pipe.issue) {
        return "EVENT";
    }
    if (nameValue.find(pipe.scalar[0]) != nameValue.npos || nameValue.find(pipe.scalar[1]) != nameValue.npos) {
        return "SCALAR";
    }
    return "ALL";
}

void MteThroughput::Process(const std::string &dumpDir, const ChipProductType &chipType,
                            uint32_t threadPoolSize)
{
    std::vector<std::string> dumpVec;
    GetFileNames(dumpDir, dumpVec);
    std::regex mteDumpPattern("^core[0-9]{1,2}\\.mte_log\\.dump(\\.[0-9]+)?$");
    Parse::DataCenter dataCenter;
    std::shared_ptr<std::vector<Parse::MteLogInstrMap>> mteLogInstrMapVecPtr =
            MakeShared<std::vector<Parse::MteLogInstrMap>>();
    if (!dataCenter.DataTableRegister(mteLogInstrMapVecPtr)) {
        Utility::LogError("DataCenter Register mteLogInstrMapVec failed.");
        return;
    }
    Utility::LogInfo("PMSampling is enabled. Start to parse mte log dump files");
    std::vector<Parse::MteLogCfg> cfgVec;
    for (const auto &dumpFile: dumpVec) {
        if (std::regex_match(dumpFile, mteDumpPattern)) {
            std::string filePath = JoinPath({dumpDir, dumpFile});
            if (Utility::GetFileSize(filePath) <= INPUT_DUMP_FILE_MIN_SIZE) {
                continue;
            }
            uint32_t coreId = static_cast<uint32_t>(mteLogInstrMapVecPtr->size());
            Parse::MteLogInstrMap mteLogInstrMap;
            mteLogInstrMapVecPtr->emplace_back(mteLogInstrMap);
            Parse::MteLogCfg mteLogCfg{coreId, filePath, chipType};
            cfgVec.emplace_back(mteLogCfg);
        }
    }
    // 多线程解析corex.mte_log.dump
    ThreadPool pool(threadPoolSize);
    pool.Start();
    for (const auto &cfg: cfgVec) {
        pool.AddTask([this, &dataCenter, cfg]() {
            SingleCoreParse(dataCenter, cfg);
        });
    }
    pool.WaitAllTasks();
    pool.Stop();
    std::vector<Parse::PluginErrorCode> results;
    // 计算
    Parse::PluginManager calPluginManager(1);
    calPluginManager.AddPlugin<Parse::MteLogCalculator>(dataCenter, chipType);
    calPluginManager.RunAllPlugins(results);

    // 可视化
    Parse::PluginManager visPluginManager(1);
    visPluginManager.AddPlugin<Parse::MteLogVisualizer>(dataCenter, chipType);
    visPluginManager.RunAllPlugins(results);

    std::shared_ptr<std::vector<nlohmann::json>> mteJsonPtr = dataCenter.GetDbPtr<std::vector<nlohmann::json>>();
    if (mteJsonPtr != nullptr) {
        jsonList_ =  std::move(*mteJsonPtr);
    }
}

void MteThroughput::SingleCoreParse(Parse::DataCenter &dataCenter, Parse::MteLogCfg mteLogCfg) const
{
    std::vector<Parse::PluginErrorCode> results;
    Parse::PluginManager pluginManager(1);
    pluginManager.AddPlugin<Parse::MteLogParser>(dataCenter, mteLogCfg);
    pluginManager.RunAllPlugins(results);
}

}
