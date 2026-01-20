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


#ifndef DATA_VISUALIZE_CONST_H
#define DATA_VISUALIZE_CONST_H

#include <string>
#include <vector>
#include <map>
#include "common/defs.h"
#include "common/dbi_defs.h"

namespace Visualize {

enum class Event : uint64_t {
    VEC_INSTR = 1,
    CUBE_INSTR = 3,
    MTE1_INSTR = 4,
    MTE2_INSTR = 5,
    MTE3_INSTR = 6,
    VEC_CYCLES = 8,
    SCALAR_CYCLES = 9,
 
    MTE2_CYCLES = 12,
    MTE3_CYCLES = 13,
 
    MAIN_MEM_REQ = 19,
    MTE_TO_L0A_REQ = 28,
    L0B_READ_REQ = 33,
    MTE_TO_L0B_REQ = 34,
    LOC_TO_VEC_REQ = 44,
    L1_TO_MTE_REQ = 49,
    MTE_TO_L1_REQ = 50,
    UB_TO_MTE_REQ = 61,
    MTE_TO_UB_REQ = 62,
    UB_TO_VEC_REQ = 67,
    VEC_TO_UB_REQ = 68,

    CUBE_FP_EXECUTED = 73,
    CUBE_INT_EXECUTED = 74,
    VEC_FP32_EXECUTED = 75,
    VEC_FP16_EXECUTED_128 = 76,
    VEC_FP16_EXECUTED_64 = 77,
    VEC_S32_EXECUTED = 78,
    VEC_MISC_EXECUTED = 79,
 
    ICACHE_REQ = 84,
    ICACHE_MISS = 85,
    SCALAR_WAIT_CYCLES = 87,
    CUBE_WAIT_CYCLES = 88,
    VEC_WAIT_CYCLES = 89,
 
    MTE1_WAIT_CYCLES = 90,
    MTE2_WAIT_CYCLES = 91,
    MTE3_WAIT_CYCLES = 92,
 
    BANK_GROUP_CONFLICT = 100,
    BANK_CONFLICT = 101,
    VALU_RESOURCE_CONFLICT = 102,
    MTE_URGENT_REQUEST = 103,
 
    VEC_FP16_EXECUTED_32 = 174,
    FUSION_INSTR = 184,
    PARA_INSTR = 185,
    VEC_S16_EXECUTED = 186,
 
    // 310P max pmu id is 197, The remaining value is the content of 910B.

    FIXP_TO_L1_REQ = 518,
    L0C_TO_FIXP_REQ = 524,
    WRITE_HIT = 1280,
    WRITE_MISS = 1282,
    READ_HIT_R0 = 1284,
    READ_MISS_R0 = 1286,
    READ_HIT_R1 = 1288,
    READ_MISS_R1 = 1290,
 
    FP_MMAD_INSTR = 1032,
    INT_MMAD_INSTR = 1033,
 
    WRITE_DATA = 1292, // number of write data sent (128B word granularity)
    READ_DATA_R0 = 1293,
    READ_DATA_R1 = 1294,

    // A5 pmu
    READ_DATA_RECEIVED = 1058,
    WRITE_DATA_SENT = 1059,
    AR_CLOSE_L2_HIT_CORE = 1060,
    AR_CLOSE_L2_MISS_CORE = 1061,
    AR_CLOSE_L2_VICTIM_CORE = 1062,
    AR_FAR_L2_HIT_CORE = 1063,
    AR_FAR_L2_MISS_CORE = 1064,
    AR_FAR_L2_VICTIM_CORE = 1065,
    AW_CLOSE_L2_HIT_CORE = 1066,
    AW_CLOSE_L2_MISS_CORE = 1067,
    AW_CLOSE_L2_VICTIM_CORE = 1068,
    AW_FAR_L2_HIT_CORE = 1069,
    AW_FAR_L2_MISS_CORE = 1070,
    AW_FAR_L2_VICTIM_CORE = 1071,

