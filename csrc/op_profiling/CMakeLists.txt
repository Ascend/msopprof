get_filename_component(MSOPPROF_ROOT_DIR ./ ABSOLUTE)
get_filename_component(INTERFACE_DIR ./interface ABSOLUTE)
get_filename_component(COMMON_DIR ./common ABSOLUTE)
get_filename_component(PROFILING_DIR ./profiling ABSOLUTE)
get_filename_component(ARGPARSER_DIR ./argparser ABSOLUTE)
get_filename_component(PROFSTUB_DIR ./prof_injection ABSOLUTE)
get_filename_component(COMMUNCATION_DIR ./communication ABSOLUTE)
get_filename_component(PLUGIN_DIR ./plugin ABSOLUTE)
get_filename_component(INSTR_ENCODING_DIR ./instr_encoding ABSOLUTE)
get_filename_component(PARSE_DIR ./parse ABSOLUTE)

add_executable(msopprof main.cpp)

add_subdirectory(${INTERFACE_DIR})
add_subdirectory(${COMMON_DIR})
add_subdirectory(${PROFILING_DIR})
add_subdirectory(${ARGPARSER_DIR})
add_subdirectory(${PROFSTUB_DIR})
add_subdirectory(${COMMUNCATION_DIR})
add_subdirectory(${PLUGIN_DIR})
add_subdirectory(${INSTR_ENCODING_DIR})
add_subdirectory(${PARSE_DIR})

target_include_directories(msopprof PUBLIC ${MSOPPROF_ROOT_DIR}
                                    ${JSON_INC_DIR}
                                    ${MSOPT_SRC_DIR}/utils/cpputils
                                    ${MSOPT_SRC_DIR}/op_runner
                                    ${SECUREC_INC_DIR}
                                    ${MSOPSCOMMON_DIR}
                                    ${INSTR_ENCODING_DIR}
                                    ${MSOPT_THIRDPARTY_INCLUDE})

target_link_libraries(msopprof c_sec)
target_link_libraries(msopprof cpputils op_runner instr_encoding stdc++fs $<TARGET_OBJECTS:core_obj>)