
## WebSocket
1. webSocket库使用模块


## 编译libwebsockets库脚本

```Bash
#!/bin/bash

# 编译链前缀
CROSS_PREFIX=aarch64-none-linux-gnu-

# libwebsockets版本
LIBWEBSOCKETS_VERSION=4.3.0

# 安装路径
INSTALL_PATH=/home/yanzh/Tools/build/websockets_install

# openssl路径
OPENSSL_PATH=/home/yanzh/Tools/build/openssl_install

# 自定义工具链文件
TOOLCHAIN_FILE=/home/yanzh/Tools/Toolchain-aarch64-linux-gnu.cmake

# 下载源码
if [ ! -f libwebsockets-$LIBWEBSOCKETS_VERSION.tar.gz ]; then
  wget https://github.com/warmcat/libwebsockets/archive/v$LIBWEBSOCKETS_VERSION.tar.gz
  if [ $? -ne 0 ]; then
      echo "下载源码失败"
      exit -1
  fi
  echo "下载源码成功"
else
  echo "源码已存在，跳过下载"
fi

# 解压缩
if [ ! -d libwebsockets-$LIBWEBSOCKETS_VERSION.tar.gz ]; then
  tar xf libwebsockets-$LIBWEBSOCKETS_VERSION.tar.gz
  if [ $? -ne 0 ]; then
      echo "解压缩源码失败"
      exit -1
  fi
  echo "解压缩源码成功"
fi

# 进入目录
cd libwebsockets-$LIBWEBSOCKETS_VERSION
if [ $? -ne 0 ]; then
  echo "进入libwebsockets-$LIBWEBSOCKETS_VERSION 目录失败"
  exit -1
fi

# 配置交叉编译选项
cmake . -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE} \
      -DCMAKE_INSTALL_PREFIX=${INSTALL_PATH} \
      -DLWS_WITH_HTTP2=1 \
      -DLWS_OPENSSL_INCLUDE_DIRS=${OPENSSL_PATH}/include \
      -DLWS_OPENSSL_LIBRARIES="${OPENSSL_PATH}/lib64/libssl.so;${OPENSSL_PATH}/lib64/libcrypto.so" \
      -DLWS_WITH_SSL=ON \
      -DLWS_WITH_SHARED=OFF
if [ $? -ne 0 ]; then
  echo "配置交叉编译选项失败"
  exit -1
fi
echo "配置交叉编译选项成功"

# 编译和安装
make -j$(nproc)
if [ $? -ne 0 ]; then
  echo "编译失败"
  exit -1
fi
echo "编译成功"

make install
if [ $? -ne 0 ]; then
  echo "安装失败"
  exit -1
fi
echo "安装成功"

```

### 自定义工具链文件 Toolchain-aarch64-linux-gnu.cmake

```
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
SET(CMAKE_C_COMPILER "aarch64-none-linux-gnu-gcc")
SET(CMAKE_CXX_COMPILER "aarch64-none-linux-gnu-g++")
SET(CMAKE_AR "aarch64-none-linux-gnu-ar")
SET(CMAKE_LINKER "aarch64-none-linux-gnu-ld")
```


## 编译OpenSSL库脚本

```Bash
#!/bin/bash

# 编译链前缀
CROSS_PREFIX=aarch64-none-linux-gnu-

# 指定目标主机的类型
HOST_PARAM=aarch64-none-linux-gnu
TARGET_PARAM=aarch64-none-linux-gnu

# OpenSSL版本
OPENSSL_VERSION=openssl-3.1.2

# 安装路径
INSTALL_PATH=/home/yanzh/Tools/build/openssl_install

# 下载源码
if [ ! -f $OPENSSL_VERSION.tar.gz ]; then
  wget https://www.openssl.org/source/${OPENSSL_VERSION}.tar.gz
  if [ $? -ne 0 ]; then
      echo "下载源码失败"
      exit -1
  fi
  echo "下载源码成功"
else
  echo "源码已存在，跳过下载"
fi

# 解压缩
if [ ! -d $OPENSSL_VERSION ]; then
  tar xf $OPENSSL_VERSION.tar.gz
  if [ $? -ne 0 ]; then
      echo "解压缩源码失败"
      exit -1
  fi
  echo "解压缩源码成功"
fi

# 进入目录
cd $OPENSSL_VERSION
if [ $? -ne 0 ]; then
  echo "进入$OPENSSL_VERSION 目录失败"
  exit -1
fi

# 配置交叉编译选项
./Configure no-asm --prefix=${INSTALL_PATH} --cross-compile-prefix=${CROSS_PREFIX}
if [ $? -ne 0 ]; then
  echo "配置交叉编译选项失败"
  exit -1
fi
echo "配置交叉编译选项成功"

# 编译和安装
make -j$(nproc)
if [ $? -ne 0 ]; then
  echo "编译失败"
  exit -1
fi
echo "编译成功"

make install
if [ $? -ne 0 ]; then
  echo "安装失败"
  exit -1
fi
echo "安装成功"

```

### 注意
1. 报错aarch64-linux-gcc.br_real: error: unrecognized command line option '-m64'
   1. 修改 Makefile 文件，将 -m64 移除

## 说明
1. https://github.com/warmcat/libwebsockets.git