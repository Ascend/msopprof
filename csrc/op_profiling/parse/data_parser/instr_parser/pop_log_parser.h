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


#ifndef MSOPT_POP_LOG_PARSER_H
#define MSOPT_POP_LOG_PARSER_H

#include <vector>
#include <string>
#include <regex>
#include "parse/data_parser/sim_data_parser_config.h"
#include "profiling/simulator/data_parse/sim_dump_parser_task.h"

namespace Profiling {
namespace Parse {

class PopLogParser {
public:
    explicit PopLogParser(SimDataParserConfig& config) : dataParserConfig_(config) {};
    bool ParseDumpLog(MatchMode matchMode = MatchMode::PC_MATCH);
    void ParseRealTimeDumpLog(PoppedInstrParseInfo &poppedInstrParseInfo);
    const std::unordered_map<uint64_t, std::vector<PoppedInstrParseInfo>> &GetPopLog() const { return popMap_;}
    const std::string &GetLogicalName() const { return logicalCorename_; }
    int GetCoreId() const;
    int IsGetCoreId() const
    {
        if (useLogicalCore_ && logicalCorename_.empty()) {
            return false;
        }
        return true;
    }
private:
    void ParseLine(const std::string &line, MatchMode matchMode);
    void ParseA5Detail(PoppedInstrParseInfo &poppedInstr);
    void DisposeLine(PoppedInstrParseInfo &poppedInstrParseInfo, MatchMode matchMode = MatchMode::PC_MATCH);
    bool LineFilter(std::smatch &lineMatch);
    bool LineFilter(const PoppedInstrParseInfo &poppedInstrParseInfo);
    void GetLogicalCoreName(const std::string &name, const std::string &detail);

    std::map<std::string, uint64_t> instrPoppedRuleNamePos_ = {
        {"tick", 2}, {"pc", 3},  {"pipe", 5}, {"id", 9}, {"name", 10}, {"detail", 13}, {"pop_str", 16}
    };
    std::regex instrPoppedMatchPattern_ = std::regex(
        "(\\[info\\] )?\\[([0-9]+)\\]\\s?\\(PC: (0x[0-9a-f]{1,16})\\)(@CORE[0-9]{1,2})?\\s?"
        "([A-Za-z0-3_]+)( ISSUE| IB ISSUE)?\\s*(: \\(Binary: 0x[0-9a-f ]{8,34}\\) (\\(ID: ([0-9]{6,15})\\)\\s)?"
        "([0-9a-zA-Z_-]*)((\\(|\\s{2})([\\[\\]\\|a-zA-Z0-9_, :=/#-]*)[) ]?)?(, instr ID is: [0-9]+.)?"
        "(\\.? ((poped from IQ)|(pop success) [0-9]+))?)?.*");
    // stallCyc range:1-15, yeild range:0-1, inv range:0-1, warpId range:0-63, bundleId range:0-15, schId range:0-3
    std::regex a5DetailPattern_ = std::regex("\\[stallCyc:([0-9abcdef]{1,2})\\],\\[yeild:[0,1]\\],\\[inv:[0,1]\\],"
        "\\[warpId:([0-9]{1,2})\\],\\[bundleId:[0-9abcdef]{1,2}\\],\\[schId:([0-3])\\]");
    std::regex logicCorePattern_ = std::regex("([A-Z0-9/:]{0,10})=([0-9xXa-fA-F]{0,4}),SPR:BLOCKID");
    std::regex logicSubCorePattern_ = std::regex("([A-Z0-9/:]{0,10})=([0-9xXa-fA-F]{0,4}),SPR:SUBBLOCKID");
    std::unordered_map<uint64_t, std::vector<PoppedInstrParseInfo>> popMap_;
    SimDataParserConfig dataParserConfig_;
    std::string logicalCorename_ = {};
    PoppedInstrListInfo poppedInstrListInfo_;
    bool useLogicalCore_ = false;
    int coreId_ = 0;
    int subCoreId_ = 0;
    bool findCoreId_ = false;
    bool findSubCoreId_ = false;
};
}
}
#endif // MSOPT_POP_LOG_PARSER_H
