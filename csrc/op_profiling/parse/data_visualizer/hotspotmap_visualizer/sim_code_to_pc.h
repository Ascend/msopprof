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


#ifndef MSOPT_SIM_CODE_TO_PC_H
#define MSOPT_SIM_CODE_TO_PC_H

#include <string>
#include "json.hpp"
#include "profiling/simulator/data_parse/sim_defs.h"
#include "profiling/simulator/data_parse/api_data.h"
#include "parse/data_visualizer/sim_visualizer_config.h"
#include "parse/data_center/data_center.h"

namespace Profiling {
namespace Parse {

struct CodeExe {
    int callCount;
    int cycles;
    float runningTimeUS;
    std::string code;
};

class SimCodeToPc {
public:
    SimCodeToPc(SimVisualizerConfig &config, DataCenter &dataCenter, std::vector<std::string> &cores)
        : simConfig_(config), dataCenter_(dataCenter), pc2code_(config.GetPc2Code()), cores_(cores) {};
    void CalCulate();
    std::vector<Serialization::CodeFile> GetCodeFile();

private:
    void Statistic(const std::string &coreName, SimData &data);
    bool CalAddrRange();
    bool UpdateCodeStat(const struct UpdateCode &updateCode, const std::string &coreName);
    bool WriteExeStat(const std::string &coreName, std::vector<CodeExe> &exeStatList);
    bool VisualizeKernelFiles(const std::string &outputPath, const std::string &filePath) const;

    std::map <std::string, std::vector<MergeInfo>> MergeInstr(const std::vector <MergeInfo> &instrs);
    SimVisualizerConfig simConfig_;
    DataCenter& dataCenter_;
    Pc2CodeMap& pc2code_;
    std::vector<std::string> cores_;
    std::mutex codeLineMappingMutex_;
    // codeInfoMap_的key是cpp文件,value的key是行号，value的value是该行代码在核上运行情况
    std::map<std::string, std::map<std::string, Serialization::CodeLine>> codeInfoMap_;
    // lineAdrrMap_的key是cpp文件,value的key是行号，value的value是该行代码涉及的所有pc地址
    std::map<std::string, std::map<std::string, std::set<uint64_t>>> lineAdrrMap_;
};

}
}
#endif // MSOPT_SIM_CODE_TO_PC_H
