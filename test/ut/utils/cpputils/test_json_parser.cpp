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

#include <iterator>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "mockcpp/mockcpp.hpp"

#include "filesystem.h"
#include "json_parser.h"

using namespace Utility;
const std::string caseFileWrongDType = "test/ut/resources/config_json/test_dtype.json";
const std::string caseFileNoParamDesc = "test/ut/resources/config_json/test_nodesc.json";
const std::string caseFileLargeShape = "test/ut/resources/config_json/test_shape.json";
 const std::string caseNormal = "test/ut/resources/config_json/test_normal.json";

/**
 * |  用例集  | JsonParser
 * | 测试函数 | ParseRunConfigJson
 * |  用例名  | test_ParseRunConfigJson_with_invalid_input_file_expect_empty_result
 * | 用例描述 | 在输入的config文件不满足文件校验函数时，返回空的配置项
 */
 TEST(JsonParser, test_ParseRunConfigJson_with_invalid_input_file_expect_empty_result)
{
    GlobalMockObject::verify();
    MOCKER(&Utility::CheckInputFileValid)
            .stubs()
            .will(returnValue(false))
            .then(returnValue(true));
    MOCKER(&Utility::CheckOwnerPermission)
            .stubs()
            .will(returnValue(true));
    MOCKER(&Utility::CheckPermission)
            .stubs()
            .will(returnValue(false));
    // Check Input File failed
    std::vector<CaseConfig> checkInputFileFailedRes = ParseRunConfigJson(caseFileWrongDType);
    EXPECT_EQ(checkInputFileFailedRes.size(), 0);

    // Check ParentPath Owner Permission passed (downgraded to warning)
    // Check ParentPath Permission failed
    std::vector<CaseConfig> checkParentPathPermissionFailedRes = ParseRunConfigJson(caseFileWrongDType);
    EXPECT_EQ(checkParentPathPermissionFailedRes.size(), 0);

    // Check json file Permission failed
    std::vector<CaseConfig> checkJsonFilePermissionFailedRes = ParseRunConfigJson(caseFileWrongDType);
    EXPECT_EQ(checkJsonFilePermissionFailedRes.size(), 0);
    
    GlobalMockObject::verify();
}

/**
 * |  用例集  | JsonParser
 * | 测试函数 | ParseRunConfigJson
 * |  用例名  | test_ParseRunConfigJson_with_wrong_dType_expect_empty_result
 * | 用例描述 | 用户输入的json文件中基础信息均正确，但test_cases中的param_desc里的input的type是不支持类型，函数返回空配置
 */
TEST(JsonParser, test_ParseRunConfigJson_with_wrong_dType_expect_empty_result)
{
    GlobalMockObject::verify();
    MOCKER(&Utility::CheckInputFileValid)
            .stubs()
            .will(returnValue(true));
    MOCKER(&Utility::CheckOwnerPermission)
            .stubs()
            .will(returnValue(true));
    MOCKER(&Utility::CheckPermission)
            .stubs()
            .will(returnValue(true));

    std::vector<CaseConfig> caseConfigVec = ParseRunConfigJson(caseFileWrongDType);
    EXPECT_EQ(caseConfigVec.size(), 0);

    GlobalMockObject::verify();
}

/**
 * |  用例集  | JsonParser
 * | 测试函数 | ParseRunConfigJson
 * |  用例名  | test_ParseRunConfigJson_without_param_desc_expect_empty_result
 * | 用例描述 | 用户输入的json文件中基础信息均正确，但test_cases中缺少必选param_desc参数，函数返回空配置
 */
TEST(JsonParser, test_ParseRunConfigJson_without_param_desc_expect_empty_result)
{
    GlobalMockObject::verify();
    MOCKER(&Utility::CheckInputFileValid)
            .stubs()
            .will(returnValue(true));
    MOCKER(&Utility::CheckOwnerPermission)
            .stubs()
            .will(returnValue(true));
    MOCKER(&Utility::CheckPermission)
            .stubs()
            .will(returnValue(true));

    std::vector<CaseConfig> caseConfigVec = ParseRunConfigJson(caseFileNoParamDesc);
    EXPECT_EQ(caseConfigVec.size(), 0);

    GlobalMockObject::verify();
}

/**
 * |  用例集  | JsonParser
 * | 测试函数 | ParseRunConfigJson
 * |  用例名  | test_ParseRunConfigJson_with_large_input_shape_expect_empty_result
 * | 用例描述 | 用户输入的json文件中基础信息和test case均正确，但param_desc中输入的input shape过大，函数返回空配置
 */
TEST(JsonParser, test_ParseRunConfigJson_with_large_input_shape_expect_empty_result)
{
    GlobalMockObject::verify();
    MOCKER(&Utility::CheckInputFileValid)
            .stubs()
            .will(returnValue(true));
    MOCKER(&Utility::CheckOwnerPermission)
            .stubs()
            .will(returnValue(true));
    MOCKER(&Utility::CheckPermission)
            .stubs()
            .will(returnValue(true));

    std::vector<CaseConfig> caseConfigVec = ParseRunConfigJson(caseFileLargeShape);
    EXPECT_EQ(caseConfigVec.size(), 0);

    GlobalMockObject::verify();
}

/**
 * |  用例集  | JsonParser
 * | 测试函数 | ParseRunConfigJson
 * |  用例名  | test_ParseRunConfigJson_with_large_input_shape_expect_empty_result
 * | 用例描述 | 用户输入的json文件中基础信息和test case均正确，但param_desc中输入的input shape过大，函数返回空配置
 */
TEST(JsonParser, test_ParseRunConfigJson_with_normal_input_expect_success)
{
    GlobalMockObject::verify();
     MOCKER(&Utility::CheckInputFileValid)
             .stubs()
             .will(returnValue(true));
    MOCKER(&Utility::CheckOwnerPermission)
        .stubs()
        .will(returnValue(true));
    MOCKER(&Utility::CheckPermission)
        .stubs()
        .will(returnValue(true));

    std::vector<CaseConfig> caseConfigVec = ParseRunConfigJson(caseNormal, "msopprof", "ca");
    EXPECT_EQ(caseConfigVec.size(), 1);

    GlobalMockObject::verify();
}
