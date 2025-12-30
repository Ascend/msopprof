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


#include "gpr_live_register_calculator.h"
#include "common/defs.h"
#include "ustring.h"

namespace Profiling {
namespace Parse {

using FunctionType = std::function<void(std::vector<std::string>&, std::vector<std::string>&,
                                        const std::vector<uint16_t>&, std::vector<std::string>&)>;
// The dst register locations must be in ascending order.
const std::map<std::string, std::vector<uint16_t>> A300DstRegisterLocation = {
    {"scalar_add",                {0}},
    {"scalar_sub",                {0}},
    {"scalar_mul",                {0}},
    {"scalar_madd",               {0}},
    {"scalar_div",                {0}},
    {"scalar_mod",                {0}},
    {"scalar_max",                {0}},
    {"scalar_min",                {0}},
    {"scalar_sel",                {0}},
    {"scalar_and",                {0}},
    {"scalar_or",                 {0}},
    {"scalar_xor",                {0}},
    {"scalar_cmp",                 {}},
    {"scalar_cmpn",               {0}},
    {"scalar_ld",                 {0}},
    {"scalar_st",                  {}},
    {"scalar_ld_ddr",             {0}},
    {"scalar_st_ddr",              {}},
    {"scalar_dc_preload",          {}},
    {"scalar_st4",                 {}},
    {"scalar_sqrt",               {0}},
    {"scalar_neg",                {0}},
    {"scalar_abs",                {0}},
    {"scalar_not",                {0}},
    {"scalar_shl",                {0}},
    {"scalar_shr",                {0}},
    {"scalar_sbcnt",              {0}},
    {"scalar_sff",                {0}},
    {"scalar_sbitset",            {0}},
    {"scalar_clz",                {0}},
    {"scalar_sflbits",            {0}},
    {"scalar_conv",               {0}},
    {"scalar_mov_xd_xn",          {0}},
    {"scalar_mov_xd_special",     {0}},
    {"scalar_mov_special_xn",      {}},
    {"scalar_signext",            {0}},
    {"scalar_zeroext",            {0}},
    {"scalar_insert",             {0}},
    {"scalar_insert_ext",         {0}},
    {"scalar_ld_imm",             {0}},
    {"scalar_st_imm",              {}},
    {"scalar_st4_imm",             {}},
    {"scalar_mov_xd_imme16",      {0}},
    {"scalar_movk_xd_imme16",     {0}},
    {"scalar_add_imm12",          {0}},
    {"scalar_mul_imm12",          {0}},
    {"scalar_sub_imm12",          {0}},
    {"scalar_dc_preload_imm12",    {}},
    {"scalar_cmp_imm12",           {}},
    {"scalar_ldp",             {0, 1}},
    {"scalar_stp",                 {}},
    {"scalar_ld_ddr_imm",         {0}},
    {"scalar_st_ddr_imm",          {}},
    {"scalar_sti",                 {}},
    {"scalar_sti_imm",             {}},
    {"scalar_movx8",              {0}},
    {"scalar_mov_spr_imm16",       {}},
    // special instr, different way to get dst register
    {"scalar_ld4",                {0}},
    {"scalar_ld4_imm",            {0}}
};

// The dst register locations must be in ascending order.
const std::map<std::string, std::vector<uint16_t>> A2DstRegisterLocation = {
    {"ADD",                {0}},
    {"SUB",                {0}},
    {"MUL",                {0}},
    {"MADD",               {0}},
    {"DIV",                {0}},
    {"REM",                {0}},
    {"MAX",                {0}},
    {"MIN",                {0}},
    {"SEL",                {0}},
    {"AND",                {0}},
    {"OR",                 {0}},
    {"XOR",                {0}},
    {"CMP",                {}},
    {"CMPN",               {0}},
    {"LD_XD_XN",           {0}},
    {"ST_XD_XN",           {}},
    {"LD_DDR",             {0}},
    {"ST_DDR",             {}},
    {"DC_PRELOAD_XN_XM",   {}},
    {"ST4_XD_XN",          {}},
    {"ST_ATOMIC",          {}},
    {"SQRT",               {0}},
    {"NEG",                {0}},
    {"ABS",                {0}},
    {"NOT",                {0}},
    {"SHL",                {0}},
    {"SHR",                {0}},
    {"SBCNT",              {0}},
    {"SFF",                {0}},
    {"SBITSET",            {0}},
    {"CLZ",                {0}},
    {"SFLBITS",            {0}},
    {"CONV",               {0}},
    {"MOV_XD_XN",          {0}},
    {"MOV_XD_SPR",         {0}},
    {"MOV_SPR_XN",         {}},
    {"SIGNEXT",            {0}},
    {"ZEROEXT",            {0}},
    {"INSERT_XD_XN",       {0}},
    {"INSERT_XD",          {0}},
    {"LD_XD_XN_IMM",       {0}},
    {"ST_XD_XN_IMM",       {}},
    {"ST4_XD_XN_IMM",      {}},
    {"MOV_XD_IMM",         {0}},
    {"MOVK",               {0}},
    {"ADD_IMM",            {0}},
    {"MUL_IMM",            {0}},
    {"SUB_IMM",            {0}},
    {"DC_PRELOAD_XN_IMM",  {}},
    {"CMP_IMM",            {}},
    {"LDP_XI_XJ_XN",       {0, 1}},
    {"STP_XI_XJ_XN",       {}},
    {"LD_DEV_XD_XN_IMM12", {0}},
    {"ST_DEV_XD_XN_IMM12", {}},
    {"STI_XN_XM",          {}},
    {"STI_XN_IMM",         {}},
    {"MOVX8_XD_IMM",       {0}},
    {"MOV_SPR_IMM",        {}},
    {"ST_ATOMIC_IMM",      {1}},
    {"ST_DEV",             {1}},
    {"LD_DEV",             {0}},
    // special instr, different way to get dst register
    {"LD4_XD_XN",          {0}},
    {"LD4_XD_XN_IMM",      {0}}
};

const std::map<std::string, FunctionType> InstrToFunctionMap {
    {"ADD",                       GetNormalDstAndSrcRegister},
    {"SUB",                       GetNormalDstAndSrcRegister},
    {"MUL",                       GetNormalDstAndSrcRegister},
    {"MADD",                      GetNormalDstAndSrcRegister},
    {"DIV",                       GetNormalDstAndSrcRegister},
    {"REM",                       GetNormalDstAndSrcRegister},
    {"MAX",                       GetNormalDstAndSrcRegister},
    {"MIN",                       GetNormalDstAndSrcRegister},
    {"SEL",                       GetNormalDstAndSrcRegister},
    {"AND",                       GetNormalDstAndSrcRegister},
    {"OR",                        GetNormalDstAndSrcRegister},
    {"XOR",                       GetNormalDstAndSrcRegister},
    {"CMP",                       GetNormalDstAndSrcRegister},
    {"CMPN",                      GetNormalDstAndSrcRegister},
    {"LD_XD_XN",                  GetNormalDstAndSrcRegister},
    {"ST_XD_XN",                  GetNormalDstAndSrcRegister},
    {"LD_DDR",                    GetNormalDstAndSrcRegister},
    {"ST_DDR",                    GetNormalDstAndSrcRegister},
    {"DC_PRELOAD_XN_XM",          GetNormalDstAndSrcRegister},
    {"ST4_XD_XN",                 GetNormalDstAndSrcRegister},
    {"ST_ATOMIC",                 GetNormalDstAndSrcRegister},
    {"SQRT",                      GetNormalDstAndSrcRegister},
    {"NEG",                       GetNormalDstAndSrcRegister},
    {"ABS",                       GetNormalDstAndSrcRegister},
    {"NOT",                       GetNormalDstAndSrcRegister},
    {"SHL",                       GetNormalDstAndSrcRegister},
    {"SHR",                       GetNormalDstAndSrcRegister},
    {"SBCNT",                     GetNormalDstAndSrcRegister},
    {"SFF",                       GetNormalDstAndSrcRegister},
    {"SBITSET",                   GetNormalDstAndSrcRegister},
    {"CLZ",                       GetNormalDstAndSrcRegister},
    {"SFLBITS",                   GetNormalDstAndSrcRegister},
    {"CONV",                      GetNormalDstAndSrcRegister},
    {"MOV_XD_XN",                 GetNormalDstAndSrcRegister},
    {"MOV_XD_SPR",                GetNormalDstAndSrcRegister},
    {"MOV_SPR_XN",                GetNormalDstAndSrcRegister},
    {"SIGNEXT",                   GetNormalDstAndSrcRegister},
    {"ZEROEXT",                   GetNormalDstAndSrcRegister},
    {"INSERT_XD_XN",              GetNormalDstAndSrcRegister},
    {"INSERT_XD",                 GetNormalDstAndSrcRegister},
    {"LD_XD_XN_IMM",              GetNormalDstAndSrcRegister},
    {"ST_XD_XN_IMM",              GetNormalDstAndSrcRegister},
    {"ST4_XD_XN_IMM",             GetNormalDstAndSrcRegister},
    {"MOV_XD_IMM",                GetNormalDstAndSrcRegister},
    {"MOVK",                      GetNormalDstAndSrcRegister},
    {"ADD_IMM",                   GetNormalDstAndSrcRegister},
    {"MUL_IMM",                   GetNormalDstAndSrcRegister},
    {"SUB_IMM",                   GetNormalDstAndSrcRegister},
    {"DC_PRELOAD_XN_IMM",         GetNormalDstAndSrcRegister},
    {"CMP_IMM",                   GetNormalDstAndSrcRegister},
    {"LDP_XI_XJ_XN",              GetNormalDstAndSrcRegister},
    {"STP_XI_XJ_XN",              GetNormalDstAndSrcRegister},
    {"LD_DEV_XD_XN_IMM12",        GetNormalDstAndSrcRegister},
    {"ST_DEV_XD_XN_IMM12",        GetNormalDstAndSrcRegister},
    {"STI_XN_XM",                 GetNormalDstAndSrcRegister},
    {"STI_XN_IMM",                GetNormalDstAndSrcRegister},
    {"MOVX8_XD_IMM",              GetNormalDstAndSrcRegister},
    {"MOV_SPR_IMM",               GetNormalDstAndSrcRegister},
    {"ST_ATOMIC_IMM",             GetNormalDstAndSrcRegister},
    {"ST_DEV",                    GetNormalDstAndSrcRegister},
    {"LD_DEV",                    GetNormalDstAndSrcRegister},
    {"scalar_add",                GetNormalDstAndSrcRegister},
    {"scalar_sub",                GetNormalDstAndSrcRegister},
    {"scalar_mul",                GetNormalDstAndSrcRegister},
    {"scalar_madd",               GetNormalDstAndSrcRegister},
    {"scalar_div",                GetNormalDstAndSrcRegister},
    {"scalar_mod",                GetNormalDstAndSrcRegister},
    {"scalar_max",                GetNormalDstAndSrcRegister},
    {"scalar_min",                GetNormalDstAndSrcRegister},
    {"scalar_sel",                GetNormalDstAndSrcRegister},
    {"scalar_and",                GetNormalDstAndSrcRegister},
    {"scalar_or",                 GetNormalDstAndSrcRegister},
    {"scalar_xor",                GetNormalDstAndSrcRegister},
    {"scalar_cmp",                GetNormalDstAndSrcRegister},
    {"scalar_cmpn",               GetNormalDstAndSrcRegister},
    {"scalar_ld",                 GetNormalDstAndSrcRegister},
    {"scalar_st",                 GetNormalDstAndSrcRegister},
    {"scalar_ld_ddr",             GetNormalDstAndSrcRegister},
    {"scalar_st_ddr",             GetNormalDstAndSrcRegister},
    {"scalar_dc_preload",         GetNormalDstAndSrcRegister},
    {"scalar_st4",                GetNormalDstAndSrcRegister},
    {"scalar_sqrt",               GetNormalDstAndSrcRegister},
    {"scalar_neg",                GetNormalDstAndSrcRegister},
    {"scalar_abs",                GetNormalDstAndSrcRegister},
    {"scalar_not",                GetNormalDstAndSrcRegister},
    {"scalar_shl",                GetNormalDstAndSrcRegister},
    {"scalar_shr",                GetNormalDstAndSrcRegister},
    {"scalar_sbcnt",              GetNormalDstAndSrcRegister},
    {"scalar_sff",                GetNormalDstAndSrcRegister},
    {"scalar_sbitset",            GetNormalDstAndSrcRegister},
    {"scalar_clz",                GetNormalDstAndSrcRegister},
    {"scalar_sflbits",            GetNormalDstAndSrcRegister},
    {"scalar_conv",               GetNormalDstAndSrcRegister},
    {"scalar_mov_xd_xn",          GetNormalDstAndSrcRegister},
    {"scalar_mov_xd_special",     GetNormalDstAndSrcRegister},
    {"scalar_mov_special_xn",     GetNormalDstAndSrcRegister},
    {"scalar_signext",            GetNormalDstAndSrcRegister},
    {"scalar_zeroext",            GetNormalDstAndSrcRegister},
    {"scalar_insert",             GetNormalDstAndSrcRegister},
    {"scalar_insert_ext",         GetNormalDstAndSrcRegister},
    {"scalar_ld_imm",             GetNormalDstAndSrcRegister},
    {"scalar_st_imm",             GetNormalDstAndSrcRegister},
    {"scalar_st4_imm",            GetNormalDstAndSrcRegister},
    {"scalar_mov_xd_imme16",      GetNormalDstAndSrcRegister},
    {"scalar_movk_xd_imme16",     GetNormalDstAndSrcRegister},
    {"scalar_add_imm12",          GetNormalDstAndSrcRegister},
    {"scalar_mul_imm12",          GetNormalDstAndSrcRegister},
    {"scalar_sub_imm12",          GetNormalDstAndSrcRegister},
    {"scalar_dc_preload_imm12",   GetNormalDstAndSrcRegister},
    {"scalar_cmp_imm12",          GetNormalDstAndSrcRegister},
    {"scalar_ldp",                GetNormalDstAndSrcRegister},
    {"scalar_stp",                GetNormalDstAndSrcRegister},
    {"scalar_ld_ddr_imm",         GetNormalDstAndSrcRegister},
    {"scalar_st_ddr_imm",         GetNormalDstAndSrcRegister},
    {"scalar_sti",                GetNormalDstAndSrcRegister},
    {"scalar_sti_imm",            GetNormalDstAndSrcRegister},
    {"scalar_movx8",              GetNormalDstAndSrcRegister},
    {"scalar_mov_spr_imm16",      GetNormalDstAndSrcRegister},
    {"scalar_ld4",                GetSpecialDstAndSrcRegister},
    {"scalar_ld4_imm",            GetSpecialDstAndSrcRegister},
    {"LD4_XD_XN",                 GetSpecialDstAndSrcRegister},
    {"LD4_XD_XN_IMM",             GetSpecialDstAndSrcRegister},
};

PluginErrorCode GPRLiveRegisterCalculator::Entry()
{
    // key is register name ,value is instr last used index
    std::map<std::string, uint32_t> registerWithIndex;

    std::shared_ptr<InstrDetailTable> instrDetailTable = dataCenter_.GetDbPtr<InstrDetailTable>();
    if (instrDetailTable == nullptr || instrDetailTable->GetColumnData<MergeInfo>(InstrDetailTable::MERGE_INFO) ==
        nullptr) {
        return PluginErrorCode::NONBLOCKING_ERROR;
    }
    auto mergeInfo = *instrDetailTable->GetColumnData<MergeInfo>(InstrDetailTable::MERGE_INFO);
    for (size_t i = 0; i < instrDetailTable->GetSize(); i++) {
        auto instrName = mergeInfo[i].name;
        std::vector<std::string> dstRegisters;
        std::vector<std::string> srcRegisters;
        if (Common::IsChipSeriesTypeValid(chipType_, Common::ChipProductType::ASCEND910_95_SERIES)) {
            GetDstAndSrcRegisterA5(dstRegisters, srcRegisters, mergeInfo[i]);
        } else {
            GetDstAndSrcRegister(dstRegisters, srcRegisters, mergeInfo[i]);
        }
        UpdateSrcRegister(registerWithIndex, srcRegisters, i);
        UpdateDstRegister(*instrDetailTable, registerWithIndex, dstRegisters, i);
        instrDetailTable->UpdateColumnValue(InstrDetailTable::GPR_COUNT, i, static_cast<int>(registerWithIndex.size()));
    }
    std::vector<std::string> lastRegisters;
    for (const auto &lastRegister : registerWithIndex) {
        lastRegisters.emplace_back(lastRegister.first);
    }
    UpdateDstRegister(*instrDetailTable, registerWithIndex, lastRegisters, instrDetailTable->GetSize());
    return PluginErrorCode::SUCCESS;
}

void GetNormalDstAndSrcRegister(std::vector<std::string> &dstRegisters, std::vector<std::string> &srcRegisters,
                                const std::vector<uint16_t> &dstRegisterLoc, std::vector<std::string> &allRegisters)
{
    if (dstRegisterLoc.empty()) {
        srcRegisters.insert(srcRegisters.end(), allRegisters.begin(), allRegisters.end());
        return;
    }
    for (int i = static_cast<int>(dstRegisterLoc.size()) - 1; i >= 0 ; i--) {
        if (allRegisters.size() > dstRegisterLoc[i]) {
            dstRegisters.emplace_back(allRegisters[dstRegisterLoc[i]]);
            allRegisters.erase(allRegisters.begin() + dstRegisterLoc[i]);
        }
    }
    srcRegisters.insert(srcRegisters.end(), allRegisters.begin(), allRegisters.end());
}

void GetSpecialDstAndSrcRegister(std::vector<std::string> &dstRegisters, std::vector<std::string> &srcRegisters,
                                 const std::vector<uint16_t> &dstRegisterLoc, std::vector<std::string> &allRegisters)
{
    if (dstRegisterLoc.size() != 1 || allRegisters.size() <= dstRegisterLoc[0]) {
        Utility::LogDebug("Parse special register get wrong param");
        return;
    }
    for (size_t i = 0; i < allRegisters.size(); i++) {
        if (i != dstRegisterLoc[0]) {
            srcRegisters.emplace_back(allRegisters[i]);
        }
    }
    std::string firstDstRegister = allRegisters[dstRegisterLoc[0]];
    int num;
    if (!Utility::StoiConverter(firstDstRegister.substr(1), num)) {
        Utility::LogDebug("Failed to get dst register");
        return;
    }
    for (int i = 0; i < 4; i++) { // 4 Register used
        int nextRegister = (num + i) % 32;
        std::string nextDstRegister = "X" + std::to_string(nextRegister);
        dstRegisters.emplace_back(nextDstRegister);
    }
}

void GPRLiveRegisterCalculator::ExtractRegister(const std::string &detail, const std::regex &pattern,
                                                std::vector<std::string> &gprVector) const
{
    std::smatch matches;
    std::string::const_iterator searchStart(detail.cbegin());
    while (std::regex_search(searchStart, detail.cend(), matches, pattern)) {
        for (size_t i = 1; i < matches.size(); ++i) {
            if (matches[i].str().empty()) { continue; }
            gprVector.emplace_back(matches[i].str());
        }
        searchStart = matches.suffix().first;
    }
}

void GPRLiveRegisterCalculator::UpdateSrcRegister(std::map<std::string, uint32_t> &registerWithIndex,
                                                  std::vector<std::string> &srcRegisters, uint32_t index) const
{
    for (const auto &srcRegister : srcRegisters) {
        registerWithIndex[srcRegister] = index;
    }
}

void GPRLiveRegisterCalculator::UpdateDstRegister(InstrDetailTable &instrDetailTable,
    std::map<std::string, uint32_t> &registerWithIndex, std::vector<std::string> &dstRegisters, uint32_t index) const
{
    for (const auto &dstRegister : dstRegisters) {
        if (registerWithIndex.find(dstRegister) == registerWithIndex.end()) {
            registerWithIndex[dstRegister] = index;
        } else {
            uint32_t beforeRegister = registerWithIndex[dstRegister];
            auto func = [](uint32_t value) -> uint32_t {
                return value == 0 ? 0 : value - 1;
            };
            instrDetailTable.UpdateColumnValueInRange<int>(InstrDetailTable::GPR_COUNT, beforeRegister + 1,
                                                           index, func);
            registerWithIndex[dstRegister] = index;
        }
    }
}

void GPRLiveRegisterCalculator::GetDstAndSrcRegisterA5(std::vector<std::string> &dstRegisters,
    std::vector<std::string> &srcRegisters, const MergeInfo &mergeInfo)
{
    const std::string &detail = mergeInfo.detail;
    // david detail : Px:0|P], [Rm:7|R], [Rn:8|R], [Rn1:9|R], [Rd:b|R],  [Rd1:c|R],  [Rd2:d|R]
    std::sregex_iterator it(detail.begin(), detail.end(), pattern_);
    std::sregex_iterator end;

    for (; it != end; ++it) {
        std::smatch match = *it;
        std::string letter = match[1].str();  // 提取字母（如 "m", "n", "d", "d1")
        std::string number = match[2].str();  // 提取gpr（如 "4", "d"）
        if (letter.empty()) {
            continue;
        }
        if (letter.at(0) == 'd') { // Rd代表了dst寄存器
            dstRegisters.emplace_back(number);
        } else {
            srcRegisters.emplace_back(number);
        }
    }
}

void GPRLiveRegisterCalculator::GetDstAndSrcRegister(std::vector<std::string> &dstRegisters,
    std::vector<std::string> &srcRegisters, const MergeInfo &mergeInfo) const
{
    std::vector<std::string> allRegisters;
    const std::string &instrName = mergeInfo.name;
    ExtractRegister(mergeInfo.detail, pattern_, allRegisters);
    std::vector<uint16_t> dstRegisterLoc = {};
    if ((Common::IsChipSeriesTypeValid(chipType_, Common::ChipProductType::ASCEND310P_SERIES)) &&
        A300DstRegisterLocation.count(instrName) > 0 &&
        InstrToFunctionMap.find(instrName) != InstrToFunctionMap.end()) {
        dstRegisterLoc = A300DstRegisterLocation.at(instrName);
        InstrToFunctionMap.at(instrName)(dstRegisters, srcRegisters, dstRegisterLoc, allRegisters);
        return;
    }
    if (((Common::IsChipSeriesTypeValid(chipType_, Common::ChipProductType::ASCEND910_93_SERIES)) ||
        (Common::IsChipSeriesTypeValid(chipType_, Common::ChipProductType::ASCEND910B_SERIES))) &&
        A2DstRegisterLocation.count(instrName) > 0 && InstrToFunctionMap.find(instrName) != InstrToFunctionMap.end()) {
        dstRegisterLoc = A2DstRegisterLocation.at(instrName);
        InstrToFunctionMap.at(instrName)(dstRegisters, srcRegisters, dstRegisterLoc, allRegisters);
        return;
    }
    srcRegisters.insert(srcRegisters.end(), allRegisters.begin(), allRegisters.end());
}
}
}
