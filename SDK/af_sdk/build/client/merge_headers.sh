#!/bin/bash
set -e  # 遇到错误立即退出

# ===================== 配置与参数校验 =====================
# 检查参数数量是否正确
if [ $# -ne 3 ]; then
    echo -e "\033[31m用法错误！正确用法：\033[0m $0 <NetTVSDKCommon.h路径> <NetTVSDKClientInterface.h路径> <目标输出目录>"
    echo -e "示例：$0 ./src/NetTVSDKCommon.h ./src/NetTVSDKClientInterface.h ./output"
    exit 1
fi

# 定义核心变量
COMMON_H="$1"          # 第一个源文件路径
SERVER_INTERFACE_H="$2" # 第二个源文件路径
TARGET_DIR="$3"        # 目标输出目录
TARGET_H="${TARGET_DIR}/NetTVSDK.h"      # Linux 侧原始生成头文件
TARGET_ITC_H="${TARGET_DIR}/ItcNetTVSDK.h" # 对外交付的 Itc 版本头文件

# 检查源文件是否存在
check_file_exists() {
    if [ ! -f "$1" ]; then
        echo -e "\033[31m错误：文件 $1 不存在！\033[0m"
        exit 1
    fi
}
check_file_exists "${COMMON_H}"
check_file_exists "${SERVER_INTERFACE_H}"

# 创建目标目录（不存在则创建）
mkdir -p "${TARGET_DIR}" || {
    echo -e "\033[31m错误：无法创建目标目录 ${TARGET_DIR}（权限不足？）\033[0m"
    exit 1
}

# ===================== 核心合并逻辑 =====================
echo -e "\033[32m开始合并头文件...\033[0m"

# 1. 写入目标文件的统一开头（自定义保护宏+extern "C"）
# 注意：需要定义 NETTVSDK_COMMON_H 以避免其他文件重复包含 NetTVSDKCommon.h
cat > "${TARGET_H}" << 'EOF'
#ifndef NETTVSDK_H
#define NETTVSDK_H

// 定义此宏以避免其他文件重复包含 NetTVSDKCommon.h
#define NETTVSDK_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

EOF

# 2. 处理NetTVSDKCommon.h：剔除头尾保护块，只保留核心内容
# 兼容GNU/BSD sed，拆分-e参数，使用更通用的正则
echo "正在处理 ${COMMON_H}..."
sed \
    -e '/^#ifndef NETTVSDK_COMMON_H/,/^#endif/ d' \
    -e '/^#ifdef  __cplusplus/,/^#endif  \* end of __cplusplus \*\/$/ d' \
    -e '/^#endif  \* end of _NET_TV_SDK_H_ \*\/$/ d' \
    "${COMMON_H}" >> "${TARGET_H}"

# 3. 处理NetTVSDKClientInterface.h：剔除头尾保护块，只保留核心内容
echo "正在处理 ${SERVER_INTERFACE_H}..."
sed \
    -e '/^#ifndef _NETTVSDKCLIENTINTERFACE_H/,/^#endif/ d' \
    -e '/^#ifdef __cplusplus/,/^#endif/ d' \
    -e '/^#endif$/ d' \
    "${SERVER_INTERFACE_H}" >> "${TARGET_H}"

# 4. 写入目标文件的统一结尾
cat >> "${TARGET_H}" << 'EOF'

#ifdef __cplusplus
}
#endif

#endif /* NETTVSDK_H */
EOF

# ===================== 完成提示 =====================
echo -e "\033[32m合并完成！\033[0m"
echo -e "目标文件路径：\033[36m${TARGET_H}\033[0m"
ls -l "${TARGET_H}"  # 展示生成的文件信息

cp -f "${TARGET_H}" "${TARGET_ITC_H}"
echo -e "Itc 版本头文件路径：\033[36m${TARGET_ITC_H}\033[0m"
ls -l "${TARGET_ITC_H}"
