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


#ifndef __MSOPPROF_PROFILING_SIMULATOR_TASK_H__
#define __MSOPPROF_PROFILING_SIMULATOR_TASK_H__

#include <string>
#include <utility>
#include <vector>
#include "ascend_helper.h"
#include "filesystem.h"
#include "profiling/simulator/op_sim_prof.h"
#include "ascend_helper.h"
#include "common/defs.h"
#include "parse/data_parser/real_time_data_parser.h"

namespace Profiling {
class SimulatorTask : public Task {
public:
    SimulatorTask(std::string taskName, OpProf &config) : Task(std::move(taskName))
    {
        using namespace Common;
        auto* profConfig = dynamic_cast<OpSimProf *>(&config);
        std::string opprofPath = Utility::GetMsopprofPath();
        std::string opprofInjectionLib = Utility::JoinPath({opprofPath,
            Path::MSOPPROF_INJECTION_LIB_PATH_FROM_MSOPPROF});

        simSocVersion = profConfig->socVersion_;
        isSetSocVersion = IsSetSocVersion(simSocVersion);
        auto it = SOC_STRING_TO_CHIP_PRODUCT.find(simSocVersion);
        auto chipType = (it == SOC_STRING_TO_CHIP_PRODUCT.end()) ? ChipProductType::UNKNOWN_PRODUCT_TYPE : it->second;
        std::string simulatorLibrarySearchPath =
            Utility::GetSimulatorLibrarySearchPath(profConfig->socVersion_);
        auto librarySource = Utility::GetSimulatorLibrarySource(simulatorLibrarySearchPath);
        if (GetProductSeriesType(chipType) == ChipProductType::ASCEND950_SERIES) {
            bool enableRealTime = librarySource == Utility::SimulatorLibrarySource::CAMODEL;
            profConfig->dump_ = !enableRealTime;
            profConfig->rawCallbackDump_ = enableRealTime && profConfig->rawCallbackDump_;
            if (librarySource == Utility::SimulatorLibrarySource::CAMODEL) {
                Utility::LogInfo("Ascend950 simulator library is loaded from camodel; real-time parsing is enabled%s",
                    profConfig->rawCallbackDump_ ? " with instruction callback dump" : "");
            } else if (librarySource == Utility::SimulatorLibrarySource::LIB) {
                Utility::LogInfo(
                    "Ascend950 simulator library is loaded from lib; legacy offline dump mode is preserved");
            } else {
                Utility::LogWarn(
                    "Cannot determine Ascend950 simulator mode from the selected camodel library directory");
            }
        }
        env["ENABLE_CA_LOG_TRANS"] = profConfig->dump_ ? "false" : "true";
        env["ENABLE_CA_RAW_INSTR_DUMP"] = profConfig->rawCallbackDump_ ? "true" : "false";
        if (!profConfig->dump_) {
            std::set<int> coreIdSet = Utility::SplitString<int32_t>(profConfig->coreId_, '|');
            realTimeSimParseContext_ = RealTimeSimParseContext{coreIdSet,
                profConfig->aicMetrics_.IsOn(ProfMetrics::RESOURCE_CONFLICT_RATIO), chipType, profConfig->aicMetrics_};
            realTimeDataParser_ = std::make_shared<Parse::RealTimeDataParser>(realTimeSimParseContext_);
            needRegisterEvent_ = true;
        }
        if (isSetSocVersion) {
            env["LD_LIBRARY_PATH"] = simulatorLibrarySearchPath;
        }
        tmpPath_ = Utility::JoinPath({profConfig->output_, "device0", TMP_DUMP});
        outputPath = profConfig->output_;
        env["CAMODEL_SOC_VERSION"] = simSocVersion;
        env["CAMODEL_LOG_PATH"] = tmpPath_;
        env["IS_SIMULATOR_ENV"] = "true";
        isMstxEnable = profConfig->isMstxEnable_;
        mstxEnabledMessageString = profConfig->mstxInclude_;
        timeout_ = profConfig->timeout_;
        Common::ProfConfig profCf(outputPath, profConfig->kernelName_, profConfig->launchCount_,
            profConfig->launchSkipBeforeMatch_);
        profConfig_ = profCf;
        pmSamplingEnable_ = profConfig->aicMetrics_.pmSamplingEnable;
        env["LD_PRELOAD"] = opprofInjectionLib + ":libruntime_camodel.so";
        env["TASK_QUEUE_ENABLE"] = "0";
        env["GE_INIT_DISABLE"] = "1";
        if (!profConfig->config_.empty()) {
            RuntimeToTargetLib(env, env["CAMODEL_LOG_PATH"], opprofInjectionLib);
            opRunMode = std::string(OpRunnerMode::RUN_KERNEL);
            kernelConfig = profConfig->kernelConfig_;
            cmd.emplace_back(Utility::JoinPath({Utility::GetMsopprofPath(), Path::KERNEL_LAUNCHER_PATH_FROM_MSOPPROF}));
        } else {
            cmd = profConfig->cmd_;
        }
    }
    ~SimulatorTask() override = default;
    bool Run() override;

private:
    bool PreProcess();
    bool RuntimeToTargetLib(std::map<std::string, std::string> &env,
                            const std::string &runtimePath, const std::string &targetPath) const;
    bool pmSamplingEnable_ = false;
};

}

#endif // __MSOPPROF_PROFILING_SIMULATOR_TASK_H__
