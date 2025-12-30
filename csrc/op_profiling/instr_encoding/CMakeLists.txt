add_library(instr_encoding STATIC instr_encoding.cpp)
file(GLOB_RECURSE SOURCE "*.cpp")
list(SORT SOURCE)
target_sources(instr_encoding PRIVATE ${SOURCE})

target_include_directories(instr_encoding PUBLIC ${MSOPT_SRC_DIR}/utils/cpputils ${JSON_INC_DIR} ${SECUREC_INC_DIR})

target_link_libraries(instr_encoding PRIVATE c_sec cpputils)