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

#include "argparser/result.h"
#include "argparser/tokens.h"

using namespace Parser;

constexpr int argc = 2;
constexpr char const *argv[2] = {"foo", "bar"};

TEST(Result, construct_either_with_left_expect_error_msg)
{
    Error error = {ErrorType::ParseEos, "some error"};
    Either left = Either::Left(error);
    ASSERT_FALSE(left.Valid());
    ASSERT_EQ(left.Left().type, ErrorType::ParseEos);
    ASSERT_EQ(left.Left().msg, "some error");
}

TEST(Result, construct_either_with_right_expect_tokens)
{
    TokenS tokens{argc, argv};
    Either right = Either::Right(tokens);
    ASSERT_TRUE(right.Valid());
    ASSERT_EQ(right.Right().Get(), "foo");
}
