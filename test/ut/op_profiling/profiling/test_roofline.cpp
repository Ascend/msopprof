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
#include <string>
#include "smart_pointer.h"
#define private public
#include "profiling/device/data_visualize/roofline.h"
#include "profiling/device/data_visualize/data_visualize.h"
#include "profiling/device/data_parse/device_data_parse.h"
#include "profiling/device/data_parse/metric_data_handler.h"
#include "profiling/device/data_visualize/pmu_calculator.h"
#include "common/prof_args.h"
#include "common/hal_helper.h"
#undef private

using namespace Visualize;
using namespace Profiling;
using namespace Common;
using namespace Utility;
using namespace std;

std::map<uint16_t, uint64_t> cubeEvents910b4 = {{73, 98400}, {74, 500}, {49, 31558}, {50, 30932}, {518, 0}, {33, 9999}};
std::map<uint16_t, uint64_t> vecEvents910b4 = {{75, 10240}, {76, 0}, {77, 0}, {174, 100}, {78, 0}, {186, 100},
                                               {79, 136}, {61, 40964}, {62, 40960}, {67, 20480}, {68, 20480},
                                               {1280, 1280}, {1282, 1282}, {1284, 1284}};

std::map<uint16_t, uint64_t> events310p3 = {{73, 98400}, {74, 500}, {49, 31558}, {50, 30932}, {75, 10240}, {76, 0},
                                            {77, 0}, {174, 100}, {78, 0}, {186, 100}, {79, 136}, {61, 40964}, {33, 9999},
                                            {62, 40960}, {67, 20480}, {68, 20480}, {28, 1280}, {34, 1282}, {44, 1284}};
int64_t freqRoofline = 1650;
int64_t aiCoreNumRoofline = 20;

unique_ptr<DataHandler> &GetHandleTest(const ChipType &type)
{
    static unique_ptr<DataHandler> handlePtr;
    if (type == ChipType::ASCEND910B) {
        handlePtr = Utility::MakeUnique<DataHandlerOf910B>();
    } else {
        handlePtr = Utility::MakeUnique<DataHandlerOf310P>();
    }
    return handlePtr;
}

inline shared_ptr<Visualize::OpBasicInfo> &GetOpBasicInfoObjTest(const ChipType &type, unique_ptr<DataHandler> &handler)
{
    static shared_ptr<Visualize::OpBasicInfo> opBasicInfoPtr = Utility::MakeShared<Visualize::OpBasicInfo>(handler);
    return opBasicInfoPtr;
}

inline shared_ptr<Visualize::BasicPmu> &GetBasicPmuObjTest(const ChipType &type, unique_ptr<DataHandler> &handler)
{
    static shared_ptr<Visualize::BasicPmu> basicPmuPtr = Utility::MakeShared<Visualize::BasicPmu>(handler);
    if (type == ChipType::ASCEND910B) {
        basicPmuPtr->totalPmuData_ = {
            {{0, "cube0"}, {cubeEvents910b4, 10000000, "cube"}},
            {{0, "vector0"}, {vecEvents910b4, 2000000, "vector"}},
            {{0, "vector1"}, {vecEvents910b4, 2000000, "vector"}}
        };
    } else {
        basicPmuPtr->totalPmuData_ = {
            { {{0, "AiCore"}, {events310p3, 10000000, "cube"}} }
        };
    }
    return basicPmuPtr;
}

