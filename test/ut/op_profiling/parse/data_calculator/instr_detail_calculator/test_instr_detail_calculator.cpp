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
#include "parse/data_calculator/instr_detail_calculator/instr_detail_calculator.h"
#undef private
#undef protected
#include "profiling/simulator/data_parse/sim_defs.h"

using namespace Profiling;
using namespace Profiling::Parse;

namespace TestInstrDetail {
class TestInstrDetailCalculator : public InstrDetailCalculator {
public:
    explicit TestInstrDetailCalculator(DataCenter &dataCenter, InstrDetailConfig &context) :
            InstrDetailCalculator(dataCenter, context) {};

    PluginErrorCode Entry() override {
        return PluginErrorCode::SUCCESS;
    }

    void DependencyRegister() override {}
};

void IsMemOpInfoEqual(MemOpInfo test, MemOpInfo golden) {
    ASSERT_EQ(test.isValid, golden.isValid);
    ASSERT_EQ(test.srcAddr, golden.srcAddr);
    ASSERT_EQ(test.dstAddr, golden.dstAddr);
    ASSERT_EQ(test.blockNum, golden.blockNum);
    ASSERT_EQ(test.blockSize, golden.blockSize);
    ASSERT_EQ(test.blockStride, golden.blockStride);
    ASSERT_EQ(test.repeatTimes, golden.repeatTimes);
    ASSERT_EQ(test.repeatStride, golden.repeatStride);
}

void IsSpRegEqual(SpReg test, SpReg golden) {
    ASSERT_EQ(test.vectorMask0, golden.vectorMask0);
    ASSERT_EQ(test.vectorMask1, golden.vectorMask1);
    ASSERT_EQ(test.ndParaConfig, golden.ndParaConfig);
    ASSERT_TRUE(test.maskMode == golden.maskMode);
}
}

using namespace TestInstrDetail;

/**
 * |  用例集  | InstrDetailConfigUt
 * | 测试函数 | GetChipType
 * |  用例名  | test_GetChipType_should_return_chip_type_same_as_initializing
 * | 用例描述 | 测试GetChipType函数，返回值应该与初始化时的chipType相同
 */
TEST(InstrDetailConfigUt, test_GetChipType_should_return_chip_type_same_as_initializing) {
    InstrDetailConfig instrDetailContext {ChipProductType::ASCEND910B_SERIES};
    ASSERT_EQ(instrDetailContext.GetChipType(), ChipProductType::ASCEND910B_SERIES);

    InstrDetailConfig instrDetailContext2 {ChipProductType::ASCEND910B1};
    ASSERT_EQ(instrDetailContext2.GetChipType(), ChipProductType::ASCEND910B1);
}

/**
 * |  用例集  | InstrDetailConfigUt
 * | 测试函数 | GetProductSeriesType
 * |  用例名  | test_GetProductSeriesType_should_return_correct_chip_type_series
 * | 用例描述 | 测试GetProductSeriesType函数，返回值应该为初始化值对应的芯片系列
 */
TEST(InstrDetailConfigUt, test_GetProductSeriesType_should_return_correct_chip_type_series) {
    InstrDetailConfig instrDetailContext {ChipProductType::ASCEND310P_SERIES};
    ASSERT_EQ(instrDetailContext.GetProductSeriesType(), ChipProductType::ASCEND310P_SERIES);

    InstrDetailConfig instrDetailContext2 {ChipProductType::ASCEND910B1};
    ASSERT_EQ(instrDetailContext2.GetProductSeriesType(), ChipProductType::ASCEND910B_SERIES);

    InstrDetailConfig instrDetailContext3 {ChipProductType::ASCEND910B4_1};
    ASSERT_EQ(instrDetailContext3.GetProductSeriesType(), ChipProductType::ASCEND910B_SERIES);

    InstrDetailConfig instrDetailContext4 {ChipProductType::ASCEND615_SERIES};
    ASSERT_EQ(instrDetailContext4.GetProductSeriesType(), ChipProductType::ASCEND615_SERIES);
}


/**
 * |  用例集  | InstrDetailCalculatorUt
 * | 测试函数 | AttributeMapInit
 * |  用例名  | test_MapInit_should_update_2_map_attribute_according_to_chip_series_type
 * | 用例描述 | 测试MapInit函数，根据chipType初始化2个map:regDetailRegexMap_和instrDetailRegexMap_
 */
