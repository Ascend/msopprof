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

#include <algorithm>
#include <array>
#include <cstdlib>
#include <fstream>
#include <string>
#include <sys/stat.h>
#define private public
#define protected public
#include "profiling/device/op_device_prof.h"
#include "profiling/device/run/device_task.h"
#include "instr_encoding/instr_encoding.h"
#include "profiling/device/data_parse/device_data_parse.h"
#include "profiling/device/data_parse/pmu_calculate.h"
#include "profiling/device/data_parse/dbi_parser.h"
#include "profiling/device/data_parse/hotspot_function_generator.h"
#include "profiling/device/data_parse/l2cache/l2cache.h"
#include "profiling/device/data_parse/metric_csv_header.h"
#include "profiling/device/data_parse/metric_data_handler.h"
#include "profiling/device/data_visualize/biu_timeline.h"
#include "profiling/device/data_visualize/storage_access.h"
#include "parse/data_calculator/compute_load_calculator.h"
#include "instr_encoding/instr_encoding.h"
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
using namespace Encode;
using namespace Visualize;
using nlohmann::json;

namespace {
constexpr size_t TEST_PC_SAMPLING_DATA_LEN = 16;
constexpr size_t TEST_PC_SAMPLING_CHUNK_SIZE = sizeof(InstrProfHeadInfo) + DATA_BUFFER_SIZE;

void WritePcSamplingChunk(std::ofstream &file, const InstrProfHeadInfo &headInfo,
                          const std::vector<std::array<uint8_t, TEST_PC_SAMPLING_DATA_LEN>> &records)
{
    std::vector<char> chunk(TEST_PC_SAMPLING_CHUNK_SIZE, 0);
    ASSERT_EQ(memcpy_s(chunk.data(), chunk.size(), &headInfo, sizeof(InstrProfHeadInfo)), EOK);
    size_t offset = sizeof(InstrProfHeadInfo);
    for (const auto &record : records) {
        ASSERT_LE(offset + record.size(), chunk.size());
        ASSERT_EQ(memcpy_s(chunk.data() + offset, chunk.size() - offset, record.data(), record.size()), EOK);
        offset += record.size();
    }
    file.write(chunk.data(), chunk.size());
}

std::array<uint8_t, TEST_PC_SAMPLING_DATA_LEN> BuildPcSamplingRecord(uint32_t pcBase,
                                                                     const std::array<uint8_t, 9> &states)
{
    std::array<uint8_t, TEST_PC_SAMPLING_DATA_LEN> record {};
    record[0] = static_cast<uint8_t>(pcBase & 0xFF);
    record[1] = static_cast<uint8_t>((pcBase >> 8) & 0xFF);
    record[2] = static_cast<uint8_t>((pcBase >> 16) & 0xFF);
    std::copy(states.begin(), states.end(), record.begin() + 3);
    return record;
}
}

std::map<std::string, uint64_t> basicScalarPmu = {{"Scalar Time", 1832}, {"Scalar Single", 1899}, {"Scalar Dual", 1835}, {"Scalar Mte1 Stall", 1412}, {"Scalar Mte2 Stall", 1455}, {"Scalar Mte3 Stall", 1777},
    {"Scalar Wait IB", 133}, {"Scalar Wait", 1844}, {"Scalar Cube Stall", 1877}, {"Scalar Ub Stall", 1478}, {"Scalar Vector Stall", 1428},
    {"Scalar Time Vec0", 1148}, {"Scalar Single Vec0", 4318}, {"Scalar Dual Vec0", 7158}, {"Scalar Mte2 Stall Vec0", 1388}, {"Scalar Mte3 Stall Vec0", 8158}, {"Scalar Vector Stall Vec0", 118}, {"Scalar Wait IB Vec0", 318}, {"Scalar Wait Vec0", 148}, {"Scalar Ub Stall Vec0", 818},
    {"Scalar Time Vec1", 1148}, {"Scalar Single Vec1", 4318}, {"Scalar Dual Vec1", 7158}, {"Scalar Mte2 Stall Vec1", 1388}, {"Scalar Mte3 Stall Vec1", 8158}, {"Scalar Vector Stall Vec1", 118}, {"Scalar Wait IB Vec1", 318}, {"Scalar Wait Vec1", 148}, {"Scalar Ub Stall Vec1", 818},
    {"Scalar Internuclear ID0", 1275}, {"Scalar Internuclear ID1", 3542}, {"Scalar Internuclear ID2", 4345}, {"Scalar Internuclear ID3", 6514}, {"Scalar Internuclear ID4", 3144}, {"Scalar Internuclear ID5", 5315},
    {"Scalar Internuclear ID6", 6432}, {"Scalar Internuclear ID7", 3152}, {"Scalar Internuclear ID8", 5476}, {"Scalar Internuclear ID9", 2453}, {"Scalar Internuclear ID10", 653},
    {"Scalar Internuclear ID11", 324}, {"Scalar Internuclear ID12", 424}, {"Scalar Internuclear ID13", 653}, {"Scalar Internuclear ID14", 536}, {"Scalar Internuclear ID15", 133},

    {"Scalar Internuclear ID0 Vec0", 1275}, {"Scalar Internuclear ID1 Vec0", 3542}, {"Scalar Internuclear ID2 Vec0", 4345}, {"Scalar Internuclear ID3 Vec0", 6514},
    {"Scalar Internuclear ID4 Vec0", 3144}, {"Scalar Internuclear ID5 Vec0", 5315}, {"Scalar Internuclear ID6 Vec0", 6432}, {"Scalar Internuclear ID7 Vec0", 3152},
    {"Scalar Internuclear ID8 Vec0", 5476}, {"Scalar Internuclear ID9 Vec0", 2453}, {"Scalar Internuclear ID10 Vec0", 653}, {"Scalar Internuclear ID11 Vec0", 324},
    {"Scalar Internuclear ID12 Vec0", 424}, {"Scalar Internuclear ID13 Vec0", 653}, {"Scalar Internuclear ID14 Vec0", 536}, {"Scalar Internuclear ID15 Vec0", 133},

    {"Scalar Internuclear ID0 Vec1", 1275}, {"Scalar Internuclear ID1 Vec1", 3542}, {"Scalar Internuclear ID2 Vec1", 4345}, {"Scalar Internuclear ID3 Vec1", 6514},
    {"Scalar Internuclear ID4 Vec1", 3144}, {"Scalar Internuclear ID5 Vec1", 5315}, {"Scalar Internuclear ID6 Vec1", 6432}, {"Scalar Internuclear ID7 Vec1", 3152},
    {"Scalar Internuclear ID8 Vec1", 5476}, {"Scalar Internuclear ID9 Vec1", 2453}, {"Scalar Internuclear ID10 Vec1", 653}, {"Scalar Internuclear ID11 Vec1", 324},
    {"Scalar Internuclear ID12 Vec1", 424}, {"Scalar Internuclear ID13 Vec1", 653}, {"Scalar Internuclear ID14 Vec1", 536}, {"Scalar Internuclear ID15 Vec1", 133},
};

TEST(OpDeviceProf, GetDataParser_expect_success)
{
    GlobalMockObject::verify();
    MOCKER(&HalHelper::GetPlatformType)
        .stubs()
        .will(returnValue(ChipType::ASCEND910B));
    ProfArgs args;
    OpDeviceProf opDeviceProf(args);
    ASSERT_TRUE(opDeviceProf.GetDataParser().get() != nullptr);
}

TEST(DeviceDataHandler, request_count_rounds_up_and_handles_zero_alignment)
{
    EXPECT_EQ(DataHandlerOf91095::CalcRequest(0, 32), 0U);
    EXPECT_EQ(DataHandlerOf91095::CalcRequest(32, 32), 1U);
    EXPECT_EQ(DataHandlerOf91095::CalcRequest(33, 32), 2U);
    EXPECT_EQ(DataHandlerOf91095::CalcRequest(33, 0), 0U);
}

TEST(DeviceDataHandler, operand_records_select_cube_and_vector_subblocks)
{
    DataHandler handler;
    handler.operandRecords_.resize(6);
    for (size_t i = 0; i < handler.operandRecords_.size(); ++i) {
        handler.operandRecords_[i].simdMap[OperandType::DATA_S32].instructions = i + 1;
    }
    auto instructions = [&](uint16_t block, const std::string &subblock, const std::string &type) {
        return handler.GetOperandRecordMap(block, subblock, type).simdMap[OperandType::DATA_S32].instructions;
    };
    EXPECT_EQ(instructions(0, "cube0", Common::OpType::CUBE), 3U);
    EXPECT_EQ(instructions(1, "vector0", Common::OpType::MIX), 4U);
    EXPECT_EQ(instructions(1, "vector1", Common::OpType::MIX), 5U);
    EXPECT_EQ(instructions(3, "vector0", Common::OpType::VECTOR), 5U);
    EXPECT_EQ(instructions(10, "cube0", Common::OpType::CUBE), 0U);
}

TEST(DeviceDataHandler, profile_file_order_is_numeric_with_lexical_ties)
{
    FileNameCompare less;
    EXPECT_TRUE(less("DeviceProf2.bin", "DeviceProf10.bin"));
    EXPECT_FALSE(less("DeviceProf10.bin", "DeviceProf2.bin"));
    EXPECT_TRUE(less("DeviceProf2.a", "DeviceProf2.b"));
    EXPECT_FALSE(less("DeviceProf2.bin", "DeviceProf2.bin"));
    EXPECT_TRUE(less("a", "b"));
    EXPECT_EQ(FileNameCompare::extractNumberFromFileName("DeviceProf123.bin"), 123);
}

TEST(DeviceDataHandler, basic_info_parses_flags_and_rejects_invalid_numeric_fields)
{
    DataHandler handler;
    const std::string path = "test/ut/resources/coverage_basic_info.txt";
    EXPECT_FALSE(handler.ParseBasicInfoFile(path));
    ASSERT_TRUE(WriteFileByStream(path,
        "malformed\nIs MC2=1\nIs Lccl=1\nHas Simt=1\nRun Soc Version=Ascend950\n"
        "Block Dim=4\nCurrent Freq=1650\nRated Freq=1800\n"));
    EXPECT_TRUE(handler.ParseBasicInfoFile(path));
    EXPECT_TRUE(handler.GetMC2Flag());
    EXPECT_TRUE(handler.GetLcclFlag());
    EXPECT_TRUE(handler.GetHasSimt());
    EXPECT_EQ(handler.blockDim_, 4U);
    EXPECT_EQ(handler.curFreq_, 1650);
    EXPECT_EQ(handler.socVersion_, "Ascend950");
    ASSERT_TRUE(WriteFileByStream(path, "Block Dim=invalid\nCurrent Freq=invalid\nRated Freq=invalid\n"));
    EXPECT_TRUE(handler.ParseBasicInfoFile(path));
    EXPECT_EQ(handler.blockDim_, 4U);
    EXPECT_EQ(handler.curFreq_, -1);
    EXPECT_EQ(std::remove(path.c_str()), 0);
}

TEST(DeviceDataHandler, operand_binary_validates_size_and_accumulates_threads)
{
    DataHandler handler;
    const std::string path = "test/ut/resources/coverage_operand.bin";
    handler.ParseOperandRecords(path);
    ASSERT_TRUE(WriteFileByStream(path, "bad"));
    handler.ParseOperandRecords(path);
    EXPECT_TRUE(handler.operandRecords_.empty());
    std::vector<char> data(sizeof(OperandHeader) + sizeof(BlockOperandRecords), 0);
    auto block = MakeUnique<BlockOperandRecords>();
    *block = {};
    block->simdRecord.records[0].instructions = 7;
    block->simdRecord.records[0].operands = 14;
    block->simtRecord[0].records[0].instructions = 3;
    block->simtRecord[1].records[0].instructions = 5;
    ASSERT_EQ(memcpy_s(data.data() + sizeof(OperandHeader), sizeof(BlockOperandRecords),
                      block.get(), sizeof(BlockOperandRecords)), EOK);
    WriteBinaryFile(path, data.data(), data.size());
    handler.ParseOperandRecords(path);
    ASSERT_EQ(handler.operandRecords_.size(), 1U);
    auto type = static_cast<OperandType>(0);
    EXPECT_EQ(handler.operandRecords_[0].simdMap.at(type).instructions, 7U);
    EXPECT_EQ(handler.operandRecords_[0].simdMap.at(type).operands, 14U);
    EXPECT_EQ(handler.operandRecords_[0].simtMap.at(type).instructions, 8U);
    const uint64_t offset = 0x1234;
    WriteBinaryFile(path, reinterpret_cast<const char *>(&offset), sizeof(offset));
    handler.ParsePcSamplingRecords(path);
    EXPECT_EQ(handler.pcOffsetByPcSampling_, offset);
    EXPECT_EQ(std::remove(path.c_str()), 0);
    handler.ParsePcSamplingRecords(path);
}

template<typename Record>
void CheckDmaPadding()
{
    DBIParser parser("test/ut/resources");
    Record record {};
    record.src = 1024;
    record.dst = 2048;
    record.nBurst = 2;
    record.srcStride = 1;
    record.dstStride = 1;
    MemRecord memory {};
    const std::vector<uint64_t> scales = {32, 16, 8, 4, 2, 8, 4, 2};
    for (size_t i = 0; i < scales.size(); ++i) {
        record.padMode = static_cast<PadMode>(i + 1);
        EXPECT_EQ(parser.CalcPadModeSize(record), 2 * MOV_LOCAL_BLOCK_SIZE / scales[i]);
        EXPECT_TRUE(parser.ParseDmaMovWithPadMode(0, record, memory));
        if (i < 5) {
            EXPECT_EQ(memory.srcAddr, 1024U);
            EXPECT_EQ(memory.srcMemSize, 2 * MOV_LOCAL_BLOCK_SIZE / scales[i]);
            EXPECT_EQ(memory.dstMemSize, MOV_LOCAL_BLOCK_SIZE);
        } else {
            EXPECT_EQ(memory.dstAddr, 2048U);
            EXPECT_EQ(memory.dstMemSize, 2 * MOV_LOCAL_BLOCK_SIZE / scales[i]);
            EXPECT_EQ(memory.srcMemSize, MOV_LOCAL_BLOCK_SIZE);
        }
    }
    record.padMode = static_cast<PadMode>(0);
    EXPECT_EQ(parser.CalcPadModeSize(record), 2 * MOV_LOCAL_BLOCK_SIZE);
    EXPECT_FALSE(parser.ParseDmaMovWithPadMode(0, record, memory));
    EXPECT_EQ(parser.memoryChartMetrics_[0].invalidParam, 1U);
}

TEST(DeviceDataHandler, dma_and_mov_v2_padding_modes_preserve_transfer_sizes)
{
    CheckDmaPadding<DmaMovRecord>();
    CheckDmaPadding<MovV2Record>();
}

TEST(HotSpotBoundary, source_and_instruction_json_distinguish_sampling_and_cache_metrics)
{
    GlobalMockObject::verify();
    MOCKER(GetProductTypeBySocVersion).stubs().will(returnValue(ChipProductType::ASCEND950_SERIES));
    CodeLine line;
    line.line = 12;
    line.pcSampling = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    line.samplingPercentAllSample = 0.5f;
    nlohmann::json json;
    line.ToJson("test", json);
    EXPECT_EQ(json["Line"], 12);
    EXPECT_EQ(json["Process Bytes"], -1);
    EXPECT_EQ(json["Stall Sampling(All Samples)"]["Details"]["Active"], 9);
    EXPECT_EQ(json["Stall Sampling(Not Issue)"]["Details"].size(), 8U);
    InstrInfo instr;
    instr.addr = "0x10";
    instr.processBytes = 64;
    instr.pcSampling = line.pcSampling;
    nlohmann::json instrJson;
    instr.ToJson("test", instrJson);
    EXPECT_EQ(instrJson["Address"], "0x10");
    EXPECT_EQ(instrJson["Process Bytes"], 64);
    EXPECT_TRUE(instrJson.contains("Stall Sampling(All Samples)"));
    GlobalMockObject::verify();
    MOCKER(GetProductTypeBySocVersion).stubs().will(returnValue(ChipProductType::ASCEND910B_SERIES));
    line.l2cacheHitRate = 75.0f;
    instr.l2cacheHitRate = 50.0f;
    json.clear();
    instrJson.clear();
    line.ToJson("test", json);
    instr.ToJson("test", instrJson);
    EXPECT_EQ(json["L2Cache Hit Rate"], 75.0f);
    EXPECT_EQ(instrJson["L2Cache Hit Rate"], 50.0f);
    EXPECT_FALSE(json.contains("Stall Sampling(All Samples)"));
    GlobalMockObject::verify();
}

TEST(HotSpotBoundary, extraction_allows_sampling_without_bbcount_but_requires_it_for_source)
{
    GlobalMockObject::verify();
    HotSpotFunctionGenerator sampling({"", "", 0, true, true, false});
    MOCKER(&HotSpotFunctionGenerator::ProcessBBCount).stubs().will(returnValue(false));
    EXPECT_TRUE(sampling.ExtractData("test/ut/resources/missing_hotspot"));
    EXPECT_FALSE(sampling.sourceEnable_);
    EXPECT_EQ(sampling.startPc_, 0U);
    EXPECT_EQ(sampling.startPcForPcSampling_, 0U);
    HotSpotFunctionGenerator source({"", "", 0, true, false, false});
    EXPECT_FALSE(source.ExtractData("test/ut/resources/missing_hotspot"));
    EXPECT_TRUE(source.IsObjKernel("any"));
    EXPECT_TRUE(source.IsObjKernelPcAddr(0));
    GlobalMockObject::verify();
}

TEST(HotSpotBoundary, kernel_filter_includes_both_endpoints_and_excludes_unrelated_kernels)
{
    HotSpotFunctionGenerator generator({"", "kernel", 0, true, false, false});
    generator.beginAddr_["kernel"] = 16;
    generator.endAddr_["kernel"] = 32;
    EXPECT_TRUE(generator.IsObjKernel("kernel"));
    EXPECT_FALSE(generator.IsObjKernel("other"));
    EXPECT_TRUE(generator.IsObjKernelPcAddr(16));
    EXPECT_TRUE(generator.IsObjKernelPcAddr(32));
    EXPECT_FALSE(generator.IsObjKernelPcAddr(15));
    EXPECT_FALSE(generator.IsObjKernelPcAddr(33));
}

TEST(HotSpotBoundary, pipeline_propagates_extraction_calculation_and_visualization_failures)
{
    GlobalMockObject::verify();
    HotSpotFunctionGenerator generator({"", "", 0, true, false, false});
    MOCKER(&HotSpotFunctionGenerator::ExtractData).stubs().will(returnValue(false));
    EXPECT_FALSE(generator.Process("unused", {}, nullptr));
    GlobalMockObject::verify();
    MOCKER(&HotSpotFunctionGenerator::ExtractData).stubs().will(returnValue(true));
    MOCKER(&HotSpotFunctionGenerator::CalculateData).stubs().will(returnValue(false));
    EXPECT_FALSE(generator.Process("unused", {}, nullptr));
    GlobalMockObject::verify();
    MOCKER(&HotSpotFunctionGenerator::ExtractData).stubs().will(returnValue(true));
    MOCKER(&HotSpotFunctionGenerator::CalculateData).stubs().will(returnValue(true));
    MOCKER(&HotSpotFunctionGenerator::VisualizeData).stubs().will(returnValue(false));
    EXPECT_FALSE(generator.Process("unused", {}, nullptr));
    GlobalMockObject::verify();
}

TEST(HotSpotBoundary, calculation_propagates_encoding_and_symbolization_failures)
{
    GlobalMockObject::verify();
    HotSpotFunctionGenerator generator({"", "", 0, true, true, true});
    std::map<std::string, std::vector<Encoding>> lines;
    MOCKER(&HotSpotFunctionGenerator::ProcessEncoding).stubs().will(returnValue(false));
    EXPECT_FALSE(generator.CalculateData("unused", {}, nullptr, lines));
    GlobalMockObject::verify();
    MOCKER(&HotSpotFunctionGenerator::ProcessEncoding).stubs().will(returnValue(true));
    MOCKER(&HotSpotFunctionGenerator::UpdatePcSampling).stubs();
    MOCKER(&HotSpotFunctionGenerator::UpdateProcessBytes).stubs();
    MOCKER(&HotSpotFunctionGenerator::GenLine2Encodings).stubs().will(returnValue(false));
    EXPECT_FALSE(generator.CalculateData("unused", {}, nullptr, lines));
    GlobalMockObject::verify();
}

TEST(DeviceDataHandler, dbi_parameters_select_the_correct_mixed_subblock)
{
    DataHandlerOf91095 handler;
    handler.memMapDetail_.resize(1);
    auto &detail = handler.memMapDetail_[0];
    detail.apiDataTransVolumeVec0["UB_TO_GM_DATA"] = 32;
    detail.apiDataTransVolumeVec1["UB_TO_GM_DATA"] = 64;
    detail.ApiDataTransVolume_["UB_TO_GM_DATA"] = 96;
    handler.isAic_ = true;
    handler.isAiv_ = true;
    CalculateParams params {};
    handler.PrepareDbiParams(params, 0, "vector0");
    EXPECT_TRUE(params.hasDbiUBToGMData);
    EXPECT_EQ(params.dbiUBToGMData, 32U);
    handler.PrepareDbiParams(params, 0, "vector1");
    EXPECT_EQ(params.dbiUBToGMData, 64U);
    handler.isAiv_ = false;
    handler.PrepareDbiParams(params, 0, "cube0");
    EXPECT_EQ(params.dbiUBToGMData, 96U);
    detail.ApiDataTransVolume_.clear();
    params = {};
    handler.PrepareDbiParams(params, 0, "cube0");
    EXPECT_FALSE(params.hasDbiUBToGMData);
}

TEST(DeviceDataHandler, gm_transfer_statistics_round_requests_and_keep_blocks_separate)
{
    DataHandlerOf910B handler;
    handler.memMapDetail_.resize(2);
    handler.memMapDetail_[0].blockId = 0;
    handler.memMapDetail_[1].blockId = 1;
    std::vector<MemRecord> records;
    for (auto destination : {MemType::L0A, MemType::L0B, MemType::L1}) {
        MemRecord record {};
        record.src = MemType::GM;
        record.dst = destination;
        record.srcMemSize = 129;
        record.blockId = 0;
        records.push_back(record);
    }
    auto extra = records[0];
    extra.srcMemSize = 1;
    records.push_back(extra);
    std::map<uint64_t, std::map<std::string, uint64_t>> sizes;
    handler.Statics(records, sizes);
    EXPECT_EQ(sizes[0][GM_TO_L0A_DATA], 130U);
    EXPECT_EQ(sizes[0][GM_TO_L0B_DATA], 129U);
    EXPECT_EQ(sizes[0][GM_TO_L1_DATA], 129U);
    EXPECT_EQ(sizes[1][GM_TO_L0A_DATA], 0U);
    EXPECT_GT(handler.memMapDetail_[0].ApiDataTransVolume_[GM_TO_L0A], 1U);
}

