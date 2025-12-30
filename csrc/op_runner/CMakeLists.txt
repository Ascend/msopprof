project(op_runner)
get_filename_component(OPRUNNER_ROOT_DIR ./ ABSOLUTE)

add_library(op_runner STATIC op_runner.cpp)
target_include_directories(op_runner PUBLIC ${OPRUNNER_ROOT_DIR}
                                    ${JSON_INC_DIR}
                                    ${MSOPT_SRC_DIR}/utils/cpputils
                                    ${MSOPT_SRC_DIR}/op_profiling
                                    ${MSOPT_THIRDPARTY_INCLUDE})
target_link_libraries(op_runner PRIVATE dl pthread cpputils)
add_subdirectory(runner_impl)