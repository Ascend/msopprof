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


#ifndef __MSOPPROF_PARSE_DATA_CALCULATOR_SCALAR_TICK_CALCULATOR_H__
#define __MSOPPROF_PARSE_DATA_CALCULATOR_SCALAR_TICK_CALCULATOR_H__

#include "common/defs.h"
#include "profiling/simulator/data_parse/sim_defs.h"
#include "instr_detail_calculator.h"
#include "parse/data_table/instr_detail_table.h"
#include "parse/data_table/cache_detail_table.h"

namespace Profiling {
namespace Parse {

class ScalarCalculator : public InstrDetailCalculator {
public:
    explicit ScalarCalculator(DataCenter& dataCenter, InstrDetailConfig& instrDetailConfig)
        : InstrDetailCalculator(dataCenter, instrDetailConfig) {}
    PluginErrorCode Entry() override;
    void DependencyRegister() override
    {
        RegisterPluginName("ScalarRegisterCalculator");
        RegisterMandatoryDb({typeid(InstrDetailTable)});
        RegisterChip({ChipProductType::ASCEND910B_SERIES,
                      ChipProductType::ASCEND910_93_SERIES});
    }
private:
    void MergeScalar(const scalarHeadCache &icacheDetailTable, scalarHead &scalarDetailTable);
};
}
}


#endif //__MSOPPROF_PARSE_DATA_CALCULATOR_SCALAR_TICK_CALCULATOR_H__
