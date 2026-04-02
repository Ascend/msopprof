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

#ifndef __CPPUTILS_LOG_H__
#define __CPPUTILS_LOG_H__

#include <string>
#include <unordered_map>
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

class Log {
public:
    static Log &GetLog(void);

    template<typename... Args>
    inline void Printf(const char* file, int line, std::string const &format, LogLv lv, Args &&... args) const;

    template<typename... Args>
    inline void Printf(std::string const &format, LogLv lv, Args &&... args) const {
        Printf(nullptr, 0, format, lv, std::forward<Args>(args)...);
    }

    void SetLogLevelByEnvVar();
    LogLv GetLogLv() const { return lv_; }

private:
    Log(void) = default;
    ~Log(void) = default;
    Log(Log const &) = delete;
    Log &operator=(Log const &) = delete;
    std::string AddPrefixInfo(const char* file, int line, std::string const &format, LogLv lv) const;

private:
    LogLv lv_ { LogLv::INFO };
    FILE *fp_ { stdout };
};

template <typename... Args>
void Log::Printf(const char* file, int line, const std::string &format, LogLv lv, Args &&...args) const
{
    if (fp_ == nullptr) {
        return;
    }
    if (lv < lv_) {
        return;
    }
    std::string f = AddPrefixInfo(file, line, format, lv).append("\n");

    char msg[MAX_PRINT] = {0};
    auto res = snprintf_s(msg, MAX_PRINT, MAX_PRINT - 1, f.c_str(), std::forward<Args>(args)...);
    if (res >= 0) {
        fprintf(fp_, "%s", msg);
        return;
    }
    std::string lengthLimit = AddPrefixInfo(file, line,
        "Log length reach limit, message truncated", lv).append("\n");
    fprintf(fp_, "%s", lengthLimit.c_str());
}

template <typename... Args>
inline void LogDebugInternal(const char* file, int line, std::string const &format, Args &&...args)
{
    Log::GetLog().Printf(file, line, format, LogLv::DEBUG, std::forward<Args>(args)...);
}

template <typename... Args>
inline void LogInfoInternal(const char* file, int line, std::string const &format, Args &&...args)
{
    Log::GetLog().Printf(file, line, format, LogLv::INFO, std::forward<Args>(args)...);
}

template <typename... Args>
inline void LogWarnInternal(const char* file, int line, std::string const &format, Args &&...args)
{
    Log::GetLog().Printf(file, line, format, LogLv::WARN, std::forward<Args>(args)...);
}

template <typename... Args>
inline void LogErrorInternal(const char* file, int line, std::string const &format, Args &&...args)
{
    Log::GetLog().Printf(file, line, format, LogLv::ERROR, std::forward<Args>(args)...);
}

template <typename... Args>
inline void LogSummaryInternal(const char* file, int line, std::string const &format, Args &&...args)
{
    Log::GetLog().Printf(file, line, format, LogLv::INFO, std::forward<Args>(args)...);
}

inline void SetLogLevelByEnvVar()
{
    Log::GetLog().SetLogLevelByEnvVar();
}
} // namespace Utility

#define LogDebug(format, ...) \
    LogDebugInternal(__FILE__, __LINE__, Utility::ToSafeString(format), ##__VA_ARGS__)

#define LogInfo(format, ...) \
    LogInfoInternal(__FILE__, __LINE__, Utility::ToSafeString(format), ##__VA_ARGS__)

#define LogWarn(format, ...) \
    LogWarnInternal(__FILE__, __LINE__, Utility::ToSafeString(format), ##__VA_ARGS__)

#define LogError(format, ...) \
    LogErrorInternal(__FILE__, __LINE__, Utility::ToSafeString(format), ##__VA_ARGS__)

#define LogSummary(format, ...) \
    LogSummaryInternal(__FILE__, __LINE__, format, ##__VA_ARGS__)

#endif  // __CPPUTILS_LOG_H__
