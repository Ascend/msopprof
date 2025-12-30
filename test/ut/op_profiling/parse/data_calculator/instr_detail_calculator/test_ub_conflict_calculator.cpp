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
#include "parse/data_calculator/instr_detail_calculator/ub_conflict_calculator.h"
#undef private
#undef protected

using namespace Profiling;
using namespace Parse;

namespace TestUbConflict {
DataCenter CreateTempDataCenter(Common::ChipProductType chipProductType)
{
    MergeInfo mergeInfo;
    if (chipProductType == Common::ChipProductType::ASCEND310P_SERIES) {
        mergeInfo.name = "vadd";
        mergeInfo.detail = "op0:4, op1:2, op2:1, type:5, repeat:1, dest_addr:0x400, src_addr:0x0, "
                           "src1_addr:0x200, dst_stride:0x1, src_stride:1, src1_stride:1, "
                           "dst_rep_stride:0x8, src_rep_stride:8, src1_rep_stride:8, reg:0, h0:0";
        mergeInfo.spStatus = {0x80, 0, 0, MaskMode::ELEMENT_COUNT_MODE};
    } else {
        mergeInfo.name = "VADDS";
        mergeInfo.detail = "XD:X12=0x2ac0, XN:X12=0x2ac0, XM:X14=0, XT:X7=0x100080800010001, Dtype:F32, Id:465";
    }

    std::vector<Profiling::MergeInfo> mergeVec {mergeInfo};

    DataCenter dataCenter;
    std::shared_ptr<InstrDetailTable> testDb = Utility::MakeShared<InstrDetailTable>(mergeVec);
    dataCenter.DataTableRegister(testDb);
    return dataCenter;
}

void CompareInstrDetailEvent(const InstrDetailEvent &input, const InstrDetailEvent &golden)
{
    ASSERT_TRUE(input.isValid == golden.isValid);
    ASSERT_EQ(input.dType, golden.dType);
    ASSERT_EQ(input.srcAddrList.size(), golden.srcAddrList.size());
    ASSERT_EQ(input.dstAddrList.size(), golden.dstAddrList.size());
    for (auto i = 0; i < input.srcAddrList.size(); i++) {
        ASSERT_TRUE(input.srcAddrList[i].isValid == golden.srcAddrList[i].isValid);
        ASSERT_EQ(input.srcAddrList[i].addr, golden.srcAddrList[i].addr);
        ASSERT_EQ(input.srcAddrList[i].blockNum, golden.srcAddrList[i].blockNum);
        ASSERT_EQ(input.srcAddrList[i].blockSize, golden.srcAddrList[i].blockSize);
        ASSERT_EQ(input.srcAddrList[i].blockStride, golden.srcAddrList[i].blockStride);
        ASSERT_EQ(input.srcAddrList[i].repeatTimes, golden.srcAddrList[i].repeatTimes);
        ASSERT_EQ(input.srcAddrList[i].repeatStride, golden.srcAddrList[i].repeatStride);
    }
    for (auto i = 0; i < input.dstAddrList.size(); i++) {
        ASSERT_TRUE(input.dstAddrList[i].isValid == golden.dstAddrList[i].isValid);
        ASSERT_EQ(input.dstAddrList[i].addr, golden.dstAddrList[i].addr);
        ASSERT_EQ(input.dstAddrList[i].blockNum, golden.dstAddrList[i].blockNum);
        ASSERT_EQ(input.dstAddrList[i].blockSize, golden.dstAddrList[i].blockSize);
        ASSERT_EQ(input.dstAddrList[i].blockStride, golden.dstAddrList[i].blockStride);
        ASSERT_EQ(input.dstAddrList[i].repeatTimes, golden.dstAddrList[i].repeatTimes);
        ASSERT_EQ(input.dstAddrList[i].repeatStride, golden.dstAddrList[i].repeatStride);
    }
}
}

using namespace TestUbConflict;

/**
 * |  用例集  | UbConflictCalculatorUt
 * | 测试函数 | FindSameTimesMax
 * |  用例名  | test_FindSameTimesMax_should_return_sum_of_unique_elements_counts_beyond_1
 * | 用例描述 | 测试FindSameTimesMax函数应该返回元素值统计大于1的计数总和
 */
