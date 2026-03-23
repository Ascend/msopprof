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


#include "dbi_parser.h"
#include <utility>
#include <vector>
#include <type_traits>
#include "log.h"
#include "number_operation.h"
#include "prof_injection/packet.h"
#include "filesystem.h"
#include "common/defs.h"

using namespace Common;
using namespace Utility;

namespace Profiling {
constexpr uint64_t MOV_32_BYTE = 32UL;
constexpr uint32_t DEFAULT_THREAD_NUM = 8;
constexpr uint32_t MAX_THREAD_USAGE_RATIO = 10; // 10%

template<uint64_t alignSize>
inline uint64_t AlignToCeil(uint64_t value)
{
    static const std::string LOCATION("alignToCeil");
    static_assert(alignSize != 0, "align size cannot be zero");
    return SafeMul(((SafeAdd(value, alignSize, LOCATION) - 1) / alignSize), alignSize, LOCATION);
}

template <uint64_t N>
constexpr unsigned GetBitLen()
{
    return (N == 1) ? 0 : 1 + GetBitLen<(N >> 1)>();
}

template<uint64_t ALIGN = 32>
static uint64_t GetBurstLenAligned(uint64_t len)
{
    constexpr unsigned ALIGN_SHIFT = GetBitLen<ALIGN>();
    return ((len + ALIGN - 1) >> ALIGN_SHIFT) << ALIGN_SHIFT;
}

template <uint32_t alignSize, typename T>
T DBIParser::AlignUp(T value) const
{
    static_assert(std::is_integral<T>::value, "AlignUp requires an integral type");
    static_assert(alignSize != 0, "AlignSize must be a non-zero positive integer!");
    return (value + static_cast<T>(alignSize) - 1U) / static_cast<T>(alignSize);
}

void DBIParser::ParseMemoryChart(std::size_t key, const std::string &msg, const DBIDataHeader &dbiDataHeader)
{
    if (dbiDataHeader.count == 0) {
        return;
    }
    std::size_t index = sizeof(DBIDataHeader);
    std::vector<uint32_t> processed(static_cast<uint32_t>(Common::RecordType::END), 0);
    std::vector<uint32_t> failed(static_cast<uint32_t>(Common::RecordType::END), 0);
    uint64_t processedCount = 0;
    for (;processedCount < dbiDataHeader.count && index < msg.size(); ++processedCount) {
        RecordHeader rh;
        rh.recordType = RecordType::INVALID;
        if (memcpy_s(&rh, sizeof(rh), &msg[index], sizeof(rh)) != EOK) {
            std::lock_guard<std::mutex> lock(parseMutex_);
            ++memoryChartMetrics_[key].memoryCopyFailed;
            break;
        }
        index += sizeof(RecordHeader);
        if (index >= msg.size()) {
            std::lock_guard<std::mutex> lock(parseMutex_);
            ++memoryChartMetrics_[key].invalidDataCount;
            break;
        }
        if (rh.magicWords != DBI_RECORD_MAGIC_WORDS) {
            std::lock_guard<std::mutex> lock(parseMutex_);
            ++memoryChartMetrics_[key].memoryCorruption;
            break;
        }
        auto funcIter = recordParserFunc_.find(rh.recordType);
        if (funcIter == recordParserFunc_.end()) {
            std::lock_guard<std::mutex> lock(parseMutex_);
            ++memoryChartMetrics_[key].invalidRecordType;
            break;
        }
        ++processed[static_cast<uint32_t>(rh.recordType)];
        if (!funcIter->second(msg, index, key, dbiDataHeader.blockId)) {
            ++failed[static_cast<uint32_t>(rh.recordType)];
        }
    }
    {
        std::lock_guard<std::mutex> lock(parseMutex_);
        memoryChartMetrics_[key].processedRecord += processedCount;
        memoryChartMetrics_[key].droppedRecord += dbiDataHeader.count - processedCount;
        for (uint32_t i = 0; i < static_cast<uint32_t>(Common::RecordType::END); ++i) {
            memoryChartMetrics_[key].typeProcessed[i] += processed[i];
            memoryChartMetrics_[key].typeFailed[i] += failed[i];
        }
    }
}

void DBIParser::TryDumpMemoryChartMetrics(std::size_t clientId)
{
    MemoryChartMetrics metrics;
    {
        std::lock_guard<std::mutex> lock(parseMutex_);
        MemoryChartMetrics &tmpMetrics = memoryChartMetrics_[clientId];
        if (!tmpMetrics.endFlag || tmpMetrics.receivedPacket > tmpMetrics.processedPacket) {
            return;
        }
        metrics = std::move(memoryChartMetrics_[clientId]);
        memoryChartMetrics_.erase(clientId);
    }
    VerifyAndDump(metrics);
}

bool DBIParser::HandleEndFlag(const DBIDataHeader &dbiDataHeader, std::size_t clientId,
                              const std::string &msg)
{
    if (dbiDataHeader.endFlag == 0) {
        return false;
    }
    {
        std::lock_guard<std::mutex> lock(parseMutex_);
        memoryChartMetrics_[clientId].endFlag = true;
        // 如：/xxx/OPPROF_20250317011811_DNMFKOVQMZFKYLUP/device0/add_custom_0/0/dump
        memoryChartMetrics_[clientId].outputPath = msg.substr(sizeof(dbiDataHeader));
    }
    TryDumpMemoryChartMetrics(clientId);
    return true;
}

void DBIParser::PacketHandler(std::size_t clientId, const std::string &msg)
{
    DBIDataHeader dbiDataHeader{};
    Communication::Deserialize(msg, dbiDataHeader);
    if (HandleEndFlag(dbiDataHeader, clientId, msg)) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(parseMutex_);
        memoryChartMetrics_[clientId].receivedPacket++;
        memoryChartMetrics_[clientId].overflow += dbiDataHeader.overflow;
    }
    ParseMemoryChart(clientId, msg, dbiDataHeader);
    {
        std::lock_guard<std::mutex> lock(parseMutex_);
        memoryChartMetrics_[clientId].processedPacket++;
    }
    TryDumpMemoryChartMetrics(clientId);
}

void DBIParser::ParsePacket(std::size_t clientId, std::string &&msg)
{
    pool_.AddTask(std::bind(&DBIParser::PacketHandler, this, clientId, std::move(msg)));
    {
        std::lock_guard<std::mutex> lock(parseMutex_);
        if (started_ == State::INITED) {
            started_ = State::RUNNING;
            pool_.Start();
        }
    }
}

