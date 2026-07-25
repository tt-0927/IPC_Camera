#!/bin/bash

# 获取配置文件所在目录
CUR_PATH=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
echo "${CUR_PATH}"
#导入环境配置
# shellcheck source=ipc_platform/build/path.conf
source "${CUR_PATH}"/path.conf
# shellcheck source=ipc_platform/build/help.conf
source "${CUR_PATH}"/help.conf

# 命令失败时立即退出
set -e

# 使用getopt来处理长选项
# :前面的需要带参数, 没有的不需要带参数
if ! ARGS=$(getopt -o m:b:s:d:e:v:t:cahpi -l target:,build:,device:,sensor:,project:,packet_type:,extra:,version:,clean,all,help,strip,packet,img,autofile,all-focal,all-project -n "$0" -- "$@"); then
    error "选项和参数解析失败"
    error "查看帮助说明：./build.sh -h/--help"
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
# 打升级包类型 all、app、model
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
# BUILD_DEVICE_TYPE="-DDEVICE_TYPE=Hi3516_IPC"
# 是否设置了设备型号
DEVICE_MODE=false
# 是否设置了镜头型号
SENSOR_MODE=false
# 是否设置了项目类型
PROJECT_MODE=false
# 设备型号
# DEVICE_TYPE=""
# 裁剪bin文件
STRIP_MODE=false
# 是否自动摆渡
AUTOFILE_MODE=false
# 是否执行过 set 操作
DID_SET=false
# 是否打包当前 sensor 前缀下的所有焦距
ALL_FOCAL_MODE=false
# 是否打包所有项目类型
ALL_PROJECT_MODE=false

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
    -d | --device) # -d 设置设备型号
        DEVICE_MODE=true
        DEVICE_TYPE="$2"
        shift 2
        ;;
    -s | --sensor) # -s 设置镜头型号
        SENSOR_MODE=true
        SENSOR_TYPE="$2"
        shift 2
        ;;
    --project) # --project 设置项目类型
        PROJECT_MODE=true
        PROJECT_TYPE="$2"
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
    -c | --clean) # -c 清空模式
        CLEAN_MODE=true
        shift
        ;;
    --strip) # --strip 裁剪bin文件
        STRIP_MODE=true
        shift
        ;;
    -p | --packet) # -p 打包升级包
        PACKET_MODE=true
        shift
        ;;
    -t | --packet_type) # 升级包类型如 app，model
        PACKET_TYPE="$2"
        shift 2
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
    --all-focal)
        ALL_FOCAL_MODE=true
        shift
        ;;
    --all-project)
        ALL_PROJECT_MODE=true
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
    list_device_types
    list_sensor_types
    list_project_types
    display_current_config
    exit 0
fi

# 删除临时文件
if [ "$CLEAN_MODE" = true ]; then
    "${CUR_PATH}"/make_process.sh -c
    # 删除本地型号文件
    rm -rf "$LOCAL_DEVICE_FILE"
    # 删除本地镜头文件
    rm -rf "$LOCAL_SENSOR_FILE"
    # 删除本地项目类型文件
    rm -rf "$LOCAL_PROJECT_FILE"
    info "成功清除所有临时文件"
    exit 0
fi

