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


#include "packet.h"
#include <climits>
#include "common/dbi_defs.h"
#include "securec.h"
#include "log.h"
#include "number_operation.h"

namespace ProfStub {
using namespace Communication;
using namespace Utility;
using namespace Common;
Packet::Packet(std::size_t clientId) : clientId_(clientId)
{
    msgParseFunc_.emplace(ProfPacketType::CONFIG, std::bind(&Packet::InitConfigAsk, this));
    msgParseFunc_.emplace(ProfPacketType::DATA_PATH, std::bind(&Packet::ProfPathAsk, this));
    msgParseFunc_.emplace(ProfPacketType::PROCESS_CTRL, std::bind(&Packet::ProcessCtrlAsk, this));
    msgParseFunc_.emplace(ProfPacketType::DBI_DATA, std::bind(&Packet::ProcessDBIData, this));
    msgParseFunc_.emplace(ProfPacketType::INSTR_LOG, std::bind(&Packet::ProcessInstrData, this));
    msgParseFunc_.emplace(ProfPacketType::POPPED_LOG, std::bind(&Packet::ProcessInstrData, this));
    msgParseFunc_.emplace(ProfPacketType::ICACHE_LOG, std::bind(&Packet::ProcessICacheData, this));
    msgParseFunc_.emplace(ProfPacketType::MTE_LOG, std::bind(&Packet::ProcessMteData, this));
    msgParseFunc_.emplace(ProfPacketType::COLLECT_START, std::bind(&Packet::ProcessCollectStartMessage, this));
    msgParseFunc_.emplace(ProfPacketType::PROF_FINISH, std::bind(&Packet::InitConfigAsk, this));

    msgMaxLength_.emplace(ProfPacketType::CONFIG, 0);
    msgMaxLength_.emplace(ProfPacketType::DATA_PATH, sizeof(ProfDataPathConfig));
    msgMaxLength_.emplace(ProfPacketType::PROCESS_CTRL, sizeof(ProcessCtrl::Req));
    msgMaxLength_.emplace(ProfPacketType::DBI_DATA, sizeof(DBIDataHeader) + MAX_BLOCK_DATA_SIZE);
    msgMaxLength_.emplace(ProfPacketType::INSTR_LOG, sizeof(DvcInstrLog));
    msgMaxLength_.emplace(ProfPacketType::POPPED_LOG, sizeof(DvcInstrLog));
    msgMaxLength_.emplace(ProfPacketType::ICACHE_LOG, sizeof(DvciCacheLog));
    msgMaxLength_.emplace(ProfPacketType::MTE_LOG, sizeof(DvcMteLog));
    msgMaxLength_.emplace(ProfPacketType::COLLECT_START, sizeof(CollectLogStart));
    msgMaxLength_.emplace(ProfPacketType::PROF_FINISH, 0);
}

PacketParseRet Packet::ProfPathAsk()
{
    if (askMsg_.length() != sizeof(ProfDataPathConfig)) {
        LogDebug("Packet for DATA_PATH invalid, expect length: %lu, actual length: %lu.",
                 sizeof(ProfDataPathConfig), askMsg_.length());
        return PacketParseRet::FAILED;
    }
    Deserialize<ProfDataPathConfig>(askMsg_, payload_.profDataPathConfig);
    return PacketParseRet::SUCCESS;
}

PacketParseRet Packet::ProcessCtrlAsk()
{
    if (askMsg_.size() == sizeof(ProcessCtrl::Req)) {
        Deserialize<ProcessCtrl::Req>(askMsg_, payload_.processCtrlReq);
        return PacketParseRet::SUCCESS;
    }
    LogDebug("Packet for PROCESS_CTRL invalid, expect length: %lu, actual length: %lu.",
             sizeof(ProcessCtrl::Req), askMsg_.length());
    return PacketParseRet::FAILED;
}

PacketParseRet Packet::ProcessDBIData()
{
    DBIDataHeader dbiDataHeader{};
    Deserialize(askMsg_, dbiDataHeader);
    constexpr uint64_t maxLength = MAX_BLOCK_DATA_SIZE - sizeof(BlockHeader) > PATH_MAX ?
            MAX_BLOCK_DATA_SIZE - sizeof(BlockHeader) : PATH_MAX;
    if (dbiDataHeader.length == 0 || dbiDataHeader.length > maxLength) {
        LogDebug("Drop invalid DBI data, data length: %lu.", dbiDataHeader.length);
        return PacketParseRet::FAILED;
    }
    uint64_t targetLength = sizeof(DBIDataHeader) + dbiDataHeader.length;
    if (targetLength != askMsg_.length()) {
        LogDebug("Drop long DBI data, expect length: %lu, actual length: %lu.", targetLength, askMsg_.length());
        return PacketParseRet::FAILED;
    }
    return PacketParseRet::SUCCESS;
}

PacketParseRet Packet::ProcessMsg(const std::string &msg, size_t &idx)
{
    static const std::string LOCATION = "PacketProcessMsg";
    if (head_.type == ProfPacketType::INVALID) {
        if (SafeSub(msg.length(), idx, LOCATION) == std::numeric_limits<size_t>::max()) {
            Utility::LogWarn("Set packet head failed, msg length: %lu, idx: %lu.", msg.length(), idx);
            idx = msg.length();
            return PacketParseRet::FAILED;
        }
        size_t headSize = SafeAdd(askMsg_.length(), (msg.length() - idx), LOCATION, false);
        if (headSize < sizeof(ProfPacketHead)) {
            askMsg_ += msg.substr(idx);
            idx = msg.length();
            return PacketParseRet::PARSING;
        }
        size_t length = SafeSub(sizeof(ProfPacketHead), askMsg_.length(), LOCATION);
        if (length == std::numeric_limits<size_t>::max()) {
            Utility::LogWarn("Set packet head failed, msg length: %lu.", askMsg_.length());
            idx = msg.length();
            return PacketParseRet::FAILED;
        }
        askMsg_ += msg.substr(idx, length);
        Deserialize(askMsg_, head_);
        askMsg_.clear();
        idx += length;
        if (head_.type < ProfPacketType::INSTR_LOG) {
            Utility::LogDebug("Message get, type: %u, msg length: %u.",
                              static_cast<uint32_t>(head_.type), head_.length);
        }
        if (!IsPacketHeadValid()) {
            // 运行时不应该走入；一般客户端一次只会发送一个packet，非法情况全部丢弃对后续流程的影响比较小
            idx = msg.length();
            return PacketParseRet::FAILED;
        }
    }
    size_t leftLength = SafeSub(static_cast<size_t>(head_.length), askMsg_.length(), LOCATION);
    if (leftLength == std::numeric_limits<size_t>::max()) {
        Utility::LogWarn("Append packet payload failed, askMsg:%lu should not longer than "
                         "packet_length:%u at this time.", askMsg_.length(), head_.length);
        idx = msg.length();
        return PacketParseRet::FAILED;
    }
    if (leftLength > msg.length() - idx) {
        askMsg_ += msg.substr(idx);
        idx = msg.length();
        return PacketParseRet::PARSING;
    }
    askMsg_ += msg.substr(idx, leftLength);
    idx += leftLength;
    return msgParseFunc_[head_.type]();
}

bool Packet::IsPacketHeadValid() const
{
    if (msgParseFunc_.count(head_.type) == 0) {
        Utility::LogWarn("Can not find process function, type is %u.", static_cast<uint32_t>(head_.type));
        return false;
    }
    // 当前会接收的消息类型的最大长度
    if (head_.length > msgMaxLength_.at(head_.type)) {
        Utility::LogWarn("Message length is too long: %u.", head_.length);
        return false;
    }
    return true;
}

PacketParseRet Packet::ProcessInstrData()
{
    if (askMsg_.size() == sizeof(DvcInstrLog)) {
        Deserialize<struct DvcInstrLog>(askMsg_, payload_.dvcInstrLog);
        return PacketParseRet::SUCCESS;
    }
    LogDebug("Packet for instr data invalid, expect length: %lu, actual length: %lu.",
             sizeof(DvcInstrLog), askMsg_.length());
    return PacketParseRet::FAILED;
}

PacketParseRet Packet::ProcessCollectStartMessage()
{
    if (askMsg_.size() == sizeof(CollectLogStart)) {
        Deserialize<CollectLogStart>(askMsg_, payload_.collectLogStart);
        return PacketParseRet::SUCCESS;
    }
    LogDebug("Packet for collect start invalid");
    return PacketParseRet::FAILED;
}

PacketParseRet Packet::ProcessMteData()
{
    if (askMsg_.size() == sizeof(DvcMteLog)) {
        Deserialize<struct DvcMteLog>(askMsg_, payload_.dvcMteLog);
        return PacketParseRet::SUCCESS;
    }
    LogDebug("Packet for mte data invalid, expect length: %lu, actual length: %lu.",
             sizeof(DvcMteLog), askMsg_.length());
    return PacketParseRet::FAILED;
}

PacketParseRet Packet::ProcessICacheData()
{
    if (askMsg_.size() == sizeof(DvciCacheLog)) {
        Deserialize<struct DvciCacheLog>(askMsg_, payload_.dvcIcacheLog);
        return PacketParseRet::SUCCESS;
    }
    LogDebug("Packet for icache data invalid, expect length: %lu, actual length: %lu.",
             sizeof(DvciCacheLog), askMsg_.length());
    return PacketParseRet::FAILED;
}
}