bool DBIParser::ParseLoad2dRecord(const std::string &buffer, std::size_t &index,
                                  std::size_t key, uint16_t blockId)
{
    static const std::string LOCATION("ParseLoad2dRecord");
    Load2DRecord record{};
    index += sizeof(Load2DRecord);
    if (index > buffer.size()) {
        std::lock_guard<std::mutex> lock(parseMutex_);
        ++memoryChartMetrics_[key].outOfBoundsRead;
        return false;
    } else if (memcpy_s(&record, sizeof(Load2DRecord),
                        &buffer[index - sizeof(Load2DRecord)], sizeof(Load2DRecord)) != EOK) {
        std::lock_guard<std::mutex> lock(parseMutex_);
        ++memoryChartMetrics_[key].memoryCopyFailed;
        return false;
    }
    MemRecord memRec{};
    memRec.pc = record.pc;
    memRec.src = record.srcMemType;
    memRec.dst = record.dstMemType;
    memRec.srcMemSize = LOAD_2D_MOV_SIZE;
    memRec.dstMemSize = LOAD_2D_MOV_SIZE;
    memRec.blockId = blockId;

    for (uint64_t repeatIdx = 0; repeatIdx < record.repeat; ++repeatIdx) {
        uint64_t srcRepeatAddr {0U};
        if (record.addrCalMode == AddrCalMode::INC) {
            srcRepeatAddr = SafeAddAll<uint64_t>({record.src, static_cast<uint64_t>(record.baseIdx) * LOAD_2D_MOV_SIZE,
                static_cast<uint64_t>(record.srcStride) * repeatIdx * LOAD_2D_MOV_SIZE}, LOCATION);
        } else {
            srcRepeatAddr = SafeSub(SafeAdd(record.src, static_cast<uint64_t>(record.baseIdx) * LOAD_2D_MOV_SIZE,
                LOCATION), static_cast<uint64_t>(record.srcStride) * repeatIdx * LOAD_2D_MOV_SIZE, LOCATION);
        }
        memRec.srcAddr = srcRepeatAddr;
        uint64_t dstAddr = SafeAdd(record.dst, static_cast<uint64_t>(record.dstStride) * repeatIdx * LOAD_2D_MOV_SIZE,
                           LOCATION);
        memRec.dstAddr = dstAddr;
        Emplace(key, memRec);
    }
    return true;
}

// COPY_CBUF_TO_GM_BYTE
void DBIParser::ParseDmaMovWithByteMode(std::size_t key, DmaMovRecord const &record, MemRecord &memRec)
{
    static const std::string LOCATION("ParseDmaMovWithByteMode");
    if (record.nBurst == 0) {
        LogDebug("NBurst is 0 when parse dma move with byte mode.");
    }
    for (uint64_t idx = 0; idx < record.nBurst; ++idx) {
        uint64_t srcLen = AlignToCeil<MOV_LOCAL_BLOCK_SIZE>(record.lenBurst);
        uint64_t srcGap = static_cast<uint64_t>(record.srcStride) * MOV_LOCAL_BLOCK_SIZE;
        uint64_t srcAddr = SafeAdd(record.src, SafeMul(idx, SafeAdd(srcLen, srcGap, LOCATION), LOCATION), LOCATION);
        memRec.srcAddr = srcAddr;
        memRec.srcMemSize = srcLen;

        uint64_t dstLen = record.lenBurst;
        uint64_t dstGap = static_cast<uint64_t>(record.dstStride) * MOV_LOCAL_BLOCK_SIZE;
        uint64_t dstAddr = SafeAdd(record.dst, SafeMul(idx, SafeAdd(dstLen, dstGap, LOCATION), LOCATION), LOCATION);
        memRec.dstAddr = dstAddr;
        memRec.dstMemSize = dstLen;
        Emplace(key, memRec);
    }
}

bool DBIParser::ParseDmaMovRecord(const std::string &buffer, std::size_t &index,
                                  std::size_t key, uint16_t blockId)
{
    static const std::string LOCATION("ParseDmaMovRecord");
    DmaMovRecord record{};
    if (!CheckRecord(record, buffer, index, key)) {
        return false;
    }

    MemRecord memRec{};
    memRec.pc = record.pc;
    memRec.src = record.srcMemType;
    memRec.dst = record.dstMemType;
    memRec.blockId = blockId;

    if (record.byteMode == ByteMode::BM_ENABLE) {
        ParseDmaMovWithByteMode(key, record, memRec);
        return true;
    }

    if (record.padMode != PadMode::PAD_NONE) {
        return ParseDmaMovWithPadMode<DmaMovRecord>(key, record, memRec);
    }
    for (uint64_t idx = 0; idx < record.nBurst; ++idx) {
        uint64_t srcLen = static_cast<uint64_t>(record.lenBurst) * MOV_LOCAL_BLOCK_SIZE;
        uint64_t srcGap = static_cast<uint64_t>(record.srcStride) * MOV_LOCAL_BLOCK_SIZE;
        uint64_t srcAddr = SafeAdd(record.src, SafeMul(idx, SafeAdd(srcLen, srcGap, LOCATION), LOCATION), LOCATION);
        memRec.srcAddr = srcAddr;
        memRec.srcMemSize = srcLen;

        uint64_t dstLen = static_cast<uint64_t>(record.lenBurst) * MOV_LOCAL_BLOCK_SIZE;
        uint64_t dstGap = static_cast<uint64_t>(record.dstStride) * MOV_LOCAL_BLOCK_SIZE;
        uint64_t dstAddr = SafeAdd(record.dst, SafeMul(idx, SafeAdd(dstLen, dstGap, LOCATION), LOCATION), LOCATION);
        memRec.dstAddr = dstAddr;
        memRec.dstMemSize = dstLen;
        Emplace(key, memRec);
    }
    return true;
}

// gm_to_ubuf
bool DBIParser::ParseMovAlignRecord(const std::string &buffer, std::size_t &index,
                                    std::size_t key, uint16_t blockId)
{
    static const std::string LOCATION("ParseMovAlignRecord");
    MovAlignRecord record{};
    if (!CheckRecord(record, buffer, index, key)) {
        return false;
    }
    MemRecord memRec{};
    memRec.pc = record.pc;
    memRec.src = record.srcMemType;
    memRec.dst = record.dstMemType;
    memRec.blockId = blockId;

    for (uint64_t idx = 0; idx < record.nBurst; ++idx) {
        uint64_t srcLen = record.srcMemType == MemType::GM ?
                          record.lenBurst : AlignToCeil<MOV_LOCAL_BLOCK_SIZE>(record.lenBurst);
        uint64_t srcGap = record.srcMemType == MemType::GM ?
                          record.srcGap : static_cast<uint64_t>(record.srcGap) * MOV_LOCAL_BLOCK_SIZE;
        uint64_t srcAddr = SafeAdd(record.src, SafeMul(idx, SafeAdd(srcLen, srcGap, LOCATION), LOCATION), LOCATION);
        memRec.srcAddr = srcAddr;
        memRec.srcMemSize = srcLen;

        uint64_t paddingSize = SafeMul(static_cast<uint64_t>(record.leftPaddingNum) +
            static_cast<uint64_t>(record.rightPaddingNum), GetDataTypeSizeInBits(record.dataType), LOCATION) / BITS_EACH_BYTE;
        uint64_t dstLen = record.dstMemType == MemType::GM ?
                          record.lenBurst : AlignToCeil<MOV_LOCAL_BLOCK_SIZE>(SafeAdd<uint64_t>(record.lenBurst,
                          paddingSize, LOCATION));
        uint64_t dstGap = record.dstMemType == MemType::GM ?
                          record.dstGap : static_cast<uint64_t>(record.dstGap) * MOV_LOCAL_BLOCK_SIZE;
        uint64_t dstAddr = SafeAdd(record.dst, SafeMul(idx, SafeAdd(dstLen, dstGap, LOCATION), LOCATION), LOCATION);
        memRec.dstAddr = dstAddr;
        memRec.dstMemSize = dstLen;
        Emplace(key, memRec);
    }
    return true;
}

