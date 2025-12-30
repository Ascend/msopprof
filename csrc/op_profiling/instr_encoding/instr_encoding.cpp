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


#include "instr_encoding.h"
#include "filesystem.h"
#include "cmd_execute.h"
#include "ascend_helper.h"

using namespace Common;

namespace Encode {
bool InstrEncoding::Init(Common::ChipProductType socVersion)
{
    if (IsChipSeriesTypeValid(socVersion, Common::ChipProductType::ASCEND910B_SERIES) ||
        IsChipSeriesTypeValid(socVersion, Common::ChipProductType::ASCEND910_93_SERIES)) {
        mask2Groups_ = MaskOrder<uint32_t, InstrTable>(MASK2GROUPS_A2);
        return true;
    } else if (IsChipSeriesTypeValid(socVersion, Common::ChipProductType::ASCEND910_95_SERIES)) {
        mask2Groups_ = MaskOrder<uint32_t, InstrTable>(MASK2GROUPS_A5);
        mask2Groups64Bit_ = MaskOrder<uint64_t, InstrTable64>(MASK2GROUPS_A5_64BIT);
        mask2Groups128Bit_ = MASK2GROUPS_A5_128BIT;
        return true;
    }
    Utility::LogWarn("Instr encoding is not supported on current soc.");
    return false;
}

bool InstrEncoding::GenerateEncoding(const std::string &kernelPath, std::vector<EncodingInfo> &encInfo)
{
    std::string pcDump;
    if (kernelPath != lastKernelPath_ && !InstrEncoding::GetEncodingPc(kernelPath, pcDump)) {
        Utility::LogWarn("Get Text Encoding Pc failed");
        return false;
    }

    if (!GetTextEncodingLines(kernelPath, pcDump, encInfo)) {
        Utility::LogError("Get Text Encoding Lines failed");
        return false;
    }

    for (auto &line:encInfo) {
        if (line.bitType == EncodingType::BIT32) {
            UpdateEncoding32(static_cast<uint32_t>(line.enc.low64), line.pipe, line.name);
        } else if (line.bitType == EncodingType::BIT64) {
            UpdateEncoding64(line.enc.low64, line.pipe, line.name);
        } else if (line.bitType == EncodingType::BIT128) {
            UpdateEncoding128(line.enc, line.pipe, line.name);
        }
    }
    return true;
}

bool InstrEncoding::GetEncodingPc(const std::string &kernelPath, std::string &pcDump)
{
    std::string ascendHomePath;
    if (!Utility::GetAscendHomePath(ascendHomePath)) {
        Utility::LogError("Get ASCEND_HOME_PATH failed");
        return false;
    }
    std::vector<std::string> cmd = {"llvm-objdump", "-d", "-j", ".text", kernelPath.c_str()};
    std::map<std::string, std::string> env = {};
    if (!Utility::CmdExecute(cmd, env, pcDump)) {
        Utility::LogDebug("Execute llvm-Objdump failed, can not get decompile info");
        return false;
    }
    return true;
}

bool InstrEncoding::GetTextEncodingLines(const std::string &kernelPath, const std::string &pcDump,
                                         std::vector<EncodingInfo> &encInfo)
{
    std::vector<std::string> cmd = {"llvm-objcopy", "-j", ".text", "-O", "binary"};
    cmd.emplace_back(kernelPath.c_str());
    std::string timeBuf;
    Utility::GenerateTimeStamp(timeBuf);
    std::string outputName = "text_" + timeBuf + ".o";
    if (!Utility::CmdExecuteWithOutput(cmd, outputName)) {
        Utility::LogDebug("Execute llvm-Objcopy failed, can not get decompile info");
        return false;
    }

    std::ifstream binaryFile(outputName, std::ios::binary);
    if (!binaryFile.is_open()) {
        Utility::LogDebug("Failed to Get Text Encoding Lines");
        return false;
    }

    if (!GetEncodingByPc(binaryFile, pcDump, encInfo)) {
        return false;
    }

    binaryFile.close();
    if (std::remove(outputName.c_str()) != 0) {
        Utility::LogDebug("Failed to Remove BinaryFile");
    }
    lastKernelPath_ = kernelPath;
    return true;
}

bool InstrEncoding::GetEncodingByPc(std::ifstream &binaryFile, const std::string &pcDump,
                                    std::vector<EncodingInfo> &encInfo)
{
    uint64_t lastPc = 0;
    std::istringstream pcStream(pcDump);
    std::string line;
    while (std::getline(pcStream, line)) {
        uint64_t curPc = 0;
        if (!ExtractPc(line, curPc)) {
            continue;
        }
        if (curPc <= lastPc) {
            continue;
        }
        uint32_t bitLen = curPc - lastPc;
        binaryFile.seekg(lastPc, std::ios::beg);
        if (!binaryFile.good()) {
            Utility::LogDebug("Find encoding failed by pc");
            return false;
        }
        uint64_t encode[2] = {0, 0};
        EncodingType type = EncodingType::INVALID;
        BinRead(binaryFile, encode, type, bitLen);
        encInfo.push_back({{encode[0], encode[1]}, lastPc, type, "", ""});
        lastPc = curPc;
    }
    uint64_t encode[2] = {0, 0};
    EncodingType type = EncodingType::INVALID;
    int32_t cur = binaryFile.tellg();
    binaryFile.seekg(0, std::ios::end);
    int32_t end = binaryFile.tellg();
    if (end <= cur) {
        Utility::LogDebug("Encoding file is empty.");
        return false;
    }
    binaryFile.seekg(lastPc, std::ios::beg);
    BinRead(binaryFile, encode, type, end - cur);
    encInfo.push_back({{encode[0], encode[1]}, lastPc, type, "", ""});
    return true;
}

void InstrEncoding::BinRead(std::ifstream &binaryFile, uint64_t *encode, EncodingType &type, uint32_t bitLen)
{
    if (bitLen == 4) {  // 4*8=32
        binaryFile.read(reinterpret_cast<char *>(encode), 4); // 4*8=32
        type = EncodingType::BIT32;
    } else if (bitLen == 8) { // 8*8=64
        binaryFile.read(reinterpret_cast<char *>(encode), 8); // 8*8=64
        type = EncodingType::BIT64;
    } else if (bitLen == 16) { // 16*8=128
        binaryFile.read(reinterpret_cast<char *>(encode), 16); // 16*8=128
        type = EncodingType::BIT128;
    } else {
        type = EncodingType::INVALID;
        Utility::LogDebug("Failed to parse encoding bin because of error bit len:%d.", bitLen);
    }
}

bool InstrEncoding::ExtractPc(std::string str, uint64_t &pc)
{
    std::string pcStr;
    size_t seg = str.find(":");
    if (seg == std::string::npos) {
        return false;
    }
    for (int i = static_cast<int>(seg) - 1; i >= 0; --i) {
        if ((str[i] >= '0' && str[i] <= '9') || (str[i] >= 'a' && str[i] <= 'f')) {
            pcStr.insert(pcStr.begin(), str[i]);
        } else if (str[i] == ' ') {
            break;
        } else {
            pcStr.clear();
        }
    }
    if (pcStr.empty()) {
        return false;
    }
    return Utility::StoullConverter(pcStr, pc, Utility::RADIX_16);
}

void InstrEncoding::UpdateEncoding32(uint32_t encoding, std::string &pipe, std::string &name)
{
    for (const auto &mask2group : mask2Groups_) {
        uint32_t encodingKey = encoding & mask2group.first;
        for (const auto &instrTable : mask2group.second) {
            if (instrTable.table.find(encodingKey) != instrTable.table.end()) {
                pipe = instrTable.pipe;
                name = instrTable.table.at(encodingKey);
                InstrUpdate(pipe, encoding);
                return;
            }
        }
    }
    Utility::LogDebug("Encoding32 0x[%x] can not find instruction", encoding);
}

void InstrEncoding::UpdateEncoding64(uint64_t encoding, std::string &pipe, std::string &name)
{
    for (const auto &mask2group : mask2Groups64Bit_) {
        uint64_t encodingKey = encoding & mask2group.first;
        for (const auto &instrTable : mask2group.second) {
            if (instrTable.table.find(encodingKey) != instrTable.table.end()) {
                pipe = instrTable.pipe;
                name = instrTable.table.at(encodingKey);
                return;
            }
        }
    }
    Utility::LogDebug("Encoding64 0x[%lx] can not find instruction", encoding);
}

void InstrEncoding::UpdateEncoding128(Enc128 encoding, std::string &pipe, std::string &name)
{
    for (const auto &mask2group : mask2Groups128Bit_) {
        Enc128 encodingKey = encoding & mask2group.first;
        for (const auto &instrTable : mask2group.second) {
            if (instrTable.table.find(encodingKey) != instrTable.table.end()) {
                pipe = instrTable.pipe;
                name = instrTable.table.at(encodingKey);
                return;
            }
        }
    }
    Utility::LogDebug("Encoding128 0x[%lx][%lx] can not find instruction", encoding.high64, encoding.low64);
}

void InstrEncoding::InstrUpdate(std::string &pipe, uint32_t encoding)
{
    // PIPE为FLOWCTRL的非特殊指令展示为SCALAR, SIMD指令展示为VECTOR
    if (pipe == Common::FC_PIPE) {
        auto tmpPipe = GetSpecialInstrPipeByEncoding(encoding);
        if (!tmpPipe.empty()) {
            pipe = tmpPipe;
        } else {
            pipe = Common::SU_PIPE;
        }
    }
    if (pipe == Common::SIMD_PIPE) {
        pipe = Common::VEC_PIPE;
    }
}

std::string InstrEncoding::GetSpecialInstrPipeByEncoding(uint32_t encoding)
{
    for (const auto &mask2table : mask2InstrWithPipeTable_) {
        auto &table = mask2table.second;
        uint32_t HexCode = encoding & mask2table.first;
        if (table.find(HexCode) != table.end()) {
            HexCode = encoding & PIPE_ID_MASK;
            if (pipeIDTable_.find(HexCode) != pipeIDTable_.end()) {
                return pipeIDTable_.at(HexCode);
            }
        }
    }
    return "";
}
}