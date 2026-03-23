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
#include "parse/data_center/data_center.h"
#undef private
#undef protected

using namespace Profiling::Parse;
using namespace Utility;

class TestDb {};
class TestDb1 : TestDb {};
class TestDb2 : TestDb {};

template <typename T>
class TestDbTem {};

DataCenter CreateTempDataCenter()
{
    std::shared_ptr<TestDb1> testDb1 = MakeShared<TestDb1>();
    std::shared_ptr<TestDb2> testDb2 = MakeShared<TestDb2>();

    DataCenter dataCenter;
    dataCenter.DataTableRegister(testDb1);
    dataCenter.DataTableRegister(testDb2);
    return dataCenter;
}

/**
 * |  用例集  | DataCenterUTest
 * | 测试函数 | DataTableRegister
 * |  用例名  | test_DataCenter_DataTableRegister_should_have_2_databases_in_map_when_registering_2_database
 * | 用例描述 | 测试DataCenter的DataTableRegister函数能够正确注册2个数据库
 */
TEST(DataCenterUTest, test_DataCenter_DataTableRegister_should_have_1_databases_in_map_when_registering_1_database) {
    std::shared_ptr<TestDb1> testDb1;
    testDb1 = MakeShared<TestDb1>();

    DataCenter dataCenter;
    dataCenter.DataTableRegister(testDb1);
    ASSERT_EQ(dataCenter.dataTablePtrMap_.size(), 1);
}

/**
 * |  用例集  | DataCenterUTest
 * | 测试函数 | DataTableRegister
 * |  用例名  | test_DataCenter_DataTableRegister_should_return_false_when_registering_same_database_type
 * | 用例描述 | 测试DataCenter的DataTableRegister函数在注册相同类型数据库时返回false
 */
TEST(DataCenterUTest, test_DataCenter_DataTableRegister_should_return_false_when_registering_same_database_type) {
    std::shared_ptr<TestDb1> testDb1;
    testDb1 = MakeShared<TestDb1>();

    std::shared_ptr<TestDb1> testDb2;
    testDb2 = MakeShared<TestDb1>();

    DataCenter dataCenter;
    bool res = dataCenter.DataTableRegister(testDb1);
    ASSERT_TRUE(res);
    res = dataCenter.DataTableRegister(testDb2);
    ASSERT_FALSE(res);
}

/**
 * |  用例集  | DataCenterUTest
 * | 测试函数 | DataTableRegister
 * |  用例名  | test_DataCenter_DataTableRegister_should_return_ture_when_registering_different_template_database_type
 * | 用例描述 | 测试DataCenter的DataTableRegister函数在注册不同类型的模板类数据库时返回true
 */
TEST(DataCenterUTest, test_DataCenter_DataTableRegister_should_return_ture_when_registering_different_template_database_type) {
    std::shared_ptr<TestDbTem<int>> testDb1;
    testDb1 = MakeShared<TestDbTem<int>>();

    std::shared_ptr<TestDbTem<float>> testDb2;
    testDb2 = MakeShared<TestDbTem<float>>();

    DataCenter dataCenter;
    bool res = dataCenter.DataTableRegister(testDb1);
    ASSERT_TRUE(res);
    res = dataCenter.DataTableRegister(testDb2);
    ASSERT_TRUE(res);
}

/**
 * |  用例集  | DataCenterUTest
 * | 测试函数 | DataTableRegister
 * |  用例名  | test_DataCenter_DataTableRegister_should_return_false_when_registering_same_template_database_type
 * | 用例描述 | 测试DataCenter的DataTableRegister函数在注册相同类型的模板类数据库时返回false
 */
TEST(DataCenterUTest, test_DataCenter_DataTableRegister_should_return_false_when_registering_same_template_database_type) {
    std::shared_ptr<TestDbTem<int>> testDb1;
    testDb1 = MakeShared<TestDbTem<int>>();

    std::shared_ptr<TestDbTem<int>> testDb2;
    testDb2 = MakeShared<TestDbTem<int>>();

    DataCenter dataCenter;
    bool res = dataCenter.DataTableRegister(testDb1);
    ASSERT_TRUE(res);
    res = dataCenter.DataTableRegister(testDb2);
    ASSERT_FALSE(res);
}

