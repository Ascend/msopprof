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
#include "parse/data_parser/real_time_data_parser.h"
#include "smart_pointer.h"
#undef private
#undef protected
#include "filesystem.h"
#include "parse/data_parser/sim_dump_parser.h"
#include "profiling/simulator/data_parse/sim_defs.h"

using namespace Profiling::Parse;
using namespace Profiling;
using namespace Utility;

/**
 * |  用例集  | RealTimeDataParseTest
 * | 测试函数 | Start
 * |  用例名  | test_null_instr_log
 * | 用例描述 | 测试instr_log为空时的结果
 */
TEST(RealTimeDataParseTest, test_null_instr_log)
{
    RealTimeSimParseContext realTimeSimParseContext;
    realTimeSimParseContext.chipType = ChipProductType::ASCEND910B4;
    RealTimeDataParser realTimeDataParse(realTimeSimParseContext);
    Common::DvcInstrLog dvcInstrLog{};
    Common::DvciCacheLog iCacheLog{};
    realTimeDataParse.Start("./aaa", "add");
    realTimeDataParse.SetPopInstrLog(dvcInstrLog);
    realTimeDataParse.SetInstrLog(dvcInstrLog);
    realTimeDataParse.SetICacheLog(iCacheLog);
    usleep(10);
    realTimeDataParse.Stop();
    EXPECT_EQ(realTimeDataParse.isStop_, true);
}

/**
 * |  用例集  | RealTimeDataParseTest
 * | 测试函数 | SetMteLog
 * |  用例名  | test_null_mte_log
 * | 用例描述 | 输入空的mte_log，没有数据生成
 */
TEST(RealTimeDataParseTest, test_null_mte_log)
{
    RealTimeSimParseContext realTimeSimParseContext;
    realTimeSimParseContext.chipType = ChipProductType::ASCEND910B4;
    realTimeSimParseContext.metricsConfig.pmSamplingEnable = true;
    RealTimeDataParser realTimeDataParse(realTimeSimParseContext);
    realTimeDataParse.Start("./aaa", "add");
    Common::DvcMteLog mteLog{};
    realTimeDataParse.SetMteLog(mteLog);
    usleep(1000);
    realTimeDataParse.Stop();
    EXPECT_EQ(realTimeDataParse.isStop_, true);
}

/**
 * |  用例集  | RealTimeDataParseTest
 * | 测试函数 | SetMteLog
 * |  用例名  | test_collect_mte_log
 * | 用例描述 | 输入无效时，返回错误
 */
TEST(RealTimeDataParseTest, test_collect_mte_log)
{
    RealTimeSimParseContext realTimeSimParseContext;
    realTimeSimParseContext.chipType = ChipProductType::ASCEND910B4;
    realTimeSimParseContext.metricsConfig.pmSamplingEnable = true;
    RealTimeDataParser realTimeDataParse(realTimeSimParseContext);
    realTimeDataParse.Start("./aaa", "add");
    Common::DvcMteLog mteLog{};
    mteLog.instrId = 0;
    std::string in = "BRIF";
    size_t len = std::min(in.length(), sizeof(mteLog.intf) - 1);
    mteLog.coreId = 0;
    mteLog.size = 10;
    std::copy_n(in.c_str(), len, mteLog.intf);
    realTimeDataParse.SetMteLog(mteLog);
    usleep(1000);
    realTimeDataParse.Stop();
    auto ptr = realTimeDataParse.realTimeMteParser_.dataCenter_.GetDbPtr<
            std::vector<Parse::MteLogInstrMap>>();
    EXPECT_TRUE(ptr != nullptr);
    EXPECT_EQ(realTimeDataParse.isStop_, true);
}

/**
 * |  用例集  | RealTimeDataParseTest
 * | 测试函数 | SetInstrLog
 * |  用例名  | test_null_instr_log
 * | 用例描述 | 测试instr_log为空时的结果
 */
TEST(RealTimeDataParseTest, test_collect_instr_log)
{
    GlobalMockObject::verify();
    RealTimeSimParseContext realTimeSimParseContext;
    realTimeSimParseContext.chipType = ChipProductType::ASCEND910B4;
    RealTimeDataParser realTimeDataParse(realTimeSimParseContext);
    Common::DvcInstrLog dvcInstrLog{};
    dvcInstrLog.coreId = 0;
    dvcInstrLog.subCoreId = 0;
    dvcInstrLog.time = 111;
    dvcInstrLog.pc = 222;
    std::string decode = "(PC: 0x126448f0) SCALAR   : (Binary: 0x00d8518a) AND";
    std::string execDescr = "dtype:S64, XD:X0=0x103afea0, XN:X2=0x103afe40, IMM:0x60,";
    size_t len1 = std::min(decode.length(), sizeof(dvcInstrLog.decodeDescr) - 1);
    size_t len2 = std::min(execDescr.length(), sizeof(dvcInstrLog.execDescr) - 1);
    std::copy_n(decode.c_str(), len1, dvcInstrLog.decodeDescr);
    std::copy_n(execDescr.c_str(), len2, dvcInstrLog.execDescr);
    realTimeDataParse.Start("./aaa", "add");
    realTimeDataParse.SetInstrLog(dvcInstrLog);
    usleep(10);
    realTimeDataParse.Stop();
    auto size = realTimeDataParse.realTimeInstrParser_.realTimeInstrParserPlugin_->instrLogParsers_.size();
    EXPECT_TRUE(size != 0);
    EXPECT_EQ(realTimeDataParse.isStop_, true);
    GlobalMockObject::verify();
}

