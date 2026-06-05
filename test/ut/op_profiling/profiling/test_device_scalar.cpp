/* -------------------------------------------------------------------------
 *  This file is part of the MindStudio project.
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
 * ------------------------------------------------------------------------- */

#include <gtest/gtest.h>
#include "mockcpp/mockcpp.hpp"

#include <string>
#include <sys/stat.h>
#include "smart_pointer.h"
#define private public
#define protected public
#include "profiling/device/data_parse/metric_data_handler.h"
#include "profiling/device/data_parse/metric_csv_header.h"
#include "profiling/device/data_visualize/storage_access.h"
#include "profiling/device/data_visualize/basic_op_and_pmu.h"
#include "common/hal_helper.h"
#undef private
#undef protected
#include "common/defs.h"

using namespace Utility;
using namespace Profiling;
using namespace Common;
using namespace Visualize;
using namespace std;

SplitBlockPmuData CreatePmuDataWithScalar(uint16_t pmuId, uint64_t pmuValue, const string &blockType) {
    SplitBlockPmuData pmuData;
    pmuData.blockType = blockType;
    pmuData.pmuEventValueMap[pmuId] = pmuValue;
    return pmuData;
}

class StorageAccessScalarTest : public testing::Test {
protected:
    unique_ptr<DataHandler> handler_;
    shared_ptr<OpBasicInfo> opBasicInfo_;
    shared_ptr<BasicPmu> basicPmu_;
    unique_ptr<PmuCalculator> pmuCalculator_;
    unique_ptr<StorageAccess910B> storage_;
    CalDeviceInfo calDeviceInfo_;
    map<uint16_t, uint64_t> pmuMap_;
    Calculate *cal_;

    void SetUp() override {
        handler_ = Utility::MakeUnique<DataHandlerOf910B>();
        opBasicInfo_ = Utility::MakeShared<OpBasicInfo>(handler_);
        basicPmu_ = Utility::MakeShared<BasicPmu>(handler_);
        pmuCalculator_ = Utility::MakeUnique<PmuCalculator>();
        storage_ = Utility::MakeUnique<StorageAccess910B>(opBasicInfo_, basicPmu_, pmuCalculator_);

        calDeviceInfo_ = {ChipType::ASCEND910B, 1650, 24, 8, "Ascend910B4"};
        cal_ = new Calculate(pmuMap_, 1000, calDeviceInfo_);
    }

    void TearDown() override {
        delete cal_;
        cal_ = nullptr;
        handler_.reset();
        opBasicInfo_.reset();
        basicPmu_.reset();
        pmuCalculator_.reset();
        storage_.reset();
        GlobalMockObject::verify();
    }
};

class DataHandlerScalarTest : public testing::Test {
protected:
    unique_ptr<DataHandlerOf910B> handler910B_;
    ProfMetricsAbilityConfig metrics_;
    ParserConfig config_;

    void SetUp() override {
        handler910B_ = Utility::MakeUnique<DataHandlerOf910B>();
        metrics_.isMemoryDetail = false;
    }

    void TearDown() override {
        handler910B_.reset();
        GlobalMockObject::verify();
    }
};

/**
 * |  用例集  | StorageAccessLoadScalarMixPmu
 * | 测试函数 | LoadScalarMixPmu
 * |  用例名  | test_LoadScalarMixPmu_when_valid_pmu_and_expect_correct_index_map
 * | 用例描述 | 验证输入有效时，返回正确结果
 */
TEST_F(StorageAccessScalarTest, LoadScalarMixPmu_when_valid_pmu_and_expect_correct_index_map) {
    std::map<uint64_t, uint64_t> pmuMap;
    pmuMap[9] = 100;
    pmuMap[112] = 200;
    pmuMap[113] = 300;
    pmuMap[108] = 400;
    pmuMap[109] = 500;
    pmuMap[111] = 600;
    pmuMap[106] = 700;
    pmuMap[114] = 800;
    pmuMap[87] = 900;
    pmuMap[1792] = 1000;
    pmuMap[1793] = 1100;
    pmuMap[1794] = 1200;
    pmuMap[1795] = 1300;
    pmuMap[1796] = 1400;
    pmuMap[1797] = 1500;
    pmuMap[1798] = 1600;
    pmuMap[1799] = 1700;
    pmuMap[1780] = 1800;
    pmuMap[1781] = 1900;
    pmuMap[1782] = 2000;
    pmuMap[1783] = 2100;
    pmuMap[1784] = 2200;
    pmuMap[1785] = 2300;
    pmuMap[1786] = 2400;
    pmuMap[1787] = 2500;

    std::map<std::string, uint64_t> indexMap;

    storage_->LoadScalarMixPmu(" Vec0", pmuMap, indexMap);

    ASSERT_EQ(indexMap["Scalar Time Vec0"], 100);
    ASSERT_EQ(indexMap["Scalar Single Vec0"], 200);
    ASSERT_EQ(indexMap["Scalar Dual Vec0"], 300);
    ASSERT_EQ(indexMap["Scalar Mte2 Stall Vec0"], 400);
    ASSERT_EQ(indexMap["Scalar Mte3 Stall Vec0"], 500);
    ASSERT_EQ(indexMap["Scalar Vector Stall Vec0"], 600);
    ASSERT_EQ(indexMap["Scalar Ub Stall Vec0"], 700);
    ASSERT_EQ(indexMap["Scalar Wait IB Vec0"], 800);
    ASSERT_EQ(indexMap["Scalar Wait Vec0"], 900);

    ASSERT_EQ(indexMap["Scalar Internuclear ID0 Vec0"], 1000);
    ASSERT_EQ(indexMap["Scalar Internuclear ID1 Vec0"], 1100);
    ASSERT_EQ(indexMap["Scalar Internuclear ID2 Vec0"], 1200);
    ASSERT_EQ(indexMap["Scalar Internuclear ID3 Vec0"], 1300);
    ASSERT_EQ(indexMap["Scalar Internuclear ID4 Vec0"], 1400);
    ASSERT_EQ(indexMap["Scalar Internuclear ID5 Vec0"], 1500);
    ASSERT_EQ(indexMap["Scalar Internuclear ID6 Vec0"], 1600);
    ASSERT_EQ(indexMap["Scalar Internuclear ID7 Vec0"], 1700);
    ASSERT_EQ(indexMap["Scalar Internuclear ID8 Vec0"], 1800);
    ASSERT_EQ(indexMap["Scalar Internuclear ID9 Vec0"], 1900);
    ASSERT_EQ(indexMap["Scalar Internuclear ID10 Vec0"], 2000);
    ASSERT_EQ(indexMap["Scalar Internuclear ID11 Vec0"], 2100);
    ASSERT_EQ(indexMap["Scalar Internuclear ID12 Vec0"], 2200);
    ASSERT_EQ(indexMap["Scalar Internuclear ID13 Vec0"], 2300);
    ASSERT_EQ(indexMap["Scalar Internuclear ID14 Vec0"], 2400);
    ASSERT_EQ(indexMap["Scalar Internuclear ID15 Vec0"], 2500);
}

