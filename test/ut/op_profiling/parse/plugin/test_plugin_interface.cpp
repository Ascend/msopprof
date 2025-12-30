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
#include "parse/plugin/plugin_interface.h"
#undef private
#undef protected

using namespace Profiling::Parse;
using namespace Utility;

namespace TestPluginInterfaceFunUtils {
class TestDb1 {};
class TestDb2 {};
class TestOptionalDB {};

DataCenter CreateTempDataCenter(bool db1 = true, bool db2 = true)
{
    DataCenter dataCenter;
    if (db1) {
        std::shared_ptr<TestDb1> testDb1;
        testDb1 = MakeShared<TestDb1>();
        dataCenter.DataTableRegister(testDb1);
    }
    if (db2) {
        std::shared_ptr<TestDb2> testDb2;
        testDb2 = MakeShared<TestDb2>();
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
        return PluginErrorCode::SUCCESS;
    }
    void DependencyRegister() override
    {
        RegisterPluginName("TestPlugin");
        RegisterMandatoryDb({typeid(TestDb1), typeid(TestDb2)});
        RegisterOptionalDb({typeid(TestOptionalDB)});
        RegisterChip({Common::ChipProductType::ASCEND910B1});
    }
};


class TestPlugin2 : public PluginInterface {
public:
    explicit TestPlugin2(DataCenter &dataCenter, BaseContext &context) :
            PluginInterface(dataCenter, context.GetChipType()) {};

    PluginErrorCode Entry() override
    {
        return PluginErrorCode::SUCCESS;
    }
    void DependencyRegister() override
    {
        RegisterPluginName("TestPlugin2");
        RegisterChip({Common::ChipProductType::ASCEND910B_SERIES});
    }
};

class TestPlugin3 : public PluginInterface {
public:
    explicit TestPlugin3(DataCenter &dataCenter, BaseContext &context) :
            PluginInterface(dataCenter, context.GetChipType()) {};

    PluginErrorCode Entry() override
    {
        return PluginErrorCode::SUCCESS;
    }
    void DependencyRegister() override
    {
        RegisterPluginName("TestPlugin3");
        RegisterChip({Common::ChipProductType::ALL_PRODUCT_TYPE});
    }
};
}

using namespace TestPluginInterfaceFunUtils;

/**
 * |  用例集  | PluginInterfaceUTest
 * | 测试函数 | RegisterPluginName
 * |  用例名  | test_PluginInterface_RegisterPluginName_should_save_to_attribute_when_registering
 * | 用例描述 | 测试PluginInterface的RegisterPluginName,当注册插件名时,插件名应该保存到属性中
 */
TEST(PluginInterfaceUTest, test_PluginInterface_DependencyRegisterData_should_save_to_attribute_when_registering) {
    DataCenter testDc = CreateTempDataCenter();
    TestContext testContext {};
    TestPlugin testPlugin {testDc, testContext};
    std::string testName = "Test";
    testPlugin.RegisterPluginName(testName);
    ASSERT_EQ(testPlugin.pluginInfo_.pluginName, testName);
}

/**
 * |  用例集  | PluginInterfaceUTest
 * | 测试函数 | RegisterMandatoryDb
 * |  用例名  | test_PluginInterface_RegisterData_should_save_to_attribute_when_registering
 * | 用例描述 | 测试PluginInterface的RegisterData,当注册数据库时,数据库类型应该保存到属性中
 */
TEST(PluginInterfaceUTest, test_PluginInterface_RegisterData_should_save_to_attribute_when_registering) {
    DataCenter testDc = CreateTempDataCenter();
    TestContext testContext {};
    TestPlugin testPlugin {testDc, testContext};
    testPlugin.RegisterMandatoryDb({typeid(TestDb1), typeid(TestDb2)});
    ASSERT_EQ(testPlugin.pluginInfo_.mandatoryDb.size(), 2);
    testPlugin.RegisterMandatoryDb({typeid(TestDb1), typeid(TestDb2), typeid(TestDb2)});
    ASSERT_EQ(testPlugin.pluginInfo_.mandatoryDb.size(), 3);
}

