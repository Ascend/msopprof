/* -------------------------------------------------------------------------
 *  This file is part of the MindStudio project.
 *  Copyright (c) 2026 Huawei Technologies Co.,Ltd.
 *
 *  MindStudio is licensed under Mulan PSL v2.
 *  You can use this software according to the terms and conditions of the Mulan PSL v2.
 *  You may obtain a copy of Mulan PSL v2 at:
 *
 *           http://license.coscl.org.cn/MulanPSL2
 *
 *  THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 *  EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 *  MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *  See the Mulan PSL v2 for more details.
 *  ------------------------------------------------------------------------- */

#include <gtest/gtest.h>
#include "mockcpp/mockcpp.hpp"

#define private public
#define protected public
#include "parse/data_calculator/instr_detail_calculator/scalar_calculator.h"
#include "parse/data_table/instr_detail_table.h"
#include "parse/data_table/cache_detail_table.h"
#undef private
#undef protected

using namespace Profiling;
using namespace Profiling::Parse;
using namespace Utility;

class TestScalarCalculator : public ScalarCalculator {
public:
    TestScalarCalculator(DataCenter &dataCenter, InstrDetailConfig &config) : ScalarCalculator(dataCenter, config) {}

    void TestMergeScalar(const scalarHeadCache &icacheDetailTable, scalarHead &scalarDetailTable) {
        MergeScalar(icacheDetailTable, scalarDetailTable);
    }
};

scalarHeadCache CreateTestIcacheCache() {
    scalarHeadCache cache;
    cache[0x1000] = {100, 200, 300};
    cache[0x2000] = {50, 150, 250, 350};
    cache[0x3000] = {10};
    return cache;
}

scalarHead CreateTestScalarHead() {
    scalarHead scalar;
    scalar[0x1000] = {ScalarInstrInfo(UINT64_MAX, 150, 0x1000), ScalarInstrInfo(UINT64_MAX, 250, 0x1000),
        ScalarInstrInfo(UINT64_MAX, 350, 0x1000)};
    scalar[0x2000] = {ScalarInstrInfo(UINT64_MAX, 100, 0x2000), ScalarInstrInfo(UINT64_MAX, 200, 0x2000),
        ScalarInstrInfo(UINT64_MAX, 400, 0x2000)};
    scalar[0x4000] = {ScalarInstrInfo(UINT64_MAX, 100, 0x4000)};
    return scalar;
}

std::vector<MergeInfo> CreateTestMergeInfo() {
    std::vector<MergeInfo> mergeInfo;
    MergeInfo info1;
    info1.pc = 0x1000;
    info1.startTick = 140;
    info1.endTick = 160;
    mergeInfo.push_back(info1);

    MergeInfo info2;
    info2.pc = 0x1000;
    info2.startTick = 240;
    info2.endTick = 260;
    mergeInfo.push_back(info2);

    MergeInfo info3;
    info3.pc = 0x2000;
    info3.startTick = 90;
    info3.endTick = 110;
    mergeInfo.push_back(info3);

    MergeInfo info4;
    info4.pc = 0x3000;
    info4.startTick = 100;
    info4.endTick = 120;
    mergeInfo.push_back(info4);

    return mergeInfo;
}

/**
 * |  用例集 | ScalarCalculatorTest
 * | 测试函数 | MergeScalar
 * |  用例名  | test_merge_scalar_when_icache_before_ccu_and_expect_match_success
 * | 用例描述 | 测试MergeScalar算法，当ICache tick小于CCU tick时，应正确匹配最近的ICache tick
 */
TEST(ScalarCalculatorTest, test_merge_scalar_when_icache_before_ccu_and_expect_match_success) {
    scalarHeadCache icacheCache = CreateTestIcacheCache();
    scalarHead scalarDetail = CreateTestScalarHead();

    DataCenter dataCenter;
    InstrDetailConfig config(ChipProductType::ASCEND910B1);
    TestScalarCalculator calculator(dataCenter, config);

    calculator.TestMergeScalar(icacheCache, scalarDetail);

    ASSERT_TRUE(scalarDetail[0x1000][0].icacheTick == 100);
    ASSERT_TRUE(scalarDetail[0x1000][1].icacheTick == 200);
    ASSERT_TRUE(scalarDetail[0x1000][2].icacheTick == 300);

    ASSERT_TRUE(scalarDetail[0x2000][0].icacheTick == 50);
    ASSERT_TRUE(scalarDetail[0x2000][1].icacheTick == 150);
    ASSERT_TRUE(scalarDetail[0x2000][2].icacheTick == 350);

    ASSERT_TRUE(scalarDetail[0x4000][0].icacheTick == UINT64_MAX);
}