void MovAlignV2RecordSize(MovAlignV2Record &record, MemRecord &memRec)
{
    if (record.dstMemType == MemType::UB) {
        // GM to UB
        if (record.dstStride == record.lenBurst) {
            /* Compact mode */
            memRec.srcMemSize = record.lenBurst * record.nBurst;
            memRec.dstMemSize = GetBurstLenAligned<32>(
                record.lenBurst * record.nBurst + record.leftPaddingNum + record.rightPaddingNum);
            record.nBurst = 1;
        } else {
            memRec.srcMemSize = record.lenBurst;
            memRec.dstMemSize = GetBurstLenAligned<32>(record.lenBurst + record.leftPaddingNum + record.rightPaddingNum);
        }
    } else if (record.dstMemType == MemType::GM) {
        // UB to GM
        if (record.srcStride == record.lenBurst) {
            /* Compact mode */
            memRec.srcMemSize = GetBurstLenAligned<32>(
                record.lenBurst * record.nBurst + record.leftPaddingNum + record.rightPaddingNum);
            memRec.dstMemSize = record.lenBurst * record.nBurst;
            record.nBurst = 1;
        } else {
            memRec.srcMemSize = GetBurstLenAligned<32>(record.lenBurst + record.leftPaddingNum + record.rightPaddingNum);
            memRec.dstMemSize = record.lenBurst;
        }
    }
}

bool DBIParser::ParseMovAlignV2Record(const std::string &buffer, std::size_t &index,
                                      std::size_t key, uint16_t blockId)
{
    MovAlignV2Record record{};
    if (!CheckRecord(record, buffer, index, key)) {
        return false;
    }
    MemRecord memRec{};
    memRec.pc = record.pc;
    memRec.src = record.srcMemType;
    memRec.dst = record.dstMemType;
    memRec.blockId = blockId;

    bool isLoopMode = (record.loop1Size >= 1) && (record.loop2Size >= 1);
    // 不支持同时开启loop模式和LP/RP模式
    if (!isLoopMode) {
        record.loop1Size = 1;
        record.loop2Size = 1;
        record.loop1SrcStride = 0;
        record.loop1DstStride = 0;
        record.loop2SrcStride = 0;
        record.loop2DstStride = 0;
    } else {
        record.rightPaddingNum = 0;
        record.leftPaddingNum = 0;
    }

    MovAlignV2RecordSize(record, memRec);

    for (uint32_t k = 0; k < record.loop2Size; ++k) {
        for (uint32_t j = 0; j < record.loop1Size; ++j) {
            for (uint32_t i = 0; i < record.nBurst; ++i) {
                memRec.srcAddr =
                    SafeMulAddAll(std::vector<uint64_t>({1, k, j, i}),
                        std::vector<uint64_t>({record.src, record.loop2SrcStride, record.loop1SrcStride, record.srcStride}),
                        __FUNCTION__);
                memRec.dstAddr =
                    SafeMulAddAll(std::vector<uint64_t>({1, k, j, i}),
                        std::vector<uint64_t>({record.dst, record.loop2DstStride, record.loop1DstStride, record.dstStride}),
                        __FUNCTION__);
                Emplace(key, memRec);
            }
        }
    }

    return true;
}

// gm_to_cbuf
bool DBIParser::ParseDmaMovNd2nzRecord(const std::string &buffer, std::size_t &index,
                                       std::size_t key, uint16_t blockId)
{
    static const std::string LOCATION("ParseDmaMovNd2nzRecord");
    DmaMovNd2nzRecord record{};
    if (!CheckRecord(record, buffer, index, key)) {
        return false;
    }
    MemRecord memRec{};
    memRec.pc = record.pc;
    memRec.src = record.srcMemType;
    memRec.dst = record.dstMemType;
    memRec.blockId = blockId;
    constexpr uint64_t c0Size = 32U;
    uint64_t dTypeBitSize = GetDataTypeSizeInBits(record.dataType);
    // D维度长度
    uint64_t dDimByteSize = SafeMul<uint64_t>(record.srcDValue, dTypeBitSize, LOCATION) / BITS_EACH_BYTE;
    // D维度上有效数据的长度
    uint64_t dByteSize = SafeMul<uint64_t>(record.dValue, dTypeBitSize, LOCATION) / BITS_EACH_BYTE;
    memRec.srcMemSize = dByteSize;
    memRec.dstMemSize = c0Size;
    for (uint16_t nd = 0; nd < record.ndNum; ++nd) {
        for (uint16_t n = 0; n < record.nValue; ++n) {
            memRec.srcAddr = SafeAddAll<uint64_t>({record.src, SafeMulAll<uint64_t>({record.srcNdMatrixStride,
                dTypeBitSize, nd}, LOCATION) / BITS_EACH_BYTE, SafeMul<uint64_t>(dDimByteSize, n, LOCATION)}, LOCATION);
            Emplace(key, memRec);
        }
    }
    return true;
}

template <bool isSrcNd>
void DBIParser::ParseDmaMovNdOrDn2nzDavRecordSmallC0Mode(const DmaMovNd2nzDavRecord &record, std::size_t key,
                                                         uint16_t blockId)
{
    MemRecord memRec{};
    memRec.pc = record.pc;
    memRec.src = record.srcMemType;
    memRec.dst = record.dstMemType;
    memRec.blockId = blockId;
    const uint64_t c0Size = 32;
    const uint64_t sizeOfDataType = GetDataTypeSizeInBytes(record.dataType);
    const uint64_t smallC0Size = 4 * sizeOfDataType;
    const uint64_t rowNum = isSrcNd ? record.nValue : record.dValue;
    const uint64_t colNum = isSrcNd ? record.dValue : record.nValue;
    memRec.dstMemSize = smallC0Size;
    for (uint64_t h = 0; h < record.ndNum; h++) {
        const uint64_t srcNdAddr = record.src + h * record.loop4SrcStride;
        const uint64_t dstNdAddr = record.dst + h * record.loop4DstStride * c0Size;
        for (uint64_t j = 0; j < rowNum; j++) {
            memRec.srcAddr = srcNdAddr + j * record.loop1SrcStride;
            memRec.srcMemSize = colNum * sizeOfDataType;
            memRec.dstAddr = dstNdAddr + j * smallC0Size;
            Emplace(key, memRec);
        }
    }
}

