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

#include <string>
#include "smart_pointer.h"
#define private public
#define protected public
#include "profiling/device/data_visualize/data_visualize.h"
#include "profiling/device/data_visualize/occupancy.h"
#include "profiling/device/data_visualize/pmu_calculator.h"
#include "profiling/device/data_visualize/mc2_timeline_parser.h"
#include "profiling/device/data_visualize/lccl_timeline_parser.h"
#include "common/hal_helper.h"
#undef private
#undef protected

using namespace Visualize;
using namespace Profiling;
using namespace Common;
using namespace Utility;
using namespace std;

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

TEST(DataVisualize, occupancy_analysis_occupy_and_expect_success)
{
    std::vector<std::pair<std::string, uint64_t>> cyclesOccupancy = {
        {"0vector0", 100}, {"1vector0", 101}, {"2vector0", 98}, {"3vector0", 200}, {"4vector0", 113}
    };
    std::vector<std::pair<std::string, float>> throughputOccupancy = {
        {"0cube0", 1000}, {"1cube0", 1082}, {"2cube0", 1000}, {"3cube0", 1279}, {"4cube0", 1700}
    };
    std::vector<std::pair<std::string, float>> cacheHitRateOccupancy = {
        {"0vector0", 70}, {"0vector1", 72}, {"1vector0", 70}, {"1vector1", 90}
    };

    std::vector<std::string> advicesCycles;
    std::vector<std::string> advicesThroughput;
    std::vector<std::string> advicesCacheHitRate;
    
    occupancy.opType_ = Common::OpType::VECTOR;
    advicesCycles = occupancy.AnalysisOccupy(cyclesOccupancy,
        Visualize::OccupancyDataType::OCPY_CYCLES);
    occupancy.opType_ = Common::OpType::CUBE;
    advicesThroughput = occupancy.AnalysisOccupy(throughputOccupancy,
        Visualize::OccupancyDataType::OCPY_THROUGHPUT);
    occupancy.opType_ = Common::OpType::MIX;
    advicesCacheHitRate = occupancy.AnalysisOccupy(cacheHitRateOccupancy,
        Visualize::OccupancyDataType::OCPY_CACHE_HIT_RATE);
    
    std::vector<std::string> cyclesRes = {{"core1 vector1 took more time than other vector cores"}};
    std::vector<std::string> throughtputRes = {{"core4 cube0 write/read more data than other cube cores"}};
    std::vector<std::string> cacheHitRateRes = {{"core0 vector0 cache hit rate lower than other vector cores"},
                                                {"core1 vector0 cache hit rate lower than other vector cores"}};

    EXPECT_EQ(advicesCycles[0], cyclesRes[0]);
    EXPECT_EQ(advicesThroughput[0], throughtputRes[0]);
    EXPECT_EQ(advicesCacheHitRate[0], cacheHitRateRes[0]);
    EXPECT_EQ(advicesCacheHitRate[1], cacheHitRateRes[1]);
}

TEST(DataVisualize, 910b_gen_occupy_and_expect_success)
{
    std::string soc = "Ascend910B1";
    nlohmann::json occupancyMapJson;
    bool res = occupancy.GetOccupancyMap(occupancyMapJson);
    ASSERT_TRUE(res == true);
}

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

TEST(DataVisualize, occupancy_getOccupancyBlockJson_expect_success)
{
    occupancy.opType_ = Common::OpType::MIX;
    MemMapDetail test = mixPmuMapDetails;
    auto res1 = occupancy.GetOccupancyBlockJson(test);
    ASSERT_TRUE(res1.size()==3);
}

