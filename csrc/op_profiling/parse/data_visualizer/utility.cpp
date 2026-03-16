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


#ifndef MSOPT_VISUAL_UTILY_H
#define MSOPT_VISUAL_UTILY_H

#include "utility.h"
#include <vector>
#include <string>
#include <algorithm>
#include "log.h"
#include "number_operation.h"
#include "sim_visualizer_config.h"
#include "profiling/simulator/data_parse/sim_defs.h"
#include "timeline_visualizer/timeline_defs.h"


using namespace Utility;

namespace Profiling {
namespace Parse {

int CalCycles(const std::vector<MergeInfo> &instrList)
{
    uint64_t cycles = 0;
    std::string location = "cycles";
    for (auto &instr: instrList) {
        uint64_t temp = SafeSub(instr.endTick, instr.startTick, location, false);
        cycles = SafeAdd(cycles, temp, location);
    }
    // 可视化当前只接收int数据，这里需要强制转换
    if (cycles > INT32_MAX) {
        return INT32_MAX;
    }
    return static_cast<int>(cycles);
}

float GetMicrosecond(const ChipProductType &chipType, const uint64_t &cycles, int roundParam)
{
    double clockSpeed;
    auto find_soc = CLOCK_SPEED_SERIES_MAP.find(chipType);
    clockSpeed = (find_soc == CLOCK_SPEED_SERIES_MAP.end()) ? DEFAULT_MHZ : find_soc->second;
    if (roundParam == -1) {
        return static_cast<float>(cycles / clockSpeed);
    }
    return static_cast<float>(round(cycles / clockSpeed *
                                    pow(10, roundParam)) / pow(10, roundParam)); // 10:used to calculate round
}

float CalculateRunningTime(std::vector<MergeInfo> &instrList, const ChipProductType &chipType)
{
    uint64_t runningTime = 0;
    int roundParam = 2; // 2: Precise number of digits
    if (instrList.empty()) {
        return GetMicrosecond(chipType, runningTime, roundParam);
    }
    // 按照起始时间排序
    sort(instrList.begin(), instrList.end(),
         [](MergeInfo &instr1, MergeInfo &instr2) {
            return instr1.startTick < instr2.startTick;
        });
    // 合并区间重叠的时间段
    std::vector<MergeInfo> merge;
    merge.push_back(instrList[0]);
    for (size_t i = 1; i < instrList.size(); i++) {
        MergeInfo& lastInfo = merge.back();
        if (lastInfo.endTick >= instrList[i].startTick) {
            // 如果区间重叠，合并区间，将上一个区间的结束时间设置为两个区间结束时间的最大值
            lastInfo.endTick = std::max(instrList[i].endTick, lastInfo.endTick);
        } else {
            // 如果没有重叠，将区间加入集合
            merge.push_back(instrList[i]);
        }
    }
    // 计算无重叠区间的时间总和
    std::string location = "simulator calculate runningTime";
    bool isStartBiggerThanEnd = false;
    for (const auto& info : merge) {
        // 非法时间段，打印异常信息，跳过
        if (info.startTick >= info.endTick) {
            if (info.startTick > info.endTick) { isStartBiggerThanEnd = true; }
            continue;
        }
        uint64_t partCycles = info.endTick - info.startTick;
        runningTime = SafeAdd(partCycles, runningTime, location);
    }
    if (isStartBiggerThanEnd) {
        LogDebug("Some running time startTick is bigger or equals to endTick, those time part is skip");
    }
    return GetMicrosecond(chipType, runningTime, roundParam);
}

const std::map<std::string, uint32_t>& getLaneOrderMap()
{
    static std::map <std::string, uint32_t> laneOrderMap{{"SCALAR", 1}, {"FLOWCTRL", 2}, {"MTE1", 3},  {"CUBE", 4}, {"FIXP", 5},
        {"MTE2", 6}, {"VECTOR", 7}, {"MTE3", 8}, {"CACHEMISS", 9}, {"SCALARLDST", 10}, {"ALL", 11}, {Profiling::USER_MARK, 12},
        {"EVENT", 13}, {"SET_FLAG", 14}, {"WAIT_FLAG", 15}, {"set_event", 16}, {"wait_event", 17}, {"RVECEX", 18}, {"RVECLP", 19},
        {"RVECST", 20}, {"RVECLD", 21}, {"PUSHQ", 22}, {"RVECSU", 23}};
 
    return laneOrderMap;
}

std::string GetCNameByInstrName(const std::string &instrName)
{
    std::string cName = "thread_state_unknown";
    if (instrToColorMap.count(instrName) != 0) {
        cName = instrToColorMap.at(instrName);
    } else {
        LogDebug("Cannot find pipe %s in cname, use default color.", instrName.c_str());
    }
    return cName;
}

std::string GetCNameByPipe(const std::string &pipe)
{
    std::string cName = "thread_state_stopped";
    if (cNamesMap.count(pipe) != 0) {
        cName = cNamesMap.at(pipe);
    } else {
        LogDebug("Cannot find pipe %s in cname, use default color.", pipe.c_str());
    }
    return cName;
}
}
}
#endif // MSOPT_VISUAL_UTILY_H
