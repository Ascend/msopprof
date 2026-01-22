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

#ifndef __MSOPPROF_COMMON_DEFS_H__
#define __MSOPPROF_COMMON_DEFS_H__

#include <cstdint>
#include <map>
#include <vector>
#include <string>
#include <set>

namespace Common {

constexpr int RADIX = 8;
// MetricEventsMapType:{{metric name, pmu events vector}} eg:{{"memory", {4, 5, 6}}}
using MetricEventsMapType = std::map<std::string, std::vector<uint16_t>>;

/// 算子 profiling 数据获取模式
enum class OpProfileMode : uint32_t {
    ON_BOARD = 0,
    SIMULATOR
};

struct Enc128 {
    uint64_t low64;
    uint64_t high64;
    bool operator==(const Enc128 &v) const
    {
        return low64 == v.low64 && high64 == v.high64;
    }
    Enc128 operator&(const Enc128 &v) const
    {
        return Enc128{ low64 & v.low64, high64 & v.high64 };
    }
};

struct Enc128Hash {
    size_t operator()(const Enc128 &v) const
    {
        constexpr uint64_t prime = 0x9e3779b97f4a7c15; // 一个大的质数，用于减少哈希冲突的概率
        return std::hash<uint64_t>{}(v.low64) * prime + std::hash<uint64_t>{}(v.high64);
    }
};

/// profiling 数据采集类型
/// RESOURCE_CONFLICT_RATIO 也用于仿真关闭SET/WAIT_FLAG指令
enum class ProfMetrics : uint32_t {
    PIPE_UTILIZATION = 0,
    ARITHMETIC_UTILIZATION,
    L2_CACHE,
    MEMORY,
    RESOURCE_CONFLICT_RATIO,
    MEMORY_L0,
    MEMORY_UB,
    OCCUPANCY,
    ROOFLINE,
    COUNT
};

const std::set<std::string> SOC_910B = {
    "Ascend910B1", "Ascend910B2", "Ascend910B3", "Ascend910B4", "Ascend910B2C", "Ascend910B4-1"
};

const std::set<std::string> SOC_910_93 = {
    "Ascend910_9391", "Ascend910_9392", "Ascend910_9381", "Ascend910_9382",
    "Ascend910_9372", "Ascend910_9362"
};

const std::set<std::string> SOC_310B = {
    "Ascend310B1", "Ascend310B2", "Ascend310B3", "Ascend310B4"
};

const std::set<std::string> SOC_310P = {
    "Ascend310P1", "Ascend310P2", "Ascend310P3", "Ascend310P4",  "Ascend310P5", "Ascend310P7"
};

/// 运行芯片平台类型
enum class ChipType : uint32_t {
    ASCEND310 = 0,
    ASCEND910A,
    ASCEND610,
    ASCEND615,
    ASCEND310P,
    ASCEND910B,
    ASCEND310B = 7,
    ASCEND910_95 = 15,
    END_TYPE
};

struct DvcMteLog {
    uint64_t time;
    uint64_t size;
    uint64_t instrId;
    uint32_t coreId;
    uint32_t reqId;
    char intf[32];
};

struct DvciCacheLog {
    uint64_t time;
    uint64_t addr;
    uint32_t coreId;
    uint32_t subCoreId;
    uint32_t size;
    uint32_t type;
    uint8_t last;
};

struct DvcInstrLog {
    uint64_t time;
    uint64_t pc;
    uint32_t coreId;
    uint32_t subCoreId;
    char decodeDescr[200];
    char execDescr[200];
};

enum class ChipProductType : uint32_t {
    ALL_PRODUCT_TYPE = 0,
    UNKNOWN_PRODUCT_TYPE = 1,

    ASCEND310P_SERIES = 1 << RADIX, // ASCEND310P series product type
    ASCEND310P1,
    ASCEND310P2,
    ASCEND310P3,
    ASCEND310P4,
    ASCEND310P5,
    ASCEND310P7,

    ASCEND310B_SERIES = 2 << RADIX, // ASCEND310B series product type
    ASCEND310B1,
    ASCEND310B2,
    ASCEND310B3,
    ASCEND310B4,

