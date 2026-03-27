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
constexpr uint64_t MAX_MEM_BYTE_SIZE = 32212254720; // 30G for file and memory malloc

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
}

#endif  // __MSOPPROF_UTILITY_DATA_FORMAT_H__
