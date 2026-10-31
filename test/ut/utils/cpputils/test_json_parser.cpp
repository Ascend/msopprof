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

#include <cstdlib>
#include <fstream>
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

 namespace {
 std::string MakeJsonParserTestDir() {
     char path[] = "/tmp/msopprof_json_parser_XXXXXX";
     char *result = mkdtemp(path);
     return result == nullptr ? "" : result;
 }

 CaseConfig MakeValidCaseConfig(const std::string &kernelPath) {
     CaseConfig config{};
     config.caseName = "case1";
     config.kernelConfig.magic = "RT_DEV_BINARY_MAGIC_ELF_AIVEC";
     config.kernelConfig.kernelBinaryPath = kernelPath;
     config.kernelConfig.blockDim = 1;
     config.kernelConfig.deviceID = 0;
     config.kernelConfig.kernelName = "kernel1";
     config.kernelConfig.runMode = "ca";
     return config;
 }
 }

 TEST(JsonParser, test_ParseKernelPath_only_extracts_valid_kernel_file) {
     std::string root = MakeJsonParserTestDir();
     ASSERT_FALSE(root.empty());
     std::string kernelPath = root + "/kernel.o";
     std::ofstream(kernelPath, std::ios::out | std::ios::binary).put('\0');
     std::string configPath = root + "/config.json";
     std::ofstream(configPath) << nlohmann::json({{"kernel_path", kernelPath}}).dump();

     // 参数阶段只读取 kernel_path，不要求 test_cases 等完整执行字段。
     std::string parsedKernelPath;
     EXPECT_TRUE(ParseKernelPath(configPath, parsedKernelPath));
     EXPECT_EQ(parsedKernelPath, kernelPath);

     std::string wrongTypeConfig = root + "/wrong_type.json";
     std::ofstream(wrongTypeConfig) << nlohmann::json({{"kernel_path", 1}}).dump();
     EXPECT_FALSE(ParseKernelPath(wrongTypeConfig, parsedKernelPath));

     std::string missingKernelConfig = root + "/missing_kernel.json";
     std::ofstream(missingKernelConfig) << nlohmann::json({{"kernel_path", root + "/missing.o"}}).dump();
     EXPECT_FALSE(ParseKernelPath(missingKernelConfig, parsedKernelPath));
     std::experimental::filesystem::remove_all(root);
 }

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

// 覆盖 IsJsonDataTypeValid 对 FLOAT 类型的校验分支及未知配置键返回 false 的分支
TEST(JsonParser, test_IsJsonDataTypeValid_with_float_type_and_unknown_key)
{
    nlohmann::json floatData = 1.5;
    EXPECT_TRUE(IsJsonDataTypeValid(floatData, CfgDataType::FLOAT));
    nlohmann::json intData = 1;
    EXPECT_FALSE(IsJsonDataTypeValid(intData, CfgDataType::FLOAT));
    // 配置键不在类型映射表中时返回 false
    nlohmann::json strData = "abc";
    EXPECT_FALSE(IsJsonDataTypeValid(strData, "unknown_key_not_in_map"));
}

// 覆盖 GetJsonData 打开不存在文件失败及解析非法 json 抛异常返回 false 的分支
TEST(JsonParser, test_GetJsonData_with_invalid_file_expect_false)
{
    nlohmann::json jsonData;
    // 文件不存在，打开失败
    EXPECT_FALSE(GetJsonData("test/ut/resources/config_json/not_exist_xyz.json", jsonData));

    // 文件内容不是合法 json，解析抛异常
    std::string root = MakeJsonParserTestDir();
    ASSERT_FALSE(root.empty());
    std::string badJson = root + "/bad.json";
    std::ofstream(badJson) << "{invalid json content";
    EXPECT_FALSE(GetJsonData(badJson, jsonData));
    std::experimental::filesystem::remove_all(root);
}

// 覆盖 ParseKernelPath 在 kernel_path 为空字符串时返回 false 的分支
TEST(JsonParser, test_ParseKernelPath_with_empty_kernel_path_expect_false)
{
    std::string root = MakeJsonParserTestDir();
    ASSERT_FALSE(root.empty());
    std::string configPath = root + "/empty_kernel.json";
    std::ofstream(configPath) << nlohmann::json({{"kernel_path", ""}}).dump();
    std::string kernelPath;
    EXPECT_FALSE(ParseKernelPath(configPath, kernelPath));
    std::experimental::filesystem::remove_all(root);
}

