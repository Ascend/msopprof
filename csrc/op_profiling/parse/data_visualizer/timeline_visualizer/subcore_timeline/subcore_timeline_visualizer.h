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


#ifndef MSOPT_SUBCORE_TIMELINE_VISUALIZER_H
#define MSOPT_SUBCORE_TIMELINE_VISUALIZER_H

#include <vector>
#include "json.hpp"
#include "parse/data_center/data_center.h"
#include "parse/plugin/plugin_interface.h"
#include "parse/data_visualizer/sim_visualizer_config.h"
#include "parse/data_visualizer/sim_data_visualizer.h"
#include "parse/data_table/instr_detail_table.h"
#include "parse/data_table/cache_detail_table.h"

namespace Profiling {
namespace Parse {

struct XEvent {
    inline void ToJson(nlohmann::json &jsonData) const
    {
        jsonData["name"] = this->name;
        jsonData["cname"] = this->cName;
        jsonData["ph"] = this->ph;
        jsonData["ts"] = this->ts;
        jsonData["dur"] = this->dur;
        jsonData["pid"] = this->pid;
        jsonData["tid"] = this->tid;
        jsonData["args"] = this->args;
    }

    std::string name;
    std::string ph;
    int pid;
    int tid;
    float ts;
    float dur;
    std::string cName;
    std::map<std::string, std::string> args;
};

class SubcoreTimelineVisualizer : public SimDataVisualizer {
public:
    SubcoreTimelineVisualizer(DataCenter &dataCenter, SimVisualizerConfig &config)
        : SimDataVisualizer(dataCenter, config), pc2code_(config.GetPc2Code())
    {
        if (GetProductSeriesType(config.GetChipType()) == ChipProductType::ASCEND310P_SERIES) {
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
        RegisterPluginName("SingleCoreTimeLineVisualizer");
        RegisterMandatoryDb({typeid(std::map<std::string, SimData>)});
        RegisterChip({ChipProductType::ALL_PRODUCT_TYPE});
    }

private:
    bool ParseByCore(const std::string &coreName, SimData &data);
    void CollectInstrEvents4SepTrace(std::vector<MergeInfo> &instrs, std::vector<nlohmann::json> &coreJsonList,
    std::set<std::string> &pipeSet);
    void CollectUserMarkEvents(const SimData &data, std::vector<nlohmann::json> &coreJsonList);
    void CollectEvents(const std::map<int, std::vector<MergeInfo>> &instrsGroup,
        std::vector<nlohmann::json> &coreJsonList, std::map<std::string, std::vector<XEvent>> &setFlagRecord,
        std::map<std::string, std::vector<XEvent>> &waitFlagRecord);
    void CollectFlagAndFlowEventsForSep(std::vector<nlohmann::json> &coreJsonList,
        std::map<std::string, std::vector<XEvent>> &setFlagRecord,
        std::map<std::string, std::vector<XEvent>> &waitFlagRecord);
    void GetFlowEventsForSeq(const XEvent &begin, const XEvent &end, std::vector<nlohmann::json> &coreJsonList,
        int &waitFlagId);
    int GetUserMarkTid(const MergeInfo &instr, std::map<std::string, bool> &userMarkTidMap,
        std::vector<nlohmann::json> &coreJsonList);
    int GetTid(const MergeInfo &instr, uint64_t maxCycle,
        std::vector<std::vector<int>> &pipeOccupy, std::vector<nlohmann::json> &coreJsonList, int pid);
    bool FindAvailableThread(int curThreadNum, const MergeInfo &instr, std::vector<std::vector<int>> &pipeOccupy,
        int &tid) const;
    void AddThreadMetaData(std::vector<nlohmann::json> &coreJsonList, int threadIndex,
        const std::string &threadName, int pid) const;
    void AddProcessMetaData(std::vector<nlohmann::json> &coreJsonList, const std::string &pipe, int pid) const;
    void CollectSIMTEvents(std::map<int, std::vector<MergeInfo>> &instrsGroup,
                           std::vector<nlohmann::json> &coreJsonList);
    bool WriteSepJson(const std::string &filePath, std::vector<nlohmann::json> &coreJsonList) const;

    std::string waitFlagName_;
    std::string setFlagName_;
    int maxCycle_{};
    Pc2CodeMap& pc2code_;
    std::map<std::string, int> pidMap_{{"SCALAR", 10}, {"SCALARLDST", 20}, {"VECTOR", 30}, {"CUBE", 40},
                                       {"MTE1",   50}, {"MTE2", 60}, {"MTE3", 70}, {"FIXP", 80}, {"FLOWCTRL", 90},
                                       {"ALL",    100}, {"CACHEMISS", 110}, {Profiling::USER_MARK, 120}};
    std::map<int, std::string> flagMap_{{10, "SCALAR"}, {20, "SCALARLDST"}, {30, "VECTOR"}, {40, "CUBE"},
                                        {50, "MTE1"}, {60, "MTE2"}, {70, "MTE3"}, {80, "FIXP"}, {90, "FLOWCTRL"},
                                        {100, "ALL"}, {110, "CACHEMISS"}, {120, Profiling::USER_MARK}};
};
}
}
#endif // MSOPT_SUBCORE_TIMELINE_VISUALIZER_H
