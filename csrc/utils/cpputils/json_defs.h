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

#ifndef __CPPUTILS_JSONDEFS_H__
#define __CPPUTILS_JSONDEFS_H__

#include <vector>
#include "json.hpp"
#include "log.h"

namespace Utility {

enum class CfgDataType : uint32_t {
    STRING = 0,
    INT,
    UINT64,
    ARRAY,
    FLOAT
};

enum class JsonType : uint16_t {
    KERNEL_NAME,
    KERNEL_PATH,
    BLOCK_DIM,
    MODE,
    DEVICE_ID,
    TILING_KEY,
    MAGIC,
    TEST_CASES,
    CASE_NAME,
    PARAM_DESC,
    PARAM_TYPE,
    TYPE,
    SHAPE,
    DATA_PATH,
    NAME,
    USER_WORKSPACE_SIZE,
    TILING_DATA_SIZE,
    TILING_DATA_PATH,
    DATA_SIZE,
    SIMULATOR_DUMP_PATH,
    OUTPUT_DATA_PATH,
    START,
    END,
    GPR_COUNT,
    PROCESS_BYTES,
    UB_READ_CONFLICT,
    UB_WRITE_CONFLICT,
    VEC_UTILIZATION,
    PIPE,
    PC,
    DETAIL,
    THEO_STALL_CYC,
    REAL_STALL_CYC,
    WARP_ID,
    SCH_ID,
};

const std::map<JsonType, std::string> JSON_KEY = {
    {JsonType::KERNEL_NAME,           "kernel_name"},
    {JsonType::KERNEL_PATH,           "kernel_path"},
    {JsonType::BLOCK_DIM,             "blockdim"},
    {JsonType::MODE,                  "mode"},
    {JsonType::DEVICE_ID,             "device_id"},
    {JsonType::TILING_KEY,            "tiling_key"},
    {JsonType::MAGIC,                 "magic"},
    {JsonType::TEST_CASES,            "test_cases"},
    {JsonType::CASE_NAME,             "case_name"},
    {JsonType::PARAM_DESC,            "param_desc"},
    {JsonType::PARAM_TYPE,            "param_type"},
    {JsonType::TYPE,                  "type"},
    {JsonType::SHAPE,                 "shape"},
    {JsonType::DATA_PATH,             "data_path"},
    {JsonType::NAME,                  "name"},
    {JsonType::USER_WORKSPACE_SIZE,   "user_workspace_size"},
    {JsonType::TILING_DATA_SIZE,      "tiling_data_size"},
    {JsonType::TILING_DATA_PATH,      "tiling_data_path"},
    {JsonType::DATA_SIZE,             "data_size"},
    {JsonType::SIMULATOR_DUMP_PATH,   "simulator_dump_path"},
    {JsonType::OUTPUT_DATA_PATH,      "output_data_path"},
    {JsonType::START,                 "start"},
    {JsonType::END,                   "end"},
    {JsonType::GPR_COUNT,             "gprCount"},
    {JsonType::UB_READ_CONFLICT,      "ubReadConflict"},
    {JsonType::UB_WRITE_CONFLICT,     "ubWriteConflict"},
    {JsonType::PROCESS_BYTES,         "processBytes"},
    {JsonType::VEC_UTILIZATION,       "vecUtilization"},
    {JsonType::PIPE,                  "pipe"},
    {JsonType::PC,                    "pc"},
    {JsonType::DETAIL,                "detail"},
    {JsonType::THEO_STALL_CYC,        "theoStallCyc"},
    {JsonType::REAL_STALL_CYC,        "realStallCyc"},
    {JsonType::WARP_ID,               "warpId"},
    {JsonType::SCH_ID,                "schId"}
};

}
#endif  // __CPPUTILS_JSONDEFS_H__
