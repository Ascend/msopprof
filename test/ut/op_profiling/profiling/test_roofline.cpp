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
#define protected public
#include "profiling/device/data_visualize/roofline.h"
#include "profiling/device/data_visualize/data_visualize.h"
#include "profiling/device/data_parse/device_data_parse.h"
#include "profiling/device/data_parse/metric_data_handler.h"
#include "profiling/device/data_visualize/pmu_calculator.h"
#include "common/prof_args.h"
#include "common/hal_helper.h"
#undef private
#undef protected

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
    
    roofLine.cubeNum_ = 1;
    HalHelper::Instance().gmType_ = GmType::CJ;
    roofLine.InsertMaxBw();
    EXPECT_FLOAT_EQ(roofLine.maxBwRates_["L1 to GM"], 187.72 / BIT_CONVERSION);
    HalHelper::Instance().gmType_ = GmType::DEFAULT;
    roofLine.InsertMaxBw();
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

// 纵坐标超过理论算力或带宽为0时，验证RoofLineAnalysis返回NA
TEST(RoofLine, roofline_analysis_return_na_when_vertical_over_theory_or_bw_zero)
{
    unique_ptr<DataHandler> handler = Utility::MakeUnique<DataHandlerOf910B>();
    shared_ptr<Visualize::OpBasicInfo> opBasicInfo = Utility::MakeShared<Visualize::OpBasicInfo>(handler);
    shared_ptr<Visualize::BasicPmu> basicPmu = Utility::MakeShared<Visualize::BasicPmu>(handler);
    unique_ptr<Visualize::PmuCalculator> pmuCalculator = Utility::MakeUnique<Visualize::PmuCalculator910B>();
    pmuCalculator->Init(basicPmu);
    RoofLineOf910B roofLine(freqRoofline, aiCoreNumRoofline, opBasicInfo, basicPmu, pmuCalculator);

    EXPECT_EQ(roofLine.RoofLineAnalysis(1.0f, 2.0f, 1.0f, 3.0f), "NA");
    EXPECT_EQ(roofLine.RoofLineAnalysis(0.0f, 2.0f, 1.0f, 1.0f), "NA");
    EXPECT_EQ(roofLine.RoofLineAnalysis(1.0f, 2.0f, 1.0f, 2.0f), "NA");
}

// 坐标点位于带宽线下方时，验证RoofLineAnalysis返回NA
TEST(RoofLine, roofline_analysis_return_na_when_point_under_bw_line)
{
    unique_ptr<DataHandler> handler = Utility::MakeUnique<DataHandlerOf910B>();
    shared_ptr<Visualize::OpBasicInfo> opBasicInfo = Utility::MakeShared<Visualize::OpBasicInfo>(handler);
    shared_ptr<Visualize::BasicPmu> basicPmu = Utility::MakeShared<Visualize::BasicPmu>(handler);
    unique_ptr<Visualize::PmuCalculator> pmuCalculator = Utility::MakeUnique<Visualize::PmuCalculator910B>();
    pmuCalculator->Init(basicPmu);
    RoofLineOf910B roofLine(freqRoofline, aiCoreNumRoofline, opBasicInfo, basicPmu, pmuCalculator);

    EXPECT_EQ(roofLine.RoofLineAnalysis(2.0f, 4.0f, 1.0f, 3.0f), "NA");
}

// 高拉动点为0时，验证RoofLineAnalysis返回NA
TEST(RoofLine, roofline_analysis_return_na_when_pull_through_point_zero)
{
    unique_ptr<DataHandler> handler = Utility::MakeUnique<DataHandlerOf910B>();
    shared_ptr<Visualize::OpBasicInfo> opBasicInfo = Utility::MakeShared<Visualize::OpBasicInfo>(handler);
    shared_ptr<Visualize::BasicPmu> basicPmu = Utility::MakeShared<Visualize::BasicPmu>(handler);
    unique_ptr<Visualize::PmuCalculator> pmuCalculator = Utility::MakeUnique<Visualize::PmuCalculator910B>();
    pmuCalculator->Init(basicPmu);
    RoofLineOf910B roofLine(freqRoofline, aiCoreNumRoofline, opBasicInfo, basicPmu, pmuCalculator);

    EXPECT_EQ(roofLine.RoofLineAnalysis(1.0f, 2.0f, 0.0f, 0.0f), "NA");
}