namespace {
bool MockSymbolizerBoundaryLines(SymbolizerParser *parser, std::string, std::vector<std::string>)
{
    parser->offset2Lines_ = {{"10", "source.cpp", 12, "kernel", {}},
        {"10", "source.cpp", 12, "kernel", {}}, {"14", "source.cpp", 13, "kernel", {}},
        {"18", "ascend-toolkit/internal.cpp", 1, "internal", {}}};
    return true;
}
}

TEST(HotSpotBoundary, symbolization_deduplicates_source_lines_and_accumulates_block_calls)
{
    GlobalMockObject::verify();
    HotSpotFunctionGenerator generator({"", "", 0, true, false, false});
    generator.encodings_[0x10] = {};
    generator.encodings_[0x14] = {};
    generator.bbCalls_[0x10] = {0x14, 7};
    MOCKER(&SymbolizerParser::Parse).stubs().will(invoke(MockSymbolizerBoundaryLines));
    std::unordered_map<uint64_t, std::vector<std::string>> lines;
    EXPECT_TRUE(generator.GenAddr2Lines("unused", {}, lines));
    EXPECT_EQ(lines[0x10], (std::vector<std::string>{"source.cpp:12"}));
    EXPECT_EQ(generator.fileLineCalls_["source.cpp"][12], 7U);
    EXPECT_EQ(generator.encodings_[0x14].calls, 7U);
    EXPECT_EQ(generator.encodings_[0x10].cceCode, "source.cpp:12");
    generator.bbCalls_.clear();
    lines.clear();
    EXPECT_TRUE(generator.GenAddr2Lines("unused", {}, lines));
    EXPECT_EQ(generator.encodings_[0x10].calls, 0U);
    generator.encodings_.clear();
    lines.clear();
    EXPECT_FALSE(generator.GenAddr2Lines("unused", {}, lines));
    GlobalMockObject::verify();
}

namespace {
bool MockRegisterStream(TlvParser *parser)
{
    parser->pcNumCount_ = {{0x10, 8}, {0x20, 0}, {0x30, 4}};
    return true;
}
}

TEST(HotSpotBoundary, register_generation_updates_matching_instructions_and_propagates_failures)
{
    GlobalMockObject::verify();
    HotSpotFunctionGenerator generator({"", "", 0, true, false, false});
    MOCKER(&Utility::CmdExecute).stubs().will(returnValue(false));
    EXPECT_FALSE(generator.GenTlvdata("unused", "unused"));
    GlobalMockObject::verify();
    MOCKER(&Utility::CmdExecute).stubs().will(returnValue(true));
    MOCKER(&::chmod).stubs().will(returnValue(-1));
    EXPECT_FALSE(generator.GenTlvdata("unused", "unused"));
    GlobalMockObject::verify();
    MOCKER(&Utility::CmdExecute).stubs().will(returnValue(true));
    MOCKER(&::chmod).stubs().will(returnValue(0));
    MOCKER(&TlvParser::ParseStream).stubs().will(returnValue(false));
    EXPECT_FALSE(generator.GenTlvdata("unused", "unused"));
    GlobalMockObject::verify();
    MOCKER(&Utility::CmdExecute).stubs().will(returnValue(true));
    MOCKER(&::chmod).stubs().will(returnValue(0));
    MOCKER(&TlvParser::ParseStream).stubs().will(invoke(MockRegisterStream));
    MOCKER(&TlvParser::CountPCNum).stubs().will(returnValue(true));
    generator.encodings_[0x10] = {};
    generator.encodings_[0x20] = {};
    EXPECT_TRUE(generator.GenTlvdata("unused", "unused"));
    EXPECT_EQ(generator.encodings_[0x10].gprCount, 8U);
    EXPECT_EQ(generator.encodings_[0x20].gprCount, 0U);
    EXPECT_EQ(generator.encodings_.count(0x30), 0U);
    GlobalMockObject::verify();
}

TEST(HotSpotBoundary, visualization_stops_when_source_or_instruction_generation_fails)
{
    GlobalMockObject::verify();
    HotSpotFunctionGenerator generator({"", "", 0, true, false, false});
    std::map<std::string, std::vector<Encoding>> lines;
    MOCKER(&HotSpotFunctionGenerator::GenCodeFiles).stubs().will(returnValue(false));
    EXPECT_FALSE(generator.VisualizeData("unused", lines));
    GlobalMockObject::verify();
    MOCKER(&HotSpotFunctionGenerator::GenCodeFiles).stubs().will(returnValue(true));
    MOCKER(&HotSpotFunctionGenerator::GenInstrInfos).stubs().will(returnValue(false));
    EXPECT_FALSE(generator.VisualizeData("unused", lines));
    GlobalMockObject::verify();
}

TEST(DeviceDataHandler, duration_310p_uses_start_and_end_record_times)
{
    GlobalMockObject::verify();
    DataHandlerOf310P handler;
    std::vector<char> bytes(HWTS_LENGTH * 2, 0);
    MOCKER(&HwtsBean::GetTimeType).stubs()
        .will(returnValue(Common::TimeType::START)).then(returnValue(Common::TimeType::END));
    MOCKER(&HwtsBean::GetSystemTime).stubs().will(returnValue(uint64_t{100})).then(returnValue(uint64_t{200}));
    uint64_t start = 0;
    uint64_t end = 0;
    handler.ParseDurationBin("", bytes, bytes.size(), start, end);
    EXPECT_EQ(start, 100U);
    EXPECT_EQ(end, 200U);
    GlobalMockObject::verify();
}

TEST(DeviceDataParse, Parse_data_expect_failed)
{
    GlobalMockObject::verify();
    ProfArgs args;
    PmuEventsId pmuEventsId;
    ProfMetricsAbilityConfig metrics;
    args.argConfig = "test.json";
    DeviceDataParse deviceDataParse(ChipType::ASCEND910B, pmuEventsId, metrics);
    ASSERT_FALSE(deviceDataParse.ParseExactKernelData(""));
}

TEST(DeviceDataParse, Parse_data_expect_failed_with_wrong_chip)
{
    GlobalMockObject::verify();
    ProfArgs config;
    PmuEventsId pmuEventsId;
    ProfMetricsAbilityConfig metrics;
    DeviceDataParse deviceDataParse(ChipType::ASCEND910A, pmuEventsId, metrics);
    ASSERT_FALSE(deviceDataParse.ParseExactKernelData(""));
}

TEST(DeviceDataParse, Parse_data_basicinfo_expect_success_910B)
{
    PmuEventsId pmuEventsId;
    ProfMetricsAbilityConfig metrics;
    metrics.isBasicInfo = true;
    DeviceDataParse deviceDataParse(ChipType::ASCEND910B, pmuEventsId, metrics);
    ASSERT_FALSE(deviceDataParse.ParseExactKernelData(""));
}

/**
 * | 测试函数 | DeviceDataParse::ParseExactKernelData
 * | 测试场景 | 910B开启occupancy、roofline和内存详情，基本信息加载与保存均mock为成功。
 * | 预期结果 | 使用910B测试数据解析成功，ParseExactKernelData返回true。
 */
TEST(DeviceDataParse, parse_910b_with_occupancy_roofline_and_memory_detail_expect_success) {
    GlobalMockObject::verify();
    MOCKER(&DataHandler::LoadOpBasicInfoTxtFile)
        .stubs()
        .will(returnValue(true));
    MOCKER(&DataHandler::SaveOpBasicInfo)
            .stubs()
            .will(returnValue(true));
    ProfArgs config;
    PmuEventsId pmuEventsId;
    ProfMetricsAbilityConfig metricsConfig;
    config.argOutput = "test/ut/resources/op_profiling/device910B";
    metricsConfig.occupancyEnable = true;
    metricsConfig.roofline = true;
    metricsConfig.isMemoryDetail = true;
    pmuEventsId.LoadPmuVec(metricsConfig, ChipType::ASCEND910B);
    DeviceDataParse deviceDataParse(ChipType::ASCEND910B, pmuEventsId, metricsConfig);

    deviceDataParse.ParserInit();
    ASSERT_TRUE(deviceDataParse.ParseExactKernelData(config.argOutput));
}

TEST(DeviceDataParse, Parse_data_expect_success_310P)
{
    GlobalMockObject::verify();
    MOCKER(&DataHandler::LoadOpBasicInfoTxtFile)
        .stubs()
        .will(returnValue(true));
    MOCKER(&DataHandler::SaveOpBasicInfo)
            .stubs()
            .will(returnValue(true));
    ProfArgs config;
    ProfMetricsAbilityConfig metricsConfig;
    PmuEventsId pmuEventsId;
    config.argOutput = "test/ut/resources/op_profiling/device310P";
    metricsConfig.roofline = true;
    pmuEventsId.LoadPmuVec(metricsConfig, ChipType::ASCEND310P);
    DeviceDataParse deviceDataParse(ChipType::ASCEND310P, pmuEventsId, metricsConfig);
    deviceDataParse.ParserInit();
    ASSERT_TRUE(deviceDataParse.ParseExactKernelData(config.argOutput));
}

TEST(DeviceDataParse, Parse_data_expect_success_A5)
{
    GlobalMockObject::verify();
    MOCKER(&RuntimeHelper::GetSocVersion).stubs().will(returnValue(std::string("Ascend950PR_9599")));
    MOCKER(&DataHandler::LoadOpBasicInfoTxtFile)
            .stubs()
            .will(returnValue(true));
    MOCKER(&DataHandler::SaveOpBasicInfo)
            .stubs()
            .will(returnValue(true));
    ProfArgs config;
    PmuEventsId pmuEventsId;
    ProfMetricsAbilityConfig metricsConfig;
    config.argOutput = "test/ut/resources/op_profiling/deviceA5";
    metricsConfig.occupancyEnable = true;
    pmuEventsId.LoadPmuVec(metricsConfig, ChipType::ASCEND950);
    DeviceDataParse deviceDataParse(ChipType::ASCEND950, pmuEventsId, metricsConfig);

    deviceDataParse.ParserInit();
    ASSERT_TRUE(deviceDataParse.ParseExactKernelData(config.argOutput));
}

TEST(DeviceDataParse, Execute_expect_success_with_one_kernel)
{
    GlobalMockObject::verify();
    MOCKER(&DeviceDataParse::ParseExactKernelData)
            .stubs()
            .will(returnValue(true));
    ProfMetricsAbilityConfig metricsConfig;
    PmuEventsId pmuEventsId;
    DeviceDataParse deviceDataParse(ChipType::ASCEND910B, pmuEventsId, metricsConfig);
    std::string testDir = "test/ut/resources/op_profiling/OPPO";
    auto dumpDir = JoinPath({testDir, "device0/add/0/dump"});
    MkdirRecusively(dumpDir);
    ASSERT_TRUE(deviceDataParse.Execute(testDir));
    deviceDataParse.SingleKernelOutputReorganize(testDir);
    std::string oneKernelDir = "test/ut/resources/op_profiling/OPPO/dump";
    ASSERT_TRUE(IsExist(oneKernelDir));
    std::experimental::filesystem::remove_all(testDir);
}

/*
 * |  用例集  | DeviceDataParse
 * | 测试函数 | DeviceDataParse::ParseKernelTasks
 * | 测试场景 | 提交4个目录解析任务，mock每个任务均解析失败。
 * | 预期结果 | ParseKernelTasks返回后失败数为4，待解析任务队列为空。
 */
TEST(DeviceDataParse, parse_four_failed_tasks_expect_four_failures_and_empty_queue) {
    GlobalMockObject::verify();
    MOCKER(&DeviceDataParse::ParseExactKernelData).stubs().will(returnValue(false));
    DeviceDataParse parser(ChipType::ASCEND910B, PmuEventsId{}, ProfMetricsAbilityConfig{});
    parser.kernelParseTasks_ = {{"add/0", "add", "device0"}, {"abs/0", "abs", "device0"}, {"add/1", "add", "device1"},
        {"abs/1", "abs", "device1"}};
    parser.ParseKernelTasks();
    EXPECT_EQ(parser.failedKernelNum_, 4);
    EXPECT_TRUE(parser.kernelParseTasks_.empty());
    GlobalMockObject::verify();
}

/*
 * |  用例集  | DeviceDataParse
 * | 测试函数 | DeviceDataParse::ParseKernelTasks
 * | 测试场景 | 仅提交1个目录解析任务，mock解析失败。
 * | 预期结果 | 解析函数恰好调用1次，失败数为1，待解析任务队列为空。
 */
TEST(DeviceDataParse, parse_single_failed_task_expect_one_call_one_failure_and_empty_queue) {
    GlobalMockObject::verify();
    MOCKER(&DeviceDataParse::ParseExactKernelData).expects(exactly(1)).will(returnValue(false));
    DeviceDataParse parser(ChipType::ASCEND910B, PmuEventsId{}, ProfMetricsAbilityConfig{});
    parser.kernelParseTasks_ = {{"add/0", "add", "device0"}};
    parser.ParseKernelTasks();
    EXPECT_EQ(parser.failedKernelNum_, 1);
    EXPECT_TRUE(parser.kernelParseTasks_.empty());
    GlobalMockObject::verify();
}

/*
 * |  用例集  | DeviceDataParse
 * | 测试函数 | DeviceDataParse::ParseKernelTasks
 * | 测试场景 | 提交4个真实目录任务，其中2个含完整BasicInfo/PMU数据，另外2个缺少输入。
 * | 预期结果 | 仅缺少输入的2个任务失败，最终失败数为2，待解析任务队列为空。
 */
TEST(DeviceDataParse, parse_two_valid_and_two_missing_inputs_expect_two_failures_and_empty_queue) {
    GlobalMockObject::verify();
    // 并发下mockcpp的共享返回序列会被多个任务非原子地消费导致随机失败，这里改用真实解析：
    // 两份完整BasicInfo/PMU输入成功，两份缺失输入失败，验证恰好统计2个失败并清空队列。
    const std::string root = "test/ut/resources/parallel_partial_failures";
    RemoveAll(root);
    DeviceDataParse parser(ChipType::ASCEND910B, PmuEventsId{}, ProfMetricsAbilityConfig(false));
    ASSERT_TRUE(parser.ParserInit());
    for (size_t i = 0; i < 4; ++i) {
        const auto path = JoinPath({root, std::to_string(i)});
        const auto dump = JoinPath({path, "dump"});
        ASSERT_TRUE(MkdirRecusively(dump));
        if (i % 2 == 0) {
            std::ofstream metadata(JoinPath({dump, "op_basic_info.txt"}));
            metadata << "Device Id=0\nOp Name=add\nRun Soc Version=Ascend910B4\n"
                     << "Block Dim=1\nCurrent Freq=1000\nRated Freq=1000\nPid=1\n";
            metadata.close();
            std::ifstream source("test/ut/resources/op_profiling/device910B/dump/DeviceProf1.bin", std::ios::binary);
            ASSERT_TRUE(source.is_open());
            std::ofstream destination(JoinPath({dump, "DeviceProf1.bin"}), std::ios::binary);
            destination << source.rdbuf();
            ASSERT_TRUE(destination.good());
        }
        parser.kernelParseTasks_.push_back({path, i % 2 == 0 ? "add" : "abs", "device0"});
    }
    parser.ParseKernelTasks();
    EXPECT_EQ(parser.failedKernelNum_, 2);
    EXPECT_TRUE(parser.kernelParseTasks_.empty());
    RemoveAll(root);
    GlobalMockObject::verify();
}

TEST(DeviceDataParse, Execute_expect_success_with_one_device)
{
    GlobalMockObject::verify();
    MOCKER(&DeviceDataParse::ParseExactKernelData)
            .stubs()
            .will(returnValue(true));
    ProfMetricsAbilityConfig metricsConfig;
    PmuEventsId pmuEventsId;
    DeviceDataParse deviceDataParse(ChipType::ASCEND910B, pmuEventsId, metricsConfig);
    std::string testDir = "test/ut/resources/op_profiling/OPPO";
    MkdirRecusively(JoinPath({testDir, "device0/add/0/dump"}));
    MkdirRecusively(JoinPath({testDir, "device0/sub/0/dump"}));
    ASSERT_TRUE(deviceDataParse.Execute(testDir));
    deviceDataParse.SingleKernelOutputReorganize(testDir);
    ASSERT_TRUE(IsExist("test/ut/resources/op_profiling/OPPO/add"));
    ASSERT_TRUE(IsExist("test/ut/resources/op_profiling/OPPO/sub"));
    std::experimental::filesystem::remove_all(testDir);
}

/**
 * | 测试函数 | DeviceDataParse::Execute
 * | 测试场景 | 950开启指令时间线，同一算子有2次launch，仅第1次含DFX日志和region map；mock解析成功。
 * | 预期结果 | Execute返回true，第2次launch的dump目录也包含dfx_tune.log和dfx_region_map.txt。
 */
TEST(DeviceDataParse, execute_950_two_launches_expect_dfx_files_copied_to_second_launch) {
    GlobalMockObject::verify();
    MOCKER(&DeviceDataParse::ParseExactKernelData).stubs().will(returnValue(true));
    ProfMetricsAbilityConfig metricsConfig;
    metricsConfig.instrTimelineEnable = true;
    PmuEventsId pmuEventsId;
    DeviceDataParse deviceDataParse(ChipType::ASCEND950, pmuEventsId, metricsConfig);
    std::string testDir = "test/ut/resources/op_profiling/OPPO";
    std::string firstDump = JoinPath({testDir, "device0/add/0/dump"});
    std::string secondDump = JoinPath({testDir, "device0/add/1/dump"});
    MkdirRecusively(firstDump);
    MkdirRecusively(secondDump);
    std::ofstream tuneFile(JoinPath({firstDump, "dfx_tune.log"}).c_str(), std::ios::out | std::ios::binary);
    tuneFile << "test tune log\n";
    tuneFile.close();
    std::ofstream mapFile(JoinPath({firstDump, "dfx_region_map.txt"}).c_str(), std::ios::out | std::ios::binary);
    mapFile << "region_id=1\n";
    mapFile.close();
    ASSERT_TRUE(deviceDataParse.Execute(testDir));
    ASSERT_TRUE(IsExist(JoinPath({secondDump, "dfx_tune.log"})));
    ASSERT_TRUE(IsExist(JoinPath({secondDump, "dfx_region_map.txt"})));
    std::experimental::filesystem::remove_all(testDir);
}

TEST(DeviceDataParse, GetRangeFreq_expert_NA_when_get_req_failed)
{
    std::string testDir = "test/ut/resources/op_profiling/OPPO";
    std::string path = JoinPath({testDir, "tmp_dump/device0/21211/0"});
    MkdirRecusively(path);
    ProfMetricsAbilityConfig metricsConfig;
    PmuEventsId pmuEventsId;
    DeviceDataParse deviceDataParse(ChipType::ASCEND910B, pmuEventsId, metricsConfig);
    vector<string> freqs1;
    deviceDataParse.GetRangeFreq(path, freqs1);
    ASSERT_EQ(freqs1[0], "Current Freq=NA");
    ASSERT_EQ(freqs1[1], "Rated Freq=NA");

    string freqTxt = JoinPath({testDir, "tmp_dump/device0/21211/0", "freq.txt"});
    std::ofstream outFile(freqTxt.c_str(), std::ios::out | std::ios::binary);
    outFile << "-1" << "\n";
    outFile.close();
    chmod(freqTxt.c_str(), SAVE_DATA_FILE_AUTHORITY);
    vector<string> freqs2;
    deviceDataParse.GetRangeFreq(path, freqs2);
    ASSERT_EQ(freqs2[0], "Current Freq=NA");
    ASSERT_EQ(freqs2[1], "Rated Freq=NA");
    std::experimental::filesystem::remove_all(testDir);
}

TEST(DeviceDataParse, GetRangeFreq_expert_value_when_get_req_success)
{
    std::string testDir = "test/ut/resources/op_profiling/OPPO";
    std::string path = JoinPath({testDir, "tmp_dump/device0/21211/0"});
    MkdirRecusively(path);
    ProfMetricsAbilityConfig metricsConfig;
    PmuEventsId pmuEventsId;
    DeviceDataParse deviceDataParse(ChipType::ASCEND910B, pmuEventsId, metricsConfig);
    string freqTxt = JoinPath({testDir, "tmp_dump/device0/21211/0", "freq.txt"});
    std::ofstream outFile(freqTxt.c_str(), std::ios::out | std::ios::binary);
    outFile << "Current Freq=800" << "\n";
    outFile << "Rated Freq=1850" << "\n";
    outFile.close();
    chmod(freqTxt.c_str(), SAVE_DATA_FILE_AUTHORITY);
    vector<string> freqs;
    deviceDataParse.GetRangeFreq(path, freqs);
    ASSERT_EQ(freqs[0], "Current Freq=800");
    ASSERT_EQ(freqs[1], "Rated Freq=1850");
    std::experimental::filesystem::remove_all(testDir);
}

TEST(DeviceDataParse, ParseSingleRangeData_expert_generate_bin_success)
{
    std::string testDir = "test/ut/resources/op_profiling/OPPO";
    std::string addDir = JoinPath({testDir, "device0/add/0/dump"});
    std::string tmpDir = JoinPath({testDir, "tmp_dump/device0/21211/0"});
    MkdirRecusively(addDir);
    MkdirRecusively(tmpDir);
    string outputTxt = JoinPath({tmpDir, "output.txt"});
    std::ofstream outFile(outputTxt.c_str(), std::ios::out | std::ios::binary);
    outFile << addDir << "\n";
    outFile << "-1" << "\n";
    outFile.close();
    chmod(outputTxt.c_str(), SAVE_DATA_FILE_AUTHORITY);
    std::experimental::filesystem::copy("test/ut/resources/op_profiling/device910B/dump/duration.bin", JoinPath({tmpDir, "duration.bin"}));
    for (int i = 1; i <= 6; i++) {
        string file = "DeviceProf" + to_string(i) + ".bin";
        std::experimental::filesystem::copy("test/ut/resources/op_profiling/device910B/dump/" + file, JoinPath({tmpDir, file}));
    }
    ProfMetricsAbilityConfig metricsConfig;
    PmuEventsId pmuEventsId;
    DeviceDataParse deviceDataParse(ChipType::ASCEND910B, pmuEventsId, metricsConfig);
    deviceDataParse.ParseSingleRangeData(tmpDir);
    ASSERT_TRUE(IsExist(JoinPath({addDir, "duration.bin"})));
    for (int i = 1; i <= 6; i++) {
        string file = "DeviceProf" + to_string(i) + ".bin";
        ASSERT_TRUE(IsExist(JoinPath({addDir, file})));
    }
    std::experimental::filesystem::remove_all(testDir);
}

