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


#ifndef __MSOPPROF_PROFILING_OP_PROF_H__
#define __MSOPPROF_PROFILING_OP_PROF_H__

#include "common/prof_args.h"
#include "op_prof_task.h"
#include "op_prof_data_parse.h"

namespace Profiling {

using TaskPtr = std::unique_ptr<Profiling::Task>;

class OpProf {
public:
    explicit OpProf(const Common::ProfArgs &args);
    virtual Profiling::TaskPtr GetTask() = 0;
    virtual std::unique_ptr<DataParse> GetDataParser() = 0;

    virtual bool Run();
    bool RunTask();
    bool RunDataParse(bool needMoveDir = true);

    Common::ProfMetricsAbilityConfig aicMetrics_;
    uint16_t launchCount_{0};
    uint16_t launchSkipBeforeMatch_{0};
    bool isMstxEnable_{false};
    std::string mstxInclude_;
    bool kill_{false};
    bool dump_ {true};
    std::string kernelName_;
    std::vector<std::string> cmd_;
    std::string output_;
    std::string config_;
    OpRunner::KernelConfig kernelConfig_;
};

} // namespace Profiling

#endif  // __MSOPPROF_INTERFACE_OP_PROF_H__
