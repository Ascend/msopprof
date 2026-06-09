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

#include <fstream>
#include <string>
#include "smart_pointer.h"
#define private public
#define protected public
#include "common/dbi_defs.h"
#include "profiling/device/data_visualize/warp_timeline_visualize.h"
#include "profiling/device/data_visualize/data_visualize.h"
#include "profiling/device/data_visualize/occupancy.h"
#include "profiling/device/data_visualize/pmu_calculator.h"
#include "profiling/device/data_visualize/timeline_parser/mc2_timeline_parser.h"
#include "profiling/device/data_visualize/timeline_parser/lccl_timeline_parser.h"
#include "common/hal_helper.h"
#undef private
#undef protected

using namespace Visualize;
using namespace Profiling;
using namespace Common;
using namespace Utility;
using namespace std;

namespace {
constexpr int64_t TEST_AICORE_FREQ = 1000;
constexpr uint32_t TEST_CORE_TYPE_VEC0 = 0U;
constexpr uint32_t TEST_CORE_TYPE_VEC1 = 1U;
constexpr uint32_t TEST_CORE_TYPE_CUBE0 = 2U;

class ScopedAicoreFreqMock {
public:
    ScopedAicoreFreqMock() : oldFunc_(HalHelper::Instance().halGetDeviceInfo_) {
        HalHelper::Instance().halGetDeviceInfo_ = [](uint32_t, int32_t, int32_t, int64_t *value) {
            if (value != nullptr) {
                *value = TEST_AICORE_FREQ;
            }
            return DRV_ERROR_NONE;
        };
    }

    ~ScopedAicoreFreqMock() { HalHelper::Instance().halGetDeviceInfo_ = oldFunc_; }

private:
    halGetDeviceInfoFunc oldFunc_;
};

void WriteWarpTimelineBin(const std::string &outputPath, const std::vector<std::pair<uint64_t, uint64_t>> &warpTimes,
                          uint32_t coreId = 0, uint32_t coreType = TEST_CORE_TYPE_VEC0, bool append = false) {
    const std::string dumpPath = JoinPath({outputPath, "dump"});
    ASSERT_TRUE(MkdirRecusively(dumpPath));

    BlockWarpRecords blockRecords{};
    blockRecords.header.magicWords = DBI_RECORD_MAGIC_WORDS;
    blockRecords.header.warpCount = static_cast<uint32_t>(warpTimes.size());
    blockRecords.header.coreId = coreId;
    blockRecords.header.coreType = coreType;
    for (size_t warpId = 0; warpId < warpTimes.size() && warpId < WARP_NUM_PER_BLOCK; ++warpId) {
        blockRecords.records[warpId].startTime = warpTimes[warpId].first;
        blockRecords.records[warpId].endTime = warpTimes[warpId].second;
    }

    const std::string warpBinPath = JoinPath({dumpPath, WARP_TIMELINE});
    std::ofstream output(warpBinPath, std::ios::binary | (append ? std::ios::app : std::ios::trunc));
    ASSERT_TRUE(output.is_open());
    output.write(reinterpret_cast<const char *>(&blockRecords), sizeof(blockRecords));
    ASSERT_TRUE(output.good());
}

nlohmann::json MakeTracePayload(const std::string &profilingType, const std::string &eventName) {
    nlohmann::json payload;
    payload["profilingType"] = profilingType;
    payload["displayTimeUnit"] = "ns";
    payload["schemaVersion"] = 1;
    payload["traceEvents"] = nlohmann::json::array(
        {nlohmann::json{{"name", eventName}, {"ph", "X"}, {"pid", "pid"}, {"tid", "tid"}, {"ts", 1}, {"dur", 2}}});
    return payload;
}
}

MemMapDetail mixPmuMapDetails = {
    .blockId = 0,
    .blockType = "cube",
    .opType = "mix",
    .soc = "Ascend910B4",
    .freq = 111,
    .totalCycles = 234,
    .aiCoreNum = 2,
    .blockDim = 2,
    .eventMap = {
        {1280, 4}, {1282, 1}, {1284, 4}, {1286, 1}, {1288, 4}, {1290, 1}, {1292, 100}, {1293, 100}, {1294, 100},
        {1283, 0}, {1287, 0}, {1291, 0}
    },
    .eventMapVec0 = {
        {1280, 4}, {1282, 1}, {1284, 4}, {1286, 1}, {1288, 4}, {1290, 1}, {1292, 300}, {1293, 300}, {1294, 300},
        {1283, 0}, {1287, 0}, {1291, 0}
    },
    .eventMapVec1 = {
        {1280, 3}, {1282, 3}, {1284, 3}, {1286, 3}, {1288, 3}, {1290, 3}, {1292, 50}, {1293, 50}, {1294, 50},
        {1283, 0}, {1287, 0}, {1291, 0}
    },
    .cycMap = {
        {"Cube", 234}, {"Vector", 135}, {"Vector1", 344}
    }
};

MemMapDetail vecPmuMapDetails1 = {
    .blockId = 0,
    .blockType = "vector0",
    .opType = "vector",
    .soc = "Ascend910B4",
    .freq = 111,
    .totalCycles = 200,
    .aiCoreNum = 2,
    .blockDim = 2,
    .eventMap = {
        {1280, 4}, {1282, 1}, {1284, 4}, {1286, 1}, {1288, 4}, {1290, 1}, {1292, 100}, {1293, 100}, {1294, 100},
        {1283, 0}, {1287, 0}, {1291, 0}
    },
};

MemMapDetail vecPmuMapDetails2 = {
    .blockId = 1,
    .blockType = "vector0",
    .opType = "vector",
    .soc = "Ascend910B4",
    .freq = 111,
    .totalCycles = 500,
    .aiCoreNum = 2,
    .blockDim = 2,
    .eventMap = {
        {1280, 3}, {1282, 3}, {1284, 3}, {1286, 3}, {1288, 3}, {1290, 3}, {1292, 50}, {1293, 50}, {1294, 50}
    },
};

uint64_t minAcsqTimeCyc = 536000000000;
AcsqTimeMapType acsqTimeMap = {
    // {{taskId, streamId}, {taskType, startTime, endTime}}
    {{1, 4},   {1,  minAcsqTimeCyc,        minAcsqTimeCyc + 5000}},
    {{2, 3},   {0,  minAcsqTimeCyc + 100,  minAcsqTimeCyc + 4900}},
    {{3, 53},  {7,  minAcsqTimeCyc + 1300, minAcsqTimeCyc + 1400}},
    {{4, 53},  {7,  minAcsqTimeCyc + 1500, minAcsqTimeCyc + 1700}},
    {{5, 53},  {11, minAcsqTimeCyc + 1300, minAcsqTimeCyc + 1800}},
    {{6, 53},  {11, minAcsqTimeCyc + 2100, minAcsqTimeCyc + 2200}},
    {{7, 54},  {8,  minAcsqTimeCyc + 2000, minAcsqTimeCyc + 3000}},
    {{8, 55},  {20, minAcsqTimeCyc + 2100, minAcsqTimeCyc + 2400}},
    {{9, 56},  {21, minAcsqTimeCyc + 3000, minAcsqTimeCyc + 3100}},
    {{10, 56}, {20, 0, 0}},
};

unique_ptr<DataHandler> &GetHandleTest1(const ChipType &type)
{
    static unique_ptr<DataHandler> handlePtr;
    if (type == ChipType::ASCEND910B) {
        handlePtr = Utility::MakeUnique<DataHandlerOf910B>();
    } else {
        handlePtr = Utility::MakeUnique<DataHandlerOf310P>();
    }
    return handlePtr;
}

inline shared_ptr<Visualize::OpBasicInfo> &GetOpBasicInfoObjTest1(const ChipType &type, unique_ptr<DataHandler> &handler)
{
    static shared_ptr<Visualize::OpBasicInfo> opBasicInfoPtr;
    opBasicInfoPtr = Utility::MakeShared<Visualize::OpBasicInfo>(handler);
    return opBasicInfoPtr;
}

inline shared_ptr<Visualize::BasicPmu> &GetBasicPmuObjTest1(const ChipType &type, unique_ptr<DataHandler> &handler)
{
    static shared_ptr<Visualize::BasicPmu> basicPmuPtr;
    basicPmuPtr = Utility::MakeShared<Visualize::BasicPmu>(handler);
    return basicPmuPtr;
}

inline unique_ptr<Visualize::PmuCalculator> &GetPmuCalculatorObjTest1(const ChipType &type,
                                                                 shared_ptr<OpBasicInfo> &opBasicInfoObj,
                                                                 shared_ptr<BasicPmu> &basicPmuObj)
{
    static unique_ptr<Visualize::PmuCalculator> pmuCalculatorPtr;
    if (type == ChipType::ASCEND910B) {
        pmuCalculatorPtr = Utility::MakeUnique<Visualize::PmuCalculator910B>();
    } else {
        pmuCalculatorPtr = Utility::MakeUnique<Visualize::PmuCalculator310P>();
    }
    pmuCalculatorPtr->Init(basicPmuObj);
    return pmuCalculatorPtr;
}

inline unique_ptr<Visualize::StorageAccess> &GetStorageAccessObjTest1(const ChipType &type,
                                                                 shared_ptr<Visualize::OpBasicInfo> &opBasicInfoObj,
                                                                 shared_ptr<Visualize::BasicPmu> &basicPmuObj,
                                                                 unique_ptr<Visualize::PmuCalculator> &pmuCalculatorObj)
{
    static unique_ptr<Visualize::StorageAccess> storageAccessPtr;
    if (type == ChipType::ASCEND910B) {
        storageAccessPtr = Utility::MakeUnique<StorageAccess910B>(opBasicInfoObj, basicPmuObj, pmuCalculatorObj);
    } else {
        storageAccessPtr = Utility::MakeUnique<StorageAccess310P>(opBasicInfoObj, basicPmuObj, pmuCalculatorObj);
    }
    return storageAccessPtr;
}

