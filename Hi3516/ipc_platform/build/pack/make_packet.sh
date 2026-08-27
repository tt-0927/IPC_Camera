#!/bin/bash
###
 # @FilePath     : make_packet.sh
# @Author       : zhouzr@kfb.cn
# @Date         : 2025-06-12 10:59:07
 # @LastEditors  : zhouzr@kfb.cn
 # @LastEditTime : 2026-08-04 13:46:27
# @Description  : 编译升级包脚本
###

# 获取配置文件所在目录
CUR_PATH=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
# echo ${CUR_PATH}
#导入环境配置
source "${CUR_PATH}/../path.conf"
source /etc/profile

# 命令失败时立即退出
set -e

# 使用getopt来处理长选项
# :前面的需要带参数, 没有的不需要带参数
if ! ARGS=$(getopt -o d:b:g:v:c:s:t: --long version:,sensor:,project:,packet_type:,autofile,copyright -n "$0" -- "$@"); then
    error "选项和参数解析失败"
    exit 1
fi
eval set -- "$ARGS"
# 设备型号
DEVICE_TYPE=""
# 镜头型号
SENSOR_TYPE=""
# 项目类型
PROJECT_TYPE=""
# 编译模式
BUILD_MODE="Release"
# 清空
CLEAN_MODE=false
# 版本号
VERSION_NUM=""
# 是否自动摆渡
AUTOFILE_MODE=false
# 是否启用软著打包模式
COPYRIGHT_MODE=false
# 打升级包类型 all、app、model、lib、custom、script
PACKET_TYPE="all"
# 语言类型 中文、英文、中英文通用
LANGUAGE_TYPE="中文"

# 解析选项
while true; do
    case "$1" in
    -d) # -d 设置设备型号
        DEVICE_TYPE="$2"
        shift 2
        ;;
    -s) # -s 设置镜头型号
        SENSOR_TYPE="$2"
        shift 2
        ;;
    -b) # -b 编译模式
        BUILD_MODE="$2"
        shift 2
        ;;
    -v | --version) # 设置软件版本
        VERSION_NUM="$2"
        shift 2
        ;;
    -t | --packet_type) # 升级包类型如 app，model
        PACKET_TYPE="$2"
        shift 2
        ;;
    --project) # 项目类型如 itc，中性
        PROJECT_TYPE="$2"
        shift 2
        ;;
    -c) # 删除临时文件
        CLEAN_MODE=true
        shift 2
        ;;
    --autofile)
        AUTOFILE_MODE=true
        shift
        ;;
    --copyright) # 软著模式：文件名用软著名称包裹
        COPYRIGHT_MODE=true
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

# 检查编译类型是否有效的函数
check_buile_type "$BUILD_MODE"

# 检查版本号
if [ -z "$VERSION_NUM" ]; then
    error "版本号为空 eg: V1.01"
    exit 1
fi

# 检查升级包类型
if [ -z "$PACKET_TYPE" ]; then
    error "打包类型为空 eg: all"
    exit 1
fi

# 项目根目录
ROOT_PATH=${PROJECT_ROOT_PATH}
# 平台路径
PLATFORM_PATH=${ROOT_PATH}/ipc_platform
# 升级包输出路径
OUTPUT_PATH=${ROOT_PATH}/output/packet
# 升级文件根目录
UPGRADE_PATH=${OUTPUT_PATH}/upgradefile
# 固件环境目录
ENVIRONMENT_PATH=${PLATFORM_PATH}/environment
# 第三方目录
THIRD_PARTY_PATH=${PLATFORM_PATH}/third-party
# 第三方依赖库
LIB_PATH=${PLATFORM_PATH}/lib
# 工具
TOOLS_PATH=${PLATFORM_PATH}/tools
# 升级ID索引
UPGRADE_ID_INDEX=""
# 根据 SENSOR_TYPE 提取 sensor 型号（sc500ai / sc533hai）
SENSOR_MODEL="$(get_sensor_model)"