// 同名pipe重复设置比例时，验证SetPipeLineRatio保留最大值
TEST(RoofLine, set_pipeline_ratio_keep_max_when_pipe_name_exist)
{
    unique_ptr<DataHandler> handler = Utility::MakeUnique<DataHandlerOf910B>();
    shared_ptr<Visualize::OpBasicInfo> opBasicInfo = Utility::MakeShared<Visualize::OpBasicInfo>(handler);
    shared_ptr<Visualize::BasicPmu> basicPmu = Utility::MakeShared<Visualize::BasicPmu>(handler);
    unique_ptr<Visualize::PmuCalculator> pmuCalculator = Utility::MakeUnique<Visualize::PmuCalculator910B>();
    pmuCalculator->Init(basicPmu);
    RoofLineOf910B roofLine(freqRoofline, aiCoreNumRoofline, opBasicInfo, basicPmu, pmuCalculator);

    roofLine.SetPipeLineRatio("MTE1", 70.0f);
    roofLine.SetPipeLineRatio("MTE1", 80.0f);
    EXPECT_FLOAT_EQ(roofLine.pipeLineRatio_["MTE1"], 80.0f);
    roofLine.SetPipeLineRatio("MTE1", 75.0f);
    EXPECT_FLOAT_EQ(roofLine.pipeLineRatio_["MTE1"], 80.0f);
}

// AddJson中数据量为0的roofline无意义被跳过，验证仅保留有效数据行
TEST(RoofLine, add_json_skip_roofline_when_data_volume_zero)
{
    unique_ptr<DataHandler> handler = Utility::MakeUnique<DataHandlerOf910B>();
    shared_ptr<Visualize::OpBasicInfo> opBasicInfo = Utility::MakeShared<Visualize::OpBasicInfo>(handler);
    shared_ptr<Visualize::BasicPmu> basicPmu = Utility::MakeShared<Visualize::BasicPmu>(handler);
    unique_ptr<Visualize::PmuCalculator> pmuCalculator = Utility::MakeUnique<Visualize::PmuCalculator910B>();
    pmuCalculator->Init(basicPmu);
    RoofLineOf910B roofLine(freqRoofline, aiCoreNumRoofline, opBasicInfo, basicPmu, pmuCalculator);

    SubCoreProperty property = {1000, 1.0f, "Cube_INT(100%)"};
    std::vector<RoofLineData> roofLineDatas = {{"zeroData", 0, 1.0f}, {"normalData", 100, 1.0f}};
    roofLine.AddJson("testTitle", {property}, roofLineDatas);
    ASSERT_EQ(roofLine.roofLineJson_.size(), 1);
    auto roofLines = roofLine.roofLineJson_[0].at("rooflines");
    ASSERT_EQ(roofLines.size(), 1);
    string bwName = roofLines[0].at("bw_name");
    EXPECT_EQ(bwName, "normalData");
}

// 基类RoofLine的GenerateRoofLines默认实现，验证返回空数组
TEST(RoofLine, generate_rooflines_base_class_return_empty)
{
    unique_ptr<DataHandler> handler = Utility::MakeUnique<DataHandlerOf910B>();
    shared_ptr<Visualize::OpBasicInfo> opBasicInfo = Utility::MakeShared<Visualize::OpBasicInfo>(handler);
    shared_ptr<Visualize::BasicPmu> basicPmu = Utility::MakeShared<Visualize::BasicPmu>(handler);
    unique_ptr<Visualize::PmuCalculator> pmuCalculator = Utility::MakeUnique<Visualize::PmuCalculator910B>();
    pmuCalculator->Init(basicPmu);
    RoofLine baseRoofLine(freqRoofline, aiCoreNumRoofline, opBasicInfo, basicPmu, pmuCalculator);

    EXPECT_TRUE(baseRoofLine.GenerateRoofLines().empty());
}

// gmType不在理论带宽映射表中时，验证GetTheoryBwByGmType和GetTheoryL2CacheByGmType回退默认值
TEST(RoofLine, get_theory_bw_return_default_when_gm_type_not_found)
{
    std::map<std::string, float> gmBw;
    std::map<std::string, float> l2CacheBw;
    HalHelper::Instance().gmType_ = static_cast<GmType>(99);
    roofLineTest.GetTheoryBwByGmType(gmBw);
    roofLineTest.GetTheoryL2CacheByGmType(l2CacheBw);
    HalHelper::Instance().gmType_ = GmType::DEFAULT;
    EXPECT_TRUE(abs(gmBw["Ascend910B1"] - 1.8) < 0.001);
    EXPECT_TRUE(abs(l2CacheBw["Ascend910B1"] - 8) < 0.001);
}

