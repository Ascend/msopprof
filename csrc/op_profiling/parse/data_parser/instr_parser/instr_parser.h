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


#ifndef MSOPT_INSTR_PARSER_H
#define MSOPT_INSTR_PARSER_H

#include <vector>
#include <string>
#include <unordered_map>
#include "parse/data_parser/instr_parser/instr_log_parser.h"
#include "parse/data_parser/instr_parser/pop_log_parser.h"
#include "parse/data_parser/sim_data_parser_config.h"
#include "parse/data_parser/sim_data_parser.h"

namespace Profiling {
namespace Parse {

class InstrParser : public SimDataParser {
public:
    explicit InstrParser(DataCenter &dataCenter, SimDataParserConfig& config) : SimDataParser(dataCenter, config) {};
    PluginErrorCode Entry() override;
    void DependencyRegister() override
    {
        RegisterPluginName("InstrParser");
        RegisterMandatoryDb({});
        RegisterChip({Common::ChipProductType::ALL_PRODUCT_TYPE});
    }
protected:
    bool MergeLog(const InstrLogParser &instrLogParser, const PopLogParser &popParser,
                  MatchMode matchMode = MatchMode::PC_MATCH);

private:
    size_t GetPruneSize(std::vector<PoppedInstrParseInfo> &instrPoppedVec, std::vector<InstrParseInfo> &instrVec) const;
    bool MergeInstr(const std::unordered_map<uint64_t, std::vector<InstrParseInfo>> &instrMap,
                    const std::unordered_map<uint64_t, std::vector<PoppedInstrParseInfo>> &popMap,
                    std::vector<MergeInfo> &mergeList, MatchMode matchMode);
    void MergeInstrByPc(const std::unordered_map<uint64_t, std::vector<InstrParseInfo>> &instrMap,
                        const std::unordered_map<uint64_t, std::vector<PoppedInstrParseInfo>> &popMap,
                        std::vector<MergeInfo> &mergeList);
    void MergeInstrById(const std::unordered_map<uint64_t, std::vector<InstrParseInfo>> &instrMap,
                        const std::unordered_map<uint64_t, std::vector<PoppedInstrParseInfo>> &popMap,
                        std::vector<MergeInfo> &mergeList);
    void InitMergeItem(const PoppedInstrParseInfo& instrPopped, const InstrParseInfo& instr,
                       MergeInfo& mergeItem) const;
};
}
}
#endif // MSOPT_INSTR_PARSER_H
