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

#include "arg_checker.h"

#include <vector>
#include <unordered_set>

#include "common/prof_args.h"
#include "ascend_helper.h"
#include "filesystem.h"
#include "common/hal_helper.h"
#include "common/defs.h"

using namespace Common;
using namespace Utility;

namespace Parser {
constexpr uint32_t LAUNCH_COUNT_MAX_LENGTH = 4;
constexpr int32_t MAX_LAUNCH_COUNT = 5000;
constexpr int32_t MAX_LAUNCH_SKIP_NUMBER = 1000;
constexpr int32_t MAX_WARM_UP_TIMES = 500;
constexpr int32_t MAX_PARSE_CORE_ID = 49;
constexpr int32_t MAX_TIMEOUT = 2880;

// 上板模式下支持的AIC指标，key是指标名称，value是支持的产品类型列表
const AicMetricsSupportMap DEVICE_AIC_METRICS_SUPPORT_MAP{
    {std::string(Common::MsprofMetrics::PIPE_UTILIZATION),        {Common::ChipProductType::ALL_PRODUCT_TYPE}},
    {std::string(Common::MsprofMetrics::ARITHMETIC_UTILIZATION),  {Common::ChipProductType::ALL_PRODUCT_TYPE}},
    {std::string(Common::MsprofMetrics::L2_CACHE),                {Common::ChipProductType::ALL_PRODUCT_TYPE}},
    {std::string(Common::MsprofMetrics::MEMORY),                  {Common::ChipProductType::ALL_PRODUCT_TYPE}},
    {std::string(Common::MsprofMetrics::MEMORY_L0),               {Common::ChipProductType::ALL_PRODUCT_TYPE}},
    {std::string(Common::MsprofMetrics::MEMORY_UB),               {Common::ChipProductType::ALL_PRODUCT_TYPE}},
    {std::string(Common::MsprofMetrics::RESOURCE_CONFLICT_RATIO), {Common::ChipProductType::ALL_PRODUCT_TYPE}},
    {std::string(Common::MsprofMetrics::DEFAULT),                 {Common::ChipProductType::ALL_PRODUCT_TYPE}},
    {std::string(Common::MsprofMetrics::KERNEL_SCALE),            {Common::ChipProductType::ASCEND910B_SERIES,
                                                                   Common::ChipProductType::ASCEND910_93_SERIES,
                                                                   Common::ChipProductType::ASCEND950_SERIES}},
    {std::string(Common::MsprofMetrics::OCCUPANCY),               {Common::ChipProductType::ASCEND910B_SERIES,
                                                                   Common::ChipProductType::ASCEND910_93_SERIES,
                                                                   Common::ChipProductType::ASCEND950_SERIES}},
    {std::string(Common::MsprofMetrics::TIMELINE_DETAIL),         {Common::ChipProductType::ASCEND910B_SERIES,
                                                                   Common::ChipProductType::ASCEND910_93_SERIES}},
    {std::string(Common::MsprofMetrics::ROOFLINE),                {Common::ChipProductType::ASCEND910B_SERIES,
                                                                   Common::ChipProductType::ASCEND910_93_SERIES,
                                                                   Common::ChipProductType::ASCEND310P_SERIES,
                                                                   Common::ChipProductType::ASCEND950_SERIES}},
    {std::string(Common::MsprofMetrics::BASIC_INFO),              {Common::ChipProductType::ASCEND910B_SERIES,
                                                                   Common::ChipProductType::ASCEND910_93_SERIES,
                                                                   Common::ChipProductType::ASCEND950_SERIES,
                                                                   Common::ChipProductType::ASCEND310P_SERIES}},
    {std::string(Common::MsprofMetrics::SOURCE),                  {Common::ChipProductType::ASCEND910B_SERIES,
                                                                   Common::ChipProductType::ASCEND910_93_SERIES,
                                                                   Common::ChipProductType::ASCEND950_SERIES}},
    {std::string(Common::MsprofMetrics::MEMORYDETAIL),            {Common::ChipProductType::ASCEND910B_SERIES,
                                                                   Common::ChipProductType::ASCEND910_93_SERIES,
                                                                   Common::ChipProductType::ASCEND950_SERIES}},
    {std::string(Common::MsprofMetrics::TIMELINE),                {Common::ChipProductType::ASCEND950_SERIES}},
    {std::string(Common::MsprofMetrics::PCSAMPLING),              {Common::ChipProductType::ASCEND950_SERIES}},
};
// 仿真模式下支持的AIC指标，key是指标名称，value是支持的产品类型列表
const AicMetricsSupportMap SIMULATOR_AIC_METRICS_SUPPORT_MAP{
    {std::string(Common::MsprofMetrics::PIPE_UTILIZATION),        {Common::ChipProductType::ALL_PRODUCT_TYPE}},
    {std::string(Common::MsprofMetrics::RESOURCE_CONFLICT_RATIO), {Common::ChipProductType::ASCEND910B_SERIES,
                                                                   Common::ChipProductType::ASCEND910_93_SERIES,
                                                                   Common::ChipProductType::ASCEND310P_SERIES,
                                                                   Common::ChipProductType::ASCEND950_SERIES}},
    {std::string(Common::MsprofMetrics::PMSAMPLING),              {Common::ChipProductType::ASCEND910B_SERIES,
                                                                   Common::ChipProductType::ASCEND910_93_SERIES,
                                                                   Common::ChipProductType::ASCEND950_SERIES}},
};

ArgChecker::ArgChecker(const std::string &runMode)
{
    checkers_.emplace_back(&ArgChecker::CheckRunModeValid);
    checkers_.emplace_back(&ArgChecker::CheckApplicationValid);
    checkers_.emplace_back(&ArgChecker::CheckOutputPathValid);
    checkers_.emplace_back(&ArgChecker::CheckKernelNameValid);
    checkers_.emplace_back(&ArgChecker::CheckLaunchCount);
    checkers_.emplace_back(&ArgChecker::CheckMstx);
    checkers_.emplace_back(&ArgChecker::CheckMstxInclude);
    checkers_.emplace_back(&ArgChecker::CheckAicMetrics);
    checkers_.emplace_back(&ArgChecker::CheckCoreId);
    checkers_.emplace_back(&ArgChecker::CheckDump);
    if (runMode == "simulator") {
        checkers_.emplace_back(&ArgChecker::CheckExportPathValid);
        checkers_.emplace_back(&ArgChecker::CheckSimSocVersion);
        checkers_.emplace_back(&ArgChecker::CheckTimeout);
    } else {
        checkers_.emplace_back(&ArgChecker::CheckLaunchSkipBeforeMatch);
        checkers_.emplace_back(&ArgChecker::CheckKillAdvance);
        checkers_.emplace_back(&ArgChecker::CheckReplayMode);
        checkers_.emplace_back(&ArgChecker::CheckWarmUp);
    }
}

bool ArgChecker::CheckMetrics(const std::vector<std::string> &metricsVec, const Common::ChipProductType &productType,
                              const AicMetricsSupportMap &supports, std::string &msg) const
{
    auto isSupport = [&supports, &productType] (const std::string &metric) {
        for (const auto &supportType : supports.at(metric)) {
            if (IsChipSeriesTypeValid(productType, supportType)) {
                return true;
            }
        }
        return false;
    };
    for (const auto &metric : metricsVec) {
        if (supports.find(metric) == supports.end()) {
            msg = "Unexpected argument --aic-metrics=" + metric + ", maybe in wrong run mode";
            return false;
        }
        if (isSupport(metric)) {
            continue;
        }
        msg = "Unexpected argument --aic-metrics=" + metric + ", maybe in wrong soc platform";
        return false;
    }
    return true;
}

bool ArgChecker::CheckAicMetrics(const Common::ProfArgs &config, std::string &msg) const
{
    std::vector<std::string> metricVec = config.argAicMetrics.metricVec;
    if (config.runMode == "device") {
        ChipType chipType = Common::HalHelper::Instance().GetPlatformType();
        if (CHIP_ARCHITECTURE_TO_PRODUCT_SERIES.find(chipType) == CHIP_ARCHITECTURE_TO_PRODUCT_SERIES.end()) {
            msg = "chiptype " + std::to_string(static_cast<int>(chipType)) + " not support.";
            return false;
        }
        Common::ChipProductType productType = CHIP_ARCHITECTURE_TO_PRODUCT_SERIES.at(chipType);
        return CheckMetrics(metricVec, productType, DEVICE_AIC_METRICS_SUPPORT_MAP, msg);
    } else if (config.runMode == "simulator") {
        std::string socVersion = config.argSocVersion;
        if (socVersion.empty() && !GetSocVersionFromEnvVar(socVersion)) {
            socVersion = "Ascend910B1";
        }
        Common::ChipProductType productType = Common::GetProductSeriesTypeBySocVersion(socVersion);
        return CheckMetrics(metricVec, productType, SIMULATOR_AIC_METRICS_SUPPORT_MAP, msg);
    }
    return true;
}

bool ArgChecker::Check(const ProfArgs &config, std::string &msg) const
{
    for (auto const &c : checkers_) {
        if (!(this->*c)(config, msg)) {
            return false;
        }
    }
    return true;
}

bool ArgChecker::CheckRunModeValid(const ProfArgs &config, std::string &msg) const
{
    if (config.runMode == "device") {
        if (!HalHelper::Instance().IsSupportPlatform()) {
            msg = "Device profiling is not supported on current chip.";
            return false;
        }
        return true;
    }

    if (config.runMode != "simulator") {
        msg = "unexpected run mode";
        return false;
    }
    if (config.argAicMetrics.isDeviceToSimulator) {
        msg = "--aic-metrics=TimelineDetail is invalid in simulator";
        return false;
    }
    std::vector<std::string> sims;
    if (!GetSimulators(sims)) {
        msg = "get simulators from ascend path failed";
        return false;
    }

    return true;
}

bool ArgChecker::CheckApplicationValid(const ProfArgs &args, std::string &msg) const
{
    /// check if programs be used correctly
    const int32_t inputArgsCount = static_cast<int32_t>(!args.argConfig.empty()) +
                                   static_cast<int32_t>(!args.cmd.empty()) +
                                   static_cast<int32_t>(!args.argExport.empty());
    constexpr int32_t argsCount = 1;
    if (inputArgsCount != argsCount) {
        msg = "Input parameter config, export and application can not be used together or empty at the same time";
        return false;
    } else if (args.cmd.empty()) {
        return true;
    }

    if (IsDir(args.cmd[0]) || !IsExecutable(args.cmd[0])) {
        msg = "application to be profiled is not exist or not executable.";
        return false;
    }

    return true;
}

bool ArgChecker::CheckOutputPathValid(const ProfArgs &config, std::string &msg) const
{
    if (IsExist(config.argOutput) && !IsDir(config.argOutput)) {
        msg = "--output parameter is not a folder but already exist, please check output path is correct.";
        return false;
    }

    std::string errorMsg;
    if (!IsStringCharValid(config.argOutput, errorMsg)) {
        msg = "--output parameter contains " + errorMsg;
        return false;
    }

    if (!PathLenCheckValid(config.argOutput)) {
        msg = "--output parameter length is larger than 200.";
        return false;
    }

    // Search for the created path in the absolute path of the output path and saved in checkPath
    std::vector<std::string> dirs;
    Utility::Split(config.argOutput, std::back_inserter(dirs), Utility::PATH_SEP);
    std::string checkPath;
    for (const auto &dir : dirs) {
        if (dir.empty()) { continue; }
        checkPath.append(Utility::PATH_SEP + dir);
        if (!Utility::IsDir(checkPath)) {
            checkPath.erase(checkPath.size() - dir.size());
            break;
        }
    }
    if (IsSoftLinkRecursively(checkPath)) {
        msg = "soft link is not supported for output dir: " + checkPath;
        return false;
    }
    if (!IsWritable(checkPath)) {
        msg = "output dir is not writable: " + checkPath;
        return false;
    }
    if (!CheckOwnerPermission(checkPath, msg)) {
        return false;
    }
    if (!CheckPermission(checkPath)) {
        msg = "output parent dir permission wrong";
        return false;
    }
    return true;
}

bool ArgChecker::CheckExportPathValid(const ProfArgs &config, std::string &msg) const
{
    if (config.argExport.empty()) {
        return true;
    }
    if (!CheckInputFileValid(config.argExport, "dir")) {
        msg = "In input parameter --export receive parent dir permission wrong";
        return false;
    } else if (!CheckPermission(config.argExport)) {
        msg = "In input parameter --export dir cannot have write permission of group or other users";
        return false;
    }
    return true;
}

bool ArgChecker::CheckKernelNameValid(const Common::ProfArgs &config, std::string &msg) const
{
    if (config.argKernelName.empty()) {
        return true;
    }
    if (!config.argConfig.empty() || !config.argExport.empty()) {
        msg = "--kernel-name only supports application mode";
        return false;
    }

    if (config.argKernelName.size() >= MAX_KERNEL_NAME_LENGTH) {
        msg = "--kernel-name input length exceeds limitation.";
        return false;
    }

    std::set<std::string> kernelNameSet;
    Utility::SplitString(config.argKernelName, '|', kernelNameSet);
    std::regex namePattern("^[A-Za-z0-9_*]+$");
    for (const auto &kernelName : kernelNameSet) {
        if (kernelName.empty() || kernelName.length() > MAX_KERNEL_NAME_LENGTH) {
            msg = "invalid kernel name, name is too long or empty";
            return false;
        }
        if (!std::regex_match(kernelName, namePattern)) {
            msg = "invalid kernel name, name contains unsupported character in name,"
                  "Support characters in one kernel name are: A-Z a-z 0-9 _ *";
            return false;
        }
    }
    return true;
}

bool ArgChecker::CheckLaunchCount(const Common::ProfArgs &config, std::string &msg) const
{
    if (config.argLaunchCount.length() > LAUNCH_COUNT_MAX_LENGTH) {
        msg = "Launch count should in [1, 5000]";
        return false;
    }
    int32_t num {0};
    if (!StringToNum<int32_t>(config.argLaunchCount, num)) {
        msg = "Launch count should be number and within [1, 5000]";
        return false;
    }
    if (num < 1 || num > MAX_LAUNCH_COUNT) { // num should in [1, 5000]
        msg = "Launch count should within [1, 5000]";
        return false;
    }
    return true;
}

bool ArgChecker::CheckLaunchSkipBeforeMatch(const Common::ProfArgs &config, std::string &msg) const
{
    int32_t num {0};
    size_t maxLength = 5;
    if (config.argLaunchSkipBeforeMatch.size() >= maxLength ||
        !StringToNum<int32_t>(config.argLaunchSkipBeforeMatch, num)) {
        msg = "Launch-skip-before-match should be number and within [0, 1000]";
        return false;
    }
    if (num < 0 || num > MAX_LAUNCH_SKIP_NUMBER) {
        msg = "Launch-skip-before-match should within [0, 1000]";
        return false;
    }
    return true;
}

bool ArgChecker::CheckReplayMode(const Common::ProfArgs &config, std::string &msg) const
{
    std::set<std::string> targetModes = {"application", "kernel"};
    ChipType chipType = Common::HalHelper::Instance().GetPlatformType();
    if (chipType == ChipType::ASCEND910B || chipType == ChipType::ASCEND950) {
        targetModes.insert("range");
    }
    if (targetModes.count(config.argReplayMode) == 0) {
        msg = "Replay mode should be " + Join(targetModes.begin(), targetModes.end(), "/");
        return false;
    }
    if (config.argReplayMode == "application" && config.argAicMetrics.isDeviceToSimulator) {
        msg = "--aic-metrics=TimelineDetail is invalid when --replay-mode=application";
        return false;
    }
    if (config.argReplayMode == "range") {
        if (config.argMstx != "on") {
            msg = "--replay-mode=range only support when --mstx=on";
            return false;
        }
        if (config.argAicMetrics.isDeviceToSimulator || config.argAicMetrics.isSource ||
            config.argAicMetrics.isMemoryDetail) {
            msg = "--aic-metrics=TimelineDetail/Source/MemoryDetail is invalid when --replay-mode=range";
            return false;
        }
    }
    return true;
}

bool ArgChecker::CheckKillAdvance(const Common::ProfArgs &config, std::string &msg) const
{
    if (config.argKill != "on" && config.argKill != "off") {
        msg = "Kill should be on/off";
        return false;
    }
    return true;
}

bool ArgChecker::CheckDump(const Common::ProfArgs &config, std::string &msg) const
{
    if (config.argDump != "on" && config.argDump != "off") {
        msg = "--dump should be on/off";
        return false;
    }
    return true;
}

bool ArgChecker::CheckMstx(const Common::ProfArgs &config, std::string &msg) const
{
    if (config.argMstx.empty()) {
        return true;
    }
    if (config.argMstx != "on" && config.argMstx != "off") {
        msg = "--mstx should use on/off";
        return false;
    }
    return true;
}

bool ArgChecker::CheckMstxInclude(const Common::ProfArgs &config, std::string &msg) const
{
    if (config.argMstxInclude.empty()) {
        return true;
    }

    if (config.argMstx == "off") {
        msg = "--mstx-include only support when --mstx=on";
        return false;
    }

    if (config.argMstxInclude.size() >= MAX_KERNEL_NAME_LENGTH) {
        msg = "--mstx-include input length exceeds limitation";
        return false;
    }

    std::set<std::string> messageSet;
    Utility::SplitString(config.argMstxInclude, '|', messageSet);
    for (const auto &message : messageSet) {
        if (!Utility::CheckInputStringValid(message, MAX_MSTX_INCLUDE_NAME_LENGTH)) {
            msg = "invalid include string, include string is too long or use unsupported character in include string. "
                    "Support characters in one message are: A-Z a-z 0-9 _";
            return false;
        }
    }
    return true;
}

bool ArgChecker::CheckSimSocVersion(const Common::ProfArgs &config, std::string &msg) const
{
    if (config.runMode != "simulator" || config.argSocVersion.empty()) {
        return true;
    }
    if (!config.argConfig.empty()) {
        msg = "--soc-version is not effective in config mode";
        return false;
    }
    std::string ascendHomePath;
    if (!GetAscendHomePath(ascendHomePath)) {
        msg = "$ASCEND_HOME_PATH not found";
        return false;
    }
    std::vector<std::string> sims;
    if (!GetFileNames(ascendHomePath + "/tools/simulator", sims)) {
        msg = "get simulator failed, please check $ASCEND_HOME_PATH/tools/simulator";
        return false;
    }
    if (std::count(sims.begin(), sims.end(), config.argSocVersion) == 0) {
        msg = "--soc-version is invalid, please specify a simulator in $ASCEND_HOME_PATH/tools/simulator";
        return false;
    }
    return true;
}

bool ArgChecker::CheckWarmUp(const Common::ProfArgs &config, std::string &msg) const
{
    int32_t num {0};
    if (!StringToNum<int32_t>(config.argWarmUp, num)) {
        msg = "Warm up times should be number and within [0, 500]";
        return false;
    }
    if (num < 0 || num > MAX_WARM_UP_TIMES) {
        msg = "Warm up times should within [0, 500]";
        return false;
    }
    return true;
}

bool ArgChecker::CheckCoreId(const Common::ProfArgs &config, std::string &msg) const
{
    if (config.argCoreId.empty()) {
        return true;
    }

    if (config.argCoreId.size() > MAX_INPUT_STR_LENGTH) {
        msg = "--core-id input length exceeds limitation.";
        return false;
    }

    std::set<std::string> coreSet;
    Utility::SplitString(config.argCoreId, '|', coreSet);
    for (const auto &core: coreSet) {
        uint16_t coreId = 0;
        // core ID should be [0, 49]
        if (!Utility::StringToNum<uint16_t>(core, coreId) || coreId > MAX_PARSE_CORE_ID) {
            msg = "--core-id is invalid, the cores to be parsed should be separated by '|',"
                  " and each core id should be an integer which within [0, 49].";
            return false;
        }
    }
    return true;
}

bool ArgChecker::CheckTimeout(const Common::ProfArgs &config, std::string &msg) const
{
    if (config.argTimeout.empty()) {
        return true;
    }
    uint32_t num {0};
    if (!StringToNum<uint32_t>(config.argTimeout, num) || num == 0 || num > MAX_TIMEOUT) {
        msg = "--timeout is invalid, it should be an integer which within [1, 2880].";
        return false;
    }
    return true;
}
}  // namespace Parser