TEST(InstrDetailCalculatorUt, test_AttributeMapInit_should_update_2_map_attribute_according_to_chip_series_type) {
    InstrDetailConfig instrDetailContext {ChipProductType::ASCEND910B_SERIES};
    DataCenter dataCenter;
    TestInstrDetailCalculator testClass {dataCenter, instrDetailContext};

    testClass.AttributeMapInit();

    ASSERT_TRUE(testClass.regDetailRegexMap_.find(RegNameKey::XM_VALUE) != testClass.regDetailRegexMap_.end());
    std::string regexTestString = "XM:X0=0x1092";
    std::smatch lineMatch;
    std::regex_search(regexTestString, lineMatch, testClass.regDetailRegexMap_.at(RegNameKey::XM_VALUE));
    std::regex targetRegex = std::regex("XM:X[0-9]{1,2}=(?:0x)?([0-9a-f]+)");
    std::smatch lineMatch2;
    std::regex_search(regexTestString, lineMatch2, targetRegex);
    ASSERT_EQ(lineMatch[0].str(), lineMatch2[0].str());

    ASSERT_TRUE(testClass.instrProcessMap_.find("MOV_OUT_TO_L1_MULTI_ND2NZ") != testClass.instrProcessMap_.end());
}

/**
 * |  用例集  | InstrDetailCalculatorUt
 * | 测试函数 | GetMemOpInfo
 * |  用例名  | test_GetMemOpInfo_should_return_true_when_MemOpInfo_is_generate_success_on_A2_with_LOAD_SRC_TO_DST_2D
 * | 用例描述 | 测试GetMemOpInfo函数在A2芯片上，当输入合法的mergeInfo时，返回值为true，且MemOpInfo的isValid为true
 */
TEST(InstrDetailCalculatorUt,
     test_GetMemOpInfo_should_return_true_when_MemOpInfo_is_generate_success_on_A2_with_LOAD_SRC_TO_DST_2D) {
    InstrDetailConfig instrDetailContext {ChipProductType::ASCEND910B_SERIES};
    DataCenter dataCenter;
    TestInstrDetailCalculator testClass {dataCenter, instrDetailContext};
    testClass.AttributeMapInit();

    // e.g. [info] [00008549] (PC: 0x1269c9a8) MTE2     : (Binary: 0x711201a0) LOAD_2D  Src:OUT, Dst:L0A, XD:X0=0x70000, XN:X0=0x113f9600, XM:X0=0x100010, Dtype:B8,  id: 1
    Profiling::MergeInfo mergeInfo;
    mergeInfo.name = "LOAD_2D";
    mergeInfo.detail = "Src:OUT, Dst:L0A, XD:X0=0x70000, XN:X0=0x113f9600, XM:X0=0x100010, Dtype:B8,  id: 1";

    MemOpInfo memOpInfo = {false, 0, 0, 0, 0, 0, 0, 0};
    bool res = testClass.GetMemOpInfo(mergeInfo, memOpInfo);
    ASSERT_TRUE(res);
    MemOpInfo checkMemOpInfo = {true, 289379840, 458752, 1, 512, 0, 16, 0};
    IsMemOpInfoEqual(memOpInfo, checkMemOpInfo);
}

/**
 * |  用例集  | InstrDetailCalculatorUt
 * | 测试函数 | GetMemOpInfo
 * |  用例名  | test_GetMemOpInfo_should_return_false_when_MemOpInfo_is_generate_success_on_A3_with_LOAD_OUT_TO_L1_IMAGE
 * | 用例描述 | 测试GetMemOpInfo函数在A2芯片上，当输入合法的mergeInfo时，返回值为true，且MemOpInfo的isValid为true
 */
TEST(InstrDetailCalculatorUt,
     test_GetMemOpInfo_should_return_false_when_MemOpInfo_is_generate_success_on_A3_with_LOAD_OUT_TO_L1_IMAGE) {
    InstrDetailConfig instrDetailContext {ChipProductType::ASCEND910_9392};
    DataCenter dataCenter;
    TestInstrDetailCalculator testClass {dataCenter, instrDetailContext};
    testClass.AttributeMapInit();

    Profiling::MergeInfo mergeInfo;
    mergeInfo.name = "LOAD_IMAGE";
    mergeInfo.detail = "Src:OUT, Dst:L1, XD:X0=0x70000, XT:X0=0x100010, Dtype:F16,  id: 2";

    MemOpInfo memOpInfo = {false, 0, 0, 0, 0, 0, 0, 0};
    bool res = testClass.GetMemOpInfo(mergeInfo, memOpInfo);
    ASSERT_FALSE(res);
}

