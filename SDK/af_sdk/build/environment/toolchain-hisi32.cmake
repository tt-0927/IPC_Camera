set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(TOOLCHAIN_ROOT "/opt/hisi-linux/x86-arm/arm-v01c02-linux-musleabi-gcc")

set(CMAKE_C_COMPILER "${TOOLCHAIN_ROOT}/bin/arm-v01c02-linux-musleabi-gcc")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_ROOT}/bin/arm-v01c02-linux-musleabi-g++")

set(CMAKE_C_COMPILER_FORCED ON)
set(CMAKE_CXX_COMPILER_FORCED ON)

# 确保ssl相关的静态库存在
set(LIB_SSL "${CMAKE_CURRENT_LIST_DIR}/third-party/hi3516/openssl/lib/libssl.a")
if(NOT LIB_SSL)
    message(FATAL_ERROR "Static library ssl not found")
endif()
set(LIB_CRYPTO "${CMAKE_CURRENT_LIST_DIR}/third-party/hi3516/openssl/lib/libcrypto.a")
if(NOT LIB_CRYPTO)
    message(FATAL_ERROR "Static library crypto not found")
endif()

set(INCLUDE_SSL "${CMAKE_CURRENT_LIST_DIR}/third-party/hi3516/openssl/include")
message("INCLUDE_SSL===========>" ${INCLUDE_SSL})