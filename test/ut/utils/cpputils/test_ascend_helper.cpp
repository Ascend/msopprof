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
#include <dlfcn.h>
#include <stdlib.h>
#include "mockcpp/mockcpp.hpp"
#define private public
#include "ascend_helper.h"
#undef private
#include "filesystem.h"

using namespace Utility;

class AscendHelperTest : public testing::Test {
protected:
    void SetUp() override {
        const char* tmpAscendHomePath = getenv("ASCEND_HOME_PATH");
        const char* tmpLdLibraryPath = getenv("LD_LIBRARY_PATH");
        ascendHomePathEnv_ = (tmpAscendHomePath == nullptr) ? "" : tmpAscendHomePath;
        ldLibraryPathEnv_ = (tmpLdLibraryPath == nullptr) ? "" : tmpLdLibraryPath;
        unsetenv("ASCEND_HOME_PATH");
        unsetenv("LD_LIBRARY_PATH");
    }

    void TearDown() override {
        if (!ascendHomePathEnv_.empty()) {
            setenv("ASCEND_HOME_PATH", ascendHomePathEnv_.c_str(), 1);
        }
        if (!ldLibraryPathEnv_.empty()) {
            setenv("LD_LIBRARY_PATH", ldLibraryPathEnv_.c_str(), 1);
        }
        GlobalMockObject::verify();
    }
    std::string ascendHomePathEnv_;
    std::string ldLibraryPathEnv_;
};

/**
* |  用例集  | AscendHelper
* | 测试函数 | GetAscendHomePath
* |  用例名  | test_GetAscendHomePath_env_not_set_expect_return_false
* | 用例描述 | 测试ASCEND_HOME_PATH环境变量未设置，返回false
*/
TEST_F(AscendHelperTest, test_GetAscendHomePath_env_not_set_expect_return_false)
{
    std::string path;
    bool ret = GetAscendHomePath(path);
    ASSERT_FALSE(ret);
}

/**
* |  用例集  | AscendHelper
* | 测试函数 | GetAscendHomePath
* |  用例名  | test_GetAscendHomePath_invalid_path_expect_return_false
* | 用例描述 | 测试ASCEND_HOME_PATH环境变量路径不存在，返回false
*/
TEST_F(AscendHelperTest, test_GetAscendHomePath_invalid_path_expect_return_false)
{
    setenv("ASCEND_HOME_PATH", "test/ut/resources/dump/invalid", 1);
    std::string path;
    bool ret = GetAscendHomePath(path);
    ASSERT_FALSE(ret);
}

/**
* |  用例集  | AscendHelper
* | 测试函数 | GetAscendHomePath
* |  用例名  | test_GetAscendHomePath_valid_path_expect_return_true
* | 用例描述 | 测试ASCEND_HOME_PATH环境变量路径存在，返回true
*/
TEST_F(AscendHelperTest, test_GetAscendHomePath_valid_path_expect_return_true)
{
    std::string tmpPath = "test/ut/resources/dump/valid";
    MkdirRecusively(tmpPath);
    setenv("ASCEND_HOME_PATH", tmpPath.c_str(), 1);
    std::string path;
    bool ret = GetAscendHomePath(path);
    ASSERT_TRUE(ret);
    std::experimental::filesystem::remove_all(tmpPath);
}

/**
* |  用例集  | AscendHelper
* | 测试函数 | GetSimulators
* |  用例名  | test_GetSimulators_env_not_set_expect_return_false
* | 用例描述 | 测试ASCEND_HOME_PATH环境变量未设置，获取simulators失败
*/
TEST_F(AscendHelperTest, test_GetSimulators_env_not_set_expect_return_false)
{
    MOCKER(&GetAscendHomePath)
        .stubs()
        .will(returnValue(false));
    std::vector<std::string> sims;
    bool ret = GetSimulators(sims);
    ASSERT_FALSE(ret);
}

