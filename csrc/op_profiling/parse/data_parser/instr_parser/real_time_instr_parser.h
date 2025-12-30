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


#ifndef MSOPT_REAL_TIME_INSTR_PARSER_H
#define MSOPT_REAL_TIME_INSTR_PARSER_H

#include "instr_parser.h"
#include "instr_log_parser.h"
#include "pop_log_parser.h"
#include "parse/plugin/plugin_manager.h"

namespace Profiling {
namespace Parse {
class RealTimeInstrParserPlugin : public SimDataParser {
public:
    RealTimeInstrParserPlugin(DataCenter& dataCenter,
                              SimDataParserConfig config) : SimDataParser(dataCenter, config) {};
    // coreName, InstrLogParser
    std::unordered_map<std::string, InstrLogParser> instrLogParsers_;

    PluginErrorCode Entry() override;
    void DependencyRegister() override
    {
        RegisterPluginName("RealTimeInstrParser");
        RegisterMandatoryDb({});
        RegisterChip({Common::ChipProductType::ASCEND910B_SERIES, Common::ChipProductType::ASCEND910_93_SERIES});
    }
};

class RealTimePopParserPlugin : public SimDataParser {
public:
    RealTimePopParserPlugin(DataCenter& dataCenter, SimDataParserConfig config) : SimDataParser(dataCenter, config) {};
    std::mutex mtx_;
    std::unordered_map<std::string, PopLogParser> popLogParsers_;

    PluginErrorCode Entry() override;
    void DependencyRegister() override
    {
        RegisterPluginName("RealTimeInstrPopParser");
        RegisterMandatoryDb({});
        RegisterChip({Common::ChipProductType::ASCEND910B_SERIES, Common::ChipProductType::ASCEND910_93_SERIES});
    }
};
class RealTimeInstrMergeParser : public InstrParser {
public:
    explicit RealTimeInstrMergeParser(DataCenter &dataCenter,
                                      SimDataParserConfig& config) : InstrParser(dataCenter, config) {};
    PluginErrorCode Entry() override;
    void DependencyRegister() override
    {
        RegisterPluginName("RealTimeInstrMergeParser");
        RegisterMandatoryDb({});
        RegisterChip({Common::ChipProductType::ASCEND910B_SERIES, Common::ChipProductType::ASCEND910_93_SERIES});
    }
    std::string coreName_;
};

class RealTimeInstrParser : public RealTimeLogParer {
public:
    explicit RealTimeInstrParser(RealTimeSimParseContext context);
    ~RealTimeInstrParser() = default;
    void SetInstrLog(const Profiling::InstrParseInfoForRealTime &instrParseInfo);
    void SetPopInstrLog(const Profiling::PoppedInstrParseInfoForRealTime &popParseInfo);
    void Start() override;
    void Stop() override
    {
        auto instrDataStream = dataCenter_.GetStreamPtr<InstrParseInfoForRealTime>();
        if (instrDataStream != nullptr) {
            instrDataStream->Shutdown();
        }
        auto popDataStream = dataCenter_.GetStreamPtr<PoppedInstrParseInfoForRealTime>();
        if (popDataStream != nullptr) {
            popDataStream->Shutdown();
        }
        pluginManager_.WaitForStop();
    }
    void Merge(std::map<std::string, std::shared_ptr<Profiling::Parse::DataCenter>> &dateCenterMap);
private:
    bool IsSkipSetLog(const std::string &coreName);
    std::shared_ptr<RealTimeInstrParserPlugin> realTimeInstrParserPlugin_;
    std::shared_ptr<RealTimePopParserPlugin> realTimePopParserPlugin_;
    std::set<PhysicalAndLogicalPair> coreToLogiCore_;
};
}
}
#endif // MSOPT_REAL_TIME_INSTR_PARSER_H
