#!/bin/bash

# 获取配置文件所在目录
CUR_PATH=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
echo ${CUR_PATH}
#导入环境配置
source ${CUR_PATH}/path.conf
source ${CUR_PATH}/help.conf

# 命令失败时立即退出
set -e

# 使用getopt来处理长选项
# :前面的需要带参数, 没有的不需要带参数
if ! ARGS=$(getopt -o m:b:d:e:v:s:t:cahpi -l target:,build:,device:,extra:,project:,version:,clean,all,help,strip,packet,img,autofile -n "$0" -- "$@"); then
    error "选项和参数解析失败"
    exit 1
fi
eval set -- "$ARGS"

# 单个编译
MAKE_MODE=false
# 要单个编译名称
MAKE_NAME=""
# 全部编译
MAKE_ALL_MODE=false
# 帮助
HELP_MODE=false
# 清空
CLEAN_MODE=false
# 打升级包
PACKET_MODE=false
# 打升级包类型all,app或model
PACKET_TYPE="all"
# 打固件包
IMAGE_MODE=false
# 编译模式
BUILD_MODE="Release"
# 额外参数
EXTRA_PARAM=""
# 软件版本
VERSION_NUM=""
# 增加型号定义的cmake参数
BUILD_DEVICE_TYPE="-DDEVICE_TYPE=RV1126B_IPC"
# 是否设置了型号
DEVICE_MODE=false
# 型号
# DEVICE_TYPE=""
# 裁剪bin文件
STRIP_MODE=false
# 是否自动摆渡
AUTOFILE_MODE=false

# 解析选项
while true; do
    case "$1" in
    -m | --target) # -m 单个编译
        MAKE_MODE=true
        MAKE_NAME="$2"
        shift 2 # 移除命令行参数
        ;;
    -b | --build) # -b 编译模式
        BUILD_MODE="$2"
        shift 2
        ;;
    -d | --device) # -d 设置型号
        DEVICE_MODE=true
        DEVICE_TYPE="$2"
        shift 2
        ;;
    -e | --extra) # -e 额外参数
        EXTRA_PARAM="$2"
        shift 2
        ;;
    -v | --version) # 设置软件版本
        VERSION_NUM="$2"
        shift 2
        ;;
    -t)             # 升级包类型如app，model
        PACKET_TYPE="$2"
        shift 2
        ;;
    --project) # --project 设置项目类型
        PROJECT_MODE=true
        PROJECT_TYPE="$2"
        shift 2
        ;;
    -c | --clean) # -c 清空模式
        CLEAN_MODE=true
        shift
        ;;
    -s) # -s 裁剪bin文件
        STRIP_MODE=true
        shift
        ;;
    -p | --packet) # -p 打包升级包
        PACKET_MODE=true
        shift
        ;;
    -i | --img) # -i 打固件包
        IMAGE_MODE=true
        shift
        ;;
    -a | --all) # -a 编译全部程序
        MAKE_ALL_MODE=true
        shift
        ;;
    -h | --help)
        HELP_MODE=true
        shift
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

# 帮助说明
if [ "$HELP_MODE" = true ]; then
    display_help
    exit 0
fi

# 删除临时文件
if [ "$CLEAN_MODE" = true ]; then
    ${CUR_PATH}/make_process.sh -c
    # 删除本地型号文件
    rm -rf $LOCAL_DEVICE_FILE
    info "成功清除所有临时文件"
    exit 0
fi

# 生成型号
if [ "$DEVICE_MODE" = true ]; then
    set_device_type $DEVICE_TYPE
    exit 0
fi

# 裁剪bin文件的调试信息
if [ "$STRIP_MODE" = true ]; then
    info "裁剪bin文件的调试信息" $LINENO
    # 直接传入型号参数
    ./make_strip.sh $DEVICE_TYPE
    exit 0
fi

# 判断是否存在本地型号文件
check_device_file

# 检查编译类型是否有效的函数
check_buile_type $BUILD_MODE

# 打包升级包
if [ "$PACKET_MODE" = true ]; then
    info "============> 打包升级包前，先生成版本号 <============"
    # set_version
    (
        cd $BUILD_ROOT_PATH/pack/
        info "============> 打包升级包 <============"
        make_packet_args=(--version "$VERSION_NUM" -d "$DEVICE_TYPE" -b "$BUILD_MODE" -t "$PACKET_TYPE" --project "$PROJECT_TYPE")

        if [ "$AUTOFILE_MODE" = true ]; then
            make_packet_args+=(--autofile)
        fi
        echo "${make_packet_args[@]}"
        if ! ./make_packet.sh "${make_packet_args[@]}"; then
            error "升级包编译失败"
            exit 1
        fi
        info "打包升级包成功, 执行文件生成在[$OUTPUT_DIR_NAME/packet]"
    )
    exit 0
fi

# 打包固件
if [ "$IMAGE_MODE" = true ]; then

    info "============> 编译升级包前，先生成版本号 <============"
    # set_version
    (
        cd $BUILD_ROOT_PATH/pack/
        info "============> 打包固件前，先编译升级包 <============"
        make_packet_args=(--version "$VERSION_NUM" -d "$DEVICE_TYPE" -b "$BUILD_MODE" -t "$PACKET_TYPE" --project "$PROJECT_TYPE")
        # 固件包打包模型文件
        make_packet_args+=(--copy-model)
        if [ "$AUTOFILE_MODE" = true ]; then
            make_packet_args+=(--autofile)
        fi
        echo "${make_packet_args[@]}"
        if ! ./make_packet.sh "${make_packet_args[@]}"; then
            error "升级包编译失败"
            exit 1
        fi
        info "编译升级包成功, 执行文件生成在[$OUTPUT_DIR_NAME/packet]"
    )

    info "============> 打包固件 <============"
    (
        cd $BUILD_ROOT_PATH/image/
        make_image_args=(--version "$VERSION_NUM" -d "$DEVICE_TYPE")
        if [ "$AUTOFILE_MODE" = true ]; then
            make_image_args+=(--autofile)
        fi
        echo "${make_image_args[@]}"
        if ! ./make_image.sh "${make_image_args[@]}"; then
            error "打包固件失败"
            exit 1
        fi
        info "打包固件成功, 执行文件生成在[$OUTPUT_DIR_NAME/image]"
    )
    exit 0
fi

make_process_args=(-b "$BUILD_MODE" -e "$EXTRA_PARAM")
if [ "$AUTOFILE_MODE" = true ]; then
    make_process_args+=(--autofile)
fi
echo "${make_process_args[@]}"

# 编译程序
if [ "$MAKE_MODE" = true ]; then
    info "============> 程序编译 <============" $LINENO
    # set_version
    make_process_args+=(-m "$MAKE_NAME")
    if ! ${CUR_PATH}/make_process.sh "${make_process_args[@]}"; then
        error "程序编译失败" $LINENO
        exit 1
    fi

    #裁剪掉调试信息
    #./make_strip.sh

    exit 0
fi

# 编译全部程序
if [ "$MAKE_ALL_MODE" = true ]; then
    info "============> 程序全部编译 <============" $LINENO
    # set_version
    if ! ${CUR_PATH}/make_process.sh "${make_process_args[@]}"; then
        error "程序编译失败" $LINENO
        exit 1
    fi

    #裁剪掉调试信息
    #./make_strip.sh
    exit 0
fi

error "参数处理未定义 $0 $*"
exit 1