template <bool isSrcNd>
void DBIParser::ParseDmaMovNdOrDn2nzDavRecordNormalMode(const DmaMovNd2nzDavRecord &record, std::size_t key,
                                                        uint16_t blockId)
{
    MemRecord memRec{};
    memRec.pc = record.pc;
    memRec.src = record.srcMemType;
    memRec.dst = record.dstMemType;
    memRec.blockId = blockId;
    const uint64_t c0Size = 32;
    const uint64_t sizeOfDataType = GetDataTypeSizeInBytes(record.dataType);
    const uint64_t rowNum = isSrcNd ? record.nValue : record.dValue;
    const uint64_t colNum = isSrcNd ? record.dValue : record.nValue;
    memRec.dstMemSize = GetBurstLenAligned<c0Size>(rowNum * sizeOfDataType);
    for (uint64_t h = 0; h < record.ndNum; h++) {
        const uint64_t srcNdAddr = record.src + h * record.loop4SrcStride;
        const uint64_t dstNdAddr = record.dst + h * record.loop4DstStride * c0Size;
        for (uint64_t j = 0; j < rowNum; j++) {
            memRec.srcAddr = srcNdAddr + j * record.loop1SrcStride;
            memRec.srcMemSize = colNum * sizeOfDataType;
            memRec.dstAddr = dstNdAddr + j * record.loop2DstStride * c0Size;
            Emplace(key, memRec);
        }
    }
}

bool DBIParser::ParseDmaMovNd2nzDavRecord(const std::string &buffer, std::size_t &index, std::size_t key, uint16_t blockId)
{
    DmaMovNd2nzDavRecord record{};
    if (!CheckRecord(record, buffer, index, key)) {
        return false;
    }
    if (record.smallC0 != 0) {
        ParseDmaMovNdOrDn2nzDavRecordSmallC0Mode<true>(record, key, blockId);
    } else {
        ParseDmaMovNdOrDn2nzDavRecordNormalMode<true>(record, key, blockId);
    }
    return true;
}

bool DBIParser::ParseDmaMovDn2nzDavRecord(const std::string &buffer, std::size_t &index, std::size_t key, uint16_t blockId)
{
    DmaMovNd2nzDavRecord record{};
    if (!CheckRecord(record, buffer, index, key)) {
        return false;
    }
    if (record.smallC0 != 0) {
        ParseDmaMovNdOrDn2nzDavRecordSmallC0Mode<false>(record, key, blockId);
    } else {
        ParseDmaMovNdOrDn2nzDavRecordNormalMode<false>(record, key, blockId);
    }
    return true;
}

template<typename T>
void DBIParser::FixPipeStoreToOut_NZ2ND(T const &record, MemRecord &memRecord, std::size_t key)
{
    static const std::string LOCATION("FixPipeStoreToOut_NZ2ND");
    // 4bits时2个合并为1个
    uint32_t blockSize = record.quantPreBits == 4 ? 1U : (record.quantPreBits / BITS_EACH_BYTE);
    uint32_t blockNum = record.quantPreBits == 4 ? (record.nSize / 2) : record.nSize;  // 4bits时nSize是2的倍数
    // 4bits时dstStride是2的倍数
    uint32_t repeatStride = record.quantPreBits == 4 ? (record.dstStride / 2) : record.dstStride;
    memRecord.dstMemSize = blockSize * blockNum;
    memRecord.srcMemSize = blockSize * blockNum; // 源内存大小与目标内存大小相同

    for (uint16_t nd = 0; nd < record.ndNum; ++nd) {
        // 将与 m 无关的部分提取到外层循环中
        uint64_t ndOffset = SafeMulAll<uint64_t>({record.dstNdStride, nd, record.quantPreBits / BITS_EACH_BYTE}, LOCATION);
        uint64_t ndSrcOffset = SafeMulAll<uint64_t>({record.srcNdStride, nd, record.quantPreBits / BITS_EACH_BYTE}, LOCATION);

        for (uint16_t m = 0; m < record.mSize; ++m) {
            // 计算与 m 相关的部分
            uint64_t mOffset = SafeMulAll<uint64_t>({m, blockSize, repeatStride}, LOCATION);
            memRecord.dstAddr = SafeAddAll<uint64_t>({record.dst, ndOffset, mOffset}, LOCATION);
            memRecord.srcAddr = SafeAddAll<uint64_t>({record.src, ndSrcOffset, mOffset}, LOCATION);
            Emplace(key, memRecord);
        }
    }
}

template<typename T>
void DBIParser::FixPipeStoreToOut_Normal(T const &record, MemRecord &memRecord, uint16_t nColumn,
                                         uint16_t splitColumn, std::size_t key)
{
    static const std::string LOCATION("FixPipeStoreToOut_Normal");
    if (record.channelSplit || record.int4ChannelMerge || record.int8ChannelMerge) {
        memRecord.dstMemSize = static_cast<uint64_t>(record.mSize) * MOV_LOCAL_BLOCK_SIZE;
        memRecord.srcMemSize = static_cast<uint64_t>(record.mSize) * MOV_LOCAL_BLOCK_SIZE;
        for (uint16_t n = 0; n < splitColumn; ++n) {
            memRecord.dstAddr = SafeAdd(record.dst, static_cast<uint64_t>(record.dstStride) *
                static_cast<uint64_t>(n) * MOV_LOCAL_BLOCK_SIZE, LOCATION);
            memRecord.srcAddr = SafeAdd(record.src, static_cast<uint64_t>(record.srcStride) *
                static_cast<uint64_t>(n) * MOV_LOCAL_BLOCK_SIZE, LOCATION);
            Emplace(key, memRecord);
        }

        // 如果int8ChannelMerge使能，但N维不是16的偶数倍（被2整除），剩下数据直接搬出
        if (record.int8ChannelMerge && (nColumn % 2) != 0) {
            memRecord.dstMemSize = memRecord.dstMemSize / 2;  // 剩下数据大小在原基础上除以2
            memRecord.srcMemSize = memRecord.srcMemSize / 2;  // 源内存大小与目标内存大小相同
            memRecord.dstAddr = SafeAdd(record.dst, static_cast<uint64_t>(record.dstStride) *
                static_cast<uint64_t>(splitColumn) * MOV_LOCAL_BLOCK_SIZE, LOCATION);
            memRecord.srcAddr = SafeAdd(record.src, static_cast<uint64_t>(record.srcStride) *
                static_cast<uint64_t>(splitColumn) * MOV_LOCAL_BLOCK_SIZE, LOCATION);
            Emplace(key, memRecord);
        }
    } else {
        memRecord.dstMemSize = static_cast<uint64_t>(record.mSize) * static_cast<uint64_t>(record.quantPreBits) /
                               BITS_EACH_BYTE / 2; // *16/32B, 简化为/2
        memRecord.srcMemSize = static_cast<uint64_t>(record.mSize) * static_cast<uint64_t>(record.quantPreBits) /
                               BITS_EACH_BYTE / 2; // 源内存大小与目标内存大小相同
        for (uint16_t n = 0; n < splitColumn; ++n) {
            memRecord.dstAddr = SafeAdd(record.dst, static_cast<uint64_t>(record.dstStride) *
                static_cast<uint64_t>(n) * MOV_LOCAL_BLOCK_SIZE, LOCATION);
            memRecord.srcAddr = SafeAdd(record.src, static_cast<uint64_t>(record.srcStride) *
                static_cast<uint64_t>(n) * MOV_LOCAL_BLOCK_SIZE, LOCATION);
            Emplace(key, memRecord);
        }
    }
}

