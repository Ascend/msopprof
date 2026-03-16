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

#ifndef MSOPT_MTE_LOG_PARSER_H
#define MSOPT_MTE_LOG_PARSER_H

#include "parse/data_table/mte_throughput_table.h"
#include "parse/plugin/plugin_interface.h"
namespace Profiling {
namespace Parse {
class MteLogParser : public PluginInterface {
public:
    explicit MteLogParser(DataCenter &dataCenter, Parse::MteLogCfg &mteLogCfg)
        : PluginInterface(dataCenter, mteLogCfg.chipType), dumpFile_(mteLogCfg.dumpFile), coreId_(mteLogCfg.coreId) {}
    explicit MteLogParser(DataCenter &dataCenter, ChipProductType chipType)
        : PluginInterface(dataCenter, chipType) {}
    PluginErrorCode Entry() override;
    void DependencyRegister() override
    {
        RegisterPluginName("MteLogParser");
        RegisterMandatoryDb({});
        RegisterChip({ChipProductType::ASCEND910B_SERIES, ChipProductType::ASCEND910_93_SERIES,
            ChipProductType::ASCEND950_SERIES});
    }
protected:
    void ParseMTELogLine(const Common::DvcMteLog &mteLog, MteLogInstrMap &mteLogInstrMap) const;
private:
    void ParseMTELogLine(std::string &line, MteLogInstrMap &mteLogInstrMap) const;
    bool MatchIf(const std::string &line, std::unordered_map<std::string, std::string> &ifMap) const;
    bool MatchName(const std::string &name, std::unordered_map<std::string, std::string> &ifMap) const;
    void ParseGMIf(MteLogInstrMap &mteLogInstrMap, std::unordered_map<std::string, std::string> &ifMap) const;
    void ParseOthIf(MteLogInstrMap &mteLogInstrMap, std::unordered_map<std::string, std::string> &ifMap) const;
    bool MatchIf(const Common::DvcMteLog &mteLog, std::unordered_map<std::string, std::string> &ifMap) const;
    std::string dumpFile_;
    uint32_t coreId_ = 0;
};
}
}

#endif