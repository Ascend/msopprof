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
#include <sys/stat.h>
#include "mockcpp/mockcpp.hpp"

#define private public
#include "parse/data_parser/cache_parser/icache_parser.h"
#undef private
#include "profiling/simulator/data_parse/sim_defs.h"
using namespace Profiling::Parse;
using namespace Utility;
using namespace Profiling;

SimDataParserConfig GetSimDataConfig (const Common::ChipProductType &type) {
    std::string dumpPath910B = "test/ut/resources/dump/910B";
    std::string dumpPath310P = "test/ut/resources/dump/310P";
    CoreNameAndPreFixPair coreNamePair910B {"core0.veccore0", "core0.veccore0."};
    CoreNameAndPreFixPair coreNamePair310P {"core0", "core0."};
    std::set<int> parseIds = {0};
    if (type == Common::ChipProductType::ASCEND310P1) {
        SimDataParserConfig config {dumpPath310P, coreNamePair310P, parseIds, false, type };
        return config;
    }
    SimDataParserConfig config {dumpPath910B, coreNamePair910B, parseIds, false, type};
    return config;
};

/**
 * |  用例集 | ICacheParser
 * | 测试函数 | Entry
 * |  用例名  | test_ICacheParser_910B_should_return_ture_when_parse_ok
 * | 用例描述 | ICacheParser的st，检查解析正常的全部功能
 */
TEST(ICacheParser, test_ICacheParser_910B_should_return_ture_when_parse_ok) {
    DataCenter dataCenter;
    SimDataParserConfig config = GetSimDataConfig(Common::ChipProductType::ASCEND910B1);
    ICacheParser cahcheParser {dataCenter, config};
    std::string dumpPath = "test/ut/resources/dump/910B/core0.veccore0.ifu.icache_log.dump";
    EXPECT_EQ(chmod(dumpPath.c_str(), 0640), 0);
    ASSERT_TRUE(cahcheParser.Entry() == PluginErrorCode::SUCCESS);
    auto cachePtr = dataCenter.GetDbPtr<CacheDetailTable>();
    ASSERT_TRUE(cachePtr != nullptr);
    ASSERT_TRUE(cachePtr->GetCache().size() == 3);
}