/**
 * |  用例集  | StorageAccessLoadScalarMixPmu
 * | 测试函数 | LoadScalarMixPmu
 * |  用例名  | test_LoadScalarMixPmu_when_missing_pmu_and_expect_zero_value
 * | 用例描述 | 验证值为零时,计算值符合预期
 */
TEST_F(StorageAccessScalarTest, LoadScalarMixPmu_when_missing_pmu_and_expect_zero_value) {
    std::map<uint64_t, uint64_t> pmuMap;
    pmuMap[9] = 100;

    std::map<std::string, uint64_t> indexMap;

    storage_->LoadScalarMixPmu(" Vec1", pmuMap, indexMap);

    ASSERT_EQ(indexMap["Scalar Time Vec1"], 100);
    ASSERT_EQ(indexMap["Scalar Single Vec1"], 0);
    ASSERT_EQ(indexMap["Scalar Dual Vec1"], 0);
}

/**
 * |  用例集  | StorageAccessLoadScalarMixPmu
 * | 测试函数 | LoadScalarMixPmu
 * |  用例名  | test_LoadScalarMixPmu_when_empty_pmu_map_and_expect_all_zero
 * | 用例描述 | 验证输入为空时，值为零时，计算值符合预期
 */
TEST_F(StorageAccessScalarTest, LoadScalarMixPmu_when_empty_pmu_map_and_expect_all_zero) {
    std::map<uint64_t, uint64_t> pmuMap;

    std::map<std::string, uint64_t> indexMap;

    storage_->LoadScalarMixPmu(" Vec0", pmuMap, indexMap);

    ASSERT_EQ(indexMap["Scalar Time Vec0"], 0);
    ASSERT_EQ(indexMap["Scalar Single Vec0"], 0);
    ASSERT_EQ(indexMap["Scalar Internuclear ID0 Vec0"], 0);
}

/**
 * |  用例集  | StorageAccessLoadScalarMixPmu
 * | 测试函数 | LoadScalarMixPmu
 * |  用例名  | test_LoadScalarMixPmu_with_different_core_suffix_and_expect_correct_keys
 * | 用例描述 | 验证返回正确结果
 */
TEST_F(StorageAccessScalarTest, LoadScalarMixPmu_with_different_core_suffix_and_expect_correct_keys) {
    std::map<uint64_t, uint64_t> pmuMap;
    pmuMap[9] = 100;

    std::map<std::string, uint64_t> indexMap1;
    std::map<std::string, uint64_t> indexMap2;

    storage_->LoadScalarMixPmu(" Vec0", pmuMap, indexMap1);
    storage_->LoadScalarMixPmu(" Vec1", pmuMap, indexMap2);

    ASSERT_EQ(indexMap1["Scalar Time Vec0"], 100);
    ASSERT_EQ(indexMap2["Scalar Time Vec1"], 100);

    ASSERT_TRUE(indexMap1.find("Scalar Time Vec1") == indexMap1.end());
    ASSERT_TRUE(indexMap2.find("Scalar Time Vec0") == indexMap2.end());
}

/**
 * |  用例集  | StorageAccessCalculateScalarIndex
 * | 测试函数 | CalculateScalarIndex
 * |  用例名  | test_CalculateScalarIndex_when_valid_input_and_expect_correct_result
 * | 用例描述 | 验证输入有效时，计算值符合预期
 */
