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


#ifndef __MSOPPROF_PARSE_DATA_CALCULATOR_INSTR_DETAIL_DEFS_H__
#define __MSOPPROF_PARSE_DATA_CALCULATOR_INSTR_DETAIL_DEFS_H__

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace Profiling {
namespace Parse {

constexpr uint32_t UB_ALIGN_SIZE = 32U;
constexpr uint32_t VEC_STD_BLOCK_NUMBER = 8U;
constexpr uint32_t VEC_STD_BLOCK_SIZE = 32U;
constexpr uint32_t MAX_VEC_REPEAT_COUNT = 255U;
constexpr uint32_t MAX_VEC_PROCESSED_BYTES = MAX_VEC_REPEAT_COUNT * VEC_STD_BLOCK_NUMBER * VEC_STD_BLOCK_SIZE;

constexpr uint32_t BIT4 = 4;
constexpr uint32_t BIT8 = 8;
constexpr uint32_t BIT12 = 12;
constexpr uint32_t BIT16 = 16;
constexpr uint32_t BIT24 = 24;
constexpr uint32_t BIT32 = 32;
constexpr uint32_t BIT40 = 40;
constexpr uint32_t BIT52 = 52;
constexpr uint32_t BIT56 = 56;

struct MemOpInfo {
    bool isValid;
    uint64_t srcAddr;
    uint64_t dstAddr;
    // 由于内存操作的地址不一定连续,这里通过blockNum/blockSize/blockStride来描述一次内存操作的"length"
    uint64_t blockNum;
    uint64_t blockSize;
    uint64_t blockStride;
    uint64_t repeatTimes;
    uint64_t repeatStride;
};

enum class InstrDataType : uint8_t {
    S4 = 1,
    B4 = 1,
    U8 = 1,
    S8 = 1,
    B8 = 1,
    U16 = 2,
    S16 = 2,
    B16 = 2,
    F16 = 2,
    U32 = 4,
    S32 = 4,
    B32 = 4,
    B32S = 4,
    B32L = 4,
    F32 = 4,
    U64 = 8,
    S64 = 8,
    B64 = 8,
    F64 = 8,
    FMIX = 4,
    F16F16 = 2,
    F16F32 = 4,
    F16U2 = 2,
    U8S8 = 1,
    B8U2 = 1,
    DEQ = 1,
    BF16F32 = 4,
    F32F32 = 4,
    BF16 = 2,
    INVALID = 0,
};

struct BaseMemInfo {
    bool isValid;
    uint64_t addr;
    uint64_t blockNum;
    uint64_t blockSize;
    uint64_t blockStride;
    uint64_t repeatTimes;
    uint64_t repeatStride;
};

struct InstrDetailEvent {
    bool isValid;
    InstrDataType dType;
    uint8_t repeat;
    std::vector<BaseMemInfo> srcAddrList;
    std::vector<BaseMemInfo> dstAddrList;
};

enum class InstrMemType : uint8_t {
    L1 = 0,
    L0A,
    L0B,
    L0C,
    UB,
    BT,
    GM,
    PRIVATE,
    INVALID,
};

const std::unordered_map<std::string, InstrMemType> INSTR_MEM_TYPE_A2A3_MAP = {
    {"L1", InstrMemType::L1},
    {"L0A", InstrMemType::L0A},
    {"L0B", InstrMemType::L0B},
    {"L0C", InstrMemType::L0C},
    {"UB", InstrMemType::UB},
    {"BT", InstrMemType::BT},
    {"OUT", InstrMemType::GM},
};

struct InstrRegCommon {
    bool isValid;
    uint64_t src;
    uint64_t dst;
    uint64_t config;
};

struct InstrLoadImage {
    bool isValid;
    uint64_t dst;
    uint64_t xs;
    uint64_t xt;
};

struct InstrDmaMovNd2Nz {
    bool isValid;
    InstrDataType instrDetailDataType;
    uint64_t src;
    uint64_t dst;
    uint64_t xm;
    uint64_t xt;
};

struct InstrDmaMovAlign {
    bool isValid;
    InstrMemType instrMemType;
    uint64_t src;
    uint64_t dst;
    uint64_t xm;
    uint64_t xt;
};

struct InstrFixPipe {
    bool isValid;
    InstrDataType srcDataType;
    InstrMemType dstMemType;
    uint64_t src;
    uint64_t dst;
    uint64_t xm;
    uint64_t xt;
};

struct InstrVec {
    bool isValid;
    InstrDataType dType;
    uint64_t xd;
    uint64_t xn;
    uint64_t xm;
    uint64_t xt;
};


union InstrRegDetail {
    InstrRegCommon instrRegCommon;
    InstrLoadImage instrLoadImage;
    InstrDmaMovNd2Nz instrDmaMovNd2Nz;
    InstrDmaMovAlign instrDmaMovAlign;
    InstrFixPipe instrFixPipe;
    InstrVec instrVec;
};

enum class InstrTypeTemplate : uint32_t {
    // data move instructions template
    LOAD_SRC_TO_DST_2D,
    SET_DST_2D,
    LOAD_OUT_TO_DST_UNZIP,
    BRC_SRC_TO_DST,
    DECOMPRESS_OUT_TO_DST,
    SET_RAWHEADER_TO_OUT,
    DECOMPRESS_HEADER,
    UNZIP_INDEX,
    LOAD_OUT_TO_L1_IMAGE,
    COMPRESS_UB_TO_OUT,
    LOAD_L1_TO_L0A_WINOGRAD,
    LOAD_L1_TO_L0B_WINOGRAD,
    LOAD_L1_TO_DST_3D,
    LOAD_SRC_TO_L0B_B2,
    LOAD_SRC_TO_SMASK,
    COL2IMG,
    LOAD_L1_TO_DST_3DV2,
    MOV_SRC_TO_DST,
    HEBCD_OUT_TO_UB,
    HEBCE_SRC_TO_OUT,
    LOAD_L1_TO_L0A_WINOGRAD_V2,
    LOAD_L1_TO_L0B_WINOGRAD_V2,
    STORE_L1_TO_OUT_IMAGE,
    MOV_SRC_TO_DST_PAD,
    MVF_OUT_TO_UB,
    MVF_DCI,
    MOV_OUT_TO_L1_MULTI_ND2NZ,
    MOV_SRC_TO_DST_ALIGN,
    LOAD_L1_TO_DST_2D_TRANSPOSE,
    LOAD_L1_TO_L0B_2D_SP,
    FIX_L0C_TO_DST,
};

}
}

#endif // __MSOPPROF_PARSE_DATA_CALCULATOR_INSTR_DETAIL_DEFS_H__