inline unique_ptr<Visualize::Occupancy> &GetOccupancyObjTest1(ChipType chipType,
                                                              shared_ptr<Visualize::OpBasicInfo> &opBasicInfoObj,
                                                              shared_ptr<Visualize::BasicPmu> &basicPmuObj)
{
    static unique_ptr<Visualize::Occupancy> occupancyPtr;
    if (chipType == ChipType::ASCEND910B) {
        occupancyPtr = Utility::MakeUnique<Occupancy910B>(opBasicInfoObj, basicPmuObj);
    } else if (chipType == ChipType::ASCEND950) {
        occupancyPtr = Utility::MakeUnique<OccupancyA5>(opBasicInfoObj, basicPmuObj);
    } else {
        occupancyPtr = Utility::MakeUnique<Occupancy>(opBasicInfoObj, basicPmuObj);
    }
    return occupancyPtr;
}

inline unique_ptr<RoofLine> &GetRoofLineObjTest1(const ChipType &type, const int64_t aicoreConst[2],
                                            shared_ptr<OpBasicInfo> &opBasicInfoObj, shared_ptr<BasicPmu> &basicPmuObj,
                                            unique_ptr<PmuCalculator> &pmuCalculatorObj)
{
    static unique_ptr<Visualize::RoofLine> roofLinePtr;
    if (type == ChipType::ASCEND910B) {
        roofLinePtr = MakeUnique<RoofLineOf910B>(aicoreConst[0], aicoreConst[1],
                                                 opBasicInfoObj, basicPmuObj, pmuCalculatorObj);
    } else {
        roofLinePtr = MakeUnique<RoofLineOf310P>(aicoreConst[0], aicoreConst[1],
                                                 opBasicInfoObj, basicPmuObj, pmuCalculatorObj);
    }
    return roofLinePtr;
}


ChipType g_chipType = ChipType::ASCEND910B;
Common::ProfMetricsAbilityConfig metrics;
ParserConfig parserConfig1;
string timeStamp1;
std::map<std::string, ProfBinInfo, FileNameCompare> eventMap_1;

auto &handler1 = GetHandleTest1(g_chipType);
bool rese1 = handler1->ParseDeviceData(parserConfig1, eventMap_1, metrics, timeStamp1);
auto &opBasicInfoObj1 = GetOpBasicInfoObjTest1(g_chipType, handler1);
auto &basicPmuObj1 = GetBasicPmuObjTest1(g_chipType, handler1);
auto &pmuCalculatorObj1 = GetPmuCalculatorObjTest1(g_chipType, opBasicInfoObj1, basicPmuObj1);
auto &storageAccessObj1 = GetStorageAccessObjTest1(g_chipType, opBasicInfoObj1, basicPmuObj1, pmuCalculatorObj1);
Occupancy910B occupancy(opBasicInfoObj1, basicPmuObj1);

/**
 * |  用例集  | DataVisualize
 * | 测试函数 | Occupancy::MergeSameCorePmu
 * |  用例名  | occupancy_merge_same_core_pmu_and_expect_success
 * | 用例描述 | 测试相同core的PMU数据可以正确合并
 */
TEST(DataVisualize, occupancy_merge_same_core_pmu_and_expect_success)
{
    std::vector<MemMapDetail> pmuMapDetails;
    std::vector<MemMapDetail> fusionPmuMapDetails;
    pmuMapDetails.emplace_back(vecPmuMapDetails1);
    pmuMapDetails.emplace_back(vecPmuMapDetails2);
    std::vector<std::pair<uint16_t, uint16_t>> blockIdCoreIdPairVec = {{0, 13}, {1,13}};
    occupancy.blockIdCoreIdPairVec_ = blockIdCoreIdPairVec;
    occupancy.MergeSameCorePmu(pmuMapDetails);
    fusionPmuMapDetails = occupancy.fusionPmuMapDetails_;
    occupancy.ClearOccupancyJson();

    ASSERT_TRUE(fusionPmuMapDetails.size() == 1);
    ASSERT_TRUE(fusionPmuMapDetails[0].totalCycles ==
        vecPmuMapDetails1.totalCycles + vecPmuMapDetails2.totalCycles);
    ASSERT_TRUE(fusionPmuMapDetails[0].eventMap[1280] ==
        vecPmuMapDetails1.eventMap[1280] + vecPmuMapDetails2.eventMap[1280]);
}

/**
 * |  用例集  | DataVisualize
 * | 测试函数 | Occupancy::GetOccupancyBlockJson
 * |  用例名  | occupancy_calculate_metric_and_expect_success
 * | 用例描述 | 测试生成occupancy块信息时吞吐、周期和命中率指标计算正确
 */
TEST(DataVisualize, occupancy_calculate_metric_and_expect_success)
{
    std::map<std::string, OccupancyMetrics> metricsMapMix;
    std::map<std::string, OccupancyMetrics> metricsMapVector;
    occupancy.GetOccupancyBlockJson(mixPmuMapDetails);
    metricsMapMix = occupancy.metricsMap_;
    occupancy.ClearOccupancyJson();

    occupancy.GetOccupancyBlockJson(vecPmuMapDetails1);
    metricsMapVector = occupancy.metricsMap_;
    occupancy.ClearOccupancyJson();

    ASSERT_TRUE(metricsMapMix["0cube0"].throughput == 38400);
    ASSERT_TRUE(metricsMapMix["0vector0"].throughput == 115200);
    ASSERT_TRUE(metricsMapMix["0vector1"].throughput == 19200);
    ASSERT_TRUE(metricsMapVector["0vector0"].throughput == 38400);
    ASSERT_TRUE(metricsMapMix["0cube0"].cycles == 234);
    ASSERT_TRUE(metricsMapMix["0vector0"].cycles == 135);
    ASSERT_TRUE(metricsMapMix["0vector1"].cycles == 344);
    ASSERT_TRUE(metricsMapVector["0vector0"].cycles == 200);
    ASSERT_TRUE(metricsMapMix["0cube0"].cacheHitRate == 80);
    ASSERT_TRUE(metricsMapMix["0vector0"].cacheHitRate == 80);
    ASSERT_TRUE(metricsMapMix["0vector1"].cacheHitRate == 50);
    ASSERT_TRUE(metricsMapVector["0vector0"].cacheHitRate == 80);
}

/**
 * |  用例集  | DataVisualize
 * | 测试函数 | Occupancy::AnalyzeOccupy
 * |  用例名  | occupancy_analyze_occupy_and_expect_success
 * | 用例描述 | 测试不同类型occupancy数据可生成预期优化建议
 */
TEST(DataVisualize, occupancy_analyze_occupy_and_expect_success)
{
    std::vector<std::pair<std::string, double>> cyclesOccupancy = {
        {"0vector0", 100}, {"1vector0", 101}, {"2vector0", 98}, {"3vector0", 200}, {"4vector0", 113}
    };
    std::vector<std::pair<std::string, double>> throughputOccupancy = {
        {"0cube0", 1000}, {"1cube0", 1082}, {"2cube0", 1000}, {"3cube0", 1279}, {"4cube0", 1700}
    };
    std::vector<std::pair<std::string, double>> cacheHitRateOccupancy = {
        {"0vector0", 70}, {"0vector1", 72}, {"1vector0", 70}, {"1vector1", 90}
    };
    std::vector<std::pair<std::string, double>> simtInstrOccupancy = {
        {"0vector0", 1792768}, {"1vector0", 448}, {"2vector0", 448}, {"3vector0", 448},
        {"4vector0", 448}, {"5vector0", 448}, {"6vector0", 448}, {"7vector0", 448},
    };

    std::vector<std::string> advicesCycles;
    std::vector<std::string> advicesThroughput;
    std::vector<std::string> advicesCacheHitRate;
    std::vector<std::string> advicesSimtInstr;

    occupancy.opType_ = Common::OpType::VECTOR;
    occupancy.NormalizeOccupy(cyclesOccupancy, Visualize::OccupancyDataType::OCPY_CYCLES);
    occupancy.AnalyzeOccupy(cyclesOccupancy,
        Visualize::OccupancyDataType::OCPY_CYCLES, advicesCycles);
    occupancy.opType_ = Common::OpType::CUBE;
    occupancy.NormalizeOccupy(throughputOccupancy, Visualize::OccupancyDataType::OCPY_THROUGHPUT);
    occupancy.AnalyzeOccupy(throughputOccupancy,
        Visualize::OccupancyDataType::OCPY_THROUGHPUT, advicesThroughput);
    occupancy.opType_ = Common::OpType::MIX;
    occupancy.NormalizeOccupy(cacheHitRateOccupancy, Visualize::OccupancyDataType::OCPY_CACHE_HIT_RATE);
    occupancy.AnalyzeOccupy(cacheHitRateOccupancy,
        Visualize::OccupancyDataType::OCPY_CACHE_HIT_RATE, advicesCacheHitRate);
    occupancy.opType_ = Common::OpType::VECTOR;
    occupancy.NormalizeOccupy(simtInstrOccupancy, Visualize::OccupancyDataType::OCPY_SIMT_INSTR);
    occupancy.AnalyzeOccupy(simtInstrOccupancy,
        Visualize::OccupancyDataType::OCPY_SIMT_INSTR, advicesSimtInstr);

    std::vector<std::string> cyclesRes = {{"vector1 of core[1] take more time than other vector cores"}};
    std::vector<std::string> throughtputRes = {{"cube0 of core[4] write/read more data than other cube cores"}};
    std::vector<std::string> cacheHitRateRes = {{"vector0, vector1 of core[0], vector0 of core[1] cache hit rate lower than other vector cores"}};
    std::vector<std::string> simtInstrRes = {{"vector0 of core[0] execute more instructions than other vector cores"}};

    EXPECT_EQ(advicesCycles, cyclesRes);
    EXPECT_EQ(advicesThroughput, throughtputRes);
    EXPECT_EQ(advicesCacheHitRate, cacheHitRateRes);
    EXPECT_EQ(advicesSimtInstr, simtInstrRes);
}

/**
 * |  用例集  | DataVisualize
 * | 测试函数 | Occupancy910B::GetOccupancyMap
 * |  用例名  | 910b_gen_occupy_and_expect_success
 * | 用例描述 | 测试910B场景下可以成功生成occupancy地图
 */
