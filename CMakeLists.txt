cmake_minimum_required(VERSION 3.20.2)

set(MAX_SUPPORTED_CMAKE_VERSION "3.31.10")
if(CMAKE_VERSION VERSION_GREATER MAX_SUPPORTED_CMAKE_VERSION)
    message(FATAL_ERROR 
        "CMake ${CMAKE_VERSION} is too new! The maximum supported version is ${MAX_SUPPORTED_CMAKE_VERSION}."
    )
else()
    message(STATUS "CMake version check passed: ${CMAKE_VERSION} ≤ ${MAX_SUPPORTED_CMAKE_VERSION}")
endif()

project(msopt)
set(CMAKE_SKIP_RPATH TRUE)

# 启用编译命令导出，用于IDE代码导航（跳转到定义）
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 设置默认构建类型为 Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type (Debug/Release)" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

get_filename_component(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR} ABSOLUTE)
get_filename_component(MSOPT_SRC_DIR csrc ABSOLUTE)
get_filename_component(THIRD_PARTY_DIR thirdparty ABSOLUTE)
get_filename_component(BUILD_DIR ${CMAKE_BINARY_DIR} ABSOLUTE)
get_filename_component(SECUREC_INC_DIR ${THIRD_PARTY_DIR}/securec/include ABSOLUTE)
get_filename_component(JSON_INC_DIR ${THIRD_PARTY_DIR}/json/single_include/nlohmann ABSOLUTE)
get_filename_component(MSOPSCOMMON_DIR ${ROOT_DIR}/msopscommon/csrc ABSOLUTE)
get_filename_component(MSOPT_THIRDPARTY_INCLUDE ${MSOPT_SRC_DIR}/include/thirdparty ABSOLUTE)
get_filename_component(LLVM_SRC_DIR ${THIRD_PARTY_DIR}/llvm-project/llvm ABSOLUTE)

# 检查是否可用 ccache
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
else()
    message(STATUS "ccache not found, using native compiler.")
endif()

set(INSTALL_GTEST OFF)

# 为了适用于工程本身构建，也适用于其它工程调用此工程，CMAKE_INSTALL_PREFIX的判断方式需要判断是否未设定或者系统默认路径
if (${CMAKE_INSTALL_PREFIX} STREQUAL "/usr/local" OR ${CMAKE_INSTALL_PREFIX} STREQUAL "/usr" OR NOT DEFINED CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX ${ROOT_DIR}/output CACHE STRING "install prefix" FORCE)
    message(STATUS "No install prefix selected, default to ${CMAKE_INSTALL_PREFIX}")
endif() 

option(BUILD_TESTS "build unit tests" OFF)
include(cmake/options.cmake)
include(cmake/module/securec.cmake)
include(cmake/module/llvm.cmake)
include(cmake/module/preparePackage.cmake)

if (BUILD_TESTS)
    include_directories(thirdparty/boost/libs/type_traits/include)
 	include_directories(thirdparty/boost/libs/config/include)
 	include_directories(thirdparty/boost/libs/typeof/include)
endif ()

add_subdirectory(csrc)
if (BUILD_TESTS)
    add_subdirectory(thirdparty)
    add_subdirectory(test)
endif ()

option(BUILD_INJECTION "build function injection library" ON)
if(BUILD_INJECTION)
    set(BUILD_TESTS off)
    add_subdirectory(msopscommon EXCLUDE_FROM_ALL)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tmp.cpp
        COMMAND touch ${CMAKE_CURRENT_BINARY_DIR}/tmp.cpp
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    add_custom_target(injection_laucher ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/tmp.cpp)
    add_dependencies(injection_laucher msopprof_injection kernel_launcher_bin dfx_kernel_dav-c220-cube dfx_kernel_dav-c310-cube dfx_kernel_dav-m200 empty_kernel_dav-c220-vec)
    install(TARGETS msopprof_injection kernel_launcher_bin
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
        LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib64
    )

    get_property(KERNEL_OUTPUT_PATH GLOBAL PROPERTY DFX_KERNEL_PATH)

    install(FILES
        ${KERNEL_OUTPUT_PATH}/dfx_kernel_dav-c220-cube.o
        ${KERNEL_OUTPUT_PATH}/dfx_kernel_dav-c310-cube.o
        ${KERNEL_OUTPUT_PATH}/dfx_kernel_dav-m200.o
        ${KERNEL_OUTPUT_PATH}/empty_kernel_dav-c220-vec.o
        ${MSOPSCOMMON_DIR}/tools/dfx_kernel/empty_kernel_dav-c220-vec.json
        ${MSOPSCOMMON_DIR}/tools/dfx_kernel/dfx_kernel_dav-m200.json
        ${MSOPSCOMMON_DIR}/tools/dfx_kernel/dfx_kernel_dav-c220-cube.json
        ${MSOPSCOMMON_DIR}/tools/dfx_kernel/dfx_kernel_dav-c310-cube.json
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib64
    )
endif()