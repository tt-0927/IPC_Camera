#!/bin/bash
###
 # @FilePath     : make_packet.sh
 # @Author       : zhouzr@kfb.cn
 # @Date         : 2025-06-12 10:59:07
 # @LastEditors: leiyy leiyy@kfb.cn
 # @LastEditTime: 2026-06-15 17:22:02
 # @Description  : 编译升级包脚本
###

# 获取配置文件所在目录
CUR_PATH=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
# echo ${CUR_PATH}
#导入环境配置
source ${CUR_PATH}/../path.conf
source /etc/profile
# 命令失败时立即退出
set -e

# 使用getopt来处理长选项
# :前面的需要带参数, 没有的不需要带参数
if ! ARGS=$(getopt -o d:b:g:v:c:t: --long version:,project:,autofile,copy-model -n "$0" -- "$@"); then
    error "选项和参数解析失败"
    exit 1
fi
eval set -- "$ARGS"
# 是否设置了型号
DEVICE_MODE=false
# 型号
DEVICE_TYPE=""
# 设备项目类型
PROJECT_TYPE=""
# 编译模式
BUILD_MODE="Release"
# 清空
CLEAN_MODE=false
# 版本号
VERSION_NUM=""
# 是否自动摆渡
AUTOFILE_MODE=false
# 是否拷贝模型
IS_COPY_MODEL=true
# 升级包类型
PACKET_TYPE="all"

# 解析选项
while true; do
    case "$1" in
    -d) # -d 设置型号
        #DEVICE_MODE=true
        DEVICE_TYPE="$2"
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
    -c) # 删除临时文件
        CLEAN_MODE=true
        shift 2
        ;;
    -t)
        PACKET_TYPE="$2"
        shift 2
        ;;
    --project) # 项目类型如 itc，中性
        PROJECT_TYPE="$2"
        shift 2
        ;;
    --autofile)
        AUTOFILE_MODE=true
        shift
        ;;
    # 增加参数是否拷贝模型文件    
    --copy-model)
        IS_COPY_MODEL=true
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

# 根据型号选择
if [ "${DEVICE_TYPE}" == "TV-3881T" ]; then
    UPGRADE_ID_INDEX=881
elif [ "${DEVICE_TYPE}" == "TV-3882TI" ]; then
    UPGRADE_ID_INDEX=882
fi

# 删除临时文件
if [ "$CLEAN_MODE" = true ]; then
    rm -rf $UPGRADE_PATH
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
# nginx路径
NGINX_PATH=${THIRD_PARTY_PATH}/nginx
# 字体文件路径
FONT_PATH=${THIRD_PARTY_PATH}/ttf
# iqfiles
IQ_PATH=${ENVIRONMENT_PATH}/iqfiles
# 板端工作目录
RUN_PATH=/opt/cam

# 网页程序路径
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
        error "例如指定参数:--project itc"
        exit 1
        ;;
esac

# 芯片型号
CHIP_TYPE=RV1126B
# 软件包类型
SOFTWARE_TYPE=升级包
# 软件包名称  型号+芯片型号+版本号+升级包类型+软件包类型
SOFTWARE_NAME=${DEVICE_TYPE}-${CHIP_TYPE}-${VERSION_NUM}-${PACKET_TYPE}-${SOFTWARE_TYPE}-${PROJECT_TYPE}

#清理上次编译的文件
#rm -rvf $OUTPUT_PATH/*.bin $OUTPUT_PATH/*.tar.gz
rm -rf ${UPGRADE_PATH}
info "清理上次编译的文件成功"

