find_program(BISHENG_COMPILER bisheng)
if(NOT BISHENG_COMPILER)
    message(FATAL_ERROR "Compiler bisheng is not installed or not in PATH")
endif()

# setup toolchain for ccec_stub compiling
set(CMAKE_CXX_COMPILER bisheng)
set(CMAKE_LINKER llvm-ar)
set(CMAKE_CXX_LINK_FLAGS "")
set(CMAKE_CXX_FLAGS "")

# set aicore arch list for msopprof dynamic stub
set(CCE_AICORE_ARCH_LIST
    "dav-c220-vec"
    "dav-c220-cube"
    "dav-c220"
    "dav-c310"
    "dav-c310-vec"
    "dav-c310-cube"
)

message(STATUS "msopprof dynamic binary instruction probes built for: ${CCE_AICORE_ARCH_LIST}")

file(GLOB_RECURSE DYNAMIC_PROBES_FILES *.cpp)

# 桩源码实际包含的头文件。多数目录下所有头文件都会被包含，按目录收集即可；
# 公共目录和公共仓只列实际用到的单个文件，避免把无关头文件变成依赖。
file(GLOB MSOPPROF_STUB_HEADERS CONFIGURE_DEPENDS
    ${MSOPT_SRC_DIR}/op_profiling/plugin/*.h
    ${MSOPT_SRC_DIR}/op_profiling/plugin/memory_chart/*.h
    ${MSOPT_SRC_DIR}/op_profiling/plugin/operand_record/*.h)
list(APPEND MSOPPROF_STUB_HEADERS
    ${MSOPT_SRC_DIR}/op_profiling/common/dbi_defs.h
    ${MSOPSCOMMON_DIR}/kernel_injection/include/MSBit.h
    ${MSOPSCOMMON_DIR}/include/opprof/DbiDefs.h)

# @description compile msopprof stub files to virtual object.....................
function(compile_cce_object CCE_AICORE_ARCH MSOPPROF_STUB_OBJECT MSOPPROF_STUB_FILES)
    set(MSOPPROF_COMPILE_FLAGS
        "-xcce -std=c++17 -O2 --cce-aicore-only --cce-aicore-arch=${CCE_AICORE_ARCH} \
        -mllvm -cce-aicore-function-stack-size=16000 -mllvm -cce-aicore-record-overflow=false \
        -mllvm -cce-aicore-addr-transform -mllvm -cce-aicore-long-call -mllvm -cce-aicore-relax \
        -fPIC -pthread -DBUILD_DYNAMIC_PROBE"
    )

    add_library(${MSOPPROF_STUB_OBJECT} OBJECT ${MSOPPROF_STUB_FILES})
    target_include_directories(${MSOPPROF_STUB_OBJECT} PUBLIC ${MSOPT_SRC_DIR}/op_profiling ${MSOPSCOMMON_DIR} ${MSOPT_THIRDPARTY_INCLUDE})
    # remove secure flags from parent project
    set_target_properties(${MSOPPROF_STUB_OBJECT} PROPERTIES COMPILE_OPTIONS "")
    set_target_properties(${MSOPPROF_STUB_OBJECT} PROPERTIES COMPILE_FLAGS ${MSOPPROF_COMPILE_FLAGS})
endfunction()

# @description link multiple cce stub object to relocatable object
function(link_cce_object_to_relocatable MERGED_OBJECT MSOPPROF_STUB_OBJECT)
    # 桩源码/桩头文件/共享布局定义变化后都要重新合并，否则插件会一直链接旧的 merged.o。
    # 不能只写 DEPENDS target 名：Makefile 生成器不会因对象文件更新而重跑该命令；
    # 也不能把 $<TARGET_OBJECTS:...> 当文件依赖：全新构建树里这些对象没有独立生成规则，
    # 且 OBJECT 库不允许挂 POST_BUILD 命令。因此显式列出输入文件。
    add_custom_command(
        OUTPUT ${MERGED_OBJECT}
        VERBATIM
        COMMENT "Build merged stub object ${MERGED_OBJECT} from msopprof stub source files"
        DEPENDS ${MSOPPROF_STUB_OBJECT} ${DYNAMIC_PROBES_FILES} ${MSOPPROF_STUB_HEADERS}
        COMMAND_EXPAND_LISTS
        COMMAND ld.lld -r $<TARGET_OBJECTS:${MSOPPROF_STUB_OBJECT}> -o ${MERGED_OBJECT}
    )
endfunction()

# @description insert object to specific section of object
function(insert_section_to_object
    LAST_OBJECT NEXT_OBJECT
    SECTION_NAME SECTION_OBJECT)
    add_custom_command(
        OUTPUT ${NEXT_OBJECT}
        VERBATIM
        COMMENT "Insert section ${SECTION_NAME} to ${LAST_OBJECT}"
        # 直接从输入对象生成新对象，避免原地追加：重复执行时不会在已含该段的对象上再追加一次，
        # 否则会出现重复的架构段。
        DEPENDS ${LAST_OBJECT} ${SECTION_OBJECT}
        COMMAND llvm-objcopy --add-section ${SECTION_NAME}=${SECTION_OBJECT}
        --set-section-flags ${SECTION_NAME}=alloc,contents,readonly ${LAST_OBJECT} ${NEXT_OBJECT}
    )
endfunction()

# [WORKAROUND] cmake cannot establish correct dependency between custom command
# and objects. Thus we build dynamic_bind as static library, then extract object
# from it.
add_custom_command(
    OUTPUT dynamic_bind.cpp.o
    VERBATIM
    COMMENT "Extract object dynamic_bind.cpp.o from libdynamic_bind.a"
    DEPENDS dynamic_bind_memory
    COMMAND ar -x $<TARGET_FILE:dynamic_bind_memory>
)

list(GET CCE_AICORE_ARCH_LIST 0 LAST_AICORE_ARCH)
foreach(CCE_AICORE_ARCH ${CCE_AICORE_ARCH_LIST})
    set(MSOPPROF_STUB_OBJECT msopprof_memory_probe_object_${CCE_AICORE_ARCH})
    set(MERGED_OBJECT ${MSOPPROF_STUB_OBJECT}_merged.o)

    set(LAST_OBJECT dynamic_bind_memory_${LAST_AICORE_ARCH}.o)
    set(NEXT_OBJECT dynamic_bind_memory_${CCE_AICORE_ARCH}.o)
    if(${CCE_AICORE_ARCH} STREQUAL ${LAST_AICORE_ARCH})
        set(LAST_OBJECT dynamic_bind.cpp.o)
    endif()
    compile_cce_object(${CCE_AICORE_ARCH} ${MSOPPROF_STUB_OBJECT} "${DYNAMIC_PROBES_FILES}")
    link_cce_object_to_relocatable(${MERGED_OBJECT} ${MSOPPROF_STUB_OBJECT})
    insert_section_to_object(
      ${LAST_OBJECT} ${NEXT_OBJECT}
      .${CCE_AICORE_ARCH} ${MERGED_OBJECT}
    )

    # update last cce aicore arch
    set(LAST_AICORE_ARCH ${CCE_AICORE_ARCH})
endforeach()

add_custom_command(
    OUTPUT libprofplugin_memorychart.so
    VERBATIM
    COMMENT "Link libprofplugin_memorychart.so"
    DEPENDS msbit dynamic_bind_memory_${LAST_AICORE_ARCH}.o
    COMMAND g++ -shared -fPIC -Wl,-z,relro,-z,now -s -o libprofplugin_memorychart.so
    dynamic_bind_memory_${LAST_AICORE_ARCH}.o -static-libstdc++ $<TARGET_FILE:msbit>
)
# property ALL is necessary for making libprofplugin_memorychart.so built
add_custom_target(profplugin_memorychart ALL DEPENDS libprofplugin_memorychart.so)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libprofplugin_memorychart.so DESTINATION lib64)
