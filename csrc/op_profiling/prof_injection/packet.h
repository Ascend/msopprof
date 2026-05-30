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


#ifndef MSOPT_PACKET_H
#define MSOPT_PACKET_H
#include <cstdint>
#include <functional>
#include "communication/serializer.h"
#include "log.h"
#include "common/defs.h"
#include "include/opprof/Protocol.h"

namespace ProfStub {
enum class PacketParseRet : uint8_t {
    PARSING,
    SUCCESS,
    FAILED,
    INVALID,
};
class Packet {
public:
    explicit Packet(std::size_t clientId);
    union Payload {
        ProfDataPathConfig profDataPathConfig;
        ProcessCtrl::Req processCtrlReq;
        Common::DvcInstrLog dvcInstrLog;
        Common::DvciCacheLog dvcIcacheLog;
        Common::DvcMteLog dvcMteLog;
        Common::DvcCcuLog dvcCcuLog;
        CollectLogStart collectLogStart;
    };
    PacketParseRet ProcessMsg(const std::string &msg, size_t &idx);
    inline const Payload& GetPayload() const { return payload_; }
    std::string &GetAskMsg() { return askMsg_; } // only used for DBI task
    inline std::size_t GetClientId() const { return clientId_; }
    inline ProfPacketType GetType() const { return head_.type; }
private:
    PacketParseRet InitConfigAsk() const { return PacketParseRet::SUCCESS; }
    PacketParseRet ProfPathAsk();
    PacketParseRet ProcessCtrlAsk();
    PacketParseRet ProcessDBIData();
    PacketParseRet ProcessInstrData();
    PacketParseRet ProcessMteData();
    PacketParseRet ProcessICacheData();
    PacketParseRet ProcessCcuData();
    PacketParseRet ProcessCollectStartMessage();
    bool IsPacketHeadValid() const;

    Payload payload_{};
    std::string askMsg_ {};
    std::size_t clientId_ {UINT64_MAX};
    ProfPacketHead head_{ProfPacketType::INVALID, 0};
    // 每种消息的处理函数
    std::unordered_map<ProfPacketType, std::function<PacketParseRet(void)>> msgParseFunc_;
    // 每种消息会接收的最大报文长度（不含ProfPacketHead）
    std::unordered_map<ProfPacketType, uint32_t> msgMaxLength_;
};
}
#endif // MSOPT_PACKET_H
