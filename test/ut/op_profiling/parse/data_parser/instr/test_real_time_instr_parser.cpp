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
#include <thread>
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
#include "log.h"
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
    EXPECT_EQ(data.binary, Encode::InstrDisassembler::ParseBinary("003bd781"));
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
    EXPECT_EQ(data.binary, Encode::InstrDisassembler::ParseBinary("073a7fa0"));
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
    EXPECT_EQ(data.binary, Encode::InstrDisassembler::ParseBinary("ffffffe80000000104060000000e005d"));
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

TEST(InstrDisassembler, test_from_encoding) {
    const auto binary32 = Encode::InstrDisassembler::FromEncoding(0x073a7fa0, 0, Encode::EncodingType::BIT32);
    EXPECT_EQ(binary32, Encode::InstrDisassembler::ParseBinary("073a7fa0"));
    EXPECT_EQ(binary32.bitType, Encode::EncodingType::BIT32);
    const auto binary64 = Encode::InstrDisassembler::FromEncoding(0x06002e00029e065c, 0, Encode::EncodingType::BIT64);
    EXPECT_EQ(binary64, Encode::InstrDisassembler::ParseBinary("06002e00029e065c"));
    EXPECT_EQ(binary64.bitType, Encode::EncodingType::BIT64);
    const auto binary128 =
        Encode::InstrDisassembler::FromEncoding(0x0e0e0010088e005d, 0xffffffff00000001, Encode::EncodingType::BIT128);
    EXPECT_EQ(binary128, Encode::InstrDisassembler::ParseBinary("ffffffff000000010e0e0010088e005d"));
    EXPECT_EQ(binary128.bitType, Encode::EncodingType::BIT128);
    EXPECT_TRUE(Encode::InstrDisassembler::FromEncoding(0, 0, Encode::EncodingType::INVALID).empty());
}

TEST(InstrDisassembler, test_binary_matches_by_requested_width)
{
    const auto a = Encode::InstrDisassembler::ParseBinary("00000004000000030000000200000001");
    const auto b = Encode::InstrDisassembler::ParseBinary("00000008000000070000000600000001");
    EXPECT_TRUE(a.Matches(b, Encode::EncodingType::BIT32));
    EXPECT_FALSE(a.Matches(b, Encode::EncodingType::BIT64));
    EXPECT_FALSE(a.Matches(b));
    EXPECT_FALSE(a == b);
    EXPECT_FALSE(a.Matches(a, Encode::EncodingType::INVALID));
    const auto c = Encode::InstrDisassembler::ParseBinary("00000008000000070000000200000001");
    EXPECT_TRUE(a.Matches(c, Encode::EncodingType::BIT64));
    EXPECT_FALSE(a.Matches(c));
}

TEST(InstrDisassembler, test_binary_parsing_and_hash)
{
    const auto binary = Encode::InstrDisassembler::ParseBinary("0x0e0e0010088e005d ffffffff00000001");
    EXPECT_EQ(binary.words[0], 0x088e005dU);
    EXPECT_EQ(binary.words[1], 0x0e0e0010U);
    EXPECT_EQ(binary.words[2], 0x00000001U);
    EXPECT_EQ(binary.words[3], 0xffffffffU);
    EXPECT_EQ(binary.bitType, Encode::EncodingType::BIT128);
    EXPECT_TRUE(Encode::InstrDisassembler::ParseBinary("123").empty());
    EXPECT_TRUE(Encode::InstrDisassembler::ParseBinary("0000000g").empty());
    EXPECT_TRUE(Encode::InstrDisassembler::ParseBinary(std::string(40, '0')).empty());
    const auto narrow = Encode::InstrDisassembler::FromEncoding(~0ULL, ~0ULL, Encode::EncodingType::BIT32);
    EXPECT_EQ(narrow.words[0], 0xffffffffU);
    EXPECT_EQ(narrow.words[1], 0U);
    EXPECT_EQ(narrow.words[2], 0U);
    EXPECT_EQ(narrow.words[3], 0U);
    auto same = narrow;
    same.bitType = Encode::EncodingType::BIT128;
    EXPECT_EQ(narrow, same);
    EXPECT_EQ(Encode::InstrBinaryHash{}(narrow), Encode::InstrBinaryHash{}(same));
}

TEST(InstrDisassembler, test_lookup_uses_requested_width_index)
{
    Encode::InstrDisassembler disassembler;
    for (auto type : {Encode::EncodingType::BIT32, Encode::EncodingType::BIT64,
                      Encode::EncodingType::BIT128}) {
        const auto key = Encode::InstrDisassembler::FromEncoding(1, 0, type);
        const auto text = std::to_string(static_cast<int>(type));
        disassembler.binary2Disasm_[static_cast<size_t>(type)][key] = text;
        EXPECT_EQ(disassembler.GetDisassembly(key, type, 0x100), text);
    }
    const auto query = Encode::InstrDisassembler::ParseBinary("00000004000000030000000200000001");
    EXPECT_EQ(disassembler.GetDisassembly(query, Encode::EncodingType::BIT32, 0x100), "0");
    EXPECT_TRUE(disassembler.GetDisassembly(query, Encode::EncodingType::BIT64, 0x100).empty());
    EXPECT_TRUE(disassembler.GetDisassembly(query, Encode::EncodingType::BIT128, 0x100).empty());
    EXPECT_TRUE(disassembler.GetDisassembly(query, Encode::EncodingType::INVALID, 0x100).empty());
}

