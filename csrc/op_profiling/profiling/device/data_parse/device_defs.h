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

enum class EventOfA5 : uint64_t {
    // su pmu
    PMU_SCALAR_INSTR_BUSY_P = 1,
    PMU_RD_ACC_UB_INSTR_P = 3,
    PMU_WR_ACC_UB_INSTR_P = 5,
    PMU_CUBE_IQ_WAIT = 11,
    PMU_VEC_IQ_WAIT = 12,
    PMU_MTE1_IQ_WAIT = 13,
    PMU_MTE2_IQ_WAIT = 14,
    PMU_MTE3_IQ_WAIT = 15,
    PMU_ICACHE_REQ = 52,
    PMU_ICACHE_MISS = 53,

    // mte pmu
    MTE_SC_PMU_MTE2_INSTR = 512,
    MTE_SC_PMU_MTE3_INSTR = 513,
    MTE_SC_PMU_MTE2_INSTR_BUSY = 514,
    MTE_SC_PMU_MTE3_INSTR_BUSY = 515,

    // cube pmu
    CUBE_SC_PMU_CUBE_INSTR = 768,
    CUBE_SC_PMU_FP_INSTR = 770,
    CUBE_SC_PMU_INT_INSTR = 771,
    CUBE_SC_PMU_READ_L0A_INSTR = 772,
    CUBE_SC_PMU_READ_L0B_INSTR = 774,
    CUBE_SC_PMU_WRITE_L0C_INSTR = 776,
    CUBE_SC_PMU_READ_L0C_INSTR = 778,
    CUBE_SC_PMU_CUBE_FP_INSTR_CALC_CYC = 808,
    CUBE_SC_PMU_CUBE_INT_INSTR_CALC_CYC = 809,
    CUBE_SC_PMU_CUBE_INSTR_CALC_CYC = 810,

    // vector pmu
    PMU_CSW_FCSW = 1280,
    PMU_IDC_AIC_VEC_BUSY_O = 1281,
    PMU_IDC_AIC_VEC_INSTR_VF_BUSY_O = 1282,
    PMU_IDC_AIC_VEC_INSTR_SFU_BUSY_O = 1283,
    PMU_IDC_AIC_VEC_INSTR_SIMT_VF_BUSY_O = 1284,
    PMU_AIC_EXT_RD_UB_INSTR = 1391,
    PMU_AIC_EXT_WR_UB_INSTR = 1392,
    UB_PMU_VEC_RD_UB_ACC = 1393,
    DCU_PMU_REQ_LDG = 1395,
    DCU_PMU_REQ_STG = 1397,
    DCU_PMU_REQ_STK = 1398,
    DCU_PMU_REQ_LDS = 1401,
    DCU_PMU_REQ_STS = 1402,
    DCU_PMU_REQ_ATOM_SM = 1403,
    DCU_PMU_REQ_ATOM_GM = 1404,

    // bif pmu
    BIF_SC_PMU_READ_DATA_RECEIVED_R_CHN0 = 1035,
    BIF_SC_PMU_READ_DATA_RECEIVED_R_CHN3 = 1038,
    BIF_SC_PMU_WRITE_DATA_SENT_W_CHN0 = 1039,
    BIF_SC_PMU_AR_CLOSE_L2_HIT_CORE = 1060,
    BIF_SC_PMU_AR_FAR_L2_HIT_CORE = 1063,
    BIF_SC_PMU_AR_FAR_L2_MISS_CORE = 1064,
    BIF_SC_PMU_AW_CLOSE_L2_HIT_CORE = 1066,
    BIF_SC_PMU_AW_FAR_L2_HIT_CORE = 1069,
    BIF_SC_PMU_AW_FAR_L2_MISS_CORE = 1070,

    // l1 pmu
    PMU_MTE1_INSTR = 1792,
    PMU_MTE1_INSTR_BUSY = 1793,
    PMU_MTE1_INSTR_ACTIVE = 1794,
    PMU_WR_L0A_INSTR = 1795,
    PMU_WR_L0B_INSTR = 1797,
    PMU_RD_L1_INSTR = 1799,
    PMU_WR_L1_INSTR = 1801,
    PMU_FIXP_WR_UB_BUSY = 1803,
    PMU_FIXP_WR_L1_INSTR = 1806,
    PMU_FIXP_RD_L0C_INSTR = 1810,
    PMU_FIXP_INSTR_BUSY = 1812,
    PMU_FIXP_WR_UB1_INSTR = 1815
};

