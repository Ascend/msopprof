file(GLOB_RECURSE SOURCE "*.cpp")
list(SORT SOURCE)
target_include_directories(op_runner PUBLIC ${SECUREC_INC_DIR})
target_sources(op_runner PRIVATE ${SOURCE})