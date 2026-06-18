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


#include "op_prof.h"

#include "log.h"
#include "smart_pointer.h"
#include "op_prof_task.h"
#include "common/hal_helper.h"
#include "common/defs.h"
#include "ascend_helper.h"

using namespace Utility;

namespace Profiling {
ChipProductType GetChipType(const Common::ProfArgs &args)
{
    using namespace Common;
    if (args.runMode == "device") {
        Common::ChipType chipType = Common::HalHelper::Instance().GetPlatformType();
        auto iter = CHIP_ARCHITECTURE_TO_PRODUCT_SERIES.find(chipType);
        if (iter == CHIP_ARCHITECTURE_TO_PRODUCT_SERIES.end()) {
            return ChipProductType::UNKNOWN_PRODUCT_TYPE;
        }
        return iter->second;
    }
    std::string simSocVersion = args.argSocVersion;
    if (simSocVersion.empty()) {
        if (!Utility::GetSocVersionFromEnvVar(simSocVersion)) {
            Utility::LogDebug("Can not get socVersion from LD_LIBRARY_PATH");
            return ChipProductType::UNKNOWN_PRODUCT_TYPE;
        }
    }
    auto it = SOC_STRING_TO_CHIP_PRODUCT.find(simSocVersion);
    auto chipType = (it == SOC_STRING_TO_CHIP_PRODUCT.end()) ? ChipProductType::UNKNOWN_PRODUCT_TYPE : it->second;
    auto chipSeries = GetProductSeriesType(chipType);
    return chipSeries;
}

OpProf::OpProf(const Common::ProfArgs &args)
{
    StringToNum<uint16_t>(args.argLaunchCount, launchCount_);
    StringToNum<uint16_t>(args.argLaunchSkipBeforeMatch, launchSkipBeforeMatch_);
    kill_ = (args.argKill == "on");
    isMstxEnable_ = (args.argMstx == "on");
    mstxInclude_ = args.argMstxInclude;
    aicMetrics_ = args.argAicMetrics;
    kernelName_ = args.argKernelName;
    cmd_ = args.cmd;
    output_ = args.argOutput;
    config_ = args.argConfig;
    customDotJson_ = args.argCustomInput;
    kernelConfig_ = args.kernelConfig;
    if (args.argDump != "on") {
        ChipProductType chipType = GetChipType(args);
        if (chipType == ChipProductType::ASCEND910B_SERIES ||
            chipType == ChipProductType::ASCEND910_93_SERIES) {
            dump_ = false;
        }
    }
}

bool OpProf::Run()
{
    if (!RunTask()) {
        LogWarn("Running task failed, data parsing start");
    }

    if (!RunDataParse()) {
        LogError("Profiling data parse failed. Please check");
        return false;
    }
    return true;
}

bool OpProf::RunTask()
{
    TaskPtr task = GetTask();
    if (task != nullptr && task->Run()) {
        LogInfo("Profiling running finished. All task success.");
    } else {
        LogWarn("Profiling running finished. May cause generating performance files to fail.");
        return false;
    }
    dump_ = !task->isCaLogTransStartSuc_;
    return true;
}

bool OpProf::RunDataParse(bool needMoveDir)
{
    std::unique_ptr<DataParse> dataParse = std::move(GetDataParser());
    if (dataParse == nullptr) {
        LogError("Data parse task generate failed.");
        return false;
    }
    bool res = dataParse->Execute(output_);
    if (needMoveDir) {
        dataParse->SingleKernelOutputReorganize(output_);
    }
    if (res && needMoveDir) {
        LogInfo("Profiling data parse finished.");
    }
    return res;
}

}  // namespace Profiling
