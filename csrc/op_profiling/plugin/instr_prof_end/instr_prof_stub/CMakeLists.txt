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
    "dav-c310"
    "dav-c310-vec"
    "dav-c310-cube"
)

message(STATUS "msopprof dynamic binary instrprof end probes built for: ${CCE_AICORE_ARCH_LIST}")

file(GLOB_RECURSE DYNAMIC_PROBES_FILES *.cpp)

# @description compile msopprof stub files to virtual object.....................
function(compile_cce_object CCE_AICORE_ARCH MSOPPROF_STUB_OBJECT MSOPPROF_STUB_FILES)
    set(MSOPPROF_COMPILE_FLAGS
        "-xcce -std=c++17 -O2 --cce-aicore-only --cce-aicore-arch=${CCE_AICORE_ARCH} \
        -mllvm -cce-aicore-function-stack-size=16000 -mllvm -cce-aicore-record-overflow=false \
        -mllvm -cce-aicore-addr-transform -mllvm -cce-aicore-long-call -mllvm -cce-aicore-relax \
        -fPIC -pthread -DBUILD_DYNAMIC_PROBE"
    )

    add_library(${MSOPPROF_STUB_OBJECT} OBJECT ${MSOPPROF_STUB_FILES})
    target_include_directories(${MSOPPROF_STUB_OBJECT} PUBLIC ${ROOT_DIR}/csrc/op_profiling)
    target_include_directories(${MSOPPROF_STUB_OBJECT} PUBLIC ${MSOPSCOMMON_DIR})
    target_include_directories(${MSOPPROF_STUB_OBJECT} PUBLIC ${MSOPT_THIRDPARTY_INCLUDE})
    # remove secure flags from parent project
    set_target_properties(${MSOPPROF_STUB_OBJECT} PROPERTIES COMPILE_OPTIONS "")
    set_target_properties(${MSOPPROF_STUB_OBJECT} PROPERTIES COMPILE_FLAGS ${MSOPPROF_COMPILE_FLAGS})
endfunction()

# @description link multiple cce stub object to relocatable object
function(link_cce_object_to_relocatable MERGED_OBJECT MSOPPROF_STUB_OBJECT)
    add_custom_command(
        OUTPUT ${MERGED_OBJECT}
        VERBATIM
        COMMENT "Build merged stub object ${MERGED_OBJECT} from msopprof stub source files"
        DEPENDS ${MSOPPROF_STUB_OBJECT}
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
        DEPENDS ${LAST_OBJECT} ${SECTION_OBJECT}
        COMMAND llvm-objcopy --add-section ${SECTION_NAME}=${SECTION_OBJECT}
        --set-section-flags ${SECTION_NAME}=alloc,contents,readonly ${LAST_OBJECT}
        COMMAND cp ${LAST_OBJECT} ${NEXT_OBJECT}
    )
endfunction()

# [WORKAROUND] cmake cannot establish correct dependency between custom command
# and objects. Thus we build instrprof_dynamic_bind as static library, then extract object
# from it.
add_custom_command(
    OUTPUT instrprof_dynamic_bind.cpp.o
    VERBATIM
    COMMENT "Extract object instrprof_dynamic_bind.cpp.o from libinstrprof_dynamic_bind.a"
    DEPENDS instrprofend_dynamic_bind
    COMMAND ar -x $<TARGET_FILE:instrprofend_dynamic_bind>
)

list(GET CCE_AICORE_ARCH_LIST 0 LAST_AICORE_ARCH)
foreach(CCE_AICORE_ARCH ${CCE_AICORE_ARCH_LIST})
    set(MSOPPROF_STUB_OBJECT msopprof_instrprofend_object_${CCE_AICORE_ARCH})
    set(MERGED_OBJECT ${MSOPPROF_STUB_OBJECT}_merged.o)

    set(LAST_OBJECT instrprofend_dynamic_bind_${LAST_AICORE_ARCH}.o)
    set(NEXT_OBJECT instrprofend_dynamic_bind_${CCE_AICORE_ARCH}.o)
    if(${CCE_AICORE_ARCH} STREQUAL ${LAST_AICORE_ARCH})
        set(LAST_OBJECT instrprof_dynamic_bind.cpp.o)
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
    OUTPUT libprofplugin_instrprofend.so
    VERBATIM
    COMMENT "Link libprofplugin_instrprofend.so"
    DEPENDS msbit instrprofend_dynamic_bind_${LAST_AICORE_ARCH}.o
    COMMAND g++ -shared -fPIC -Wl,-z,relro,-z,now -s -o libprofplugin_instrprofend.so
    instrprofend_dynamic_bind_${LAST_AICORE_ARCH}.o -static-libstdc++ $<TARGET_FILE:msbit>
)
# property ALL is necessary for making libprofplugin_instrprofend.so built
add_custom_target(profplugin_instrprofend ALL DEPENDS libprofplugin_instrprofend.so)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libprofplugin_instrprofend.so DESTINATION lib64)