/**
 * |  用例集  | PluginInterfaceUTest
 * | 测试函数 | RegisterChip
 * |  用例名  | test_PluginInterface_RegisterChip_should_deduplicate_chip_type_when_registering
 * | 用例描述 | RegisterChip,当注册芯片类型时,芯片类型应该保存到属性中且去重
 */
TEST(PluginInterfaceUTest, test_PluginInterface_RegisterChip_should_deduplicate_chip_type_when_registering) {
    DataCenter testDc = CreateTempDataCenter();
    TestContext testContext {};
    TestPlugin testPlugin {testDc, testContext};
    testPlugin.RegisterChip({Common::ChipProductType::ASCEND910B1, Common::ChipProductType::ASCEND310P1});
    ASSERT_EQ(testPlugin.pluginInfo_.chipSupport.size(), 2);
    testPlugin.RegisterChip({Common::ChipProductType::ASCEND910B1, Common::ChipProductType::ASCEND910B1});
    ASSERT_EQ(testPlugin.pluginInfo_.chipSupport.size(), 1);
}

/**
 * |  用例集  | PluginInterfaceUTest
 * | 测试函数 | RegisterKeyPlugin
 * |  用例名  | test_PluginInterface_RegisterKeyPlugin_should_save_to_attribute_when_registering
 * | 用例描述 | 测试PluginInterface的RegisterKeyPlugin,当注册插件为关键插件时,关键插件标记应该保存到属性中
 */
TEST(PluginInterfaceUTest, test_PluginInterface_RegisterKeyPlugin_should_save_to_attribute_when_registering) {
    DataCenter testDc = CreateTempDataCenter();
    TestContext testContext {};
    TestPlugin testPlugin {testDc, testContext};
    std::string testName = "Test";
    testPlugin.RegisterPluginName(testName);
    ASSERT_EQ(testPlugin.pluginInfo_.pluginName, testName);
}

/**
 * |  用例集  | PluginInterfaceUTest
 * | 测试函数 | DependencyCheck
 * |  用例名  | test_PluginInterface_DependencyCheck_should_return_true_when_all_requirement_are_satisfy
 * | 用例描述 | 测试PluginInterface的DependencyCheck,当满足所有依赖时,返回true
 */
TEST(PluginInterfaceUTest, test_PluginInterface_DependencyCheck_should_return_true_when_all_requirement_are_satisfy) {
    DataCenter testDc = CreateTempDataCenter();
    Common::ChipProductType testChipType = Common::ChipProductType::ASCEND910B1;
    TestContext testContext {};

    // 具体芯片类型被注册
    TestPlugin testPlugin {testDc, testContext};
    testPlugin.DependencyRegister();
    ASSERT_TRUE(testPlugin.DependencyCheck(testChipType));

    // 芯片系列被注册
    TestPlugin2 testPlugin2 {testDc, testContext};
    testPlugin2.DependencyRegister();
    ASSERT_TRUE(testPlugin2.DependencyCheck(testChipType));

    // 不区分芯片类型
    TestPlugin3 testPlugin3 {testDc, testContext};
    testPlugin3.DependencyRegister();
    ASSERT_TRUE(testPlugin3.DependencyCheck(testChipType));
}

/**
 * |  用例集  | PluginInterfaceUTest
 * | 测试函数 | DependencyCheck
 * |  用例名  | test_PluginInterface_DependencyCheck_should_return_false_when_missing_mandatory_db
 * | 用例描述 | 测试PluginInterface的DependencyCheck,当缺少必选数据库时,返回false
 */
TEST(PluginInterfaceUTest, test_PluginInterface_DependencyCheck_should_return_false_when_missing_required_db) {
    DataCenter testDc = CreateTempDataCenter(false, true);
    TestContext testContext {};
    TestPlugin testPlugin {testDc, testContext};
    testPlugin.DependencyRegister();

    Common::ChipProductType testChipType = Common::ChipProductType::ASCEND910B1;
    ASSERT_FALSE(testPlugin.DependencyCheck(testChipType));
}