TEST(DeviceDataParse, ParseSingleRangeData_expert_generate_basic_info_bin_success)
{
    std::string testDir = "test/ut/resources/op_profiling/OPPO";
    std::string addDir = JoinPath({testDir, "device0/add/0/dump"});
    std::string tmpDir = JoinPath({testDir, "tmp_dump/device0/21211/0"});
    MkdirRecusively(addDir);
    MkdirRecusively(tmpDir);
    string outputTxt = JoinPath({tmpDir, "output.txt"});
    std::ofstream outFile(outputTxt.c_str(), std::ios::out | std::ios::binary);
    outFile << addDir << "\n";
    outFile.close();
    chmod(outputTxt.c_str(), SAVE_DATA_FILE_AUTHORITY);
    std::experimental::filesystem::copy("test/ut/resources/op_profiling/device910B/dump/duration.bin", JoinPath({tmpDir, "duration.bin"}));
    std::experimental::filesystem::copy("test/ut/resources/op_profiling/device910B/dump/DeviceProf1.bin", JoinPath({tmpDir, "DeviceProf1.bin"}));
    ProfMetricsAbilityConfig metricsConfig;
    metricsConfig.isBasicInfo = true;
    PmuEventsId pmuEventsId;
    DeviceDataParse deviceDataParse(ChipType::ASCEND910B, pmuEventsId, metricsConfig);
    deviceDataParse.ParseSingleRangeData(tmpDir);
    ASSERT_TRUE(IsExist(JoinPath({addDir, "duration.bin"})));
    ASSERT_TRUE(IsExist(JoinPath({addDir, "DeviceProf1.bin"})));
    std::experimental::filesystem::remove_all(testDir);
}

TEST(DeviceDataParse, ParseSingleRangeData_expert_generate_bin_fail)
{
    std::string testDir = "test/ut/resources/op_profiling/OPPO";
    std::string addDir = JoinPath({testDir, "device0/add/0/dump"});
    std::string tmpDir = JoinPath({testDir, "tmp_dump/device0/21211/0"});
    MkdirRecusively(addDir);
    MkdirRecusively(tmpDir);
    std::experimental::filesystem::copy("test/ut/resources/op_profiling/device910B/dump/duration.bin", JoinPath({tmpDir, "duration.bin"}));
    for (int i = 1; i <= 6; i++) {
        string file = "DeviceProf" + to_string(i) + ".bin";
        std::experimental::filesystem::copy("test/ut/resources/op_profiling/device910B/dump/" + file, JoinPath({tmpDir, file}));
    }
    ProfMetricsAbilityConfig metricsConfig;
    PmuEventsId pmuEventsId;
    DeviceDataParse deviceDataParse(ChipType::ASCEND910B, pmuEventsId, metricsConfig);
    deviceDataParse.ParseSingleRangeData(tmpDir);
    ASSERT_FALSE(IsExist(JoinPath({addDir, "duration.bin"})));
    for (int i = 1; i <= 6; i++) {
        string file = "DeviceProf" + to_string(i) + ".bin";
        ASSERT_FALSE(IsExist(JoinPath({addDir, file})));
    }
    std::experimental::filesystem::remove_all(testDir);
}

TEST(DeviceDataParse, ParseTmpDump_expert_generate_bin_success)
{
    std::string testDir = "test/ut/resources/op_profiling/OPPO";
    std::string addDir = JoinPath({testDir, "device0/add/0/dump"});
    std::string subDir = JoinPath({testDir, "device1/sub/0/dump"});
    std::string tmp0Dir = JoinPath({testDir, "tmp_dump/device0/21211/0"});
    std::string tmp1Dir = JoinPath({testDir, "tmp_dump/device1/21212/0"});
    MkdirRecusively(addDir);
    MkdirRecusively(subDir);
    MkdirRecusively(tmp0Dir);
    MkdirRecusively(tmp1Dir);
    string outputTxt = JoinPath({tmp0Dir, "output.txt"});
    std::ofstream outFile0(outputTxt.c_str(), std::ios::out | std::ios::binary);
    outFile0 << addDir << "\n";
    outFile0.close();
    chmod(outputTxt.c_str(), SAVE_DATA_FILE_AUTHORITY);
    outputTxt = JoinPath({tmp1Dir, "output.txt"});
    std::ofstream outFile1(outputTxt.c_str(), std::ios::out | std::ios::binary);
    outFile1 << subDir << "\n";
    outFile1.close();
    chmod(outputTxt.c_str(), SAVE_DATA_FILE_AUTHORITY);
    std::experimental::filesystem::copy("test/ut/resources/op_profiling/device910B/dump/duration.bin", JoinPath({tmp0Dir, "duration.bin"}));
    std::experimental::filesystem::copy("test/ut/resources/op_profiling/device910B/dump/duration.bin", JoinPath({tmp1Dir, "duration.bin"}));
    for (int i = 1; i <= 6; i++) {
        string file = "DeviceProf" + to_string(i) + ".bin";
        std::experimental::filesystem::copy("test/ut/resources/op_profiling/device910B/dump/" + file, JoinPath({tmp0Dir, file}));
        std::experimental::filesystem::copy("test/ut/resources/op_profiling/device910B/dump/" + file, JoinPath({tmp1Dir, file}));
    }
    ProfMetricsAbilityConfig metricsConfig;
    PmuEventsId pmuEventsId;
    DeviceDataParse deviceDataParse(ChipType::ASCEND910B, pmuEventsId, metricsConfig);
    deviceDataParse.ParseTmpDump(JoinPath({testDir, "tmp_dump"}));
    ASSERT_TRUE(IsExist(JoinPath({addDir, "duration.bin"})));
    ASSERT_TRUE(IsExist(JoinPath({subDir, "duration.bin"})));
    for (int i = 1; i <= 6; i++) {
        string file = "DeviceProf" + to_string(i) + ".bin";
        ASSERT_TRUE(IsExist(JoinPath({addDir, file})));
        ASSERT_TRUE(IsExist(JoinPath({subDir, file})));
    }
    std::experimental::filesystem::remove_all(testDir);
}

TEST(DeviceTask, Run_binary_expect_success)
{
    GlobalMockObject::verify();
    MOCKER(&OpRunner::RunOpBinary)
        .stubs()
        .will(returnValue(true));
    MOCKER(&DeviceTask::PreProcess)
        .stubs()
        .will(returnValue(true));
    ProfArgs args;
    OpDeviceProf opDeviceProf(args);
    DeviceTask deviceTask{"DeviceProf", opDeviceProf};
    deviceTask.replayCount_ = 1;
    deviceTask.inExitMode = false;
    ASSERT_TRUE(deviceTask.Run());
}

TEST(DeviceTask, Run_kernel_expect_success)
{
    GlobalMockObject::verify();
    MOCKER(&DeviceTask::PreProcess)
            .stubs()
            .will(returnValue(true));
    CaseConfig caseConfig;
    std::vector<CaseConfig> caseConfigs = {caseConfig};
    MOCKER(&ParseRunConfigJson)
            .stubs()
            .will(returnValue(caseConfigs));
    MOCKER(&OpRunner::RunOpBinary)
            .stubs()
            .will(returnValue(true));
    ProfArgs args;
    OpDeviceProf opDeviceProf(args);
    DeviceTask deviceTask{"DeviceProf", opDeviceProf};
    deviceTask.replayCount_ = 1;
    deviceTask.inExitMode = false;
    deviceTask.opRunMode = OpRunnerMode::RUN_KERNEL;
    ASSERT_TRUE(deviceTask.Run());
}

TEST(DeviceTask, PreProcess_expect_success)
{
    GlobalMockObject::verify();
    MOCKER(&IsExist)
        .stubs()
        .will(returnValue(false));
    MOCKER(&MkdirRecusively)
        .stubs()
        .will(returnValue(true));
    ProfArgs args;
    args.argOutput = "./";
    OpDeviceProf opDeviceProf(args);
    DeviceTask deviceTask{"DeviceProf", opDeviceProf};
    deviceTask.opRunMode = OpRunnerMode::EXECUTE_BINARY;
    deviceTask.metrics_ = ProfMetricsAbilityConfig(false);
    deviceTask.metrics_[ProfMetrics::ARITHMETIC_UTILIZATION].isOn = true;
    deviceTask.pmuValue_.LoadPmuVec(deviceTask.metrics_, ChipType::ASCEND910B);
    ASSERT_TRUE(deviceTask.PreProcess());
    ASSERT_EQ(deviceTask.profMessage_.aicPmu[2], 1280);
    ASSERT_EQ(deviceTask.profMessage_.aivPmu[11], 1288);
    ASSERT_EQ(deviceTask.profMessage_.l2CachePmu[0], 0);
}

TEST(DeviceTask, PreProcess_mkdir_output_directory_failed)
{
    GlobalMockObject::verify();
    MOCKER(&IsExist)
            .stubs()
            .will(returnValue(false));
    MOCKER(&MkdirRecusively)
            .stubs()
            .will(returnValue(false));
    ProfArgs args;
    args.argOutput = "testOutputPath";
    OpDeviceProf opDeviceProf(args);
    DeviceTask deviceTask{"DeviceProf", opDeviceProf};
    ASSERT_FALSE(deviceTask.PreProcess());
}

TEST(DeviceTask, PreProcess_output_not_directory)
{
    GlobalMockObject::verify();
    MOCKER(&IsExist)
            .stubs()
            .will(returnValue(true));
    MOCKER(&IsDir)
            .stubs()
            .will(returnValue(false));
    ProfArgs args;
    args.argOutput = "testOutputPath";
    OpDeviceProf opDeviceProf(args);
    DeviceTask deviceTask{"DeviceProf", opDeviceProf};
    ASSERT_FALSE(deviceTask.PreProcess());
}

TEST(DeviceTask, Check_events_for_910B)
{
    GlobalMockObject::verify();
    for (const auto &pair:AIC_EVENTS_FOR_910B) {
        for (uint16_t e: pair.second) {
            ASSERT_EQ(std::count(REPLAY_AIC_EVENTS_FOR_910B.begin(), REPLAY_AIC_EVENTS_FOR_910B.end(), e), 1);
        }
    }
    for (const auto &pair:AIV_EVENTS_FOR_910B) {
        for (uint16_t e: pair.second) {
            ASSERT_EQ(std::count(REPLAY_AIV_EVENTS_FOR_910B.begin(), REPLAY_AIV_EVENTS_FOR_910B.end(), e), 1);
        }
    }
}

/**
 * |  用例集  | DeviceTask
 * | 测试函数 | DeviceTask::GetReplayTimes
 * |  用例名  | test_GetReplayTimes_success_in_kernel_replay_mode
 * | 用例描述 | 测试kernel重放模式，返回正确的replay次数
 */
TEST(DeviceTask, test_GetReplayTimes_success_in_kernel_replay_mode)
{
    GlobalMockObject::verify();
    ProfArgs args;
    args.argReplayMode = "kernel";
    OpDeviceProf opDeviceProf(args);
    DeviceTask deviceTask{"DeviceProf", opDeviceProf};
    ASSERT_EQ(deviceTask.GetReplayTimes(), 1);
}

/**
 * |  用例集  | DeviceTask
 * | 测试函数 | DeviceTask::GetReplayTimes
 * |  用例名  | test_GetReplayTimes_success_in_application_replay_mode_910B
 * | 用例描述 | 测试application重放模式，910B芯片部分插桩，返回正确的replay次数
 */
TEST(DeviceTask, test_GetReplayTimes_success_in_application_replay_mode_910B)
{
    GlobalMockObject::verify();
    MOCKER(&Common::HalHelper::GetPlatformType)
        .stubs()
        .will(returnValue(Common::ChipType::ASCEND910B));
    ProfMetricsAbilityConfig metrics;
    metrics.isMemoryDetail = true;
    metrics.pcSamplingEnable = true;
    ProfArgs args;
    args.argReplayMode = "application";
    args.argAicMetrics = metrics;
    OpDeviceProf opDeviceProf(args);
    DeviceTask deviceTask{"DeviceProf", opDeviceProf};
    ASSERT_EQ(deviceTask.GetReplayTimes(), 12);
}

/**
 * |  用例集  | DeviceTask
 * | 测试函数 | DeviceTask::GetReplayTimes
 * |  用例名  | test_GetReplayTimes_success_in_application_replay_mode_950
 * | 用例描述 | 测试application重放模式，950芯片全部插桩，返回正确的replay次数
 */
TEST(DeviceTask, test_GetReplayTimes_success_in_application_replay_mode_950)
{
    GlobalMockObject::verify();
    MOCKER(&Common::HalHelper::GetPlatformType)
        .stubs()
        .will(returnValue(Common::ChipType::ASCEND950));
    ProfMetricsAbilityConfig metrics;
    metrics.isMemoryDetail = true;
    metrics.pcSamplingEnable = true;
    metrics.pipeTimelineEnable = true;
    metrics.instrTimelineEnable = true;
    metrics.isSource = true;
    ProfArgs args;
    args.argReplayMode = "application";
    args.argAicMetrics = metrics;
    OpDeviceProf opDeviceProf(args);
    DeviceTask deviceTask{"DeviceProf", opDeviceProf};
    ASSERT_EQ(deviceTask.GetReplayTimes(), 13);
}

/**
 * |  用例集  | DeviceTask
 * | 测试函数 | DeviceTask::BuildEnabledDbiTypes
 * |  用例名  | test_BuildEnabledDbiTypes_success_on_950_without_pmu
 * | 用例描述 | 测试950芯片未使能PMU时，只包含WarpTimeline插桩
 */
TEST(DeviceTask, test_BuildEnabledDbiTypes_success_on_950_without_pmu) {
    GlobalMockObject::verify();
    MOCKER(&Common::HalHelper::GetPlatformType).stubs().will(returnValue(Common::ChipType::ASCEND950));
    ProfArgs args;
    args.argAicMetrics = ProfMetricsAbilityConfig(false);
    OpDeviceProf opDeviceProf(args);
    DeviceTask deviceTask{"DeviceProf", opDeviceProf};
    std::vector<ProfDBIType> expected = {ProfDBIType::WARP_TIMELINE};
    ASSERT_EQ(deviceTask.BuildEnabledDbiTypes(), expected);
}

/**
 * |  用例集  | DeviceTask
 * | 测试函数 | DeviceTask::BuildEnabledDbiTypes
 * |  用例名  | test_BuildEnabledDbiTypes_success_on_950_with_default_pmu
 * | 用例描述 | 测试950芯片使用默认PMU配置时，包含OperandRecord和WarpTimeline插桩
 */
TEST(DeviceTask, test_BuildEnabledDbiTypes_success_on_950_with_default_pmu) {
    GlobalMockObject::verify();
    MOCKER(&Common::HalHelper::GetPlatformType).stubs().will(returnValue(Common::ChipType::ASCEND950));
    ProfArgs args;
    OpDeviceProf opDeviceProf(args);
    DeviceTask deviceTask{"DeviceProf", opDeviceProf};
    std::vector<ProfDBIType> expected = {ProfDBIType::OPERAND_RECORD, ProfDBIType::WARP_TIMELINE};
    ASSERT_EQ(deviceTask.BuildEnabledDbiTypes(), expected);
}

/**
 * |  用例集  | DeviceTask
 * | 测试函数 | DeviceTask::BuildEnabledDbiTypes
 * |  用例名  | test_BuildEnabledDbiTypes_success_on_950_with_all_metrics
 * | 用例描述 | 测试950芯片使能全部指标时，返回全部插桩类型列表
 */
TEST(DeviceTask, test_BuildEnabledDbiTypes_success_on_950_with_all_metrics) {
    GlobalMockObject::verify();
    MOCKER(&Common::HalHelper::GetPlatformType).stubs().will(returnValue(Common::ChipType::ASCEND950));
    ProfMetricsAbilityConfig metrics;
    metrics.isMemoryDetail = true;
    metrics.pcSamplingEnable = true;
    metrics.pipeTimelineEnable = true;
    metrics.instrTimelineEnable = true;
    metrics.isSource = true;
    ProfArgs args;
    args.argAicMetrics = metrics;
    OpDeviceProf opDeviceProf(args);
    DeviceTask deviceTask{"DeviceProf", opDeviceProf};
    std::vector<ProfDBIType> expected = {ProfDBIType::OPERAND_RECORD, ProfDBIType::MEMORY_CHART,
        ProfDBIType::INSTR_PROF_START, ProfDBIType::INSTR_PROF_END, ProfDBIType::INSTR_PROF_DFX, ProfDBIType::BB_COUNT,
        ProfDBIType::WARP_TIMELINE};
    ASSERT_EQ(deviceTask.BuildEnabledDbiTypes(), expected);
}

TEST(DeviceDataParse, Execute_Success)
{
    std::string sourceDir = "test/ut/resources/op_profiling/device910B";
    std::string testDir = "test/ut/resources/op_profiling/OPPO";
    auto kerneldir = JoinPath({testDir, "device0/add/0"});
    auto tmpdumpDir = JoinPath({testDir, "dump"});
    MkdirRecusively(kerneldir);
    char destination[PATH_MAX + 1];
    realpath(kerneldir.c_str(), destination);
    char source[PATH_MAX + 1];
    realpath(sourceDir.c_str(), source);
    CopyFolder(source, destination, destination, nullptr);

    ProfArgs  config;
    ProfMetricsAbilityConfig metricsConfig;
    config.argOutput = testDir;
    PmuEventsId pmuEventsId;
    pmuEventsId.LoadPmuVec(metricsConfig, ChipType::ASCEND910B);
    DeviceDataParse deviceDataParse(ChipType::ASCEND910B, pmuEventsId, metricsConfig);
    ASSERT_TRUE(deviceDataParse.Execute(testDir));
    deviceDataParse.SingleKernelOutputReorganize(config.argOutput);

    std::vector<std::string> fileNames;
    GetFileNames(testDir, fileNames);
    for (const auto &file : fileNames) {
        std::string filePath = JoinPath({config.argOutput, file});
        if (file == "dump" || file == "visualize_data.bin" || file.find("bbb") != string::npos) {
            continue;
        }
        std::regex pattern("[A-Za-z0-9_]*.csv");
        ASSERT_TRUE(std::regex_match(file, pattern));
    }
    std::experimental::filesystem::remove_all(testDir);
}

TEST(DeviceDataParse, bean_test)
{
    GlobalMockObject::verify();
    vector<uint16_t> events = {1, 2, 3, 4, 5, 6, 7, 8};

    vector<char> fftsBin(FFTS_LENGTH * 2);
    ReadBinaryFile("test/ut/resources/op_profiling/device910B/dump/DeviceProf1.bin", fftsBin);
    vector<char> fftsBinData{&fftsBin[0], &fftsBin[0] + FFTS_LENGTH};
    FftsBlockBean fftsBean(ChipProductType::ASCEND910B_SERIES, fftsBinData);
    SplitBlockPmuData fftsData = fftsBean.GetBlockData(events, events);
    ASSERT_EQ(fftsData.totalCycles, 10000);
    ASSERT_EQ(fftsData.blockType, "vector");
    ASSERT_EQ(fftsData.pmuEventValueMap.at(1), 1001);

    vector<char> acsqBin(ACSQ_LENGTH * 2);
    ReadBinaryFile("test/ut/resources/op_profiling/device910B/dump/duration.bin", acsqBin);
    for (size_t i = 0; i < HWTS_LENGTH * 2; i = i + HWTS_LENGTH) {
        vector<char> acsqBinData{&acsqBin[i], &acsqBin[i] + ACSQ_LENGTH};
        AcsqBean acsqBean(ChipProductType::ASCEND910B_SERIES, acsqBinData);
        uint64_t time = acsqBean.GetSystemTime();
        Common::TimeType type = acsqBean.GetTimeType();
        if (i == 0) {
            ASSERT_EQ(type, Common::TimeType::START);
            ASSERT_EQ(time, 30000);
        } else {
            ASSERT_EQ(type, Common::TimeType::END);
            ASSERT_EQ(time, 40000);
        }
    }

    vector<char> aiCoreBin(AICORE_LENGTH);
    ReadBinaryFile("test/ut/resources/op_profiling/device310P/dump/DeviceProf1.bin", aiCoreBin);
    vector<char> aiCoreBinData{&aiCoreBin[0], &aiCoreBin[0] + AICORE_LENGTH};
    AiCoreBean aiCoreBean(aiCoreBinData);
    SplitBlockPmuData aiCoreInfo = aiCoreBean.GetAiCoreData(events);
    ASSERT_EQ(aiCoreInfo.totalCycles, 50000);
    ASSERT_EQ(aiCoreInfo.pmuEventValueMap.at(1), 1001);

    vector<char> hwtsBin(HWTS_LENGTH * 2);
    ReadBinaryFile("test/ut/resources/op_profiling/device310P/dump/duration.bin", hwtsBin);
    for (size_t i = 0; i < HWTS_LENGTH * 2; i = i + HWTS_LENGTH) {
        vector<char> hwtsBinData{&hwtsBin[i], &hwtsBin[i] + HWTS_LENGTH};
        HwtsBean hwtsBean(hwtsBinData);
        uint64_t time = hwtsBean.GetSystemTime();
        Common::TimeType type = hwtsBean.GetTimeType();
        if (i == 0) {
            ASSERT_EQ(type, Common::TimeType::START);
            ASSERT_EQ(time, 60000);
        } else {
            ASSERT_EQ(type, Common::TimeType::END);
            ASSERT_EQ(time, 70000);
        }
    }

    vector<char> l2CacheBin(L2_CACHE_LENGTH);
    ReadBinaryFile("test/ut/resources/op_profiling/device310P/dump/L2Cache.bin", l2CacheBin);
    vector<char> l2CacheBinData{&l2CacheBin[0], &l2CacheBin[0] + L2_CACHE_LENGTH};
    L2CacheBean l2CacheBean(l2CacheBinData);
    SplitBlockPmuData l2CacheInfo = l2CacheBean.GetL2CacheData(events);
    ASSERT_EQ(aiCoreInfo.pmuEventValueMap.at(1), 1001);
}

