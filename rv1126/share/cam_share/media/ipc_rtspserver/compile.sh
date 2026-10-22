#!/bin/bash
#
# ipc_rtspserver 本地编译脚本（源码树直接构建）
#
# 在本目录执行即可完成 cmake 配置与编译，封装常用参数：
# 无参数为主机构建（源码模式 libevent，含 PoC 与单测）；
# 传入交叉前缀为板端构建（MinSizeRel，只出静态库）。
#
# 用法:
#   ./compile.sh [cross-prefix] [选项]
#
# 示例:
#   ./compile.sh                                     # 主机编译
#   ./compile.sh /opt/hisi-linux/x86-arm/arm-v01c02-linux-musleabi-gcc/bin/arm-linux-musleabi-
#                                                    # Hi3516 交叉编译
#   ./compile.sh <cross-prefix> --sync-ipc-platform  # 交叉编译并同步固件 SDK 归档
#   ./compile.sh --libevent-sdk <dir>                # SDK 模式接入 libevent（产品形态）
#
# 选项:
#   --sync-ipc-platform [DIR]  编译后同步 libipc_rtspserver.a 与 include/ 到固件 SDK
#                              归档目录，缺省为仓库内 ipc_platform/lib/rtsp_smol；
#                              libsmolrtsp.a 基线归档不会被覆盖
#   --libevent-sdk DIR         libevent 以 SDK 模式接入（DIR 含 include/ 与 lib/）
#   --min                      只编译 ipc_rtspserver 库目标（跳过 PoC 与单测）
#   -h, --help                 显示本说明

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "${SCRIPT_DIR}"

# ---- 参数解析 ----
CROSS_PREFIX=
SYNC_IPC_PLATFORM_DIR=
LIBEVENT_SDK_DIR=
BUILD_MIN=0
while [ $# -gt 0 ]; do
    case "$1" in
        --sync-ipc-platform)
            # 允许 --sync-ipc-platform 后不带目录（使用缺省推导）
            if [ $# -ge 2 ] && [ "${2#-}" = "$2" ]; then
                SYNC_IPC_PLATFORM_DIR="$2"
                shift 2
            else
                SYNC_IPC_PLATFORM_DIR="__default__"
                shift
            fi
            ;;
        --sync-ipc-platform=*)
            SYNC_IPC_PLATFORM_DIR="${1#*=}"
            [ -n "${SYNC_IPC_PLATFORM_DIR}" ] || SYNC_IPC_PLATFORM_DIR="__default__"
            shift
            ;;
        --libevent-sdk)
            if [ $# -lt 2 ]; then
                echo "--libevent-sdk 缺少目录参数" >&2
                exit 2
            fi
            LIBEVENT_SDK_DIR="$2"
            shift 2
            ;;
        --libevent-sdk=*)
            LIBEVENT_SDK_DIR="${1#*=}"
            shift
            ;;
        --min)
            BUILD_MIN=1
            shift
            ;;
        -h|--help)
            grep '^#' "$0" | { read _; cat; } | sed 's/^# \{0,1\}//'
            exit 0
            ;;
        --*)
            echo "未知参数:$1" >&2
            exit 2
            ;;
        *)
            if [ -z "$CROSS_PREFIX" ]; then
                CROSS_PREFIX="$1"
            else
                echo "只能指定一个 cross-prefix:$1" >&2
                exit 2
            fi
            shift
            ;;
    esac
done

# ---- 固件 SDK 归档目录缺省推导（share/cam_share/media/ipc_rtspserver -> 仓库根） ----
DEFAULT_SDK_DIR="${SCRIPT_DIR}/../../../../ipc_platform/lib/rtsp_smol"
if [ "${SYNC_IPC_PLATFORM_DIR}" = "__default__" ]; then
    SYNC_IPC_PLATFORM_DIR="$(cd "${DEFAULT_SDK_DIR}" 2>/dev/null && pwd)" || true
fi

# ---- 交叉工具链检测 ----
STRIP=strip
CMAKE_TOOL_ARGS=
BUILD_DIR="${SCRIPT_DIR}/build/host_local"
BUILD_TYPE=RelWithDebInfo
EXTRA_C_FLAGS=
if [ -n "$CROSS_PREFIX" ]; then
    for TOOL in gcc g++ ar ranlib strip; do
        command -v "${CROSS_PREFIX}${TOOL}" >/dev/null 2>&1 || {
            echo "找不到交叉工具链:${CROSS_PREFIX}${TOOL}" >&2
            exit 1
        }
    done
    STRIP="${CROSS_PREFIX}strip"
    CMAKE_TOOL_ARGS="-DCMAKE_C_COMPILER=${CROSS_PREFIX}gcc \
-DCMAKE_CXX_COMPILER=${CROSS_PREFIX}g++ \
-DCMAKE_AR=${CROSS_PREFIX}ar \
-DCMAKE_RANLIB=${CROSS_PREFIX}ranlib \
-DCMAKE_STRIP=${CROSS_PREFIX}strip"
    BUILD_DIR="${SCRIPT_DIR}/build/cross"
    BUILD_TYPE=MinSizeRel
    EXTRA_C_FLAGS=" -Os -ffunction-sections -fdata-sections"
fi

# ---- libevent 接入模式 ----
LIBEVENT_ARGS=
if [ -n "$LIBEVENT_SDK_DIR" ]; then
    if [ ! -f "${LIBEVENT_SDK_DIR}/lib/libevent_core.a" ] || \
       [ ! -f "${LIBEVENT_SDK_DIR}/include/event2/event-config.h" ]; then
        echo "libevent SDK 不完整（需 include/event2 + lib/libevent_core.a）:${LIBEVENT_SDK_DIR}" >&2
        exit 1
    fi
    LIBEVENT_ARGS="-DIPC_RTSP_LIBEVENT_MODE=SDK -DIPC_RTSP_LIBEVENT_SDK_DIR=${LIBEVENT_SDK_DIR}"