TEST_F(StorageAccessScalarTest, CalculateScalarIndex_when_valid_input_and_expect_correct_result) {
    std::map<std::string, uint64_t> basicPmuData;
    basicPmuData["Scalar Time"] = 100;
    basicPmuData["Scalar Single"] = 200;
    basicPmuData["Scalar Dual"] = 300;

    std::vector<std::string> indexVec = {"Scalar Time", "Scalar Single", "Scalar Dual"};

    auto result = storage_->CalculateScalarIndex(indexVec, basicPmuData, *cal_);

    ASSERT_EQ(result["Scalar Time"].cycle, 100);
    ASSERT_STREQ(result["Scalar Time"].time.c_str(), "0.060606");
    ASSERT_EQ(result["Scalar Single"].cycle, 200);
    ASSERT_STREQ(result["Scalar Single"].time.c_str(), "0.121212");
    ASSERT_EQ(result["Scalar Dual"].cycle, 300);
    ASSERT_STREQ(result["Scalar Dual"].time.c_str(), "0.181818");
}

/**
 * |  用例集  | StorageAccessCalculateScalarIndex
 * | 测试函数 | CalculateScalarIndex
 * |  用例名  | test_CalculateScalarIndex_when_empty_index_vec_and_expect_empty_result
 * | 用例描述 | 验证输入为空时，计算值符合预期
 */
TEST_F(StorageAccessScalarTest, CalculateScalarIndex_when_empty_index_vec_and_expect_empty_result) {
    std::map<std::string, uint64_t> basicPmuData;

    std::vector<std::string> indexVec;

    auto result = storage_->CalculateScalarIndex(indexVec, basicPmuData, *cal_);

    ASSERT_EQ(result.size(), 0);
}

/**
 * |  用例集  | StorageAccessSetScalarMemInfo
 * | 测试函数 | SetScalarMemInfo
 * |  用例名  | test_SetScalarMemInfo_with_large_pmu_values_and_expect_no_overflow
 * | 用例描述 | 验证大数值输入时，溢出场景下，计算值符合预期
 */
TEST_F(StorageAccessScalarTest, SetScalarMemInfo_with_large_pmu_values_and_expect_no_overflow) {
    std::map<std::string, uint64_t> basicPmuData;
    basicPmuData["Scalar Time"] = UINT64_MAX;
    basicPmuData["Scalar Single"] = UINT64_MAX;
    basicPmuData["Scalar Dual"] = UINT64_MAX;

    ASSERT_NO_THROW(storage_->SetScalarMemInfo("cube", basicPmuData, *cal_));
}

/**
 * |  用例集  | StorageAccessSetScalarMemInfo
 * | 测试函数 | SetScalarMemInfo
 * |  用例名  | test_SetScalarMemInfo_with_partial_scalar_stall_and_expect_correct_result
 * | 用例描述 | 验证返回正确结果
 */
TEST_F(StorageAccessScalarTest, SetScalarMemInfo_with_partial_scalar_stall_and_expect_correct_result) {
    std::map<std::string, uint64_t> basicPmuData;
    basicPmuData["Scalar Time"] = 1000;
    basicPmuData["Scalar Mte1 Stall"] = 500;
    basicPmuData["Scalar Mte2 Stall"] = 600;

    storage_->SetScalarMemInfo("cube", basicPmuData, *cal_);

    auto &scalarMap = storage_->memInfoScalarMap_;

    bool foundMte1Stall = false;
    bool foundMte2Stall = false;

    if (scalarMap.count("Scalar")) {
        for (size_t i = 0; i < scalarMap["Scalar"].size(); i++) {
            if (storage_->scalarCube_[i].find("Mte1 Stall") != string::npos) {
                foundMte1Stall = true;
                ASSERT_EQ(scalarMap["Scalar"][i].cycle, 500);
            }
            if (storage_->scalarCube_[i].find("Mte2 Stall") != string::npos) {
                foundMte2Stall = true;
                ASSERT_EQ(scalarMap["Scalar"][i].cycle, 600);
            }
        }
    }

    ASSERT_TRUE(foundMte1Stall || foundMte2Stall);
}

/**
 * |  用例集  | StorageAccessAddInternuclearScalarIndex
 * | 测试函数 | AddInternuclearScalarIndex
 * |  用例名  | test_AddInternuclearScalarIndex_with_mix_op_and_both_cores_have_data
 * | 用例描述 | 验证正常功能执行正确
 */
TEST_F(StorageAccessScalarTest, AddInternuclearScalarIndex_with_mix_op_and_both_cores_have_data) {
    std::map<std::string, uint64_t> basicPmuData;
    basicPmuData["Scalar Internuclear ID0"] = 1000;
    basicPmuData["Scalar Internuclear ID0 Vec0"] = 2000;
    basicPmuData["Scalar Internuclear ID0 Vec1"] = 3000;

    storage_->AddInternuclearScalarIndex("mix", basicPmuData, *cal_);

    auto &scalarMap = storage_->memInfoScalarMap_;

    ASSERT_TRUE(scalarMap.count("Scalar Cube"));
    ASSERT_TRUE(scalarMap.count("Scalar Vector Core0"));
    ASSERT_TRUE(scalarMap.count("Scalar Vector Core1"));
}

/**
 * |  用例集  | StorageAccessAddInternuclearScalarIndex
 * | 测试函数 | AddInternuclearScalarIndex
 * |  用例名  | test_AddInternuclearScalarIndex_with_all_ids_present_and_expect_all_added
 * | 用例描述 | 验证全添加id时结果正确
 */
