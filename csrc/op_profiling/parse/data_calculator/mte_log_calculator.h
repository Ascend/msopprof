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

#ifndef MSOPT_PARSE_MTE_LOG_CALCULATE_H
#define MSOPT_PARSE_MTE_LOG_CALCULATE_H

#include "parse/data_table/mte_throughput_table.h"
#include "parse/data_center/data_center.h"
#include "parse/plugin/plugin_interface.h"

namespace Profiling {
namespace Parse {

class MteLogCalculator : public PluginInterface {
public:
    explicit MteLogCalculator(DataCenter &dataCenter,
                              const ChipProductType &chipType) : PluginInterface(dataCenter, chipType) {};
    PluginErrorCode Entry() override;
    void DependencyRegister() override
    {
        RegisterPluginName("MteLogCalculator");
        RegisterMandatoryDb({});
        RegisterChip({ChipProductType::ASCEND910B_SERIES, ChipProductType::ASCEND910_93_SERIES,
            ChipProductType::ASCEND950_SERIES});
    }
private:
    void CalOneInstrThroughput(MteLogInstrType instrType, const std::unordered_map<uint64_t, MteLogReqInfo> &reqInfo,
                               MteThroughputChart &mteThroughputInfo) const;
};

}
}
#endif