/**
 * |  用例集  | InstrDetailCalculatorUt
 * | 测试函数 | GetMemOpInfo
 * |  用例名  | test_GetMemOpInfo_should_return_true_when_MemOpInfo_is_generate_success_on_A3_with_MOV_SRC_TO_DST_ALIGN
 * | 用例描述 | 测试GetMemOpInfo函数在A2芯片上，当输入合法的mergeInfo时，返回值为true，且MemOpInfo的isValid为true
 */
TEST(InstrDetailCalculatorUt,
     test_GetMemOpInfo_should_return_true_when_MemOpInfo_is_generate_success_on_A3_with_MOV_SRC_TO_DST_ALIGN) {
    InstrDetailConfig instrDetailContext {ChipProductType::ASCEND910B4};
    DataCenter dataCenter;
    TestInstrDetailCalculator testClass {dataCenter, instrDetailContext};
    testClass.AttributeMapInit();

    Profiling::MergeInfo mergeInfo;
    mergeInfo.name = "MOV_SRC_TO_DST_ALIGN";
    mergeInfo.detail = "Src:OUT, Dst:UB, XD:X0=0x70000, XN:X0=0x113f9600, XM:X0=0x100010, XT:X0=0, Dtype:B16,  id: 1";

    MemOpInfo memOpInfo = {false, 0, 0, 0, 0, 0, 0, 0};
    bool res = testClass.GetMemOpInfo(mergeInfo, memOpInfo);
    ASSERT_TRUE(res);
    MemOpInfo checkMemOpInfo = {true, 289379840, 458752, 16, 1, 1, 1, 16};
    IsMemOpInfoEqual(memOpInfo, checkMemOpInfo);
}


/**
 * |  用例集  | InstrDetailCalculatorUt
 * | 测试函数 | GetMemOpInfo
 * |  用例名  | test_GetMemOpInfo_should_return_true_when_MemOpInfo_is_generate_success_on_A2_with_MOV_OUT_TO_L1_MULTI_ND2NZ
 * | 用例描述 | 测试GetMemOpInfo函数在A2芯片上，当输入合法的mergeInfo时，返回值为true，且MemOpInfo的isValid为true
 */
TEST(InstrDetailCalculatorUt,
     test_GetMemOpInfo_should_return_true_when_MemOpInfo_is_generate_success_on_A2_with_MOV_OUT_TO_L1_MULTI_ND2NZ) {
    InstrDetailConfig instrDetailContext {ChipProductType::ASCEND910B4};
    DataCenter dataCenter;
    TestInstrDetailCalculator testClass {dataCenter, instrDetailContext};
    testClass.AttributeMapInit();

    Profiling::MergeInfo mergeInfo;
    mergeInfo.name = "MOV_OUT_TO_L1_MULTI_ND2NZ";
    mergeInfo.detail = "Src:OUT, Dst:L1, XD:X0=0x70000, XN:X0=0x113f9600, XM:X0=0x100010, XT:X0=0, Dtype:B16,  id: 1";

    MemOpInfo memOpInfo = {false, 0, 0, 0, 0, 0, 0, 0};
    bool res = testClass.GetMemOpInfo(mergeInfo, memOpInfo);
    ASSERT_TRUE(res);
    MemOpInfo checkMemOpInfo = {true, 289379840, 458752, 0, 2, 1, 16, 0};
    IsMemOpInfoEqual(memOpInfo, checkMemOpInfo);
}

/**
 * |  用例集  | InstrDetailCalculatorUt
 * | 测试函数 | GetMemOpInfo
 * |  用例名  | test_GetMemOpInfo_should_return_true_when_MemOpInfo_is_generate_success_on_310P_with_MOV_SRC_TO_DST
 * | 用例描述 | 测试GetMemOpInfo函数在A2芯片上，当输入合法的mergeInfo时，返回值为true，且MemOpInfo的isValid为true
 */
