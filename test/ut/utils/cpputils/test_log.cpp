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
    std::string time = Log::GetLog().AddPrefixInfo(format, LogLv::WARN);
    auto loc = time.find(" ");
    ASSERT_NE(loc, std::string::npos);
    time = time.substr(0, loc);
    struct tm t;
    std::istringstream ss(time);
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