TEST_F(StorageAccessScalarTest, AddInternuclearScalarIndex_with_all_ids_present_and_expect_all_added) {
    std::map<std::string, uint64_t> basicPmuData;

    for (int i = 0; i <= 7; i++) {
        basicPmuData["Scalar Internuclear ID" + to_string(i)] = 1000 * (i + 1);
    }
    for (int i = 8; i <= 15; i++) {
        basicPmuData["Scalar Internuclear ID" + to_string(i)] = 1000 * (i + 1);
    }

    storage_->AddInternuclearScalarIndex("vector", basicPmuData, *cal_);

    auto &scalarMap = storage_->memInfoScalarMap_;

    if (scalarMap.count("Scalar")) {
        size_t internuclearCount = 0;
        for (size_t i = 0; i < scalarMap["Scalar"].size(); i++) {
            if (scalarMap["Scalar"][i].cycle > 0) {
                internuclearCount++;
            }
        }

        ASSERT_EQ(internuclearCount, 16);
    }
}

/**
 * |  用例集  | DataHandlerAddIndexToCsv
 * | 测试函数 | AddIndexToCsv
 * |  用例名  | test_AddIndexToCsv_cube_block_with_scalar_pmu_and_expect_return_true
 * | 用例描述 | 验证Cube算子PMU 1792核间wait指标添加到CSV，aicCalMetricItems包含正确指标名
 */
TEST_F(DataHandlerScalarTest, AddIndexToCsv_cube_block_with_scalar_pmu_and_expect_return_true) {
    SplitBlockPmuData cubePmuData = CreatePmuDataWithScalar(1792, 1000, OpType::CUBE);
    handler910B_->MergeTotalPmuData(cubePmuData);

    handler910B_->AddIndexToCsv(metrics_, config_);

    auto &pipeCsv = handler910B_->metricHeader[string(Common::MsprofMetrics::PIPE_UTILIZATION)];

    bool foundScalarWait = false;
    for (const auto &metric : pipeCsv) {
        if (metric.find("aic_scalar_wait_id0_time") != string::npos) {
            foundScalarWait = true;
            break;
        }
    }

    ASSERT_TRUE(foundScalarWait);
    ASSERT_TRUE(config_.aicCalMetricItems.count("aic_scalar_wait_id0_time(us)"));
}

/**
 * |  用例集  | DataHandlerAddIndexToCsv
 * | 测试函数 | AddIndexToCsv
 * |  用例名  | test_AddIndexToCsv_vector_block_with_scalar_pmu_and_expect_return_true
 * | 用例描述 | 验证Vector算子PMU 1792核间wait指标添加到CSV，aivCalMetricItems包含正确指标名
 */
TEST_F(DataHandlerScalarTest, AddIndexToCsv_vector_block_with_scalar_pmu_and_expect_return_true) {
    SplitBlockPmuData vectorPmuData = CreatePmuDataWithScalar(1792, 2000, OpType::VECTOR);
    handler910B_->MergeTotalPmuData(vectorPmuData);

    handler910B_->AddIndexToCsv(metrics_, config_);

    auto &pipeCsv = handler910B_->metricHeader[string(Common::MsprofMetrics::PIPE_UTILIZATION)];

    bool foundScalarWait = false;
    for (const auto &metric : pipeCsv) {
        if (metric.find("aiv_scalar_wait_id0_time") != string::npos) {
            foundScalarWait = true;
            break;
        }
    }

    ASSERT_TRUE(foundScalarWait);
    ASSERT_TRUE(config_.aivCalMetricItems.count("aiv_scalar_wait_id0_time(us)"));
}

/**
 * |  用例集  | DataHandlerAddIndexToCsv
 * | 测试函数 | AddIndexToCsv
 * |  用例名  | test_AddIndexToCsv_with_memory_detail_and_expect_return_true
 * | 用例描述 | 验证Memory detail模式下DBI指标正确添加到aicCalMetricItems和CSV
 */
TEST_F(DataHandlerScalarTest, AddIndexToCsv_with_memory_detail_and_expect_return_true) {
    metrics_.isMemoryDetail = true;

    SplitBlockPmuData cubePmuData = CreatePmuDataWithScalar(1792, 1000, OpType::CUBE);
    handler910B_->MergeTotalPmuData(cubePmuData);

    handler910B_->AddIndexToCsv(metrics_, config_);

    bool foundDbiMetrics = false;
    for (const auto &metric : config_.aicCalMetricItems) {
        if (metric.find("aic_mte1_active_bw") != string::npos) {
            foundDbiMetrics = true;
            break;
        }
    }

    ASSERT_TRUE(foundDbiMetrics);
}

/**
 * |  用例集  | DataHandlerAddIndexToCsv
 * | 测试函数 | AddIndexToCsv
 * |  用例名  | test_AddIndexToCsv_with_zero_pmu_value_and_expect_skip
 * | 用例描述 | 验证PMU值为零时跳过添加，aicCalMetricItems不包含该指标
 */
TEST_F(DataHandlerScalarTest, AddIndexToCsv_with_zero_pmu_value_and_expect_skip) {
    SplitBlockPmuData cubePmuData = CreatePmuDataWithScalar(1792, 0, OpType::CUBE);
    handler910B_->MergeTotalPmuData(cubePmuData);

    handler910B_->AddIndexToCsv(metrics_, config_);

    ASSERT_FALSE(config_.aicCalMetricItems.count("aic_scalar_wait_id0_time(us)"));
}

