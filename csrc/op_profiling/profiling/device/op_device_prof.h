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


#ifndef __MSOPPROF_PROFILING_OP_DEVICE_PROF_H__
#define __MSOPPROF_PROFILING_OP_DEVICE_PROF_H__

#include "profiling/op_prof.h"

namespace Profiling {

class OpDeviceProf : public OpProf {
friend class DeviceTaskGenerator;
public:
    explicit OpDeviceProf(const Common::ProfArgs &profArgs);
    Profiling::TaskPtr GetTask() override;
    std::unique_ptr<DataParse> GetDataParser() override;

    Common::ChipType chipType_;
    Common::PmuEventsId pmuEventsId_;
    ReplayMode replayMode_;
    uint16_t warmUp_;
    std::string coreId_;
};

}

#endif // __MSOPPROF_PROFILING_OP_DEVICE_PROF_H__
