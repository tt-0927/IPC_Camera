#!/bin/bash
set -euo pipefail  # 开启严格模式：出错退出、未定义变量报错、管道失败触发退出

# ===================== 彩色输出配置 =====================
RED="\033[31m"
GREEN="\033[32m"
YELLOW="\033[33m"
BLUE="\033[34m"
PURPLE="\033[35m"
RESET="\033[0m"

echo_red() { echo -e "${RED}$1${RESET}"; }
echo_green() { echo -e "${GREEN}$1${RESET}"; }
echo_yellow() { echo -e "${YELLOW}$1${RESET}"; }
echo_blue() { echo -e "${BLUE}$1${RESET}"; }
echo_purple() { echo -e "${PURPLE}$1${RESET}"; }

# ===================== 配置项 =====================
CURRENT_ROOT="$(pwd)"
TOOLCHAIN_ROOT="$(pwd)/environment/toolchain"
TOOLCHAIN_32_NAME="gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf"
TOOLCHAIN_64_NAME="gcc-linaro-7.5.0-2019.12-x86_64_aarch64-linux-gnu"
TOOLCHAIN_32_FILE="${TOOLCHAIN_ROOT}/../toolchain-arm32.cmake"
TOOLCHAIN_HISI32_FILE="${TOOLCHAIN_ROOT}/../toolchain-hisi32.cmake"
TOOLCHAIN_64_FILE="${TOOLCHAIN_ROOT}/../toolchain-aarch64.cmake"
TOOLCHAIN_WIN64_FILE="${TOOLCHAIN_ROOT}/../toolchain-win64.cmake"
TOOLCHAIN_X86_64_FILE="${TOOLCHAIN_ROOT}/../toolchain-x86.cmake"
BUILD_ROOT="${CURRENT_ROOT}/output"
PACKAGE_DIR="${BUILD_ROOT}/package"

# ===================== 帮助函数 =====================
usage() {
    echo_purple "==================================== 编译脚本帮助 ===================================="
    echo_green "用法: $0 [选项] <编译端> [参数...]"
    echo_green "参数说明："
    echo_green "  <编译端>   必须，可选值："
    echo_green "             ${YELLOW}server${GREEN} <位数>      （服务端库，32或64位）"
    echo_green "             ${YELLOW}client${GREEN} <位数>      （客户端库，32或64位）"
    echo_green "             可选平台：32 / hisi32 / 64 / linux64 / win64"
    echo_green "             其中：32=arm32(RK), hisi32=arm32(Hisi), 64=aarch64, linux64=x86_64 Linux, win64=x86_64 Windows-mingw"
    echo_green "             ${YELLOW}all${GREEN}                （编译所有库并打包）"
    echo_green "             ${YELLOW}all_32${GREEN}             （仅编译并打包32位 server/client）"
    echo_green "             ${YELLOW}all_64${GREEN}             （仅编译并打包64位 server/client）"
    echo_green "             ${YELLOW}all_linux64${GREEN}        （仅编译并打包 linux64 server/client）"
    echo_green "             ${YELLOW}all_win64${GREEN}          （仅编译并打包 win64 server/client）"
    echo_green "             ${YELLOW}demo_server${GREEN} <demo名称> [工具链前缀]（服务端demo）"
    echo_green "             ${YELLOW}demo_client${GREEN} <demo名称> [工具链前缀]（客户端demo）"
    echo_green ""
    echo_green "  [工具链前缀] Demo编译可选，若不指定则使用CMakeLists中定义的默认编译器"
    echo_green "               例如：${YELLOW}arm-rockchip1240-linux-gnueabihf${GREEN}"
    echo_green "               脚本会自动添加 -gcc, -g++ 等后缀"
    echo_green "  选项："
    echo_green "    -h       显示此帮助信息"
    echo_green "    -j <n>   编译线程数（可选，默认使用CPU核心数）"
    echo_green "    -c       构建前清理本次对应的输出目录（默认开启）"
    echo_green "    --no-clean  构建前不清理输出目录，允许复用旧的 CMakeCache"
    echo_green "    --autofile  编译完成后自动执行 autofile 命令传输产物"
    echo ""
    echo_green "示例："
    echo_green "  编译64位服务端库并传输：      ${YELLOW}$0 server 64 --autofile${RESET}"
    echo_green "  编译64位客户端库并传输：     ${YELLOW}$0 client 64 --autofile${RESET}"
    echo_green "  一键编译打包并传输：          ${YELLOW}$0 all --autofile${RESET}"
    echo_green "  仅打包64位 server/client：   ${YELLOW}$0 all_64 --autofile${RESET}"
    echo_green "  仅打包 linux64 server/client：${YELLOW}$0 all_linux64 --autofile${RESET}"
    echo_green "  编译demo（使用默认编译器）：  ${YELLOW}$0 demo_server capability --autofile${RESET}"
    echo_green "  编译64位版demo：            ${YELLOW}$0 demo_client config aarch64-linux-gnu --autofile${RESET}"
    echo_green "  编译demo（指定工具链）：      ${YELLOW}$0 demo_server capability arm-rockchip1240-linux-gnueabihf --autofile${RESET}"
    echo_green "  编译HTTP人脸服务端demo：     ${YELLOW}$0 demo_server http_face${RESET}"
    echo_green "  编译HTTP人脸客户端demo：     ${YELLOW}$0 demo_client http_face${RESET}"
    echo_purple "======================================================================================"
    exit 1
}

