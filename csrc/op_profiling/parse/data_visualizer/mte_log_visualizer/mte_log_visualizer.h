
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

#ifndef MSOPT_MTE_LOG_VISUALIZER_H
#define MSOPT_MTE_LOG_VISUALIZER_H

#include <string>
#include <vector>
#include "json.hpp"
#include "common/defs.h"
#include "parse/plugin/plugin_interface.h"

namespace Profiling {
namespace Parse {

class MteLogVisualizer : public PluginInterface {
public:
    explicit MteLogVisualizer(DataCenter &dataCenter,
                              const ChipProductType &chipType) : PluginInterface(dataCenter, chipType) {};
    PluginErrorCode Entry() override;
    void DependencyRegister() override
    {
        RegisterPluginName("MteLogVisualizer");
        RegisterMandatoryDb({});
        RegisterChip({ChipProductType::ASCEND910B_SERIES, ChipProductType::ASCEND910_93_SERIES,
            ChipProductType::ASCEND950_SERIES});
    }
private:
    void FillData(std::vector<nlohmann::json> &mteThroughputJson, const std::vector<double> &valueList, size_t ts);
    uint64_t validTsNum_;
    uint64_t endTs_;
};
}
}
#endif