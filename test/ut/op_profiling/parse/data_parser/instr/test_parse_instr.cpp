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
#include "parse/data_parser/instr_parser/instr_parser.h"
#include "parse/data_parser/instr_parser/instr_log_parser.h"
#include "parse/data_parser/instr_parser/real_time_instr_parser.h"
#include "profiling/simulator/data_parse/sim_data_parse.h"
#undef protected
#undef private
#include "parse/data_table/instr_detail_table.h"
#include "profiling/simulator/data_parse/sim_defs.h"

using namespace Profiling::Parse;
using namespace Utility;
using namespace Profiling;

SimDataParserConfig GetSimConfig (const ChipProductType &type, bool showSetWait) {
    std::string dumpPath910B = "test/ut/resources/dump/910B";
    std::string dumpPath91095 = "test/ut/resources/dump/91095";
    CoreNameAndPreFixPair coreNamePair910B {"core0.veccore0", "core0.veccore0."};
    CoreNameAndPreFixPair coreNamePair91095 {"core0.veccore0", "core0.veccore0."};
    std::set<int> parseIds = {0};
    if (type == ChipProductType::ASCEND950PR_9599) {
        SimDataParserConfig config {dumpPath91095, coreNamePair91095, parseIds, showSetWait, type };
        return config;
    }
    SimDataParserConfig config {dumpPath910B, coreNamePair910B, parseIds, showSetWait, type};
    return config;
};
/**
 * |  用例集 | InstrParser
 * | 测试函数 | Entry
 * |  用例名  | test_InstrParser_should_return_ture_when_parse_ok
 * | 用例描述 | InstrParser的st，检查解析正常的全部功能
 */
TEST(InstrParser, test_910B1_InstrParser_should_return_ture_when_parse_ok) {
    DataCenter dataCenter;
    SimDataParserConfig config = GetSimConfig(ChipProductType::ASCEND910B1, true);
    InstrParser instrParse {dataCenter, config};
    ASSERT_TRUE(instrParse.Entry() == PluginErrorCode::SUCCESS);
    auto instrPtr = dataCenter.GetDbPtr<InstrDetailTable>();
    auto userMarkPtr = dataCenter.GetDbPtr<UserMarkStruct>();
    ASSERT_TRUE(instrPtr != nullptr);
    ASSERT_TRUE(userMarkPtr != nullptr);
    ASSERT_TRUE(instrPtr->GetColumnData<MergeInfo>(InstrDetailTable::MERGE_INFO)->size() == 6);
    ASSERT_TRUE(userMarkPtr->userMarkInstrs.size() == 1);
}

/**
 * |  用例集 | InstrParser
 * | 测试函数 | Entry
 * |  用例名  | test_InstrParser_should_return_ture_when_parse_ok
 * | 用例描述 | InstrParser的st，检查解析正常的全部功能
 */
TEST(InstrParser, test_9109599_InstrParser_should_return_ture_when_parse_ok) {
    DataCenter dataCenter;
    SimDataParserConfig config = GetSimConfig(ChipProductType::ASCEND950PR_9599, true);
    InstrParser instrParse {dataCenter, config};
    ASSERT_TRUE(instrParse.Entry() == PluginErrorCode::SUCCESS);
    auto instrPtr = dataCenter.GetDbPtr<InstrDetailTable>();
    ASSERT_TRUE(instrPtr != nullptr);
    ASSERT_TRUE(instrPtr->GetColumnData<MergeInfo>(InstrDetailTable::MERGE_INFO)->size() == 3);
}

/**
 * |  用例集 | AllPraseProcess
 * | 测试函数 | Entry
 * |  用例名  | test_AllPraseProcess_should_return_ture_when_parse_ok
 * | 用例描述 | AllPraseProcess的st，检查解析的全部功能
 */
