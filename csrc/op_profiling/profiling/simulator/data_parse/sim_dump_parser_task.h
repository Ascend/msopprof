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



#ifndef MSOPT_SIM_DUMP_PARSER_TASK_H
#define MSOPT_SIM_DUMP_PARSER_TASK_H

#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <set>
#include <regex>
#include "sim_defs.h"
#include "thread_safe_unordered_map.h"
#include "parse/data_center/data_center.h"
#include "parse/data_table/mte_throughput_table.h"
#include "json.hpp"

namespace Profiling {
struct DumpParserArgs {
    ChipProductType chipType;
    std::string coreName;
    bool enableResourceConflictRatio =  true;
    std::set<int> parseCoreIds;
};

struct UserMarkInfoType {
    std::map<std::string, uint32_t> startCnt;
    std::map<std::string, uint32_t> stopCnt;
    // UserMark map. 根据 name 查找对应的打点信息列表
    std::map<std::string, std::vector<UserMarkInfo>> nameToInfo;
};

// simt 有32个warp 每个warp有4个sch，以1个warp的1个sch作为一个channel，在一个channel上统计stallCyc
struct SimtChnInfo {
    uint64_t stallCyc;
    uint64_t tick;
};

struct PoppedInstrListInfo {
    std::unordered_map<std::string, size_t> simtChnIdMap;
    std::vector<SimtChnInfo> simtChnInfoGrp;
};

struct SpRegexStruct {
    std::regex pattern;
    uint8_t expectResNum;
    uint8_t posInSpReg;
};

class MteThroughput {
public:
    void Process(const std::string &dumpDir, const ChipProductType &chipType, uint32_t threadPoolSize);
    std::vector<nlohmann::json> jsonList_;
private:
    void SingleCoreParse(Parse::DataCenter &dataCenter, Parse::MteLogCfg mteLogCfg) const;
};

class PipeType {
public:
    std::string FindPipe(const std::string &pipeValue, const std::string &nameValue, const std::string &detailValue);
private:
    std::vector<std::string> pipeVec_ = {"SCALAR", "FLOWCTRL", "VECTOR", "CUBE", "MTE1", "MTE2", "MTE3",
        "FIXP", "EVENT", "CACHEMISS", "ALL", USER_MARK, RVECST, RVECLD, RVECEX, PUSHQ, RVECSU, RVECLP};
    struct PipeStruct {
        std::vector<std::string> aiVector = {"VECTOR0", "VEC0", "VEC"};
        std::string flowctrl = "FC";
        std::string fixp = "FIX_PIPE";
        std::string issue = "ISSUE";
        std::vector<std::string> scalar = {"scalar_ld", "scalar_st"};
    };
};

const std::vector<std::string> DUMP_FILE_RULES = {"id = [0-9]+",
                                                  "core_id = core{id}",
                                                  "core_type = (?:vec|cube)core",
                                                  "core_name = {core_id}(\\.{core_type}[0-9]+)?",
                                                  "core_prefix = {core_name}[_.]",
                                                  "dump_type = icache|ifu.icache|instr|instr_popped",
                                                  "dump_file = {core_prefix}{dump_type}_log"
};
const std::vector<std::string> DUMP_FILE_RULE_NAMES = {"id", "core_id", "core_type", "core_name",
                                                       "core_prefix", "dump_type", "dump_file"};
const std::string DUMP_SUFFIX = ".dump";

bool GetCoresTuple(const std::string &dumpDir, std::set<CoreNameAndPreFixPair> &coresNamePair);
void GetCoreNameFromDumpFile(std::map<CoreNameAndPreFixPair, std::vector<std::string>> &coresMap,
    const std::string &dumpFilePath);
}

#endif // MSOPT_SIM_DUMP_PARSER_TASK_H
