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


#ifndef MSOPT_DBIPARSER_H
#define MSOPT_DBIPARSER_H
#include <cstdint>
#include <vector>
#include <map>
#include <mutex>
#include <functional>
#include <unordered_map>
#include <atomic>
#include "common/dbi_defs.h"
#include "thread_pool.h"
#include "prof_injection/packet.h"
#include "number_operation.h"
#include "dbi_helper.h"

namespace Profiling {

constexpr uint32_t LOAD_2D_MOV_SIZE = 512U;
constexpr uint64_t MOV_LOCAL_BLOCK_SIZE = 32UL;

class DBIParser {
public:
    explicit DBIParser(std::string outputPath);
    ~DBIParser() { WaitParseTask(); }
    void ParsePacket(std::size_t clientId, std::string &&msg);
    void WaitParseTask();
    void InitializeMovRecordParserFunctions();
    void InitializeRecordParserFunctions();

private:
    enum class State {
        INITED,    // default state after constructed
        RUNNING,   // is parsing data
        FINISHED,  // all data have been parsed
    };

    struct MemoryChartMetrics {
        void PrintMetrics() const;
        void Dump() const;

        std::string outputPath;
        std::vector<Common::MemRecord> memoryRecords{};
        std::vector<uint32_t> typeProcessed = std::vector<uint32_t>(static_cast<uint32_t>(Common::RecordType::END), 0);
        std::vector<uint32_t> typeFailed = std::vector<uint32_t>(static_cast<uint32_t>(Common::RecordType::END), 0);
        uint32_t processedRecord{0};
        uint32_t droppedRecord{0};
        uint32_t memoryCopyFailed{0};
        uint32_t invalidParam{0};
        uint16_t receivedPacket{0};
        uint16_t processedPacket{0};
        uint16_t overflow{0};
        uint8_t invalidDataCount{0};
        uint8_t memoryCorruption{0};
        uint8_t invalidRecordType{0};
        uint8_t outOfBoundsRead{0};
        // 是否接收到了基础组件发送的结束标志，true表示该kernel的数据已经全部发送，丢弃后续接收到的该kernel数据
        bool endFlag{false};
    };

    struct ParseMemoryChartParams {
        const std::size_t key;
        const std::string &msg;
        std::size_t index;
        uint64_t dataCount;
        uint16_t blockId;
    };

    inline uint64_t GetDataTypeSizeInBits(Common::DataType dataType) const
    {
        static const std::map<Common::DataType, uint64_t> DATA_TYPE_SIZE_MAP = {
            {Common::DataType::DATA_B4,  4},
            {Common::DataType::DATA_B8,  8},
            {Common::DataType::DATA_B16, 16},
            {Common::DataType::DATA_B32, 32},
            {Common::DataType::DATA_F16, 16},
            {Common::DataType::DATA_BF16, 16},
            {Common::DataType::DATA_F32, 32},
            {Common::DataType::DATA_S32, 32},
        };
        auto it = DATA_TYPE_SIZE_MAP.find(dataType);
        return it == DATA_TYPE_SIZE_MAP.cend() ? 1 : it->second;
    }
    inline uint64_t GetDataTypeSizeInBytes(Common::DataType dataType) const
    {
        uint64_t res = GetDataTypeSizeInBits(dataType) / Common::BITS_EACH_BYTE;
        return res == 0 ? 1 : res;
    }
    inline void Emplace(std::size_t key, const Common::MemRecord &memRecord)
    {
        std::lock_guard<std::mutex> lock(parseMutex_);
        if (memoryChartMetrics_.find(key) == memoryChartMetrics_.end()) {
            ++procedureErrorCount_;
            return;
        }
        memoryChartMetrics_[key].memoryRecords.emplace_back(memRecord);
    }

    template<typename T>
    bool CheckRecord(T &record, const std::string &buffer, std::size_t &index, std::size_t key)
    {
        index += sizeof(T);
        if (index > buffer.size()) {
            std::lock_guard<std::mutex> lock(parseMutex_);
            ++memoryChartMetrics_[key].outOfBoundsRead;
            return false;
        } else if (memcpy_s(&record, sizeof(T), &buffer[index - sizeof(T)], sizeof(T)) != EOK) {
            std::lock_guard<std::mutex> lock(parseMutex_);
            ++memoryChartMetrics_[key].memoryCopyFailed;
            return false;
        }
        DBIHelper::Instance().PrintRecord(record);
        return true;
    }

