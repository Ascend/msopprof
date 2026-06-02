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

#ifndef __CPPUTILS_STRING_H__
#define __CPPUTILS_STRING_H__

#include <iomanip>
#include <string>
#include <set>
#include <regex>
#include <cstdint>

#include "sstream"
#include "log.h"

namespace Utility {
constexpr int32_t RADIX_10 = 10;
constexpr int32_t RADIX_16 = 16;

template<typename Iterator>
inline std::string Join(Iterator beg, Iterator end, std::string const &sep = " ")
{
    std::string ret;
    for (auto it = beg; it != end; ++it) {
        if (!it->empty()) {
            if (!ret.empty()) {
                ret.append(sep);
            }
            ret.append(*it);
        }
    }
    return ret;
}

bool IsDigit(std::string const &digit);

template <class Type>
bool StringToNum(const std::string& str, Type &number)
{
    if (!IsDigit(str)) {
        return false;
    }
    std::istringstream iss(str);
    Type num;
    iss >> num;
    if (iss.fail()) {
        return false;
    }
    number = num;
    return true;
}

inline void SplitString(const std::string &str, const char split, std::vector<std::string> &res)
{
    std::istringstream iss(str);
    std::string token;
    while (getline(iss, token, split)) {
        res.emplace_back(token);
    }
}

inline void SplitString(const std::string &str, const char split, std::set<std::string> &res)
{
    std::istringstream iss(str);
    std::string token;
    while (getline(iss, token, split)) {
        res.insert(token);
    }
}

template <class T>
std::set<T> SplitString(const std::string &str, const char split)
{
    std::set<std::string> strSet;
    std::set<T> tSet;
    Utility::SplitString(str, split, strSet);
    for (const auto &s : strSet) {
        T id;
        if (Utility::StringToNum<T>(s, id)) {
            tSet.insert(id);
        }
    }
    if (strSet.size() != tSet.size()) {
        LogWarn("Split string to number failed");
    }
    return tSet;
}

inline std::string Strip(std::string const &str, std::string const &whitespaces)
{
    size_t first = str.find_first_not_of(whitespaces);
    if (first == std::string::npos) {
        return str;
    }
    size_t last = str.find_last_not_of(whitespaces);
    return str.substr(first, (last - first + 1));
}

inline bool StartsWith(const std::string &str, const std::string &prefix)
{
    return (str.rfind(prefix, 0) == 0);
}

inline bool EndsWith(const std::string &str, const std::string& suffix)
{
    if (suffix.length() > str.length()) { return false; }
    return (str.rfind(suffix) == (str.length() - suffix.length()));
}

inline bool EndsWith(const std::string &str, const std::vector<std::string> &suffixList)
{
    for (const std::string& suffix : suffixList) {
        if (suffix.length() > str.length()) { continue; }
        if (str.rfind(suffix) == (str.length() - suffix.length())) {
            return true;
        }
    }
    return false;
}

inline void TrimBlank(std::string &str)
{
    std::string::size_type idx = 0;
    for (std::string::size_type i = 0; i < str.size(); ++i) {
        if (str[i] != ' ') {
            str[idx++] = str[i];
        }
    }
    str.resize(idx);
}

inline bool StoiConverter(const std::string &numString, int32_t &num, int32_t radix = RADIX_10)
{
    try {num = stoi(numString, nullptr, radix);}
    catch (std::invalid_argument&) {
        LogDebug("Invalid argument [%s]", numString.c_str());
        return false;
    }
    catch (std::out_of_range&) {
        LogDebug("Input out of range");
        return false;
    }
    return true;
}

inline std::string NumToHexString(uint64_t num)
{
    std::stringstream ss;
    ss << "0x" << std::hex << num;
    return ss.str();
}

inline std::string NumToHexString(uint64_t num, int size)
{
    std::stringstream ss;
    ss << "0x" << std::hex << std::setw(size) << std::setfill('0') << num;
    return ss.str();
}

inline std::string ToUpper(const std::string &s) {
    std::string res = s;
    for (auto& c : res) {
        c = (char)toupper((unsigned char)c);
    }
    return res;
}

std::string KernelNameConver(const std::string& kernelName);
bool IsStringCharValid(const std::string &inputString, std::string &msg);
bool GetUint64ListFromStr(const std::regex &pattern, const std::string &inputStr, std::vector<uint64_t> &resValueList);
bool StollConverter(const std::string &numString, int64_t &num, int32_t radix = RADIX_10, bool strict = true);
bool StoullConverter(const std::string &numString, uint64_t &num, int32_t radix = RADIX_10, bool strict = true);
bool CheckInputStringValid(std::string const &inputString, uint64_t length);
std::string ReplaceSubStr(const std::string& resourceString, const std::string& subString,
                          const std::string& newString, uint32_t count = UINT32_MAX);
bool MatchWithPatterns(const std::string &str, const std::vector<std::regex> &patterns);
bool StringMatch(const std::string &str, const std::string &subStr);
} // namespace Utility

#endif  // __CPPUTILS_STRING_H__
