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

#include "ascend_helper.h"

#include <algorithm>
#include <cstdlib>
#include <unistd.h>

#include "filesystem.h"
#include "log.h"

namespace Utility {

bool GetAscendHomePath(std::string &ascendHomePath)
{
    char const *env = getenv("ASCEND_HOME_PATH");
    std::string pathFromEnv = env == nullptr ? "" : env;

    if (pathFromEnv.empty()) {
        LogError("no $ASCEND_HOME_PATH env set, please set it",
                 "source <CANN-install-path>/ascend-toolkit/set_env.sh");
        return false;
    }

    char buf[PATH_MAX];
    if (realpath(pathFromEnv.c_str(), buf) == nullptr) {
        LogError("no such path for CANN: [%s]", pathFromEnv.c_str());
        return false;
    }

    ascendHomePath = buf;
    return true;
}

bool GetSimulators(std::vector<std::string> &simulators)
{
    std::string ascendHomePath;
    if (!GetAscendHomePath(ascendHomePath)) {
        return false;
    }

    std::string simulatorPath = ascendHomePath + "/tools/simulator";
    std::vector<std::string> dirs;
    if (!Utility::ListDir(simulatorPath, std::back_inserter(dirs))) {
        return false;
    }

    std::copy_if(dirs.cbegin(), dirs.cend(), std::back_inserter(simulators),
                 [](std::string const& dir) { return dir.find("Ascend") == 0; });

    return true;
}

std::string GetMsopprofPath()
{
    char opprofPath[PATH_MAX];
    int ret = readlink("/proc/self/exe", opprofPath, sizeof(opprofPath) - 1);
    if (ret != -1) {
        std::string opprofPathDir = opprofPath;
        size_t pos = opprofPathDir.rfind("/bin/msopprof");
        if (pos == std::string::npos) {
            std::string ascendHomePath;
            if (!GetAscendHomePath(ascendHomePath)) {
                LogError("Get msopprof path failed");
                return "";
            }
            return JoinPath({ascendHomePath, "tools", "msopt"});
        }
        opprofPathDir = opprofPathDir.substr(0, pos);
        return opprofPathDir;
    } else {
        LogError("Get msopprof path failed");
        return "";
    }
}

const std::map<std::string, std::string> CHIP_TO_DEFAULT_SOC {
    {"dav_2002", "Ascend310P1"},
    {"dav_2201", "Ascend910B1"},
    {"dav_3510", "Ascend950PR_9599"},
};

bool GetSocVersionFromEnvVar(std::string &socVersion)
{
    std::string ascendHomePath;
    if (!GetAscendHomePath(ascendHomePath)) {
        return false;
    }

    char const *ldEnv = getenv("LD_LIBRARY_PATH");
    if (ldEnv == nullptr) {
        return false;
    }
    std::string pathFromEnv = ldEnv;
    std::vector<std::string> envs;
    SplitString(pathFromEnv, ':', envs);

    std::smatch pathMatch;
    std::regex pattern("(Ascend\\d{3}[0-9a-zA-Z_]{0,8}|dav_\\d{4})/lib");
    RollbackPath(ascendHomePath, 1);
    for (const std::string &path: envs) {
        if (!StartsWith(path, ascendHomePath)) {
            continue;
        }
        if (std::regex_search(path, pathMatch, pattern)) {
            if (StartsWith(pathMatch[1], "Ascend")) {
                socVersion = pathMatch[1];
                return true;
            }
            auto it = CHIP_TO_DEFAULT_SOC.find(pathMatch[1]);
            if (it != CHIP_TO_DEFAULT_SOC.end()) {
                socVersion = it->second;
                return true;
            }     
        }
    }
    return false;
}

// Get the absolute path of so form  LD_LIBRARY_PATH
std::string GetSoFromEnvVar(const std::string &soName)
{
    char const *ldEnv = getenv("LD_LIBRARY_PATH");
    if (ldEnv == nullptr) {
        return "";
    }
    std::string pathFromEnv = ldEnv;
    std::vector<std::string> envs;
    SplitString(pathFromEnv, ':', envs);
    for (const std::string &path : envs) {
        std::string soPath = JoinPath({path.c_str(), soName});
        std::string realSoPath = Realpath(soPath);
        if (realSoPath.empty()) {
            continue;
        }
        return realSoPath;
    }
    return "";
}

}  // namespace Utility
