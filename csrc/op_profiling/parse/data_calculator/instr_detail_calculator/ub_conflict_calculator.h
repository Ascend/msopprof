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


#ifndef __MSOPPROF_PARSE_DATA_CALCULATOR_INSTR_DETAIL_UB_CONFLICT_CALCULATOR_H__
#define __MSOPPROF_PARSE_DATA_CALCULATOR_INSTR_DETAIL_UB_CONFLICT_CALCULATOR_H__

#include "common/defs.h"
#include "instr_detail_calculator.h"
#include "instr_in_camodel/camodel_vec_name_map.h"
#include "profiling/simulator/data_parse/sim_defs.h"
#include "parse/data_table/instr_detail_table.h"

namespace Profiling {
namespace Parse {

class UbConflictCalculator : public InstrDetailCalculator {
public:
    explicit UbConflictCalculator(DataCenter& dataCenter, InstrDetailConfig& instrDetailConfig)
        : InstrDetailCalculator(dataCenter, instrDetailConfig) {};

    PluginErrorCode Entry() override;
    void DependencyRegister() override
    {
        RegisterPluginName("Ub Conflict Calculation");
        RegisterMandatoryDb({typeid(InstrDetailTable)});
        RegisterChip({Common::ChipProductType::ASCEND310P_SERIES,
                      Common::ChipProductType::ASCEND910B_SERIES,
                      Common::ChipProductType::ASCEND910_93_SERIES});
    }

private:
    void Get310PVecEvent(const MergeInfo &mergeInfo, InstrDetailEvent &event);
    void GetA2A3VecEvent(const MergeInfo &mergeInfo, VecInstrTemplate vecTemplate,
                         InstrDetailEvent &event);

    std::pair<int, int> ConflictCalculator(const InstrDetailEvent &instrDetailEvent) const;
    int FindSameTimesMax(const std::vector<uint64_t> &idList) const;
};

}
}

#endif // __MSOPPROF_PARSE_DATA_CALCULATOR_INSTR_DETAIL_UB_CONFLICT_CALCULATOR_H__
