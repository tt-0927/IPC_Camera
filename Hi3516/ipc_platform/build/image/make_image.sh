#!/bin/bash
###
 # @FilePath     : make_image.sh
# @Author       : zhouzr@kfb.cn
# @Date         : 2026-01-21 10:36:39
 # @LastEditors  : zhouzr@kfb.cn
 # @LastEditTime : 2026-07-28 13:35:54
# @Description  : 打包固件脚本
###

set -euo pipefail

# 兼容 set -u
export LC_IDENTIFICATION=""
export LC_BYOBU=""
export LC_TERMTYPE=""

# 获取脚本目录
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
error() {
    echo "❌ $*" >&2
    exit 1
}
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
if ! ARGS=$(getopt -o d:v:c:s: --long version:,project:,autofile,copyright -n "$0" -- "$@"); then
    error "选项和参数解析失败"
fi
eval set -- "$ARGS"

# 设备型号
DEVICE_TYPE=""
# 镜头型号
SENSOR_TYPE=""
# 项目类型
PROJECT_TYPE=""
# 语言类型 中文、英文、中英文通用
LANGUAGE_TYPE="中文"
# 芯片型号
CHIP_TYPE="Hi3516CV610-20S"
# 版本号
VERSION_NUM=""
# 是否自动摆渡
AUTOFILE_MODE=false
# 是否启用软著打包模式
COPYRIGHT_MODE=false

while true; do
    case "$1" in
    -d)
        DEVICE_TYPE="$2"
        shift 2
        ;;
    -s)
        SENSOR_TYPE="$2"
        shift 2
        ;;
    --project)
        PROJECT_TYPE="$2"
        shift 2
        ;;
    -c)
        CHIP_TYPE="$2"
        shift 2
        ;;
    -v | --version)
        VERSION_NUM="$2"
        shift 2
        ;;
    --autofile)
        AUTOFILE_MODE=true
        shift
        ;;
    --copyright)
        COPYRIGHT_MODE=true
        shift
        ;;
    --)
        shift
        break
        ;;
    *) error "未知选项: $1" ;;
    esac
done

check_device_type "$DEVICE_TYPE" || error "设备型号无效: $DEVICE_TYPE"
check_sensor_type "$SENSOR_TYPE" || error "镜头型号无效: $SENSOR_TYPE"
check_project_type "$PROJECT_TYPE" || error "项目类型无效: $PROJECT_TYPE"

# 3852TLW 和 3852TL4G 使用 Hi3516CV610-00S 主控，其他型号使用 Hi3516CV610-20S
if [[ "$DEVICE_TYPE" =~ ^TV-3852(TLW|TL4G)$ ]]; then
    CHIP_TYPE="Hi3516CV610-00S"
    info "当前型号 $DEVICE_TYPE 使用主控: $CHIP_TYPE"
fi

info "项目根路径: $PROJECT_ROOT_PATH"

# ==============================
# 路径定义
# ==============================
RUN_PATH="/opt/cam"
DHCP_PATH="${PROJECT_ROOT_PATH}/ipc_platform/environment/system/usr/share/udhcpc"
LIB_PATH="${PROJECT_ROOT_PATH}/ipc_platform/lib"
THIRD_PARTY_PATH="${PROJECT_ROOT_PATH}/ipc_platform/third-party"
MODEL_PATH="${PROJECT_ROOT_PATH}/ipc_platform/environment/model"
UPGRADE_PATH="${PROJECT_ROOT_PATH}/output/packet/upgradefile"
OUTPUT_PATH="${PROJECT_ROOT_PATH}/output/image"

# ==============================
# 函数：根据镜头型号生成包名中的镜头标识
# 参数：$1 - SENSOR_TYPE (如 sc500ai-f4mm, sc533hai-f4mm)
# 返回：设置全局变量 SENSOR_NAME_FOR_PACKAGE
# ==============================
generate_sensor_name_for_package() {
    local sensor_type=$1

    # 检查是否包含 533hai
    if [[ "$sensor_type" == *"533hai"* ]]; then
        # 提取焦距部分（如 f4mm 或 f2_8mm）
        # 支持格式：sc533hai-f4mm 或 sc533hai_f4mm 或 sc533hai f4mm
        if [[ "$sensor_type" =~ (f[0-9_]+mm) ]]; then
            SENSOR_NAME_FOR_PACKAGE="${BASH_REMATCH[1]}"
            info "镜头型号为533hai，包名中使用焦距: ${SENSOR_NAME_FOR_PACKAGE}"
        else
            # 如果没有匹配到焦距，使用原始值
            SENSOR_NAME_FOR_PACKAGE="$sensor_type"
            info "未匹配到焦距，使用原始镜头型号: ${SENSOR_NAME_FOR_PACKAGE}"
        fi
    else
        # 其他情况（如 500ai），使用完整的镜头型号
        SENSOR_NAME_FOR_PACKAGE="$sensor_type"
        info "镜头型号为500ai或其他，使用完整型号: ${SENSOR_NAME_FOR_PACKAGE}"
    fi
}

