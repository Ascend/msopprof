get_filename_component(BOOST_INC_DIR ${ROOT_DIR}/msopscommon/thirdparty/boost ABSOLUTE)
get_filename_component(MOCKCPP_INC_DIR ${THIRD_PARTY_DIR}/mockcpp/include ABSOLUTE)
set(TEST_BIN_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(TEST_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

if (BUILD_TESTS)
    set(CMAKE_CXX_FLAGS "-fprofile-arcs -ftest-coverage -lgcov")
    add_subdirectory(ut)
endif ()
