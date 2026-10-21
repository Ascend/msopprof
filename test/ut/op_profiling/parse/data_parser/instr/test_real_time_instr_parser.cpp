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
#include "parse/data_parser/real_time_data_parser.h"
#include "profiling/simulator/data_parse/sim_data_parse.h"
#include "parse/plugin/plugin_manager.h"
#include "parse/plugin/plugin_interface.h"
#undef protected
#undef private
#include "parse/data_table/instr_detail_table.h"
#include "profiling/simulator/data_parse/sim_defs.h"
#include "instr_encoding/instr_disassembler.h"

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

/**
 * |  用例集 | RealTimeInstrParser
 * | 测试函数 | SetInstrLog(DvcInstrLog)
 * |  用例名  | test_set_instr_log_v1_should_extract_binary
 * | 用例描述 | 910B实时v1路径，从decodeDescr中解析Binary字段并归一化，
 * |          回归验证"临时字符串传给regex_match导致的悬垂指针"修复后binary不被污染
 */
TEST(RealTimeInstrParser, test_set_instr_log_v1_should_extract_binary) {
    Common::ProfMetricsAbilityConfig metricsConfig;
    RealTimeSimParseContext realTimeSimParseContext{{}, true, ChipProductType::ASCEND910B_SERIES, metricsConfig};
    RealTimeDataParser parser(realTimeSimParseContext);
    parser.realTimeInstrParser_.dataCenter_.DataStreamRegister<InstrParseInfoForRealTime>();

    Common::DvcInstrLog log{};
    log.time = 100;
    log.pc = 0x10d0d00c;
    log.coreId = 0;
    log.subCoreId = 1; // 1 -> veccore0
    snprintf(log.decodeDescr, sizeof(log.decodeDescr), "(PC: 0x10d0d00c) SCALAR   : (Binary: 0x003bd781) ADD");
    snprintf(log.execDescr, sizeof(log.execDescr), "dtype:S64, XD:X29=0x107fa0, XN:X29=0x107fa0, XM:X15=0,");
    parser.SetInstrLog(log);

    auto instrPtr = parser.realTimeInstrParser_.dataCenter_.GetStreamPtr<InstrParseInfoForRealTime>();
    ASSERT_TRUE(instrPtr != nullptr);
    InstrParseInfoForRealTime data;
    ASSERT_TRUE(instrPtr->TryPop(data));
    EXPECT_EQ(data.name, "ADD");
    EXPECT_EQ(data.pipe, "SCALAR");
    EXPECT_EQ(data.binary, "003bd781");
}

/**
 * |  用例集 | RealTimeInstrParser
 * | 测试函数 | SetPopInstrLog(DvcInstrLog)
 * |  用例名  | test_set_pop_instr_log_v1_should_extract_binary
 * | 用例描述 | 910B实时v1路径，从popped日志的decodeDescr中解析Binary字段并归一化
 */
TEST(RealTimeInstrParser, test_set_pop_instr_log_v1_should_extract_binary) {
    Common::ProfMetricsAbilityConfig metricsConfig;
    RealTimeSimParseContext realTimeSimParseContext{{}, true, ChipProductType::ASCEND910B_SERIES, metricsConfig};
    RealTimeDataParser parser(realTimeSimParseContext);
    parser.realTimeInstrParser_.dataCenter_.DataStreamRegister<PoppedInstrParseInfoForRealTime>();

    Common::DvcInstrLog log{};
    log.time = 100;
    log.pc = 0x10d0d000;
    log.coreId = 0;
    log.subCoreId = 1; // 1 -> veccore0
    snprintf(log.decodeDescr, sizeof(log.decodeDescr), "(PC: 0x10d0d000) SCALAR   : (Binary: 0x073a7fa0) MOV_XD_IMM");
    snprintf(log.execDescr, sizeof(log.execDescr), "XD:X29=0x7fa0, IMM:0x7fa0,");
    parser.SetPopInstrLog(log);

    auto popPtr = parser.realTimeInstrParser_.dataCenter_.GetStreamPtr<PoppedInstrParseInfoForRealTime>();
    ASSERT_TRUE(popPtr != nullptr);
    PoppedInstrParseInfoForRealTime data;
    ASSERT_TRUE(popPtr->TryPop(data));
    EXPECT_EQ(data.name, "MOV_XD_IMM");
    EXPECT_EQ(data.pipe, "SCALAR");
    EXPECT_EQ(data.binary, "073a7fa0");
}

