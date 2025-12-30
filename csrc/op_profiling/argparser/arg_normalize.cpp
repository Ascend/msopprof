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


#include "arg_normalize.h"
#include "utils.h"
#include "filesystem.h"

namespace Parser {
ArgNormalize::ArgNormalize(void)
{
    normalizeFunc_.emplace_back(&ArgNormalize::NormalizeKernelName);
    normalizeFunc_.emplace_back(&ArgNormalize::NormalizeMstxMessage);
    normalizeFunc_.emplace_back(&ArgNormalize::NormalizeApp);
    normalizeFunc_.emplace_back(&ArgNormalize::NormalizeOutput);
    normalizeFunc_.emplace_back(&ArgNormalize::NormalizeExport);
    normalizeFunc_.emplace_back(&ArgNormalize::NormalizeConfig);
}

bool ArgNormalize::Normalize(Common::ProfArgs &config, std::string &msg) const
{
    for (const auto &func : normalizeFunc_) {
        if (!(this->*func)(config, msg)) {
            return false;
        }
    }
    return true;
}

bool ArgNormalize::NormalizeKernelName(Common::ProfArgs &config, std::string &msg) const
{
    (void)msg;
    Utility::TrimBlank(config.argKernelName);
    return true;
}

bool ArgNormalize::NormalizeMstxMessage(Common::ProfArgs &config, std::string &msg) const
{
    (void)msg;
    Utility::TrimBlank(config.argMstxInclude);
    return true;
}

bool ArgNormalize::NormalizeApp(Common::ProfArgs &config, std::string &msg) const
{
    (void)msg;
    // null input application and apps
    if (config.argApplication.empty() && config.argApps.empty()) {
        return true;
    }
    // --application is not recommended.
    if (!config.argApplication.empty()) {
        Utility::LogWarn("The option \"--application\" will be deprecated in future version. "
                "It is recommended to specify <app> [arguments] instead.");
    } else {
        config.argApplication = config.argApps;  // apps will replace --application
    }

    config.cmd = StringToArgv(config.argApplication);
    // 输入 / ./ ../ 则不拼接直接校验
    if (config.cmd.empty() ||
        config.cmd[0].rfind("/", 0) == 0 ||
        config.cmd[0].rfind("./", 0) == 0 ||
        config.cmd[0].rfind("../", 0) == 0) {
        return true;
    }

    std::string exeCmd = Utility::FindExecutableCommand(config.cmd[0]);
    if (!exeCmd.empty()) {
        config.cmd[0] = exeCmd;
    }
    return true;
}

bool ArgNormalize::NormalizeOutput(Common::ProfArgs &config, std::string &msg) const
{
    return NormalizeOptionPath(config.argOutput, msg, "output");
}

bool ArgNormalize::NormalizeExport(Common::ProfArgs &config, std::string &msg) const
{
    if (config.argExport.empty()) {
        return true;
    }
    return NormalizeOptionPath(config.argExport, msg, "export");
}

bool ArgNormalize::NormalizeConfig(Common::ProfArgs &config, std::string &msg) const
{
    if (config.argConfig.empty()) {
        return true;
    }
    return NormalizeOptionPath(config.argConfig, msg, "config");
}

bool ArgNormalize::NormalizeOptionPath(std::string &configOpt, std::string &msg, std::string msgOption) const
{
    std::string absOptionPath = Utility::GetAbsolutePath(configOpt);
    if (absOptionPath.empty()) {
        msg = "Normalize option \"--" + msgOption + "\" failed.";
        return false;
    }
    configOpt = absOptionPath;
    return true;
}

} // namespace Parser