/**
 * |  用例集  | DataHandlerAddIndexToCsv
 * | 测试函数 | AddIndexToCsv
 * |  用例名  | test_AddIndexToCsv_with_multiple_scalar_pmu_and_expect_all_added
 * | 用例描述 | 验证多个scalar PMU指标（1792/1793/1794）全部添加到config
 */
TEST_F(DataHandlerScalarTest, AddIndexToCsv_with_multiple_scalar_pmu_and_expect_all_added) {
    SplitBlockPmuData cubePmuData1 = CreatePmuDataWithScalar(1792, 1000, OpType::CUBE);
    SplitBlockPmuData cubePmuData2 = CreatePmuDataWithScalar(1793, 2000, OpType::CUBE);
    SplitBlockPmuData cubePmuData3 = CreatePmuDataWithScalar(1794, 3000, OpType::CUBE);

    handler910B_->MergeTotalPmuData(cubePmuData1);
    handler910B_->MergeTotalPmuData(cubePmuData2);
    handler910B_->MergeTotalPmuData(cubePmuData3);

    handler910B_->AddIndexToCsv(metrics_, config_);

    ASSERT_TRUE(config_.aicCalMetricItems.count("aic_scalar_wait_id0_time(us)"));
    ASSERT_TRUE(config_.aicCalMetricItems.count("aic_scalar_wait_id1_time(us)"));
    ASSERT_TRUE(config_.aicCalMetricItems.count("aic_scalar_wait_id2_time(us)"));
}

/**
 * |  用例集  | DataHandlerAddIndexToCsv
 * | 测试函数 | AddIndexToCsv
 * |  用例名  | test_AddIndexToCsv_with_invalid_pmu_id_and_expect_skip
 * | 用例描述 | 验证无效PMU ID（9999）不匹配scalarPmuToIndex，跳过添加
 */
TEST_F(DataHandlerScalarTest, AddIndexToCsv_with_invalid_pmu_id_and_expect_skip) {
    SplitBlockPmuData cubePmuData = CreatePmuDataWithScalar(9999, 1000, OpType::CUBE);
    handler910B_->MergeTotalPmuData(cubePmuData);

    handler910B_->AddIndexToCsv(metrics_, config_);

    ASSERT_EQ(config_.aicCalMetricItems.size(), 0);
}

/**
 * |  用例集  | DataHandlerAddIndexToCsv
 * | 测试函数 | AddIndexToCsv
 * |  用例名  | test_AddIndexToCsv_with_all_internuclear_ids_and_expect_all_added
 * | 用例描述 | 验证所有16个核间同步ID（1792-1799/1780-1787）全部添加到config
 */
TEST_F(DataHandlerScalarTest, AddIndexToCsv_with_all_internuclear_ids_and_expect_all_added) {
    vector<uint16_t> internuclearIds = {
        1792, 1793, 1794, 1795, 1796, 1797, 1798, 1799, 1780, 1781, 1782, 1783, 1784, 1785, 1786, 1787};

    for (auto pmuId : internuclearIds) {
        SplitBlockPmuData pmuData = CreatePmuDataWithScalar(pmuId, 1000, OpType::CUBE);
        handler910B_->MergeTotalPmuData(pmuData);
    }

    handler910B_->AddIndexToCsv(metrics_, config_);

    for (int i = 0; i <= 7; i++) {
        string metricName = "aic_scalar_wait_id" + to_string(i) + "_time(us)";
        ASSERT_TRUE(config_.aicCalMetricItems.count(metricName));
    }
    for (int i = 8; i <= 15; i++) {
        string metricName = "aic_scalar_wait_id" + to_string(i) + "_time(us)";
        ASSERT_TRUE(config_.aicCalMetricItems.count(metricName));
    }
}

/**
 * |  用例集  | DataHandlerAddIndexToCsv
 * | 测试函数 | AddIndexToCsv
 * |  用例名  | test_AddIndexToCsv_with_mix_op_and_expect_both_aic_and_aiv_metrics
 * | 用例描述 | 验证Mix算子同时添加aic和aiv类型的scalar指标到config
 */
TEST_F(DataHandlerScalarTest, AddIndexToCsv_with_mix_op_and_expect_both_aic_and_aiv_metrics) {
    SplitBlockPmuData cubePmuData = CreatePmuDataWithScalar(1792, 1000, OpType::CUBE);
    SplitBlockPmuData vectorPmuData = CreatePmuDataWithScalar(1792, 2000, OpType::VECTOR);

    handler910B_->MergeTotalPmuData(cubePmuData);
    handler910B_->MergeTotalPmuData(vectorPmuData);

    handler910B_->AddIndexToCsv(metrics_, config_);

    ASSERT_TRUE(config_.aicCalMetricItems.count("aic_scalar_wait_id0_time(us)"));
    ASSERT_TRUE(config_.aivCalMetricItems.count("aiv_scalar_wait_id0_time(us)"));
}

/**
 * |  用例集  | DataHandlerAddIndexToCsv
 * | 测试函数 | AddIndexToCsv
 * |  用例名  | test_AddIndexToCsv_with_empty_total_pmu_data_and_expect_no_metrics_added
 * | 用例描述 | 验证空PMU数据时aicCalMetricItems和aivCalMetricItems都为空
 */
TEST_F(DataHandlerScalarTest, AddIndexToCsv_with_empty_total_pmu_data_and_expect_no_metrics_added) {
    handler910B_->AddIndexToCsv(metrics_, config_);

    ASSERT_EQ(config_.aicCalMetricItems.size(), 0);
    ASSERT_EQ(config_.aivCalMetricItems.size(), 0);
}

