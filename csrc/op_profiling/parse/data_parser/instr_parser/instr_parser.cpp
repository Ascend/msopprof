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


#include "instr_parser.h"
#include "parse/data_table/instr_detail_table.h"
using namespace Utility;

namespace Profiling {
namespace Parse {

PluginErrorCode InstrParser::Entry()
{
    // matchMode 0表示按pc匹配， 1表示按id匹配
    MatchMode matchMode = IsChipSeriesTypeValid(chipType_, ChipProductType::ASCEND950_SERIES) ?
        MatchMode::ID_MATCH : MatchMode::PC_MATCH;
    std::vector<PoppedInstrParseInfo> poppedInstrList;
    PopLogParser popParser(dataParserConfig_);
    popParser.ParseDumpLog(matchMode);
    const std::string &physisName = dataParserConfig_.GetCoreInfo().first;
    const std::string &logicName = popParser.GetLogicalName();
    const std::string coreName = logicName.empty() ? physisName : logicName;
    InstrLogParser instrLogParser(dataParserConfig_, coreName);

    int coreId = popParser.GetCoreId();
    std::set<int> parseIds = dataParserConfig_.GetParseCoreId();
    if (!parseIds.empty() && parseIds.find(coreId) == parseIds.end()) {
        return PluginErrorCode::SUCCESS;
    }
    instrLogParser.ParseDumpLog(matchMode);
    if (!MergeLog(instrLogParser, popParser, matchMode)) {
        return PluginErrorCode::FATAL_ERROR;
    }
    if (!logicName.empty()) {
        auto physisToLogicPtr = Utility::MakeShared<std::pair<std::string, std::string>>(std::pair<std::string,
            std::string> {physisName, logicName});
        if (physisToLogicPtr == nullptr) {
            return PluginErrorCode::FATAL_ERROR;
        }
        dataCenter_.DataTableRegister<std::pair<std::string, std::string>>(physisToLogicPtr);
    }

    return PluginErrorCode::SUCCESS;
}

bool InstrParser::MergeLog(const InstrLogParser &instrLogParser, const PopLogParser &popParser, MatchMode matchMode)
{
    const std::unordered_map<uint64_t, std::vector<InstrParseInfo>> &instrMap = instrLogParser.GetInstrLog();
    const std::unordered_map<uint64_t, std::vector<PoppedInstrParseInfo>> &popMap = popParser.GetPopLog();
    std::vector<MergeInfo> mergeVec;

    if (!MergeInstr(instrMap, popMap, mergeVec, matchMode)) {
        return false;
    }

    std::shared_ptr<Parse::InstrDetailTable> instrDetailTable = Utility::MakeShared<Parse::InstrDetailTable>(mergeVec);
    if (instrDetailTable == nullptr) {
        LogWarn("Failed to register instr table");
        return false;
    }
    auto userMarkStruct = Utility::MakeShared<UserMarkStruct>();
    if (userMarkStruct == nullptr) {
        LogWarn("Failed to register userMark info");
        return false;
    }
    userMarkStruct->userMarkInstrs = instrLogParser.GetUserMarkInstr();
    userMarkStruct->userMarkInfos = instrLogParser.GetUserMarkInfo();
    if (!dataCenter_.DataTableRegister(instrDetailTable)) {
        return false;
    }
    if (!userMarkStruct->userMarkInfos.empty() && !dataCenter_.DataTableRegister(userMarkStruct)) {
        return false;
    }
    return true;
}

bool InstrParser::MergeInstr(const std::unordered_map<uint64_t, std::vector<InstrParseInfo>> &instrMap,
                             const std::unordered_map<uint64_t, std::vector<PoppedInstrParseInfo>> &popMap,
                             std::vector<MergeInfo> &mergeList, MatchMode matchMode)
{
    if (matchMode == MatchMode::PC_MATCH) {
        MergeInstrByPc(instrMap, popMap, mergeList);
    } else {
        MergeInstrById(instrMap, popMap, mergeList);
    }
    return !mergeList.empty();
}
 
void InstrParser::MergeInstrByPc(const std::unordered_map<uint64_t, std::vector<InstrParseInfo>> &instrMap,
                                 const std::unordered_map<uint64_t, std::vector<PoppedInstrParseInfo>> &popMap,
                                 std::vector<MergeInfo> &mergeList)
{
    // 遍历InstrPopLog下的所有pc组
    for (auto &instrGrp: popMap) {
        uint64_t pc = instrGrp.first;
        std::vector<PoppedInstrParseInfo> instrPoppedVec = instrGrp.second;
        // 关联instrLog的相同pc组
        auto iter = instrMap.find(pc);
        if (iter == instrMap.end()) {
            continue;
        }
        std::vector<InstrParseInfo> instrVec = iter->second;
        size_t size = instrPoppedVec.size();
        if (instrVec.size() != size) {
            size = GetPruneSize(instrPoppedVec, instrVec);
        }
        for (size_t i = 0; i < size; i++) {
            if (instrPoppedVec[i].tick > instrVec[i].tick) {
                LogDebug("Discard instruction %s, start cycle > end cycle, core name is %s, pc is 0x%s,"
                         " tick is %llu", instrPoppedVec[i].name.c_str(), dataParserConfig_.GetCoreName().c_str(),
                         NumToHexString(pc).c_str(), instrPoppedVec[i].tick);
                continue;
            }
            MergeInfo mergeItem;
            mergeItem.pc = pc;
            mergeItem.id = 0;
            InitMergeItem(instrPoppedVec[i], instrVec[i], mergeItem);
            mergeList.push_back(mergeItem);
        }
    }
    sort(mergeList.begin(), mergeList.end(), [](const MergeInfo &l, const MergeInfo &r) {
        return l.pc < r.pc;
    });
}
 
void InstrParser::MergeInstrById(const std::unordered_map<uint64_t, std::vector<InstrParseInfo>> &instrMap,
                                 const std::unordered_map<uint64_t, std::vector<PoppedInstrParseInfo>> &popMap,
                                 std::vector<MergeInfo> &mergeList)
{
    for (auto &instrGrp : popMap) {
        uint64_t id = instrGrp.first;
        std::vector<PoppedInstrParseInfo> instrPoppedVec = instrGrp.second;
        auto iter = instrMap.find(id);
        if (iter == instrMap.end()) {
            continue;
        }
        std::vector<InstrParseInfo> instrVec = iter->second;
        size_t size = instrPoppedVec.size();
        if (size != instrVec.size()) {
            size = std::min(size, instrVec.size());
            instrPoppedVec.resize(size);
            instrVec.resize(size);
        }
        for (size_t i = 0; i < size; ++i) {
            if (instrPoppedVec[i].tick > instrVec[i].tick) {
                LogDebug("Discard instruction %s, start cycle > end cycle, core name is %s, id is %llu,"
                         " tick is %llu", instrPoppedVec[i].name.c_str(), dataParserConfig_.GetCoreName().c_str(),
                         id, instrPoppedVec[i].tick);
                continue;
            }
            MergeInfo mergeItem;
            mergeItem.pc = instrPoppedVec[i].pc;
            mergeItem.id = id;
            InitMergeItem(instrPoppedVec[i], instrVec[i], mergeItem);
            mergeList.push_back(mergeItem);
        }
    }
    sort(mergeList.begin(), mergeList.end(), [](const MergeInfo &l, const MergeInfo &r) {
        return l.id < r.id;
    });
}

void InstrParser::InitMergeItem(const PoppedInstrParseInfo& instrPopped, const InstrParseInfo& instr,
                                MergeInfo& mergeItem) const
{
    mergeItem.startTick = instrPopped.tick;
    mergeItem.endTick = instr.tick;
    mergeItem.pipe = instrPopped.pipe;
    mergeItem.name = instrPopped.name;
    mergeItem.detail = instrPopped.detail;
    mergeItem.spStatus = {}; // pre-design attribute, assign empty
    mergeItem.gprCount = instrPopped.gprCount;
    mergeItem.theoStallCyc = instrPopped.theoStallCyc;
    mergeItem.realStallCyc = instrPopped.realStallCyc;
    mergeItem.warpId = instrPopped.warpId;
    mergeItem.schId = instrPopped.schId;
    mergeItem.processBytes = 0;
    mergeItem.vecUtilization = -1;
    mergeItem.ubReadConflict = -1;
    mergeItem.ubWriteConflict = -1;
}

size_t InstrParser::GetPruneSize(std::vector<PoppedInstrParseInfo> &instrPoppedVec,
                                 std::vector<InstrParseInfo> &instrVec) const
{
    constexpr int maxGapBetweenMicroInstrs = 8;
    if (instrVec.empty() || instrPoppedVec.empty()) {
        return 0;
    }
    if (instrVec.size() > instrPoppedVec.size()) { // size instrVec and instrPoppedVec regularly range from 1 to 100.
        uint64_t lastIdx = 0;
        for (size_t i = 1; i < instrVec.size(); i++) {
            uint64_t tickCur = instrVec[i].tick;
            uint64_t tickPrev = instrVec[lastIdx].tick;
            if (tickCur - tickPrev <= maxGapBetweenMicroInstrs) {
                instrVec[lastIdx] = instrVec[i];
            } else {
                lastIdx++;
                instrVec[lastIdx] = instrVec[i];
            }
        }
        instrVec.resize(lastIdx + 1);
    }
    if (instrVec.size() < instrPoppedVec.size()) {
        uint64_t lastIdx = 0;
        for (size_t i = 1; i < instrPoppedVec.size(); i++) {
            uint64_t tickCur = instrPoppedVec[i].tick;
            uint64_t tickPrev = instrPoppedVec[lastIdx].tick;
            if (tickCur - tickPrev == 1) {
                instrPoppedVec[lastIdx] = instrPoppedVec[i];
            } else {
                lastIdx++;
                instrPoppedVec[lastIdx] = instrPoppedVec[i];
            }
        }
        instrPoppedVec.resize(lastIdx + 1);
    }
    // If the number is not matched before PruneInstr,
    // the bigger instr set is truncated and used as the incomplete data of the ca model.
    return instrVec.size() < instrPoppedVec.size() ? instrVec.size() : instrPoppedVec.size();
}
}
}