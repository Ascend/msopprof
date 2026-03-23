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

#ifndef __CPPUTILS_MAKE_H__
#define __CPPUTILS_MAKE_H__

#include <type_traits>
#include <string>
#include <unordered_map>
#include <vector>
#include "securec.h"

namespace Utility {
constexpr int MAX_PRINT = 1000;
std::string ToSafeString(const std::string &str);
const std::unordered_map<std::string, std::string>& GetInvalidChar(void);

enum class LogLv {
    DEBUG = 0,
    INFO,
    WARN,
    ERROR,
    COUNT
};

inline bool operator<(LogLv a, LogLv b)
{
    using underlying = typename std::underlying_type<LogLv>::type;
    return static_cast<underlying>(a) < static_cast<underlying>(b);
}

class Log {
public:
    static Log &GetLog(void);

    template<typename... Args>
    inline void Printf(std::string const &format, LogLv lv, Args &&... args) const;
    void SetLogLevelByEnvVar();
    LogLv GetLogLv() const { return lv_; }

private:
    Log(void) = default;
    ~Log(void) = default;
    Log(Log const &) = delete;
    Log &operator=(Log const &) = delete;
    std::string AddPrefixInfo(std::string const &format, LogLv lv) const;

private:
    LogLv lv_ { LogLv::INFO };
    FILE *fp_ { stdout };
};

template <typename... Args>
void Log::Printf(const std::string &format, LogLv lv, Args &&...args) const
{
    if (fp_ == nullptr) {
        return;
    }
    if (lv < lv_) {
        return;
    }
    std::string f = AddPrefixInfo(format, lv).append("\n");

    char msg[MAX_PRINT] = {0};
    auto res = snprintf_s(msg, MAX_PRINT, MAX_PRINT - 1, f.c_str(), std::forward<Args>(args)...);
    if (res >= 0) {
        fprintf(fp_, "%s", msg);
        return;
    }
    std::string lengthLimit = "Log length reach limit,only show part message";
    lengthLimit = AddPrefixInfo(lengthLimit, lv).append("\n");
    fprintf(fp_, "%s", lengthLimit.c_str());
    fprintf(fp_, "%s \n", msg);
}

template <typename... Args>
inline void LogDebug(std::string const &format, Args &&...args)
{
    Log::GetLog().Printf(ToSafeString(format), LogLv::DEBUG, std::forward<Args>(args)...);
}

template <typename... Args>
inline void LogInfo(std::string const &format, Args &&...args)
{
    Log::GetLog().Printf(ToSafeString(format), LogLv::INFO, std::forward<Args>(args)...);
}

template <typename... Args>
inline void LogWarn(std::string const &format, Args &&...args)
{
    Log::GetLog().Printf(ToSafeString(format), LogLv::WARN, std::forward<Args>(args)...);
}

template <typename... Args>
inline void LogError(std::string const &format, Args &&...args)
{
    Log::GetLog().Printf(ToSafeString(format), LogLv::ERROR, std::forward<Args>(args)...);
}

template <typename... Args>
inline void LogSummary(std::string const &format, Args &&...args)
{
    Log::GetLog().Printf(format, LogLv::INFO, std::forward<Args>(args)...);
}

inline void SetLogLevelByEnvVar()
{
    Log::GetLog().SetLogLevelByEnvVar();
}
} // namespace Utility

#endif  // __CPPUTILS_MAKE_H__
