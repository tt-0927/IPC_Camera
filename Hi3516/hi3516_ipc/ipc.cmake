# 项目根目录
set(ROOT_PATH  ${CMAKE_CURRENT_LIST_DIR}/../)
set(SHARE_PATH ${ROOT_PATH}/share)
set(IPC_PLATFORM_PATH ${ROOT_PATH}/ipc_platform)
# 顶层CMakelist.txt所在路径
set(SRC_ROOT_PATH   ${CMAKE_CURRENT_LIST_DIR})
# 摄像机共享库路径
set(CAM_SHARE_PATH   ${SHARE_PATH}/cam_share)
# 网络摄像机共享库路径
set(IPC_SHARE_PATH   ${SHARE_PATH}/ipc_share)
# AI共享库路径
set(AI_SHARE_PATH   ${SHARE_PATH}/ai_share)
# 海思api封装仓库路径
set(HI_PIPELINE_PATH  ${ROOT_PATH}/hi_pipeline)
# 海思mpp路径
set(HI_MPP_PATH  ${ROOT_PATH}/hi_pipeline/mpp)
# 海思cipher路径
set(HI_CIPHER_PATH  ${ROOT_PATH}/hi_pipeline/cipher)
# 海思 svp 路径
set(HI_SVP_PATH  ${ROOT_PATH}/hi_pipeline/svp)

include(${CMAKE_CURRENT_LIST_DIR}/../ipc_platform/ipc_platform.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/device_profiles.cmake)

# 引用lib
include(${IPC_PLATFORM_PATH}/lib/freetype/freetype.cmake)
include(${IPC_PLATFORM_PATH}/lib/ss_mpi/ss_mpi.cmake)
include(${IPC_PLATFORM_PATH}/lib/z/z.cmake)
include(${IPC_PLATFORM_PATH}/lib/xml2/xml2.cmake)
include(${IPC_PLATFORM_PATH}/lib/RtspServer/RtspServer.cmake)
include(${IPC_PLATFORM_PATH}/lib/MemoryCheck/MemoryCheck.cmake)
include(${IPC_PLATFORM_PATH}/lib/sqlite3/sqlite3.cmake)
include(${IPC_PLATFORM_PATH}/lib/zlog/zlog.cmake)
include(${IPC_PLATFORM_PATH}/lib/srt/srt.cmake)
include(${IPC_PLATFORM_PATH}/lib/eXosip2/eXosip2.cmake)
include(${IPC_PLATFORM_PATH}/lib/osip2/osip2.cmake)
include(${IPC_PLATFORM_PATH}/lib/osipparser2/osipparser2.cmake)
include(${IPC_PLATFORM_PATH}/lib/pugixml/pugixml.cmake)
# include(${IPC_PLATFORM_PATH}/lib/gmssl/gmssl.cmake)
include(${IPC_PLATFORM_PATH}/lib/openssl/openssl.cmake)
include(${IPC_PLATFORM_PATH}/lib/agent++/agent++.cmake)
include(${IPC_PLATFORM_PATH}/lib/snmp++/snmp++.cmake)
include(${IPC_PLATFORM_PATH}/lib/upnp/upnp.cmake)
include(${IPC_PLATFORM_PATH}/lib/iconv/iconv.cmake)
include(${IPC_PLATFORM_PATH}/lib/websockets/websockets.cmake)
# include(${IPC_PLATFORM_PATH}/lib/opencv/opencv.cmake)
# include(${IPC_PLATFORM_PATH}/lib/opencvfont/opencvfont.cmake)
include(${IPC_PLATFORM_PATH}/lib/SDL2/SDL2.cmake)
include(${IPC_PLATFORM_PATH}/lib/SDL2_ttf/SDL2_ttf.cmake)
include(${IPC_PLATFORM_PATH}/lib/twolame/twolame.cmake)
include(${IPC_PLATFORM_PATH}/lib/onvif/onvif.cmake)
include(${IPC_PLATFORM_PATH}/lib/fcgi/fcgi.cmake)
include(${IPC_PLATFORM_PATH}/lib/ffmpeg/ffmpeg.cmake)
include(${IPC_PLATFORM_PATH}/lib/fdk-aac/fdk-aac.cmake)
include(${IPC_PLATFORM_PATH}/lib/x264/x264.cmake)
include(${IPC_PLATFORM_PATH}/lib/x265/x265.cmake)
include(${IPC_PLATFORM_PATH}/lib/mqtt/mqtt.cmake)
include(${IPC_PLATFORM_PATH}/lib/curl/curl.cmake)
# include(${IPC_PLATFORM_PATH}/lib/HCNetSDK/HCNetSDK.cmake)
include(${IPC_PLATFORM_PATH}/lib/tvsdk/tvsdk.cmake)

# 设置生成文件存放路径
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ROOT_PATH}/output/bin)

# 应用设备画像（业务仓库统一入口）
if(NOT DEFINED DEVICE_TYPE)
    message(FATAL_ERROR "DEVICE_TYPE 未定义, 请传递:-DDEVICE_TYPE=xxx")
endif()
apply_device_profile("${DEVICE_TYPE}")

if(NOT DEFINED IPC_CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE)
    set(IPC_CAP_RECORD_NEEDS_CAM_SHARE_INCLUDE 0)
endif()
if(NOT DEFINED IPC_CAP_RECORD_LINK_FDK_AAC)
    set(IPC_CAP_RECORD_LINK_FDK_AAC 0)