# app类型拷贝函数
copy_app_files() {

    # 编译全部程序                          
    (
        cd $PLATFORM_PATH/build
        ./make_process.sh -c
        if ! ./make_process.sh -b "$BUILD_MODE" -e "$EXTRA_PARAM"; then
            error "软件编译失败"
            exit 1
        else
            info "软件编译成功"
        fi
    )

    #创建与板卡一致的目录
    mkdir -p ${UPGRADE_PATH}
    mkdir -p ${UPGRADE_PATH}/bin
    mkdir -p ${UPGRADE_PATH}/sbin
    mkdir -p ${UPGRADE_PATH}/opt/cam
    mkdir -p ${UPGRADE_PATH}/opt/course/upload
    # mkdir -p ${UPGRADE_PATH}/opt/course/record
    # mkdir -p ${UPGRADE_PATH}/opt/sys_shell
    mkdir -p ${UPGRADE_PATH}/etc/init.d
    mkdir -p ${UPGRADE_PATH}/etc/profile.d
    mkdir -p ${UPGRADE_PATH}/etc/udev/scripts
    mkdir -p ${UPGRADE_PATH}/etc/iqfiles
    mkdir -p ${UPGRADE_PATH}/etc/dropbear
    mkdir -p ${UPGRADE_PATH}/lib
    mkdir -p ${UPGRADE_PATH}/var/www/html
    mkdir -p ${UPGRADE_PATH}/var/spool/cron/crontabs
    mkdir -p ${UPGRADE_PATH}/var/run
    # mkdir -p ${UPGRADE_PATH}/etc/udev/rules.d
    # mkdir -p ${UPGRADE_PATH}/etc/iqfiles
    # mkdir -p ${UPGRADE_PATH}/userdata

    mkdir -p ${UPGRADE_PATH}/${RUN_PATH}/.config/user_data
    mkdir -p ${UPGRADE_PATH}/${RUN_PATH}/.config/audio
    mkdir -p ${UPGRADE_PATH}/${RUN_PATH}/.config/design_data/register
    mkdir -p ${UPGRADE_PATH}/${RUN_PATH}/bin/.uds_socket
    mkdir -p ${UPGRADE_PATH}/${RUN_PATH}/cert
    mkdir -p ${UPGRADE_PATH}/${RUN_PATH}/gm_cert
    mkdir -p ${UPGRADE_PATH}/${RUN_PATH}/gm_cert/device
    mkdir -p ${UPGRADE_PATH}/${RUN_PATH}/gm_cert/request
    mkdir -p ${UPGRADE_PATH}/${RUN_PATH}/gm_cert/trust
    mkdir -p ${UPGRADE_PATH}/${RUN_PATH}/db
    mkdir -p ${UPGRADE_PATH}/${RUN_PATH}/lib
    mkdir -p "${UPGRADE_PATH}/${RUN_PATH}"/shell
    mkdir -p ${UPGRADE_PATH}/${RUN_PATH}/third-party/ttf
    mkdir -p ${UPGRADE_PATH}/${RUN_PATH}/tools
    mkdir -p ${UPGRADE_PATH}/${RUN_PATH}/run


# ***************** 拷贝需要更新的第三方库 ****************************************************************************************#

    #拷贝NetTVSDK库
    cp -a ${LIB_PATH}/NetTVSDK/lib/*  ${UPGRADE_PATH}/${RUN_PATH}/lib

# ******************************************************************************************************************************#


    #拷贝audio配置
    cp -a ${CONFIG_PATH}/audio/* ${UPGRADE_PATH}/${RUN_PATH}/.config/audio/
    #拷贝config配置
    cp -a ${CONFIG_PATH}/design_data/* ${UPGRADE_PATH}/${RUN_PATH}/.config/design_data/

    #服务器所有执行文件到bin目录
    cp -r ${BIN_PATH:?}/* ${UPGRADE_PATH}/${RUN_PATH}/bin

    chmod 777 -R ${UPGRADE_PATH}/${RUN_PATH}/bin/*

    # 拷贝所有依赖库到lib目录
    # copy_third_party_libs ${LIB_PATH} ${UPGRADE_PATH}${RUN_PATH}/lib
    # 移除符号表、调试信息
    # /opt/hisi-linux/x86-arm/arm-v01c02-linux-musleabi-gcc/bin/arm-v01c02-linux-musleabi-strip --strip-all ${UPGRADE_PATH}${RUN_PATH}/lib/*
    #瑞芯微RV1126B板级包路径
    SDK_PATH=/home/itc/workdir/rockchip/rv1126b_1.1.0/RV1126B_Linux_IPC_SDK
    #编译工具链
    TOOLCHAIN_PATH="$SDK_PATH/tools/linux/toolchain/arm-rockchip1240-linux-gnueabihf/bin"
    ${TOOLCHAIN_PATH}/arm-rockchip1240-linux-gnueabihf-strip ${UPGRADE_PATH}${RUN_PATH}/bin/*

    #拷贝证书文件
    cp -a ${CERT_PATH}/* ${UPGRADE_PATH}/${RUN_PATH}/cert/

    #拷贝网页代码
    cp -a ${WEB_PATH}/* ${UPGRADE_PATH}/var/www/html
    cp -a ${NGINX_PATH} ${UPGRADE_PATH}/${RUN_PATH}/third-party

    # 拷贝字体文件
    #cp -a ${FONT_PATH} ${UPGRADE_PATH}/${RUN_PATH}/third-party

    # 拷贝插件
    cp -a ${THIRD_PARTY_PATH}/IpcComponents-V* ${UPGRADE_PATH}/${RUN_PATH}/third-party

    # 拷贝开源许可证
    cp -a ${THIRD_PARTY_PATH}/softwareLicense.txt ${UPGRADE_PATH}/${RUN_PATH}/third-party

    # 拷贝工具
    # cp -a ${TOOLS_PATH}/gmssl ${UPGRADE_PATH}/${RUN_PATH}/tools
    cp -a ${TOOLS_PATH}/kill.sh ${UPGRADE_PATH}/${RUN_PATH}/tools
    cp -a ${TOOLS_PATH}/ethtool ${UPGRADE_PATH}/${RUN_PATH}/tools

    # 拷贝uftpd至系统目录
    # cp -a ${TOOLS_PATH}/uftpd ${UPGRADE_PATH}/bin
    # cp -a ${TOOLS_PATH}/uftpd/lib* ${UPGRADE_PATH}/lib

    # 拷贝dropbear至系统目录
    cp -a ${TOOLS_PATH}/dropbear/bin/dropbearkey ${UPGRADE_PATH}/bin
    cp -a ${TOOLS_PATH}/dropbear/sbin/dropbear ${UPGRADE_PATH}/sbin
    cp -a ${TOOLS_PATH}/dropbear/stop_ssh_service.sh ${UPGRADE_PATH}/var/spool/cron/crontabs
    cp -a ${TOOLS_PATH}/dropbear/root ${UPGRADE_PATH}/var/spool/cron/crontabs

    #拷贝执行脚本
    #拷贝用户执行脚本
    cp -a "${SHELL_PATH}"/* "${UPGRADE_PATH}/${RUN_PATH}"/shell
    #拷贝系统执行脚本
    cp -a ${ETC_PATH}/asound.conf ${UPGRADE_PATH}/etc/
    cp -a ${ETC_PATH}/profile ${UPGRADE_PATH}/etc/
    cp -a ${ETC_PATH}/resolv.conf ${UPGRADE_PATH}/etc/
    cp -a ${ETC_PATH}/init.d/* ${UPGRADE_PATH}/etc/init.d
    cp -a ${ETC_PATH}/scripts/* ${UPGRADE_PATH}/etc/udev/scripts
    cp -a ${ETC_PATH}/profile.d/* ${UPGRADE_PATH}/etc/profile.d

    #拷贝iqfiles
    if [ "$DEVICE_TYPE" == "TV-3882TI" ]; then 
        cp -a ${IQ_PATH}/sc831hai_default_default.json ${UPGRADE_PATH}/etc/iqfiles
    elif [ "$DEVICE_TYPE" == "TV-3881T" ]; then
        cp -a ${IQ_PATH}/sc850sl_default_default.json ${UPGRADE_PATH}/etc/iqfiles
    else
        PROJECT_MK=""
        error "设备型号异常: $DEVICE_TYPE" $LINENO
        exit 1
    fi

    info "APP文件拷贝完成"

}


# model类型拷贝函数
copy_model_files() {

    mkdir -p ${UPGRADE_PATH}/${RUN_PATH}/model

    #拷贝AI模型
    if [ "$IS_COPY_MODEL" = true ]; then
        echo "拷贝模型文件..."

        if [ "${DEVICE_TYPE}" == "TV-3881T" ]; then
        cp -a ${MODEL_PATH}/*Detect* ${UPGRADE_PATH}/${RUN_PATH}/model/
        cp -a ${MODEL_PATH}/*Face* ${UPGRADE_PATH}/${RUN_PATH}/model/
        cp -a ${MODEL_PATH}/*LicensePlateRec* ${UPGRADE_PATH}/${RUN_PATH}/model/
        cp -a ${MODEL_PATH}/*group* ${UPGRADE_PATH}/${RUN_PATH}/model/
        cp -a ${MODEL_PATH}/*Attribute* ${UPGRADE_PATH}/${RUN_PATH}/model/
        elif [ "${DEVICE_TYPE}" == "TV-3882TI" ]; then
        cp -a ${MODEL_PATH}/*Detect* ${UPGRADE_PATH}/${RUN_PATH}/model/
        cp -a ${MODEL_PATH}/*Face* ${UPGRADE_PATH}/${RUN_PATH}/model/
        cp -a ${MODEL_PATH}/*LicensePlateRec* ${UPGRADE_PATH}/${RUN_PATH}/model/
        cp -a ${MODEL_PATH}/*group* ${UPGRADE_PATH}/${RUN_PATH}/model/
        cp -a ${MODEL_PATH}/*Attribute* ${UPGRADE_PATH}/${RUN_PATH}/model/
        cp -a ${MODEL_PATH}/internvl3-1B-Instruct_vision_rv1126b.rknn ${UPGRADE_PATH}/${RUN_PATH}/model/
        cp -a ${MODEL_PATH}/internvl3-1b-instruct_122_w8a8_rv1126b.rkllm ${UPGRADE_PATH}/${RUN_PATH}/model/
        fi
    else
        echo "不拷贝模型文件..."
    fi

    info "Model文件拷贝完成"

}



# 打包函数
mkpack_files() {

    info "添加权限 !!!!"
    chmod 777 -R ${UPGRADE_PATH}/*

    info "打包中......."
    # 服务端类型
    SERVER_TYPE=0
    #升级包类型
    UPGRADE_PACKET_TYPE=0
    #打包升级包所需文件路径
    UPGRADE_PACKET_PATH=$UPGRADE_PATH

    # 打包类型，设置不同参数
    if [ "${PACKET_TYPE}" == "app" ]; then
        # 软件包名称  型号+芯片型号+版本号+升级包类型+软件包类型
        SOFTWARE_NAME=${DEVICE_TYPE}-${CHIP_TYPE}-${VERSION_NUM}-APP-${SOFTWARE_TYPE}

        UPGRADE_PACKET_PATH=$UPGRADE_PATH
        UPGRADE_PACKET_TYPE=0
    elif [ "${PACKET_TYPE}" == "model" ]; then
        # 软件包名称  型号+芯片型号+版本号+升级包类型+软件包类型
        SOFTWARE_NAME=${DEVICE_TYPE}-${CHIP_TYPE}-${VERSION_NUM}-MODEL-${SOFTWARE_TYPE}

        UPGRADE_PACKET_PATH=${UPGRADE_PATH}/${RUN_PATH}/model
        UPGRADE_PACKET_TYPE=1
    fi

    info "当前打包的版本号: $VERSION_NUM"
    if ! ./mkpack $UPGRADE_PACKET_PATH $SOFTWARE_NAME $VERSION_NUM $UPGRADE_ID_INDEX $SERVER_TYPE $UPGRADE_PACKET_TYPE; then
        error "打包中失败"
        exit 1
    fi

    mv $SOFTWARE_NAME.tar.gz $OUTPUT_PATH/$SOFTWARE_NAME.tar.gz
    mv $SOFTWARE_NAME.bin $OUTPUT_PATH/$SOFTWARE_NAME.bin

    # 自动摆渡
    if [ "$AUTOFILE_MODE" = true ]; then
        info "============> 自动摆渡升级包 <============"
        (
            export BL_DEVICE=$DEVICE_TYPE
            cp -a ${CUR_PATH}/../compile.sh $OUTPUT_PATH/compile.sh
            cd $OUTPUT_PATH
            # cp $OUTPUT_PATH/$SOFTWARE_NAME.tar.gz ${PROJECT_ROOT_PATH}/output/bin
            # autofile "${SOFTWARE_NAME}.tar.gz"
            # rm -rf ${PROJECT_ROOT_PATH}/output/bin/${SOFTWARE_NAME}.tar.gz
            cp $OUTPUT_PATH/$SOFTWARE_NAME.bin ${PROJECT_ROOT_PATH}/output/bin
            autofile "${SOFTWARE_NAME}.bin"
            rm -rf ${PROJECT_ROOT_PATH}/output/bin/${SOFTWARE_NAME}.bin
        )
    fi

    info "当前打包的升级包名为: $SOFTWARE_NAME"

}


main() {
    # 检查PACKET_TYPE是否为空
    if [[ -z "$PACKET_TYPE" ]]; then
        log_error "错误: PACKET_TYPE未指定"
        echo "用法: $0 [-t all|app|model]"
        exit 1
    fi
    
    # 根据PACKET_TYPE执行对应操作
    case "$PACKET_TYPE" in
        app)
            copy_app_files
            mkpack_files
            ;;
        model)
            copy_model_files
            mkpack_files
            ;;
        all)
            #app
            copy_app_files

            PACKET_TYPE=app
            mkpack_files

            #model
            copy_model_files

            PACKET_TYPE=model
            mkpack_files
            ;;
        *)
            log_error "错误: 不支持的PACKET_TYPE '$PACKET_TYPE'"
            echo "支持的类型: all,app,model"
            exit 1
            ;;
    esac
    
}

# 执行主函数
main

exit 0
