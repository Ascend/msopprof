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

#include "communication/communication.h"

TEST(Interface, test_send_msg_asyn)
{
    GlobalMockObject::verify();
    MOCKER(&std::condition_variable::notify_one)
        .expects(once())
        .will(ignoreReturnValue());
    Communication::Communication communication;
    std::string testStr = "test";
    communication.SendMsgAsyn(0, testStr);
    GlobalMockObject::verify();
}

TEST(Interface, test_receive_msg_success)
{
    Communication::Message message;
    message.msg = "testStr";
    message.clientId = 1;
    GlobalMockObject::verify();
    typedef std::queue<Communication::Message> QueueMessage;
    MOCKER(&QueueMessage::empty)
        .stubs()
        .will(returnValue(false));
    MOCKER(&QueueMessage::pop)
            .stubs()
            .will(ignoreReturnValue());
    MOCKER_CPP(&QueueMessage::front, QueueMessage::reference(QueueMessage::*)())
        .stubs()
        .will(returnValue(message));
    Communication::Communication communication;
    std::string testStr = "test";
    size_t clientId = 0;
    ASSERT_EQ(communication.ReceiveMsg(clientId, testStr), message.msg.size());
    ASSERT_EQ(testStr, message.msg);
    ASSERT_EQ(clientId, message.clientId);
}
