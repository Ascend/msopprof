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

#ifndef THIRD_PARTY_PROFAPI_H
#define THIRD_PARTY_PROFAPI_H

struct MsprofAdditionalInfo {
    uint16_t magicNumber;
    uint16_t level;
    uint32_t type;
    uint32_t threadId;
    uint32_t dataLen;
    uint64_t timeStamp;
    uint8_t data[232];
};

#pragma pack(1)
struct MsprofAicpuHcclTaskInfo {
    uint64_t itemId;
    uint64_t cclTag;
    uint64_t groupName;
    uint32_t localRank;
    uint32_t remoteRank;
    uint32_t rankSize;
    uint32_t stage;
    uint64_t notifyID;
    uint64_t timeStamp;
    double durationEstimated;
    uint64_t srcAddr;
    uint64_t dstAddr;
    uint64_t dataSize; // bytes
    uint32_t taskId;
    uint32_t reserve;
    uint16_t streamId;
    uint16_t planeID;
    uint8_t opType; // {0: sum, 1: mul, 2: max, 3: min}
    uint8_t dataType; // data type {0: INT8, 1: INT16, 2: INT32, 3: FP16, 4:FP32, 5:INT64, 6:UINT64}
    uint8_t linkType; // link type {0: 'OnChip', 1: 'HCCS', 2: 'PCIe', 3: 'RoCE'}
    uint8_t transportType; // transport type {0: SDMA, 1: RDMA, 2:LOCAL}
    uint8_t rdmaType; // RDMA type {0: RDMASendNotify, 1:RDMASendPayload}
    uint8_t role; // role {0: dst, 1:src}
    uint8_t workFlowMode;
    uint8_t reserves[9];
};
#pragma pack()

struct MsprofAicTimeStampInfo {
    uint64_t syscyc;  // dotting timestamp with system cycle
    uint32_t blockId; // core block id
    uint32_t descId;  // dot Id for description
    uint64_t curPc;   // currrent pc for source line
};

struct MsprofAicTimeStampInfoUpdate {
    MsprofAicTimeStampInfoUpdate() {};
    MsprofAicTimeStampInfoUpdate(const MsprofAicTimeStampInfo &info, const std::string &type) {
        this->syscyc = info.syscyc;
        this->blockId = info.blockId;
        this->descId = info.descId;
        this->curPc = info.curPc;
        this->type = type;
    }
    uint64_t syscyc;
    uint32_t blockId; // logic core id
    uint32_t descId;
    uint64_t curPc;
    std::string type;
};

struct AicpuKfcProfCommTurn {
    uint64_t waitNotifyStartTime;  // 开始等待通信参数
    uint64_t kfcAlgExeStartTime;   // 开始通信算法执行
    uint64_t sendTaskStartTime;    // 开始下发task
    uint64_t waitActiveStartTime;  // 开始等待激活
    uint64_t activeStartTime;      // 开始激活处理
    uint64_t waitExeEndStartTime;  // 开始等待任务执行结束
    uint64_t rtsqExeEndTime;       // 任务执行结束时间
    uint64_t dataLen;              // 本轮通信数据长度
    uint32_t deviceId;
    uint16_t streamId;
    uint16_t taskId;
    uint8_t version;
    uint8_t commTurn;              // 总通信轮次
    uint8_t currentTurn;
    uint8_t reserve[5];
};

struct LcclDumpLogInfo {
    uint32_t logId;
    uint32_t blockId;
    uint64_t syscyc;
    uint64_t curPc;
    uint32_t operationType;
    uint32_t rsv;
};
#endif // THIRD_PARTY_PROFAPI_H
