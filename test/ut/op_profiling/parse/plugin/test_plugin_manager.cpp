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
#include "parse/plugin/plugin_manager.h"
#undef private
#undef protected

#include "smart_pointer.h"

using namespace Profiling::Parse;
using namespace Utility;

namespace TestPluginManagerFunUtils {
class TestDb1 {};
class TestDb2 {};

DataCenter CreateTempDataCenter(bool db1 = true, bool db2 = true)
{
    DataCenter dataCenter;
    if (db1) {
        std::shared_ptr<TestDb1> testDb1 = MakeShared<TestDb1>();
        dataCenter.DataTableRegister(testDb1);
    }
    if (db2) {
        std::shared_ptr<TestDb2> testDb2 = MakeShared<TestDb2>();
        dataCenter.DataTableRegister(testDb2);
    }
    return dataCenter;
}

class TestContext : public BaseContext {
public:
    TestContext(Common::ChipProductType chipType = Common::ChipProductType::ASCEND910B1) : chipType_(chipType) {}
    Common::ChipProductType GetChipType() const override
    {
        return chipType_;
    }

private:
    Common::ChipProductType chipType_;
};

class TestPlugin : public PluginInterface {
public:
    explicit TestPlugin(DataCenter &dataCenter, BaseContext &context) :
            PluginInterface(dataCenter, context.GetChipType()) {};

    PluginErrorCode Entry() override
    {
        std::shared_ptr<int> a = Utility::MakeShared<int>(1);
        dataCenter_.DataTableRegister(a);
        return PluginErrorCode::SUCCESS;
    }
    void DependencyRegister() override
    {
        RegisterChip({Common::ChipProductType::ALL_PRODUCT_TYPE});
    }
};


class TestPlugin2 : public PluginInterface {
public:
    explicit TestPlugin2(DataCenter &dataCenter, BaseContext &context) :
            PluginInterface(dataCenter, context.GetChipType()) {};

    PluginErrorCode Entry() override
    {
        std::shared_ptr<float> b = Utility::MakeShared<float>(3.14f);
        dataCenter_.DataTableRegister(b);
        return PluginErrorCode::SUCCESS;
    }
    void DependencyRegister() override
    {
        RegisterChip({Common::ChipProductType::ALL_PRODUCT_TYPE});
    }
};
}

using namespace TestPluginManagerFunUtils;

/**
 * |  用例集  | PluginManagerUt
 * | 测试函数 | AddPlugin
 * |  用例名  | test_AddPlugin_should_add_plugin_success_when_adding_same_plugin
 * | 用例描述 | 测试AddPlugin添加2次插件，插件数量是否正确
  */
TEST(PluginManagerUt, test_AddPlugin_should_add_plugin_success_when_adding_same_plugin) {
    DataCenter testDc = CreateTempDataCenter();
    TestContext testContext {};
    
    PluginManager pluginManager(5);
    pluginManager.AddPlugin<TestPlugin>(testDc, testContext);
    ASSERT_EQ(pluginManager.plugins_.size(), 1);

    pluginManager.AddPlugin<TestPlugin>(testDc, testContext);
    ASSERT_EQ(pluginManager.plugins_.size(), 2);
}

/**
 * |  用例集  | PluginManagerUt
 * | 测试函数 | AddPlugin
 * |  用例名  | test_AddPlugin_should_execute_all_plugins_success
 * | 用例描述 | 测试AddPlugin添加2次插件，插件数量是否正确
  */
TEST(PluginManagerUt, test_AddPlugin_should_execute_all_plugins_success) {
    DataCenter testDc = CreateTempDataCenter();
    TestContext testContext {};

    PluginManager pluginManager(2);
    pluginManager.AddPlugin<TestPlugin>(testDc, testContext);
    ASSERT_EQ(pluginManager.plugins_.size(), 1);

    pluginManager.AddPlugin<TestPlugin2>(testDc, testContext);
    ASSERT_EQ(pluginManager.plugins_.size(), 2);

    std::vector<PluginErrorCode> res;
    pluginManager.RunAllPlugins(res);
    std::shared_ptr<int> testPlugin1Res = testDc.GetDbPtr<int>();
    ASSERT_TRUE(testPlugin1Res != nullptr);
    ASSERT_EQ(*testPlugin1Res, 1);

    std::shared_ptr<float> testPlugin2Res = testDc.GetDbPtr<float>();
    ASSERT_TRUE(testPlugin2Res != nullptr);
    ASSERT_EQ(*testPlugin2Res, 3.14f);
}