inline unique_ptr<Visualize::PmuCalculator> &GetPmuCalculatorObjTest(const ChipType &type,
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

ChipType chipType = ChipType::ASCEND910B;
auto &handler = GetHandleTest(chipType);
auto &opBasicInfoObj = GetOpBasicInfoObjTest(chipType, handler);
auto &basicPmuObj = GetBasicPmuObjTest(chipType, handler);
auto &pmuCalculatorObj = GetPmuCalculatorObjTest(chipType, opBasicInfoObj, basicPmuObj);
RoofLineOf910B roofLineTest(freqRoofline, aiCoreNumRoofline, opBasicInfoObj, basicPmuObj, pmuCalculatorObj);


TEST(RoofLine, Init_test_cal_all_core_events_total_value_success)
{
    ChipType chipType = ChipType::ASCEND910B;
    auto &handler = GetHandleTest(chipType);
    auto &opBasicInfoObj = GetOpBasicInfoObjTest(chipType, handler);
    auto &basicPmuObj = GetBasicPmuObjTest(chipType, handler);
    auto &pmuCalculatorObj = GetPmuCalculatorObjTest(chipType, opBasicInfoObj, basicPmuObj);
    RoofLineOf910B roofLine(freqRoofline, aiCoreNumRoofline, opBasicInfoObj, basicPmuObj, pmuCalculatorObj);
    roofLine.Init();
    EXPECT_EQ(roofLine.pmuValues_[Event::CUBE_FP_EXECUTED], 98400);
    EXPECT_EQ(roofLine.pmuValues_[Event::VEC_FP32_EXECUTED], 10240 * 2);
    EXPECT_EQ(roofLine.pmuValues_[Event::VEC_MISC_EXECUTED], 136 * 2);
}

TEST(RoofLine, CalCubeBaseData_test_cal_all_core_base_cube_value_success)
{
    ChipType chipType = ChipType::ASCEND910B;
    auto &handler = GetHandleTest(chipType);
    auto &opBasicInfoObj = GetOpBasicInfoObjTest(chipType, handler);
    auto &basicPmuObj = GetBasicPmuObjTest(chipType, handler);
    opBasicInfoObj->SetBlockDetail();
    auto &pmuCalculatorObj = GetPmuCalculatorObjTest(chipType, opBasicInfoObj, basicPmuObj);
    RoofLineOf910B roofLine(freqRoofline, aiCoreNumRoofline, opBasicInfoObj, basicPmuObj, pmuCalculatorObj);
    roofLine.Init();
    int64_t cubeNum = 20;
    roofLine.CalCubeBaseData(cubeNum);
    // 9999 * CUBE_FOPS_INT8_NUM
    uint64_t fops = 163823616;
    EXPECT_EQ(roofLine.cubeProperty_.fops, fops);
    EXPECT_EQ(roofLine.cubeProperty_.computilityName, "Cube_INT(100%)");
    // fops*1650*cubeNum/(9999)/1000000
    EXPECT_FLOAT_EQ(roofLine.cubeProperty_.theoryTfops, 540.671997);
}

TEST(RoofLine, CalVecBaseData_test_cal_all_core_base_cube_value_success)
{
    ChipType chipType = ChipType::ASCEND910B;
    auto &handler = GetHandleTest(chipType);
    auto &opBasicInfoObj = GetOpBasicInfoObjTest(chipType, handler);
    auto &basicPmuObj = GetBasicPmuObjTest(chipType, handler);
    opBasicInfoObj->SetBlockDetail();
    auto &pmuCalculatorObj = GetPmuCalculatorObjTest(ChipType::ASCEND910B, opBasicInfoObj, basicPmuObj);
    RoofLineOf910B roofLine(freqRoofline, aiCoreNumRoofline, opBasicInfoObj, basicPmuObj, pmuCalculatorObj);
    roofLine.Init();
    int64_t vectorNum = 40;
    roofLine.CalVecBaseData(vectorNum);
    // (10240 * 64 + 100 * 32 + 100 * 128 + 136 * 32) * 2
    uint64_t fops = 1351424;
    EXPECT_EQ(roofLine.vecProperty_.fops, fops);
    EXPECT_EQ(roofLine.vecProperty_.computilityName, "Vec_FP16_32(0.945537%),Vec_FP32(96.822998%),Vec_MISC(1.285930%),"
                                                     "Vec_S16(0.945537%)");
    // fops*1650*vectorNum/(2*(10240+100+100+136))/1000000
    EXPECT_FLOAT_EQ(roofLine.vecProperty_.theoryTfops, 4.21681);
}

TEST(RoofLine, GenerateRoofLines_test_910b_cal_rooflines_success)
{
    ChipType chipType = ChipType::ASCEND910B;
    auto &handler = GetHandleTest(chipType);
    auto &opBasicInfoObj = GetOpBasicInfoObjTest(chipType, handler);
    auto &basicPmuObj = GetBasicPmuObjTest(chipType, handler);
    opBasicInfoObj->SetBlockDetail();
    opBasicInfoObj->opType_ = OpType::MIX;
    opBasicInfoObj->duration_ = 120;
    opBasicInfoObj->soc_ = "Ascend910B4";
    auto &pmuCalculatorObj = GetPmuCalculatorObjTest(chipType, opBasicInfoObj, basicPmuObj);
    RoofLineOf910B roofLine(freqRoofline, aiCoreNumRoofline, opBasicInfoObj, basicPmuObj, pmuCalculatorObj);

    vector<nlohmann::json> jsonData = roofLine.GenerateRoofLines();
    nlohmann::json l1Total;
    nlohmann::json ubTotal;
    nlohmann::json l2Cache;
    for (auto rooflines: jsonData) {
        string title = rooflines.at("title");
        for (auto line: rooflines.at("rooflines")) {
            if (line.at("bw_name") == "L1 Read + Write") {
                l1Total = line;
            }
            if (line.at("bw_name") == "UB Read + Write") {
                ubTotal = line;
            }
            if (line.at("bw_name") == "L2 Read + Write") {
                l2Cache = line;
            }
        }
    }

    // test CubeMemoryUnit
    EXPECT_FLOAT_EQ(l1Total.at("bw"), 15.366822);
    EXPECT_EQ(l1Total.at("computility_name"), "Cube_INT(100%)");
    EXPECT_FLOAT_EQ(l1Total.at("computility"), 540.671997);
    // 814284800/((pmu(50)-pmu(518))*256+pmu(49)*256+pmu(518)*128)
    EXPECT_FLOAT_EQ(l1Total.at("point")[0], 10.2406149);
    // 814284800/Duration
    EXPECT_FLOAT_EQ(l1Total.at("point")[1], 1.36519682);

    // test VecMemoryUnit
    EXPECT_FLOAT_EQ(ubTotal.at("bw"), 30.733645);
    EXPECT_EQ(ubTotal.at("computility_name"), "Vec_FP16_32(0.945537%),Vec_FP32(96.822998%),Vec_MISC(1.285930%),"
                                              "Vec_S16(0.945537%)");
    EXPECT_FLOAT_EQ(ubTotal.at("computility"), 4.21681);
    // 1351424/(pmu(61)*128+pmu(62)*128+pmu(67)*256+pmu(68)*256)
    EXPECT_FLOAT_EQ(ubTotal.at("point")[0], 0.03221967);
    // 1351424/Duration
    EXPECT_FLOAT_EQ(ubTotal.at("point")[1], 0.01126187);

    // test GmAndL2cache
    EXPECT_FLOAT_EQ(l2Cache.at("bw"), 4.0);
    EXPECT_EQ(l2Cache.at("computility_name"), "Cube_INT(100%) + Vec_FP16_32(0.945537%),Vec_FP32(96.822998%),Vec_MISC(1.285930%),Vec_S16(0.945537%)");
    EXPECT_FLOAT_EQ(l2Cache.at("computility"), 544.888794);
    // (814284800+1351424)/(pmu(1280)*512+pmu((1284)+pmu(1288))*128)
    EXPECT_FLOAT_EQ(l2Cache.at("point")[0], 55.949966);
    // (814284800+1351424)/Duration
    EXPECT_FLOAT_EQ(l2Cache.at("point")[1], 1.37645864);
}

TEST(RoofLine, GetTheoryBwByGmType_Return_True)
{
    std::map<std::string, float> gmBw;
    std::map<std::string, float> gmBwCJ;
    HalHelper::Instance().gmType_ = GmType::CJ;
    roofLineTest.GetTheoryBwByGmType(gmBwCJ);
    HalHelper::Instance().gmType_ = GmType::DEFAULT;
    roofLineTest.GetTheoryBwByGmType(gmBw);
    EXPECT_TRUE(abs(gmBw["Ascend910B1"] - 1.8) < 0.001);
    EXPECT_TRUE(abs(gmBwCJ["Ascend910B1"] - 1.34) < 0.001);
}

TEST(RoofLine, GetTheoryL2CacheByGmType_Return_True)
{

    std::map<std::string, float> l2CacheBw;
    std::map<std::string, float> l2CacheBwCJ;
    HalHelper::Instance().gmType_ = GmType::CJ;
    roofLineTest.GetTheoryL2CacheByGmType(l2CacheBwCJ);
    HalHelper::Instance().gmType_ = GmType::DEFAULT;
    roofLineTest.GetTheoryL2CacheByGmType(l2CacheBw);
    EXPECT_TRUE(abs(l2CacheBw["Ascend910B1"] - 8) < 0.001);
    EXPECT_TRUE(abs(l2CacheBwCJ["Ascend910B1"] - 7.4) < 0.001);
}

TEST(RoofLine, InsertMemPipeMaxBw_return_expect_value)
{
    ChipType chipType = ChipType::ASCEND910B;
    auto &handler = GetHandleTest(chipType);
    auto &opBasicInfoObj = GetOpBasicInfoObjTest(chipType, handler);
    auto &basicPmuObj = GetBasicPmuObjTest(chipType, handler);
    opBasicInfoObj->soc_ = "Ascend910B1";
    auto &pmuCalculatorObj = GetPmuCalculatorObjTest(chipType, opBasicInfoObj, basicPmuObj);
    RoofLineOf910B roofLine(freqRoofline, aiCoreNumRoofline, opBasicInfoObj, basicPmuObj, pmuCalculatorObj);
    
    HalHelper::Instance().gmType_ = GmType::CJ;
    roofLine.InsertMemPipeMaxBw(1);
    EXPECT_FLOAT_EQ(roofLine.maxBwRates_["L1 to GM"], 187.72 / BIT_CONVERSION);
    HalHelper::Instance().gmType_ = GmType::DEFAULT;
    roofLine.InsertMemPipeMaxBw(1);
    EXPECT_FLOAT_EQ(roofLine.maxBwRates_["L1 to GM"], 199.43 / BIT_CONVERSION);
}

TEST(RoofLine, generaterooflines_test_advice_rooflines_success)
{
    ChipType chipType = ChipType::ASCEND910B;
    auto &handler = GetHandleTest(chipType);
    auto &opBasicInfoObj = GetOpBasicInfoObjTest(chipType, handler);
    auto &basicPmuObj = GetBasicPmuObjTest(chipType, handler);
    auto &pmuCalculatorObj = GetPmuCalculatorObjTest(chipType, opBasicInfoObj, basicPmuObj);
    RoofLineOf910B roofLine(freqRoofline, aiCoreNumRoofline, opBasicInfoObj, basicPmuObj, pmuCalculatorObj);

    string adviceComputeBound = roofLine.RoofLineAnalysis(1.0, 2.0, 1.9, 1.5);
    roofLine.SetPipeLineRatio("MTE1", 77.0);
    roofLine.SetPipeLineRatio("MTE2", 78.0);
    EXPECT_EQ(roofLine.GenerateAdvice(), "latency bound:pipeline caused");

    roofLine.SetPipeLineRatio("MTE3", 82.0);
    EXPECT_EQ(roofLine.GenerateAdvice(), "latency bound:memory caused");

    roofLine.SetPipeLineRatio("Cube", 87.0);
    EXPECT_EQ(roofLine.GenerateAdvice(), "latency bound:compute caused");

    adviceComputeBound = roofLine.RoofLineAnalysis(1.0, 2.0, 3.0, 1.65);
    EXPECT_EQ(roofLine.GenerateAdvice(), "compute bound");

    adviceComputeBound = roofLine.RoofLineAnalysis(1.0, 2.0, 1.9, 1.7);
    EXPECT_EQ(roofLine.GenerateAdvice(), "memory bound");
}

TEST(RoofLine, generaterooflines_test_310b_cal_rooflines_success)
{
    ChipType chipType = ChipType::ASCEND310P;
    auto &handler = GetHandleTest(chipType);
    auto &opBasicInfoObj = GetOpBasicInfoObjTest(chipType, handler);
    auto &basicPmuObj = GetBasicPmuObjTest(chipType, handler);
    opBasicInfoObj->SetBlockDetail();
    opBasicInfoObj->opType_ = OpType::AI_CORE;
    opBasicInfoObj->duration_ = 120;
    opBasicInfoObj->soc_ = "Ascend310P3";
    auto &pmuCalculatorObj = GetPmuCalculatorObjTest(chipType, opBasicInfoObj, basicPmuObj);
    RoofLineOf310P roofLine(freqRoofline, aiCoreNumRoofline, opBasicInfoObj, basicPmuObj, pmuCalculatorObj);

    roofLine.RoofLineToJson();
    nlohmann::json l1Total;
    nlohmann::json ubTotal;
    nlohmann::json vectorReadUb;
    for (auto rooflines: roofLine.visualRoofLineJson_["multiple_rooflines"]) {
        string title = rooflines.at("title");
        for (auto line: rooflines.at("rooflines")) {
            if (line.at("bw_name") == "L1 Read + Write") {
                l1Total = line;
            }
            if (line.at("bw_name") == "UB Read + Write") {
                ubTotal = line;
            }
            if (line.at("bw_name") == "Vector Read UB") {
                vectorReadUb = line;
            }
        }
    }

    // test L1 Read + Write
    EXPECT_FLOAT_EQ(l1Total.at("bw"), 23.0502338);
    EXPECT_EQ(l1Total.at("computility_name"), "Cube_INT(100%) + Vec_FP16_32(0.945537%),Vec_FP32(96.822998%),Vec_MISC(1.285930%),Vec_S16(0.945537%)");
    EXPECT_FLOAT_EQ(l1Total.at("computility"), 542.743652);
    EXPECT_FLOAT_EQ(l1Total.at("point")[0], 6.83193159);
    EXPECT_FLOAT_EQ(l1Total.at("point")[1], 1.37072957);

    // test UB Read + Write
    EXPECT_FLOAT_EQ(ubTotal.at("bw"), 7.6834111);
    EXPECT_EQ(ubTotal.at("computility_name"), "Cube_INT(100%) + Vec_FP16_32(0.945537%),Vec_FP32(96.822998%),Vec_MISC(1.285930%),Vec_S16(0.945537%)");
    EXPECT_FLOAT_EQ(ubTotal.at("computility"), 542.743652);
    EXPECT_FLOAT_EQ(ubTotal.at("point")[0], 83.6599731);
    EXPECT_FLOAT_EQ(ubTotal.at("point")[1], 1.37072957);

    // test Vector Read UB
    EXPECT_FLOAT_EQ(vectorReadUb.at("bw"), 15.366822);
    EXPECT_EQ(vectorReadUb.at("computility_name"), "Cube_INT(100%) + Vec_FP16_32(0.945537%),Vec_FP32(96.822998%),Vec_MISC(1.285930%),Vec_S16(0.945537%)");
    EXPECT_FLOAT_EQ(vectorReadUb.at("computility"), 542.743652);
    EXPECT_FLOAT_EQ(vectorReadUb.at("point")[0], 501.976166);
    EXPECT_FLOAT_EQ(vectorReadUb.at("point")[1], 1.37072957);

    EXPECT_EQ(roofLine.visualRoofLineJson_["advice"], "memory bound");
}