# ============================================================
# 函数：根据设备型号获取 sensor 配置目录
# 参数：$1 - sensor ini 目录名（如 sensor_sc533hai）
#       $2 - 设备型号（如 TV-3852T）
# 说明：sc500ai 直接返回根目录，sc533hai 根据设备型号选择子目录
# ============================================================
get_sensor_config_dir_by_device() {
    local sensor_ini_name=$1
    local device_type=$2
    local sensor_root="${CONFIG_PATH}/${sensor_ini_name}"

    # sc500ai 直接返回根目录（无子目录结构）
    if [[ "$SENSOR_MODEL" == "sc500ai" ]]; then
        echo "$sensor_root"
        return
    fi

    # sc533hai 根据设备型号选择子目录
    case "$device_type" in
    TV-3852T) echo "${sensor_root}/tv_3852t" ;;
    TV-3852H) echo "${sensor_root}/tv_3852h" ;;
    TV-3852HZT) echo "${sensor_root}/tv_3852h" ;;
    TV-3852TL) echo "${sensor_root}/tv_3852tl" ;;
    TV-3852TL4G) echo "${sensor_root}/tv_3852tl4g" ;;
    TV-3852TLW) echo "${sensor_root}/tv_3852tlw" ;;
    TV-3852HL) echo "${sensor_root}/tv_3852hl" ;;
    *)
        error "未知的设备型号: $device_type，无法确定 sensor 配置目录" $LINENO
        exit 1
        ;;
    esac
}

# 根据型号选择
case "${DEVICE_TYPE}" in
TV-3852T | TV-3852H | TV-3852TL | TV-3852HL | TV-3852TL4G | TV-3852TLW | TV-3852HZT)
    UPGRADE_ID_INDEX=3852
    ;;
*)
    # 其他设备类型的处理可以放在这里
    ;;
esac

# 删除临时文件
if [ "$CLEAN_MODE" = true ]; then
    rm -rf "$UPGRADE_PATH"
    exit 0
fi

# 配置路径
CONFIG_PATH=${ENVIRONMENT_PATH}/config
# 证书路径
CERT_PATH=${ENVIRONMENT_PATH}/cert
# 模型路径
MODEL_PATH=${ENVIRONMENT_PATH}/model
# 系统路径
SYSTEM_PATH=${ENVIRONMENT_PATH}/system
# 脚本路径
SHELL_PATH=${ENVIRONMENT_PATH}/shell
# etc目录路径
ETC_PATH=${SYSTEM_PATH}/etc
# root目录路径
ROOT_USER_PATH=${SYSTEM_PATH}/root
# admin目录路径
ADMIN_USER_PATH=${SYSTEM_PATH}/home/admin
# nginx路径
NGINX_PATH=${THIRD_PARTY_PATH}/nginx
# 字体文件路径
# FONT_PATH=${THIRD_PARTY_PATH}/ttf
# 板端工作目录
RUN_PATH=/opt/cam

# 网页程序路径
WEB_PATH=${THIRD_PARTY_PATH}/html

# 芯片型号（默认 20S，3852TLW/TL4G 使用 00S）
CHIP_TYPE=Hi3516CV610-20S

# 3852TLW 和 3852TL4G 使用 Hi3516CV610-00S 主控，其他型号使用 Hi3516CV610-20S
if [[ "$DEVICE_TYPE" =~ ^TV-3852(TLW|TL4G)$ ]]; then
    CHIP_TYPE="Hi3516CV610-00S"
    info "当前型号 $DEVICE_TYPE 使用主控: $CHIP_TYPE"
fi

# 软件包类型
SOFTWARE_TYPE=升级包

# ============================================================
# 函数：根据镜头型号生成包名中的镜头标识
# 参数：$1 - SENSOR_TYPE (如 sc500ai-f4mm, sc533hai-f4mm)
# 返回：设置全局变量 SENSOR_NAME_FOR_PACKAGE
# ============================================================
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