    template<typename T>
    uint64_t CalcPadModeSize(T const &record) const
    {
        using namespace Common;
        static_assert(std::is_same<T, DmaMovRecord>::value || std::is_same<T, MovV2Record>::value,
                      "Record type doesn't support to calculate pad mode size.");
        static const std::map<PadMode, uint64_t> SCALE_MAP = {
            {PadMode::PAD_MODE1, 32},
            {PadMode::PAD_MODE2, 16},
            {PadMode::PAD_MODE3, 8},
            {PadMode::PAD_MODE4, 4},
            {PadMode::PAD_MODE5, 2},
            {PadMode::PAD_MODE6, 8},
            {PadMode::PAD_MODE7, 4},
            {PadMode::PAD_MODE8, 2}
        };
        auto it = SCALE_MAP.find(record.padMode);
        if (it != SCALE_MAP.cend()) {
            return static_cast<uint64_t>(record.nBurst) * (MOV_LOCAL_BLOCK_SIZE / it->second);
        }
        return static_cast<uint64_t>(record.nBurst) * MOV_LOCAL_BLOCK_SIZE;
    }

    template<typename T>
    bool ParseDmaMovWithPadMode(std::size_t key, T const &record, Common::MemRecord &memRec)
    {
        using namespace Common;
        using namespace Utility;
        static_assert(std::is_same<T, DmaMovRecord>::value || std::is_same<T, MovV2Record>::value,
                      "Record type doesn't support to parse dma mov instruction with pad mode");

        if (record.padMode >= PadMode::PAD_MODE1 && record.padMode <= PadMode::PAD_MODE5) {
            /// PAD_MODE1 到 PAD_MODE5 源地址的数据都是连续排布并且 lenBurst 必须是 1
            memRec.srcAddr = record.src;
            memRec.srcMemSize = CalcPadModeSize(record);

            for (uint64_t idx = 0; idx < record.nBurst; ++idx) {
                uint64_t dstLen = MOV_LOCAL_BLOCK_SIZE;
                uint64_t dstGap = static_cast<uint64_t>(record.dstStride) * MOV_LOCAL_BLOCK_SIZE;
                uint64_t dstAddr = SafeAdd(record.dst, idx * (dstLen + dstGap), __FUNCTION__);
                memRec.dstAddr = dstAddr;
                memRec.dstMemSize = dstLen;
            }
            Emplace(key, memRec);
        } else if (record.padMode >= PadMode::PAD_MODE6 && record.padMode <= PadMode::PAD_MODE8) {
            /// PAD_MODE6 到 PAD_MODE8 目的地址的数据都是连续排布并且 lenBurst 必须是 1
            memRec.dstAddr = record.dst;
            memRec.dstMemSize = CalcPadModeSize(record);

            for (uint64_t idx = 0; idx < record.nBurst; ++idx) {
                uint64_t srcLen = MOV_LOCAL_BLOCK_SIZE;
                uint64_t srcGap = static_cast<uint64_t>(record.srcStride) * MOV_LOCAL_BLOCK_SIZE;
                uint64_t srcAddr = SafeAdd(record.src, idx * (srcLen + srcGap), __FUNCTION__);
                memRec.srcAddr = srcAddr;
                memRec.srcMemSize = srcLen;
            }
            Emplace(key, memRec);
        } else {
            std::lock_guard<std::mutex> lock(parseMutex_);
            ++memoryChartMetrics_[key].invalidParam;
            return false;
        }
        return true;
    }

