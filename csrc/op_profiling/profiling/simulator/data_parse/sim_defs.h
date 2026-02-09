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


#ifndef MSOPT_SIM_DEFS_H
#define MSOPT_SIM_DEFS_H

#include <string>
#include <map>
#include <utility>
#include <vector>
#include <set>
#include <sstream>
#include "common/defs.h"
#include "thread_safe_unordered_map.h"
#include "common/prof_args.h"

namespace Profiling {

// CoreNameAndPreFixPair [coreName, corePrefix] e.g. [core0.veccore0, core0.veccore0.]
using CoreNameAndPreFixPair = std::pair<std::string, std::string>;
using PhysicalAndLogicalPair = std::pair<std::string, std::string>;

// Pc2CodeLineMapType [pc_address, codelines] e.g. ["0x1195e004", "/home/tmp/add_custom.cpp:79"
using Pc2CodeLineMapType = Utility::ThreadSafeUnorderedMap<std::string, std::vector<std::string>>;
using Pc2CodeMap = Utility::ThreadSafeUnorderedMap<uint64_t, std::vector<std::string>>;

constexpr char const *USER_MARK = "USERMARK";
constexpr char const *RVECST = "RVECST";
constexpr char const *RVECLD = "RVECLD";
constexpr char const *RVECEX = "RVECEX";
constexpr char const *PUSHQ = "PUSHQ";
constexpr char const *RVECSU = "RVECSU";
constexpr char const *RVECLP = "RVECLP";
constexpr char const *WAIT_FLAG = "WAIT_FLAG";
constexpr char const *SET_FLAG = "SET_FLAG";
constexpr char const *WAIT_EVENT = "wait_event";
constexpr char const *SET_EVENT = "set_event";
constexpr char const *LPCNT_FLAG = "LPCNT";
constexpr char const *COND_FLAG = "SPR:COND,";
constexpr uint32_t THREAD_IDEAL_NUM = 75;
constexpr uint32_t PARSE_PLUGIN_THREAD = 5;
constexpr uint32_t EACH_CORE_THREAD = THREAD_IDEAL_NUM / PARSE_PLUGIN_THREAD;
constexpr double MAX_THREAD_USAGE_RATIO = 0.6;
constexpr uint8_t MASK_MODE_BIT_IN_CTRL_REGISTER = 56;

enum class MaskMode : uint8_t {
    REPEAT_MODE = 0U,
    ELEMENT_COUNT_MODE,
};

enum class MatchMode : uint8_t {
    PC_MATCH = 0U,
    ID_MATCH,
};

struct SpReg {
    uint64_t vectorMask0;
    uint64_t vectorMask1;
    uint64_t ndParaConfig;
    MaskMode maskMode;
};

enum class SpRegPosEnum : uint8_t {
    VECTOR_MASK0 = 0U,
    VECTOR_MASK1,
    ND_PARA_CONFIG,
    MASK_MODE,
};

struct InstrParseInfo {
    uint64_t tick;
    uint64_t pc;
    uint64_t id;
    int warpId;
    int schId;
    std::string pipe;
    std::string name;
    std::string detail;
    SpReg spStatus;
};

struct InstrParseInfoForRealTime : public InstrParseInfo {
    std::string coreName;
    InstrParseInfoForRealTime() = default;
    InstrParseInfoForRealTime(const InstrParseInfo &instrParseInfo,
                              std::string coreName) : InstrParseInfo(instrParseInfo), coreName(std::move(coreName)) {}
};

struct MergeInfo : InstrParseInfo {
    uint64_t startTick;
    uint64_t endTick;
    // simt struction
    uint64_t theoStallCyc;
    uint64_t realStallCyc;
    int gprCount;
    int processBytes;
    int ubReadConflict;
    int ubWriteConflict;
    float vecUtilization;
};

struct PoppedInstrParseInfo : InstrParseInfo {
    int gprCount = 0;
    uint64_t theoStallCyc  = 0;
    uint64_t realStallCyc = 0;
    explicit PoppedInstrParseInfo(const InstrParseInfo &instrParseInfo) : InstrParseInfo(instrParseInfo), gprCount(0) {}

