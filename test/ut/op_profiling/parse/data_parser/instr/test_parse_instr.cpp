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
#include "parse/data_parser/sim_dump_parser.h"
#include "profiling/simulator/data_parse/sim_defs.h"

using namespace Profiling::Parse;
using namespace Utility;
using namespace Profiling;

SimDataParserConfig GetSimConfig (const ChipProductType &type, bool overhead) {
    std::string dumpPath910B = "test/ut/resources/dump/910B";
    std::string dumpPath91095 = "test/ut/resources/dump/91095";
    CoreNameAndPreFixPair coreNamePair910B {"core0.veccore0", "core0.veccore0."};
    CoreNameAndPreFixPair coreNamePair91095 {"core0.veccore0", "core0.veccore0."};
    std::set<int> parseIds = {0};
    if (type == ChipProductType::ASCEND950PR_9599) {
        SimDataParserConfig config {dumpPath91095, coreNamePair91095, parseIds, true, overhead, type };
        return config;
    }
    SimDataParserConfig config {dumpPath910B, coreNamePair910B, parseIds, true, overhead, type};
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
    auto mergeInfo = instrPtr->GetColumnData<MergeInfo>(InstrDetailTable::MERGE_INFO);
    ASSERT_TRUE(mergeInfo != nullptr);
    ASSERT_EQ(mergeInfo->size(), 3U);
    EXPECT_EQ(mergeInfo->front().detail, R"({"processed_bytes":8192,"src_mem":"OUT"})");
    EXPECT_EQ(mergeInfo->at(1).detail, "XD:X0=0x106b8000,SPR:PARA_BASE,");
}

TEST(InstrParser, test_a5_two_dump_versions_should_preserve_expected_detail) {
    SimDataParserConfig config = GetSimConfig(ChipProductType::ASCEND950PR_9599, true);
    InstrLogParser instrParser{config, "core0.veccore0"};
    instrParser.ParseLine("[info] [00000749] (PC: 0x11bbe040) SCALAR   : (Binary: 0x02004880) (ID: 000124) "
                          "MOV_XD_SPR  XD:X0=0x106b9000, SPR:PARA_BASE,",
        MatchMode::ID_MATCH);
    instrParser.ParseLine("[info] [00034438] (PC: 0x9000d10628) RVECEX   : (Binary: 0x80082700) (ID: 183300) "
                          "RV_VADD Dtype: F16",
        MatchMode::ID_MATCH);
    instrParser.ParseLine("[info] [00012323] (PC: 0x9000d0d080) ALL      : (Binary: 0x40e01800) (ID: 000032) "
                          "BAR  {\"sync_kind\":\"BARRIER\",\"target_pipe\":\"ALL\"}",
        MatchMode::ID_MATCH);

    const auto &instrMap = instrParser.GetInstrLog();
    ASSERT_EQ(instrMap.at(124).size(), 1U);
    EXPECT_EQ(instrMap.at(124).front().detail, "XD:X0=0x106b9000,SPR:PARA_BASE,");
    ASSERT_EQ(instrMap.at(183300).size(), 1U);
    EXPECT_TRUE(instrMap.at(183300).front().detail.empty());
    ASSERT_EQ(instrMap.at(32).size(), 1U);
    EXPECT_EQ(instrMap.at(32).front().detail, R"({"sync_kind":"BARRIER","target_pipe":"ALL"})");

    PopLogParser popParser{config};
    popParser.ParseLine("[info] [00000745] (PC: 0x11bbe040) SCALAR   : (Binary: 0x02004880) (ID: 000124) "
                        "MOV_XD_SPR  XD:X0=0x106b8000, SPR:PARA_BASE,",
        MatchMode::ID_MATCH);
    popParser.ParseLine("[info] [00034438] (PC: 0x9000d10628) RVECEX   : (Binary: 0x80082700) (ID: 183300) "
                        "RV_VADD Dtype: F16",
        MatchMode::ID_MATCH);
    popParser.ParseLine(
        "[info] [00012320] (PC: 0x9000d0d080) ALL      : (Binary: 0x40e01800) (ID: 000032) BAR", MatchMode::ID_MATCH);

    const auto &popMap = popParser.GetPopLog();
    ASSERT_EQ(popMap.at(124).size(), 1U);
    EXPECT_EQ(popMap.at(124).front().detail, "XD:X0=0x106b8000,SPR:PARA_BASE,");
    ASSERT_EQ(popMap.at(183300).size(), 1U);
    EXPECT_TRUE(popMap.at(183300).front().detail.empty());

    DataCenter dataCenter;
    InstrParser mergeParser{dataCenter, config};
    ASSERT_TRUE(mergeParser.MergeLog(instrParser, popParser, MatchMode::ID_MATCH, true));
    auto detailTable = dataCenter.GetDbPtr<InstrDetailTable>();
    ASSERT_TRUE(detailTable != nullptr);
    auto mergeInfo = detailTable->GetColumnData<MergeInfo>(InstrDetailTable::MERGE_INFO);
    ASSERT_TRUE(mergeInfo != nullptr);
    ASSERT_EQ(mergeInfo->size(), 3U);
    EXPECT_EQ(mergeInfo->at(0).detail, R"({"sync_kind":"BARRIER","target_pipe":"ALL"})");
    EXPECT_EQ(mergeInfo->at(1).detail, "XD:X0=0x106b8000,SPR:PARA_BASE,");
    EXPECT_TRUE(mergeInfo->at(2).detail.empty());
}