TEST(DeviceDataParse, pmu_calculate_test)
{
    map<uint16_t, uint64_t> pmuEventValueMap = {
        {11, 100}, {4, 123}, {73, 456}, {1, 1000}, {8, 500}, {76, 100}, {77, 200}, {174, 300}, {49, 400}, {50, 750},
        {74, 44}, {19, 600}, {518, 700}, {524, 800}, {106, 102}, {120, 100}, {121, 1}, {10, 500}
    };
    set<std::string> metricItems = {"aic_total_cycles", "aic_mte1_ratio", "aic_mte1_instructions", "aic_vec_fp16_ratio",
        "aic_l1_read_bw(GB/s)", "aic_cube_time(us)", "aic_write_cache_hit", "aic_l2_cache_hit_rate(%)", "unknown"};
    Calculate cal(pmuEventValueMap, 10000, {ChipType::ASCEND310P, 1150, 8, 24, "Ascend310P3"});
    std::map<std::string, uint64_t> dbiMap;
    map<string, string> metricValues = CalMetricItems(cal, metricItems, FormulaFor310P, dbiMap);
    ASSERT_EQ(metricValues.at("aic_total_cycles"), "10000");
    ASSERT_EQ(metricValues.at("aic_mte1_ratio"), "0.010000");
    ASSERT_EQ(metricValues.at("aic_mte1_instructions"), "123");
    ASSERT_EQ(metricValues.at("aic_vec_fp16_ratio"), "0.030000");
    ASSERT_EQ(metricValues.at("aic_l1_read_bw(GB/s)"), "175.476074");
    ASSERT_EQ(metricValues.at("aic_cube_time(us)"), "0.054348");
    ASSERT_EQ(metricValues.at("aic_write_cache_hit"), "NA");
    ASSERT_EQ(metricValues.at("aic_l2_cache_hit_rate(%)"), "NA");
    ASSERT_EQ(metricValues.at("unknown"), "NA");

    set<std::string> metricItems910B = {"L1_to_GM_bw_usage_rate(%)(estimate)", "GM_to_L1_datas(KB)",
        "L1_to_GM_datas(KB)(estimate)", "L0C_to_L1_datas(KB)", "L0C_to_GM_datas(KB)", "GM_to_L1_bw_usage_rate(%)",
        "L0C_to_L1_bw_usage_rate(%)", "L0C_to_GM_bw_usage_rate(%)"};
    Calculate cal2(pmuEventValueMap, 10000, {ChipType::ASCEND910B, 1650, 20, 24, "Ascend910B4"});
    map<string, string> metricValues2 = CalMetricItems(cal2, metricItems910B, FormulaFor910B, dbiMap);
    ASSERT_EQ(metricValues2.at("L1_to_GM_bw_usage_rate(%)(estimate)"), "23.824282");
    ASSERT_EQ(metricValues2.at("GM_to_L1_datas(KB)"), "12.500000");
    ASSERT_EQ(metricValues2.at("L1_to_GM_datas(KB)(estimate)"), "287.500000");
    ASSERT_EQ(metricValues2.at("L0C_to_L1_datas(KB)"), "87.500000");
    ASSERT_EQ(metricValues2.at("L0C_to_GM_datas(KB)"), "12.500000");
    ASSERT_EQ(metricValues2.at("GM_to_L1_bw_usage_rate(%)"), "0.886015");
    ASSERT_EQ(metricValues2.at("L0C_to_L1_bw_usage_rate(%)"), "7.220070");
    ASSERT_EQ(metricValues2.at("L0C_to_GM_bw_usage_rate(%)"), "1.031439");
}

TEST(DeviceDataParse, LoadOpBasicInfoTxtFile_expect_false)
{
    DataHandler dataHandler;
    std::string filePath = "test/ut/resources/op_profiling/device910B/dump/op_basic_info.txt";
    ASSERT_FALSE(dataHandler.LoadOpBasicInfoTxtFile(filePath));
}

TEST(DeviceDataParse, SaveOpBasicInfo_expect_false)
{
    DataHandler dataHandler;
    std::string filePath = "test/ut/resources/op_profiling/device910B/dump/op_basic_info.txt";
    ASSERT_FALSE(dataHandler.SaveOpBasicInfo(filePath));
}

TEST(DeviceDataParse, SaveOpBasicInfo_expect_true)
{
    DataHandler dataHandler;
    std::string filePath = "test/ut/resources/op_profiling/device910B/dump";
    ASSERT_TRUE(dataHandler.SaveOpBasicInfo(filePath));
}

TEST(DeviceDataParse, load2d_parse_success)
{
    DBIParser dbiParser("");
    std::string buffer(sizeof(Load2DRecord), 0);
    Load2DRecord record{};
    record.srcMemType = MemType::GM;
    record.dstMemType = MemType::L0A;
    record.repeat = 3;
    if (memcpy_s(&buffer[0], sizeof(Load2DRecord), &record, sizeof(Load2DRecord)) != EOK) {
        printf("memcpy_s failed\n");
    }
    std::size_t index = 0;
    ASSERT_TRUE(dbiParser.ParseLoad2dRecord(buffer, index, 0, 0));
}

TEST(DeviceDataParse, parse_dma_mov_record)
{
    DBIParser dbiParser("");
    std::string buffer(sizeof(DmaMovRecord), 0);
    DmaMovRecord record{};
    record.srcMemType = MemType::GM;
    record.dstMemType = MemType::L0A;
    record.nBurst = 3;
    if (memcpy_s(&buffer[0], sizeof(DmaMovRecord), &record, sizeof(DmaMovRecord)) != EOK) {
        printf("memcpy_s failed\n");
    }
    std::size_t index = 0;
    ASSERT_TRUE(dbiParser.ParseDmaMovRecord(buffer, index, 0, 0));
}

TEST(DeviceDataParse, parse_dma_mov_record_byte_mode)
{
    DBIParser dbiParser("");
    std::string buffer(sizeof(DmaMovRecord), 0);
    DmaMovRecord record{};
    record.srcMemType = MemType::GM;
    record.dstMemType = MemType::L0A;
    record.nBurst = 3;
    record.byteMode = ByteMode::BM_ENABLE;
    if (memcpy_s(&buffer[0], sizeof(DmaMovRecord), &record, sizeof(DmaMovRecord)) != EOK) {
        printf("memcpy_s failed\n");
    }
    std::size_t index = 0;
    ASSERT_TRUE(dbiParser.ParseDmaMovRecord(buffer, index, 0, 0));
}

TEST(DeviceDataParse, parse_dma_mov_record_pad_mode)
{
    DBIParser dbiParser("");
    std::string buffer(sizeof(DmaMovRecord), 0);
    DmaMovRecord record{};
    record.srcMemType = MemType::GM;
    record.dstMemType = MemType::L0A;
    record.nBurst = 3;
    record.padMode = PadMode::PAD_MODE1;
    if (memcpy_s(&buffer[0], sizeof(DmaMovRecord), &record, sizeof(DmaMovRecord)) != EOK) {
        printf("memcpy_s failed\n");
    }
    std::size_t index = 0;
    ASSERT_TRUE(dbiParser.ParseDmaMovRecord(buffer, index, 0, 0));
}

TEST(DeviceDataParse, parse_mov_align_record)
{
    DBIParser dbiParser("");
    std::string buffer(sizeof(MovAlignRecord), 0);
    MovAlignRecord record{};
    record.srcMemType = MemType::GM;
    record.dstMemType = MemType::L0A;
    record.nBurst = 3;
    if (memcpy_s(&buffer[0], sizeof(MovAlignRecord), &record, sizeof(MovAlignRecord)) != EOK) {
        printf("memcpy_s failed\n");
    }
    std::size_t index = 0;
    ASSERT_TRUE(dbiParser.ParseMovAlignRecord(buffer, index, 0, 0));
}

TEST(DeviceDataParse, parse_dma_mov_nd2nz_record)
{
    DBIParser dbiParser("");
    std::string buffer(sizeof(MovAlignRecord), 0);
    DmaMovNd2nzRecord record{};
    record.srcMemType = MemType::GM;
    record.dstMemType = MemType::L0A;
    record.ndNum = 3;
    if (memcpy_s(&buffer[0], sizeof(DmaMovNd2nzRecord), &record, sizeof(DmaMovNd2nzRecord)) != EOK) {
        printf("memcpy_s failed\n");
    }
    std::size_t index = 0;
    ASSERT_TRUE(dbiParser.ParseDmaMovNd2nzRecord(buffer, index, 0, 0));
}

TEST(DeviceDataParse, parse_mov_fp_record)
{
    DBIParser dbiParser("");
    std::string buffer(sizeof(MovFpRecord), 0);
    MovFpRecord record{};
    record.pc = 11188;
    record.dstStride = 1024.;
    record.srcStride = 128;
    record.nSize = 256;
    record.mSize = 128;
    record.ndNum = 1;
    record.quantPreBits = 32;
    record.enUnitFlag = true;
    record.enNZ2ND = true;
    if (memcpy_s(&buffer[0], sizeof(MovFpRecord), &record, sizeof(MovFpRecord)) != EOK) {
        printf("memcpy_s failed\n");
    }
    std::size_t index = 0;
    ASSERT_TRUE(dbiParser.ParseMovFpRecord(buffer, index, 0, 0));
}

TEST(DeviceDataParse, parse_packet)
{
    DBIParser dbiParser("");
    DBIDataHeader dataHeader{1, sizeof(MovFpRecord), 1, 0, 0};
    std::string buffer(sizeof(DBIDataHeader) + 1 + sizeof(MovFpRecord), 0);
    MovFpRecord record{};
    record.pc = 11188;
    record.dstStride = 1024.;
    record.srcStride = 128;
    record.nSize = 256;
    record.mSize = 128;
    record.ndNum = 1;
    record.quantPreBits = 32;
    record.enUnitFlag = true;
    record.enNZ2ND = true;
    if (memcpy_s(&buffer[0], sizeof(DBIDataHeader), &dataHeader, sizeof(DBIDataHeader)) != EOK) {
        printf("memcpy_s failed\n");
    }
    buffer[sizeof(DBIDataHeader)] = '/';
    if (memcpy_s(&buffer[sizeof(DBIDataHeader) + 1],
                 sizeof(MovFpRecord), &record, sizeof(MovFpRecord)) != EOK) {
        printf("memcpy_s failed\n");
    }
    dbiParser.ParsePacket(0, std::move(buffer));
}

TEST(DeviceDataParse, ParseMemoryChart_one_invalid_data) {
    DBIParser dbiParser("");
    dbiParser.ParseMemoryChart(0, "", {});
    DBIDataHeader dbiDataHeader{};
    dbiDataHeader.count = 1;
    RecordHeader rh;
    rh.recordType = RecordType::INVALID;
    string msg = Communication::Serialize(dbiDataHeader, rh);
    dbiParser.ParseMemoryChart(0, msg, dbiDataHeader);
    ASSERT_TRUE(dbiParser.memoryChartMetrics_[0].invalidDataCount != 0);
}

TEST(DeviceDataParse, ParseMemoryChart_memcpy_failed) {
    DBIParser dbiParser("");
    MOCKER(memcpy_s).stubs().will(returnValue(EOVERFLOW));
    DBIDataHeader dbiDataHeader{};
    dbiDataHeader.count = 1;
    string msg = Communication::Serialize(dbiDataHeader) + "123";
    dbiParser.ParseMemoryChart(0, msg, dbiDataHeader);
    ASSERT_TRUE(dbiParser.memoryChartMetrics_[0].memoryCopyFailed != 0);
    GlobalMockObject::verify();
}

TEST(DeviceDataParse, ParseMemoryChart_invalid_record_type) {
    DBIParser dbiParser("");
    MovFpRecord record{};
    DBIDataHeader dbiDataHeader{};
    dbiDataHeader.count = 1;
    RecordHeader rh;
    rh.recordType = RecordType::INVALID;
    string msg = Communication::Serialize(dbiDataHeader, rh, record);
    dbiParser.ParseMemoryChart(0, msg, dbiDataHeader);
    ASSERT_TRUE(dbiParser.memoryChartMetrics_[0].invalidRecordType != 0);
}

TEST(DeviceDataParse, ParseMemoryChart_one_record)
{
    DBIParser dbiParser("");
    MovFpRecord record{};
    record.pc = 11188;
    record.dstStride = 1024.;
    record.srcStride = 128;
    record.nSize = 256;
    record.mSize = 128;
    record.ndNum = 1;
    record.quantPreBits = 32;
    record.enUnitFlag = true;
    record.enNZ2ND = true;
    DBIDataHeader dbiDataHeader{};
    dbiDataHeader.count = 1;
    RecordHeader rh;
    rh.recordType = RecordType::MOV_FP;
    string msg = Communication::Serialize(dbiDataHeader, rh, record);
    dbiParser.ParseMemoryChart(0, msg, dbiDataHeader);
    ASSERT_TRUE(dbiParser.memoryChartMetrics_[0].typeProcessed[static_cast<uint32_t>(RecordType::MOV_FP)] != 0);
}

TEST(DeviceDataParse, L2CacheLoadAndStore)
{
    auto gm = std::make_shared<GM>("GM", 512);
    L2Cache cache{{"L2Cache", gm, gm, 4, 1, 512, CachePolicy::LRU, true, true}};
    CacheOpStat opStat{true};

    opStat = cache.Load({0, 1024});
    EXPECT_EQ(opStat.hit, 0);
    EXPECT_EQ(opStat.miss, 2);
    EXPECT_EQ(opStat.allocate, 2);
    EXPECT_EQ(opStat.evictAndWrite, 0);
    EXPECT_EQ(opStat.evictWithoutWrite, 0);

    opStat = cache.Store({1000, 32});
    EXPECT_EQ(opStat.hit, 1);
    EXPECT_EQ(opStat.miss, 1);
    EXPECT_EQ(opStat.allocate, 1);
    EXPECT_EQ(opStat.evictAndWrite, 0);
    EXPECT_EQ(opStat.evictWithoutWrite, 0);

    opStat = cache.Load({2000, 256});
    EXPECT_EQ(opStat.miss, 2);
    EXPECT_EQ(opStat.allocate, 2);

    opStat = cache.Load({5000, 256});
    EXPECT_EQ(opStat.miss, 2);
    EXPECT_EQ(opStat.allocate, 2);
    EXPECT_EQ(opStat.evictAndWrite, 2);

    opStat = cache.Store({2100, 32});
    EXPECT_EQ(opStat.hit, 1);

    opStat = cache.Store({2100, 32});
    EXPECT_EQ(opStat.hit, 1);

    opStat = cache.Load({1000, 3000});
    EXPECT_EQ(opStat.hit, 2);
    EXPECT_EQ(opStat.miss, 5);
    EXPECT_EQ(opStat.allocate, 5);
    EXPECT_EQ(opStat.evictAndWrite, 0);
    EXPECT_EQ(opStat.evictWithoutWrite, 5);
}

TEST(DataHandler, ParseDurationBin_910B)
{
    std::string outputPath = "test/ut/resources/op_profiling/device910B/dump";
    string timeFilePath = Utility::JoinPath({outputPath, "duration.bin"});
    size_t fileSize = GetFileSize(timeFilePath);
    vector<char> totalBin(fileSize);
    ReadBinaryFile(timeFilePath, totalBin);
    uint64_t startTime = 0;
    uint64_t endTime = 0;

    DataHandlerOf910B dataHandler;
    dataHandler.isMC2_ = true;
    dataHandler.ParseDurationBin(outputPath, totalBin, fileSize, startTime, endTime);
    EXPECT_EQ(startTime, 30000);
    EXPECT_EQ(endTime, 40000);
    EXPECT_EQ(dataHandler.minMc2TimeCyc_, 30000);
    auto taskInfo = dataHandler.acsqTimeMap_.at({0, 0});
    EXPECT_EQ(taskInfo.startTime, 30000);
    EXPECT_EQ(taskInfo.endTime, 40000);
    EXPECT_EQ(taskInfo.taskType, 0);
}

/**
 * |  用例集  | DataHandler
 * | 测试函数 | GetOperandRecordMap
 * |  用例名  | test_GetOperandRecordMap_when_mix_return_right
 * | 用例描述 | 测试mix算子获取插桩操作数顺序正确
 */
TEST(DataHandler, test_GetOperandRecordMap_when_mix_return_right)
{
    using namespace Visualize;
    DataHandler dataHandler;
    OperandRecord Record;
    Record.instructions = 10;
    OperandRecordMap map1 = {{OperandType::DATA_B4, Record}};
    OperandRecordMap map2 = {{OperandType::DATA_B8, Record}};
    OperandRecordMap map3 = {{OperandType::DATA_B16, Record}};
    std::vector<TypeOperandRecord> operandRecords = {{map1, map1}, {map2, map2}, {map3, map3}};
    dataHandler.operandRecords_ = operandRecords;
    auto vector0 = dataHandler.GetOperandRecordMap(0, "vector0", Common::OpType::MIX);
    ASSERT_EQ(vector0.simdMap.at(OperandType::DATA_B4).instructions, 10);
    auto vector1 = dataHandler.GetOperandRecordMap(0, "vector1", Common::OpType::MIX);
    ASSERT_EQ(vector1.simdMap.at(OperandType::DATA_B8).instructions, 10);
    auto cube0 = dataHandler.GetOperandRecordMap(0, "cube0", Common::OpType::MIX);
    ASSERT_EQ(cube0.simdMap.at(OperandType::DATA_B16).instructions, 10);
}

/**
 * |  用例集  | DataHandler
 * | 测试函数 | GetOperandRecordMap
 * |  用例名  | test_GetOperandRecordMap_when_not_mix_return_right
 * | 用例描述 | 测试非mix算子获取插桩操作数顺序正确
 */
TEST(DataHandler, test_GetOperandRecordMap_when_not_mix_return_right)
{
    using namespace Visualize;
    DataHandler dataHandler;
    OperandRecord Record;
    Record.instructions = 10;
    OperandRecordMap map1 = {{OperandType::DATA_B4, Record}};
    OperandRecordMap map2 = {{OperandType::DATA_B8, Record}};
    OperandRecordMap map3 = {{OperandType::DATA_B16, Record}};
    OperandRecordMap map4 = {{OperandType::DATA_S8, Record}};
    OperandRecordMap map5 = {{OperandType::DATA_S16, Record}};
    OperandRecordMap map6 = {{OperandType::DATA_S32, Record}};
    std::vector<TypeOperandRecord> operandRecords = {{map1, map1}, {map2, map2}, {map3, map3}, {map4, map4}, {map5, map5}, {map6, map6}};
    dataHandler.operandRecords_ = operandRecords;
    auto vector0 = dataHandler.GetOperandRecordMap(0, "vector0", Common::OpType::VECTOR);
    ASSERT_EQ(vector0.simdMap.at(OperandType::DATA_B4).instructions, 10);
    auto vector1 = dataHandler.GetOperandRecordMap(1, "vector0", Common::OpType::VECTOR);
    ASSERT_EQ(vector1.simdMap.at(OperandType::DATA_B8).instructions, 10);
    auto vector2 = dataHandler.GetOperandRecordMap(2, "vector0", Common::OpType::VECTOR);
    ASSERT_EQ(vector2.simdMap.at(OperandType::DATA_S8).instructions, 10);
    auto vector3 = dataHandler.GetOperandRecordMap(3, "vector0", Common::OpType::VECTOR);
    ASSERT_EQ(vector3.simdMap.at(OperandType::DATA_S16).instructions, 10);
    auto cube0 = dataHandler.GetOperandRecordMap(0, "cube0", Common::OpType::CUBE);
    ASSERT_EQ(cube0.simdMap.at(OperandType::DATA_B16).instructions, 10);
    auto cube1 = dataHandler.GetOperandRecordMap(1, "cube0", Common::OpType::CUBE);
    ASSERT_EQ(cube1.simdMap.at(OperandType::DATA_S32).instructions, 10);
}

TEST(DeviceDataParse, HotSpot_ProcessBBCount_expect_false)
{
    GlobalMockObject::verify();
    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend910B4", "", 0, true, false, false});
    ASSERT_FALSE(hotSpotFunctionGenerator.ProcessBBCount("not_exist_dir_path"));

    std::vector<std::string> filenames;
    MOCKER(&Utility::ListDir<decltype(std::back_inserter(filenames))>)
        .stubs()
        .will(returnValue(true));
    ASSERT_FALSE(hotSpotFunctionGenerator.ProcessBBCount("not_exist_dir_path"));
    GlobalMockObject::verify();
}

TEST(DeviceDataParse, HotSpot_ProcessBBCount_expect_true)
{
    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend910B4", "", 0, true, false, false});
    ASSERT_FALSE(hotSpotFunctionGenerator.ProcessBBCount("not_exist_dir_path"));
}

TEST(DeviceDataParse, HotSpot_UpdateBBBMap_expect_true)
{
    string path = "test/ut/resources/op_profiling/device910B/dump/kernel0Stub.o.bbbmap.0";
    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend910B4", "", 0, true, false, false});
    ASSERT_TRUE(hotSpotFunctionGenerator.UpdateBBBMap(path));
}

TEST(DeviceDataParse, HotSpot_UpdateBBBMap_expect_false)
{
    GlobalMockObject::verify();
    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend910B4", "", 0, true, false, false});
    ASSERT_FALSE(hotSpotFunctionGenerator.UpdateBBBMap("not_exist_dir_path"));
    MOCKER(Utility::IsReadable)
           .stubs()
           .will(returnValue(true));
    ASSERT_FALSE(hotSpotFunctionGenerator.UpdateBBBMap("not_exist_dir_path"));
    GlobalMockObject::verify();
}

TEST(DeviceDataParse, HotSpot_UpdateExtra_expect_true)
{
    string path = "test/ut/resources/op_profiling/device910B/dump/kernel0Stub.o.bbbmap.0";
    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend910B4", "", 0, true, false, false});
    ASSERT_TRUE(hotSpotFunctionGenerator.UpdateExtra(path));
}

TEST(DeviceDataParse, HotSpot_UpdateExtra_expect_false)
{
    GlobalMockObject::verify();
    string path = "test/ut/resources/op_profiling/device910B/dump/kernel0Stub.o.bbbmap.0";
    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend910B4", "", 0, true, false, false});
    ASSERT_FALSE(hotSpotFunctionGenerator.UpdateExtra("not_exist_dir_path"));

    MOCKER(Utility::IsWritable)
            .stubs()
            .will(returnValue(false));
    ASSERT_FALSE(hotSpotFunctionGenerator.UpdateExtra(path));
    GlobalMockObject::verify();
}

TEST(DeviceDataParse, HotSpot_GenL2cacheStat_expect_empty)
{
    GlobalMockObject::verify();
    std::vector<Common::MemRecord> memoryRecords;
    auto l2Cache1 = GetDefaultL2Cache("invalid_soc_version");
    l2Cache1->Modeling(memoryRecords);
    ASSERT_TRUE(l2Cache1->GetPcBasedCacheData().empty());
    auto l2Cache2 = GetDefaultL2Cache("Ascend910B4");
    l2Cache2->Modeling(memoryRecords);
    ASSERT_TRUE(l2Cache2->GetPcBasedCacheData().empty());
    auto l2Cache3 = GetDefaultL2Cache("Ascend310P3");
    l2Cache3->Modeling(memoryRecords);
    ASSERT_TRUE(l2Cache3->GetPcBasedCacheData().empty());
    GlobalMockObject::verify();
}