    bool ParseLoad2dRecord(const std::string &buffer, std::size_t &index, std::size_t key, uint16_t blockId);
    bool ParseMovA5Record(const std::string &buffer, std::size_t &index, std::size_t key, uint16_t blockId);
    void ParseDmaMovWithByteMode(std::size_t key, Common::DmaMovRecord const &record,
                                 Common::MemRecord &memRec);
    bool ParseDmaMovRecord(const std::string &buffer, std::size_t &index, std::size_t key, uint16_t blockId);
    bool ParseMovAlignRecord(const std::string &buffer, std::size_t &index, std::size_t key, uint16_t blockId);
    bool ParseMovAlignV2Record(const std::string &buffer, std::size_t &index, std::size_t key, uint16_t blockId);
    bool ParseDmaMovNd2nzRecord(const std::string &buffer, std::size_t &index, std::size_t key,
                                uint16_t blockId);
    bool ParseMovV2Record(const std::string &buffer, std::size_t &index, std::size_t key, uint16_t blockId);
    bool ParseDmaMovNd2nzDavRecord(const std::string &buffer, std::size_t &index, std::size_t key, uint16_t blockId);
    bool ParseDmaMovDn2nzDavRecord(const std::string &buffer, std::size_t &index, std::size_t key, uint16_t blockId);
    template <bool isSrcNd>
    void ParseDmaMovNdOrDn2nzDavRecordSmallC0Mode(const Common::DmaMovNd2nzDavRecord &record,
                                                  std::size_t key, uint16_t blockId);
    template <bool isSrcNd>
    void ParseDmaMovNdOrDn2nzDavRecordNormalMode(const Common::DmaMovNd2nzDavRecord &record,
                                                            std::size_t key, uint16_t blockId);
    bool ParseMovFpRecord(const std::string &buffer, std::size_t &index, std::size_t key, uint16_t blockId);
    bool ParseLoad2DTransposeRecord(const std::string &buffer, std::size_t &index, std::size_t key,
                                           uint16_t blockId);
    bool ParseLoad2DV2Record(const std::string &buffer, std::size_t &index, std::size_t key, uint16_t blockId);
    bool ParseLoadMX2DV2Record(const std::string &buffer, std::size_t &index, std::size_t key, uint16_t blockId);
    bool ParseLoad2DV2DecRecord(const std::string &buffer, std::size_t &index, std::size_t key, uint16_t blockId);
    bool ParseSet2DRecord(const std::string &buffer, std::size_t &index, std::size_t key, uint16_t blockId);
    bool ParseNdDMAOut2UbRecord(const std::string &buffer, std::size_t &index, std::size_t key, uint16_t blockId);
    template<typename T>
    void FixPipeStoreToOut(T const &record, Common::MemRecord &memRecord, uint16_t nColumn,
                           uint16_t splitColumn, std::size_t key);
    template<typename T>
    void FixPipeStoreToOut_Normal(T const &record, Common::MemRecord &memRecord, uint16_t nColumn,
                                            uint16_t splitColumn, std::size_t key);
    template<typename T>
    void FixPipeStoreToOut_NZ2ND(T const &record, Common::MemRecord &memRecord, std::size_t key);
    template<typename T>
    uint16_t GetSplitColumn(const T &record, uint16_t nColumn) const;
    template <uint32_t alignSize, typename T>
    T AlignUp(T value);
    void ParseMemoryChart(std::size_t key, const std::string &msg, const ProfStub::DBIDataHeader &dbiDataHeader);
    void TryDumpMemoryChartMetrics(std::size_t clientId);
    void PacketHandler(std::size_t clientId, const std::string &msg);
    bool HandleEndFlag(const ProfStub::DBIDataHeader &dbiDataHeader, std::size_t clientId, const std::string &msg);
    void VerifyAndDump(const MemoryChartMetrics& metrics);
    bool ValidateFixL0CGMRecord(const Common::FixL0CGMRecord &record, std::size_t key);
    bool ParseFixL0CGMRecord(const std::string &buffer, std::size_t &index, std::size_t key, uint16_t blockId);
    // key: clientId, 所有client一次性只会发送一个kernel的数据，基础组件每个线程clientId唯一
    std::unordered_map<std::size_t, MemoryChartMetrics> memoryChartMetrics_;
    std::string outputPath_;
    std::map<Common::RecordType,
        std::function<bool(const std::string &, std::size_t &, std::size_t, uint16_t blockId)>>
        recordParserFunc_;
    std::mutex parseMutex_;
    Utility::ThreadPool pool_;
    State started_{State::INITED};
    std::atomic<uint64_t> procedureErrorCount_{0};
    std::atomic<uint16_t> invalidPacketCount_{0};
};
}
#endif // MSOPT_DBIPARSER_H
