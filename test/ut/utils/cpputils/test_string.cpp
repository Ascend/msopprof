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
#include <iterator>

#include "ustring.h"

using namespace Utility;

TEST(String, join_empty_list_of_string_expect_empty_string)
{
    std::vector<std::string> emptyList;
    std::string result = Join(emptyList.cbegin(), emptyList.cend());
    ASSERT_TRUE(result.empty());
}

TEST(String, join_list_of_on_element_expect_equal_to_element)
{
    std::vector<std::string> oneElem = {
        "elem"
    };
    std::string result = Join(oneElem.cbegin(), oneElem.cend());
    ASSERT_EQ(result, oneElem.front());
}

TEST(String, join_list_of_string_expect_correct_result)
{
    std::vector<std::string> strs = {
        "abc",
        "def",
        "ghi"
    };
    std::string result = Join(strs.cbegin(), strs.cend(), ":");
    ASSERT_EQ(result, "abc:def:ghi");
}

TEST(String, split_empty_string_expect_empty_list_of_string)
{
    std::string str;
    std::vector<std::string> strs;
    Split(str, std::back_inserter(strs), " ");
    ASSERT_TRUE(strs.empty());
}

TEST(String, split_string_with_empty_seps_expect_return_whole_string)
{
    std::string str = "abc";
    std::vector<std::string> strs;
    Split(str, std::back_inserter(strs), "");
    ASSERT_EQ(strs.size(), 1);
    ASSERT_EQ(strs[0], str);
}

TEST(String, split_string_without_specific_seps_expect_return_whole_string)
{
    std::string str = "abc";
    std::vector<std::string> strs;
    Split(str, std::back_inserter(strs), ":");
    ASSERT_EQ(strs.size(), 1);
    ASSERT_EQ(strs[0], str);
}

TEST(String, split_string_with_spaces_expect_return_correct_strs)
{
    std::string str = "abc def   ghi";
    std::vector<std::string> strs;
    Split(str, std::back_inserter(strs), " ");
    ASSERT_EQ(strs.size(), 3);
    ASSERT_EQ(strs[0], "abc");
    ASSERT_EQ(strs[1], "def");
    ASSERT_EQ(strs[2], "ghi");
}

TEST(String, string_to_num_error)
{
    std::string str = "";
    uint32_t a;
    ASSERT_FALSE(StringToNum(str, a));
    str = "11a";
    ASSERT_FALSE(StringToNum(str, a));
}

TEST(String, replace_subStr_to_num_error)
{
    std::string str = "111";
    std::string subString = "1";
    std::string newString = "2";
    ASSERT_STREQ(ReplaceSubStr(str, subString, newString).c_str(), "222");
    ASSERT_STREQ(ReplaceSubStr(str, str, str).c_str(), "111");
    ASSERT_STREQ(ReplaceSubStr(str, subString, newString, 2).c_str(), "221");
}

/**
 * |  用例集  | UstringUt
 * | 测试函数 | IsStringCharValid
 * |  用例名  | test_IsStringCharValid_with_carriage_return_expect_false
 * | 用例描述 | 对于输入字符串中包含的非法字符，进行检测。若存在会被转义的字符，则返回false并将错误字符提示信息放到msg中
 */
TEST(UstringUt, test_IsStringCharValid_with_carriage_return_expect_false)
{
    std::string msg;
    std::string inputString = "test carriage_return\r";
    bool res = IsStringCharValid(inputString, msg);
    ASSERT_FALSE(res);
    ASSERT_EQ(msg, "invalid character: \\r");
}


/**
 * |  用例集  | UstringUt
 * | 测试函数 | GetUint64ListFromStr
 * |  用例名  | test_GetUint64ListFromStr_with_string_expect_return_correct_value_list
 * | 用例描述 | 测试根据输入的regex表达式，获取string中匹配的正确的uint64_t list
 */
TEST(UstringUt, test_GetUint64ListFromStr_with_string_expect_return_correct_value_list)
{
    std::string detail = "XN:X0=0x10, Pos:1, Id:545";
    std::regex pattern = std::regex("XN:X[0-9]{1,2}=(?:0x)?([0-9a-f]+), Pos:([0-9]{1})");
    std::vector<uint64_t> resList;
    GetUint64ListFromStr(pattern, detail, resList);
    ASSERT_EQ(resList.size(), 2);
    ASSERT_EQ(resList[0], 0x10);
    ASSERT_EQ(resList[1], 1);

    detail = "SPR:CTRL, XN:X1=0x100000000000000,";
    pattern = std::regex("SPR:CTRL, XN:X[0-9]{1,2}=(?:0x)?([0-9a-f]+)");
    resList = {};
    GetUint64ListFromStr(pattern, detail, resList);
    ASSERT_EQ(resList.size(), 1);
    ASSERT_EQ(resList[0], 0x100000000000000ULL);
}

/**
 * |  用例集  | UstringUt
 * | 测试函数 | StringMatch
 * |  用例名  | test_StringMatch_with_string_expect_return_correct_value
 * | 用例描述 | 测试根据输入的str，按照substr规则是否能匹配，匹配规则包含1.前缀完全匹配2.*匹配任意字符
 */
