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


#ifndef MSOPT_SINGLE_WHOLE_TIME_LINE_H
#define MSOPT_SINGLE_WHOLE_TIME_LINE_H

#include <vector>
#include "json.hpp"
#include "parse/data_center/data_center.h"
#include "parse/plugin/plugin_interface.h"
#include "parse/data_visualizer/sim_visualizer_config.h"
#include "parse/data_visualizer/sim_data_visualizer.h"
#include "parse/data_table/instr_detail_table.h"
#include "parse/data_table/cache_detail_table.h"
#include "parse/data_visualizer/timeline_visualizer/timeline_defs.h"

namespace Profiling {
namespace Parse {

class CoreTimeLineVisualizer : public SimDataVisualizer {
public:
    CoreTimeLineVisualizer(DataCenter &dataCenter, SimVisualizerConfig &config)
        : SimDataVisualizer(dataCenter, config), pc2code_(config.GetPc2Code())
    {
        if (Common::GetProductSeriesType(config.GetChipType()) == Common::ChipProductType::ASCEND310P_SERIES) {
            // 310P series
            waitFlagName_ = Profiling::WAIT_EVENT;
            setFlagName_ = Profiling::SET_EVENT;
        } else {
            // other series, include A2, 910_93
            waitFlagName_ = Profiling::WAIT_FLAG;
            setFlagName_ = Profiling::SET_FLAG;
        }
    };

    PluginErrorCode Entry() override;

    void DependencyRegister() override
    {
        RegisterPluginName("CoreTimeLineVisualizer");
        RegisterMandatoryDb({typeid(std::map<std::string, SimData>)});
        RegisterChip({Common::ChipProductType::ALL_PRODUCT_TYPE});
    }

private:
    bool ParseByCore(const std::string &coreName, SimData &data);
    void CollectInstrEvents(const std::string &coreName, std::vector<MergeInfo> &instrs,
        std::vector<nlohmann::json> &coreJson);
    void CollectFlowEvents(std::map<std::string, std::vector<SetWaitFlag>> &setFlagRecord,
        std::map<std::string, std::vector<SetWaitFlag>> &waitFlagRecord, std::vector<nlohmann::json> &coreJson);
    void AddFlag(const SetWaitFlag &flag, const std::string &id, std::vector<nlohmann::json> &coreJson);
    void GetFlowEvents(SetWaitFlag &begin, SetWaitFlag &end, std::string &id,
                       std::vector<nlohmann::json> &coreJson) const;
    void CollectUserMarkEvents(const std::string &coreName, const SimData &data, std::vector<nlohmann::json> &coreJson);
    Event GenerateEvent(const MergeInfo &instr, EventArgs &evtArgs, int startCycle, int durationCycle,
        const std::string &coreName) const;
    void WriteFile(const std::string &filePath);
    void CollectLaneOrderEvents(const std::string &coreName, std::vector<MergeInfo> &instrs,
        std::vector<nlohmann::json> &coreJson);
    void AddCoreNameOrder(const std::string &coreName);
    uint32_t ExtraNumAfterKey(const std::string &str, const std::string &key);
    std::string waitFlagName_;
    std::string setFlagName_;
    std::mutex timeLineLock_;
    std::vector<nlohmann::json> coresJsonList_;
    std::vector<nlohmann::json> laneJsonList_;
    Pc2CodeMap& pc2code_;
};
}
}

#endif // MSOPT_SINGLE_WHOLE_TIME_LINE_H
