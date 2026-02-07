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

#ifndef __MSOPPROF_PROFILING_PMU_FORMULA_H__
#define __MSOPPROF_PROFILING_PMU_FORMULA_H__

#include <map>
#include <string>
#include <vector>

#include "pmu_calculate.h"
#include "equation_utils.h"
#include "number_operation.h"

namespace Profiling {
// Ascend910B
const std::map<std::string, CalInfo> FormulaFor910B = {
    {"aic_time(us)", {{}, {}, FuncType::TOTAL_TIME}},
    {"aic_total_cycles", {{}, {}, FuncType::TOTAL_CYCLES}},
    {"aic_cube_ratio", {{10}, {}, FuncType::RATIO}},
    {"aic_cube_fp16_ratio", {{73, 74, 10}, {}, FuncType::CUBE_RATIO}},
    {"aic_cube_int8_ratio", {{74, 73, 10}, {}, FuncType::CUBE_RATIO}},
    {"aic_cube_fops", {{1032, 1033}, {}, FuncType::CUBE_FOPS}},
    {"aic_cube_total_instr_number", {{3}, {}, FuncType::PMU_SELF}},
    {"aic_cube_fp_instr_number", {{1032}, {}, FuncType::PMU_SELF}},
    {"aic_cube_int_instr_number", {{1033}, {}, FuncType::PMU_SELF}},
    {"aiv_time(us)", {{}, {}, FuncType::TOTAL_TIME}},
    {"aiv_total_cycles", {{}, {}, FuncType::TOTAL_CYCLES}},
    {"aiv_vec_ratio", {{8}, {}, FuncType::RATIO}},
    {"aiv_vec_fp32_ratio", {{8, 1, 75, 76}, {}, FuncType::VEC_FP32_RATIO}},
    {"aiv_vec_fp16_ratio", {{8, 1, 76, 77, 174}, {}, FuncType::VEC_RATIO}},
    {"aiv_vec_int32_ratio", {{8, 1, 78}, {}, FuncType::VEC_RATIO}},
    {"aiv_vec_int16_ratio", {{8, 1, 186}, {}, FuncType::VEC_RATIO}},
    {"aiv_vec_misc_ratio", {{8, 1, 79}, {}, FuncType::VEC_RATIO}},
    {"aiv_vec_fops", {{75, 76, 77, 174, 78, 186, 79}, {}, FuncType::VEC_FOPS}},

    {"aic_write_cache_hit", {{1280}, {}, FuncType::PMU_SELF}},
    {"aic_write_cache_miss_allocate", {{1282}, {}, FuncType::PMU_SELF}},
    {"aic_r0_read_cache_hit", {{1284}, {}, FuncType::PMU_SELF}},
    {"aic_r0_read_cache_miss_allocate", {{1286}, {}, FuncType::PMU_SELF}},
    {"aic_r1_read_cache_hit", {{1288}, {}, FuncType::PMU_SELF}},
    {"aic_r1_read_cache_miss_allocate", {{1290}, {}, FuncType::PMU_SELF}},
    {"aic_write_hit_rate(%)", {{1280, 1282, 1283}, {}, FuncType::WRITE_HIT_RATE}},
    {"aic_read_hit_rate(%)", {{1284, 1286, 1287, 1288, 1290, 1291}, {}, FuncType::READ_HIT_RATE}},
    {"aic_total_hit_rate(%)", {{1280, 1282, 1283, 1284, 1286, 1287, 1288, 1290, 1291}, {}, FuncType::TOTAL_HIT_RATE}},
    {"aiv_write_cache_hit", {{1280}, {}, FuncType::PMU_SELF}},
    {"aiv_write_cache_miss_allocate", {{1282}, {}, FuncType::PMU_SELF}},
    {"aiv_r0_read_cache_hit", {{1284}, {}, FuncType::PMU_SELF}},
    {"aiv_r0_read_cache_miss_allocate", {{1286}, {}, FuncType::PMU_SELF}},
    {"aiv_r1_read_cache_hit", {{1288}, {}, FuncType::PMU_SELF}},
    {"aiv_r1_read_cache_miss_allocate", {{1290}, {}, FuncType::PMU_SELF}},
    {"aiv_write_hit_rate(%)", {{1280, 1282, 1283}, {}, FuncType::WRITE_HIT_RATE}},
    {"aiv_read_hit_rate(%)", {{1284, 1286, 1287, 1288, 1290, 1291}, {}, FuncType::READ_HIT_RATE}},
    {"aiv_total_hit_rate(%)", {{1280, 1282, 1283, 1284, 1286, 1287, 1288, 1290, 1291}, {}, FuncType::TOTAL_HIT_RATE}},

    {"aic_l1_read_bw(GB/s)", {{49}, {256, 8}, FuncType::BANDWIDTH}},
    {"aic_l1_write_bw(GB/s)", {{50}, {256, 8}, FuncType::BANDWIDTH}},
    {"aic_main_mem_read_bw(GB/s)", {{1293, 1294}, {128, 8}, FuncType::READ_MAIN_MEMORY_BANDWIDTH}},
    {"aic_main_mem_write_bw(GB/s)", {{1292}, {128, 8}, FuncType::BANDWIDTH}},
    {"aic_mte1_instructions", {{4}, {}, FuncType::PMU_SELF}},
    {"aic_mte1_ratio", {{770}, {}, FuncType::RATIO}},
    {"aic_mte2_instructions", {{5}, {}, FuncType::PMU_SELF}},
    {"aic_mte2_ratio", {{12}, {}, FuncType::RATIO}},
    {"aic_mte3_instructions", {{6}, {}, FuncType::PMU_SELF}},
    {"aic_mte3_ratio", {{13}, {}, FuncType::RATIO}},
    {"aiv_ub_to_gm_bw(GB/s)", {{61}, {128, 8}, FuncType::BANDWIDTH}},
    {"aiv_gm_to_ub_bw(GB/s)", {{62}, {128, 8}, FuncType::BANDWIDTH}},
    {"aiv_main_mem_read_bw(GB/s)", {{1293, 1294}, {128, 8}, FuncType::READ_MAIN_MEMORY_BANDWIDTH}},
    {"aiv_main_mem_write_bw(GB/s)", {{1292}, {128, 8}, FuncType::BANDWIDTH}},
    {"aiv_mte2_instructions", {{5}, {}, FuncType::PMU_SELF}},
    {"aiv_mte2_ratio", {{12}, {}, FuncType::RATIO}},
    {"aiv_mte3_instructions", {{6}, {}, FuncType::PMU_SELF}},
    {"aiv_mte3_ratio", {{13}, {}, FuncType::RATIO}},
    {"read_main_memory_datas(KB)", {{1293, 1294}, {}, FuncType::READ_MAIN_MEMORY_DATAS}},
    {"write_main_memory_datas(KB)", {{1292}, {}, FuncType::WRITE_MAIN_MEMORY_DATAS}},
    {"GM_to_L1_datas(KB)", {{50, 518}, {}, FuncType::GM_TO_L1_DATAS}},
    {"GM_to_L1_bw_usage_rate(%)", {{50, 518}, {}, FuncType::GM_TO_L1_BW_USAGE_RATE}},
    {"L1_to_GM_datas(KB)(estimate)", {{19, 518, 524}, {}, FuncType::L1_TO_GM_DATAS}},
    {"L1_to_GM_bw_usage_rate(%)(estimate)", {{19, 518, 524}, {}, FuncType::L1_TO_GM_BW_USAGE_RATE}},
    {"L0C_to_L1_datas(KB)", {{518}, {}, FuncType::L0C_TO_L1_DATAS}},
    {"L0C_to_L1_bw_usage_rate(%)", {{518}, {}, FuncType::L0C_TO_L1_BW_USAGE_RATE}},
    {"L0C_to_GM_datas(KB)", {{518, 524}, {}, FuncType::L0C_TO_GM_DATAS}},
    {"L0C_to_GM_bw_usage_rate(%)", {{518, 524}, {}, FuncType::L0C_TO_GM_BW_USAGE_RATE}},
    {"GM_to_UB_datas(KB)", {{62}, {}, FuncType::GM_TO_UB_DATAS}},
    {"GM_to_UB_bw_usage_rate(%)", {{62}, {}, FuncType::GM_TO_UB_BW_USAGE_RATE}},
    {"UB_to_GM_datas(KB)", {{61}, {}, FuncType::UB_TO_GM_DATAS}},
    {"UB_to_GM_bw_usage_rate(%)", {{61}, {}, FuncType::UB_TO_GM_BW_USAGE_RATE}},

    {"aic_l0a_read_bw(GB/s)", {{27}, {256, 16}, FuncType::BANDWIDTH}},
    {"aic_l0a_write_bw(GB/s)", {{28}, {256, 8}, FuncType::BANDWIDTH}},
    {"aic_l0b_read_bw(GB/s)", {{33}, {256, 16}, FuncType::BANDWIDTH}},
    {"aic_l0b_write_bw(GB/s)", {{34}, {128, 8}, FuncType::BANDWIDTH}},
    {"aic_l0c_read_bw_cube(GB/s)", {{40}, {256, 32}, FuncType::BANDWIDTH}},
    {"aic_l0c_write_bw_cube(GB/s)", {{42}, {256, 32}, FuncType::BANDWIDTH}},

    {"aiv_ub_read_bw_vector(GB/s)", {{67}, {256, 8}, FuncType::BANDWIDTH}},
    {"aiv_ub_write_bw_vector(GB/s)", {{68}, {256, 8}, FuncType::BANDWIDTH}},
    {"aiv_ub_read_bw_scalar(GB/s)", {{55}, {128, 1}, FuncType::BANDWIDTH}},
    {"aiv_ub_write_bw_scalar(GB/s)", {{56}, {128, 1}, FuncType::BANDWIDTH}},

    {"aic_cube_time(us)", {{10}, {}, FuncType::PIPE_TIME}},
    {"aic_scalar_time(us)", {{9}, {}, FuncType::PIPE_TIME}},
    {"aic_scalar_ratio", {{9}, {}, FuncType::RATIO}},
    {"aic_mte1_time(us)", {{770}, {}, FuncType::PIPE_TIME}},
    {"aic_mte1_ratio", {{770}, {}, FuncType::RATIO}},
    {"aic_mte1_active_bw(GB/s)", {{28, 34, 770}, {256, 8, 128, 8}, FuncType::AIC_MTE1_ACTIVATE_BANDWIDTH}},
    {"aic_mte2_time(us)", {{12}, {}, FuncType::PIPE_TIME}},
    {"aic_mte2_ratio", {{12}, {}, FuncType::RATIO}},
    {"aic_mte2_active_bw(GB/s)", {{12}, {}, FuncType::AIC_MTE2_ACTIVATE_BANDWIDTH}},
    {"aic_mte3_time(us)", {{13}, {}, FuncType::PIPE_TIME}},
    {"aic_mte3_ratio", {{13}, {}, FuncType::RATIO}},
    {"aic_mte3_active_bw(GB/s)", {{19, 518, 524, 13}, {128}, FuncType::AIC_MTE3_ACTIVATE_BANDWIDTH}},
    {"aic_fixpipe_time(us)", {{771}, {}, FuncType::PIPE_TIME}},
    {"aic_fixpipe_ratio", {{771}, {}, FuncType::RATIO}},
    {"aic_fixpipe_active_bw(GB/s)", {{524, 771}, {128}, FuncType::AIV_MTE_ACTIVATE_BANDWIDTH}},
    {"aic_icache_miss_rate", {{85, 84}, {}, FuncType::PMU_DIV}},

    {"aic_scalar_single_time(us)", {{112}, {}, FuncType::PIPE_TIME}},
    {"aic_scalar_dual_time(us)", {{113}, {}, FuncType::PIPE_TIME}},
    {"aic_scalar_mte1_stall_time(us)", {{107}, {}, FuncType::PIPE_TIME}},
    {"aic_scalar_mte2_stall_time(us)", {{108}, {}, FuncType::PIPE_TIME}},
    {"aic_scalar_mte3_stall_time(us)", {{109}, {}, FuncType::PIPE_TIME}},
    {"aic_scalar_cube_stall_time(us)", {{110}, {}, FuncType::PIPE_TIME}},
    {"aic_scalar_wait_ib_time(us)", {{114}, {}, FuncType::PIPE_TIME}},
    {"aic_scalar_wait_time(us)", {{87}, {}, FuncType::PIPE_TIME}},

    {"aic_scalar_wait_id0_time(us)", {{1792}, {}, FuncType::PIPE_TIME}},
    {"aic_scalar_wait_id1_time(us)", {{1793}, {}, FuncType::PIPE_TIME}},
    {"aic_scalar_wait_id2_time(us)", {{1794}, {}, FuncType::PIPE_TIME}},
    {"aic_scalar_wait_id3_time(us)", {{1795}, {}, FuncType::PIPE_TIME}},
    {"aic_scalar_wait_id4_time(us)", {{1796}, {}, FuncType::PIPE_TIME}},
    {"aic_scalar_wait_id5_time(us)", {{1797}, {}, FuncType::PIPE_TIME}},
    {"aic_scalar_wait_id6_time(us)", {{1798}, {}, FuncType::PIPE_TIME}},
    {"aic_scalar_wait_id7_time(us)", {{1799}, {}, FuncType::PIPE_TIME}},
    {"aic_scalar_wait_id8_time(us)", {{1780}, {}, FuncType::PIPE_TIME}},
    {"aic_scalar_wait_id9_time(us)", {{1781}, {}, FuncType::PIPE_TIME}},
    {"aic_scalar_wait_id10_time(us)", {{1782}, {}, FuncType::PIPE_TIME}},
    {"aic_scalar_wait_id11_time(us)", {{1783}, {}, FuncType::PIPE_TIME}},
    {"aic_scalar_wait_id12_time(us)", {{1784}, {}, FuncType::PIPE_TIME}},
    {"aic_scalar_wait_id13_time(us)", {{1785}, {}, FuncType::PIPE_TIME}},
    {"aic_scalar_wait_id14_time(us)", {{1786}, {}, FuncType::PIPE_TIME}},
    {"aic_scalar_wait_id15_time(us)", {{1787}, {}, FuncType::PIPE_TIME}},


    {"aiv_vec_time(us)", {{8}, {}, FuncType::PIPE_TIME}},
    {"aiv_scalar_time(us)", {{9}, {}, FuncType::PIPE_TIME}},
    {"aiv_scalar_ratio", {{9}, {}, FuncType::RATIO}},
    {"aiv_mte2_time(us)", {{12}, {}, FuncType::PIPE_TIME}},
    {"aiv_mte2_ratio", {{12}, {}, FuncType::RATIO}},
    {"aiv_mte2_active_bw(GB/s)", {{62, 12}, {128}, FuncType::AIV_MTE_ACTIVATE_BANDWIDTH}},
    {"aiv_mte3_time(us)", {{13}, {}, FuncType::PIPE_TIME}},
    {"aiv_mte3_ratio", {{13}, {}, FuncType::RATIO}},
    {"aiv_mte3_active_bw(GB/s)", {{61, 13}, {128}, FuncType::AIV_MTE_ACTIVATE_BANDWIDTH}},
    {"aiv_icache_miss_rate", {{85, 84}, {}, FuncType::PMU_DIV}},

    {"aiv_scalar_single_time(us)", {{112}, {}, FuncType::PIPE_TIME}},
    {"aiv_scalar_dual_time(us)", {{113}, {}, FuncType::PIPE_TIME}},
    {"aiv_scalar_mte2_stall_time(us)", {{108}, {}, FuncType::PIPE_TIME}},
    {"aiv_scalar_mte3_stall_time(us)", {{109}, {}, FuncType::PIPE_TIME}},
    {"aiv_scalar_stall_by_ub_time(us)", {{106}, {}, FuncType::PIPE_TIME}},
    {"aiv_scalar_vector_stall_time(us)", {{111}, {}, FuncType::PIPE_TIME}},
    {"aiv_scalar_wait_ib_time(us)", {{114}, {}, FuncType::PIPE_TIME}},
    {"aiv_scalar_wait_time(us)", {{87}, {}, FuncType::PIPE_TIME}},

    {"aiv_scalar_wait_id0_time(us)", {{1792}, {}, FuncType::PIPE_TIME}},
    {"aiv_scalar_wait_id1_time(us)", {{1793}, {}, FuncType::PIPE_TIME}},
    {"aiv_scalar_wait_id2_time(us)", {{1794}, {}, FuncType::PIPE_TIME}},
    {"aiv_scalar_wait_id3_time(us)", {{1795}, {}, FuncType::PIPE_TIME}},
    {"aiv_scalar_wait_id4_time(us)", {{1796}, {}, FuncType::PIPE_TIME}},
    {"aiv_scalar_wait_id5_time(us)", {{1797}, {}, FuncType::PIPE_TIME}},
    {"aiv_scalar_wait_id6_time(us)", {{1798}, {}, FuncType::PIPE_TIME}},
    {"aiv_scalar_wait_id7_time(us)", {{1799}, {}, FuncType::PIPE_TIME}},
    {"aiv_scalar_wait_id8_time(us)", {{1780}, {}, FuncType::PIPE_TIME}},
    {"aiv_scalar_wait_id9_time(us)", {{1781}, {}, FuncType::PIPE_TIME}},
    {"aiv_scalar_wait_id10_time(us)", {{1782}, {}, FuncType::PIPE_TIME}},
    {"aiv_scalar_wait_id11_time(us)", {{1783}, {}, FuncType::PIPE_TIME}},
    {"aiv_scalar_wait_id12_time(us)", {{1784}, {}, FuncType::PIPE_TIME}},
    {"aiv_scalar_wait_id13_time(us)", {{1785}, {}, FuncType::PIPE_TIME}},
    {"aiv_scalar_wait_id14_time(us)", {{1786}, {}, FuncType::PIPE_TIME}},
    {"aiv_scalar_wait_id15_time(us)", {{1787}, {}, FuncType::PIPE_TIME}},

    {"aic_cube_wait_ratio", {{88}, {}, FuncType::RATIO}},
    {"aic_mte1_wait_ratio", {{90}, {}, FuncType::RATIO}},
    {"aic_mte2_wait_ratio", {{91}, {}, FuncType::RATIO}},
    {"aic_mte3_wait_ratio", {{92}, {}, FuncType::RATIO}},
    {"aiv_vec_total_cflt_ratio", {{100, 101, 102, 103}, {}, FuncType::SUM_RATIO}},
    {"aiv_vec_bankgroup_cflt_ratio", {{100}, {}, FuncType::RATIO}},
    {"aiv_vec_bank_cflt_ratio", {{101}, {}, FuncType::RATIO}},
    {"aiv_vec_resc_cflt_ratio", {{102}, {}, FuncType::RATIO}},
    {"aiv_vec_mte_cflt_ratio", {{103}, {}, FuncType::RATIO}},
    {"aiv_vec_wait_ratio", {{89}, {}, FuncType::RATIO}},
    {"aiv_mte2_wait_ratio", {{91}, {}, FuncType::RATIO}},
    {"aiv_mte3_wait_ratio", {{92}, {}, FuncType::RATIO}},
};

// Ascend310P
const std::map<std::string, CalInfo> FormulaFor310P = {
    {"aic_time(us)", {{}, {}, FuncType::TOTAL_TIME}},
    {"aic_total_cycles", {{}, {}, FuncType::TOTAL_CYCLES}},
    {"aic_cube_ratio", {{10}, {}, FuncType::RATIO}},
    {"aic_cube_fp16_ratio", {{73, 74, 10}, {}, FuncType::CUBE_RATIO}},
    {"aic_cube_int8_ratio", {{74, 73, 10}, {}, FuncType::CUBE_RATIO}},
    {"aic_cube_fops", {{73, 74}, {}, FuncType::CUBE_FOPS}},
    {"aic_cube_total_instr_number", {{3}, {}, FuncType::PMU_SELF}},
    {"aic_vec_ratio", {{8}, {}, FuncType::RATIO}},
    {"aic_vec_fp32_ratio", {{8, 1, 75}, {}, FuncType::VEC_RATIO}},
    {"aic_vec_fp16_ratio", {{8, 1, 76, 77, 174}, {}, FuncType::VEC_RATIO}},
    {"aic_vec_int32_ratio", {{8, 1, 78}, {}, FuncType::VEC_RATIO}},
    {"aic_vec_int16_ratio", {{8, 1, 186}, {}, FuncType::VEC_RATIO}},
    {"aic_vec_misc_ratio", {{8, 1, 79}, {}, FuncType::VEC_RATIO}},
    {"aic_vec_fops", {{75, 76, 77, 174, 78, 186, 79}, {}, FuncType::VEC_FOPS}},

    {"aic_l2_cache_hit_rate(%)", {{106, 120, 121}, {}, FuncType::L2_CACHE_HIT_RATE}},

    {"aic_l1_read_bw(GB/s)", {{49}, {512, 8}, FuncType::BANDWIDTH}},
    {"aic_l1_write_bw(GB/s)", {{50}, {256, 8}, FuncType::BANDWIDTH}},
    {"aic_ub_to_gm_bw(GB/s)", {{61}, {16, 8}, FuncType::BANDWIDTH}},
    {"aic_gm_to_ub_bw(GB/s)", {{62}, {16, 8}, FuncType::BANDWIDTH}},
    {"aic_main_mem_read_bw(GB/s)", {{18}, {512, 8}, FuncType::BANDWIDTH}},
    {"aic_main_mem_write_bw(GB/s)", {{19}, {512, 8}, FuncType::BANDWIDTH}},
    {"aic_mte1_instructions", {{4}, {}, FuncType::PMU_SELF}},
    {"aic_mte1_ratio", {{11}, {}, FuncType::RATIO}},
    {"aic_mte2_instructions", {{5}, {}, FuncType::PMU_SELF}},
    {"aic_mte2_ratio", {{12}, {}, FuncType::RATIO}},
    {"aic_mte3_instructions", {{6}, {}, FuncType::PMU_SELF}},
    {"aic_mte3_ratio", {{13}, {}, FuncType::RATIO}},

    {"aic_l0a_read_bw(GB/s)", {{27}, {256, 16}, FuncType::BANDWIDTH}},
    {"aic_l0a_write_bw(GB/s)", {{28}, {256, 16}, FuncType::BANDWIDTH}},
    {"aic_l0b_read_bw(GB/s)", {{33}, {256, 16}, FuncType::BANDWIDTH}},
    {"aic_l0b_write_bw(GB/s)", {{34}, {128, 8}, FuncType::BANDWIDTH}},
    {"aic_l0c_read_bw_cube(GB/s)", {{40}, {256, 32}, FuncType::BANDWIDTH}},
    {"aic_l0c_write_bw_cube(GB/s)", {{42}, {256, 32}, FuncType::BANDWIDTH}},
    {"aic_l0c_read_bw(GB/s)", {{39}, {256, 8}, FuncType::BANDWIDTH}},
    {"aic_l0c_write_bw(GB/s)", {{41}, {256, 8}, FuncType::BANDWIDTH}},

    {"aic_ub_read_bw_vector(GB/s)", {{67}, {32, 8}, FuncType::BANDWIDTH}},
    {"aic_ub_write_bw_vector(GB/s)", {{68}, {32, 8}, FuncType::BANDWIDTH}},
    {"aic_ub_read_bw_scalar(GB/s)", {{55}, {128, 1}, FuncType::BANDWIDTH}},
    {"aic_ub_write_bw_scalar(GB/s)", {{56}, {128, 1}, FuncType::BANDWIDTH}},

    {"aic_cube_time(us)", {{10}, {}, FuncType::PIPE_TIME}},
    {"aic_scalar_time(us)", {{9}, {}, FuncType::PIPE_TIME}},
    {"aic_scalar_ratio", {{9}, {}, FuncType::RATIO}},
    {"aic_mte1_time(us)", {{11}, {}, FuncType::PIPE_TIME}},
    {"aic_mte1_ratio", {{11}, {}, FuncType::RATIO}},
    {"aic_mte2_time(us)", {{12}, {}, FuncType::PIPE_TIME}},
    {"aic_mte2_ratio", {{12}, {}, FuncType::RATIO}},
    {"aic_mte3_time(us)", {{13}, {}, FuncType::PIPE_TIME}},
    {"aic_mte3_ratio", {{13}, {}, FuncType::RATIO}},
    {"aic_icache_miss_rate", {{85, 84}, {}, FuncType::PMU_DIV}},
    {"aic_vec_time(us)", {{8}, {}, FuncType::PIPE_TIME}},

    {"aic_cube_wait_ratio", {{88}, {}, FuncType::RATIO}},
    {"aic_vec_wait_ratio", {{89}, {}, FuncType::RATIO}},
    {"aic_mte1_wait_ratio", {{90}, {}, FuncType::RATIO}},
    {"aic_mte2_wait_ratio", {{91}, {}, FuncType::RATIO}},
    {"aic_mte3_wait_ratio", {{92}, {}, FuncType::RATIO}},
    {"aic_vec_total_cflt_ratio", {{100, 101, 102, 103}, {}, FuncType::SUM_RATIO}},
    {"aic_vec_bankgroup_cflt_ratio", {{100}, {}, FuncType::RATIO}},
    {"aic_vec_bank_cflt_ratio", {{101}, {}, FuncType::RATIO}},
    {"aic_vec_resc_cflt_ratio", {{102}, {}, FuncType::RATIO}},
    {"aic_vec_mte_cflt_ratio", {{103}, {}, FuncType::RATIO}},
};

// AscendA5
const std::map<std::string, std::function<std::string(const CalculateParams &params)>> FormulaForA5 {
    {"aic_time(us)", [](const CalculateParams &params) {
        return Ratio(params.totalCycles, params.frequency);}},
    {"aic_total_cycles", [](const CalculateParams &params) {
        return std::to_string(params.totalCycles);}},
    {"aic_cube_ratio", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(810), params.totalCycles);}},
    {"aic_cube_fp_ratio", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(808), params.totalCycles);}},
    {"aic_cube_int_ratio", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(809), params.totalCycles);}},
    {"aic_cube_total_instr_number", [](const CalculateParams &params) {
        return std::to_string(params.pmuMap.At(768));}},
    {"aic_cube_fp_instr_number", [](const CalculateParams &params) {
        return std::to_string(params.pmuMap.At(789));}},
    {"aic_cube_int_instr_number", [](const CalculateParams &params) {
        return std::to_string(params.pmuMap.At(790));}},
    {"aiv_time(us)", [](const CalculateParams &params) {
        return Ratio(params.totalCycles, params.frequency);}},
    {"aiv_total_cycles", [](const CalculateParams &params) {
        return std::to_string(params.totalCycles);}},
    {"aiv_vec_ratio", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(1281), params.totalCycles);}},
    {"aiv_vec_vf_ratio", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(1282), params.totalCycles);}},
    {"aiv_vec_sfu_ratio", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(1283), params.totalCycles);}},
    {"aiv_vec_simt_vf_ratio", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(1284), params.totalCycles);}},

    {"aic_read_close_hit", [](const CalculateParams &params) { return std::to_string(params.pmuMap.At(1060)); }},
    {"aic_read_close_miss", [](const CalculateParams &params) { return std::to_string(params.pmuMap.At(1061)); }},
    {"aic_read_close_victim", [](const CalculateParams &params) { return std::to_string(params.pmuMap.At(1062)); }},
    {"aic_read_far_hit", [](const CalculateParams &params) { return std::to_string(params.pmuMap.At(1063)); }},
    {"aic_read_far_miss", [](const CalculateParams &params) { return std::to_string(params.pmuMap.At(1064)); }},
    {"aic_read_far_victim", [](const CalculateParams &params) { return std::to_string(params.pmuMap.At(1065)); }},
    {"aic_read_hit_rate(%)", [](const CalculateParams &params) {
        uint64_t val1 = Utility::SafeAdd(params.pmuMap.At(1060), params.pmuMap.At(1063), "read hit");
        uint64_t val2 = Utility::SafeAddAll<uint64_t>({params.pmuMap.At(1060), params.pmuMap.At(1061), params.pmuMap.At(1062),
            params.pmuMap.At(1063), params.pmuMap.At(1064), params.pmuMap.At(1065)}, "read total");
        return std::to_string(RatioFp(val1, val2) * PERCENTAGE_CONVERSION);}},
    {"aic_write_close_hit", [](const CalculateParams &params) { return std::to_string(params.pmuMap.At(1066)); }},
    {"aic_write_close_miss", [](const CalculateParams &params) { return std::to_string(params.pmuMap.At(1067)); }},
    {"aic_write_close_victim", [](const CalculateParams &params) { return std::to_string(params.pmuMap.At(1068)); }},
    {"aic_write_far_hit", [](const CalculateParams &params) { return std::to_string(params.pmuMap.At(1069)); }},
    {"aic_write_far_miss", [](const CalculateParams &params) { return std::to_string(params.pmuMap.At(1070)); }},
    {"aic_write_far_victim", [](const CalculateParams &params) { return std::to_string(params.pmuMap.At(1071)); }},
    {"aic_write_hit_rate(%)", [](const CalculateParams &params) {
        uint64_t val1 = Utility::SafeAdd(params.pmuMap.At(1066), params.pmuMap.At(1069), "write hit");
        uint64_t val2 = Utility::SafeAddAll<uint64_t>({params.pmuMap.At(1066), params.pmuMap.At(1067), params.pmuMap.At(1068),
            params.pmuMap.At(1069), params.pmuMap.At(1070), params.pmuMap.At(1071)}, "write total");
        return std::to_string(RatioFp(val1, val2) * PERCENTAGE_CONVERSION);}},
    {"aiv_read_close_hit", [](const CalculateParams &params) { return std::to_string(params.pmuMap.At(1060)); }},
    {"aiv_read_close_miss", [](const CalculateParams &params) { return std::to_string(params.pmuMap.At(1061)); }},
    {"aiv_read_close_victim", [](const CalculateParams &params) { return std::to_string(params.pmuMap.At(1062)); }},
    {"aiv_read_far_hit", [](const CalculateParams &params) { return std::to_string(params.pmuMap.At(1063)); }},
    {"aiv_read_far_miss", [](const CalculateParams &params) { return std::to_string(params.pmuMap.At(1064)); }},
    {"aiv_read_far_victim", [](const CalculateParams &params) { return std::to_string(params.pmuMap.At(1065)); }},
    {"aiv_read_hit_rate(%)", [](const CalculateParams &params) {
        uint64_t val1 = Utility::SafeAdd(params.pmuMap.At(1060), params.pmuMap.At(1063), "read hit");
        uint64_t val2 = Utility::SafeAddAll<uint64_t>({params.pmuMap.At(1060), params.pmuMap.At(1061), params.pmuMap.At(1062),
            params.pmuMap.At(1063), params.pmuMap.At(1064), params.pmuMap.At(1065)}, "read total");
        return std::to_string(RatioFp(val1, val2) * PERCENTAGE_CONVERSION);}},
    {"aiv_write_close_hit", [](const CalculateParams &params) { return std::to_string(params.pmuMap.At(1066)); }},
    {"aiv_write_close_miss", [](const CalculateParams &params) { return std::to_string(params.pmuMap.At(1067)); }},
    {"aiv_write_close_victim", [](const CalculateParams &params) { return std::to_string(params.pmuMap.At(1068)); }},
    {"aiv_write_far_hit", [](const CalculateParams &params) { return std::to_string(params.pmuMap.At(1069)); }},
    {"aiv_write_far_miss", [](const CalculateParams &params) { return std::to_string(params.pmuMap.At(1070)); }},
    {"aiv_write_far_victim", [](const CalculateParams &params) { return std::to_string(params.pmuMap.At(1071)); }},
    {"aiv_write_hit_rate(%)", [](const CalculateParams &params) {
        uint64_t val1 = Utility::SafeAdd(params.pmuMap.At(1066), params.pmuMap.At(1069), "write hit");
        uint64_t val2 = Utility::SafeAddAll<uint64_t>({params.pmuMap.At(1066), params.pmuMap.At(1067), params.pmuMap.At(1068),
            params.pmuMap.At(1069), params.pmuMap.At(1070), params.pmuMap.At(1071)}, "write total");
        return std::to_string(RatioFp(val1, val2) * PERCENTAGE_CONVERSION);}},

    {"aic_l1_read_bw(GB/s)", [](const CalculateParams &params) {
        return BandWidth(GetDataNumberFp(params.pmuMap.At(1799), REQ_DATA_OF_A5.at(TransportType::L1_TO_MTE)), params.duration);}},
    {"aic_l1_write_bw(GB/s)", [](const CalculateParams &params) {
        return BandWidth(GetDataNumberFp(params.pmuMap.At(1801), REQ_DATA_OF_A5.at(TransportType::GM_TO_L1)), params.duration);}},
    {"aic_main_mem_read_bw(GB/s)", [](const CalculateParams &params) {
        return BandWidth(GetDataNumberFp(params.pmuMap.At(1058), REQ_DATA_OF_A5.at(TransportType::READ_MAIN_MEMORY)), params.duration);}},
    {"aic_main_mem_write_bw(GB/s)", [](const CalculateParams &params) {
        return BandWidth(GetDataNumberFp(params.pmuMap.At(1059), REQ_DATA_OF_A5.at(TransportType::WRITE_MAIN_MEMORY)), params.duration);}},
    {"aic_mte1_instructions",  [](const CalculateParams &params) {
        return std::to_string(params.pmuMap.At(1792));}},
    {"aic_mte1_ratio", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(1794), params.totalCycles);}},
    {"aic_mte2_instructions", [](const CalculateParams &params) {
        return std::to_string(params.pmuMap.At(512));}},
    {"aic_mte2_ratio", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(514), params.totalCycles);}},
    {"aic_mte3_instructions", [](const CalculateParams &params) {
        return std::to_string(params.pmuMap.At(513));}},
    {"aic_mte3_ratio", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(515), params.totalCycles);}},
    {"aiv_gm_to_ub_bw(GB/s)", [](const CalculateParams &params) {
        uint64_t other = Utility::SafeAdd(params.pmuMap.At(1407), params.pmuMap.At(1408), "gm to ub");
        auto val = Utility::SafeSub(params.pmuMap.At(1058), other, "gm to ub", false);
        return BandWidth(GetDataNumberFp(val, REQ_DATA_OF_A5.at(TransportType::GM_TO_UB)), params.duration);}},
    {"aiv_main_mem_read_bw(GB/s)", [](const CalculateParams &params) {
        return BandWidth(GetDataNumberFp(params.pmuMap.At(1058), REQ_DATA_OF_A5.at(TransportType::READ_MAIN_MEMORY)), params.duration);}},
    {"aiv_main_mem_write_bw(GB/s)", [](const CalculateParams &params) {
        return BandWidth(GetDataNumberFp(params.pmuMap.At(1059), REQ_DATA_OF_A5.at(TransportType::WRITE_MAIN_MEMORY)), params.duration);}},
    {"aiv_mte2_instructions", [](const CalculateParams &params) {
        return std::to_string(params.pmuMap.At(512));}},
    {"aiv_mte2_ratio", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(514), params.totalCycles);}},
    {"aiv_mte3_instructions", [](const CalculateParams &params) {
        return std::to_string(params.pmuMap.At(513));}},
    {"aiv_mte3_ratio", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(515), params.totalCycles);}},
    {"read_main_memory_datas(KB)", [](const CalculateParams &params) {
        return std::to_string(GetDataNumberFp(params.pmuMap.At(1058), REQ_DATA_OF_A5.at(TransportType::READ_MAIN_MEMORY), "KB"));}},
    {"write_main_memory_datas(KB)", [](const CalculateParams &params) {
        return std::to_string(GetDataNumberFp(params.pmuMap.At(1059), REQ_DATA_OF_A5.at(TransportType::WRITE_MAIN_MEMORY), "KB"));}},
    {"GM_to_L1_datas(KB)", [](const CalculateParams &params) {
        return std::to_string(GetDataNumberFp(params.pmuMap.At(1058), REQ_DATA_OF_A5.at(TransportType::GM_TO_L1), "KB"));}},
    {"L0C_to_L1_datas(KB)", [](const CalculateParams &params) {
        return std::to_string(GetDataNumberFp(params.pmuMap.At(1806), REQ_DATA_OF_A5.at(TransportType::L0C_TO_L1), "KB"));}},
    {"L0C_to_GM_datas(KB)", [](const CalculateParams &params) {
        return std::to_string(GetDataNumberFp(params.pmuMap.At(1059), REQ_DATA_OF_A5.at(TransportType::L0C_TO_GM), "KB"));}},
    {"GM_to_UB_datas(KB)", [](const CalculateParams &params) {
        uint64_t other = Utility::SafeAdd(params.pmuMap.At(1407), params.pmuMap.At(1408), "gm to ub");
        auto val = Utility::SafeSub(params.pmuMap.At(1058), other, "gm to ub", false);
        return std::to_string(GetDataNumberFp(val, REQ_DATA_OF_A5.at(TransportType::GM_TO_UB), "KB"));}},
    {"GM_to_L1_bw_usage_rate(%)", [](const CalculateParams &params) {
        return BandWidthUsage(GetDataNumberFp(params.pmuMap.At(1058), REQ_DATA_OF_A5.at(TransportType::GM_TO_L1)),
            params.duration, TransportType::GM_TO_L1, params.socVersion);}},
    {"L0C_to_L1_bw_usage_rate(%)", [](const CalculateParams &params) {
        return BandWidthUsage(GetDataNumberFp(params.pmuMap.At(1806), REQ_DATA_OF_A5.at(TransportType::L0C_TO_L1)),
            params.duration, TransportType::L0C_TO_L1, params.socVersion);}},
    {"L0C_to_GM_bw_usage_rate(%)", [](const CalculateParams &params) {
        return BandWidthUsage(GetDataNumberFp(params.pmuMap.At(1059), REQ_DATA_OF_A5.at(TransportType::L0C_TO_GM)),
            params.duration, TransportType::L0C_TO_GM, params.socVersion);}},
    {"GM_to_UB_bw_usage_rate(%)", [](const CalculateParams &params) {
        uint64_t other = Utility::SafeAdd(params.pmuMap.At(1407), params.pmuMap.At(1408), "gm to ub");
        auto val = Utility::SafeSub(params.pmuMap.At(1058), other, "gm to ub", false);
        return BandWidthUsage(GetDataNumberFp(val, REQ_DATA_OF_A5.at(TransportType::GM_TO_UB)),
            params.duration, TransportType::GM_TO_UB, params.socVersion);}},

    {"aic_l0a_read_bw(GB/s)", [](const CalculateParams &params) {
        return BandWidth(GetDataNumberFp(params.pmuMap.At(772), REQ_DATA_OF_A5.at(TransportType::L0A_TO_CUBE)), params.duration);}},
    {"aic_l0a_write_bw(GB/s)", [](const CalculateParams &params) {
        return BandWidth(GetDataNumberFp(params.pmuMap.At(1795), REQ_DATA_OF_A5.at(TransportType::L1_TO_L0A)), params.duration);}},
    {"aic_l0b_read_bw(GB/s)", [](const CalculateParams &params) {
        return BandWidth(GetDataNumberFp(params.pmuMap.At(774), REQ_DATA_OF_A5.at(TransportType::L0B_TO_CUBE)), params.duration);}},
    {"aic_l0b_write_bw(GB/s)", [](const CalculateParams &params) {
        return BandWidth(GetDataNumberFp(params.pmuMap.At(1797), REQ_DATA_OF_A5.at(TransportType::L1_TO_L0B)), params.duration);}},
    {"aic_l0c_read_bw_cube(GB/s)", [](const CalculateParams &params) {
        return BandWidth(GetDataNumberFp(params.pmuMap.At(778), REQ_DATA_OF_A5.at(TransportType::L0C_TO_CUBE)), params.duration);}},
    {"aic_l0c_write_bw_cube(GB/s)", [](const CalculateParams &params) {
        return BandWidth(GetDataNumberFp(params.pmuMap.At(776), REQ_DATA_OF_A5.at(TransportType::CUBE_TO_L0C)), params.duration);}},

    {"aiv_ub_read_bw_vector(GB/s)", [](const CalculateParams &params) {
        return BandWidth(GetDataNumberFp(params.pmuMap.At(1393), REQ_DATA_OF_A5.at(TransportType::UB_TO_VEC)), params.duration);}},
    {"aiv_ub_write_bw_vector(GB/s)", [](const CalculateParams &params) {
        return BandWidth(GetDataNumberFp(params.pmuMap.At(1394), REQ_DATA_OF_A5.at(TransportType::VEC_TO_UB)), params.duration);}},
    {"aiv_ub_write_bw_gm(GB/s)", [](const CalculateParams &params) {
        uint64_t other = Utility::SafeAdd(params.pmuMap.At(1407), params.pmuMap.At(1408), "gm to ub");
        auto val = Utility::SafeSub(params.pmuMap.At(1058), other, "gm to ub", false);
        return BandWidth(GetDataNumberFp(val, REQ_DATA_OF_A5.at(TransportType::GM_TO_UB)), params.duration);}},

    {"aic_cube_time(us)", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(810), params.frequency);}},
     {"aic_cube_ratio", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(810), params.totalCycles);}},
    {"aic_scalar_time(us)", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(1), params.frequency);}},
    {"aic_scalar_ratio", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(1), params.totalCycles);}},
    {"aic_mte1_time(us)", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(1794), params.frequency);}},
    {"aic_mte1_ratio", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(1794), params.totalCycles);}},
    {"aic_mte1_active_bw(GB/s)", [](const CalculateParams &params) {
        float mte1Data = GetDataNumberFp(params.pmuMap.At(1799), REQ_DATA_OF_A5.at(TransportType::L1_TO_MTE));
        return BandWidth(mte1Data, RatioFp(params.pmuMap.At(1794), params.frequency));}},
    {"aic_mte2_time(us)", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(514), params.frequency);}},
    {"aic_mte2_ratio", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(514), params.totalCycles);}},
    {"aic_mte2_active_bw(GB/s)", [](const CalculateParams &params) {
        return BandWidth(GetDataNumberFp(params.pmuMap.At(1058), REQ_DATA_OF_A5.at(TransportType::GM_TO_L1)), RatioFp(params.pmuMap.At(514), params.frequency));}},
    {"aic_mte3_time(us)", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(515), params.frequency);}},
    {"aic_mte3_ratio", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(515), params.totalCycles);}},
    {"aic_mte3_active_bw(GB/s)", [](const CalculateParams &params) {
        uint64_t pmuUbToL1 = Utility::SafeSub(params.pmuMap.At(1801), Utility::SafeAdd(params.pmuMap.At(1806), params.pmuMap.At(1058) / 2, "pmuUbToL1"), "pmuUbToL1", false);
        return BandWidth(GetDataNumberFp(pmuUbToL1, REQ_DATA_OF_A5.at(TransportType::MTE_TO_L1)), RatioFp(params.pmuMap.At(515), params.frequency));}},
    {"aic_fixpipe_time(us)", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(1812), params.frequency);}},
    {"aic_fixpipe_ratio", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(1812), params.totalCycles);}},
    {"aic_fixpipe_active_bw(GB/s)", [](const CalculateParams &params) {
        auto pmuFixp = Utility::SafeAddAll<uint64_t>({params.pmuMap.At(1804), params.pmuMap.At(1806), params.pmuMap.At(1815), params.pmuMap.At(1059)}, "pmuFixp");
        return BandWidth(GetDataNumberFp(pmuFixp, REQ_DATA_OF_A5.at(TransportType::L0C_TO_FIXP)), RatioFp(params.pmuMap.At(1812), params.frequency));}},
    {"aic_icache_miss_rate", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(53), params.pmuMap.At(52));}},
    {"aiv_vec_time(us)", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(1281), params.frequency);}},
    {"aiv_vec_ratio", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(1281), params.totalCycles);}},
    {"aiv_scalar_time(us)", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(1), params.frequency);}},
    {"aiv_scalar_ratio", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(1), params.totalCycles);}},
    {"aiv_mte2_time(us)", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(514), params.frequency);}},
    {"aiv_mte2_ratio", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(514), params.totalCycles);}},
    {"aiv_mte2_active_bw(GB/s)", [](const CalculateParams &params) {
        uint64_t pmuGmToUb = Utility::SafeSub(params.pmuMap.At(1058), Utility::SafeAdd(params.pmuMap.At(1407), params.pmuMap.At(1408), "pmuGmToUb"), "pmuGmToUb", false);
        return BandWidth(GetDataNumberFp(pmuGmToUb, REQ_DATA_OF_A5.at(TransportType::GM_TO_UB)), RatioFp(params.pmuMap.At(514), params.frequency));}},
    {"aiv_mte3_time(us)", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(515), params.frequency);}},
    {"aiv_mte3_ratio", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(515), params.totalCycles);}},
    {"aiv_icache_miss_rate", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(53), params.pmuMap.At(52));}},

    {"aic_cube_wait_ratio", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(11), params.totalCycles);}},
    {"aic_mte1_wait_ratio", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(13), params.totalCycles);}},
    {"aic_mte2_wait_ratio", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(14), params.totalCycles);}},
    {"aic_mte3_wait_ratio", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(15), params.totalCycles);}},
    {"aiv_vec_stu_cflt_ratio", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(1366), params.totalCycles);}},
    {"aiv_vec_ldu_cflt_ratio", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(1344), params.totalCycles);}},
    {"aiv_vec_sfu_cflt_ratio", [](const CalculateParams &params) {
        uint64_t val = Utility::SafeAddAll<uint64_t>({params.pmuMap.At(1376), params.pmuMap.At(1377), params.pmuMap.At(1378), params.pmuMap.At(1379)}, "sfu cflt");
        return Ratio(val, params.totalCycles);}},
    {"aiv_vec_wait_ratio", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(12), params.totalCycles);}},
    {"aiv_mte2_wait_ratio", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(14), params.totalCycles);}},
    {"aiv_mte3_wait_ratio", [](const CalculateParams &params) {
        return Ratio(params.pmuMap.At(15), params.totalCycles);}},
};

// metric names do not start with aic/aiv
const std::vector<std::string> ExtraAicMetricItems = {
    "read_main_memory_datas(KB)", "write_main_memory_datas(KB)", "GM_to_L1_datas(KB)", "GM_to_L1_bw_usage_rate(%)",
    "L1_to_GM_datas(KB)(estimate)", "L1_to_GM_bw_usage_rate(%)(estimate)", "L0C_to_L1_datas(KB)",
    "L0C_to_L1_bw_usage_rate(%)", "L0C_to_GM_datas(KB)", "L0C_to_GM_bw_usage_rate(%)"
};

const std::vector<std::string> ExtraAivMetricItems = {
    "read_main_memory_datas(KB)", "write_main_memory_datas(KB)", "GM_to_UB_datas(KB)", "GM_to_UB_bw_usage_rate(%)",
    "UB_to_GM_datas(KB)", "UB_to_GM_bw_usage_rate(%)"
};
}
#endif // __MSOPPROF_PROFILING_PMU_FORMULA_H__
