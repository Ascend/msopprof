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
#include "parse/data_calculator/instr_detail_calculator/vector_utilization_calculator.h"
#undef private
#undef protected

using namespace Profiling;
using namespace Parse;

namespace TestVecUtils {
DataCenter CreateTempDataCenter(ChipProductType chipProductType)
{
    Profiling::MergeInfo mergeInfo;
    std::vector<Profiling::MergeInfo> mergeVec;
    if (chipProductType == ChipProductType::ASCEND310P_SERIES) {
        // repeat mode, select 64 elements, detail with space
        mergeInfo.name = "movemask";
        mergeInfo.detail = "mask:0, X0:0xffffffffffffffff";
        mergeVec.emplace_back(mergeInfo);
        mergeInfo.name = "movemask";
        mergeInfo.detail = "mask:1, X1:0x0";
        mergeVec.emplace_back(mergeInfo);
        mergeInfo.name = "scalar_mov_special_xn";
        mergeInfo.detail = "SPR_CTRL= 0x8, x[3]= 0x8";
        mergeVec.emplace_back(mergeInfo);
        mergeInfo.name = "vlrelu";
        mergeInfo.detail = "op0:4, op1:9, op2:3, type/convType 3, repeat:1, dest_addr:0x0, src_addr:0x0,"
                           " dst_stride:1, src_stride:1, dst_rep_stride:8, src_rep_stride:8, reg:0, h0:0";
        mergeVec.emplace_back(mergeInfo);

    } else {
        // element select mode, select 64 elements, detail with no space
        mergeInfo.name = "MOVEMASK";
        mergeInfo.detail = "XN:X19=0x800,Pos:0, Id:547";
        mergeVec.emplace_back(mergeInfo);
        mergeInfo.name = "MOV_SPR_XN";
        mergeInfo.detail = "SPR:CTRL,XN:X1=0x100000000000000, ";
        mergeVec.emplace_back(mergeInfo);
        mergeInfo.name = "VADD";
        mergeInfo.detail = "XD:X16=0x400, XN:X13=0, XM:X14=0x200, XT:X8=0x100080808010101, Dtype:F16, Id:552";
        mergeVec.emplace_back(mergeInfo);
    }

    DataCenter dataCenter;
    std::shared_ptr<InstrDetailTable> testDb = Utility::MakeShared<InstrDetailTable>(mergeVec);
    dataCenter.DataTableRegister(testDb);
    return dataCenter;
}
}

using namespace TestVecUtils;

/**
 * |  用例集  | InstrDetailConfigUt
 * | 测试函数 | GetChipType
 * |  用例名  | test_ElementCount_should_return_correct_result_based_on_diff_mode
 * | 用例描述 | 测试ElementCount函数应根据不同的mask mode模式返回正确的元素数量计算结果
 */
TEST(VectorUtilizationCalculatorUT, test_ElementCount_should_return_correct_result_based_on_diff_mode) {
    ChipProductType testChipType = ChipProductType::ALL_PRODUCT_TYPE;
    InstrDetailConfig instrDetailContext {testChipType};
    DataCenter dataCenter = CreateTempDataCenter(testChipType);
    VectorUtilizationCalculator vectorUtilizationCalculator {dataCenter, instrDetailContext};

    uint64_t testRepeatTimes = 8;
    SpReg testSpRegStatus = {0x1a, 0xb, 0, MaskMode::REPEAT_MODE};
    ASSERT_EQ(vectorUtilizationCalculator.ElementCount(testRepeatTimes, testSpRegStatus), 48);

    testRepeatTimes = 4;
    testSpRegStatus = {0xc, 0, 0, MaskMode::REPEAT_MODE};
    ASSERT_EQ(vectorUtilizationCalculator.ElementCount(testRepeatTimes, testSpRegStatus), 8);

    testRepeatTimes = 0;
    testSpRegStatus = {0xc, 0, 0, MaskMode::ELEMENT_COUNT_MODE};
    ASSERT_EQ(vectorUtilizationCalculator.ElementCount(testRepeatTimes, testSpRegStatus), 12);

    testRepeatTimes = 0;
    testSpRegStatus = {0, 0xff, 0, MaskMode::ELEMENT_COUNT_MODE};
    ASSERT_EQ(vectorUtilizationCalculator.ElementCount(testRepeatTimes, testSpRegStatus), 255);
}

