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
#include "json.hpp"

using namespace Utility;
namespace Profiling {
namespace Parse {

namespace {
constexpr char const *A5_DETAIL_SEPARATOR = "  {";
constexpr char const *DFX_REGION = "DFX_REGION";
constexpr char const *DFX_XT_VALUE = "xt_value";
constexpr uint64_t DFX_TRACE_START_FLAG = 0x400;
constexpr uint64_t DFX_TRACE_STOP_FLAG = 0xc00;
constexpr uint64_t DFX_TRACE_TYPE_MASK = 0xc00;
constexpr uint64_t DFX_TRACE_ID_MASK = 0x3ff;
constexpr uint64_t DFX_TRACE_MAX_VALUE = 0xfff;

enum class MarkType : uint8_t {
    INVALID = 0U,
    START,
    STOP,
};

struct MarkInfo {
    MarkType type = MarkType::INVALID;
    std::string name;
    bool adjustWithNop = false;
};

std::string GetMarkDetail(const InstrParseInfo &instrInfo)
{
    if (!instrInfo.xnValue.empty()) {
        return instrInfo.xnValue;
    } else if (instrInfo.detail.find(LPCNT_FLAG) != std::string::npos || instrInfo.detail.find(COND_FLAG) != std::string::npos) {
        return instrInfo.detail;
    } else {
        return "";
    }
}

std::string GetDfxRegionValue(const std::string &detail)
{
    const auto detailJson = nlohmann::json::parse(detail, nullptr, false);
    if (detailJson.is_discarded() || !detailJson.is_object()) {
        return "";
    }
    const auto iter = detailJson.find(DFX_XT_VALUE);
    if (iter == detailJson.end() || !iter->is_string()) {
        return "";
    }
    return iter->get<std::string>();
}

MarkInfo DecodeLegacyMark(const InstrParseInfo &instrInfo)
{
    const std::string markDetail = GetMarkDetail(instrInfo);
    if (markDetail.empty()) {
        return {};
    }
    std::smatch matchRes;
    const std::regex pattern(R"(0x[8c]000000[0-9])");
    if (!std::regex_search(markDetail, matchRes, pattern)) {
        return {};
    }
    MarkInfo markInfo;
    markInfo.type = matchRes.str().at(2) == '8' ? MarkType::START : MarkType::STOP;
    markInfo.name = "Mark 0x";
    markInfo.name.push_back(matchRes.str().back());
    markInfo.adjustWithNop = true;
    return markInfo;
}

MarkInfo DecodeDfxRegionMark(const InstrParseInfo &instrInfo, bool checkInstrName)
{
    if (checkInstrName && instrInfo.name != DFX_REGION) {
        return {};
    }
    const std::string markValue = GetDfxRegionValue(instrInfo.detail);
    if (!std::regex_match(markValue, std::regex(R"(0[xX][0-9a-fA-F]+)"))) {
        return {};
    }
    uint64_t value = 0;
    if (!StoullConverter(markValue, value, RADIX_16) || value > DFX_TRACE_MAX_VALUE) {
        return {};
    }
    MarkInfo markInfo;
    const uint64_t traceType = value & DFX_TRACE_TYPE_MASK;
    if (traceType == DFX_TRACE_START_FLAG) {
        markInfo.type = MarkType::START;
    } else if (traceType == DFX_TRACE_STOP_FLAG) {
        markInfo.type = MarkType::STOP;
    } else {
        return {};
    }
    std::ostringstream markName;
    markName << "Mark 0x" << std::hex << std::nouppercase << (value & DFX_TRACE_ID_MASK);
    markInfo.name = markName.str();
    return markInfo;
}

MarkInfo DecodeMark(const InstrParseInfo &instrInfo)
{
    if (instrInfo.name == DFX_REGION) {
        return DecodeDfxRegionMark(instrInfo, true);
    }
    return DecodeLegacyMark(instrInfo);
}

bool IsStartMark(const InstrParseInfo &instrInfo)
{
    MarkInfo markInfo = DecodeLegacyMark(instrInfo);
    if (markInfo.type != MarkType::INVALID) {
        return markInfo.type == MarkType::START;
    }
    return DecodeDfxRegionMark(instrInfo, false).type == MarkType::START;
}

std::string GetCompleteDetail(const std::string &line, const std::string &parsedDetail, MatchMode matchMode)
{
    if (matchMode != MatchMode::ID_MATCH) {
        return parsedDetail;
    }
    const size_t jsonPosition = line.rfind(A5_DETAIL_SEPARATOR);
    if (jsonPosition == std::string::npos) {
        return parsedDetail;
    }
    return line.substr(jsonPosition + 2);
}
}

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

    const MarkInfo markInfo = DecodeMark(instrInfo);
    if (markInfo.type == MarkType::INVALID) {
        return;
    }
    instrInfo.name = markInfo.name;
    instrInfo.pipe = USER_MARK;

    if (markInfo.type == MarkType::START) {
        // 循环嵌套的userMark不进行绘制，记录在vector中后续打印信息提示
        if (!userMarkMap_[markInfo.name].empty() && userMarkMap_[markInfo.name].back().endTick == UINT64_MAX) {
            invalidMarkName_.insert(markInfo.name);
            return;
        }
        userMarkMap_[markInfo.name].push_back({instrInfo.tick, UINT64_MAX, instrInfo.pc, UINT64_MAX});
        if (markInfo.adjustWithNop) {
            userMarkName_ = markInfo.name;
            userMarkStatus_[markInfo.name] = true;
        }
    } else if (markInfo.type == MarkType::STOP) {
        if (userMarkMap_[markInfo.name].empty()) {
            invalidMarkName_.insert(markInfo.name);
            return;
        }
        userMarkMap_[markInfo.name].back().endTick = instrInfo.tick;
        userMarkMap_[markInfo.name].back().endPc = instrInfo.pc;
        if (markInfo.adjustWithNop) {
            userMarkName_ = markInfo.name;
            userMarkStatus_[markInfo.name] = false;
        }
    }
}

void InstrLogParser::DisposeLine(InstrParseInfo &instrInfo, MatchMode matchMode)
{
    UpdateMark(instrInfo);
    PipeType pipeType;
    TrimBlank(instrInfo.detail);
    instrInfo.pipe = pipeType.FindPipe(instrInfo.pipe, instrInfo.name, instrInfo.detail, instrInfo.xnValue);
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
    instrInfo.detail = GetCompleteDetail(line, lineMatch[instrRuleNamePos_["detail"]].str(), matchMode);
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
            mergeItem.xnValue = userMarkParseInfo_[i].xnValue;
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
    if (!IsStartMark(instrInfo)) {
        return false;
    }
    size_t markId = markCnt[instrInfo.name];
    if (userMarkMap.count(instrInfo.name) == 0) {
        return false;
    }
    if (markId >= userMarkMap.at(instrInfo.name).size()) {
        return false;
    }
    userMarkTimePoint.first = userMarkMap.at(instrInfo.name).at(markId).startTick;
    userMarkTimePoint.second = userMarkMap.at(instrInfo.name).at(markId).endTick;
    markCnt[instrInfo.name]++;
    return true;
}

void InstrLogParser::ParseRealTimeDumpLog(InstrParseInfo &instrInfo, MatchMode matchMode)
{
    if (dataParserConfig_.DisableSetAndWaitInstr(instrInfo.name)) {
        return;
    }
    DisposeLine(instrInfo, matchMode);
}

void InstrLogParser::DisposeUserMark()
{
    DeleteUserMarkWithoutEnd();
    MergeUserMark();
}
}
}
