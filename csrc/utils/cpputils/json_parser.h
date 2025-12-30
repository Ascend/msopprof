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

#ifndef __CPPUTILS_JSONPARSER_H__
#define __CPPUTILS_JSONPARSER_H__

#include <vector>
#include "json.hpp"
#include "log.h"
#include "op_runner.h"
#include "json_defs.h"

namespace Utility {
struct CaseConfig {
    std::string caseName;
    std::string simulatorDumpPath;
    OpRunner::KernelConfig kernelConfig;
};

std::vector<CaseConfig> ParseRunConfigJson(const std::string &jsonFile,
    const std::string &toolsName = "msopt", const std::string &runMode = "onboard");
bool IsJsonDataTypeValid(const nlohmann::json &jsonData, CfgDataType requiredType);
bool IsJsonDataTypeValid(const nlohmann::json &jsonData, const std::string& cfgKeyName);
bool GetJsonData(const std::string &jsonPath, nlohmann::json &jsonData);
size_t GetSize(const std::vector<int64_t>& shape, const std::string& dType);
bool IsParamDescValid(const nlohmann::json &param);
bool ParseParamDesc(const nlohmann::json &singleTestCase, std::vector<OpRunner::Param> &params);
bool ParseKernelConfig(const nlohmann::json &jsonData, OpRunner::KernelConfig &kernelConfig);
bool ParseCaseConfig(const nlohmann::json &jsonData, std::vector<CaseConfig> &configs);
bool CheckRequiredCommonParam(const CaseConfig &singleConfig);
bool RequiredParamCheck(const std::vector<CaseConfig> &configs, const std::string &toolName,
    const std::string &runMode);
bool ParamDescCheck(std::vector<CaseConfig> &configs);
std::vector<CaseConfig> ParseRunConfigJson(const std::string &jsonFile, const std::string &toolsName,
    const std::string &runMode);

template<typename T>
inline void GetJsonParam(const nlohmann::json &jsonData, const std::string &key, T &dest, const T &defaultVal)
{
    if (jsonData.contains(key) && IsJsonDataTypeValid(jsonData[key], key)) {
        jsonData[key].get_to(dest);
    } else {
        LogDebug("Failed to parse json, key %s not exist", key.c_str());
        dest = defaultVal;
    }
}

template<typename T>
inline bool GetJsonParam(const nlohmann::json &jsonData, const std::string &key, T &dest)
{
    if (jsonData.contains(key) && IsJsonDataTypeValid(jsonData[key], key)) {
        jsonData[key].get_to(dest);
        return true;
    }
    return false;
}
}
#endif  // __CPPUTILS_JSONPARSER_H__
