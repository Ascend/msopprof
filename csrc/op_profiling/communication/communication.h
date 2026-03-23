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

#ifndef MSOPT_COMMUNICATION_H
#define MSOPT_COMMUNICATION_H
#include <string>
#include <functional>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include "core/RemoteProcess.h"
namespace Communication {
struct Message {
    std::string msg;
    size_t clientId;
};
class Communication {
public:
    void Start();
    void Stop();
    void SendMsgAsyn(size_t clientId, const std::string &msg); // join the message to queue
    int32_t ReceiveMsg(size_t &clientId, std::string &msg);
    Communication() : server_(RemoteProcess(CommType::SOCKET)) {}
    ~Communication()
    {
        Stop();
    }
private:
    void MsgSend(); // set message in the queue
    int Write(size_t clientId, const std::string &msg) const;
    void MsgHandle(size_t &clientId, std::string &msg);

    RemoteProcess server_;
    std::queue<Message> sendMsgQueue_;
    std::mutex sendMtx_;
    std::condition_variable sendCv_;
    // for sending
    std::thread sendThread_;

    std::queue<Message> receiveMsgQueue_;
    std::mutex receiveMtx_;
    std::condition_variable receiveCv_;
    bool runFlag_ { false };
};
}
#endif // MSOPT_COMMUNICATION_H
