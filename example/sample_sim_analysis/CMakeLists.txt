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

set(CMAKE_ASC_RUN_MODE "sim" CACHE STRING "Run mode: npu, sim")
set_property(CACHE CMAKE_ASC_RUN_MODE PROPERTY STRINGS npu sim)
set(CMAKE_ASC_ARCHITECTURES "dav-3510" CACHE STRING "NPU architecture: dav-2201, dav-3510")
set_property(CACHE CMAKE_ASC_ARCHITECTURES PROPERTY STRINGS dav-2201 dav-3510)
set(GROUPED_MATMUL_PROFILE "after" CACHE STRING "Optimization profile: before, after")
set_property(CACHE GROUPED_MATMUL_PROFILE PROPERTY STRINGS before after)

find_package(ASC REQUIRED)

project(grouped_matmul LANGUAGES ASC CXX)

file(COPY scripts DESTINATION ${CMAKE_BINARY_DIR})

if(CMAKE_ASC_ARCHITECTURES STREQUAL "dav-2201")
    set(NPU_ARCH_ID 2201)
elseif(CMAKE_ASC_ARCHITECTURES STREQUAL "dav-3510")
    set(NPU_ARCH_ID 3510)
else()
    message(FATAL_ERROR "Unsupported CMAKE_ASC_ARCHITECTURES=${CMAKE_ASC_ARCHITECTURES}, only dav-2201 and dav-3510 are supported")
endif()

if(GROUPED_MATMUL_PROFILE STREQUAL "before")
    set(GROUPED_MATMUL_AIV_NUM 1)
    set(GROUPED_MATMUL_PIPELINE_DEPTH 1)
    set(GROUPED_MATMUL_BUFFER_NUM 1)
elseif(GROUPED_MATMUL_PROFILE STREQUAL "after")
    set(GROUPED_MATMUL_AIV_NUM 2)
    set(GROUPED_MATMUL_PIPELINE_DEPTH 4)
    set(GROUPED_MATMUL_BUFFER_NUM 2)
else()
    message(FATAL_ERROR "Unsupported GROUPED_MATMUL_PROFILE=${GROUPED_MATMUL_PROFILE}, only before and after are supported")
endif()

set(TARGET_NAME grouped_matmul_${GROUPED_MATMUL_PROFILE})

add_executable(${TARGET_NAME}
    grouped_matmul_custom.asc
)

target_link_libraries(${TARGET_NAME} PRIVATE
    tiling_api
    register
    platform
    m
    dl
)

message(STATUS "[INFO] Build ${GROUPED_MATMUL_PROFILE}: run mode=${CMAKE_ASC_RUN_MODE}, AIC:AIV=1:${GROUPED_MATMUL_AIV_NUM}, pipeline depth=${GROUPED_MATMUL_PIPELINE_DEPTH}, vector buffer=${GROUPED_MATMUL_BUFFER_NUM}")

target_compile_definitions(${TARGET_NAME} PRIVATE
    NPU_ARCH=${NPU_ARCH_ID}
    GROUPED_MATMUL_M=1024
    GROUPED_MATMUL_N=8192
    GROUPED_MATMUL_K=1024
    GROUPED_MATMUL_GROUP_NUM=8
    GROUPED_MATMUL_CORE_NUM=8
    GROUPED_MATMUL_AIV_NUM=${GROUPED_MATMUL_AIV_NUM}
    GROUPED_MATMUL_PIPELINE_DEPTH=${GROUPED_MATMUL_PIPELINE_DEPTH}
    GROUPED_MATMUL_BUFFER_NUM=${GROUPED_MATMUL_BUFFER_NUM}
)

target_compile_options(${TARGET_NAME} PRIVATE
    $<$<COMPILE_LANGUAGE:ASC>:--npu-arch=${CMAKE_ASC_ARCHITECTURES}>
)