TEST(DataVisualize, 910b_gen_occupy_and_expect_success)
{
    std::string soc = "Ascend910B1";
    nlohmann::json occupancyMapJson;
    bool res = occupancy.GetOccupancyMap(occupancyMapJson);
    ASSERT_TRUE(res == true);
}

/**
 * |  用例集  | DataVisualize
 * | 测试函数 | Occupancy::GetOccupancyMap
 * |  用例名  | 310P_gen_occupy_and_expect_fail
 * | 用例描述 | 测试310P场景下生成occupancy地图时返回空的op_detail
 */
TEST(DataVisualize, 310P_gen_occupy_and_expect_fail)
{
    ChipType chipType = ChipType::ASCEND310P;

    auto &handler2 = GetHandleTest1(chipType);
    bool resu = handler2->ParseDeviceData(parserConfig1, eventMap_1, metrics, timeStamp1);

    auto &opBasicInfoObj = GetOpBasicInfoObjTest1(chipType, handler2);
    auto &basicPmuObj = GetBasicPmuObjTest1(chipType, handler2);
    opBasicInfoObj->SetBlockDetail();

    auto &pmuCalculatorObj = GetPmuCalculatorObjTest1(chipType, opBasicInfoObj, basicPmuObj);

    auto &storageAccessObj = GetStorageAccessObjTest1(chipType, opBasicInfoObj, basicPmuObj, pmuCalculatorObj);

    Occupancy occupancy(opBasicInfoObj, basicPmuObj);
    nlohmann::json occupancyMapJson;
    bool res = occupancy.GetOccupancyMap(occupancyMapJson);
    ASSERT_EQ(occupancyMapJson.at("op_detail").size(), 0);
}

/**
 * |  用例集  | DataVisualize
 * | 测试函数 | Occupancy::GetOccupancyBlockJson
 * |  用例名  | occupancy_getOccupancyBlockJson_expect_success
 * | 用例描述 | 测试混合算子可正确生成3个occupancy块JSON
 */
TEST(DataVisualize, occupancy_getOccupancyBlockJson_expect_success)
{
    occupancy.opType_ = Common::OpType::MIX;
    MemMapDetail test = mixPmuMapDetails;
    auto res1 = occupancy.GetOccupancyBlockJson(test);
    ASSERT_TRUE(res1.size()==3);
}

/**
 * |  用例集  | DeviceDataParse
 * | 测试函数 | DataHandler::SaveOpBasicInfo
 * |  用例名  | SaveOpBasicInfo_Return_True
 * | 用例描述 | 测试保存算子基础信息成功返回true
 */
TEST(DeviceDataParse, SaveOpBasicInfo_Return_True)
{
    GlobalMockObject::verify();
    DataHandler dataHandler;
    const std::string path = "test/ut/resources/op_profiling/device910B";
    HalHelper::Instance().handleDcmi_ = nullptr;
    ASSERT_TRUE(dataHandler.SaveOpBasicInfo(path));
}

/**
 * |  用例集  | DataVisualize
 * | 测试函数 | OpBasicInfo::GenFreAdvice
 * |  用例名  | genfreadvice_low_freq_advice
 * | 用例描述 | 测试当前频率低于额定频率时生成低频告警建议
 */
TEST(DataVisualize, genfreadvice_low_freq_advice)
{
    ChipType chipType = ChipType::ASCEND310P;
    auto &handler2 = GetHandleTest1(chipType);
    std::string curFreq = "800";
    std::string ratedFreq = "1850";
    handler2->SetCurFreq(curFreq);
    handler2->SetRatedFreq(ratedFreq);
    auto &opBasicInfoObj = GetOpBasicInfoObjTest1(chipType, handler2);
    auto res = opBasicInfoObj->GenFreAdvice();
    EXPECT_EQ(res.size(), 1);
    EXPECT_EQ(res[0], "The current frequency is lower than the rated frequency");
}

/**
 * |  用例集  | DataVisualize
 * | 测试函数 | PmuCalculator910B::GetPipeBwByWeight
 * |  用例名  | GetPipeBwByWeight_by_freq_datas_success
 * | 用例描述 | 测试不同频点和数据量下返回正确的pipe带宽权重
 */
TEST(DataVisualize, GetPipeBwByWeight_by_freq_datas_success)
{
    ChipType chipType = ChipType::ASCEND910B;
    auto &handler2 = GetHandleTest1(chipType);
    bool resu = handler2->ParseDeviceData(parserConfig1, eventMap_1, metrics, timeStamp1);
    auto &opBasicInfoObj = GetOpBasicInfoObjTest1(chipType, handler2);
    auto &basicPmuObj = GetBasicPmuObjTest1(chipType, handler2);
    opBasicInfoObj->SetBlockDetail();
    auto &pmuCalculatorObj = GetPmuCalculatorObjTest1(chipType, opBasicInfoObj, basicPmuObj);
    // 输入数据量为0时，取默认数据
    uint64_t l0aData = 0;
    uint64_t l0bData = 0;
    uint64_t l0cToGmData = 0;
    uint64_t l0cToL1Data = 0;
    // 有效可用通路为MTE1\MTE2\MTE3\MTE2 Vector\MTE3 Vector 5种
    auto resB1 = pmuCalculatorObj->GetPipeBwByWeight("Ascend910B1", l0aData, l0bData, l0cToGmData, l0cToL1Data);
    EXPECT_FLOAT_EQ(resB1["MTE1"], 324.0);
    EXPECT_FLOAT_EQ(resB1["MTE2"], 340.1);
    EXPECT_FLOAT_EQ(resB1["MTE3"], 199.43);
    EXPECT_FLOAT_EQ(resB1["MTE2 vector"], 220.06);
    EXPECT_FLOAT_EQ(resB1["MTE3 vector"], 186.8);
    // B2=B3
    auto resB2 = pmuCalculatorObj->GetPipeBwByWeight("Ascend910B2", l0aData, l0bData, l0cToGmData, l0cToL1Data);
    auto resB3 = pmuCalculatorObj->GetPipeBwByWeight("Ascend910B3", l0aData, l0bData, l0cToGmData, l0cToL1Data);
    EXPECT_FLOAT_EQ(resB2["MTE1"], 315.245);
    EXPECT_FLOAT_EQ(resB2["MTE2"], 330.91);
    EXPECT_FLOAT_EQ(resB2["MTE3"], 193.99);
    EXPECT_FLOAT_EQ(resB2["MTE2 vector"], 214.11);
    EXPECT_FLOAT_EQ(resB2["MTE3 vector"], 181.75);
    for (auto &pipe : resB3) {
        EXPECT_FLOAT_EQ(pipe.second, resB2[pipe.first]);
    }
    auto resB4 = pmuCalculatorObj->GetPipeBwByWeight("Ascend910B4", l0aData, l0bData, l0cToGmData, l0cToL1Data);
    EXPECT_FLOAT_EQ(resB4["MTE1"], 271.005);
    EXPECT_FLOAT_EQ(resB4["MTE2"], 222.17);
    EXPECT_FLOAT_EQ(resB4["MTE3"], 189.89);
    EXPECT_FLOAT_EQ(resB4["MTE2 vector"], 195.27);
    EXPECT_FLOAT_EQ(resB4["MTE3 vector"], 176.75);

    // 输入数据量有效时，根据数据占比求峰值带宽
    resB1 = pmuCalculatorObj->GetPipeBwByWeight("Ascend910B1", 100, 300, 100, 700);
    resB4 = pmuCalculatorObj->GetPipeBwByWeight("Ascend910B4", 100, 300, 100, 700);
    EXPECT_FLOAT_EQ(resB1["MTE1"], maxBwRateOf910B1.at(TransportType::MTE_TO_L0A) * 0.25 + maxBwRateOf910B1.at(TransportType::MTE_TO_L0B) * 0.75);
    EXPECT_FLOAT_EQ(resB4["MTE1"], maxBwRateOf910B4.at(TransportType::MTE_TO_L0A) * 0.25 + maxBwRateOf910B4.at(TransportType::MTE_TO_L0B) * 0.75);
    EXPECT_FLOAT_EQ(resB1["FIXP"], maxBwRateOf910B1.at(TransportType::L0C_TO_GM) * 0.125 + maxBwRateOf910B1.at(TransportType::L0C_TO_L1) * 0.875);
    EXPECT_FLOAT_EQ(resB4["FIXP"], maxBwRateOf910B4.at(TransportType::L0C_TO_GM) * 0.125 + maxBwRateOf910B4.at(TransportType::L0C_TO_L1) * 0.875);
}

/**
 * |  用例集  | DataVisualize
 * | 测试函数 | PmuCalculator910B::AddBasicPmu910B
 * |  用例名  | addbasicpmu910b_success
 * | 用例描述 | 测试910B场景下基础PMU指标补充结果正确
 */
