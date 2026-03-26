cmake_minimum_required(VERSION 3.13) # to support add_link_options

project(msopprof_superbuild VERSION 26.0.0)
set(CMAKE_SKIP_RPATH TRUE)

# 检查是否可用 ccache
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
else()
    message(STATUS "ccache not found, using native compiler.")
endif()

get_filename_component(MSOPPROF_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../ ABSOLUTE)
# 为了适用于工程本身构建，也适用于其它工程调用此工程，CMAKE_INSTALL_PREFIX的判断方式需要判断是否未设定或者系统默认路径
if (${CMAKE_INSTALL_PREFIX} STREQUAL "/usr/local" OR ${CMAKE_INSTALL_PREFIX} STREQUAL "/usr" OR NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX ${MSOPPROF_SRC_DIR}/output CACHE STRING "install prefix" FORCE)
  message(STATUS "No install prefix selected, default to ${CMAKE_INSTALL_PREFIX}")
endif() 

option(BUILD_TESTS "build unit tests" OFF)
option(FUZZ_TESTS "build fuzz tests" OFF)

# 通过ExternalProject构建本工程，从而实现在install后再将打包过程纳入CMAKE管理
include(ExternalProject)


ExternalProject_Add(build_msopprof
                    SOURCE_DIR  ${MSOPPROF_SRC_DIR}
                    CMAKE_ARGS  -DBUILD_TESTS=${BUILD_TESTS} 
                                -DFUZZ_TESTS=${FUZZ_TESTS}
                                -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                                -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
                    BUILD_COMMAND $(MAKE)
                    INSTALL_COMMAND $(MAKE) install
                    EXCLUDE_FROM_ALL FALSE
                    BUILD_ALWAYS TRUE
)

# 检查是否可用 python
find_program(PYTHON NAMES python3 python)
if(NOT PYTHON)
    message(FATAL_ERROR "Could not find python3 or python in PATH!")
else()
    message(STATUS "Found Python: ${PYTHON}")
endif()

# 读取version.info，并从中获取包名及版本信息
file(READ "${MSOPPROF_SRC_DIR}/package/conf/version.info" FILE_CONTENT)
if(FILE_CONTENT MATCHES "\\[PACKAGE\\]([^\\[]+)")
    set(TMP_SECTION "${CMAKE_MATCH_1}")
    
    # 提取 name
    if(TMP_SECTION MATCHES "Name=([^\n]+)")
        string(STRIP "${CMAKE_MATCH_1}" NAME)
        message(STATUS "Name: ${NAME}")
    endif()
    
    # 提取 version
    if(TMP_SECTION MATCHES "Version=([^\\n]+)")
        string(STRIP "${CMAKE_MATCH_1}" PACKAGE_VERSION)
        message(STATUS "Version: ${PACKAGE_VERSION}")
    endif()
endif()

set(packageName "${NAME}_${PACKAGE_VERSION}_${CMAKE_SYSTEM_PROCESSOR}.run")
set(makeselfTool  ${MSOPPROF_SRC_DIR}/thirdparty/makeself/makeself.sh)
set(makeselfHeader ${MSOPPROF_SRC_DIR}/thirdparty/makeself/makeself-header.sh)
set(helpInfo ${MSOPPROF_SRC_DIR}/package/conf/help.info)
set(installShell share/info/${NAME}/script/install.sh)
set(comments "ASCEND MINDSTUDIO MSOPPROF RUN PACKAGE")
file(GLOB_RECURSE PACAKGE_SCRIPT_FILES ${MSOPPROF_SRC_DIR}/package/**/*)

file(GLOB_RECURSE MAKESELF_TAR_FILE_GLOB ${MSOPPROF_SRC_DIR}/thirdparty/makeself/makeself-*.tar.gz)
file(GLOB_RECURSE MAKESELF_PATCH_FILE_GLOB ${MSOPPROF_SRC_DIR}/thirdparty/makeself/makeself-*.patch)
list(GET MAKESELF_TAR_FILE_GLOB 0 MAKESELF_TAR_FILE)
list(GET MAKESELF_PATCH_FILE_GLOB 0 MAKESELF_PATCH_FILE)
message(STATUS "Use makeself tar file: ${MAKESELF_TAR_FILE}")
message(STATUS "Use makeself patch file: ${MAKESELF_PATCH_FILE}")

add_custom_command(
    OUTPUT ${makeselfTool} ${makeselfHeader}
    COMMENT "Extract makeself component from ${MAKESELF_TAR_FILE}"
    # 跳过最外层目录，直接将内容解压到当前目录
    COMMAND tar xf ${MAKESELF_TAR_FILE} --strip-components=1
    COMMAND git apply ${MAKESELF_PATCH_FILE}
    # git 应用 patch 后不一定会更新akeself.sh 文件的时间戳，此处需要手动更新一下，
    # 保证 makeself 解压流程完成后重新触发打包
    COMMAND touch ${makeselfTool} ${makeselfHeader}
    DEPENDS ${MAKESELF_TAR_FILE} ${MAKESELF_PATCH_FILE}
    WORKING_DIRECTORY ${MSOPPROF_SRC_DIR}/thirdparty/makeself/
)

# 解析打包配置文件filelist.xml，并完成打包
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/package.flag
    COMMAND cd ${MSOPPROF_SRC_DIR}/package/script/ && ${PYTHON} parser.py -x ${MSOPPROF_SRC_DIR}/package/conf/filelist.xml
            --delivery_path ${CMAKE_INSTALL_PREFIX} -o ${CMAKE_SYSTEM_PROCESSOR}
    COMMAND cd ${CMAKE_INSTALL_PREFIX} && ${makeselfTool} --header ${makeselfHeader} --help-header ${helpInfo} --pigz
            --complevel 4 --nocrc --nomd5 --sha256 --chown --tar-quietly ${NAME} ${packageName} ${comments} ${installShell}
    COMMAND chmod 750 ${CMAKE_INSTALL_PREFIX}/*.run
    COMMAND touch ${CMAKE_CURRENT_BINARY_DIR}/package.flag
    DEPENDS ${PACAKGE_SCRIPT_FILES} ${makeselfTool} ${makeselfHeader}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_target(package_msopprof ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/package.flag build_msopprof)
