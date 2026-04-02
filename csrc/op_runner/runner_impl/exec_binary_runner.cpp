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

#include <spawn.h>
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

    auto cmd = ToRawCArgv(executeCmd);

    // 构建环境变量：为空时继承父进程的 environ，否则合并自定义环境变量
    std::vector<std::string> envpStorage;
    std::vector<char *> rawEnvp;
    char **envp = environ;
    if (!envs.empty()) {
        JoinWithSystemEnv(envs, envpStorage, true);
        rawEnvp = ToRawCArgv(envpStorage);
        envp = rawEnvp.data();
    }

    int ret = posix_spawnp(&pid_, executeCmd[0].c_str(), nullptr, nullptr, cmd.data(), envp);
    if (ret != 0) {
        char errBuf[256];
        strerror_r(ret, errBuf, sizeof(errBuf));
        LogError("posix_spawnp failed: %s", errBuf);
        return false;
    }

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
                    " The existing data has been saved for subsequent analysis", status);
            return true;
        }
        if (WIFSIGNALED(status)) {
            LogWarn("Child process killed by signal %d", WTERMSIG(status));
        } else if (WIFEXITED(status)) {
            LogWarn("Child process exited with return value %d", WEXITSTATUS(status));
        }
        return false;
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
