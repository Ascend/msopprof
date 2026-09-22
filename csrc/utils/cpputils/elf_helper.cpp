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

#include "elf_helper.h"

#include <algorithm>
#include <fstream>
#include <limits>
#include <utility>

namespace Utility {
namespace {

bool OpenElfFile(const std::string &path, std::ifstream &file, uint64_t &fileSize) {
    file.open(path, std::ios::in | std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return false;
    }
    std::streampos end = file.tellg();
    if (end < 0 || static_cast<uint64_t>(end) < sizeof(Elf64_Ehdr)) {
        return false;
    }
    fileSize = static_cast<uint64_t>(end);
    file.seekg(0, std::ios::beg);
    return file.good();
}

template <typename T> bool ReadElfValue(std::ifstream &file, uint64_t fileSize, uint64_t offset, T &value) {
    // 所有偏移和长度先在无符号整数域内校验，避免损坏 ELF 触发越界或整数回绕。
    if (offset > fileSize || sizeof(T) > fileSize - offset ||
        offset > static_cast<uint64_t>(std::numeric_limits<std::streamoff>::max())) {
        return false;
    }
    file.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    file.read(reinterpret_cast<char *>(&value), sizeof(T));
    return file.good();
}

bool ReadElfBytes(std::ifstream &file, uint64_t fileSize, uint64_t offset, uint64_t size, std::vector<char> &value) {
    if (offset > fileSize || size > fileSize - offset ||
        offset > static_cast<uint64_t>(std::numeric_limits<std::streamoff>::max()) ||
        size > static_cast<uint64_t>(std::numeric_limits<std::streamsize>::max())) {
        return false;
    }
    value.resize(static_cast<size_t>(size));
    file.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    file.read(value.data(), static_cast<std::streamsize>(size));
    return file.good();
}

bool IsValidElfHeader(const Elf64_Ehdr &header) {
    // 当前调用方只支持本机可直接解析的 ELF64 小端格式；文件头校验不依赖 Program Header。
    return header.e_ident[EI_MAG0] == ELFMAG0 && header.e_ident[EI_MAG1] == ELFMAG1 &&
        header.e_ident[EI_MAG2] == ELFMAG2 && header.e_ident[EI_MAG3] == ELFMAG3 &&
        header.e_ident[EI_CLASS] == ELFCLASS64 && header.e_ident[EI_DATA] == ELFDATA2LSB;
}

bool LoadProgramHeaders(
    std::ifstream &file, uint64_t fileSize, const Elf64_Ehdr &header, std::vector<Elf64_Phdr> &programHeaders) {
    // 动态段解析才要求完整 Program Header；普通 kernel .o 仍可通过 ReadElfHeader 读取 e_flags。
    if (header.e_phentsize != sizeof(Elf64_Phdr) || header.e_phnum == 0 || header.e_phnum == PN_XNUM ||
        header.e_phoff > fileSize ||
        static_cast<uint64_t>(header.e_phnum) > (fileSize - header.e_phoff) / sizeof(Elf64_Phdr)) {
        return false;
    }
    programHeaders.reserve(header.e_phnum);
    for (uint16_t index = 0; index < header.e_phnum; ++index) {
        Elf64_Phdr programHeader{};
        uint64_t offset = header.e_phoff + static_cast<uint64_t>(index) * sizeof(Elf64_Phdr);
        if (!ReadElfValue(file, fileSize, offset, programHeader)) {
            return false;
        }
        programHeaders.emplace_back(programHeader);
    }
    return true;
}

bool VirtualAddressToFileOffset(
    const std::vector<Elf64_Phdr> &programHeaders, Elf64_Addr address, uint64_t &fileOffset) {
    // 动态表中的字符串表使用虚拟地址，需要借助对应 PT_LOAD 段换算成文件偏移。
    for (const auto &programHeader : programHeaders) {
        if (programHeader.p_type != PT_LOAD || address < programHeader.p_vaddr) {
            continue;
        }
        uint64_t relativeOffset = address - programHeader.p_vaddr;
        if (relativeOffset < programHeader.p_filesz &&
            programHeader.p_offset <= std::numeric_limits<uint64_t>::max() - relativeOffset) {
            fileOffset = programHeader.p_offset + relativeOffset;
            return true;
        }
    }
    return false;
}

bool GetDynamicEntries(std::ifstream &file, uint64_t fileSize, const std::vector<Elf64_Phdr> &programHeaders,
    std::vector<Elf64_Dyn> &dynamicEntries) {
    auto dynamicHeader = std::find_if(programHeaders.begin(), programHeaders.end(),
        [](const Elf64_Phdr &header) { return header.p_type == PT_DYNAMIC; });
    if (dynamicHeader == programHeaders.end() || dynamicHeader->p_filesz < sizeof(Elf64_Dyn) ||
        dynamicHeader->p_offset > fileSize || dynamicHeader->p_filesz > fileSize - dynamicHeader->p_offset) {
        return false;
    }
    uint64_t entryCount = dynamicHeader->p_filesz / sizeof(Elf64_Dyn);
    dynamicEntries.reserve(static_cast<size_t>(entryCount));
    for (uint64_t index = 0; index < entryCount; ++index) {
        Elf64_Dyn entry{};
        uint64_t offset = dynamicHeader->p_offset + index * sizeof(Elf64_Dyn);
        if (!ReadElfValue(file, fileSize, offset, entry)) {
            return false;
        }
        dynamicEntries.emplace_back(entry);
        if (entry.d_tag == DT_NULL) {
            break;
        }
    }
    return true;
}

bool GetStringFromTable(const std::vector<char> &stringTable, Elf64_Xword offset, std::string &value) {
    if (offset >= stringTable.size()) {
        return false;
    }
    auto begin = stringTable.begin() + static_cast<std::ptrdiff_t>(offset);
    auto end = std::find(begin, stringTable.end(), '\0');
    if (end == stringTable.end()) {
        return false;
    }
    value.assign(begin, end);
    return true;
}

} // namespace

bool ReadElfHeader(const std::string &path, Elf64_Ehdr &header) {
    header = {};
    std::ifstream file;
    uint64_t fileSize = 0;
    return OpenElfFile(path, file, fileSize) && ReadElfValue(file, fileSize, 0, header) && IsValidElfHeader(header);
}

bool ReadElfDynamicInfo(const std::string &path, ElfDynamicInfo &dynamicInfo) {
    dynamicInfo = {};
    std::ifstream file;
    uint64_t fileSize = 0;
    Elf64_Ehdr header{};
    if (!OpenElfFile(path, file, fileSize) || !ReadElfValue(file, fileSize, 0, header) || !IsValidElfHeader(header)) {
        return false;
    }

    std::vector<Elf64_Phdr> programHeaders;
    std::vector<Elf64_Dyn> dynamicEntries;
    if (!LoadProgramHeaders(file, fileSize, header, programHeaders) ||
        !GetDynamicEntries(file, fileSize, programHeaders, dynamicEntries)) {
        return false;
    }

    Elf64_Addr stringTableAddress = 0;
    Elf64_Xword stringTableSize = 0;
    for (const auto &entry : dynamicEntries) {
        if (entry.d_tag == DT_STRTAB) {
            stringTableAddress = entry.d_un.d_ptr;
        } else if (entry.d_tag == DT_STRSZ) {
            stringTableSize = entry.d_un.d_val;
        }
    }
    uint64_t stringTableOffset = 0;
    std::vector<char> stringTable;
    if (stringTableAddress == 0 || stringTableSize == 0 ||
        !VirtualAddressToFileOffset(programHeaders, stringTableAddress, stringTableOffset) ||
        !ReadElfBytes(file, fileSize, stringTableOffset, stringTableSize, stringTable)) {
        return false;
    }

    // 保持动态段中的出现顺序；损坏的单个字符串沿用旧逻辑跳过，不影响其他有效条目。
    for (const auto &entry : dynamicEntries) {
        if (entry.d_tag != DT_NEEDED && entry.d_tag != DT_RPATH && entry.d_tag != DT_RUNPATH) {
            continue;
        }
        std::string value;
        if (!GetStringFromTable(stringTable, entry.d_un.d_val, value)) {
            continue;
        }
        if (entry.d_tag == DT_NEEDED) {
            dynamicInfo.neededLibraries.emplace_back(std::move(value));
        } else {
            dynamicInfo.searchPaths.emplace_back(std::move(value));
        }
    }
    return true;
}

} // namespace Utility
