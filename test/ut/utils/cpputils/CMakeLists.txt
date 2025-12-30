get_filename_component(UTIL_ROOT_DIR ${MSOPT_SRC_DIR}/utils ABSOLUTE)
file(GLOB_RECURSE UTIL_SOURCE ${UTIL_ROOT_DIR}/cpputils/*.cpp)

file(GLOB_RECURSE TEST_SOURCE "*.cpp")
list(SORT TEST_SOURCE)
target_sources(msopt_test PRIVATE ${TEST_SOURCE} ${UTIL_SOURCE})

target_include_directories(msopt_test PUBLIC ${UTIL_ROOT_DIR} ${MSOPT_SRC_DIR}/utils/cpputils)

