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

#ifndef MSOPT_CCU_CACHE_LOG_PARSER_H
#define MSOPT_CCU_CACHE_LOG_PARSER_H

#include <regex>
#include <string>
#include <vector>
#include "parse/data_parser/sim_data_parser_config.h"
#include "parse/data_parser/sim_data_parser.h"
#include "parse/data_center/data_center.h"
#include "parse/plugin/plugin_interface.h"
#include "profiling/simulator/data_parse/sim_dump_parser_task.h"

namespace Profiling {
namespace Parse {

class CcuParser : public SimDataParser {
public:
    explicit CcuParser(DataCenter &dataCenter, SimDataParserConfig& config) : SimDataParser(dataCenter, config) {}
    PluginErrorCode Entry() override;
    void DependencyRegister() override
    {
        RegisterPluginName("CcueParser");
        RegisterMandatoryDb({});
        RegisterChip({ChipProductType::ASCEND910B_SERIES, ChipProductType::ASCEND910_93_SERIES});
    }
private:
    void ParseLine(const std::string &line);
    scalarHead ccuInstrMap_;
    std::map<std::string, uint64_t> scalarRuleNamePos_ = {{"tick", 1}, {"pc", 3}};
    std::regex instrMatchPattern_ = std::regex("\\[info\\] ([0-9]+): (is_single_issue instr, pc|ccu do issue success.issue isa pc ):(0x[0-9a-f]+)");
};
}
}

#endif // MSOPT_CCU_CACHE_LOG_PARSER_H