TEST(DeviceDataParse, HotSpot_GenFdata_expect_false)
{
    GlobalMockObject::verify();
    string path = "test/ut/resources/op_profiling/device910B/dump/kernel0Stub.o.bbbmap.0";
    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend910B4", "", 0, true, false, false});
    ASSERT_FALSE(hotSpotFunctionGenerator.GenFdata("", "", path));
    MOCKER(&Utility::CmdExecute)
            .stubs()
            .with(any())
            .will(returnValue(true));
    ASSERT_TRUE(hotSpotFunctionGenerator.GenFdata("", "", path));
    GlobalMockObject::verify();
}

TEST(DeviceDataParse, HotSpot_GenBBCalls_expect_true)
{
    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend910B4", "", 0, true, false, false});
    hotSpotFunctionGenerator.kernelStartAddr_["matmul_custom_0_mix_aic"] = 0;
    vector<string> fdata{"1 matmul_custom_0_mix_aic 0 1",
                         "1 matmul_custom_0_mix_aic 4c 1",
                         "1 matmul_custom_0_mix_aic 50 1",
                         "1 matmul_custom_0_mix_aic 58 0",
                         "1 matmul_custom_0_mix_aic 68 1"};
    ASSERT_TRUE(hotSpotFunctionGenerator.GenBBCalls(fdata));
}

TEST(DeviceDataParse, HotSpot_GenVisualizeData)
{
    string path = "test/ut/resources/op_profiling/device910B/";
    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend910B4", "", 0, true, false, false});
    vector<CodeFile> codeFile(1);
    codeFile[0].file = "some_file";
    codeFile[0].lines.resize(1);
    vector<InstrInfo> instrInfo(1);
    ASSERT_NO_THROW(hotSpotFunctionGenerator.GenVisualizeData(path, codeFile, instrInfo));
}

TEST(DeviceDataParse, HotSpot_GenCodeFiles_expect_true)
{
    GlobalMockObject::verify();
    string path = "test/ut/resources/op_profiling/device910B/";
    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend910B4", "", 0, true, false, false});
    map<string, vector<Encoding>> line2Encodings = {
        {"matmul_custom.cpp:53", {{17840, "SCALAR", "ADD.s64", "matmul_custom.cpp:53", 2, 0, 0},
                                  {17844, "SCALAR", "SUB.s64", "matmul_custom.cpp:53", 2, 0, 1}}},
        {"matmul_custom.cpp:91", {{632, "SCALAR", "INSERT", "matmul_custom.cpp:91", 1, 0, 0}}},
        {"matmul_custom.cpp:92", {{404, "SCALAR", "ADD.s64", "matmul_custom.cpp:92", 1, 0, 0},
                                  {616, "SCALAR", "LD.b32", "matmul_custom.cpp:92", 1, 0, 0}}},
    };
    vector<CodeFile> codeFiles;
    ASSERT_TRUE(hotSpotFunctionGenerator.GenCodeFiles(path, line2Encodings, codeFiles));
    MOCKER(&Utility::Visualize::CodeWriter::Write)
            .stubs();
    MOCKER(&string::empty)
            .stubs()
            .will(returnValue(false));
    ASSERT_TRUE(hotSpotFunctionGenerator.GenCodeFiles(path, line2Encodings, codeFiles));
    GlobalMockObject::verify();
}

TEST(DeviceDataParse, HotSpot_GenInstrInfos_expect_false)
{
    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend910B4", "", 0, true, false, false});
    vector<InstrInfo> instrInfos;
    ASSERT_FALSE(hotSpotFunctionGenerator.GenInstrInfos(instrInfos));
}

TEST(DeviceDataParse, HotSpot_GenInstrInfos_expect_true)
{
    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend910B4", "", 0, true, false, false});
    vector<InstrInfo> instrInfos;
    hotSpotFunctionGenerator.encodings_[632] = Encoding{632, "SCALAR", "INSERT", "", 1, 0, 0};
    ASSERT_TRUE(hotSpotFunctionGenerator.GenInstrInfos(instrInfos));
}

TEST(EquationsUtils, Ratio_expect_success)
{
    auto res1 = Ratio(100, 20);
    ASSERT_STREQ(res1.c_str(), "5.000000");
    auto res2 = Ratio(100, 0);
    ASSERT_STREQ(res2.c_str(), "NA");
}

TEST(EquationsUtils, BandWidth_expect_success)
{
    auto res1 = BandWidth(10, 10, 100);
    ASSERT_STREQ(res1.c_str(), "100000000.000000");
    auto res2 = BandWidth(10, 0, 1);
    ASSERT_STREQ(res2.c_str(), "NA");
}

TEST(EquationsUtils, BandWidthUsage_expect_success)
{
    auto res1 = BandWidthUsage(10, 0, TransportType::GM_TO_L1, ChipProductType::ASCEND950PR_9599);
    ASSERT_STREQ(res1.c_str(), "NA");
    auto res2 = BandWidthUsage(10, 1, TransportType::UNKNOWN, ChipProductType::ASCEND950PR_9599);
    ASSERT_STREQ(res2.c_str(), "NA");
    auto res3 = BandWidthUsage(0.0001, 1, TransportType::GM_TO_L1, ChipProductType::ASCEND950PR_9599);
    ASSERT_STREQ(res3.c_str(), "56.315811");
}

/**
 * | 测试函数 | GetMaxBwBySoc
 * | 测试场景 | Ascend910B1分别查询CJ与DEFAULT两种GM类型对应的带宽。
 * | 预期结果 | GM_TO_L1依次为296、264，MTE_TO_L0A依次为439.32、437.5。
 */
TEST(EquationsUtils, get_910b1_bandwidth_for_cj_and_default_gm_expect_matching_limits) {
    // Mock the queried type instead of the scratch field; GetGmType caches by device.
    MOCKER(&HalHelper::GetGmType).stubs().will(returnValue(GmType::CJ)).then(returnValue(GmType::DEFAULT));
    auto resCJ = GetMaxBwBySoc("Ascend910B1", ChipProductType::ASCEND910B1);
    EXPECT_FLOAT_EQ(resCJ[TransportType::GM_TO_L1], 296);
    EXPECT_FLOAT_EQ(resCJ[TransportType::MTE_TO_L0A], 439.32);
    auto res = GetMaxBwBySoc("Ascend910B1", ChipProductType::ASCEND910B1);
    EXPECT_FLOAT_EQ(res[TransportType::GM_TO_L1], 264);
    EXPECT_FLOAT_EQ(res[TransportType::MTE_TO_L0A], 437.5);
    GlobalMockObject::verify();
}

// A5正式出来后该函数需要修改使用真实数据校验
TEST(CalMetricItems, CalMetricItems_A5_success)
{
    std::map<uint16_t, uint64_t> pmuEventValueMap;
    for (const auto &pmu : REPLAY_AIC_EVENTS_FOR_A5) {
        pmuEventValueMap[pmu] = 1;
    }
    for (const auto &pmu : REPLAY_AIV_EVENTS_FOR_A5) {
        pmuEventValueMap[pmu] = 1;
    }
    pmuEventValueMap.erase(pmuEventValueMap.begin());
    PmuMap pmuMap(pmuEventValueMap);

    CalculateParams params;
    params.totalCycles = 1000;
    params.frequency = 1000;
    params.duration = 1;
    params.socVersion = "Ascend950PR_9599";
    params.pmuMap = pmuMap;
    set<std::string> metricItems;
    for (const auto &temp : MetricHeaderForA5) {
        metricItems.insert(temp.second.begin(), temp.second.end());
    }
    auto res = CalMetricItems(params, metricItems, FormulaForA5);
    ASSERT_STREQ(res["aic_total_cycles"].c_str(), "1000");
    ASSERT_STREQ(res["aic_cube_total_instr_number"].c_str(), "1");
}

/**
/* | 用例集 | DeviceDataParse
/* |测试函数| DataHandlerOf91095::ParseMemoryChartData
/* | 用例名 | a5_execute_func_when_pcStart_not_found_and_expect_no_throw
/* |用例描述| 执行测试函数，不抛异常
*/
TEST(DeviceDataParse, a5_execute_func_when_pcStart_not_found_and_expect_no_throw)
{
    DataHandlerOf91095 dataHandlerA5;
    dataHandlerA5.soc_ = "Ascend950PR_9599";
    string outputPath;
    ProfMetricsAbilityConfig metrics;
    metrics.pcSamplingEnable = true;
    vector<MemRecord> memoryRecords;
    ASSERT_NO_THROW(dataHandlerA5.ParseMemoryChartData(outputPath, metrics, memoryRecords, ""));
}

/**
/* | 用例集 | DeviceDataParse
/* |测试函数| ProcessEncoding
/* | 用例名 | process_encoding_when_soc_is_a5_and_expect_success
/* |用例描述| 执行测试函数，soc 为a5时，返回预期结果
*/
TEST(DeviceDataParse, process_encoding_when_soc_is_a5_and_expect_success)
{
    GlobalMockObject::verify();
    string kernelPath;
    string soc = "Ascend950PR_9599";
    std::shared_ptr<L2Cache> l2CachePtr;
    std::vector<Encode::EncodingInfo> instrEncodingVec = {
        {{0, 0}, 0, EncodingType::BIT32, "PIPEA", "NAMEA"}, {{1, 0}, 1, EncodingType::BIT32, "PIPEB", "NAMEB"},
        {{2, 0}, 2, EncodingType::BIT32, "PIPEC", "NAMEC"}
    };
    HotSpotFunctionGenerator hotSpotFunctionGenerator({soc, "", 0, false, true, false});
    MOCKER(&Encode::InstrEncoding::GenerateEncoding)
            .stubs()
            .with(any(), outBound(instrEncodingVec))
            .will(returnValue(true));
    ASSERT_TRUE(hotSpotFunctionGenerator.ProcessEncoding(kernelPath, l2CachePtr));
    ASSERT_EQ(hotSpotFunctionGenerator.encodings_.size(), 3);
    ASSERT_EQ(hotSpotFunctionGenerator.encodings_[0].addr, 0);
    ASSERT_EQ(hotSpotFunctionGenerator.encodings_[1].pipe, "PIPEB");
    ASSERT_EQ(hotSpotFunctionGenerator.encodings_[2].source, "NAMEC");
    GlobalMockObject::verify();
}

/**
/* | 用例集 | DeviceDataParse
/* |测试函数| UpdatePcSampling
/* | 用例名 | process_update_pcsampling_given_bin_and_expect_success
/* |用例描述| 测试单个pcSampling文件解析后，指令采样和top stall统计结果正确
*/
TEST(DeviceDataParse, process_update_pcsampling_given_bin_and_expect_success)
{
    GlobalMockObject::verify();

    string soc = "Ascend950PR_9599";
    const string dir = "test/ut/resources/op_profiling/instr_prof_single";
    RemoveAll(dir);
    ASSERT_TRUE(MkdirRecusively(dir));
    const string filePath = JoinPath({dir, "pcSampling.bin.0"});
    std::ofstream file(filePath, std::ios::out | std::ios::binary);
    ASSERT_TRUE(file.is_open());
    InstrProfHeadInfo headInfo;
    headInfo.coreId = 0;
    headInfo.coreType = 1;
    headInfo.validLen = static_cast<uint32_t>(TEST_PC_SAMPLING_DATA_LEN * 3);
    WritePcSamplingChunk(file, headInfo, {
        BuildPcSamplingRecord(0x05, {1, 2, 3, 4, 5, 6, 7, 8, 9}),
        BuildPcSamplingRecord(0x08, {2, 3, 4, 5, 6, 7, 8, 9, 10}),
        BuildPcSamplingRecord(0x01, {3, 4, 5, 6, 7, 8, 9, 10, 11})
    });
    file.close();

    HotSpotFunctionGenerator hotSpotFunctionGenerator({soc, "", 0, false, true, false});
    hotSpotFunctionGenerator.startPc_ = 0;
    hotSpotFunctionGenerator.encodings_[0x28] = {0x28, "PIPEA", "NAMEA", "", 0, 0, 0, 0};  // 0x05 * 8 = 0x28
    hotSpotFunctionGenerator.encodings_[0x40] = {0x40, "PIPEB", "NAMEB", "", 0, 0, 0, 0};  // 0x08 * 8 = 0x40
    hotSpotFunctionGenerator.encodings_[0x08] = {0x08, "PIPEC", "NAMEC", "", 0, 0, 0, 0};  // 0x01 * 8 = 0x08
    hotSpotFunctionGenerator.UpdatePcSampling(dir);
    ASSERT_EQ(hotSpotFunctionGenerator.encodings_[0x28].pcSampling.size(), 9);
    ASSERT_EQ(hotSpotFunctionGenerator.encodings_[0x40].pcSampling.size(), 9);
    ASSERT_EQ(hotSpotFunctionGenerator.encodings_[0x08].pcSampling.size(), 9);
    ASSERT_EQ(hotSpotFunctionGenerator.encodings_[0x28].pcSampling[0], 1);
    ASSERT_EQ(hotSpotFunctionGenerator.encodings_[0x28].pcSampling[8], 9);
    ASSERT_EQ(hotSpotFunctionGenerator.encodings_[0x40].pcSampling[0], 2);
    ASSERT_EQ(hotSpotFunctionGenerator.encodings_[0x40].pcSampling[8], 10);
    ASSERT_EQ(hotSpotFunctionGenerator.encodings_[0x08].pcSampling[0], 3);
    ASSERT_EQ(hotSpotFunctionGenerator.encodings_[0x08].pcSampling[8], 11);
    ASSERT_EQ(hotSpotFunctionGenerator.topStallReasonByCore_[0], 6);
    ASSERT_EQ(hotSpotFunctionGenerator.topStallReasonByCore_[7], 27);
    RemoveAll(dir);
    GlobalMockObject::verify();
}

/**
/* | 用例集 | DeviceDataParse
/* |测试函数| UpdatePcSampling
/* | 用例名 | process_update_pcsampling_group_by_coreid_and_include_all_coretype_expect_success
/* |用例描述| 测试不同coreType的pcSampling记录都会累计到全局top stall统计中
*/
TEST(DeviceDataParse, process_update_pcsampling_group_by_coreid_and_include_all_coretype_expect_success)
{
    const string tmpDir = "test/ut/resources/op_profiling/instr_prof_group";
    RemoveAll(tmpDir);
    ASSERT_TRUE(MkdirRecusively(tmpDir));
    const string filePath = JoinPath({tmpDir, "pcSampling.bin.0"});
    std::ofstream file(filePath, std::ios::out | std::ios::binary);
    ASSERT_TRUE(file.is_open());
    InstrProfHeadInfo core2Head;
    core2Head.coreId = 2;
    core2Head.coreType = 1;
    core2Head.validLen = static_cast<uint32_t>(TEST_PC_SAMPLING_DATA_LEN * 2);
    WritePcSamplingChunk(file, core2Head, {
        BuildPcSamplingRecord(0x05, {1, 2, 3, 4, 5, 6, 7, 8, 9}),
        BuildPcSamplingRecord(0x08, {2, 3, 4, 5, 6, 7, 8, 9, 10})
    });
    InstrProfHeadInfo core3Head;
    core3Head.coreId = 3;
    core3Head.coreType = 2;
    core3Head.validLen = static_cast<uint32_t>(TEST_PC_SAMPLING_DATA_LEN);
    WritePcSamplingChunk(file, core3Head, {
        BuildPcSamplingRecord(0x01, {3, 4, 5, 6, 7, 8, 9, 10, 11})
    });
    InstrProfHeadInfo cubeHead;
    cubeHead.coreId = 7;
    cubeHead.coreType = 0;
    cubeHead.validLen = static_cast<uint32_t>(TEST_PC_SAMPLING_DATA_LEN);
    WritePcSamplingChunk(file, cubeHead, {
        BuildPcSamplingRecord(0x05, {50, 51, 52, 53, 54, 55, 56, 57, 58})
    });
    file.close();

    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend950PR_9599", "", 0, false, true, false});
    hotSpotFunctionGenerator.startPc_ = 0;
    hotSpotFunctionGenerator.encodings_[0x28] = {0x28, "PIPEA", "NAMEA", "", 0, 0, 0, 0};
    hotSpotFunctionGenerator.encodings_[0x40] = {0x40, "PIPEB", "NAMEB", "", 0, 0, 0, 0};
    hotSpotFunctionGenerator.encodings_[0x08] = {0x08, "PIPEC", "NAMEC", "", 0, 0, 0, 0};

    hotSpotFunctionGenerator.UpdatePcSampling(tmpDir);

    ASSERT_EQ(hotSpotFunctionGenerator.topStallReasonByCore_[0], 56);
    ASSERT_EQ(hotSpotFunctionGenerator.topStallReasonByCore_[7], 84);
    RemoveAll(tmpDir);
}

/**
/* | 用例集 | DeviceDataParse
/* |测试函数| UpdatePcSampling
/* | 用例名 | process_update_pcsampling_accumulate_duplicate_pc_and_skip_missing_pc_expect_success
/* |用例描述| 测试重复PC采样会累加，缺失encoding的PC会被跳过且不影响top stall统计
*/
TEST(DeviceDataParse, process_update_pcsampling_accumulate_duplicate_pc_and_skip_missing_pc_expect_success)
{
    const string tmpDir = "test/ut/resources/op_profiling/instr_prof_duplicate_pc";
    RemoveAll(tmpDir);
    ASSERT_TRUE(MkdirRecusively(tmpDir));
    const string filePath = JoinPath({tmpDir, "pcSampling.bin.0"});
    std::ofstream file(filePath, std::ios::out | std::ios::binary);
    ASSERT_TRUE(file.is_open());

    InstrProfHeadInfo headInfo;
    headInfo.coreId = 5;
    headInfo.coreType = 1;
    headInfo.validLen = static_cast<uint32_t>(TEST_PC_SAMPLING_DATA_LEN * 4);
    WritePcSamplingChunk(file, headInfo, {
        BuildPcSamplingRecord(0x05, {1, 2, 3, 4, 5, 6, 7, 8, 9}),
        BuildPcSamplingRecord(0x05, {10, 11, 12, 13, 14, 15, 16, 17, 18}),
        BuildPcSamplingRecord(0x09, {20, 21, 22, 23, 24, 25, 26, 27, 28}),
        BuildPcSamplingRecord(0x09, {30, 31, 32, 33, 34, 35, 36, 37, 38})
    });
    file.close();

    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend950PR_9599", "", 0, false, true, false});
    hotSpotFunctionGenerator.startPc_ = 0;
    hotSpotFunctionGenerator.encodings_[0x28] = {0x28, "PIPEA", "NAMEA", "", 0, 0, 0, 0};

    hotSpotFunctionGenerator.UpdatePcSampling(tmpDir);

    ASSERT_EQ(hotSpotFunctionGenerator.encodings_[0x28].pcSampling.size(), 9);
    ASSERT_EQ(hotSpotFunctionGenerator.encodings_[0x28].pcSampling[0], 11);
    ASSERT_EQ(hotSpotFunctionGenerator.encodings_[0x28].pcSampling[1], 13);
    ASSERT_EQ(hotSpotFunctionGenerator.encodings_[0x28].pcSampling[7], 25);
    ASSERT_EQ(hotSpotFunctionGenerator.encodings_[0x28].pcSampling[8], 27);
    ASSERT_EQ(hotSpotFunctionGenerator.encodings_.count(0x48), 0);
    ASSERT_EQ(hotSpotFunctionGenerator.topStallReasonByCore_[0], 11);
    ASSERT_EQ(hotSpotFunctionGenerator.topStallReasonByCore_[7], 25);

    RemoveAll(tmpDir);
}

/**
/* | 用例集 | DeviceDataParse
/* |测试函数| DataHandlerOf91095::ParseMemoryChartData
/* | 用例名 | parse_memory_chart_data_skip_top_stall_when_not_simt_expect_success
/* |用例描述| 测试非SIMT场景下不会进入top stall热点图生成流程
*/
TEST(DeviceDataParse, parse_memory_chart_data_skip_top_stall_when_not_simt_expect_success)
{
    GlobalMockObject::verify();
    DataHandlerOf91095 dataHandler;
    ProfMetricsAbilityConfig metricsConfig;
    metricsConfig.pcSamplingEnable = true;
    std::vector<MemRecord> memoryRecords;
    dataHandler.hasSimt_ = false;
    MOCKER(&HotSpotFunctionGenerator::Process)
        .expects(never());
    dataHandler.ParseMemoryChartData("test/ut/resources/op_profiling/deviceA5", metricsConfig, memoryRecords, "");
    GlobalMockObject::verify();
}

/**
/* | 用例集 | DeviceDataParse
/* |测试函数| GenTopStallReasonFigureJson
/* | 用例名 | gen_top_stall_reason_figure_json_expect_todo_format
/* |用例描述| 测试top stall统计结果能够按约定格式生成figure json
*/
TEST(DeviceDataParse, gen_top_stall_reason_figure_json_expect_todo_format)
{
    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend950PR_9599", "", 0, false, true, false});
    hotSpotFunctionGenerator.topStallReasonByCore_ = {3, 5, 7, 9, 11, 13, 15, 17};

    nlohmann::json figure = hotSpotFunctionGenerator.GenTopStallReasonFigureJson();

    ASSERT_TRUE(figure.contains("top_stall_reason_table"));
    auto topStallReasonTable = figure["top_stall_reason_table"];
    ASSERT_EQ(topStallReasonTable["IBuf_Empty"], 3);
    ASSERT_EQ(topStallReasonTable["Nop_Cycles"], 5);
    ASSERT_EQ(topStallReasonTable["Scoreboard_Not_Ready"], 7);
    ASSERT_EQ(topStallReasonTable["Register_bank_conflict"], 9);
    ASSERT_EQ(topStallReasonTable["Resource_conflict"], 11);
    ASSERT_EQ(topStallReasonTable["Warp_Level_Sync"], 13);
    ASSERT_EQ(topStallReasonTable["Divergence_Stack_Spill"], 15);
    ASSERT_EQ(topStallReasonTable["Others"], 17);
}

/**
/* | 用例集 | DeviceDataParse
/* |测试函数| BaseSource::GenStallSampling
/* | 用例名 | gen_stall_sampling_all_sample_expect_active_field_present
/* |用例描述| 测试生成all sample采样详情时，结果中包含Active字段
*/
TEST(DeviceDataParse, gen_stall_sampling_all_sample_expect_active_field_present)
{
    BaseSource baseSource;
    baseSource.pcSampling = {3, 5, 7, 9, 11, 13, 15, 17, 19};

    nlohmann::json stallSampling = baseSource.GenStallSampling(true);

    ASSERT_TRUE(stallSampling.contains("Details"));
    ASSERT_EQ(stallSampling["Details"]["IBuf_Empty"], 3);
    ASSERT_EQ(stallSampling["Details"]["Others"], 17);
    ASSERT_EQ(stallSampling["Details"]["Active"], 19);
}

