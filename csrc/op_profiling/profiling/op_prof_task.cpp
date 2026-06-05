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


#include "op_prof_task.h"
#include <dlfcn.h>
#include <fstream>
#include "common/defs.h"
#include "ascend_helper.h"
#include "filesystem.h"
#include "umask_guard.h"
#include "prof_injection/injection_event.h"

namespace Profiling {
ExecStatus Task::execStatus = ExecStatus::NOT_RUNNING;
std::atomic<bool> Task::inExitMode {false};
std::atomic<bool> Task::killAdvance {false};
std::atomic<bool> Task::needRegisterEvent_ {false};
using namespace Utility;
using namespace Common;
bool Task::CheckSimulatorSoExist() const
{
    std::string simulatorSo = GetSoFromEnvVar("libruntime_camodel.so");
    if (simulatorSo.empty()) {
        return false;
    }
    simulatorSo = Realpath(simulatorSo);
    if (simulatorSo.empty()) {
        return false;
    }
    void *rtHandle = dlopen(simulatorSo.c_str(), RTLD_LAZY);
    if (!rtHandle) {
        return false;
    }
    dlclose(rtHandle);
    rtHandle = nullptr;
    return true;
}

bool Task::IsSetSocVersion(std::string &paramSocVersion) const
{
    if (paramSocVersion.empty()) {
        if (!Utility::GetSocVersionFromEnvVar(paramSocVersion)) {
            Utility::LogDebug("Can not get socVersion from LD_LIBRARY_PATH");
        }
        return false;
    }
    return true;
}

bool Task::ReadConfigFile(const std::string &fileName, const std::map<std::string, std::string> &replaceStrMap,
                          bool &createNewFile, std::vector<std::string> &newLines)
{
    std::string srcPath = JoinPath({camodelLibDir_, fileName});
    std::ifstream file(srcPath);
    if (!file.is_open()) {
        Utility::LogDebug("Failed to open camodel src file [%s].", PrintableFileName(srcPath).c_str());
        return false;
    }
    std::string line;
    // create new config file only when need to update camodel l2cache/multi-thread config,
    // otherwise use config in simulator/lib
    while (getline(file, line)) {
        for (const auto &pair: replaceStrMap) {
            if (line.find(pair.first) != std::string::npos) {
                line = ReplaceSubStr(line, pair.first, pair.second);
                createNewFile = true;
            }
        }
        newLines.push_back(line);
    }
    file.close();
    return true;
}

bool Task::WriteNewConfig(const std::string &fileName, const std::vector<std::string> &newLines)
{
    if (camodelConfigDir_.empty()) {
        std::string configDir = JoinPath({tmpPath_, "config"});
        if (CreateTaskDir(configDir)) {
            camodelConfigDir_ = configDir;
        } else {
            return false;
        }
    }
    std::string destPath = JoinPath({camodelConfigDir_, fileName});
    if (IsExist(destPath)) {
        UpdateNewConfig(destPath, newLines);
        return true;
    }
    {
        Utility::UmaskGuard mask(0027); // umask为0027则文件生成默认权限为750
        std::ofstream outFile(destPath.c_str(), std::ios::out);
        if (!outFile.is_open()) {
            LogDebug("Cannot create camodel config file [%s].", PrintableFileName(destPath).c_str());
            return false;
        }
        for (const std::string &l: newLines) {
            outFile << l << "\n";
        }
        outFile.close();
    }
    if (chmod(destPath.c_str(), READ_ONLY_FILE_AUTHORITY) != 0) {
        LogError("File [%s] chmod failed.", PrintableFileName(destPath).c_str());
        return false;
    }
    LogDebug("Create camodel config file [%s] success.", PrintableFileName(destPath).c_str());
    return true;
}

bool Task::GenOpConfig(nlohmann::json &jsonData)
{
    jsonData["bin_path"] = kernelConfig.kernelBinaryPath;
    jsonData["block_dim"] = std::to_string(kernelConfig.blockDim);
    jsonData["device_id"] = std::to_string(kernelConfig.deviceID);
    jsonData["ffts"] = "N";
    std::map<std::string, std::vector<std::string>> paramMap;
    for (const auto &v: kernelConfig.params) {
        if (v.type == "fftsAddr") {
            jsonData["ffts"] = "Y";
        } else if (v.type == "input") {
            if (v.isRequired) {
                paramMap["input_size"].push_back(std::to_string(v.dataSize));
                paramMap["input_path"].push_back(v.dataPath);
                continue;
            }
            paramMap["input_size"].push_back(std::to_string(0));
            paramMap["input_path"].push_back("n");
        } else if (v.type == "tiling") {
            std::string tiling = v.dataPath + ";" + std::to_string(v.dataSize);
            jsonData["tiling_data_path"] = tiling;
        } else if (v.type == "output") {
            paramMap["output_size"].emplace_back(std::to_string(v.dataSize));
            paramMap["output_name"].emplace_back(v.name);
        } else if (v.type == "workspace") {
            jsonData["workspace_size"] = std::to_string(v.dataSize);
        }
    }
    jsonData["input_path"] = Join(paramMap["input_path"].begin(), paramMap["input_path"].end(), ";");
    jsonData["input_size"] = Join(paramMap["input_size"].begin(), paramMap["input_size"].end(), ";");
    jsonData["kernel_name"] = kernelConfig.kernelName;
    jsonData["magic"] = kernelConfig.magic;

    if (kernelConfig.hasTilingKey) {
        jsonData["tiling_key"] = std::to_string(kernelConfig.tilingKey);
    }
    if (!paramMap["output_size"].empty()) {
        jsonData["output_size"] = Join(paramMap["output_size"].begin(), paramMap["output_size"].end(), ";");
        jsonData["output_name"] = Join(paramMap["output_name"].begin(), paramMap["output_name"].end(), ";");
        jsonData["output_dir"] = kernelConfig.outputDataPath;
    }
    std::string content = jsonData.dump();
    std::string binFilePath = JoinPath({tmpPath_, KERNEL_CONFIG_NAME});
    if (!WriteFileByStream(binFilePath, content.data())) {
        LogWarn("Kernel context save failed, json write failed");
        return false;
    }
    cmd.emplace_back("-c");
    cmd.emplace_back(binFilePath);
    return true;
}

bool Task::UpdateNewConfig(const std::string &filePath, const std::vector<std::string> &newLines) const
{
    struct stat st;
    if (stat(filePath.c_str(), &st) == -1) {
        LogDebug("Stat file [%s] failed.", PrintableFileName(filePath).c_str());
        return false;
    }
    mode_t newMode = st.st_mode | S_IWUSR;
    chmod(filePath.c_str(), newMode);
    newMode = st.st_mode & ~S_IWUSR;
    std::ofstream outFile(filePath.c_str(), std::ios::out);
    if (!outFile.is_open()) {
        LogDebug("Cannot update camodel config file [%s].", PrintableFileName(filePath).c_str());
        chmod(filePath.c_str(), newMode);
        return false;
    }
    for (const std::string &l: newLines) {
        outFile << l << "\n";
    }
    outFile.close();
    chmod(filePath.c_str(), newMode);
    return true;
}

bool Task::CreateConfigFile(const std::string &fileName, const std::map<std::string, std::string> &replaceStrMap)
{
    bool createNewFile = false;
    std::vector<std::string> newLines;
    if (!ReadConfigFile(fileName, replaceStrMap, createNewFile, newLines)) {
        return false;
    }
    if (!createNewFile) {
        LogDebug("No need to create camodel config file [%s].", PrintableFileName(fileName).c_str());
        return false;
    }
    return WriteNewConfig(fileName, newLines);
}

// 仅用于list中添加新的一项， part 为 [LOG]， listName 为 [LOG] 下的enable_list
bool Task::CreateConfigFileWithAppend(const std::string &fileName, const std::string &part,
                                      const std::string &listName, const std::string &item)
{
    std::vector<std::string> newLines;
    if (!CheckIfNeedAppend(fileName, part, listName, item, newLines)) {
        LogDebug("No need to create camodel config file [%s].", PrintableFileName(fileName).c_str());
        return false;
    }
    return WriteNewConfig(fileName, newLines);
}

bool Task::CheckIfNeedAppend(const std::string &fileName, const std::string &part, const std::string &listName,
                             const std::string &item, std::vector<std::string> &newLines)
{
    bool needUpdate = false;
    std::string srcPath = JoinPath({camodelConfigDir_, fileName});
    if (!IsExist(srcPath)) {
        srcPath = JoinPath({camodelLibDir_, fileName});
    }
    ReadFileByMMap(srcPath, newLines);
    if (newLines.empty()) {
        LogDebug("Can't find camodel config src file [%s]", PrintableFileName(srcPath).c_str());
        return false;
    }
    size_t id = 0;
    while (id < newLines.size() && newLines[id] != part + "\r") {
        id++;
    }
    if (id == newLines.size()) {
        LogDebug("Can't find part [%s] in camodel config file [%s]", part.c_str(), PrintableFileName(srcPath).c_str());
        return false;
    }
    while (id < newLines.size()) {
        if (newLines[id].empty()) {
            break;
        }
        if (newLines[id].find(listName) != std::string::npos && newLines[id].find(item) == std::string::npos) {
            size_t pos = newLines[id].find("[");
            if (pos == std::string::npos) {
                id++;
                continue;
            }
            newLines[id].insert(pos + 1, " \"" + item + "\",");
            needUpdate = true;
            break;
        }
        id++;
    }
    return needUpdate;
}

void Task::CreateCamodelConfig(bool pmSamplingEnable)
{
    std::string ascendHomePath;
    if (simSocVersion.empty() || !Utility::GetAscendHomePath(ascendHomePath)) {
        return;
    }
    camodelLibDir_ = JoinPath({ascendHomePath, "tools/simulator", simSocVersion, "lib"});

    if (SOC_910B.count(simSocVersion) > 0 || SOC_910_93.count(simSocVersion) > 0) {
        // master branch: "Ascend910_93_model.toml", "pem_config_cloud.toml"
        // business branch: "config_stars.json", "config.json"
        bool starsRes = CreateConfigFile("Ascend910_93_model.toml",
                                         {{"parsim = 0",           "parsim = 1"},
                                          {"parsim_thd_limit = 0", "parsim_thd_limit = 24"}});
        if (!starsRes) {
            starsRes = CreateConfigFile("config_stars.json", {{"\"parsim\": 0,",          "\"parsim\": 1,"},
                                                              {"\"parsim_thd_limit\": 0", "\"parsim_thd_limit\": 24"}});
        }
        bool configRes = CreateConfigFile("pem_config_cloud.toml", {{"cache_enable                = 0",
                                                                     "cache_enable                = 1"}});
        if (pmSamplingEnable) {
            configRes = CreateConfigFileWithAppend("pem_config_cloud.toml", "[LOG]", "enable_list", "mte_log") || configRes;
        }
        if (!configRes) {
            configRes = CreateConfigFile("config.json", {{"\"cache_enable\": 0,", "\"cache_enable\": 1,"}});
        }
        if (starsRes || configRes) {
            env["CAMODEL_CONFIG_PATH"] = camodelConfigDir_;
        }
    } else if (SOC_310P.count(simSocVersion) != 0) {
        // master branch: "davinci_vec_core.spec", "davinci_mini.spec", and these two files rely on "common.spec"
        // business branch: "config.json"
        std::map<std::string, std::string> replaceStrMap = {{"flush_level = \"3\"", "flush_level = \"2\""}};
        bool vecCoreRes = CreateConfigFile("davinci_vec_core.spec", replaceStrMap);
        bool miniRes = CreateConfigFile("davinci_mini.spec", replaceStrMap);
        bool res = vecCoreRes || miniRes;
        if (res) {
            std::string fileName = "common.spec";
            std::string destPath = JoinPath({camodelConfigDir_, fileName});
            CopyFile(JoinPath({camodelLibDir_, fileName}), destPath);
            if (chmod(destPath.c_str(), READ_ONLY_FILE_AUTHORITY) != 0) {
                LogWarn("File [%s] chmod failed.", destPath.c_str());
            }
        } else {
            res = CreateConfigFile("config.json", {{"\"flush_level\": 3", "\"flush_level\": 2"}});
        }
        if (res) {
            env["CAMODEL_CONFIG_PATH"] = camodelConfigDir_;
        }
    }
}

bool Task::CreateTaskDir(const std::string &path) const
{
    if (IsExist(path)) {
        if (!IsDir(path)) {
            LogError("[%s] is exists and not a directory", path.c_str());
            return false;
        }
    } else {
        if (!MkdirRecusively(path)) {
            return false;
        }
    }
    return true;
}

void Task::RegisterRunningEvent()
{
    if (!needRegisterEvent_) { return; }
    ProfStub::InjectionEvent::Instance().RegisterPacketHandler(
        {ProfPacketType::INSTR_LOG, [&](const std::shared_ptr<ProfStub::Packet>& pkt, size_t) {
            if (pkt != nullptr) { realTimeDataParser_->SetInstrLog(pkt->GetPayload().dvcInstrLog);}
            return "";
        }}
    );
    ProfStub::InjectionEvent::Instance().RegisterPacketHandler(
        {ProfPacketType::POPPED_LOG, [&](const std::shared_ptr<ProfStub::Packet>& pkt, size_t) {
            if (pkt != nullptr) { realTimeDataParser_->SetPopInstrLog(pkt->GetPayload().dvcInstrLog); }
            return "";
        }}
    );
    ProfStub::InjectionEvent::Instance().RegisterPacketHandler(
        {ProfPacketType::MTE_LOG, [&](const std::shared_ptr<ProfStub::Packet>& pkt, size_t) {
            if (pkt != nullptr && realTimeSimParseContext_.metricsConfig.pmSamplingEnable) {
                realTimeDataParser_->SetMteLog(pkt->GetPayload().dvcMteLog);
            }
            return "";
        }}
    );
    ProfStub::InjectionEvent::Instance().RegisterPacketHandler(
        {ProfPacketType::ICACHE_LOG, [&](const std::shared_ptr<ProfStub::Packet>& pkt, size_t) {
            if (pkt != nullptr) { realTimeDataParser_->SetICacheLog(pkt->GetPayload().dvcIcacheLog); }
            return "";
        }}
    );
    ProfStub::InjectionEvent::Instance().RegisterPacketHandler(
        {ProfPacketType::COLLECT_START, [&](const std::shared_ptr<ProfStub::Packet>& pkt, size_t) {
            if (pkt == nullptr) { return "";}
            std::string kernelOutputPath = pkt->GetPayload().collectLogStart.outputPath;
            std::string kernelName = pkt->GetPayload().collectLogStart.kernelName;
            if (!kernelOutputPath.empty() && !kernelName.empty()) {
                isCaLogTransStartSuc_ = true;
                realTimeDataParser_->Start(kernelOutputPath, kernelName);
            }
            return "SUC";
        }}
    );
    ProfStub::InjectionEvent::Instance().RegisterPacketHandler(
        {ProfPacketType::CCU_LOG, [&](const std::shared_ptr<ProfStub::Packet> &pkt, size_t) {
            if (pkt != nullptr) { realTimeDataParser_->SetCcuLog(pkt->GetPayload().dvcCcuLog); }
            return "";
        }}
    );
    ProfStub::InjectionEvent::Instance().RegisterPacketHandler(
        {ProfPacketType::PROF_FINISH, [&](const std::shared_ptr<ProfStub::Packet>& pkt, size_t) {
            (void)pkt;
            realTimeDataParser_->Stop();
            return "SUC";
        }}
    );
    needRegisterEvent_ = false;
}
}  // namespace Profiling
