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


#ifndef __MSOPPROF_PROFILING_OP_SIM_PROF_H__
#define __MSOPPROF_PROFILING_OP_SIM_PROF_H__

#include "profiling/op_prof.h"

namespace Profiling {

class OpSimProf : public OpProf {
friend class SimulatorTaskGenerator;
public:
    explicit OpSimProf(const Common::ProfArgs &profArgs);
    Profiling::TaskPtr GetTask() override;
    std::unique_ptr<DataParse> GetDataParser() override;
    bool Run() override;

    std::string exportPath_;
    std::string coreId_;
    int32_t timeout_ = -1;
    std::string socVersion_;
};

}

#endif // __MSOPPROF_PROFILING_OP_SIM_PROF_H__