TEST(DataVisualize, addbasicpmu910b_success)
{
    ChipType chipType = ChipType::ASCEND910B;
    MOCKER(&BasicPmu::GetL2cacheEvict).stubs().will(returnValue(-1));
    auto &handler2 = GetHandleTest1(chipType);
    bool resu = handler2->ParseDeviceData(parserConfig1, eventMap_1, metrics, timeStamp1);
    auto &opBasicInfoObj = GetOpBasicInfoObjTest1(chipType, handler2);
    auto &basicPmuObj = GetBasicPmuObjTest1(chipType, handler2);
    opBasicInfoObj->SetBlockDetail();

    PmuCalculator910B pmuCalculatorObj910B = PmuCalculator910B();
    pmuCalculatorObj910B.Init(basicPmuObj);

    // A2 training vector op
    std::string opType = Common::OpType::VECTOR;
    std::map<uint64_t, uint64_t> eventMap = { {1280, 1280}, {1284, 1284}, {1288, 1288}, {1282, 1282}, {1286, 1286},
                                              {1290, 1290}, {700, 700}, {526, 526}, {771, 771}, {771, 771}, {62, 62},
                                              {61, 61}, {67, 67}, {68, 68}, {55, 55}, {56, 56} };
    MemMapDetail memMapDetail;
    memMapDetail.eventMap = eventMap;
    std::map<std::string, uint64_t> basicPmu;
    pmuCalculatorObj910B.AddBasicPmu910B(opType, memMapDetail, basicPmu);
    EXPECT_EQ(basicPmu["L2Cache Write hit"], 1280);
    EXPECT_EQ(basicPmu["L2Cache Read hit"], 1284 + 1288);
    EXPECT_EQ(basicPmu["L2Cache Write miss"], 1282);
    EXPECT_EQ(basicPmu["L2Cache Read miss"], 1286 + 1290);

    EXPECT_EQ(basicPmu["MTE1 Cyc"], 0);
    EXPECT_EQ(basicPmu["FIXP Ins"], 526);
    EXPECT_EQ(basicPmu["FIXP Cyc"], 771);

    EXPECT_EQ(basicPmu["GM Read Total"], 1286 + 1290);

    EXPECT_EQ(basicPmu["UB MTE Read"], 62);
    EXPECT_EQ(basicPmu["UB MTE Write"], 61);
    EXPECT_EQ(basicPmu["UB Vec Read"], 67);
    EXPECT_EQ(basicPmu["UB Vec Write"], 68);
    EXPECT_EQ(basicPmu["Scalar Read"], 55);
    EXPECT_EQ(basicPmu["Scalar Write"], 56);

    // A2 training cube op
    opType = Common::OpType::CUBE;
    std::map<uint64_t, uint64_t> eventMapCube ={ {50, 5000}, {518, 518}, {49, 49000}, {19, 1900}, {524, 524},
                                                 {518, 518}, {40, 40}, {28, 28}, {27, 27}, {34, 34}, {33, 33}, {42, 42} };
    MemMapDetail memMapDetailCube;
    memMapDetailCube.eventMap = eventMapCube;
    std::map<std::string, uint64_t> basicPmuCube;
    pmuCalculatorObj910B.AddBasicPmu910B(opType, memMapDetailCube, basicPmuCube);

    EXPECT_EQ(basicPmuCube["MTE/GM Read"], 5000 - 518);
    EXPECT_EQ(basicPmuCube["L0A/L0B Write"], 49000 - 4 * 1900 + 524 - 518);
    EXPECT_EQ(basicPmuCube["L1_L0C Read"], 518);
    EXPECT_EQ(basicPmuCube["L1 GM Write"], 4 * 1900 - 524 + 518);
    EXPECT_EQ(basicPmuCube["L0C Read"], 40);
    EXPECT_EQ(basicPmuCube["GM Write"], 524 - 518);
    EXPECT_EQ(basicPmuCube["MTE Write"], 49000);
    EXPECT_EQ(basicPmuCube["L0A Read"], 28);
    EXPECT_EQ(basicPmuCube["L0A Cube Write"], 27);
    EXPECT_EQ(basicPmuCube["L0B Read"], 34);
    EXPECT_EQ(basicPmuCube["L0B Cube Write"], 33);
    EXPECT_EQ(basicPmuCube["L0C Write"], 42);
    GlobalMockObject::verify();
}

/**
 * |  用例集  | DataVisualize
 * | 测试函数 | PmuCalculator910B::GetTableLineAiCore
 * |  用例名  | gettablelineaicore_success
 * | 用例描述 | 测试不同算子类型对应的AiCore表头信息生成正确
 */
TEST(DataVisualize, gettablelineaicore_success)
{
    std::vector<std::string> ubMem = {"UB Read MTE", "UB Write MTE", "UB Read GM", "UB Write GM", "UB Write Vector",
                                      "UB Read Vector", "UB Read Scalar", "UB Write Scalar"};
    ChipType chipType = ChipType::ASCEND910B;
    auto &handler2 = GetHandleTest1(chipType);
    bool resu = handler2->ParseDeviceData(parserConfig1, eventMap_1, metrics, timeStamp1);
    auto &opBasicInfoObj = GetOpBasicInfoObjTest1(chipType, handler2);
    auto &basicPmuObj = GetBasicPmuObjTest1(chipType, handler2);
    opBasicInfoObj->SetBlockDetail();
    PmuCalculator910B pmuCalculatorObj910B = PmuCalculator910B();
    pmuCalculatorObj910B.Init(basicPmuObj);

    // A2 training vector op
    std::string opType = Common::OpType::VECTOR;
    pmuCalculatorObj910B.LoadLineMap(opType);
    auto res = pmuCalculatorObj910B.GetTableLineAiCore();
    std::map<std::string, std::vector<std::string>> vecTable = {
        {"GM", {"Read Main Memory", "Write Main Memory"} },
        {"Cache", {"L2 Cache Write", "L2 Cache Read", "L2 Cache Total", "iCache Total"}},
        {"Pipe", {"MTE1", "MTE2", "MTE3", "FIXP", "Scalar"}},
        {"UB", ubMem},
        {"Vector", {"Vector Write UB", "Vector Read UB"}}
    };
    // test Cache table
    for (auto &table: vecTable) {
        EXPECT_EQ(res.count(table.first), 1);
        for (int i=0; i<table.second.size(); i++) {
            EXPECT_EQ(res[table.first][i], table.second.at(i));
        }
    }
    // A2 training cube op
    opType = Common::OpType::CUBE;
    pmuCalculatorObj910B.LoadLineMap(opType);
    res = pmuCalculatorObj910B.GetTableLineAiCore();
    std::map<std::string, std::vector<std::string>> cubeTable = {
        {"GM", {"Read Main Memory", "Write Main Memory"} },
        {"Cache", {"L2 Cache Write", "L2 Cache Read", "L2 Cache Total", "iCache Total"}},
        {"Pipe", {"MTE1", "MTE2", "MTE3", "FIXP", "Scalar"}},
        {"L1", {"L1 Read MTE", "L1 Write MTE", "L1 Write L0A/L0B", "L1 Read L0C", "L1 Write GM", "L1 Read GM"}},
        {"L0A", {"L0A Read MTE", "L0A Write Cube", "L0A Read L1/GM"}},
        {"L0B", {"L0B Read MTE", "L0B Write Cube", "L0B Read L1/GM"}},
        {"Cube", {"Cube Read L0A", "Cube Read L0B", "Cube Write L0C", "Cube Read L0C"}},
        {"L0C", {"L0C Read Cube", "L0C Write Cube", "L0C Write GM", "L0C Write L1"}}
    };
    // test Cache table
    for (auto &table: cubeTable) {
        EXPECT_EQ(res.count(table.first), 1);
        for (int i=0; i<table.second.size(); i++) {
            EXPECT_EQ(res[table.first][i], table.second.at(i));
        }
    }
    // A2 training mix op
    opType = Common::OpType::MIX;
    pmuCalculatorObj910B.LoadLineMap(opType);
    res = pmuCalculatorObj910B.GetTableLineAiCore();
    std::map<std::string, std::vector<std::string>> mixTable = {
        {"Cache", {"L2 Cache Write", "L2 Cache Read", "L2 Cache Total", "iCache Total"}},
        {"UB Core0", ubMem},{"Vector Core0",  {"Vector Write UB", "Vector Read UB"}},
        {"UB Core1", ubMem}, {"Vector Core1", {"Vector Write UB", "Vector Read UB"}},
        {"GM Cube", {"Read Main Memory", "Write Main Memory"}},
        {"GM Vector Core0", {"Read Main Memory", "Write Main Memory"}}, {"GM Vector Core1", {"Read Main Memory", "Write Main Memory"}},
        {"Pipe Cube", {"MTE1", "MTE2", "MTE3", "FIXP", "Scalar"}},
        {"Pipe Vector Core0", {"MTE2", "MTE3", "Scalar"}}, {"Pipe Vector Core1", {"MTE2", "MTE3", "Scalar"}},
        {"L1", {"L1 Read MTE", "L1 Write MTE", "L1 Write L0A/L0B", "L1 Read L0C", "L1 Write GM", "L1 Read GM"}},
        {"L0A", {"L0A Read MTE", "L0A Write Cube", "L0A Read L1/GM"}},
        {"L0B", {"L0B Read MTE", "L0B Write Cube", "L0B Read L1/GM"}},
        {"Cube", {"Cube Read L0A", "Cube Read L0B", "Cube Write L0C", "Cube Read L0C"}},
        {"L0C", {"L0C Read Cube", "L0C Write Cube", "L0C Write GM", "L0C Write L1"}}
    };
    EXPECT_EQ(res.count("GM"), 0);
    EXPECT_EQ(res.count("Pipe"), 0);
    // test Cache table
    for (auto &table: mixTable) {
        EXPECT_EQ(res.count(table.first), 1);
        for (int i=0; i<table.second.size(); i++) {
            EXPECT_EQ(res[table.first][i], table.second.at(i));
        }
    }
}

/**
 * |  用例集  | MC2TimelineParser
 * | 测试函数 | MC2TimelineParser::ProcessHcclData
 * |  用例名  | ProcessHcclDataSuccess
 * | 用例描述 | 测试处理HCCL数据后生成预期timeline事件
 */
