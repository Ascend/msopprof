/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
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
 * -------------------------------------------------------------------------
 */

#include <gtest/gtest.h>
#include "mockcpp/mockcpp.hpp"

#define private public
#define protected public
#include "parse/data_visualizer/mte_log_visualizer/mte_log_visualizer.h"
#include "parse/data_table/mte_throughput_table.h"
#include "smart_pointer.h"
#undef private
#undef protected
#include "filesystem.h"
#include "common/defs.h"

using namespace Profiling::Parse;
using namespace Utility;

/**
 * |  用例集  | MteLogVisualizerTest
 * | 测试函数 | Entry
 * |  用例名  | test_entry_when_input_is_invalid_and_expect_return_error
 * | 用例描述 | 输入无效时，返回错误
 */
TEST(MteLogVisualizerTest, test_entry_when_input_is_invalid_and_expect_return_error)
{
    DataCenter dataCenter;
    MteLogVisualizer mteLogCalculator(dataCenter, ChipProductType::ASCEND910B4);
    mteLogCalculator.DependencyRegister();
    ASSERT_TRUE(mteLogCalculator.Entry() == PluginErrorCode::NONBLOCKING_ERROR);
}

TEST(MteLogVisualizerFill, test_entry_fill_data)
{
    DataCenter dataCenter;
    MteLogVisualizer mteLogCalculator(dataCenter, ChipProductType::ASCEND910B4);
    std::vector<double> valueList;
    valueList.resize(static_cast<size_t>(MteLogInstrType::END));
    size_t type = 0;
    for (type = 0; type < static_cast<size_t>(MteLogInstrType::END); ++type) {
        valueList[type] = 0.1;
    }
    std::vector<nlohmann::json> mteThroughputJson;
    size_t ts = 1;
    mteLogCalculator.FillData(mteThroughputJson, valueList, ts);
    ASSERT_EQ(mteThroughputJson.size(), static_cast<size_t>(MteLogInstrType::END));
}

/**
 * |  用例集  | MteLogVisualizerTest
 * | 测试函数 | Entry
 * |  用例名  | test_entry_when_input_is_valid_and_expect_success
 * | 用例描述 | 输入有效时，返回数据正确
 */
TEST(MteLogVisualizerTest, test_entry_when_input_is_valid_and_expect_success)
{
    DataCenter dataCenter;
    MteLogVisualizer mteLogVisualizer(dataCenter, ChipProductType::ASCEND910B4);
    mteLogVisualizer.DependencyRegister();

    MteThroughputChart mteChart = {
        {1, 1, 1, 1, 1, 1}, {4, 4, 4, 4, 4, 4}, {9, 9, 9, 9, 9, 9}
    };
    std::shared_ptr<MteThroughputChart> mteChartPtr = MakeShared<MteThroughputChart>(mteChart);
    dataCenter.DataTableRegister(mteChartPtr);
    ASSERT_TRUE(mteLogVisualizer.Entry() == PluginErrorCode::SUCCESS);
    auto mteThroughputJsonPtr = dataCenter.GetDbPtr<std::vector<nlohmann::json>>();
    ASSERT_TRUE(mteThroughputJsonPtr != nullptr);
    ASSERT_EQ(mteThroughputJsonPtr->size(), 24);
}
