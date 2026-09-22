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
#include <set>
#include <unistd.h>

#include "elf_helper.h"
#include "filesystem.h"
#include "log.h"

namespace Utility {

namespace {
constexpr char const *RUNTIME_CAMODEL_SO = "libruntime_camodel.so";
constexpr char const *PEM_DAVINCI_SO = "libpem_davinci.so";
constexpr char const *DRV_CAMODEL_SO = "libnpu_drv_camodel.so";
constexpr Elf64_Half HIIPU_MACHINE = 0x1029;
// CANN 9.2 中已验证的 dav-3510 ELF 标志：0x990000 用于内置 Ascend950 kernel，
// 0x9a0000 用于当前 ASC 编译器生成的 dav-3510 kernel。
const std::set<Elf64_Word> ASCEND950_KERNEL_FLAGS = {0x990000, 0x9a0000};
const std::set<std::string> SIMULATOR_NEEDED_LIBRARIES = {
    RUNTIME_CAMODEL_SO,
    DRV_CAMODEL_SO,
    PEM_DAVINCI_SO,
};

std::string ExpandOrigin(const std::string &searchPath, const std::string &binaryPath) {
    std::string binaryParent = GetParentPath(binaryPath);
    std::string expandedPath = ReplaceSubStr(searchPath, "${ORIGIN}", binaryParent);
    return ReplaceSubStr(expandedPath, "$ORIGIN", binaryParent);
}

bool GetSimulatorLibPathFromDynamicInfo(
    const ElfDynamicInfo &dynamicInfo, const std::string &binaryPath, std::string &librarySearchPath) {
    // 通用层只返回 ELF 原始信息；依赖库白名单和 dav_3510 目录判断属于 Ascend 仿真业务。
    bool hasSimulatorDependency = std::any_of(
        dynamicInfo.neededLibraries.begin(), dynamicInfo.neededLibraries.end(), [](const std::string &neededLibrary) {
            return SIMULATOR_NEEDED_LIBRARIES.count(GetFileName(neededLibrary)) != 0;
        });
    if (!hasSimulatorDependency) {
        return false;
    }
    for (const auto &libraryPath : dynamicInfo.searchPaths) {
        std::string expandedLibraryPath = ExpandOrigin(libraryPath, binaryPath);
        if (GetSimulatorLibrarySource(expandedLibraryPath) == SimulatorLibrarySource::LIB) {
            librarySearchPath = expandedLibraryPath;
            return true;
        }
    }
    return false;
}

std::string GetSoFromSearchPath(const std::string &librarySearchPath, const std::string &soName)
{
    size_t begin = 0;
    while (begin <= librarySearchPath.size()) {
        size_t end = librarySearchPath.find(':', begin);
        std::string path = librarySearchPath.substr(begin, end - begin);
        // LD_LIBRARY_PATH 中的空路径项表示当前工作目录，搜索时需要保留该语义。
        if (path.empty()) {
            path = ".";
        }
        std::string realSoPath = Realpath(JoinPath({path, soName}));
        if (!realSoPath.empty()) {
            return realSoPath;
        }
        if (end == std::string::npos) {
            break;
        }
        begin = end + 1;
    }
    return "";
}

}

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
    std::regex pattern("(Ascend\\d{3}[0-9a-zA-Z_]{0,8}|dav_\\d{4})/(lib|camodel)");
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
    return GetSoFromSearchPath(ldEnv, soName);
}

std::string GetSimulatorDirName(const std::string &socVersion)
{
    if (StartsWith(socVersion, "Ascend950")) {
        return "dav_3510";
    }
    if (StartsWith(socVersion, "Ascend910_93")) {
        return "dav_2201";
    }
    return socVersion;
}

std::string GetSocVersionBySimDir(const std::string &simDirName)
{
    auto it = CHIP_TO_DEFAULT_SOC.find(simDirName);
    return it == CHIP_TO_DEFAULT_SOC.end() ? simDirName : it->second;
}

