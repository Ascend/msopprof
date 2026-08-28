# ----------------------------------------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.
# ----------------------------------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.16)

set(CMAKE_ASC_RUN_MODE "npu" CACHE STRING "Run mode: npu, sim")
set_property(CACHE CMAKE_ASC_RUN_MODE PROPERTY STRINGS npu sim)
set(CMAKE_ASC_ARCHITECTURES "dav-3510" CACHE STRING "NPU architecture: dav-3510")
set_property(CACHE CMAKE_ASC_ARCHITECTURES PROPERTY STRINGS dav-3510)
set(SCENARIO_NUM "2" CACHE STRING "Optimization case: 0 (naive) - 2 (double buffer)")
set_property(CACHE SCENARIO_NUM PROPERTY STRINGS 0 1 2)

find_package(ASC REQUIRED)

project(matrix_transpose LANGUAGES ASC CXX)

if(NOT SCENARIO_NUM MATCHES "^[0-2]$")
    message(FATAL_ERROR "SCENARIO_NUM must be one of 0, 1, 2.")
endif()

if(NOT CMAKE_ASC_ARCHITECTURES STREQUAL "dav-3510")
    message(FATAL_ERROR "Unsupported CMAKE_ASC_ARCHITECTURES=${CMAKE_ASC_ARCHITECTURES}, only dav-3510 (Ascend 950PR/950DT) is supported")
endif()

set(TARGET_NAME matrix_transpose_case${SCENARIO_NUM})

add_executable(${TARGET_NAME}
    matrix_transpose.asc
)

target_link_libraries(${TARGET_NAME} PRIVATE
    m
)

message(STATUS "[INFO] Build case${SCENARIO_NUM}: run mode=${CMAKE_ASC_RUN_MODE}, arch=${CMAKE_ASC_ARCHITECTURES}")

target_compile_definitions(${TARGET_NAME} PRIVATE
    SCENARIO_NUM=${SCENARIO_NUM}
)

# -g 调试信息：sim 模式下 msopprof simulator 采集指令流水图需要；
# npu 模式下带 -g 可支持 msopprof 源码级（PC sampling 等）分析
target_compile_options(${TARGET_NAME} PRIVATE
    $<$<COMPILE_LANGUAGE:ASC>:--npu-arch=${CMAKE_ASC_ARCHITECTURES} --cce-res-usage -g>
    $<$<COMPILE_LANGUAGE:CXX>:-g>
)
