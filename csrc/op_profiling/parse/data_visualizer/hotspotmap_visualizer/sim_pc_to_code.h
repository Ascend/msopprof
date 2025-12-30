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


#ifndef MSOPT_SIM_PC_STATIC_H
#define MSOPT_SIM_PC_STATIC_H

#include <string>
#include <regex>
#include "profiling/simulator/data_parse/sim_defs.h"
#include "parse/data_center/data_center.h"
#include "parse/data_visualizer/sim_visualizer_config.h"
#include "profiling/simulator/data_parse/api_data.h"

namespace Profiling {
namespace Parse {

struct InstrExe {
    std::string instr;
    std::string pipe;
    uint64_t addr;
    int callCount;
    int cycles;
    float runningTime;
    std::string detail;
};

class SimPcToCode {
public:
    SimPcToCode(SimVisualizerConfig &config, DataCenter &dataCenter, std::vector<std::string> &cores)
        : simConfig_(config), pc2code_(config.GetPc2Code()), dataCenter_(dataCenter), cores_(cores) {};
    void CalCulate();
    std::vector <Serialization::InstrInfo> GetInstrInfo();
    bool GetStallCyc() const { return hasStallCyc_; }

private:
    void Statistic(const std::string &coreName, SimData &data);
    void CalCycles(const std::vector<MergeInfo> &instrList, Serialization::CycleInfo &cycleInfo);
    std::map<uint64_t, std::vector<MergeInfo>> MergeInstr(const std::vector<MergeInfo> &instrs) const;
    std::string UpdateInstr(const std::string &instr, const std::regex &waitSetPattern,
                const std::string &waitFlagName, const std::string &setFlagName) const;
    void UpdateInstrDetail(std::vector<MergeInfo> &instrList, MergeInfo &selectedInstr) const;
    void InitNewInstrInfo(Serialization::InstrInfo &instrInfo);
    void UpdatePCStat(const MergeInfo &selectedInstr, const std::string &coreName,
        const Serialization::CycleInfo &cycleInfo, int callCount);
    bool WriteExeStat(const std::string &coreName, std::vector<InstrExe> &exeStatList);
    std::string GetAscendCInnerCode(uint64_t pc);

    // instrInfoMap_的key是pc，value是该core上的使用情况
    Utility::ThreadSafeUnorderedMap <uint64_t, Serialization::InstrInfo> instrInfoMap_;
    SimVisualizerConfig &simConfig_;
    Pc2CodeMap& pc2code_;
    DataCenter &dataCenter_;
    std::vector<std::string> cores_;
    bool hasStallCyc_ = false;
};
}
}
#endif // MSOPT_SIM_PC_STATIC_H