TEST(AllPraseProcess, test_AllPraseProcess_should_return_ture_when_parse_ok) {
    GlobalMockObject::verify();
    Common::ProfMetricsAbilityConfig aicMetrics;
    std::string dumpPath910B = "test/ut/resources/dump/910B";
    const std::string output = "test/ut/resources/dump/output";
    Utility::MkdirRecusively(output);
    SimDataParse sim("Ascend910B1", dumpPath910B, "", aicMetrics);
    MOCKER(CheckFolder)
            .stubs()
            .will(returnValue(true));
    ASSERT_TRUE(sim.ParseMergeDumpData(dumpPath910B, dumpPath910B, dumpPath910B));
    std::experimental::filesystem::remove_all(output);
    GlobalMockObject::verify();
}


/**
 * |  用例集 | AllPraseProcess
 * | 测试函数 | Entry
 * |  用例名  | test_AllPraseProcess_should_return_ture_when_parse_ok
 * | 用例描述 | AllPraseProcess的st，检查解析的全部功能
 */
TEST(AllPraseProcess, test_GetPruneSize_return_0) {
    GlobalMockObject::verify();
    DataCenter dataCenter;
    SimDataParserConfig config {"core0", {}, false};
    InstrParser instrParser(dataCenter, config);
    std::vector<PoppedInstrParseInfo> instrPoppedVec;
    std::vector<InstrParseInfo> instrVec;
    EXPECT_EQ(instrParser.GetPruneSize(instrPoppedVec, instrVec), 0);
    GlobalMockObject::verify();
}

TEST(AllPraseProcess, test_GetPruneSize_more_instr_return_1) {
    GlobalMockObject::verify();
    DataCenter dataCenter;
    SimDataParserConfig config {"core0", {}, false};
    InstrParser instrParser(dataCenter, config);

    PoppedInstrParseInfo poppedInstrParseInfo;
    InstrParseInfo instrParseInfo;
    InstrParseInfo instrParseInfo1;
    std::vector<InstrParseInfo> instrVec = {instrParseInfo, instrParseInfo1};
    std::vector<PoppedInstrParseInfo> instrPoppedVec = {poppedInstrParseInfo};
    EXPECT_EQ(instrParser.GetPruneSize(instrPoppedVec, instrVec), 1);
    GlobalMockObject::verify();
}


TEST(AllPraseProcess, test_GetPruneSize_more_pop_return_1) {
    GlobalMockObject::verify();
    DataCenter dataCenter;
    SimDataParserConfig config {"core0", {}, false};
    InstrParser instrParser(dataCenter, config);

    PoppedInstrParseInfo poppedInstrParseInfo;
    PoppedInstrParseInfo poppedInstrParseInfo1;

    InstrParseInfo instrParseInfo;
    std::vector<InstrParseInfo> instrVec = {instrParseInfo};
    std::vector<PoppedInstrParseInfo> instrPoppedVec = {poppedInstrParseInfo, poppedInstrParseInfo1};
    EXPECT_EQ(instrParser.GetPruneSize(instrPoppedVec, instrVec), 1);
    GlobalMockObject::verify();
}

TEST(AllPraseProcess, test_parse_real_time_pop) {
    GlobalMockObject::verify();
    DataCenter dataCenter;
    SimDataParserConfig config {"core0", {}, false};
    PopLogParser popParser(config);

    PoppedInstrParseInfo poppedInstrParseInfo;
    popParser.ParseRealTimeDumpLog(poppedInstrParseInfo);
    EXPECT_EQ(poppedInstrParseInfo.gprCount, 0);
    GlobalMockObject::verify();
}

TEST(AllPraseProcess, test_parse_real_time_is_skip_set_log_expect_false) {
    GlobalMockObject::verify();
    DataCenter dataCenter;
    Common::ProfMetricsAbilityConfig metricsConfig;
    RealTimeSimParseContext context {
        {},false, ChipProductType::ASCEND910B1, metricsConfig
    };
    // create parser whose popLogParsers_ is empty
    RealTimeInstrParser parser(context);
    std::string testCoreName = "test";
    EXPECT_EQ(parser.IsSkipSetLog(testCoreName), false);
    GlobalMockObject::verify();
}