/**
 * |  用例集  | InstrDetailConfigUt
 * | 测试函数 | GetChipType
 * |  用例名  | test_CalVecUtilization_should_return_correct_result_on_A2A3
 * | 用例描述 | 测试ElementCount函数应根据不同的mask mode模式返回正确的元素数量计算结果
 */
TEST(VectorUtilizationCalculatorUT, test_CalVecUtilization_should_return_correct_result_on_A2A3) {
    ChipProductType testChipType = ChipProductType::ASCEND910_93_SERIES;
    InstrDetailConfig instrDetailContext {testChipType};
    DataCenter dataCenter = CreateTempDataCenter(testChipType);
    VectorUtilizationCalculator vectorUtilizationCalculator {dataCenter, instrDetailContext};
    vectorUtilizationCalculator.AttributeMapInit();

    VecInstrTemplate testInstrTemplate = VecInstrTemplate::VADD;
    MergeInfo testMergeInfo;
    testMergeInfo.name = "VSUBRELU";
    testMergeInfo.detail = "XD:X12=0x2ac0, XN:X14=0x980, XM:X11=0, XT:X6=0x100080808010101, Dtype:F32, Id:453";
    SpReg spStatus = {0x260, 0, 0, MaskMode::ELEMENT_COUNT_MODE};
    float res = vectorUtilizationCalculator.CalVecUtilization(testInstrTemplate, testMergeInfo, spStatus);
    int64_t golden = 3725490;
    ASSERT_EQ(static_cast<int64_t>(res * 1000000), golden);
}

/**
 * |  用例集  | VectorUtilizationCalculatorUT
 * | 测试函数 | Entry
 * |  用例名  | test_Entry_should_success_and_save_result_into_data_center_on_310P
 * | 用例描述 | 测试Entry函数在310P上的计算结果是否正确
 */
TEST(VectorUtilizationCalculatorUT, test_Entry_should_success_and_save_result_into_data_center_on_310P) {
    ChipProductType testChipType = ChipProductType::ASCEND310P_SERIES;
    InstrDetailConfig instrDetailContext {testChipType};
    DataCenter dataCenter = CreateTempDataCenter(testChipType);
    VectorUtilizationCalculator vectorUtilizationCalculator {dataCenter, instrDetailContext};
    vectorUtilizationCalculator.DependencyRegister();

    auto res = vectorUtilizationCalculator.Run();
    ASSERT_EQ(res, PluginErrorCode::SUCCESS);

    std::shared_ptr<InstrDetailTable> testDb = dataCenter.GetDbPtr<InstrDetailTable>();
    ASSERT_EQ(testDb->GetSize(), 4);
    ASSERT_EQ(static_cast<int64_t>(*testDb->QueryColumnValue<float>(
            InstrDetailTable::VEC_UTILIZATION, 3) * 1000000), 98039);
}

/**
 * |  用例集  | VectorUtilizationCalculatorUT
 * | 测试函数 | Entry
 * |  用例名  | test_Entry_should_success_and_save_result_into_data_center_on_A3
 * | 用例描述 | 测试Entry函数在310P上的计算结果是否正确
 */
TEST(VectorUtilizationCalculatorUT, test_Entry_should_success_and_save_result_into_data_center_on_A3) {
    ChipProductType testChipType = ChipProductType::ASCEND910_9391;
    InstrDetailConfig instrDetailContext {testChipType};
    DataCenter dataCenter = CreateTempDataCenter(testChipType);
    VectorUtilizationCalculator vectorUtilizationCalculator {dataCenter, instrDetailContext};
    vectorUtilizationCalculator.DependencyRegister();

    auto res = vectorUtilizationCalculator.Run();
    ASSERT_EQ(res, PluginErrorCode::SUCCESS);

    std::shared_ptr<InstrDetailTable> testDb = dataCenter.GetDbPtr<InstrDetailTable>();
    ASSERT_EQ(testDb->GetSize(), 3);
    ASSERT_EQ(static_cast<int64_t>(*testDb->QueryColumnValue<float>(InstrDetailTable::VEC_UTILIZATION, 2) * 1000000), 6274510);
}