TEST(UstringUt, test_StringMatch_with_string_expect_return_correct_value)
{
    ASSERT_TRUE(StringMatch("Abs_00000", "Abs"));
    ASSERT_TRUE(StringMatch("Abs_00000", "A*"));
    ASSERT_TRUE(StringMatch("Abs_00000", "*"));
    ASSERT_TRUE(StringMatch("Abs_00000", "*000"));
    ASSERT_TRUE(StringMatch("Abs_00000", "*s_0*"));
    ASSERT_TRUE(StringMatch("Abs_00000", "A*_0*"));
    ASSERT_TRUE(StringMatch("Abs_00000", "A**"));

    ASSERT_FALSE(StringMatch("Abs_00000", "bs"));
    ASSERT_FALSE(StringMatch("Abs_00000", "a*"));
    ASSERT_FALSE(StringMatch("Abs_00000", "Abs_0*c"));
    ASSERT_FALSE(StringMatch("Abs_00000", "*s"));
    ASSERT_FALSE(StringMatch("Abs_00000", "s*"));
    ASSERT_FALSE(StringMatch("Abs_00000", "*x*"));
    ASSERT_FALSE(StringMatch("Abs_00000", "A**d"));
}

/**
 * |  用例集  | StoiConverter
 * | 测试函数 | StoiConverter
 * |  用例名  | test_StoiConverter_expect_return_true
 * | 用例描述 | 测试根据输入的str，能正确地转化成对应进制的整数
 */
TEST(StoiConverter, test_StoiConverter_expect_return_true)
{
    int32_t num;
    ASSERT_TRUE(StoiConverter("123", num, RADIX_10));
    ASSERT_EQ(num, 123);
    ASSERT_TRUE(StoiConverter("-123", num, RADIX_10));
    ASSERT_EQ(num, -123);
    ASSERT_TRUE(StoiConverter("1A", num, RADIX_16));
    ASSERT_EQ(num, 26);
}

/**
 * |  用例集  | StoiConverter
 * | 测试函数 | StoiConverter
 * |  用例名  | test_StoiConverter_expect_return_false
 * | 用例描述 | 测试根据输入的str，无法正确地转化成对应进制的整数
 */
TEST(StoiConverter, test_StoiConverter_expect_return_false)
{
    int32_t num;
    // 超出范围
    ASSERT_FALSE(StoiConverter("2147483648", num, RADIX_10));
    // 空字符串
    ASSERT_FALSE(StoiConverter("", num, RADIX_10));
    // 非法字符
    ASSERT_FALSE(StoiConverter("G1", num, RADIX_16));
}

/**
 * |  用例集  | StollConverter
 * | 测试函数 | StollConverter
 * |  用例名  | test_StollConverter_expect_return_true
 * | 用例描述 | 测试根据输入的str，能正确地转化成对应进制的整数
 */
TEST(StollConverter, test_StollConverter_expect_return_true)
{
    int64_t num;
    ASSERT_TRUE(StollConverter("2147483648", num, RADIX_10));
    ASSERT_EQ(num, 2147483648);
    ASSERT_TRUE(StollConverter("-2147483648", num, RADIX_10));
    ASSERT_EQ(num, -2147483648);
    ASSERT_TRUE(StollConverter("1A", num, RADIX_16));
    ASSERT_EQ(num, 26);
}

/**
 * |  用例集  | StollConverter
 * | 测试函数 | StollConverter
 * |  用例名  | test_StollConverter_expect_return_false
 * | 用例描述 | 测试根据输入的str，无法正确地转化成对应进制的整数
 */
TEST(StollConverter, test_StollConverter_expect_return_false)
{
    int64_t num;
    // 超出范围
    ASSERT_FALSE(StollConverter("9223372036854775808", num, RADIX_10));
    // 空字符串
    ASSERT_FALSE(StollConverter("", num, RADIX_10));
    // 非法字符
    ASSERT_FALSE(StollConverter("G1", num, RADIX_16));
}

/**
 * |  用例集  | StoullConverter
 * | 测试函数 | StoullConverter
 * |  用例名  | test_StollConverter_expect_return_true
 * | 用例描述 | 测试根据输入的str，能正确地转化成对应进制的整数
 */
TEST(StoullConverter, test_StollConverter_expect_return_true)
{
    uint64_t num;
    ASSERT_TRUE(StoullConverter("2147483648", num, RADIX_10));
    ASSERT_EQ(num, 2147483648);
    ASSERT_TRUE(StoullConverter("1A", num, RADIX_16));
    ASSERT_EQ(num, 26);
}

/**
 * |  用例集  | StoullConverter
 * | 测试函数 | StoullConverter
 * |  用例名  | test_StollConverter_expect_return_false
 * | 用例描述 | 测试根据输入的str，无法正确地转化成对应进制的整数
 */
TEST(StoullConverter, test_StollConverter_expect_return_false)
{
    uint64_t num;
    // 超出范围
    ASSERT_FALSE(StoullConverter("18446744073709551616", num, RADIX_10));
    // 空字符串
    ASSERT_FALSE(StoullConverter("", num, RADIX_10));
    // 非法字符
    ASSERT_FALSE(StoullConverter("G1", num, RADIX_16));
}