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


#ifndef MSOPT_SERIALIZE_STATISTIC_H
#define MSOPT_SERIALIZE_STATISTIC_H
#include <string>
#include <vector>
#include <map>
#include <set>
#include <regex>

#include "sim_common_statistic.h"
#include "common/visualize.h"

namespace Serialization {

struct CoreExe {
    std::string coreName;
    float durationTimeUs;
    float runningTime;
};

struct CodeExe {
    int callCount;
    int cycles;
    float runningTimeUS;
    std::string code;
};

struct InstrExe {
    std::string instr;
    std::string addr;
    std::string pipe;
    int callCount;
    int cycles;
    float runningTime;
    std::string detail;
};

struct ExeNames {
    std::vector<std::string> InstrHeads = {"instr", "addr", "pipe", "call_count", "cycles",
                                           "running_time(us)", "detail"};
    std::vector<std::string> CodeHeads = {"code", "call_count", "cycles", "running_time(us)"};
    std::vector<std::string> CoreHeads = {"core_name", "duration_time(us)", "running_time(us)"};
};

struct CodeLine {
    int line;
    std::vector<int> cycles;
    std::vector<int> callCount;
    std::vector<int> gprCount;
    std::vector<int> processBytes;
    std::vector<std::vector<std::string>> addrRange;
    void ToJson(nlohmann::json &lineDetails);
};

struct CodeFile {
    std::string file;
    std::vector<CodeLine> lines;
};

struct InstrInfo {
    std::string addr;
    std::string cceCode;
    std::string instr;
    std::string pipe;
    std::vector<int> gprCount;
    std::vector<int> processBytes;
    std::vector<float> vecUtilization;
    std::vector<int> ubReadConflict;
    std::vector<int> ubWriteConflict;
    std::vector<int> callCount;
    std::vector<int> cycles;
    std::vector<int> realStallCyc;
    std::vector<int> iCacheCyc;
    std::vector<int> ccuCyc;
    std::vector<int> scalarCyc;
    void ToJson(nlohmann::json &instrDetails, bool hasStallCyc, bool hasScalarCyc) const;
};

struct CodeInstrData {
    bool hasStallCyc;
    bool hasScalarCyc;
    std::vector<std::string> cores;
    std::vector<CodeFile> files;
    std::vector<InstrInfo> instrs;
};

struct FileDtype {
    int addrRange = static_cast<int>(Utility::VisualizeBinDType::DO_NOT_DISPLAY);
    int cycles = static_cast<int>(Utility::VisualizeBinDType::INT);
    int instructionExecuted = static_cast<int>(Utility::VisualizeBinDType::INT);
    int line = static_cast<int>(Utility::VisualizeBinDType::INT);
    int gprCount = static_cast<int>(Utility::VisualizeBinDType::INT);
    int processBytes = static_cast<int>(Utility::VisualizeBinDType::INT);
};

struct InstructionsDtype {
    int address = static_cast<int>(Utility::VisualizeBinDType::STRING);
    int ascendCInnerCode = static_cast<int>(Utility::VisualizeBinDType::STRING);
    int cycles = static_cast<int>(Utility::VisualizeBinDType::INT);
    int instructionExecuted = static_cast<int>(Utility::VisualizeBinDType::INT);
    int pipe = static_cast<int>(Utility::VisualizeBinDType::STRING);
    int source = static_cast<int>(Utility::VisualizeBinDType::STRING);
    int gprCount = static_cast<int>(Utility::VisualizeBinDType::INT);
    int processBytes = static_cast<int>(Utility::VisualizeBinDType::INT);
    int vecUtilization = static_cast<int>(Utility::VisualizeBinDType::PERCENTAGE);
    int ubReadConflict = static_cast<int>(Utility::VisualizeBinDType::INT);
    int ubWriteConflict = static_cast<int>(Utility::VisualizeBinDType::INT);
    int realStallCyc = static_cast<int>(Utility::VisualizeBinDType::INT);
    int iCacheCyc = static_cast<int>(Utility::VisualizeBinDType::INT);
    int ccuCyc = static_cast<int>(Utility::VisualizeBinDType::INT);
    int scalarCyc = static_cast<int>(Utility::VisualizeBinDType::INT);
};

struct UpdateCode {
    int callCount;
    int cycles;
    int gprCount;
    int processBytes;
    std::string code;
    std::string coreName;
    std::set<std::string> pcSet;
};

class ApiData {
public:
    static bool VisualizeApiData(const std::string &outputPath, CodeInstrData &codeInstrData);
    static void FileDtypeStats(nlohmann::json &apiJson);
    static void InstrDtypeStats(nlohmann::json &apiJson, bool hasStallCyc, bool hasScalarCyc);
    static bool FileStats(CodeInstrData &codeInstrData, const std::string &outputPath, nlohmann::json &apiJson);
    static bool InstrStats(CodeInstrData &codeInstrData, const std::string &outputPath, nlohmann::json &apiJson);

private:
    static std::string CanonicalizeInstrSource(const std::string &source);
};
}

#endif // MSOPT_SERIALIZE_STATISTIC_H
