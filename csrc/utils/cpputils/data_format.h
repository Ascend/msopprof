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


#ifndef __MSOPPROF_UTILITY_DATA_FORMAT_H__
#define __MSOPPROF_UTILITY_DATA_FORMAT_H__

#include <utility>
#include <vector>
#include <string>
#include <map>

namespace Utility {

constexpr uint32_t PMU_EVENT_MAX_NUM = 8U;
constexpr uint32_t PMU_EVENT_MAX_NUM_A5 = 10U;
constexpr uint32_t EVENT_MAX_NUM = 96;
constexpr uint32_t EVENT_MAX_NUM_A5 = 60U;
constexpr uint64_t MAX_MEM_BYTE_SIZE = 32212254720; // 30G for file and memory malloc

constexpr uint32_t UINT32_INVALID = UINT32_MAX;
constexpr uint16_t PATH_MAX_LENGTH = 4096U;

using CsvRow = std::vector<std::string>;
using CsvData = std::vector<CsvRow>;

struct CsvFileStruct {
    std::string fileName;
    std::vector<std::string> headers;
    CsvData data;
    std::map<std::string, int> headerIndex;
    bool valid = false;
};

const mode_t SAVE_DATA_FILE_AUTHORITY = 0640;
const mode_t READ_ONLY_FILE_AUTHORITY = 0400;

struct MstxProfConfig {
    bool isMstxEnable {false};
    char mstxEnabledMessage[1024] {'\0'}; // op_profiling/common/defs.h:MAX_KERNEL_NAME_LENGTH + 1
};

struct ProfConfig {
    ProfConfig(std::string outputPath, std::string kernelName, uint16_t profMaxTimes, uint16_t profSkipTimes)
        :   outputPath_(std::move(outputPath)), kernelName_(std::move(kernelName)),
        profMaxTimes_(profMaxTimes), profSkipTimes_(profSkipTimes) {}
    ProfConfig() = default;
    std::string outputPath_;
    std::string kernelName_;
    uint16_t profMaxTimes_ {1};
    uint16_t profSkipTimes_ {0};
};
enum class ProfDBIType {
    AS_IS, // 不插桩
    OPERAND_RECORD, // operand record桩
    MEMORY_CHART, // memory chart桩
    INSTR_PROF_START, // start桩
    INSTR_PROF_END, // end桩
    BB_COUNT // bb count桩
};

constexpr uint32_t DBI_FLAG_OPERAND_RECORD = 1U << static_cast<uint32_t>(ProfDBIType::OPERAND_RECORD);
constexpr uint32_t DBI_FLAG_MEMORY_CHART = 1U << static_cast<uint32_t>(ProfDBIType::MEMORY_CHART);
constexpr uint32_t DBI_FLAG_INSTR_PROF_START = 1U << static_cast<uint32_t>(ProfDBIType::INSTR_PROF_START);
constexpr uint32_t DBI_FLAG_INSTR_PROF_END = 1U << static_cast<uint32_t>(ProfDBIType::INSTR_PROF_END);
constexpr uint32_t DBI_FLAG_BB_COUNT = 1U << static_cast<uint32_t>(ProfDBIType::BB_COUNT);

struct MessageOfProfConfig {
    MstxProfConfig mstxProfConfig { };
    uint32_t replayCount {UINT32_INVALID};
    uint32_t dbiFlag {0};
    uint16_t profWarmUpTimes {0};
    uint16_t aicPmu[EVENT_MAX_NUM]{};
    uint16_t aivPmu[EVENT_MAX_NUM]{};
    uint16_t l2CachePmu[EVENT_MAX_NUM]{};
    uint8_t replayMode {0};
    bool useProfileMode {false};
    bool killAdvance {false};
    bool isDeviceToSimulator {false};
    bool isSimulator {false};
    bool pmSamplingEnable {false};
};
}

#endif  // __MSOPPROF_UTILITY_DATA_FORMAT_H__