/**
 * |  用例集 | RealTimeInstrParser
 * | 测试函数 | SetInstrLog(DvcInstrLogV2)
 * |  用例名  | test_set_instr_log_v2_simt_should_concat_binary
 * | 用例描述 | A5实时v2路径，SIMT 128位binary日志中拆两段，需反序拼接后再归一化
 */
TEST(RealTimeInstrParser, test_set_instr_log_v2_simt_should_concat_binary) {
    Common::ProfMetricsAbilityConfig metricsConfig;
    RealTimeSimParseContext realTimeSimParseContext{{}, true, ChipProductType::ASCEND950PR_9599, metricsConfig};
    RealTimeDataParser parser(realTimeSimParseContext);
    parser.realTimeInstrParser_.dataCenter_.DataStreamRegister<InstrParseInfoForRealTime>();

    Common::DvcInstrLogV2 log{};
    log.time = 100;
    log.pc = 0x9154d4fca8;
    log.coreId = 0;
    log.subCoreId = 1; // 1 -> veccore0
    snprintf(log.decodeDescr, sizeof(log.decodeDescr),
        "(PC: 0x9154d4fca8) RVECEX   : (Binary: 0x04060000000e005d ffffffe800000001) "
        "(ID: 000140) SIMT_IADD_I");
    snprintf(log.extendParamsJson, sizeof(log.extendParamsJson), "{\"core_type\":\"AIV0\"}");
    parser.SetInstrLog(log);

    auto instrPtr = parser.realTimeInstrParser_.dataCenter_.GetStreamPtr<InstrParseInfoForRealTime>();
    ASSERT_TRUE(instrPtr != nullptr);
    InstrParseInfoForRealTime data;
    ASSERT_TRUE(instrPtr->TryPop(data));
    EXPECT_EQ(data.name, "SIMT_IADD_I");
    EXPECT_EQ(data.pipe, "RVECEX");
    EXPECT_EQ(data.binary, "ffffffe80000000104060000000e005d");
}

/**
 * |  用例集 | RealTimeInstrParser
 * | 测试函数 | NormalizeBinary
 * |  用例名  | test_normalize_binary
 * | 用例描述 | 归一化binary：去0x/小写；SIMT两段反序拼接；空串返回空
 */
TEST(InstrDisassembler, test_normalize_binary) {
    EXPECT_EQ(Encode::InstrDisassembler::NormalizeBinary("0x073a7fa0"), "073a7fa0");
    EXPECT_EQ(Encode::InstrDisassembler::NormalizeBinary("0x06002e00029e065c"), "06002e00029e065c");
    EXPECT_EQ(Encode::InstrDisassembler::NormalizeBinary("0x0e0e0010088e005d ffffffff00000001"),
        "ffffffff000000010e0e0010088e005d");
    EXPECT_TRUE(Encode::InstrDisassembler::NormalizeBinary("").empty());
}

/**
 * |  用例集 | RealTimeInstrParser
 * | 测试函数 | EncodingToBinary
 * |  用例名  | test_encoding_to_binary
 * | 用例描述 | 把InstrEncoding解出的encoding转成反汇编map key：32/64/128位及非法位宽
 */
TEST(InstrDisassembler, test_encoding_to_binary) {
    EXPECT_EQ(Encode::InstrDisassembler::EncodingToBinary(0x073a7fa0, 0, Encode::EncodingType::BIT32), "073a7fa0");
    EXPECT_EQ(Encode::InstrDisassembler::EncodingToBinary(0x06002e00029e065c, 0, Encode::EncodingType::BIT64),
        "06002e00029e065c");
    EXPECT_EQ(Encode::InstrDisassembler::EncodingToBinary(
                  0x0e0e0010088e005d, 0xffffffff00000001, Encode::EncodingType::BIT128),
        "ffffffff000000010e0e0010088e005d");
    EXPECT_TRUE(Encode::InstrDisassembler::EncodingToBinary(0, 0, Encode::EncodingType::INVALID).empty());
}