/**
* |  用例集  | AscendHelper
* | 测试函数 | GetSimulators
* |  用例名  | test_GetSimulators_list_dir_fail_expect_return_false
* | 用例描述 | 测试ListDir遍历目录失败，返回false
*/
TEST_F(AscendHelperTest, test_GetSimulators_list_dir_fail_expect_return_false)
{
    std::string tmpPath = "test/ut/resources/dump/valid";
    MkdirRecusively(tmpPath);
    setenv("ASCEND_HOME_PATH", tmpPath.c_str(), 1);
    std::vector<std::string> sims;
    bool ret = GetSimulators(sims);
    ASSERT_FALSE(ret);
    std::experimental::filesystem::remove_all(tmpPath);
}

/**
* |  用例集  | AscendHelper
* | 测试函数 | GetSimulators
* |  用例名  | test_GetSimulators_no_ascend_dir_expect_return_true
* | 用例描述 | 测试ListDir遍历目录成功，但无Ascend开头目录，返回true，sim为空
*/
TEST_F(AscendHelperTest, test_GetSimulators_no_ascend_dir_expect_return_true)
{
    std::string tmpPath = "test/ut/resources/dump/valid";
    MkdirRecusively(tmpPath + "/tools/simulator/sim");
    setenv("ASCEND_HOME_PATH", tmpPath.c_str(), 1);
    std::vector<std::string> sims;
    bool ret = GetSimulators(sims);
    ASSERT_TRUE(ret);
    ASSERT_EQ(sims.size(), 0);
    std::experimental::filesystem::remove_all(tmpPath);
}

/**
* |  用例集  | AscendHelper
* | 测试函数 | GetSimulators
* |  用例名  | test_GetSimulators_success_with_ascend_dir_expect_return_true
* | 用例描述 | 测试ListDir遍历目录成功，过滤出Ascend开头目录，返回true，sim不为空
*/
TEST_F(AscendHelperTest, test_GetSimulators_with_ascend_dir_expect_return_true)
{
    std::string tmpPath = "test/ut/resources/dump/valid";
    MkdirRecusively(tmpPath + "/tools/simulator/Ascend1");
    MkdirRecusively(tmpPath + "/tools/simulator/Ascend2");
    setenv("ASCEND_HOME_PATH", tmpPath.c_str(), 1);
    std::vector<std::string> sims;
    bool ret = GetSimulators(sims);
    ASSERT_TRUE(ret);
    ASSERT_EQ(sims.size(), 2);
    std::experimental::filesystem::remove_all(tmpPath);
}

/**
* |  用例集  | AscendHelper
* | 测试函数 | GetSocVersionFromEnvVar
* |  用例名  | test_GetSocVersionFromEnvVar_ascend_home_path_not_set_expect_return_false
* | 用例描述 | 测试ASCEND_HOME_PATH环境变量未设置，返回false
*/
TEST_F(AscendHelperTest, test_GetSocVersionFromEnvVar_ascend_home_path_not_set_expect_return_false)
{
    std::string soc;
    bool ret = GetSocVersionFromEnvVar(soc);
    ASSERT_FALSE(ret);
}

/**
* |  用例集  | AscendHelper
* | 测试函数 | GetSocVersionFromEnvVar
* |  用例名  | test_GetSocVersionFromEnvVar_ld_library_path_not_set_expect_return_false
* | 用例描述 | 测试LD_LIBRARY_PATH环境变量未设置，返回false
*/
TEST_F(AscendHelperTest, test_GetSocVersionFromEnvVar_ld_library_path_not_set_expect_return_false)
{
    setenv("ASCEND_HOME_PATH", "test/ut/resources/dump/valid", 1);
    std::string soc;
    bool ret = GetSocVersionFromEnvVar(soc);
    ASSERT_FALSE(ret);
}

/**
* |  用例集  | AscendHelper
* | 测试函数 | GetSocVersionFromEnvVar
* |  用例名  | test_GetSocVersionFromEnvVar_path_not_match_expect_return_false
* | 用例描述 | 测试LD_LIBRARY_PATH不匹配正则，返回false
*/
TEST_F(AscendHelperTest, test_GetSocVersionFromEnvVar_path_not_match_expect_return_false)
{
    setenv("ASCEND_HOME_PATH", "test/ut/resources/dump/valid", 1);
    setenv("LD_LIBRARY_PATH", "test/ut/resources/dump/valid", 1);
    std::string soc;
    bool ret = GetSocVersionFromEnvVar(soc);
    ASSERT_FALSE(ret);
}

