#include <gtest/gtest.h>

#include "argparser/tokens.h"

using namespace Parser;

constexpr int argc = 2;
constexpr char const *argv[2] = {"foo", "bar"};

TEST(TokenS, construct_with_tokens_then_get_expect_foo)
{
    TokenS tokens{argc, argv};
    ASSERT_EQ(tokens.Get(), "foo");
}

TEST(TokenS, tokens_consume_one_then_get_expect_bar)
{
    TokenS tokens{argc, argv};
    tokens = tokens.Consume(1);
    ASSERT_EQ(tokens.Get(), "bar");
}

TEST(TokenS, tokens_consume_more_then_two_expect_eos)
{
    TokenS tokens{argc, argv};
    ASSERT_TRUE(tokens.Consume(2).Eos());
    ASSERT_TRUE(tokens.Consume(20).Eos());
}