TEST(MC2TimelineParser, ProcessHcclDataSuccess)
{
    unique_ptr<DataHandler> handler = Utility::MakeUnique<DataHandlerOf910B>();
    shared_ptr<Visualize::OpBasicInfo> opBasicInfoPtr = Utility::MakeShared<Visualize::OpBasicInfo>(handler);
    shared_ptr<Visualize::BasicPmu> basicPmuPtr = Utility::MakeShared<Visualize::BasicPmu>(handler);
    MC2TimelineParser parser{acsqTimeMap, minAcsqTimeCyc, opBasicInfoPtr, basicPmuPtr};
    parser.aicpuFreq_ = 50000;
    nlohmann::json aicore;
    nlohmann::json aicpu;
    nlohmann::json waitSqe1;
    nlohmann::json waitSqe2;
    nlohmann::json sdmaSqe1;
    nlohmann::json sdmaSqe2;
    nlohmann::json writeValue;
    nlohmann::json cCoreSqe;
    nlohmann::json threadName;
    nlohmann::json threadSortIndex;
    parser.ProcessHcclData();
    EXPECT_EQ(parser.timelineJson_.size(), 14);
    for (const auto &i: parser.timelineJson_) {
        if (i.at("tid") == "AI_CORE" && i.at("name") == "AI_CORE") { aicore = i; }
        if (i.at("tid") == "AI_CPU" && i.at("name") == "AI_CPU") { aicpu = i; }
        if (i.at("tid") == "STREAM53" && i.at("name") == "NOTIFY_WAIT_SQE" && int(i.at("ts")) == 26) { waitSqe1 = i; }
        if (i.at("tid") == "STREAM53" && i.at("name") == "NOTIFY_WAIT_SQE" && int(i.at("ts")) == 30) { waitSqe2 = i; }
        if (i.at("tid") == "STREAM53" && i.at("name") == "SDMA_SQE" && int(i.at("ts")) == 26) { sdmaSqe1 = i; }
        if (i.at("tid") == "STREAM53" && i.at("name") == "SDMA_SQE" && int(i.at("ts")) == 42) { sdmaSqe2 = i; }
        if (i.at("tid") == "STREAM54" && i.at("name") == "WRITE_VALUE_SQE" && int(i.at("ts")) == 40) { writeValue = i; }
        if (i.at("tid") == "STREAM55" && i.at("name") == "C_CORE_SQE" && int(i.at("ts")) == 42) { cCoreSqe = i; }
        if (i.at("tid") == "STREAM53" && i.at("name") == "thread_name") { threadName = i; }
        if (i.at("tid") == "STREAM53" && i.at("name") == "thread_sort_index") { threadSortIndex = i; }
    }
    EXPECT_FLOAT_EQ(aicore.at("dur"), 96);
    EXPECT_FLOAT_EQ(aicpu.at("dur"), 100);
    EXPECT_FLOAT_EQ(waitSqe1.at("dur"), 2);
    EXPECT_FLOAT_EQ(waitSqe2.at("dur"), 4);
    EXPECT_FLOAT_EQ(sdmaSqe1.at("dur"), 10);
    EXPECT_FLOAT_EQ(sdmaSqe2.at("dur"), 2);
    EXPECT_FLOAT_EQ(writeValue.at("dur"), 20);
    EXPECT_FLOAT_EQ(cCoreSqe.at("dur"), 6);
    EXPECT_EQ(threadName.at("args").at("name"), "STREAM53");
    EXPECT_EQ(threadSortIndex.at("args").at("sort_index"), 10053); // SORT_BIAS + streamid
}

/**
 * |  用例集  | MC2TimelineParser
 * | 测试函数 | MC2TimelineParser::ProcessHcclTaskData
 * |  用例名  | ProcessHcclTaskDataSuccess
 * | 用例描述 | 测试处理HCCL task数据后生成预期plane事件
 */
TEST(MC2TimelineParser, ProcessHcclTaskDataSuccess)
{
    unique_ptr<DataHandler> handler = Utility::MakeUnique<DataHandlerOf910B>();
    shared_ptr<Visualize::OpBasicInfo> opBasicInfoPtr = Utility::MakeShared<Visualize::OpBasicInfo>(handler);
    shared_ptr<Visualize::BasicPmu> basicPmuPtr = Utility::MakeShared<Visualize::BasicPmu>(handler);
    MC2TimelineParser parser{acsqTimeMap, minAcsqTimeCyc, opBasicInfoPtr, basicPmuPtr};
    parser.aicpuFreq_ = 50000;
    nlohmann::json hcclInfo;
    nlohmann::json notifyRecord;
    nlohmann::json notifyWait;
    MsprofAicpuHcclTaskInfo info1 = {
        0x448d1a5ea052639, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 53, 0, 0, 0, 0, 0, 0, 0, 0,
        {0, 0, 0, 0, 0, 0, 0, 0, 0}
    };
    MsprofAicpuHcclTaskInfo info2 = {
        0x9eb5886f0b5ef22f, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 53, 1, 0, 1, 1, 1, 0, 0, 0,
        {0, 0, 0, 0, 0, 0, 0, 0, 0}
    };
    MsprofAicpuHcclTaskInfo info3 = {
        0xab44b101f36f4b07, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 53, 2, 0, 2, 2, 2, 0, 0, 0,
        {0, 0, 0, 0, 0, 0, 0, 0, 0}
    };
    MsprofAicpuHcclTaskInfo info4 = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 0, 56, 0, 0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0, 0}
    };
    parser.ProcessHcclTaskData({info1, info2, info3, info4});
    EXPECT_EQ(parser.timelineJson_.size(), 9);
    for (const auto &i: parser.timelineJson_) {
        if (i.at("tid") == "PLANE0" && i.at("name") == "hccl_info") { hcclInfo = i; }
        if (i.at("tid") == "PLANE1" && i.at("name") == "Notify_Record") { notifyRecord = i; }
        if (i.at("tid") == "PLANE2" && i.at("name") == "Notify_Wait") { notifyWait = i; }
    }
    EXPECT_FLOAT_EQ(hcclInfo.at("dur"), 2);
    EXPECT_FLOAT_EQ(hcclInfo.at("ts"), 26);
    EXPECT_EQ(hcclInfo.at("args").at("stream id"), "53");
    EXPECT_EQ(hcclInfo.at("args").at("task id"), "3");
    EXPECT_EQ(hcclInfo.at("args").at("transport type"), "SDMA");
    EXPECT_EQ(hcclInfo.at("args").at("data type"), "INT8");
    EXPECT_EQ(hcclInfo.at("args").at("link type"), "ON_CHIP");

    EXPECT_FLOAT_EQ(notifyRecord.at("dur"), 4);
    EXPECT_FLOAT_EQ(notifyRecord.at("ts"), 30);
    EXPECT_EQ(notifyRecord.at("args").at("stream id"), "53");
    EXPECT_EQ(notifyRecord.at("args").at("task id"), "4");
    EXPECT_EQ(notifyRecord.at("args").at("transport type"), "RDMA");
    EXPECT_EQ(notifyRecord.at("args").at("data type"), "INT16");
    EXPECT_EQ(notifyRecord.at("args").at("link type"), "HCCS");

    EXPECT_FLOAT_EQ(notifyWait.at("dur"), 10);
    EXPECT_FLOAT_EQ(notifyWait.at("ts"), 26);
    EXPECT_EQ(notifyWait.at("args").at("stream id"), "53");
    EXPECT_EQ(notifyWait.at("args").at("task id"), "5");
    EXPECT_EQ(notifyWait.at("args").at("transport type"), "LOCAL");
    EXPECT_EQ(notifyWait.at("args").at("data type"), "INT32");
    EXPECT_EQ(notifyWait.at("args").at("link type"), "PCIE");
}

/**
 * |  用例集  | MC2TimelineParser
 * | 测试函数 | MC2TimelineParser::ProcessAicpuTurnData
 * |  用例名  | ProcessAicpuTurnDataSuccess
 * | 用例描述 | 测试处理AICPU turn数据后生成预期turn事件
 */
TEST(MC2TimelineParser, ProcessAicpuTurnDataSuccess)
{
    unique_ptr<DataHandler> handler = Utility::MakeUnique<DataHandlerOf910B>();
    shared_ptr<Visualize::OpBasicInfo> opBasicInfoPtr = Utility::MakeShared<Visualize::OpBasicInfo>(handler);
    shared_ptr<Visualize::BasicPmu> basicPmuPtr = Utility::MakeShared<Visualize::BasicPmu>(handler);
    MC2TimelineParser parser{acsqTimeMap, minAcsqTimeCyc, opBasicInfoPtr, basicPmuPtr};
    parser.aicpuFreq_ = 50000;
    AicpuKfcProfCommTurn commTurn1 = {
        536000001000, 536000002000, 536000004000, 536000007000, 536000010000, 536000014000, 536000019000,
        0, 0, 60, 100, 0, 1, 0, {0, 0, 0, 0, 0}
    };
    AicpuKfcProfCommTurn commTurn2 = {
        536000001000, 536000002000, 536000003000, 536000004000, 536000005000, 536000006000, 536000007000,
        0, 0, 61, 101, 0, 2, 1, {0, 0, 0, 0, 0}
    };
    nlohmann::json turn0;
    nlohmann::json turn1;
    nlohmann::json threadName;
    nlohmann::json threadSortIndex;
    parser.ProcessAicpuTurnData({commTurn1, commTurn2});
    EXPECT_EQ(parser.timelineJson_.size(), 16);
    for (const auto &i: parser.timelineJson_) {
        if (i.at("tid") == "TURN0" && i.at("name") == "StartServer") { turn0 = i; }
        if (i.at("tid") == "TURN1" && i.at("name") == "Finalize") { turn1 = i; }
        if (i.at("tid") == "TURN0" && i.at("name") == "thread_name") { threadName = i; }
        if (i.at("tid") == "TURN0" && i.at("name") == "thread_sort_index") { threadSortIndex = i; }
    }
    EXPECT_FLOAT_EQ(turn0.at("dur"), 20);
    EXPECT_FLOAT_EQ(turn0.at("ts"), 20);
    EXPECT_EQ(turn0.at("args").at("stream id"), "60");
    EXPECT_EQ(turn0.at("args").at("task id"), "100");

    EXPECT_FLOAT_EQ(turn1.at("dur"), 20);
    EXPECT_FLOAT_EQ(turn1.at("ts"), 120);
    EXPECT_EQ(turn1.at("args").at("stream id"), "61");
    EXPECT_EQ(turn1.at("args").at("task id"), "101");

    EXPECT_EQ(threadName.at("args").at("name"), "TURN0");
    EXPECT_EQ(threadSortIndex.at("args").at("sort_index"), 10000); // SORT_BIAS + turnid
}

/**
 * |  用例集  | OpProf
 * | 测试函数 | Calculate::CalPipeTime
 * |  用例名  | test_cal_pipe_time_when_timefactor_is_zero_and_expect_no_throw
 * | 用例描述 | 测试timefactor为0时调用CalPipeTime不抛出异常
 */