    DCU_REQ_STG = 1397,
    DCU_REQ_STK = 1398,
    DCU_MISS_LDG = 1407,
    DCU_MISS_LDK = 1408,
    WR_L0A_INSTR = 1795,
    WR_L0B_INSTR = 1797,
    RD_L1_INSTR = 1799,
    WR_L1_INSTR = 1801,
    FIXP_WR_UB_INSTR = 1804,
    FIXP_WR_L1_INSTR = 1806,
    FIXP_WR_UB1_INSTR = 1815,
};

const std::map<std::string, int> FREQ_MAP {
    {"Ascend910B1",       1850},
    {"Ascend910B2",       1800},
    {"Ascend910B3",       1800},
    {"Ascend910B4",       1650},
    {"Ascend910B4-1",     1650}
};

// unit is TB/s
const std::map<std::string, float> THEORY_BW_GM {
    {"Ascend910B1",       1.8},
    {"Ascend910B2",       1.75},
    {"Ascend910B3",       1.75},
    {"Ascend910B4",       0.8},
    {"Ascend910B4-1",     0.8}
};

const std::map<std::string, float> THEORY_BW_GM_CJ {
    {"Ascend910B1",       1.34},
    {"Ascend910B2",       1.3},
    {"Ascend910B3",       1.3},
    {"Ascend910B4",       1.25},
    {"Ascend910B4-1",     1.25}
};

const std::map<Common::GmType, std::map<std::string, float>> GM_PRODUCT_THEORY_BW = {
    {Common::GmType::CJ, THEORY_BW_GM_CJ},
    {Common::GmType::SK, THEORY_BW_GM},
    {Common::GmType::SS, THEORY_BW_GM},
    {Common::GmType::DEFAULT, THEORY_BW_GM}
};

// unit is TB/s
const std::map<std::string, float> L2CACHE_THEORY_BW {
    {"Ascend910B1",       8},
    {"Ascend910B2",       7.78},
    {"Ascend910B3",       7.78},
    {"Ascend910B4",       4},
    {"Ascend910B4-1",     4}
};

const std::map<std::string, float> L2CACHE_THEORY_BW_GM_CJ {
    {"Ascend910B1",       7.4},
    {"Ascend910B2",       7.2},
    {"Ascend910B3",       7.2},
    {"Ascend910B4",       6.3},
    {"Ascend910B4-1",     6.3}
};

const std::map<Common::GmType, std::map<std::string, float>> GM_PRODUCT_THEORY_L2CACHE_BW = {
    {Common::GmType::CJ, L2CACHE_THEORY_BW_GM_CJ},
    {Common::GmType::SK, L2CACHE_THEORY_BW},
    {Common::GmType::SS, L2CACHE_THEORY_BW},
    {Common::GmType::DEFAULT, L2CACHE_THEORY_BW}
};

// memory unit roofline name
struct MemoryUnit {
    static constexpr char const *L1_TOTAL = "L1 Read + Write";
    static constexpr char const *WRITE_TO_L1 = "Write to L1";
    static constexpr char const *READ_FROM_L1 = "Read from L1";
    static constexpr char const *WRITE_TO_L0A = "Write to L0A";
    static constexpr char const *WRITE_TO_L0B = "Write to L0B";
    static constexpr char const *READ_FROM_L0C = "Read from L0C";

    static constexpr char const *UB_TOTAL = "UB Read + Write";
    static constexpr char const *READ_FROM_UB = "Read from UB";
    static constexpr char const *WRITE_TO_UB = "Write to UB";
    static constexpr char const *VECTOR_READ_UB = "Vector Read UB";
    static constexpr char const *VECTOR_WRITE_UB = "Vector Write UB";
    static constexpr char const *SIMT_VF = "SIMT VF";
};

// GM/L2 unit roofline name
struct GmAndL2cacheUnit {
    static constexpr char const *GM = "GM Read + Write";
    static constexpr char const *L2_CACHE = "L2 Read + Write";
};

// memory unit roofline name
struct MemoryPipe {
    static constexpr char const *L1_TO_GM = "L1 to GM";
    static constexpr char const *L0C_TO_GM = "L0C to GM";
    static constexpr char const *L0C_TO_L1 = "L0C to L1";
    static constexpr char const *GM_L1_TO_L0A = "GM/L1 to L0A";
    static constexpr char const *GM_L1_TO_L0B = "GM/L1 to L0B";

