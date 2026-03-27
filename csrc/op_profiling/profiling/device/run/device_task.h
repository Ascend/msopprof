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


#ifndef __MSOPPROF_PROFILING_DEVICE_TASK_H__
#define __MSOPPROF_PROFILING_DEVICE_TASK_H__

#include <string>
#include <vector>

#include "profiling/op_prof_task.h"
#include "data_format.h"
#include "common/prof_args.h"
#include "profiling/device/op_device_prof.h"
#include "ascend_helper.h"
#include "common/defs.h"
#include "filesystem.h"
#include "common/hal_helper.h"
#include "common/runtime_helper.h"
#include "common/dbi_defs.h"

namespace Profiling {
class DeviceTask : public Task {
public:
    DeviceTask(std::string taskName, OpProf &config) : Task(std::move(taskName))
    {
        using namespace Common;
        std::string opprofInjectionLib = Utility::JoinPath({Utility::GetMsopprofPath(),
                                                            Path::MSOPPROF_INJECTION_LIB_PATH_FROM_MSOPPROF});
        auto* profConfig = dynamic_cast<OpDeviceProf *>(&config);
        if (profConfig->aicMetrics_.isDeviceToSimulator) {
            simSocVersion = RuntimeHelper::Instance().GetSocVersion();
            std::string ascendHomePath;
            Utility::GetAscendHomePath(ascendHomePath);
            env["LD_LIBRARY_PATH"] = Utility::JoinPath({ascendHomePath, "tools/simulator", simSocVersion, "lib"});
            auto it = SOC_STRING_TO_CHIP_PRODUCT.find(simSocVersion);
            auto chipType = (it == SOC_STRING_TO_CHIP_PRODUCT.end()) ?
                ChipProductType::UNKNOWN_PRODUCT_TYPE : it->second;
            if (!profConfig->dump_) {
                // all set/wait flags are enabled.
                std::set<int> coreIdSet = Utility::SplitString<int32_t>(profConfig->coreId_, '|');
                realTimeSimParseContext_ = RealTimeSimParseContext{coreIdSet, true, chipType, profConfig->aicMetrics_};
                realTimeDataParser_ = std::make_shared<Parse::RealTimeDataParser>(realTimeSimParseContext_);
                env["ENABLE_CA_LOG_TRANS"] = "true";
                needRegisterEvent_ = true;
            }
        }
        tmpPath_ = Utility::JoinPath({profConfig->output_, Common::TMP_DUMP});
        outputPath = profConfig->output_;
        env["DEVICE_PROF_DUMP_PATH"] = tmpPath_;
        env["TASK_QUEUE_ENABLE"] = "0";
        env["MSOPPROF_EXE_PATH"] = Utility::GetMsopprofPath();
        Profiling::Task::killAdvance = profConfig->kill_;
        chipType_ = Common::HalHelper::Instance().GetPlatformType();
        metrics_ = profConfig->aicMetrics_;
        profMaxTimes_ = profConfig->launchCount_;
        profSkipTimes_ = profConfig->launchSkipBeforeMatch_;
        replayMode_ = profConfig->replayMode_;
        pmuValue_ = profConfig->pmuEventsId_;
        isDeviceToSimulator_ = profConfig->aicMetrics_.isDeviceToSimulator;
        profWarmUpTimes_ = profConfig->warmUp_;
        isMstxEnable = profConfig->isMstxEnable_;
        mstxEnabledMessageString = profConfig->mstxInclude_;
        opRunMode = Common::OpRunnerMode::EXECUTE_BINARY;
        env["LD_PRELOAD"] = opprofInjectionLib;
        if (profConfig->config_.empty()) {
            kernelName_ = profConfig->kernelName_;
            cmd = profConfig->cmd_;
        } else {
            opRunMode =std::string(Common::OpRunnerMode::RUN_KERNEL);
            kernelConfig = profConfig->kernelConfig_;
            cmd.emplace_back(Utility::JoinPath({Utility::GetMsopprofPath(), Path::KERNEL_LAUNCHER_PATH_FROM_MSOPPROF}));
        }
        profConfig_ = {outputPath, kernelName_, profMaxTimes_, profSkipTimes_};
    }
    ~DeviceTask() override = default;
    bool Run() override;

private:
    bool PreProcess();
    void ProcessApplication(uint32_t count);
    uint32_t GetReplayTimes();

    std::string kernelName_;
    std::vector<ProfDBIType> dbiTypes;
    Common::ProfMetricsAbilityConfig metrics_{};
    Common::ChipType chipType_{Common::ChipType::END_TYPE};
    uint16_t profMaxTimes_ {1};
    uint16_t profWarmUpTimes_ {0};
    uint16_t profSkipTimes_ {0};
    ReplayMode replayMode_ {ReplayMode::KERNEL};
    uint32_t replayCount_{7};
    Common::PmuEventsId pmuValue_;
    bool isDeviceToSimulator_ {false};
};

}

#endif // __MSOPPROF_PROFILING_DEVICE_TASK_H__