template<typename T>
void DBIParser::FixPipeStoreToOut(T const &record, MemRecord &memRecord, uint16_t nColumn,
                                  uint16_t splitColumn, std::size_t key)
{
    if (record.enNZ2ND) {
        FixPipeStoreToOut_NZ2ND(record, memRecord, key);
    } else {
        FixPipeStoreToOut_Normal(record, memRecord, nColumn, splitColumn, key);
    }
}

bool DBIParser::ParseLoad2DTransposeRecord(const std::string &buffer, std::size_t &index,
                                           std::size_t key, uint16_t blockId)
{
    Load2DTransposeRecord record;
    if (!CheckRecord(record, buffer, index, key)) {
        return false;
    }
    MemRecord memRec;
    memRec.pc = record.pc;
    memRec.src = record.srcMemType;
    memRec.dst = record.dstMemType;
    memRec.blockId = blockId;
    memRec.srcMemSize = LOAD_2D_MOV_SIZE * record.srcStride;
    memRec.dstMemSize =  LOAD_2D_MOV_SIZE * record.dstStride;
    for (uint8_t repeatId = 0; repeatId < record.repeat; ++repeatId) {
        uint64_t srcFractalId = record.baseIdx + static_cast<uint64_t>(record.srcStride) * repeatId;
        memRec.srcAddr = SafeAdd(record.src, srcFractalId * LOAD_2D_MOV_SIZE, __FUNCTION__);
        memRec.dstAddr = SafeAdd(record.dst,
            static_cast<uint64_t>(record.dstStride) * repeatId * LOAD_2D_MOV_SIZE, __FUNCTION__);
        Emplace(key, memRec);
    }
    return true;
}

bool DBIParser::ParseLoad2DV2Record(const std::string &buffer, std::size_t &index, std::size_t key, uint16_t blockId)
{
    Load2DV2Record record;
    if (!CheckRecord(record, buffer, index, key)) {
        return false;
    }
    MemRecord memRec;
    memRec.pc = record.pc;
    memRec.src = record.srcMemType;
    memRec.dst = record.dstMemType;
    memRec.blockId = blockId;
    memRec.srcMemSize = static_cast<uint64_t>(record.mStep) * record.kStep * LOAD_2D_MOV_SIZE;
    memRec.dstMemSize = memRec.srcMemSize;
    // srcStride 是有符号型，需要取绝对值
    record.srcStride = abs(static_cast<int16_t>(record.srcStride));
    uint64_t step = static_cast<uint64_t>(record.srcStride) * record.kStartPos + record.mStartPos;
    memRec.srcAddr = SafeAdd(record.src, step * LOAD_2D_MOV_SIZE, __FUNCTION__);
    memRec.dstAddr = record.dst;
    Emplace(key, memRec);
    return true;
}

bool DBIParser::ParseLoadMX2DV2Record(const std::string &buffer, std::size_t &index, std::size_t key, uint16_t blockId)
{
    LoadMX2DV2Record record;
    if (!CheckRecord(record, buffer, index, key)) {
        return false;
    }
    MemRecord memRec;
    memRec.pc = record.pc;
    memRec.src = record.srcMemType;
    memRec.dst = record.dstMemType;
    memRec.blockId = blockId;
    memRec.srcMemSize = static_cast<uint64_t>(record.xStep) * record.yStep * MOV_32_BYTE;
    memRec.dstMemSize = memRec.srcMemSize;
    uint64_t step = static_cast<uint64_t>(record.srcStride) * record.xStartPos + record.yStartPos;
    memRec.srcAddr = SafeAdd(record.src, step * MOV_32_BYTE, __FUNCTION__);
    memRec.dstAddr = record.dst;
    Emplace(key, memRec);
    return true;
}

bool DBIParser::ParseLoad2DV2DecRecord(const std::string &buffer, std::size_t &index, std::size_t key, uint16_t blockId)
{
    static const std::string LOCATION("ParseLoad2DV2DecRecord");
    Load2DV2DecRecord record;
    if (!CheckRecord(record, buffer, index, key)) {
        return false;
    }
    uint64_t srcFracSize = 512;
    uint64_t dstFracSize = 512;
    if (record.decompMode == 1 || record.decompMode == 4) {
        srcFracSize = 128;
    } else if (record.decompMode == 2) {
        srcFracSize = 192;
    } else if (record.decompMode == 3) {
        srcFracSize = 256;
    }
    MemRecord memRec;
    memRec.pc = record.pc;
    memRec.src = record.srcMemType;
    memRec.dst = record.dstMemType;
    memRec.blockId = blockId;
    memRec.srcMemSize = srcFracSize;
    memRec.dstMemSize = dstFracSize;
    const uint64_t absSrcStride = static_cast<uint64_t>(abs(static_cast<int16_t>(record.srcStride)));
    uint64_t srcAddrStart =
        SafeAddAll(std::vector<uint64_t>{record.src, SafeMulAll(std::vector<uint64_t>{record.kStartPos, absSrcStride, srcFracSize}, LOCATION),
                   SafeMul<uint64_t>(record.mStartPos, srcFracSize, LOCATION)}, LOCATION);
    uint64_t dstAddrStart = record.dst;
    for (uint64_t k = 0; k < record.kStep; k++) {
        uint64_t srcAddrTmp =
            SafeAdd(srcAddrStart, SafeMulAll(std::vector<uint64_t>{k, record.srcStride, srcFracSize}, LOCATION), LOCATION);
        uint64_t dstAddrTmp =
            SafeAdd(dstAddrStart, SafeMulAll(std::vector<uint64_t>{k, record.dstStride, dstFracSize}, LOCATION), LOCATION);
        for (uint64_t m = 0; m < record.mStep; m++) {
            uint64_t srcAddrFrac = SafeAdd(srcAddrTmp, SafeMul(m, srcFracSize, LOCATION), LOCATION);
            uint64_t dstAddrFrac = SafeAdd(dstAddrTmp, SafeMul(m, dstFracSize, LOCATION), LOCATION);
            memRec.srcAddr = srcAddrFrac;
            memRec.dstAddr = dstAddrFrac;
            Emplace(key, memRec);
        }
    }
    return true;
}

