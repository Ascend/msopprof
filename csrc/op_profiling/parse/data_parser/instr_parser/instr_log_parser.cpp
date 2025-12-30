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


#include "instr_log_parser.h"
#include "filesystem.h"

using namespace Utility;
namespace Profiling {
namespace Parse {

void InstrLogParser::DeleteUserMarkWithoutEnd()
{
    for (const auto &invalidName : invalidMarkName_) {
        LogWarn("Failed to create userMark %s for %s because loop nesting exists or "
                "TraceStart is not equal to TraceStop.", invalidName.c_str(), coreName_.c_str());
        userMarkMap_.erase(invalidName);
    }
    for (auto &tempMap: userMarkMap_) {
        const std::string &userMarkName = tempMap.first;
        std::vector<UserMarkInfo> &userMarkVec = tempMap.second;
        auto size = userMarkVec.size();
        userMarkVec.erase(std::remove_if(userMarkVec.begin(), userMarkVec.end(),
            [](const UserMarkInfo& x) { return x.endTick == UINT64_MAX; }), userMarkVec.end());
        if (size != userMarkVec.size()) {
            LogWarn("%s cnt of TraceStart is not equal to TraceStop in %s",
                userMarkName.c_str(), coreName_.c_str());
        }
    }
}

bool InstrLogParser::ParseDumpLog(MatchMode matchMode)
{
    std::vector<InstrParseInfo> instrList;
    const auto &corePrefix = dataParserConfig_.GetCoreInfo().second;
    std::vector<std::string> splitDumpFileVec = dataParserConfig_.
        GetSplitFilesVec(corePrefix + "instr_log", DUMP_SUFFIX);
    std::sort(splitDumpFileVec.begin(), splitDumpFileVec.end());
    if (splitDumpFileVec.empty()) {
        return false;
    }
    InstrParseInfo lastInstr;
    std::vector<UserMark> record;
    for (const std::string &splitFile : splitDumpFileVec) {
        std::vector<std::string> fileLine;
        ReadFileByMMap(splitFile, fileLine);
        for (const auto &line : fileLine) {
            ParseLine(line, matchMode);
        }
    }
    DisposeUserMark();
    return true;
}

void InstrLogParser::UpdateMark(InstrParseInfo &instrInfo)
{
    if (instrInfo.name == "NOP_PIPE" && userMarkName_ != "NA" && !userMarkMap_[userMarkName_].empty()) {
        if (userMarkStatus_[userMarkName_]) {
            // 以起始指令后的一个NOP指令作为起点
            userMarkMap_[userMarkName_].back().startTick = instrInfo.tick;
            userMarkName_ = "NA";
        } else {
            // 以结束指令后的最后一个NOP指令作为终点
            userMarkMap_[userMarkName_].back().endTick = instrInfo.tick;
        }
    } else {
        userMarkName_ = "NA";
    }

    if (instrInfo.detail.find("LPCNT") == std::string::npos) { return; }
    std::string markName("Mark 0x");
    std::smatch matchRes;
    std::regex pattern(R"(0x[8c]000000[0-9])");
    if (!regex_search(instrInfo.detail, matchRes, pattern)) { return; }
    markName.push_back(matchRes.str().back());
    instrInfo.name = markName;

    // 0x8 代表标记起始指令, 2: 表示 UserMark是起始还是结束指令的下标号
    if (matchRes.str().at(2) == '8') {
        // 循环嵌套的userMark不进行绘制，记录在vector中后续打印信息提示
        if (!userMarkMap_[markName].empty() && userMarkMap_[markName].back().endTick == UINT64_MAX) {
            invalidMarkName_.insert(markName);
            return;
        }
        userMarkMap_[markName].push_back({instrInfo.tick, UINT64_MAX, instrInfo.pc, UINT64_MAX});
        userMarkName_ = markName;
        userMarkStatus_[markName] = true;
        // 0xc 代表标记结束指令，2: 表示 UserMark是起始还是结束指令的下标号
    } else if (matchRes.str().at(2) == 'c') {
        if (userMarkMap_[markName].empty()) {
            invalidMarkName_.insert(markName);
            return;
        }
        userMarkMap_[markName].back().endTick = instrInfo.tick;
        userMarkMap_[markName].back().endPc = instrInfo.pc;
        userMarkName_ = markName;
        userMarkStatus_[markName] = false;
    }
}

void InstrLogParser::DisposeLine(InstrParseInfo &instrInfo, MatchMode matchMode)
{
    UpdateMark(instrInfo);
    PipeType pipeType;
    TrimBlank(instrInfo.detail);
    instrInfo.pipe = pipeType.FindPipe(instrInfo.pipe, instrInfo.name, instrInfo.detail);
    if (instrInfo.pipe == USER_MARK) {
        userMarkParseInfo_.emplace_back(instrInfo);
        return;
    }
    if (matchMode == MatchMode::PC_MATCH) {
        instrMap_[instrInfo.pc].emplace_back(instrInfo);
    } else {
        instrMap_[instrInfo.id].emplace_back(instrInfo);
    }
}

void InstrLogParser::ParseLine(const std::string &line, MatchMode matchMode)
{
    std::smatch lineMatch;
    bool res = regex_match(line, lineMatch, instrMatchPattern_);
    if (!res) {
        return;
    }

    std::string nameMatchStr = lineMatch[instrRuleNamePos_["name"]].str();
    if (dataParserConfig_.DisableSetAndWaitInstr(nameMatchStr)) {
        return;
    }
    // 行解析得到原始数据, 不保存特殊寄存器信息

    InstrParseInfo instrInfo;
    if (!StoullConverter(lineMatch[instrRuleNamePos_["tick"]].str(), instrInfo.tick, RADIX_10) ||
        !StoullConverter(lineMatch[instrRuleNamePos_["pc"]].str(), instrInfo.pc, RADIX_16)) {
        return;
    }
    instrInfo.id = 0;
    if (matchMode == MatchMode::ID_MATCH &&
        !StoullConverter(lineMatch[instrRuleNamePos_["id"]].str(), instrInfo.id)) {
        return;
    }
    instrInfo.pipe = lineMatch[instrRuleNamePos_["pipe"]].str();
    instrInfo.name = lineMatch[instrRuleNamePos_["name"]].str();
    instrInfo.detail = lineMatch[instrRuleNamePos_["detail"]].str();
    instrInfo.spStatus = {};
    instrInfo.warpId = DEFAULT_INT_VALUE;
    instrInfo.schId = DEFAULT_INT_VALUE;
    DisposeLine(instrInfo, matchMode);
}

// 重组合并UserMark类指令，生成中间件
void InstrLogParser::MergeUserMark()
{
    std::map<std::string, uint32_t> markCnt;
    // 遍历相同pc下的所有指令
    for (size_t i = 0; i < userMarkParseInfo_.size(); i++) {
        const std::string &instrPipe = userMarkParseInfo_[i].pipe;
        std::pair<uint64_t, uint64_t> userMarkTimePoint;
        if ((instrPipe == USER_MARK) &&
            GetUserMark(userMarkMap_, userMarkParseInfo_[i], markCnt, userMarkTimePoint)) {
            MergeInfo mergeItem;
            mergeItem.startTick = userMarkTimePoint.first;
            mergeItem.endTick = userMarkTimePoint.second;
            mergeItem.pc = userMarkParseInfo_[i].pc;
            mergeItem.pipe = userMarkParseInfo_[i].pipe;
            mergeItem.name = userMarkParseInfo_[i].name;
            mergeItem.detail = userMarkParseInfo_[i].detail;
            mergeItem.spStatus = userMarkParseInfo_[i].spStatus;
            mergeItem.gprCount = DEFAULT_INT_VALUE;
            mergeItem.processBytes = DEFAULT_INT_VALUE;
            mergeItem.ubWriteConflict = DEFAULT_INT_VALUE;
            mergeItem.ubReadConflict = DEFAULT_INT_VALUE;
            mergeItem.vecUtilization = static_cast<float>(DEFAULT_INT_VALUE);
            userMarkInstr_.push_back(mergeItem);
        }
    }
}

bool InstrLogParser::GetUserMark(const std::map<std::string, std::vector<UserMarkInfo>> &userMarkMap,
                                 const InstrParseInfo &instrInfo, std::map<std::string, uint32_t> &markCnt,
                                 std::pair<uint64_t, uint64_t> &userMarkTimePoint) const
{
    const std::string &instrName = instrInfo.name;
    const std::string &instrDetail = instrInfo.detail;
    std::smatch matchRes;
    std::regex pattern(R"(0x8000000[0-9])");
    bool found = regex_search(instrDetail, matchRes, pattern);
    if (!found) {
        return false;
    }
    size_t markId = markCnt[instrName];
    if (userMarkMap.count(instrName) == 0) {
        return false;
    }
    if (markId >= userMarkMap.at(instrName).size()) {
        return false;
    }
    userMarkTimePoint.first = userMarkMap.at(instrName).at(markId).startTick;
    userMarkTimePoint.second = userMarkMap.at(instrName).at(markId).endTick;
    markCnt[instrName]++;
    return true;
}

void InstrLogParser::ParseRealTimeDumpLog(InstrParseInfo &instrInfo)
{
    if (dataParserConfig_.DisableSetAndWaitInstr(instrInfo.name)) {
        return;
    }
    DisposeLine(instrInfo);
}

void InstrLogParser::DisposeUserMark()
{
    DeleteUserMarkWithoutEnd();
    MergeUserMark();
}
}
}