/**
 * |  用例集 | ScalarCalculatorTest
 * | 测试函数 | MergeScalar
 * |  用例名  | test_merge_scalar_when_ccu_tick_smaller_than_all_icache_and_expect_no_match
 * | 用例描述 | 测试MergeScalar算法边界情况，当CCU tick小于所有ICache tick时，第一条指令不匹配，第二条指令跳出也不匹配
 */
TEST(ScalarCalculatorTest, test_merge_scalar_when_ccu_tick_smaller_than_all_icache_and_expect_no_match) {
    scalarHeadCache icacheCache;
    icacheCache[0x1000] = {200, 300, 400};

    scalarHead scalarDetail;
    scalarDetail[0x1000] = {ScalarInstrInfo(UINT64_MAX, 100, 0x1000), ScalarInstrInfo(UINT64_MAX, 250, 0x1000)};

    DataCenter dataCenter;
    InstrDetailConfig config(ChipProductType::ASCEND910B1);
    TestScalarCalculator calculator(dataCenter, config);

    calculator.TestMergeScalar(icacheCache, scalarDetail);

    ASSERT_TRUE(scalarDetail[0x1000][0].icacheTick == UINT64_MAX);
    ASSERT_TRUE(scalarDetail[0x1000][1].icacheTick == UINT64_MAX);
}

/**
 * |  用例集 | ScalarCalculatorTest
 * | 测试函数 | MergeScalar
 * |  用例名  | test_merge_scalar_when_icache_not_exist_and_expect_skip
 * | 用例描述 | 测试MergeScalar算法边界情况，当PC在ICache中不存在时，应跳过该PC的匹配，保持icacheTick为UINT64_MAX
 */
TEST(ScalarCalculatorTest, test_merge_scalar_when_icache_not_exist_and_expect_skip) {
    scalarHeadCache icacheCache;
    icacheCache[0x1000] = {100, 200};

    scalarHead scalarDetail;
    scalarDetail[0x2000] = {ScalarInstrInfo(UINT64_MAX, 150, 0x2000)};

    DataCenter dataCenter;
    InstrDetailConfig config(ChipProductType::ASCEND910B1);
    TestScalarCalculator calculator(dataCenter, config);

    calculator.TestMergeScalar(icacheCache, scalarDetail);

    ASSERT_TRUE(scalarDetail[0x2000][0].icacheTick == UINT64_MAX);
}

/**
 * |  用例集 | ScalarCalculatorTest
 * | 测试函数 | MergeScalar
 * |  用例名  | test_merge_scalar_when_empty_tables_and_expect_no_change
 * | 用例描述 | 测试MergeScalar算法边界情况，当ICache和Scalar数据表为空时，应不发生任何变化
 */
TEST(ScalarCalculatorTest, test_merge_scalar_when_empty_tables_and_expect_no_change) {
    scalarHeadCache emptyIcache;
    scalarHead emptyScalar;

    DataCenter dataCenter;
    InstrDetailConfig config(ChipProductType::ASCEND910B1);
    TestScalarCalculator calculator(dataCenter, config);

    calculator.TestMergeScalar(emptyIcache, emptyScalar);

    ASSERT_TRUE(emptyScalar.empty());
}

/**
 * |  用例集 | ScalarCalculatorTest
 * | 测试函数 | Entry
 * |  用例名  | test_entry_when_tables_not_registered_and_expect_nonblocking_error
 * | 用例描述 | 测试Entry函数错误处理，当数据表未注册时，应返回NONBLOCKING_ERROR
 */
TEST(ScalarCalculatorTest, test_entry_when_tables_not_registered_and_expect_nonblocking_error) {
    DataCenter dataCenter;
    InstrDetailConfig config(ChipProductType::ASCEND910B1);
    TestScalarCalculator calculator(dataCenter, config);

    PluginErrorCode result = calculator.Entry();
    ASSERT_TRUE(result == PluginErrorCode::NONBLOCKING_ERROR);
}

/**
 * |  用例集 | ScalarCalculatorTest
 * | 测试函数 | Entry
 * |  用例名  | test_entry_when_valid_data_and_expect_success
 * | 用例描述 | 测试Entry函数完整流程，当数据表正确注册时，应成功执行并填充ICACHE_CYC和CCU_CYC列
 */