/**
 * |  用例集  | DataHandlerAddIndexToCsv
 * | 测试函数 | AddIndexToCsv
 * |  用例名  | test_AddIndexToCsv_with_large_pmu_value_and_expect_correct_handling
 * | 用例描述 | 验证UINT64_MAX大数值PMU值时正确处理，不抛出异常
 */
TEST_F(DataHandlerScalarTest, AddIndexToCsv_with_large_pmu_value_and_expect_correct_handling) {
    SplitBlockPmuData cubePmuData = CreatePmuDataWithScalar(1792, UINT64_MAX, OpType::CUBE);
    handler910B_->MergeTotalPmuData(cubePmuData);

    ASSERT_NO_THROW(handler910B_->AddIndexToCsv(metrics_, config_));

    ASSERT_TRUE(config_.aicCalMetricItems.count("aic_scalar_wait_id0_time(us)"));
}

/**
 * |  用例集  | DataHandlerAddIndexToCsv
 * | 测试函数 | AddIndexToCsv
 * |  用例名  | test_AddIndexToCsv_with_duplicate_pmu_ids_and_expect_no_duplicate_metrics
 * | 用例描述 | 验证重复PMU ID时去重处理，aicCalMetricItems不包含重复指标
 */
TEST_F(DataHandlerScalarTest, AddIndexToCsv_with_duplicate_pmu_ids_and_expect_no_duplicate_metrics) {
    SplitBlockPmuData cubePmuData1 = CreatePmuDataWithScalar(1792, 1000, OpType::CUBE);
    SplitBlockPmuData cubePmuData2 = CreatePmuDataWithScalar(1792, 2000, OpType::CUBE);

    handler910B_->MergeTotalPmuData(cubePmuData1);
    handler910B_->MergeTotalPmuData(cubePmuData2);

    handler910B_->AddIndexToCsv(metrics_, config_);

    size_t count = config_.aicCalMetricItems.count("aic_scalar_wait_id0_time(us)");
    ASSERT_LE(count, 1);
}

/**
 * |  用例集  | DataHandlerAddIndexToCsv
 * | 测试函数 | AddIndexToCsv
 * |  用例名  | test_AddIndexToCsv_with_partial_scalar_pmu_set_and_expect_subset_added
 * | 用例描述 | 验证部分scalar PMU子集（1792/1793/1794）时只添加子集指标
 */
TEST_F(DataHandlerScalarTest, AddIndexToCsv_with_partial_scalar_pmu_set_and_expect_subset_added) {
    vector<uint16_t> partialIds = {1792, 1793, 1794};

    for (auto pmuId : partialIds) {
        SplitBlockPmuData pmuData = CreatePmuDataWithScalar(pmuId, 1000, OpType::CUBE);
        handler910B_->MergeTotalPmuData(pmuData);
    }

    handler910B_->AddIndexToCsv(metrics_, config_);

    ASSERT_TRUE(config_.aicCalMetricItems.count("aic_scalar_wait_id0_time(us)"));
    ASSERT_TRUE(config_.aicCalMetricItems.count("aic_scalar_wait_id1_time(us)"));
    ASSERT_TRUE(config_.aicCalMetricItems.count("aic_scalar_wait_id2_time(us)"));
    ASSERT_FALSE(config_.aicCalMetricItems.count("aic_scalar_wait_id3_time(us)"));
}

/**
 * |  用例集  | StorageAccessSetScalarMemInfo
 * | 测试函数 | SetScalarMemInfo
 * |  用例名  | test_SetScalarMemInfo_with_vector_op_and_expect_scalar_vector_stall_added
 * | 用例描述 | 验证Vector算子类型时，包含Scalar Ub Stall和Scalar Vector Stall指标
 */
TEST_F(StorageAccessScalarTest, SetScalarMemInfo_with_vector_op_and_expect_scalar_vector_stall_added) {
    std::map<std::string, uint64_t> basicPmuData;
    basicPmuData["Scalar Time"] = 1000;
    basicPmuData["Scalar Single"] = 200;
    basicPmuData["Scalar Dual"] = 300;
    basicPmuData["Scalar Mte2 Stall"] = 400;
    basicPmuData["Scalar Mte3 Stall"] = 500;
    basicPmuData["Scalar Wait IB"] = 600;
    basicPmuData["Scalar Wait"] = 700;
    basicPmuData["Scalar Ub Stall"] = 800;
    basicPmuData["Scalar Vector Stall"] = 900;

    storage_->SetScalarMemInfo(OpType::VECTOR, basicPmuData, *cal_);

    auto &scalarMap = storage_->memInfoScalarMap_;
    ASSERT_TRUE(scalarMap.count("Scalar"));

    bool foundUbStall = false;
    bool foundVectorStall = false;
    for (size_t i = 0; i < scalarMap["Scalar"].size(); i++) {
        if (storage_->scalarVec_[i].find("Ub Stall") != string::npos) {
            foundUbStall = true;
            ASSERT_EQ(scalarMap["Scalar"][i].cycle, 800);
        }
        if (storage_->scalarVec_[i].find("Vector Stall") != string::npos) {
            foundVectorStall = true;
            ASSERT_EQ(scalarMap["Scalar"][i].cycle, 900);
        }
    }
    ASSERT_TRUE(foundUbStall);
    ASSERT_TRUE(foundVectorStall);
}

