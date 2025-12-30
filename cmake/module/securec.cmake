set(SECUREC_PATH ${SECUREC_INC_DIR}/../)

set(securec_src
    "${SECUREC_PATH}/src/vsprintf_s.c"
    "${SECUREC_PATH}/src/wmemmove_s.c"
    "${SECUREC_PATH}/src/strncat_s.c"
    "${SECUREC_PATH}/src/vsnprintf_s.c"
    "${SECUREC_PATH}/src/fwscanf_s.c"
    "${SECUREC_PATH}/src/scanf_s.c"
    "${SECUREC_PATH}/src/strcat_s.c"
    "${SECUREC_PATH}/src/sscanf_s.c"
    "${SECUREC_PATH}/src/secureprintoutput_w.c"
    "${SECUREC_PATH}/src/wmemcpy_s.c"
    "${SECUREC_PATH}/src/wcsncat_s.c"
    "${SECUREC_PATH}/src/secureprintoutput_a.c"
    "${SECUREC_PATH}/src/secureinput_w.c"
    "${SECUREC_PATH}/src/memcpy_s.c"
    "${SECUREC_PATH}/src/fscanf_s.c"
    "${SECUREC_PATH}/src/vswscanf_s.c"
    "${SECUREC_PATH}/src/secureinput_a.c"
    "${SECUREC_PATH}/src/memmove_s.c"
    "${SECUREC_PATH}/src/swscanf_s.c"
    "${SECUREC_PATH}/src/snprintf_s.c"
    "${SECUREC_PATH}/src/vscanf_s.c"
    "${SECUREC_PATH}/src/vswprintf_s.c"
    "${SECUREC_PATH}/src/wcscpy_s.c"
    "${SECUREC_PATH}/src/vfwscanf_s.c"
    "${SECUREC_PATH}/src/memset_s.c"
    "${SECUREC_PATH}/src/wscanf_s.c"
    "${SECUREC_PATH}/src/vwscanf_s.c"
    "${SECUREC_PATH}/src/strtok_s.c"
    "${SECUREC_PATH}/src/wcsncpy_s.c"
    "${SECUREC_PATH}/src/vfscanf_s.c"
    "${SECUREC_PATH}/src/vsscanf_s.c"
    "${SECUREC_PATH}/src/wcstok_s.c"
    "${SECUREC_PATH}/src/securecutil.c"
    "${SECUREC_PATH}/src/gets_s.c"
    "${SECUREC_PATH}/src/swprintf_s.c"
    "${SECUREC_PATH}/src/strcpy_s.c"
    "${SECUREC_PATH}/src/wcscat_s.c"
    "${SECUREC_PATH}/src/strncpy_s.c"
)

add_library(c_sec SHARED ${securec_src})

target_include_directories(c_sec PRIVATE ${SECUREC_INC_DIR})

target_compile_options(c_sec PRIVATE
    -std=c++11
    -fPIC
    -fstack-protector-all
    -fno-common
    -fno-strict-aliasing
    -Wfloat-equal
    -Wextra
)

target_link_options(c_sec PRIVATE
    -Wl,-z,relro,-z,now,-z,noexecstack
    -s
)

