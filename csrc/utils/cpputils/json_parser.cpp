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

#include "json_parser.h"
#include <fstream>
#include "common/defs.h"
#include "filesystem.h"
#include "number_operation.h"
namespace Utility {
constexpr char const *NULLPTR = "null";
using FunctionType = std::function<void(const nlohmann::json&, std::vector<OpRunner::Param>&)>;

std::unordered_map<std::string, FunctionType> JsonTypeFunc = {
    {"input", [](const nlohmann::json& p, std::vector<OpRunner::Param>& params) {
        if (p[JSON_KEY.at(JsonType::DATA_PATH)] == NULLPTR) {
            params.emplace_back(OpRunner::Param{p[JSON_KEY.at(JsonType::PARAM_TYPE)],
                p[JSON_KEY.at(JsonType::NAME)], "", 0, p[JSON_KEY.at(JsonType::DATA_PATH)], false});
        } else {
            params.emplace_back(OpRunner::Param{p[JSON_KEY.at(JsonType::PARAM_TYPE)],
                p[JSON_KEY.at(JsonType::NAME)], p[JSON_KEY.at(JsonType::TYPE)],
                GetSize(p[JSON_KEY.at(JsonType::SHAPE)], p[JSON_KEY.at(JsonType::TYPE)]),
                p[JSON_KEY.at(JsonType::DATA_PATH)], true});
        }
    }},
    {"output", [](const nlohmann::json& p, std::vector<OpRunner::Param>& params) {
        params.emplace_back(OpRunner::Param{p[JSON_KEY.at(JsonType::PARAM_TYPE)], p[JSON_KEY.at(JsonType::NAME)],
            p[JSON_KEY.at(JsonType::TYPE)], GetSize(p[JSON_KEY.at(JsonType::SHAPE)], p[JSON_KEY.at(JsonType::TYPE)]),
            "", true});
    }},
    {"tiling", [](const nlohmann::json& p, std::vector<OpRunner::Param>& params) {
        params.emplace_back(OpRunner::Param{p[JSON_KEY.at(JsonType::PARAM_TYPE)], "", "",
            p[JSON_KEY.at(JsonType::TILING_DATA_SIZE)], p[JSON_KEY.at(JsonType::TILING_DATA_PATH)], true});
    }},
    {"workspace", [](const nlohmann::json& p, std::vector<OpRunner::Param>& params) {
        params.emplace_back(OpRunner::Param{p[JSON_KEY.at(JsonType::PARAM_TYPE)], "", "",
            p[JSON_KEY.at(JsonType::USER_WORKSPACE_SIZE)], "", true});
    }},
    {"fftsAddr", [](const nlohmann::json& p, std::vector<OpRunner::Param>& params) {
        params.emplace_back(OpRunner::Param{p[JSON_KEY.at(JsonType::PARAM_TYPE)], "", "",
            p[JSON_KEY.at(JsonType::DATA_SIZE)], "", true});
    }},
};

void ProcessCommand(const std::string& key, const nlohmann::json& p, std::vector<OpRunner::Param>& params)
{
    auto it = JsonTypeFunc.find(key);
    if (it != JsonTypeFunc.end()) {
        it->second(p, params); // 调用对应的函数
    }
}

std::map<std::string, uint64_t> TYPE_SIZE_MAP = {
    {"bool", 1UL},
    {"int8", 1UL},
    {"int16", 2UL},
    {"int32", 4UL},
    {"int64", 8UL},
    {"uint8", 1UL},
    {"uint16", 2UL},
    {"uint32", 4UL},
    {"uint64", 8UL},
    {"bfloat16", 2UL},
    {"float16", 2UL},
    {"float32", 4UL},
    {"float64", 8UL}
};
const std::vector<std::string> MSOPT_RUN_MODES = {"ca", "pv", "onboard"};
const std::vector<std::string> MSOPPROF_RUN_MODES = {"ca", "onboard"};
const std::vector<std::string> DATA_TYPE = {
    "bool", "int8", "int16", "int32", "int64", "uint8", "uint16", "uint32", "uint64", "bfloat16", "float16",
    "float32", "float64"
};

const std::map<std::string, CfgDataType> ConfigKeyDataTypeMap = {
    {JSON_KEY.at(JsonType::KERNEL_NAME),                       CfgDataType::STRING},
    {JSON_KEY.at(JsonType::KERNEL_PATH),                       CfgDataType::STRING},
    {JSON_KEY.at(JsonType::BLOCK_DIM),                         CfgDataType::INT},
    {JSON_KEY.at(JsonType::MODE),                              CfgDataType::STRING},
    {JSON_KEY.at(JsonType::DEVICE_ID),                         CfgDataType::INT},
    {JSON_KEY.at(JsonType::TILING_KEY),                        CfgDataType::UINT64},
    {JSON_KEY.at(JsonType::MAGIC),                             CfgDataType::STRING},
    {JSON_KEY.at(JsonType::TEST_CASES),                        CfgDataType::ARRAY},
    {JSON_KEY.at(JsonType::CASE_NAME),                         CfgDataType::STRING},
    {JSON_KEY.at(JsonType::PARAM_DESC),                        CfgDataType::ARRAY},
    {JSON_KEY.at(JsonType::PARAM_TYPE),                        CfgDataType::STRING},
    {JSON_KEY.at(JsonType::TYPE),                              CfgDataType::STRING},
    {JSON_KEY.at(JsonType::SHAPE),                             CfgDataType::ARRAY},
    {JSON_KEY.at(JsonType::DATA_PATH),                         CfgDataType::STRING},
    {JSON_KEY.at(JsonType::USER_WORKSPACE_SIZE),               CfgDataType::INT},
    {JSON_KEY.at(JsonType::TILING_DATA_SIZE),                  CfgDataType::INT},
    {JSON_KEY.at(JsonType::TILING_DATA_PATH),                  CfgDataType::STRING},
    {JSON_KEY.at(JsonType::DATA_SIZE),                         CfgDataType::INT},
    {JSON_KEY.at(JsonType::SIMULATOR_DUMP_PATH),               CfgDataType::STRING},
    {JSON_KEY.at(JsonType::OUTPUT_DATA_PATH),                  CfgDataType::STRING},
    {JSON_KEY.at(JsonType::NAME),                              CfgDataType::STRING},
    {JSON_KEY.at(JsonType::START),                             CfgDataType::INT},
    {JSON_KEY.at(JsonType::END),                               CfgDataType::INT},
    {JSON_KEY.at(JsonType::GPR_COUNT),                         CfgDataType::INT},
    {JSON_KEY.at(JsonType::PROCESS_BYTES),                     CfgDataType::INT},
    {JSON_KEY.at(JsonType::UB_READ_CONFLICT),                  CfgDataType::INT},
    {JSON_KEY.at(JsonType::UB_WRITE_CONFLICT),                 CfgDataType::INT},
    {JSON_KEY.at(JsonType::VEC_UTILIZATION),                   CfgDataType::FLOAT},
    {JSON_KEY.at(JsonType::PIPE),                              CfgDataType::STRING},
    {JSON_KEY.at(JsonType::PC),                                CfgDataType::STRING},
    {JSON_KEY.at(JsonType::DETAIL),                            CfgDataType::STRING},
    {JSON_KEY.at(JsonType::REAL_STALL_CYC),                    CfgDataType::INT},
    {JSON_KEY.at(JsonType::WARP_ID),                           CfgDataType::INT},
    {JSON_KEY.at(JsonType::SCH_ID),                            CfgDataType::INT}
};

bool IsJsonDataTypeValid(const nlohmann::json &jsonData, CfgDataType requiredType)
{
    if (requiredType == CfgDataType::STRING) {
        return jsonData.is_string();
    }
    if (requiredType == CfgDataType::INT) {
        return jsonData.is_number_integer();
    }
    if (requiredType == CfgDataType::UINT64) {
        return jsonData.is_number_unsigned();
    }
    if (requiredType == CfgDataType::ARRAY) {
        return jsonData.is_array();
    }
    if (requiredType == CfgDataType::FLOAT) {
        return jsonData.is_number_float();
    }
    return false;
}

bool IsJsonDataTypeValid(const nlohmann::json &jsonData, const std::string& cfgKeyName)
{
    // cfgKeyName must in ConfigKeyDataTypeMap assured by code implement
    if (ConfigKeyDataTypeMap.find(cfgKeyName) == ConfigKeyDataTypeMap.end()) {
        return false;
    }
    return IsJsonDataTypeValid(jsonData, ConfigKeyDataTypeMap.at(cfgKeyName));
}

inline bool CheckRequiredParamsJson(const nlohmann::json &jsonData, const std::vector<std::string> &params)
{
    for (const std::string &param : params) {
        if (!jsonData.contains(param)) {
            LogError("Json config error, param:[%s] is not exist.", param.c_str());
            return false;
        }
        if (!IsJsonDataTypeValid(jsonData[param], param)) {
            LogError("Json config error, param:[%s] input data type is not correct.", param.c_str());
            return false;
        }
    }
    return true;
}

bool GetJsonData(const std::string &jsonPath, nlohmann::json &jsonData)
{
    // 读取JSON文件
    std::ifstream file(jsonPath);
    if (!file) {
        LogError("Fail to open json file");
        return false;
    }
    // 解析JSON数据
    try {
        file >> jsonData;
    } catch (const std::exception &e) {
        LogError("Json parsing error: %s", e.what());
        return false;
    }
    file.close();
    LogInfo("Json parse success");
    return true;
}

size_t GetSize(const std::vector<int64_t>& shape, const std::string& dType)
{
    std::vector<uint64_t> temp;
    for (const auto& num : shape) {
        temp.emplace_back(static_cast<uint64_t>(num));
    }
    std::string location = "config mode get size";
    size_t sizeShape = Utility::SafeMulAll(temp, location, false);
    if (TYPE_SIZE_MAP.find(dType) == TYPE_SIZE_MAP.end()) {
        return 0;
    }
    return sizeShape * TYPE_SIZE_MAP[dType];
}

bool IsParamDescValid(const nlohmann::json &param)
{
    if (!CheckRequiredParamsJson(param, {JSON_KEY.at(JsonType::PARAM_TYPE)})) {
        return false;
    }
    if (param[JSON_KEY.at(JsonType::PARAM_TYPE)] == "input") {
        return param[JSON_KEY.at(JsonType::DATA_PATH)] == NULLPTR ? CheckRequiredParamsJson(param,
            {JSON_KEY.at(JsonType::NAME), JSON_KEY.at(JsonType::DATA_PATH)})
            : CheckRequiredParamsJson(param, {JSON_KEY.at(JsonType::NAME), JSON_KEY.at(JsonType::TYPE),
            JSON_KEY.at(JsonType::SHAPE), JSON_KEY.at(JsonType::DATA_PATH)});
    } else if (param[JSON_KEY.at(JsonType::PARAM_TYPE)] == "output") {
        return CheckRequiredParamsJson(param, {JSON_KEY.at(JsonType::NAME), JSON_KEY.at(JsonType::TYPE),
            JSON_KEY.at(JsonType::SHAPE)});
    } else if (param[JSON_KEY.at(JsonType::PARAM_TYPE)] == "tiling") {
        return CheckRequiredParamsJson(param, {JSON_KEY.at(JsonType::TILING_DATA_SIZE),
            JSON_KEY.at(JsonType::TILING_DATA_PATH)});
    } else if (param[JSON_KEY.at(JsonType::PARAM_TYPE)] == "workspace") {
        return CheckRequiredParamsJson(param, {JSON_KEY.at(JsonType::USER_WORKSPACE_SIZE)});
    } else if (param[JSON_KEY.at(JsonType::PARAM_TYPE)] == "fftsAddr") {
        return CheckRequiredParamsJson(param, {JSON_KEY.at(JsonType::DATA_SIZE)});
    }
    return true;
}

bool ParseParamDesc(const nlohmann::json &singleTestCase, std::vector<OpRunner::Param> &params)
{
    if (!CheckRequiredParamsJson(singleTestCase, {JSON_KEY.at(JsonType::PARAM_DESC)})) {
        return false;
    }
    for (const auto &p: singleTestCase[JSON_KEY.at(JsonType::PARAM_DESC)]) {
        if (!IsParamDescValid(p)) {
            return false;
        }
        ProcessCommand(p[JSON_KEY.at(JsonType::PARAM_TYPE)], p, params);
    }
    return true;
}

bool ParseKernelConfig(const nlohmann::json &jsonData, OpRunner::KernelConfig &kernelConfig)
{
    if (!CheckRequiredParamsJson(jsonData, {JSON_KEY.at(JsonType::KERNEL_PATH), JSON_KEY.at(JsonType::KERNEL_NAME),
        JSON_KEY.at(JsonType::MAGIC), JSON_KEY.at(JsonType::MODE)})) {
        return false;
    }
    jsonData[JSON_KEY.at(JsonType::KERNEL_PATH)].get_to(kernelConfig.kernelBinaryPath);
    jsonData[JSON_KEY.at(JsonType::KERNEL_NAME)].get_to(kernelConfig.kernelName);
    jsonData[JSON_KEY.at(JsonType::MAGIC)].get_to(kernelConfig.magic);
    jsonData[JSON_KEY.at(JsonType::MODE)].get_to(kernelConfig.runMode);
    GetJsonParam(jsonData, JSON_KEY.at(JsonType::BLOCK_DIM), kernelConfig.blockDim, 1);
    GetJsonParam(jsonData, JSON_KEY.at(JsonType::DEVICE_ID), kernelConfig.deviceID, 0);
    kernelConfig.hasTilingKey = GetJsonParam(jsonData, JSON_KEY.at(JsonType::TILING_KEY), kernelConfig.tilingKey);
    return true;
}

bool ParseCaseConfig(const nlohmann::json &jsonData, std::vector<CaseConfig> &configs)
{
    if (!CheckRequiredParamsJson(jsonData, {JSON_KEY.at(JsonType::TEST_CASES)})) {
        return false;
    }
    OpRunner::KernelConfig kernelConfig;
    if (!ParseKernelConfig(jsonData, kernelConfig)) {
        return false;
    }

    std::string simulatorDumpPath;
    GetJsonParam(jsonData, JSON_KEY.at(JsonType::SIMULATOR_DUMP_PATH), simulatorDumpPath, std::string{"./model"});
    simulatorDumpPath = JoinPath({simulatorDumpPath, kernelConfig.runMode});

    std::string outputDataPath;
    GetJsonParam(jsonData, JSON_KEY.at(JsonType::OUTPUT_DATA_PATH), outputDataPath, std::string{"./msopt_test_result"});
    for (const auto &singleTestCase: jsonData[JSON_KEY.at(JsonType::TEST_CASES)]) {
        if (!CheckRequiredParamsJson(singleTestCase, {JSON_KEY.at(JsonType::CASE_NAME)})) {
            return false;
        }
        CaseConfig caseConfig{};
        caseConfig.kernelConfig = kernelConfig;
        if (!ParseParamDesc(singleTestCase, caseConfig.kernelConfig.params)) {
            return false;
        }
        singleTestCase[JSON_KEY.at(JsonType::CASE_NAME)].get_to(caseConfig.caseName);
        caseConfig.kernelConfig.outputDataPath = JoinPath({outputDataPath, caseConfig.caseName});
        caseConfig.simulatorDumpPath = JoinPath({simulatorDumpPath, caseConfig.caseName});
        configs.emplace_back(caseConfig);
    }
    return true;
}

bool CheckRequiredCommonParam(const CaseConfig &singleConfig)
{
    if (std::find(Common::MAGICS.begin(), Common::MAGICS.end(), singleConfig.kernelConfig.magic) ==
        Common::MAGICS.end()) {
        LogError("magic value is invalid.");
        return false;
    }
    if (!CheckInputFileValid(singleConfig.kernelConfig.kernelBinaryPath, "kernel",
                             GetSystemAvailableMemory(), "kernel")) {
        return false;
    }
    if (singleConfig.kernelConfig.blockDim <= 0) {
        LogError("blockdim value is invalid.");
        return false;
    }
    if (singleConfig.kernelConfig.deviceID < 0) {
        LogError("device_id value is invalid, it should be >= 0");
        return false;
    }
    if (!CheckInputStringValid(singleConfig.kernelConfig.kernelName, FILE_NAME_LENGTH_LIMIT)) {
        LogError("kernel_name is invalid.");
        return false;
    }
    if (!CheckInputStringValid(singleConfig.caseName, FILE_NAME_LENGTH_LIMIT)) {
        LogError("case_name is invalid.");
        return false;
    }
    return true;
}

bool RequiredParamCheck(const std::vector<CaseConfig> &configs,
                        const std::string &toolName,
                        const std::string &runMode)
{
    if (configs.empty()) {
        LogError("no test case to run");
        return false;
    }
    for (const CaseConfig &singleConfig : configs) {
        if (toolName == "msopt" && std::find(MSOPT_RUN_MODES.begin(), MSOPT_RUN_MODES.end(),
                                             singleConfig.kernelConfig.runMode) == MSOPT_RUN_MODES.end()) {
            LogError("The msopt run mode only support ca/pv/onboard.");
            return false;
        }
        if (toolName == "msopprof") {
            auto it = std::find(MSOPPROF_RUN_MODES.begin(), MSOPPROF_RUN_MODES.end(),
                                singleConfig.kernelConfig.runMode);
            if (it == MSOPPROF_RUN_MODES.end()) {
                LogError("The msopprof run mode only support ca/onboard.");
                return false;
            } else if (runMode != *it) {
                LogError("The command-line-mode:%s and json-run-mode:%s are different.", runMode.c_str(), it->c_str());
                return false;
            }
        }
        if (!CheckRequiredCommonParam(singleConfig)) {
            return false;
        }
    }
    return true;
}

bool ParamDescCheck(std::vector<CaseConfig> &configs)
{
    for (const CaseConfig &singleConfig : configs) {
        if (singleConfig.kernelConfig.params.empty()) {
            LogError("param_desc is not successfully parsed");
            return false;
        }
        for (const OpRunner::Param &param : singleConfig.kernelConfig.params) {
            // check name parameter
            if ((param.type == "input" || param.type == "output") &&
                !CheckInputStringValid(param.name, FILE_NAME_LENGTH_LIMIT)) {
                LogError("%s param name is invalid.", param.type.c_str());
                return false;
            }
            if (!param.isRequired) {
                continue;
            }
            // check dType parameter, type in config
            if ((param.type == "input" || param.type == "output") &&
                std::find(DATA_TYPE.begin(), DATA_TYPE.end(), param.dType) == DATA_TYPE.end()) {
                LogError("%s param dtype is invalid.", param.type.c_str());
                return false;
            }
            // check dataSize parameter. shape user_workspace_size tiling_data_size data_size parameter in config
            if (param.dataSize == 0 || param.dataSize > Utility::MAX_MEM_BYTE_SIZE) {
                LogError("Invalid data_size value, should > 0 and < 30GB");
                return false;
            }
            unsigned long availableMemory = GetSystemAvailableMemory();
            if (param.type == "input" &&
                !CheckInputFileValid(param.dataPath, "bin",
                                     availableMemory, "input " + param.name)) {
                return false;
            }
            if (param.type == "tiling" &&
                !CheckInputFileValid(param.dataPath, "bin",
                                     availableMemory, "tiling " + param.name)) {
                return false;
            }
        }
    }
    return true;
}

std::vector<CaseConfig> ParseRunConfigJson(const std::string &jsonFile,
                                           const std::string &toolsName,
                                           const std::string &runMode)
{
    std::vector<CaseConfig> configs;
    nlohmann::json jsonData;
    if (!CheckInputFileValid(jsonFile, "json", MAX_JSON_FILE_SIZE, "json config")) {
        return configs;
    }
    char path [PATH_MAX + 1];
    auto absPath = realpath(jsonFile.c_str(), path);
    if (absPath == nullptr) {
        LogError("Json file parent path is wrong!");
        return configs;
    }
    std::string jsonParentPath = absPath;
    std::string errorMsg;
    RollbackPath(jsonParentPath, 1);
    if (!CheckOwnerPermission(jsonParentPath, errorMsg)) {
        LogError("%s", errorMsg.c_str());
        return configs;
    }
    if (!CheckPermission(jsonParentPath)) {
        return configs;
    }
    if (!CheckPermission(jsonFile)) {
        return configs;
    }
    if (!GetJsonData(jsonFile, jsonData)) {
        return configs;
    }
    if (!ParseCaseConfig(jsonData, configs)) {
        return {};
    }
    if (!RequiredParamCheck(configs, toolsName, runMode)) {
        return {};
    }
    if (!ParamDescCheck(configs)) {
        return {};
    }
    return configs;
}
}