TEST(AllPraseProcess, test_parse_real_time_is_skip_set_log_expect_true) {
    GlobalMockObject::verify();
    DataCenter dataCenter;
    Common::ProfMetricsAbilityConfig metricsConfig;
    RealTimeSimParseContext context {
            {1},false, ChipProductType::ASCEND910B1, metricsConfig
    };
    RealTimeInstrParser parser(context);
    SimDataParserConfig config {"core0", {}, false};
    PopLogParser popParser(config);
    popParser.coreId_ = 0;
    // add parsers for test core name
    parser.realTimePopParserPlugin_->popLogParsers_.insert({"test", popParser});
    std::string testCoreName = "test";
    EXPECT_EQ(parser.IsSkipSetLog(testCoreName), true);
    GlobalMockObject::verify();
}

TEST(AllPraseProcess, test_parse_real_time_set_instr_and_instr_pop_log_expect_failed) {
    GlobalMockObject::verify();
    DataCenter dataCenter;
    Common::ProfMetricsAbilityConfig metricsConfig;
    RealTimeSimParseContext context {
            {1},false, ChipProductType::ASCEND910B1, metricsConfig
    };
    RealTimeInstrParser parser(context);
    InstrParseInfoForRealTime parserInfo;
    parserInfo.coreName = "test";

    PoppedInstrParseInfoForRealTime poppedParserInfo;
    poppedParserInfo.coreName = "test";

    // check branch which stream ptr is nullptr
    parser.SetInstrLog(parserInfo);
    parser.SetPopInstrLog(poppedParserInfo);
    EXPECT_EQ(parser.dataCenter_.GetStreamPtr<InstrParseInfoForRealTime>(), nullptr);
    EXPECT_EQ(parser.dataCenter_.GetStreamPtr<PoppedInstrParseInfoForRealTime>(), nullptr);

    // check branch which get core name failed
    parser.dataCenter_.DataStreamRegister<InstrParseInfoForRealTime>();
    parser.dataCenter_.DataStreamRegister<PoppedInstrParseInfoForRealTime>();
    SimDataParserConfig config {"test", {}, false};
    PopLogParser popParser(config);
    parser.realTimePopParserPlugin_->popLogParsers_.insert({"test", popParser});
    parser.SetInstrLog(parserInfo);
    parser.SetPopInstrLog(poppedParserInfo);
    // if core name is found, no data will be push into dataQueue_
    EXPECT_EQ(parser.dataCenter_.GetStreamPtr<InstrParseInfoForRealTime>()->dataQueue_.empty(), true);
    EXPECT_EQ(parser.dataCenter_.GetStreamPtr<PoppedInstrParseInfoForRealTime>()->dataQueue_.empty(), true);
    GlobalMockObject::verify();
}

TEST(AllPraseProcess, test_parse_real_time_set_instr_and_instr_pop_log_expect_success) {
    GlobalMockObject::verify();
    DataCenter dataCenter;
    Common::ProfMetricsAbilityConfig metricsConfig;
    RealTimeSimParseContext context {
            {},false, ChipProductType::ASCEND910B1, metricsConfig
    };
    RealTimeInstrParser parser(context);
    InstrParseInfoForRealTime parserInfo;
    parserInfo.coreName = "test";

    PoppedInstrParseInfoForRealTime poppedParserInfo;
    poppedParserInfo.coreName = "test";

    parser.dataCenter_.DataStreamRegister<InstrParseInfoForRealTime>();
    parser.dataCenter_.DataStreamRegister<PoppedInstrParseInfoForRealTime>();
    SimDataParserConfig config {"core0", {}, false};
    PopLogParser popParser(config);
    popParser.coreId_ = 0;
    // add parsers for test core name
    parser.realTimePopParserPlugin_->popLogParsers_.insert({"test", popParser});
    parser.SetInstrLog(parserInfo);
    parser.SetPopInstrLog(poppedParserInfo);

    // if core name is not found, instr data will be push into dataQueue_
    EXPECT_EQ(parser.dataCenter_.GetStreamPtr<InstrParseInfoForRealTime>()->dataQueue_.empty(), false);
    EXPECT_EQ(parser.dataCenter_.GetStreamPtr<PoppedInstrParseInfoForRealTime>()->dataQueue_.empty(), false);

    GlobalMockObject::verify();
}
