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


#include "runtime_helper.h"
#include <dlfcn.h>
#include "log.h"
#include "runtime/runtime.h"
#include "ascend_helper.h"
#include "filesystem.h"
using namespace Utility;
namespace Common {
using getSocVersionType = rtError_t(*)(char *, const uint32_t);
getSocVersionType getSocVersionFunc {nullptr};
bool RuntimeHelper::isSim_ = 0;
RuntimeHelper &RuntimeHelper::Instance(void)
{
    static RuntimeHelper instance;
    return instance;
}

// 需要在Instance前调用Init，否则isSim_为false
void RuntimeHelper::Init(bool isSim)
{
    isSim_ = isSim;
}

RuntimeHelper::RuntimeHelper() noexcept
{
    std::string ascendHomePath;
    if (!GetAscendHomePath(ascendHomePath)) {
        LogError("Failed to get ASCEND_HOME_PATH. Please source ${HOME}/Ascend/ascend-toolkit/set_env.sh first!");
        return;
    }
    std::string soName;
    if (isSim_) {
        soName = GetSoFromEnvVar("libruntime_camodel.so");
        if (soName.empty() || !CheckInputFileValid(soName, "so")) {
            LogWarn("Can't find valid libruntime_camodel.so, please check your LD_LIBRARY_PATH");
        }
    } else {
        soName = ascendHomePath + "/lib64/libruntime.so";
    }
    handle_ = dlopen(soName.c_str(), RTLD_LAZY);
    if (handle_ != nullptr) {
        getSocVersionFunc = reinterpret_cast<getSocVersionType>(dlsym(handle_, "rtGetSocVersion"));
    }
}

RuntimeHelper::~RuntimeHelper()
{
    if (handle_ != nullptr) {
        dlclose(handle_);
        handle_ = nullptr;
    }
}

std::string RuntimeHelper::GetSocVersion() const
{
    static std::string socVersion;
    if (!socVersion.empty()) {
        return socVersion;
    }
    constexpr uint64_t socVersionBufLen = 64ULL;
    char socVersionArr[socVersionBufLen] = "";
    if (getSocVersionFunc == nullptr || getSocVersionFunc(socVersionArr, sizeof(socVersionArr)) != 0) {
        Utility::LogWarn("Get soc version from runtime failed");
        return socVersion;
    }
    socVersion = socVersionArr;
    return socVersion;
}
}  // namespace Common
