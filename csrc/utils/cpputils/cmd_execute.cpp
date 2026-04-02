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


#include "filesystem.h"
#include "cmd_execute.h"

#include <set>
#include <sys/wait.h>

using namespace std;

namespace Utility {

std::vector<char *> ToRawCArgv(std::vector<std::string> const &argv)
{
    std::vector<char *> rawArgv;
    for (auto const &arg: argv) {
        rawArgv.emplace_back(const_cast<char *>(arg.data()));
    }
    rawArgv.emplace_back(nullptr);
    return rawArgv;
}

void JoinWithSystemEnv(std::map<std::string, std::string> envs,
                       std::vector<std::string> &outEnv, bool combineMode)
{
    static set<std::string> needJoinEnv = {"LD_PRELOAD", "LD_LIBRARY_PATH", "PATH"};
    /// append/combined system envs into program defined envs
    char **systemEnvs = environ;
    while (*systemEnvs != nullptr) {
        if (!combineMode) {
            // only add new env
            outEnv.emplace_back(*systemEnvs);
            systemEnvs++;
            continue;
        }
        string tmpSysEnvs = *systemEnvs;
        size_t pos = tmpSysEnvs.find('=');
        if (pos == string::npos) {
            systemEnvs++;
            continue;
        }
        string key = tmpSysEnvs.substr(0, pos);
        auto it = envs.find(key);
        if (it != envs.end() && needJoinEnv.find(it->first) != needJoinEnv.end()) {
            if (pos == (tmpSysEnvs.size() - 1)) {
                // handle env value is empty
                tmpSysEnvs.replace(0, pos + 1, it->first + "=" + it->second);
            } else {
                tmpSysEnvs.replace(0, pos + 1, it->first + "=" + it->second + ":");
            }
            envs.erase(it);
        }
        outEnv.emplace_back(tmpSysEnvs);
        systemEnvs++;
    }

    /// append envs not in system envs
    for (auto &pair: envs) {
        string envStr = pair.first + "=" + pair.second;
        outEnv.push_back(envStr);
    }
}

bool CmdExecuteWithOutput(const std::vector<std::string> &executeCmd, std::string &outputName)
{
    std::vector<char *> cmd;
    for (const auto &arg:executeCmd) {
        cmd.emplace_back(const_cast<char *>(arg.data()));
    }
    cmd.emplace_back(const_cast<char *>(outputName.data()));
    cmd.emplace_back(nullptr);
    pid_t pid = vfork();
    if (pid < 0) {
        return false;
    } else if (pid == 0) {
        if (execvp(executeCmd[0].c_str(), cmd.data()) < 0) {
            perror("execvp");
        }
        exit(-1);
    } else {
        int status;
        waitpid(pid, &status, 0);
        if (status != 0) {
            Utility::LogError("Child process exited with status %d, return value is %d", WIFEXITED(status),
                              WEXITSTATUS(status));
            return false;
        }
    }
    return true;
}

bool CmdExecute(const std::vector<std::string> &executeCmd,
                const std::map<std::string, std::string> &envs,
                std::string &output)
{
    int fds[2];
    if (pipe(fds) == -1 || executeCmd.empty()) {
        return false;
    }
    pid_t pid = vfork();
    if (pid < 0) {
        close(fds[0]);
        close(fds[1]);
        return false;
    } else if (pid == 0) {
        /// subprocess to run gather task, redirect stdout to pipe
        dup2(fds[1], STDOUT_FILENO);
        close(fds[0]);
        close(fds[1]);

        std::vector<char *> cmd = ToRawCArgv(executeCmd);

        if (envs.empty()) {
            if (execvp(executeCmd[0].c_str(), cmd.data()) < 0) {
                perror("execvp");
            }
            exit(-1);
        } else {
            vector<string> envp;
            JoinWithSystemEnv(envs, envp, true);
            if (execvpe(executeCmd[0].c_str(), cmd.data(), ToRawCArgv(envp).data()) < 0) {
                perror("execvpe");
            }
            exit(-1);
        }
    } else {
        /// parent process wait gather task finish
        close(fds[1]);
        char buf[256] = {'\0'};
        int nbytes = 0;
        for (output.clear(); (nbytes = read(fds[0], buf, sizeof(buf) - 1)) > 0;) {
            output.append(buf, nbytes);
        }
        close(fds[0]);

        int status;
        waitpid(pid, &status, 0);
        if (status != 0) {
            LogError("Child process exited with status %d, return value is %d", WIFEXITED(status), WEXITSTATUS(status));
            return false;
        }
    }
    return true;
}
}