TEST(OpProf, test_cal_pipe_time_when_timefactor_is_zero_and_expect_no_throw)
{
    Profiling::CalDeviceInfo calDeviceInfo =
        {Common::ChipType::ASCEND910B, mixPmuMapDetails.freq, 0, 0, mixPmuMapDetails.soc};
    Profiling::Calculate cal(std::map<uint16_t, uint64_t>(), 0, calDeviceInfo);
    uint64_t pmu = 1000;
    ASSERT_NO_THROW(cal.CalPipeTime({pmu}));
}

/**
 * |  用例集  | OpProf
 * | 测试函数 | Calculate::CalTransportBwUsageRate
 * |  用例名  | test_cal_transport_bw_usage_rate_when_timefactor_is_zero_and_expect_no_throw
 * | 用例描述 | 测试timefactor为0时调用CalTransportBwUsageRate不抛出异常
 */
TEST(OpProf, test_cal_transport_bw_usage_rate_when_timefactor_is_zero_and_expect_no_throw)
{
    Profiling::CalDeviceInfo calDeviceInfo =
        {Common::ChipType::ASCEND910B, mixPmuMapDetails.freq, 0, 0, mixPmuMapDetails.soc};
    Profiling::Calculate cal(std::map<uint16_t, uint64_t>(), 0, calDeviceInfo);
    const uint64_t type = static_cast<uint64_t>(TransportType::L0C_TO_L1);
    uint64_t pmu = 1000;
    ASSERT_NO_THROW(cal.CalTransportBwUsageRate(pmu, type));
}

/**
 * |  用例集  | DataVisualize
 * | 测试函数 | Occupancy::AnalyzeOccupy
 * |  用例名  | test_analyze_occupy_when_given_zero_and_expect_no_throw
 * | 用例描述 | 测试occupancy最大值为0时调用AnalyzeOccupy不抛出异常
 */
TEST(DataVisualize, test_analyze_occupy_when_given_zero_and_expect_no_throw)
{
    std::vector<std::pair<std::string, double>> cyclesOccupancy = {
        {"0vector0", 0}, {"1vector0", 0}, {"2vector0", 0}
    };
    std::vector<std::pair<std::string, double>> throughputOccupancy = {
        {"0cube0", 0.0}, {"1cube0", 0.0}, {"2cube0", 0.0}
    };
    std::vector<std::string> adviceList;
    occupancy.opType_ = Common::OpType::VECTOR;
    ASSERT_NO_THROW(occupancy.AnalyzeOccupy(cyclesOccupancy, Visualize::OccupancyDataType::OCPY_CYCLES, adviceList));
    occupancy.opType_ = Common::OpType::CUBE;
    ASSERT_NO_THROW(occupancy.AnalyzeOccupy(throughputOccupancy, Visualize::OccupancyDataType::OCPY_THROUGHPUT, adviceList));
}

/**
 * |  用例集  | WarpTimelineVisualize
 * | 测试函数 | WarpTimelineVisualize::TimelineToJson
 * |  用例名  | timeline_to_json_generates_warp_trace_only_in_memory
 * | 用例描述 | 测试warp timeline仅在内存中生成标准trace数据且不落独立文件
 */
TEST(WarpTimelineVisualize, timeline_to_json_generates_warp_trace_only_in_memory) {
    GlobalMockObject::verify();
    const std::string testDir = "test/ut/resources/op_profiling/warp_timeline_visualize";
    std::experimental::filesystem::remove_all(testDir);
    const std::string outputPath = JoinPath({testDir, "device0/add/0"});
    WriteWarpTimelineBin(outputPath, {{100, 160}});
    ScopedAicoreFreqMock freqMock;

    WarpTimelineVisualize warpTimelineVisualize;
    ASSERT_TRUE(warpTimelineVisualize.TimelineToJson(outputPath));

    nlohmann::json traceJson = warpTimelineVisualize.GetTimelineJson();
    ASSERT_TRUE(traceJson.contains("traceEvents"));
    ASSERT_TRUE(traceJson["traceEvents"].is_array());
    ASSERT_EQ(traceJson["traceEvents"].size(), 2);
    EXPECT_EQ(traceJson["traceEvents"][0]["name"], "Warp_0");
    EXPECT_EQ(traceJson["traceEvents"][0]["pid"], "warp.core0.veccore0");
    EXPECT_EQ(traceJson["traceEvents"][0]["tid"], "Warp 0");
    EXPECT_DOUBLE_EQ(traceJson["traceEvents"][0]["ts"].get<double>(), 0.0);
    EXPECT_DOUBLE_EQ(traceJson["traceEvents"][0]["dur"].get<double>(), 0.06);
    EXPECT_EQ(traceJson["traceEvents"][0]["args"]["core_id"], 0);
    EXPECT_EQ(traceJson["traceEvents"][0]["args"]["core_type"], TEST_CORE_TYPE_VEC0);
    EXPECT_EQ(traceJson["traceEvents"][1]["name"], "thread_sort_index");
    EXPECT_EQ(traceJson["traceEvents"][1]["args"]["sort_index"], 0);
    EXPECT_FALSE(IsExist(JoinPath({outputPath, "warp_timeline_trace.json"})));

    std::experimental::filesystem::remove_all(testDir);
    GlobalMockObject::verify();
}

/**
 * |  用例集  | WarpTimelineVisualize
 * | 测试函数 | WarpTimelineVisualize::TimelineToJson
 * |  用例名  | timeline_to_json_sorts_warp_threads_by_numeric_id
 * | 用例描述 | 测试warp timeline按warp数字序号排序显示
 */
TEST(WarpTimelineVisualize, timeline_to_json_sorts_warp_threads_by_numeric_id) {
    GlobalMockObject::verify();
    const std::string testDir = "test/ut/resources/op_profiling/warp_timeline_visualize_sort";
    std::experimental::filesystem::remove_all(testDir);
    const std::string outputPath = JoinPath({testDir, "device0/add/0"});
    std::vector<std::pair<uint64_t, uint64_t>> warpTimes(11, {0, 0});
    warpTimes[0] = {100, 160};
    warpTimes[1] = {110, 170};
    warpTimes[10] = {120, 180};
    WriteWarpTimelineBin(outputPath, warpTimes);
    ScopedAicoreFreqMock freqMock;

    WarpTimelineVisualize warpTimelineVisualize;
    ASSERT_TRUE(warpTimelineVisualize.TimelineToJson(outputPath));

    nlohmann::json traceJson = warpTimelineVisualize.GetTimelineJson();
    ASSERT_EQ(traceJson["traceEvents"].size(), 6);
    EXPECT_EQ(traceJson["traceEvents"][3]["name"], "thread_sort_index");
    EXPECT_EQ(traceJson["traceEvents"][3]["tid"], "Warp 0");
    EXPECT_EQ(traceJson["traceEvents"][3]["args"]["sort_index"], 0);
    EXPECT_EQ(traceJson["traceEvents"][4]["name"], "thread_sort_index");
    EXPECT_EQ(traceJson["traceEvents"][4]["tid"], "Warp 1");
    EXPECT_EQ(traceJson["traceEvents"][4]["args"]["sort_index"], 1);
    EXPECT_EQ(traceJson["traceEvents"][5]["name"], "thread_sort_index");
    EXPECT_EQ(traceJson["traceEvents"][5]["tid"], "Warp 10");
    EXPECT_EQ(traceJson["traceEvents"][5]["args"]["sort_index"], 10);

    std::experimental::filesystem::remove_all(testDir);
    GlobalMockObject::verify();
}

/**
 * |  用例集  | WarpTimelineVisualize
 * | 测试函数 | WarpTimelineVisualize::TimelineToJson
 * |  用例名  | timeline_to_json_normalizes_each_core_warps_to_min_start
 * | 用例描述 | 测试warp timeline按core分别对齐到统一起点
 */
TEST(WarpTimelineVisualize, timeline_to_json_normalizes_each_core_warps_to_min_start) {
    GlobalMockObject::verify();
    const std::string testDir = "test/ut/resources/op_profiling/warp_timeline_visualize_min";
    std::experimental::filesystem::remove_all(testDir);
    const std::string outputPath = JoinPath({testDir, "device0/add/0"});
    WriteWarpTimelineBin(outputPath, {{300, 360}, {340, 380}}, 0, TEST_CORE_TYPE_VEC0);
    WriteWarpTimelineBin(outputPath, {{100, 180}}, 0, TEST_CORE_TYPE_VEC1, true);
    ScopedAicoreFreqMock freqMock;

    WarpTimelineVisualize warpTimelineVisualize;
    ASSERT_TRUE(warpTimelineVisualize.TimelineToJson(outputPath));

    nlohmann::json traceJson = warpTimelineVisualize.GetTimelineJson();
    ASSERT_EQ(traceJson["traceEvents"].size(), 6);
    EXPECT_DOUBLE_EQ(traceJson["traceEvents"][0]["ts"].get<double>(), 0.0);
    EXPECT_DOUBLE_EQ(traceJson["traceEvents"][0]["dur"].get<double>(), 0.06);
    EXPECT_EQ(traceJson["traceEvents"][0]["pid"], "warp.core0.veccore0");
    EXPECT_NEAR(traceJson["traceEvents"][1]["ts"].get<double>(), 0.04, 1e-9);
    EXPECT_DOUBLE_EQ(traceJson["traceEvents"][1]["dur"].get<double>(), 0.04);
    EXPECT_EQ(traceJson["traceEvents"][1]["pid"], "warp.core0.veccore0");
    EXPECT_DOUBLE_EQ(traceJson["traceEvents"][2]["ts"].get<double>(), 0.0);
    EXPECT_DOUBLE_EQ(traceJson["traceEvents"][2]["dur"].get<double>(), 0.08);
    EXPECT_EQ(traceJson["traceEvents"][2]["pid"], "warp.core0.veccore1");

    std::experimental::filesystem::remove_all(testDir);
    GlobalMockObject::verify();
}

