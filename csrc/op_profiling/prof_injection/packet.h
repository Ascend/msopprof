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
#include "data_format.h"
#include "log.h"
#include "common/defs.h"
namespace ProfStub {
constexpr uint16_t PATH_MAX_LENGTH = 4096U;
constexpr uint16_t NAME_MAX_LENGTH = 1024U;
/*
 * 增加枚举时，必须同时在 Packet::Packet() 适配两个哈希表
 * 确保通信枚举和结构体定义和基础组件在 ProfConfig.h 的定义一致
 */
enum class ProfPacketType : uint32_t {
    CONFIG = 0,
    DATA_PATH,
    PROCESS_CTRL,
    DBI_DATA,
    COLLECT_START,
    PROF_FINISH,
    INSTR_LOG = 20,
    POPPED_LOG,
    ICACHE_LOG,
    MTE_LOG,
    INVALID,
};

#pragma pack(4)
struct ProfPacketHead {
    ProfPacketType type;
    uint32_t length;
};

struct ProfDataPathConfig {
    char kernelName[NAME_MAX_LENGTH];
    uint32_t deviceId;
};

struct ProcessCtrl {
    // 采集后通信请求
    struct Req {
        uint8_t done; // 是否完成采集任务
        int32_t deviceId;
    };
    // 采集后通信回复
    struct Rsp {
        uint8_t termination; // 是否提前结束进程
    };
};

struct CollectLogStart {
    char outputPath[PATH_MAX_LENGTH];
    char kernelName[NAME_MAX_LENGTH];
};

struct DBIDataHeader {
    uint64_t count;      // 该block记录的条目数
    uint64_t length;     // Header后紧跟的数据长度，也就是输出路径长度或者动态插桩数据长度
    uint64_t overflow;   // 缓冲区不足而未记录的数据条目数
    uint16_t blockId;
    uint8_t endFlag;    // 该path下所有block的数据都发送完成
};
#pragma pack()

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
        CollectLogStart collectLogStart;
    };
    PacketParseRet ProcessMsg(const std::string &msg, size_t &idx);
    inline const Payload& GetPayload() { return payload_; }
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
