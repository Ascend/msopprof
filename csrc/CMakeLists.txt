# 安全规范编译命令
add_compile_options("-Wall")
add_compile_options("-fPIC")
add_compile_options("-fstack-protector-all")
add_compile_options("-fvisibility=hidden")
add_compile_options("-ftrapv")
add_compile_options("-fstack-check")

add_link_options("-Wl,-z,relro")
add_link_options("-Wl,-z,now")
add_link_options("-Wl,-z,noexecstack")
add_link_options("-pie")

# 根据CMAKE_BUILD_TYPE选择编译选项
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Debug build: -O0, strip disabled")
    add_compile_options("-O0")
else()
    message(STATUS "Release build: -O2, strip enabled")
    add_compile_options("-O2")
    add_compile_options("-D_FORTIFY_SOURCE=2")
    add_link_options("-s")  # strip symbols
endif()

# 提供给测试的插桩，统计测试覆盖率
if (DEFINED HITEST AND HITEST STREQUAL "ON")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE hitestwrapper)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK hitestwrapper)
endif()

add_subdirectory(op_profiling)
add_subdirectory(op_runner)
add_subdirectory(utils)

install(TARGETS msopprof
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
        LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib64)

install(DIRECTORY ${MSOPT_SRC_DIR}/framework DESTINATION ${CMAKE_INSTALL_PREFIX}
        FILES_MATCHING PATTERN "*.py")

install(DIRECTORY ${MSOPT_SRC_DIR}/utils/pyutils/ DESTINATION ${CMAKE_INSTALL_PREFIX}/framework/utils
        FILES_MATCHING PATTERN "*.py")

