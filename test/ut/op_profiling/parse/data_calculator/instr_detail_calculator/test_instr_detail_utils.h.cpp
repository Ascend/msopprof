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

#include "parse/data_calculator/instr_detail_calculator/instr_detail_utils.h"

using namespace Profiling::Parse;

/**
 * |  用例集  | InstrDetailUtilsUt
 * | 测试函数 | CeilByAlignSize
 * |  用例名  | test_CeilByAlignSize_should_return_correct_result
 * | 用例描述 | 测试CeilByAlignSize函数能够正确计算上对齐后的大小
 */
TEST(InstrDetailUtilsUt, test_CeilByAlignSize_should_return_correct_result) {
    // test with default alignSize
    ASSERT_EQ(CeilByAlignSize(16), UB_ALIGN_SIZE);
    ASSERT_EQ(CeilByAlignSize(17), UB_ALIGN_SIZE);
    ASSERT_EQ(CeilByAlignSize(32), UB_ALIGN_SIZE);
    ASSERT_EQ(CeilByAlignSize(33), UB_ALIGN_SIZE * 2);

    uint32_t testAlignSize = 12;
    // test with default alignSize
    ASSERT_EQ(CeilByAlignSize(16, testAlignSize), testAlignSize * 2);
    ASSERT_EQ(CeilByAlignSize(17, testAlignSize), testAlignSize * 2);
    ASSERT_EQ(CeilByAlignSize(11, testAlignSize), testAlignSize);
    ASSERT_EQ(CeilByAlignSize(12, testAlignSize), testAlignSize);
}

/**
 * |  用例集  | InstrDetailUtilsUt
 * | 测试函数 | CeilByAlignSize
 * |  用例名  | test_CeilByAlignSize_should_return_0_when_input_is_invalid
 * | 用例描述 | 测试CeilByAlignSize函数在输入无效时返回0
 */
TEST(InstrDetailUtilsUt, test_CeilByAlignSize_should_return_0_when_input_is_invalid) {
    ASSERT_EQ(CeilByAlignSize(16, 0), 0);
    ASSERT_EQ(CeilByAlignSize(20, 0), 0);
}

