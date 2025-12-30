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


#ifndef MSOPT_INSTR_LOG_PARSER_H
#define MSOPT_INSTR_LOG_PARSER_H

#include <regex>
#include <string>
#include <vector>
#include "profiling/simulator/data_parse/sim_dump_parser_task.h"
#include "parse/data_parser/sim_data_parser_config.h"

namespace Profiling {
namespace Parse {

struct UserMark {
    std::string markName;
    bool start;
    bool end;
};

class InstrLogParser {
public:
    explicit InstrLogParser(SimDataParserConfig &config, std::string coreName) : dataParserConfig_(config),
        coreName_(std::move(coreName)) {};
    explicit InstrLogParser(SimDataParserConfig &config) : dataParserConfig_(config) {};
    bool ParseDumpLog(MatchMode matchMode = MatchMode::PC_MATCH);
    void ParseRealTimeDumpLog(InstrParseInfo &instrInfo);
    void DisposeUserMark();
    inline void SetCoreName(const std::string &coreName)
    {
        coreName_ = coreName;
    }
    const std::unordered_map<uint64_t, std::vector<InstrParseInfo>> &GetInstrLog() const { return instrMap_; }

    const std::vector <MergeInfo> &GetUserMarkInstr() const { return userMarkInstr_; }

    const std::map <std::string, std::vector<UserMarkInfo>> &GetUserMarkInfo() const { return userMarkMap_; }

private:
    void DeleteUserMarkWithoutEnd();
    void ParseLine(const std::string &line, MatchMode matchMode);
    void DisposeLine(InstrParseInfo &instrInfo, MatchMode matchMode = MatchMode::PC_MATCH);
    bool GetUserMark(const std::map <std::string, std::vector<UserMarkInfo>> &userMarkMap,
                     const InstrParseInfo &instrInfo, std::map <std::string, uint32_t> &markCnt,
                     std::pair <uint64_t, uint64_t> &userMarkTimePoint) const;
    void MergeUserMark();
    void UpdateMark(InstrParseInfo &instrInfo);

    std::map <std::string, uint64_t> instrRuleNamePos_ = {
        {"tick",   2},
        {"pc",     3},
        {"pipe",   5},
        {"id",     9},
        {"name",   10},
        {"detail", 13}
    };
    std::regex instrMatchPattern_ = std::regex(
        "(\\[info\\] )?\\[([0-9]+)\\]\\s?\\(PC: (0x[0-9a-f]{1,16})\\)(@CORE[0-9]{1,2})?\\s?"
        "([A-Za-z0-3_]+)( ISSUE| IB ISSUE)?\\s*(: \\(Binary: 0x[0-9a-f ]{8,34}\\) (\\(ID: ([0-9]{6,15})\\)\\s)?"
        "([0-9a-zA-Z_-]*)((\\(|\\s{2})([\\[\\]\\|a-zA-Z0-9_, :=/#-]*)[) ]?)?(, instr ID is: [0-9]+.)?)?.*");
    std::unordered_map<uint64_t, std::vector<InstrParseInfo>> instrMap_;
    // 记录当前是哪一个userMark,最多只有10个userMark
    std::string userMarkName_;
    // key是userMark的名称，对应记录不同userMark的起点终点
    std::map<std::string, std::vector<UserMarkInfo>> userMarkMap_;
    std::set<std::string> invalidMarkName_;
    // 记录不同userMark的状态，0表示查找终点的tick，1表示查找起点的tick
    std::map<std::string, bool> userMarkStatus_;
    std::vector<InstrParseInfo> userMarkParseInfo_;
    std::vector<MergeInfo> userMarkInstr_;
    SimDataParserConfig dataParserConfig_;
    std::string coreName_;
};

}
}
#endif // MSOPT_INSTR_LOG_PARSER_H
