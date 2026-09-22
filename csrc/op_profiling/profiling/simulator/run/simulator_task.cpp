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


#ifndef __MSOPPROF_PROFILING_SIMULATOR_TASK_GENERATOR_H__
#define __MSOPPROF_PROFILING_SIMULATOR_TASK_GENERATOR_H__

#include "simulator_task.h"

#include "common/defs.h"
#include "op_runner.h"
#include "prof_injection/injection_event.h"
#include "ascend_helper.h"

using namespace Utility;
using namespace Common;
using namespace std;

namespace Profiling {
bool SimulatorTask::Run()
{
    // 实时模式先注册回调，再启动注入消息处理线程和子进程，避免丢失启动阶段的仿真数据。
    RegisterRunningEvent();
    string socInfo;
    if (!isSetSocVersion) {
        socInfo = "use simulator in LD_LIBRARY_PATH";
    } else {
        std::string replaceSocVersion = Utility::ReplaceSubStr(simSocVersion, "_", " ");
        socInfo = "use simulator " + replaceSocVersion + " by --soc-version";
    }
    LogInfo("Running simulation task: %s, %s", taskName.c_str(), socInfo.c_str());

    if (!PreProcess()) {
        return false;
    }
    Task::execStatus = ExecStatus::RUNNING;
    ProfStub::InjectionEvent::Instance().StartDisposeClientAsk(profMessage_, profConfig_);
    bool ret = OpRunner::RunOpBinary(cmd, env, timeout_);
    ProfStub::InjectionEvent::Instance().Stop();
    if (realTimeDataParser_ != nullptr) {
        realTimeDataParser_->Stop();
    }
    Task::execStatus = ExecStatus::STOPPED;
    return ret;
}

bool SimulatorTask::RuntimeToTargetLib(std::map<std::string, std::string> &env, const std::string &runtimePath,
                                       const std::string &targetPath) const

{
    if (runtimePath.empty() || !MkdirRecusively(runtimePath)) {
        LogError("Sym link failed when dispose ca path");
        return false;
    }
    // kernel-launcher 固定依赖 libruntime.so，这里让它先加载 msopprof 注入库。
    std::string soName = JoinPath({runtimePath, "libruntime.so"});
    std::string ldEnv = env["LD_LIBRARY_PATH"];
    if (IsExist(soName)) {
        RemoveAll(soName);
    }
    if (symlink(targetPath.c_str(), soName.c_str()) != 0) {
        LogError("Symbol link runtime to simulator failed");
        return false;
    }
    env["LD_LIBRARY_PATH"] = runtimePath;
    if (!ldEnv.empty()) {
        // 注入目录只提供入口库，原搜索路径仍负责解析仿真 Runtime、PEM 等后续依赖。
        env["LD_LIBRARY_PATH"] += ":" + ldEnv;
    }
    LogDebug("Symbol link runtime to simulator success, so path is %s, simulator path is %s",
             soName.c_str(), targetPath.c_str());
    return true;
}

bool SimulatorTask::PreProcess()
{
    if (!isSetSocVersion && !CheckSimulatorSoExist()) {
        Utility::LogError("Failed to load simulator so, please check your LD_LIBRARY_PATH");
        return false;
    }
    if (!tmpPath_.empty() && !CreateTaskDir(tmpPath_)) {
        return false;
    }
    profMessage_.isSimulator = true;
    if (strcpy_s(profMessage_.mstxProfConfig.mstxEnabledMessage, sizeof(profMessage_.mstxProfConfig.mstxEnabledMessage),
                 mstxEnabledMessageString.c_str()) != 0) {
        LogError("Subprocess control message generate failed");
        return false;
    }
    profMessage_.pmSamplingEnable = pmSamplingEnable_;
    profMessage_.mstxProfConfig.isMstxEnable = isMstxEnable;
    // 配置生成根据最终 LD_LIBRARY_PATH 区分 A5 camodel 实时模式和 lib 离线模式。
    CreateCamodelConfig(pmSamplingEnable_);
    unsetenv("ASCEND_RT_VISIBLE_DEVICES");
    if (opRunMode == OpRunnerMode::RUN_KERNEL) {
        // 将用户 config 解析后的 kernel 信息转换为 kernel-launcher 使用的内部 JSON。
        nlohmann::json jsonData;
        if (!GenOpConfig(jsonData)) {
            return false;
        }
    }
    return true;
}
}
#endif // __MSOPPROF_PROFILING_SIMULATOR_TASK_GENERATOR_H__