TEST(InstrDetailCalculatorUt,
     test_GetMemOpInfo_should_return_true_when_MemOpInfo_is_generate_success_on_310P_with_MOV_SRC_TO_DST) {
    InstrDetailConfig instrDetailContext {ChipProductType::ASCEND310P1};
    DataCenter dataCenter;
    TestInstrDetailCalculator testClass {dataCenter, instrDetailContext};
    testClass.AttributeMapInit();

    // e.g. [info] [00024514](PC: 0x11508478) MTE2 : (Binary: 0x74441000) mov_out_to_ub(xd:2, xn:1, xm:0, xdValue:0x23000, xnValue:0x114f1000, xmValue:0x10010, srcIDValue:2, destIDValue:1, reluValue:0, padValue:0), instr ID is: 17
    Profiling::MergeInfo mergeInfo;
    mergeInfo.name = "mov_out_to_ub";
    mergeInfo.detail = "xd:2, xn:1, xm:0, xdValue:0x23000, xnValue:0x114f1000, xmValue:0x10010, srcIDValue:2, destIDValue:1, reluValue:0, padValue:0";

    MemOpInfo memOpInfo = {false, 0, 0, 0, 0, 0, 0, 0};
    bool res = testClass.GetMemOpInfo(mergeInfo, memOpInfo);
    ASSERT_TRUE(res);
    MemOpInfo checkMemOpInfo = {true, 290394112, 143360, 1, 32, 1, 1, 1};
    IsMemOpInfoEqual(memOpInfo, checkMemOpInfo);
}

/**
 * |  用例集  | InstrDetailCalculatorUt
 * | 测试函数 | GetMemOpInfo
 * |  用例名  | test_GetMemOpInfo_should_return_true_when_MemOpInfo_is_generate_success_on_310P_with_LOAD_SRC_TO_SMASK
 * | 用例描述 | 测试GetMemOpInfo函数在A2芯片上，当输入合法的mergeInfo时，返回值为true，且MemOpInfo的isValid为true
 */
TEST(InstrDetailCalculatorUt,
     test_GetMemOpInfo_should_return_true_when_MemOpInfo_is_generate_success_on_310P_with_LOAD_SRC_TO_SMASK) {
    InstrDetailConfig instrDetailContext {ChipProductType::ASCEND310P2};
    DataCenter dataCenter;
    TestInstrDetailCalculator testClass {dataCenter, instrDetailContext};
    testClass.AttributeMapInit();

    Profiling::MergeInfo mergeInfo;
    mergeInfo.name = "load_src_to_smask";
    mergeInfo.detail = "xd:2, xn:1, xt:0, xdValue:0x23000, xnValue:0x114f1000, xtValue:0x10010, srcIDValue:2";

    MemOpInfo memOpInfo = {false, 0, 0, 0, 0, 0, 0, 0};
    bool res = testClass.GetMemOpInfo(mergeInfo, memOpInfo);
    ASSERT_TRUE(res);
    MemOpInfo checkMemOpInfo = {true, 290394112, 143360, 1, 32, 0, 1, 0};
    IsMemOpInfoEqual(memOpInfo, checkMemOpInfo);
}

/**
 * |  用例集  | InstrDetailCalculatorUt
 * | 测试函数 | UpdateSpReg
 * |  用例名  | test_UpdateSpReg_should_update_vectorMask1_when_encounter_movemask_on_A2
 * | 用例描述 | 测试UpdateSpReg函数在A2上遇到movemask指令时能够正确更新vectorMask1
 */
TEST(InstrDetailCalculatorUt, test_UpdateSpReg_should_update_vectorMask1_when_encounter_movemask_on_A2)
{
    InstrDetailConfig instrDetailContext {ChipProductType::ASCEND910B3};
    DataCenter dataCenter;
    TestInstrDetailCalculator testClass {dataCenter, instrDetailContext};
    testClass.AttributeMapInit();

    ASSERT_TRUE(testClass.spEnable_);

    SpReg spRegStatus {0, 0, 0, MaskMode::ELEMENT_COUNT_MODE};
    std::string testInstrName = "MOVEMASK";
    std::string detail = "XN:X0=0xffffffffffffffff, Pos:1, Id:545";
    testClass.UpdateSpReg(testInstrName, detail, spRegStatus);
    SpReg goldenStatus {0, 0xffffffffffffffff, 0, MaskMode::ELEMENT_COUNT_MODE};
    IsSpRegEqual(spRegStatus, goldenStatus);
}

/**
 * |  用例集  | InstrDetailCalculatorUt
 * | 测试函数 | UpdateSpReg
 * |  用例名  | test_UpdateSpReg_should_update_vectorMask0_when_encounter_movemask_on_310P2
 * | 用例描述 | 测试UpdateSpReg函数在310P2上遇到movemask指令时能够正确更新vectorMask0
 */