endif()

# 接收传入的 SENSOR_TYPE
if(NOT DEFINED SENSOR_TYPE)
    message(FATAL_ERROR "SENSOR_TYPE 未定义, 请传递:-DSENSOR_TYPE=xxx")
endif()

message(STATUS "SENSOR_TYPE = ${SENSOR_TYPE}")
# 拆分「型号 + 焦距」
# 按 '-' 拆
string(REPLACE "-" ";" SENSOR_PARTS "${SENSOR_TYPE}")
list(LENGTH SENSOR_PARTS SENSOR_PART_COUNT)

if(NOT SENSOR_PART_COUNT EQUAL 2)
    message(FATAL_ERROR "Invalid SENSOR_TYPE format: ${SENSOR_TYPE}")
endif()

list(GET SENSOR_PARTS 0 SENSOR_MODEL)   # eg: sc500ai
list(GET SENSOR_PARTS 1 SENSOR_FOCAL)   # eg: f2_8mm
# 生成【传感器型号】宏
string(TOUPPER "${SENSOR_MODEL}" SENSOR_MODEL_UPPER)
string(REPLACE "-" "_" SENSOR_MODEL_UPPER "${SENSOR_MODEL_UPPER}")

# 生成【焦距】宏
# 去掉前缀 f
string(REGEX REPLACE "^f" "" SENSOR_FOCAL_VAL "${SENSOR_FOCAL}")
# 转为大写宏友好格式
string(TOUPPER "${SENSOR_FOCAL_VAL}" SENSOR_FOCAL_UPPER)
string(REPLACE "_" "_" SENSOR_FOCAL_UPPER "${SENSOR_FOCAL_UPPER}")

add_compile_definitions(SENSOR_${SENSOR_MODEL_UPPER})
add_compile_definitions(FOCAL_${SENSOR_FOCAL_UPPER})
add_compile_definitions(SENSOR_TYPE_STR="${SENSOR_TYPE}")

# 项目类型宏
if(PROJECT_TYPE STREQUAL "itc")
    add_compile_definitions(PROJECT_TYPE_ITC)
elseif(PROJECT_TYPE STREQUAL "中性")
    add_compile_definitions(PROJECT_TYPE_ZHONGXING)
endif()

# 输出型号信息
message("==========> 当前编译型号: ${DEVICE_TYPE}")
# Debug版本
if(BUILD_VERSION STREQUAL "Debug")
    # set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fsanitize=leak -fno-omit-frame-pointer -static-libasan")
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=leak -fno-omit-frame-pointer -static-libasan")

    # 添加调试符号
    # 开启调试信息生成
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -ggdb -fdebug-types-section -fdebug-prefix-map=$(pwd)=.")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -ggdb -fdebug-types-section -fdebug-prefix-map=$(pwd)=.")
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
    # set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")

    message("==========> 当前编译版本: Debug")
else()
    # Release 构建类型设置
    # set(CMAKE_BUILD_TYPE Release)
    # 链接时省略符号信息
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s -Os -ffunction-sections -fdata-sections")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s -Os -ffunction-sections -fdata-sections")

    # 控制 .so 和 .elf 链接行为 链接时清除未被引用的函数/变量
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections")

    # 开启链接时优化 LTO（Link Time Optimization）
    # 方式一：
    # set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    # 方式二：
    # set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto")
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
    # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")

    message("==========> 当前编译版本: Release")
endif()

# 显示所有警告信息
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fPIC")

# 添加编译警告颜色提示
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fdiagnostics-color=always")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always")

# 预防地址位数被截断导致程序崩溃的问题
# 隐式函数声明
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror=implicit-function-declaration ")
# c++无需开启此选项
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=implicit-function-declaration ")

# 格式化字符串的警告会报错，
# 除了format-truncation-可能的截断的警告
# 除了format-overflow-目标缓冲区可能会溢出
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror=format -Wno-error=format-truncation -Wno-error=format-overflow")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=format -Wno-error=format-truncation -Wno-error=format-overflow")
# 要求所有函数都必须显式地返回值
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror=return-type -Werror=int-conversion")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=return-type")

#宏定义为 "" 空不报错   字符串字面量不报错  变量赋值未使用  指针类型不兼容  已弃用声明
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-error=format-zero-length -Wno-write-strings -Wno-unused-but-set-variable -Wno-incompatible-pointer-types -Wno-deprecated-declarations")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=format-zero-length -Wno-write-strings")

# 隐式类型转换
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror=incompatible-pointer-types")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=incompatible-pointer-types")

# 开启内存管理错误检查(gcc的版本11以上才支持)
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -Werror=mismatched-dealloc")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -Werror=mismatched-dealloc")

# 隐藏ABI变化相关的提示信息
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -Wno-psabi")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -Wno-psabi")

# 设置 C 标准为 C11
set(CMAKE_C_STANDARD 11)
# 如果编译器不支持则直接报错（而非降级）
set(CMAKE_C_STANDARD_REQUIRED ON)
# 设置 C++ 标准为 C++17
set(CMAKE_CXX_STANDARD 17)
# 如果编译器不支持则直接报错（而非降级）
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 禁用编译器专属扩展（如 GNU 的 -std=gnu++11）
set(CMAKE_CXX_EXTENSIONS OFF)
