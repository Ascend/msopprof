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
#include "mockcpp/mockcpp.hpp"

#define private public
#define protected public
#include "parse/data_parser/parser_utils/parse_pc_code.h"
#undef private
#undef protected
#include "filesystem.h"
#include "cmd_execute.h"

using namespace Profiling::Parse;
using namespace Profiling;
using namespace Utility;

/**
 * |  用例集  | ParsePcCode
 * | 测试函数 | ParsePcCode
 * |  用例名  | test_ParsePcCode_GetStartPc_should_return_true_when_pc_start_txt_exist
 */
TEST(ParsePcCode, test_ParsePcCode_GetStartPc_should_return_true_when_pc_start_txt_exist) {
    std::set<uint64_t> pcSet = {0x800000, 0x800004, 0x800008};
    const std::string dumpPath = "test/ut/resources/dump/output/";
    Utility::MkdirRecusively(dumpPath);
    ParsePcCode test(dumpPath, pcSet);
    std::string fileName = JoinPath({dumpPath, "pc_start_addr.txt"});
    std::ofstream writeFile(fileName, std::fstream::out | std::fstream::trunc);
    writeFile << "0x800000" << std::endl;
    writeFile.close();
    uint64_t startPc = test.GetStartPc();
    ASSERT_TRUE(startPc == 0x800000);
    std::experimental::filesystem::remove_all(dumpPath);
}

/**
 * |  用例集  | ParsePcCode
 * | 测试函数 | GenPc2Code
 * |  用例名  | test_ParsePcCode_GenPc2Code_should_return_true
 */
TEST(ParsePcCode, test_GenPc2Code_should_return_true_when_pc_start_txt_exist) {
    std::set<uint64_t> pcSet = {0x800000, 0x800004, 0x800008};
    const std::string dumpPath = "test/ut/resources/dump/output/";
    ParsePcCode test(dumpPath, pcSet);
    uint64_t startPc = 0x800000;
    Symbol symbol {"18", 18, "func", 18, 18, "start", 18, "0x4"};
    Offset2Line line {"4", "a.cpp", 18, "func", {symbol}};
    std::vector<Offset2Line> lines = {line};
    ASSERT_TRUE(test.GenPc2Code(lines, startPc));
}

/**
 * |  用例集  | ParsePcCode
 * | 测试函数 | GetPcSetByKernelName
 * |  用例名  | test_GetPcSetByKernelName_should_return_false_when_aicore_not_exist
 */
TEST(ParsePcCode, test_GetPcSetByKernelName_should_return_false_when_aicore_not_exist) {
    std::set<uint64_t> pcSet = {0x800000, 0x800004, 0x800008};
    const std::string dumpPath = "test/ut/resources/dump/output/";
    Utility::MkdirRecusively(dumpPath);
    ParsePcCode test(dumpPath, pcSet);
    std::string fileName = JoinPath({dumpPath, "pc_start_addr.txt"});
    std::ofstream writeFile(fileName, std::fstream::out | std::fstream::trunc);
    writeFile << "0x800000" << std::endl;
    writeFile.close();
    MOCKER(&Utility::CmdExecute).stubs().will(returnValue(false));
    ASSERT_FALSE(test.GetPcSetByKernelName("aaa_mix_aic.o"));
    std::experimental::filesystem::remove_all(dumpPath);
}

/**
 * |  用例集  | ParsePcCode
 * | 测试函数 | GetAllPc
 * |  用例名  | 测试输入起始pc和偏移后拿到的pcSet是否正确
 */
TEST(ParsePcCode, test_get_all_pc) {
    ParsePcCode pc2code("");
    pc2code.GetAllPc("00", "8");
    EXPECT_EQ(pc2code.pcSet_.size(), 3);
}