SOFTWARE_TYPE="固件包"

# 生成包名中使用的镜头标识
generate_sensor_name_for_package "$SENSOR_TYPE"

# 软件包名称格式：型号-芯片型号-镜头标识-版本号-[中英文类型]-软件包类型-项目类型
SOFTWARE_NAME="${DEVICE_TYPE}-${CHIP_TYPE}-${SENSOR_NAME_FOR_PACKAGE}-${VERSION_NUM}-${LANGUAGE_TYPE}-${SOFTWARE_TYPE}-${PROJECT_TYPE}"

# 软著模式：用软著前缀包裹文件名
if [[ "$COPYRIGHT_MODE" == true ]]; then
    SOFTWARE_NAME="${COPYRIGHT_PREFIX}(${SOFTWARE_NAME})"
fi

STRIP_TOOL="/opt/hisi-linux/x86-arm/arm-v01c02-linux-musleabi-gcc/bin/arm-v01c02-linux-musleabi-strip"

FIRMWARE_BASE_PATH="${PROJECT_ROOT_PATH}/hi3516cv610"
ROOTFS_PATH="$FIRMWARE_BASE_PATH/rootfs_debug_musl_arm"

# rootfs 子路径
ROOTFS_UDHCPC="$ROOTFS_PATH/usr/share/udhcpc"

# ==============================
# 函数：生成镜头信息文件
# 说明：仅写入烧录固件的 rootfs，升级包不得携带该文件，防止覆盖设备出厂镜头信息
# ==============================
generate_lens_info_file() {
    local sensor_type="$SENSOR_TYPE"
    local focal_length=""
    local lens_info_file="${ROOTFS_PATH}${RUN_PATH}/lens_info.txt"

    # 提取焦距部分（如 f4mm 或 f2_8mm）。
    if [[ "$sensor_type" =~ (f[0-9_]+mm) ]]; then
        local focal_part="${BASH_REMATCH[1]}"
        focal_length="${focal_part#f}"
        focal_length="${focal_length//_/.}"
    fi

    mkdir -p "$(dirname "$lens_info_file")"
    printf 'SENSOR_TYPE=%s\n' "$sensor_type" >"$lens_info_file"
    if [[ -n "$focal_length" ]]; then
        printf 'FOCAL_LENGTH=%s\n' "$focal_length" >>"$lens_info_file"
    fi

    info "镜头信息已写入烧录固件: ${lens_info_file}"
}

