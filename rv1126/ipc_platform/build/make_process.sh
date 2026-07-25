#!/bin/bash
###
 # @FilePath     : make_process.sh
# @Author       : zhouzr@kfb.cn
# @Date         : 2025-06-13 09:47:25
 # @LastEditors  : zhouzr@kfb.cn
 # @LastEditTime : 2026-01-06 17:21:31
# @Description  : 编译程序脚本
###

# 获取配置文件所在目录
CUR_PATH=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
echo "${CUR_PATH}"
#导入环境配置
source "${CUR_PATH}/path.conf"

# 命令失败时立即退出
set -e

# 使用getopt来处理长选项
# :前面的需要带参数, 没有的不需要带参数
if ! ARGS=$(getopt -o b:e:m:c -l build:,extra:,target:,clean,autofile -n "$0" -- "$@"); then
    error "选项和参数解析失败"
    exit 1
fi
eval set -- "$ARGS"

# 单个编译
MAKE_MODE=false
MAKE_NAME=""
# 清空
CLEAN_MODE=false
# 编译模式
BUILD_MODE="Release"
# 额外参数
EXTRA_PARAM=""
# 是否自动摆渡
AUTOFILE_MODE=false

# 解析选项
# :前面的需要带参数, 没有的不需要带参数
while true; do
    case "$1" in
    -b | --build) # -b 编译模式
        BUILD_MODE="$2"
        shift 2
        ;;
    -c | --clean) # -c 清空模式
        CLEAN_MODE=true
        shift
        ;;
    -e | --extra) # -e 额外参数
        EXTRA_PARAM="$2"
        shift 2
        ;;
    -m | --target) # -m 单个编译
        MAKE_MODE=true
        MAKE_NAME="$2"
        shift 2
        ;;
    --autofile)
        AUTOFILE_MODE=true
        shift
        ;;
    --)
        shift
        break
        ;;
    *)
        error "选项和参数未定义处理：$ARGS"
        exit 1
        ;;
    esac
done

# 删除临时文件
if [ "$CLEAN_MODE" = true ]; then
    info "清除临时文件..."

    rm -rf "$CMAKE_TEMP_PATH"

    info "成功清除临时文件"

    info "清除上一次生成的程序..."
    find "${BIN_PATH:?}" -mindepth 1 -delete
    # rm -rvf ${BIN_PATH:?}/*
    # find "${STRIP_BIN_PATH:?}" -mindepth 1 -delete
    # rm -rvf "${OUTPUT_DIR_NAME:?}/bin_strip/*"
    info "成功清除上一次生成的程序"
    exit 0
fi

# 检查编译类型是否有效的函数
check_buile_type "$BUILD_MODE"

# 检查设备类型是否有效的函数
check_device_type "$DEVICE_TYPE"

# 定义一个函数
check_build() {
    # 编译服务器程序
    BUILD_TYPE="-DBUILD_VERSION=${BUILD_MODE}"

    # 判断是否需要重新 cmake
    CHECK_FILE_PATH="$CMAKE_TEMP_PATH/CMakeCache.txt"

    LAST_BUILD_VERSION=""
    LAST_BUILD_DEVICE_TYPE=""

    if [ -f "$CHECK_FILE_PATH" ]; then
        local last_version
        local last_device

        last_version=$(cmake_cache_get "BUILD_VERSION" "$CHECK_FILE_PATH")
        last_device=$(cmake_cache_get "DEVICE_TYPE" "$CHECK_FILE_PATH")

        [ -n "$last_version" ] && LAST_BUILD_VERSION="-DBUILD_VERSION=${last_version}"
        [ -n "$last_device" ] && LAST_BUILD_DEVICE_TYPE="-DDEVICE_TYPE=${last_device}"
    fi
}

# 增加设备型号定义的cmake参数
BUILD_DEVICE_TYPE="-DDEVICE_TYPE=${DEVICE_TYPE}"

