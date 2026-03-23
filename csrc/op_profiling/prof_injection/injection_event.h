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

#ifndef MSOPT_INJECTION_EVENT_H
#define MSOPT_INJECTION_EVENT_H
#include <set>
#include <atomic>
#include <future>
#include "communication/communication.h"
#include "device_process_manager.h"
#include "data_format.h"
#include "packet.h"
#include "singleton.h"
namespace ProfStub {

using PacketProcessFunc = std::function<std::string(const std::shared_ptr<Packet>&, size_t)>;
struct PacketHandler {
    ProfPacketType packetType;
    PacketProcessFunc processFunc;
};

class InjectionEvent : public Singleton<InjectionEvent> {
friend class Singleton<InjectionEvent>;
public:
    // Start 1 thread to receive message
    bool StartDisposeClientAsk(const MessageOfProfConfig &profMessage,
                               const Utility::ProfConfig &profConfig);
    void Stop();
    void RegisterPacketHandler(const PacketHandler &handler);
    void UnregisterPacketHandler(ProfPacketType packetType);

private:
    InjectionEvent();
    ~InjectionEvent() override
    {
        Stop();
    }
    void DisposeAsk();
    std::string ProcessConfigDataEvent(const std::shared_ptr<Packet>& packPtr, size_t clientId) const;
    void Distribute(size_t clientId, const std::string &msg);

    std::thread listenThread_;
    std::atomic<bool> isRunning_ {true};
    MessageOfProfConfig profMessage_;
    std::unique_ptr<Communication::Communication> communication_;
    DeviceProcessManager deviceProcessManager_;
    std::unordered_map<uint32_t, std::shared_ptr<Packet>> packetMap_;
    std::unordered_map<ProfPacketType, PacketProcessFunc> msgProcessFunc_;
};
}
#endif // MSOPT_INJECTION_EVENT_H
