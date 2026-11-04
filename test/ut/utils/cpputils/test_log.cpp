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

#include <gtest/gtest.h>
#include <gtest/internal/gtest-port.h>
#include <iostream>
#include <ctime>
#include <sstream>
#define private public
#include "log.h"
#undef private

using namespace Utility;

TEST(LogBoundary, truncation_emits_warning_and_null_output_is_ignored)
{
    auto &log = Log::GetLog();
    const auto savedLevel = log.lv_;
    FILE *savedOutput = log.fp_;
    log.lv_ = LogLv::DEBUG;
    testing::internal::CaptureStdout();
    log.Printf("source/test.cpp", 12, "%s", LogLv::INFO, std::string(MAX_PRINT * 2, 'x').c_str());
    const auto output = testing::internal::GetCapturedStdout();
    EXPECT_NE(output.find("Log length reach limit, message truncated"), std::string::npos);
    EXPECT_NE(output.find("source/test.cpp:12"), std::string::npos);
    log.fp_ = nullptr;
    EXPECT_NO_THROW(log.Printf("ignored", LogLv::ERROR));
    log.fp_ = savedOutput;
    log.lv_ = savedLevel;
}

TEST(LogBoundary, environment_level_accepts_valid_values_and_preserves_level_for_invalid_values)
{
    auto &log = Log::GetLog();
    const auto savedLevel = log.lv_;
    const char *previous = std::getenv("MSOPT_LOG_LEVEL");
    const bool existed = previous != nullptr;
    const std::string savedEnv = existed ? previous : "";
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(setenv("MSOPT_LOG_LEVEL", std::to_string(i).c_str(), 1), 0);
        log.SetLogLevelByEnvVar();
        EXPECT_EQ(log.GetLogLv(), static_cast<LogLv>(i));
    }
    EXPECT_EQ(setenv("MSOPT_LOG_LEVEL", "invalid", 1), 0);
    log.SetLogLevelByEnvVar();
    EXPECT_EQ(log.GetLogLv(), LogLv::ERROR);
    EXPECT_EQ(unsetenv("MSOPT_LOG_LEVEL"), 0);
    log.SetLogLevelByEnvVar();
    EXPECT_EQ(log.GetLogLv(), LogLv::ERROR);
    if (existed) {
        EXPECT_EQ(setenv("MSOPT_LOG_LEVEL", savedEnv.c_str(), 1), 0);
    }
    log.lv_ = savedLevel;
}

TEST(FileSystem, log_debug_with_default_log_level_warn_expect_NOT_output)
{
    testing::internal::CaptureStdout();
    std::string testLog = "test log debug";
    LogDebug(testLog);
    std::string captureLog = testing::internal::GetCapturedStdout();
    ASSERT_EQ(captureLog.find(testLog), std::string::npos);
}

TEST(FileSystem, log_warn_expect_output_warn_log)
{
    testing::internal::CaptureStdout();
    std::string testLog = "test log warn";
    LogWarn(testLog);
    std::string captureLog = testing::internal::GetCapturedStdout();
    ASSERT_NE(captureLog.find("[WARN]"), std::string::npos);
    ASSERT_NE(captureLog.find(testLog), std::string::npos);
}

TEST(FileSystem, log_error_expect_output_erro_log)
{
    testing::internal::CaptureStdout();
    std::string testLog = "test log error";
    LogError(testLog);
    std::string captureLog = testing::internal::GetCapturedStdout();
    ASSERT_NE(captureLog.find("[ERROR]"), std::string::npos);
    ASSERT_NE(captureLog.find(testLog), std::string::npos);
}

TEST(FileSystem, addPrefixInfo_with_success)
{
    std::string format = "test";
    
    // Temporarily trigger verbose mode by setting internal global log level to DEBUG
    Utility::LogLv oldLv = Log::GetLog().lv_;
    Log::GetLog().lv_ = LogLv::DEBUG;
    
    std::string time = Log::GetLog().AddPrefixInfo(__FILE__, __LINE__, format, LogLv::DEBUG);
    
    // Restore
    Log::GetLog().lv_ = oldLv;
    
    auto loc1 = time.find(" [");            // e.g. "[DEBUG] [2026-..."
    auto loc2 = time.find("] ", loc1 + 1);
    
    ASSERT_NE(loc1, std::string::npos);
    ASSERT_NE(loc2, std::string::npos);
    std::string dateStr = time.substr(loc1 + 2, loc2 - loc1 - 2);
    struct tm t;
    
    std::istringstream ss(dateStr);
    ss >> std::get_time(&t, "%Y-%m-%d %H:%M:%S");
    ASSERT_FALSE(ss.fail());
}

/**
 * |  用例集  | FileSystem
 * | 测试函数 | ToSafeString
 * |  用例名  | test_ToSafeString_with_invalid_character_expect_normal_log_print
 * | 用例描述 | 对于输入字符串中包含的非法字符，进行转义，并返回安全的字符串
 */
TEST(FileSystem, test_ToSafeString_with_invalid_character_expect_normal_log_print)
{
    std::string invalidString = "hello \v world \n\r";
    std::string expectSafeString = "hello \\v world \\n\\r";
    std::string safeString = ToSafeString(invalidString);
    EXPECT_EQ(safeString, expectSafeString);

}