# 编译程序
if [ "$MAKE_MODE" = true ]; then
    info "============> 编译单个程序: $MAKE_NAME <============"
    info "============> 编译模式: $BUILD_MODE <============"

    # 调用函数
    check_build

    # 判断是否需要重新 cmake
    if [ "$LAST_BUILD_VERSION" != "$BUILD_TYPE" ] ||
        [ "$LAST_BUILD_DEVICE_TYPE" != "$BUILD_DEVICE_TYPE" ]; then
        trace "重新 cmake"

        cd "$SRC_ROOT_PATH"
        trace "进入源码目录$(pwd)"

        if [ -e "${CMAKE_TEMP_PATH}/CMakeCache.txt" ]; then
            trace "删除cmake缓存变量"
            rm -v "${CMAKE_TEMP_PATH}/CMakeCache.txt"
        fi

        # cmake 输出文件到output
        trace "cmake命令参数 [${BUILD_TYPE} ${BUILD_DEVICE_TYPE} ${EXTRA_PARAM} . -B${CMAKE_TEMP_PATH} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON]"
        if ! cmake "${BUILD_TYPE}" "${BUILD_DEVICE_TYPE}" "${EXTRA_PARAM}" . -B"${CMAKE_TEMP_PATH}" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON; then
            error "cmake执行出现错误"
            exit 1
        fi

        # 强制删除可能存在的旧链接或文件（-f 选项保证文件不存在时不报错）
        rm -f "${PROJECT_ROOT_PATH}/compile_commands.json"
        # 软链接至项目根目录
        ln -sv "${CMAKE_TEMP_PATH}/compile_commands.json" "${PROJECT_ROOT_PATH}/"

        cp "${AUTO_FILE}" "${CMAKE_TEMP_PATH}"
    else
        trace "不执行cmake"
    fi

    # make 程序
    cd "$CMAKE_TEMP_PATH"
    trace "进入输出目录 [$CMAKE_TEMP_PATH]"

    #编译单个目标
    if ! make "$MAKE_NAME" -j6; then
        error "编译 $MAKE_NAME 失败"
        exit 1
    fi
    # 提取.debug调试信息
    # if [ "$BUILD_MODE" == "Debug" ]; then
    #     (
    #         cd $BIN_PATH
    #         /opt/hisi-linux/x86-arm/arm-v01c02-linux-musleabi-gcc/bin/arm-v01c02-linux-musleabi-objcopy --only-keep-debug $MAKE_NAME $MAKE_NAME.debug
    #         /opt/hisi-linux/x86-arm/arm-v01c02-linux-musleabi-gcc/bin/arm-v01c02-linux-musleabi-strip --strip-unneeded $MAKE_NAME
    #         /opt/hisi-linux/x86-arm/arm-v01c02-linux-musleabi-gcc/bin/arm-v01c02-linux-musleabi-objcopy --add-gnu-debuglink=$MAKE_NAME.debug $MAKE_NAME
    #     )
    # fi
    info "编译成功, 执行文件生成在[$BIN_PATH]"
    md5sum "$BIN_PATH/$MAKE_NAME"
else
    info "============> 编译整个系统: $MAKE_NAME <============"
    # 调用函数
    check_build

    # 判断是否需要重新 cmake
    if [ "$LAST_BUILD_VERSION" != "$BUILD_TYPE" ] ||
        [ "$LAST_BUILD_DEVICE_TYPE" != "$BUILD_DEVICE_TYPE" ]; then
        trace "重新 cmake"

        cd "$SRC_ROOT_PATH"
        trace "进入源码目录$(pwd)"

        if [ -e "${CMAKE_TEMP_PATH}/CMakeCache.txt" ]; then
            trace "删除cmake缓存变量"
            rm -v "${CMAKE_TEMP_PATH}/CMakeCache.txt"
        fi

        #cmake 输出文件到output
        trace "cmake命令参数 [${BUILD_TYPE} ${BUILD_DEVICE_TYPE} ${EXTRA_PARAM} . -B${CMAKE_TEMP_PATH} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON]"
        if ! cmake "${BUILD_TYPE}" "${BUILD_DEVICE_TYPE}" "${EXTRA_PARAM}" . -B"${CMAKE_TEMP_PATH}" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON; then
            error "cmake执行出现错误"
            exit 1
        fi

        # 强制删除可能存在的旧链接或文件（-f 选项保证文件不存在时不报错）
        rm -f "${PROJECT_ROOT_PATH}/compile_commands.json"
        # 软链接至项目根目录
        ln -sv "${CMAKE_TEMP_PATH}/compile_commands.json" "${PROJECT_ROOT_PATH}/"

        cp "${AUTO_FILE}" "${CMAKE_TEMP_PATH}"
    else
        trace "不执行cmake"
    fi

    # make 程序
    cd "$CMAKE_TEMP_PATH"
    trace "进入输出目录 [$CMAKE_TEMP_PATH]"
    #编译所有目标
    if ! make -j6; then
        error "编译所有程序失败"
        exit 1
    fi
    # 提取.debug调试信息
    # if [ "$BUILD_MODE" == "Debug" ]; then
    #     (
    #         cd $BIN_PATH
    #         /opt/hisi-linux/x86-arm/arm-v01c02-linux-musleabi-gcc/bin/arm-v01c02-linux-musleabi-objcopy --only-keep-debug $MAKE_NAME $MAKE_NAME.debug
    #         /opt/hisi-linux/x86-arm/arm-v01c02-linux-musleabi-gcc/bin/arm-v01c02-linux-musleabi-strip --strip-unneeded $MAKE_NAME
    #         /opt/hisi-linux/x86-arm/arm-v01c02-linux-musleabi-gcc/bin/arm-v01c02-linux-musleabi-objcopy --add-gnu-debuglink=$MAKE_NAME.debug $MAKE_NAME
    #     )
    # fi
    info "编译成功, 执行文件生成在[$BIN_PATH]"
    md5sum "$BIN_PATH"/*
fi

# 自动摆渡
if [ "$AUTOFILE_MODE" = true ]; then
    info "============> 自动摆渡执行程序 <============"
    (
        export BL_DEVICE=$DEVICE_TYPE
        cp -a "${CUR_PATH}/compile.sh" "$CMAKE_TEMP_PATH/compile.sh"
        cd "$CMAKE_TEMP_PATH"
        if [ "$MAKE_MODE" = true ]; then
            autofile "$MAKE_NAME"
        else
            # 遍历BIN_PATH目录下的所有文件并执行autofile
            for file in "$BIN_PATH"/*; do
                # 检查是否为文件（非目录）
                if [ -f "$file" ]; then
                    # 提取文件名（去除路径部分）
                    filename=$(basename "$file")
                    autofile "$filename"
                fi
            done
        fi
    )
fi

sync
exit 0