/**
 * |  用例集  | DataCenterUTest
 * | 测试函数 | DataTableRegister
 * |  用例名  | test_DataCenter_DataTableRegister_should_return_false_when_registering_same_database_type
 * | 用例描述 | 测试DataCenter的DataTableRegister函数在注册空指针数据库时返回false
 */
TEST(DataCenterUTest, test_DataCenter_DataTableRegister_should_return_false_when_registering_null) {
    DataCenter dataCenter;
    bool res = dataCenter.DataTableRegister<std::set<int>>(nullptr);
    ASSERT_FALSE(res);
}

/**
 * |  用例集  | DataCenterUTest
 * | 测试函数 | move construct
 * |  用例名  | test_DataCenter_constructor_should_return_moved_instance_when_construct_with_exist_instance
 * | 用例描述 | 测试DataCenter的拷贝构造函数能够正确的保存临时实例的数据
 */
TEST(DataCenterUTest, test_DataCenter_constructor_should_return_moved_instance_data_when_construct_with_exist_instance) {
    DataCenter dataCenter(CreateTempDataCenter());
    ASSERT_EQ(dataCenter.dataTablePtrMap_.size(), 2);
}


/**
 * |  用例集  | DataCenterUTest
 * | 测试函数 | DataTableUnRegister
 * |  用例名  | test_DataCenter_DataTableUnRegister_should_have_1_databases_in_map_when_unregister_1_database
 * | 用例描述 | 测试DataCenter的DataTableUnRegister能够根据输入的acivateDbSet正确删除不需要的数据库
 */
TEST(DataCenterUTest, test_DataCenter_DataTableUnRegister_should_have_1_databases_in_map_when_unregister_1_database) {
    DataCenter dataCenter(CreateTempDataCenter());

    std::type_index activateDbType = typeid(TestDb1);
    std::set<std::type_index> activateDbSet = {activateDbType};
    dataCenter.DataTableUnRegister(activateDbSet);
    ASSERT_EQ(dataCenter.dataTablePtrMap_.size(), 1);
    ASSERT_NE(dataCenter.dataTablePtrMap_.find(activateDbType), dataCenter.dataTablePtrMap_.end());
}

/**
 * |  用例集  | DataCenterUTest
 * | 测试函数 | IsRequiredDbExist
 * |  用例名  | test_DataCenter_IsRequiredDbExist_should_return_true_when_all_query_dbs_are_in_datacenter
 * | 用例描述 | 测试DataCenter的IsRequiredDbExist在输入的数据库类型列表都在DataCenter中时返回true
 */
TEST(DataCenterUTest, test_DataCenter_IsRequiredDbExist_should_return_true_when_all_query_dbs_are_in_datacenter) {
    DataCenter dataCenter(CreateTempDataCenter());
    std::vector<std::type_index> checkDbTpyeIdx = {typeid(TestDb1), typeid(TestDb2)};
    std::vector<bool> expectResult {true, true};
    ASSERT_EQ(dataCenter.IsRequiredDbExist(checkDbTpyeIdx), expectResult);
}

/**
 * |  用例集  | DataCenterUTest
 * | 测试函数 | IsRequiredDbExist
 * |  用例名  | test_DataCenter_IsRequiredDbExist_should_return_false_when_any_db_are_not_in_datacenter
 * | 用例描述 | 测试DataCenter的IsRequiredDbExist在输入的数据库类型列表中存在任意一个不在DataCenter中时返回flase
 */
TEST(DataCenterUTest, test_DataCenter_IsRequiredDbExist_should_return_false_when_any_db_are_not_in_datacenter) {
    DataCenter dataCenter(CreateTempDataCenter());
    std::vector<std::type_index> checkDbTpyeIdx = {typeid(TestDb1), typeid(TestDb2), typeid(TestDbTem<int>)};
    std::vector<bool> expectResult {true, true, false};
    ASSERT_EQ(dataCenter.IsRequiredDbExist(checkDbTpyeIdx), expectResult);
}