/**
 * |  用例集  | PluginInterfaceUTest
 * | 测试函数 | DependencyCheck
 * |  用例名  | test_PluginInterface_DependencyCheck_should_return_false_when_missing_optional_db
 * | 用例描述 | 测试PluginInterface的DependencyCheck,当缺少可选数据库时,返回true
 */
TEST(PluginInterfaceUTest, test_PluginInterface_DependencyCheck_should_return_false_when_missing_optional_db) {
    DataCenter testDc = CreateTempDataCenter(true, true);
    TestContext testContext {};
    TestPlugin testPlugin {testDc, testContext};
    testPlugin.DependencyRegister();

    Common::ChipProductType testChipType = Common::ChipProductType::ASCEND910B1;
    ASSERT_TRUE(testPlugin.DependencyCheck(testChipType));
}

/**
 * |  用例集  | PluginInterfaceUTest
 * | 测试函数 | DependencyCheck
 * |  用例名  | test_PluginInterface_DependencyCheck_should_return_false_when_missing_required_chip_type
 * | 用例描述 | 测试PluginInterface的DependencyCheck,当缺少所需芯片类型时,返回false
 */
TEST(PluginInterfaceUTest, test_PluginInterface_DependencyCheck_should_return_false_when_missing_required_chip_type) {
    DataCenter testDc = CreateTempDataCenter();
    TestContext testContext {};
    TestPlugin testPlugin {testDc, testContext};
    testPlugin.DependencyRegister();

    Common::ChipProductType testChipType = Common::ChipProductType::ASCEND310P1;
    ASSERT_FALSE(testPlugin.DependencyCheck(testChipType));
}

/**
 * |  用例集  | PluginInterfaceUTest
 * | 测试函数 | Run
 * |  用例名  | test_PluginInterface_Run_should_return_success_when_normal_usage
 * | 用例描述 | 测试PluginInterface的Run,当调用主入口Run时,会自行调用依赖注册和检查函数,通过后调用Entry并返回Entry的返回值
 */
TEST(PluginInterfaceUTest, test_PluginInterface_Run_should_return_success_when_normal_usage) {
    DataCenter testDc = CreateTempDataCenter();
    TestContext testContext {Common::ChipProductType::ASCEND910B1};
    TestPlugin testPlugin {testDc, testContext};
    auto res = testPlugin.Run();
    ASSERT_EQ(res, PluginErrorCode::SUCCESS);
}


/**
 * |  用例集  | PluginInterfaceUTest
 * | 测试函数 | Run
 * |  用例名  | test_PluginInterface_Run_should_return_success_when_normal_usage
 * | 用例描述 | 测试PluginInterface的Run,当插件为key插件但依赖不满足时,应返回FATAL_ERROR
 */
TEST(PluginInterfaceUTest, test_PluginInterface_Run_should_return_fatal_error_when_is_key_plugin_and_dependence_not_satisfy) {
    DataCenter testDc = CreateTempDataCenter();
    TestContext testContext {Common::ChipProductType::ASCEND310P1};
    TestPlugin testPlugin {testDc, testContext};
    testPlugin.RegisterKeyPlugin(true);
    auto res = testPlugin.Run();
    ASSERT_EQ(res, PluginErrorCode::FATAL_ERROR);
}

/**
 * |  用例集  | PluginInterfaceUTest
 * | 测试函数 | Run
 * |  用例名  | test_PluginInterface_Run_should_return_success_when_normal_usage
 * | 用例描述 | 测试PluginInterface的Run,当插件为key插件但依赖不满足时,应返回FATAL_ERROR
 */
TEST(PluginInterfaceUTest, test_PluginInterface_Run_should_return_nonbloking_error_when_is_not_key_plugin_and_dependence_not_satisfy) {
    DataCenter testDc = CreateTempDataCenter();
    TestContext testContext {Common::ChipProductType::ASCEND310P1};
    TestPlugin testPlugin {testDc, testContext};
    testPlugin.RegisterKeyPlugin(false);
    auto res = testPlugin.Run();
    ASSERT_EQ(res, PluginErrorCode::NONBLOCKING_ERROR);
}