/**
/* | 用例集 | DeviceDataParse
/* |测试函数| WriteTopStallReasonFigure
/* | 用例名 | write_top_stall_reason_figure_expect_visualize_bin_contains_todo_format
/* |用例描述| 测试top stall统计结果能够写入visualize bin并包含约定字段
*/
TEST(DeviceDataParse, write_top_stall_reason_figure_expect_visualize_bin_contains_todo_format)
{
    const string tmpDir = "test/ut/resources/op_profiling/top_stall_visualize";
    RemoveAll(tmpDir);
    ASSERT_TRUE(MkdirRecusively(tmpDir));

    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend950PR_9599", "", 0, false, true, false});
    hotSpotFunctionGenerator.topStallReasonByCore_ = {1000, 111, 111, 111, 111, 111, 111, 111};
    Utility::Visualize::DefaultWriter writer(Utility::VisualizeType::TOP_STALL_REASON, tmpDir);
    writer.Write(hotSpotFunctionGenerator.GenTopStallReasonFigureJson().dump());

    const string visualizeBinPath = JoinPath({tmpDir, Utility::VISUALIZE_DATA_BIN});
    ASSERT_TRUE(IsExist(visualizeBinPath));
    std::vector<char> binData;
    ASSERT_TRUE(ReadBinaryFile(visualizeBinPath, binData));
    ASSERT_GT(binData.size(), sizeof(uint64_t));
    const uint8_t visualizeType = static_cast<uint8_t>(static_cast<unsigned char>(binData[sizeof(uint64_t)]));
    ASSERT_EQ(visualizeType, static_cast<uint8_t>(Utility::VisualizeType::TOP_STALL_REASON));
    std::string binContent(binData.begin(), binData.end());
    ASSERT_NE(binContent.find("\"top_stall_reason_table\""), std::string::npos);
    ASSERT_NE(binContent.find("\"IBuf_Empty\":1000"), std::string::npos);
    ASSERT_NE(binContent.find("\"Others\":111"), std::string::npos);

    RemoveAll(tmpDir);
}

TEST(DeviceDataParse, HotSpot_UpdateProcessBytes_expect_true)
{
    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend910B4", "", 0, true, false, false});
    Common::MemRecord record1;
    record1.srcAddr = 10; record1.dstAddr = 10; record1.srcMemSize = 10; record1.dstMemSize = 10; record1.pc = 10;
    record1.blockId = 10; record1.src = MemType::GM; record1.dst = MemType::GM;
    Common::MemRecord record2 = record1; record2.src = MemType::L0A;
    std::vector<Common::MemRecord> memoryRecords = {record1, record2};
    Encoding encode {10, "vector", "aaa", "cce", 10, 10, 10, 0};
    hotSpotFunctionGenerator.encodings_[10] = encode;
    hotSpotFunctionGenerator.UpdateProcessBytes(memoryRecords);
    ASSERT_TRUE(hotSpotFunctionGenerator.encodings_[10].processBytes == 20);
}


TEST(DeviceDataParse, HotSpot_ProcessBytesLargeTotals)
{
    constexpr uint64_t gib = 1024ULL * 1024 * 1024;
    constexpr uint64_t chunkBytes = 64 * 1024;
    for (uint64_t total : {2 * gib - 1, 2 * gib, 3 * gib, 4 * gib, 5 * gib}) {
        HotSpotFunctionGenerator generator({"Ascend910B4", "", 0, true, false, false});
        Encoding encoding {};
        encoding.addr = 10;
        generator.encodings_[10] = encoding;
        std::vector<Common::MemRecord> records;
        for (uint64_t remaining = total; remaining > 0;) {
            Common::MemRecord record {};
            record.pc = 10;
            record.blockId = records.size() % 32;
            record.src = MemType::GM;
            record.dst = MemType::UB;
            record.srcMemSize = std::min(remaining, chunkBytes);
            record.dstMemSize = record.srcMemSize;
            remaining -= record.srcMemSize;
            records.emplace_back(record);
        }
        generator.UpdateProcessBytes(records);
        ASSERT_EQ(generator.encodings_.at(10).processBytes, total);
        InstrInfo info;
        info.processBytes = generator.encodings_.at(10).processBytes;
        nlohmann::json output;
        info.AddCommonFieldsToJson(output);
        auto restored = nlohmann::json::parse(output.dump());
        EXPECT_EQ(restored["Process Bytes"].get<uint64_t>(), total);
    }
}

TEST(DeviceDataParse, HotSpot_ProcessBytesCodeLineAndMissingData)
{
    constexpr uint64_t perInstructionBytes = 1536ULL * 1024 * 1024;
    HotSpotFunctionGenerator generator({"Ascend910B4", "", 0, true, false, false});
    Encoding first {};
    first.addr = 10;
    first.processBytes = perInstructionBytes;
    Encoding second = first;
    second.addr = 20;
    CodeLine line;
    ASSERT_TRUE(generator.GenCodeLine("1", {first, second, first}, line));
    EXPECT_EQ(line.processBytes, 2 * perInstructionBytes);
    nlohmann::json output;
    line.AddCommonFieldsToJson(output);
    EXPECT_EQ(output["Process Bytes"].get<uint64_t>(), 2 * perInstructionBytes);
    InstrInfo empty;
    empty.AddCommonFieldsToJson(output);
    EXPECT_EQ(output["Process Bytes"].get<int64_t>(), -1);
}

TEST(DeviceDataParse, HotSpot_ProcessEncoding_expect_true)
{
    GlobalMockObject::verify();
    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend910B4", "", 0, true, false, false});
    auto gm = std::make_shared<GM>("GM", 512);
    auto ptr = Utility::MakeShared<L2Cache>(CacheConfig{"L2Cache", gm, gm, 4, 1, 512, CachePolicy::LRU, true, true});
    MOCKER(&InstrEncoding::GenerateEncoding)
            .stubs()
            .will(returnValue(true));
    ASSERT_TRUE(hotSpotFunctionGenerator.ProcessEncoding("", ptr));
    GlobalMockObject::verify();
}


TEST(DeviceDataParse, HotSpot_GenLine2Encodings_expect_true)
{
    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend910B4", "", 0, true, false, false});
    std::map<std::string, std::vector<Encoding>> line2Encodings;
    Encoding encode {10, "vector", "aaa", "cce", 10, 10, 10, 0};
    line2Encodings["a"] = {encode};
    hotSpotFunctionGenerator.encodings_[10] = encode;
    hotSpotFunctionGenerator.bbCalls_[10] = make_pair(10,10);
    MOCKER(&Utility::IsReadable)
            .stubs()
            .will(returnValue(true));
    MOCKER(&HotSpotFunctionGenerator::GenAddr2Lines)
            .stubs()
            .will(returnValue(true));
    ASSERT_TRUE(hotSpotFunctionGenerator.GenLine2Encodings("", line2Encodings));
    GlobalMockObject::verify();
}

TEST(DeviceDataParse, HotSpot_GenLine2Encodings_skips_ranges_larger_than_kernel)
{
    GlobalMockObject::verify();
    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend910B4", "", 0, true, false, false});
    hotSpotFunctionGenerator.bbCalls_[10] = std::make_pair(18, 0);
    std::map<std::string, std::vector<Encoding>> line2Encodings;
    MOCKER(&Utility::IsReadable)
        .stubs()
        .will(returnValue(true));
    MOCKER(&Utility::GetFileSize)
        .stubs()
        .will(returnValue(static_cast<size_t>(4)));
    MOCKER(&HotSpotFunctionGenerator::GenAddr2Lines)
        .stubs()
        .will(returnValue(true));

    testing::internal::CaptureStdout();
    EXPECT_TRUE(hotSpotFunctionGenerator.GenLine2Encodings("mocked-kernel", line2Encodings));
    const std::string capturedLog = testing::internal::GetCapturedStdout();
    GlobalMockObject::verify();
    EXPECT_NE(capturedLog.find("Skip invalid bbbmap address range"), std::string::npos);
}

TEST(DeviceDataParse, HotSpot_GenAddr2Lines_expect_true)
{
    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend910B4", "", 0, true, false, false});
    std::vector<std::string> addrVec;
    std::unordered_map<uint64_t, std::vector<std::string>> addr2Lines;
    MOCKER(&SymbolizerParser::Parse)
            .stubs()
            .will(returnValue(true));
    ASSERT_TRUE(hotSpotFunctionGenerator.GenAddr2Lines("", addrVec, addr2Lines));
    GlobalMockObject::verify();
}

/**
 * |  用例集  | DeviceCsvEquation
 * | 测试函数 | Entry
 * |  用例名  | test_CalAicMte3ActivateBw_when_input_is_valid_and_expect_return_true
 * | 用例描述 | 输入没有问题时，返回正确的计算结果
 */
TEST(DeviceCsvEquation, test_CalAicMte3ActivateBw_when_input_is_valid_and_expect_return_true)
{
    Profiling::CalDeviceInfo calDeviceInfo = {Common::ChipType::ASCEND910B, 1650, 24, 8, "Ascend910B4"};
    std::map<uint16_t, uint64_t> pmuMap;
    Profiling::Calculate cal(pmuMap, 111111, calDeviceInfo);
    std::string mte3Bw = cal.CalAicMte3ActivateBw(128, 999, 111, 222, 999);
    ASSERT_STREQ(mte3Bw.c_str(), "764.926270");
}

/**
 * |  用例集  | DeviceCsvEquation
 * | 测试函数 | Entry
 * |  用例名  | test_CalAicMte3ActivateBw_when_input_is_invalid_and_expect_return_NA
 * | 用例描述 | 输入数据异常时，返回NA
 */
TEST(DeviceCsvEquation, test_CalAicMte3ActivateBw_when_input_is_invalid_and_expect_return_NA)
{
    Profiling::CalDeviceInfo calDeviceInfo = {Common::ChipType::ASCEND910B, 1650, 24, 8, "Ascend910B4"};
    std::map<uint16_t, uint64_t> pmuMap;
    Profiling::Calculate cal(pmuMap, 111111, calDeviceInfo);
    std::string mte3Bw = cal.CalAicMte3ActivateBw(156, 999, 111, 999999, 12);
    ASSERT_STREQ(mte3Bw.c_str(), "NA");
}

/**
 * |  用例集  | DeviceCsvEquation
 * | 测试函数 | Entry
 * |  用例名  | test_CalAivMteActivateBw_when_input_is_valid_and_expect_return_true
 * | 用例描述 | 输入没有问题时，返回正确的计算结果
 */
TEST(DeviceCsvEquation, test_CalAivMteActivateBw_when_input_is_valid_and_expect_return_true)
{
    Profiling::CalDeviceInfo calDeviceInfo = {Common::ChipType::ASCEND910B, 1650, 24, 8, "Ascend910B4"};
    std::map<uint16_t, uint64_t> pmuMap;
    Profiling::Calculate cal(pmuMap, 111111, calDeviceInfo);
    std::string mte3Bw = cal.CalAivMteActivateBw(128, 111, 222);
    ASSERT_STREQ(mte3Bw.c_str(), "98.347656");
}

/**
 * |  用例集  | DeviceCsvEquation
 * | 测试函数 | Entry
 * |  用例名  | test_CalAivMteActivateBw_when_input_is_invalid_and_expect_return_NA
 * | 用例描述 | 输入数据异常时，返回NA
 */
TEST(DeviceCsvEquation, test_CalAivMteActivateBw_when_input_is_invalid_and_expect_return_NA)
{
    Profiling::CalDeviceInfo calDeviceInfo = {Common::ChipType::ASCEND910B, 1650, 24, 8, "Ascend910B4"};
    std::map<uint16_t, uint64_t> pmuMap;
    Profiling::Calculate cal(pmuMap, 111111, calDeviceInfo);
    std::string mte3Bw = cal.CalAivMteActivateBw(128, 999, 0);
    ASSERT_STREQ(mte3Bw.c_str(), "NA");
}

/**
 * |  用例集  | VisualizeDataAccuracy
 * | 测试函数 | Entry
 * |  用例名  | test_CalCulateForVecOrCubeBw_when_input_is_valid_and_expect_return_true
 * | 用例描述 | 输入没有问题时，返回正确的计算结果
 */
TEST(VisualizeDataAccuracy, test_CalCulateForVecOrCubeBw_when_input_is_valid_and_expect_return_true)
{
    using namespace Visualize;
    Profiling::CalDeviceInfo calDeviceInfo = {Common::ChipType::ASCEND910B, 1650, 24, 8, "Ascend910B4"};
    std::map<uint16_t, uint64_t> pmuMap;
    Profiling::Calculate cal(pmuMap, 111111, calDeviceInfo);
    std::shared_ptr<OpBasicInfo> s1 = nullptr; std::shared_ptr<BasicPmu> s2 = nullptr; unique_ptr<PmuCalculator> s3 = nullptr;
    StorageAccess910B st(s1, s2, s3);
    std::map<std::string, uint64_t> basicPmu = {{"L0A Read", 100}, {"L0B Read", 100}, {"MTE1 Cyc", 10},
        {"UB MTE Read", 100}, {"MTE2 Cyc", 10}, {"UB MTE Write", 100}, {"MTE3 Cyc", 10}, {"L1 GM Write", 10}, {"L0C READ", 1}, {"FIXP Cyc", 1}};
    std::map<std::string, uint64_t> dbiRequest;
    auto bw = st.CalCulateForVecOrCubeBw(basicPmu, "vector", cal, dbiRequest);
    ASSERT_STREQ(bw["MTE1"].c_str(), "NA");
    ASSERT_STREQ(bw["MTE2"].c_str(), "1966.953247");
    ASSERT_STREQ(bw["MTE3"].c_str(), "1966.953247");
    std::map<std::string, uint64_t> dbiRequest2 {{GM_TO_L0A_DATA, 1}, {GM_TO_L0B_DATA, 1}, {GM_TO_L1, 1}};
    auto bw2 = st.CalCulateForVecOrCubeBw(basicPmu, "cube", cal, dbiRequest2);
    ASSERT_STREQ(bw2["MTE1"].c_str(), "5900.552246");
    ASSERT_STREQ(bw2["MTE2"].c_str(), "0.307336");
    ASSERT_STREQ(bw2["MTE3"].c_str(), "196.695328");
    ASSERT_STREQ(bw2["FIXP"].c_str(), "196.695328");
}

/**
 * |  用例集  | VisualizeDataAccuracy
 * | 测试函数 | Entry
 * |  用例名  | test_CalCulateForVecOrCubeBw_when_input_is_invalid_and_expect_return_NA
 * | 用例描述 | 输入数据异常时，返回NA
 */
TEST(DeviceCsvEquation, test_CalCulateForVecOrCubeBw_when_input_is_invalid_and_expect_return_NA)
{
    using namespace Visualize;
    Profiling::CalDeviceInfo calDeviceInfo = {Common::ChipType::ASCEND910B, 1650, 24, 8, "Ascend910B4"};
    std::map<uint16_t, uint64_t> pmuMap;
    Profiling::Calculate cal(pmuMap, 111111, calDeviceInfo);
    std::shared_ptr<OpBasicInfo> s1 = nullptr; std::shared_ptr<BasicPmu> s2 = nullptr; std::unique_ptr<PmuCalculator> s3 = nullptr;
    StorageAccess910B st(s1, s2, s3);
    std::map<std::string, uint64_t> basicPmu = {};
    std::map<std::string, uint64_t> dbiRequest;
    auto bw = st.CalCulateForVecOrCubeBw(basicPmu, "vector", cal, dbiRequest);
    ASSERT_STREQ(bw["MTE1"].c_str(), "NA");
    ASSERT_STREQ(bw["MTE2"].c_str(), "NA");
    ASSERT_STREQ(bw["MTE3"].c_str(), "NA");
}

/**
 * |  用例集  | VisualizeDataAccuracy
 * | 测试函数 | Entry
 * |  用例名  | test_CalCulateForMixBw_when_input_is_valid_and_expect_return_true
 * | 用例描述 | 输入没有问题时，返回正确的计算结果
 */
TEST(VisualizeDataAccuracy, test_CalCulateForMixBw_when_input_is_valid_and_expect_return_true)
{
    using namespace Visualize;
    std::vector<ComputeLoadBlockDetail> computeLoadBlockDetailVec = {};
    Profiling::CalDeviceInfo calDeviceInfo = {Common::ChipType::ASCEND910B, 1650, 24, 8, "Ascend910B4"};
    std::map<std::string, uint64_t> basicPmu;
    std::map<uint16_t, uint64_t> pmuMap;
    Profiling::Calculate cal(pmuMap, 111111, calDeviceInfo);
    std::shared_ptr<OpBasicInfo> s1 = nullptr;
    auto handler = Utility::MakeUnique<Profiling::DataHandler>();
    auto s2 = Utility::MakeShared<BasicPmu>(handler);
    std::unique_ptr<PmuCalculator> s3 = nullptr;
    StorageAccess910B st(s1, s2, s3);
    MemMapDetail detail;
    detail.eventMap[28] = 10; detail.eventMap[34] = 10; detail.eventMap[50] = 1000; detail.eventMap[518] = 100; detail.eventMap[62] = 10;
    detail.eventMap[61] = 10; detail.eventMap[12] = 30; detail.eventMap[13] = 30; detail.eventMap[770] = 20;

    detail.eventMapVec0[28] = 0; detail.eventMapVec0[34] = 0; detail.eventMapVec0[50] = 1000; detail.eventMapVec0[518] = 100; detail.eventMapVec0[62] = 10;
    detail.eventMapVec0[61] = 10; detail.eventMapVec0[12] = 30; detail.eventMapVec0[13] = 30; detail.eventMapVec0[770] = 0;

    detail.eventMapVec1[28] = 0; detail.eventMapVec1[34] = 0; detail.eventMapVec1[50] = 1000; detail.eventMapVec1[518] = 100; detail.eventMapVec1[62] = 10;
    detail.eventMapVec1[61] = 10; detail.eventMapVec1[12] = 30; detail.eventMapVec1[13] = 30; detail.eventMapVec1[770] = 0;
    PmuCalculator pmuCalculator;
    basicPmu = pmuCalculator.GetBasicPmu(detail);
    st.AddBasicPmu910B("mix", detail, basicPmu);
    std::map<std::string, uint64_t> dbiRequest {{GM_TO_L1, 100}};
    st.memoryDetail_ = true;
    auto bw = st.CalCulateForMixBw(basicPmu, cal, dbiRequest);
    ASSERT_STREQ(bw["Cube MTE1"].c_str(), "295.042999");
    ASSERT_STREQ(bw["Cube MTE2"].c_str(), "0.000000");
    ASSERT_STREQ(bw["Cube MTE3"].c_str(), "655.651123");

    ASSERT_STREQ(bw["Vector0 MTE2"].c_str(), "65.565109");
    ASSERT_STREQ(bw["Vector0 MTE3"].c_str(), "65.565109");

    ASSERT_STREQ(bw["Vector1 MTE2"].c_str(), "65.565109");
    ASSERT_STREQ(bw["Vector1 MTE3"].c_str(), "65.565109");
    GlobalMockObject::verify();
}

/**
 * |  用例集  | VisualizeDataAccuracy
 * | 测试函数 | Entry
 * |  用例名  | test_CalCulateForMixBw_when_input_is_invalid_and_expect_return_NA
 * | 用例描述 | 输入数据异常时，返回NA
 */
TEST(VisualizeDataAccuracy, test_CalMte1ActivateBw_when_input_is_invalid_and_expect_return_NA)
{
    using namespace Visualize;
    std::vector<ComputeLoadBlockDetail> computeLoadBlockDetailVec = {};
    Profiling::CalDeviceInfo calDeviceInfo = {Common::ChipType::ASCEND910B, 1650, 24, 8, "Ascend910B4"};
    std::map<uint16_t, uint64_t> pmuMap;
    std::map<std::string, uint64_t> basicPmu;
    Profiling::Calculate cal(pmuMap, 111111, calDeviceInfo);
    std::shared_ptr<OpBasicInfo> s1 = nullptr;
    auto handler = Utility::MakeUnique<Profiling::DataHandler>();
    auto s2 = Utility::MakeShared<BasicPmu>(handler);
    std::unique_ptr<PmuCalculator> s3 = nullptr;
    StorageAccess910B st(s1, s2, s3);
    MemMapDetail detail;
    PmuCalculator pmuCalculator;
    basicPmu = pmuCalculator.GetBasicPmu(detail);
    st.AddBasicPmu910B("mix", detail, basicPmu);
    std::map<std::string, uint64_t> dbiRequest;
    auto bw = st.CalCulateForMixBw(basicPmu, cal, dbiRequest);
    ASSERT_STREQ(bw["Cube MTE1"].c_str(),    "NA");
    ASSERT_STREQ(bw["Cube MTE2"].c_str(),    "NA");
    ASSERT_STREQ(bw["Cube MTE3"].c_str(),    "NA");

    ASSERT_STREQ(bw["Vector0 MTE2"].c_str(), "NA");
    ASSERT_STREQ(bw["Vector0 MTE3"].c_str(), "NA");

    ASSERT_STREQ(bw["Vector1 MTE2"].c_str(), "NA");
    ASSERT_STREQ(bw["Vector1 MTE3"].c_str(), "NA");
    GlobalMockObject::verify();
}

/**
 * |  用例集  | VisualizeDataAccuracy
 * | 测试函数 | StorageAccess910B::SetScalarMemInfo
 * |  用例名  | test_SetScalarMemInfo_mix_operate_and_expect_return_true
 * | 用例描述 | 输入没有问题时，返回正确的计算结果
 */
TEST(VisualizeDataAccuracy, test_SetScalarMemInfo_mix_operate_and_expect_return_true)
{
    using namespace Visualize;
    Profiling::CalDeviceInfo calDeviceInfo = {Common::ChipType::ASCEND910B, 1650, 24, 8, "Ascend910B4"};
    std::map<uint16_t, uint64_t> pmuMap;
    Profiling::Calculate cal(pmuMap, 111111, calDeviceInfo);
    std::shared_ptr<OpBasicInfo> s1 = nullptr; std::shared_ptr<BasicPmu> s2 = nullptr; unique_ptr<PmuCalculator> s3 = nullptr;
    StorageAccess910B st(s1, s2, s3);
    st.SetScalarMemInfo("mix", basicScalarPmu, cal);
    std::vector<std::string> cubeIndex = {"1.110303", "1.150909", "1.112121", "0.855758", "0.881818", "1.076970", "0.080606", "1.117576", "1.137576"};
    std::vector<uint64_t> vec0Index = {1148UL, 4318UL, 7158UL, 1388UL, 8158UL, 318UL, 148UL, 818UL, 118UL};
    std::vector<std::string> vec1Index = {"0.695758", "2.616970", "4.338182", "0.841212", "4.944242", "0.192727", "0.089697", "0.495758", "0.071515"};
    for (auto i = 0; i < 9; i++) {
        ASSERT_STREQ(st.memInfoScalarMap_["Scalar Cube"][i].time.c_str(), cubeIndex[i].c_str());
    }
    for (auto i = 0; i < 9; i++) {
        ASSERT_EQ(st.memInfoScalarMap_["Scalar Vector Core0"][i].cycle, vec0Index[i]);
    }
    for (auto i = 0; i < 9; i++) {
        ASSERT_STREQ(st.memInfoScalarMap_["Scalar Vector Core1"][i].time.c_str(), vec1Index[i].c_str());
    }
}

