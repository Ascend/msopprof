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

#include <string>
#include <sys/stat.h>
#define private public
#define protected public
#include "profiling/device/data_visualize/timeline_parser/aicore_timeline_parser.h"
#include "profiling/device/data_parse/metric_csv_header.h"
#include "profiling/device/data_parse/metric_data_handler.h"
#include "common/hal_helper.h"
#include "common/visualize.h"
#include "packet.h"
#undef private
#undef protected
#include "op_runner.h"
#include "common/defs.h"
#include "filesystem.h"
#include "json_parser.h"
#include "json_defs.h"
#include "cmd_execute.h"

using namespace Utility;
using namespace Profiling;
using namespace Common;
using namespace std;
using namespace Visualize;

unique_ptr<DataHandler> &GetHandleTest()
{
    static unique_ptr<DataHandler> handlePtr = Utility::MakeUnique<DataHandlerOf910B>();
    return handlePtr;
}

shared_ptr<Visualize::BasicPmu> &GetbasicPmuObj(const std::string &type)
{
    static shared_ptr<Visualize::BasicPmu> basicPmuPtr = Utility::MakeShared<Visualize::BasicPmu>(GetHandleTest());
    if (type == "mix") {
        basicPmuPtr->totalPmuData_ = {
            {{0, "cube0"}, {{}, 10000000, "cube", {0, 1000}}},
            {{0, "vector0"}, {{}, 2000000, "vector", {0, 1000}}},
            {{0, "vector1"}, {{}, 2000000, "vector", {0, 1000}}},
        };
    } else if (type == "vector") {
        basicPmuPtr->totalPmuData_ = {
            {{0, "vector0"}, {{}, 2000000, "vector", {0, 1000}}},
            {{0, "vector1"}, {{}, 2000000, "vector", {0, 1000}}}
        };
    } else {
         basicPmuPtr->totalPmuData_ = {
            {{0, "cube0"}, {{}, 10000000, "cube", {0, 1000}}},
        };
    }
    return basicPmuPtr;
}

inline shared_ptr<Visualize::OpBasicInfo> &GetOpBasicInfoObj()
{
    static shared_ptr<Visualize::OpBasicInfo> opBasicInfoPtr = Utility::MakeShared<Visualize::OpBasicInfo>(GetHandleTest());
    return opBasicInfoPtr;
}

std::vector<MsprofAicTimeStampInfo> &GetAicoreTimeStamps(const std::string &opType)
{
    MsprofAicTimeStampInfo cube0Start {10, 0, 65577, 0x22};
    MsprofAicTimeStampInfo cube0End {20, 0, 65577, 0x22};
    MsprofAicTimeStampInfo vec0Start {10, 1638400, 65577, 0x22};
    MsprofAicTimeStampInfo vec0End {20, 1638400, 65577, 0x22};
    MsprofAicTimeStampInfo vec1Start {10, 1703937, 65577, 0x22};
    MsprofAicTimeStampInfo vec1End {20, 1703937, 65577, 0x22};
    static std::vector<MsprofAicTimeStampInfo> aicoreTimeStamps = {vec0Start, vec0End, vec1Start, vec1End};
    if (opType == "mix") {
        aicoreTimeStamps = {cube0Start, cube0End, vec0Start, vec0End, vec1Start, vec1End};
    } else if (opType == "cube") {
       aicoreTimeStamps = {cube0Start, cube0End};
    }
    return aicoreTimeStamps;
}

/**
 * |  用例集  | AicoreTimelineParser
 * | 测试函数 | ProcessBlockDur
 * |  用例名  | test_ProcessBlockDur_should_return_true
 */
TEST(AicoreTimelineParser, test_ProcessBlockDur_should_return_true)
{
    GlobalMockObject::verify();
    AicoreTimelineParser aicoreTimeStamps(0, GetOpBasicInfoObj(), GetbasicPmuObj("mix"));
    aicoreTimeStamps.ProcessAicoreBlockDur();
    auto aicoreDuration = aicoreTimeStamps.blockSystemTimes_;
    for (const auto & temp : aicoreDuration) {
        for (const auto & timeRecord : temp.second) {
            ASSERT_TRUE(timeRecord.first == 0);
            ASSERT_TRUE(timeRecord.second == 1000);
        }
    }
}

/**
 * |  用例集  | AicoreTimelineParser
 * | 测试函数 | GetMinSysCycle
 * |  用例名  | test_GetMinSysCycle_should_return_true
 */
TEST(AicoreTimelineParser, test_GetMinSysCycle_should_return_true)
{
    GlobalMockObject::verify();
    AicoreTimelineParser aicoreTimeStamps(0, GetOpBasicInfoObj(), GetbasicPmuObj("mix"));
    aicoreTimeStamps.ProcessAicoreBlockDur();
    ASSERT_TRUE(aicoreTimeStamps.minSysCyc_ == 0);
}


/**
 * |  用例集  | AicoreTimelineParser
 * | 测试函数 | GetTimeStampType
 * |  用例名  | test_GetTimeStampType_should_return_true
 */