TEST(InstrParser, test_a5_extend_params_should_supply_warp_schedule_and_gpr_count) {
    SimDataParserConfig config = GetSimConfig(ChipProductType::ASCEND950PR_9599, true);
    InstrLogParser instrParser{config, "core0.veccore0"};
    const std::string detail = R"({"core_type":"AIV0","gpr_count":2,"sch_id":3,"warp_id":7})";
    instrParser.ParseLine("[info] [00000120] (PC: 0x9000d0d348) RVECST   : "
                          "(Binary: 0x120800041c100480 ) (ID: 000042) SIMT_STS  " +
            detail,
        MatchMode::ID_MATCH);

    PopLogParser popParser{config};
    popParser.ParseLine("[info] [00000100] (PC: 0x9000d0d348) RVECST   : "
                        "(Binary: 0x120800041c100480 ) (ID: 000042) SIMT_STS",
        MatchMode::ID_MATCH);

    DataCenter dataCenter;
    InstrParser mergeParser{dataCenter, config};
    ASSERT_TRUE(mergeParser.MergeLog(instrParser, popParser, MatchMode::ID_MATCH, true));
    auto detailTable = dataCenter.GetDbPtr<InstrDetailTable>();
    ASSERT_TRUE(detailTable != nullptr);
    auto mergeInfo = detailTable->GetColumnData<MergeInfo>(InstrDetailTable::MERGE_INFO);
    ASSERT_TRUE(mergeInfo != nullptr);
    ASSERT_EQ(mergeInfo->size(), 1U);
    EXPECT_EQ(mergeInfo->front().detail, detail);
    EXPECT_EQ(mergeInfo->front().warpId, 7);
    EXPECT_EQ(mergeInfo->front().schId, 3);

    Common::ProfMetricsAbilityConfig metricsConfig;
    CalCulateDetail(dataCenter, ChipProductType::ASCEND950PR_9599, metricsConfig, 1);
    EXPECT_EQ(mergeInfo->front().gprCount, 2);
}

TEST(InstrParser, test_dfx_region_json_detail_should_create_user_mark_without_nop_dependency) {
    SimDataParserConfig config = GetSimConfig(ChipProductType::ASCEND950PR_9599, true);
    InstrLogParser instrParser{config, "core0.veccore0"};
    instrParser.ParseLine("[info] [00000100] (PC: 0x10d0d128) FLOWCTRL : (Binary: 0x42c20020) (ID: 000001) "
                          R"(DFX_REGION  {"xt_value":"0x401"})",
        MatchMode::ID_MATCH);
    instrParser.ParseLine("[info] [00000140] (PC: 0x10d0d1c8) FLOWCTRL : (Binary: 0x42c20040) (ID: 000003) "
                          R"(DFX_REGION  {"xt_value":"0xc01"})",
        MatchMode::ID_MATCH);

    const auto &markMap = instrParser.GetUserMarkInfo();
    ASSERT_EQ(markMap.count("Mark 0x1"), 1U);
    ASSERT_EQ(markMap.at("Mark 0x1").size(), 1U);
    EXPECT_EQ(markMap.at("Mark 0x1").front().startTick, 100U);
    EXPECT_EQ(markMap.at("Mark 0x1").front().endTick, 140U);
    EXPECT_EQ(markMap.at("Mark 0x1").front().startPc, 0x10d0d128U);
    EXPECT_EQ(markMap.at("Mark 0x1").front().endPc, 0x10d0d1c8U);
    EXPECT_EQ(instrParser.GetInstrLog().count(1), 0U);
    EXPECT_EQ(instrParser.GetInstrLog().count(3), 0U);

    instrParser.DisposeUserMark();
    const auto &userMarks = instrParser.GetUserMarkInstr();
    ASSERT_EQ(userMarks.size(), 1U);
    EXPECT_EQ(userMarks.front().name, "Mark 0x1");
    EXPECT_EQ(userMarks.front().pipe, USER_MARK);
    EXPECT_EQ(userMarks.front().startTick, 100U);
    EXPECT_EQ(userMarks.front().endTick, 140U);
}

