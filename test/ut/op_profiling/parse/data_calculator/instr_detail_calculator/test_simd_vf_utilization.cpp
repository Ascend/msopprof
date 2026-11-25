/* -------------------------------------------------------------------------
 *  This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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
#include <set>
#include <tuple>
#include <thread>
#include "parse/data_calculator/instr_detail_calculator/simd_vf_utilization_calculator.h"
#include <experimental/filesystem>
#include <fstream>
#include <limits>
#include <cstring>
#include <cstdlib>
#include "parse/data_parser/instr_parser/real_time_instr_parser.h"
#include "parse/data_parser/real_time_data_parser.h"
#include "profiling/simulator/data_parse/api_data.h"
#include "parse/data_parser/sim_dump_parser.h"
#include "parse/data_parser/instr_parser/instr_parser.h"
#include "parse/data_parser/instr_parser/instr_log_parser.h"
#include "parse/data_visualizer/hotspotmap_visualizer/sim_pc_to_code.h"
#include "parse/data_visualizer/hotspotmap_visualizer/sim_code_to_pc.h"

using namespace Profiling;
using namespace Profiling::Parse;

namespace {
std::string Detail(uint64_t bytes)
{
    return "{\"processed_bytes\":" + std::to_string(bytes) + "}";
}
SimdVfUtilization RunCalculator(const std::string &detail,
    const std::string &name = "RV_VADD", const std::string &pipe = "RVECEX")
{
    SimDataParserConfig parserConfig{"core0.veccore0", {}, false, false, ChipProductType::ASCEND950PR_9599};
    auto parser = std::make_shared<InstrLogParser>(parserConfig);
    InstrParseInfo instr{};
    instr.id = 1;
    instr.pc = 0x1234;
    instr.tick = 100;
    instr.name = name;
    instr.pipe = pipe;
    instr.detail = detail;
    parser->ParseRealTimeDumpLog(instr, MatchMode::ID_MATCH);
    MergeInfo row{};
    row.id = instr.id;
    row.pc = instr.pc;
    row.name = name;
    row.pipe = pipe;
    row.startTick = 99;
    row.endTick = 100;
    std::vector<MergeInfo> rows{row, row}; // Duplicate rows must contribute one execution.
    DataCenter center;
    if (!center.DataTableRegister(parser) ||
        !center.DataTableRegister(std::make_shared<InstrDetailTable>(rows))) {
        ADD_FAILURE() << "Failed to prepare calculator input";
        return {};
    }
    InstrDetailConfig config{ChipProductType::ASCEND950PR_9599};
    SimdVfUtilizationCalculator calculator{center, config};
    EXPECT_EQ(calculator.Run(), PluginErrorCode::SUCCESS);
    const auto result = center.GetDbPtr<SimdVfResultTable>();
    return result == nullptr ? SimdVfUtilization{} : result->byPc.at(row.pc);
}
class SimdVfPipeline : public testing::Test {
protected:
    void SetUp() override
    {
        char pattern[] = "/tmp/msopprof-simd-XXXXXX";
        const auto created = mkdtemp(pattern);
        ASSERT_NE(created, nullptr);
        path = created;
        parser.Start();
    }
    void TearDown() override
    {
        parser.Stop();
        if (!path.empty()) {
            std::experimental::filesystem::remove_all(path);
        }
    }
    InstrParseInfo MakeInstr(uint64_t id, uint64_t tick, uint64_t pc) {
        InstrParseInfo instr{};
        instr.tick = tick;
        instr.pc = pc;
        instr.id = id;
        instr.pipe = "RVECEX";
        instr.name = "RV_VADD";
        instr.binary = Encode::InstrDisassembler::ParseBinary("0x80082700");
        return instr;
    }
    void Send(RealTimeInstrParser &parser, uint64_t bytes, uint64_t pc = 0x1234, uint64_t tick = 100,
        const std::string &detail = "", uint64_t id = 10, uint32_t coreId = 0, bool paired = true) {
        if (paired && paired_.emplace(coreId, id, pc).second) {
            SendPop(parser, id, tick > 0 ? tick - 1 : 0, coreId, pc);
        }
        auto instr = MakeInstr(id, tick, pc);
        instr.detail = detail.empty() ? Detail(bytes) : detail;
        parser.SetInstrLog({instr, "core" + std::to_string(coreId) + ".veccore0"});
    }
    void SendPop(RealTimeInstrParser &parser, uint64_t id, uint64_t tick, uint32_t coreId = 0, uint64_t pc = 0x1234) {
        auto instr = MakeInstr(id, tick, pc);
        PoppedInstrParseInfo popped(instr);
        parser.SetPopInstrLog(PoppedInstrParseInfoForRealTime(popped, "core" + std::to_string(coreId) + ".veccore0"));
    }
    void Finish(RealTimeInstrParser &parser, std::map<std::string, std::shared_ptr<DataCenter>> &centers) {
        parser.Stop(); // Both queues must be drained before the calculators read completion records.
        parser.Merge(centers);
        paired_.clear();
        CalCulate(centers, nullptr, context.metricsConfig, false, context.chipType);
    }
    RealTimeSimParseContext context{{}, false, ChipProductType::ASCEND950PR_9599, Common::ProfMetricsAbilityConfig{}};
    RealTimeInstrParser parser{context};
    std::map<std::string, std::shared_ptr<DataCenter>> centers;
    std::string path;
    std::set<std::tuple<uint32_t, uint64_t, uint64_t>> paired_;
};
}

TEST(SimdVfUtilization, DividesValidBytesWithoutIntegerTruncation)
{
    for (uint64_t bytes : {0U, 1U, 128U, 255U, 256U}) {
        const auto value = RunCalculator(Detail(bytes));
        EXPECT_FALSE(value.invalid);
        EXPECT_EQ(value.processedBytes, bytes);
        EXPECT_EQ(value.capacityBytes, 256U);
        EXPECT_FLOAT_EQ(value.Percentage(), 100.0f * bytes / 256);
    }
}
TEST(SimdVfUtilization, MissingMalformedAndInvalidValuesAreUnavailable)
{
    for (const std::string json : {"", "{}", "{bad", "[]", "null", R"({"processed_bytes":null})",
             R"({"processed_bytes":true})", R"({"processed_bytes":"128"})", R"({"processed_bytes":128.0})",
             R"({"processed_bytes":-1})", R"({"processed_bytes":257})", R"({"nested":{"processed_bytes":128}})"}) {
        EXPECT_FLOAT_EQ(RunCalculator(json).Percentage(), -1.0f) << json;
    }
}
TEST(SimdVfUtilization, OnlyConfirmedSimdComputationInstructionsContribute)
{
    for (const std::string name : {"RV_PLT", "RV_PLTM", "RV_VDUP", "RV_VMOV", "RV_VNEW"}) {
        const auto value = RunCalculator(R"({"queue_type":"SIMD","vf_class":"EXECUTION","processed_bytes":256})", name);
        EXPECT_EQ(value.capacityBytes, 0U) << name;
        EXPECT_FALSE(value.invalid) << name;
    }
    EXPECT_EQ(RunCalculator(Detail(256), "RV_VADD", "SIMT").capacityBytes, 0U);
    EXPECT_EQ(RunCalculator(Detail(256), "RV_VLD", "RVECLD").capacityBytes, 0U);
}
TEST(SimdVfUtilization, ArithmeticConversionAndReductionAreIncluded)
{
    for (const std::string name : {"RV_VADDC", "RV_WMUL", "RV_VCVTFI", "RV_VCADD", "RV_VCGMAX", "RV_VBCNT"}) {
        EXPECT_FLOAT_EQ(RunCalculator(Detail(64), name).Percentage(), 25.0f) << name;
    }
}
TEST(SimdVfUtilization, ExplicitNonSimdAndNonExecutionPoliciesAreExcluded)
{
    for (const std::string json : {R"({"queue_type":"SIMT","processed_bytes":256})",
             R"({"vf_class":"LOAD_STORE","processed_bytes":256})"}) {
        const auto value = RunCalculator(json);
        EXPECT_EQ(value.capacityBytes, 0U);
        EXPECT_FALSE(value.invalid);
    }
    EXPECT_TRUE(RunCalculator(R"({"queue_type":1,"processed_bytes":256})").invalid);
    EXPECT_TRUE(RunCalculator(R"({"vf_class":null,"processed_bytes":256})").invalid);
}
TEST(SimdVfUtilization, AggregatesIntegerTotalsBeforePercentage)
{
    auto value = RunCalculator(Detail(128));
    value.Add(RunCalculator(Detail(256)));
    EXPECT_EQ(value.processedBytes, 384U);
    EXPECT_EQ(value.capacityBytes, 512U);
    EXPECT_FLOAT_EQ(value.Percentage(), 75.0f);
    auto invalid = value;
    invalid.Add(RunCalculator("{}"));
    EXPECT_FLOAT_EQ(invalid.Percentage(), -1.0f);
    value.processedBytes = std::numeric_limits<uint64_t>::max();
    value.Add(RunCalculator(Detail(1)));
    EXPECT_FLOAT_EQ(value.Percentage(), -1.0f);
}
TEST(SimdVfUtilization, SerializesOnlyTheMainFloatingPointColumn)
{
    Serialization::InstrInfo instruction{};
    instruction.vecUtilization.push_back(RunCalculator(Detail(1)).Percentage());
    nlohmann::json json;
    instruction.ToJson(json, false, false);
    size_t columns = 0;
    for (auto iter = json.begin(); iter != json.end(); ++iter) {
        if (iter.key().find("Vector Utilization") != std::string::npos) {
            ++columns;
        }
    }
    EXPECT_EQ(columns, 1U);
    const auto &value = json.at("Vector Utilization Percentage").at(0);
    EXPECT_TRUE(value.is_number_float());
    EXPECT_FLOAT_EQ(value.get<float>(), 0.390625f);
}

TEST_F(SimdVfPipeline, NewTaskDoesNotReusePreviousKernelTotals)
{
    Send(parser, 256);
    std::map<std::string, std::shared_ptr<DataCenter>> first;
    Finish(parser, first);
    parser.Start();
    Send(parser, 128);
    std::map<std::string, std::shared_ptr<DataCenter>> second;
    Finish(parser, second);
    EXPECT_FLOAT_EQ(first.at("core0.veccore0")->GetDbPtr<SimdVfResultTable>()->byPc.at(0x1234).Percentage(), 100.0f);
    EXPECT_FLOAT_EQ(second.at("core0.veccore0")->GetDbPtr<SimdVfResultTable>()->byPc.at(0x1234).Percentage(), 50.0f);
}
TEST_F(SimdVfPipeline, UnpairedCoreDoesNotCreateTables) {
    Send(parser, 256, 0x1234, 100, "", 10, 0, false);
    Send(parser, 64, 0x1238, 110, "", 11, 0, false);
    Finish(parser, centers);
    EXPECT_TRUE(centers.empty());
}

TEST_F(SimdVfPipeline, PcValuesRespectProductsAndPreserveTimingStatistics) {
    std::vector<MergeInfo> rows(2);
    for (auto &row : rows) {
        row.pc = 0x1234;
        row.startTick = 100;
        row.endTick = 102;
        row.icacheTick = row.ccuTick = UINT64_MAX;
        row.vecUtilization = 25.0f;
        row.gprCount = 7;
        row.ubReadConflict = 3;
        row.ubWriteConflict = 4;
    }
    auto metrics = std::make_shared<SimdVfResultTable>();
    metrics->byPc[0x1234] = RunCalculator(Detail(192));
    auto cores = std::make_shared<std::map<std::string, SimData>>();
    cores->emplace("core0.veccore0", SimData{std::make_shared<InstrDetailTable>(rows), nullptr, nullptr, metrics});
    DataCenter center;
    ASSERT_TRUE(center.DataTableRegister(cores));
    Pc2CodeMap mapping;
    std::vector<std::string> names{"core0.veccore0"};
    for (auto chip : {ChipProductType::ASCEND910B_SERIES, context.chipType}) {
        SimVisualizerConfig config{path, mapping, 1, chip};
        SimPcToCode pc{config, center, names};
        pc.CalCulate();
        const auto instructions = pc.GetInstrInfo();
        ASSERT_EQ(instructions.size(), 1U);
        const auto &row = instructions[0];
        EXPECT_FLOAT_EQ(row.vecUtilization[0], chip == context.chipType ? 75.0f : 25.0f);
        EXPECT_EQ(row.callCount[0], 2);
        EXPECT_EQ(row.cycles[0], 4);
        EXPECT_EQ(row.gprCount[0], 7);
        EXPECT_EQ(row.ubReadConflict[0], 3);
        EXPECT_EQ(row.ubWriteConflict[0], 4);
    }
}

TEST_F(SimdVfPipeline, AccumulatesExecutionsWithoutMixingCoreIds)
{
    Send(parser, 0, 0x1234, 100, "", 10);
    Send(parser, 0, 0x1234, 101, "", 11);
    Send(parser, 256, 0x1234, 110, "", 10);
    Send(parser, 128, 0x1234, 111, "", 11);
    Send(parser, 256, 0x1234, 111, "", 10, 1);
    Finish(parser, centers);
    const auto &value = centers.at("core0.veccore0")->GetDbPtr<SimdVfResultTable>()->byPc.at(0x1234);
    EXPECT_EQ(value.processedBytes, 384U);
    EXPECT_EQ(value.capacityBytes, 512U);
    EXPECT_FLOAT_EQ(value.Percentage(), 75.0f);
    ASSERT_EQ(centers.size(), 2U);
    EXPECT_FLOAT_EQ(centers.at("core1.veccore0")->GetDbPtr<SimdVfResultTable>()->byPc.at(0x1234).Percentage(), 100.0f);
}

TEST_F(SimdVfPipeline, LastRecordWinsEvenWhenBytesDecreaseOrBecomeUnavailable)
{
    Send(parser, 256, 0x1234, 100, "", 10);
    Send(parser, 128, 0x1234, 110, "", 10);
    Send(parser, 256, 0x1238, 100, "", 11);
    Send(parser, 0, 0x1238, 110, "", 11);
    Send(parser, 256, 0x1240, 100, "", 12);
    Send(parser, 0, 0x1240, 110, "{}", 12);
    Send(parser, 0, 0x1248, 100, "{}", 13);
    Send(parser, 256, 0x1248, 110, "", 13);
    Finish(parser, centers);
    const auto table = centers.at("core0.veccore0")->GetDbPtr<SimdVfResultTable>();
    EXPECT_FLOAT_EQ(table->byPc.at(0x1234).Percentage(), 50.0f);
    EXPECT_FLOAT_EQ(table->byPc.at(0x1238).Percentage(), 0.0f);
    EXPECT_FLOAT_EQ(table->byPc.at(0x1240).Percentage(), -1.0f);
    EXPECT_FLOAT_EQ(table->byPc.at(0x1248).Percentage(), 100.0f);
}

TEST_F(SimdVfPipeline, ConcurrentProducersAreDrainedBeforeFinalCalculation) {
    auto writer = [&](uint64_t firstId) {
        for (uint64_t i = 0; i < 32; ++i) {
            const auto id = firstId + i;
            SendPop(parser, id, 99 + i);
            Send(parser, 0, 0x1234, 100 + i, "", id, 0, false);
            Send(parser, 128, 0x1234, 100 + i, "", id, 0, false);
        }
    };
    std::thread first(writer, 1000);
    std::thread second(writer, 2000);
    first.join();
    second.join();
    Finish(parser, centers);
    const auto &total = centers.at("core0.veccore0")->GetDbPtr<SimdVfResultTable>()->byPc.at(0x1234);
    EXPECT_EQ(total.processedBytes, 64U * 128U);
    EXPECT_EQ(total.capacityBytes, 64U * 256U);
    EXPECT_FLOAT_EQ(total.Percentage(), 50.0f);
}

TEST(SimdVfStream, ClosedQueueDrainsEveryAcceptedRecordInOrder) {
    DataStreamImpl<uint64_t> stream;
    for (uint64_t i = 0; i < 1024; ++i) {
        stream.Push(i);
    }
    stream.Shutdown();
    uint64_t value = 0;
    for (uint64_t i = 0; i < 1024; ++i) {
        ASSERT_TRUE(stream.Pop(value));
        EXPECT_EQ(value, i);
    }
    EXPECT_FALSE(stream.Pop(value));
    stream.Push(9999); // The closed stream must reject a late producer.
    EXPECT_FALSE(stream.Pop(value));
}


TEST_F(SimdVfPipeline, UnpairedIdsAtAnExistingPcDoNotContributeOrInvalidate) {
    Send(parser, 128, 0x1234, 100, "", 10);
    Send(parser, 256, 0x1234, 110, "", 11, 0, false);
    Send(parser, 0, 0x1234, 120, "{}", 12, 0, false);
    Send(parser, 256, 0x1238, 130, "", 13, 0, false);
    Finish(parser, centers);
    const auto table = centers.at("core0.veccore0")->GetDbPtr<SimdVfResultTable>();
    ASSERT_NE(table, nullptr);
    ASSERT_EQ(table->byPc.size(), 1U);
    EXPECT_EQ(table->byPc.at(0x1234).processedBytes, 128U);
    EXPECT_EQ(table->byPc.at(0x1234).capacityBytes, 256U);
    EXPECT_FLOAT_EQ(table->byPc.at(0x1234).Percentage(), 50.0f);
}

TEST_F(SimdVfPipeline, SharedPcDoesNotBorrowAPairFromAnotherCore) {
    Send(parser, 256, 0x1234, 120, "", 10, 0, false);
    SendPop(parser, 10, 100, 1);
    Send(parser, 192, 0x1234, 120, "", 10, 1, false);
    Finish(parser, centers);
    auto combined = std::make_shared<DataCenter>();
    ASSERT_TRUE(CombineCoreData(centers, combined));
    Pc2CodeMap mapping;
    std::vector<std::string> names{"core0.veccore0", "core1.veccore0"};
    SimVisualizerConfig config{path, mapping, 2, ChipProductType::ASCEND950PR_9599};
    SimPcToCode pc{config, *combined, names};
    pc.CalCulate();
    const auto rows = pc.GetInstrInfo();
    ASSERT_EQ(rows.size(), 1U);
    EXPECT_FLOAT_EQ(rows[0].vecUtilization[0], -1.0f);
    EXPECT_FLOAT_EQ(rows[0].vecUtilization[1], 75.0f);
    EXPECT_EQ(rows[0].cycles[0], 0);
    EXPECT_EQ(rows[0].callCount[0], 0);
    EXPECT_EQ(rows[0].cycles[1], 20);
    EXPECT_EQ(rows[0].callCount[1], 1);
}

TEST_F(SimdVfPipeline, RejectedTimingDoesNotBecomeAnEligibleExecution) {
    SendPop(parser, 10, 200);
    Send(parser, 256, 0x1234, 100, "", 10, 0, false);
    Finish(parser, centers);
    ASSERT_EQ(centers.size(), 1U);
    EXPECT_EQ(centers.begin()->second->GetDbPtr<InstrDetailTable>(), nullptr);
    EXPECT_EQ(centers.begin()->second->GetDbPtr<SimdVfResultTable>(), nullptr);
}

TEST_F(SimdVfPipeline, LastMatchingCallbackIsUsedWhenAnIdContainsOtherInstructions) {
    Send(parser, 0, 0x1234, 100);
    Send(parser, 256, 0x1234, 110);
    Send(parser, 128, 0x1238, 120, "", 10, 0, false);
    InstrParseInfo end{};
    end.id = 10;
    end.pc = 0x11111111;
    end.tick = 130;
    end.name = "END_LABEL";
    end.pipe = "FLOWCTRL";
    parser.SetInstrLog({end, "core0.veccore0"});
    Finish(parser, centers);
    const auto table = centers.at("core0.veccore0")->GetDbPtr<SimdVfResultTable>();
    ASSERT_NE(table, nullptr);
    ASSERT_EQ(table->byPc.size(), 1U);
    EXPECT_FLOAT_EQ(table->byPc.at(0x1234).Percentage(), 100.0f);
}

TEST_F(SimdVfPipeline, PublicCalculationDispatcherKeepsA2OffTheA5Calculator) {
    Send(parser, 256);
    parser.Stop();
    parser.Merge(centers);
    ASSERT_EQ(centers.size(), 1U);
    CalCulate(centers, nullptr, context.metricsConfig, false, ChipProductType::ASCEND910B_SERIES);
    EXPECT_EQ(centers.begin()->second->GetDbPtr<SimdVfResultTable>(), nullptr);
}

namespace {
template<class Record, class Consumer, class Result, class Fill, class Verify>
void CheckDrainedConsumer(Fill fill, Verify verify)
{
    DataCenter center;
    ASSERT_TRUE(center.DataStreamRegister<Record>());
    auto stream = center.GetStreamPtr<Record>();
    for (uint64_t i = 0; i < 512; ++i) {
        Record info{};
        fill(info, i);
        stream->Push(info);
    }
    stream->Shutdown();
    Consumer consumer{center, ChipProductType::ASCEND910B_SERIES};
    ASSERT_EQ(consumer.Run(), PluginErrorCode::SUCCESS);
    const auto result = center.GetDbPtr<Result>();
    ASSERT_NE(result, nullptr);
    verify(*result);
}
}

TEST(SimdVfStream, IcacheAndMteAndCcuConsumersDrainAfterShutdown) {
    CheckDrainedConsumer<IcacheParseInfoForRealTime, RealTimeICacheParserPlugin,
        std::map<std::string, scalarHeadCache>>([](IcacheParseInfoForRealTime &info, uint64_t i) {
            info.tick = 100 + i;
            info.pc = 0x1000;
            info.coreName = "core0.veccore0";
            info.opType = "hit_read";
        }, [](const std::map<std::string, scalarHeadCache> &result) { EXPECT_EQ(result.at("core0.veccore0").at(0x1000).size(), 512U); });
    CheckDrainedConsumer<Common::DvcMteLog, RealTimeMteParserPlugin,
        std::vector<MteLogInstrMap>>([](Common::DvcMteLog &info, uint64_t i) {
            info.time = 100 + i;
            info.instrId = i + 1;
            info.reqId = 1;
            info.size = 32;
            std::memcpy(info.intf, "BRIF", sizeof("BRIF"));
        }, [](const std::vector<MteLogInstrMap> &result) {
            ASSERT_FALSE(result.empty());
            EXPECT_EQ(result.at(0).size(), 512U);
            EXPECT_DOUBLE_EQ(result.at(0).at(512).reqTbl.at(1).dataSize, 32.0);
        });
    CheckDrainedConsumer<CcuParseInfoForRealTime, RealTimeCcuParserPlugin,
        std::map<std::string, scalarHead>>([](CcuParseInfoForRealTime &info, uint64_t i) {
            info.tick = 100 + i;
            info.pc = 0x1000;
            info.coreName = "core0.veccore0";
        }, [](const std::map<std::string, scalarHead> &result) {
            EXPECT_EQ(result.at("core0.veccore0").at(0x1000).size(), 512U);
            EXPECT_EQ(result.at("core0.veccore0").at(0x1000).back().ccuTick, 611U);
        });
}

namespace {
class SimdVfOffline : public SimdVfPipeline {
protected:
    std::string DumpLine(uint64_t tick, uint64_t id, const std::string &detail) {
        return "[info] [" + std::to_string(tick) + "] (PC: 0x1234) RVECEX : (Binary: 0x80082700) (ID: " +
            std::to_string(id) + ") RV_VADD  " + detail + "\n";
    }
    void WriteLogs(const std::string &finalDetail, const std::string &extra = "", bool paired = true) {
        std::ofstream(path + "/core0.veccore0.instr_log.dump") <<
            DumpLine(100, 10, Detail(0)) << DumpLine(110, 10, finalDetail) << extra;
        std::ofstream(path + "/core0.veccore0.instr_popped_log.dump") <<
            (paired ? DumpLine(99, 10, "") : "");
        std::experimental::filesystem::create_directories(path + "/core0.veccore0");
    }
    bool ParseFiles(std::shared_ptr<DataCenter> &combined) {
        SimParseContext context{};
        context.dumpPath = path;
        context.outputPath = path;
        context.chipType = ChipProductType::ASCEND950PR_9599;
        context.coresNamePair.emplace("core0.veccore0", "core0.veccore0.");
        combined = std::make_shared<DataCenter>();
        std::shared_ptr<Pc2CodeMap> mapping;
        return ParseDumpFile(context, combined, mapping, 2);
    }
};
}

TEST_F(SimdVfOffline, FileEntryMatchesRealtimeFinalSnapshotAndHotspot) {
    WriteLogs(Detail(128));
    std::shared_ptr<DataCenter> offline;
    ASSERT_TRUE(ParseFiles(offline));
    const auto cores = offline->GetDbPtr<std::map<std::string, SimData>>();
    ASSERT_NE(cores, nullptr);
    ASSERT_EQ(cores->size(), 1U);
    const auto table = cores->at("core0.veccore0").simdVf;
    ASSERT_NE(table, nullptr);
    EXPECT_EQ(table->byPc.at(0x1234).processedBytes, 128U);
    EXPECT_EQ(table->byPc.at(0x1234).capacityBytes, 256U);

    Send(parser, 0);
    Send(parser, 128, 0x1234, 110);
    Finish(parser, centers);
    const auto realtime = centers.at("core0.veccore0")->GetDbPtr<SimdVfResultTable>();
    ASSERT_NE(realtime, nullptr);
    EXPECT_FLOAT_EQ(table->byPc.at(0x1234).Percentage(), realtime->byPc.at(0x1234).Percentage());
    Pc2CodeMap mapping;
    SimVisualizerConfig config{path, mapping, 1, context.chipType};
    std::vector<std::string> names{"core0.veccore0"};
    SimPcToCode pc{config, *offline, names};
    pc.CalCulate();
    const auto rows = pc.GetInstrInfo();
    ASSERT_EQ(rows.size(), 1U);
    EXPECT_FLOAT_EQ(rows[0].vecUtilization.at(0), 50.0f);
    EXPECT_EQ(rows[0].callCount.at(0), 1);
}

TEST_F(SimdVfOffline, FileEntryHandlesUnpairedAndInvalidRecords) {
    struct Case {
        std::string detail;
        std::string extra;
        bool paired;
        float percentage;
    };
    const Case cases[] = {
        {Detail(128), DumpLine(120, 11, Detail(256)) + DumpLine(130, 12, "{}"), true, 50.0f},
        {"{}", "", true, -1.0f},
        {Detail(256), "", false, -1.0f},
    };
    for (const auto &item : cases) {
        SCOPED_TRACE(item.detail + item.extra + std::to_string(item.paired));
        WriteLogs(item.detail, item.extra, item.paired);
        std::shared_ptr<DataCenter> combined;
        ASSERT_TRUE(ParseFiles(combined));
        const auto cores = combined->GetDbPtr<std::map<std::string, SimData>>();
        ASSERT_NE(cores, nullptr);
        if (!item.paired) {
            EXPECT_TRUE(cores->empty());
            continue;
        }
        ASSERT_EQ(cores->size(), 1U);
        const auto table = cores->at("core0.veccore0").simdVf;
        ASSERT_NE(table, nullptr);
        const auto &metric = table->byPc.at(0x1234);
        EXPECT_FLOAT_EQ(metric.Percentage(), item.percentage);
        EXPECT_EQ(metric.invalid, item.percentage < 0);
        if (!metric.invalid) {
            EXPECT_EQ(metric.processedBytes, 128U);
            EXPECT_EQ(metric.capacityBytes, 256U);
        }
    }
}

TEST_F(SimdVfOffline, LegacyFileParserDoesNotPublishA5RawInput) {
    WriteLogs(Detail(128));
    DataCenter center;
    SimDataParserConfig config{path, {"core0.veccore0", "core0.veccore0."}, {}, false, false,
        ChipProductType::ASCEND910B_SERIES};
    InstrParser parser{center, config};
    ASSERT_EQ(parser.Run(), PluginErrorCode::SUCCESS);
    EXPECT_NE(center.GetDbPtr<InstrDetailTable>(), nullptr);
    EXPECT_EQ(center.GetDbPtr<InstrLogParser>(), nullptr);
    EXPECT_EQ(center.GetDbPtr<SimdVfResultTable>(), nullptr);
}

TEST_F(SimdVfPipeline, SourceLinesWeightUniquePcsAndSerializeZeroAndInvalid) {
    const auto source = path + "/kernel.cpp";
    std::ofstream(source) << "void a() {}\nvoid b() {}\nvoid c() {}\n";
    std::experimental::filesystem::create_directories(path + "/core0.veccore0");
    std::vector<MergeInfo> rows(6);
    auto metrics = std::make_shared<SimdVfResultTable>();
    Pc2CodeMap mapping;
    for (size_t i = 0; i < rows.size(); ++i) {
        rows[i].pc = i == 0 ? 0x1234 : (i < 4 ? 0x1238 : 0x1240 + (i - 4) * 4);
        rows[i].startTick = 100;
        rows[i].endTick = 110;
        rows[i].icacheTick = rows[i].ccuTick = UINT64_MAX;
    }
    metrics->byPc[0x1234] = RunCalculator(Detail(256));
    for (size_t i = 0; i < 3; ++i) {
        metrics->byPc[0x1238].Add(RunCalculator(Detail(128)));
    }
    metrics->byPc[0x1240] = RunCalculator(Detail(0));
    metrics->byPc[0x1244] = RunCalculator("{}");
    mapping.Insert({0x1234, {source + ":1"}});
    mapping.Insert({0x1238, {source + ":1"}});
    mapping.Insert({0x1240, {source + ":2", source + ":3"}});
    mapping.Insert({0x1244, {source + ":3"}});
    auto cores = std::make_shared<std::map<std::string, SimData>>();
    cores->emplace("core0.veccore0", SimData{std::make_shared<InstrDetailTable>(rows), nullptr, nullptr, metrics});
    DataCenter center;
    ASSERT_TRUE(center.DataTableRegister(cores));
    std::vector<std::string> names{"core0.veccore0", "core1.veccore0"};
    SimVisualizerConfig config{path, mapping, 1, context.chipType};
    SimCodeToPc code{config, center, names};
    code.CalCulate();
    const auto files = code.GetCodeFile();
    ASSERT_EQ(files.size(), 1U);
    ASSERT_EQ(files[0].lines.size(), 3U);
    const float expected[] = {62.5f, 0.0f, -1.0f};
    for (size_t i = 0; i < 3; ++i) {
        EXPECT_FLOAT_EQ(files[0].lines[i].vecUtilization.at(0), expected[i]);
        EXPECT_FLOAT_EQ(files[0].lines[i].vecUtilization.at(1), -1.0f);
    }
    EXPECT_EQ(files[0].lines[0].callCount.at(0), 4);
    EXPECT_EQ(files[0].lines[0].cycles.at(0), 40);
    Serialization::CodeInstrData data{false, false, names, files, {}};
    nlohmann::json json;
    ASSERT_TRUE(Serialization::ApiData::FileStats(data, path, json));
    Serialization::ApiData::FileDtypeStats(json);
    EXPECT_EQ(json["Files Dtype"]["Lines"]["Vector Utilization Percentage"], 4);
    EXPECT_EQ(json["Files"][0]["Lines"][0]["Instructions Executed"][0], 4);
    for (size_t i = 0; i < 3; ++i) {
        EXPECT_FLOAT_EQ(json["Files"][0]["Lines"][i]["Vector Utilization Percentage"][0].get<float>(), expected[i]);
    }
}