bool DBIParser::ParseSet2DRecord(const std::string &buffer, std::size_t &index, std::size_t key, uint16_t blockId)
{
    Set2DRecord record;
    if (!CheckRecord(record, buffer, index, key)) {
        return false;
    }
    MemRecord memRec;
    memRec.pc = record.pc;
    memRec.src = record.srcMemType;
    memRec.dst = record.dstMemType;
    memRec.blockId = blockId;
    memRec.srcMemSize = 0;
    memRec.dstMemSize = (static_cast<uint64_t>(record.blockNum) + record.repeatGap) * MOV_32_BYTE;
    for (uint16_t repeatId = 0; repeatId < record.repeat; ++repeatId) {
        memRec.srcAddr = record.src;
        memRec.dstAddr = SafeAdd(record.dst, memRec.dstMemSize * repeatId, __FUNCTION__);
        Emplace(key, memRec);
    }
    return true;
}

bool DBIParser::ParseMovA5Record(const std::string &buffer, std::size_t &index, std::size_t key, uint16_t blockId)
{
    MovRecord record;
    if (!CheckRecord(record, buffer, index, key)) {
        return false;
    }
    MemRecord memRec{};
    memRec.pc = record.pc;
    memRec.src = record.srcMemType;
    memRec.dst = record.dstMemType;
    memRec.blockId = blockId;

    for (uint64_t idx = 0; idx < record.nBurst; ++idx) {
        uint64_t Len = record.lenBurst * MOV_LOCAL_BLOCK_SIZE;
        uint64_t srcGap = record.srcGap * MOV_LOCAL_BLOCK_SIZE;
        uint64_t srcAddr = SafeAdd(record.src, idx * (Len + srcGap), __FUNCTION__);
        memRec.srcAddr = srcAddr;
        memRec.srcMemSize = Len;

        uint64_t dstGap = record.dstGap * MOV_LOCAL_BLOCK_SIZE;
        uint64_t dstAddr = SafeAdd(record.dst, idx * (Len + dstGap), __FUNCTION__);
        memRec.dstAddr = dstAddr;
        memRec.dstMemSize = Len;
        Emplace(key, memRec);
    }
    return true;
}

bool DBIParser::ParseMovV2Record(const std::string &buffer, std::size_t &index, std::size_t key, uint16_t blockId)
{
    MovV2Record record;
    if (!CheckRecord(record, buffer, index, key)) {
        return false;
    }
    MemRecord memRec{};
    memRec.pc = record.pc;
    memRec.src = record.srcMemType;
    memRec.dst = record.dstMemType;
    memRec.blockId = blockId;

    if (record.padMode != PadMode::PAD_NONE) {
        return ParseDmaMovWithPadMode<MovV2Record>(key, record, memRec);
    }
    for (uint64_t idx = 0; idx < record.nBurst; ++idx) {
        uint64_t Len = record.lenBurst * MOV_LOCAL_BLOCK_SIZE;
        uint64_t srcGap = record.srcStride * MOV_LOCAL_BLOCK_SIZE;
        uint64_t srcAddr = SafeAdd(record.src, idx * (Len + srcGap), __FUNCTION__);
        memRec.srcAddr = srcAddr;
        memRec.srcMemSize = Len;

        uint64_t dstGap = record.dstStride * MOV_LOCAL_BLOCK_SIZE;
        uint64_t dstAddr = SafeAdd(record.dst, idx * (Len + dstGap), __FUNCTION__);
        memRec.dstAddr = dstAddr;
        memRec.dstMemSize = Len;
        Emplace(key, memRec);
    }
    return true;
}

bool DBIParser::ParseMovFpRecord(const std::string &buffer, std::size_t &index, std::size_t key, uint16_t blockId)
{
    MovFpRecord record{};
    index += sizeof(MovFpRecord);
    if (index > buffer.size()) {
        std::lock_guard<std::mutex> lock(parseMutex_);
        ++memoryChartMetrics_[key].outOfBoundsRead;
        return false;
    } else if (memcpy_s(&record, sizeof(MovFpRecord),
                        &buffer[index - sizeof(MovFpRecord)], sizeof(MovFpRecord)) != EOK) {
        std::lock_guard<std::mutex> lock(parseMutex_);
        ++memoryChartMetrics_[key].memoryCopyFailed;
        return false;
    }
    MemRecord memRec{};
    memRec.pc = record.pc;
    memRec.src = MemType::L0C;
    memRec.dst = MemType::GM;
    memRec.blockId = blockId;
    // M = 0或N = 0或nd_number = 0表示不执行，该指令将被视为NOP
    if (record.mSize == 0 || record.nSize == 0 || (record.enNZ2ND && record.ndNum == 0)) {
        std::lock_guard<std::mutex> lock(parseMutex_);
        ++memoryChartMetrics_[key].invalidParam;
        return false;
    }
    // channelSplit/int4ChannelMerge/int8ChannelMerge/enNZ2ND最多只能使能一个
    auto enableNum = static_cast<uint8_t>(record.int8ChannelMerge) +
                     static_cast<uint8_t>(record.int4ChannelMerge) +
                     static_cast<uint8_t>(record.channelSplit) +
                     static_cast<uint8_t>(record.enNZ2ND);
    if (enableNum > 1) {
        LogDebug("Don't parse FIX_L0C_TO_OUT with more than one of ChannelMerge/Split/NZ2ND enabled.");
    }
    if ((record.srcStride % 16) != 0) {  // srcStride必须是16的倍数
        LogDebug("SrcStride must be multiples of 16 when parse FIX_L0C_TO_OUT.");
    }
    if (!record.int4ChannelMerge && !record.channelSplit && !record.enNZ2ND) {
        // normal_movement或者int8ChannelMerge使能时，nSize必须是16的倍数
        if (record.nSize % 16 != 0) {
            LogDebug("NSize should be multiples of 16 when parse FIX_L0C_TO_OUT.");
        }
    }
    if (record.int4ChannelMerge && (record.nSize % 64) != 0) {  // int4ChannelMerge使能时，nSize必须是64的倍数
        LogDebug("NSize should be multiples of 64 when parse FIX_L0C_TO_OUT with int4 channel merge.");
    }
    if (record.channelSplit && (record.nSize % 8) != 0) {  // channelSplit使能时，nSize必须是8的倍数
        LogDebug("NSize should be multiples of 8 when parse FIX_L0C_TO_OUT with f32 channel split.");
    }

    uint16_t nColumn = (record.nSize + 16U - 1) / 16U;  // 向上取整, 16 is c0 base num
    uint16_t splitColumn = GetSplitColumn(record, nColumn);

    FixPipeStoreToOut(record, memRec, nColumn, splitColumn, key);
    return true;
}


