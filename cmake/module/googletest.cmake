function(patch_gtest_warnings)
  # gcc 11 及以上对 googletest 某些代码会触发 -Wmaybe-uninitialized，
  # 而 googletest 自身在其 CMake 中开启了 -Werror，导致告警当成错误。
  # 这里不改动 googletest 源码，而是在本工程中对 gtest/gmock 目标
  # 单独关闭“将 maybe-uninitialized 视为错误”的设置。
  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
      CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 11)
    foreach(tgt gtest gtest_main gmock gmock_main)
      if (TARGET ${tgt})
        target_compile_options(${tgt} PRIVATE -Wno-error=maybe-uninitialized)
      endif()
    endforeach()
  endif()
endfunction()


