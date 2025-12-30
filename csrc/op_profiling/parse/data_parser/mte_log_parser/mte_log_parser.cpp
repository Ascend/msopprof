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

#include "mte_log_parser.h"

#include <regex>
#include <string>
#include <algorithm>
#include "filesystem.h"
#include "ustring.h"
#include "profiling/simulator/data_parse/sim_data_parse.h"
#include "number_operation.h"

using namespace std;
using namespace Utility;

namespace Profiling {
namespace Parse {

static constexpr char const *TICK = "tick";
static constexpr char const *UNIT = "unit";
static constexpr char const *TYPE = "type";
static constexpr char const *SIZE = "size";
static constexpr char const *INSTR_ID = "instr_id";
static constexpr char const *REQ_ID = "req_id";
static constexpr char const *GID = "gid";

PluginErrorCode MteLogParser::Entry()
{
    // 填充数据中心
    std::shared_ptr<std::vector<Parse::MteLogInstrMap>> mteLogInstrMapVecPtr =
        dataCenter_.GetDbPtr<std::vector<Parse::MteLogInstrMap>>();
    if (mteLogInstrMapVecPtr == nullptr) {
        LogWarn("Mte log instr map ptr is nullptr");
        return PluginErrorCode::NONBLOCKING_ERROR;
    }
    if (coreId_ >= mteLogInstrMapVecPtr->size()) {
        LogWarn("CoreId: %d is out of range, total num:%d", coreId_, mteLogInstrMapVecPtr->size());
        return PluginErrorCode::NONBLOCKING_ERROR;
    }
    MteLogInstrMap &mteLogInstrMap = (*mteLogInstrMapVecPtr)[coreId_];
    // 解析mte log
    std::vector<std::string> mteLogFileLines;
    ReadFileByMMap(dumpFile_, mteLogFileLines);

    for (std::string &line: mteLogFileLines) {
        ParseMTELogLine(line, mteLogInstrMap);
    }
    return PluginErrorCode::SUCCESS;
}
void MteLogParser::ParseMTELogLine(const Common::DvcMteLog &mteLog, MteLogInstrMap &mteLogInstrMap) const
{
    std::unordered_map<std::string, std::string> ifMap;
    if (!MatchIf(mteLog, ifMap)) {
        return;
    }
    if (ifMap[UNIT] == "B") {
        ParseGMIf(mteLogInstrMap, ifMap);   // BWIF/BRIF决定每条req的时间以及数据量
    } else {
        ParseOthIf(mteLogInstrMap, ifMap);  // 其余IF请求决定每条req的instrType
    }
}
void MteLogParser::ParseMTELogLine(std::string &line, MteLogInstrMap &mteLogInstrMap) const
{
    std::unordered_map<std::string, std::string> ifMap;
    if (!MatchIf(line, ifMap)) {
        return;
    }
    if (ifMap[UNIT] == "B") {
        ParseGMIf(mteLogInstrMap, ifMap);   // BWIF/BRIF决定每条req的时间以及数据量
    } else {
        ParseOthIf(mteLogInstrMap, ifMap);  // 其余IF请求决定每条req的instrType
    }
}

bool MteLogParser::MatchIf(const Common::DvcMteLog &mteLog, std::unordered_map<std::string, std::string> &ifMap) const
{
    std::string name = mteLog.intf;
    if (!EndsWith(name, "IF")) {return false;}
    ifMap[TICK] = std::to_string(mteLog.time);
    ifMap[UNIT] = name.substr(0, name.size() - 3);  // 3表示尾缀(R|W)IF的长度
    ifMap[TYPE] = name.substr(name.size() - 3, 1);  // 3表示尾缀(R|W)IF的长度

    ifMap[SIZE] = std::to_string(mteLog.size);
    ifMap[INSTR_ID] = std::to_string(mteLog.instrId);
    ifMap[REQ_ID] = std::to_string(mteLog.reqId);
    return true;
}

bool MteLogParser::MatchIf(const std::string &line, std::unordered_map<std::string, std::string> &ifMap) const
{
    size_t id = 0;
    size_t infoPos = line.find("[info]");
    if (infoPos == std::string::npos) {return false;}
    id = infoPos + 7; // 7 表示下一个字段的起始
    if (infoPos != 0 || id >= line.size()) {return false;}
    // 匹配 tick
    size_t timeEndPos = line.find(":", id);
    if (timeEndPos == std::string::npos) {return false;}
    string time = line.substr(id, timeEndPos - id);
    id = timeEndPos + 2; // 2 表示下一个字段的起始
    if (id >= line.size() || !IsDigit(time)) {return false;}
    ifMap[TICK] = time;
    // 匹配 unit 和 type
    size_t nameEndPos = line.find(" ", id);
    if (nameEndPos == std::string::npos) {return false;}
    string name = line.substr(id, nameEndPos - id);
    id = nameEndPos + 1;
    if (!MatchName(name, ifMap)) {return false;}
    while (id < line.size()) {
        size_t sepPos = line.find(":", id);
        if (sepPos == std::string::npos) {return false;}
        size_t keyStartPos = line.rfind(" ", sepPos);
        if (keyStartPos == std::string::npos) {return false;}
        size_t valueEndPos = line.find(",", sepPos);
        if (valueEndPos == std::string::npos) {
            valueEndPos = line.size();
            if (line[valueEndPos - 1] == '.') {
                valueEndPos -= 1;
            }
        }
        string key = line.substr(keyStartPos + 1, sepPos - keyStartPos - 1);
        string value = line.substr(sepPos + 2, valueEndPos - sepPos - 2);
        ifMap[key] = value;
        id = valueEndPos + 1;
    }
    return true;
}

bool MteLogParser::MatchName(const std::string &name, std::unordered_map<std::string, std::string> &ifMap) const
{
    if (Utility::EndsWith(name, "IF")) {
        ifMap[UNIT] = name.substr(0, name.size() - 3);   // 3是尾缀(R|W)IF的长度
        ifMap[TYPE] = name.substr(name.size() - 3, 1);   // 3是尾缀(R|W)IF的长度
        return true;
    }
    size_t sepId = name.rfind("IF.");
    if (sepId == std::string::npos) {return false;}
    size_t startId = name.rfind(".", sepId);
    ifMap[UNIT] = name.substr(startId + 1, sepId - startId - 2);   // 2 对齐长度
    ifMap[TYPE] = name.substr(sepId - 1, 1);
    return true;
}

void MteLogParser::ParseGMIf(MteLogInstrMap &mteLogInstrMap, std::unordered_map<std::string, std::string> &ifMap) const
{
    int64_t tick = 0;
    uint64_t instrId = 0;
    uint64_t reqId = 0;
    uint64_t size = 0;
    if (!StollConverter(ifMap[TICK], tick) || !StoullConverter(ifMap[INSTR_ID], instrId) ||
        (!StoullConverter(ifMap[REQ_ID], reqId) && !StoullConverter(ifMap[GID], reqId))) {
        return;
    }
    StoullConverter(ifMap[SIZE], size);

    MteLogReqInfo reqInfo;
    reqInfo.dataSize = static_cast<double>(size);
    reqInfo.ts = GetMicrosecond(chipType_, tick);
    // 如果当指令表中还不存在key为instrId的项，则新增指令，同时新增该指令下一条请求，type默认未知
    if (mteLogInstrMap.find(instrId) == mteLogInstrMap.end()) {
        MteLogInstrInfo instrInfo;
        instrInfo.instrType = MteLogInstrType::END;
        instrInfo.reqTbl[reqId] = reqInfo;
        instrInfo.maxReqTs = reqInfo.ts;
        mteLogInstrMap[instrId] = instrInfo;
    // 如果当前指令表中已存在key为instrId的项，但是请求表中不存在key为reqId的项，则新增请求
    } else if (mteLogInstrMap[instrId].reqTbl.find(reqId) == mteLogInstrMap[instrId].reqTbl.end()) {
        mteLogInstrMap[instrId].reqTbl[reqId] = reqInfo;
        mteLogInstrMap[instrId].maxReqTs = std::max(mteLogInstrMap[instrId].maxReqTs, reqInfo.ts);
    // 若请求已存在，则更新该请求时间
    } else {
        if (SafeEqual(mteLogInstrMap[instrId].reqTbl[reqId].dataSize, 0.0, 0.0) && !SafeEqual(reqInfo.dataSize, 0.0, 0.0)) {
            mteLogInstrMap[instrId].reqTbl[reqId].dataSize = reqInfo.dataSize;
        }
        mteLogInstrMap[instrId].reqTbl[reqId].ts = std::max(mteLogInstrMap[instrId].reqTbl[reqId].ts, reqInfo.ts);
        mteLogInstrMap[instrId].maxReqTs = std::max(mteLogInstrMap[instrId].maxReqTs, reqInfo.ts);
    }
}

void MteLogParser::ParseOthIf(MteLogInstrMap &mteLogInstrMap, std::unordered_map<std::string, std::string> &ifMap) const
{
    uint64_t instrId = 0;
    if (!StoullConverter(ifMap[INSTR_ID], instrId)) {
        return;
    }
    if (mteLogInstrMap.find(instrId) != mteLogInstrMap.end() &&
        mteLogInstrMap[instrId].instrType != MteLogInstrType::END) {
        return;
    }

    MteLogInstrType instrType = MteLogInstrType::END;
    std::string unit = ifMap[UNIT];
    std::string type = ifMap[TYPE];
    if (unit == "L1") {
        instrType = type == "W" ? MteLogInstrType::GM_TO_L1 : MteLogInstrType::L1_TO_GM;
    } else if (Utility::StartsWith(unit, "UB")) {
        instrType = type == "W" ? MteLogInstrType::GM_TO_UB : MteLogInstrType::UB_TO_GM;
    } else {
        instrType = type == "W" ? MteLogInstrType::GM_TO_TOTAL : MteLogInstrType::TOTAL_TO_GM;
    }
    // 如果instr不存在，则新增一条instr
    if (mteLogInstrMap.find(instrId) == mteLogInstrMap.end()) {
        MteLogInstrInfo instrInfo;
        instrInfo.instrType = instrType;
        instrInfo.maxReqTs = -1;
        mteLogInstrMap[instrId] = instrInfo;
    // 如果instr已存在，且instrType=END，则更新instrType
    } else {
        mteLogInstrMap[instrId].instrType = instrType;
    }
}

}
}
