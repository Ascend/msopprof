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


#include "pop_log_parser.h"
#include "filesystem.h"

using namespace Utility;

namespace Profiling {
namespace Parse {

bool PopLogParser::ParseDumpLog(MatchMode matchMode)
{
    const std::string &corePrefix = dataParserConfig_.GetCoreInfo().second;
    std::vector<std::string> splitDumpFileVec = dataParserConfig_.
        GetSplitFilesVec(corePrefix + "instr_popped_log", DUMP_SUFFIX);
    reverse(splitDumpFileVec.begin(), splitDumpFileVec.end());
    ChipProductType chiptTypeProduces = dataParserConfig_.GetProductSeriesType();
    if (chiptTypeProduces == ChipProductType::ASCEND910B_SERIES ||
        chiptTypeProduces == ChipProductType::ASCEND910_93_SERIES) {
        useLogicalCore_ = true;
    }
    if (splitDumpFileVec.empty()) {
        return false;
    }
    for (std::string &splitFile : splitDumpFileVec) {
        std::vector<std::string> fileLines;
        ReadFileByMMap(splitFile, fileLines);
        for (const auto &line : fileLines) {
            ParseLine(line, matchMode);
        }
    }
    return true;
}

int PopLogParser::GetCoreId() const
{
    if (useLogicalCore_) {
        return coreId_;
    }
    std::string coreName = dataParserConfig_.GetCoreName();
    int coreId = 0;
    if (coreName.length() >= 5) { // 5确保core取id不会越界
        coreName = coreName.substr(4); // 4用来取core12.veccore0中12和后面的字符
        StoiConverter(coreName, coreId, RADIX_10);
        return coreId;
    }
    LogWarn("Failed to parse core id, default parse all core info");
    return -1;
}

void PopLogParser::ParseA5Detail(PoppedInstrParseInfo &poppedInstr)
{
    std::smatch matchRes;
    poppedInstr.warpId = DEFAULT_INT_VALUE;
    poppedInstr.schId = DEFAULT_INT_VALUE;
    if (!std::regex_search(poppedInstr.detail, matchRes, a5DetailPattern_)) {
        return;
    }
    uint64_t nextStallCycUint = 0;
    std::string nextStallCyc = matchRes[1];         // stallCyc index = 1
    std::string warpId = matchRes[2];           // warpId index = 2
    std::string schId = matchRes[3];            // schId index = 3
    if (!Utility::StoiConverter(warpId, poppedInstr.warpId, RADIX_10) ||
        !Utility::StoiConverter(schId, poppedInstr.schId, RADIX_16) ||
        !Utility::StoullConverter(nextStallCyc, nextStallCycUint, RADIX_16)) {
        LogWarn("Analyse warpId or schId failed, warpId: [%s], schId: [%s]", warpId.c_str(), schId.c_str());
        return;
    }
    std::string chnId = warpId.append(schId);
    // 此通道首次创建
    if (!poppedInstrListInfo_.simtChnIdMap.count(chnId)) {
        poppedInstrListInfo_.simtChnIdMap[chnId] = poppedInstrListInfo_.simtChnInfoGrp.size();
        poppedInstrListInfo_.simtChnInfoGrp.push_back({nextStallCycUint, poppedInstr.tick});
        return;
    }
    // 此通道已经存在则计算stall cyc
    size_t chnIdInt = poppedInstrListInfo_.simtChnIdMap[chnId];
    poppedInstr.theoStallCyc = poppedInstrListInfo_.simtChnInfoGrp[chnIdInt].stallCyc;
    uint32_t curCyc = 0;
    uint32_t lastCyc = 0;
    if (poppedInstr.tick < poppedInstrListInfo_.simtChnInfoGrp[chnIdInt].tick) {
        Utility::LogDebug("Simt instr was parsed incorrectly, curCyc:%d lastCyc:%d", curCyc, lastCyc);
        return;
    }
    poppedInstr.realStallCyc = poppedInstr.tick - poppedInstrListInfo_.simtChnInfoGrp[chnIdInt].tick;

    // 更新此通道信息
    poppedInstrListInfo_.simtChnInfoGrp[chnIdInt].stallCyc = nextStallCycUint;
    poppedInstrListInfo_.simtChnInfoGrp[chnIdInt].tick = poppedInstr.tick;
}

bool PopLogParser::LineFilter(std::smatch &lineMatch)
{
    std::string popMatchStr = lineMatch[instrPoppedRuleNamePos_["pop_str"]].str();
    if (StartsWith(popMatchStr, "poped from IQ")) {
        // pop_str is the 1st name, break here to filter current instruction.
        return true;
    }
    return dataParserConfig_.DisableSetAndWaitInstr(lineMatch[instrPoppedRuleNamePos_["name"]].str());
}

bool PopLogParser::LineFilter(const PoppedInstrParseInfo &poppedInstrParseInfo)
{
    std::string popMatchStr = poppedInstrParseInfo.detail;
    if (popMatchStr.find("poped from IQ") != std::string::npos) {
        // pop_str is the 1st name, break here to filter current instruction.
        return true;
    }
    return dataParserConfig_.DisableSetAndWaitInstr(poppedInstrParseInfo.name);
}

void PopLogParser::DisposeLine(PoppedInstrParseInfo &poppedInstrParseInfo, MatchMode matchMode)
{
    PipeType pipeType;
    TrimBlank(poppedInstrParseInfo.detail);
    ParseA5Detail(poppedInstrParseInfo);
    poppedInstrParseInfo.pipe = pipeType.FindPipe(poppedInstrParseInfo.pipe, poppedInstrParseInfo.name,
                                                  poppedInstrParseInfo.detail);
    if (matchMode == MatchMode::PC_MATCH) {
        popMap_[poppedInstrParseInfo.pc].emplace_back(poppedInstrParseInfo);
    } else {
        popMap_[poppedInstrParseInfo.id].emplace_back(poppedInstrParseInfo);
    }

    GetLogicalCoreName(poppedInstrParseInfo.name, poppedInstrParseInfo.detail);
}

void PopLogParser::ParseLine(const std::string &line, MatchMode matchMode)
{
    std::smatch lineMatch;
    bool res = regex_match(line, lineMatch, instrPoppedMatchPattern_);
    if (!res) {
        return;
    }
    if (LineFilter(lineMatch)) {
        return;
    }
    std::string pipe = lineMatch[instrPoppedRuleNamePos_["pipe"]].str();

    // 行解析得到原始数据, 不保存特殊寄存器信息
    PoppedInstrParseInfo poppedInstrParseInfo;
    if (!StoullConverter(lineMatch[instrPoppedRuleNamePos_["tick"]].str(), poppedInstrParseInfo.tick, RADIX_10) ||
        !StoullConverter(lineMatch[instrPoppedRuleNamePos_["pc"]].str(), poppedInstrParseInfo.pc, RADIX_16)) {
        return;
    }
    poppedInstrParseInfo.id = 0;
    if (matchMode == MatchMode::ID_MATCH &&
        !StoullConverter(lineMatch[instrPoppedRuleNamePos_["id"]].str(), poppedInstrParseInfo.id)) {
        return;
    }
    poppedInstrParseInfo.pipe = pipe;
    poppedInstrParseInfo.name = lineMatch[instrPoppedRuleNamePos_["name"]].str();
    poppedInstrParseInfo.detail = lineMatch[instrPoppedRuleNamePos_["detail"]].str();
    poppedInstrParseInfo.spStatus = {};
    poppedInstrParseInfo.gprCount = 0;
    poppedInstrParseInfo.theoStallCyc = 0;
    poppedInstrParseInfo.realStallCyc = 0;
    poppedInstrParseInfo.warpId = DEFAULT_INT_VALUE;
    poppedInstrParseInfo.schId = DEFAULT_INT_VALUE;

    DisposeLine(poppedInstrParseInfo, matchMode);
}

void PopLogParser::GetLogicalCoreName(const std::string &name, const std::string &detail)
{
    if (!useLogicalCore_ || name != "MOV_XD_SPR") {
        return;
    }
    if (findCoreId_ && findSubCoreId_) {
        return;
    }
    const std::string &coreName = dataParserConfig_.GetCoreName();
    size_t pos = coreName.find_last_of('.');
    std::string coreType = (pos != std::string::npos) ? coreName.substr(pos + 1) : "";
    std::smatch coreIdResult;
    std::smatch subCoreResult;
    // SubBlockId not exist in cubecore

    if (!findCoreId_ && regex_search(detail, coreIdResult, logicCorePattern_)
        && coreIdResult.size() >= 3 && // 3 防止越界
        Utility::StoiConverter(coreIdResult[2], coreId_, RADIX_16)) { // 2 是blockId，16是进制
        findCoreId_ = true;
        if (coreType == "cubecore0") {
            subCoreId_ = 0;
            findSubCoreId_ = true;
            logicalCorename_ = "core" + std::to_string(coreId_) + ".cubecore" +  std::to_string(subCoreId_);
            return;
        }
    }
    if (!findSubCoreId_ && regex_search(detail, subCoreResult, logicSubCorePattern_)
        && subCoreResult.size() >= 3 && // 3 防止越界
        Utility::StoiConverter(subCoreResult[2], subCoreId_, RADIX_16)) { // 2 是blockId，16是进制
        findSubCoreId_ = true;
    }
    if (findCoreId_ && findSubCoreId_) {
        coreType = (pos != std::string::npos) ? coreName.substr(pos + 1, coreName.length() - pos - 2) : "";
        logicalCorename_ = "core" + std::to_string(coreId_) + "." + coreType +  std::to_string(subCoreId_);
    }
}

void PopLogParser::ParseRealTimeDumpLog(PoppedInstrParseInfo &poppedInstrParseInfo)
{
    if (dataParserConfig_.GetProductSeriesType() != ChipProductType::ASCEND310P_SERIES) {
        useLogicalCore_ = true;
    }
    if (LineFilter(poppedInstrParseInfo)) {
        return;
    }
    DisposeLine(poppedInstrParseInfo);
}
}
}
