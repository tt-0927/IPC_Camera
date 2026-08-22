cmake_minimum_required(VERSION 3.10)

project(HttpFaceClientDemo)

# ===================== 编译标准 =====================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ===================== 功能开关 =====================
# 默认按纯 HTTP 编译，便于跨局域网推送测试；如需 HTTPS，可在 cmake 时增加：
#   -DHTTP_FACE_ENABLE_HTTPS=ON
option(HTTP_FACE_ENABLE_HTTPS "Enable HTTPS support by linking OpenSSL" OFF)

# ===================== 路径配置 =====================
set(SDK_SHARE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../../../sdk_share)

include_directories(
    ${SDK_SHARE_PATH}/tools
    ${SDK_SHARE_PATH}/tools/http
)

add_executable(${PROJECT_NAME} main.cpp)

if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        _WIN32_WINNT=0x0A00
        WINVER=0x0A00
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
endif()

set(LIBS "")
if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(APPEND LIBS
        ws2_32
    )
else()
    list(APPEND LIBS
        pthread
        dl
    )
endif()

if(HTTP_FACE_ENABLE_HTTPS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)

    # 兼容两种目录结构：
    # 1. build/environment/third-party
    # 2. sdk/build/environment/third-party
    set(OPENSSL_ROOT_CANDIDATES
        "${CMAKE_CURRENT_LIST_DIR}/../../../../build/environment/third-party/openssl"
        "${CMAKE_CURRENT_LIST_DIR}/../../../../sdk/build/environment/third-party/openssl"
    )

    set(OPENSSL_ROOT "")
    foreach(_ROOT ${OPENSSL_ROOT_CANDIDATES})
        if(EXISTS "${_ROOT}")
            set(OPENSSL_ROOT "${_ROOT}")
            break()
        endif()
    endforeach()

    if(NOT OPENSSL_ROOT)
        message(FATAL_ERROR "OpenSSL root not found. Tried: ${OPENSSL_ROOT_CANDIDATES}")
    endif()

    if(DEFINED INCLUDE_SSL AND EXISTS "${INCLUDE_SSL}")
        set(OPENSSL_INCLUDE_DIR "${INCLUDE_SSL}")
    else()
        set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT}/include")
    endif()

    if(DEFINED LIB_SSL AND EXISTS "${LIB_SSL}")
        set(OPENSSL_SSL_LIB "${LIB_SSL}")
    elseif(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(OPENSSL_SSL_LIB "${OPENSSL_ROOT}/win64/libssl.a")
    elseif(CMAKE_C_COMPILER MATCHES "aarch64|arm64")
        set(OPENSSL_SSL_LIB "${OPENSSL_ROOT}/lib64/libssl.a")
    elseif(CMAKE_C_COMPILER MATCHES "x86_64|/usr/bin/gcc|/usr/bin/cc")
        set(OPENSSL_SSL_LIB "${OPENSSL_ROOT}/lib_linux64/libssl.a")
    else()
        set(OPENSSL_SSL_LIB "${OPENSSL_ROOT}/lib/libssl.a")
    endif()

    if(DEFINED LIB_CRYPTO AND EXISTS "${LIB_CRYPTO}")
        set(OPENSSL_CRYPTO_LIB "${LIB_CRYPTO}")
    elseif(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(OPENSSL_CRYPTO_LIB "${OPENSSL_ROOT}/win64/libcrypto.a")
    elseif(CMAKE_C_COMPILER MATCHES "aarch64|arm64")
        set(OPENSSL_CRYPTO_LIB "${OPENSSL_ROOT}/lib64/libcrypto.a")
    elseif(CMAKE_C_COMPILER MATCHES "x86_64|/usr/bin/gcc|/usr/bin/cc")
        set(OPENSSL_CRYPTO_LIB "${OPENSSL_ROOT}/lib_linux64/libcrypto.a")
    else()
        set(OPENSSL_CRYPTO_LIB "${OPENSSL_ROOT}/lib/libcrypto.a")
    endif()

    if(NOT EXISTS "${OPENSSL_INCLUDE_DIR}")
        message(FATAL_ERROR "OpenSSL include not found: ${OPENSSL_INCLUDE_DIR}")
    endif()

    if(NOT EXISTS "${OPENSSL_SSL_LIB}")
        message(FATAL_ERROR "libssl not found: ${OPENSSL_SSL_LIB}")
    endif()

    if(NOT EXISTS "${OPENSSL_CRYPTO_LIB}")
        message(FATAL_ERROR "libcrypto not found: ${OPENSSL_CRYPTO_LIB}")
    endif()

    target_include_directories(${PROJECT_NAME} PRIVATE ${OPENSSL_INCLUDE_DIR})
    list(APPEND LIBS ${OPENSSL_SSL_LIB} ${OPENSSL_CRYPTO_LIB})

    if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
        list(APPEND LIBS crypt32 iphlpapi bcrypt advapi32)
    endif()
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBS})

message(STATUS "项目名称: ${PROJECT_NAME}")
message(STATUS "SDK_SHARE_PATH: ${SDK_SHARE_PATH}")
message(STATUS "HTTP_FACE_ENABLE_HTTPS: ${HTTP_FACE_ENABLE_HTTPS}")
message(STATUS "最终链接库: ${LIBS}")
