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


#include "interface/ms_op_prof.h"
#include "common/prof_args.h"
#include "common/hal_helper.h"
#include "common/runtime_helper.h"
#include "filesystem.h"

int main(int argc, char *argv[], char *env[])
{
    Utility::SetLogLevelByEnvVar();
    Common::ProfArgs args;
    if (!Interface::ProfArgsInit(args, argc, argv, env)) {
        return -1;
    }
    Common::RuntimeHelper::Init(args.runMode == "simulator"); // 初始化RuntimeHelper
    if (args.printVersion) {
        Interface::PrintVersion();
        return 0;
    }
    if (args.printHelp) {
        if (args.runMode == "device") {
            Interface::PrintDeviceHelp(Common::HalHelper::Instance().GetPlatformType());
        } else {
            Interface::PrintSimulatorHelp();
        }
        return 0;
    }
    if (Utility::IsRootUser()) {
        Utility::LogWarn("Currently, the root permission is used, which is unrecommended."
                         " Please ensure the security of the execution environment and programs.");
    }
    Utility::LogInfo("Op profiling analysis start.");
    Interface::ProfilingRun(args);
    Utility::LogInfo("Op profiling finish. Welcome to next use.");

    return 0;
}
