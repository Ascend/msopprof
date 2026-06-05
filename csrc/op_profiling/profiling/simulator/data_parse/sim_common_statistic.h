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


#ifndef MSOPT_COMMON_STATISTIC_H
#define MSOPT_COMMON_STATISTIC_H
#include <string>
#include <vector>
#include <map>
#include "json.hpp"
#include "data_format.h"
#include "sim_defs.h"
#include "common/defs.h"

namespace Serialization {

struct CycleInfo {
    int cycles;
    int realStallCyc;
    int iCacheCyc;
    int ccuCyc;
    int scalarCyc;
};

struct Instr {
    int starts;
    int end;
    int gprCount;
    int realStallCyc;
    int processBytes;
    float vecUtilization;
    int ubReadConflict;
    int ubWriteConflict;
    int warpId;
    int schId;
    std::string pc;
    std::string pipe;
    std::string name;
    std::string detail;
};

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

constexpr int DIR_DEFAULT_MODE = 0740;

}
#endif // MSOPT_COMMON_STATISTIC_H
