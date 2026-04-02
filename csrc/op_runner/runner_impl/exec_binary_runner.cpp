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


#include "exec_binary_runner.h"

#include <sys/wait.h>
#include <unistd.h>
#include <functional>

#include "log.h"
#include "timer.h"
#include "cmd_execute.h"
#include "profiling/op_prof_task.h"
#include "profiling/op_prof_data_parse.h"

using namespace std;
using namespace Utility;

namespace OpRunner {

bool ExecBinaryRunner::Run(const std::vector<std::string>& executeCmd, const std::map<std::string, std::string> &envs,
                           int32_t timeout)
{
    if (executeCmd.empty()) {
        return false;
    }
    pid_ = fork();
    if (pid_ < 0) {
        return false;
    } else if (pid_ == 0) {
        auto cmd = ToRawCArgv(executeCmd);

        if (envs.empty()) {
            execvp(executeCmd[0].c_str(), cmd.data());
            exit(-1);
        } else {
            vector<string> envp;
            JoinWithSystemEnv(envs, envp, true);
            execvpe(executeCmd[0].c_str(), cmd.data(), ToRawCArgv(envp).data());
            exit(-1);
        }
    } else {
        Timer timer;
        if (timeout > 0) {
            timer.Start(static_cast<uint32_t>(timeout), [this] { KillBinaryProcess(); });
        }

        int status;
        waitpid(pid_, &status, 0);
        if (timeout > 0) {
            timer.Stop();
        }
        if (status != 0) {
            if (Profiling::Task::inExitMode || Profiling::Task::killAdvance) {
                LogWarn("Child process exited with status %d, The process is manually stopped."
                        " The existing optimization data has been saved for subsequent analysis", status);
                return true;
            } else {
                LogWarn("Child process exited with status %d", status);
            }
            return false;
        }
    }
    return true;
}

void ExecBinaryRunner::KillBinaryProcess() const
{
    LogInfo("The timeout has reached and the application will be forcibly killed.");
    Profiling::Task::inExitMode = true;
    Profiling::DataParse::inExitMode = true;
    kill(pid_, SIGINT);
    std::this_thread::sleep_for(std::chrono::seconds(3));  // if not killed, kill forcibly after sleep 3s.
    kill(pid_, SIGKILL);
}
}
