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


#ifndef MSOPT_MTETHROUGHPUTTABLE_H
#define MSOPT_MTETHROUGHPUTTABLE_H


#include <unordered_map>
#include <vector>
#include "common/defs.h"

namespace Profiling {
namespace Parse {

enum class MteLogInstrType : uint8_t {
    GM_TO_UB = 0,
    GM_TO_L1,
    GM_TO_TOTAL,
    UB_TO_GM,
    L1_TO_GM,
    TOTAL_TO_GM,
    END,
};

const std::unordered_map<MteLogInstrType, std::string> MteLogInstrTypeStr = {
    {MteLogInstrType::GM_TO_UB, "GM_TO_UB"},
    {MteLogInstrType::GM_TO_L1, "GM_TO_L1"},
    {MteLogInstrType::GM_TO_TOTAL, "GM_TO_TOTAL"},
    {MteLogInstrType::UB_TO_GM, "UB_TO_GM"},
    {MteLogInstrType::L1_TO_GM, "L1_TO_GM"},
    {MteLogInstrType::TOTAL_TO_GM, "TOTAL_TO_GM"},
};

struct MteLogReqInfo {
    float ts;              // 单位us
    double dataSize;       // 读/写数据量
};

struct MteLogInstrInfo {
    MteLogInstrType instrType;
    float maxReqTs;  // -1表示无效
    std::unordered_map<uint64_t, MteLogReqInfo> reqTbl;       // key:req_id
};

struct MteLogCfg {
    uint32_t coreId;
    std::string dumpFile;
    Common::ChipProductType chipType;
};

using MteLogInstrMap = std::unordered_map<uint64_t, MteLogInstrInfo>;  // key:instr_id

// MTEThroughputInfoVec[2][GM_TO_UB] 表示在2us时间点上，从GM写入UB的数据量
using MteThroughputChart = std::vector<std::vector<double>>;

}
}
#endif