/**
 * |  用例集  | VisualizeDataAccuracy
 * | 测试函数 | StorageAccess910B::SetScalarMemInfo
 * |  用例名  | test_SetScalarMemInfo_vector_operate_and_expect_return_true
 * | 用例描述 | 输入没有问题时，返回正确的计算结果
 */
TEST(VisualizeDataAccuracy, test_SetScalarMemInfo_vector_operate_and_expect_return_true)
{
    using namespace Visualize;
    Profiling::CalDeviceInfo calDeviceInfo = {Common::ChipType::ASCEND910B, 1650, 24, 8, "Ascend910B4"};
    std::map<uint16_t, uint64_t> pmuMap;
    Profiling::Calculate cal(pmuMap, 111111, calDeviceInfo);
    std::shared_ptr<OpBasicInfo> s1 = nullptr; std::shared_ptr<BasicPmu> s2 = nullptr; unique_ptr<PmuCalculator> s3 = nullptr;
    StorageAccess910B st(s1, s2, s3);
    st.SetScalarMemInfo("vector", basicScalarPmu, cal);
    std::vector<std::string> vecIndex = {"1.110303", "1.150909", "1.112121", "0.881818", "1.076970", "0.080606", "1.117576"};
     for (auto i = 0; i < 7; i++) {
        ASSERT_STREQ(st.memInfoScalarMap_["Scalar"][i].time.c_str(), vecIndex[i].c_str());
    }
    ASSERT_EQ(st.memInfoScalarMap_["Scalar"][7].cycle, 1478UL);
    ASSERT_EQ(st.memInfoScalarMap_["Scalar"][8].cycle, 1428UL);
}

/**
 * |  用例集  | VisualizeDataAccuracy
 * | 测试函数 | StorageAccess910B::SetScalarMemInfo
 * |  用例名  | test_SetScalarMemInfo_cube_operate_and_expect_return_true
 * | 用例描述 | 输入没有问题时，返回正确的计算结果
 */
TEST(VisualizeDataAccuracy, test_SetScalarMemInfo_cube_operate_and_expect_return_true)
{
    using namespace Visualize;
    Profiling::CalDeviceInfo calDeviceInfo = {Common::ChipType::ASCEND910B, 1650, 24, 8, "Ascend910B4"};
    std::map<uint16_t, uint64_t> pmuMap;
    Profiling::Calculate cal(pmuMap, 111111, calDeviceInfo);
    std::shared_ptr<OpBasicInfo> s1 = nullptr; std::shared_ptr<BasicPmu> s2 = nullptr; unique_ptr<PmuCalculator> s3 = nullptr;
    StorageAccess910B st(s1, s2, s3);
    st.SetScalarMemInfo("cube", basicScalarPmu, cal);
    std::vector<std::string> cubeIndex = {"1.110303", "1.150909", "1.112121", "0.855758", "0.881818", "1.076970", "0.080606"};
     for (auto i = 0; i < 7; i++) {
        ASSERT_STREQ(st.memInfoScalarMap_["Scalar"][i].time.c_str(), cubeIndex[i].c_str());
    }
    ASSERT_EQ(st.memInfoScalarMap_["Scalar"][7].cycle, 1844UL);
    ASSERT_EQ(st.memInfoScalarMap_["Scalar"][8].cycle, 1877UL);
}

/**
 * |  用例集  | VisualizeDataAccuracy
 * | 测试函数 | StorageAccess910B::AddInternuclearScalarIndex
 * |  用例名  | test_AddInternuclearScalarIndex_mix_operate_and_expect_return_true
 * | 用例描述 | 输入没有问题时，返回正确的计算结果
 */
TEST(VisualizeDataAccuracy, test_AddInternuclearScalarIndex_mix_operate_and_expect_return_true)
{
    using namespace Visualize;
    Profiling::CalDeviceInfo calDeviceInfo = {Common::ChipType::ASCEND910B, 1650, 24, 8, "Ascend910B4"};
    std::map<uint16_t, uint64_t> pmuMap;
    Profiling::Calculate cal(pmuMap, 111111, calDeviceInfo);
    std::shared_ptr<OpBasicInfo> s1 = nullptr; std::shared_ptr<BasicPmu> s2 = nullptr; unique_ptr<PmuCalculator> s3 = nullptr;
    StorageAccess910B st(s1, s2, s3);
    st.AddInternuclearScalarIndex("mix", basicScalarPmu, cal);
    std::vector<uint64_t> indexValue = {1275UL, 3542UL, 4345UL, 6514UL, 3144UL, 5315UL, 6432UL, 3152UL, 5476UL, 2453UL, 653UL, 324UL, 424UL, 653UL, 536UL, 133UL};
    for (auto i = 0; i < 16; i++) {
        ASSERT_EQ(st.memInfoScalarMap_["Scalar Cube"][i].cycle, indexValue[i]);
        ASSERT_EQ(st.memInfoScalarMap_["Scalar Vector Core0"][i].cycle, indexValue[i]);
        ASSERT_EQ(st.memInfoScalarMap_["Scalar Vector Core1"][i].cycle, indexValue[i]);
    }
}

/**
 * |  用例集  | VisualizeDataAccuracy
 * | 测试函数 | StorageAccess910B::AddInternuclearScalarIndex
 * |  用例名  | test_AddInternuclearScalarIndex_cube_operate_and_expect_return_true
 * | 用例描述 | 输入没有问题时，返回正确的计算结果
 */
TEST(VisualizeDataAccuracy, test_AddInternuclearScalarIndex_cube_operate_and_expect_return_true)
{
    using namespace Visualize;
    Profiling::CalDeviceInfo calDeviceInfo = {Common::ChipType::ASCEND910B, 1650, 24, 8, "Ascend910B4"};
    std::map<uint16_t, uint64_t> pmuMap;
    Profiling::Calculate cal(pmuMap, 111111, calDeviceInfo);
    std::shared_ptr<OpBasicInfo> s1 = nullptr; std::shared_ptr<BasicPmu> s2 = nullptr; unique_ptr<PmuCalculator> s3 = nullptr;
    StorageAccess910B st(s1, s2, s3);
    st.AddInternuclearScalarIndex("cube", basicScalarPmu, cal);
    std::vector<uint64_t> indexValue = {1275UL, 3542UL, 4345UL, 6514UL, 3144UL, 5315UL, 6432UL, 3152UL, 5476UL, 2453UL, 653UL, 324UL, 424UL, 653UL, 536UL, 133UL};
    for (auto i = 0; i < 16; i++) {
        ASSERT_EQ(st.memInfoScalarMap_["Scalar"][i].cycle, indexValue[i]);
    }
}

/**
 * |  用例集  | VisualizeDataAccuracy
 * | 测试函数 | StorageAccess910B::AddInternuclearScalarIndex
 * |  用例名  | test_AddInternuclearScalarIndex_vector_operate_and_expect_return_true
 * | 用例描述 | 输入没有问题时，返回正确的计算结果
 */
TEST(VisualizeDataAccuracy, test_AddInternuclearScalarIndex_vector_operate_and_expect_return_true)
{
    using namespace Visualize;
    Profiling::CalDeviceInfo calDeviceInfo = {Common::ChipType::ASCEND910B, 1650, 24, 8, "Ascend910B4"};
    std::map<uint16_t, uint64_t> pmuMap;
    Profiling::Calculate cal(pmuMap, 111111, calDeviceInfo);
    std::shared_ptr<OpBasicInfo> s1 = nullptr; std::shared_ptr<BasicPmu> s2 = nullptr; unique_ptr<PmuCalculator> s3 = nullptr;
    StorageAccess910B st(s1, s2, s3);
    st.AddInternuclearScalarIndex("vector", basicScalarPmu, cal);
    std::vector<uint64_t> indexValue = {1275UL, 3542UL, 4345UL, 6514UL, 3144UL, 5315UL, 6432UL, 3152UL, 5476UL, 2453UL, 653UL, 324UL, 424UL, 653UL, 536UL, 133UL};
    for (auto i = 0; i < 16; i++) {
        ASSERT_EQ(st.memInfoScalarMap_["Scalar"][i].cycle, indexValue[i]);
    }
}

/**
 * | 用例集 | DataHandlerOf91095
 * |测试函数| ParseDurationBin
 * | 测试场景 | A5输入2条ACSQ记录，mock为AIC任务的START=100、END=200。
 * | 预期结果 | 输出startTime为100、endTime为200。
 */
TEST(DataHandlerOf91095, parse_aic_start_and_end_records_expect_times_100_and_200) {
    GlobalMockObject::verify();
    DataHandlerOf91095 handler;
    std::vector<char> bytes(ACSQ_LENGTH_A5 * 2, 0);
    MOCKER(&AcsqBean::GetTaskType).stubs().will(returnValue(static_cast<uint16_t>(SqeType::AIC_TYPE)));
    MOCKER(&AcsqBean::GetTimeType)
        .stubs()
        .will(returnValue(Common::TimeType::START))
        .then(returnValue(Common::TimeType::END));
    MOCKER(&AcsqBean::GetSystemTime).stubs().will(returnValue(uint64_t{100})).then(returnValue(uint64_t{200}));
    uint64_t start = 0;
    uint64_t end = 0;
    handler.ParseDurationBin("", bytes, bytes.size(), start, end);
    EXPECT_EQ(start, 100U);
    EXPECT_EQ(end, 200U);
    GlobalMockObject::verify();
}

/**
 * | 用例集 | DataHandler
 * |测试函数| ParseMemoryChartBin
 * | 测试场景 | 分别读取含2条MemRecord的文件、不存在的文件、长度仅1字节的不完整文件。
 * | 预期结果 | 有效文件返回2条记录，缺失和不完整文件均返回空记录列表。
 */
TEST(DataHandler, read_memory_chart_expect_two_records_for_valid_file_and_empty_for_invalid_files) {
    const std::string dir = "test/ut/resources/op_profiling/memory_chart_test";
    RemoveAll(dir);
    ASSERT_TRUE(MkdirRecusively(dir));
    const std::string valid = JoinPath({dir, "valid.bin"});
    const std::string invalid = JoinPath({dir, "invalid.bin"});
    {
        std::ofstream out(valid, std::ios::out | std::ios::binary);
        std::vector<MemRecord> records(2);
        out.write(
            reinterpret_cast<char *>(records.data()), static_cast<std::streamsize>(records.size() * sizeof(MemRecord)));
        out.close();
    }
    {
        std::ofstream out(invalid, std::ios::out | std::ios::binary);
        char byte = 0;
        out.write(&byte, 1);
        out.close();
    }
    DataHandler handler;
    auto parsed = handler.ParseMemoryChartBin(valid);
    EXPECT_EQ(parsed.size(), 2U);
    auto missing = handler.ParseMemoryChartBin(JoinPath({dir, "missing.bin"}));
    EXPECT_TRUE(missing.empty());
    auto bad = handler.ParseMemoryChartBin(invalid);
    EXPECT_TRUE(bad.empty());
    RemoveAll(dir);
}

/**
 * | 用例集 | ComputeLoadCalculator
 * |测试函数| Entry
 * | 测试场景 | A5的F16 SIMD有10条指令、SIMT为0，S8指令均为0；另检查重复注册和缺失BasicPmu。
 * | 预期结果 | 表格为3行，F16和Vector All Active指令数均为10，不生成S8行；重复Entry返回
 * |          | NONBLOCKING_ERROR；950/910B/310P缺少BasicPmu时Entry仍返回SUCCESS。
 */
TEST(ComputeLoadCalculator, a5_vector_expect_ten_instructions_and_safe_duplicate_or_missing_data_handling) {
    using namespace Visualize;
    Parse::DataCenter dataCenter;
    auto handler = Utility::MakeUnique<DataHandler>();
    ComputeLoadBlockDetail detail{};
    detail.blockId = 0;
    detail.blockType = "vector";
    detail.opType = "vector";
    detail.freq = 1000;
    detail.totalCycles = 100;
    OperandRecord rec;
    rec.instructions = 10;
    detail.operandRecordMap.simdMap[OperandType::DATA_F16] = rec;
    rec.instructions = 0;
    detail.operandRecordMap.simdMap[OperandType::DATA_S8] = rec;
    detail.operandRecordMap.simtMap[OperandType::DATA_F16] = {};
    detail.operandRecordMap.simtMap[OperandType::DATA_S8] = {};
    detail.eventMap[1281] = 5;
    detail.eventMap[12] = 500;
    handler->computeLoadBlockDetailVec_.push_back(detail);
    auto basicPmu = Utility::MakeShared<BasicPmu>(handler);
    ASSERT_TRUE(dataCenter.DataTableRegister(basicPmu));
    Parse::ComputeLoadCalculator calc(dataCenter, ChipProductType::ASCEND950_SERIES);
    EXPECT_EQ(calc.Entry(), Parse::PluginErrorCode::SUCCESS);
    auto result = dataCenter.GetDbPtr<Parse::ComputeLoadInfo>();
    ASSERT_NE(result, nullptr);
    const auto &table = result->table["subblock_detail"];
    ASSERT_EQ(table.size(), 3U);
    EXPECT_EQ(table[0]["name"], "Vector F16");
    EXPECT_EQ(table[0]["value"], "10");
    EXPECT_EQ(table[1]["name"], "Vector All Active");
    EXPECT_EQ(table[1]["value"], "10");
    EXPECT_EQ(calc.Entry(), Parse::PluginErrorCode::NONBLOCKING_ERROR);

    Parse::DataCenter emptyCenter;
    Parse::ComputeLoadCalculator calc950(emptyCenter, ChipProductType::ASCEND950_SERIES);
    EXPECT_EQ(calc950.Entry(), Parse::PluginErrorCode::SUCCESS);
    emptyCenter.Clear();
    Parse::ComputeLoadCalculator calc910B(emptyCenter, ChipProductType::ASCEND910B_SERIES);
    EXPECT_EQ(calc910B.Entry(), Parse::PluginErrorCode::SUCCESS);
    emptyCenter.Clear();
    Parse::ComputeLoadCalculator calc310P(emptyCenter, ChipProductType::ASCEND310P_SERIES);
    EXPECT_EQ(calc310P.Entry(), Parse::PluginErrorCode::SUCCESS);
    GlobalMockObject::verify();
}

/**
 * | 测试函数 | FIFOPolicy::Inject / HitRefresh
 * | 测试场景 | 容量为2，依次插入1、2，刷新位置0，再插入3；同时传入越界的刷新位置。
 * | 预期结果 | 前2次不淘汰，刷新后顺序为[2,1]且保留dirty标记；越界刷新不改变顺序；
 * |          | 插入3时淘汰2，保留被淘汰项的valid/dirty标记，最终顺序为[1,3]。
 */
TEST(CachePolicy, fifo_hit_then_insert_expect_refreshed_order_and_oldest_entry_evicted) {
    FIFOPolicy policy;
    std::vector<CLEntry> data;
    CLEntry replaced{};
    EXPECT_FALSE(policy.Inject(2, {1, true, true}, data, replaced));
    EXPECT_FALSE(policy.Inject(2, {2, false, true}, data, replaced));
    policy.HitRefresh(0, data);
    ASSERT_EQ(data.size(), 2U);
    EXPECT_EQ(data[0].id, 2U);
    EXPECT_EQ(data[1].id, 1U);
    EXPECT_TRUE(data[1].isDirty);
    policy.HitRefresh(data.size(), data);
    EXPECT_EQ(data[0].id, 2U);
    EXPECT_EQ(data[1].id, 1U);
    EXPECT_TRUE(policy.Inject(2, {3, false, true}, data, replaced));
    EXPECT_EQ(replaced.id, 2U);
    EXPECT_FALSE(replaced.isDirty);
    EXPECT_TRUE(replaced.isValid);
    ASSERT_EQ(data.size(), 2U);
    EXPECT_EQ(data[0].id, 1U);
    EXPECT_EQ(data[1].id, 3U);
}

class TimelineSummaryTest : public testing::Test {
protected:
    void SetUp() override {
        GlobalMockObject::verify();
        CustomInputConfig::Instance().Reset();
        char directory[] = "/tmp/msopprof_summary_XXXXXX";
        const auto created = mkdtemp(directory);
        ASSERT_NE(created, nullptr);
        root_ = created;
    }

    void TearDown() override {
        CustomInputConfig::Instance().Reset();
        if (!root_.empty()) {
            RemoveAll(root_);
        }
        GlobalMockObject::verify();
    }

    TimelineInput Input(uint32_t device, uint64_t index = 0) {
        MergeTimelineData timeline;
        timeline.minCycle = (1ULL << 60) + device * 5000;
        timeline.isCommunication = true;
        timeline.timelineJson = {{"profilingType", "op"}, {"schemaVersion", 1},
            {"traceEvents",
                json::array(
                    {{{"ph", "X"}, {"pid", "BLOCK"}, {"tid", "core0"}, {"name", "work"}, {"ts", 1}, {"dur", 2}}})}};
        return {"kernel", device, index, timeline, 50000};
    }

    std::unique_ptr<DeviceDataParse> Parser() {
        return MakeUnique<DeviceDataParse>(ChipType::ASCEND910B, PmuEventsId{}, ProfMetricsAbilityConfig{});
    }

    TimelineMergeConfig Config(uint32_t launches) {
        TimelineMergeConfig config;
        config.kernelName = "kernel*";
        config.mstxName = "range1";
        config.rankCount = 2;
        config.launchCount = launches;
        return config;
    }

    void Collect(DeviceDataParse &parser, uint32_t device, uint64_t index, const std::string &name = "kernel",
        uint64_t start = 0) {
        DataHandler handler;
        handler.opBasicHeader = {"Op Name", "Device Id", "Task Duration(us)"};
        handler.opInfoMap_ = {{"Op Name", name}, {"Device Id", std::to_string(device)}, {"Task Duration(us)", "2"}};
        handler.startTime_ = start;
        handler.isMC2_ = true;
        handler.taskSchedulerFreq_ = 50000;
        parser.RecordSummary(handler,
            root_ + "/device" + std::to_string(device) + "/" + name + "/" + std::to_string(index),
            Input(device, index).timeline);
    }

    std::string Record(uint32_t config, uint32_t device, const std::string &path) {
        return json({{"config_index", config}, {"device_id", device}, {"output_path", path}}).dump() + "\n";
    }

    void WriteRecords(const std::string &content) {
        ASSERT_TRUE(MkdirRecusively(root_ + "/tmp_dump"));
        ASSERT_TRUE(WriteFileByStream(root_ + "/tmp_dump/timeline_mstx.json", content));
    }

    std::string root_;
};

/**
 * | 用例集 | TimelineSummaryTest
 * |测试函数| DeviceDataParse::MergeTimelines
 * | 用例名 | merge_timelines_expect_aligned_trace_and_matching_bin
 * |用例描述| 两卡流水合并后按设备区分并对齐时间，同目录生成内容一致的trace.json与visualize_data.bin。
 */
TEST_F(TimelineSummaryTest, merge_timelines_expect_aligned_trace_and_matching_bin) {
    ASSERT_TRUE(DeviceDataParse::MergeTimelines({{Input(1, 3), Input(0, 3)}}, root_ + "/summary"));
    const auto directory = root_ + "/summary/kernel/3";
    std::ifstream traceFile(directory + "/trace.json");
    ASSERT_TRUE(traceFile.is_open());
    const auto trace = json::parse(traceFile);
    const auto &events = trace.at("traceEvents");
    ASSERT_EQ(events.size(), 2U);
    EXPECT_EQ(events[0]["pid"], "Device 0 | BLOCK");
    EXPECT_EQ(events[1]["pid"], "Device 1 | BLOCK");
    EXPECT_FLOAT_EQ(events[0]["ts"].get<float>(), 1.0f);
    EXPECT_FLOAT_EQ(events[1]["ts"].get<float>(), 101.0f);
    EXPECT_EQ(events[0]["dur"], 2);
    EXPECT_EQ(events[1]["dur"], 2);

    // TRACE块头为8字节长度及4字节type/padding/version/reserved。
    std::ifstream binFile(directory + "/visualize_data.bin", std::ios::binary);
    uint64_t contentSize = 0;
    uint8_t header[4] = {};
    ASSERT_TRUE(static_cast<bool>(binFile.read(reinterpret_cast<char *>(&contentSize), sizeof(contentSize))));
    ASSERT_TRUE(static_cast<bool>(binFile.read(reinterpret_cast<char *>(header), sizeof(header))));
    EXPECT_EQ(header[0], static_cast<uint8_t>(Utility::VisualizeType::TRACE));
    ASSERT_GT(contentSize, header[1]);
    std::string content(contentSize, '\0');
    ASSERT_TRUE(static_cast<bool>(binFile.read(&content[0], content.size())));
    EXPECT_EQ(json::parse(content.substr(0, contentSize - header[1])), trace);
    EXPECT_EQ(binFile.peek(), std::ifstream::traits_type::eof());
}

/**
 * | 用例集 | TimelineSummaryTest
 * |测试函数| DeviceDataParse::ReadTimelineMstxPaths
 * | 用例名 | read_mstx_paths_expect_config_device_buckets_and_record_order
 * |用例描述| 合法记录按配置和设备保存，去除末尾dump层，并保留同卡记录顺序。
 */
TEST_F(TimelineSummaryTest, read_mstx_paths_expect_config_device_buckets_and_record_order) {
    WriteRecords(Record(0, 1, "/device1/kernel/4/dump") + Record(1, 0, "/device0/kernel/8") +
        Record(0, 1, "/device1/kernel/9/dump"));
    std::vector<TimelineMstxPaths> recorded(2);
    DeviceDataParse::ReadTimelineMstxPaths(root_, recorded);
    ASSERT_EQ(recorded[0].size(), 1U);
    ASSERT_EQ(recorded[1].size(), 1U);
    EXPECT_EQ(recorded[0].at(1), (std::vector<std::string>{"/device1/kernel/4", "/device1/kernel/9"}));
    EXPECT_EQ(recorded[1].at(0), (std::vector<std::string>{"/device0/kernel/8"}));
}

/**
 * | 用例集 | TimelineSummaryTest
 * |测试函数| DeviceDataParse::ReadTimelineMstxPaths
 * | 用例名 | read_invalid_mstx_records_expect_skip_and_keep_following_valid_record
 * |用例描述| 非法JSON、配置/设备字段及空或非字符串路径被跳过，后续合法记录仍可正常读取。
 */