// 覆盖 IsParamDescValid 缺少 param_type 返回 false 及 param_type 为未知类型返回 true 的分支
TEST(JsonParser, test_IsParamDescValid_with_missing_or_unknown_param_type)
{
    // 缺少 param_type 必选字段返回 false
    nlohmann::json missingParamType = {{"name", "x"}};
    EXPECT_FALSE(IsParamDescValid(missingParamType));

    // 未知 param_type 不需要额外字段校验，返回 true
    nlohmann::json unknownType = {{"param_type", "unknown_type"}};
    EXPECT_TRUE(IsParamDescValid(unknownType));
}

// 覆盖 ParseParamDesc 中 input 的 data_path 为 "null" 时构造非必填参数的分支
TEST(JsonParser, test_ParseParamDesc_with_null_data_path_input_expect_success)
{
    nlohmann::json testCase = {
        {"param_desc", nlohmann::json::array({
            {{"param_type", "input"}, {"name", "x"}, {"data_path", "null"}}
        })}
    };
    std::vector<OpRunner::Param> params;
    ASSERT_TRUE(ParseParamDesc(testCase, params));
    ASSERT_EQ(params.size(), 1U);
    EXPECT_FALSE(params[0].isRequired);
}

// 覆盖 ParseParamDesc 在单个参数校验失败时返回 false 的分支
TEST(JsonParser, test_ParseParamDesc_with_invalid_param_expect_false)
{
    // input 缺少 name 必选字段，参数校验失败
    nlohmann::json testCase = {
        {"param_desc", nlohmann::json::array({
            {{"param_type", "input"}, {"type", "float32"}, {"shape", {2}}, {"data_path", "x.bin"}}
        })}
    };
    std::vector<OpRunner::Param> params;
    EXPECT_FALSE(ParseParamDesc(testCase, params));
}

// 覆盖 ParseKernelConfig 缺少必选 kernel 配置字段时返回 false 的分支
TEST(JsonParser, test_ParseKernelConfig_with_missing_required_param_expect_false)
{
    nlohmann::json jsonData = {{"kernel_path", "a.o"}, {"kernel_name", "k"}};
    OpRunner::KernelConfig kernelConfig;
    EXPECT_FALSE(ParseKernelConfig(jsonData, kernelConfig));
}

// 覆盖 ParseCaseConfig 缺少 test_cases 字段及 kernel 配置不完整时返回 false 的分支
TEST(JsonParser, test_ParseCaseConfig_with_invalid_config_expect_false)
{
    std::vector<CaseConfig> configs;
    // 缺少 test_cases 字段
    nlohmann::json noTestCases = {{"kernel_path", "a.o"}};
    EXPECT_FALSE(ParseCaseConfig(noTestCases, configs));

    // test_cases 存在但缺少 kernel 必选配置
    nlohmann::json badKernel = {{"test_cases", nlohmann::json::array()}};
    EXPECT_FALSE(ParseCaseConfig(badKernel, configs));
}

// 覆盖 CheckRequiredCommonParam 对非法 magic 返回 false 的分支
TEST(JsonParser, test_CheckRequiredCommonParam_with_invalid_magic_expect_false)
{
    CaseConfig config = MakeValidCaseConfig("whatever.o");
    config.kernelConfig.magic = "INVALID_MAGIC";
    EXPECT_FALSE(CheckRequiredCommonParam(config));
}

// 覆盖 CheckRequiredCommonParam 在 kernel 文件不存在时返回 false 的分支
TEST(JsonParser, test_CheckRequiredCommonParam_with_nonexist_kernel_expect_false)
{
    CaseConfig config = MakeValidCaseConfig("test/ut/resources/config_json/not_exist_kernel_xyz.o");
    EXPECT_FALSE(CheckRequiredCommonParam(config));
}

