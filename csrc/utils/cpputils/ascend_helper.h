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

#ifndef __CPPUTILS_ASCEND_HELPER_H__
#define __CPPUTILS_ASCEND_HELPER_H__

#include <string>
#include <vector>

namespace Utility {

bool GetAscendHomePath(std::string &ascendHomePath);
bool GetSimulators(std::vector<std::string> &simulators);
std::string GetMsopprofPath();
bool GetSocVersionFromEnvVar(std::string &socVersion);
std::string GetSoFromEnvVar(const std::string &soName);
}  // namespace Utility

#endif  // __CPPUTILS_ASCEND_HELPER_H__
