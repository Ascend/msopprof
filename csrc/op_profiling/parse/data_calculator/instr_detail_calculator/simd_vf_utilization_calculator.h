/* -------------------------------------------------------------------------
 *  This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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

#ifndef MSOPPROF_SIMD_VF_UTILIZATION_CALCULATOR_H
#define MSOPPROF_SIMD_VF_UTILIZATION_CALCULATOR_H

#include "instr_detail_calculator.h"

namespace Profiling {
namespace Parse {
class SimdVfUtilizationCalculator : public InstrDetailCalculator {
public:
    explicit SimdVfUtilizationCalculator(DataCenter &dataCenter, InstrDetailConfig &config)
        : InstrDetailCalculator(dataCenter, config) {}
    PluginErrorCode Entry() override;
    void DependencyRegister() override;
};
}
}
#endif // MSOPPROF_SIMD_VF_UTILIZATION_CALCULATOR_H