TEST(DeviceDataParse, SaveOpBasicInfo_Return_True)
{
    GlobalMockObject::verify();
    DataHandler dataHandler;
    const std::string path = "test/ut/resources/op_profiling/device910B";
    HalHelper::Instance().handleDcmi_ = nullptr;
    ASSERT_TRUE(dataHandler.SaveOpBasicInfo(path));
}

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
    uint64_t l0aDatas = 0;
    uint64_t l0bDatas = 0;
    uint64_t l0cToGmDatas = 0;
    uint64_t l0cToL1Datas = 0;
    // 有效可用通路为MTE1\MTE2\MTE3\MTE2 Vector\MTE3 Vector 5种
    auto resB1 = pmuCalculatorObj->GetPipeBwByWeight("Ascend910B1", l0aDatas, l0bDatas, l0cToGmDatas, l0cToL1Datas);
    EXPECT_FLOAT_EQ(resB1["MTE1"], 324.0);
    EXPECT_FLOAT_EQ(resB1["MTE2"], 340.1);
    EXPECT_FLOAT_EQ(resB1["MTE3"], 199.43);
    EXPECT_FLOAT_EQ(resB1["MTE2 vector"], 220.06);
    EXPECT_FLOAT_EQ(resB1["MTE3 vector"], 186.8);
    // B2=B3
    auto resB2 = pmuCalculatorObj->GetPipeBwByWeight("Ascend910B2", l0aDatas, l0bDatas, l0cToGmDatas, l0cToL1Datas);
    auto resB3 = pmuCalculatorObj->GetPipeBwByWeight("Ascend910B3", l0aDatas, l0bDatas, l0cToGmDatas, l0cToL1Datas);
    EXPECT_FLOAT_EQ(resB2["MTE1"], 315.245);
    EXPECT_FLOAT_EQ(resB2["MTE2"], 330.91);
    EXPECT_FLOAT_EQ(resB2["MTE3"], 193.99);
    EXPECT_FLOAT_EQ(resB2["MTE2 vector"], 214.11);
    EXPECT_FLOAT_EQ(resB2["MTE3 vector"], 181.75);
    for (auto &pipe : resB3) {
        EXPECT_FLOAT_EQ(pipe.second, resB2[pipe.first]);
    }
    auto resB4 = pmuCalculatorObj->GetPipeBwByWeight("Ascend910B4", l0aDatas, l0bDatas, l0cToGmDatas, l0cToL1Datas);
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