# 生成设备型号
if [ "$DEVICE_MODE" = true ]; then
    set_device_type "$DEVICE_TYPE"

    # 切换 hi3516cv610 固件分支
    # TV-3852TL4G、TV-3852TLW 使用对应分支，其他型号统一使用 TV-3852T 分支
    FIRMWARE_PATH="${PROJECT_ROOT_PATH}/hi3516cv610"
    if [[ -d "$FIRMWARE_PATH" ]]; then
        info "检查固件分支..."
        cd "$FIRMWARE_PATH"

        # 确定目标分支
        case "$DEVICE_TYPE" in
        TV-3852TL4G | TV-3852TLW)
            TARGET_BRANCH="$DEVICE_TYPE"
            ;;
        *)
            TARGET_BRANCH="TV-3852T"
            ;;
        esac

        # 获取远程源名称（取第一个）
        REMOTE_NAME=$(git remote | head -n1)
        if [[ -z "$REMOTE_NAME" ]]; then
            error "hi3516cv610 仓库未配置远程源"
        fi

        # 获取当前分支名
        CURRENT_BRANCH=$(git symbolic-ref --short HEAD 2>/dev/null || echo "")

        if [[ "$CURRENT_BRANCH" != "$TARGET_BRANCH" ]]; then
            info "当前分支为 '$CURRENT_BRANCH'，需要切换到 '$TARGET_BRANCH'"

            # 检查本地分支是否存在
            if ! git show-ref --verify --quiet "refs/heads/${TARGET_BRANCH}"; then
                info "本地分支 '$TARGET_BRANCH' 不存在，从远程创建..."

                # 检查远程分支是否存在
                if ! git show-ref --verify --quiet "refs/remotes/${REMOTE_NAME}/${TARGET_BRANCH}"; then
                    error "远程分支 '${REMOTE_NAME}/${TARGET_BRANCH}' 不存在，请先在固件仓库创建"
                fi

                # 创建本地分支并跟踪远程分支
                git branch --track "$TARGET_BRANCH" "${REMOTE_NAME}/${TARGET_BRANCH}"
                info "已创建本地分支 '$TARGET_BRANCH' 并跟踪 '${REMOTE_NAME}/${TARGET_BRANCH}'"
            fi

            # 切换分支并拉取最新代码
            git checkout "$TARGET_BRANCH"
            git pull
            info "已切换到分支 '$TARGET_BRANCH' 并更新代码"
        else
            info "当前已在分支 '$TARGET_BRANCH' 上，拉取最新代码..."
            git pull
        fi

        cd "$PROJECT_ROOT_PATH"
    else
        warn "固件目录不存在: $FIRMWARE_PATH，跳过分支切换"
    fi

    DID_SET=true
fi

# 生成镜头型号
if [ "$SENSOR_MODE" = true ]; then
    set_sensor_type "$SENSOR_TYPE"
    DID_SET=true
fi

# 生成项目类型
if [ "$PROJECT_MODE" = true ]; then
    set_project_type "$PROJECT_TYPE"
    DID_SET=true
fi

# 裁剪bin文件的调试信息
if [ "$STRIP_MODE" = true ]; then
    info "裁剪bin文件的调试信息" $LINENO
    # 直接传入型号参数
    ./make_strip.sh "$DEVICE_TYPE"
    DID_SET=true
fi

# 如果只做配置设置，则直接退出；打包升级包或固件时沿用 set_* 结果继续执行
if [ "$DID_SET" = true ] && [ "$PACKET_MODE" = false ] && [ "$IMAGE_MODE" = false ]; then
    exit 0
fi

# 判断是否存在本地设备型号文件
check_device_file
# 判断是否存在本地镜头型号文件
check_sensor_file
# 判断是否存在本地项目类型文件
check_project_file
# 检查编译类型是否有效的函数
check_buile_type "$BUILD_MODE"

# 组合打包时实际使用的镜头型号列表
PACKAGE_SENSOR_TYPES=()
# 组合打包时实际使用的项目类型列表
PACKAGE_PROJECT_TYPES=()
# 本次成功生成的升级包文件列表
PACKET_OUTPUT_FILES=()
# 本次成功生成的固件包文件列表
IMAGE_OUTPUT_FILES=()

# ============================================================
# 函数：根据镜头型号生成包名中的镜头标识
# 参数：$1 - 镜头型号
# 输出：用于包名的镜头标识
# ============================================================
get_package_sensor_name() {
    local sensor_type=$1

    if [[ "$sensor_type" == *"533hai"* && "$sensor_type" =~ (f[0-9_]+mm) ]]; then
        echo "${BASH_REMATCH[1]}"
        return 0
    fi

    echo "$sensor_type"
}

# ============================================================
# 函数：根据升级包类型生成包名类型标识
# 参数：$1 - 升级包类型
# 输出：用于升级包名的类型标识
# ============================================================
get_packet_type_label() {
    local packet_type=$1

    case "$packet_type" in
    all)
        echo ""
        ;;
    app)
        echo "-APP"
        ;;
    model)
        echo "-MODEL"
        ;;
    lib)
        echo "-LIB"
        ;;
    custom)
        echo "-CUSTOM"
        ;;
    *)
        error "不支持的升级包类型: $packet_type"
        exit 1
        ;;
    esac
}

