# build llvm-symbolizer
include(ExternalProject)
set(LLVM_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/llvm)

set(LINKER_FLAGS "-s -pie -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack")
set(CXX_FLAGS "${LINKER_FLAGS} -Wall -fPIC -fPIE -fstack-protector-all -D_FORTIFY_SOURCE=2 -ftrapv -fstack-check")

ExternalProject_Add(llvm_build
                    SOURCE_DIR  ${LLVM_SRC_DIR}
                    CMAKE_ARGS
                                -DCMAKE_BUILD_TYPE=Release
                                -DLLVM_TARGETS_TO_BUILD=host
                                -DLLVM_BUILD_TOOLS=NO
                                -DLLVM_BUILD_LLVM_DYLIB=YES
                                -DCMAKE_INSTALL_PREFIX=llvm
                                -DCMAKE_CXX_FLAGS=${CXX_FLAGS}
                                -DCMAKE_SKIP_RPATH=TRUE
                                -DLLVM_ENABLE_ZSTD=OFF
                                -DCMAKE_CXX_COMPILER_LAUNCHER=${CMAKE_CXX_COMPILER_LAUNCHER}
                                -DCMAKE_C_COMPILER_LAUNCHER=${CMAKE_C_COMPILER_LAUNCHER}
                                -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
                    BUILD_COMMAND $(MAKE) llvm-symbolizer
                    BINARY_DIR ${LLVM_BINARY_DIR}
                    INSTALL_COMMAND ""
                    EXCLUDE_FROM_ALL FALSE
)
install(PROGRAMS ${LLVM_BINARY_DIR}/bin/llvm-symbolizer DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/)
