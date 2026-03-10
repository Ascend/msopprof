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
#include "profiling/device/data_visualize/aicore_timeline_parser.h"
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

std::vector<std::string> &GetAicoreType(const std::string &opType)
{
    static std::vector<string> aicoreTimeStampsType = {"AIV BLOCK", "AIV BLOCK", "AIV BLOCK", "AIV BLOCK"};
    if (opType == "mix") {
        aicoreTimeStampsType = {"AIC BLOCK", "AIC BLOCK", "AIV BLOCK", "AIV BLOCK", "AIV BLOCK", "AIV BLOCK"};
    } else if (opType == "cube") {
       aicoreTimeStampsType = {"AIC BLOCK", "AIC BLOCK"};
    }
    return aicoreTimeStampsType;
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
    aicoreTimeStamps.ProcessBlockDur(GetAicoreTimeStamps("mix"), GetAicoreType("mix"));
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
    aicoreTimeStamps.ProcessBlockDur(GetAicoreTimeStamps("mix"), GetAicoreType("mix"));
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
    std::vector<std::string> type;
    aicoreTimeStamps.GetTimeStampType(GetAicoreTimeStamps("mix"), type);
    ASSERT_STREQ(type[0].c_str(), "AIC BLOCK");
    ASSERT_STREQ(type[1].c_str(), "AIC BLOCK");
    ASSERT_STREQ(type[2].c_str(), "AIV BLOCK");
    ASSERT_STREQ(type[3].c_str(), "AIV BLOCK");
    ASSERT_STREQ(type[4].c_str(), "AIV BLOCK");
    ASSERT_STREQ(type[4].c_str(), "AIV BLOCK");
}