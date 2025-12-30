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
#define private public
#include "profiling/simulator/data_parse/api_data.h"
#undef private
#include "parse/data_visualizer/sim_visualizer_config.h"
#include "profiling/simulator/data_parse/sim_defs.h"
#include "filesystem.h"
#include "../test_data.h"

using namespace Profiling::Parse;
using namespace Utility;
using namespace Profiling;
using namespace Serialization;
using namespace std;

namespace Visualize {
/**
* |  用例集 | ApiData
* | 测试函数 | FileStats
* |  用例名  | test_file_stats_should_return_ture_when_data_ok
* | 用例描述 | ApiData测试FileStats功能
*/

TEST(ApiData, test_file_stats_should_return_ture_when_data_ok) {
    const std::string output = "test/ut/resources/dump/output";
    const std::string fileName = "test/ut/resources/dump/output/visualize_data.bin";
    std::vector<std::string> fileNames;
    nlohmann::json json;
    Utility::MkdirRecusively(output);
    CodeInstrData data ;
    data.cores = {"core0"};
    CodeLine line {10, {0}, {0}, {1}, {1}, {{"0x0"}, {"0x4"}}};
    CodeFile file {"a", {line}};
    data.files =  {file};
    InstrInfo instr {"0x0", "aa", "ss", "vector", {0}, {0}, {0}, {0}, {0}, {0}, {0}};
    data.instrs = {instr};
    ApiData::FileStats(data, output, json);
    ASSERT_TRUE(Utility::IsExist(fileName));
    std::experimental::filesystem::remove_all(output);
}

/**
* |  用例集 | ApiData
* | 测试函数 | FileDtypeStats
* |  用例名  | test_file_dtype_stats_should_return_ture
* | 用例描述 | ApiData测试FileDtypeStats功能
*/

TEST(ApiData, test_file_dtype_stats_should_return_ture) {
    nlohmann::json json;
    ApiData::FileDtypeStats(json);
    ASSERT_TRUE(json.contains("Files Dtype"));
}
}