/**
 * |  用例集  | RealTimeDataParseTest
 * | 测试函数 | SetInstrLog
 * |  用例名  | test_null_instr_log
 * | 用例描述 | 传输错误的subcore，将不会处理并打印错误日志
 */
TEST(RealTimeDataParseTest, test_wrong_subcore_instr_log)
{
    RealTimeSimParseContext realTimeSimParseContext;
    realTimeSimParseContext.chipType = ChipProductType::ASCEND910B4;
    RealTimeDataParser realTimeDataParse(realTimeSimParseContext);
    Common::DvcInstrLog dvcInstrLog{};
    dvcInstrLog.coreId = 0;
    dvcInstrLog.subCoreId = 10;
    dvcInstrLog.time = 111;
    dvcInstrLog.pc = 222;
    std::string decode = "(PC: 0x126448f0) SCALAR   : (Binary: 0x00d8518a) AND";
    std::string execDescr = "dtype:S64, XD:X0=0x103afea0, XN:X2=0x103afe40, IMM:0x60,";
    size_t len1 = std::min(decode.length(), sizeof(dvcInstrLog.decodeDescr) - 1);
    size_t len2 = std::min(execDescr.length(), sizeof(dvcInstrLog.execDescr) - 1);
    std::copy_n(decode.c_str(), len1, dvcInstrLog.decodeDescr);
    std::copy_n(execDescr.c_str(), len2, dvcInstrLog.execDescr);
    realTimeDataParse.Start("./aaa", "add");
    realTimeDataParse.SetInstrLog(dvcInstrLog);
    usleep(10);
    realTimeDataParse.Stop();
    auto size = realTimeDataParse.realTimeInstrParser_.realTimeInstrParserPlugin_->instrLogParsers_.size();
    EXPECT_TRUE(size == 0);
    EXPECT_EQ(realTimeDataParse.isStop_, true);
}

/**
 * |  用例集  | RealTimeDataParseTest
 * | 测试函数 | SetInstrLog
 * |  用例名  | test_collect_a5_instr_log_with_decode_detail
 * | 用例描述 | A5 decode 描述携带 Dtype 字段时仍可采集，详情使用 V2 JSON
 */
TEST(RealTimeDataParseTest, test_collect_a5_instr_log_with_decode_detail)
{
    RealTimeSimParseContext context;
    context.chipType = ChipProductType::ASCEND950PR_9599;
    RealTimeDataParser parser(context);
    Common::DvcInstrLogV2 log{};
    log.coreId = 0;
    log.subCoreId = 1;
    log.time = 111;
    log.pc = 0x9000d10628;
    const std::string decode =
        "(PC: 0x9000d10628) RVECEX   : (Binary: 0x80082700) (ID: 002816) RV_VADD Dtype: F16";
    const std::string detail = R"({"burst_len":8192})";
    std::copy_n(decode.c_str(), decode.size(), log.decodeDescr);
    std::copy_n(detail.c_str(), detail.size(), log.extendParamsJson);

    parser.Start("./aaa", "add");
    parser.SetInstrLog(log);
    usleep(10);
    parser.Stop();

    auto &instrParsers = parser.realTimeInstrParser_.realTimeInstrParserPlugin_->instrLogParsers_;
    ASSERT_EQ(instrParsers.count("core0.veccore0"), 1U);
    auto &instrMap = instrParsers.at("core0.veccore0").instrMap_;
    ASSERT_EQ(instrMap.count(2816), 1U);
    ASSERT_EQ(instrMap.at(2816).size(), 1U);
    EXPECT_EQ(instrMap.at(2816).front().name, "RV_VADD");
    EXPECT_EQ(instrMap.at(2816).front().detail, detail);
    EXPECT_TRUE(instrMap.at(2816).front().xnValue.empty());
}

