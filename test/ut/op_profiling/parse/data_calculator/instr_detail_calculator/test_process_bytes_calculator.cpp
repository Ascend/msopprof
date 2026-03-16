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
#include "parse/data_calculator/instr_detail_calculator/process_bytes_calculator.h"
#include "parse/data_table/instr_detail_table.h"
#undef private
#undef protected

using namespace Profiling::Parse;

namespace TestProcessByte {
DataCenter CreateTempDataCenter(ChipProductType chipProductType)
{
    Profiling::MergeInfo mergeInfo;
    if (chipProductType == ChipProductType::ASCEND310P_SERIES) {
        mergeInfo.name = "mov_out_to_ub";
        mergeInfo.detail = "xd:2, xn:1, xm:0, xdValue:0x23000, xnValue:0x114f1000, xmValue:0x10010, srcIDValue:2, destIDValue:1, reluValue:0, padValue:0";
    } else {
        mergeInfo.name = "MOV_OUT_TO_L1";
        mergeInfo.detail = "Src:OUT, Dst:L1, XD:X9=0x70000, XN:X0=0x113f9600, XM:X3=0x100010,  id: 34";
    }

    std::vector<Profiling::MergeInfo> mergeVec {mergeInfo};

    DataCenter dataCenter;
    std::shared_ptr<InstrDetailTable> testDb = Utility::MakeShared<InstrDetailTable>(mergeVec);
    dataCenter.DataTableRegister(testDb);
    return dataCenter;
}
}

using namespace TestProcessByte;

/**
 * |  用例集  | ProcessBytesCalculatorUt
 * | 测试函数 | DependencyRegister
 * |  用例名  | test_DependencyRegister_should_success_register_plugin_info
 * | 用例描述 | 测试DependencyRegister函数应该正确注册插件信息
 */
TEST(ProcessBytesCalculatorUt, test_DependencyRegister_should_success_register_plugin_info) {
    InstrDetailConfig instrDetailContext {ChipProductType::ASCEND910B_SERIES};
    DataCenter dataCenter;
    ProcessBytesCalculator processBytesCalculator {dataCenter, instrDetailContext};

    processBytesCalculator.DependencyRegister();
    ASSERT_EQ(processBytesCalculator.pluginInfo_.pluginName, "ProcessBytesCalculator");
    std::vector<std::type_index> checkMandatoryDb = {typeid(InstrDetailTable)};
    ASSERT_EQ(processBytesCalculator.pluginInfo_.mandatoryDb, checkMandatoryDb);
    std::set<ChipProductType> checkChipSupport = {ChipProductType::ASCEND310P_SERIES,
                                                          ChipProductType::ASCEND910B_SERIES,
                                                          ChipProductType::ASCEND910_93_SERIES};
    ASSERT_EQ(processBytesCalculator.pluginInfo_.chipSupport, checkChipSupport);
}

/**
 * |  用例集  | ProcessBytesCalculatorUt
 * | 测试函数 | Entry
 * |  用例名  | test_Entry_should_success_and_save_result_into_data_center_on_A2
 * | 用例描述 | 测试Entry函数在A2上的计算结果是否正确
 */
TEST(ProcessBytesCalculatorUt, test_Entry_should_success_and_save_result_into_data_center_on_A2) {
    ChipProductType testChipType = ChipProductType::ASCEND910B_SERIES;
    InstrDetailConfig instrDetailContext {testChipType};
    DataCenter dataCenter = CreateTempDataCenter(testChipType);
    ProcessBytesCalculator processBytesCalculator {dataCenter, instrDetailContext};
    processBytesCalculator.DependencyRegister();

    auto res = processBytesCalculator.Run();
    ASSERT_EQ(res, PluginErrorCode::SUCCESS);

    std::shared_ptr<InstrDetailTable> testDb = dataCenter.GetDbPtr<InstrDetailTable>();
    uint64_t processByte = 16 * 32 * 1;
    ASSERT_EQ(testDb->GetSize(), 1);
    int* actualProcessByte = testDb->QueryColumnValue<int>(InstrDetailTable::PROCESS_BYTES, 0);
    ASSERT_NE(actualProcessByte, nullptr);
    ASSERT_EQ(*actualProcessByte, processByte);
}

/**
 * |  用例集  | ProcessBytesCalculatorUt
 * | 测试函数 | Entry
 * |  用例名  | test_Entry_should_success_and_save_result_into_data_center_on_310P
 * | 用例描述 | 测试Entry函数在310P上的计算结果是否正确
 */
TEST(ProcessBytesCalculatorUt, test_Entry_should_success_and_save_result_into_data_center_on_310P) {
    ChipProductType testChipType = ChipProductType::ASCEND310P_SERIES;
    InstrDetailConfig instrDetailContext {testChipType};
    DataCenter dataCenter = CreateTempDataCenter(testChipType);
    ProcessBytesCalculator processBytesCalculator {dataCenter, instrDetailContext};
    processBytesCalculator.DependencyRegister();

    auto res = processBytesCalculator.Run();
    ASSERT_EQ(res, PluginErrorCode::SUCCESS);

    std::shared_ptr<InstrDetailTable> testDb = dataCenter.GetDbPtr<InstrDetailTable>();
    uint64_t processByte = 1 * 32 * 1;
    ASSERT_EQ(testDb->GetSize(), 1);
    int* actualProcessByte = testDb->QueryColumnValue<int>(InstrDetailTable::PROCESS_BYTES, 0);
    ASSERT_NE(actualProcessByte, nullptr);
    ASSERT_EQ(*actualProcessByte, processByte);
}