    static constexpr char const *UB_TO_GM = "UB to GM";
    static constexpr char const *GM_TO_UB = "GM to UB";
};

struct BoundType {
    static constexpr char const *MEMORY_BOUND = "memory bound";
    static constexpr char const *COMPUTE_BOUND = "compute bound";
    static constexpr char const *LATENCY_BOUND_PIPELINE_CAUSED = "latency bound:pipeline caused";
    static constexpr char const *LATENCY_BOUND_COMPUTE_CAUSED = "latency bound:compute caused";
    static constexpr char const *LATENCY_BOUND_MEMORY_CAUSED = "latency bound:memory caused";
    static constexpr char const *NONE_BOUND = "none bound";
};

const std::map<std::string, float> MEMORY_PIPE_MAX_BW_RATE_910B1_GM = {
    {std::string(MemoryPipe::L1_TO_GM), Common::MAX_BW_910B1.at(std::string {Common::L1_TO_GM})},
    {std::string(MemoryPipe::L0C_TO_GM), Common::MAX_BW_910B1.at(std::string {Common::L0C_TO_GM})},
    {std::string(MemoryPipe::L0C_TO_L1), Common::MAX_BW_910B1.at(std::string {Common::L0C_TO_L1})},
    {std::string(MemoryPipe::GM_L1_TO_L0A), 437.5},
    {std::string(MemoryPipe::GM_L1_TO_L0B), 210.5},
    {std::string(MemoryPipe::UB_TO_GM), Common::MAX_BW_910B1.at(std::string {Common::UB_TO_GM})},
    {std::string(MemoryPipe::GM_TO_UB), Common::MAX_BW_910B1.at(std::string {Common::GM_TO_UB})}
};

const std::map<std::string, float> MEMORY_PIPE_MAX_BW_RATE_910B1_GM_CJ = {
    {std::string(MemoryPipe::L1_TO_GM), Common::MAX_BW_910B1_CJ.at(std::string {Common::L1_TO_GM})},
    {std::string(MemoryPipe::L0C_TO_GM),  Common::MAX_BW_910B1_CJ.at(std::string {Common::L0C_TO_GM})},
    {std::string(MemoryPipe::L0C_TO_L1),  Common::MAX_BW_910B1_CJ.at(std::string {Common::L0C_TO_L1})},
    {std::string(MemoryPipe::GM_L1_TO_L0A), 439.32},
    {std::string(MemoryPipe::GM_L1_TO_L0B), 220.10},
    {std::string(MemoryPipe::UB_TO_GM),  Common::MAX_BW_910B1_CJ.at(std::string {Common::UB_TO_GM})},
    {std::string(MemoryPipe::GM_TO_UB),  Common::MAX_BW_910B1_CJ.at(std::string {Common::GM_TO_UB})}
};

const std::map<std::string, float> MEMORY_PIPE_MAX_BW_RATE_910B4_GM = {
    {std::string(MemoryPipe::L1_TO_GM), Common::MAX_BW_910B4.at(std::string {Common::L1_TO_GM})},
    {std::string(MemoryPipe::L0C_TO_GM), Common::MAX_BW_910B4.at(std::string {Common::L0C_TO_GM})},
    {std::string(MemoryPipe::L0C_TO_L1), Common::MAX_BW_910B4.at(std::string {Common::L0C_TO_L1})},
    {std::string(MemoryPipe::GM_L1_TO_L0A), 368.2},
    {std::string(MemoryPipe::GM_L1_TO_L0B), 173.81},
    {std::string(MemoryPipe::UB_TO_GM), Common::MAX_BW_910B4.at(std::string {Common::UB_TO_GM})},
    {std::string(MemoryPipe::GM_TO_UB), Common::MAX_BW_910B4.at(std::string {Common::GM_TO_UB})}
};

const std::map<std::string, float> MEMORY_PIPE_MAX_BW_RATE_910B4_GM_CJ = {
    {std::string(MemoryPipe::L1_TO_GM), Common::MAX_BW_910B4.at(std::string {Common::L1_TO_GM})},
    {std::string(MemoryPipe::L0C_TO_GM), Common::MAX_BW_910B4.at(std::string {Common::L0C_TO_GM})},
    {std::string(MemoryPipe::L0C_TO_L1), Common::MAX_BW_910B4.at(std::string {Common::L0C_TO_L1})},
    {std::string(MemoryPipe::GM_L1_TO_L0A), 391.82},
    {std::string(MemoryPipe::GM_L1_TO_L0B), 196.3},
    {std::string(MemoryPipe::UB_TO_GM), Common::MAX_BW_910B4.at(std::string {Common::UB_TO_GM})},
    {std::string(MemoryPipe::GM_TO_UB), Common::MAX_BW_910B4.at(std::string {Common::GM_TO_UB})}
};

const std::map<std::string, std::map<std::string, float>> GM_PRODUCT_MAX_BW_REQ_CJ = {
    {"Ascend910B1", MEMORY_PIPE_MAX_BW_RATE_910B1_GM_CJ},
    {"Ascend910B4", MEMORY_PIPE_MAX_BW_RATE_910B4_GM_CJ},
    {"Ascend910B4-1", MEMORY_PIPE_MAX_BW_RATE_910B4_GM_CJ}
};

const std::map<std::string, std::map<std::string, float>> GM_PRODUCT_MAX_BW_REQ_DEFAULT = {
    {"Ascend910B1", MEMORY_PIPE_MAX_BW_RATE_910B1_GM},
    {"Ascend910B4", MEMORY_PIPE_MAX_BW_RATE_910B4_GM},
    {"Ascend910B4-1", MEMORY_PIPE_MAX_BW_RATE_910B4_GM},
};

const std::map<std::string, float> MEMORY_PIPE_MAX_BW_RATE_A5 = {
    {std::string(MemoryPipe::L1_TO_GM), Common::MAX_BW_A5.at(std::string {Common::L1_TO_GM})},
    {std::string(MemoryPipe::L0C_TO_GM), Common::MAX_BW_A5.at(std::string {Common::L0C_TO_GM})},
    {std::string(MemoryPipe::L0C_TO_L1), Common::MAX_BW_A5.at(std::string {Common::L0C_TO_L1})},
    {std::string(MemoryPipe::GM_L1_TO_L0A), 437.5},
    {std::string(MemoryPipe::GM_L1_TO_L0B), 210.5},
    {std::string(MemoryPipe::UB_TO_GM), Common::MAX_BW_A5.at(std::string {Common::UB_TO_GM})},
    {std::string(MemoryPipe::GM_TO_UB), Common::MAX_BW_A5.at(std::string {Common::GM_TO_UB})}
};

const std::map<std::string, std::map<std::string, float>> MAX_BW_REQ_A5 = {
    {"Ascend910_9599",   MEMORY_PIPE_MAX_BW_RATE_A5},
    {"Ascend910_9589",   MEMORY_PIPE_MAX_BW_RATE_A5},
    {"Ascend910_9581",   MEMORY_PIPE_MAX_BW_RATE_A5},
    {"Ascend910_958a",   MEMORY_PIPE_MAX_BW_RATE_A5},
    {"Ascend910_958b",   MEMORY_PIPE_MAX_BW_RATE_A5},
    {"Ascend910_9579",   MEMORY_PIPE_MAX_BW_RATE_A5},
    {"Ascend910_957b",   MEMORY_PIPE_MAX_BW_RATE_A5},
    {"Ascend910_957d",   MEMORY_PIPE_MAX_BW_RATE_A5},
    {"Ascend910_950z",   MEMORY_PIPE_MAX_BW_RATE_A5},
};

const std::map<Common::GmType, std::map<std::string, std::map<std::string, float>>> GM_PRODUCT_MAX_BW_REQ = {
    {Common::GmType::CJ, GM_PRODUCT_MAX_BW_REQ_CJ},
    {Common::GmType::SK, GM_PRODUCT_MAX_BW_REQ_DEFAULT},
    {Common::GmType::SS, GM_PRODUCT_MAX_BW_REQ_DEFAULT},
    {Common::GmType::DEFAULT, GM_PRODUCT_MAX_BW_REQ_DEFAULT}
};

// 以下结构体和枚举类为自定义数据类型
struct BlockDetail {
    uint16_t blockId;
    std::vector<float> duration;
};

enum class UnitType {
    US = 0,  // 微秒单位
    INSTR,   // 指令个数单位
    BYTE,    // 字节单位
    PER,     // 百分比单位
    OPS      // 操作数单位
};

using OperandRecordMap = std::map<Common::OperandType, Common::OperandRecord>;
struct TypeOperandRecord {
    OperandRecordMap simdMap;
    OperandRecordMap simtMap;
};

struct ComputeLoadBlockDetail {
    uint8_t blockId;
    std::string blockType;
    std::string opType;
    int64_t freq;
    uint64_t totalCycles;
    std::map<uint64_t, uint64_t> eventMap;
    TypeOperandRecord operandRecordMap;
};

struct DetailInfo {
    std::string name;
    UnitType type;
    uint8_t jsonType;
};

// 这里为A2\A3\A5\310的并集
enum class PipeAll {
    // 公共pipe
    GM_TO_L2 = 0,
    L2_TO_GM,
    // cube Pipe
    L2_TO_L1,
    L1_TO_L2,
    GM_OR_L1_TO_L0A,
    GM_OR_L1_TO_L0B = 5,
    L0A_TO_CUBE,
    L0B_TO_CUBE,
    CUBE_TO_L0C,
    L0C_TO_L1,
    L0C_TO_L2 = 10,
    L0C_TO_CUBE,
    // Vec Pipe
    L2_TO_UB,
    UB_TO_L2,
    UB_TO_VEC,
    VEC_TO_UB = 15,
    L2_TO_UB_2,
    UB_TO_L2_2,
    UB_TO_VEC_2,
    VEC_TO_UB_2,
    // 310P
    L2_OR_UB_TO_L1 = 20,
    L1_TO_L0A,
    L2_OR_L1_TO_L0B,
    VEC_TO_L0C,
    L0C_TO_VEC,
    L2_OR_L1_TO_UB = 25,
    // DBI task / A5
    GM_TO_L0A_AIC = 35,
    GM_TO_L0B_AIC,
    L1_TO_L0A_AIC,
    L1_TO_L0B_AIC,
    // A5
    L0C_TO_FIXP,
    FIXP_TO_L2,
    FIXP_TO_L1,
    FIXP_TO_UB,
    FIXP_TO_UB_2,
    L1_TO_UB,
    L1_TO_UB_2,
    UB_TO_L1,
    UB_TO_L1_2,
    // A5 simt
    L2_TO_DCACHE,
    L2_TO_DCACHE_2,
    DCACHE_TO_L2 = 50,
    DCACHE_TO_L2_2,
    VEC_TO_DCACHE,
    VEC_TO_DCACHE_2,
    DCACHE_TO_VEC,
    DCACHE_TO_VEC_2
};

struct MemInfoAiCore {
    uint64_t request;
    float throughput;
    std::string peak;
};

struct MemInfoCache {
    uint64_t hit;
    uint64_t miss;
    uint64_t total;
};

struct MemInfoPipe {
    uint64_t instr;
    uint64_t cycle;
    uint64_t waitCycle;
    std::string bandWidth;
    float activeRate;
};

struct MemMapDetail {
    uint8_t blockId;
    std::string blockType;
    std::string opType;
    std::string soc;
    int64_t freq;                   // current frequency
    uint64_t totalCycles;
    int64_t aiCoreNum;
    uint64_t blockDim;
    std::map<uint64_t, uint64_t> eventMap;
    std::map<uint64_t, uint64_t> eventMapVec0;
    std::map<uint64_t, uint64_t> eventMapVec1;
    std::map<std::string, uint64_t> cycMap;
    std::map<std::string, uint64_t> ApiDataTransVolume_;
};

// for MC2 timeline
struct AcsqTaskInfo {
    uint16_t taskType;
    uint64_t startTime;
    uint64_t endTime;
};
// AcsqTaskPairType  eg:<<taskId, streamId>, {taskType, startTime, endTime}>
using AcsqTaskPairType = std::pair<std::pair<uint16_t, uint16_t>, AcsqTaskInfo>;
// AcsqTimeMapType  eg:{{<taskId, streamId>, {taskType, startTime, endTime}}}
using AcsqTimeMapType = std::map<std::pair<uint16_t, uint16_t>, AcsqTaskInfo>;
// BlockSystemTimeType: {{blockId, {{startTime_cube0, endTime_cube0}, {startTime_vec0, endTime_vec0}}}}
using BlockSystemTimeType = std::map<uint16_t, std::vector<std::pair<uint64_t, uint64_t>>>;
constexpr uint16_t CUBE_BLOCK_START_INDEX = 50; // the blockId of AICore cube block dot data begin from 50
}
#endif // DATA_VISUALIZE_CONST_H