TEST(UbConflictCalculatorUt, test_FindSameTimesMax_should_return_sum_of_unique_elements_counts_beyond_1) {
    Common::ChipProductType testChipType = Common::ChipProductType::ALL_PRODUCT_TYPE;
    InstrDetailConfig instrDetailContext {testChipType};
    DataCenter dataCenter = CreateTempDataCenter(testChipType);
    UbConflictCalculator ubConflictCalculator {dataCenter, instrDetailContext};

    std::vector<uint64_t> idList = {0, 1, 2, 3, 4, 0, 1, 3, 4};
    ASSERT_EQ(ubConflictCalculator.FindSameTimesMax(idList), 1);

    idList = {0, 1, 2, 3, 4, 5, 6};
    ASSERT_EQ(ubConflictCalculator.FindSameTimesMax(idList), 0);

    idList = {0, 1, 2, 3, 4, 5, 6, 5, 2, 5, 1};
    ASSERT_EQ(ubConflictCalculator.FindSameTimesMax(idList), 2);
}

/**
 * |  用例集  | UbConflictCalculatorUt
 * | 测试函数 | ConflictCalculator
 * |  用例名  | test_ConflictCalculator_should_correct_conflict_count_pair
 * | 用例描述 | 测试ConflictCalculator函数应该返回正确的conflict pair数据
 */
TEST(UbConflictCalculatorUt, test_ConflictCalculator_should_correct_conflict_count_pair) {
    Common::ChipProductType testChipType = Common::ChipProductType::ALL_PRODUCT_TYPE;
    InstrDetailConfig instrDetailContext {testChipType};
    DataCenter dataCenter = CreateTempDataCenter(testChipType);
    UbConflictCalculator ubConflictCalculator {dataCenter, instrDetailContext};

    InstrDetailEvent event = {
        true, InstrDataType::F16, 1,
        {{true, 0x0, 8, 32, 1, 0, 8},
            {true, 0x200, 8, 32, 1, 0, 8}},
        {{true, 0x400, 8, 32, 1, 0, 8 }}
    };

    std::pair<float, float> res = ubConflictCalculator.ConflictCalculator(event);
    ASSERT_EQ(res.first, 1);
    ASSERT_EQ(res.second, 0);
}

/**
 * |  用例集  | UbConflictCalculatorUt
 * | 测试函数 | Get310PVecEvent
 * |  用例名  | test_Get310PVecEvent_should_get_instr_detail_event_from_merge_info_success
 * | 用例描述 | 测试Get310PVecEvent应该成功获取到310P的InstrDetailEvent并保存到InstrDetailEvent
 */
TEST(UbConflictCalculatorUt, test_Get310PVecEvent_should_get_instr_detail_event_from_merge_info_success) {
    Common::ChipProductType testChipType = Common::ChipProductType::ASCEND310P1;
    InstrDetailConfig instrDetailContext {testChipType};
    DataCenter dataCenter = CreateTempDataCenter(testChipType);
    UbConflictCalculator ubConflictCalculator {dataCenter, instrDetailContext};
    ubConflictCalculator.AttributeMapInit();

    MergeInfo mergeInfo;
    mergeInfo.name = "vadd";
    mergeInfo.detail = "op0:4, op1:2, op2:1, type:5, repeat:1, dest_addr:0x400, src_addr:0x0, "
                       "src1_addr:0x200, dst_stride:0x1, src_stride:1, src1_stride:1, "
                       "dst_rep_stride:0x8, src_rep_stride:8, src1_rep_stride:8, reg:0, h0:0";
    InstrDetailEvent event {};
    ubConflictCalculator.Get310PVecEvent(mergeInfo, event);
    InstrDetailEvent golden = {
            true, InstrDataType::F16, 1,
            {{true, 0x0, 8, 32, 1, 1, 8},
             {true, 0x200, 8, 32, 1, 1, 8}},
            {{true, 0x400, 8, 32, 1, 1, 8 }}
    };
    CompareInstrDetailEvent(event, golden);
}

/**
 * |  用例集  | UbConflictCalculatorUt
 * | 测试函数 | GetA2A3VecEvent
 * |  用例名  | test_GetA2A3VecEvent_should_get_instr_detail_event_from_merge_info_success_when_binary_op
 * | 用例描述 | 测试GetA2A3VecEvent应该成功获取到310P的InstrDetailEvent并保存到InstrDetailEvent
 */
TEST(UbConflictCalculatorUt, test_GetA2A3VecEvent_should_get_instr_detail_event_from_merge_info_success_when_binary_op) {
    Common::ChipProductType testChipType = Common::ChipProductType::ASCEND910B1;
    InstrDetailConfig instrDetailContext {testChipType};
    DataCenter dataCenter = CreateTempDataCenter(testChipType);
    UbConflictCalculator ubConflictCalculator {dataCenter, instrDetailContext};
    ubConflictCalculator.AttributeMapInit();

    MergeInfo mergeInfo;
    mergeInfo.name = "VADD";
    mergeInfo.detail = "XD:X16=0x400, XN:X13=0, XM:X14=0x200, XT:X8=0x100080808010101, Dtype:F16, Id:552";
    InstrDetailEvent event {};
    ubConflictCalculator.GetA2A3VecEvent(mergeInfo, VecInstrTemplate::VADD, event);
    InstrDetailEvent golden = {
        true, InstrDataType::F16, 1,
        {{true, 0x0, 8, 32, 1, 1, 8},
         {true, 0x200, 8, 32, 1, 1, 8}},
        {{true, 0x400, 8, 32, 1, 1, 8 }}
    };
    CompareInstrDetailEvent(event, golden);
}