/**
 * |  用例集  | StorageAccessSetScalarMemInfo
 * | 测试函数 | SetScalarMemInfo
 * |  用例名  | test_SetScalarMemInfo_with_cube_op_and_expect_scalar_cube_stall_added
 * | 用例描述 | 验证Cube算子类型时，包含Scalar Cube Stall和Scalar Mte1 Stall指标
 */
TEST_F(StorageAccessScalarTest, SetScalarMemInfo_with_cube_op_and_expect_scalar_cube_stall_added) {
    std::map<std::string, uint64_t> basicPmuData;
    basicPmuData["Scalar Time"] = 1000;
    basicPmuData["Scalar Single"] = 200;
    basicPmuData["Scalar Dual"] = 300;
    basicPmuData["Scalar Mte1 Stall"] = 400;
    basicPmuData["Scalar Mte2 Stall"] = 500;
    basicPmuData["Scalar Mte3 Stall"] = 600;
    basicPmuData["Scalar Wait IB"] = 700;
    basicPmuData["Scalar Wait"] = 800;
    basicPmuData["Scalar Cube Stall"] = 900;

    storage_->SetScalarMemInfo(OpType::CUBE, basicPmuData, *cal_);

    auto &scalarMap = storage_->memInfoScalarMap_;
    ASSERT_TRUE(scalarMap.count("Scalar"));

    bool foundCubeStall = false;
    bool foundMte1Stall = false;
    for (size_t i = 0; i < scalarMap["Scalar"].size(); i++) {
        if (storage_->scalarCube_[i].find("Cube Stall") != string::npos) {
            foundCubeStall = true;
            ASSERT_EQ(scalarMap["Scalar"][i].cycle, 900);
        }
        if (storage_->scalarCube_[i].find("Mte1 Stall") != string::npos) {
            foundMte1Stall = true;
            ASSERT_EQ(scalarMap["Scalar"][i].cycle, 400);
        }
    }
    ASSERT_TRUE(foundCubeStall);
    ASSERT_TRUE(foundMte1Stall);
}

/**
 * |  用例集  | StorageAccessSetScalarMemInfo
 * | 测试函数 | SetScalarMemInfo
 * |  用例名  | test_SetScalarMemInfo_with_mix_op_and_expect_all_scalar_cores_added
 * | 用例描述 | 验证Mix算子类型时，包含Scalar Cube、Scalar Vector Core0、Scalar Vector Core1三个部分
 */
TEST_F(StorageAccessScalarTest, SetScalarMemInfo_with_mix_op_and_expect_all_scalar_cores_added) {
    std::map<std::string, uint64_t> basicPmuData;
    basicPmuData["Scalar Time"] = 1000;
    basicPmuData["Scalar Single"] = 200;
    basicPmuData["Scalar Dual"] = 300;
    basicPmuData["Scalar Mte1 Stall"] = 400;
    basicPmuData["Scalar Mte2 Stall"] = 500;
    basicPmuData["Scalar Mte3 Stall"] = 600;
    basicPmuData["Scalar Wait IB"] = 700;
    basicPmuData["Scalar Wait"] = 800;
    basicPmuData["Scalar Cube Stall"] = 900;
    basicPmuData["Scalar Time Vec0"] = 1100;
    basicPmuData["Scalar Single Vec0"] = 1200;
    basicPmuData["Scalar Dual Vec0"] = 1300;
    basicPmuData["Scalar Mte2 Stall Vec0"] = 1400;
    basicPmuData["Scalar Mte3 Stall Vec0"] = 1500;
    basicPmuData["Scalar Wait IB Vec0"] = 1600;
    basicPmuData["Scalar Wait Vec0"] = 1700;
    basicPmuData["Scalar Ub Stall Vec0"] = 1800;
    basicPmuData["Scalar Vector Stall Vec0"] = 1900;
    basicPmuData["Scalar Time Vec1"] = 2100;
    basicPmuData["Scalar Single Vec1"] = 2200;
    basicPmuData["Scalar Dual Vec1"] = 2300;
    basicPmuData["Scalar Mte2 Stall Vec1"] = 2400;
    basicPmuData["Scalar Mte3 Stall Vec1"] = 2500;
    basicPmuData["Scalar Wait IB Vec1"] = 2600;
    basicPmuData["Scalar Wait Vec1"] = 2700;
    basicPmuData["Scalar Ub Stall Vec1"] = 2800;
    basicPmuData["Scalar Vector Stall Vec1"] = 2900;

    storage_->SetScalarMemInfo(OpType::MIX, basicPmuData, *cal_);

    auto &scalarMap = storage_->memInfoScalarMap_;
    ASSERT_TRUE(scalarMap.count("Scalar Cube"));
    ASSERT_TRUE(scalarMap.count("Scalar Vector Core0"));
    ASSERT_TRUE(scalarMap.count("Scalar Vector Core1"));

    ASSERT_EQ(scalarMap["Scalar Cube"].size(), 9);
    ASSERT_EQ(scalarMap["Scalar Vector Core0"].size(), 9);
    ASSERT_EQ(scalarMap["Scalar Vector Core1"].size(), 9);
}

/**
 * |  用例集  | StorageAccessAddInternuclearScalarIndex
 * | 测试函数 | AddInternuclearScalarIndex
 * |  用例名  | test_AddInternuclearScalarIndex_with_vector_op_and_expect_added_to_scalar
 * | 用例描述 | 验证Vector算子类型时，核间同步指标添加到Scalar而不是Scalar Cube
 */
