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


#include "op_device_prof.h"

#include "smart_pointer.h"
#include "profiling/device/data_parse/device_data_parse.h"
#include "profiling/device/run/device_task.h"
#include "common/hal_helper.h"
#include "ascend_helper.h"
#include "filesystem.h"
#include "common/defs.h"

using namespace Utility;

namespace Profiling {
OpDeviceProf::OpDeviceProf(const Common::ProfArgs &profArgs): OpProf(profArgs),
    chipType_(Common::HalHelper::Instance().GetPlatformType()), replayMode_(Common::ReplayModeMap.at(profArgs.argReplayMode)),
    coreId_(profArgs.argCoreId), instrTimelinePipe_(profArgs.argInstrTimelinePipe)
{
    pmuEventsId_.LoadPmuVec(profArgs.argAicMetrics, chipType_, profArgs.argReplayMode);
    StringToNum<uint16_t>(profArgs.argWarmUp, warmUp_);
}

Profiling::TaskPtr OpDeviceProf::GetTask()
{
    Profiling::TaskPtr task = nullptr;
    std::string opprofPath = Utility::GetMsopprofPath();
    std::string tmpPath = Utility::JoinPath({output_, Common::TMP_DUMP});
    if (opprofPath.empty()) {
        LogWarn("Ascend Toolkit path or required lib not found. No device task will generate.");
    } else if (Utility::IsExist(tmpPath)) {
        LogError("Tmp output path maybe not secure, %s already exist", tmpPath.c_str());
    } else {
        task = MakeUnique<DeviceTask>(Common::MSPROF_DUMPFILE_PREFIX, *this);
    }
    return task;
}

std::unique_ptr<DataParse> OpDeviceProf::GetDataParser()
{
    return MakeUnique<DeviceDataParse>(chipType_, pmuEventsId_, aicMetrics_, kernelName_, customDotJson_);
}
}