TEST(InstrParser, test_dfx_region_callback_dump_should_support_max_id) {
    SimDataParserConfig config = GetSimConfig(ChipProductType::ASCEND950PR_9599, true);
    InstrLogParser instrParser{config, "core0.veccore0"};
    instrParser.ParseLine("[info] [00000200] (PC: 0x1000) FLOWCTRL : (Binary: 0x42c20020) (ID: 000010) "
                          R"(DFX_REGION  {"xt_value":"0x7ff"})",
        MatchMode::ID_MATCH);
    instrParser.ParseLine("[info] [00000280] (PC: 0x1004) FLOWCTRL : (Binary: 0x42c20040) (ID: 000011) "
                          R"(DFX_REGION  {"xt_value":"0xfff"})",
        MatchMode::ID_MATCH);

    instrParser.DisposeUserMark();
    const auto &userMarks = instrParser.GetUserMarkInstr();
    ASSERT_EQ(userMarks.size(), 1U);
    EXPECT_EQ(userMarks.front().name, "Mark 0x3ff");
    EXPECT_EQ(userMarks.front().startTick, 200U);
    EXPECT_EQ(userMarks.front().endTick, 280U);
    EXPECT_EQ(userMarks.front().detail, R"({"xt_value":"0x7ff"})");
}

TEST(InstrParser, test_invalid_dfx_region_data_should_remain_normal_instruction) {
    SimDataParserConfig config = GetSimConfig(ChipProductType::ASCEND950PR_9599, true);
    InstrLogParser instrParser{config, "core0.veccore0"};
    instrParser.ParseLine("[info] [00000300] (PC: 0x2000) FLOWCTRL : (Binary: 0x42c20020) (ID: 000020) "
                          R"(DFX_REGION  {"xn_value":"0x401"})",
        MatchMode::ID_MATCH);
    instrParser.ParseLine("[info] [00000310] (PC: 0x2004) FLOWCTRL : (Binary: 0x42c20020) (ID: 000021) "
                          "DFX_REGION  PIPE:SCALAR, XT:X8=0x401,",
        MatchMode::ID_MATCH);
    instrParser.ParseLine("[info] [00000320] (PC: 0x2008) FLOWCTRL : (Binary: 0x42c20020) (ID: 000022) "
                          R"(DFX_REGION  {"xt_value":"invalid"})",
        MatchMode::ID_MATCH);
    instrParser.ParseLine("[info] [00000330] (PC: 0x200c) FLOWCTRL : (Binary: 0x42c20020) (ID: 000023) "
                          R"(MOV  {"xt_value":"0x401"})",
        MatchMode::ID_MATCH);
    instrParser.ParseLine("[info] [00000340] (PC: 0x2010) FLOWCTRL : (Binary: 0x42c20020) (ID: 000024) "
                          R"(DFX_REGION  {"xt_value":"0x401")",
        MatchMode::ID_MATCH);
    instrParser.ParseLine("[info] [00000350] (PC: 0x2014) FLOWCTRL : (Binary: 0x42c20020) (ID: 000025) "
                          R"(DFX_REGION  {"processed_bytes":64})",
        MatchMode::ID_MATCH);
    instrParser.ParseLine("[info] [00000360] (PC: 0x2018) FLOWCTRL : (Binary: 0x42c20020) (ID: 000026) "
                          R"(DFX_REGION  {"xt_value":"0x1000"})",
        MatchMode::ID_MATCH);
    instrParser.ParseLine("[info] [00000370] (PC: 0x201c) FLOWCTRL : (Binary: 0x42c20020) (ID: 000027) "
                          R"(DFX_REGION  {"xt_value":"0x800"})",
        MatchMode::ID_MATCH);

    EXPECT_TRUE(instrParser.GetUserMarkInfo().empty());
    const auto &instrMap = instrParser.GetInstrLog();
    EXPECT_EQ(instrMap.count(20), 1U);
    EXPECT_EQ(instrMap.count(21), 1U);
    EXPECT_EQ(instrMap.count(22), 1U);
    EXPECT_EQ(instrMap.count(23), 1U);
    EXPECT_EQ(instrMap.count(24), 1U);
    EXPECT_EQ(instrMap.count(25), 1U);
    EXPECT_EQ(instrMap.count(26), 1U);
    EXPECT_EQ(instrMap.count(27), 1U);
}

