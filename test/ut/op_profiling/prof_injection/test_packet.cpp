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
#include <climits>

#define private public
#define protected public
#include "prof_injection/packet.h"
#undef private
#undef protected
#include "common/dbi_defs.h"
using namespace ProfStub;
using namespace std;

TEST(Packet, test_ProcessMsg_with_error_type_expect_failed)
{
    Packet packet(0);
    string msg = Communication::Serialize(ProfPacketHead{ProfPacketType::INVALID, 0});
    size_t idx = 0;
    auto res = packet.ProcessMsg(msg, idx);
    ASSERT_EQ(res, PacketParseRet::FAILED);
}

TEST(Packet, test_ProcessMsg_DATA_PATH_with_long_length_expect_failed)
{
    Packet packet(0);
    string msg = Communication::Serialize(ProfPacketHead{ProfPacketType::DATA_PATH, PATH_MAX});
    size_t idx = 0;
    auto res = packet.ProcessMsg(msg + string(PATH_MAX, '1'), idx);
    ASSERT_EQ(res, PacketParseRet::FAILED);
}

TEST(Packet, test_ProcessMsg_PROCESS_CTRL_with_long_length_expect_failed)
{
    Packet packet(0);
    string msg = Communication::Serialize(ProfPacketHead{ProfPacketType::PROCESS_CTRL, PATH_MAX});
    size_t idx = 0;
    auto res = packet.ProcessMsg(msg + string(PATH_MAX, '1'), idx);
    ASSERT_EQ(res, PacketParseRet::FAILED);
}

TEST(Packet, test_ProcessMsg_DBI_DATA_with_invalid_length_expect_failed)
{
    Packet packet(0);
    uint32_t len = PATH_MAX * 2;
    string msg = Communication::Serialize(ProfPacketHead{ProfPacketType::DBI_DATA, len});
    size_t idx = 0;
    auto res = packet.ProcessMsg(msg + string(len, '1'), idx);
    ASSERT_EQ(res, PacketParseRet::FAILED);
}

TEST(Packet, test_ProcessMsg_CONFIG_with_long_length_expect_parsing)
{
    Packet packet(0);
    string msg = Communication::Serialize(ProfPacketHead{ProfPacketType::CONFIG, PATH_MAX});
    size_t idx = 0;
    auto res = packet.ProcessMsg(msg, idx);
    ASSERT_EQ(res, PacketParseRet::FAILED);
}

TEST(Packet, test_ProcessMsg_with_config_type_expect_success)
{
    Packet packet(0);
    size_t idx = 0;
    auto res = packet.ProcessMsg(Communication::Serialize(ProfPacketHead{ProfPacketType::CONFIG}), idx);
    ASSERT_EQ(res, PacketParseRet::SUCCESS);
    ASSERT_EQ(packet.head_.type, ProfPacketType::CONFIG);
}

TEST(Packet, test_ProcessMsg_with_data_path_type)
{
    ProfDataPathConfig config{};
    ProfPacketHead head{ProfPacketType::DATA_PATH, sizeof(ProfDataPathConfig)};
    string msg = Communication::Serialize(head, config);
    size_t idx = 0;
    Packet packet(0);
    auto res = packet.ProcessMsg(msg, idx);
    ASSERT_EQ(res, PacketParseRet::SUCCESS);
}

TEST(Packet, test_ProcessMsg_with_process_ctrl_type)
{
    Packet packet(0);
    // ProcessCtrl::Req for PROCESS_CTRL
    ProcessCtrl::Req req{};
    ProfPacketHead head{ProfPacketType::PROCESS_CTRL, sizeof(ProcessCtrl::Req)};
    string msg = Communication::Serialize(head, req);
    size_t idx = 0;
    auto res = packet.ProcessMsg(msg, idx);
    ASSERT_EQ(res, PacketParseRet::SUCCESS);
}

TEST(Packet, test_ProcessDBIData_expect_return_success)
{
    Packet packet(0);
    DBIDataHeader dataHeader{};
    dataHeader.length = 1;
    ProfPacketHead head{ProfPacketType::DBI_DATA, sizeof(DBIDataHeader) + 1};
    string msg = Communication::Serialize(head, dataHeader) + "/";
    size_t idx = 0;
    auto res = packet.ProcessMsg(msg, idx);
    ASSERT_EQ(res, PacketParseRet::SUCCESS);
}

TEST(Packet, test_ProfPathAskexpect_return_false)
{
    Packet packet(0);
    DBIDataHeader dataHeader{};
    dataHeader.length = 1;
    ProfPacketHead head{ProfPacketType::DBI_DATA, sizeof(DBIDataHeader) + 1};
    string msg = Communication::Serialize(head, dataHeader) + "/";
    packet.askMsg_ = msg;
    auto res = packet.ProfPathAsk();
    ASSERT_EQ(res, PacketParseRet::FAILED);
}

TEST(Packet, test_Processreturn_false)
{
    Packet packet(0);
    packet.askMsg_ = "";
    auto res = packet.ProcessCtrlAsk();
    ASSERT_EQ(res, PacketParseRet::FAILED);
    res = packet.ProfPathAsk();
    ASSERT_EQ(res, PacketParseRet::FAILED);
    res = packet.ProcessInstrData();
    ASSERT_EQ(res, PacketParseRet::FAILED);
    res = packet.ProcessCollectStartMessage();
    ASSERT_EQ(res, PacketParseRet::FAILED);
    res = packet.ProcessMteData();
    ASSERT_EQ(res, PacketParseRet::FAILED);
    res = packet.ProcessICacheData();
    ASSERT_EQ(res, PacketParseRet::FAILED);
}

TEST(Packet, test_ProcessInstrData_return_success)
{
    Packet packet(0);
    Common::DvcInstrLog dataHeader {};
    string msg = Communication::Serialize(dataHeader);
    packet.askMsg_ = msg;
    auto res = packet.ProcessInstrData();
    ASSERT_EQ(res, PacketParseRet::SUCCESS);
}

TEST(Packet, test_ProcessCollectStartMessage_return_success)
{
    Packet packet(0);
    CollectLogStart dataHeader{};
    string msg = Communication::Serialize(dataHeader);
    packet.askMsg_ = msg;
    auto res = packet.ProcessCollectStartMessage();
    ASSERT_EQ(res, PacketParseRet::SUCCESS);
}

TEST(Packet, test_ProcessMteData_return_success)
{
    Packet packet(0);
    Common::DvcMteLog dataHeader{};
    string msg = Communication::Serialize(dataHeader);
    packet.askMsg_ = msg;
    auto res = packet.ProcessMteData();
    ASSERT_EQ(res, PacketParseRet::SUCCESS);
}

TEST(Packet, test_ProcessICacheData_return_success)
{
    Packet packet(0);
    Common::DvciCacheLog dataHeader{};
    string msg = Communication::Serialize(dataHeader);
    packet.askMsg_ = msg;
    auto res = packet.ProcessICacheData();
    ASSERT_EQ(res, PacketParseRet::SUCCESS);
}
