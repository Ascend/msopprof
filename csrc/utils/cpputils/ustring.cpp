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


#include <unordered_map>
#include <fnmatch.h>
#include "ustring.h"

namespace Utility {

bool IsStringCharValid(const std::string &inputString, std::string &msg)
{
    const std::unordered_map<std::string, std::string> invalidChar = GetInvalidChar();
    for (auto &item: invalidChar) {
        if (inputString.find(item.first) != std::string::npos) {
            msg = "invalid character: " + item.second;
            return false;
        }
    }
    return true;
}

bool GetUint64ListFromStr(const std::regex &pattern, const std::string &inputStr, std::vector<uint64_t> &resValueList)
{
    std::smatch lineMatch;
    bool res = std::regex_search(inputStr, lineMatch, pattern);
    if (!res) {
        return false;
    }
    uint64_t tmpValue = 0;
    for (size_t i = 1; i < lineMatch.size(); i++) {
        if (!Utility::StoullConverter(lineMatch[i].str(), tmpValue, RADIX_16, true)) {
            return false;
        }
        resValueList.emplace_back(tmpValue);
    }
    return true;
}

bool StollConverter(const std::string &numString, int64_t &num, int32_t radix, bool strict)
{
    if (numString.empty()) {
        return false;
    }
    if (radix == RADIX_10 && !IsDigit(numString)) {
        return false;
    }
    try {
        size_t pos;
        num = static_cast<int64_t>(stoll(numString, &pos, radix));
        if (strict && pos != numString.size()) {
            // check if all character in numString are processed.
            LogDebug("Input string [%s] can not be fully converted", numString.c_str());
            return false;
        }
        return true;
    }
    catch (std::invalid_argument&) {
        LogDebug("Invalid argument [%s]", numString.c_str());
        return false;
    }
    catch (std::out_of_range&) {
        LogDebug("Input out of range");
        return false;
    }
}

bool StoullConverter(const std::string &numString, uint64_t &num, int32_t radix, bool strict)
{
    if (numString.empty()) {
        return false;
    }
    if (radix == RADIX_10 && !IsDigit(numString)) {
        return false;
    }
    try {
        size_t pos;
        num = stoull(numString, &pos, radix);
        if (strict && pos != numString.size()) {
            // check if all character in numString are processed.
            LogDebug("Input string [%s] can not be fully converted", numString.c_str());
            return false;
        }
        return true;
    }
    catch (std::invalid_argument&) {
        LogDebug("Invalid argument [%s]", numString.c_str());
        return false;
    }
    catch (std::out_of_range&) {
        LogDebug("Input out of range");
        return false;
    }
}

bool CheckInputStringValid(std::string const &inputString, uint64_t length)
{
    if (inputString.empty()) {
        return false;
    }
    if (inputString.length() > length) {
        return false;
    }
    std::regex namePattern("^[A-Za-z0-9_]+$");
    if (!std::regex_match(inputString, namePattern)) {
        return false;
    }
    return true;
}

std::string ReplaceSubStr(const std::string& resourceString, const std::string& subString,
                          const std::string& newString, uint32_t count)
{
    std::string dstString = resourceString;
    std::string::size_type pos = 0;
    uint32_t replaceCount = 0;
    while ((pos = dstString.find(subString, pos)) != std::string::npos) {
        if (replaceCount >= count) {
            break;
        }
        dstString.replace(pos, subString.length(), newString);
        pos += newString.length();
        replaceCount++;
    }
    return dstString;
}

bool IsDigit(std::string const &digit)
{
    uint32_t index = 0;
    if ((digit.length() > 0) && (digit[0] == '-' || digit[0] == '+')) {
        index = 1;
    }
    if ((digit.length() == index) || (digit.length() > UINT32_MAX)) {
        return false;
    }
    for (;index < digit.length(); index++) {
        if (!isdigit(digit[index])) {
            return false;
        }
    }
    return true;
}

bool MatchWithPatterns(const std::string &str, const std::vector<std::regex> &patterns)
{
    for (const auto &pattern : patterns) {
        if (std::regex_match(str, pattern)) {
            return true;
        }
    }
    return false;
}

bool StringMatch(const std::string &str, const std::string &subStr)
{
    if (StartsWith(str, subStr)) {
        return true;
    }
    return fnmatch(subStr.c_str(), str.c_str(), 0) == 0;
}

}
