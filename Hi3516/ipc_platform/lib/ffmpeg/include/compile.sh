#!/bin/bash

# 检查是否需要重新编译
../check_rebuild.sh $1
if [ $? -eq 0 ]; then
    echo "无需重新编译"
    exit 0
fi

rm -rf ./build/
mkdir ./build
rm -rvf ./ffmpeg-4.3.2/
tar -C ./ -xvf ./ffmpeg-4.3.2.tar.xz
cd ffmpeg-4.3.2

# 清除旧Makefile
if find . -name "Makefile" -print -quit | grep -q "Makefile"; then
    make distclean -j8
    echo "清除旧MakeFile成功"
fi

# 依赖库编译
cd ../../libx264/
./compile.sh $1
echo "编译x264成功"
cd -

cd ../../libx265/
./compile.sh $1
echo "编译依赖的x265库文件成功"
cd -

cd ../../libfdk-aac/
./compile.sh $1
echo "编译fdk-aac成功"
cd -

# 默认平台为linux
TARGET=linux

# 设置PKG_CONFIG_PATH
export PKG_CONFIG_PATH=${PWD}/../../libx264/build/lib/pkgconfig:$PKG_CONFIG_PATH
export PKG_CONFIG_PATH=${PWD}/../../libx265/build/lib/pkgconfig:$PKG_CONFIG_PATH
export PKG_CONFIG_PATH=${PWD}/../../libfdk-aac/build/lib/pkgconfig:$PKG_CONFIG_PATH

# 判断交叉编译参数
if [ -n "$1" ]; then
    HOST=$1
    if [[ $HOST == *"/"* ]]; then
        HOST=${HOST##*/}
    fi
    PREFIX=$HOST
    HOST=${HOST%?}
    TARGET=$HOST
    ARCH="${HOST%%-*}"
fi

# 交叉编译参数拼接
CROSS_OPTIONS+=" --cross-prefix=$1 "
CROSS_OPTIONS+=" --enable-cross-compile "
CROSS_OPTIONS+=" --arch=$ARCH "
CROSS_OPTIONS+=" --target-os=linux "
CROSS_OPTIONS+=" --pkg-config=pkg-config "
CROSS_OPTIONS+=" --pkg-config-flags=--static "

CROSS_OPTIONS+=" --enable-libx264 "
CROSS_OPTIONS+=" --extra-cflags=-I${PWD}/../../libx264/build/include/ "
CROSS_OPTIONS+=" --extra-ldflags=-L${PWD}/../../libx264/build/lib/ "
CROSS_OPTIONS+=" --enable-libx265 "
CROSS_OPTIONS+=" --extra-cflags=-I${PWD}/../../libx265/build/include/ "
CROSS_OPTIONS+=" --extra-ldflags=-L${PWD}/../../libx265/build/lib/ "
CROSS_OPTIONS+=" --enable-libfdk-aac "
CROSS_OPTIONS+=" --extra-cflags=-I${PWD}/../../libfdk-aac/build/include/ "
CROSS_OPTIONS+=" --extra-ldflags=-L${PWD}/../../libfdk-aac/build/lib/ "

# 最小化功能配置
CROSS_OPTIONS+=" --disable-everything "
CROSS_OPTIONS+=" --enable-static "
CROSS_OPTIONS+=" --disable-shared "
CROSS_OPTIONS+=" --enable-gpl "
CROSS_OPTIONS+=" --enable-nonfree "
CROSS_OPTIONS+=" --enable-muxer=mpegts "
CROSS_OPTIONS+=" --enable-protocol=file "
CROSS_OPTIONS+=" --enable-avformat "
CROSS_OPTIONS+=" --enable-avcodec "
CROSS_OPTIONS+=" --enable-avutil "
CROSS_OPTIONS+=" --enable-encoder=libx264 "
CROSS_OPTIONS+=" --enable-encoder=libx265 "
CROSS_OPTIONS+=" --enable-encoder=libfdk_aac "
CROSS_OPTIONS+=" --enable-parser=h264 "
CROSS_OPTIONS+=" --enable-parser=hevc "
CROSS_OPTIONS+=" --enable-parser=aac "
CROSS_OPTIONS+=" --disable-doc "
CROSS_OPTIONS+=" --disable-debug "

# 执行configure
set -e
./configure \
    --prefix=${PWD}/../build \
    --extra-cflags=-Wno-attributes \
    $CROSS_OPTIONS

# 编译 & 安装（只安装库文件、头文件、工具）
make clean -j8
make -j8
make install-headers -j8
make install-libs -j8
make install-progs -j8

# 保存平台信息
echo $TARGET > ../platform