TEST(InstrDetailCalculatorUt, test_UpdateSpReg_should_update_vectorMask0_when_encounter_movemask_on_310P2)
{
    InstrDetailConfig instrDetailContext {ChipProductType::ASCEND310P2};
    DataCenter dataCenter;
    TestInstrDetailCalculator testClass {dataCenter, instrDetailContext};
    testClass.AttributeMapInit();

    ASSERT_TRUE(testClass.spEnable_);

    SpReg spRegStatus {0, 0, 0, MaskMode::REPEAT_MODE};
    std::string testInstrName = "movemask";
    std::string detail = "num_block:0, mask:0, X1:0x1af";
    testClass.UpdateSpReg(testInstrName, detail, spRegStatus);
    SpReg goldenStatus {0x1af, 0, 0, MaskMode::REPEAT_MODE};
    IsSpRegEqual(spRegStatus, goldenStatus);
}

/**
 * |  用例集  | InstrDetailCalculatorUt
 * | 测试函数 | UpdateSpReg
 * |  用例名  | test_UpdateSpReg_should_update_ndParaConfig_when_encounter_ndpara_register_on_A3
 * | 用例描述 | 测试UpdateSpReg函数在A3上遇到ndpara指令时能够正确更新ndParaConfig
 */
TEST(InstrDetailCalculatorUt, test_UpdateSpReg_should_update_ndParaConfig_when_encounter_ndpara_register_on_A3)
{
    InstrDetailConfig instrDetailContext {ChipProductType::ASCEND910_9382};
    DataCenter dataCenter;
    TestInstrDetailCalculator testClass {dataCenter, instrDetailContext};
    testClass.AttributeMapInit();

    ASSERT_TRUE(testClass.spEnable_);

    SpReg spRegStatus {0, 0, 0, MaskMode::REPEAT_MODE};
    std::string testInstrName = "MOV_SPR_XN";
    std::string detail = "SPR:ND_PARA, XN:X5=0xf,  id: 36";
    testClass.UpdateSpReg(testInstrName, detail, spRegStatus);
    SpReg goldenStatus {0, 0, 0xf, MaskMode::REPEAT_MODE};
    IsSpRegEqual(spRegStatus, goldenStatus);
}

/**
 * |  用例集  | InstrDetailCalculatorUt
 * | 测试函数 | UpdateSpReg
 * |  用例名  | test_UpdateSpReg_should_update_maskMode_when_encounter_ctrl_register_on_310P1
 * | 用例描述 | 测试UpdateSpReg函数在310P1上遇到ctrl寄存器时能够正确更新maskMode
 */
TEST(InstrDetailCalculatorUt, test_UpdateSpReg_should_update_maskMode_when_encounter_ctrl_register_on_310P1)
{
    InstrDetailConfig instrDetailContext {ChipProductType::ASCEND310P1};
    DataCenter dataCenter;
    TestInstrDetailCalculator testClass {dataCenter, instrDetailContext};
    testClass.AttributeMapInit();

    ASSERT_TRUE(testClass.spEnable_);

    SpReg spRegStatus {0, 0, 0, MaskMode::ELEMENT_COUNT_MODE};
    std::string testInstrName = "scalar_mov_xd_special";
    std::string detail = "x[16]= 0x2000000000000008, SPR_CTRL= 0x2000000000000008,";
    testClass.UpdateSpReg(testInstrName, detail, spRegStatus);
    SpReg goldenStatus {0, 0, 0, MaskMode::REPEAT_MODE};
    IsSpRegEqual(spRegStatus, goldenStatus);
}


/**
 * |  用例集  | InstrDetailCalculatorUt
 * | 测试函数 | UpdateSpReg
 * |  用例名  | test_UpdateSpReg_should_update_maskMode_when_encounter_ctrl_register_on_A2
 * | 用例描述 | 测试UpdateSpReg函数在A2上遇到ctrl寄存器时能够正确更新maskMode
 */
TEST(InstrDetailCalculatorUt, test_UpdateSpReg_should_update_maskMode_when_encounter_ctrl_register_on_A2)
{
    InstrDetailConfig instrDetailContext {ChipProductType::ASCEND910B4};
    DataCenter dataCenter;
    TestInstrDetailCalculator testClass {dataCenter, instrDetailContext};
    testClass.AttributeMapInit();

    ASSERT_TRUE(testClass.spEnable_);

    SpReg spRegStatus {0, 0, 0, MaskMode::REPEAT_MODE};
    std::string testInstrName = "MOV_SPR_XN";
    std::string detail = "SPR:CTRL, XN:X13=0x100000000000000,";
    testClass.UpdateSpReg(testInstrName, detail, spRegStatus);
    SpReg goldenStatus {0, 0, 0, MaskMode::ELEMENT_COUNT_MODE};
    IsSpRegEqual(spRegStatus, goldenStatus);
}
