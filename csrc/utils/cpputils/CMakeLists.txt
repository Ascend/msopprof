add_library(cpputils STATIC log.cpp)
file(GLOB_RECURSE SOURCE "*.cpp")
list(SORT SOURCE)
target_sources(cpputils PRIVATE ${SOURCE})

target_include_directories(cpputils PUBLIC ${JSON_INC_DIR} ${SECUREC_INC_DIR} ${MSOPT_SRC_DIR}/op_profiling ${MSOPSCOMMON_DIR})
target_link_libraries(cpputils PRIVATE op_runner c_sec stdc++fs)