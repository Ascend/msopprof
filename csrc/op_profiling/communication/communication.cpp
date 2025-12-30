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


#include "communication.h"
#include <string>
#include "log.h"
namespace Communication {
void Communication::SendMsgAsyn(size_t clientId, const std::string &msg)
{
    std::unique_lock<std::mutex> lk(sendMtx_);
    sendMsgQueue_.push({msg, clientId});
    sendCv_.notify_one();
}

int32_t Communication::ReceiveMsg(size_t &clientId, std::string &msg)
{
    // timeout is 1s
    std::unique_lock<std::mutex> lk(receiveMtx_);
    if (receiveMsgQueue_.empty()) {
        if (!receiveCv_.wait_for(lk, std::chrono::seconds(1), [this] { return !receiveMsgQueue_.empty(); })) {
            return 0;
        }
    }
    auto message = receiveMsgQueue_.front();
    clientId = message.clientId;
    msg = message.msg;
    receiveMsgQueue_.pop();
    return msg.size();
}

void Communication::MsgHandle(size_t &clientId, std::string &msg)
{
    std::unique_lock<std::mutex> lk(receiveMtx_);
    receiveMsgQueue_.push({msg, clientId});
    receiveCv_.notify_one();
}

void Communication::MsgSend()
{
    while (runFlag_) {
        Message msg;
        {
            std::unique_lock<std::mutex> lk(sendMtx_);
            if (!sendCv_.wait_for(lk, std::chrono::seconds(1), [this] { return !sendMsgQueue_.empty(); })) {
                continue;
            }
            msg = sendMsgQueue_.front();
            sendMsgQueue_.pop();
            if (Write(msg.clientId, msg.msg) == 0) {
                Utility::LogDebug("msg send failed, client is %zu", msg.clientId);
            }
        }
    }
}

int Communication::Write(size_t clientId, const std::string &msg)
{
    return server_.Notify(clientId, msg);
}

void Communication::Start()
{
    runFlag_ = true;
    sendThread_ = std::thread(&Communication::MsgSend, this);
    auto func = std::bind(&Communication::MsgHandle, this, std::placeholders::_1, std::placeholders::_2);
    server_.SetMsgHandlerHook(func);
    server_.Start();
}

void Communication::Stop()
{
    runFlag_ = false;
    sendCv_.notify_all();
    receiveCv_.notify_all();
    if (sendThread_.joinable()) {
        sendThread_.join();
    }
}
}
