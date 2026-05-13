#!/bin/bash

set -e

# 配置参数
CHIP="hi3516cv610"
PAGE="2k"
BLOCK="128k"
ROOTFS="rootfs_debug_musl_arm/"
SIZE="120M"
TOOLS="bin/pc/"
OPT="0"

# 目标文件和目录
TARGET_DIR="hi3516cv610_image_debug_musl"
UBIFS_FILE="rootfs_hi3516cv610_2k_128k_120M.ubifs"

# 工具路径
MKUBI="./mkubiimg.sh"

# 颜色定义
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

log() { echo -e "${GREEN}[+]${NC} $*"; }
err() { echo -e "${RED}[!]${NC} $*" >&2; }

# 主函数
main() {
    log "开始 Hi3516CV610 打包..."
    
    # 检查工具
    [[ -f "$MKUBI" ]] || { err "文件不存在: $MKUBI"; exit 1; }
    [[ -x "$MKUBI" ]] || chmod +x "$MKUBI"
    
    # 检查目录
    [[ -d "$ROOTFS" ]] || { err "目录不存在: $ROOTFS"; exit 1; }
    [[ -d "$TOOLS" ]] || { err "目录不存在: $TOOLS"; exit 1; }
    
    # 步骤1: 删除旧文件
    TARGET_FILE="${TARGET_DIR}/${UBIFS_FILE}"
    if [[ -f "$TARGET_FILE" ]]; then
        log "删除旧文件: $TARGET_FILE"
        rm -f "$TARGET_FILE"
    fi
    
    # 确保目标目录存在
    mkdir -p "$TARGET_DIR"
    
    # 步骤2: 创建EXT4文件系统镜像
    log "创建UBIFS文件系统镜像..."
    log "执行: $MKUBI $CHIP $PAGE $BLOCK $ROOTFS $SIZE $TOOLS $OPT"
    "$MKUBI" "$CHIP" "$PAGE" "$BLOCK" "$ROOTFS" "$SIZE" "$TOOLS" "$OPT"
    
    # 步骤3: 拷贝新文件
    log "移动新文件: $UBIFS_FILE -> $TARGET_FILE"
    mv "$UBIFS_FILE" "$TARGET_FILE"
    log "打包完成!"
}

main "$@"