    PoppedInstrParseInfo() = default;
};

struct PoppedInstrParseInfoForRealTime : PoppedInstrParseInfo {
    std::string coreName;
    PoppedInstrParseInfoForRealTime() = default;
    explicit PoppedInstrParseInfoForRealTime(PoppedInstrParseInfo &instrParseInfo, std::string coreName)
        : PoppedInstrParseInfo(instrParseInfo),
        coreName(std::move(coreName)) {}
};

struct IcacheParseInfoForRealTime {
    uint64_t tick;
    uint64_t pc;
    std::string coreName;
    std::string detail;
};

struct IcacheParseInfo {
    uint64_t tick;
    uint64_t pc;
    std::string detail;
};

using PoppedInstrDataParseMap = Utility::ThreadSafeUnorderedMap<std::string, std::vector<MergeInfo>>;
using InstrDataParseMap = Utility::ThreadSafeUnorderedMap<std::string, std::vector<InstrParseInfo>>;
using ICacheDataParseMap = Utility::ThreadSafeUnorderedMap<std::string, std::vector<IcacheParseInfo>>;

struct DumpMaps {
    ICacheDataParseMap icacheParseMap;
    PoppedInstrDataParseMap poppedParseMap;
    InstrDataParseMap instrParseMap;
};

struct UserMarkInfo {
    uint64_t startTick;
    uint64_t endTick;
    uint64_t startPc;
    uint64_t endPc;
};

// 存放userMark映射，[CoreName, [MarkName , UserMarkInfo List]]
using UserMarkMapType = Utility::ThreadSafeUnorderedMap<std::string, std::map<std::string, std::vector<UserMarkInfo>>>;
struct UserMarkStruct {
    std::map<std::string, std::vector<UserMarkInfo>> userMarkInfos;
    std::vector<MergeInfo> userMarkInstrs;
};

struct ParseInfoStruct {
    std::vector<std::string> coresNameVec;
    std::string startPc;
    std::set<CoreNameAndPreFixPair> coresNamePair;
    DumpMaps maps;
    UserMarkMapType userMarkMap;
    std::string path;
    Common::ChipProductType chipType;
};

struct SimParseContext {
    std::set<CoreNameAndPreFixPair> coresNamePair;
    std::string dumpPath;
    std::string outputPath;
    Common::ChipProductType chipType;
    std::set<int> parseCorIds;
    Common::ProfMetricsAbilityConfig metricsConfig;
};
struct RealTimeSimParseContext {
    std::set<int> parseCoreId;
    bool enableResourceConflictRatio;
    Common::ChipProductType chipType;
    Common::ProfMetricsAbilityConfig metricsConfig;
};

const std::string NAME = "name";
const std::string PIPE = "pipe";
const std::string PC = "pc";
const std::string DETAIL = "detail";
const std::string START = "start";
const std::string END = "end";
const std::string GPR_COUNT = "gprCount";
const std::string PROCESS_BYTES = "processBytes";
const std::string VEC_UTILIZATION = "vecUtilization";
const std::string UB_READ_CONFLICT = "ubReadConflict";
const std::string UB_WRITE_CONFLICT = "ubWriteConflict";
const std::string THEO_STALL_CYC = "theoStallCyc";
const std::string REAL_STALL_CYC = "realStallCyc";
const std::string WARP_ID = "warpId";
const std::string SCH_ID = "schId";
const std::string DEFAULT_STR_VALUE = "NA";
constexpr int DEFAULT_INT_VALUE = -1;
constexpr float DEFAULT_FLOAT_VALUE = -1;
constexpr int DEFAULT_MHZ = 1850;

const std::map<Common::ChipProductType, int> CLOCK_SPEED_SERIES_MAP{
    {Common::ChipProductType::ASCEND310B1,       1224},
    {Common::ChipProductType::ASCEND310B2,       1224},
    {Common::ChipProductType::ASCEND310B3,       1224},
    {Common::ChipProductType::ASCEND310B4,       500},
    {Common::ChipProductType::ASCEND310P1,       1060},
    {Common::ChipProductType::ASCEND310P2,       1060},
    {Common::ChipProductType::ASCEND310P3,       1060},
    {Common::ChipProductType::ASCEND310P4,       1060},
    {Common::ChipProductType::ASCEND310P5,       800},
    {Common::ChipProductType::ASCEND310P7,       550},
    {Common::ChipProductType::ASCEND910B1,       1850},
    {Common::ChipProductType::ASCEND910B2,       1800},
    {Common::ChipProductType::ASCEND910B2C,      1800},
    {Common::ChipProductType::ASCEND910B3,       1800},
    {Common::ChipProductType::ASCEND910B4,       1650},
    {Common::ChipProductType::ASCEND950DT_950X,    1650},
    {Common::ChipProductType::ASCEND950DT_950Y,    1650},
    {Common::ChipProductType::ASCEND950PR_950Z,    1650},
    {Common::ChipProductType::ASCEND950DT_9571,    1650},
    {Common::ChipProductType::ASCEND950DT_9572,    1650},
    {Common::ChipProductType::ASCEND950DT_9573,    1650},
    {Common::ChipProductType::ASCEND950DT_9574,    1650},
    {Common::ChipProductType::ASCEND950DT_9575,    1650},
    {Common::ChipProductType::ASCEND950DT_9576,    1650},
    {Common::ChipProductType::ASCEND950DT_9577,    1650},
    {Common::ChipProductType::ASCEND950DT_9578,    1650},
    {Common::ChipProductType::ASCEND950PR_9579,    1650},
    {Common::ChipProductType::ASCEND950PR_957B,    1650},
    {Common::ChipProductType::ASCEND950PR_957C,    1650},
    {Common::ChipProductType::ASCEND950PR_957D,    1650},
    {Common::ChipProductType::ASCEND950DT_9581,    1650},
    {Common::ChipProductType::ASCEND950DT_9582,    1650},
    {Common::ChipProductType::ASCEND950DT_9583,    1650},
    {Common::ChipProductType::ASCEND950DT_9584,    1650},
    {Common::ChipProductType::ASCEND950DT_9585,    1650},
    {Common::ChipProductType::ASCEND950DT_9586,    1650},
    {Common::ChipProductType::ASCEND950DT_9587,    1650},
    {Common::ChipProductType::ASCEND950DT_9588,    1650},
    {Common::ChipProductType::ASCEND950PR_9589,    1650},
    {Common::ChipProductType::ASCEND950PR_958B,    1650},
    {Common::ChipProductType::ASCEND950DT_9591,    1800},
    {Common::ChipProductType::ASCEND950DT_9592,    1800},
    {Common::ChipProductType::ASCEND950DT_9595,    1800},
    {Common::ChipProductType::ASCEND950DT_9596,    1800},
    {Common::ChipProductType::ASCEND950PR_9599,    1800},
    {Common::ChipProductType::ASCEND950DT_95A1,    1800},
    {Common::ChipProductType::ASCEND950DT_95A2,    1800},
};

inline std::string GetPc2String(uint64_t pc)
{
    std::ostringstream oss;
    oss << std::hex << std::showbase << pc;
    return oss.str();
}
}
#endif // MSOPT_SIM_DEFS_H
