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


#include "injection_event.h"
#include <thread>
#include "log.h"
#include "smart_pointer.h"
namespace ProfStub {
using namespace Utility;
bool InjectionEvent::StartDisposeClientAsk(const Utility::MessageOfProfConfig &profMessage,
                                           const Utility::ProfConfig &profConfig)
{
    isRunning_ = true;
    profMessage_ = profMessage;
    deviceProcessManager_.SetProfConfig(profConfig);
    if (communication_) {
        Utility::LogWarn("Starting a new client when last is not released yet,"
                         " data collection may results problematic.");
        communication_.reset();
    }
    communication_ = Utility::MakeUnique<Communication::Communication>();
    if (!communication_) {
        isRunning_ = false;
        return false;
    }
    communication_->Start();
    listenThread_ = std::thread(&InjectionEvent::DisposeAsk, this);
    return true;
}

void InjectionEvent::Stop()
{
    isRunning_ = false;
    if (listenThread_.joinable()) {
        listenThread_.join();
    }
    communication_.reset();
}

std::string InjectionEvent::ProcessConfigDataEvent(const std::shared_ptr<Packet>& packPtr, size_t clientId)
{
    (void)packPtr;
    (void)clientId;
    return Communication::Serialize(profMessage_);
}

void InjectionEvent::DisposeAsk()
{
    size_t clientId;
    std::string msg;
    while (isRunning_) {
        auto msgLen = communication_->ReceiveMsg(clientId, msg);
        if (msgLen == 0) {
            continue;
        } else if (msgLen < 0) {
            LogWarn("msg read failed, client is %zu", clientId);
            break;
        }
        Distribute(clientId, msg);
        msg.clear();
    }
    LogDebug("Client listening will return");
}

void InjectionEvent::Distribute(size_t clientId, const std::string &msg)
{
    size_t idx = 0;
    while (idx < msg.size()) {
        if (packetMap_[clientId] == nullptr) {
            packetMap_[clientId] = Utility::MakeShared<Packet>(clientId);
            if (!packetMap_[clientId]) {
                LogDebug("Get packet failed because of nullptr, client is %zu", clientId);
                continue;
            }
        }
        PacketParseRet ret = packetMap_[clientId]->ProcessMsg(msg, idx);
        if (ret == PacketParseRet::FAILED) {
            LogWarn("msg parse failed, client is %zu", clientId);
            packetMap_[clientId] = nullptr;
            continue;
        }
        if (ret == PacketParseRet::SUCCESS) {
            auto type = packetMap_[clientId]->GetType();
            if (msgProcessFunc_.find(type) == msgProcessFunc_.end()) {
                LogWarn("cannot find packet callback, client: %zu, type: %u", clientId, static_cast<uint32_t>(type));
                continue;
            }
            std::string sendMsg = msgProcessFunc_[type](packetMap_[clientId], clientId);
            if (!sendMsg.empty()) {
                communication_->SendMsgAsyn(clientId, sendMsg);
            }
            packetMap_[clientId] = nullptr;
        }
    }
}

InjectionEvent::InjectionEvent()
{
    msgProcessFunc_.insert({ProfPacketType::DATA_PATH, std::bind(&DeviceProcessManager::ProcessKernelEvent,
        &deviceProcessManager_, std::placeholders::_1, std::placeholders::_2)});
    msgProcessFunc_.insert({ProfPacketType::PROCESS_CTRL, std::bind(&DeviceProcessManager::ProcessCtrl,
        &deviceProcessManager_, std::placeholders::_1, std::placeholders::_2)});
    msgProcessFunc_.insert({ProfPacketType::CONFIG,
        std::bind(&InjectionEvent::ProcessConfigDataEvent, this, std::placeholders::_1, std::placeholders::_2)});
}

void InjectionEvent::RegisterPacketHandler(const PacketHandler &handler)
{
    if (!msgProcessFunc_.emplace(handler.packetType, handler.processFunc).second) {
        LogWarn("add packet processor failed, existence is %u.",
                msgProcessFunc_.count(handler.packetType));
    }
}

void InjectionEvent::UnregisterPacketHandler(ProfPacketType packetType)
{
    msgProcessFunc_.erase(packetType);
}
}