TEST(InstrParser, test_spr_cond_should_keep_legacy_nop_boundaries) {
    SimDataParserConfig config = GetSimConfig(ChipProductType::ASCEND910B1, true);
    InstrLogParser instrParser{config, "core0.veccore0"};
    instrParser.ParseLine("[info] [00000400] (PC: 0x3000) SCALAR   : (Binary: 0x02004880) "
                          "MOV_XD_SPR  XD:X0=0x80000001, SPR:COND,",
        MatchMode::PC_MATCH);
    instrParser.ParseLine("[info] [00000410] (PC: 0x3004) SCALAR   : (Binary: 0x00000000) NOP_PIPE",
        MatchMode::PC_MATCH);
    instrParser.ParseLine("[info] [00000440] (PC: 0x3008) SCALAR   : (Binary: 0x02004880) "
                          "MOV_XD_SPR  XD:X0=0xc0000001, SPR:COND,",
        MatchMode::PC_MATCH);
    instrParser.ParseLine("[info] [00000450] (PC: 0x300c) SCALAR   : (Binary: 0x00000000) NOP_PIPE",
        MatchMode::PC_MATCH);

    instrParser.DisposeUserMark();
    const auto &userMarks = instrParser.GetUserMarkInstr();
    ASSERT_EQ(userMarks.size(), 1U);
    EXPECT_EQ(userMarks.front().name, "Mark 0x1");
    EXPECT_EQ(userMarks.front().startTick, 410U);
    EXPECT_EQ(userMarks.front().endTick, 450U);
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
 * |  用例集 | InstrParser
 * | 测试函数 | ParseThreadId
 * |  用例名  | test_ParseThreadId_should_parse_ok
 * | 用例描述 | 检查解析ParseThreadId值的功能是否正确
 */
TEST(InstrParser, test_ParseThreadId_should_parse_ok) {
    DataCenter dataCenter;
    SimDataParserConfig config = GetSimConfig(ChipProductType::ASCEND950DT_950Y, true);
    InstrParser instrParse {dataCenter, config};
    MergeInfo s {};
    std::string detail = "[PEX:7|P],[Rn:5|R],[Rd:5|R],[#imm32:1],[waitBitMask:0],[stallCyc:4],[yeild:0],[inv:0],[warpId:59],[bundleId:14],[schId:3],[prdctMask:ffffffff],[execMask:ffffffff],[exec_time:8]";
    instrParse.ParseThreadId(detail, s.detail);
    ASSERT_TRUE(s.detail.find("[threadNum:32") != std::string::npos);
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
    SimDataParserConfig config {"core0", {}, false, false};
    InstrParser instrParser(dataCenter, config);
    std::vector<PoppedInstrParseInfo> instrPoppedVec;
    std::vector<InstrParseInfo> instrVec;
    EXPECT_EQ(instrParser.GetPruneSize(instrPoppedVec, instrVec), 0);
    GlobalMockObject::verify();
}

TEST(AllPraseProcess, test_GetPruneSize_more_instr_return_1) {
    GlobalMockObject::verify();
    DataCenter dataCenter;
    SimDataParserConfig config {"core0", {}, false, false};
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
    SimDataParserConfig config {"core0", {}, false, false};
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
    SimDataParserConfig config {"core0", {}, false, false};
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
    SimDataParserConfig config {"core0", {}, false, false};
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
    SimDataParserConfig config {"test", {}, false, false};
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
    SimDataParserConfig config {"core0", {}, false, false};
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