# ===================== 变量初始化 =====================
JOBS=$(nproc)
USE_AUTOFILE=false
CLEAN_BUILD=true

# ===================== 解析命令行参数 =====================
ARGS=()
while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help) usage ;;
        -j)
            if [[ $# -lt 2 ]]; then echo_red "错误：-j 后必须跟数字！"; usage; fi
            JOBS="$2"
            if ! [[ "$JOBS" =~ ^[0-9]+$ ]]; then echo_red "错误：-j 后必须跟数字！"; usage; fi
            shift 2
            ;;
        -j*)
            JOBS="${1#-j}"
            if ! [[ "$JOBS" =~ ^[0-9]+$ ]]; then echo_red "错误：-j 后必须跟数字！"; usage; fi
            shift
            ;;
        --autofile)
            USE_AUTOFILE=true
            shift
            ;;
        -c|--clean)
            CLEAN_BUILD=true
            shift
            ;;
        --no-clean)
            CLEAN_BUILD=false
            shift
            ;;
        -*)
            echo_red "错误：无效选项 $1"
            usage
            ;;
        *)
            ARGS+=("$1")
            shift
            ;;
    esac
done

set -- "${ARGS[@]}"
if [ $# -lt 1 ]; then echo_red "错误：必须指定 <编译端>！"; usage; fi

TARGET_ARG="$1"

reset_build_dir() {
    local TARGET_DIR="$1"

    if [ -z "$TARGET_DIR" ]; then
        echo_red "错误：清理目录为空，已拒绝执行。"
        return 1
    fi

    case "$TARGET_DIR" in
        "$BUILD_ROOT"/*) ;;
        *)
            echo_red "错误：清理目录不在 BUILD_ROOT 下，已拒绝执行：$TARGET_DIR"
            return 1
            ;;
    esac

    if [ -d "$TARGET_DIR" ]; then
        echo_yellow "-> 清理旧构建目录: $TARGET_DIR"
        rm -rf "$TARGET_DIR"
    fi
}

# ===================== 核心编译函数 =====================
build_project() {
    local TARGET_NAME=$1
    local BIT_MODE=$2
    
    echo_purple "\n==================== 开始构建: ${TARGET_NAME} (${BIT_MODE}位) ===================="
    
    # 1. 设置变量
    local TOOLCHAIN_FILE=""
    local WORK_BUILD_DIR=""
    local LIB_OUT_DIR=""
    # local BIN_OUT_DIR=""
    
    if [ "$BIT_MODE" = "32" ]; then
        TOOLCHAIN_FILE="$TOOLCHAIN_32_FILE"
        WORK_BUILD_DIR="$BUILD_ROOT/${TARGET_NAME}_32"
        LIB_OUT_DIR="$CURRENT_ROOT/${TARGET_NAME}/lib"
        # BIN_OUT_DIR="$BUILD_ROOT/bin/${TARGET_NAME}_32"
        # 补全库路径
        
        if [ ! -d "$TOOLCHAIN_ROOT/$TOOLCHAIN_32_NAME" ]; then
            echo_red "错误：32位工具链不存在！"
            return 1
        fi
    elif [ "$BIT_MODE" = "hisi32" ]; then
        TOOLCHAIN_FILE="$TOOLCHAIN_HISI32_FILE"
        WORK_BUILD_DIR="$BUILD_ROOT/${TARGET_NAME}_hisi32"
        LIB_OUT_DIR="$CURRENT_ROOT/${TARGET_NAME}/lib_hisi32"
        # BIN_OUT_DIR="$BUILD_ROOT/bin/${TARGET_NAME}_hisi32"
    elif [ "$BIT_MODE" = "64" ]; then
        TOOLCHAIN_FILE="$TOOLCHAIN_64_FILE"
        WORK_BUILD_DIR="$BUILD_ROOT/${TARGET_NAME}_64"
        LIB_OUT_DIR="$CURRENT_ROOT/${TARGET_NAME}/lib64"
        # BIN_OUT_DIR="$BUILD_ROOT/bin/${TARGET_NAME}_64"
        
        if [ ! -d "$TOOLCHAIN_ROOT/$TOOLCHAIN_64_NAME" ]; then
            echo_red "错误：64位工具链不存在！"
            return 1
        fi
    elif [ "$BIT_MODE" = "linux64" ]; then
        TOOLCHAIN_FILE="$TOOLCHAIN_X86_64_FILE"
        WORK_BUILD_DIR="$BUILD_ROOT/${TARGET_NAME}_linux64"
        LIB_OUT_DIR="$CURRENT_ROOT/${TARGET_NAME}/lib_linux64"
        # BIN_OUT_DIR="$BUILD_ROOT/bin/${TARGET_NAME}_linux64"
    elif [ "$BIT_MODE" = "win64" ]; then
        TOOLCHAIN_FILE="$TOOLCHAIN_WIN64_FILE"
        WORK_BUILD_DIR="$BUILD_ROOT/${TARGET_NAME}_win64"
        LIB_OUT_DIR="$CURRENT_ROOT/${TARGET_NAME}/lib_win64"
        # BIN_OUT_DIR="$BUILD_ROOT/bin/${TARGET_NAME}_win64"

        if ! command -v x86_64-w64-mingw32-gcc >/dev/null 2>&1; then
            echo_red "错误：未找到 x86_64-w64-mingw32-gcc，请先安装 mingw-w64！"
            return 1
        fi
    else
        echo_red "错误：位数/平台必须是 32、hisi32、64、linux64 或 win64！"
        return 1
    fi
    
    if [ ! -d "./$TARGET_NAME" ]; then
        echo_red "错误：项目目录 $TARGET_NAME 不存在！"
        return 1
    fi

    # 2. 合并头文件
    echo_blue "-> 合并头文件..."
    local FILE1="$(pwd)/../sdk_share/include/NetTVSDKCommon.h"
    local FILE2=""
    local HEADER_OUT_DIR=""
    local MERGE_SCRIPT=""
    
    if [ "$TARGET_NAME" == "server" ]; then
        FILE2="$(pwd)/../sdk_server/include/NetTVSDKServerInterface.h"
        HEADER_OUT_DIR="$(pwd)/server/include/"
        MERGE_SCRIPT="$(pwd)/server/merge_headers.sh"
    else
        FILE2="$(pwd)/../sdk_client/include/NetTVSDKClientInterface.h"
        HEADER_OUT_DIR="$(pwd)/client/include/"
        MERGE_SCRIPT="$(pwd)/client/merge_headers.sh"
    fi
    
    if [ -f "$MERGE_SCRIPT" ]; then
        bash "$MERGE_SCRIPT" "$FILE1" "$FILE2" "$HEADER_OUT_DIR"

        if [ "$TARGET_NAME" == "client" ]; then
            if [ ! -f "${HEADER_OUT_DIR}/NetTVSDK.h" ]; then
                echo_red "错误：客户端合并头文件失败，缺少 ${HEADER_OUT_DIR}/NetTVSDK.h"
                return 1
            fi
            if [ ! -f "${HEADER_OUT_DIR}/ItcNetTVSDK.h" ]; then
                echo_red "错误：客户端合并头文件失败，缺少 ${HEADER_OUT_DIR}/ItcNetTVSDK.h"
                return 1
            fi
        fi

        if [ "$TARGET_NAME" == "server" ]; then
            if [ ! -f "${HEADER_OUT_DIR}/NetTVSDKServer.h" ]; then
                echo_red "错误：服务端合并头文件失败，缺少 ${HEADER_OUT_DIR}/NetTVSDKServer.h"
                return 1
            fi
            if [ ! -f "${HEADER_OUT_DIR}/ItcNetTVSDKServer.h" ]; then
                echo_red "错误：服务端合并头文件失败，缺少 ${HEADER_OUT_DIR}/ItcNetTVSDKServer.h"
                return 1
            fi
        fi
    else
        echo_yellow "警告：合并脚本不存在，跳过 ($MERGE_SCRIPT)"
    fi

    # 3. CMake 配置与编译
    echo_blue "-> CMake 配置与编译..."
    if [ "$CLEAN_BUILD" == "true" ]; then
        reset_build_dir "$WORK_BUILD_DIR" || return 1
    fi
    mkdir -p "$WORK_BUILD_DIR"
    
    # 使用子shell进入目录，避免影响主流程
    (
        set -e  # 子shell中开启严格模式，错误立即退出
        cd "$WORK_BUILD_DIR"
        cmake ../../"$TARGET_NAME" \
            -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_VERBOSE_MAKEFILE=OFF \
            -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY="$LIB_OUT_DIR" \
            -DCMAKE_LIBRARY_OUTPUT_DIRECTORY="$LIB_OUT_DIR"
            
        make -j"$JOBS"
    ) || {
        echo_red "错误：${TARGET_NAME} (${BIT_MODE}位) 编译失败！"
        return 1
    }
    
    # 4. 检查结果
    local LIB_FILES=$(find "$LIB_OUT_DIR" -maxdepth 1 -name "*.so" -o -name "*.a")
    if [ -z "$LIB_FILES" ]; then
        echo_red "警告：未在 $LIB_OUT_DIR 找到编译产物！"
    else
        echo_green "构建成功！产物位置: $LIB_OUT_DIR"
        
        if [[ "$TARGET_ARG" != "all" && "$TARGET_ARG" != all_* ]] && [ "$USE_AUTOFILE" == "true" ]; then
            echo_purple "\n==================== 执行 Autofile 传输 ===================="
            
            # 创建 bin 输出目录
            local BIN_OUT_DIR="$BUILD_ROOT/bin"
            # 创建当前构建专属的 bin 输出目录，避免不同架构产物互相覆盖
            mkdir -p "$BIN_OUT_DIR"
            
            # 拷贝编译产物到 bin 目录
            for f in $LIB_FILES; do
                cp "$f" "$BIN_OUT_DIR/"
            done
            echo_blue "已拷贝编译产物到: $BIN_OUT_DIR"
            
            # 拷贝 compile.sh 到 bin 目录
            local COMPILE_SCRIPT="$CURRENT_ROOT/compile.sh"
            if [ -f "$COMPILE_SCRIPT" ]; then
                cp "$COMPILE_SCRIPT" "$BIN_OUT_DIR/"
                
                # 进入 bin 目录执行 autofile (只处理 .so 文件)
                (
                    cd "$BIN_OUT_DIR"
                    for f in $LIB_FILES; do
                        local FILENAME=$(basename "$f")
                        # 只处理 .so 文件
                        if [[ "$FILENAME" == *.so ]]; then
                            echo_blue "正在传输: $FILENAME"
                            autofile "$FILENAME"
                        fi
                    done
                )
                
                # 删除拷贝的 compile.sh
                rm -f "$BIN_OUT_DIR/compile.sh"
            else
                echo_yellow "警告：compile.sh 不存在，跳过传输"
            fi
            
            echo_purple "============================================================"
        fi
    fi
}

# ===================== 打包函数 =====================
package_selected() {
    local PACKAGE_TAG="$1"
    shift
    local PACKAGE_MODES=("$@")

    echo_purple "\n==================================== 开始打包 ===================================="
    
    # 0. 清理旧包
    rm -rf "$PACKAGE_DIR"
    mkdir -p "$PACKAGE_DIR"
    
    # 1. 准备目录结构
    mkdir -p "$PACKAGE_DIR/server/include"
    mkdir -p "$PACKAGE_DIR/client/include"

    for mode in "${PACKAGE_MODES[@]}"; do
        case "$mode" in
            32)
                mkdir -p "$PACKAGE_DIR/server/lib"
                mkdir -p "$PACKAGE_DIR/client/lib"
                ;;
            hisi32)
                mkdir -p "$PACKAGE_DIR/server/lib_hisi32"
                mkdir -p "$PACKAGE_DIR/client/lib_hisi32"
                ;;
            64)
                mkdir -p "$PACKAGE_DIR/server/lib64"
                mkdir -p "$PACKAGE_DIR/client/lib64"
                ;;
            linux64)
                mkdir -p "$PACKAGE_DIR/server/lib_linux64"
                mkdir -p "$PACKAGE_DIR/client/lib_linux64"
                ;;
            win64)
                mkdir -p "$PACKAGE_DIR/server/lib_win64"
                mkdir -p "$PACKAGE_DIR/client/lib_win64"
                ;;
        esac
    done
    
    # 2. 复制文件
    echo_blue "-> 正在收集文件..."
    
    cp -r server/include/* "$PACKAGE_DIR/server/include/" 2>/dev/null || true
    cp -r client/include/* "$PACKAGE_DIR/client/include/" 2>/dev/null || true

    for mode in "${PACKAGE_MODES[@]}"; do
        case "$mode" in
            32)
                cp server/lib/*.so "$PACKAGE_DIR/server/lib/" 2>/dev/null || true
                cp server/lib/*.a  "$PACKAGE_DIR/server/lib/" 2>/dev/null || true
                cp client/lib/*.so "$PACKAGE_DIR/client/lib/" 2>/dev/null || true
                cp client/lib/*.a  "$PACKAGE_DIR/client/lib/" 2>/dev/null || true
                ;;
            hisi32)
                cp server/lib_hisi32/*.so "$PACKAGE_DIR/server/lib_hisi32/" 2>/dev/null || true
                cp server/lib_hisi32/*.a  "$PACKAGE_DIR/server/lib_hisi32/" 2>/dev/null || true
                cp client/lib_hisi32/*.so "$PACKAGE_DIR/client/lib_hisi32/" 2>/dev/null || true
                cp client/lib_hisi32/*.a  "$PACKAGE_DIR/client/lib_hisi32/" 2>/dev/null || true
                ;;
            64)
                cp server/lib64/*.so "$PACKAGE_DIR/server/lib64/" 2>/dev/null || true
                cp server/lib64/*.a  "$PACKAGE_DIR/server/lib64/" 2>/dev/null || true
                cp client/lib64/*.so "$PACKAGE_DIR/client/lib64/" 2>/dev/null || true
                cp client/lib64/*.a  "$PACKAGE_DIR/client/lib64/" 2>/dev/null || true
                ;;
            linux64)
                cp server/lib_linux64/*.so "$PACKAGE_DIR/server/lib_linux64/" 2>/dev/null || true
                cp server/lib_linux64/*.a  "$PACKAGE_DIR/server/lib_linux64/" 2>/dev/null || true
                cp client/lib_linux64/*.so "$PACKAGE_DIR/client/lib_linux64/" 2>/dev/null || true
                cp client/lib_linux64/*.a  "$PACKAGE_DIR/client/lib_linux64/" 2>/dev/null || true
                ;;
            win64)
                cp server/lib_win64/*.dll "$PACKAGE_DIR/server/lib_win64/" 2>/dev/null || true
                cp server/lib_win64/*.a   "$PACKAGE_DIR/server/lib_win64/" 2>/dev/null || true
                cp server/lib_win64/*.lib "$PACKAGE_DIR/server/lib_win64/" 2>/dev/null || true
                cp client/lib_win64/*.dll "$PACKAGE_DIR/client/lib_win64/" 2>/dev/null || true
                cp client/lib_win64/*.a   "$PACKAGE_DIR/client/lib_win64/" 2>/dev/null || true
                cp client/lib_win64/*.lib "$PACKAGE_DIR/client/lib_win64/" 2>/dev/null || true
                ;;
            *)
                echo_yellow "警告：未知打包模式，已跳过: $mode"
                ;;
        esac
    done
    
    # 3. 压缩
    local TIME_TAG=$(date "+%Y%m%d_%H%M%S")
    local PKG_NAME="sdk_${PACKAGE_TAG}_${TIME_TAG}.tar.gz"
    local PKG_PATH="$BUILD_ROOT/$PKG_NAME"
    
    echo_blue "-> 正在压缩..."
    (
        cd "$BUILD_ROOT"
        tar -zcf "$PKG_NAME" -C package .
    )
    
    echo_green "\n==================================== 打包完成 ===================================="
    echo_green "压缩包路径：${YELLOW}$PKG_PATH${RESET}"
    echo_green "包名称：${YELLOW}$PKG_NAME${RESET}"
    
    # Autofile 处理
    if [ "$USE_AUTOFILE" == "true" ]; then
        echo_purple "\n==================== 执行 Autofile 传输 ===================="
        
        # 创建 bin 输出目录
        local BIN_OUT_DIR="$BUILD_ROOT/bin"
        mkdir -p "$BIN_OUT_DIR"
        
        # 拷贝压缩包到 bin 目录
        cp "$PKG_PATH" "$BIN_OUT_DIR/"
        echo_blue "已拷贝压缩包到: $BIN_OUT_DIR"
        
        # 拷贝 compile.sh 到 bin 目录
        local COMPILE_SCRIPT="$CURRENT_ROOT/compile.sh"
        if [ -f "$COMPILE_SCRIPT" ]; then
            cp "$COMPILE_SCRIPT" "$BIN_OUT_DIR/"
            
            # 进入 bin 目录执行 autofile
            (
                cd "$BIN_OUT_DIR"
                echo_blue "正在传输: $PKG_NAME"
                autofile "$PKG_NAME"
            )
            
            # 删除拷贝的 compile.sh
            rm -f "$BIN_OUT_DIR/compile.sh"
        else
            echo_yellow "警告：compile.sh 不存在，跳过传输"
        fi
        
        echo_purple "============================================================"
    fi
}

# ===================== Demo编译函数 =====================
build_demo() {
    local DEMO_TYPE=$1   # server 或 client
    local DEMO_NAME=$2   # demo 名称，如 capability, alarm
    local TOOLCHAIN=$3   # 工具链前缀名称，如 arm-rockchip1240-linux-gnueabihf，或空(使用CMakeLists默认编译器)
    
    local TOOLCHAIN_DESC="CMakeLists默认编译器"
    if [ -n "$TOOLCHAIN" ]; then
        TOOLCHAIN_DESC="工具链: $TOOLCHAIN"
    fi
    
    echo_purple "\n==================== 开始构建 Demo: ${DEMO_NAME} (${DEMO_TYPE}, ${TOOLCHAIN_DESC}) ===================="
    
    # 1. 设置变量
    local DEMO_SRC_DIR=""
    local WORK_BUILD_DIR=""
    local DEMO_OUT_DIR=""
    
    if [ "$DEMO_TYPE" = "server" ]; then
        DEMO_SRC_DIR="$CURRENT_ROOT/../sdk_server/demo/${DEMO_NAME}"
    else
        DEMO_SRC_DIR="$CURRENT_ROOT/../sdk_client/demo/${DEMO_NAME}"
    fi
    
    # 检查demo目录是否存在
    if [ ! -d "$DEMO_SRC_DIR" ]; then
        echo_red "错误：Demo目录不存在！($DEMO_SRC_DIR)"
        return 1
    fi
    
    # 设置输出目录后缀
    local BUILD_SUFFIX="default"
    if [ -n "$TOOLCHAIN" ]; then
        BUILD_SUFFIX="$TOOLCHAIN"
    fi
    
    WORK_BUILD_DIR="$BUILD_ROOT/demo_${DEMO_TYPE}_${DEMO_NAME}_${BUILD_SUFFIX}"
    DEMO_OUT_DIR="$WORK_BUILD_DIR"
    # BIN_OUT_DIR="$BUILD_ROOT/bin/demo_${DEMO_TYPE}_${DEMO_NAME}_${BUILD_SUFFIX}"
    
    # 2. CMake 配置与编译
    echo_blue "-> CMake 配置与编译..."
    if [ "$CLEAN_BUILD" == "true" ]; then
        reset_build_dir "$WORK_BUILD_DIR" || return 1
    fi
    mkdir -p "$WORK_BUILD_DIR"
    
    (
        cd "$WORK_BUILD_DIR"

        if [ -n "$TOOLCHAIN" ]; then
            if [ "$TOOLCHAIN" = "win64" ]; then
                cmake "$DEMO_SRC_DIR" \
                    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_WIN64_FILE" \
                    -DCMAKE_BUILD_TYPE=Release \
                    -DCMAKE_VERBOSE_MAKEFILE=OFF \
                    -DCMAKE_RUNTIME_OUTPUT_DIRECTORY="$DEMO_OUT_DIR"
            elif [ "$TOOLCHAIN" = "linux64" ] || [ "$TOOLCHAIN" = "x86_64" ]; then
                cmake "$DEMO_SRC_DIR" \
                    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_X86_64_FILE" \
                    -DCMAKE_BUILD_TYPE=Release \
                    -DCMAKE_VERBOSE_MAKEFILE=OFF \
                    -DCMAKE_RUNTIME_OUTPUT_DIRECTORY="$DEMO_OUT_DIR"
            elif [ "$TOOLCHAIN" = "aarch64-linux-gnu" ] || [[ "$TOOLCHAIN" == *"aarch64"* ]]; then
                cmake "$DEMO_SRC_DIR" \
                    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_64_FILE" \
                    -DCMAKE_BUILD_TYPE=Release \
                    -DCMAKE_VERBOSE_MAKEFILE=OFF \
                    -DCMAKE_RUNTIME_OUTPUT_DIRECTORY="$DEMO_OUT_DIR"
            elif [ "$TOOLCHAIN" = "arm-linux-gnueabihf" ] || [[ "$TOOLCHAIN" == *"gnueabihf"* ]]; then
                cmake "$DEMO_SRC_DIR" \
                    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_32_FILE" \
                    -DCMAKE_BUILD_TYPE=Release \
                    -DCMAKE_VERBOSE_MAKEFILE=OFF \
                    -DCMAKE_RUNTIME_OUTPUT_DIRECTORY="$DEMO_OUT_DIR"
            elif command -v "${TOOLCHAIN}-gcc" >/dev/null 2>&1 && command -v "${TOOLCHAIN}-g++" >/dev/null 2>&1; then
                # 其他情况保持原逻辑（兼容）
                cmake "$DEMO_SRC_DIR" \
                    -DCMAKE_C_COMPILER="${TOOLCHAIN}-gcc" \
                    -DCMAKE_CXX_COMPILER="${TOOLCHAIN}-g++" \
                    -DCMAKE_AR="${TOOLCHAIN}-ar" \
                    -DCMAKE_RANLIB="${TOOLCHAIN}-ranlib" \
                    -DCMAKE_BUILD_TYPE=Release \
                    -DCMAKE_VERBOSE_MAKEFILE=OFF \
                    -DCMAKE_RUNTIME_OUTPUT_DIRECTORY="$DEMO_OUT_DIR"
            else
                echo_red "错误：未识别或未找到工具链: $TOOLCHAIN"
                echo_yellow "当前找不到: ${TOOLCHAIN}-gcc / ${TOOLCHAIN}-g++"
                echo_yellow "32位 ARM 请使用 arm-linux-gnueabihf，或名称包含 gnueabihf，例如 arm-rockchip1240-linux-gnueabihf"
                echo_yellow "64位 ARM 请使用 aarch64-linux-gnu，或名称包含 aarch64"
                echo_yellow "x86_64 Linux 请使用 linux64 或 x86_64"
                echo_yellow "Windows 交叉编译请使用 win64"
                return 1
            fi
        else
            # 不指定工具链，使用CMakeLists.txt中定义的默认编译器
            cmake "$DEMO_SRC_DIR" \
                -DCMAKE_BUILD_TYPE=Release \
                -DCMAKE_VERBOSE_MAKEFILE=OFF \
                -DCMAKE_RUNTIME_OUTPUT_DIRECTORY="$DEMO_OUT_DIR"
        fi
            
        make -j"$JOBS"
    )
    
    # 3. 检查结果
    local DEMO_FILES=$(find "$DEMO_OUT_DIR" -maxdepth 1 -type f -executable 2>/dev/null || find "$DEMO_OUT_DIR" -maxdepth 1 -type f ! -name "*.cmake" ! -name "Makefile" ! -name "CMakeCache.txt" 2>/dev/null)
    if [ -z "$DEMO_FILES" ]; then
        echo_red "警告：未在 $DEMO_OUT_DIR 找到编译产物！"
    else
        echo_green "构建成功！产物位置: $DEMO_OUT_DIR"
        
        if [ "$USE_AUTOFILE" == "true" ]; then
            echo_purple "\n==================== 执行 Autofile 传输 ===================="
            
             # 创建 bin 输出目录
            local BIN_OUT_DIR="$BUILD_ROOT/bin"
            # 创建当前 demo 专属的 bin 输出目录，避免不同工具链产物互相覆盖
            mkdir -p "$BIN_OUT_DIR"
            
            # 拷贝编译产物到 bin 目录
            for f in $DEMO_FILES; do
                local FILENAME=$(basename "$f")
                # 过滤掉cmake相关文件
                if [[ "$FILENAME" != *.cmake ]] && [[ "$FILENAME" != Makefile ]] && [[ "$FILENAME" != CMakeCache.txt ]]; then
                    cp "$f" "$BIN_OUT_DIR/"
                fi
            done
            echo_blue "已拷贝编译产物到: $BIN_OUT_DIR"
            
            # 拷贝 compile.sh 到 bin 目录
            local COMPILE_SCRIPT="$CURRENT_ROOT/compile.sh"
            if [ -f "$COMPILE_SCRIPT" ]; then
                cp "$COMPILE_SCRIPT" "$BIN_OUT_DIR/"
                
                # 进入 bin 目录执行 autofile
                (
                    cd "$BIN_OUT_DIR"
                    for f in $DEMO_FILES; do
                        local FILENAME=$(basename "$f")
                        # 过滤掉cmake相关文件
                        if [[ "$FILENAME" != *.cmake ]] && [[ "$FILENAME" != Makefile ]] && [[ "$FILENAME" != CMakeCache.txt ]]; then
                            echo_blue "正在传输: $FILENAME"
                            autofile "$FILENAME"
                        fi
                    done
                )
                
                # 删除拷贝的 compile.sh
                rm -f "$BIN_OUT_DIR/compile.sh"
            else
                echo_yellow "警告：compile.sh 不存在，跳过传输"
            fi
            
            echo_purple "============================================================"
        fi
    fi
}

if [ "$TARGET_ARG" == "all" ]; then
    build_project server 32 || exit 1
    build_project server 64 || exit 1
    build_project client 32 || exit 1
    build_project client 64 || exit 1
    package_selected "all" 32 64
elif [ "$TARGET_ARG" == "all_32" ]; then
    build_project server 32 || exit 1
    build_project client 32 || exit 1
    package_selected "all_32" 32
elif [ "$TARGET_ARG" == "all_64" ]; then
    build_project server 64 || exit 1
    build_project client 64 || exit 1
    package_selected "all_64" 64
elif [ "$TARGET_ARG" == "all_linux64" ]; then
    build_project server linux64 || exit 1
    build_project client linux64 || exit 1
    package_selected "all_linux64" linux64
elif [ "$TARGET_ARG" == "all_win64" ]; then
    build_project server win64 || exit 1
    build_project client win64 || exit 1
    package_selected "all_win64" win64
elif [ "$TARGET_ARG" == "demo_server" ] || [ "$TARGET_ARG" == "demo_client" ]; then
    # Demo编译模式
    if [ $# -lt 2 ]; then
        echo_red "错误：demo 模式下必须指定 <demo名称>！"
        usage
    fi
    DEMO_NAME="$2"
    TOOLCHAIN_ARG="${3:-}"  # 可选的工具链前缀参数，如 arm-rockchip1240-linux-gnueabihf
    
    # 提取 demo 类型 (server 或 client)
    if [ "$TARGET_ARG" == "demo_server" ]; then
        build_demo "server" "$DEMO_NAME" "$TOOLCHAIN_ARG"
    else
        build_demo "client" "$DEMO_NAME" "$TOOLCHAIN_ARG"
    fi
else
    # 单独编译库模式
    if [ $# -lt 2 ]; then
        echo_red "错误：非 all 模式下必须指定 <位数>！"
        usage
    fi
    BIT_ARG="$2"
    
    if [ "$BIT_ARG" != "32" ] && [ "$BIT_ARG" != "hisi32" ] && [ "$BIT_ARG" != "64" ] && [ "$BIT_ARG" != "linux64" ] && [ "$BIT_ARG" != "win64" ]; then
        echo_red "错误：位数/平台必须是 32、hisi32、64、linux64 或 win64！"
        usage
    fi
    
    build_project "$TARGET_ARG" "$BIT_ARG"
fi

exit 0
