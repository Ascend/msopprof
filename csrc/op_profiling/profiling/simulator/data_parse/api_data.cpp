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


#include "api_data.h"
#include <filesystem.h>
#include <string>
#include <algorithm>
#include <iostream>
#include <set>
#include <iomanip>
#include <common/defs.h>

#include "json.hpp"
#include "log.h"
#include "sim_common_statistic.h"
#include "cmd_execute.h"
#include "common/visualize.h"
#include "thread_pool.h"
#include "sim_defs.h"
#include "profiling/op_prof_data_parse.h"
#include "pc_process.h"

using namespace std;
using namespace Utility;
using namespace Profiling;

namespace Serialization {

void CodeLine::ToJson(nlohmann::json &lineDetails)
{
    lineDetails["Line"] = this->line;
    lineDetails["Cycles"] = this->cycles;
    lineDetails["Instructions Executed"] = this->callCount;
    lineDetails["Address Range"] = this->addrRange;
    lineDetails["GPR Count"] = this->gprCount;
    lineDetails["Process Bytes"] = this->processBytes;
}

bool ApiData::FileStats(CodeInstrData &codeInstrData, const std::string &outputPath, nlohmann::json &apiJson)
{
    nlohmann::json apiFileJson; // for bin file writing
    if (codeInstrData.files.empty()) {
        LogWarn("Code call stack is empty");
        return false;
    }
    apiFileJson["Cores"] = codeInstrData.cores;
    FileDtypeStats(apiFileJson);
    vector<nlohmann::json> fileJsonList;
    for (CodeFile &file: codeInstrData.files) {
        nlohmann::json fileJson;
        fileJson["Source"] = file.file;
        vector<nlohmann::json> linesJson;
        for (CodeLine &line: file.lines) {
            nlohmann::json lineDetails;
            line.ToJson(lineDetails);
            linesJson.emplace_back(lineDetails);
        }
        fileJson["Lines"] = linesJson;
        fileJsonList.emplace_back(fileJson);
    }
    apiJson["Files"] = fileJsonList;
    apiFileJson["Files"] = fileJsonList;
    Visualize::WriteBin<VisualizeType::FILE_API>(outputPath, apiFileJson);
    return true;
}

void ApiData::FileDtypeStats(nlohmann::json &apiJson)
{
    struct FileDtype fileDtype;
    nlohmann::json fileJson;
    fileJson["Address Range"] = fileDtype.addrRange;
    fileJson["Cycles"] = fileDtype.cycles;
    fileJson["Instructions Executed"] = fileDtype.instructionExecuted;
    fileJson["Line"] = fileDtype.line;
    fileJson["GPR Count"] = fileDtype.gprCount;
    fileJson["Process Bytes"] = fileDtype.processBytes;

    nlohmann::json fileDtypeJson;
    fileDtypeJson["Lines"] = fileJson;
    apiJson["Files Dtype"] = fileDtypeJson;
}

void ApiData::InstrDtypeStats(nlohmann::json &apiJson, bool hasStallCyc, bool hasScalarCyc)
{
    struct InstructionsDtype instructionsDtype;
    nlohmann::json instrJson;
    instrJson["Address"] = instructionsDtype.address;
    instrJson["AscendC Inner Code"] = instructionsDtype.ascendCInnerCode;
    instrJson["Cycles"] = instructionsDtype.cycles;
    instrJson["Instructions Executed"] = instructionsDtype.instructionExecuted;
    instrJson["Pipe"] = instructionsDtype.pipe;
    instrJson["Source"] = instructionsDtype.source;
    instrJson["GPR Count"] = instructionsDtype.gprCount;
    instrJson["Process Bytes"] = instructionsDtype.processBytes;
    instrJson["Vector Utilization Percentage"] = instructionsDtype.vecUtilization;
    instrJson["UB Read Conflict"] = instructionsDtype.ubReadConflict;
    instrJson["UB Write Conflict"] = instructionsDtype.ubWriteConflict;
    if (hasStallCyc) {
        instrJson["RealStallCycles"] = instructionsDtype.realStallCyc;
    }
    if (hasScalarCyc) {
        instrJson["ICacheCycles"] = instructionsDtype.iCacheCyc;
        instrJson["CcuCycles"] = instructionsDtype.ccuCyc;
        instrJson["ScalarCycles"] = instructionsDtype.scalarCyc;
    }
    nlohmann::json instrDTypeJson;
    instrDTypeJson["Instructions"] = instrJson;
    apiJson["Instructions Dtype"] = instrDTypeJson;
}

void InstrInfo::ToJson(nlohmann::json &instrDetails, bool hasStallCyc, bool hasScalarCyc) const
{
    instrDetails["Address"] = this->addr;
    instrDetails["AscendC Inner Code"] = this->cceCode;
    instrDetails["Source"] = this->instr;
    instrDetails["Pipe"] = this->pipe;
    instrDetails["Instructions Executed"] = this->callCount;
    instrDetails["Cycles"] = this->cycles;
    if (hasStallCyc) {
        instrDetails["RealStallCycles"] = this->realStallCyc;
    }
    if (hasScalarCyc) {
        instrDetails["ICacheCycles"] = this->iCacheCyc;
        instrDetails["CcuCycles"] = this->ccuCyc;
        instrDetails["ScalarCycles"] = this->scalarCyc;
    }
    instrDetails["GPR Count"] = this->gprCount;
    instrDetails["Process Bytes"] = this->processBytes;
    instrDetails["Vector Utilization Percentage"] = this->vecUtilization;
    instrDetails["UB Read Conflict"] = this->ubReadConflict;
    instrDetails["UB Write Conflict"] = this->ubWriteConflict;
}

bool ApiData::InstrStats(CodeInstrData &codeInstrData, const std::string &outputPath, nlohmann::json &apiJson)
{
    nlohmann::json apiInstrJson; // for bin file writing
    apiInstrJson["Cores"] = codeInstrData.cores;
    vector<nlohmann::json> instrJsonList;
    InstrDtypeStats(apiInstrJson, codeInstrData.hasStallCyc, codeInstrData.hasScalarCyc);
    if (codeInstrData.instrs.empty()) {
        LogError("Instr info list is empty");
        return false;
    }
    for (const InstrInfo &instrInfo: codeInstrData.instrs) {
        nlohmann::json instrDetails;
        instrInfo.ToJson(instrDetails, codeInstrData.hasStallCyc, codeInstrData.hasScalarCyc);
        instrDetails["Source"] = CanonicalizeInstrSource(instrDetails["Source"]);
        instrJsonList.emplace_back(instrDetails);
    }
    apiJson["Instructions"] = instrJsonList;
    apiInstrJson["Instructions"] = instrJsonList;
    Visualize::WriteBin<VisualizeType::INSTR_API>(outputPath, apiInstrJson);
    return true;
}

bool ApiData::VisualizeApiData(const std::string &outputPath, CodeInstrData &codeInstrData)
{
    nlohmann::json apiJson;
    apiJson["Cores"] = codeInstrData.cores;

    if (!FileStats(codeInstrData, outputPath, apiJson)) {
        LogWarn("Lack of code info of files");
    }
    if (!InstrStats(codeInstrData, outputPath, apiJson)) {
        LogWarn("Lack of instruction info");
    }
    return true;
}

string ApiData::CanonicalizeInstrSource(const string &source)
{
    vector<string> sourceVec;
    Split(source, back_inserter(sourceVec), ",");
    vector<string> removes = {"=0x", ":0x"};
    string::size_type idx;
    for (string &str : sourceVec) {
        for (const string &rem : removes) {
            idx = str.find(rem);
            if (idx != string::npos) {
                str.resize(idx);
                break;
            }
        }
    }
    return Join(sourceVec.begin(), sourceVec.end(), ",");
}
}
