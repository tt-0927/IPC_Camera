cmake_minimum_required(VERSION 3.10)

project(ConfigClientDemo)

add_definitions(-DCPPHTTPLIB_OPENSSL_SUPPORT)

# ===================== 工具链配置 =====================
if(MSVC)
    message(STATUS "当前编译器: MSVC")
elseif(MINGW)
    message(STATUS "当前编译器: MinGW")
elseif(DEFINED CMAKE_TOOLCHAIN_FILE)
    message(STATUS "使用指定的工具链文件: ${CMAKE_TOOLCHAIN_FILE}")
elseif(DEFINED CMAKE_C_COMPILER)
    message(STATUS "使用指定的编译器: ${CMAKE_C_COMPILER}")
else()
    message(STATUS "未指定工具链，使用默认编译器")
endif()

# ===================== 路径配置 =====================
set(SDK_CLIENT_PATH ${CMAKE_CURRENT_LIST_DIR}/../../../../sdk_client)
set(SDK_SHARE_PATH  ${CMAKE_CURRENT_LIST_DIR}/../../../../sdk_share)

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

# ===================== 编译标准 =====================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

# ===================== OpenSSL 配置 =====================
# 优先兼容 toolchain-xxx.cmake 传入的变量：
#   INCLUDE_SSL
#   LIB_SSL
#   LIB_CRYPTO

if(MSVC)
    set(OPENSSL_LIB_CANDIDATES
        "${OPENSSL_ROOT}/lib_msvc64"
        "${OPENSSL_ROOT}/win64"
    )
elseif(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(OPENSSL_LIB_CANDIDATES
        "${OPENSSL_ROOT}/win64"
        "${OPENSSL_ROOT}/lib_msvc64"
    )
elseif(CMAKE_C_COMPILER MATCHES "aarch64|arm64")
    set(OPENSSL_LIB_CANDIDATES
        "${OPENSSL_ROOT}/lib64"
        "${OPENSSL_ROOT}/lib"
    )
elseif(CMAKE_C_COMPILER MATCHES "x86_64|/usr/bin/gcc|/usr/bin/cc")
    set(OPENSSL_LIB_CANDIDATES
        "${OPENSSL_ROOT}/lib_linux64"
        "${OPENSSL_ROOT}/lib"
    )
else()
    set(OPENSSL_LIB_CANDIDATES
        "${OPENSSL_ROOT}/lib"
    )
endif()

set(OPENSSL_LIB_DIR "")
foreach(_LIB_ROOT ${OPENSSL_LIB_CANDIDATES})
    if(EXISTS "${_LIB_ROOT}")
        set(OPENSSL_LIB_DIR "${_LIB_ROOT}")
        break()
    endif()
endforeach()

if(MSVC)
    # 禁用常见警告
    add_compile_options(
        /wd4800    # 禁用 C4800: 指针转布尔值的警告
        /wd4244    # 禁用 C4244: 类型转换可能丢失数据
        /wd4930    # 禁用 C4930: 未调用原型函数
        /wd4819    # 禁用 C4819: 文件编码问题
        /utf-8     # 使用 UTF-8 编码
    )
endif()

if(DEFINED INCLUDE_SSL AND EXISTS "${INCLUDE_SSL}")
    set(OPENSSL_INCLUDE_DIR "${INCLUDE_SSL}")
else()
    set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT}/include")
endif()

if(NOT EXISTS "${OPENSSL_INCLUDE_DIR}")
    message(FATAL_ERROR "OpenSSL include not found: ${OPENSSL_INCLUDE_DIR}")
endif()

# 如果 toolchain 已经传了具体静态库，优先使用
if(DEFINED LIB_SSL AND EXISTS "${LIB_SSL}")
    set(OPENSSL_SSL_LIB "${LIB_SSL}")
else()
    if(MSVC)
        set(OPENSSL_SSL_LIB "${OPENSSL_LIB_DIR}/libssl.lib")
    elseif(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(OPENSSL_SSL_LIB "${OPENSSL_LIB_DIR}/libssl.a")
    else()
        set(OPENSSL_SSL_LIB "${OPENSSL_LIB_DIR}/libssl.a")
    endif()
endif()

if(DEFINED LIB_CRYPTO AND EXISTS "${LIB_CRYPTO}")
    set(OPENSSL_CRYPTO_LIB "${LIB_CRYPTO}")
else()
    if(MSVC)
        set(OPENSSL_CRYPTO_LIB "${OPENSSL_LIB_DIR}/libcrypto.lib")
    elseif(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(OPENSSL_CRYPTO_LIB "${OPENSSL_LIB_DIR}/libcrypto.a")
    else()
        set(OPENSSL_CRYPTO_LIB "${OPENSSL_LIB_DIR}/libcrypto.a")
    endif()
endif()

if(NOT OPENSSL_LIB_DIR)
    message(FATAL_ERROR "OpenSSL lib dir not found. Tried: ${OPENSSL_LIB_CANDIDATES}")
endif()

if(NOT EXISTS "${OPENSSL_SSL_LIB}")
    message(FATAL_ERROR "libssl not found: ${OPENSSL_SSL_LIB}")
endif()

if(NOT EXISTS "${OPENSSL_CRYPTO_LIB}")
    message(FATAL_ERROR "libcrypto not found: ${OPENSSL_CRYPTO_LIB}")
endif()

message(STATUS "OpenSSL root: ${OPENSSL_ROOT}")
message(STATUS "OpenSSL include: ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OpenSSL lib dir: ${OPENSSL_LIB_DIR}")
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

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8 /W3 /EHsc)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

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