TEST(RealTimeDataParseTest, test_a5_extend_params_should_supply_warp_schedule_and_gpr_count)
{
    RealTimeSimParseContext context;
    context.chipType = ChipProductType::ASCEND950PR_9599;
    RealTimeDataParser parser(context);
    Common::DvcInstrLogV2 poppedLog{};
    poppedLog.coreId = 0;
    poppedLog.subCoreId = 1;
    poppedLog.time = 100;
    poppedLog.pc = 0x9000d0d348;
    const std::string decode =
        "(PC: 0x9000d0d348) RVECST   : (Binary: 0x120800041c100480 ) (ID: 000042) SIMT_STS";
    const std::string poppedDetail =
        R"({"core_type":"AIV0","instr_type":"RVECST","queue_type":"SIMD"})";
    std::copy_n(decode.c_str(), decode.size(), poppedLog.decodeDescr);
    std::copy_n(poppedDetail.c_str(), poppedDetail.size(), poppedLog.extendParamsJson);

    Common::DvcInstrLogV2 completeLog = poppedLog;
    completeLog.time = 120;
    const std::string completeDetail =
        R"({"core_type":"AIV0","gpr_count":2,"sch_id":3,"warp_id":7})";
    std::fill_n(completeLog.extendParamsJson, sizeof(completeLog.extendParamsJson), '\0');
    std::copy_n(completeDetail.c_str(), completeDetail.size(), completeLog.extendParamsJson);

    parser.Start("./aaa", "add");
    parser.SetPopInstrLog(poppedLog);
    parser.SetInstrLog(completeLog);
    usleep(100000);
    parser.realTimeInstrParser_.Stop();
    parser.isStop_ = true;

    std::map<std::string, std::shared_ptr<DataCenter>> dataCenterMap;
    parser.realTimeInstrParser_.Merge(dataCenterMap);
    ASSERT_EQ(dataCenterMap.count("core0.veccore0"), 1U);
    auto &dataCenter = *dataCenterMap.at("core0.veccore0");
    auto detailTable = dataCenter.GetDbPtr<InstrDetailTable>();
    ASSERT_TRUE(detailTable != nullptr);
    auto mergeInfo = detailTable->GetColumnData<MergeInfo>(InstrDetailTable::MERGE_INFO);
    ASSERT_TRUE(mergeInfo != nullptr);
    ASSERT_EQ(mergeInfo->size(), 1U);
    EXPECT_EQ(mergeInfo->front().detail, completeDetail);
    EXPECT_EQ(mergeInfo->front().warpId, 7);
    EXPECT_EQ(mergeInfo->front().schId, 3);

    CalCulateDetail(dataCenter, ChipProductType::ASCEND950PR_9599, context.metricsConfig, 1);
    EXPECT_EQ(mergeInfo->front().gprCount, 2);
}

TEST(RealTimeDataParseTest, test_collect_dfx_region_user_mark_from_xt_value)
{
    RealTimeSimParseContext context;
    context.chipType = ChipProductType::ASCEND950PR_9599;
    RealTimeDataParser parser(context);
    Common::DvcInstrLogV2 startLog{};
    startLog.coreId = 0;
    startLog.subCoreId = 1;
    startLog.time = 400;
    startLog.pc = 0x10d0d128;
    const std::string startDecode =
        "(PC: 0x10d0d128) FLOWCTRL : (Binary: 0x42c20020) (ID: 000030) DFX_REGION";
    const std::string startDetail = R"({"xt_value":"0x401"})";
    std::copy_n(startDecode.c_str(), startDecode.size(), startLog.decodeDescr);
    std::copy_n(startDetail.c_str(), startDetail.size(), startLog.extendParamsJson);

    Common::DvcInstrLogV2 stopLog{};
    stopLog.coreId = 0;
    stopLog.subCoreId = 1;
    stopLog.time = 460;
    stopLog.pc = 0x10d0d1c8;
    const std::string stopDecode =
        "(PC: 0x10d0d1c8) FLOWCTRL : (Binary: 0x42c20040) (ID: 000031) DFX_REGION";
    const std::string stopDetail = R"({"xt_value":"0xc01"})";
    std::copy_n(stopDecode.c_str(), stopDecode.size(), stopLog.decodeDescr);
    std::copy_n(stopDetail.c_str(), stopDetail.size(), stopLog.extendParamsJson);

    parser.Start("./aaa", "add");
    parser.SetInstrLog(startLog);
    parser.SetInstrLog(stopLog);
    usleep(100000);
    parser.Stop();

    auto &instrParsers = parser.realTimeInstrParser_.realTimeInstrParserPlugin_->instrLogParsers_;
    ASSERT_EQ(instrParsers.count("core0.veccore0"), 1U);
    auto &instrParser = instrParsers.at("core0.veccore0");
    ASSERT_EQ(instrParser.userMarkParseInfo_.size(), 2U);
    EXPECT_EQ(instrParser.userMarkParseInfo_.front().detail, startDetail);
    EXPECT_EQ(instrParser.userMarkParseInfo_.back().detail, stopDetail);
    instrParser.DisposeUserMark();
    const auto &userMarks = instrParser.GetUserMarkInstr();
    ASSERT_EQ(userMarks.size(), 1U);
    EXPECT_EQ(userMarks.front().name, "Mark 0x1");
    EXPECT_EQ(userMarks.front().pipe, USER_MARK);
    EXPECT_EQ(userMarks.front().startTick, 400U);
    EXPECT_EQ(userMarks.front().endTick, 460U);
}
