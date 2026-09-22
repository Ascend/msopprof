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

#ifndef __CPPUTILS_ELF_HELPER_H__
#define __CPPUTILS_ELF_HELPER_H__

#include <elf.h>
#include <string>
#include <vector>

namespace Utility {

// 保存 ELF 动态段中的原始业务无关信息。路径顺序与 DT_RPATH/DT_RUNPATH 在动态段中的顺序一致。
struct ElfDynamicInfo {
    std::vector<std::string> neededLibraries;
    std::vector<std::string> searchPaths;
};

// 只校验并读取 ELF64 小端文件头，允许目标文件不包含 Program Header 或动态段。
bool ReadElfHeader(const std::string &path, Elf64_Ehdr &header);

// 在文件头校验基础上解析 Program Header 和动态段，提取依赖库及运行时搜索路径。
bool ReadElfDynamicInfo(const std::string &path, ElfDynamicInfo &dynamicInfo);

} // namespace Utility

#endif // __CPPUTILS_ELF_HELPER_H__