# ============================================================
# 函数：记录单次升级包输出文件
# 参数：$1 - 镜头型号；$2 - 项目类型
# ============================================================
record_packet_outputs() {
    local sensor_type=$1
    local project_type=$2
    local chip_type="Hi3516CV610-20S"
    local language_type="中文"
    local software_type="升级包"
    local sensor_name
    local type_label
    local software_name

    sensor_name=$(get_package_sensor_name "$sensor_type")
    type_label=$(get_packet_type_label "$PACKET_TYPE")
    software_name="${DEVICE_TYPE}-${chip_type}-${sensor_name}${type_label}-${VERSION_NUM}-${language_type}-${software_type}-${project_type}"

    if [ -f "$PACK_BIN_PATH/${software_name}.bin" ]; then
        PACKET_OUTPUT_FILES+=("$PACK_BIN_PATH/${software_name}.bin")
    fi
    if [ -f "$PACK_BIN_PATH/${software_name}.tar.gz" ]; then
        PACKET_OUTPUT_FILES+=("$PACK_BIN_PATH/${software_name}.tar.gz")
    fi
}

# ============================================================
# 函数：记录单次固件包输出文件
# 参数：$1 - 镜头型号；$2 - 项目类型
# ============================================================
record_image_outputs() {
    local sensor_type=$1
    local project_type=$2
    local chip_type="Hi3516CV610-20S"
    local language_type="中文"
    local software_type="固件包"
    local sensor_name
    local software_name

    sensor_name=$(get_package_sensor_name "$sensor_type")
    software_name="${DEVICE_TYPE}-${chip_type}-${sensor_name}-${VERSION_NUM}-${language_type}-${software_type}-${project_type}"

    if [ -f "$IMAGE_BIN_PATH/${software_name}.zip" ]; then
        IMAGE_OUTPUT_FILES+=("$IMAGE_BIN_PATH/${software_name}.zip")
    fi
}

# ============================================================
# 函数：打印本次打包输出文件汇总
# 说明：在批量打包结束后统一展示升级包与固件包路径
# ============================================================
print_generated_outputs() {
    local output_file

    info "============> 本次打包生成文件汇总 <============"

    if [ "${#PACKET_OUTPUT_FILES[@]}" -gt 0 ]; then
        info "升级包文件:"
        for output_file in "${PACKET_OUTPUT_FILES[@]}"; do
            echo "  $output_file" >&2
        done
    fi

    if [ "${#IMAGE_OUTPUT_FILES[@]}" -gt 0 ]; then
        info "固件包文件:"
        for output_file in "${IMAGE_OUTPUT_FILES[@]}"; do
            echo "  $output_file" >&2
        done
    fi

    info "============> 文件汇总结束 <============"
}

# ============================================================
# 函数：生成组合打包列表
# 说明：根据 --all-focal 和 --all-project 展开镜头焦距与项目类型
# ============================================================
prepare_package_matrix() {
    local sensor_model="${SENSOR_TYPE%%-*}"
    local sensor_type

    check_device_type "$DEVICE_TYPE"
    check_sensor_type "$SENSOR_TYPE"
    check_project_type "$PROJECT_TYPE"

    PACKAGE_SENSOR_TYPES=("$SENSOR_TYPE")
    if [ "$ALL_FOCAL_MODE" = true ]; then
        if [ "$sensor_model" != "sc533hai" ]; then
            error "--all-focal 当前仅支持 sc533hai-*，当前镜头型号: $SENSOR_TYPE"
            exit 1
        fi

        PACKAGE_SENSOR_TYPES=()
        for sensor_type in "${SENSOR_TYPES[@]}"; do
            if [[ "$sensor_type" == "${sensor_model}-"* ]]; then
                PACKAGE_SENSOR_TYPES+=("$sensor_type")
            fi
        done

        if [ "${#PACKAGE_SENSOR_TYPES[@]}" -eq 0 ]; then
            error "未找到 ${sensor_model} 的可打包焦距列表"
            exit 1
        fi
    fi

    PACKAGE_PROJECT_TYPES=("$PROJECT_TYPE")
    if [ "$ALL_PROJECT_MODE" = true ]; then
        PACKAGE_PROJECT_TYPES=("${PROJECT_TYPES[@]}")
    fi
}