// 覆盖 CheckRequiredCommonParam 对非法 blockdim、device_id、kernel_name、case_name 返回 false 的分支
TEST(JsonParser, test_CheckRequiredCommonParam_with_invalid_fields_expect_false)
{
    std::string root = MakeJsonParserTestDir();
    ASSERT_FALSE(root.empty());
    std::string kernelPath = root + "/kernel.o";
    std::ofstream(kernelPath, std::ios::out | std::ios::binary).put('\0');

    CaseConfig config = MakeValidCaseConfig(kernelPath);
    // blockdim 不大于 0
    config.kernelConfig.blockDim = 0;
    EXPECT_FALSE(CheckRequiredCommonParam(config));

    // device_id 小于 0
    config.kernelConfig.blockDim = 1;
    config.kernelConfig.deviceID = -1;
    EXPECT_FALSE(CheckRequiredCommonParam(config));

    // kernel_name 包含非法字符
    config.kernelConfig.deviceID = 0;
    config.kernelConfig.kernelName = "bad-name!";
    EXPECT_FALSE(CheckRequiredCommonParam(config));

    // case_name 包含非法字符
    config.kernelConfig.kernelName = "kernel1";
    config.caseName = "bad case";
    EXPECT_FALSE(CheckRequiredCommonParam(config));

    std::experimental::filesystem::remove_all(root);
}

// 覆盖 RequiredParamCheck 空配置、运行模式不支持及模式不匹配、通用参数校验失败的分支
TEST(JsonParser, test_RequiredParamCheck_with_invalid_run_mode_expect_false)
{
    // 空配置返回 false
    std::vector<CaseConfig> emptyConfigs;
    EXPECT_FALSE(RequiredParamCheck(emptyConfigs, "msopt", "ca"));

    std::string root = MakeJsonParserTestDir();
    ASSERT_FALSE(root.empty());
    std::string kernelPath = root + "/kernel.o";
    std::ofstream(kernelPath, std::ios::out | std::ios::binary).put('\0');
    CaseConfig config = MakeValidCaseConfig(kernelPath);

    // msopt 不支持的模式
    config.kernelConfig.runMode = "invalid_mode";
    std::vector<CaseConfig> configs = {config};
    EXPECT_FALSE(RequiredParamCheck(configs, "msopt", "ca"));

    // msopprof 不支持 pv 模式
    config.kernelConfig.runMode = "pv";
    configs = {config};
    EXPECT_FALSE(RequiredParamCheck(configs, "msopprof", "pv"));

    // msopprof 命令行模式与 json 配置模式不一致
    config.kernelConfig.runMode = "ca";
    configs = {config};
    EXPECT_FALSE(RequiredParamCheck(configs, "msopprof", "onboard"));

    // 通用参数校验失败（kernel 文件不存在）
    config.kernelConfig.kernelBinaryPath = root + "/missing.o";
    configs = {config};
    EXPECT_FALSE(RequiredParamCheck(configs, "msopt", "ca"));

    std::experimental::filesystem::remove_all(root);
}

// 覆盖 ParamDescCheck 参数为空、名称非法、非必填跳过及数据文件校验失败的分支
TEST(JsonParser, test_ParamDescCheck_with_invalid_params_expect_false)
{
    // params 为空返回 false
    CaseConfig emptyParamConfig = MakeValidCaseConfig("kernel.o");
    std::vector<CaseConfig> configs = {emptyParamConfig};
    EXPECT_FALSE(ParamDescCheck(configs));

    // input/output 参数名包含非法字符返回 false
    CaseConfig config = MakeValidCaseConfig("kernel.o");
    config.kernelConfig.params = {{"input", "bad name!", "float32", 4, "x.bin", true}};
    configs = {config};
    EXPECT_FALSE(ParamDescCheck(configs));

    // 非必填参数跳过后续校验，返回 true
    config.kernelConfig.params = {{"input", "x", "", 0, "", false}};
    configs = {config};
    EXPECT_TRUE(ParamDescCheck(configs));

    // input 数据文件不存在返回 false
    config.kernelConfig.params = {{"input", "x", "float32", 4, "not_exist_xyz.bin", true}};
    configs = {config};
    EXPECT_FALSE(ParamDescCheck(configs));

    // tiling 数据文件不存在返回 false
    config.kernelConfig.params = {{"tiling", "", "", 4, "not_exist_xyz.bin", true}};
    configs = {config};
    EXPECT_FALSE(ParamDescCheck(configs));
}