# ============================================================
# 函数：生成软件包名称
# 参数：$1 - PACKET_TYPE (all/app/model/lib/custom/script)
# 返回：设置全局变量 SOFTWARE_NAME
# ============================================================
generate_software_name() {
    local packet_type=$1
    local type_label=""

    case "$packet_type" in
    all)
        # all 类型不添加标识
        type_label=""
        ;;
    app)
        type_label="-APP"
        ;;
    model)
        type_label="-MODEL"
        ;;
    lib)
        type_label="-LIB"
        ;;
    custom)
        type_label="-CUSTOM"
        ;;
    script)
        type_label="-SCRIPT"
        ;;
    *)
        error "不支持的升级包类型: $packet_type"
        exit 1
        ;;
    esac

    # 生成包名中使用的镜头标识
    generate_sensor_name_for_package "$SENSOR_TYPE"

    # 软件包名称格式：型号-芯片型号-镜头标识-[升级包类型]-版本号-[中英文类型]-软件包类型-项目类型
    if [ -z "$type_label" ]; then
        SOFTWARE_NAME="${DEVICE_TYPE}-${CHIP_TYPE}-${SENSOR_NAME_FOR_PACKAGE}-${VERSION_NUM}-${LANGUAGE_TYPE}-${SOFTWARE_TYPE}-${PROJECT_TYPE}"
    else
        SOFTWARE_NAME="${DEVICE_TYPE}-${CHIP_TYPE}-${SENSOR_NAME_FOR_PACKAGE}${type_label}-${VERSION_NUM}-${LANGUAGE_TYPE}-${SOFTWARE_TYPE}-${PROJECT_TYPE}"
    fi

    # 软著模式：用软著前缀包裹文件名
    if [ "$COPYRIGHT_MODE" = true ]; then
        SOFTWARE_NAME="${COPYRIGHT_PREFIX}(${SOFTWARE_NAME})"
    fi

    info "生成的软件包名称: ${SOFTWARE_NAME}"
}

# 调用函数生成软件包名称
generate_software_name "$PACKET_TYPE"

# ============================================================
# 函数：创建升级包目录结构
# 说明：创建与板卡一致的目录结构
# ============================================================
create_upgrade_directories() {
    info "创建升级包目录结构"

    # 创建系统级目录
    mkdir -p "${UPGRADE_PATH}"
    mkdir -p "${UPGRADE_PATH}"/bin
    mkdir -p "${UPGRADE_PATH}"/sbin
    mkdir -p "${UPGRADE_PATH}"/opt/cam
    mkdir -p "${UPGRADE_PATH}"/etc/init.d
    mkdir -p "${UPGRADE_PATH}"/etc/scripts
    mkdir -p "${UPGRADE_PATH}"/home/admin
    mkdir -p "${UPGRADE_PATH}"/root
    mkdir -p "${UPGRADE_PATH}"/lib
    mkdir -p "${UPGRADE_PATH}"/var/www/html
    mkdir -p "${UPGRADE_PATH}"/var/spool/cron/crontabs
    mkdir -p "${UPGRADE_PATH}"/var/run

    # 创建应用级目录
    mkdir -p "${UPGRADE_PATH}/${RUN_PATH}"/.config/user_data
    mkdir -p "${UPGRADE_PATH}/${RUN_PATH}"/.config/audio
    mkdir -p "${UPGRADE_PATH}/${RUN_PATH}"/.config/design_data/register
    mkdir -p "${UPGRADE_PATH}/${RUN_PATH}"/bin/.uds_socket
    mkdir -p "${UPGRADE_PATH}/${RUN_PATH}"/cert
    mkdir -p "${UPGRADE_PATH}/${RUN_PATH}"/gm_cert
    mkdir -p "${UPGRADE_PATH}/${RUN_PATH}"/gm_cert/device
    mkdir -p "${UPGRADE_PATH}/${RUN_PATH}"/gm_cert/request
    mkdir -p "${UPGRADE_PATH}/${RUN_PATH}"/gm_cert/trust
    mkdir -p "${UPGRADE_PATH}/${RUN_PATH}"/db
    mkdir -p "${UPGRADE_PATH}/${RUN_PATH}"/lib
    mkdir -p "${UPGRADE_PATH}/${RUN_PATH}"/model
    mkdir -p "${UPGRADE_PATH}/${RUN_PATH}"/shell
    mkdir -p "${UPGRADE_PATH}/${RUN_PATH}"/third-party/ttf
    mkdir -p "${UPGRADE_PATH}/${RUN_PATH}"/tools

    info "目录结构创建完成"
}

