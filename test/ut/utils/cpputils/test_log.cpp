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
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <thread>
#include <vector>
#define private public
#include "log.h"
#undef private

using namespace Utility;

class LogTest : public testing::Test {
protected:
    void SetUp() override {
        auto &log = Log::GetLog();
        previousLevel_ = log.lv_;
        previousOutput_ = log.fp_;
        output_ = std::tmpfile();
        ASSERT_NE(output_, nullptr);
        log.lv_ = LogLv::INFO;
        log.fp_ = output_;
        Log::EndCapture();
    }

    void TearDown() override {
        Log::EndCapture();
        Log::GetLog().lv_ = previousLevel_;
        Log::GetLog().fp_ = previousOutput_;
        if (output_ != nullptr) {
            std::fclose(output_);
        }
    }

    std::string ReadOutput() {
        std::fflush(output_);
        std::rewind(output_);
        std::string content;
        char buffer[512];
        while (std::fgets(buffer, sizeof(buffer), output_) != nullptr) {
            content += buffer;
        }
        return content;
    }

    FILE *output_ = nullptr;
    FILE *previousOutput_ = nullptr;
    LogLv previousLevel_ = LogLv::INFO;
};

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

/**
 * | 测试场景 | 日志等级为INFO，调用LogDebug输出一条消息。
 * | 预期结果 | 输出文件中不包含该DEBUG消息。
 */
TEST_F(LogTest, debug_message_at_info_level_expect_no_output) {
    std::string testLog = "test log debug";
    LogDebug(testLog);
    std::string captureLog = ReadOutput();
    ASSERT_EQ(captureLog.find(testLog), std::string::npos);
}

/**
 * | 测试场景 | 日志等级为INFO，调用LogWarn输出一条消息。
 * | 预期结果 | 输出同时包含[WARN]前缀和原消息。
 */
TEST_F(LogTest, warn_message_at_info_level_expect_warn_prefix_and_message) {
    std::string testLog = "test log warn";
    LogWarn(testLog);
    std::string captureLog = ReadOutput();
    ASSERT_NE(captureLog.find("[WARN]"), std::string::npos);
    ASSERT_NE(captureLog.find(testLog), std::string::npos);
}

/**
 * | 测试场景 | 日志等级为INFO，调用LogError输出一条消息。
 * | 预期结果 | 输出同时包含[ERROR]前缀和原消息。
 */
TEST_F(LogTest, error_message_at_info_level_expect_error_prefix_and_message) {
    std::string testLog = "test log error";
    LogError(testLog);
    std::string captureLog = ReadOutput();
    ASSERT_NE(captureLog.find("[ERROR]"), std::string::npos);
    ASSERT_NE(captureLog.find(testLog), std::string::npos);
}

/**
 * | 测试函数 | Log::AddPrefixInfo
 * | 测试场景 | 日志等级为DEBUG，生成一条DEBUG日志的前缀。
 * | 预期结果 | 前缀中的时间可按YYYY-MM-DD HH:MM:SS格式解析。
 */
