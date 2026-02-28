add_subdirectory(${ROOT_DIR}/msopscommon/thirdparty/googletest ${CMAKE_CURRENT_BINARY_DIR}/googletest EXCLUDE_FROM_ALL)
include_directories(${ROOT_DIR}/msopscommon/thirdparty/boost/)
add_subdirectory(mockcpp EXCLUDE_FROM_ALL)
add_definitions(-w)