# ============================================================
# 函数：拷贝配置文件
# 说明：拷贝系统配置、音频配置、sensor配置等
#         仅垃圾站型号拷贝ai_garbage_detection.json、ai_mobileface.json
#         仅HZT型号拷贝ai_exhibition.json
# ============================================================
copy_config_files() {
    info "拷贝配置文件"

    # 拷贝config配置（排除垃圾检测配置，非垃圾站型号不需要）
    local filename
    for config_file in "${CONFIG_PATH}"/design_data/*; do
        if [ -f "$config_file" ]; then
            filename=$(basename "$config_file")
            # 排除垃圾检测配置（非垃圾站型号不需要）
            if [[ "$filename" == "ai_garbage_detection.json" ]]; then
                info "跳过垃圾检测配置: $filename"
                continue
            fi
            # 排除特征点提取配置（非垃圾站型号不需要）
            if [[ "$filename" == "ai_mobileface.json" ]]; then
                info "跳过特征点提取配置: $filename"
                continue
            fi
            # 排除展会检测配置（非HZT型号不需要）
            if [[ "$filename" == "ai_exhibition.json" ]]; then
                info "跳过展会检测配置: $filename"
                continue
            fi
            cp -a "$config_file" "${UPGRADE_PATH}/${RUN_PATH}"/.config/design_data/
        elif [ -d "$config_file" ]; then
            cp -a "$config_file" "${UPGRADE_PATH}/${RUN_PATH}"/.config/design_data/
        fi
    done

    # 仅垃圾站型号拷贝垃圾检测配置
    # 垃圾站型号特征：TV-3852*L* (TL/HL/TL4G/TLW)
    if [[ "$DEVICE_TYPE" =~ ^TV-3852.*L ]]; then
        local garbage_config="${CONFIG_PATH}/design_data/ai_garbage_detection.json"
        if [ -f "$garbage_config" ]; then
            info "当前型号 $DEVICE_TYPE 为垃圾站型号，拷贝垃圾检测配置..."
            cp -a "$garbage_config" "${UPGRADE_PATH}/${RUN_PATH}"/.config/design_data/
        fi
        local mobileface_config="${CONFIG_PATH}/design_data/ai_mobileface.json"
        if [ -f "$mobileface_config" ]; then
            info "当前型号 $DEVICE_TYPE 为垃圾站型号，拷贝特征点提取配置..."
            cp -a "$mobileface_config" "${UPGRADE_PATH}/${RUN_PATH}"/.config/design_data/
        fi
    fi

    # 仅HZT型号拷贝展会检测配置
    if [[ "$DEVICE_TYPE" == "TV-3852HZT" ]]; then
        local exhibition_config="${CONFIG_PATH}/design_data/ai_exhibition.json"
        if [ -f "$exhibition_config" ]; then
            info "当前型号 $DEVICE_TYPE 为HZT型号，拷贝展会检测配置..."
            cp -a "$exhibition_config" "${UPGRADE_PATH}/${RUN_PATH}"/.config/design_data/
        fi
    fi

    cp -a "${CONFIG_PATH}"/audio/* "${UPGRADE_PATH}/${RUN_PATH}"/.config/audio/

    # 拷贝 sensor ini配置，根据镜头型号和设备型号选择
    SENSOR_INI_NAME="sensor_${SENSOR_MODEL}"
    SENSOR_INI_SRC="$(get_sensor_config_dir_by_device "$SENSOR_INI_NAME" "$DEVICE_TYPE")"
    SENSOR_INI_DST="${UPGRADE_PATH}/${RUN_PATH}/.config/sensor"

    if [ -d "$SENSOR_INI_SRC" ]; then
        info "使用的sensor ini目录: ${SENSOR_INI_SRC}"
        rm -rf "$SENSOR_INI_DST"
        mkdir -p "$SENSOR_INI_DST"
        cp -a "$SENSOR_INI_SRC"/. "$SENSOR_INI_DST"/
    else
        error "未找到sensor ini目录: ${SENSOR_INI_SRC}" $LINENO
        exit 1
    fi
}

# ============================================================
# 函数：拷贝可执行程序
# 说明：拷贝所有执行文件到bin目录并进行strip处理
# ============================================================
copy_executable_files() {
    # 编译全部程序
    (
        cd "$PLATFORM_PATH"/build
        ./make_process.sh -c
        if ! ./make_process.sh -b "$BUILD_MODE" -e "$EXTRA_PARAM"; then
            error "软件编译失败"
            exit 1
        else
            info "软件编译成功"
        fi
    )
    info "拷贝可执行程序"

    # 服务器所有执行文件到bin目录
    cp -r "${BIN_PATH:?}"/* "${UPGRADE_PATH}/${RUN_PATH}"/bin
    # 删除不需要的执行程序
    rm -f "${UPGRADE_PATH}/${RUN_PATH}"/bin/encoder.cgi "${UPGRADE_PATH}/${RUN_PATH}"/bin/system_monitor
    # chmod 777 -R "${UPGRADE_PATH}/${RUN_PATH}"/bin/*

    /opt/hisi-linux/x86-arm/arm-v01c02-linux-musleabi-gcc/bin/arm-v01c02-linux-musleabi-strip "${UPGRADE_PATH}${RUN_PATH}"/bin/*
}

# ============================================================
# 函数：拷贝所有依赖库
# 说明：拷贝所有依赖库到lib目录并进行strip处理
# ============================================================
copy_lib_files() {
    info "拷贝所有依赖库"

    # 拷贝所有依赖库到lib目录
    copy_third_party_libs "${LIB_PATH}" "${UPGRADE_PATH}/${RUN_PATH}"/lib
    # 移除符号表、调试信息
    /opt/hisi-linux/x86-arm/arm-v01c02-linux-musleabi-gcc/bin/arm-v01c02-linux-musleabi-strip --strip-all "${UPGRADE_PATH}/${RUN_PATH}"/lib/*
}

# ============================================================
# 函数：拷贝证书文件
# 说明：拷贝所有证书文件
# ============================================================
copy_cert_files() {
    info "拷贝证书文件"
    cp -a "${CERT_PATH}"/* "${UPGRADE_PATH}/${RUN_PATH}"/cert/
}

# ============================================================
# 函数：拷贝AI模型
# 说明：拷贝所有AI模型文件，仅垃圾站型号拷贝垃圾检测模型，仅HZT型号拷贝展会检测模型
#         垃圾检测模型: Hi3516CV610_yolov8n_rubish_640_384_V1.0.om
#         展会检测模型: Hi3516CV610_yolov8n_exhibition_1024_576_V1.0.1.om
# ============================================================
copy_model_files() {
    info "拷贝AI模型"

    # 创建目标模型目录
    mkdir -p "${UPGRADE_PATH}/${RUN_PATH}"/model

    # 拷贝通用模型（所有型号都需要）
    local filename
    for model_file in "${MODEL_PATH}"/*; do
        if [ -f "$model_file" ]; then
            # 排除垃圾检测模型（非垃圾站型号不需要）
            filename=$(basename "$model_file")
            if [[ "$filename" =~ rubish ]]; then
                info "跳过垃圾检测模型: $filename"
                continue
            fi
            if [[ "$filename" =~ mobileface ]]; then
                info "跳过特征点提取模型: $filename"
                continue
            fi
            # 排除展会检测模型（非HZT型号不需要）
            if [[ "$filename" =~ exhibition ]]; then
                info "跳过展会检测模型: $filename"
                continue
            fi
            cp -a "$model_file" "${UPGRADE_PATH}/${RUN_PATH}"/model/
        elif [ -d "$model_file" ]; then
            # 拷贝子目录（如normal目录）
            cp -a "$model_file" "${UPGRADE_PATH}/${RUN_PATH}"/model/
        fi
    done

    # 仅垃圾站型号拷贝垃圾检测模型
    # 垃圾站型号特征：TV-3852*L* (TL/HL/TL4G/TLW)
    if [[ "$DEVICE_TYPE" =~ ^TV-3852.*L ]]; then
        info "当前型号 $DEVICE_TYPE 为垃圾站型号，拷贝垃圾检测模型..."
        for model_file in "${MODEL_PATH}"/*; do
            if [ -f "$model_file" ]; then
                filename=$(basename "$model_file")
                if [[ "$filename" =~ rubish ]]; then
                    info "拷贝垃圾检测模型: $filename"
                    cp -a "$model_file" "${UPGRADE_PATH}/${RUN_PATH}"/model/
                fi
                if [[ "$filename" =~ mobileface ]]; then
                    info "拷贝特征点提取模型: $filename"
                    cp -a "$model_file" "${UPGRADE_PATH}/${RUN_PATH}"/model/
                fi
            fi
        done
    else
        info "当前型号 $DEVICE_TYPE 非垃圾站型号，跳过垃圾检测模型"
    fi

    # 仅HZT型号拷贝展会检测模型
    if [[ "$DEVICE_TYPE" == "TV-3852HZT" ]]; then
        info "当前型号 $DEVICE_TYPE 为HZT型号，拷贝展会检测模型..."
        for model_file in "${MODEL_PATH}"/*; do
            if [ -f "$model_file" ]; then
                filename=$(basename "$model_file")
                if [[ "$filename" =~ exhibition ]]; then
                    info "拷贝展会检测模型: $filename"
                    cp -a "$model_file" "${UPGRADE_PATH}/${RUN_PATH}"/model/
                fi
            fi
        done
    else
        info "当前型号 $DEVICE_TYPE 非HZT型号，跳过展会检测模型"
    fi
}

# ============================================================
# 函数：拷贝shell脚本
# 说明：拷贝运行时脚本到 /opt/cam/shell
# ============================================================
copy_shell_files() {
    info "拷贝shell脚本"

    if [[ ! -d "$SHELL_PATH" ]]; then
        error "shell脚本目录不存在: $SHELL_PATH"
        exit 1
    fi

    mkdir -p "${UPGRADE_PATH}/${RUN_PATH}"/shell
    cp -a "${SHELL_PATH}"/* "${UPGRADE_PATH}/${RUN_PATH}"/shell/
}

# ============================================================
# 函数：拷贝网页文件
# 说明：拷贝网页代码和nginx
# ============================================================
copy_web_files() {
    info "拷贝网页文件"
    info "当前项目类型: ${PROJECT_TYPE}"

    # 网页程序路径
    case "$PROJECT_TYPE" in
    itc)
        WEB_PATH=${THIRD_PARTY_PATH}/html
        ;;
    中性)
        WEB_PATH=${THIRD_PARTY_PATH}/html-中性
        ;;
    *)
        error "不支持的项目类型: ${PROJECT_TYPE}，支持的类型: itc, 中性"
        exit 1
        ;;
    esac
    cp -a "${WEB_PATH}"/* "${UPGRADE_PATH}"/var/www/html
}

# ============================================================
# 函数：拷贝第三方文件
# 说明：拷贝开源许可证等第三方文件
# ============================================================
copy_third_party_files() {
    info "拷贝第三方文件"
    # 拷贝字体文件
    # cp -a ${FONT_PATH} ${UPGRADE_PATH}/${RUN_PATH}/third-party

    # 拷贝插件
    # cp -a "${THIRD_PARTY_PATH}"/IpcComponents-V* "${UPGRADE_PATH}/${RUN_PATH}"/third-party
    # cp -a "${THIRD_PARTY_PATH}/ttf" "${UPGRADE_PATH}/${RUN_PATH}/third-party/"
    # 拷贝开源许可证
    cp -a "${THIRD_PARTY_PATH}"/softwareLicense.txt "${UPGRADE_PATH}/${RUN_PATH}"/third-party
    # 拷贝nginx
    cp -a "${NGINX_PATH}" "${UPGRADE_PATH}/${RUN_PATH}"/third-party
}

# ============================================================
# 函数：拷贝工具文件
# 说明：拷贝ethtool、gmssl等工具
# ============================================================
copy_tool_files() {
    info "拷贝工具文件"

    # 拷贝工具
    cp -a "${TOOLS_PATH}"/ethtool "${UPGRADE_PATH}/${RUN_PATH}"/tools
    # cp -a "${TOOLS_PATH}"/gmssl "${UPGRADE_PATH}/${RUN_PATH}"/tools
    cp -a "${TOOLS_PATH}"/kill.sh "${UPGRADE_PATH}/${RUN_PATH}"/tools

    # 拷贝uftpd至系统目录
    cp -a "${TOOLS_PATH}"/uftpd "${UPGRADE_PATH}"/bin

    # 拷贝dropbear至系统目录
    cp -a "${TOOLS_PATH}"/dropbear/bin/dropbearkey "${UPGRADE_PATH}"/bin
    cp -a "${TOOLS_PATH}"/dropbear/sbin/dropbear "${UPGRADE_PATH}"/sbin
    cp -a "${TOOLS_PATH}"/dropbear/stop_ssh_service.sh "${UPGRADE_PATH}"/var/spool/cron/crontabs
}

# ============================================================
# 函数：拷贝系统脚本
# 说明：拷贝系统执行脚本和驱动加载脚本
# ============================================================
copy_system_scripts() {
    info "拷贝系统脚本"

    # 拷贝系统执行脚本
    cp -a "${ETC_PATH}"/mdev.conf "${UPGRADE_PATH}"/etc/
    cp -a "${ETC_PATH}"/profile "${UPGRADE_PATH}"/etc/
    cp -a "${ETC_PATH}"/resolv.conf "${UPGRADE_PATH}"/etc/
    
    if [[ "$DEVICE_TYPE" =~ ^TV-3852.*L ]]; then #垃圾站项目
    cp -a "${ETC_PATH}"/scripts_face/* "${UPGRADE_PATH}"/etc/scripts
    else
    cp -a "${ETC_PATH}"/scripts/* "${UPGRADE_PATH}"/etc/scripts
    fi

    # 拷贝 init.d 脚本，排除 S90upgrade（垃圾站项目 init_00S.d 下无 S90）
    if [[ "$DEVICE_TYPE" =~ ^TV-3852(TLW|TL4G)$ ]]; then
        cp -a "${ETC_PATH}"/init_00S.d/* "${UPGRADE_PATH}"/etc/init.d
    else
        # 只有非垃圾站项目才需要排除 S90upgrade
        for file in "${ETC_PATH}"/init.d/*; do
            if [ -f "$file" ]; then
                filename=$(basename "$file")
                if [[ "$filename" != "S90upgrade" ]]; then
                    cp -a "$file" "${UPGRADE_PATH}"/etc/init.d/
                fi
            fi
        done
    fi
    # 拷贝admin用户profile
    cp -a "${ADMIN_USER_PATH}"/profile "${UPGRADE_PATH}"/home/admin/.profile

    # 拷贝root驱动加载脚本，根据镜头型号选择
    
    if [[ "$DEVICE_TYPE" =~ ^TV-3852(TLW|TL4G)$ ]]; then #垃圾站项目
    INSMOD_NAME="insmod_sc533hai_00S.sh"
    INSMOD_SRC="${ROOT_USER_PATH}/insmod_sc533hai_00S.sh"
    else
    INSMOD_NAME="insmod_${SENSOR_MODEL}.sh"
    INSMOD_SRC="${ROOT_USER_PATH}/insmod_${SENSOR_MODEL}.sh"
    fi

    if [ -f "$INSMOD_SRC" ]; then
        info "使用驱动加载脚本: ${INSMOD_NAME}"
        cp -a "$INSMOD_SRC" "${UPGRADE_PATH}/root/insmod.sh"
    else
        error "未找到驱动加载脚本: ${INSMOD_SRC}" $LINENO
        exit 1
    fi
}

#清理上次编译的文件
#rm -rvf $OUTPUT_PATH/*.bin $OUTPUT_PATH/*.tar.gz
rm -rf "${UPGRADE_PATH}"
info "清理上次编译的文件成功"

# 创建目录结构
create_upgrade_directories

# ============================================================
# 函数：拷贝完整升级包内容（PACKET_TYPE=all）
# 说明：拷贝所有文件，用于完整升级（因模型大小过大，现在不进行拷贝）
# ============================================================
copy_all_files() {
    info "============> 开始拷贝完整升级包内容（不包含动态库） <============"

    copy_config_files
    copy_executable_files
    copy_cert_files
    # 注意：不拷贝AI模型
    # copy_model_files
    copy_web_files
    copy_third_party_files
    copy_tool_files
    copy_shell_files
    copy_system_scripts

    info "============> 完整升级包内容拷贝完成 <============"
}

# ============================================================
# 函数：拷贝应用升级包内容（PACKET_TYPE=app）
# 说明：拷贝除AI模型、lib 动态库外的所有文件
# ============================================================
copy_app_files() {
    info "============> 开始拷贝应用升级包内容（不包含AI模型） <============"

    copy_config_files
    # copy_executable_files
    copy_cert_files
    # 注意：不拷贝AI模型、lib 动态库
    copy_web_files
    copy_third_party_files
    copy_tool_files
    copy_shell_files
    copy_system_scripts

    info "============> 应用升级包内容拷贝完成 <============"
}

# ============================================================
# 函数：拷贝模型升级包内容（PACKET_TYPE=model）
# 说明：仅拷贝AI模型文件
# ============================================================
copy_model_only_files() {
    info "============> 开始拷贝模型升级包内容（仅AI模型） <============"

    copy_model_files

    info "============> 模型升级包内容拷贝完成 <============"
}

# ============================================================
# 函数：拷贝库升级包内容（PACKET_TYPE=lib）
# 说明：仅拷贝相关动态库
# ============================================================
copy_lib_only_files() {
    info "============> 开始拷贝库升级包内容（仅动态库） <============"

    copy_lib_files
    copy_executable_files

    info "============> 库升级包内容拷贝完成 <============"
}

# ============================================================
# 函数：拷贝自定义内容（PACKET_TYPE=custom）
# 说明：仅拷贝自定义内容
# ============================================================
copy_custom_files() {
    info "============> 开始拷贝库自定义内容（默认拷贝可执行程序、动态库） <============"

    copy_executable_files
    # copy_web_files
    # copy_config_files

    info "============> 库升级包内容拷贝完成 <============"
}

# ============================================================
# 函数：拷贝脚本升级包内容（PACKET_TYPE=script）
# 说明：仅拷贝 /etc/init.d/S90upgrade
# ============================================================
copy_script_only_files() {
    info "============> 开始拷贝脚本升级包内容（仅 S90upgrade） <============"

    cp -a "${ETC_PATH}"/init.d/S90upgrade "${UPGRADE_PATH}"/etc/init.d/

    info "============> 脚本升级包内容拷贝完成 <============"
}

# ============================================================
# 根据 PACKET_TYPE 执行对应的拷贝操作
# ============================================================
info "当前升级包类型: ${PACKET_TYPE}"

case "$PACKET_TYPE" in
all)
    copy_all_files
    ;;
app)
    copy_app_files
    ;;
model)
    copy_model_only_files
    ;;
lib)
    copy_lib_only_files
    ;;
custom)
    copy_custom_files
    ;;
script)
    copy_script_only_files
    ;;
*)
    error "不支持的升级包类型: ${PACKET_TYPE}，支持的类型: all, app, model, lib, custom, script"
    exit 1
    ;;
esac

info "添加权限 !!!!"
chmod 777 -R "${UPGRADE_PATH}"/*

info "打包中......."
# 服务端类型
SERVER_TYPE=0

info "当前打包的版本号: $VERSION_NUM"
if ! ./mkpack "$UPGRADE_PATH" "$SOFTWARE_NAME" "$VERSION_NUM" "$UPGRADE_ID_INDEX" "$SERVER_TYPE"; then
    error "打包中失败"
    exit 1
fi

mv "$SOFTWARE_NAME.tar.gz" "$OUTPUT_PATH/$SOFTWARE_NAME.tar.gz"
mv "$SOFTWARE_NAME.bin" "$OUTPUT_PATH/$SOFTWARE_NAME.bin"

# 自动摆渡
if [ "$AUTOFILE_MODE" = true ]; then
    info "============> 自动摆渡升级包 <============"
    (
        export BL_DEVICE=$DEVICE_TYPE
        cp -a "${CUR_PATH}"/../compile.sh "$OUTPUT_PATH"/compile.sh
        cd "$OUTPUT_PATH"
        # cp $OUTPUT_PATH/$SOFTWARE_NAME.tar.gz ${PROJECT_ROOT_PATH}/output/bin
        # autofile "${SOFTWARE_NAME}.tar.gz"
        # rm -rf ${PROJECT_ROOT_PATH}/output/bin/${SOFTWARE_NAME}.tar.gz
        cp "$OUTPUT_PATH/$SOFTWARE_NAME.bin" "${PROJECT_ROOT_PATH}/output/bin"
        autofile "${SOFTWARE_NAME}.bin"
        rm -rf "${PROJECT_ROOT_PATH}/output/bin/${SOFTWARE_NAME}.bin"
    )
    exit 0
fi

info "当前打包的升级包名为: $SOFTWARE_NAME"
exit 0