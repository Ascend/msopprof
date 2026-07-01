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


#ifndef MSOPT_DEVICE_PROCESS_MANAGER_H
#define MSOPT_DEVICE_PROCESS_MANAGER_H
#include <map>
#include <memory>
#include <set>
#include "packet.h"
#include "filesystem.h"
namespace ProfStub {
struct DeviceProcess {
public:
    explicit DeviceProcess(uint32_t deviceId) : deviceId(deviceId) {}
    std::string GetOutputPath(const std::string &name)
    {
        auto iter = kernelToRunningTimes.find(name);
        if (iter == kernelToRunningTimes.end()) {
            kernelToRunningTimes[name] = 0;
            iter = kernelToRunningTimes.find(name);
        }
        uint16_t num = iter->second;
        iter->second++;
        std::string path = Utility::JoinPath({"device" + std::to_string(deviceId), name, std::to_string(num), "dump"});
        path += "\n";
        return path;
    }
private:
    uint32_t deviceId;
    // map<kernel name, kernel running times>
    std::map<std::string, uint16_t> kernelToRunningTimes;
};
class DeviceProcessManager {
public:
    DeviceProcessManager() = default;
    std::string ProcessKernelEvent(const std::shared_ptr<Packet>& packPtr, size_t clientId);
    std::string ProcessCtrl(const std::shared_ptr<Packet>& packPtr, size_t clientId);
    void SetProfConfig(const Common::ProfConfig &profConfig);

private:
    // map<deviceid_, Process>
    std::map<int32_t, std::shared_ptr<DeviceProcess>> processMap_;
    Common::ProfConfig profConfig_;
    // map<(deviceid, clientid), skiped kernels numbers>
    std::map<std::string, uint16_t> skipedKernels_;
    // map<(deviceid, clientid), profed kernels numbers>
    std::map<std::string, uint16_t> profedKernels_;
    std::set<std::string> kernelNameSet_;

    bool CheckKernel(const std::string &kernelName);
};
}
#endif // MSOPT_DEVICE_PROCESS_MANAGER_H
