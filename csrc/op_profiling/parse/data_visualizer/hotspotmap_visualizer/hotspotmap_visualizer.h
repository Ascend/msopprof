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


#ifndef MSOPT_SINGLECOREHOTSPOTMAPVISUALIZER_H
#define MSOPT_SINGLECOREHOTSPOTMAPVISUALIZER_H

#include "sim_code_to_pc.h"
#include "sim_pc_to_code.h"
#include "parse/data_visualizer/sim_data_visualizer.h"
#include "profiling/simulator/data_parse/sim_defs.h"
#include "parse/data_table/instr_detail_table.h"
#include "parse/data_parser/sim_dump_parser.h"
#include "profiling/simulator/data_parse/api_data.h"

namespace Profiling {
namespace Parse {

class HotSpotMapVisualizer : public SimDataVisualizer {
public:
    HotSpotMapVisualizer(DataCenter &dataCenter, SimVisualizerConfig &config)
        : SimDataVisualizer(dataCenter, config), pc2code_(config.GetPc2Code()) {}

    PluginErrorCode Entry() override;

    void DependencyRegister() override
    {
        RegisterPluginName("hotSpotMap");
        RegisterMandatoryDb({typeid(std::map<std::string, SimData>)});
        RegisterChip({Common::ChipProductType::ALL_PRODUCT_TYPE});
    }
private:
    bool ParseCoreInfo(const std::string &coreName, std::vector<MergeInfo> instrs);
    void PrintCoreInfo();
    std::vector<Serialization::CoreExe> exeStatList_;
    std::mutex hotSpotMapMutex_;
    Pc2CodeMap& pc2code_;
    Utility::ThreadSafeUnorderedMap<std::string, Serialization::InstrInfo> instrInfoMap_;
};
}
}

#endif // MSOPT_SINGLECOREHOTSPOTMAPVISUALIZER_H
