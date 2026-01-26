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

#ifndef __MSOPPROF_PROFILING_METRIC_CSV_HEADER_H__
#define __MSOPPROF_PROFILING_METRIC_CSV_HEADER_H__

#include <map>
#include <string>
#include <vector>

#include "common/defs.h"

namespace Profiling {
// Ascend910B
const std::vector<std::string> arithmeticFor910B_ = {
    "block_id",
    "sub_block_id",
    "aic_time(us)",
    "aic_total_cycles",
    "aic_cube_ratio",
    "aic_cube_fp16_ratio",
    "aic_cube_int8_ratio",
    "aic_cube_fops",
    "aic_cube_total_instr_number",
    "aic_cube_fp_instr_number",
    "aic_cube_int_instr_number",
    "aiv_time(us)",
    "aiv_total_cycles",
    "aiv_vec_ratio",
    "aiv_vec_fp32_ratio",
    "aiv_vec_fp16_ratio",
    "aiv_vec_int32_ratio",
    "aiv_vec_int16_ratio",
    "aiv_vec_misc_ratio",
    "aiv_vec_fops",
};

const std::vector<std::string> l2CacheFor910B_ = {
    "block_id",
    "sub_block_id",
    "aic_time(us)",
    "aic_total_cycles",
    "aic_write_cache_hit",
    "aic_write_cache_miss_allocate",
    "aic_r0_read_cache_hit",
    "aic_r0_read_cache_miss_allocate",
    "aic_r1_read_cache_hit",
    "aic_r1_read_cache_miss_allocate",
    "aic_write_hit_rate(%)",
    "aic_read_hit_rate(%)",
    "aic_total_hit_rate(%)",
    "aiv_time(us)",
    "aiv_total_cycles",
    "aiv_write_cache_hit",
    "aiv_write_cache_miss_allocate",
    "aiv_r0_read_cache_hit",
    "aiv_r0_read_cache_miss_allocate",
    "aiv_r1_read_cache_hit",
    "aiv_r1_read_cache_miss_allocate",
    "aiv_write_hit_rate(%)",
    "aiv_read_hit_rate(%)",
    "aiv_total_hit_rate(%)",
};

const std::vector<std::string> memoryFor910B_ = {
    "block_id",
    "sub_block_id",
    "aic_time(us)",
    "aic_total_cycles",
    "aic_l1_read_bw(GB/s)",
    "aic_l1_write_bw(GB/s)",
    "aic_main_mem_read_bw(GB/s)",
    "aic_main_mem_write_bw(GB/s)",
    "aic_mte1_instructions",
    "aic_mte1_ratio",
    "aic_mte2_instructions",
    "aic_mte2_ratio",
    "aic_mte3_instructions",
    "aic_mte3_ratio",
    "aiv_time(us)",
    "aiv_total_cycles",
    "aiv_ub_to_gm_bw(GB/s)",
    "aiv_gm_to_ub_bw(GB/s)",
    "aiv_main_mem_read_bw(GB/s)",
    "aiv_main_mem_write_bw(GB/s)",
    "aiv_mte2_instructions",
    "aiv_mte2_ratio",
    "aiv_mte3_instructions",
    "aiv_mte3_ratio",
    "read_main_memory_datas(KB)",
    "write_main_memory_datas(KB)",
    "GM_to_L1_datas(KB)",
    "GM_to_L1_bw_usage_rate(%)",
    "L1_to_GM_datas(KB)(estimate)",
    "L1_to_GM_bw_usage_rate(%)(estimate)",
    "L0C_to_L1_datas(KB)",
    "L0C_to_L1_bw_usage_rate(%)",
    "L0C_to_GM_datas(KB)",
    "L0C_to_GM_bw_usage_rate(%)",
    "GM_to_UB_datas(KB)",
    "GM_to_UB_bw_usage_rate(%)",
    "UB_to_GM_datas(KB)",
    "UB_to_GM_bw_usage_rate(%)",
};

const std::vector<std::string> memoryL0For910B_ = {
    "block_id",
    "sub_block_id",
    "aic_time(us)",
    "aic_total_cycles",
    "aic_l0a_read_bw(GB/s)",
    "aic_l0a_write_bw(GB/s)",
    "aic_l0b_read_bw(GB/s)",
    "aic_l0b_write_bw(GB/s)",
    "aic_l0c_read_bw_cube(GB/s)",
    "aic_l0c_write_bw_cube(GB/s)",
    "aiv_time(us)",
    "aiv_total_cycles",
};

const std::vector<std::string> memoryUbFor910B_ = {
    "block_id",
    "sub_block_id",
    "aic_time(us)",
    "aic_total_cycles",
    "aiv_time(us)",
    "aiv_total_cycles",
    "aiv_ub_read_bw_vector(GB/s)",
    "aiv_ub_write_bw_vector(GB/s)",
    "aiv_ub_read_bw_scalar(GB/s)",
    "aiv_ub_write_bw_scalar(GB/s)",
};

const std::vector<std::string> pipeFor910B_ = {
    "block_id",
    "sub_block_id",
    "aic_time(us)",
    "aic_total_cycles",
    "aic_cube_time(us)",
    "aic_cube_ratio",
    "aic_mte1_time(us)",
    "aic_mte1_ratio",
    "aic_mte2_time(us)",
    "aic_mte2_ratio",
    "aic_mte3_time(us)",
    "aic_mte3_ratio",
    "aic_mte3_active_bw(GB/s)",
    "aic_fixpipe_time(us)",
    "aic_fixpipe_ratio",
    "aic_fixpipe_active_bw(GB/s)",
    "aic_icache_miss_rate",
    "aic_scalar_time(us)",
    "aic_scalar_ratio",
    "aic_scalar_single_time(us)",
    "aic_scalar_dual_time(us)",
    "aic_scalar_mte1_stall_time(us)",
    "aic_scalar_mte2_stall_time(us)",
    "aic_scalar_mte3_stall_time(us)",
    "aic_scalar_cube_stall_time(us)",
    "aic_scalar_wait_ib_time(us)",
    "aic_scalar_wait_time(us)",
    "aiv_time(us)",
    "aiv_total_cycles",
    "aiv_vec_time(us)",
    "aiv_vec_ratio",
    "aiv_mte2_time(us)",
    "aiv_mte2_ratio",
    "aiv_mte2_active_bw(GB/s)",
    "aiv_mte3_time(us)",
    "aiv_mte3_ratio",
    "aiv_mte3_active_bw(GB/s)",
    "aiv_icache_miss_rate",
    "aiv_scalar_time(us)",
    "aiv_scalar_ratio",
    "aiv_scalar_single_time(us)",
    "aiv_scalar_dual_time(us)",
    "aiv_scalar_mte2_stall_time(us)",
    "aiv_scalar_mte3_stall_time(us)",
    "aiv_scalar_stall_by_ub_time(us)",
    "aiv_scalar_vector_stall_time(us)",
    "aiv_scalar_wait_ib_time(us)",
    "aiv_scalar_wait_time(us)",
};

const std::vector<std::string> pipeDbiFor910B_ = {
    "aic_mte1_active_bw(GB/s)",
    "aic_mte2_active_bw(GB/s)",
};

const std::vector<std::string> resourceConflictFor910B_ = {
    "block_id",
    "sub_block_id",
    "aic_time(us)",
    "aic_total_cycles",
    "aic_cube_wait_ratio",
    "aic_mte1_wait_ratio",
    "aic_mte2_wait_ratio",
    "aic_mte3_wait_ratio",
    "aiv_time(us)",
    "aiv_total_cycles",
    "aiv_vec_total_cflt_ratio",
    "aiv_vec_bankgroup_cflt_ratio",
    "aiv_vec_bank_cflt_ratio",
    "aiv_vec_resc_cflt_ratio",
    "aiv_vec_mte_cflt_ratio",
    "aiv_vec_wait_ratio",
    "aiv_mte2_wait_ratio",
    "aiv_mte3_wait_ratio",
};

const std::map<std::string, std::vector<std::string>> MetricHeaderFor910B = {
    {std::string(Common::MsprofMetrics::ARITHMETIC_UTILIZATION), arithmeticFor910B_},
    {std::string(Common::MsprofMetrics::L2_CACHE), l2CacheFor910B_},
    {std::string(Common::MsprofMetrics::MEMORY), memoryFor910B_},
    {std::string(Common::MsprofMetrics::MEMORY_L0), memoryL0For910B_},
    {std::string(Common::MsprofMetrics::MEMORY_UB), memoryUbFor910B_},
    {std::string(Common::MsprofMetrics::PIPE_UTILIZATION), pipeFor910B_},
    {std::string(Common::MsprofMetrics::RESOURCE_CONFLICT_RATIO), resourceConflictFor910B_},
};

const std::vector<std::string> OpBasicHeaderFor910B = {
    "Op Name", "Op Type", "Task Duration(us)", "Block Dim", "Mix Block Dim", "Device Id", "Pid", "Current Freq",
    "Rated Freq"
};

// Ascend310P
const std::vector<std::string> arithmeticFor310P_ = {
    "aic_time(us)",
    "aic_total_cycles",
    "aic_cube_ratio",
    "aic_cube_fp16_ratio",
    "aic_cube_int8_ratio",
    "aic_cube_fops",
    "aic_cube_total_instr_number",
    "aic_vec_ratio",
    "aic_vec_fp32_ratio",
    "aic_vec_fp16_ratio",
    "aic_vec_int32_ratio",
    "aic_vec_int16_ratio",
    "aic_vec_misc_ratio",
    "aic_vec_fops",
};

const std::vector<std::string> l2CacheFor310P_ = {
    "aic_l2_cache_hit_rate(%)",
};

const std::vector<std::string> memoryFor310P_ = {
    "aic_time(us)",
    "aic_total_cycles",
    "aic_l1_read_bw(GB/s)",
    "aic_l1_write_bw(GB/s)",
    "aic_ub_to_gm_bw(GB/s)",
    "aic_gm_to_ub_bw(GB/s)",
    "aic_main_mem_read_bw(GB/s)",
    "aic_main_mem_write_bw(GB/s)",
    "aic_mte1_instructions",
    "aic_mte1_ratio",
    "aic_mte2_instructions",
    "aic_mte2_ratio",
    "aic_mte3_instructions",
    "aic_mte3_ratio",
};

const std::vector<std::string> memoryL0For310P_ = {
    "aic_time(us)",
    "aic_total_cycles",
    "aic_l0a_read_bw(GB/s)",
    "aic_l0a_write_bw(GB/s)",
    "aic_l0b_read_bw(GB/s)",
    "aic_l0b_write_bw(GB/s)",
    "aic_l0c_read_bw_cube(GB/s)",
    "aic_l0c_write_bw_cube(GB/s)",
    "aic_l0c_read_bw(GB/s)",
    "aic_l0c_write_bw(GB/s)",
};

const std::vector<std::string> memoryUbFor310P_ = {
    "aic_time(us)",
    "aic_total_cycles",
    "aic_ub_read_bw_vector(GB/s)",
    "aic_ub_write_bw_vector(GB/s)",
    "aic_ub_read_bw_scalar(GB/s)",
    "aic_ub_write_bw_scalar(GB/s)",
};

const std::vector<std::string> pipeFor310P_ = {
    "aic_time(us)",
    "aic_total_cycles",
    "aic_cube_time(us)",
    "aic_cube_ratio",
    "aic_scalar_time(us)",
    "aic_scalar_ratio",
    "aic_mte1_time(us)",
    "aic_mte1_ratio",
    "aic_mte2_time(us)",
    "aic_mte2_ratio",
    "aic_mte3_time(us)",
    "aic_mte3_ratio",
    "aic_icache_miss_rate",
    "aic_vec_time(us)",
    "aic_vec_ratio",
};

const std::vector<std::string> resourceConflictFor310P_ = {
    "aic_time(us)",
    "aic_total_cycles",
    "aic_cube_wait_ratio",
    "aic_vec_wait_ratio",
    "aic_mte1_wait_ratio",
    "aic_mte2_wait_ratio",
    "aic_mte3_wait_ratio",
    "aic_vec_total_cflt_ratio",
    "aic_vec_bankgroup_cflt_ratio",
    "aic_vec_bank_cflt_ratio",
    "aic_vec_resc_cflt_ratio",
    "aic_vec_mte_cflt_ratio",
};

const std::map<std::string, std::vector<std::string>> MetricHeaderFor310P = {
    {std::string(Common::MsprofMetrics::ARITHMETIC_UTILIZATION), arithmeticFor310P_},
    {std::string(Common::MsprofMetrics::L2_CACHE), l2CacheFor310P_},
    {std::string(Common::MsprofMetrics::MEMORY), memoryFor310P_},
    {std::string(Common::MsprofMetrics::MEMORY_L0), memoryL0For310P_},
    {std::string(Common::MsprofMetrics::MEMORY_UB), memoryUbFor310P_},
    {std::string(Common::MsprofMetrics::PIPE_UTILIZATION), pipeFor310P_},
    {std::string(Common::MsprofMetrics::RESOURCE_CONFLICT_RATIO), resourceConflictFor310P_},
};

const std::vector<std::string> OpBasicHeaderFor310P = {
    "Op Name", "Op Type", "Task Duration(us)", "Block Dim", "Device Id", "Pid", "Current Freq", "Rated Freq"
};

// AscendA5
const std::vector<std::string> arithmeticForA5_ = {
    "block_id",
    "sub_block_id",
    "aic_time(us)",
    "aic_total_cycles",
    "aic_cube_ratio",
    "aic_cube_fp_ratio",
    "aic_cube_int_ratio",
    "aic_cube_total_instr_number",
    "aic_cube_fp_instr_number",
    "aic_cube_int_instr_number",
    "aiv_time(us)",
    "aiv_total_cycles",
    "aiv_vec_ratio",
    "aiv_vec_vf_ratio",
    "aiv_vec_sfu_ratio",
    "aiv_vec_simt_vf_ratio",
};

const std::vector<std::string> l2CacheForA5_ = {
    "block_id",
    "sub_block_id",
    "aic_time(us)",
    "aic_total_cycles",
    "aic_read_close_hit",
    "aic_read_close_miss",
    "aic_read_close_victim",
    "aic_read_far_hit",
    "aic_read_far_miss",
    "aic_read_far_victim",
    "aic_read_hit_rate(%)",
    "aic_write_close_hit",
    "aic_write_close_miss",
    "aic_write_close_victim",
    "aic_write_far_hit",
    "aic_write_far_miss",
    "aic_write_far_victim",
    "aic_write_hit_rate(%)",
    "aiv_time(us)",
    "aiv_total_cycles",
    "aiv_read_close_hit",
    "aiv_read_close_miss",
    "aiv_read_close_victim",
    "aiv_read_far_hit",
    "aiv_read_far_miss",
    "aiv_read_far_victim",
    "aiv_read_hit_rate(%)",
    "aiv_write_close_hit",
    "aiv_write_close_miss",
    "aiv_write_close_victim",
    "aiv_write_far_hit",
    "aiv_write_far_miss",
    "aiv_write_far_victim",
    "aiv_write_hit_rate(%)",
};

const std::vector<std::string> memoryForA5_ = {
    "block_id",
    "sub_block_id",
    "aic_time(us)",
    "aic_total_cycles",
    "aic_l1_read_bw(GB/s)",
    "aic_l1_write_bw(GB/s)",
    "aic_main_mem_read_bw(GB/s)",
    "aic_main_mem_write_bw(GB/s)",
    "aic_mte1_instructions",
    "aic_mte1_ratio",
    "aic_mte2_instructions",
    "aic_mte2_ratio",
    "aic_mte3_instructions",
    "aic_mte3_ratio",
    "aiv_time(us)",
    "aiv_total_cycles",
    "aiv_gm_to_ub_bw(GB/s)",
    "aiv_main_mem_read_bw(GB/s)",
    "aiv_main_mem_write_bw(GB/s)",
    "aiv_mte2_instructions",
    "aiv_mte2_ratio",
    "aiv_mte3_instructions",
    "aiv_mte3_ratio",
    "read_main_memory_datas(KB)",
    "write_main_memory_datas(KB)",
    "GM_to_L1_datas(KB)",
    "GM_to_L1_bw_usage_rate(%)",
    "L0C_to_L1_datas(KB)",
    "L0C_to_L1_bw_usage_rate(%)",
    "L0C_to_GM_datas(KB)",
    "L0C_to_GM_bw_usage_rate(%)",
    "GM_to_UB_datas(KB)",
    "GM_to_UB_bw_usage_rate(%)",
};

const std::vector<std::string> memoryL0ForA5_ = {
    "block_id",
    "sub_block_id",
    "aic_time(us)",
    "aic_total_cycles",
    "aic_l0a_read_bw(GB/s)",
    "aic_l0a_write_bw(GB/s)",
    "aic_l0b_read_bw(GB/s)",
    "aic_l0b_write_bw(GB/s)",
    "aic_l0c_read_bw_cube(GB/s)",
    "aic_l0c_write_bw_cube(GB/s)",
    "aiv_time(us)",
    "aiv_total_cycles",
};

const std::vector<std::string> memoryUbForA5_ = {
    "block_id",
    "sub_block_id",
    "aic_time(us)",
    "aic_total_cycles",
    "aiv_time(us)",
    "aiv_total_cycles",
    "aiv_ub_read_bw_vector(GB/s)",
    "aiv_ub_write_bw_vector(GB/s)",
    "aiv_ub_write_bw_gm(GB/s)",
};

const std::vector<std::string> pipeForA5_ = {
    "block_id",
    "sub_block_id",
    "aic_time(us)",
    "aic_total_cycles",
    "aic_cube_time(us)",
    "aic_cube_ratio",
    "aic_scalar_time(us)",
    "aic_scalar_ratio",
    "aic_mte1_time(us)",
    "aic_mte1_ratio",
    "aic_mte1_active_bw(GB/s)",
    "aic_mte2_time(us)",
    "aic_mte2_ratio",
    "aic_mte2_active_bw(GB/s)",
    "aic_mte3_time(us)",
    "aic_mte3_ratio",
    "aic_mte3_active_bw(GB/s)",
    "aic_fixpipe_time(us)",
    "aic_fixpipe_ratio",
    "aic_fixpipe_active_bw(GB/s)",
    "aic_icache_miss_rate",
    "aiv_time(us)",
    "aiv_total_cycles",
    "aiv_vec_time(us)",
    "aiv_vec_ratio",
    "aiv_scalar_time(us)",
    "aiv_scalar_ratio",
    "aiv_mte2_time(us)",
    "aiv_mte2_ratio",
    "aiv_mte2_active_bw(GB/s)",
    "aiv_mte3_time(us)",
    "aiv_mte3_ratio",
    "aiv_icache_miss_rate",
};

const std::vector<std::string> resourceConflictForA5_ = {
    "block_id",
    "sub_block_id",
    "aic_time(us)",
    "aic_total_cycles",
    "aic_cube_wait_ratio",
    "aic_mte1_wait_ratio",
    "aic_mte2_wait_ratio",
    "aic_mte3_wait_ratio",
    "aiv_time(us)",
    "aiv_total_cycles",
    "aiv_vec_stu_cflt_ratio",
    "aiv_vec_ldu_cflt_ratio",
    "aiv_vec_sfu_cflt_ratio",
    "aiv_vec_wait_ratio",
    "aiv_mte2_wait_ratio",
    "aiv_mte3_wait_ratio",
};

const std::map<std::string, std::vector<std::string>> MetricHeaderForA5 = {
    {std::string(Common::MsprofMetrics::ARITHMETIC_UTILIZATION), arithmeticForA5_},
    {std::string(Common::MsprofMetrics::L2_CACHE), l2CacheForA5_},
    {std::string(Common::MsprofMetrics::MEMORY), memoryForA5_},
    {std::string(Common::MsprofMetrics::MEMORY_L0), memoryL0ForA5_},
    {std::string(Common::MsprofMetrics::MEMORY_UB), memoryUbForA5_},
    {std::string(Common::MsprofMetrics::PIPE_UTILIZATION), pipeForA5_},
    {std::string(Common::MsprofMetrics::RESOURCE_CONFLICT_RATIO), resourceConflictForA5_},
};

const std::vector<std::string> OpBasicHeaderForA5 = {
    "Op Name", "Op Type", "Task Duration(us)", "Block Dim", "Mix Block Dim", "Device Id", "Pid", "Current Freq",
    "Rated Freq"
};
}
#endif // __MSOPPROF_PROFILING_METRIC_CSV_HEADER_H__
