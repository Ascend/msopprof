set(generated_data_path ${CMAKE_CURRENT_BINARY_DIR}/resources)
set(OUTPUT_FILES
    ${generated_data_path}/op_profiling/device310P/dump/DeviceProf1.bin
    ${generated_data_path}/op_profiling/device310P/dump/DeviceProf2.bin
    ${generated_data_path}/op_profiling/device310P/dump/DeviceProf3.bin
    ${generated_data_path}/op_profiling/device310P/dump/DeviceProf4.bin
    ${generated_data_path}/op_profiling/device310P/dump/DeviceProf5.bin
    ${generated_data_path}/op_profiling/device310P/dump/DeviceProf6.bin
    ${generated_data_path}/op_profiling/device310P/dump/L2Cache.bin
    ${generated_data_path}/op_profiling/device310P/dump/duration.bin

    ${generated_data_path}/op_profiling/device910B/dump/DeviceProf1.bin
    ${generated_data_path}/op_profiling/device910B/dump/DeviceProf2.bin
    ${generated_data_path}/op_profiling/device910B/dump/DeviceProf3.bin
    ${generated_data_path}/op_profiling/device910B/dump/DeviceProf4.bin
    ${generated_data_path}/op_profiling/device910B/dump/DeviceProf5.bin
    ${generated_data_path}/op_profiling/device910B/dump/DeviceProf6.bin
    ${generated_data_path}/op_profiling/device910B/dump/aicpu.bin
    ${generated_data_path}/op_profiling/device910B/dump/duration.bin
    ${generated_data_path}/op_profiling/device910B/dump/kernel0Stub.o.bbbmap.0

    ${generated_data_path}/op_test/add_custom/add_custom.o
    ${generated_data_path}/op_test/add_custom/golden.bin
    ${generated_data_path}/op_test/add_custom/input_x.bin
    ${generated_data_path}/op_test/add_custom/input_y.bin
    ${generated_data_path}/op_test/add_custom/tiling.bin

    ${generated_data_path}/op_test/matmul_leakyrelu_custom/bias.bin
    ${generated_data_path}/op_test/matmul_leakyrelu_custom/golden.bin
    ${generated_data_path}/op_test/matmul_leakyrelu_custom/matmul_leakyrelu_custom.o
    ${generated_data_path}/op_test/matmul_leakyrelu_custom/tiling.bin
    ${generated_data_path}/op_test/matmul_leakyrelu_custom/x1.bin
    ${generated_data_path}/op_test/matmul_leakyrelu_custom/x2.bin
)

find_program(PYTHON NAMES python3 python)
if(NOT PYTHON)
    message(FATAL_ERROR "Could not find python3 or python in PATH!")
else()
    message(STATUS "Found Python: ${PYTHON}")
endif()

add_custom_command(
    OUTPUT ${OUTPUT_FILES}
    # 为了避免在执行测试过程中污染test源码目录，将test源码拷贝到构建目录下
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${TEST_SRC_DIR} ${TEST_BIN_OUT_DIR}
    COMMAND ${PYTHON} ${TEST_BIN_OUT_DIR}/init_scripts/generate_add_custom_data.py ${generated_data_path}
    COMMAND ${PYTHON} ${TEST_BIN_OUT_DIR}/init_scripts/generate_matmul_leakyrelu_custom_data.py ${generated_data_path}
    COMMAND ${PYTHON} ${TEST_BIN_OUT_DIR}/init_scripts/generate_op_profiling_device_data.py ${generated_data_path}
    COMMAND ${PYTHON} ${TEST_BIN_OUT_DIR}/init_scripts/generate_pc_sampling.py ${generated_data_path}
    DEPENDS ${TEST_SRC_DIR}/init_scripts/generate_add_custom_data.py
            ${TEST_SRC_DIR}/init_scripts/generate_matmul_leakyrelu_custom_data.py
            ${TEST_SRC_DIR}/init_scripts/generate_op_profiling_device_data.py
            ${TEST_SRC_DIR}/init_scripts/generate_pc_sampling.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
add_custom_target(generate_test_data ALL DEPENDS ${OUTPUT_FILES})

add_executable(msopt_test)
add_dependencies(msopt_test generate_test_data)
target_link_directories(msopt_test PUBLIC ${BUILD_DIR}/lib/)
target_link_libraries(msopt_test gtest gtest_main pthread mockcpp op_runner instr_encoding dl cpputils c_sec stdc++fs $<TARGET_OBJECTS:core_obj>)
target_include_directories(msopt_test PUBLIC ${JSON_INC_DIR}
                                    ${MOCKCPP_INC_DIR}
                                    ${BOOST_INC_DIR}
                                    ${SECUREC_INC_DIR}
                                    ${MSOPSCOMMON_DIR}
                                    ${INSTR_ENCODING_DIR})
add_subdirectory(op_profiling)
add_subdirectory(op_runner)
add_subdirectory(utils)