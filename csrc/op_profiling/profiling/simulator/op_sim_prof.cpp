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

#include "op_sim_prof.h"

#include "smart_pointer.h"
#include "profiling/simulator/data_parse/sim_data_parse.h"
#include "filesystem.h"
#include "ascend_helper.h"
#include "profiling/simulator/run/simulator_task.h"
#include "common/defs.h"

using namespace Utility;

namespace Profiling {
OpSimProf::OpSimProf(const Common::ProfArgs &profArgs) : OpProf(profArgs) {
    socVersion_ = profArgs.argSocVersion;
    exportPath_ = profArgs.argExport;
    coreId_ = profArgs.argCoreId;
    if (!profArgs.argTimeout.empty()) {
        int32_t minuteTime = 0;
        Utility::StringToNum<int32_t>(profArgs.argTimeout, minuteTime);
        timeout_ = minuteTime * 60; // 60s
    }
}
Profiling::TaskPtr OpSimProf::GetTask() {
    Profiling::TaskPtr task = nullptr;
    std::string ascendHomePath;
    std::string tmpPath = JoinPath({output_, "device0", Common::TMP_DUMP});
    if (!GetAscendHomePath(ascendHomePath)) {
        LogWarn("Ascend path not Found. No simulator task generated.");
    } else if (IsExist(tmpPath)) {
        LogError("Output path maybe no secure, %s already exist", tmpPath.c_str());
    } else {
        task = MakeUnique<SimulatorTask>("Binary Simulation Running", *this);
    }
    return task;
}

std::unique_ptr<DataParse> OpSimProf::GetDataParser() {
    return MakeUnique<SimDataParse>(socVersion_, exportPath_, coreId_, aicMetrics_, dump_, kernelName_);
}

bool OpSimProf::Run() {
    if (exportPath_.empty() && !RunTask()) {
        LogWarn("Running task failed, proceeding to data parsing");
    }

    if (!RunDataParse()) {
        LogError("Profiling data parse failed. Please check");
        return false;
    }
    return true;
}
}
