get_filename_component(OPPROF_ROOT_DIR ${MSOPT_SRC_DIR}/op_profiling ABSOLUTE)
file(GLOB_RECURSE OPPROF_SOURCE ${OPPROF_ROOT_DIR}/argparser/*.cpp
        ${OPPROF_ROOT_DIR}/common/*.cpp
        ${OPPROF_ROOT_DIR}/interface/*.cpp
        ${OPPROF_ROOT_DIR}/profiling/*.cpp
        ${OPPROF_ROOT_DIR}/utility/*.cpp
        ${OPPROF_ROOT_DIR}/ascend_hal/*.cpp
        ${OPPROF_ROOT_DIR}/prof_injection/*.cpp
        ${OPPROF_ROOT_DIR}/communication/*.cpp
        ${OPPROF_ROOT_DIR}/parse/data_calculator/*.cpp
        ${OPPROF_ROOT_DIR}/parse/data_table/*.cpp
        ${OPPROF_ROOT_DIR}/parse/data_center/*.cpp
        ${OPPROF_ROOT_DIR}/parse/plugin/*.cpp
        ${OPPROF_ROOT_DIR}/parse/data_parser/*.cpp
        ${OPPROF_ROOT_DIR}/parse/data_parser/cache_parser/*.cpp
        ${OPPROF_ROOT_DIR}/parse/data_parser/instr_parser/*.cpp
        ${OPPROF_ROOT_DIR}/parse/data_visualizer/*.cpp
        ${OPPROF_ROOT_DIR}/parse/data_parser/mte_log_parser/*.cpp
        ${OPPROF_ROOT_DIR}/parse/data_visualizer/mte_log_visualizer/*.cpp
)

file(GLOB_RECURSE TEST_SOURCE "*.cpp")
list(SORT TEST_SOURCE)
target_sources(msopt_test PRIVATE ${TEST_SOURCE} ${OPPROF_SOURCE})

target_include_directories(msopt_test PUBLIC ${MSOPT_SRC_DIR}/utils/cpputils
                                        ${MSOPT_SRC_DIR}/op_profiling/ascend_hal
                                        ${SECUREC_INC_DIR}
                                        ${MSOPT_SRC_DIR}/op_profiling/common
                                        ${OPPROF_ROOT_DIR}
                                        ${OPPROF_ROOT_DIR}/prof_injection)
