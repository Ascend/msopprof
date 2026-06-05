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



#ifndef MSOPT_RUNNING_DATA_PARSE_H
#define MSOPT_RUNNING_DATA_PARSE_H

#include <utility>

#include "profiling/op_prof_data_parse.h"
#include "parse/data_parser/instr_parser/real_time_instr_parser.h"
#include "prof_injection/packet.h"
#include "cache_parser/icache_parser.h"
#include "parse/plugin/plugin_manager.h"
#include "parse/data_parser/mte_log_parser/mte_log_parser.h"
#include "parse/data_calculator/mte_log_calculator.h"
#include "parse/data_visualizer/mte_log_visualizer/mte_log_visualizer.h"
#include "parse/data_parser/parser_utils/parse_pc_code.h"

namespace Profiling {
namespace Parse {
class RealTimeICacheParserPlugin : public PluginInterface {
public:
    RealTimeICacheParserPlugin(DataCenter& dataCenter,
                               ChipProductType chipType) : PluginInterface(dataCenter, chipType) {};
    PluginErrorCode Entry() override;
    void DependencyRegister() override
    {
        RegisterPluginName("RealTimeCacheParser");
        RegisterMandatoryDb({});
        RegisterChip({ChipProductType::ASCEND910B_SERIES, ChipProductType::ASCEND910_93_SERIES});
    }
private:
    void ParseLine(const IcacheParseInfoForRealTime &info);
    void ParseScalar(const IcacheParseInfoForRealTime &info);
    std::map<std::string, scalarHeadCache> scalarMap_;
    std::map<std::string, std::vector<MergeInfo>> cacheInstrMap_;
};

class RealTimeICacheParser : public RealTimeLogParer {
public:
    explicit RealTimeICacheParser(RealTimeSimParseContext context);
    ~RealTimeICacheParser() = default;
    void SetICacheLog(const Profiling::IcacheParseInfoForRealTime &iCacheParseInfo);
    void Start() override
    {
        dataCenter_.Clear();
        dataCenter_.DataStreamRegister<IcacheParseInfoForRealTime>();
        pluginManager_.RunAllPluginsNoBlock();
    }
    void Stop() override
    {
        auto dataStream = dataCenter_.GetStreamPtr<IcacheParseInfoForRealTime>();
        if (dataStream != nullptr) {
            dataStream->Shutdown();
        }
        pluginManager_.WaitForStop();
    }
private:
    std::shared_ptr<RealTimeICacheParserPlugin> realTimeICacheParserPlugin_;
};

class RealTimeMteParserPlugin : public MteLogParser {
public:
    RealTimeMteParserPlugin(DataCenter &dataCenter, ChipProductType chipType)
        : MteLogParser(dataCenter, chipType)
    {
        static const uint16_t MAX_CORE_NUM = 100U;
        mteLogInstrVec_.resize(MAX_CORE_NUM);
    }
    PluginErrorCode Entry() override;
    void DependencyRegister() override
    {
        RegisterPluginName("RealTimeMteParser");
        RegisterMandatoryDb({});
        RegisterChip({ChipProductType::ASCEND910B_SERIES, ChipProductType::ASCEND910_93_SERIES});
    }
private:
    std::vector<Parse::MteLogInstrMap> mteLogInstrVec_;
};

class RealTimeMteParser : public RealTimeLogParer {
public:
    explicit RealTimeMteParser(RealTimeSimParseContext context);
    ~RealTimeMteParser() = default;
    void SetMteLog(const Common::DvcMteLog &mteLog);
    void Start() override
    {
        dataCenter_.Clear();
        dataCenter_.DataStreamRegister<Common::DvcMteLog>();
        pluginManager_.RunAllPluginsNoBlock();
    }
    void Stop() override
    {
        auto dataStream = dataCenter_.GetStreamPtr<Common::DvcMteLog>();
        if (dataStream != nullptr) {
            dataStream->Shutdown();
        }
        pluginManager_.WaitForStop();
    }
    void MteProcessAfterExit(const std::shared_ptr<Profiling::Parse::DataCenter> &InteDataCenterPtr);
private:
    std::shared_ptr<RealTimeMteParserPlugin> realTimeMteParserPlugin_;
};

class RealTimeCcuParserPlugin : public PluginInterface {
public:
    RealTimeCcuParserPlugin(DataCenter& dataCenter,
        ChipProductType chipType) : PluginInterface(dataCenter, chipType) {};
        PluginErrorCode Entry() override;
    void DependencyRegister() override
    {
        RegisterPluginName("RealTimeCcuParser");
        RegisterMandatoryDb({});
        RegisterChip({ChipProductType::ASCEND910B_SERIES, ChipProductType::ASCEND910_93_SERIES});
    }
private:
    std::map<std::string, scalarHead> ccuInstrMap_;
};

class RealTimeCcuParser : public RealTimeLogParer {
public:
    explicit RealTimeCcuParser(RealTimeSimParseContext context);
    ~RealTimeCcuParser() = default;
    void SetCcuLog(const CcuParseInfoForRealTime &ccuLog);
    void Start() override
    {
        dataCenter_.Clear();
        dataCenter_.DataStreamRegister<CcuParseInfoForRealTime>();
        pluginManager_.RunAllPluginsNoBlock();
    }
    void Stop() override
    {
        auto dataStream = dataCenter_.GetStreamPtr<CcuParseInfoForRealTime>();
        if (dataStream != nullptr) {
            dataStream->Shutdown();
        }
        pluginManager_.WaitForStop();
    }
private:
    std::shared_ptr<RealTimeCcuParserPlugin> realTimeCcuParserPlugin_;
};

class RealTimeDataParser {
public:
    explicit RealTimeDataParser(RealTimeSimParseContext context) : context_(std::move(context)),
        realTimeInstrParser_(context_), realTimeICacheParser_(context_), realTimeMteParser_(context_), realTimeCcuParser_(context_) {}
    ~RealTimeDataParser()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (afterExitProcessThr_.joinable()) {
            afterExitProcessThr_.join();
        }
        if (pc2CodeThr_.joinable()) {
            pc2CodeThr_.join();
        }
    }
    void Start(const std::string &outputPath, const std::string &kernelName);
    void Stop();

    void SetInstrLog(const Common::DvcInstrLog &dvcInstrLog);
    void SetPopInstrLog(const Common::DvcInstrLog &dvcInstrLog);
    void SetICacheLog(const Common::DvciCacheLog &iCacheLog);
    void SetMteLog(const Common::DvcMteLog &dvcMteLog);
    void SetCcuLog(const Common::DvcCcuLog &ccuLog);
    void InsertScalar(std::map<std::string, std::shared_ptr<Profiling::Parse::DataCenter>> &dataCenterMap);
    void InsertCache(std::map<std::string, std::shared_ptr<Profiling::Parse::DataCenter>> &dataCenterMap);
private:
    void ProcessAfterKernelExit();
    void GetPc2Code();

    RealTimeSimParseContext context_;
    std::string outputPath_;
    std::string kernelName_;
    RealTimeInstrParser realTimeInstrParser_;
    RealTimeICacheParser realTimeICacheParser_;
    RealTimeMteParser realTimeMteParser_;
    RealTimeCcuParser realTimeCcuParser_;
    std::atomic<bool> isStop_ {true};
    std::thread afterExitProcessThr_;
    std::thread pc2CodeThr_;
    std::mutex mtx_;
    std::shared_ptr<ParsePcCode> pc2Code_;
};
}
}
#endif // MSOPT_RUNNING_DATA_PARSE_H
