#!/bin/bash
###
# @FilePath     : make_image.sh
# @Author       : 
# @Date         : 
 # @LastEditTime: 2026-04-24 10:08:49
# @Description  : 
###

set -euo pipefail

# 预定义可能被系统脚本引用但未设置的变量（兼容 set -u）
export LC_IDENTIFICATION=""
export LC_BYOBU=""
export LC_TERMTYPE=""

# 安全获取当前脚本路径（兼容 shc 和无 realpath 系统）
if [[ -z "$0" ]] || [[ ! -f "$0" ]]; then
    echo "❌ 无法确定脚本路径（\$0 无效）" >&2
    exit 1
fi
if command -v realpath >/dev/null; then
    CUR_PATH=$(dirname "$(realpath "$0")")
else
    CUR_PATH=$(cd -P -- "$(dirname -- "$0")" && pwd -P)
fi

# 日志函数
error() { echo "❌ $*" >&2; exit 1; }
info() { echo "ℹ️ $*" >&2; }

# 加载配置
source "${CUR_PATH}/../path.conf" || error "无法加载 path.conf"
source /etc/profile

# 校验 PROJECT_ROOT_PATH
if [[ -z "${PROJECT_ROOT_PATH+x}" ]]; then
    error "PROJECT_ROOT_PATH 未定义，请检查 ${CUR_PATH}/../path.conf"
fi
if [[ -z "$PROJECT_ROOT_PATH" ]]; then
    error "PROJECT_ROOT_PATH 为空"
