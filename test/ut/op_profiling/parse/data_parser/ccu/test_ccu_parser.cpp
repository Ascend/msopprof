/* -------------------------------------------------------------------------
 *  This file is part of the MindStudio project.
 *  Copyright (c) 2026 Huawei Technologies Co.,Ltd.
 *
 *  MindStudio is licensed under Mulan PSL v2.
 *  You can use this software according to the terms and conditions of the Mulan PSL v2.
 *  You may obtain a copy of Mulan PSL v2 at:
 *
 *           http://license.coscl.org.cn/MulanPSL2
 *
 *  THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 *  EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 *  MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *  See the Mulan PSL v2 for more details.
 *  ------------------------------------------------------------------------- */

#include <gtest/gtest.h>
#include "mockcpp/mockcpp.hpp"
#include <fstream>

#define private public
#define protected public
#include "parse/data_parser/ccu_parser/ccu_log_parser.h"
#undef private
#undef protected

using namespace Profiling::Parse;
using namespace Utility;
using namespace Profiling;

class TestCcuParser : public CcuParser {
public:
    TestCcuParser(DataCenter &dataCenter, SimDataParserConfig &config) : CcuParser(dataCenter, config) {}

    void TestParseLine(const std::string &line) { ParseLine(line); }

    scalarHead &GetCcuInstrMap() { return ccuInstrMap_; }
};

SimDataParserConfig GetTestConfig(const ChipProductType &type) {
    std::string dumpPath = "test/ut/resources/dump/910B";
    CoreNameAndPreFixPair coreNamePair{"core0.veccore0", "core0.veccore0."};
    std::set<int> parseIds = {0};
    SimDataParserConfig config{dumpPath, coreNamePair, parseIds, false, true, type};
    return config;
}

/**
 * |  用例集 | CcuParserTest
 * | 测试函数 | ParseLine
 * |  用例名  | test_parse_line_when_valid_format_and_expect_parse_success
 * | 用例描述 | 测试CcuParser的ParseLine函数，当日志格式有效时，应成功解析tick和pc并存入ccuInstrMap_
 */
TEST(CcuParserTest, test_parse_line_when_valid_format_and_expect_parse_success) {
    DataCenter dataCenter;
    SimDataParserConfig config = GetTestConfig(ChipProductType::ASCEND910B1);
    TestCcuParser parser(dataCenter, config);

    std::string line1 = "[info] 100: is_single_issue instr, pc:0x1000";
    parser.TestParseLine(line1);

    std::string line2 = "[info] 200: ccu do issue success.issue isa pc :0x2000";
    parser.TestParseLine(line2);

    auto &ccuMap = parser.GetCcuInstrMap();
    ASSERT_TRUE(ccuMap.size() == 2);
    ASSERT_TRUE(ccuMap.find(0x1000) != ccuMap.end());
    ASSERT_TRUE(ccuMap.find(0x2000) != ccuMap.end());

    ASSERT_TRUE(ccuMap[0x1000].size() == 1);
    ASSERT_TRUE(ccuMap[0x1000][0].ccuTick == 100);
    ASSERT_TRUE(ccuMap[0x1000][0].pc == 0x1000);
    ASSERT_TRUE(ccuMap[0x1000][0].icacheTick == UINT64_MAX);

    ASSERT_TRUE(ccuMap[0x2000].size() == 1);
    ASSERT_TRUE(ccuMap[0x2000][0].ccuTick == 200);
    ASSERT_TRUE(ccuMap[0x2000][0].pc == 0x2000);
    ASSERT_TRUE(ccuMap[0x2000][0].icacheTick == UINT64_MAX);
}

/**
 * |  用例集 | CcuParserTest
 * | 测试函数 | ParseLine
 * |  用例名  | test_parse_line_when_invalid_format_and_expect_skip
 * | 用例描述 | 测试CcuParser的ParseLine函数错误处理，当日志格式无效时，应跳过不解析
 */
TEST(CcuParserTest, test_parse_line_when_invalid_format_and_expect_skip) {
    DataCenter dataCenter;
    SimDataParserConfig config = GetTestConfig(ChipProductType::ASCEND910B1);
    TestCcuParser parser(dataCenter, config);

    std::string invalidLine1 = "[error] 100: invalid line";
    parser.TestParseLine(invalidLine1);

    std::string invalidLine2 = "[info] abc: is_single_issue instr, pc:0x1000";
    parser.TestParseLine(invalidLine2);

    std::string invalidLine3 = "[info] 100: some other message";
    parser.TestParseLine(invalidLine3);

    auto &ccuMap = parser.GetCcuInstrMap();
    ASSERT_TRUE(ccuMap.empty());
}

