/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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
 * -------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include "argparser/options.h"
#include "argparser/parser.h"
#include "argparser/result.h"
#include "argparser/tokens.h"

using namespace Parser;

TEST(Parser, switch_parse_no_argument_expect_eos_error)
{
    constexpr int argc = 0;
    bool val = false;
    Switch switch_('h', "", val);
    Either result = switch_.Parse(TokenS{argc, nullptr});
    ASSERT_FALSE(result.Valid());
    ASSERT_EQ(result.Left().type, ErrorType::ParseEos);
}

TEST(Parser, switch_parse_mismatch_argument_expect_mismatch_error)
{
    constexpr int argc = 1;
    constexpr char const* argv[1] = {"--unknown"};
    bool val = false;
    Switch switch_('h', "", val);
    Either result = switch_.Parse(TokenS{argc, argv});
    ASSERT_FALSE(result.Valid());
    ASSERT_EQ(result.Left().type, ErrorType::ParseMissMatch);
}

TEST(Parser, switch_parse_nonoption_argument_expect_nonoption_error)
{
    constexpr int argc = 1;
    constexpr char const* argv[1] = {"unknown"};
    bool val = false;
    Switch switch_('h', "", val);
    Either result = switch_.Parse(TokenS{argc, argv});
    ASSERT_FALSE(result.Valid());
    ASSERT_EQ(result.Left().type, ErrorType::ParseNonOption);
}

TEST(Parser, option_parse_no_argument_expect_eos_error)
{
    constexpr int argc = 0;
    std::string val;
    Option<std::string> option('c', "count", "STRING", val);
    Either result = option.Parse(TokenS{argc, nullptr});
    ASSERT_FALSE(result.Valid());
    ASSERT_EQ(result.Left().type, ErrorType::ParseEos);
}

TEST(Parser, option_parse_mismatch_argument_expect_mismatch_error)
{
    constexpr int argc = 1;
    constexpr char const* argv[1] = {"--unknown"};
    std::string val;
    Option<std::string> option('c', "count", "STRING", val);
    Either result = option.Parse(TokenS{argc, argv});
    ASSERT_FALSE(result.Valid());
    ASSERT_EQ(result.Left().type, ErrorType::ParseMissMatch);
}

TEST(Parser, option_parse_nonoption_argument_expect_nonoption_error)
{
    constexpr int argc = 1;
    constexpr char const* argv[1] = {"unknown"};
    std::string val;
    Option<std::string> option('c', "count", "STRING", val);
    Either result = option.Parse(TokenS{argc, argv});
    ASSERT_FALSE(result.Valid());
    ASSERT_EQ(result.Left().type, ErrorType::ParseNonOption);
}

TEST(Parser, arg_parser_match_argument_expect_right_and_consume_tokens)
{
    constexpr int argc = 1;
    constexpr char const* argv[1] = {"--help"};
    bool val = false;
    Common::ProfArgs args;
    Switch switch_('h', "help", val);
    ArgParser parser("command", "description");
    parser.Add(switch_);
    Either result = parser.Parse(TokenS{argc, argv}, args);
    ASSERT_TRUE(result.Valid());
    ASSERT_TRUE(result.Right().Eos());
}

TEST(Parser, arg_parser_parse_no_argument_expect_do_nothing)
{
    constexpr int argc = 0;
    bool val = false;
    Switch switch_('h', "help", val);
    ArgParser parser("command", "description");
    parser.Add(switch_);
    Common::ProfArgs args;
    Either result = parser.Parse(TokenS{argc, nullptr}, args);
    ASSERT_TRUE(result.Valid());
    ASSERT_FALSE(val);
}

TEST(Parser, arg_parser_parse_unknown_argument_expect_mismatch_error)
{
    constexpr int argc = 1;
    constexpr char const* argv[1] = {"--unknown"};
    bool val = false;
    Switch switch_('h', "help", val);
    ArgParser parser("command", "description");
    parser.Add(switch_);
    Common::ProfArgs args;
    Either result = parser.Parse(TokenS{argc, argv}, args);
    ASSERT_FALSE(result.Valid());
    ASSERT_EQ(result.Left().type, ErrorType::ParseMissMatch);
}

TEST(Parser, arg_parser_parse_nonoptions_expect_capture_nonoptions)
{
    constexpr int argc = 4;
    constexpr char const* argv[4] = {"--help", "--count=1", "./test", "--unknown"};
    bool val = false;
    std::string count;
    Switch switch_('h', "help", val);
    Option<std::string> option('c', "count", "STRING", count);
    ArgParser parser("command", "description");
    parser.Add(switch_);
    parser.Add(option);
    Common::ProfArgs args;
    Either result = parser.Parse(TokenS{argc, argv}, args);
    ASSERT_TRUE(result.Valid());
    ASSERT_TRUE(result.Right().Eos());
    ASSERT_TRUE(val);
    ASSERT_EQ(count, "1");
}