# ==============================
# 函数：拷贝AI模型
# 说明：拷贝固件所需AI模型，仅垃圾站型号拷贝垃圾检测模型
# ==============================
copy_model_files() {
    info "拷贝AI模型"

    if [[ ! -d "$MODEL_PATH" ]]; then
        error "模型目录不存在: $MODEL_PATH"
    fi

    mkdir -p "${UPGRADE_PATH}${RUN_PATH}/model"

    local filename
    local model_file
    for model_file in "${MODEL_PATH}"/*; do
        if [[ -f "$model_file" ]]; then
            filename=$(basename "$model_file")
            if [[ "$filename" =~ rubish ]]; then
                info "跳过垃圾检测模型: $filename"
                continue
            fi
            if [[ "$filename" =~ mobileface ]]; then
                info "跳过特征点提取模型: $filename"
                continue
            fi
            cp -a "$model_file" "${UPGRADE_PATH}${RUN_PATH}/model/"
        elif [[ -d "$model_file" ]]; then
            cp -a "$model_file" "${UPGRADE_PATH}${RUN_PATH}/model/"
        fi
    done

    # 垃圾站型号特征：TV-3852*L* (TL/HL/TL4G/TLW)
    if [[ "$DEVICE_TYPE" =~ ^TV-3852.*L ]]; then
        info "当前型号 $DEVICE_TYPE 为垃圾站型号，拷贝垃圾检测模型和特征点提取模型..."
        for model_file in "${MODEL_PATH}"/*; do
            if [[ -f "$model_file" ]]; then
                filename=$(basename "$model_file")
                if [[ "$filename" =~ rubish ]]; then
                    info "拷贝垃圾检测模型: $filename"
                    cp -a "$model_file" "${UPGRADE_PATH}${RUN_PATH}/model/"
                fi
                if [[ "$filename" =~ mobileface ]]; then
                    info "拷贝特征点提取模型: $filename"
                    cp -a "$model_file" "${UPGRADE_PATH}${RUN_PATH}/model/"
                fi
            fi
        done
    else
        info "当前型号 $DEVICE_TYPE 非垃圾站型号，跳过垃圾检测模型"
    fi
}

# 安全校验 ROOTFS_PATH
if [[ -z "$ROOTFS_PATH" ]] || [[ ! -d "$ROOTFS_PATH" ]]; then
    error "ROOTFS_PATH 无效或不存在: $ROOTFS_PATH"
fi
case "$ROOTFS_PATH" in
/*/*/*) : ;;
*)
    error "ROOTFS_PATH 格式异常，可能指向系统目录: $ROOTFS_PATH"
    ;;
esac

# ==============================
# 准备升级文件
# ==============================
copy_third_party_libs "$LIB_PATH" "${UPGRADE_PATH}${RUN_PATH}/lib"

# 删除特定库
if [[ -f "${UPGRADE_PATH}${RUN_PATH}/lib/libvqe_record.so" ]]; then
    info "删除旧的 VQE 库文件"
    rm -f "${UPGRADE_PATH}${RUN_PATH}/lib/libvqe_record.so"
fi

# Strip 符号表
if compgen -G "${UPGRADE_PATH}${RUN_PATH}/lib/*" >/dev/null 2>&1; then
    info "正在 strip 依赖库..."
    "$STRIP_TOOL" --strip-all "${UPGRADE_PATH}${RUN_PATH}/lib/"*
fi

# 拷贝插件和字体
cp -a "${THIRD_PARTY_PATH}/IpcComponents-V"* "${UPGRADE_PATH}${RUN_PATH}/third-party/"
cp -a "${THIRD_PARTY_PATH}/ttf" "${UPGRADE_PATH}/${RUN_PATH}/third-party/"

# 拷贝AI模型
copy_model_files

# ==============================
# 编译固件（直接执行，无需 expect/su）
# ==============================
info "开始编译固件"

# 进入固件目录
cd "$FIRMWARE_BASE_PATH"

# 删除整个 rootfs_debug_musl_arm 文件夹
info "删除临时 rootfs 文件夹..."
rm -rf "rootfs_debug_musl_arm"

# 解压裸文件系统
info "解压裸文件系统..."
tar -xzvf rootfs_debug_musl_arm.tar.gz

# 合并到 rootfs
info "合并升级内容到 rootfs..."
cp -af "$UPGRADE_PATH"/* "$ROOTFS_PATH"/

# 镜头信息属于设备出厂属性，仅在生成烧录固件时写入 rootfs。
generate_lens_info_file

# 拷贝 DHCP 脚本
info "拷贝 DHCP 脚本..."
cp -f "$DHCP_PATH/default.script" "$ROOTFS_UDHCPC/default.script"

# 生成新固件
info "生成新固件..."
./build.sh

# ==============================
# 输出校验与打包
# ==============================
IMAGE_DIR="$FIRMWARE_BASE_PATH/hi3516cv610_image_debug_musl"
UBIFS_FILE="$IMAGE_DIR/rootfs_hi3516cv610_2k_128k_120M.ubifs"

if [[ ! -f "$UBIFS_FILE" ]]; then
    error "UBIFS 镜像未生成: $UBIFS_FILE"
fi
if [[ ! -d "$IMAGE_DIR" ]] || [[ -z "$(ls -A "$IMAGE_DIR" 2>/dev/null)" ]]; then
    error "固件输出目录为空: $IMAGE_DIR"
fi

# 复制固件前清理旧内容，避免多型号打包时残留文件混入
if [[ -d "$OUTPUT_PATH/firmware" ]]; then
    rm -rf "$OUTPUT_PATH/firmware"
fi
mkdir -p "$OUTPUT_PATH/firmware"
cp -rf "$IMAGE_DIR"/* "$OUTPUT_PATH/firmware/"

# 固件工具目录默认生成 TV-3852T.xml，最终输出需按当前打包型号命名。
PRODUCT_XML_SOURCE="$OUTPUT_PATH/firmware/TV-3852T.xml"
PRODUCT_XML_TARGET="$OUTPUT_PATH/firmware/${DEVICE_TYPE}.xml"
if [[ -f "$PRODUCT_XML_SOURCE" && "$PRODUCT_XML_SOURCE" != "$PRODUCT_XML_TARGET" ]]; then
    mv -f "$PRODUCT_XML_SOURCE" "$PRODUCT_XML_TARGET"
    info "固件产品 XML 已重命名为: ${DEVICE_TYPE}.xml"
fi

# 打包
info "打包固件为 ZIP..."
(
    cd "$OUTPUT_PATH" || exit 1
    zip -r "${SOFTWARE_NAME}.zip" firmware
)

# 自动摆渡
if [[ "$AUTOFILE_MODE" == true ]]; then
    info "============> 自动摆渡固件包 <============"

    if ! command -v autofile &>/dev/null; then
        error "autofile 命令未找到"
    fi

    (
        export BL_DEVICE="$DEVICE_TYPE"
        cp -a "${CUR_PATH}/../compile.sh" "$OUTPUT_PATH/compile.sh"
        cd "$OUTPUT_PATH" || exit 1
        cp "${SOFTWARE_NAME}.zip" "${PROJECT_ROOT_PATH}/output/bin/"

        autofile "${SOFTWARE_NAME}.zip"

        # 清理摆渡副本
        if [[ -f "${PROJECT_ROOT_PATH}/output/bin/${SOFTWARE_NAME}.zip" ]]; then
            rm -f "${PROJECT_ROOT_PATH}/output/bin/${SOFTWARE_NAME}.zip"
        fi
    ) || error "自动摆渡失败"
fi

info "✅ 固件打包完成: ${SOFTWARE_NAME}.zip"
exit 0
