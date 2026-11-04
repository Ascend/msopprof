# ----------------------------------------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in this case directory for the full text of the License.
# ----------------------------------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.16)

set(CMAKE_ASC_RUN_MODE "sim" CACHE STRING "Run mode: npu, sim")
set(CMAKE_ASC_ARCHITECTURES "dav-3510" CACHE STRING "NPU architecture: dav-3510")
set(FUSED_ITERATIONS "14" CACHE STRING "Fused recurrence iterations: positive integer")

find_package(ASC REQUIRED)

project(kernel_samples LANGUAGES ASC CXX)

add_executable(demo
    add_double_buffer.asc
)

if(NOT CMAKE_ASC_ARCHITECTURES STREQUAL "dav-3510")
    message(FATAL_ERROR "This sample only supports CMAKE_ASC_ARCHITECTURES=dav-3510")
endif()

if(NOT CMAKE_ASC_RUN_MODE MATCHES "^(npu|sim)$")
    message(FATAL_ERROR "CMAKE_ASC_RUN_MODE must be npu or sim, but got ${CMAKE_ASC_RUN_MODE}")
endif()

if(NOT FUSED_ITERATIONS MATCHES "^[1-9][0-9]*$")
    message(FATAL_ERROR "FUSED_ITERATIONS must be a positive integer, but got ${FUSED_ITERATIONS}")
endif()

message(STATUS "[INFO] FUSED_ITERATIONS is ${FUSED_ITERATIONS}")
target_compile_definitions(demo PRIVATE
    FUSED_ITERATIONS=${FUSED_ITERATIONS}
)

target_compile_options(demo PRIVATE
    $<$<COMPILE_LANGUAGE:ASC>:--npu-arch=${CMAKE_ASC_ARCHITECTURES}>
    -g
)
