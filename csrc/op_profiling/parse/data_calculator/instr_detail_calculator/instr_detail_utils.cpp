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


#include "instr_detail_utils.h"

#include <regex>

#include "ustring.h"

namespace Profiling {
namespace Parse {

static constexpr int MIN_LINE_MATCH = 2;  // linematch最小匹配数

bool GetRegValue(const std::regex &pattern, const std::string &instrDetail, uint64_t &resValue, int regValueRadix)
{
    std::smatch lineMatch;
    bool res = std::regex_search(instrDetail, lineMatch, pattern);
    if (!res || lineMatch.size() < MIN_LINE_MATCH) {
        return false;
    }
    return Utility::StoullConverter(lineMatch[1].str(), resValue, regValueRadix, true);
}

bool GetRegString(const std::regex &pattern, const std::string &instrDetail, std::string &resString)
{
    std::smatch lineMatch;
    bool res = std::regex_search(instrDetail, lineMatch, pattern);
    if (!res || lineMatch.size() < MIN_LINE_MATCH) {
        return false;
    }
    resString = lineMatch[1].str();
    return true;
}

bool GetRegValueAuto(const std::regex &pattern, const std::string &instrDetail, uint64_t &resValue)
{
    constexpr uint32_t radixHead = 2;
    std::smatch lineMatch;
    bool res = std::regex_search(instrDetail, lineMatch, pattern);
    if (!res || lineMatch.size() < MIN_LINE_MATCH) {
        return false;
    }
    int regValueRadix = Utility::RADIX_10;
    std::string matchStr = lineMatch[1].str();
    if (matchStr.size() >= radixHead && matchStr[0] == '0' && (matchStr[1] == 'x' || matchStr[1] == 'X')) {
        regValueRadix = Utility::RADIX_16;
        matchStr = matchStr.substr(radixHead);
    }
    return Utility::StoullConverter(matchStr, resValue, regValueRadix, true);
}
}
}