/**
 * |  用例集 | CcuParserTest
 * | 测试函数 | ParseLine
 * |  用例名  | test_parse_line_when_same_pc_multiple_times_and_expect_vector_append
 * | 用例描述 | 测试CcuParser的ParseLine函数，当同一PC多次出现时，应追加到向量中并按顺序存储tick值
 */
TEST(CcuParserTest, test_parse_line_when_same_pc_multiple_times_and_expect_vector_append) {
    DataCenter dataCenter;
    SimDataParserConfig config = GetTestConfig(ChipProductType::ASCEND910B1);
    TestCcuParser parser(dataCenter, config);

    std::string line1 = "[info] 100: is_single_issue instr, pc:0x1000";
    std::string line2 = "[info] 200: ccu do issue success.issue isa pc :0x1000";
    std::string line3 = "[info] 300: is_single_issue instr, pc:0x1000";

    parser.TestParseLine(line1);
    parser.TestParseLine(line2);
    parser.TestParseLine(line3);

    auto &ccuMap = parser.GetCcuInstrMap();
    ASSERT_TRUE(ccuMap.size() == 1);
    ASSERT_TRUE(ccuMap[0x1000].size() == 3);

    ASSERT_TRUE(ccuMap[0x1000][0].ccuTick == 100);
    ASSERT_TRUE(ccuMap[0x1000][1].ccuTick == 200);
    ASSERT_TRUE(ccuMap[0x1000][2].ccuTick == 300);
}

/**
 * |  用例集 | CcuParserTest
 * | 测试函数 | ParseLine
 * |  用例名  | test_parse_line_when_empty_line_and_expect_skip
 * | 用例描述 | 测试CcuParser的ParseLine函数边界情况，当输入为空行时，应跳过不处理
 */
TEST(CcuParserTest, test_parse_line_when_empty_line_and_expect_skip) {
    DataCenter dataCenter;
    SimDataParserConfig config = GetTestConfig(ChipProductType::ASCEND910B1);
    TestCcuParser parser(dataCenter, config);

    std::string emptyLine = "";
    parser.TestParseLine(emptyLine);

    auto &ccuMap = parser.GetCcuInstrMap();
    ASSERT_TRUE(ccuMap.empty());
}

/**
 * |  用例集 | CcuParserTest
 * | 测试函数 | Entry
 * |  用例名  | test_entry_when_file_not_exist_and_expect_nonblocking_error
 * | 用例描述 | 测试CcuParser的Entry函数错误处理，当ccu_log文件不存在时，应返回NONBLOCKING_ERROR
 */
TEST(CcuParserTest, test_entry_when_file_not_exist_and_expect_nonblocking_error) {
    DataCenter dataCenter;
    SimDataParserConfig config = GetTestConfig(ChipProductType::ASCEND910B1);
    TestCcuParser parser(dataCenter, config);

    PluginErrorCode result = parser.Entry();
    ASSERT_TRUE(result == PluginErrorCode::NONBLOCKING_ERROR);
}

/**
 * |  用例集 | CcuParserTest
 * | 测试函数 | regex_search
 * |  用例名  | test_regex_pattern_match_and_expect_correct_match
 * | 用例描述 | 测试CcuParser的正则表达式匹配模式，验证能正确匹配CCU日志格式并提取tick和pc值
 */
TEST(CcuParserTest, test_regex_pattern_match_and_expect_correct_match) {
    std::regex pattern =
        std::regex("\\[info\\] ([0-9]+): (is_single_issue instr, pc|ccu do issue success.issue isa pc ):(0x[0-9a-f]+)");

    std::string validLine1 = "[info] 100: is_single_issue instr, pc:0x1000";
    std::smatch match1;
    ASSERT_TRUE(std::regex_search(validLine1, match1, pattern));
    ASSERT_TRUE(match1[1].str() == "100");
    ASSERT_TRUE(match1[3].str() == "0x1000");

    std::string validLine2 = "[info] 200: ccu do issue success.issue isa pc :0x2abc";
    std::smatch match2;
    ASSERT_TRUE(std::regex_search(validLine2, match2, pattern));
    ASSERT_TRUE(match2[1].str() == "200");
    ASSERT_TRUE(match2[3].str() == "0x2abc");

    std::string invalidLine = "[warn] 100: is_single_issue instr, pc:0x1000";
    std::smatch match3;
    ASSERT_TRUE(!std::regex_search(invalidLine, match3, pattern));
}
