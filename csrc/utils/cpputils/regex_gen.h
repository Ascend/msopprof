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

#ifndef __CPPUTILS_REGEX_GEN_H__
#define __CPPUTILS_REGEX_GEN_H__

#include <string>
#include <set>

namespace Utility {
class RegexGen {
public:
    RegexGen(void) = default;
    RegexGen(const std::vector<std::string> rules, const std::vector<std::string> names);
    std::string GenPattern(std::string const &patternName);
    std::map<std::string, std::string> definitions_;
    std::map<std::string, std::int64_t> names_;

private:
    bool BuildDefinition(const std::vector<std::string> &rules, const std::vector<std::string> &names);
    bool CheckNames(const std::vector<std::string> &names);
    std::string UpdateDefinition(const std::string &patternRaw);
    int ProcessNormalToken(const std::string &patternRaw, const int pos) const;
    std::pair<std::string, int> ProcessCurly(std::string const &patternRaw, int pos);
};
}  // namespace Utility

#endif  // __CPPUTILS_REGEX_GEN_H__