std::string GetSimulatorLibrarySearchPath(const std::string &socVersion)
{
    if (socVersion.empty()) {
        const char *ldLibraryPath = getenv("LD_LIBRARY_PATH");
        return ldLibraryPath == nullptr ? "" : ldLibraryPath;
    }
    std::string ascendHomePath;
    if (!GetAscendHomePath(ascendHomePath)) {
        return "";
    }
    std::string simulatorName = GetSimulatorDirName(socVersion);
    // A5 默认从 camodel 目录加载仿真库并启用实时回调解析；其余平台仍使用 lib 目录。
    std::string libraryDir = StartsWith(socVersion, "Ascend950") ? "camodel" : "lib";
    return JoinPath({ascendHomePath, "tools/simulator", simulatorName, libraryDir});
}

std::string GetSimulatorRuntimePath(const std::string &librarySearchPath) {
    return GetSoFromSearchPath(librarySearchPath, RUNTIME_CAMODEL_SO);
}

SimulatorLibrarySource GetSimulatorLibrarySource(const std::string &librarySearchPath)
{
    if (librarySearchPath.empty()) {
        return SimulatorLibrarySource::UNKNOWN;
    }
    std::string runtimePath = GetSimulatorRuntimePath(librarySearchPath);
    std::string pemPath = GetSoFromSearchPath(librarySearchPath, PEM_DAVINCI_SO);
    if (runtimePath.empty() || pemPath.empty()) {
        return SimulatorLibrarySource::UNKNOWN;
    }
    std::string runtimeParent = GetParentPath(runtimePath);
    if (runtimeParent.empty() || runtimeParent != GetParentPath(pemPath)) {
        LogWarn("Simulator runtime and PEM libraries are not from the same directory, runtime: [%s], PEM: [%s]",
                runtimePath.c_str(), pemPath.c_str());
        return SimulatorLibrarySource::UNKNOWN;
    }
    std::string directoryName = GetFileName(runtimeParent);
    if (directoryName == "camodel") {
        return SimulatorLibrarySource::CAMODEL;
    }
    if (directoryName == "lib") {
        return SimulatorLibrarySource::LIB;
    }
    return SimulatorLibrarySource::UNKNOWN;
}

bool GetAscend950SimulatorLibPath(const std::string &binaryPath, std::string &librarySearchPath) {
    librarySearchPath.clear();
    std::string realBinaryPath = Realpath(binaryPath);
    if (realBinaryPath.empty()) {
        return false;
    }
    // 这里解析的是宿主应用的动态依赖和 RPATH/RUNPATH，用于识别它实际链接的仿真器目录。
    // kernel .o 的平台识别由 DetectAscend950Kernel 读取 e_flags，二者用途不同。
    ElfDynamicInfo dynamicInfo;
    std::string detectedSearchPath;
    if (!ReadElfDynamicInfo(realBinaryPath, dynamicInfo) ||
        !GetSimulatorLibPathFromDynamicInfo(dynamicInfo, realBinaryPath, detectedSearchPath)) {
        return false;
    }
    std::string runtimePath = GetSimulatorRuntimePath(detectedSearchPath);
    std::string libraryPath = GetParentPath(runtimePath);
    if (GetFileName(libraryPath) != "lib" || GetFileName(GetParentPath(libraryPath)) != "dav_3510") {
        return false;
    }
    librarySearchPath = detectedSearchPath;
    return true;
}

bool DetectAscend950Kernel(const std::string &kernelPath, bool &isAscend950) {
    isAscend950 = false;
    Elf64_Ehdr header{};
    if (!ReadElfHeader(kernelPath, header)) {
        return false;
    }
    // 精确匹配已经验证的 dav-3510 编码；其他合法 flags 保持原有兼容行为，不按数值范围推断。
    isAscend950 = header.e_machine == HIIPU_MACHINE && ASCEND950_KERNEL_FLAGS.count(header.e_flags) != 0;
    return true;
}

}  // namespace Utility
