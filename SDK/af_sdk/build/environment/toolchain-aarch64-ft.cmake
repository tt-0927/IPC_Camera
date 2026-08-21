#64位系统编译环境（飞腾平台）
#工具链路径：/opt/gcc-linaro-7.5.0-2019.12-x86_64_aarch64-linux-gnu
#与RK3588的区别：glibc版本不同，需要独立的OpenSSL库（lib64_ft）
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(TOOLCHAIN_ROOT "/opt/gcc-linaro-7.5.0-2019.12-x86_64_aarch64-linux-gnu")
set(CMAKE_C_COMPILER "${TOOLCHAIN_ROOT}/bin/aarch64-linux-gnu-gcc")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_ROOT}/bin/aarch64-linux-gnu-g++")
set(CMAKE_C_COMPILER_FORCED ON)
set(CMAKE_CXX_COMPILER_FORCED ON)

# 确保ssl相关的静态库存在（飞腾专用，需用本工具链重新编译）
set(LIB_SSL "${CMAKE_CURRENT_LIST_DIR}/third-party/openssl/lib64_ft/libssl.a")
if(NOT LIB_SSL)
    message(FATAL_ERROR "Static library ssl not found")
endif()
set(LIB_CRYPTO "${CMAKE_CURRENT_LIST_DIR}/third-party/openssl/lib64_ft/libcrypto.a")
if(NOT LIB_CRYPTO)
    message(FATAL_ERROR "Static library crypto not found")
endif()

set(INCLUDE_SSL "${CMAKE_CURRENT_LIST_DIR}/third-party/openssl/include")
message("INCLUDE_SSL===========>" ${INCLUDE_SSL})