static void CalNdDmaMovOut2UbLoopData(const NdDMAOut2UbRecord &ndDmaOut2UbRecord,
                                      const std::function<void(uint64_t, uint64_t, uint64_t, uint64_t)> &callback,
                                      uint64_t typeSize,
                                      std::pair<uint64_t, uint64_t> &offsetPair,
                                      int loop)
{
    static const std::string location(__FUNCTION__);
    auto &loopInfo = ndDmaOut2UbRecord.loop[loop];
    auto padLoopSize = SafeAddAll<uint64_t>({loopInfo.loopSize, loopInfo.loopLpSize, loopInfo.loopRpSize}, location);
    for (uint64_t idx = 0; idx < padLoopSize; ++idx) {
        uint64_t srcOffset = 0;
        auto rightIndex = SafeAdd(static_cast<uint64_t>(loopInfo.loopSize), static_cast<uint64_t>(loopInfo.loopLpSize), location);
        if (idx >= loopInfo.loopLpSize && idx < rightIndex) {
            auto tmpSrc = SafeMulAll<uint64_t>({static_cast<uint64_t>(idx), loopInfo.loopSrcStride, typeSize}, location);
            srcOffset = SafeAdd(offsetPair.first, tmpSrc, location);
        }
        offsetPair.first = srcOffset;
        auto tmpDst = SafeMulAll<uint64_t>({static_cast<uint64_t>(idx), loopInfo.loopDstStride, typeSize}, location);
        offsetPair.second = SafeAdd(offsetPair.second, tmpDst, location);
        if (loop == 0) {
            callback(offsetPair.first, offsetPair.second, static_cast<uint64_t>(loopInfo.loopSize), padLoopSize);
        } else {
            CalNdDmaMovOut2UbLoopData(ndDmaOut2UbRecord, callback, typeSize, offsetPair, loop - 1);
        }
    }
}

bool DBIParser::ParseNdDMAOut2UbRecord(const std::string &buffer, std::size_t &index, std::size_t key, uint16_t blockId)
{
    static const std::string location(__FUNCTION__);
    NdDMAOut2UbRecord record;
    if (!CheckRecord(record, buffer, index, key)) {
        return false;
    }
    MemRecord memRec{};
    memRec.pc = record.pc;
    memRec.src = MemType::GM;
    memRec.dst = MemType::UB;
    memRec.blockId = blockId;
    auto typeSize = GetDataTypeSizeInBytes(record.dataType);
    auto appendRecord = [&](uint64_t srcOffset, uint64_t dstOffset, uint64_t srcSize, uint64_t dstSize) {
        memRec.srcMemSize = SafeMul(srcSize, typeSize, location);
        memRec.dstMemSize = SafeMul(dstSize, typeSize, location);
        memRec.srcAddr = srcOffset == 0 ? 0 : SafeAdd(record.src, srcOffset, location);
        memRec.dstAddr = SafeAdd(record.dst, dstOffset, location);
        Emplace(key, memRec);
    };
    std::pair<uint64_t, uint64_t> offsetPair = {0, 0};
    CalNdDmaMovOut2UbLoopData(record, appendRecord, typeSize, offsetPair, NdDMAOut2UbRecord::LOOP - 1);
    return true;
}

// MOV_FP与FixL0CGM record分型数量计算
template<typename T>
uint16_t DBIParser::GetSplitColumn(const T &record, uint16_t nColumn) const
{
    uint16_t splitColumn{0};
    if (record.channelSplit) {
        splitColumn = (record.nSize + 7U) / 8U;  // 从16x16转换为16x8，除以8向上取整
    } else if (record.int4ChannelMerge) {
        splitColumn = nColumn / 4;  // 从16x16转换为16x64，N维列数除以4
    } else if (record.int8ChannelMerge) {
        splitColumn = nColumn / 2;  // 从16x16转换为16x32，N维列数除以2
    } else if (!record.enNZ2ND) {
        splitColumn = nColumn;
    }
    return splitColumn;
}

