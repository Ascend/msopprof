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


#ifndef MSOPT_TIME_LINE_VISUALIZER_DEFS_H
#define MSOPT_TIME_LINE_VISUALIZER_DEFS_H

#include <vector>
#include <string>
#include <map>
#include "json.hpp"
#include "profiling/simulator/data_parse/sim_defs.h"

namespace Profiling {
namespace Parse {

constexpr int16_t FLOW_AND_EVENT_ROUND_PARAM = -1; // not round for precision of flow event

struct Event {
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
    std::string cName;
    std::string ph;
    float ts;
    float dur;
    std::string pid;
    std::string tid;
    std::map<std::string, std::string> args;
};

struct EventArgs {
    std::string pcAddr;
    std::string code;
    std::string detail;
};

struct SetWaitFlag {
    MergeInfo instr;
    EventArgs evtArgs;
    std::string coreName;
};

struct FlowEvent {
    inline void ToJson(nlohmann::json &jsonData) const
    {
        jsonData["name"] = this->name;
        jsonData["id"] = this->id;
        jsonData["ph"] = this->ph;
        jsonData["ts"] = this->ts;
        jsonData["pid"] = this->pid;
        jsonData["tid"] = this->tid;
        jsonData["cat"] = this->cat;
    }

    std::string name;
    std::string id;
    std::string ph;
    float ts;
    std::string pid;
    std::string tid;
    std::string cat;
};

struct FlagEvent {
    inline void ToJson(nlohmann::json &jsonData) const
    {
        jsonData["name"] = this->name;
        jsonData["cname"] = this->cName;
        jsonData["ph"] = this->ph;
        jsonData["ts"] = this->ts;
        jsonData["pid"] = this->pid;
        jsonData["tid"] = this->tid;
        jsonData["args"] = this->args;
        jsonData["id"] = this->id;
    }

    std::string name;
    std::string cName;
    std::string ph;
    float ts;
    std::string pid;
    std::string tid;
    std::map<std::string, std::string> args;
    std::string id;
};

struct LaneEvent {
    inline void ToJson(nlohmann::json &jsonData) const
    {
        jsonData["name"] = this->name;
        jsonData["ph"] = this->ph;
        jsonData["pid"] = this->pid;
        jsonData["tid"] = this->tid;
        jsonData["args"] = this->args;
    }

    std::string name;
    std::string ph;
    std::string pid;
    std::string tid;
    std::map<std::string, uint32_t> args;
};

const std::map <std::string, std::string> cNamesMap {
    {"MTE1",               "thread_state_running"},
    {"MTE2",               "thread_state_iowait"},
    {"MTE3",               "rail_response"},
    {"SCALAR",             "startup"},
    {"SCALARLDST",         "startup"},
    {"VECTOR",             "cq_build_failed"},
    {"CUBE",               "background_memory_dump"},
    {"FIXP",               "thread_state_unknown"},
    {"FLOWCTRL",           "cq_build_passed"},
    {"CACHEMISS",          "thread_state_runnable"},
    {"ALL",                "good"},
    {Profiling::USER_MARK, "rail_load"},
    {"EVENT",              "rail_animation"},
    {"SET_FLAG",           "black"},
    {"WAIT_FLAG",          "yellow"},
    {"set_event",          "black"},
    {"wait_event",         "yellow"},
    {"RVECEX",             "thread_state_running"},
    {"RVECLP",             "thread_state_iowait"},
    {"RVECST",             "thread_state_runnable"},
    {"RVECLD",             "startup"},
    {"PUSHQ",              "thread_state_unknown"},
    {"RVECSU",             "yellow"}
};
}
}
#endif // MSOPT_TIME_LINE_VISUALIZER_DEFS_H