fi
# 缺省 SOURCE 模式使用 third_party/ 过渡镜像，无需联网

# ---- 测试与 PoC 开关：交叉或 --min 时关闭 ----
BUILD_TESTS=ON
BUILD_POC=ON
if [ -n "$CROSS_PREFIX" ] || [ "$BUILD_MIN" -eq 1 ]; then
    BUILD_TESTS=OFF
    BUILD_POC=OFF
fi

# ---- 陈旧缓存自检 ----
# 两种情况自动清空本 build 子目录后重新配置（不动其他构建目录）：
# 1. 源码树连同 build/ 被整体拷贝到其他机器/路径，CMakeCache.txt 记录的
#    绝对路径与当前路径不一致，cmake 会直接报错；
# 2. 换过 cmake 版本（如 4.x 缓存换 3.16 编译），缓存的生成版本与当前
#    cmake 主次版本不一致。
if [ -f "${BUILD_DIR}/CMakeCache.txt" ]; then
    CACHE_HOME="$(sed -n 's/^CMAKE_HOME_DIRECTORY:INTERNAL=//p' "${BUILD_DIR}/CMakeCache.txt")"
    CACHE_BUILD="$(sed -n 's/^CMAKE_CACHEFILE_DIR:INTERNAL=//p' "${BUILD_DIR}/CMakeCache.txt")"
    CACHE_CMAKE_MM="$(sed -n 's/^CMAKE_CACHE_MAJOR_VERSION:INTERNAL=//p' "${BUILD_DIR}/CMakeCache.txt").$(sed -n 's/^CMAKE_CACHE_MINOR_VERSION:INTERNAL=//p' "${BUILD_DIR}/CMakeCache.txt")"
    CURRENT_CMAKE_MM="$(cmake --version | head -1)"
    CURRENT_CMAKE_MM="${CURRENT_CMAKE_MM##* }"
    CURRENT_CMAKE_MM="${CURRENT_CMAKE_MM%.*}"
    if [ "${CACHE_HOME}" != "${SCRIPT_DIR}" ] || [ "${CACHE_BUILD}" != "${BUILD_DIR}" ] || \
       [ "${CACHE_CMAKE_MM}" != "${CURRENT_CMAKE_MM}" ]; then
        echo "检测到陈旧的 CMake 缓存，自动清理后重新配置:"
        echo "  缓存记录的源码目录:${CACHE_HOME:-<未知>}"
        echo "  缓存记录的构建目录:${CACHE_BUILD:-<未知>}"
        echo "  缓存记录的 cmake 版本:${CACHE_CMAKE_MM}，当前:${CURRENT_CMAKE_MM}"
        rm -rf "${BUILD_DIR}"
    fi
fi

# ---- CMake 配置与构建（并发度不超过 6，与仓库约定一致） ----
cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DCMAKE_C_FLAGS="${EXTRA_C_FLAGS}" \
    -DCMAKE_CXX_FLAGS="${EXTRA_C_FLAGS}" \
    ${LIBEVENT_ARGS} \
    -DIPC_RTSP_BUILD_TESTS=${BUILD_TESTS} \
    -DIPC_RTSP_BUILD_POC=${BUILD_POC} \
    ${CMAKE_TOOL_ARGS}

if [ "$BUILD_MIN" -eq 1 ]; then
    cmake --build "${BUILD_DIR}" --target ipc_rtspserver -- -j6
else
    cmake --build "${BUILD_DIR}" -- -j6
fi

# ---- 交叉产物 strip ----
ARCHIVE="${BUILD_DIR}/libipc_rtspserver.a"
if [ ! -f "${ARCHIVE}" ]; then
    # 单库构建时归档在 build 类型子目录
    ARCHIVE="$(find "${BUILD_DIR}" -name libipc_rtspserver.a | head -1)"
fi
if [ -n "$CROSS_PREFIX" ] && [ -n "${ARCHIVE}" ]; then
    "${STRIP}" -g "${ARCHIVE}"
fi

echo "构建完成:${ARCHIVE:-${BUILD_DIR}}"

# ---- 可选：同步 Hi3516 固件 SDK 归档 ----
if [ -n "${SYNC_IPC_PLATFORM_DIR}" ]; then
    if [ -z "$CROSS_PREFIX" ]; then
        echo "主机产物不允许同步到固件 SDK 目录，请改用交叉编译:" >&2
        echo "  ./compile.sh <cross-prefix> --sync-ipc-platform" >&2
        exit 1
    fi
    if [ -z "${SYNC_IPC_PLATFORM_DIR}" ] || [ ! -d "${SYNC_IPC_PLATFORM_DIR}" ]; then
        echo "同步目标不存在:${SYNC_IPC_PLATFORM_DIR:-<空>}" >&2
        echo "缺省路径应为 ipc_platform/lib/rtsp_smol，或显式传入目录" >&2
        exit 1
    fi
    [ -f "${ARCHIVE}" ] || { echo "未找到 libipc_rtspserver.a，无法同步" >&2; exit 1; }
    cp "${ARCHIVE}" "${SYNC_IPC_PLATFORM_DIR}/lib/"
    rm -rf "${SYNC_IPC_PLATFORM_DIR}/include"
    mkdir -p "${SYNC_IPC_PLATFORM_DIR}/include"
    cp -a "${SCRIPT_DIR}/include/." "${SYNC_IPC_PLATFORM_DIR}/include/"
    echo "已同步到 ${SYNC_IPC_PLATFORM_DIR}（libipc_rtspserver.a + include/，libsmolrtsp.a 基线未动）"
fi