TEST(ScalarCalculatorTest, test_entry_when_valid_data_and_expect_success) {
    DataCenter dataCenter;

    auto icacheCache = Utility::MakeShared<scalarHeadCache>(CreateTestIcacheCache());
    dataCenter.DataTableRegister(icacheCache);

    auto scalarDetail = Utility::MakeShared<scalarHead>(CreateTestScalarHead());
    dataCenter.DataTableRegister(scalarDetail);

    std::vector<MergeInfo> mergeInfo = CreateTestMergeInfo();
    auto instrTable = Utility::MakeShared<InstrDetailTable>(mergeInfo);
    dataCenter.DataTableRegister(instrTable);

    InstrDetailConfig config(ChipProductType::ASCEND910B1);
    TestScalarCalculator calculator(dataCenter, config);

    PluginErrorCode result = calculator.Entry();
    ASSERT_TRUE(result == PluginErrorCode::SUCCESS);

    auto icacheTicks = instrTable->GetColumnData<uint64_t>(InstrDetailTable::ICACHE_CYC);
    auto ccuTicks = instrTable->GetColumnData<uint64_t>(InstrDetailTable::CCU_CYC);

    ASSERT_TRUE(icacheTicks != nullptr);
    ASSERT_TRUE(ccuTicks != nullptr);
    ASSERT_TRUE(icacheTicks->size() == 4);
    ASSERT_TRUE(ccuTicks->size() == 4);
}

/**
 * |  用例集 | InstrDetailTableTest
 * | 测试函数 | GetColumnData
 * |  用例名  | test_get_column_icache_cyc_when_empty_and_expect_resize
 * | 用例描述 | 测试InstrDetailTable的ICACHE_CYC列，当列数据为空时，应自动resize并用UINT64_MAX填充
 */
TEST(InstrDetailTableTest, test_get_column_icache_cyc_when_empty_and_expect_resize) {
    std::vector<MergeInfo> mergeInfo = CreateTestMergeInfo();
    InstrDetailTable table(mergeInfo);

    auto column = table.GetColumnData<uint64_t>(InstrDetailTable::ICACHE_CYC);
    ASSERT_TRUE(column != nullptr);
    ASSERT_TRUE(column->size() == mergeInfo.size());

    for (auto val : *column) {
        ASSERT_TRUE(val == UINT64_MAX);
    }
}

/**
 * |  用例集 | InstrDetailTableTest
 * | 测试函数 | GetColumnData
 * |  用例名  | test_get_column_ccu_cyc_when_empty_and_expect_resize
 * | 用例描述 | 测试InstrDetailTable的CCU_CYC列，当列数据为空时，应自动resize并用UINT64_MAX填充
 */
TEST(InstrDetailTableTest, test_get_column_ccu_cyc_when_empty_and_expect_resize) {
    std::vector<MergeInfo> mergeInfo = CreateTestMergeInfo();
    InstrDetailTable table(mergeInfo);

    auto column = table.GetColumnData<uint64_t>(InstrDetailTable::CCU_CYC);
    ASSERT_TRUE(column != nullptr);
    ASSERT_TRUE(column->size() == mergeInfo.size());

    for (auto val : *column) {
        ASSERT_TRUE(val == UINT64_MAX);
    }
}

/**
 * |  用例集 | InstrDetailTableTest
 * | 测试函数 | UpdateColumnAllValue
 * |  用例名  | test_update_column_all_value_icache_cyc_and_expect_success
 * | 用例描述 | 测试InstrDetailTable的ICACHE_CYC列批量更新，应成功更新所有值
 */
TEST(InstrDetailTableTest, test_update_column_all_value_icache_cyc_and_expect_success) {
    std::vector<MergeInfo> mergeInfo = CreateTestMergeInfo();
    InstrDetailTable table(mergeInfo);

    std::vector<uint64_t> testValues = {100, 200, 150, UINT64_MAX};
    bool result = table.UpdateColumnAllValue(InstrDetailTable::ICACHE_CYC, testValues);

    ASSERT_TRUE(result);

    auto column = table.GetColumnData<uint64_t>(InstrDetailTable::ICACHE_CYC);
    ASSERT_TRUE(column != nullptr);
    ASSERT_TRUE(column->size() == testValues.size());
    ASSERT_TRUE((*column)[0] == 100);
    ASSERT_TRUE((*column)[1] == 200);
    ASSERT_TRUE((*column)[2] == 150);
    ASSERT_TRUE((*column)[3] == UINT64_MAX);
}

/**
 * |  用例集 | InstrDetailTableTest
 * | 测试函数 | UpdateColumnAllValue
 * |  用例名  | test_update_column_all_value_ccu_cyc_and_expect_success
 * | 用例描述 | 测试InstrDetailTable的CCU_CYC列批量更新，应成功更新所有值
 */
