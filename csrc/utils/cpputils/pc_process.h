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

#ifndef MSOPT_PC_PROCESS_H
#define MSOPT_PC_PROCESS_H
#include <vector>
#include <string>
#include <set>
namespace Utility {
std::vector<std::vector<std::string>> MergeAddrRange(const std::set<uint64_t> &addrSet, uint64_t startPc = 0);
std::vector<std::vector<std::string>> MergeAddrRange(const std::set<std::string> &addrSet, uint64_t startPc = 0);
}  // namespace Utility
#endif // MSOPT_PC_PROCESS_H