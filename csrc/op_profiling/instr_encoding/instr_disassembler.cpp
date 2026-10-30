/* -------------------------------------------------------------------------
 *  This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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

#include "instr_disassembler.h"

#include <algorithm>
#include <cctype>
#include <sstream>

#include "ascend_helper.h"
#include "cmd_execute.h"
#include "filesystem.h"
#include "log.h"
#include "ustring.h"

using namespace Utility;

namespace Encode {

namespace {
constexpr size_t MIN_HEX_BIN_LEN = 8;
}

std::string InstrDisassembler::GetObjdumpPath()
{
    // 只有CANN自带的llvm-objdump才能正确反汇编算子.o，未找到时不走PATH兜底，直接返回空串。
    std::string ascendHomePath;
    if (!Utility::GetAscendHomePath(ascendHomePath)) {
        return "";
    }
    const std::vector<std::string> candidates = {
        Utility::JoinPath({ascendHomePath, "tools", "bisheng_compiler", "bin", "llvm-objdump"}),
        Utility::JoinPath({ascendHomePath, "tools", "ccec_compiler", "bin", "llvm-objdump"}),
        Utility::JoinPath({ascendHomePath, "bin", "llvm-objdump"}),
    };
    for (const auto &candidate : candidates) {
        if (Utility::IsExist(candidate)) {
            return candidate;
        }
    }
    return "";
}

InstrDisassembler::~InstrDisassembler()
{
    LogUnmatchedPcs();
}

bool InstrDisassembler::Init(const std::string &kernelPath) {
    if (kernelPath.empty()) {
        Utility::LogDebug("Kernel path is empty, skip disassemble");
        return false;
    }
    const std::string objdumpPath = GetObjdumpPath();
    if (objdumpPath.empty()) {
        Utility::LogDebug("Can not find CANN llvm-objdump, skip disassemble");
        return false;
    }
    std::vector<std::string> cmd = {objdumpPath, "-d", kernelPath};
    std::map<std::string, std::string> env;
    std::string output;
    if (!Utility::CmdExecute(cmd, env, output)) {
        Utility::LogDebug("Execute %s -d failed, can not get disassemble info", objdumpPath.c_str());
        return false;
    }
    std::vector<std::string> lines;
    Utility::SplitString(output, '\n', lines);
    for (const std::string &line : lines) {
        InstrBinary binary;
        std::string disasm;
        if (!ParseObjdumpLine(line, binary, disasm)) {
            continue;
        }
        binary2Disasm_[static_cast<size_t>(binary.bitType)][binary] = disasm;
    }
    return IsValid();
}

InstrBinary InstrDisassembler::TruncateBinary(const InstrBinary &binary, EncodingType type)
{
    return FromEncoding(
        static_cast<uint64_t>(binary.words[0]) | (static_cast<uint64_t>(binary.words[1]) << 32U),
        static_cast<uint64_t>(binary.words[2]) | (static_cast<uint64_t>(binary.words[3]) << 32U), type);
}

void InstrDisassembler::BuildTruncatedIndex() const {
    std::lock_guard<std::mutex> lock(truncatedIndexMutex_);
    if (truncatedIndexBuilt_) {
        return;
    }
    for (auto &index : truncated2Disasm_) {
        index.clear();
    }
    // Each complete encoding contributes only to narrower indexes. Repeated objdump
    // encodings have already been deduplicated by binary2Disasm_.
    for (size_t width = 1; width < binary2Disasm_.size(); ++width) {
        for (const auto &entry : binary2Disasm_[width]) {
            for (size_t prefix = 0; prefix < width; ++prefix) {
                const auto key = TruncateBinary(entry.first, static_cast<EncodingType>(prefix));
                auto result = truncated2Disasm_[prefix].emplace(key, &entry.second);
                if (!result.second) {
                    // Missing upper bits cannot distinguish these complete instructions.
                    result.first->second = nullptr;
                }
            }
        }
    }
    truncatedIndexBuilt_ = true;
}

std::string InstrDisassembler::FindDisassembly(const InstrBinary &binary, EncodingType type,
                                              bool &directMatch) const
{
    directMatch = false;
    const size_t index = static_cast<size_t>(type);
    if (binary.empty() || index >= binary2Disasm_.size()) {
        return "";
    }
    const auto key = TruncateBinary(binary, type);
    const auto &table = binary2Disasm_[index];
    const auto iter = table.find(key);
    if (iter != table.end()) {
        directMatch = true;
        return iter->second;
    }
    if (index >= truncated2Disasm_.size()) {
        return "";
    }
    BuildTruncatedIndex();
    const auto &truncatedTable = truncated2Disasm_[index];
    const auto truncatedIter = truncatedTable.find(key);
    return truncatedIter == truncatedTable.end() || truncatedIter->second == nullptr ? "" : *truncatedIter->second;
}

std::string InstrDisassembler::GetDisassembly(const InstrBinary &binary, EncodingType type, uint64_t pc) const
{
    bool directMatch = false;
    const auto disasm = FindDisassembly(binary, type, directMatch);
    if (!directMatch) {
        std::lock_guard<std::mutex> lock(unmatchedPcsMutex_);
        unmatchedPcs_.insert(pc);
    }
    return disasm;
}

void InstrDisassembler::LogUnmatchedPcs() const
{
    std::set<uint64_t> pcs;
    {
        std::lock_guard<std::mutex> lock(unmatchedPcsMutex_);
        pcs.swap(unmatchedPcs_);
    }
    for (uint64_t pc : pcs) {
        Utility::LogDebug("Instruction binary did not match directly, pc=0x%llx", static_cast<unsigned long long>(pc));
    }
}

InstrBinary InstrDisassembler::FromEncoding(uint64_t low64, uint64_t high64, EncodingType type)
{
    InstrBinary binary;
    if (type != EncodingType::BIT32 && type != EncodingType::BIT64 && type != EncodingType::BIT128) {
        return binary;
    }
    binary.bitType = type;
    binary.words[0] = static_cast<uint32_t>(low64);
    if (type != EncodingType::BIT32) {
        binary.words[1] = static_cast<uint32_t>(low64 >> 32U);
    }
    if (type == EncodingType::BIT128) {
        binary.words[2] = static_cast<uint32_t>(high64);
        binary.words[3] = static_cast<uint32_t>(high64 >> 32U);
    }
    return binary;
}

InstrBinary InstrDisassembler::ParseBinary(const std::string &rawBinary)
{
    const std::string hex = NormalizeBinary(rawBinary);
    InstrBinary binary;
    switch (hex.size()) {
        case 8: binary.bitType = EncodingType::BIT32; break;
        case 16: binary.bitType = EncodingType::BIT64; break;
        case 32: binary.bitType = EncodingType::BIT128; break;
        default: return binary;
    }
    for (size_t i = 0; i < hex.size(); ++i) {
        const char c = hex[hex.size() - 1 - i];
        if (!IsHexChar(c)) {
            return InstrBinary{};
        }
        const uint32_t digit = c <= '9' ? c - '0' : c - 'a' + 10;
        binary.words[i / 8] |= digit << ((i % 8) * 4);
    }
    return binary;
}

std::string InstrDisassembler::NormalizeBinary(const std::string &rawBinary)
{
    // 日志中SIMT指令的128位binary会拆成两段(空格分隔)，真实binary需要反序拼接
    std::istringstream iss(rawBinary);
    std::vector<std::string> parts;
    std::string part;
    while (iss >> part) {
        parts.push_back(part);
    }
    std::string result;
    for (auto iter = parts.rbegin(); iter != parts.rend(); ++iter) {
        std::string hexPart = *iter;
        if (hexPart.size() >= 2 && hexPart[0] == '0' && (hexPart[1] == 'x' || hexPart[1] == 'X')) {
            hexPart = hexPart.substr(2);
        }
        result += hexPart;
    }
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

bool InstrDisassembler::ParseObjdumpLine(const std::string &line, InstrBinary &binary, std::string &disasm)
{
    size_t colonPos = line.find(':');
    if (colonPos == std::string::npos) {
        return false;
    }
    // 反汇编行的格式为: "<pc>: <binary> <指令> <细节>"，
    // 符号头行如 "0000000000000000 <symbol>:" 冒号后没有binary，会被过滤掉。
    std::string rest = line.substr(colonPos + 1);
    size_t begin = rest.find_first_not_of(" \t");
    if (begin == std::string::npos) {
        return false;
    }
    size_t end = rest.find_first_of(" \t", begin);
    std::string token = (end == std::string::npos) ? rest.substr(begin) : rest.substr(begin, end - begin);
    if (token.size() < MIN_HEX_BIN_LEN || !std::all_of(token.begin(), token.end(), IsHexChar)) {
        return false;
    }
    binary = ParseBinary(token);
    if (binary.empty()) {
        return false;
    }
    disasm = (end == std::string::npos) ? "" : rest.substr(end);
    TrimBothBlank(disasm);
    CollapseBlank(disasm);
    TrimBothBlank(disasm);
    return !disasm.empty();
}
}