# ============================================================
# 函数：执行单个升级包打包
# 参数：$1 - 镜头型号；$2 - 项目类型
# ============================================================
run_make_packet() {
    local sensor_type=$1
    local project_type=$2

    (
        cd "$BUILD_ROOT_PATH"/pack/
        info "============> 打包升级包: ${sensor_type} / ${project_type} <============"
        make_packet_args=(--version "$VERSION_NUM" -d "$DEVICE_TYPE" -s "$sensor_type" -b "$BUILD_MODE" -t "$PACKET_TYPE" --project "$project_type")

        if [ "$AUTOFILE_MODE" = true ]; then
            make_packet_args+=(--autofile)
        fi
        echo "${make_packet_args[@]}"
        if ! ./make_packet.sh "${make_packet_args[@]}"; then
            error "升级包编译失败: ${sensor_type} / ${project_type}"
            exit 1
        fi
        info "打包升级包成功: ${sensor_type} / ${project_type}, 执行文件生成在[$OUTPUT_DIR_NAME/packet]"
    )
    record_packet_outputs "$sensor_type" "$project_type"
}

# ============================================================
# 函数：执行单个固件打包
# 参数：$1 - 镜头型号；$2 - 项目类型
# ============================================================
run_make_image() {
    local sensor_type=$1
    local project_type=$2

    (
        cd "$BUILD_ROOT_PATH"/image/
        info "============> 打包固件: ${sensor_type} / ${project_type} <============"
        make_image_args=(--version "$VERSION_NUM" -d "$DEVICE_TYPE" -s "$sensor_type" --project "$project_type")

        if [ "$AUTOFILE_MODE" = true ]; then
            make_image_args+=(--autofile)
        fi
        echo "${make_image_args[@]}"
        if ! ./make_image.sh "${make_image_args[@]}"; then
            error "打包固件失败: ${sensor_type} / ${project_type}"
            exit 1
        fi
        info "打包固件成功: ${sensor_type} / ${project_type}, 执行文件生成在[$OUTPUT_DIR_NAME/image]"
    )
    record_image_outputs "$sensor_type" "$project_type"
}

# ============================================================
# 函数：按组合打包升级包
# 说明：支持 --all-focal 与 --all-project 同时展开
# ============================================================
run_packet_matrix() {
    local sensor_type
    local project_type

    prepare_package_matrix
    for sensor_type in "${PACKAGE_SENSOR_TYPES[@]}"; do
        for project_type in "${PACKAGE_PROJECT_TYPES[@]}"; do
            run_make_packet "$sensor_type" "$project_type"
        done
    done
    print_generated_outputs
}

# ============================================================
# 函数：按组合打包固件
# 说明：每个组合先生成升级内容，再生成对应固件
# ============================================================
run_image_matrix() {
    local sensor_type
    local project_type

    prepare_package_matrix
    for sensor_type in "${PACKAGE_SENSOR_TYPES[@]}"; do
        for project_type in "${PACKAGE_PROJECT_TYPES[@]}"; do
            run_make_packet "$sensor_type" "$project_type"
            run_make_image "$sensor_type" "$project_type"
        done
    done
    print_generated_outputs
}

# 打包升级包
if [ "$PACKET_MODE" = true ]; then
    info "============> 打包升级包前，先生成版本号 <============"
    # set_version
    run_packet_matrix
    exit 0
fi

# 打包固件
if [ "$IMAGE_MODE" = true ]; then

    info "============> 编译升级包前，先生成版本号 <============"
    # set_version
    run_image_matrix
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
    if ! "${CUR_PATH}"/make_process.sh "${make_process_args[@]}"; then
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
    if ! "${CUR_PATH}"/make_process.sh "${make_process_args[@]}"; then
        error "程序编译失败" $LINENO
        exit 1
    fi

    #裁剪掉调试信息
    #./make_strip.sh
    exit 0
fi

error "参数处理未定义 $0 $*"
exit 1
