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