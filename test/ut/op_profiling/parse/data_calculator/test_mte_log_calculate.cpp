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
#include "parse/data_calculator/mte_log_calculator.h"
#include "smart_pointer.h"
#undef private
#undef protected

using namespace Profiling::Parse;
using namespace Utility;

/**
 * |  用例集  | MteLogCalculatorTest
 * | 测试函数 | Entry
 * |  用例名  | test_entry_when_input_is_invalid_and_expect_return_error
 * | 用例描述 | 输入无效时，返回错误
 */
TEST(MteLogCalculatorTest, test_entry_when_input_is_invalid_and_expect_return_error)
{
    DataCenter dataCenter;
    MteLogCalculator mteLogCalculator(dataCenter, Common::ChipProductType::ASCEND910B4);
    ASSERT_TRUE(mteLogCalculator.Entry() == PluginErrorCode::NONBLOCKING_ERROR);
}

/**
 * |  用例集  | MteLogCalculatorTest
 * | 测试函数 | Entry
 * |  用例名  | test_entry_when_input_is_valid_and_expect_return_success
 * | 用例描述 | 输入有效时，返回成功
 */
TEST(MteLogCalculatorTest, test_entry_when_input_is_valid_and_expect_return_success)
{
    DataCenter dataCenter;
    std::shared_ptr<std::vector<MteLogInstrMap>> mteLogInstrMapVecPtr = MakeShared<std::vector<MteLogInstrMap>>();
    MteLogInstrMap mteLogInstrMap;
    mteLogInstrMap[4].instrType = MteLogInstrType::GM_TO_L1;
    mteLogInstrMap[4].maxReqTs = 12.52;
    mteLogInstrMap[4].reqTbl[111].ts = 12.52;
    mteLogInstrMap[4].reqTbl[111].dataSize = 128;
    mteLogInstrMap[5].instrType = MteLogInstrType::GM_TO_TOTAL;
    mteLogInstrMap[5].maxReqTs = -1;
    mteLogInstrMap[6].instrType = MteLogInstrType::UB_TO_GM;
    mteLogInstrMap[6].maxReqTs = 18.53;
    mteLogInstrMap[6].reqTbl[234].ts = 18.53;
    mteLogInstrMap[6].reqTbl[234].dataSize = 256;
    mteLogInstrMap[7].instrType = MteLogInstrType::GM_TO_UB;
    mteLogInstrMap[7].maxReqTs = 12.12;
    mteLogInstrMap[7].reqTbl[123].ts = 12.12;
    mteLogInstrMap[7].reqTbl[123].dataSize = 128;
    mteLogInstrMapVecPtr->emplace_back(mteLogInstrMap);
    dataCenter.DataTableRegister(mteLogInstrMapVecPtr);
    MteLogCalculator mteLogCalculator(dataCenter, Common::ChipProductType::ASCEND910B4);
    ASSERT_TRUE(mteLogCalculator.Entry() == PluginErrorCode::SUCCESS);
    std::shared_ptr<MteThroughputChart> mteThroughputChartPtr = dataCenter.GetDbPtr<MteThroughputChart>();
    ASSERT_TRUE(mteThroughputChartPtr != nullptr);
    ASSERT_EQ((*mteThroughputChartPtr)[12][0], 122.0703125);
    ASSERT_EQ((*mteThroughputChartPtr)[12][1], 122.0703125);
    ASSERT_EQ((*mteThroughputChartPtr)[12][2], 244.140625);
    ASSERT_EQ((*mteThroughputChartPtr)[12][3], 0);
    ASSERT_EQ((*mteThroughputChartPtr)[12][4], 0);
    ASSERT_EQ((*mteThroughputChartPtr)[12][5], 0);
    ASSERT_EQ((*mteThroughputChartPtr)[18][0], 0);
    ASSERT_EQ((*mteThroughputChartPtr)[18][1], 0);
    ASSERT_EQ((*mteThroughputChartPtr)[18][2], 0);
    ASSERT_EQ((*mteThroughputChartPtr)[18][3], 244.140625);
    ASSERT_EQ((*mteThroughputChartPtr)[18][4], 0);
    ASSERT_EQ((*mteThroughputChartPtr)[18][5], 244.140625);
}