void DBIParser::InitializeMovRecordParserFunctions()
{
    recordParserFunc_[RecordType::MOV_A5] =
        std::bind(&DBIParser::ParseMovA5Record, this, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
    recordParserFunc_[RecordType::MOV_V2_A5] =
        std::bind(&DBIParser::ParseMovV2Record, this, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
    recordParserFunc_[RecordType::DMA_MOV] =
        std::bind(&DBIParser::ParseDmaMovRecord, this, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
    recordParserFunc_[RecordType::MOV_ALIGN] =
        std::bind(&DBIParser::ParseMovAlignRecord, this, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
    recordParserFunc_[RecordType::MOV_ALIGN_V2] =
        std::bind(&DBIParser::ParseMovAlignV2Record, this, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
    recordParserFunc_[RecordType::DMA_MOV_ND2NZ] =
        std::bind(&DBIParser::ParseDmaMovNd2nzRecord, this, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
    recordParserFunc_[RecordType::MOV_FP] =
        std::bind(&DBIParser::ParseMovFpRecord, this, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
    recordParserFunc_[RecordType::FIX_L0C_TO_OUT] =
        std::bind(&DBIParser::ParseFixL0CGMRecord, this, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
    recordParserFunc_[RecordType::DMA_MOV_ND2NZ_DAV] =
        std::bind(&DBIParser::ParseDmaMovNd2nzDavRecord, this, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
    recordParserFunc_[RecordType::DMA_MOV_DN2NZ_D] =
        std::bind(&DBIParser::ParseDmaMovDn2nzDavRecord, this, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
}

void DBIParser::InitializeRecordParserFunctions()
{
    recordParserFunc_[RecordType::LOAD_L1_TO_L0B_2D_TRANSPOSE] =
        std::bind(&DBIParser::ParseLoad2DTransposeRecord, this, std::placeholders::_1, std::placeholders::_2,
        std::placeholders::_3, std::placeholders::_4);
    recordParserFunc_[RecordType::LOAD_2D] =
        std::bind(&DBIParser::ParseLoad2dRecord, this, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
    recordParserFunc_[RecordType::LOAD_2DV2] =
        std::bind(&DBIParser::ParseLoad2DV2Record, this, std::placeholders::_1, std::placeholders::_2,
        std::placeholders::_3, std::placeholders::_4);
    recordParserFunc_[RecordType::LOAD_2DV2_DEC] =
        std::bind(&DBIParser::ParseLoad2DV2DecRecord, this, std::placeholders::_1, std::placeholders::_2,
        std::placeholders::_3, std::placeholders::_4);
    recordParserFunc_[RecordType::LOAD_MX2DV2] =
        std::bind(&DBIParser::ParseLoadMX2DV2Record, this, std::placeholders::_1, std::placeholders::_2,
        std::placeholders::_3, std::placeholders::_4);
    recordParserFunc_[RecordType::SET_2D] =
        std::bind(&DBIParser::ParseSet2DRecord, this, std::placeholders::_1, std::placeholders::_2,
        std::placeholders::_3, std::placeholders::_4);
    recordParserFunc_[RecordType::ND_DMA_OUT_TO_UB] =
        std::bind(&DBIParser::ParseNdDMAOut2UbRecord, this, std::placeholders::_1, std::placeholders::_2,
        std::placeholders::_3, std::placeholders::_4);
}

DBIParser::DBIParser(std::string outputPath) : outputPath_(std::move(outputPath)),
    pool_(std::max(1u, std::min(DEFAULT_THREAD_NUM, std::thread::hardware_concurrency() / MAX_THREAD_USAGE_RATIO)))
{
    InitializeMovRecordParserFunctions();
    InitializeRecordParserFunctions();
}

bool DBIParser::ValidateFixL0CGMRecord(const FixL0CGMRecord &record, std::size_t key)
{
    // M = 0或N = 0或nd_number = 0表示不执行，该指令将被视为NOP
    if (record.mSize == 0 || record.nSize == 0 || (record.enNZ2ND && record.ndNum == 0)) {
        std::lock_guard<std::mutex> lock(parseMutex_);
        ++memoryChartMetrics_[key].invalidParam;  // 统计无效参数个数
        LogDebug("NOP FIX_L0C_TO_OUT with M/N/ndNum=0.");
        return false;
    }

    // channelSplit/int4ChannelMerge/int8ChannelMerge/enNZ2ND最多只能使能一个
    auto enableNum = static_cast<uint8_t>(record.int8ChannelMerge) +
                     static_cast<uint8_t>(record.int4ChannelMerge) +
                     static_cast<uint8_t>(record.channelSplit) +
                     static_cast<uint8_t>(record.enNZ2ND);
    if (enableNum > 1) {
        LogDebug("Don't parse FIX_L0C_TO_OUT with more than one of ChannelMerge/Split/NZ2ND enabled.");
        return false;
    }
    if ((record.srcStride % 16) != 0) {  // srcStride必须是16的倍数
        LogDebug("SrcStride must be multiples of 16 when parse FIX_L0C_TO_OUT.");
        return false;
    }
    if (!record.int4ChannelMerge && !record.channelSplit && !record.enNZ2ND) {
        // normal_movement或者int8ChannelMerge使能时，nSize必须是16的倍数
        if (record.nSize % 16 != 0) {
            LogDebug("NSize should be multiples of 16 when parse FIX_L0C_TO_OUT.");
            return false;
        }
    }
    if (record.int4ChannelMerge && (record.nSize % 64) != 0) {  // int4ChannelMerge使能时，nSize必须是64的倍数
        LogDebug("NSize should be multiples of 64 when parse FIX_L0C_TO_OUT with int4 channel merge.");
        return false;
    }
    if (record.channelSplit && (record.nSize % 8) != 0) {  // channelSplit使能时，nSize必须是8的倍数
        LogDebug("NSize should be multiples of 8 when parse FIX_L0C_TO_OUT with f32 channel split.");
        return false;
    }
    return true;
}

bool DBIParser::ParseFixL0CGMRecord(const std::string &buffer, std::size_t &index, std::size_t key, uint16_t blockId)
{
    FixL0CGMRecord record{};
    if (!CheckRecord(record, buffer, index, key)) {
        return false;
    }

    // 调用校验函数
    if (!ValidateFixL0CGMRecord(record, key)) {
        return false;
    }
    MemRecord memRec{};
    memRec.pc = record.pc;
    memRec.src = record.srcMemType;
    memRec.dst = record.dstMemType;
    memRec.blockId = blockId;
    // 向上取整, 16 is c0 base num
    uint16_t nColumn = AlignUp<16>(record.nSize);
    uint16_t splitColumn = GetSplitColumn(record, nColumn);
    FixPipeStoreToOut(record, memRec, nColumn, splitColumn, key);
    return true;
}

void DBIParser::WaitParseTask()
{
    {
        std::lock_guard<std::mutex> lock(parseMutex_);
        if (started_ != State::RUNNING) {
            return;
        }
        started_ = State::FINISHED;
    }
    pool_.WaitAllTasks();
    pool_.Stop();
    LogDebug("Handle all metrics left.");
    for (auto &p : memoryChartMetrics_) {
        VerifyAndDump(p.second);
    }
    memoryChartMetrics_.clear();
    uint16_t invalidPacketCount = invalidPacketCount_.load(std::memory_order_relaxed);
    uint64_t procedureErrorCount = procedureErrorCount_.load(std::memory_order_relaxed);
    if (invalidPacketCount != 0 || procedureErrorCount != 0) {
        LogDebug("MemoryChart data invalid packet count %hu, procedure error count %llu.",
                 invalidPacketCount, procedureErrorCount);
    }
}

void DBIParser::VerifyAndDump(const DBIParser::MemoryChartMetrics &metrics)
{
    metrics.PrintMetrics();
    if (!StartsWith(metrics.outputPath, outputPath_)) {
        ++invalidPacketCount_;
        LogDebug("MemoryChart data header's path is invalid, %s.", metrics.outputPath.c_str());
        return;
    }
    metrics.Dump();
}

void DBIParser::MemoryChartMetrics::PrintMetrics() const
{
    std::vector<uint32_t> errSum = {droppedRecord, memoryCopyFailed, invalidParam, overflow, invalidDataCount,
        memoryCorruption, invalidRecordType, outOfBoundsRead};
    if (std::any_of(errSum.begin(), errSum.end(), [](int x) { return x != 0; }) ||
        std::any_of(typeFailed.begin(), typeFailed.end(), [](int x) { return x != 0; })) {
        LogWarn("Some data of [MemoryChart] parse failed.");
    }

    std::stringstream processed;
    std::stringstream processFailed;
    processed << typeProcessed[static_cast<uint32_t>(Common::RecordType::DMA_MOV)];
    processFailed << typeFailed[static_cast<uint32_t>(Common::RecordType::DMA_MOV)];
    for (auto i = static_cast<uint32_t>(Common::RecordType::DMA_MOV) + 1;
        i < static_cast<uint32_t>(Common::RecordType::END); ++i) {
        processed << ", " << typeProcessed[i];
        processFailed << ", " << typeFailed[i];
    }
    LogDebug("processedRecord: %u, droppedRecord: %u, memoryCopyFailed %u, invalidParam %u, receivedPacket %hu, "
             "processedPacket %hu, overflow %hu, invalidDataCount %hhu, memoryCorruption %hhu, "
             "invalidRecordType %hhu, outOfBoundsRead %hhu, output num %lu, each record type process count %s, "
             "each record type process fail count %s",
             processedRecord, droppedRecord, memoryCopyFailed, invalidParam, receivedPacket,
             processedPacket, overflow, invalidDataCount, memoryCorruption,
             invalidRecordType, outOfBoundsRead, memoryRecords.size(), processed.str().c_str(),
             processFailed.str().c_str());
    LogDebug("Output path: %s.", outputPath.c_str());
}

void DBIParser::MemoryChartMetrics::Dump() const
{
    if (memoryRecords.empty() || outputPath.empty()) {
        return;
    }
    std::string binPath = JoinPath({outputPath, Common::MEMORY_CHART_BIN});
    LogDebug("Save MemoryChart data to %s.", binPath.c_str());
    if (IsExist(binPath)) {
        LogDebug("File %s already exist, will overwrite it.", binPath.c_str());
        RemoveAll(binPath);
    }
    WriteBinaryFile(binPath, reinterpret_cast<const char*>(memoryRecords.data()),
                    memoryRecords.size() * sizeof(Common::MemRecord));
}
}
