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

#include <cstring>
#include <fstream>
#include <unistd.h>

#include <gtest/gtest.h>

#include "elf_helper.h"
#include "filesystem.h"

using namespace Utility;

namespace {
std::string MakeElfTestDir() {
    char path[] = "/tmp/msopprof_elf_helper_XXXXXX";
    char *result = mkdtemp(path);
    return result == nullptr ? "" : result;
}

void CreateHeaderOnlyElf(const std::string &path, Elf64_Half machine, Elf64_Word flags) {
    Elf64_Ehdr header{};
    memcpy(header.e_ident, ELFMAG, SELFMAG);
    header.e_ident[EI_CLASS] = ELFCLASS64;
    header.e_ident[EI_DATA] = ELFDATA2LSB;
    header.e_machine = machine;
    header.e_flags = flags;
    std::ofstream output(path, std::ios::out | std::ios::binary | std::ios::trunc);
    output.write(reinterpret_cast<const char *>(&header), sizeof(header));
}
} // namespace

TEST(ElfHelper, header_only_elf_can_supply_machine_and_flags) {
    std::string root = MakeElfTestDir();
    ASSERT_FALSE(root.empty());
    std::string elfPath = root + "/kernel.o";
    // kernel .o 可以没有 Program Header，平台识别仍应读取到 e_machine 和 e_flags。
    CreateHeaderOnlyElf(elfPath, 0x1029, 0x9a0000);

    Elf64_Ehdr header{};
    ASSERT_TRUE(ReadElfHeader(elfPath, header));
    EXPECT_EQ(header.e_machine, 0x1029);
    EXPECT_EQ(header.e_flags, 0x9a0000U);

    ElfDynamicInfo dynamicInfo;
    EXPECT_FALSE(ReadElfDynamicInfo(elfPath, dynamicInfo));
    std::experimental::filesystem::remove_all(root);
}

TEST(ElfHelper, truncated_or_invalid_file_is_rejected) {
    std::string root = MakeElfTestDir();
    ASSERT_FALSE(root.empty());
    std::string truncatedPath = root + "/truncated.o";
    std::ofstream(truncatedPath, std::ios::out | std::ios::binary).write("ELF", 3);

    Elf64_Ehdr header{};
    EXPECT_FALSE(ReadElfHeader(truncatedPath, header));

    std::string invalidPath = root + "/invalid.o";
    CreateHeaderOnlyElf(invalidPath, 0x1029, 0x9a0000);
    std::fstream invalidFile(invalidPath, std::ios::in | std::ios::out | std::ios::binary);
    char invalidMagic = 0;
    invalidFile.write(&invalidMagic, 1);
    invalidFile.close();
    EXPECT_FALSE(ReadElfHeader(invalidPath, header));
    std::experimental::filesystem::remove_all(root);
}