/**
 * |  用例集  | WarpTimelineVisualize
 * | 测试函数 | WarpTimelineVisualize::TimelineToJson
 * |  用例名  | timeline_to_json_aligns_warp_start_to_external_trace_min
 * | 用例描述 | 测试warp timeline使用外部trace最小起点做对齐
 */
TEST(WarpTimelineVisualize, timeline_to_json_aligns_warp_start_to_external_trace_min) {
    GlobalMockObject::verify();
    const std::string testDir = "test/ut/resources/op_profiling/warp_timeline_visualize_external_min";
    std::experimental::filesystem::remove_all(testDir);
    const std::string outputPath = JoinPath({testDir, "device0/add/0"});
    WriteWarpTimelineBin(outputPath, {{300, 360}}, 0, TEST_CORE_TYPE_VEC0);
    WriteWarpTimelineBin(outputPath, {{100, 180}}, 0, TEST_CORE_TYPE_VEC1, true);
    ScopedAicoreFreqMock freqMock;

    WarpTimelineVisualize warpTimelineVisualize;
    ASSERT_TRUE(warpTimelineVisualize.TimelineToJson(outputPath, 7.5));

    nlohmann::json traceJson = warpTimelineVisualize.GetTimelineJson();
    ASSERT_EQ(traceJson["traceEvents"].size(), 4);
    EXPECT_NEAR(traceJson["traceEvents"][0]["ts"].get<double>(), 7.5, 1e-9);
    EXPECT_NEAR(traceJson["traceEvents"][1]["ts"].get<double>(), 7.5, 1e-9);

    std::experimental::filesystem::remove_all(testDir);
    GlobalMockObject::verify();
}

/**
 * |  用例集  | WarpTimelineVisualize
 * | 测试函数 | WarpTimelineVisualize::TimelineToJson
 * |  用例名  | timeline_to_json_reports_unexpected_cube_warp_records
 * | 用例描述 | 测试warp timeline不静默过滤异常cube记录
 */
TEST(WarpTimelineVisualize, timeline_to_json_reports_unexpected_cube_warp_records) {
    GlobalMockObject::verify();
    const std::string testDir = "test/ut/resources/op_profiling/warp_timeline_visualize_cube";
    std::experimental::filesystem::remove_all(testDir);
    const std::string outputPath = JoinPath({testDir, "device0/add/0"});
    WriteWarpTimelineBin(outputPath, {{300, 360}}, 0, TEST_CORE_TYPE_CUBE0);
    WriteWarpTimelineBin(outputPath, {{100, 160}}, 0, TEST_CORE_TYPE_VEC0, true);
    ScopedAicoreFreqMock freqMock;

    WarpTimelineVisualize warpTimelineVisualize;
    ASSERT_TRUE(warpTimelineVisualize.TimelineToJson(outputPath));

    nlohmann::json traceJson = warpTimelineVisualize.GetTimelineJson();
    ASSERT_EQ(traceJson["traceEvents"].size(), 4);
    EXPECT_EQ(traceJson["traceEvents"][0]["pid"], "warp.core0.cubecore0");
    EXPECT_DOUBLE_EQ(traceJson["traceEvents"][0]["ts"].get<double>(), 0.0);
    EXPECT_DOUBLE_EQ(traceJson["traceEvents"][0]["dur"].get<double>(), 0.06);
    EXPECT_EQ(traceJson["traceEvents"][0]["args"]["core_type"], TEST_CORE_TYPE_CUBE0);
    EXPECT_EQ(traceJson["traceEvents"][1]["pid"], "warp.core0.veccore0");
    EXPECT_DOUBLE_EQ(traceJson["traceEvents"][1]["ts"].get<double>(), 0.0);
    EXPECT_DOUBLE_EQ(traceJson["traceEvents"][1]["dur"].get<double>(), 0.06);
    EXPECT_EQ(traceJson["traceEvents"][1]["args"]["core_type"], TEST_CORE_TYPE_VEC0);

    std::experimental::filesystem::remove_all(testDir);
    GlobalMockObject::verify();
}

/**
 * |  用例集  | DataVisualize
 * | 测试函数 | DataVisualize::MergeTraceJson
 * |  用例名  | merge_trace_json_appends_warp_events_without_overwriting_metadata
 * | 用例描述 | 测试合并warp trace时追加事件且不覆盖基础元数据
 */
TEST(DataVisualize, merge_trace_json_appends_warp_events_without_overwriting_metadata) {
    nlohmann::json baseTrace = MakeTracePayload("op-biuperf", "base_event");
    nlohmann::json warpTrace = MakeTracePayload("op", "warp_event");

    DataVisualize::MergeTraceJson(baseTrace, warpTrace);

    ASSERT_TRUE(baseTrace.contains("traceEvents"));
    ASSERT_TRUE(baseTrace["traceEvents"].is_array());
    EXPECT_EQ(baseTrace["traceEvents"].size(), 2);
    EXPECT_EQ(baseTrace["traceEvents"][0]["name"], "base_event");
    EXPECT_EQ(baseTrace["traceEvents"][1]["name"], "warp_event");
    EXPECT_EQ(baseTrace["profilingType"], "op-biuperf");
    EXPECT_EQ(baseTrace["displayTimeUnit"], "ns");
    EXPECT_EQ(baseTrace["schemaVersion"], 1);
}

/**
 * |  用例集  | DataVisualize
 * | 测试函数 | DataVisualize::MergeTraceJson
 * |  用例名  | merge_trace_json_uses_warp_payload_when_base_trace_missing
 * | 用例描述 | 测试基础trace缺失时直接使用warp payload作为最终trace
 */
TEST(DataVisualize, merge_trace_json_uses_warp_payload_when_base_trace_missing) {
    nlohmann::json baseTrace;
    nlohmann::json warpTrace = MakeTracePayload("op", "warp_event");

    DataVisualize::MergeTraceJson(baseTrace, warpTrace);

    ASSERT_TRUE(baseTrace.contains("traceEvents"));
    EXPECT_EQ(baseTrace["profilingType"], "op");
    EXPECT_EQ(baseTrace["traceEvents"].size(), 1);
    EXPECT_EQ(baseTrace["traceEvents"][0]["name"], "warp_event");
}

/**
 * |  用例集  | DataVisualize
 * | 测试函数 | DataVisualize::MergeTraceJson
 * |  用例名  | merge_trace_json_ignores_invalid_warp_payload
 * | 用例描述 | 测试warp payload无效时不会污染已有trace数据
 */
TEST(DataVisualize, merge_trace_json_ignores_invalid_warp_payload) {
    nlohmann::json baseTrace = MakeTracePayload("op-biuperf", "base_event");
    nlohmann::json beforeMerge = baseTrace;
    nlohmann::json invalidWarpTrace;
    invalidWarpTrace["profilingType"] = "op";
    invalidWarpTrace["traceEvents"] = nlohmann::json::array();

    DataVisualize::MergeTraceJson(baseTrace, invalidWarpTrace);
    EXPECT_EQ(baseTrace, beforeMerge);
}

/**
 * |  用例集  | LcclTimelineParser
 * | 测试函数 | LcclTimelineParser::ProcessAicoreData
 * |  用例名  | test_lccl_process_data_correct
 * | 用例描述 | 测试处理LCCL算子AICORE数据后生成正确的timeline事件
 */
TEST(LcclTimelineParser, test_lccl_process_data_correct)
{
    unique_ptr<DataHandler> handler = Utility::MakeUnique<DataHandlerOf910B>();
    shared_ptr<Visualize::OpBasicInfo> opBasicInfoPtr = Utility::MakeShared<Visualize::OpBasicInfo>(handler);
    shared_ptr<Visualize::BasicPmu> basicPmuPtr = Utility::MakeShared<Visualize::BasicPmu>(handler);

    LcclTimelineParser parser{0, opBasicInfoPtr, basicPmuPtr};

    parser.blockSystemTimes_  = {{0, {{48117105577516, 48117105578916}, {48117105577516, 48117105581350},
                                                 {48117105577516, 48117105581921}}}};
    LcclDumpLogInfo info1 = {1, 0, 48117105578953};
    LcclDumpLogInfo info2 = {1, 0, 48117105578999};
    LcclDumpLogInfo info3 = {2, 0, 48117105579953};
    LcclDumpLogInfo info4 = {2, 0, 48117105579999};
    vector<LcclDumpLogInfo> aicoreTimeStamps = {info1, info2, info3, info4};
    parser.ProcessAicoreData(aicoreTimeStamps);
    EXPECT_EQ(parser.timelineJson_.size(), 8);
}

/**
 * |  用例集  | LcclTimelineParser
 * | 测试函数 | LcclTimelineParser::TimelineToJson
 * |  用例名  | test_generate_lccl_timeline_correct
 * | 用例描述 | 测试LCCL算子通算流水图可正确生成traceEvents输出
 */
TEST(LcclTimelineParser, test_generate_lccl_timeline_correct)
{
    GlobalMockObject::verify();
    std::string testDir = "test/ut/resources/op_profiling/OPPO";
    auto outputPath = JoinPath({testDir, "device0/add/0"});
    MkdirRecusively(outputPath);
    unique_ptr<DataHandler> handler = Utility::MakeUnique<DataHandlerOf910B>();
    shared_ptr<Visualize::OpBasicInfo> opBasicInfoPtr = Utility::MakeShared<Visualize::OpBasicInfo>(handler);
    shared_ptr<Visualize::BasicPmu> basicPmuPtr = Utility::MakeShared<Visualize::BasicPmu>(handler);
    opBasicInfoPtr->soc_ = "Ascend910B1";
    LcclTimelineParser parser{0, opBasicInfoPtr, basicPmuPtr};
    MOCKER(&LcclTimelineParser::GetTimeStamp<LcclDumpLogInfo>).stubs().will(returnValue(true));
    ASSERT_TRUE(parser.TimelineToJson(outputPath));
    ASSERT_TRUE(parser.timelineJson_.contains("traceEvents"));
    ASSERT_TRUE(parser.timelineJson_["traceEvents"].is_array());
    std::experimental::filesystem::remove_all(testDir);
    GlobalMockObject::verify();
}

