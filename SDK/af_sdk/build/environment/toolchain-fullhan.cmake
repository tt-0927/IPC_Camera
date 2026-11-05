# Fullhan（富瀚）交叉编译工具链
#
# 工具链根目录默认 /opt/fullhan/toolchain，可在命令行覆盖：
#   cmake -DFULLHAN_TOOLCHAIN_ROOT=/your/path ...
# 目录结构要求：
#   <root>/bin/<prefix>-gcc
#   <root>/bin/<prefix>-g++
# 编译器前缀自动探测，兼容不同版本的 fullhan 工具链命名。
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

if(NOT DEFINED FULLHAN_TOOLCHAIN_ROOT)
    set(FULLHAN_TOOLCHAIN_ROOT "/opt/fullhan/toolchain" CACHE PATH "Fullhan toolchain root")
endif()

if(NOT IS_DIRECTORY "${FULLHAN_TOOLCHAIN_ROOT}")
    message(FATAL_ERROR "Fullhan 工具链目录不存在：${FULLHAN_TOOLCHAIN_ROOT}")
endif()

file(GLOB _fullhan_gcc_candidates
    "${FULLHAN_TOOLCHAIN_ROOT}/bin/*-gcc"
    "${FULLHAN_TOOLCHAIN_ROOT}/*/bin/*-gcc")
list(LENGTH _fullhan_gcc_candidates _fullhan_gcc_count)
if(_fullhan_gcc_count EQUAL 0)
    message(FATAL_ERROR "未找到 Fullhan gcc：${FULLHAN_TOOLCHAIN_ROOT}/bin/*-gcc")
endif()
list(GET _fullhan_gcc_candidates 0 _fullhan_gcc)
get_filename_component(_fullhan_gcc_name "${_fullhan_gcc}" NAME)
string(REGEX REPLACE "-gcc$" "" _fullhan_prefix "${_fullhan_gcc_name}")
# 工具链可能多包一层版本目录（如 <root>/<prefix>-b1/bin），以实际 gcc 所在目录为准
get_filename_component(_fullhan_bin_dir "${_fullhan_gcc}" DIRECTORY)

set(CMAKE_C_COMPILER "${_fullhan_bin_dir}/${_fullhan_prefix}-gcc")
set(CMAKE_CXX_COMPILER "${_fullhan_bin_dir}/${_fullhan_prefix}-g++")
set(CMAKE_AR "${_fullhan_bin_dir}/${_fullhan_prefix}-ar")
set(CMAKE_RANLIB "${_fullhan_bin_dir}/${_fullhan_prefix}-ranlib")

set(CMAKE_C_COMPILER_FORCED ON)
set(CMAKE_CXX_COMPILER_FORCED ON)

# 部分 Fullhan 工具链包的 liblto_plugin.so 缺失或为 0 字节，链接时会报
# "error loading plugin: ... file too short"。SDK 未使用 LTO，直接禁用链接插件。
set(CMAKE_C_FLAGS_INIT "-fno-use-linker-plugin")
set(CMAKE_CXX_FLAGS_INIT "-fno-use-linker-plugin")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-fno-use-linker-plugin")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-fno-use-linker-plugin")

message(STATUS "Fullhan toolchain root = ${FULLHAN_TOOLCHAIN_ROOT}")
message(STATUS "Fullhan toolchain prefix = ${_fullhan_prefix}")

# openssl 静态库：优先使用 fullhan 专用目录，缺失时回退到 arm32 通用目录。
# 注意：静态库必须与工具链 ABI（glibc/uclibc、浮点 ABI）一致，否则链接或运行会失败。
set(_fullhan_ssl_dir "${CMAKE_CURRENT_LIST_DIR}/third-party/fullhan/openssl/lib")
if(NOT EXISTS "${_fullhan_ssl_dir}/libssl.a")
    set(_fullhan_ssl_dir "${CMAKE_CURRENT_LIST_DIR}/third-party/openssl/lib")
endif()

set(LIB_SSL "${_fullhan_ssl_dir}/libssl.a")
set(LIB_CRYPTO "${_fullhan_ssl_dir}/libcrypto.a")
if(NOT EXISTS "${LIB_SSL}" OR NOT EXISTS "${LIB_CRYPTO}")
    message(FATAL_ERROR "Fullhan openssl 静态库不存在：${_fullhan_ssl_dir}")
endif()

set(INCLUDE_SSL "${CMAKE_CURRENT_LIST_DIR}/third-party/fullhan/openssl/include")
if(NOT IS_DIRECTORY "${INCLUDE_SSL}")
    set(INCLUDE_SSL "${CMAKE_CURRENT_LIST_DIR}/third-party/openssl/include")
endif()

message(STATUS "Fullhan openssl lib dir = ${_fullhan_ssl_dir}")
message(STATUS "Fullhan openssl include dir = ${INCLUDE_SSL}")
