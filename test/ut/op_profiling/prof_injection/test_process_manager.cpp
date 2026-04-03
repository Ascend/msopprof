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

#include "prof_injection/device_process_manager.h"

using namespace ProfStub;

TEST(DeviceProcessManager, test_ProcessKernelEvent_with_empty_packet_expect_empty)
{
    DeviceProcessManager manager;
    auto res = manager.ProcessKernelEvent(nullptr, 0);
    ASSERT_EQ(res, "\n");
}

TEST(DeviceProcessManager, test_ProcessKernelEvent_not_reach_skip_times_expect_empty)
{
    GlobalMockObject::verify();
    DeviceProcessManager manager;
    // profSkipTimes_ = 1
    Common::ProfConfig profConfig{"", "", 1, 1};
    manager.SetProfConfig(profConfig);
    Packet::Payload payload{.profDataPathConfig={"kernelName", 0}};
    MOCKER(&Packet::GetPayload)
            .stubs()
            .will(returnValue(payload));
    auto res = manager.ProcessKernelEvent(std::make_shared<Packet>(0), 0);
    ASSERT_EQ(res, "\n");
}

TEST(DeviceProcessManager, test_ProcessKernelEvent_has_reach_max_times_expect_empty)
{
    GlobalMockObject::verify();
    DeviceProcessManager manager;
    // profMaxTimes_ = 1
    Common::ProfConfig profConfig{"", "", 1, 0};
    manager.SetProfConfig(profConfig);
    Packet::Payload payload{.profDataPathConfig={"kernelName", 0}};
    MOCKER(&Packet::GetPayload)
            .stubs()
            .will(returnValue(payload));
    manager.ProcessKernelEvent(std::make_shared<Packet>(0), 0);
    auto res = manager.ProcessKernelEvent(std::make_shared<Packet>(0), 0);
    ASSERT_EQ(res, "\n");
}

TEST(DeviceProcessManager, test_ProcessKernelEvent_kernel_name_check_failed_expect_empty)
{
    GlobalMockObject::verify();
    DeviceProcessManager manager;
    // kernelName_ = "add"
    Common::ProfConfig profConfig{"", "add", 1, 0};
    manager.SetProfConfig(profConfig);
    Packet::Payload payload{.profDataPathConfig={"kernelName", 0}};
    MOCKER(&Packet::GetPayload)
            .stubs()
            .will(returnValue(payload));
    auto res = manager.ProcessKernelEvent(std::make_shared<Packet>(0), 0);
    ASSERT_EQ(res, "\n");
}

TEST(DeviceProcessManager, test_ProcessKernelEvent_need_prof_expect_path)
{
    GlobalMockObject::verify();
    DeviceProcessManager manager;
    Common::ProfConfig profConfig{"", "add", 1, 0};
    manager.SetProfConfig(profConfig);
    Packet::Payload payload{.profDataPathConfig={"add", 0}};
    MOCKER(&Packet::GetPayload)
            .stubs()
            .will(returnValue(payload));
    auto res = manager.ProcessKernelEvent(std::make_shared<Packet>(0), 0);
    ASSERT_EQ(res, "device0/add/0/dump\n");
}

TEST(DeviceProcessManager, test_ProcessCtrl_has_reach_max_times_need_kill_expect_empty)
{
    GlobalMockObject::verify();
    DeviceProcessManager manager;
    // profMaxTimes_ = 0
    Common::ProfConfig profConfig{"", "", 0, 0};
    manager.SetProfConfig(profConfig);
    Packet::Payload payload{.processCtrlReq={1, 0}};
    MOCKER(&Packet::GetPayload)
            .stubs()
            .will(returnValue(payload));
    auto res = manager.ProcessCtrl(std::make_shared<Packet>(0), 0);
    ASSERT_EQ(res, "\x1");
}