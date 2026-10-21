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
#include <iomanip>
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

bool InstrDisassembler::Init(const std::string &kernelPath)
{
    binary2Disasm_.clear();
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
        std::string binary;
        std::string disasm;
        if (!ParseObjdumpLine(line, binary, disasm)) {
            continue;
        }
        binary2Disasm_[binary] = disasm;
    }
    return !binary2Disasm_.empty();
}

void InstrDisassembler::Clear()
{
    binary2Disasm_.clear();
}

std::string InstrDisassembler::GetDisassembly(const std::string &binary) const
{
    if (binary.empty()) {
        return "";
    }
    auto iter = binary2Disasm_.find(binary);
    return iter == binary2Disasm_.end() ? "" : iter->second;
}

std::string InstrDisassembler::EncodingToBinary(uint64_t low64, uint64_t high64, EncodingType bitType) {
    std::ostringstream oss;
    oss << std::hex << std::nouppercase;
    switch (bitType) {
    case EncodingType::BIT32:
        oss << std::setw(8) << std::setfill('0') << (low64 & 0xFFFFFFFFULL);
        break;
    case EncodingType::BIT64:
        oss << std::setw(16) << std::setfill('0') << low64;
        break;
    case EncodingType::BIT128:
        // objdump打印128位binary为 高64位+低64位(各16 hex)
        oss << std::setw(16) << std::setfill('0') << high64 << std::setw(16) << std::setfill('0') << low64;
        break;
    default:
        return "";
    }
    std::string result = oss.str();
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
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

bool InstrDisassembler::ParseObjdumpLine(const std::string &line, std::string &binary, std::string &disasm)
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
    binary = NormalizeBinary(token);
    disasm = (end == std::string::npos) ? "" : rest.substr(end);
    TrimBothBlank(disasm);
    CollapseBlank(disasm);
    TrimBothBlank(disasm);
    return !disasm.empty();
}
}