enum class TransportType : uint16_t {
    READ_MAIN_MEMORY = 0,
    WRITE_MAIN_MEMORY,
    GM_TO_L1,
    GM_TO_UB,
    GM_TO_DCACHE,
    DCACHE_TO_GM,
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
    {TransportType::READ_MAIN_MEMORY,        256},
    {TransportType::WRITE_MAIN_MEMORY,       128},
    {TransportType::CUBE_TO_L0C,             1024},
    {TransportType::GM_TO_L1,                256},
    {TransportType::GM_TO_UB,                256},
    {TransportType::GM_TO_DCACHE,            256},
    {TransportType::L1_TO_L0A,               256},
    {TransportType::L1_TO_L0B,               256},
    {TransportType::L1_TO_MTE,               256},
    {TransportType::L1_TO_UB,                128},
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
    {TransportType::DCACHE_TO_GM,            256},
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

const std::map<Common::ChipProductType, std::map<TransportType, uint16_t>> REQ_DATA_ALL = {
    {Common::ChipProductType::ASCEND910B_SERIES,       REQ_DATA_OF_910B},
    {Common::ChipProductType::ASCEND310P_SERIES,       REQ_DATA_OF_310P},
    {Common::ChipProductType::ASCEND910_95_SERIES,     REQ_DATA_OF_A5}
};

const std::map<TransportType, float> maxBwRateOf910B1 = {
    {TransportType::GM_TO_L1, 264},
    {TransportType::L1_TO_GM, Common::MAX_BW_910B1.at(std::string {Common::L1_TO_GM})},
    {TransportType::L0C_TO_L1, Common::MAX_BW_910B1.at(std::string {Common::L0C_TO_L1})},
    {TransportType::L0C_TO_GM, Common::MAX_BW_910B1.at(std::string {Common::L0C_TO_GM})},
    {TransportType::GM_TO_UB, Common::MAX_BW_910B1.at(std::string {Common::GM_TO_UB})},
    {TransportType::UB_TO_GM, Common::MAX_BW_910B1.at(std::string {Common::UB_TO_GM})},
};
 
const std::map<TransportType, float> maxBwRateOf910B1GmCJ = {
    {TransportType::GM_TO_L1, 296},
    {TransportType::L1_TO_GM, Common::MAX_BW_910B1_CJ.at(std::string {Common::L1_TO_GM})},
    {TransportType::L0C_TO_L1, Common::MAX_BW_910B1_CJ.at(std::string {Common::L0C_TO_L1})},
    {TransportType::L0C_TO_GM, Common::MAX_BW_910B1_CJ.at(std::string {Common::L0C_TO_GM})},
    {TransportType::GM_TO_UB, Common::MAX_BW_910B1_CJ.at(std::string {Common::GM_TO_UB})},
    {TransportType::UB_TO_GM, Common::MAX_BW_910B1_CJ.at(std::string {Common::UB_TO_GM})},
};
 
const std::map<TransportType, float> maxBwRateOf910B4 = {
    {TransportType::GM_TO_L1, 222},
    {TransportType::L1_TO_GM, Common::MAX_BW_910B4.at(std::string {Common::L1_TO_GM})},
    {TransportType::L0C_TO_L1, Common::MAX_BW_910B4.at(std::string {Common::L0C_TO_L1})},
    {TransportType::L0C_TO_GM, Common::MAX_BW_910B4.at(std::string {Common::L0C_TO_GM})},
    {TransportType::GM_TO_UB, Common::MAX_BW_910B4.at(std::string {Common::GM_TO_UB})},
    {TransportType::UB_TO_GM, Common::MAX_BW_910B4.at(std::string {Common::UB_TO_GM})},
};
 
const std::map<TransportType, float> maxBwRateOf910B4GmCJ = {
    {TransportType::GM_TO_L1, 274},
    {TransportType::L1_TO_GM, Common::MAX_BW_910B4.at(std::string {Common::L1_TO_GM})},
    {TransportType::L0C_TO_L1, Common::MAX_BW_910B4.at(std::string {Common::L0C_TO_L1})},
    {TransportType::L0C_TO_GM, Common::MAX_BW_910B4.at(std::string {Common::L0C_TO_GM})},
    {TransportType::GM_TO_UB, Common::MAX_BW_910B4.at(std::string {Common::GM_TO_UB})},
    {TransportType::UB_TO_GM, Common::MAX_BW_910B4.at(std::string {Common::UB_TO_GM})},
};
 
const std::map<TransportType, float> maxBwRateOf910_959X = {
    {TransportType::GM_TO_L1,                6392.52},
    {TransportType::L1_TO_GM,                5176.44},
    {TransportType::L0C_TO_L1,               14837.4},
    {TransportType::L0C_TO_GM,               5113.8},
    {TransportType::L0C_TO_UB,               7423.2},
    {TransportType::L1_TO_L0A,               14835.6},
    {TransportType::L1_TO_L0B,               14835.96},
    {TransportType::L1_TO_UB,                7422.48},
    {TransportType::UB_TO_L1,                7422.48},
    {TransportType::GM_TO_UB,                6406.56},
    {TransportType::UB_TO_GM,                5174.64},
};

const std::map<TransportType, float> maxBwRateOf910_958X = {
    {TransportType::GM_TO_L1,                5208.72},
    {TransportType::L1_TO_GM,                4217.84},
    {TransportType::L0C_TO_L1,               12089.73},
    {TransportType::L0C_TO_GM,               4166.8},
    {TransportType::L0C_TO_UB,               6048.53},
    {TransportType::L1_TO_L0A,               12088.27},
    {TransportType::L1_TO_L0B,               12088.56},
    {TransportType::L1_TO_UB,                6047.95},
    {TransportType::UB_TO_L1,                6047.95},
    {TransportType::GM_TO_UB,                5220.16},
    {TransportType::UB_TO_GM,                4216.37},
};

const std::map<TransportType, float> maxBwRateOf910_957X = {
    {TransportType::GM_TO_L1,                4557.63},
    {TransportType::L1_TO_GM,                3690.61},
    {TransportType::L0C_TO_L1,               10578.52},
    {TransportType::L0C_TO_GM,               3645.95},
    {TransportType::L0C_TO_UB,               5292.47},
    {TransportType::L1_TO_L0A,               10577.23},
    {TransportType::L1_TO_L0B,               10577.49},
    {TransportType::L1_TO_UB,                5291.95},
    {TransportType::UB_TO_L1,                5291.95},
    {TransportType::GM_TO_UB,                4567.64},
    {TransportType::UB_TO_GM,                3689.33},
};

const std::map<TransportType, float> maxBwRateOf910_950X = {
    {TransportType::GM_TO_L1,                1302.18},
    {TransportType::L1_TO_GM,                1054.46},
    {TransportType::L0C_TO_L1,               3022.43},
    {TransportType::L0C_TO_GM,               1041.7},
    {TransportType::L0C_TO_UB,               1512.13},
    {TransportType::L1_TO_L0A,               3022.07},
    {TransportType::L1_TO_L0B,               3022.14},
    {TransportType::L1_TO_UB,                1511.99},
    {TransportType::UB_TO_L1,                1511.99},
    {TransportType::GM_TO_UB,                1305.04},
    {TransportType::UB_TO_GM,                1054.09},
};

const std::map<std::string, std::map<TransportType, float>> MAX_BW_RATE_A5 = {
    {"Ascend910_9599",   maxBwRateOf910_959X},
    {"Ascend910_9581",   maxBwRateOf910_958X},
    {"Ascend910_9589",   maxBwRateOf910_958X},
    {"Ascend910_958a",   maxBwRateOf910_958X},
    {"Ascend910_958b",   maxBwRateOf910_958X},
    {"Ascend910_9579",   maxBwRateOf910_957X},
    {"Ascend910_957b",   maxBwRateOf910_957X},
    {"Ascend910_957d",   maxBwRateOf910_957X},
    {"Ascend910_950z",   maxBwRateOf910_950X},
};
 
const std::map<std::string, std::map<TransportType, float>> MAX_BW_RATE = {
    {"Ascend910B1",   maxBwRateOf910B1},
    {"Ascend910B2",   maxBwRateOf910B1},
    {"Ascend910B3",   maxBwRateOf910B1},
    {"Ascend910B4",   maxBwRateOf910B4},
    {"Ascend910B4-1", maxBwRateOf910B4},
    {"Ascend910B2C",  maxBwRateOf910B4}
};
 
const std::map<std::string, std::map<TransportType, float>> MAX_BW_RATE_GM_CJ = {
    {"Ascend910B1",   maxBwRateOf910B1GmCJ},
    {"Ascend910B2",   maxBwRateOf910B1GmCJ},
    {"Ascend910B3",   maxBwRateOf910B1GmCJ},
    {"Ascend910B4",   maxBwRateOf910B4GmCJ},
    {"Ascend910B4-1", maxBwRateOf910B4GmCJ},
    {"Ascend910B2C",  maxBwRateOf910B4GmCJ}
};
 
const std::map<Common::GmType, std::map<std::string, std::map<TransportType, float>>> GM_PRODUCT_BW = {
    {Common::GmType::CJ, MAX_BW_RATE_GM_CJ},
    {Common::GmType::SK, MAX_BW_RATE},
    {Common::GmType::SS, MAX_BW_RATE},
    {Common::GmType::DEFAULT, MAX_BW_RATE}
};

const std::map<std::pair<Common::ChipProductType, Common::GmType>, std::map<TransportType, float>> MAX_BW_RATE_ALL = {
    {{Common::ChipProductType::ASCEND910B1,     Common::GmType::DEFAULT},        maxBwRateOf910B1},
    {{Common::ChipProductType::ASCEND910B2,     Common::GmType::DEFAULT},        maxBwRateOf910B1},
    {{Common::ChipProductType::ASCEND910B3,     Common::GmType::DEFAULT},        maxBwRateOf910B1},
    {{Common::ChipProductType::ASCEND910B4,     Common::GmType::DEFAULT},        maxBwRateOf910B4},
    {{Common::ChipProductType::ASCEND910B4_1,   Common::GmType::DEFAULT},        maxBwRateOf910B4},
    {{Common::ChipProductType::ASCEND910B2C,    Common::GmType::DEFAULT},        maxBwRateOf910B4},
    {{Common::ChipProductType::ASCEND910B1,     Common::GmType::CJ},             maxBwRateOf910B1GmCJ},
    {{Common::ChipProductType::ASCEND910B2,     Common::GmType::CJ},             maxBwRateOf910B1GmCJ},
    {{Common::ChipProductType::ASCEND910B3,     Common::GmType::CJ},             maxBwRateOf910B1GmCJ},
    {{Common::ChipProductType::ASCEND910B4,     Common::GmType::CJ},             maxBwRateOf910B4GmCJ},
    {{Common::ChipProductType::ASCEND910B4_1,   Common::GmType::CJ},             maxBwRateOf910B4GmCJ},
    {{Common::ChipProductType::ASCEND910B2C,    Common::GmType::CJ},             maxBwRateOf910B4GmCJ},
    {{Common::ChipProductType::ASCEND910_9599,  Common::GmType::DEFAULT},        maxBwRateOf910_959X},
    {{Common::ChipProductType::ASCEND910_9581,  Common::GmType::DEFAULT},        maxBwRateOf910_958X},
    {{Common::ChipProductType::ASCEND910_9589,  Common::GmType::DEFAULT},        maxBwRateOf910_958X},
    {{Common::ChipProductType::ASCEND910_958A,  Common::GmType::DEFAULT},        maxBwRateOf910_958X},
    {{Common::ChipProductType::ASCEND910_958B,  Common::GmType::DEFAULT},        maxBwRateOf910_958X},
    {{Common::ChipProductType::ASCEND910_9579,  Common::GmType::DEFAULT},        maxBwRateOf910_957X},
    {{Common::ChipProductType::ASCEND910_957B,  Common::GmType::DEFAULT},        maxBwRateOf910_957X},
    {{Common::ChipProductType::ASCEND910_957D,  Common::GmType::DEFAULT},        maxBwRateOf910_957X},
    {{Common::ChipProductType::ASCEND910_950Z,  Common::GmType::DEFAULT},        maxBwRateOf910_950X},
};
}
 
#endif // __MSOPPROF_PROFILING_DEVICE_DEF_H__