/**
* |  用例集  | AscendHelper
* | 测试函数 | GetSocVersionFromEnvVar
* |  用例名  | test_GetSocVersionFromEnvVar_match_soc_success_expect_return_true
* | 用例描述 | 测试匹配Ascend开头版本号成功，返回true
*/
TEST_F(AscendHelperTest, test_GetSocVersionFromEnvVar_match_soc_success_expect_return_true)
{
    MOCKER(&GetAscendHomePath)
        .stubs()
        .will(returnValue(true));
    setenv("ASCEND_HOME_PATH", "test/ut/resources/dump/valid", 1);
    setenv("LD_LIBRARY_PATH", "test/ut/resources/dump/valid/Ascend910B1/lib", 1);
    std::string soc;
    bool ret = GetSocVersionFromEnvVar(soc);
    ASSERT_TRUE(ret);
    ASSERT_EQ(soc, "Ascend910B1");
}

/**
* |  用例集  | AscendHelper
* | 测试函数 | GetSocVersionFromEnvVar
* |  用例名  | test_GetSocVersionFromEnvVar_match_dav_success_expect_return_true
* | 用例描述 | 测试匹配dav_xxxx映射版本成功，返回true
*/
TEST_F(AscendHelperTest, test_GetSocVersionFromEnvVar_match_dav_success_expect_return_true)
{
    MOCKER(&GetAscendHomePath)
        .stubs()
        .will(returnValue(true));
    setenv("ASCEND_HOME_PATH", "test/ut/resources/dump/valid", 1);
    setenv("LD_LIBRARY_PATH", "test/ut/resources/dump/valid/dav_3510/lib", 1);
    std::string soc;
    bool ret = GetSocVersionFromEnvVar(soc);
    ASSERT_TRUE(ret);
    ASSERT_EQ(soc, "Ascend950PR_9599");
}

/**
* |  用例集  | AscendHelper
* | 测试函数 | GetSoFromEnvVar
* |  用例名  | test_GetSoFromEnvVar_env_not_set_expect_return_empty
* | 用例描述 | 测试LD_LIBRARY_PATH环境变量未设置，返回空字符串
*/
TEST_F(AscendHelperTest, test_GetSoFromEnvVar_env_not_set_expect_return_empty)
{
    std::string so = GetSoFromEnvVar("libascend_hal.so");
    ASSERT_TRUE(so.empty());
}

/**
* |  用例集  | AscendHelper
* | 测试函数 | GetSoFromEnvVar
* |  用例名  | test_GetSoFromEnvVar_env_set_error_expect_return_empty
* | 用例描述 | 测试LD_LIBRARY_PATH环境变量设置错误，返回空字符串
*/
TEST_F(AscendHelperTest, test_GetSoFromEnvVar_env_set_error_expect_return_empty)
{
    setenv("LD_LIBRARY_PATH", "/usr/lib64", 1);
    std::string soName = "";
    MOCKER(&Realpath)
        .stubs()
        .will(returnValue(soName));
    std::string so = GetSoFromEnvVar("libascend_hal.so");
    ASSERT_TRUE(so.empty());
}

/**
* |  用例集  | AscendHelper
* | 测试函数 | GetSoFromEnvVar
* |  用例名  | test_GetSoFromEnvVar_expect_return_empty_valid_path
* | 用例描述 | 测试LD_LIBRARY_PATH环境变量设置，返回正确so路径
*/
TEST_F(AscendHelperTest, test_GetSoFromEnvVar_expect_return_empty_valid_path)
{
    setenv("LD_LIBRARY_PATH", "/usr/lib64", 1);
    std::string soName = "/usr/lib64/libascend_hal.so";
    MOCKER(&Realpath)
        .stubs()
        .will(returnValue(soName));
    std::string so = GetSoFromEnvVar("libascend_hal.so");
    ASSERT_EQ(so, "/usr/lib64/libascend_hal.so");
}
