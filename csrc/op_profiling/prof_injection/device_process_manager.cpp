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


#include "device_process_manager.h"
#include "log.h"
#include "smart_pointer.h"
namespace ProfStub {

std::string DeviceProcessManager::ProcessKernelEvent(const std::shared_ptr<Packet>& packPtr, size_t clientId)
{
    if (packPtr == nullptr) {
        return "\n";
    }
    ProfDataPathConfig config = packPtr->GetPayload().profDataPathConfig;
    std::string key = std::to_string(clientId) + "_" + std::to_string(config.deviceId);
    std::string kernelName = packPtr->GetPayload().profDataPathConfig.kernelName;
    if (skipedKernels_[key] < profConfig_.profSkipTimes_) {
        skipedKernels_[key]++;
        Utility::LogDebug("Skip kernel, kernel name is %s, skip time is %d",
                          kernelName.c_str(), skipedKernels_[key]);
        return "\n";
    }
    if (profedKernels_[key] >= profConfig_.profMaxTimes_) {
        Utility::LogDebug("Prof stop, kernel name is %s, max prof time is %d", kernelName.c_str(),
                          profConfig_.profMaxTimes_);
        return "\n";
    }
    if (!CheckKernel(packPtr->GetPayload().profDataPathConfig.kernelName)) {
        return "\n";
    }
    auto processIter = processMap_.find(config.deviceId);
    if (processIter == processMap_.end()) {
        std::shared_ptr<DeviceProcess> processPtr = std::make_shared<DeviceProcess>(config.deviceId);
        processMap_[config.deviceId] = processPtr;
        processIter = processMap_.find(config.deviceId);
    }
    std::string path = Utility::JoinPath(
        {profConfig_.outputPath_, processIter->second->GetOutputPath(config.kernelName)});
    profedKernels_[key]++;
    return path;
}

std::string DeviceProcessManager::ProcessCtrl(const std::shared_ptr<Packet> &packPtr, size_t clientId)
{
    ProcessCtrl::Rsp rep{};
    rep.termination = 0;
    int32_t deviceId = packPtr->GetPayload().processCtrlReq.deviceId;
    std::string key = std::to_string(clientId) + "_" + std::to_string(deviceId);
    if (packPtr->GetPayload().processCtrlReq.done > 0 && profedKernels_[key] >= profConfig_.profMaxTimes_) {
        rep.termination = 1;
        Utility::LogDebug("Process will be killed, client id is %zu", clientId);
    }
    return Communication::Serialize<ProcessCtrl::Rsp>(rep);
}

bool DeviceProcessManager::CheckKernel(const std::string &kernelName) {
    if (kernelNameSet_.empty()) {
        return true;
    }
    // do nothing when kernelName is empty
    if (kernelName.empty()) {
        return false;
    }
    for (const auto &kernel : kernelNameSet_) {
        if (Utility::StringMatch(kernelName, kernel)) {
            return true;
        }
    }
    Utility::LogInfo("Kernel %s skipped: not selected via --kernel-name.", kernelName.c_str());
    return false;
}

void DeviceProcessManager::SetProfConfig(const Common::ProfConfig &profConfig)
{
    processMap_.clear();
    profConfig_ = profConfig;
    skipedKernels_.clear();
    profedKernels_.clear();
    kernelNameSet_.clear();
    Utility::SplitString(profConfig_.kernelName_, '|', kernelNameSet_);
}
}
