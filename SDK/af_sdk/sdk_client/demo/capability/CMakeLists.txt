cmake_minimum_required(VERSION 3.10)

project(CapabilityClientDemo)

add_definitions(-DCPPHTTPLIB_OPENSSL_SUPPORT)

# ===================== 工具链配置 =====================
if(DEFINED CMAKE_TOOLCHAIN_FILE)
    message(STATUS "使用指定的工具链文件: ${CMAKE_TOOLCHAIN_FILE}")
elseif(DEFINED CMAKE_C_COMPILER)
    message(STATUS "使用指定的编译器: ${CMAKE_C_COMPILER}")
else()
    message(STATUS "未指定工具链，使用默认编译器")
endif()

# ===================== 路径配置 =====================
set(SDK_CLIENT_PATH ${CMAKE_CURRENT_LIST_DIR}/../../../sdk_client)
set(SDK_SHARE_PATH  ${CMAKE_CURRENT_LIST_DIR}/../../../sdk_share)

set(OPENSSL_ROOT_CANDIDATES
    "${CMAKE_CURRENT_LIST_DIR}/../../../build/environment/third-party/openssl"
    "${CMAKE_CURRENT_LIST_DIR}/../../../build/third-party/openssl"
    "${CMAKE_CURRENT_LIST_DIR}/../../../sdk/build/environment/third-party/openssl"
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

# ===================== 编译标准 =====================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

# ===================== OpenSSL 配置 =====================
if(DEFINED INCLUDE_SSL AND EXISTS "${INCLUDE_SSL}")
    set(OPENSSL_INCLUDE_DIR "${INCLUDE_SSL}")
else()
    set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT}/include")
endif()

if(NOT EXISTS "${OPENSSL_INCLUDE_DIR}")
    message(FATAL_ERROR "OpenSSL include not found: ${OPENSSL_INCLUDE_DIR}")
endif()

if(DEFINED LIB_SSL AND EXISTS "${LIB_SSL}")
    set(OPENSSL_SSL_LIB "${LIB_SSL}")
else()
    if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(OPENSSL_SSL_LIB "${OPENSSL_ROOT}/win64/libssl.a")
    elseif(CMAKE_C_COMPILER MATCHES "aarch64|arm64")
        set(OPENSSL_SSL_LIB "${OPENSSL_ROOT}/lib64/libssl.a")
    elseif(CMAKE_C_COMPILER MATCHES "x86_64|/usr/bin/gcc|/usr/bin/cc")
        set(OPENSSL_SSL_LIB "${OPENSSL_ROOT}/lib_linux64/libssl.a")
    elseif(CMAKE_C_COMPILER MATCHES "gnueabihf|arm-")
        set(OPENSSL_SSL_LIB "${OPENSSL_ROOT}/lib/libssl.a")
    else()
        set(OPENSSL_SSL_LIB "${OPENSSL_ROOT}/lib/libssl.a")
    endif()
endif()

if(DEFINED LIB_CRYPTO AND EXISTS "${LIB_CRYPTO}")
    set(OPENSSL_CRYPTO_LIB "${LIB_CRYPTO}")
else()
    if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(OPENSSL_CRYPTO_LIB "${OPENSSL_ROOT}/win64/libcrypto.a")
    elseif(CMAKE_C_COMPILER MATCHES "aarch64|arm64")
        set(OPENSSL_CRYPTO_LIB "${OPENSSL_ROOT}/lib64/libcrypto.a")
    elseif(CMAKE_C_COMPILER MATCHES "x86_64|/usr/bin/gcc|/usr/bin/cc")
        set(OPENSSL_CRYPTO_LIB "${OPENSSL_ROOT}/lib_linux64/libcrypto.a")
    elseif(CMAKE_C_COMPILER MATCHES "gnueabihf|arm-")
        set(OPENSSL_CRYPTO_LIB "${OPENSSL_ROOT}/lib/libcrypto.a")
    else()
        set(OPENSSL_CRYPTO_LIB "${OPENSSL_ROOT}/lib/libcrypto.a")
    endif()
endif()

if(NOT EXISTS "${OPENSSL_SSL_LIB}")
    message(FATAL_ERROR "libssl not found: ${OPENSSL_SSL_LIB}")
endif()

if(NOT EXISTS "${OPENSSL_CRYPTO_LIB}")
    message(FATAL_ERROR "libcrypto not found: ${OPENSSL_CRYPTO_LIB}")
endif()

message(STATUS "OpenSSL root: ${OPENSSL_ROOT}")
message(STATUS "OpenSSL include: ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OpenSSL ssl lib: ${OPENSSL_SSL_LIB}")
message(STATUS "OpenSSL crypto lib: ${OPENSSL_CRYPTO_LIB}")

# ===================== 头文件目录 =====================
set(INC_PATH
    ${SDK_CLIENT_PATH}
    ${SDK_CLIENT_PATH}/include
    ${SDK_CLIENT_PATH}/src

    ${SDK_SHARE_PATH}
    ${SDK_SHARE_PATH}/include
    ${SDK_SHARE_PATH}/tools

    ${OPENSSL_INCLUDE_DIR}
)

foreach(BASE_PATH ${INC_PATH})
    if(EXISTS ${BASE_PATH})
        include_directories(${BASE_PATH})

        file(GLOB_RECURSE SUB_DIRS LIST_DIRECTORIES true "${BASE_PATH}/*")
        foreach(SUB_DIR ${SUB_DIRS})
            if(IS_DIRECTORY ${SUB_DIR})
                include_directories(${SUB_DIR})
            endif()
        endforeach()
    endif()
endforeach()

# ===================== 源文件收集 =====================
set(SRC_DIRS
    ${SDK_CLIENT_PATH}/src
    ${SDK_SHARE_PATH}/tools
)

set(ALL_SRC_FILES "")
foreach(SRC_DIR ${SRC_DIRS})
    if(EXISTS ${SRC_DIR})
        file(GLOB_RECURSE DIR_SRC_FILES
            "${SRC_DIR}/*.c"
            "${SRC_DIR}/*.cpp"
            "${SRC_DIR}/*.cc"
        )
        list(APPEND ALL_SRC_FILES ${DIR_SRC_FILES})
    endif()
endforeach()

# ===================== Demo 源文件 =====================
file(GLOB DEMO_SRC
    "${CMAKE_CURRENT_LIST_DIR}/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/*.cc"
)

# ===================== 生成可执行文件 =====================
add_executable(${PROJECT_NAME} ${DEMO_SRC} ${ALL_SRC_FILES})

# ===================== 链接库 =====================
set(LIBS "")

if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(APPEND LIBS
        ${OPENSSL_SSL_LIB}
        ${OPENSSL_CRYPTO_LIB}
        ws2_32
        crypt32
        iphlpapi
        bcrypt
        advapi32
        user32
        gdi32
    )
else()
    list(APPEND LIBS
        ${OPENSSL_SSL_LIB}
        ${OPENSSL_CRYPTO_LIB}
        pthread
        dl
    )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBS})

# ===================== 输出信息 =====================
message(STATUS "项目名称: ${PROJECT_NAME}")

list(LENGTH ALL_SRC_FILES SRC_COUNT)
message(STATUS "SDK源文件数量: ${SRC_COUNT}")

list(LENGTH DEMO_SRC DEMO_SRC_COUNT)
message(STATUS "Demo源文件数量: ${DEMO_SRC_COUNT}")

message(STATUS "最终链接库: ${LIBS}")
