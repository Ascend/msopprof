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

#ifndef __MSOPPROF_PROFILING_DEVICE_DEF_H__
#define __MSOPPROF_PROFILING_DEVICE_DEF_H__
#include <map>
#include <vector>
#include <string>

#include "common/defs.h"

namespace Profiling {
enum class TransportType : uint16_t {
    READ_MAIN_MEMORY = 0,
    WRITE_MAIN_MEMORY,
    GM_TO_L1,
    GM_TO_UB,
    GM_TO_DCACHE,
    DCACHE_TO_VEC,
    L1_TO_GM,
    L0A_TO_CUBE,
    L0B_TO_CUBE,
    L0C_TO_CUBE,
    L0C_TO_UB,
    L0C_TO_L1,
    L0C_TO_GM,
    L0C_TO_FIXP,
    L0C_TO_VEC,
    UB_TO_GM,
    UB_TO_MTE,
    UB_TO_VEC,
    UB_TO_SCA,
    UB_TO_L1,
    L1_TO_L0A,
    L1_TO_L0B,
    L1_TO_MTE,
    L1_TO_UB,
    MTE_TO_L1,
    MTE_TO_L0A,
    MTE_TO_L0B,
    MTE_TO_UB,
    VEC_TO_UB,
    VEC_TO_GM,
    VEC_TO_L1,
    VEC_TO_DCACHE,
    FIXP_TO_L1,
    FIXP_TO_UB,
    CUBE_TO_L0C,
    SCA_TO_UB,
    L2_TO_GM,
    GM_TO_L2,
    CORE_TO_L2,
    L2_TO_CORE,
    GM_READ,
    GM_WRITE,
    GM_TO_L0A,
    GM_TO_L0B,
    MTE2_WRITE,
    UNKNOWN
};

const std::map<TransportType, uint16_t> REQ_DATA_OF_910B = {
    {TransportType::READ_MAIN_MEMORY,        128},
    {TransportType::WRITE_MAIN_MEMORY,       128},
    {TransportType::GM_TO_L1,                256},
    {TransportType::GM_TO_UB,                128},
    {TransportType::L0C_TO_L1,               128},
    {TransportType::L0C_TO_GM,               128},
    {TransportType::L0C_TO_FIXP,             128},
    {TransportType::UB_TO_GM,                128},
    {TransportType::UB_TO_MTE,               128},
    {TransportType::UB_TO_VEC,               256},
    {TransportType::L1_TO_GM,                128},
    {TransportType::L1_TO_L0A,               256},
    {TransportType::L1_TO_L0B,               128},
    {TransportType::L1_TO_MTE,               256},
    {TransportType::MTE_TO_L1,               256},
    {TransportType::MTE_TO_L0A,              256},
    {TransportType::MTE_TO_L0B,              128},
    {TransportType::MTE_TO_UB,               128},
    {TransportType::VEC_TO_UB,               256},
    {TransportType::FIXP_TO_L1,              128},
    {TransportType::L2_TO_GM,                512},
    {TransportType::GM_TO_L2,                128},
    {TransportType::CORE_TO_L2,              512},
    {TransportType::L2_TO_CORE,              128},
    {TransportType::GM_READ,                 128},
    {TransportType::GM_WRITE,                128},
    {TransportType::GM_TO_L0A,               256},
    {TransportType::GM_TO_L0B,               128},
};

const std::map<TransportType, uint16_t> REQ_DATA_OF_A5 = {
    {TransportType::READ_MAIN_MEMORY,        128},
    {TransportType::WRITE_MAIN_MEMORY,       128},
    {TransportType::CUBE_TO_L0C,             1024},
    {TransportType::GM_TO_L1,                128},
    {TransportType::GM_TO_UB,                128},
    {TransportType::GM_TO_DCACHE,            128},
    {TransportType::L1_TO_L0A,               256},
    {TransportType::L1_TO_L0B,               256},
    {TransportType::L1_TO_MTE,               256},
    {TransportType::L1_TO_UB,                256},
    {TransportType::L0A_TO_CUBE,             64},
    {TransportType::L0B_TO_CUBE,             256},
    {TransportType::L0C_TO_CUBE,             1024},
    {TransportType::L0C_TO_L1,               128},
    {TransportType::L0C_TO_GM,               128},
    {TransportType::L0C_TO_UB,               128},
    {TransportType::L0C_TO_FIXP,             128},
    {TransportType::UB_TO_GM,                128},
    {TransportType::UB_TO_L1,                128},
    {TransportType::UB_TO_MTE,               128},
    {TransportType::UB_TO_VEC,               256},
    {TransportType::FIXP_TO_UB,              128},
    {TransportType::MTE_TO_UB,               128},
    {TransportType::MTE_TO_L1,               256},
    {TransportType::VEC_TO_UB,               256},
    {TransportType::VEC_TO_GM,               128},
    {TransportType::VEC_TO_DCACHE,           128}, // 估算值，假设WARP写满
    {TransportType::DCACHE_TO_VEC,           128},
};

const std::map<TransportType, uint16_t> REQ_DATA_OF_310P = {
    {TransportType::L0C_TO_VEC,              512},
    {TransportType::UB_TO_MTE,               16},
    {TransportType::UB_TO_VEC,               16},
    {TransportType::L1_TO_MTE,               512},
    {TransportType::MTE_TO_L1,               256},
    {TransportType::MTE_TO_L0A,              256},
    {TransportType::MTE_TO_L0B,              128},
    {TransportType::MTE_TO_UB,               16},
    {TransportType::VEC_TO_UB,               16},
};

// 以下均为单核带宽，单位GB/s
const std::map<TransportType, float> maxBwRateOf910B1 = {
    {TransportType::GM_TO_L1,     264},
    {TransportType::L1_TO_GM,     199.43},
    {TransportType::L0C_TO_L1,    216.88},
    {TransportType::L0C_TO_GM,    209.32},
    {TransportType::GM_TO_UB,     220.06},
    {TransportType::UB_TO_GM,     186.8},
    {TransportType::MTE_TO_L0A,   437.5},
    {TransportType::MTE_TO_L0B,   210.5},
    {TransportType::MTE2_WRITE,   340.1},
};

const std::map<TransportType, float> maxBwRateOf910B1GmCJ = {
    {TransportType::GM_TO_L1,     296},
    {TransportType::L1_TO_GM,     187.72},
    {TransportType::L0C_TO_L1,    220.48},
    {TransportType::L0C_TO_GM,    202.41},
    {TransportType::GM_TO_UB,     219.14},
    {TransportType::UB_TO_GM,     197.82},
    {TransportType::MTE_TO_L0A,   439.32},
    {TransportType::MTE_TO_L0B,   220.10},
    {TransportType::MTE2_WRITE,   302.32},
};

const std::map<TransportType, float> maxBwRateOf910B2 = {
    {TransportType::GM_TO_L1,     256.86},
    {TransportType::L1_TO_GM,     193.99},
    {TransportType::L0C_TO_L1,    211.00},
    {TransportType::L0C_TO_GM,    203.66},
    {TransportType::GM_TO_UB,     214.11},
    {TransportType::UB_TO_GM,     181.75},
    {TransportType::MTE_TO_L0A,   425.68},
    {TransportType::MTE_TO_L0B,   204.81},
    {TransportType::MTE2_WRITE,   330.91},
};

const std::map<TransportType, float> maxBwRateOf910B2GmCJ = {
    {TransportType::GM_TO_L1,     288.00},
    {TransportType::L1_TO_GM,     182.65},
    {TransportType::L0C_TO_L1,    214.52},
    {TransportType::L0C_TO_GM,    196.94},
    {TransportType::GM_TO_UB,     213.22},
    {TransportType::UB_TO_GM,     192.47},
    {TransportType::MTE_TO_L0A,   427.45},
    {TransportType::MTE_TO_L0B,   214.15},
    {TransportType::MTE2_WRITE,   294.15},
};

const std::map<TransportType, float> maxBwRateOf910B4 = {
    {TransportType::GM_TO_L1,     222},
    {TransportType::L1_TO_GM,     189.89},
    {TransportType::L0C_TO_L1,    190.7},
    {TransportType::L0C_TO_GM,    190.7},
    {TransportType::GM_TO_UB,     195.27},
    {TransportType::UB_TO_GM,     176.75},
    {TransportType::MTE_TO_L0A,   368.2},
    {TransportType::MTE_TO_L0B,   173.81},
    {TransportType::MTE2_WRITE,   222.17},
};

const std::map<TransportType, float> maxBwRateOf910B4GmCJ = {
    {TransportType::GM_TO_L1,     274},
    {TransportType::L1_TO_GM,     182.07},
    {TransportType::L0C_TO_L1,    196.27},
    {TransportType::L0C_TO_GM,    189.08},
    {TransportType::GM_TO_UB,     195.52},
    {TransportType::UB_TO_GM,     176.06},
    {TransportType::MTE_TO_L0A,   391.82},
    {TransportType::MTE_TO_L0B,   196.3},
    {TransportType::MTE2_WRITE,   274},
};

const std::map<TransportType, float> maxBwRateOf950_959X = {
    {TransportType::GM_TO_L1,     177.57},
    {TransportType::L1_TO_GM,     143.79},
    {TransportType::L0C_TO_L1,    412.15},
    {TransportType::L0C_TO_GM,    142.05},
    {TransportType::L0C_TO_UB,    206.20},
    {TransportType::L1_TO_L0A,    412.10},
    {TransportType::L1_TO_L0B,    412.11},
    {TransportType::L1_TO_UB,     206.18},
    {TransportType::UB_TO_L1,     206.18},
    {TransportType::GM_TO_UB,     177.96},
    {TransportType::UB_TO_GM,     143.74},
};

const std::map<TransportType, float> maxBwRateOf950_958X = {
    {TransportType::GM_TO_L1,     162.77},
    {TransportType::L1_TO_GM,     131.81},
    {TransportType::L0C_TO_L1,    377.80},
    {TransportType::L0C_TO_GM,    130.21},
    {TransportType::L0C_TO_UB,    189.02},
    {TransportType::L1_TO_L0A,    377.76},
    {TransportType::L1_TO_L0B,    377.77},
    {TransportType::L1_TO_UB,     188.99},
    {TransportType::UB_TO_L1,     188.99},
    {TransportType::GM_TO_UB,     163.13},
    {TransportType::UB_TO_GM,     131.76},
};
const std::map<TransportType, float> maxBwRateOf950_957X = maxBwRateOf950_958X;
const std::map<TransportType, float> maxBwRateOf950_950X = maxBwRateOf950_958X;

const std::map<std::pair<ChipProductType, Common::GmType>, std::map<TransportType, float>> MAX_BW_RATE_ALL = {
    {{ChipProductType::ASCEND910B1,       Common::GmType::DEFAULT},        maxBwRateOf910B1},
    {{ChipProductType::ASCEND910B2,       Common::GmType::DEFAULT},        maxBwRateOf910B2},
    {{ChipProductType::ASCEND910B2C,      Common::GmType::DEFAULT},        maxBwRateOf910B2},
    {{ChipProductType::ASCEND910B3,       Common::GmType::DEFAULT},        maxBwRateOf910B2},
    {{ChipProductType::ASCEND910B4,       Common::GmType::DEFAULT},        maxBwRateOf910B4},
    {{ChipProductType::ASCEND910B4_1,     Common::GmType::DEFAULT},        maxBwRateOf910B4},
    {{ChipProductType::ASCEND910B1,       Common::GmType::CJ},             maxBwRateOf910B1GmCJ},
    {{ChipProductType::ASCEND910B2,       Common::GmType::CJ},             maxBwRateOf910B2GmCJ},
    {{ChipProductType::ASCEND910B2C,      Common::GmType::CJ},             maxBwRateOf910B2GmCJ},
    {{ChipProductType::ASCEND910B3,       Common::GmType::CJ},             maxBwRateOf910B2GmCJ},
    {{ChipProductType::ASCEND910B4,       Common::GmType::CJ},             maxBwRateOf910B4GmCJ},
    {{ChipProductType::ASCEND910B4_1,     Common::GmType::CJ},             maxBwRateOf910B4GmCJ},
    {{ChipProductType::ASCEND910B2C,      Common::GmType::CJ},             maxBwRateOf910B4GmCJ},
    {{ChipProductType::ASCEND950DT_950X,  Common::GmType::DEFAULT},        maxBwRateOf950_950X},
    {{ChipProductType::ASCEND950DT_950Y,  Common::GmType::DEFAULT},        maxBwRateOf950_950X},
    {{ChipProductType::ASCEND950DT_9571,  Common::GmType::DEFAULT},        maxBwRateOf950_957X},
    {{ChipProductType::ASCEND950DT_9572,  Common::GmType::DEFAULT},        maxBwRateOf950_957X},
    {{ChipProductType::ASCEND950DT_9573,  Common::GmType::DEFAULT},        maxBwRateOf950_957X},
    {{ChipProductType::ASCEND950DT_9574,  Common::GmType::DEFAULT},        maxBwRateOf950_957X},
    {{ChipProductType::ASCEND950DT_9575,  Common::GmType::DEFAULT},        maxBwRateOf950_957X},
    {{ChipProductType::ASCEND950DT_9576,  Common::GmType::DEFAULT},        maxBwRateOf950_957X},
    {{ChipProductType::ASCEND950DT_9577,  Common::GmType::DEFAULT},        maxBwRateOf950_957X},
    {{ChipProductType::ASCEND950DT_9578,  Common::GmType::DEFAULT},        maxBwRateOf950_957X},
    {{ChipProductType::ASCEND950DT_9581,  Common::GmType::DEFAULT},        maxBwRateOf950_958X},
    {{ChipProductType::ASCEND950DT_9582,  Common::GmType::DEFAULT},        maxBwRateOf950_958X},
    {{ChipProductType::ASCEND950DT_9583,  Common::GmType::DEFAULT},        maxBwRateOf950_958X},
    {{ChipProductType::ASCEND950DT_9584,  Common::GmType::DEFAULT},        maxBwRateOf950_958X},
    {{ChipProductType::ASCEND950DT_9585,  Common::GmType::DEFAULT},        maxBwRateOf950_958X},
    {{ChipProductType::ASCEND950DT_9586,  Common::GmType::DEFAULT},        maxBwRateOf950_958X},
    {{ChipProductType::ASCEND950DT_9587,  Common::GmType::DEFAULT},        maxBwRateOf950_958X},
    {{ChipProductType::ASCEND950DT_9588,  Common::GmType::DEFAULT},        maxBwRateOf950_958X},
    {{ChipProductType::ASCEND950DT_9591,  Common::GmType::DEFAULT},        maxBwRateOf950_959X},
    {{ChipProductType::ASCEND950DT_9592,  Common::GmType::DEFAULT},        maxBwRateOf950_959X},
    {{ChipProductType::ASCEND950DT_9595,  Common::GmType::DEFAULT},        maxBwRateOf950_959X},
    {{ChipProductType::ASCEND950DT_9596,  Common::GmType::DEFAULT},        maxBwRateOf950_959X},
    {{ChipProductType::ASCEND950DT_95A1,  Common::GmType::DEFAULT},        maxBwRateOf950_959X},
    {{ChipProductType::ASCEND950DT_95A2,  Common::GmType::DEFAULT},        maxBwRateOf950_959X},
    {{ChipProductType::ASCEND950PR_950Z,  Common::GmType::DEFAULT},        maxBwRateOf950_950X},
    {{ChipProductType::ASCEND950PR_9579,  Common::GmType::DEFAULT},        maxBwRateOf950_957X},
    {{ChipProductType::ASCEND950PR_957B,  Common::GmType::DEFAULT},        maxBwRateOf950_957X},
    {{ChipProductType::ASCEND950PR_957C,  Common::GmType::DEFAULT},        maxBwRateOf950_957X},
    {{ChipProductType::ASCEND950PR_957D,  Common::GmType::DEFAULT},        maxBwRateOf950_957X},
    {{ChipProductType::ASCEND950PR_9589,  Common::GmType::DEFAULT},        maxBwRateOf950_958X},
    {{ChipProductType::ASCEND950PR_958B,  Common::GmType::DEFAULT},        maxBwRateOf950_958X},
    {{ChipProductType::ASCEND950PR_9599,  Common::GmType::DEFAULT},        maxBwRateOf950_959X},
};
}
#endif // __MSOPPROF_PROFILING_DEVICE_DEF_H__
