#64位系统编译环境
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER "/usr/bin/gcc")
set(CMAKE_CXX_COMPILER "/usr/bin/g++")
set(CMAKE_C_COMPILER_FORCED ON)
set(CMAKE_CXX_COMPILER_FORCED ON)

# 确保ssl相关的静态库存在
set(LIB_SSL "${CMAKE_CURRENT_LIST_DIR}/third-party/openssl/lib_linux64/libssl.a")
if(NOT LIB_SSL)
    message(FATAL_ERROR "Static library ssl not found")
endif()
set(LIB_CRYPTO "${CMAKE_CURRENT_LIST_DIR}/third-party/openssl/lib_linux64/libcrypto.a")
if(NOT LIB_CRYPTO)
    message(FATAL_ERROR "Static library crypto not found")
endif()

set(INCLUDE_SSL "${CMAKE_CURRENT_LIST_DIR}/third-party/openssl/include")
message("INCLUDE_SSL===========>" ${INCLUDE_SSL})