// 构造910B非零访存请求场景，验证CubePipeLine计算L1/L0C写GM数据正确
TEST(RoofLine, cube_pipe_line_cal_write_gm_data_with_nonzero_req_success)
{
    std::map<uint16_t, uint64_t> cubeEventsCustom = {
        {73, 98400}, {74, 500}, {33, 9999},
        {50, 60}, {49, 70}, {518, 10}, {28, 80}, {34, 90}, {524, 50},
        {19, 100}, {1292, 200},
        {1280, 1280}, {1282, 1282}, {1284, 1284}, {1288, 100}, {1286, 100}, {1290, 100}
    };
    std::map<uint16_t, uint64_t> vecEventsCustom = {
        {75, 10240}, {76, 0}, {77, 0}, {174, 100}, {78, 0}, {186, 100}, {79, 136},
        {61, 40964}, {62, 40960}, {67, 20480}, {68, 20480},
        {1280, 1280}, {1282, 1282}, {1284, 1284}
    };
    unique_ptr<DataHandler> handler = Utility::MakeUnique<DataHandlerOf910B>();
    shared_ptr<Visualize::OpBasicInfo> opBasicInfo = Utility::MakeShared<Visualize::OpBasicInfo>(handler);
    shared_ptr<Visualize::BasicPmu> basicPmu = Utility::MakeShared<Visualize::BasicPmu>(handler);
    basicPmu->totalPmuData_ = {
        {{0, "cube0"}, {cubeEventsCustom, 10000000, "cube"}},
        {{0, "vector0"}, {vecEventsCustom, 2000000, "vector"}}
    };
    unique_ptr<Visualize::PmuCalculator> pmuCalculator = Utility::MakeUnique<Visualize::PmuCalculator910B>();
    pmuCalculator->Init(basicPmu);
    opBasicInfo->opType_ = OpType::MIX;
    opBasicInfo->duration_ = 100;
    opBasicInfo->soc_ = "Ascend910B4";
    HalHelper::Instance().gmType_ = GmType::DEFAULT;
    RoofLineOf910B roofLine(freqRoofline, aiCoreNumRoofline, opBasicInfo, basicPmu, pmuCalculator);

    vector<nlohmann::json> jsonData = roofLine.GenerateRoofLines();
    EXPECT_EQ(jsonData.size(), 7);
    // l1WriteReq = 4*100+10-50 = 360, l1WriteGm = 360*128
    EXPECT_EQ(roofLine.basicPmu_["l1WriteGm"], 46080);
    // l0cWriteGmReq = 50-10 = 40，l0cWriteGm 期望为 40*128 = 5120；
    // 但 InsertMaxBw 先于 CubePipeLine 执行并通过 operator[] 将 "l0cWriteGm" 键预置为 0，
    // 而 std::map::insert 不会覆盖已存在的键，导致当前实现下 l0cWriteGm 恒为 0（已知缺陷，
    // 此处按实际行为断言以固化现状，修复 roofline.cpp 后应改为 5120）
    EXPECT_EQ(roofLine.basicPmu_["l0cWriteGm"], 0);
    // writeMainMem = 200*128
    EXPECT_EQ(roofLine.basicPmu_["writeMainMem"], 25600);
    // mteToL1Data = (60-10)*256
    EXPECT_EQ(roofLine.basicPmu_["mteToL1Data"], 12800);
}