TEST_F(LogTest, debug_prefix_expect_parseable_timestamp) {
    std::string format = "test";

    // TearDown restores the previous log level after this test.
    Log::GetLog().lv_ = LogLv::DEBUG;
    
    std::string time = Log::GetLog().AddPrefixInfo(__FILE__, __LINE__, format, LogLv::DEBUG);

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
 * |  用例集  | LogTest
 * | 测试函数 | ToSafeString
 * | 测试场景 | 输入字符串含垂直制表符、换行符和回车符。
 * | 预期结果 | 这些控制字符被转换为可见的\v、\n、\r文本，其他字符保持不变。
 */
TEST_F(LogTest, escape_control_characters_expect_visible_escape_sequences) {
    std::string invalidString = "hello \v world \n\r";
    std::string expectSafeString = "hello \\v world \\n\\r";
    std::string safeString = ToSafeString(invalidString);
    EXPECT_EQ(safeString, expectSafeString);
}

/**
 * |  用例集  | LogTest
 * | 测试函数 | LogCaptureGuard、Log::FlushCapture
 * | 测试场景 | 捕获一条消息参数含字面量%s的日志，退出捕获后先直接写日志，再回放捕获内容。
 * | 预期结果 | 捕获期间文件为空；退出后恢复直接输出；回放保留字面量%s，不再次作为格式指令解析。
 */
TEST_F(LogTest, capture_then_replay_expect_literal_percent_text_and_restored_direct_output) {
    std::string captured;
    {
        LogCaptureGuard guard(&captured);
        LogInfo("operator=%s", "add %s");
        EXPECT_EQ(std::ftell(output_), 0);
    }
    LogInfo("direct output");
    Log::FlushCapture(captured);
    EXPECT_EQ(captured, "[INFO]  operator=add %s\n");
    EXPECT_EQ(ReadOutput(), "[INFO]  direct output\n[INFO]  operator=add %s\n");
}

/**
 * |  用例集  | LogTest
 * | 测试函数 | LogCaptureGuard、Log::Printf
 * | 测试场景 | 8个线程分别向独立缓冲写入编号0至31的日志，线程结束后主线程直接写日志。
 * | 预期结果 | 每个缓冲恰好包含本线程的32条有序日志；文件只包含主线程直接输出的消息。
 */
TEST_F(LogTest, eight_thread_capture_expect_isolated_ordered_logs_and_no_file_output) {
    constexpr size_t workers = 8;
    constexpr size_t messages = 32;
    std::vector<std::string> captured(workers);
    std::vector<std::thread> threads;
    for (size_t worker = 0; worker < workers; ++worker) {
        threads.emplace_back([worker, &captured] {
            LogCaptureGuard guard(&captured[worker]);
            for (size_t message = 0; message < messages; ++message) {
                LogInfo("kernel-%zu message-%zu", worker, message);
            }
        });
    }
    for (auto &thread : threads) {
        thread.join();
    }
    EXPECT_EQ(std::ftell(output_), 0);
    for (size_t worker = 0; worker < workers; ++worker) {
        std::string expected;
        for (size_t message = 0; message < messages; ++message) {
            expected += "[INFO]  kernel-" + std::to_string(worker) + " message-" + std::to_string(message) + "\n";
        }
        EXPECT_EQ(captured[worker], expected);
    }
    LogInfo("main thread output");
    EXPECT_EQ(ReadOutput(), "[INFO]  main thread output\n");
}

/**
 * |  用例集  | LogTest
 * | 测试函数 | Log::Printf、Log::FlushCapture
 * | 测试场景 | WARN等级下捕获一条INFO和一条长度为MAX_PRINT两倍的WARN，然后回放。
 * | 预期结果 | INFO被过滤，WARN被截断且带截断提示；回放前文件为空，回放后与缓冲内容完全一致。
 */
TEST_F(LogTest, warn_level_capture_expect_info_filtered_and_long_warning_truncated_on_replay) {
    Log::GetLog().lv_ = LogLv::WARN;
    std::string captured;
    const std::string longMessage(MAX_PRINT * 2, 'x');
    {
        LogCaptureGuard guard(&captured);
        LogInfo("filtered message");
        LogWarn("%s", longMessage.c_str());
    }
    EXPECT_EQ(captured.find("filtered message"), std::string::npos);
    EXPECT_EQ(captured.find("[WARN]"), 0);
    EXPECT_NE(captured.find("Log length reach limit, message truncated\n"), std::string::npos);
    EXPECT_LT(captured.size(), longMessage.size());
    EXPECT_EQ(std::ftell(output_), 0);
    Log::FlushCapture(captured);
    EXPECT_EQ(ReadOutput(), captured);
}

/**
 * |  用例集  | LogTest
 * | 测试函数 | Log::FlushCapture
 * | 测试场景 | 分别回放空字符串，以及在输出流为nullptr时回放非空字符串。
 * | 预期结果 | 回放空字符串不写文件；输出流为空时安全返回，不崩溃。
 */
TEST_F(LogTest, replay_empty_text_or_null_output_expect_no_write_or_crash) {
    Log::FlushCapture("");
    EXPECT_EQ(std::ftell(output_), 0);

    Log::GetLog().fp_ = nullptr;
    Log::FlushCapture("ignored");
    Log::GetLog().fp_ = output_;
}

/**
 * |  用例集  | LogTest
 * | 测试函数 | Log::FlushCapture
 * | 测试场景 | 父缓冲正在捕获日志时，调用FlushCapture回放子缓冲的inner消息。
 * | 预期结果 | 父缓冲恰好收到完整inner消息，输出文件保持为空。
 */
TEST_F(LogTest, replay_during_parent_capture_expect_child_text_in_parent_and_no_file_output) {
    std::string parent;
    std::string child = "[INFO]  inner\n";
    {
        LogCaptureGuard guard(&parent);
        Log::FlushCapture(child);
    }
    EXPECT_EQ(parent, "[INFO]  inner\n");
    EXPECT_EQ(std::ftell(output_), 0);
}