TEST(InstrDisassembler, test_truncated_binary_matches_unique_wider_instruction)
{
    Encode::InstrDisassembler disassembler;
    const auto full64 = Encode::InstrDisassembler::ParseBinary("1234567800000001");
    const auto full128 = Encode::InstrDisassembler::ParseBinary("abcdef01234567880000000300000002");
    disassembler.binary2Disasm_[1][full64] = "instruction64";
    disassembler.binary2Disasm_[2][full128] = "instruction128";
    const auto low32 = Encode::InstrDisassembler::ParseBinary("00000001");
    EXPECT_EQ(disassembler.GetDisassembly(low32, low32.bitType, 0x100), "instruction64");
    const auto low128 = Encode::InstrDisassembler::ParseBinary("00000002");
    EXPECT_EQ(disassembler.GetDisassembly(low128, low128.bitType, 0x100), "instruction128");
    const auto low64 = Encode::InstrDisassembler::ParseBinary("0000000300000002");
    EXPECT_EQ(disassembler.GetDisassembly(low64, low64.bitType, 0x100), "instruction128");
    EXPECT_EQ(disassembler.GetDisassembly(full128, Encode::EncodingType::BIT128, 0x100), "instruction128");
    EXPECT_TRUE(disassembler.GetDisassembly(low32, Encode::EncodingType::BIT128, 0x100).empty());
    EXPECT_EQ(disassembler.GetDisassembly(low32, low32.bitType, 0x100), "instruction64");
}

TEST(InstrDisassembler, test_ambiguous_truncated_binary_is_not_matched)
{
    Encode::InstrDisassembler disassembler;
    const auto a = Encode::InstrDisassembler::ParseBinary("1111111100000001");
    const auto b = Encode::InstrDisassembler::ParseBinary("2222222200000001");
    const auto c = Encode::InstrDisassembler::ParseBinary("11111111111111110000000300000002");
    const auto d = Encode::InstrDisassembler::ParseBinary("22222222222222220000000300000002");
    const auto e = Encode::InstrDisassembler::ParseBinary("3333333300000004");
    const auto f = Encode::InstrDisassembler::ParseBinary("44444444444444444444444400000004");
    disassembler.binary2Disasm_[1][a] = "a";
    disassembler.binary2Disasm_[1][b] = "b";
    disassembler.binary2Disasm_[2][c] = "c";
    disassembler.binary2Disasm_[2][d] = "d";
    disassembler.binary2Disasm_[1][e] = "e";
    disassembler.binary2Disasm_[2][f] = "f";
    for (const auto &text : {"00000001", "00000002", "00000004", "0000000300000002"}) {
        const auto key = Encode::InstrDisassembler::ParseBinary(text);
        EXPECT_TRUE(disassembler.GetDisassembly(key, key.bitType, 0x100).empty());
    }
    EXPECT_EQ(disassembler.unmatchedPcs_.count(0x100), 1U);
    // Direct matches take precedence even when the same prefix exists in wider encodings.
    const auto direct = Encode::InstrDisassembler::ParseBinary("00000001");
    disassembler.binary2Disasm_[0][direct] = "direct";
    EXPECT_EQ(disassembler.GetDisassembly(direct, direct.bitType, 0x200), "direct");
    EXPECT_EQ(disassembler.unmatchedPcs_.count(0x200), 0U);
}

TEST(InstrDisassembler, test_all_direct_miss_pcs_are_recorded_and_drained)
{
    Encode::InstrDisassembler disassembler;
    const auto full = Encode::InstrDisassembler::ParseBinary("1234567800000001");
    disassembler.binary2Disasm_[1][full] = "recovered";
    EXPECT_EQ(disassembler.GetDisassembly(full, full.bitType, 0x100), "recovered");
    EXPECT_TRUE(disassembler.unmatchedPcs_.empty());
    const auto truncated = Encode::InstrDisassembler::ParseBinary("00000001");
    EXPECT_EQ(disassembler.GetDisassembly(truncated, truncated.bitType, 0x200), "recovered");
    EXPECT_EQ(disassembler.GetDisassembly(truncated, truncated.bitType, 0x200), "recovered");
    const auto missing = Encode::InstrDisassembler::ParseBinary("000000ff");
    EXPECT_TRUE(disassembler.GetDisassembly(missing, missing.bitType, 0x300).empty());
    EXPECT_TRUE(disassembler.GetDisassembly({}, Encode::EncodingType::INVALID, 0x400).empty());
    EXPECT_EQ(disassembler.unmatchedPcs_.size(), 3U);
    EXPECT_EQ(disassembler.unmatchedPcs_.count(0x200), 1U);
    EXPECT_EQ(disassembler.unmatchedPcs_.count(0x300), 1U);
    EXPECT_EQ(disassembler.unmatchedPcs_.count(0x400), 1U);
    disassembler.LogUnmatchedPcs();
    EXPECT_TRUE(disassembler.unmatchedPcs_.empty());
    disassembler.LogUnmatchedPcs();
    EXPECT_TRUE(disassembler.unmatchedPcs_.empty());
}

