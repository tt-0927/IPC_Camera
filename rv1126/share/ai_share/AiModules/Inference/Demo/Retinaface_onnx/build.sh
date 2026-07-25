#!/bin/bash

# 1、配置系统环境
## export BL_DEVICE=TS-0663C
# 2、编译脚本 autofile 编译文件名 云桌面账号
## autofile Yolov8 yanzh

# 使用getopt来处理长选项
# :前面的需要带参数, 没有的不需要带参数
ARGS=$(getopt -o m: -n "$0" -- "$@")
if [ $? -ne 0 ]; then
    error "选项和参数解析失败" $LINENO
    exit -1
fi
eval set -- "$ARGS"

# 解析选项
while true; do
    case "$1" in
        -m) # -m 单个编译
            MAKE_NAME="$2"
            shift 2 # 移除命令行参数
            ;;
        --)
            shift
            break
            ;;
        *)
            echo "选项和参数未定义处理：$ARGS"
            exit -1
            ;;
    esac
done


# cmake .
cmake . -DUSE_CUDA=ON

if [ $? -ne 0 ]; then
    echo "编译 $MAKE_NAME 失败"
    exit -1
fi

make $MAKE_NAME -j32
if [ $? -ne 0 ]; then
    echo "编译 $MAKE_NAME 失败"
    exit -1
fi

exit 0
