get_filename_component(OPRUNNER_ROOT_DIR ${MSOPT_SRC_DIR}/op_runner ABSOLUTE)
file(GLOB_RECURSE OPRUNNER_SOURCE ${OPRUNNER_ROOT_DIR}/runner_impl/*.cpp
                                  ${OPRUNNER_ROOT_DIR}/op_runner.cpp)

file(GLOB_RECURSE TEST_SOURCE "*.cpp")
list(SORT TEST_SOURCE)
target_include_directories(msopt_test PUBLIC ${OPRUNNER_ROOT_DIR})
target_sources(msopt_test PRIVATE ${TEST_SOURCE}
                                  ${OPRUNNER_SOURCE})