TEST_F(StorageAccessScalarTest, AddInternuclearScalarIndex_with_vector_op_and_expect_added_to_scalar) {
    std::map<std::string, uint64_t> basicPmuData;
    basicPmuData["Scalar Internuclear ID0"] = 1000;
    basicPmuData["Scalar Internuclear ID1"] = 2000;
    basicPmuData["Scalar Internuclear ID2"] = 3000;

    storage_->AddInternuclearScalarIndex(OpType::VECTOR, basicPmuData, *cal_);

    auto &scalarMap = storage_->memInfoScalarMap_;
    ASSERT_TRUE(scalarMap.count("Scalar"));
    ASSERT_FALSE(scalarMap.count("Scalar Cube"));
}

/**
 * |  用例集  | StorageAccessAddInternuclearScalarIndex
 * | 测试函数 | AddInternuclearScalarIndex
 * |  用例名  | test_AddInternuclearScalarIndex_with_cube_op_and_expect_added_to_scalar
 * | 用例描述 | 验证Cube算子类型时，核间同步指标添加到Scalar而不是Scalar Cube
 */
TEST_F(StorageAccessScalarTest, AddInternuclearScalarIndex_with_cube_op_and_expect_added_to_scalar) {
    std::map<std::string, uint64_t> basicPmuData;
    basicPmuData["Scalar Internuclear ID0"] = 1000;
    basicPmuData["Scalar Internuclear ID1"] = 2000;
    basicPmuData["Scalar Internuclear ID2"] = 3000;

    storage_->AddInternuclearScalarIndex(OpType::CUBE, basicPmuData, *cal_);

    auto &scalarMap = storage_->memInfoScalarMap_;
    ASSERT_TRUE(scalarMap.count("Scalar"));
    ASSERT_FALSE(scalarMap.count("Scalar Cube"));
}

/**
 * |  用例集  | StorageAccessAddInternuclearScalarIndex
 * | 测试函数 | AddInternuclearScalarIndex
 * |  用例名  | test_AddInternuclearScalarIndex_with_zero_cycle_and_expect_filtered
 * | 用例描述 | 验证核间同步指标cycle为零时被过滤，不添加到结果
 */
TEST_F(StorageAccessScalarTest, AddInternuclearScalarIndex_with_zero_cycle_and_expect_filtered) {
    std::map<std::string, uint64_t> basicPmuData;
    basicPmuData["Scalar Internuclear ID0"] = 0;
    basicPmuData["Scalar Internuclear ID1"] = 1000;
    basicPmuData["Scalar Internuclear ID2"] = 0;
    basicPmuData["Scalar Internuclear ID3"] = 2000;

    storage_->AddInternuclearScalarIndex(OpType::VECTOR, basicPmuData, *cal_);

    auto &scalarMap = storage_->memInfoScalarMap_;
    if (scalarMap.count("Scalar")) {
        size_t nonZeroCount = 0;
        for (const auto &item : scalarMap["Scalar"]) {
            if (item.cycle > 0) {
                nonZeroCount++;
            }
        }
        ASSERT_EQ(nonZeroCount, 2);
    }
}

/**
 * |  用例集  | StorageAccessAddInternuclearScalarIndex
 * | 测试函数 | AddInternuclearScalarIndex
 * |  用例名  | test_AddInternuclearScalarIndex_with_mix_op_zero_vec_values_and_expect_filtered
 * | 用例描述 | 验证Mix算子类型时，Vec0/Vec1的零值核间同步指标被过滤
 */
TEST_F(StorageAccessScalarTest, AddInternuclearScalarIndex_with_mix_op_zero_vec_values_and_expect_filtered) {
    std::map<std::string, uint64_t> basicPmuData;
    basicPmuData["Scalar Internuclear ID0"] = 1000;
    basicPmuData["Scalar Internuclear ID1"] = 0;
    basicPmuData["Scalar Internuclear ID0 Vec0"] = 2000;
    basicPmuData["Scalar Internuclear ID1 Vec0"] = 0;
    basicPmuData["Scalar Internuclear ID0 Vec1"] = 3000;
    basicPmuData["Scalar Internuclear ID1 Vec1"] = 0;

    storage_->AddInternuclearScalarIndex(OpType::MIX, basicPmuData, *cal_);

    auto &scalarMap = storage_->memInfoScalarMap_;
    ASSERT_TRUE(scalarMap.count("Scalar Cube"));
    ASSERT_TRUE(scalarMap.count("Scalar Vector Core0"));
    ASSERT_TRUE(scalarMap.count("Scalar Vector Core1"));

    size_t cubeCount = 0;
    for (const auto &item : scalarMap["Scalar Cube"]) {
        if (item.cycle > 0) {
            cubeCount++;
        }
    }
    ASSERT_EQ(cubeCount, 1);

    size_t vec0Count = 0;
    for (const auto &item : scalarMap["Scalar Vector Core0"]) {
        if (item.cycle > 0) {
            vec0Count++;
        }
    }
    ASSERT_EQ(vec0Count, 1);

    size_t vec1Count = 0;
    for (const auto &item : scalarMap["Scalar Vector Core1"]) {
        if (item.cycle > 0) {
            vec1Count++;
        }
    }
    ASSERT_EQ(vec1Count, 1);
}
