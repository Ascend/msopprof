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


#include <gtest/gtest.h>
#include "mockcpp/mockcpp.hpp"
#define private public
#define protected public
#include "prof_injection/injection_event.h"
#undef private
#undef protected
#include "communication/serializer.h"
#include "communication/communication.h"
#include "prof_injection/packet.h"
using namespace ProfStub;

TEST(InjectionEvent, test_request_process_config_data_event)
{
    InjectionEvent injectionEvent;
    // set isSimulator to true, amd check this value in the end
    injectionEvent.profMessage_.isSimulator = true;
    std::string packet = injectionEvent.ProcessConfigDataEvent(nullptr, 10);
    Utility::MessageOfProfConfig profMessage;
    Communication::Deserialize(packet, profMessage);
    EXPECT_EQ(profMessage.isSimulator, true);
}

TEST(InjectionEvent, test_client_request_with_receive_msg_failed)
{
    GlobalMockObject::verify();
    using Communication::Communication;
    InjectionEvent injectionEvent;
    MOCKER(&Communication::ReceiveMsg)
            .stubs()
            .will(returnValue(-1));
    injectionEvent.DisposeAsk();
    EXPECT_EQ(injectionEvent.packetMap_.size(), 0);
    GlobalMockObject::verify();
}

int32_t ReceiveMsgStub(size_t &clientId, std::string &msg)
{
    clientId = 0;
    msg = "1";
    return 1;
}

PacketParseRet ProcessMsgStub(const std::string &msg, size_t &idx)
{
    idx = msg.length();
    return PacketParseRet::FAILED;
}

TEST(InjectionEvent, test_client_request_with_receive_msg_success_but_process_msg_failed)
{
    GlobalMockObject::verify();
    using Communication::Communication;
    InjectionEvent injectionEvent;

    MOCKER(&Communication::ReceiveMsg)
            .stubs()
            .will(invoke(ReceiveMsgStub))
            .then(returnValue(-1));
    MOCKER(&Packet::ProcessMsg)
            .stubs()
            .will(invoke(ProcessMsgStub));
    injectionEvent.DisposeAsk();
    EXPECT_EQ(injectionEvent.packetMap_.size(), 0);
    GlobalMockObject::verify();
}

TEST(InjectionEvent, test_client_request_with_receive_msg_success_and_process_msg_success)
{
    GlobalMockObject::verify();
    InjectionEvent injectionEvent;

    ProfPacketType profPacketType = ProfPacketType::CONFIG;
    std::string packet = Communication::Serialize(profPacketType);
    size_t cId = 10;
    MOCKER(&Communication::Communication::ReceiveMsg)
            .stubs()
            .with(outBound(cId), outBound(packet))
            .will(returnValue(1))
            .then(returnValue(-1));
    MOCKER(&Communication::Communication::SendMsgAsyn)
            .stubs();
    injectionEvent.DisposeAsk();
    EXPECT_EQ(injectionEvent.packetMap_.size(), 1);
    GlobalMockObject::verify();
}

TEST(InjectionEvent, test_client_request_with_receive_msg_success_but_msg_type_error)
{
    GlobalMockObject::verify();
    InjectionEvent injectionEvent;

    // invalid packet type 100
    ProfPacketType profPacketType = static_cast<ProfPacketType>(100);
    std::string packet = Communication::Serialize(profPacketType);
    size_t cId = 10;
    MOCKER(&Communication::Communication::ReceiveMsg)
            .stubs()
            .with(outBound(cId), outBound(packet))
            .will(returnValue(1))
            .then(returnValue(-1));
    MOCKER(&Communication::Communication::SendMsgAsyn)
            .stubs();
    injectionEvent.DisposeAsk();
    EXPECT_EQ(injectionEvent.packetMap_.size(), 1);
    GlobalMockObject::verify();
}
