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