TEST(AicoreTimelineParser, test_GetTimeStampType_should_return_true)
{
    GlobalMockObject::verify();
    AicoreTimelineParser aicoreTimeStamps(0, GetOpBasicInfoObj(), GetbasicPmuObj("mix"));
    auto infos = GetAicoreTimeStamps("mix");
    std::vector<MsprofAicTimeStampInfoUpdate> timeStamps;
    aicoreTimeStamps.GetTimeStampType(infos, timeStamps);
    ASSERT_STREQ(timeStamps[0].type.c_str(), "AIC BLOCK");
    ASSERT_STREQ(timeStamps[1].type.c_str(), "AIC BLOCK");
    ASSERT_STREQ(timeStamps[2].type.c_str(), "AIV BLOCK");
    ASSERT_STREQ(timeStamps[3].type.c_str(), "AIV BLOCK");
    ASSERT_STREQ(timeStamps[4].type.c_str(), "AIV BLOCK");
    ASSERT_STREQ(timeStamps[5].type.c_str(), "AIV BLOCK");
}

/**
 * |  用例集  | AicoreTimelineParser
 * | 测试函数 | ParseCustomDotJson
 * |  用例名  | test_ParseCustomDotJson_with_empty_path_expect_no_change
 * | 用例描述 | 测试空路径时不修改enableBlockTime_和descIdDisplay_
 */
TEST(AicoreTimelineParser, test_ParseCustomDotJson_with_empty_path_expect_no_change)
{
    AicoreTimelineParser::descIdDisplay_.clear();
    AicoreTimelineParser::descIdDisplayCached_ = false;
    AicoreTimelineParser parser(0, GetOpBasicInfoObj(), GetbasicPmuObj("vector"), "");
    parser.ParseCustomDotJson();
    EXPECT_TRUE(parser.enableBlockTime_);
    EXPECT_EQ(parser.descIdDisplay_.size(), 0);
}

/**
 * |  用例集  | AicoreTimelineParser
 * | 测试函数 | ParseDescIdDisplay
 * |  用例名  | test_ParseDescIdDisplay_with_valid_input_expect_success
 * | 用例描述 | 测试descIdDisplay解析十进制和十六进制key正确
 */
TEST(AicoreTimelineParser, test_ParseDescIdDisplay_with_valid_input_expect_success)
{
    AicoreTimelineParser::descIdDisplay_.clear();
    AicoreTimelineParser::descIdDisplayCached_ = false;
    nlohmann::json descIdJson;
    descIdJson["65536"] = "NOP";
    descIdJson["0x10"] = "LOAD";
    descIdJson["0x10001"] = "STORE";
    descIdJson["0X10002"] = "COMPUTE";
    AicoreTimelineParser parser(0, GetOpBasicInfoObj(), GetbasicPmuObj("vector"), "mock_path.json");
    parser.ParseDescIdDisplay(descIdJson);
    EXPECT_EQ(parser.descIdDisplay_.size(), 4);
    EXPECT_EQ(parser.descIdDisplay_[65536], "NOP");
    EXPECT_EQ(parser.descIdDisplay_[16], "LOAD");
    EXPECT_EQ(parser.descIdDisplay_[65537], "STORE");
    EXPECT_EQ(parser.descIdDisplay_[65538], "COMPUTE");
}

/**
 * |  用例集  | AicoreTimelineParser
 * | 测试函数 | ParseDescIdDisplay
 * |  用例名  | test_ParseDescIdDisplay_with_invalid_key_expect_skip_invalid
 * | 用例描述 | 测试无效key时跳过该条目继续解析
 */
TEST(AicoreTimelineParser, test_ParseDescIdDisplay_with_invalid_key_expect_skip_invalid)
{
    AicoreTimelineParser::descIdDisplay_.clear();
    AicoreTimelineParser::descIdDisplayCached_ = false;
    nlohmann::json descIdJson;
    descIdJson["abc"] = "INVALID_KEY";
    descIdJson["0x"] = "EMPTY_HEX";
    descIdJson["1"] = "VALID_NUM";
    AicoreTimelineParser parser(0, GetOpBasicInfoObj(), GetbasicPmuObj("vector"), "");
    parser.ParseDescIdDisplay(descIdJson);
    EXPECT_EQ(parser.descIdDisplay_.size(), 1);
    EXPECT_EQ(parser.descIdDisplay_[1], "VALID_NUM");
}

/**
 * |  用例集  | AicoreTimelineParser
 * | 测试函数 | ParseDescIdDisplay
 * |  用例名  | test_ParseDescIdDisplay_with_invalid_value_expect_skip_invalid
 * | 用例描述 | 测试无效value时跳过该条目继续解析
 */
TEST(AicoreTimelineParser, test_ParseDescIdDisplay_with_invalid_value_expect_skip_invalid)
{
    AicoreTimelineParser::descIdDisplay_.clear();
    AicoreTimelineParser::descIdDisplayCached_ = false;
    nlohmann::json descIdJson;
    descIdJson["0"] = "VALID_VALUE";
    descIdJson["1"] = 123;
    descIdJson["2"] = "invalid@chars!";
    descIdJson["3"] = std::string(65, 'a');
    AicoreTimelineParser parser(0, GetOpBasicInfoObj(), GetbasicPmuObj("vector"), "");
    parser.ParseDescIdDisplay(descIdJson);
    EXPECT_EQ(parser.descIdDisplay_.size(), 1);
    EXPECT_EQ(parser.descIdDisplay_[0], "VALID_VALUE");
}
