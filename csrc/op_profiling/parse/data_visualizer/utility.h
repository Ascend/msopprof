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


#ifndef MSOPT_UTILITY_H
#define MSOPT_UTILITY_H
#include <string>
#include <map>
#include "profiling/simulator/data_parse/sim_defs.h"
namespace Profiling {
namespace Parse {

const std::map<std::string, std::string> instrToColorMap = {
    // thread_state_uninterruptible（浮点极值/除法类）
    {"SIMT_FMNMX",              "thread_state_uninterruptible"},
    {"SIMT_FMNMX_I",            "thread_state_uninterruptible"},
    {"SIMT_FDIV",               "thread_state_uninterruptible"},
    {"SIMT_FDIV_I",             "thread_state_uninterruptible"},
 
    // heap_dump_object_type（浮点转换类）
    {"SIMT_F2I",                "heap_dump_object_type"},
    {"SIMT_I2F",                "heap_dump_object_type"},
    {"SIMT_F2F",                "heap_dump_object_type"},
    {"SIMT_FEXPDIF",            "heap_dump_object_type"},
    {"SIMT_FMULSCVT",           "heap_dump_object_type"},
 
    // cq_build_attempt_passed（搬运类-读操作）
    {"SIMT_LD",                 "cq_build_attempt_passed"},
    {"SIMT_LDG",                "cq_build_attempt_passed"},
    {"SIMT_LDS",                "cq_build_attempt_passed"},
    {"SIMT_LDK",                "cq_build_attempt_passed"},
    {"SIMT_ST",                 "cq_build_attempt_passed"},
 
    // thread_state_runnable（浮点基础运算类）
    {"SIMT_FFMA",               "thread_state_runnable"},
    {"SIMT_FFMA_I",             "thread_state_runnable"},
    {"SIMT_FMUL",               "thread_state_runnable"},
    {"SIMT_FMUL_I",             "thread_state_runnable"},
    {"SIMT_MUFU",               "thread_state_runnable"},
 
    // good（整数基础运算类）
    {"SIMT_IMAD",               "good"},
    {"SIMT_IMAD_I",             "good"},
    {"SIMT_LEA",                "good"},
    {"SIMT_LEA_HI_X",           "good"},
    {"SIMT_SHF",                "good"},
    {"SIMT_SHFI",               "good"},
 
    // thread_state_iowait（整数位运算类）
    {"SIMT_PLOP3",              "thread_state_iowait"},
    {"SIMT_LOP3",               "thread_state_iowait"},
    {"SIMT_LOP3_I",             "thread_state_iowait"},
    {"SIMT_ISETP",              "thread_state_iowait"},
    {"SIMT_ISETP_I",            "thread_state_iowait"},
 
    // thread_state_running（整数乘法/加法类）
    {"SIMT_IMUL",               "thread_state_running"},
    {"SIMT_IMUL_I",             "thread_state_running"},
    {"SIMT_IADD",               "thread_state_running"},
    {"SIMT_IADD_I",             "thread_state_running"},
 
    // startup（搬运类-写操作）
    {"SIMT_STG",                "startup"},
    {"SIMT_STS",                "startup"},
    {"SIMT_STK",                "startup"},
    {"SIMT_IMNMX",              "startup"},
    {"SIMT_IMNMX_I",            "startup"},
 
    // background_memory_dump（同步事件类+浮点比较）
    {"SIMT_BAR.THREAD BLOCK",   "background_memory_dump"},
    {"SIMT_JOIN",               "background_memory_dump"},
    {"SIMT_MEMBAR",             "background_memory_dump"},
    {"SIMT_FSETP",              "background_memory_dump"},
    {"SIMT_FSETP_I",            "background_memory_dump"},
 
    // rail_response（原子操作+整数极值类）
    {"SIMT_ATOM",               "rail_response"},
    {"SIMT_RED",                "rail_response"},
    
    // heap_dump_child_node_arrow
    {"SIMT_FADD",               "heap_dump_child_node_arrow"},
    {"SIMT_FADD_I",             "heap_dump_child_node_arrow"},
    {"SIMT_S2R",                "heap_dump_child_node_arrow"},
    {"SIMT_MOVI",               "heap_dump_child_node_arrow"},
    {"SIMT_END",                "heap_dump_child_node_arrow"}
};

const std::map<std::string, uint32_t>& getLaneOrderMap();

int CalCycles(const std::vector<MergeInfo> &instrList);

float GetMicrosecond(const ChipProductType &chipType, const uint64_t &cycles, int roundParam = 3);

float CalculateRunningTime(std::vector<MergeInfo> &instrList, const ChipProductType &chipType);

std::string GetCNameByInstrName(const std::string &instrName);

std::string GetCNameByPipe(const std::string &pipe);
}
}
#endif // MSOPT_UTILITY_H
