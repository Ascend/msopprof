#include <gtest/gtest.h>
#include <iostream>
#include "mockcpp/mockcpp.hpp"
#include "filesystem.h"
#define private public
#define protected public
#include "profiling/device/data_parse/tlv_parse.h"
#undef protected
#undef private


/**
 * |  用例集 | TlvParser
 * | 测试函数 | ParseStream
 * |  用例名  | test_parse_tlv_return_eq
 * | 用例描述 | 输入有效值，返回true
 */

TEST(TlvParser, test_parse_tlv_return_eq)
{
    TlvParser parser("../test/ut/resources/dump/91095/lrm.data");
    parser.ParseStream();
    const auto &result = parser.GetParsedData();
    parser.CountPCNum();
    ASSERT_EQ(result.instNum, 4);
    ASSERT_EQ(result.regNumber[0], 128);
    ASSERT_EQ(result.regType[0], RegType::X);
    ASSERT_EQ(result.functionNameNum, 3);
    ASSERT_EQ(result.functionNameList[1], "func2");
    ASSERT_EQ(result.functionPositionStartIdx[1], 0);
    ASSERT_EQ(parser.pcNumCount[result.instRecordList[0][1].address], 3);
}

TEST(TlvParser, test_parse_tlv2_return_eq)
{
    TlvParser parser("../test/ut/resources/dump/91095/lrm2.data");
    parser.ParseStream();
    const auto &result = parser.GetParsedData();
    parser.CountPCNum();
    ASSERT_EQ(result.instNum, 3);
    ASSERT_EQ(result.regNumber[0], 64);
    ASSERT_EQ(result.regType[2], RegType::R);
    ASSERT_EQ(result.functionNameNum, 3);
    ASSERT_EQ(result.functionNameList[1], "tell_me");
    ASSERT_EQ(result.functionPositionStartIdx[1], 1);
    ASSERT_EQ(parser.pcNumCount[result.instRecordList[0][1].address], 9);
}

/**
 * |  用例集 | TlvParser测试simt算子文件
 * | 测试函数 | ParseStream
 * |  用例名  | test_parse_tlv_return_eq
 * | 用例描述 | 输入有效值，返回true
 */

TEST(TlvParser, test_parse_tlv3_return_eq)
{
    TlvParser parser("../test/ut/resources/dump/91095/lrm3.data");
    parser.ParseStream();
    const auto &result = parser.GetParsedData();
    parser.CountPCNum();
    ASSERT_EQ(result.instNum, 149);
    ASSERT_EQ(result.regNumber[0], 128);
    ASSERT_EQ(result.regType[0], RegType::R);
    ASSERT_EQ(result.functionNameNum, 3);
    ASSERT_EQ(result.functionNameList[1], "_Z11SimtComputeIiEvPU3AS1T_S2_S2_i_simt_entry");
    ASSERT_EQ(result.functionPositionStartIdx[0], 0);
    ASSERT_EQ(result.functionPositionStartIdx[1], 53);
    ASSERT_EQ(result.functionPositionStartIdx[2], 148);
    ASSERT_EQ(parser.pcNumCount[result.instRecordList[0][55].address], 4);
}