TEST(InstrDisassembler, test_debug_log_contains_each_direct_miss_pc_once) {
    Encode::InstrDisassembler d;
    auto &log = Utility::Log::GetLog();
    auto level = log.lv_;
    log.lv_ = Utility::LogLv::DEBUG;
    const auto b = Encode::InstrDisassembler::ParseBinary("00000001");
    d.GetDisassembly(b, b.bitType, 0xabc);
    d.GetDisassembly(b, b.bitType, 0xabc);
    testing::internal::CaptureStdout();
    d.LogUnmatchedPcs();
    d.LogUnmatchedPcs();
    const auto output = testing::internal::GetCapturedStdout();
    log.lv_ = level;
    EXPECT_NE(output.find("[DEBUG]"), std::string::npos);
    const auto pos = output.find("pc=0xabc");
    ASSERT_NE(pos, std::string::npos);
    EXPECT_EQ(output.find("pc=0xabc", pos + 1), std::string::npos);
}
TEST(InstrDisassembler, test_parallel_direct_misses) {
    Encode::InstrDisassembler d;
    const auto full = Encode::InstrDisassembler::ParseBinary("123456789abcdef00000000300000001");
    d.binary2Disasm_[2][full] = "recovered";
    const auto b = Encode::InstrDisassembler::ParseBinary("00000001");
    std::vector<std::thread> workers;
    for (int t = 0; t < 8; ++t) {
        workers.emplace_back([&] {
            for (int i = 0; i < 100; ++i) {
                EXPECT_EQ(d.GetDisassembly(b, b.bitType, i % 16), "recovered");
            }
        });
    }
    for (auto &worker : workers) {
        worker.join();
    }
    EXPECT_EQ(d.unmatchedPcs_.size(), 16U);
    d.LogUnmatchedPcs();
    EXPECT_TRUE(d.unmatchedPcs_.empty());
}

TEST(InstrDisassembler, test_truncated_index_is_built_on_first_partial_lookup) {
    Encode::InstrDisassembler disassembler;
    const auto full = Encode::InstrDisassembler::ParseBinary("123456789abcdef00000000300000001");
    disassembler.binary2Disasm_[2][full] = "recovered";
    EXPECT_EQ(disassembler.GetDisassembly(full, full.bitType, 0x100), "recovered");
    EXPECT_FALSE(disassembler.truncatedIndexBuilt_);
    const auto missing = Encode::InstrDisassembler::FromEncoding(0, 0, Encode::EncodingType::BIT128);
    EXPECT_TRUE(disassembler.GetDisassembly(missing, missing.bitType, 0x200).empty());
    EXPECT_FALSE(disassembler.truncatedIndexBuilt_);
    EXPECT_TRUE(disassembler.truncated2Disasm_[0].empty());
    EXPECT_TRUE(disassembler.truncated2Disasm_[1].empty());
    const auto low32 = Encode::InstrDisassembler::ParseBinary("00000001");
    EXPECT_EQ(disassembler.GetDisassembly(low32, low32.bitType, 0x300), "recovered");
    EXPECT_TRUE(disassembler.truncatedIndexBuilt_);
    const auto low64 = Encode::InstrDisassembler::ParseBinary("0000000300000001");
    EXPECT_EQ(disassembler.GetDisassembly(low64, low64.bitType, 0x400), "recovered");
}

TEST(InstrDisassembler, test_lazy_truncated_indices_are_independent_between_instances) {
    Encode::InstrDisassembler first;
    Encode::InstrDisassembler second;
    const auto fullA = Encode::InstrDisassembler::ParseBinary("1111111100000001");
    const auto fullB = Encode::InstrDisassembler::ParseBinary("2222222200000001");
    const auto low = Encode::InstrDisassembler::ParseBinary("00000001");
    first.binary2Disasm_[1][fullA] = "first";
    second.binary2Disasm_[1][fullB] = "second";
    EXPECT_EQ(first.GetDisassembly(low, low.bitType, 0x100), "first");
    EXPECT_TRUE(first.truncatedIndexBuilt_);
    EXPECT_FALSE(second.truncatedIndexBuilt_);
    EXPECT_EQ(second.GetDisassembly(low, low.bitType, 0x100), "second");
    EXPECT_EQ(first.GetDisassembly(low, low.bitType, 0x100), "first");
}