    ASCEND910B_SERIES = 3 << RADIX, // ASCEND910B series product type
    ASCEND910B1,
    ASCEND910B2,
    ASCEND910B3,
    ASCEND910B4,
    ASCEND910B2C,
    ASCEND910B4_1,

    ASCEND910_93_SERIES = 4 << RADIX, // ASCEND910_93 series product type
    ASCEND910_9391,
    ASCEND910_9392,
    ASCEND910_9381,
    ASCEND910_9382,
    ASCEND910_9372,
    ASCEND910_9362,

    ASCEND310_SERIES = 5 << RADIX,
    ASCEND310,

    ASCEND910A_SERIES = 6 << RADIX,
    ASCEND910A,
    ASCEND910B,
    ASCEND910PROA,
    ASCEND910PROB,
    ASCEND910PREMIUMA,

    ASCEND610_SERIES = 7 << RADIX,
    ASCEND615_SERIES = 8 << RADIX,

    ASCEND910_95_SERIES = 9 << RADIX,
    ASCEND910_950X,
    ASCEND910_950Y,
    ASCEND910_950Z,
    ASCEND910_9571,
    ASCEND910_9572,
    ASCEND910_9573,
    ASCEND910_9574,
    ASCEND910_9575,
    ASCEND910_9576,
    ASCEND910_9577,
    ASCEND910_9578,
    ASCEND910_9579,
    ASCEND910_957B,
    ASCEND910_957C,
    ASCEND910_957D,
    ASCEND910_9581,
    ASCEND910_9582,
    ASCEND910_9583,
    ASCEND910_9584,
    ASCEND910_9585,
    ASCEND910_9586,
    ASCEND910_9587,
    ASCEND910_9588,
    ASCEND910_9589,
    ASCEND910_958A,
    ASCEND910_958B,
    ASCEND910_9591,
    ASCEND910_9592,
    ASCEND910_9595,
    ASCEND910_9596,
    ASCEND910_9599,
};

const std::map<std::string, ChipProductType> SOC_STRING_TO_CHIP_PRODUCT{
    {"Ascend910B1",     ChipProductType::ASCEND910B1},
    {"Ascend910B2",     ChipProductType::ASCEND910B2},
    {"Ascend910B3",     ChipProductType::ASCEND910B3},
    {"Ascend910B4",     ChipProductType::ASCEND910B4},
    {"Ascend910B2C",    ChipProductType::ASCEND910B2C},
    {"Ascend910B4-1",   ChipProductType::ASCEND910B4_1},

    {"Ascend910_9391",  ChipProductType::ASCEND910_9391},
    {"Ascend910_9392",  ChipProductType::ASCEND910_9392},
    {"Ascend910_9381",  ChipProductType::ASCEND910_9381},
    {"Ascend910_9382",  ChipProductType::ASCEND910_9382},
    {"Ascend910_9372",  ChipProductType::ASCEND910_9372},
    {"Ascend910_9362",  ChipProductType::ASCEND910_9362},

    {"Ascend310B1",     ChipProductType::ASCEND310B1},
    {"Ascend310B2",     ChipProductType::ASCEND310B2},
    {"Ascend310B3",     ChipProductType::ASCEND310B3},
    {"Ascend310B4",     ChipProductType::ASCEND310B4},

    {"Ascend310P1",     ChipProductType::ASCEND310P1},
    {"Ascend310P2",     ChipProductType::ASCEND310P2},
    {"Ascend310P3",     ChipProductType::ASCEND310P3},
    {"Ascend310P4",     ChipProductType::ASCEND310P4},
    {"Ascend310P5",     ChipProductType::ASCEND310P5},
    {"Ascend310P7",     ChipProductType::ASCEND310P7},

    {"Ascend310",       ChipProductType::ASCEND310},
 
    {"Ascend910A",      ChipProductType::ASCEND910A},
    {"Ascend910B",      ChipProductType::ASCEND910B},
    {"Ascend910ProA",   ChipProductType::ASCEND910PROA},
    {"Ascend910ProB",   ChipProductType::ASCEND910PROB},
    {"AscendPremiumA",  ChipProductType::ASCEND910PREMIUMA},
    
    {"Ascend910_950x",  ChipProductType::ASCEND910_950X},
    {"Ascend910_950y",  ChipProductType::ASCEND910_950Y},
    {"Ascend910_950z",  ChipProductType::ASCEND910_950Z},
    {"Ascend910_9571",  ChipProductType::ASCEND910_9571},
    {"Ascend910_9572",  ChipProductType::ASCEND910_9572},
    {"Ascend910_9573",  ChipProductType::ASCEND910_9573},
    {"Ascend910_9574",  ChipProductType::ASCEND910_9574},
    {"Ascend910_9575",  ChipProductType::ASCEND910_9575},
    {"Ascend910_9576",  ChipProductType::ASCEND910_9576},
    {"Ascend910_9577",  ChipProductType::ASCEND910_9577},
    {"Ascend910_9578",  ChipProductType::ASCEND910_9578},
    {"Ascend910_9579",  ChipProductType::ASCEND910_9579},
    {"Ascend910_957b",  ChipProductType::ASCEND910_957B},
    {"Ascend910_957c",  ChipProductType::ASCEND910_957C},
    {"Ascend910_957d",  ChipProductType::ASCEND910_957D},
    {"Ascend910_9581",  ChipProductType::ASCEND910_9581},
    {"Ascend910_9582",  ChipProductType::ASCEND910_9582},
    {"Ascend910_9583",  ChipProductType::ASCEND910_9583},
    {"Ascend910_9584",  ChipProductType::ASCEND910_9584},
    {"Ascend910_9585",  ChipProductType::ASCEND910_9585},
    {"Ascend910_9586",  ChipProductType::ASCEND910_9586},
    {"Ascend910_9587",  ChipProductType::ASCEND910_9587},
    {"Ascend910_9588",  ChipProductType::ASCEND910_9588},
    {"Ascend910_9589",  ChipProductType::ASCEND910_9589},
    {"Ascend910_958a",  ChipProductType::ASCEND910_958A},
    {"Ascend910_958b",  ChipProductType::ASCEND910_958B},
    {"Ascend910_9591",  ChipProductType::ASCEND910_9591},
    {"Ascend910_9592",  ChipProductType::ASCEND910_9592},
    {"Ascend910_9595",  ChipProductType::ASCEND910_9595},
    {"Ascend910_9596",  ChipProductType::ASCEND910_9596},
    {"Ascend910_9599",  ChipProductType::ASCEND910_9599},
};

const std::map<ChipType, ChipProductType> CHIP_ARCHITECTURE_TO_PRODUCT_SERIES{
    {ChipType::ASCEND310,   ChipProductType::ASCEND310_SERIES},
    {ChipType::ASCEND910A,  ChipProductType::ASCEND910A_SERIES},
    {ChipType::ASCEND610,   ChipProductType::ASCEND610_SERIES},
    {ChipType::ASCEND615,   ChipProductType::ASCEND615_SERIES},
    {ChipType::ASCEND310P,  ChipProductType::ASCEND310P_SERIES},
    {ChipType::ASCEND910B,  ChipProductType::ASCEND910B_SERIES},
    {ChipType::ASCEND310B,   ChipProductType::ASCEND310B_SERIES},
    {ChipType::ASCEND910_95, ChipProductType::ASCEND910_95_SERIES}
};

inline ChipProductType GetProductSeriesType(const ChipProductType& chipProductType)
{
    return static_cast<ChipProductType>((static_cast<uint32_t>(chipProductType) >> RADIX) << RADIX);
}

inline bool IsChipSeriesTypeValid(const ChipProductType& inputType, const ChipProductType& requiredType)
{
    if (requiredType == ChipProductType::ALL_PRODUCT_TYPE) {
        return true;
    }
    return GetProductSeriesType(inputType) == requiredType;
}

inline ChipProductType GetProductSeriesTypeBySocVersion(const std::string &socVersion)
{
    auto it = Common::SOC_STRING_TO_CHIP_PRODUCT.find(socVersion);
    Common::ChipProductType chipType = it == Common::SOC_STRING_TO_CHIP_PRODUCT.end() ?
        Common::ChipProductType::UNKNOWN_PRODUCT_TYPE : it->second;
    return chipType;
}

/// 时间类型
enum class TimeType : uint16_t {
    START = 0,
    END,
    OTHERS
};

/// 上板 profiling 输出的.bin文件名前缀
constexpr char const *MSPROF_DUMPFILE_PREFIX = "DeviceProf";
/// 上板 profiling 空指标值
constexpr char const *EMPTY_METRIC_VALUE = "NA";
constexpr uint64_t EMPTY_PMU_VALUE = UINT64_MAX;
constexpr char const *AICORE_KERNEL_NAME = "aicore_binary.o";
constexpr char const *CHECK_DUMP_FILE = "^\\S*\\.dump(\\.[0-9]{1})?$";  // 匹配dump文件 例如 .dump.0 .dump.1
constexpr char const *TMP_DUMP = "tmp_dump";
constexpr char const *DUMP = "dump";
constexpr char const *TLV_DATA = "lrm.bin";
/// 上板动态插桩输出件
constexpr char const *MEMORY_CHART_BIN = "MemoryChart.bin";

/// 上板 profiling 数据输出目录
struct MsprofMetrics {
    static constexpr char const *PIPE_UTILIZATION = "pipeutilization";
    static constexpr char const *ARITHMETIC_UTILIZATION = "arithmeticutilization";
    static constexpr char const *L2_CACHE = "l2cache";
    static constexpr char const *MEMORY = "memory";
    static constexpr char const *MEMORY_L0 = "memoryl0";
    static constexpr char const *MEMORY_UB = "memoryub";
    static constexpr char const *RESOURCE_CONFLICT_RATIO = "resourceconflictratio";
    static constexpr char const *DEFAULT = "default";
    static constexpr char const *KERNEL_SCALE = "kernelscale";
    static constexpr char const *OCCUPANCY = "occupancy";
    static constexpr char const *TIMELINE_DETAIL = "timelinedetail";
    static constexpr char const *ROOFLINE = "roofline";
    static constexpr char const *TIMELINE = "timeline";
    static constexpr char const *PCSAMPLING = "pcsampling";
    static constexpr char const *BASIC_INFO = "basicinfo";
    static constexpr char const *SOURCE = "source";
    static constexpr char const *MEMORYDETAIL = "memorydetail";
    static constexpr char const *PMSAMPLING = "pmsampling";
};

const std::map<const char*, const char*> METRICS_CSV_MAP {
    {MsprofMetrics::PIPE_UTILIZATION,           "PipeUtilization"},
    {MsprofMetrics::ARITHMETIC_UTILIZATION,     "ArithmeticUtilization"},
    {MsprofMetrics::L2_CACHE,                   "L2Cache"},
    {MsprofMetrics::MEMORY,                     "Memory"},
    {MsprofMetrics::MEMORY_L0,                  "MemoryL0"},
    {MsprofMetrics::MEMORY_UB,                  "MemoryUB"},
    {MsprofMetrics::RESOURCE_CONFLICT_RATIO,    "ResourceConflictRatio"},
};

struct Path {
    static constexpr char const *MSPROF_PATH_FROM_CANN = "/bin/msprof";
    static constexpr char const *MSOPPROF_INJECTION_LIB_PATH_FROM_MSOPPROF = "lib64/libmsopprof_injection.so";
    static constexpr char const *KERNEL_LAUNCHER_PATH_FROM_MSOPPROF = "bin/kernel-launcher";
    static constexpr char const *SIMULATOR_PATH_FROM_CANN = "/tools/simulator";
    static constexpr char const *MSOPPROF_DIR_PREFIX = "/OPPROF";
};

struct OpRunnerMode {
    static constexpr char const *EXECUTE_BINARY = "ExecBinary";
    static constexpr char const *RUN_KERNEL = "RunKernel";
};

struct OpType {
    static constexpr char const *VECTOR = "vector";
    static constexpr char const *CUBE = "cube";
    static constexpr char const *MIX = "mix";
    // default op type for 310P
    static constexpr char const *AI_CORE = "AiCore";
};

const MetricEventsMapType AIC_EVENTS_FOR_910B = {
    {std::string(Common::MsprofMetrics::PIPE_UTILIZATION),        {9, 10, 12, 13, 84, 85, 87, 770, 771}},
    {std::string(Common::MsprofMetrics::ARITHMETIC_UTILIZATION),  {3, 10, 73, 74, 1032, 1033}},
    {std::string(Common::MsprofMetrics::L2_CACHE),                {1280, 1282, 1283, 1284, 1286, 1287, 1288, 1290, 1291}},
    {std::string(Common::MsprofMetrics::MEMORY),                  {4, 5, 6, 12, 13, 19, 49, 50, 518, 524, 526,
                                                                   770, 1292, 1293, 1294}},
    {std::string(Common::MsprofMetrics::RESOURCE_CONFLICT_RATIO), {88, 90, 91, 92}},
    {std::string(Common::MsprofMetrics::MEMORY_L0),               {27, 28, 33, 34, 40, 42}},
    {std::string(Common::MsprofMetrics::MEMORY_UB),               {}},
    {std::string(Common::MsprofMetrics::OCCUPANCY),               {1280, 1282, 1283, 1284, 1286, 1287,
                                                                   1288, 1290, 1291, 1292, 1293, 1294}},
    {std::string(Common::MsprofMetrics::ROOFLINE),                {19, 28, 33, 34, 49, 50, 73, 74, 518, 524,
                                                                   1280, 1282, 1284, 1286, 1288, 1290, 1292, 1293, 1294}}
};

const MetricEventsMapType AIV_EVENTS_FOR_910B = {
    {std::string(Common::MsprofMetrics::PIPE_UTILIZATION),        {8, 9, 12, 13, 84, 85, 87, 770, 771}},
    {std::string(Common::MsprofMetrics::ARITHMETIC_UTILIZATION),  {1, 8, 75, 76, 77, 78, 79, 174, 184, 185, 186}},
    {std::string(Common::MsprofMetrics::L2_CACHE),                {1280, 1282, 1283, 1284, 1286, 1287, 1288, 1290, 1291}},
    {std::string(Common::MsprofMetrics::MEMORY),                  {4, 5, 6, 12, 13, 61, 62, 526, 1292, 1293, 1294}},
    {std::string(Common::MsprofMetrics::RESOURCE_CONFLICT_RATIO), {89, 90, 91, 92, 100, 101, 102, 103}},
    {std::string(Common::MsprofMetrics::MEMORY_L0),               {}},
    {std::string(Common::MsprofMetrics::MEMORY_UB),               {55, 56, 67, 68}},
    {std::string(Common::MsprofMetrics::OCCUPANCY),               {1280, 1282, 1283, 1284, 1286, 1287,
                                                                   1288, 1290, 1291, 1292, 1293, 1294}},
    {std::string(Common::MsprofMetrics::ROOFLINE),                {61, 62, 67, 68, 75, 76, 77, 78, 79, 174, 186,
                                                                   1280, 1282, 1284, 1286, 1288, 1290, 1292, 1293, 1294}}
};

// Eight pmu numbers for each group, add zeros to the end if not enough, put interdependent numbers together.
const std::vector<uint16_t> REPLAY_AIC_EVENTS_FOR_910B = {
    1032, 1033, 1280, 1282, 1283, 1292, 0,    0,
    1284, 1286, 1287, 1288, 1290, 1291, 1293, 1294,
    19,   49,   50,   518,  524,  0,    0,    0,
    3,    4,    5,    6,    526,  0,    0,    0,
    27,   28,   33,   34,   40,   42,   84,   85,
    13,   73,   74,   87,   92,   0,    0,    0,
    9,    10,   12,   88,   90,   91,   770,  771,
};

const std::vector<uint16_t> REPLAY_AIV_EVENTS_FOR_910B = {
    55,   56,   1280, 1282, 1283, 1292, 0,    0,
    1284, 1286, 1287, 1288, 1290, 1291, 1293, 1294,
    61,   62,   67,   68,   0,    0,    0,   0,
    1,    75,   76,   77,   78,   79,   174,  186,
    5,    6,    100,  101,  102,  103,  184,  185,
    4,    84,   85,   526,  13,   87,   92,    0,
    8,    9,    12,   770,  771,  89,   90,   91,
};

const std::vector<uint16_t> REPLAY_AIC_EVENTS_FOR_310P = {
    3,    73,   74,   75,   76,   77,   78,   79,
    174,  184,  185,  186,  4,    5,    6,    18,
    19,   44,   49,   50,   61,   62,   27,   28,
    33,   34,   39,   40,   41,   42,   55,   56,
    67,   68,   1,    84,   85,   87,   100,  101,
    102,  103,  9,    13,   92,   0,    0,    0,
    8,    10,   11,   12,   88,   89,   90,   91,
};

const MetricEventsMapType AIC_EVENTS_FOR_310P = {
    {std::string(Common::MsprofMetrics::PIPE_UTILIZATION),        {1, 8, 9, 10, 11, 12, 13, 84, 85, 87}},
    {std::string(Common::MsprofMetrics::ARITHMETIC_UTILIZATION),  {3, 8, 10, 73, 74, 75, 76, 77, 78, 79, 174, 184,
                                                                   185, 186}},
    {std::string(Common::MsprofMetrics::MEMORY),                  {4, 5, 6, 11, 12, 13, 18, 19, 44, 49, 50, 61, 62}},
    {std::string(Common::MsprofMetrics::RESOURCE_CONFLICT_RATIO), {88, 89, 90, 91, 92, 100, 101, 102, 103}},
    {std::string(Common::MsprofMetrics::MEMORY_L0),               {27, 28, 33, 34, 39, 40, 41, 42}},
    {std::string(Common::MsprofMetrics::MEMORY_UB),               {55, 56, 61, 62, 67, 68}},
    {std::string(Common::MsprofMetrics::ROOFLINE),                {28, 33, 34, 44, 49, 50, 61, 62, 67, 68, 73, 74,
                                                                   75, 76, 77, 78, 79, 174, 186}}
};

const MetricEventsMapType AIC_EVENTS_FOR_A5 = {
    {std::string(Common::MsprofMetrics::PIPE_UTILIZATION),        {0, 1, 10, 36, 52, 53, 514, 515, 810, 1794, 1812, 1813}},
    {std::string(Common::MsprofMetrics::ARITHMETIC_UTILIZATION),  {768, 789, 790, 808, 809, 810}},
    {std::string(Common::MsprofMetrics::L2_CACHE),                {1060, 1061, 1062, 1063, 1064, 1065, 1066, 1067, 1068, 1069, 1070, 1071}},
    {std::string(Common::MsprofMetrics::MEMORY),                  {512, 513, 514, 515, 1058, 1059,
                                                                   1792, 1794, 1799, 1801, 1804, 1806, 1815}},
    {std::string(Common::MsprofMetrics::RESOURCE_CONFLICT_RATIO), {11, 13, 14, 15}},
    {std::string(Common::MsprofMetrics::MEMORY_L0),               {772, 774, 776, 778, 1795, 1797}},
    {std::string(Common::MsprofMetrics::MEMORY_UB),               {}},
    {std::string(Common::MsprofMetrics::OCCUPANCY),               {1058, 1059, 1060, 1061, 1062, 1063, 1064, 1065, 1066, 1067, 1068, 1069, 1070, 1071}},
    {std::string(Common::MsprofMetrics::ROOFLINE),                {1058, 1059, 1795, 1797, 1799, 1801, 1804, 1806, 1815,
                                                                   1060, 1061, 1062, 1063, 1064, 1065, 1066, 1067, 1068, 1069, 1070, 1071}}
};

const MetricEventsMapType AIV_EVENTS_FOR_A5 = {
    {std::string(Common::MsprofMetrics::PIPE_UTILIZATION),        {0, 1, 10, 52, 53, 514, 515, 1281}},
    {std::string(Common::MsprofMetrics::ARITHMETIC_UTILIZATION),  {1281, 1282, 1283, 1284}},
    {std::string(Common::MsprofMetrics::L2_CACHE),                {1060, 1061, 1062, 1063, 1064, 1065, 1066, 1067, 1068, 1069, 1070, 1071}},
    {std::string(Common::MsprofMetrics::MEMORY),                  {512, 513, 514, 515, 516, 518, 1058, 1059, 1391,
                                                                   1397, 1398, 1407, 1408}},
    {std::string(Common::MsprofMetrics::RESOURCE_CONFLICT_RATIO), {12, 13, 14, 15, 1344, 1366, 1376, 1377, 1378, 1379}},
    {std::string(Common::MsprofMetrics::MEMORY_L0),               {}},
    {std::string(Common::MsprofMetrics::MEMORY_UB),               {1058, 1059, 1393, 1394, 1397, 1398, 1407, 1408}},
    {std::string(Common::MsprofMetrics::OCCUPANCY),               {1058, 1059, 1060, 1061, 1062, 1063, 1064, 1065, 1066, 1067, 1068, 1069, 1070, 1071}},
    {std::string(Common::MsprofMetrics::ROOFLINE),                {1058, 1059, 1397, 1398, 1407, 1408,
                                                                   1060, 1061, 1062, 1063, 1064, 1065, 1066, 1067, 1068, 1069, 1070, 1071}}
};

// Ten pmu numbers for each group, put interdependent numbers together, must contain at least one 0 for pmu=0 is valid.
const std::vector<uint16_t> REPLAY_AIC_EVENTS_FOR_A5 = {
    1060, 1061, 1062, 1063, 1064, 1065, 1066, 1067, 1068, 1069,
    1070, 1071, 512,  513,  768,  789,  790,  1792, 1813, 0,
    52,   53,   772,  774,  776,  778,  1058, 1059, 0,    0,
    1795, 1797, 1799, 1801, 1804, 1806, 1815, 0,    0,    0,
    10,   11,   13,   14,   15,   36,   0,    0,    0,    0,
    1,    514,  515,  808,  809,  810,  1794, 1812, 0,    0
};

const std::vector<uint16_t> REPLAY_AIV_EVENTS_FOR_A5 = {
    1060, 1061, 1062, 1063, 1064, 1065, 1066, 1067, 1068, 1069,
    1070, 1071, 512,  513,  1344, 1366, 1376, 1377, 1378, 1379,
    52,   53,   516,  518,  1058, 1059, 1391, 1393, 1394, 0,
    1395, 1396, 1397, 1398, 1407, 1408, 0,    0,    0,    0,
    10,   12,   14,   15,   0,    0,    0,    0,    0,    0,
    1,    514,  515,  1281, 1282, 1283, 1284, 0,    0,    0
};

const std::vector<std::string> MAGICS = {"RT_DEV_BINARY_MAGIC_ELF_AIVEC", "RT_DEV_BINARY_MAGIC_ELF_AICUBE",
                                         "RT_DEV_BINARY_MAGIC_ELF"};
const std::vector<uint16_t> L2_CACHE_EVENTS_FOR_310P = {106, 120, 121};
const std::vector<uint16_t> L2_CACHE_EVENTS_FOR_A2_A3 = {0xFB, 0xFc, 0xF6, 0xFD, 0x9C, 0x9D, 0x4F, 0x07};
constexpr uint32_t MAX_KERNEL_NAME_LENGTH = 1023;
constexpr uint32_t MAX_MSTX_INCLUDE_NAME_LENGTH = 1023;
constexpr uint32_t MAX_INPUT_STR_LENGTH = 200;

// DumpMessageType order can not be change!!!
enum class DumpMessageType : uint8_t {
    BIN_PATH = 0,
    BLOCK_DIM,
    DEVICE_ID,
    FFTS,
    INPUT_PATH,
    INPUT_SIZE,
    KERNEL_NAME,
    MAGIC,
    TILING_DATA,
    TILING_KEY,
    END
};
constexpr char const *PC_START_PATH = "pc_start_addr.txt";
constexpr char const *BIN_PATH = "bin_path";
constexpr char const *BLOCK_DIM = "block_dim";
constexpr char const *DEVICE_ID = "device_id";
constexpr char const *FFTS = "ffts";
constexpr char const *INPUT_PATH = "input_path";
constexpr char const *INPUT_SIZE = "input_size";
constexpr char const *KERNEL_NAME = "kernel_name";
constexpr char const *MAGIC = "magic";
constexpr char const *TILING_DATA = "tiling_data_path";
constexpr char const *TILING_KEY = "tiling_key";
constexpr char const *L1_TO_GM = "L1_TO_GM";
constexpr char const *L0C_TO_L1 = "L0C_TO_L1";
constexpr char const *L0C_TO_GM = "L0C_TO_GM";
constexpr char const *GM_TO_UB = "GM_TO_UB";
constexpr char const *UB_TO_GM = "UB_TO_GM";
constexpr char const *GM_TO_L0A = "GM_TO_L0A";
constexpr char const *GM_TO_L0B = "GM_TO_L0B";
constexpr char const *L1_TO_L0A = "L1_TO_L0A";
constexpr char const *L1_TO_L0B = "L1_TO_L0B";
constexpr char const *SU_PIPE = "SCALAR";
constexpr char const *FC_PIPE = "FLOWCTRL";
constexpr char const *MTE_PIPE = "MTE";
constexpr char const *SIMD_PIPE = "SIMD";
constexpr char const *FIXP_PIPE = "FIXP";
constexpr char const *CUBE_PIPE = "CUBE";
constexpr char const *VEC_PIPE = "VECTOR";
constexpr char const *GM_TO_L1 = "GM_TO_L1";
constexpr char const *GM_TO_L0A_DATA = "GM_TO_L0A_DATA";
constexpr char const *GM_TO_L0B_DATA = "GM_TO_L0B_DATA";
constexpr char const *GM_TO_L1_DATA = "GM_TO_L1_DATA";
constexpr char const *L1_TO_L0B_DATA = "L1_TO_L0B_DATA";
constexpr char const *L1_TO_L0A_DATA = "L1_TO_L0A_DATA";
constexpr char const *REG_TO_L1_DATA = "REG_TO_L1_DATA";
constexpr char const *L1_TO_BT_DATA = "L1_TO_BT_DATA";
constexpr char const *LOC_TO_GM_DATA = "LOC_TO_GM_DATA";
constexpr char const * MIX_AIC_TAIL = "_mix_aic";
constexpr char const * MIX_AIV_TAIL = "_mix_aiv";

const std::map<DumpMessageType, std::string> DUMP_MESSAGE_TYPE_ENUM_TO_STRING {
    {DumpMessageType::BIN_PATH, BIN_PATH},
    {DumpMessageType::BLOCK_DIM, BLOCK_DIM},
    {DumpMessageType::DEVICE_ID, DEVICE_ID},
    {DumpMessageType::FFTS, FFTS},
    {DumpMessageType::INPUT_PATH, INPUT_PATH},
    {DumpMessageType::INPUT_SIZE, INPUT_SIZE},
    {DumpMessageType::KERNEL_NAME, KERNEL_NAME},
    {DumpMessageType::MAGIC, MAGIC},
    {DumpMessageType::TILING_DATA, TILING_DATA},
    {DumpMessageType::TILING_KEY, TILING_KEY},
};

const std::map<std::string, float> MAX_BW_910B1 {
        {std::string {L1_TO_GM}, 199.43},
        {std::string {L0C_TO_L1}, 216.88},
        {std::string {L0C_TO_GM}, 209.32},
        {std::string {GM_TO_UB}, 220.06},
        {std::string {UB_TO_GM}, 186.8}
};

const std::map<std::string, float> MAX_BW_910B1_CJ {
        {std::string {L1_TO_GM}, 187.72},
        {std::string {L0C_TO_L1}, 220.48},
        {std::string {L0C_TO_GM}, 202.41},
        {std::string {GM_TO_UB}, 219.14},
        {std::string {UB_TO_GM}, 197.82}
};

const std::map<std::string, float> MAX_BW_910B4 {
        {std::string {L1_TO_GM}, 189.89},
        {std::string {L0C_TO_L1}, 190.7},
        {std::string {L0C_TO_GM}, 190.7},
        {std::string {GM_TO_UB}, 195.27},
        {std::string {UB_TO_GM}, 176.75}
};

const std::map<std::string, float> MAX_BW_A5 {
        {std::string {L1_TO_GM}, 189.89},
        {std::string {L0C_TO_L1}, 190.7},
        {std::string {L0C_TO_GM}, 190.7},
        {std::string {GM_TO_UB}, 195.27},
        {std::string {UB_TO_GM}, 176.75}
};

// Manufacturers and the corresponding GM types.
enum class GmType {
    SK = 86,
    SS = 87,
    CJ = 88,
    DEFAULT = 0
};

const std::map<unsigned short, GmType> GM_PRODUCT {
        {86, GmType::SK},
        {87, GmType::SS},
        {88, GmType::CJ},
        {0, GmType::DEFAULT},
};
} // namespace Common

#endif  // __MSOPPROF_COMMON_DEFS_H__
