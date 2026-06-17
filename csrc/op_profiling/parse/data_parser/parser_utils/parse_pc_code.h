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


#ifndef MSOPT_PARSE_PC_CODE_H
#define MSOPT_PARSE_PC_CODE_H

#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <atomic>
#include "json_parser.h"
#include "profiling/simulator/data_parse/sim_defs.h"
#include "parse/data_center/data_center.h"
#include "profiling/op_prof_data_parse.h"

namespace Profiling {
class ParsePcCode {
public:
    ParsePcCode(std::string dumpPath, const std::set<uint64_t> &pcSet)
        : dumpPath_(std::move(dumpPath)), pcSet_(pcSet) {}
    explicit ParsePcCode(std::string dumpPath) : dumpPath_(std::move(dumpPath)) {}
    void SetPcSet(const std::set<uint64_t> &pcSet) { pcSet_ = pcSet; }
    bool GetPcSetByKernelName(const std::string &kernelName);
    bool Parse();
    uint64_t GetStartPc();
    Pc2CodeMap GetPc2Code() const { return pc2code_; }
    uint64_t AddStartPc2Offset(uint64_t startPC, uint64_t offset) const;

private:
    uint64_t GetStartPcFromTxt() const;
    void GetAllPc(const std::string &start, const std::string &size);
    void GenPc2CodeBySymbol(const std::vector<Symbol> &symbols, uint64_t pc);
    bool GenPc2Code(const std::vector<Offset2Line> &addr2Lines, uint64_t startPc);
    std::vector<std::string> GetPcOffset(uint64_t startPc);
    std::vector<std::string> Int2Hex(const std::set<uint64_t> &inSet) const;
    std::string dumpPath_;
    std::set<uint64_t> pcSet_;
    Pc2CodeMap pc2code_;
    uint64_t startPc_{UINT64_MAX};
    bool startPcCached_{false};
    bool isGetPcOffsetByKernelName_ {false};
};
}
#endif // MSOPT_PARSE_PC_CODE_H
