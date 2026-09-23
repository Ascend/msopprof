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
        if (Utility::StartsWith(simSocVersion, "Ascend950DT")) {
            env["soc_version"] = "Ascend950DT";
        }
        auto it = SOC_STRING_TO_CHIP_PRODUCT.find(simSocVersion);
        auto chipType = (it == SOC_STRING_TO_CHIP_PRODUCT.end()) ? ChipProductType::UNKNOWN_PRODUCT_TYPE : it->second;
        // 显式 SoC 决定默认仿真器搜索目录；未显式指定时保留用户当前的 LD_LIBRARY_PATH。
        std::string simulatorLibrarySearchPath = Utility::GetSimulatorLibrarySearchPath(profConfig->socVersion_);
        if (GetProductSeriesType(chipType) == ChipProductType::ASCEND950_SERIES) {
            // A5 默认使用 --soc-version 对应的 camodel 实时回调模式。仅直接执行应用二进制，并且该
            // 二进制的动态依赖和 RPATH/RUNPATH 指向 dav_3510/lib 时，才切换到历史离线 dump 模式。
            std::string binaryLibrarySearchPath;
            profConfig->dump_ = profConfig->config_.empty() && !profConfig->cmd_.empty() &&
                Utility::GetAscend950SimulatorLibPath(profConfig->cmd_.front(), binaryLibrarySearchPath);
            // rawCallbackDump_ 只表示是否把实时回调的指令数据额外落盘；离线 dump 模式不产生该回调。
            profConfig->rawCallbackDump_ = !profConfig->dump_ && profConfig->rawCallbackDump_;
            if (profConfig->dump_) {
                simulatorLibrarySearchPath = binaryLibrarySearchPath;
                Utility::LogInfo(
                    "Ascend950 simulator library is loaded from lib; legacy offline dump mode is preserved");
            } else {
                Utility::LogInfo("Ascend950 simulator library is loaded from camodel; real-time parsing is enabled%s",
                    profConfig->rawCallbackDump_ ? " with instruction callback dump" : "");
            }
        }
        // dump_ 决定解析通道：false 开启 CA 日志回传和实时解析，true 保留文件落盘后的离线解析。
        env["ENABLE_CA_LOG_TRANS"] = profConfig->dump_ ? "false" : "true";
        env["ENABLE_CA_RAW_INSTR_DUMP"] = profConfig->rawCallbackDump_ ? "true" : "false";
        if (!profConfig->dump_) {
            // 实时模式在启动子进程前创建解析器，后续由注入事件回调持续写入指令和缓存数据。
            std::set<int> coreIdSet = Utility::SplitString<int32_t>(profConfig->coreId_, '|');
            realTimeSimParseContext_ = RealTimeSimParseContext{coreIdSet,
                profConfig->aicMetrics_.IsOn(ProfMetrics::RESOURCE_CONFLICT_RATIO), chipType, profConfig->aicMetrics_};
            realTimeDataParser_ = std::make_shared<Parse::RealTimeDataParser>(realTimeSimParseContext_);
            needRegisterEvent_ = true;
        }
        if (isSetSocVersion) {
            // 子进程最终加载哪套仿真库以这里写入的搜索路径为准，后续配置生成也读取该值判断模式。
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
            // config 模式通过 kernel-launcher 拉起 kernel。临时目录中的 libruntime.so 指向注入库，
            // 原仿真器搜索路径追加在其后，保证 kernel-launcher 的其他依赖仍能解析。
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
    bool PrepareAscend950DtLogDir(std::string &logPath, bool &removeLogDir) const;
    void CleanupAscend950DtLogDir(const std::string &logPath, bool removeLogDir) const;
    bool RuntimeToTargetLib(
        std::map<std::string, std::string> &env, const std::string &runtimePath, const std::string &targetPath) const;
    bool pmSamplingEnable_ = false;
};

}

#endif // __MSOPPROF_PROFILING_SIMULATOR_TASK_H__