TEST_F(TimelineSummaryTest, read_invalid_mstx_records_expect_skip_and_keep_following_valid_record) {
    std::string records = "invalid json\n{}\n" + Record(1, 0, "/invalid/dump");
    for (const json device : {json(-1), json(uint64_t{UINT32_MAX} + 1), json(1.5), json("0")}) {
        records += json({{"config_index", 0}, {"device_id", device}, {"output_path", "/invalid/dump"}}).dump() + "\n";
    }
    records += json({{"config_index", 0}, {"device_id", 0}, {"output_path", 123}}).dump() + "\n";
    records += Record(0, 0, "") + Record(0, 1, "/valid/dump");
    WriteRecords(records);
    std::vector<TimelineMstxPaths> recorded(1);
    DeviceDataParse::ReadTimelineMstxPaths(root_, recorded);
    ASSERT_EQ(recorded[0].size(), 1U);
    EXPECT_EQ(recorded[0].at(1), (std::vector<std::string>{"/valid"}));
}

/**
 * | 用例集 | TimelineSummaryTest
 * |测试函数| DeviceDataParse::AddTimelineMstxGroups
 * | 用例名 | add_mstx_groups_expect_pair_by_position_and_respect_launch_limit
 * |用例描述| 各卡按列表位置配组，应用launch_count上限，统一执行编号及输出目录。
 */
TEST_F(TimelineSummaryTest, add_mstx_groups_expect_pair_by_position_and_respect_launch_limit) {
    const TimelineMstxPaths paths{{0, {"a", "b"}}, {1, {"c", "d"}}};
    const TimelineInputsByPath inputs{{"a", Input(0, 4)}, {"b", Input(0, 5)}, {"c", Input(1, 8)}, {"d", Input(1, 9)}};
    TimelineGroups groups;
    DeviceDataParse::AddTimelineMstxGroups(Config(1), paths, inputs, "test ", "kernel_range1", groups);
    ASSERT_EQ(groups.size(), 1U);
    ASSERT_EQ(groups[0].size(), 2U);
    EXPECT_EQ(groups[0][0].deviceId, 0U);
    EXPECT_EQ(groups[0][1].deviceId, 1U);
    for (const auto &input : groups[0]) {
        EXPECT_EQ(input.executionIndex, 0U);
        EXPECT_EQ(input.mergeDirectory, "kernel_range1");
    }
}

/**
 * | 用例集 | TimelineSummaryTest
 * |测试函数| DeviceDataParse::AddTimelineMstxGroups
 * | 用例名 | add_mstx_groups_with_missing_timeline_expect_skip_position_without_shifting
 * |用例描述| 首次执行缺少一张卡的流水时跳过该位置，后续完整组保留原位置编号。
 */
TEST_F(TimelineSummaryTest, add_mstx_groups_with_missing_timeline_expect_skip_position_without_shifting) {
    const TimelineMstxPaths paths{{0, {"a", "b"}}, {1, {"missing", "d"}}};
    const TimelineInputsByPath inputs{{"a", Input(0)}, {"b", Input(0)}, {"d", Input(1)}};
    TimelineGroups groups;
    DeviceDataParse::AddTimelineMstxGroups(Config(2), paths, inputs, "test ", "kernel_range1", groups);
    ASSERT_EQ(groups.size(), 1U);
    ASSERT_EQ(groups[0].size(), 2U);
    EXPECT_EQ(groups[0][0].executionIndex, 1U);
    EXPECT_EQ(groups[0][1].executionIndex, 1U);
}

/**
 * | 用例集 | TimelineSummaryTest
 * |测试函数| DeviceDataParse::GetTimelineGroups
 * | 用例名 | get_mc2_groups_expect_group_by_name_and_numeric_execution_index
 * |用例描述| 无显式配置时，MC2通算流水按算子名称和执行编号自动配组。
 */
TEST_F(TimelineSummaryTest, get_mc2_groups_expect_group_by_name_and_numeric_execution_index) {
    auto parser = Parser();
    for (uint32_t device : {0U, 1U}) {
        Collect(*parser, device, 10);
        Collect(*parser, device, 2);
    }
    const auto groups = parser->GetTimelineGroups(root_);
    ASSERT_EQ(groups.size(), 2U);
    for (size_t i = 0; i < groups.size(); ++i) {
        ASSERT_EQ(groups[i].size(), 2U);
        for (const auto &input : groups[i]) {
            EXPECT_EQ(input.opName, "kernel");
            EXPECT_EQ(input.executionIndex, i == 0 ? 2U : 10U);
            EXPECT_TRUE(input.mergeDirectory.empty());
        }
        EXPECT_EQ(groups[i][0].deviceId, 0U);
        EXPECT_EQ(groups[i][1].deviceId, 1U);
    }
}

/**
 * | 用例集 | TimelineSummaryTest
 * |测试函数| DeviceDataParse::GetTimelineGroups
 * | 用例名 | get_explicit_groups_expect_record_matching_and_distinct_config_directories
 * |用例描述| 显式配置按采集目录关联两卡流水，重复配置分别生成带序号的分组目录。
 */
TEST_F(TimelineSummaryTest, get_explicit_groups_expect_record_matching_and_distinct_config_directories) {
    const json selection{{"kernel_name", "kernel*"}, {"mstx_name", "range1"}, {"rank_count", 2}, {"launch_count", 1}};
    ASSERT_TRUE(WriteFileByStream(
        root_ + "/config.json", json({{"timeline_merge_config", json::array({selection, selection})}}).dump()));
    std::string error;
    ASSERT_TRUE(CustomInputConfig::Instance().Load(root_ + "/config.json", error)) << error;
    auto parser = Parser();
    Collect(*parser, 0, 4);
    Collect(*parser, 1, 8);
    std::string records;
    for (uint32_t config : {0U, 1U}) {
        records += Record(config, 0, root_ + "/device0/kernel/4/dump");
        records += Record(config, 1, root_ + "/device1/kernel/8/dump");
    }
    WriteRecords(records);
    const auto groups = parser->GetTimelineGroups(root_);
    ASSERT_EQ(groups.size(), 2U);
    for (size_t i = 0; i < groups.size(); ++i) {
        ASSERT_EQ(groups[i].size(), 2U);
        EXPECT_EQ(groups[i][0].deviceId, 0U);
        EXPECT_EQ(groups[i][1].deviceId, 1U);
        for (const auto &input : groups[i]) {
            EXPECT_EQ(input.executionIndex, 0U);
            EXPECT_EQ(input.mergeDirectory, "kernel*_range1_" + std::to_string(i));
        }
    }
}

/**
 * | 用例集 | TimelineSummaryTest
 * |测试函数| DeviceDataParse::GetTimelineGroups / MergeTimelines
 * | 用例名 | explicit_rank_limit_expect_merge_all_actual_devices_within_limit
 * |用例描述| 实际四卡各执行五次，rank_count为2时跳过，为4或10时合并全部四卡，并应用launch_count上限。
 */
TEST_F(TimelineSummaryTest, explicit_rank_limit_expect_merge_all_actual_devices_within_limit) {
    for (uint32_t rankCount : {2U, 4U, 10U}) {
        for (uint32_t launchCount : {2U, 10U}) {
            SCOPED_TRACE("rank_count=" + std::to_string(rankCount) + ", launch_count=" + std::to_string(launchCount));
            const json selection{{"kernel_name", "kernel*"}, {"mstx_name", "range1"}, {"rank_count", rankCount},
                {"launch_count", launchCount}};
            ASSERT_TRUE(WriteFileByStream(
                root_ + "/config.json", json({{"timeline_merge_config", json::array({selection})}}).dump()));
            std::string error;
            CustomInputConfig::Instance().Reset();
            ASSERT_TRUE(CustomInputConfig::Instance().Load(root_ + "/config.json", error)) << error;
            auto parser = Parser();
            std::string records;
            for (uint32_t device = 0; device < 4; ++device) {
                for (uint64_t index = 0; index < 5; ++index) {
                    Collect(*parser, device, index);
                    records += Record(0, device,
                        root_ + "/device" + std::to_string(device) + "/kernel/" + std::to_string(index) + "/dump");
                }
            }
            WriteRecords(records);
            const auto groups = parser->GetTimelineGroups(root_);
            const size_t expectedCount = rankCount < 4 ? 0 : std::min(launchCount, 5U);
            ASSERT_EQ(groups.size(), expectedCount);
            const auto directory =
                root_ + "/rank" + std::to_string(rankCount) + "_launch" + std::to_string(launchCount);
            ASSERT_TRUE(DeviceDataParse::MergeTimelines(groups, directory));
            for (size_t index = 0; index < expectedCount; ++index) {
                ASSERT_EQ(groups[index].size(), 4U);
                const auto output = directory + "/kernel*_range1/" + std::to_string(index);
                std::ifstream traceFile(output + "/trace.json");
                ASSERT_TRUE(traceFile.is_open());
                const auto trace = json::parse(traceFile);
                const auto &events = trace.at("traceEvents");
                ASSERT_EQ(events.size(), 4U);
                EXPECT_TRUE(IsExist(output + "/visualize_data.bin"));
                for (uint32_t device = 0; device < 4; ++device) {
                    EXPECT_EQ(groups[index][device].deviceId, device);
                    EXPECT_EQ(groups[index][device].executionIndex, index);
                    EXPECT_EQ(events[device]["pid"], "Device " + std::to_string(device) + " | BLOCK");
                    EXPECT_DOUBLE_EQ(events[device]["ts"].get<double>(), 1.0 + device * 100.0);
                }
            }
            EXPECT_FALSE(IsExist(directory + "/kernel*_range1/" + std::to_string(expectedCount)));
        }
    }
}

/**
 * | 用例集 | TimelineSummaryTest
 * |测试函数| DeviceDataParse::SaveSummary
 * | 用例名 | save_summary_csv_expect_device_first_and_rows_sorted_by_device_and_time
 * |用例描述| 汇总CSV将设备列置首，记录按设备和开始时间排序，同设备未知时间置后。
 */
TEST_F(TimelineSummaryTest, save_summary_csv_expect_device_first_and_rows_sorted_by_device_and_time) {
    auto parser = Parser();
    Collect(*parser, 1, 0, "other", 5);
    Collect(*parser, 0, 1, "missing", 0);
    Collect(*parser, 0, 2, "later", 20);
    Collect(*parser, 0, 3, "earlier", 10);
    ASSERT_TRUE(parser->SaveSummary(root_));
    std::ifstream csv(root_ + "/summary/summary.csv");
    ASSERT_TRUE(csv.is_open());
    std::string line;
    for (const std::string expected :
        {"Device Id,Op Name,Task Duration(us),", "0,earlier,2,", "0,later,2,", "0,missing,2,", "1,other,2,"}) {
        ASSERT_TRUE(static_cast<bool>(std::getline(csv, line)));
        EXPECT_EQ(line, expected);
    }
    EXPECT_FALSE(static_cast<bool>(std::getline(csv, line)));
}

/**
 * | 用例集 | TimelineSummaryTest
 * |测试函数| DeviceDataParse::MergeTimelineGroup
 * | 用例名 | merge_invalid_alignment_inputs_expect_no_result
 * |用例描述| 输入不足两条，或基准、频率、流水类型及JSON结构无效时返回空结果，不生成跨卡对齐流水。
 */
TEST_F(TimelineSummaryTest, merge_invalid_alignment_inputs_expect_no_result) {
    EXPECT_TRUE(DeviceDataParse::MergeTimelineGroup({}).is_null());
    EXPECT_TRUE(DeviceDataParse::MergeTimelineGroup({Input(0)}).is_null());
    std::vector<TimelineInput> invalid;
    auto input = Input(1);
    input.timeline.minCycle = UINT64_MAX;
    invalid.push_back(input);
    input = Input(1);
    input.cycleFreq = 0;
    invalid.push_back(input);
    input.cycleFreq = -1;
    invalid.push_back(input);
    input = Input(1);
    input.timeline.isCommunication = false;
    invalid.push_back(input);
    for (const json trace : {json(), json::array(), json::object(), json{{"traceEvents", "invalid"}},
             json{{"traceEvents", json::array()}}}) {
        input = Input(1);
        input.timeline.timelineJson = trace;
        invalid.push_back(input);
    }
    for (size_t index = 0; index < invalid.size(); ++index) {
        SCOPED_TRACE(index);
        EXPECT_TRUE(DeviceDataParse::MergeTimelineGroup({Input(0), invalid[index]}).is_null());
    }
}

/**
 * | 用例集 | TimelineSummaryTest
 * |测试函数| DeviceDataParse::MergeTimelineGroup
 * | 用例名 | merge_biu_events_expect_mhz_alignment_and_metadata_preserved
 * |用例描述| BIU流水按MHz频率和64位cycle差对齐，过滤无效事件，保留时长、args及元数据并添加设备前缀。
 */
TEST_F(TimelineSummaryTest, merge_biu_events_expect_mhz_alignment_and_metadata_preserved) {
    auto first = Input(0);
    auto second = Input(1);
    first.timeline.isCommunication = second.timeline.isCommunication = false;
    first.cycleFreq = second.cycleFreq = 1000;
    auto &events = second.timeline.timelineJson["traceEvents"];
    events[0]["args"] = {{"code", "work()"}};
    events.push_back(nullptr);
    events.push_back({{"ph", "X"}, {"pid", "BLOCK"}, {"ts", "invalid"}});
    const json metadata{
        {"ph", "M"}, {"pid", "BLOCK"}, {"name", "thread_name"}, {"ts", "unchanged"}, {"args", {{"name", "worker"}}}};
    events.push_back(metadata);
    const auto merged = DeviceDataParse::MergeTimelineGroup({second, first});
    ASSERT_FALSE(merged.is_null());
    const auto &result = merged.at("traceEvents");
    ASSERT_EQ(result.size(), 3U);
    EXPECT_EQ(result[0]["pid"], "Device 0 | BLOCK");
    EXPECT_DOUBLE_EQ(result[1]["ts"].get<double>(), 6.0);
    EXPECT_EQ(result[1]["dur"], 2);
    EXPECT_EQ(result[1]["args"]["code"], "work()");
    auto expectedMetadata = metadata;
    expectedMetadata["pid"] = "Device 1 | BLOCK";
    EXPECT_EQ(result[2], expectedMetadata);
}

/**
 * | 用例集 | TimelineSummaryTest
 * |测试函数| DeviceDataParse::MergeTimelines
 * | 用例名 | merge_invalid_group_expect_failure_and_later_group_saved
 * |用例描述| 单卡组被跳过、无效基准组使整体返回失败，后续合法组仍生成trace.json及visualize_data.bin。
 */
TEST_F(TimelineSummaryTest, merge_invalid_group_expect_failure_and_later_group_saved) {
    auto invalid = Input(0);
    invalid.timeline.minCycle = UINT64_MAX;
    EXPECT_FALSE(DeviceDataParse::MergeTimelines(
        {{Input(0)}, {invalid, Input(1)}, {Input(0, 1), Input(1, 1)}}, root_ + "/summary"));
    EXPECT_FALSE(IsExist(root_ + "/summary/kernel/0/trace.json"));
    EXPECT_TRUE(IsExist(root_ + "/summary/kernel/1/trace.json"));
    EXPECT_TRUE(IsExist(root_ + "/summary/kernel/1/visualize_data.bin"));
}

/**
 * | 用例集 | TimelineSummaryTest
 * |测试函数| DeviceDataParse::MergeTimelines
 * | 用例名 | merge_directory_failure_expect_later_group_still_saved
 * |用例描述| 已有文件占用首组输出目录时返回失败，继续处理并保存后续合法组的流水JSON。
 */
TEST_F(TimelineSummaryTest, merge_directory_failure_expect_later_group_still_saved) {
    ASSERT_TRUE(MkdirRecusively(root_ + "/summary/kernel"));
    ASSERT_TRUE(WriteFileByStream(root_ + "/summary/kernel/0", "blocked"));
    EXPECT_FALSE(
        DeviceDataParse::MergeTimelines({{Input(0), Input(1)}, {Input(0, 1), Input(1, 1)}}, root_ + "/summary"));
    EXPECT_TRUE(IsExist(root_ + "/summary/kernel/1/trace.json"));
}

/**
 * | 用例集 | TimelineSummaryTest
 * |测试函数| DeviceDataParse::MergeTimelines
 * | 用例名 | merge_trace_write_failure_expect_later_group_still_saved
 * |用例描述| 目录占用首组trace.json路径使写入失败时，不写该组bin，返回失败并继续保存后续组。
 */
TEST_F(TimelineSummaryTest, merge_trace_write_failure_expect_later_group_still_saved) {
    ASSERT_TRUE(MkdirRecusively(root_ + "/summary/kernel/0/trace.json"));
    EXPECT_FALSE(
        DeviceDataParse::MergeTimelines({{Input(0), Input(1)}, {Input(0, 1), Input(1, 1)}}, root_ + "/summary"));
    EXPECT_FALSE(IsExist(root_ + "/summary/kernel/0/visualize_data.bin"));
    EXPECT_TRUE(IsExist(root_ + "/summary/kernel/1/trace.json"));
}

/**
 * | 用例集 | TimelineSummaryTest
 * |测试函数| DeviceDataParse::ReadTimelineMstxPaths / AddTimelineMstxGroups
 * | 用例名 | missing_mstx_file_expect_no_groups
 * |用例描述| MSTX记录文件不存在时保留空记录集合，基于空记录配组不会生成合并组。
 */
TEST_F(TimelineSummaryTest, missing_mstx_file_expect_no_groups) {
    std::vector<TimelineMstxPaths> recorded(1);
    DeviceDataParse::ReadTimelineMstxPaths(root_, recorded);
    EXPECT_TRUE(recorded[0].empty());
    TimelineGroups groups;
    DeviceDataParse::AddTimelineMstxGroups(Config(1), recorded[0], {}, "test ", "kernel_range1", groups);
    EXPECT_TRUE(groups.empty());
}

/**
 * | 用例集 | TimelineSummaryTest
 * |测试函数| DeviceDataParse::AddTimelineMstxGroups
 * | 用例名 | mismatched_record_device_expect_skip_position_without_shifting
 * |用例描述| 记录设备号与单卡流水设备号不匹配时跳过该次执行，后续完整组保留原执行编号及设备归属。
 */
TEST_F(TimelineSummaryTest, mismatched_record_device_expect_skip_position_without_shifting) {
    const TimelineMstxPaths paths{{0, {"a", "b"}}, {1, {"wrong", "d"}}};
    const TimelineInputsByPath inputs{{"a", Input(0)}, {"b", Input(0)}, {"wrong", Input(0)}, {"d", Input(1)}};
    TimelineGroups groups;
    DeviceDataParse::AddTimelineMstxGroups(Config(2), paths, inputs, "test ", "kernel_range1", groups);
    ASSERT_EQ(groups.size(), 1U);
    ASSERT_EQ(groups[0].size(), 2U);
    EXPECT_EQ(groups[0][0].executionIndex, 1U);
    EXPECT_EQ(groups[0][1].deviceId, 1U);
}

/**
 * | 用例集 | TimelineSummaryTest
 * |测试函数| DeviceDataParse::SaveSummary
 * | 用例名 | summary_directory_failure_expect_failure_without_overwriting_existing_file
 * |用例描述| 已有文件占用summary目录路径时保存失败，原文件内容保持不变。
 */
TEST_F(TimelineSummaryTest, summary_directory_failure_expect_failure_without_overwriting_existing_file) {
    auto parser = Parser();
    Collect(*parser, 0, 0);
    Collect(*parser, 1, 0);
    ASSERT_TRUE(WriteFileByStream(root_ + "/summary", "blocked"));
    EXPECT_FALSE(parser->SaveSummary(root_));
    std::ifstream file(root_ + "/summary");
    std::string content;
    ASSERT_TRUE(static_cast<bool>(std::getline(file, content)));
    EXPECT_EQ(content, "blocked");
}

/**
 * | 用例集 | TimelineSummaryTest
 * |测试函数| DeviceDataParse::SaveSummary
 * | 用例名 | failed_timeline_merge_expect_summary_csv_still_saved
 * |用例描述| 无效起始cycle导致流水合并失败时不生成trace.json，但仍成功保存包含两卡完整记录的汇总CSV。
 */
TEST_F(TimelineSummaryTest, failed_timeline_merge_expect_summary_csv_still_saved) {
    auto parser = Parser();
    Collect(*parser, 0, 0);
    Collect(*parser, 1, 0);
    parser->timelineInputs_[0].timeline.minCycle = UINT64_MAX;
    ASSERT_TRUE(parser->SaveSummary(root_));
    EXPECT_FALSE(IsExist(root_ + "/summary/kernel/0/trace.json"));
    std::ifstream csv(root_ + "/summary/summary.csv");
    ASSERT_TRUE(csv.is_open());
    std::string line;
    ASSERT_TRUE(static_cast<bool>(std::getline(csv, line)));
    EXPECT_EQ(line, "Device Id,Op Name,Task Duration(us),");
    ASSERT_TRUE(static_cast<bool>(std::getline(csv, line)));
    EXPECT_EQ(line, "0,kernel,2,");
    ASSERT_TRUE(static_cast<bool>(std::getline(csv, line)));
    EXPECT_EQ(line, "1,kernel,2,");
    EXPECT_FALSE(static_cast<bool>(std::getline(csv, line)));
}

/**
 * | 用例集 | TimelineSummaryTest
 * |测试函数| DeviceDataParse::RecordSummary
 * | 用例名 | explicit_biu_input_expect_aicore_frequency_without_communication_flag
 * |用例描述| 启用显式合并配置时，非通信BIU流水按采集路径保存并使用AICore频率，不进入默认自动配组集合。
 */
TEST_F(TimelineSummaryTest, explicit_biu_input_expect_aicore_frequency_without_communication_flag) {
    const json selection{{"kernel_name", "kernel*"}, {"mstx_name", "range1"}, {"rank_count", 2}, {"launch_count", 1}};
    ASSERT_TRUE(
        WriteFileByStream(root_ + "/config.json", json({{"timeline_merge_config", json::array({selection})}}).dump()));
    std::string error;
    ASSERT_TRUE(CustomInputConfig::Instance().Load(root_ + "/config.json", error)) << error;
    auto parser = Parser();
    DataHandler handler;
    handler.opBasicHeader = {"Op Name", "Device Id"};
    handler.opInfoMap_ = {{"Op Name", "kernel"}, {"Device Id", "0"}};
    handler.aicoreFreq_ = 1650;
    handler.taskSchedulerFreq_ = 50000;
    auto timeline = Input(0).timeline;
    timeline.isCommunication = false;
    parser->RecordSummary(handler, "device0/kernel/0", timeline);
    ASSERT_EQ(parser->timelineInputsByPath_.size(), 1U);
    EXPECT_EQ(parser->timelineInputsByPath_.at("device0/kernel/0").cycleFreq, 1650);
    EXPECT_FALSE(parser->timelineInputsByPath_.at("device0/kernel/0").timeline.isCommunication);
    EXPECT_TRUE(parser->timelineInputs_.empty());
}