TEST(MC2TimelineParser, ProcessHcclDataSuccess)
{
    unique_ptr<DataHandler> handler = Utility::MakeUnique<DataHandlerOf910B>();
    shared_ptr<Visualize::OpBasicInfo> opBasicInfoPtr = Utility::MakeShared<Visualize::OpBasicInfo>(handler);
    shared_ptr<Visualize::BasicPmu> basicPmuPtr = Utility::MakeShared<Visualize::BasicPmu>(handler);
    MC2TimelineParser parser{true, acsqTimeMap, minAcsqTimeCyc, opBasicInfoPtr, basicPmuPtr};
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
    EXPECT_EQ(parser.traceEvents_.size(), 14);
    for (const auto &i: parser.traceEvents_) {
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

TEST(MC2TimelineParser, ProcessHcclTaskDataSuccess)
{
    unique_ptr<DataHandler> handler = Utility::MakeUnique<DataHandlerOf910B>();
    shared_ptr<Visualize::OpBasicInfo> opBasicInfoPtr = Utility::MakeShared<Visualize::OpBasicInfo>(handler);
    shared_ptr<Visualize::BasicPmu> basicPmuPtr = Utility::MakeShared<Visualize::BasicPmu>(handler);
    MC2TimelineParser parser{true, acsqTimeMap, minAcsqTimeCyc, opBasicInfoPtr, basicPmuPtr};
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
    EXPECT_EQ(parser.traceEvents_.size(), 9);
    for (const auto &i: parser.traceEvents_) {
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

TEST(MC2TimelineParser, ProcessAicpuTurnDataSuccess)
{
    unique_ptr<DataHandler> handler = Utility::MakeUnique<DataHandlerOf910B>();
    shared_ptr<Visualize::OpBasicInfo> opBasicInfoPtr = Utility::MakeShared<Visualize::OpBasicInfo>(handler);
    shared_ptr<Visualize::BasicPmu> basicPmuPtr = Utility::MakeShared<Visualize::BasicPmu>(handler);
    MC2TimelineParser parser{true, acsqTimeMap, minAcsqTimeCyc, opBasicInfoPtr, basicPmuPtr};
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
    EXPECT_EQ(parser.traceEvents_.size(), 16);
    for (const auto &i: parser.traceEvents_) {
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
/* | 用例集 | OpProf
/* |测试函数| CalPipeTime
/* | 用例名 | test_cal_pipe_time_when_timefactor_is_zero_and_expect_no_throw
/* |用例描述| 执行测试函数，当timefactor_为0时，不抛出异常
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
/* | 用例集 | OpProf
/* |测试函数| CalTransportBwUsageRate
/* | 用例名 | test_cal_transport_bw_usage_rate_when_timefactor_is_zero_and_expect_no_throw
/* |用例描述| 执行测试函数，当timefactor_为0时，不抛出异常
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
/* | 用例集 | DataVisualize
/* |测试函数| AnalysisOccupy
/* | 用例名 | test_analysis_occupy_when_given_zero_and_expect_no_throw
/* |用例描述| 执行测试函数，当occupy中最大为0时，不抛出异常
*/
TEST(DataVisualize, test_analysis_occupy_when_given_zero_and_expect_no_throw)
{
    std::vector<std::pair<std::string, uint64_t>> cyclesOccupancy = {
        {"0vector0", 0}, {"1vector0", 0}, {"2vector0", 0}
    };
    std::vector<std::pair<std::string, float>> throughputOccupancy = {
        {"0cube0", 0.0}, {"1cube0", 0.0}, {"2cube0", 0.0}
    };
    occupancy.opType_ = Common::OpType::VECTOR;
    ASSERT_NO_THROW(occupancy.AnalysisOccupy(cyclesOccupancy, Visualize::OccupancyDataType::OCPY_CYCLES));
    occupancy.opType_ = Common::OpType::CUBE;
    ASSERT_NO_THROW(occupancy.AnalysisOccupy(throughputOccupancy, Visualize::OccupancyDataType::OCPY_THROUGHPUT));
}

/**
/* | 用例集 | LcclTimelineParser
/* |测试函数| ProcessJsonData
/* | 用例名 | test_lccl_process_data_correct
/* |用例描述| 测试lccl算子通算流水图正确生成json数据
*/
TEST(LcclTimelineParser, test_lccl_process_data_correct)
{
    unique_ptr<DataHandler> handler = Utility::MakeUnique<DataHandlerOf910B>();
    shared_ptr<Visualize::OpBasicInfo> opBasicInfoPtr = Utility::MakeShared<Visualize::OpBasicInfo>(handler);
    shared_ptr<Visualize::BasicPmu> basicPmuPtr = Utility::MakeShared<Visualize::BasicPmu>(handler);

    LcclTimelineParser parser{true, 0, opBasicInfoPtr, basicPmuPtr};

    BlockSystemTimeType blockSystemTimes = {{0, {{48117105577516, 48117105578916}, {48117105577516, 48117105581350},
                                                 {48117105577516, 48117105581921}}}};
    LcclDumpLogInfo info1 = {1, 0, 48117105578953};
    LcclDumpLogInfo info2 = {1, 0, 48117105578999};
    LcclDumpLogInfo info3 = {2, 0, 48117105579953};
    LcclDumpLogInfo info4 = {2, 0, 48117105579999};
    vector<LcclDumpLogInfo> aicoreTimeStamps = {info1, info2, info3, info4};
    parser.ProcessJsonData(blockSystemTimes, aicoreTimeStamps);
    EXPECT_EQ(parser.traceEvents_.size(), 8);
}

/**
/* | 用例集 | LcclTimelineParser
/* |测试函数| TimelineToJson
/* | 用例名 | test_generate_lccl_timeline_correct
/* |用例描述| 测试lccl算子通算流水图正确输出流水图
*/
TEST(LcclTimelineParser, test_generate_lccl_timeline_correct)
{
    std::string testDir = "test/ut/resources/op_profiling/OPPO";
    auto outputPath = JoinPath({testDir, "device0/add/0"});
    MkdirRecusively(outputPath);
    unique_ptr<DataHandler> handler = Utility::MakeUnique<DataHandlerOf910B>();
    shared_ptr<Visualize::OpBasicInfo> opBasicInfoPtr = Utility::MakeShared<Visualize::OpBasicInfo>(handler);
    shared_ptr<Visualize::BasicPmu> basicPmuPtr = Utility::MakeShared<Visualize::BasicPmu>(handler);
    opBasicInfoPtr->soc_ = "Ascend910B1";
    LcclTimelineParser parser{true, 0, opBasicInfoPtr, basicPmuPtr};
    ASSERT_TRUE(parser.TimelineToJson(outputPath));
    string tracePath = JoinPath({outputPath, "trace.json"});
    ASSERT_TRUE(IsExist(tracePath));
    std::experimental::filesystem::remove_all(testDir);
}