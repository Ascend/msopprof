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
#include "parse/plugin/plugin_manager.h"
#include "parse/plugin/plugin_interface.h"
#undef private
#undef protected
#include "parse/data_table/instr_detail_table.h"
#include "profiling/simulator/data_parse/sim_defs.h"

using namespace Profiling::Parse;
using namespace Utility;
using namespace Profiling;

SimDataParserConfig GetSimConfig ()
{
    std::set<int> parseIds = {0};
    SimDataParserConfig config {"core0.cubecore0", {}, true, false};
    return config;
};
/**
 * |  用例集 | RealTimeInstrParser
 * | 测试函数 | SetInstrLog
 * |  用例名  | test_set_instr_pop_log_success
 * | 用例描述 | instr log 和 pop log加入到datacenter成功，需要判断是否加入成功以及加入的数据是否正确，此处检查name
 */
TEST(RealTimeInstrParser, test_set_instr_pop_log_success) {
    Common::ProfMetricsAbilityConfig metricsConfig;
    RealTimeSimParseContext realTimeSimParseContext {{}, true, ChipProductType::ASCEND910B_SERIES,
                                                     metricsConfig};
    RealTimeInstrParser realTimeInstrParser(realTimeSimParseContext);
    realTimeInstrParser.dataCenter_.DataStreamRegister<InstrParseInfoForRealTime>();
    realTimeInstrParser.dataCenter_.DataStreamRegister<PoppedInstrParseInfoForRealTime>();
    Profiling::InstrParseInfoForRealTime instrParseInfo;
    instrParseInfo.name = "AAAA";
    realTimeInstrParser.SetInstrLog(instrParseInfo);

    Profiling::PoppedInstrParseInfoForRealTime popParseInfo;
    popParseInfo.name = "BBBB";
    realTimeInstrParser.SetPopInstrLog(popParseInfo);
    std::thread th {[&](){
        sleep(2);
        realTimeInstrParser.Stop();
    }};

    auto instrPtr = realTimeInstrParser.realTimeInstrParserPlugin_->dataCenter_.GetStreamPtr<
        InstrParseInfoForRealTime>();

    auto popPtr = realTimeInstrParser.realTimePopParserPlugin_->dataCenter_.GetStreamPtr<
        PoppedInstrParseInfoForRealTime>();

    EXPECT_TRUE(instrPtr != nullptr);
    EXPECT_TRUE(popPtr != nullptr);
    InstrParseInfoForRealTime instrData;
    EXPECT_TRUE(instrPtr->TryPop(instrData));
    PoppedInstrParseInfoForRealTime popDate;
    EXPECT_TRUE(popPtr->TryPop(popDate));
    EXPECT_EQ(instrData.name, "AAAA");
    EXPECT_EQ(popDate.name, "BBBB");
    if (th.joinable()) {
        th.join();
    }
}

/**
* |  用例集 | RealTimeInstrParser
* | 测试函数 | IsSkipSetLog
* |  用例名  | test_set_instr_pop_log_success
* | 用例描述 | 跳过不需要解析的核，步骤：
*              step1 设置解析的核为1
*              step2 传入核为0和1的detail数据，popinstrLogParsers类会解析出逻辑核
*              step3 传入核为0和1的instr数据,此时会跳过0核的数据只记录1核的数据
*/
TEST(RealTimeInstrParser, test_skip_core) {
    Common::ProfMetricsAbilityConfig metricsConfig;
    RealTimeSimParseContext realTimeSimParseContext {{1}, true, ChipProductType::ASCEND910B_SERIES,
                                                     metricsConfig};
    RealTimeInstrParser realTimeInstrParser(realTimeSimParseContext);
    realTimeInstrParser.Start();

    Profiling::PoppedInstrParseInfoForRealTime popParseInfo;
    popParseInfo.name = "MOV_XD_SPR";
    popParseInfo.coreName = "core0.cubecore0";
    popParseInfo.detail = "XD:X7=0, SPR:BLOCKID,";
    popParseInfo.pipe = "SCALAR";
    realTimeInstrParser.SetPopInstrLog(popParseInfo);

    Profiling::PoppedInstrParseInfoForRealTime popParseInfo1;
    popParseInfo1.name = "MOV_XD_SPR";
    popParseInfo1.coreName = "core1.cubecore0";
    popParseInfo1.detail = "XD:X7=1, SPR:BLOCKID,";
    popParseInfo1.pipe = "SCALAR";
    realTimeInstrParser.SetPopInstrLog(popParseInfo1);
    usleep(100000);

    Profiling::InstrParseInfoForRealTime instrParseInfo;
    instrParseInfo.name = "AAAA2";
    instrParseInfo.coreName = "core0.cubecore0";
    realTimeInstrParser.SetInstrLog(instrParseInfo);

    Profiling::InstrParseInfoForRealTime instrParseInfo1;
    instrParseInfo1.name = "BBBB2";
    instrParseInfo1.coreName = "core1.cubecore0";
    realTimeInstrParser.SetInstrLog(instrParseInfo1);
    usleep(100000);
    EXPECT_EQ(realTimeInstrParser.realTimeInstrParserPlugin_->instrLogParsers_.size(), 1);
    realTimeInstrParser.Stop();
}