fi
if [[ ! "$PROJECT_ROOT_PATH" = /* ]]; then
    error "PROJECT_ROOT_PATH 必须是绝对路径: $PROJECT_ROOT_PATH"
fi
if [[ ! -d "$PROJECT_ROOT_PATH" ]]; then
    error "PROJECT_ROOT_PATH 不是有效目录: $PROJECT_ROOT_PATH"
fi

# 解析参数
if ! ARGS=$(getopt -o d:v:c: --long version:,autofile -n "$0" -- "$@"); then
    error "选项和参数解析失败"
fi
eval set -- "$ARGS"

DEVICE_TYPE=""
CHIP_TYPE="RV1126B"
VERSION_NUM=""
AUTOFILE_MODE=false

while true; do
    case "$1" in
        -d) DEVICE_TYPE="$2"; shift 2 ;;
        -c) CHIP_TYPE="$2"; shift 2 ;;
        -v|--version) VERSION_NUM="$2"; shift 2 ;;
        --autofile) AUTOFILE_MODE=true; shift ;;
        --) shift; break ;;
        *) error "未知选项: $1" ;;
    esac
done

check_device_type "$DEVICE_TYPE"

info "项目根路径: $PROJECT_ROOT_PATH"

FIRMWARE_BASE_PATH=""
FIRMWARE_3881T_BASE_PATH="${PROJECT_ROOT_PATH}/rv1126b_1t_firmware"
FIRMWARE_3882TI_BASE_PATH="${PROJECT_ROOT_PATH}/rv1126b_2ti_firmware"

# === 根据设备类型设置 PROJECT_MK ===
case "$DEVICE_TYPE" in
    TV-3882TI)
        FIRMWARE_BASE_PATH="$FIRMWARE_3882TI_BASE_PATH"
        ;;
    TV-3881T)
        FIRMWARE_BASE_PATH="$FIRMWARE_3881T_BASE_PATH"
        ;;
    *)
        error "不支持的设备类型: $DEVICE_TYPE（仅支持 TV-3882TI / TV-3881T）"
        ;;
esac

# 添加打印语句显示当前设置的路径
echo "设备类型: $DEVICE_TYPE -> FIRMWARE_BASE_PATH = $FIRMWARE_BASE_PATH"

# ==============================
# 路径定义
# ==============================
RUN_PATH="/opt/cam"
LIB_PATH="${PROJECT_ROOT_PATH}/ipc_platform/lib"
THIRD_PARTY_PATH="${PROJECT_ROOT_PATH}/ipc_platform/third-party"
UPGRADE_PATH="${PROJECT_ROOT_PATH}/output/packet/upgradefile"
OUTPUT_PATH="${PROJECT_ROOT_PATH}/output/image"

SOFTWARE_TYPE="固件包"
SOFTWARE_NAME="${DEVICE_TYPE}-${CHIP_TYPE}-${VERSION_NUM}-${SOFTWARE_TYPE}"

STRIP_TOOL="/opt/rk-linux/linux-x86/rv1126b/arm-rockchip1240-linux-gnueabihf/bin/arm-rockchip1240-linux-gnueabihf-strip"
FIRMWARE_OUTPUT_PATH="$FIRMWARE_BASE_PATH/image"
ROOTFS_PATH="$FIRMWARE_BASE_PATH/image/rootfs_glibc_rv1126b"

# 拷贝依赖库并 strip
copy_third_party_libs "$LIB_PATH" "${UPGRADE_PATH}${RUN_PATH}/lib"
if compgen -G "${UPGRADE_PATH}${RUN_PATH}/lib/*" > /dev/null 2>&1; then
    "$STRIP_TOOL" --strip-all "${UPGRADE_PATH}${RUN_PATH}/lib/"*
fi

# 拷贝插件和字体
cp -a "${THIRD_PARTY_PATH}/ttf" "${UPGRADE_PATH}/${RUN_PATH}/third-party/"
cp -a "${THIRD_PARTY_PATH}/IpcComponents-V"* "${UPGRADE_PATH}${RUN_PATH}/third-party/"

mkdir -p firmware

# ==============================
# 编译固件（直接执行，无需 expect/su）
# ==============================
info "开始编译固件"

# 进入文件系统目录
cd "$FIRMWARE_BASE_PATH/image"

# 删除整个 rootfs_glibc_rv1126b 文件夹
info "删除临时 rootfs 文件夹..."
rm -rf "rootfs_glibc_rv1126b"

# 解压裸文件系统
info "解压裸文件系统..."
tar -xzvf rootfs_glibc_rv1126b.tar.gz

# 合并到 rootfs
info "合并升级内容到 rootfs..."
cp -af "$UPGRADE_PATH"/* "$ROOTFS_PATH"/

# 回到固件目录
cd "$FIRMWARE_BASE_PATH"

# 生成新固件
info "生成新固件..."
./build.sh

# ==============================
# 输出校验与打包
# ==============================
if [[ ! -f "$FIRMWARE_OUTPUT_PATH/update.img" ]]; then
    error "固件生成失败：$FIRMWARE_OUTPUT_PATH/update.img 不存在"
fi

# 复制并打包
mkdir -p "$OUTPUT_PATH/firmware"
cp -f "$FIRMWARE_OUTPUT_PATH/update.img" "$OUTPUT_PATH/firmware/"

(
    cd "$OUTPUT_PATH" || exit 1
    zip -r "${SOFTWARE_NAME}.zip" firmware -x "*/\.*"
)

# 自动摆渡
if [[ "$AUTOFILE_MODE" == true ]]; then
    info "============> 自动摆渡固件包 <============"
    if ! command -v autofile &> /dev/null; then
        error "autofile 命令未找到"
    fi

    (
        export BL_DEVICE="$DEVICE_TYPE"
        cp -a "${CUR_PATH}/../compile.sh" "$OUTPUT_PATH/compile.sh"
        cd "$OUTPUT_PATH" || exit 1
        cp "${SOFTWARE_NAME}.zip" "${PROJECT_ROOT_PATH}/output/bin/"

        autofile "${SOFTWARE_NAME}.zip"

        TARGET_FILE="${PROJECT_ROOT_PATH}/output/bin/${SOFTWARE_NAME}.zip"
        if [[ -e "$TARGET_FILE" ]]; then
            echo "ℹ️ 删除摆渡后残留包: $TARGET_FILE"
            rm -f "$TARGET_FILE"
        fi
    )
    if [ $? -ne 0 ]; then
        error "自动摆渡失败"
    fi
fi

info "✅ 固件打包完成: ${SOFTWARE_NAME}.zip"
exit 0