/**
/* | 用例集 | DataHandlerOf91095
/* |测试函数| Statics
/* | 用例名 | test_statics_mix_operator
/* |用例描述| 测试MIX算子类型的内存数据统计功能
*/
TEST(DataHandlerOf91095, test_statics_mix_operator) {
    // 创建测试对象
    DataHandlerOf91095 handler;

    // 设置 MIX 算子的 memMapDetail_
    handler.memMapDetail_.push_back({
        .blockId = 0,
        .blockType = "cube",
        .opType = Common::OpType::MIX
    });

    // 创建测试的内存记录
    std::vector<Common::MemRecord> memoryRecords;

    // 1. 添加 GM -> REG 的内存访问记录 (subblockId = 0, vec0 子块)
    Common::MemRecord rec1 {};
    rec1.srcMemSize = 1024;
    rec1.blockId = 0;
    rec1.subBlockID = 0;
    rec1.src = Common::MemType::GM;
    rec1.dst = Common::MemType::REG;
    memoryRecords.push_back(rec1);

    // 2. 添加 UB -> L1 的内存访问记录 (subblockId = 1, vec1 子块)
    Common::MemRecord rec2 {};
    rec2.srcMemSize = 512;
    rec2.blockId = 1;
    rec2.subBlockID = 0;
    rec2.src = Common::MemType::UB;
    rec2.dst = Common::MemType::L1;
    memoryRecords.push_back(rec2);

    // 3. 添加 L1 -> UB 的内存访问记录 (subblockId = 2, cube 子块)
    Common::MemRecord rec3 {};
    rec3.srcMemSize = 256;
    rec3.blockId = 2;
    rec3.subBlockID = 0;
    rec3.src = Common::MemType::L1;
    rec3.dst = Common::MemType::UB;
    memoryRecords.push_back(rec3);

    // 4. 添加 REG -> GM 的内存访问记录 (subblockId = 0, vec0 子块)
    Common::MemRecord rec4 {};
    rec4.srcMemSize = 768;
    rec4.blockId = 0;
    rec4.subBlockID = 0;
    rec4.src = Common::MemType::REG;
    rec4.dst = Common::MemType::GM;
    memoryRecords.push_back(rec4);

    // 调用 Statics 函数
    std::map<uint64_t, std::map<std::string, uint64_t>> dataSize;
    handler.Statics(memoryRecords, dataSize);

    // 验证 vec0 数据统计 (subblockId = 0)
    auto &vec0Trans = handler.memMapDetail_[0].apiDataTransVolumeVec0;
    ASSERT_FALSE(vec0Trans.empty());
    ASSERT_TRUE(vec0Trans.find("GM_TO_DCACHE") != vec0Trans.end());
    ASSERT_TRUE(vec0Trans.find("DCACHE_TO_VEC") != vec0Trans.end());
    ASSERT_TRUE(vec0Trans.find("VEC_TO_DCACHE") != vec0Trans.end());
    ASSERT_TRUE(vec0Trans.find("DCACHE_TO_GM") != vec0Trans.end());
    ASSERT_TRUE(vec0Trans.find("GM_TO_DCACHE_DATA") != vec0Trans.end());
    ASSERT_TRUE(vec0Trans.find("DCACHE_TO_VEC_DATA") != vec0Trans.end());
    ASSERT_TRUE(vec0Trans.find("VEC_TO_DCACHE_DATA") != vec0Trans.end());
    ASSERT_TRUE(vec0Trans.find("DCACHE_TO_GM_DATA") != vec0Trans.end());

    // 验证 vec1 数据统计 (subblockId = 1)
    auto &vec1Trans = handler.memMapDetail_[0].apiDataTransVolumeVec1;
    ASSERT_FALSE(vec1Trans.empty());
    ASSERT_TRUE(vec1Trans.find("UB_TO_L1") != vec1Trans.end());
    ASSERT_TRUE(vec1Trans.find("UB_TO_L1_DATA") != vec1Trans.end());

    // 验证 cube 数据统计 (subblockId = 2)
    auto &cubeTrans = handler.memMapDetail_[0].ApiDataTransVolume_;
    ASSERT_FALSE(cubeTrans.empty());
    ASSERT_TRUE(cubeTrans.find("L1_TO_UB") != cubeTrans.end());
    ASSERT_TRUE(cubeTrans.find("L1_TO_UB_DATA") != cubeTrans.end());

    // 验证 dataSize 统计
    ASSERT_TRUE(dataSize.find(0) != dataSize.end());
    auto &aicore0Data = dataSize[0];
    ASSERT_FALSE(aicore0Data.empty());
    // 所有数据应该汇总到 aicoreId = 0
}

/**
/* | 用例集 | DataHandlerOf91095
/* |测试函数| Statics
/* | 用例名 | test_statics_non_mix_operator
/* |用例描述| 测试非MIX算子类型的内存数据统计功能
*/
TEST(DataHandlerOf91095, test_statics_non_mix_operator) {
    // 创建测试对象
    DataHandlerOf91095 handler;

    // 设置非 MIX 算子的 memMapDetail_
    handler.memMapDetail_.push_back({
        .blockId = 0,
        .blockType = "vector0",
        .opType = Common::OpType::VECTOR
    });

    // 创建测试的内存记录
    std::vector<Common::MemRecord> memoryRecords;

    // 1. 添加 GM -> UB 的内存访问记录 (subblockId = 0)
    Common::MemRecord rec1 {};
    rec1.srcMemSize = 2048;
    rec1.blockId = 0;
    rec1.subBlockID = 0;
    rec1.src = Common::MemType::GM;
    rec1.dst = Common::MemType::UB;
    memoryRecords.push_back(rec1);

    // 2. 添加 REG -> GM 的内存访问记录 (subblockId = 1)
    Common::MemRecord rec2 {};
    rec2.srcMemSize = 1024;
    rec2.blockId = 1;
    rec2.subBlockID = 0;
    rec2.src = Common::MemType::REG;
    rec2.dst = Common::MemType::GM;
    memoryRecords.push_back(rec2);

    // 3. 添加 UB -> VEC 的内存访问记录 (subblockId = 0)
    Common::MemRecord rec3 {};
    rec3.srcMemSize = 512;
    rec3.blockId = 0;
    rec3.subBlockID = 0;
    rec3.src = Common::MemType::UB;
    rec3.dst = Common::MemType::REG;
    memoryRecords.push_back(rec3);

    // 4. 添加 GM -> REG 的内存访问记录 (subblockId = 1, vec1)
    Common::MemRecord rec4 {};
    rec4.srcMemSize = 768;
    rec4.blockId = 1;
    rec4.subBlockID = 0;
    rec4.src = Common::MemType::GM;
    rec4.dst = Common::MemType::REG;
    memoryRecords.push_back(rec4);

    // 调用 Statics 函数
    std::map<uint64_t, std::map<std::string, uint64_t>> dataSize;
    handler.Statics(memoryRecords, dataSize);

    // 验证通用数据统计
    auto &transVolume = handler.memMapDetail_[0].ApiDataTransVolume_;
    ASSERT_FALSE(transVolume.empty());
    ASSERT_TRUE(transVolume.find("GM_TO_UB") != transVolume.end());
    ASSERT_TRUE(transVolume.find("GM_TO_UB_DATA") != transVolume.end());
    ASSERT_TRUE(transVolume.find("VEC_TO_DCACHE") != transVolume.end());
    ASSERT_TRUE(transVolume.find("DCACHE_TO_GM") != transVolume.end());
    ASSERT_TRUE(transVolume.find("VEC_TO_DCACHE_DATA") != transVolume.end());
    ASSERT_TRUE(transVolume.find("DCACHE_TO_GM_DATA") != transVolume.end());
    ASSERT_TRUE(transVolume.find("UB_TO_VEC") != transVolume.end());
    ASSERT_TRUE(transVolume.find("UB_TO_VEC_DATA") != transVolume.end());
    ASSERT_TRUE(transVolume.find("GM_TO_DCACHE") != transVolume.end());
    ASSERT_TRUE(transVolume.find("DCACHE_TO_VEC") != transVolume.end());
    ASSERT_TRUE(transVolume.find("GM_TO_DCACHE_DATA") != transVolume.end());
    ASSERT_TRUE(transVolume.find("DCACHE_TO_VEC_DATA") != transVolume.end());

    // 验证非 MIX 算子不会填充 vec0/vec1 特定字段
    ASSERT_TRUE(handler.memMapDetail_[0].apiDataTransVolumeVec0.empty());
    ASSERT_TRUE(handler.memMapDetail_[0].apiDataTransVolumeVec1.empty());

    // 验证 dataSize 统计
    ASSERT_TRUE(dataSize.find(0) != dataSize.end());
    auto &aicore0Data = dataSize[0];
    ASSERT_FALSE(aicore0Data.empty());
    // 所有数据应该汇总到 aicoreId = 0
}

/**
/* | 用例集 | DataHandlerOf91095
/* |测试函数| Statics
/* | 用例名 | test_statics_empty_records
/* |用例描述| 测试空内存记录的处理
*/
TEST(DataHandlerOf91095, test_statics_empty_records) {
    // 创建测试对象
    DataHandlerOf91095 handler;

    // 设置 memMapDetail_
    handler.memMapDetail_.push_back({
        .blockId = 0,
        .blockType = "vector0",
        .opType = Common::OpType::VECTOR
    });

    // 空的内存记录
    std::vector<Common::MemRecord> memoryRecords;

    // 调用 Statics 函数
    std::map<uint64_t, std::map<std::string, uint64_t>> dataSize;
    handler.Statics(memoryRecords, dataSize);

    // 验证结果：应该为空
    ASSERT_TRUE(handler.memMapDetail_[0].ApiDataTransVolume_.empty());
    ASSERT_TRUE(handler.memMapDetail_[0].apiDataTransVolumeVec0.empty());
    ASSERT_TRUE(handler.memMapDetail_[0].apiDataTransVolumeVec1.empty());
    ASSERT_TRUE(dataSize.empty());
}
