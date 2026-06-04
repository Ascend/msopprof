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


#ifndef MSOPT_DBI_DEFS_H
#define MSOPT_DBI_DEFS_H

#include <cstdint>
#include <map>
#include <string>
#include "include/opprof/DbiDefs.h"

namespace Common {
// memory chart桩使用
constexpr uint64_t BITS_EACH_BYTE = 8UL;

constexpr uint32_t DBI_RECORD_MAGIC_WORDS = 0x5a5a5a5a;
constexpr uint64_t DBI_RECORD_OVERFLOW_BIT = 1ULL << 63;

// 用于标识 set_atomic 系列接口的执行类型
enum class AtomicMode : uint8_t {
    NONE = 0,
    F32,
    F16,
    S16,
    S32,
    S8,
    BF16,
    SUM,
    MAX,
    MIN,
};

/// 此枚举定义与编译器内置类型 mem_t 保持一致
enum class MemType : uint8_t {
    L1 = 0,
    L0A,
    L0B,
    L0C,
    UB,
    BT,
    GM,
    REG,
    PRIVATE,
    INVALID,
};

// SIMT指令类型枚举，用于区分不同的SIMT指令
enum class SimtType : uint8_t {
    INVALID = 0,
    // Load/Store指令
    LD,          // 通用加载
    ST,          // 通用存储
    LDG,         // 全局内存加载
    STG,         // 全局内存存储
    LDS,         // 共享内存加载
    STS,         // 共享内存存储
    LDK,         // 常量内存加载
    STK,         // 常量内存存储
    // Atomic/Reduction指令
    ATOM,        // 原子操作
    RED,         // 归约操作
};

// 当前涉及的所有记录类型，每个类型根据接口行为不同定义自己的存储结构体。
enum class RecordType : uint32_t {
    INVALID = 0,
    MOV_A5,
    MOV_V2_A5,
    DMA_MOV,
    MOV_ALIGN,
    MOV_ALIGN_V2,
    LOAD_2D,
    DMA_MOV_ND2NZ,
    DMA_MOV_DN2NZ_D,
    DMA_MOV_ND2NZ_DAV,
    MOV_FP,
    LOAD_L1_TO_L0B_2D_TRANSPOSE,
    LOAD_2DV2,
    LOAD_2DV2_DEC,
    LOAD_MX2DV2,
    SET_2D,
    FIX_L0C_TO_OUT,
    ND_DMA_OUT_TO_UB,
    SIMT_LOAD_STORE,  // 合并所有SIMT Load/Store指令
    SIMT_ATOM_RED,    // 合并所有SIMT Atomic/Reduction指令
    END,
};

enum class DataType : uint8_t {
    DATA_B4 = 0,
    DATA_B8,
    DATA_B16,
    DATA_B32,
    DATA_F16,
    DATA_BF16,
    DATA_F32,
    DATA_S32,
    END,
};

enum class ByteMode : uint8_t {
    BM_DISABLE = 0,
    BM_ENABLE
};

/// 此枚举定义与编译器内置类型 pad_t 保持一致
enum class PadMode : uint8_t {
    PAD_NONE = 0, // 不启用 Padding 特性
    PAD_MODE1,    // 每 1B 填充 31 个 PadValue，PadValue 大小为 1B，1Block 的源数据会被扩充到 32Block
    PAD_MODE2,    // 每 2B 填充 15 个 PadValue，PadValue 大小为 2B，1Block 的源数据会被扩充到 16Block
    PAD_MODE3,    // 每 4B 填充 14 个 PadValue，PadValue 大小为 2B，1Block 的源数据会被扩充到 8Block
    PAD_MODE4,    // 每 8B 填充 12 个 PadValue，PadValue 大小为 2B，1Block 的源数据会被扩充到 4Block
    PAD_MODE5,    // 每 16B 填充 8 个 PadValue，PadValue 大小为 2B，1Block 的源数据会被扩充到 2Block
    PAD_MODE6,    // 每 32B 移除 14 个 PadValue，PadValue 大小为 2B，32B 的源数据会被缩小到 4B
    PAD_MODE7,    // 每 32B 移除 12 个 PadValue，PadValue 大小为 2B，32B 的源数据会被缩小到 8B
    PAD_MODE8,    // 每 32B 移除 8 个 PadValue，PadValue 大小为 2B，32B 的源数据会被缩小到 16B
};

enum class AddrCalMode : uint8_t {
    INC = 0,
    DEC = 1,
};

#pragma pack(4)
struct MemRecord {
    uint64_t srcAddr{};
    uint64_t dstAddr{};
    uint64_t srcMemSize{};
    uint64_t dstMemSize{};
    uint64_t pc{};
    uint64_t serialNo{};
    uint16_t blockId{};
    uint16_t subBlockID{};
    MemType src{MemType::INVALID};
    MemType dst{MemType::INVALID};
};

// OperandType需要与msopcom仓include/opprof/DbiDefs.h的OperandType保持一致
const std::map<OperandType, std::string> OperandTypeStrMap = {
    {OperandType::DATA_F16,          "F16"},
    {OperandType::DATA_F16X2,        "F16X2"},
    {OperandType::DATA_BF16,         "BF16"},
    {OperandType::DATA_BF16X2,       "BF16X2"},
    {OperandType::DATA_F32,          "F32"},
    {OperandType::DATA_E4M3,         "E4M3"},
    {OperandType::DATA_E5M2,         "E5M2"},
    {OperandType::DATA_E1M2,         "E1M2"},
    {OperandType::DATA_E2M1,         "E2M1"},
    {OperandType::DATA_V2BF16,       "V2BF16"},
    {OperandType::DATA_V2F16,        "V2F16"},
    {OperandType::DATA_HIF8X2,       "HIF8X2"},
    {OperandType::DATA_F8E4M3X2,     "F8E4M3X2"},
    {OperandType::DATA_F8E5M2X2,     "F8E5M2X2"},
    {OperandType::DATA_FMIX,         "FMIX"},
    {OperandType::DATA_HALF,         "HALF"},
    {OperandType::DATA_F32X2,        "F32X2"},
    {OperandType::DATA_B4,           "B4"},
    {OperandType::DATA_B8,           "B8"},
    {OperandType::DATA_B16,          "B16"},
    {OperandType::DATA_B32,          "B32"},
    {OperandType::DATA_B64,          "B64"},
    {OperandType::DATA_B128,         "B128"},
    {OperandType::DATA_S32,          "S32"},
    {OperandType::DATA_S16,          "S16"},
    {OperandType::DATA_U16,          "U16"},
    {OperandType::DATA_U32,          "U32"},
    {OperandType::DATA_S8,           "S8"},
    {OperandType::DATA_U8,           "U8"},
    {OperandType::DATA_U64,          "U64"},
    {OperandType::DATA_S64,          "S64"},
    {OperandType::DATA_SX32,         "SX32"},
    {OperandType::DATA_ZX32,         "ZX32"},
};

struct ThreadOperandRecords {
    OperandRecord records[static_cast<uint32_t>(OperandType::END)];
    uint64_t threadGap[8]{}; // SIMT_THREAD_GAP 64B
};

struct BlockOperandRecords {
    ThreadOperandRecords simdRecord;
    ThreadOperandRecords simtRecord[MAX_THREAD_NUM];
    uint64_t blockGap[8]{}; // BLOCK_GAP 64B
};

struct BlockWarpRecords {
    WarpHeader header;
    WarpRecord records[WARP_NUM_PER_BLOCK];
    uint64_t blockGap[8]{}; // BLOCK_GAP 64B
};

struct RecordHeader {
    RecordType recordType;
    uint32_t magicWords = DBI_RECORD_MAGIC_WORDS;
};

struct BaseRecord {
    uint64_t dst;
    uint64_t src;
    uint64_t pc;
    uint64_t recordId;  // prevent compiler optimization
};

struct MovRecord : BaseRecord {
    uint16_t subBlockID;
    uint16_t coreID;
    uint16_t nBurst;
    uint16_t lenBurst;
    uint16_t srcGap;
    uint16_t dstGap;
    MemType dstMemType;
    MemType srcMemType;
    DataType dataType;
    bool cvtEn;
};

struct MovV2Record : BaseRecord {
    uint32_t srcStride;
    uint32_t dstStride;
    uint32_t lenBurst;
    uint32_t nBurst;
    uint16_t coreID;
    uint8_t sid;
    MemType dstMemType;
    MemType srcMemType;
    DataType dataType;
    PadMode padMode;
};

// DMA_MOV： DMA 搬运记录结构体
struct DmaMovRecord : BaseRecord {
    uint16_t nBurst;
    uint16_t lenBurst;
    uint16_t srcStride;
    uint16_t dstStride;
    uint16_t coreID;
    MemType dstMemType;
    MemType srcMemType;
    PadMode padMode;
    ByteMode byteMode;
};

// MOV_ALIGN： Move Align 搬运记录结构体
struct MovAlignRecord : BaseRecord {
    uint32_t srcGap;
    uint32_t dstGap;
    uint32_t lenBurst;
    uint16_t nBurst;
    uint16_t coreID;
    MemType dstMemType;
    MemType srcMemType;
    DataType dataType;
    uint8_t leftPaddingNum;
    uint8_t rightPaddingNum;
};

struct MovAlignV2Record : BaseRecord {
    uint64_t dstStride;
    uint64_t srcStride;
    uint64_t loop1DstStride;
    uint64_t loop2DstStride;
    uint64_t loop1SrcStride;
    uint64_t loop2SrcStride;
    uint32_t loop1Size;
    uint32_t loop2Size;
    uint32_t lenBurst;
    uint16_t nBurst;
    uint16_t coreID;
    MemType dstMemType;
    MemType srcMemType;
    DataType dataType;
    uint8_t leftPaddingNum;
    uint8_t rightPaddingNum;
};

struct Load2DRecord : BaseRecord {
    uint16_t coreID;
    uint16_t baseIdx;
    uint16_t srcStride;
    uint16_t dstStride;
    uint8_t repeat;
    MemType dstMemType;
    MemType srcMemType;
    AddrCalMode addrCalMode;
};

// DMA_MOV_ND2NZ
struct DmaMovNd2nzRecord : BaseRecord {
    uint16_t ndNum;
    uint16_t nValue;
    uint16_t dValue;
    uint16_t srcNdMatrixStride;
    uint16_t srcDValue;
    uint16_t dstNzC0Stride;
    uint16_t dstNzNStride;
    uint16_t dstNzMatrixStride;
    uint16_t coreID;
    MemType srcMemType;
    MemType dstMemType;
    DataType dataType;
};

struct DmaMovNd2nzDavRecord : BaseRecord {
    uint64_t loop4SrcStride; // 40 bits src_nd_matrix_stride, in unit of byte
    uint64_t loop1SrcStride; // 40 bits src_D * sizeof(src_type), in unit of byte
    uint32_t dValue;         // 21 bits
    uint16_t nValue;         // 16 bits
    uint16_t ndNum;          // 16 bits
    uint16_t loop2DstStride; // 16 bits dst_nz_n_stride, in unit of C0_size
    uint16_t loop3DstStride; // 16 bits dst_nz_c0_stride, in unit of C0_size
    uint16_t loop4DstStride; // 16 bits, in unit of C0_size, dst_nz_matrix_stride * sizeof(dst_type) / C0_size
    uint16_t coreID;
    uint8_t smallC0;         // small C0 mode enable bit
    MemType srcMemType;
    MemType dstMemType;
    DataType dataType;
};

// MOV_FP
struct MovFpRecord : BaseRecord {
    uint32_t dstStride;
    uint16_t srcStride;
    uint16_t nSize;
    uint16_t mSize;
    uint16_t coreID;
    uint16_t ndNum;
    uint16_t dstNdStride;
    uint16_t srcNdStride;
    uint16_t quantPreBits;
    bool enUnitFlag;
    bool int8ChannelMerge;
    bool int4ChannelMerge;
    bool channelSplit;
    bool enNZ2ND;
};

struct Load2DTransposeRecord : BaseRecord {
    uint16_t coreID;
    uint16_t baseIdx;
    uint16_t dstStride;
    uint16_t srcStride;
    uint8_t repeat;
    MemType dstMemType;
    MemType srcMemType;
    DataType dataType;
};

struct Load2DV2Record : BaseRecord {
    uint16_t coreID;
    uint16_t mStartPos;
    uint16_t kStartPos;
    uint16_t dstStride;
    uint16_t srcStride;
    uint8_t mStep;
    uint8_t kStep;
    MemType dstMemType;
    MemType srcMemType;
    DataType dataType;
    bool transpose;
};

struct Load2DV2DecRecord : BaseRecord {
    uint16_t coreID;
    uint16_t mStartPos;
    uint16_t kStartPos;
    uint16_t dstStride;
    uint16_t srcStride;
    uint8_t mStep;
    uint8_t kStep;
    uint8_t decompMode;
    MemType dstMemType;
    MemType srcMemType;
};

struct LoadMX2DV2Record : BaseRecord {
    uint16_t coreID;
    uint16_t xStartPos;
    uint16_t yStartPos;
    uint16_t dstStride;
    uint16_t srcStride;
    uint8_t xStep;
    uint8_t yStep;
    MemType dstMemType;
    MemType srcMemType;
};

struct Set2DRecord : BaseRecord {
    uint16_t coreID;
    uint16_t repeat;
    uint16_t repeatGap;
    uint16_t blockNum;
    MemType dstMemType;
    MemType srcMemType;
    DataType dataType;
};

struct FixL0CGMRecord : BaseRecord {
    uint32_t dstStride;
    uint32_t srcStride;
    uint16_t nSize;
    uint16_t mSize;
    uint16_t coreID;
    uint16_t ndNum;
    uint64_t srcNzC0Stride;
    uint16_t dstNdStride;
    uint16_t srcNdStride;
    uint16_t quantPreBits;
    bool enUnitFlag;
    bool int8ChannelMerge;
    bool int4ChannelMerge;
    bool channelSplit;
    bool enNZ2ND;
    bool enNZ2DN;
    MemType dstMemType;
    MemType srcMemType;
    DataType dataType;
};

struct LoadStoreRecord : BaseRecord {
    uint64_t reg;
    uint64_t addr;
    uint64_t size;
    uint16_t coreID;
    MemType memType;
    SimtType simtType;
    OperandType dataType;
};

struct AtomRedRecord : BaseRecord {
    uint64_t size;
    uint16_t coreID;
    MemType memType;
    uint64_t addr;
    SimtType simtType;
    OperandType dataType;
};

// 依据指令手册该章节的算法3/4，src/dst_stride * element_size才是偏移内存
struct LoopInfo {
    bool operator==(const LoopInfo &r) const
    {
        return
            loopSrcStride == r.loopSrcStride &&
            loopDstStride == r.loopDstStride &&
            loopSize == r.loopSize &&
            loopLpSize == r.loopLpSize &&
            loopRpSize == r.loopRpSize;
    }

    uint64_t loopSrcStride; // 40 bits, src stride of each element
    uint32_t loopDstStride; // 20 bits, dst stride of each element, also works for Lp/Rp data
    uint32_t loopSize;      // 20 bits, src element number of this loop(without padding)
    uint8_t loopLpSize;     // 8 bits, left padding data count
    uint8_t loopRpSize;     // 8 bits, right padding data count
};

struct NdDMAOut2UbRecord : BaseRecord {
    static constexpr uint64_t LOOP = 5;
    LoopInfo loop[LOOP];
    uint16_t coreID;
    DataType dataType;
};

#pragma pack()

} // namespace Common
#endif // MSOPT_DBI_DEFS_H