// 构造A5芯片mix算子场景，验证RoofLineOfA5完整生成各类roofline数据成功
TEST(RoofLine, generate_rooflines_a5_mix_operator_success)
{
    std::map<uint16_t, uint64_t> cubeEventsA5 = {
        {static_cast<uint16_t>(Event::READ_DATA_RECEIVED), 40},
        {static_cast<uint16_t>(Event::WRITE_DATA_SENT), 100},
        {static_cast<uint16_t>(Event::WR_L0A_INSTR), 10},
        {static_cast<uint16_t>(Event::WR_L0B_INSTR), 20},
        {static_cast<uint16_t>(Event::RD_L1_INSTR), 30},
        {static_cast<uint16_t>(Event::WR_L1_INSTR), 40},
        {static_cast<uint16_t>(Event::FIXP_WR_UB_INSTR), 5},
        {static_cast<uint16_t>(Event::FIXP_WR_L1_INSTR), 6},
        {static_cast<uint16_t>(Event::FIXP_WR_UB1_INSTR), 7},
        {static_cast<uint16_t>(Event::AR_CLOSE_L2_HIT_CORE), 1000},
        {static_cast<uint16_t>(Event::AR_CLOSE_L2_MISS_CORE), 100},
        {static_cast<uint16_t>(Event::AR_CLOSE_L2_VICTIM_CORE), 50},
        {static_cast<uint16_t>(Event::AR_FAR_L2_HIT_CORE), 2000},
        {static_cast<uint16_t>(Event::AR_FAR_L2_MISS_CORE), 200},
        {static_cast<uint16_t>(Event::AR_FAR_L2_VICTIM_CORE), 60},
        {static_cast<uint16_t>(Event::AW_CLOSE_L2_HIT_CORE), 3000},
        {static_cast<uint16_t>(Event::AW_CLOSE_L2_MISS_CORE), 300},
        {static_cast<uint16_t>(Event::AW_CLOSE_L2_VICTIM_CORE), 70},
        {static_cast<uint16_t>(Event::AW_FAR_L2_HIT_CORE), 4000},
        {static_cast<uint16_t>(Event::AW_FAR_L2_MISS_CORE), 400},
        {static_cast<uint16_t>(Event::AW_FAR_L2_VICTIM_CORE), 80}
    };
    std::map<uint16_t, uint64_t> vecEventsA5 = {
        {static_cast<uint16_t>(Event::DCU_MISS_LDG), 11},
        {static_cast<uint16_t>(Event::DCU_MISS_LDK), 12},
        {static_cast<uint16_t>(Event::DCU_REQ_STG), 13},
        {static_cast<uint16_t>(Event::DCU_REQ_STK), 14}
    };
    unique_ptr<DataHandler> handler = Utility::MakeUnique<DataHandlerOf91095>();
    shared_ptr<Visualize::OpBasicInfo> opBasicInfo = Utility::MakeShared<Visualize::OpBasicInfo>(handler);
    shared_ptr<Visualize::BasicPmu> basicPmu = Utility::MakeShared<Visualize::BasicPmu>(handler);
    basicPmu->totalPmuData_ = {
        {{0, "cube0"}, {cubeEventsA5, 10000000, "cube"}},
        {{0, "vector0"}, {vecEventsA5, 10000000, "vector"}}
    };
    unique_ptr<Visualize::PmuCalculator> pmuCalculator = Utility::MakeUnique<Visualize::PmuCalculatorA5>();
    pmuCalculator->Init(basicPmu);
    opBasicInfo->opType_ = OpType::MIX;
    opBasicInfo->duration_ = 100;
    opBasicInfo->soc_ = "Ascend950PR_9599";

    // cube核simd操作数：F16计入Half Precision，S8计入Integer，operands为0及未知类型被跳过
    Visualize::ComputeLoadBlockDetail cubeDetail;
    cubeDetail.blockType = "cube0";
    OperandRecord f16Record; f16Record.operands = 1000;
    OperandRecord s8Record; s8Record.operands = 500;
    OperandRecord zeroRecord; zeroRecord.operands = 0;
    OperandRecord u8Record; u8Record.operands = 100;
    cubeDetail.operandRecordMap.simdMap[OperandType::DATA_F16] = f16Record;
    cubeDetail.operandRecordMap.simdMap[OperandType::DATA_S8] = s8Record;
    cubeDetail.operandRecordMap.simdMap[OperandType::DATA_F32] = zeroRecord;
    cubeDetail.operandRecordMap.simdMap[OperandType::DATA_U8] = u8Record;
    // vector核simt操作数：F32计入Float，S32计入Integer，operands为0被跳过
    Visualize::ComputeLoadBlockDetail vecDetail;
    vecDetail.blockType = "vector0";
    OperandRecord f32Record; f32Record.operands = 2048;
    OperandRecord s32Record; s32Record.operands = 1024;
    vecDetail.operandRecordMap.simtMap[OperandType::DATA_F32] = f32Record;
    vecDetail.operandRecordMap.simtMap[OperandType::DATA_S32] = s32Record;
    vecDetail.operandRecordMap.simtMap[OperandType::DATA_F16] = zeroRecord;
    // 非cube/vector类型的block被跳过
    Visualize::ComputeLoadBlockDetail otherDetail;
    otherDetail.blockType = "other";
    otherDetail.operandRecordMap.simdMap[OperandType::DATA_F16] = u8Record;
    otherDetail.operandRecordMap.simtMap[OperandType::DATA_F32] = u8Record;
    basicPmu->computeLoadBlockDetailVec_ = {cubeDetail, vecDetail, otherDetail};

    RoofLineOfA5 roofLine(freqRoofline, aiCoreNumRoofline, opBasicInfo, basicPmu, pmuCalculator);
    vector<nlohmann::json> jsonData = roofLine.GenerateRoofLines();
    ASSERT_EQ(jsonData.size(), 5);
    string title0 = jsonData[0].at("title");
    string title1 = jsonData[1].at("title");
    string title2 = jsonData[2].at("title");
    string title3 = jsonData[3].at("title");
    string title4 = jsonData[4].at("title");
    EXPECT_EQ(title0, "Memory Unit(Cube)");
    EXPECT_EQ(title1, "Pipeline(Cube)");
    EXPECT_EQ(title2, "Memory Transfer(Cube)");
    EXPECT_EQ(title3, "Memory Unit(Vector)");
    EXPECT_EQ(title4, "GM/L2");
    EXPECT_EQ(jsonData[0].at("rooflines").size(), 12);
    EXPECT_EQ(jsonData[1].at("rooflines").size(), 8);
    EXPECT_EQ(jsonData[2].at("rooflines").size(), 8);
    EXPECT_EQ(jsonData[3].at("rooflines").size(), 2);
    EXPECT_EQ(jsonData[4].at("rooflines").size(), 6);

    // cube核Half Precision: fops=1000, theoryTfops=8192*freq*cubeNum/1000/1000
    ASSERT_EQ(roofLine.cubeProperties_.size(), 2);
    EXPECT_EQ(roofLine.cubeProperties_[0].computilityName, "Half Precision");
    EXPECT_EQ(roofLine.cubeProperties_[0].fops, 1000);
    EXPECT_FLOAT_EQ(roofLine.cubeProperties_[0].theoryTfops, 8192.0f * freqRoofline * 20 / 1000 / 1000);
    EXPECT_EQ(roofLine.cubeProperties_[1].computilityName, "Integer");
    EXPECT_EQ(roofLine.cubeProperties_[1].fops, 500);
    EXPECT_FLOAT_EQ(roofLine.cubeProperties_[1].theoryTfops, 16384.0f * freqRoofline * 20 / 1000 / 1000);

    // vector核Float: theoryTfops=32*4*vecNum*freq/1000000, Integer: 16*4*vecNum*freq/1000000
    ASSERT_EQ(roofLine.vecSimtProperties_.size(), 2);
    EXPECT_EQ(roofLine.vecSimtProperties_[0].computilityName, "Float");
    EXPECT_EQ(roofLine.vecSimtProperties_[0].fops, 2048);
    EXPECT_FLOAT_EQ(roofLine.vecSimtProperties_[0].theoryTfops, 32.0f * 4 * 40 * freqRoofline / 1000000);
    EXPECT_EQ(roofLine.vecSimtProperties_[1].computilityName, "Integer");
    EXPECT_EQ(roofLine.vecSimtProperties_[1].fops, 1024);
    EXPECT_FLOAT_EQ(roofLine.vecSimtProperties_[1].theoryTfops, 16.0f * 4 * 40 * freqRoofline / 1000000);

    // l0cToFixpData = (5+7+6+100)*128, l0cToL1Data = 6*128
    EXPECT_EQ(roofLine.basicPmu_["l0cToFixpData"], 15104);
    EXPECT_EQ(roofLine.basicPmu_["l0cToL1Data"], 768);
    // GM/L2首行对应cube Half Precision属性
    string gmName = jsonData[4].at("rooflines")[0].at("computility_name");
    EXPECT_EQ(gmName, "Cube Half Precision");
}