TEST(InstrDetailTableTest, test_update_column_all_value_ccu_cyc_and_expect_success) {
    std::vector<MergeInfo> mergeInfo = CreateTestMergeInfo();
    InstrDetailTable table(mergeInfo);

    std::vector<uint64_t> testValues = {150, 250, 100, UINT64_MAX};
    bool result = table.UpdateColumnAllValue(InstrDetailTable::CCU_CYC, testValues);

    ASSERT_TRUE(result);

    auto column = table.GetColumnData<uint64_t>(InstrDetailTable::CCU_CYC);
    ASSERT_TRUE(column != nullptr);
    ASSERT_TRUE(column->size() == testValues.size());
    ASSERT_TRUE((*column)[0] == 150);
    ASSERT_TRUE((*column)[1] == 250);
    ASSERT_TRUE((*column)[2] == 100);
    ASSERT_TRUE((*column)[3] == UINT64_MAX);
}

/**
 * |  用例集 | InstrDetailTableTest
 * | 测试函数 | UpdateColumnAllValue
 * |  用例名  | test_update_column_all_value_when_size_mismatch_and_expect_fail
 * | 用例描述 | 测试InstrDetailTable错误处理，当更新数据大小不匹配时，应返回失败
 */
TEST(InstrDetailTableTest, test_update_column_all_value_when_size_mismatch_and_expect_fail) {
    std::vector<MergeInfo> mergeInfo = CreateTestMergeInfo();
    InstrDetailTable table(mergeInfo);

    std::vector<uint64_t> wrongSizeValues = {100, 200};
    bool result = table.UpdateColumnAllValue(InstrDetailTable::ICACHE_CYC, wrongSizeValues);

    ASSERT_TRUE(!result);
}

/**
 * |  用例集 | ScalarInstrInfoTest
 * | 测试函数 | ScalarInstrInfo构造函数
 * |  用例名  | test_constructor_and_expect_correct_values
 * | 用例描述 | 测试ScalarInstrInfo结构体的构造函数，验证icacheTick、ccuTick和pc的正确初始化
 */
TEST(ScalarInstrInfoTest, test_constructor_and_expect_correct_values) {
    uint64_t icacheTick = 100;
    uint64_t ccuTick = 200;
    uint64_t pc = 0x1000;

    ScalarInstrInfo info(icacheTick, ccuTick, pc);

    ASSERT_TRUE(info.icacheTick == icacheTick);
    ASSERT_TRUE(info.ccuTick == ccuTick);
    ASSERT_TRUE(info.pc == pc);
}

/**
 * |  用例集 | ScalarHeadCacheTest
 * | 测试函数 | insert和find
 * |  用例名  | test_insert_and_find_and_expect_correct_order
 * | 用例描述 | 测试scalarHeadCache数据结构，验证tick值的插入和自动排序功能，set应按升序排列
 */
TEST(ScalarHeadCacheTest, test_insert_and_find_and_expect_correct_order) {
    scalarHeadCache cache;
    cache[0x1000].insert(300);
    cache[0x1000].insert(100);
    cache[0x1000].insert(200);

    ASSERT_TRUE(cache.find(0x1000) != cache.end());
    ASSERT_TRUE(cache[0x1000].size() == 3);

    auto it = cache[0x1000].begin();
    ASSERT_TRUE(*it == 100);
    ++it;
    ASSERT_TRUE(*it == 200);
    ++it;
    ASSERT_TRUE(*it == 300);
}

/**
 * |  用例集 | ScalarHeadTest
 * | 测试函数 | emplace_back
 * |  用例名  | test_insert_scalar_info_and_expect_correct_vector
 * | 用例描述 | 测试scalarHead数据结构，验证ScalarInstrInfo向量的插入和按时间顺序存储功能
 */
TEST(ScalarHeadTest, test_insert_scalar_info_and_expect_correct_vector) {
    scalarHead scalar;
    scalar[0x1000].emplace_back(ScalarInstrInfo(100, 150, 0x1000));
    scalar[0x1000].emplace_back(ScalarInstrInfo(200, 250, 0x1000));

    ASSERT_TRUE(scalar.find(0x1000) != scalar.end());
    ASSERT_TRUE(scalar[0x1000].size() == 2);
    ASSERT_TRUE(scalar[0x1000][0].icacheTick == 100);
    ASSERT_TRUE(scalar[0x1000][0].ccuTick == 150);
    ASSERT_TRUE(scalar[0x1000][1].icacheTick == 200);
    ASSERT_TRUE(scalar[0x1000][1].ccuTick == 250);
}
