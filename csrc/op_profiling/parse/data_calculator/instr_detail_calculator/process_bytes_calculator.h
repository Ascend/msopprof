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


#ifndef __MSOPPROF_PARSE_DATA_CALCULATOR_INSTR_DETAIL_PROCESS_BYTES_CALCULATOR_H__
#define __MSOPPROF_PARSE_DATA_CALCULATOR_INSTR_DETAIL_PROCESS_BYTES_CALCULATOR_H__

#include "instr_detail_calculator.h"
#include "instr_detail_defs.h"
#include "parse/data_table/instr_detail_table.h"

namespace Profiling {
namespace Parse {

class ProcessBytesCalculator : public InstrDetailCalculator {
public:
    explicit ProcessBytesCalculator(DataCenter& dataCenter, InstrDetailConfig& instrDetailConfig)
        :InstrDetailCalculator(dataCenter, instrDetailConfig) {};

    PluginErrorCode Entry() override;
    void DependencyRegister() override
    {
        RegisterPluginName("ProcessBytesCalculator");
        RegisterMandatoryDb({typeid(InstrDetailTable)});
        RegisterChip({ChipProductType::ASCEND310P_SERIES,
                      ChipProductType::ASCEND910B_SERIES,
                      ChipProductType::ASCEND910_93_SERIES});
    }
};

}
}


#endif // __MSOPPROF_PARSE_DATA_CALCULATOR_INSTR_DETAIL_PROCESS_BYTES_CALCULATOR_H__