/**
 * |  用例集  | UbConflictCalculatorUt
 * | 测试函数 | GetA2A3VecEvent
 * |  用例名  | test_GetA2A3VecEvent_should_get_instr_detail_event_from_merge_info_success_when_unary_op
 * | 用例描述 | 测试GetA2A3VecEvent应该成功获取到310P的InstrDetailEvent并保存到InstrDetailEvent
 */
TEST(UbConflictCalculatorUt, test_GetA2A3VecEvent_should_get_instr_detail_event_from_merge_info_success_when_unary_op) {
    Common::ChipProductType testChipType = Common::ChipProductType::ASCEND910B1;
    InstrDetailConfig instrDetailContext {testChipType};
    DataCenter dataCenter = CreateTempDataCenter(testChipType);
    UbConflictCalculator ubConflictCalculator {dataCenter, instrDetailContext};
    ubConflictCalculator.AttributeMapInit();

    MergeInfo mergeInfo;
    mergeInfo.name = "VADDS";
    mergeInfo.detail = "XD:X12=0x2ac0, XN:X12=0x2ac0, XM:X14=0, XT:X7=0x100080800010001, Dtype:F32, Id:465";
    InstrDetailEvent event {};
    ubConflictCalculator.GetA2A3VecEvent(mergeInfo, VecInstrTemplate::VADDS, event);
    InstrDetailEvent golden = {
            true, InstrDataType::F32, 1,
            {{true, 0x2ac0, 8, 32, 1, 1, 8}},
            {{true, 0x2ac0, 8, 32, 1, 1, 8 }}
    };
    CompareInstrDetailEvent(event, golden);
}

/**
 * |  用例集  | UbConflictCalculatorUt
 * | 测试函数 | Entry
 * |  用例名  | test_Entry_should_success_and_save_result_into_data_center_on_310P
 * | 用例描述 | 测试Entry函数在310P上的计算结果是否正确
 */
TEST(UbConflictCalculatorUt, test_Entry_should_success_and_save_result_into_data_center_on_310P) {
    Common::ChipProductType testChipType = Common::ChipProductType::ASCEND310P_SERIES;
    InstrDetailConfig instrDetailContext {testChipType};
    DataCenter dataCenter = CreateTempDataCenter(testChipType);
    UbConflictCalculator ubConflictCalculator {dataCenter, instrDetailContext};

    auto res = ubConflictCalculator.Run();
    ASSERT_EQ(res, PluginErrorCode::SUCCESS);

    std::shared_ptr<InstrDetailTable> testDb = dataCenter.GetDbPtr<InstrDetailTable>();
    ASSERT_EQ(testDb->GetSize(), 1);
    ASSERT_EQ(*testDb->QueryColumnValue<int>(InstrDetailTable::UB_READ_CONFLICT, 0), 1);
    ASSERT_EQ(*testDb->QueryColumnValue<int>(InstrDetailTable::UB_WRITE_CONFLICT, 0), 0);
}

/**
 * |  用例集  | UbConflictCalculatorUt
 * | 测试函数 | Entry
 * |  用例名  | test_Entry_should_success_and_save_result_into_data_center_on_A3
 * | 用例描述 | 测试Entry函数在A3上的计算结果是否正确
 */
TEST(UbConflictCalculatorUt, test_Entry_should_success_and_save_result_into_data_center_on_A3) {
    Common::ChipProductType testChipType = Common::ChipProductType::ASCEND910_9372;
    InstrDetailConfig instrDetailContext {testChipType};
    DataCenter dataCenter = CreateTempDataCenter(testChipType);
    UbConflictCalculator ubConflictCalculator {dataCenter, instrDetailContext};

    auto res = ubConflictCalculator.Run();
    ASSERT_EQ(res, PluginErrorCode::SUCCESS);

    std::shared_ptr<InstrDetailTable> testDb = dataCenter.GetDbPtr<InstrDetailTable>();
    ASSERT_EQ(testDb->GetSize(), 1);
    ASSERT_EQ(*testDb->QueryColumnValue<int>(InstrDetailTable::UB_READ_CONFLICT, 0), 0);
    ASSERT_EQ(*testDb->QueryColumnValue<int>(InstrDetailTable::UB_WRITE_CONFLICT, 0), 0);
}