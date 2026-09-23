#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# ipc_rtspserver 源码发布打包脚本
#
# 将 share/cam_share/media/ipc_rtspserver/ 的已提交源码打包为
# ipc_rtspserver.tar.gz,投递到 cam_opensource_library/libIpcRtspServer/,
# 供其 compile.sh 解压交叉编译(对齐 live555/libRtspServer 的发布习惯)。
#
# 包内容约定:
#   - 顶层目录固定为 ipc_rtspserver/(compile.sh 按此解压);
#   - 剔除 third_party/libevent/(host 自测专用镜像,发布走 libevent-openipc SDK);
#   - 写入 REVISION 文件(12 位短哈希),包内无 .git,compile.sh 以此传
#     -DIPC_RTSP_REVISION_OVERRIDE,保证归档内嵌 revision 可追溯。
#
# 用法:
#   tools/pack_release.sh --out <DIR>     # 投递目录(libIpcRtspServer 组件目录,必填)
#
# 前置条件:share/cam_share/media/ipc_rtspserver 路径下无未提交变更
# (归档 revision 必须来自 clean commit,见 docs/smolrtsp_rtspserver_docs/27 §27.3)。
# -----------------------------------------------------------------------------
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
REPO_ROOT="$(git -C "${PROJECT_DIR}" rev-parse --show-toplevel)"
# 库在所属仓库内的相对路径：cam_share 已独立成库，不能沿用工作区视角的
# 硬编码路径（否则 clean 检查恒空、git archive 产出空包），改为动态解析。
SRC_REL="$(git -C "${PROJECT_DIR}" ls-files --full-name "${PROJECT_DIR}/CMakeLists.txt" | head -n1)"
SRC_REL="${SRC_REL%/CMakeLists.txt}"
if [ -z "${SRC_REL}" ] || [ "${SRC_REL}" = "${PROJECT_DIR}" ]; then
    echo "错误:无法定位库在仓库内的相对路径（文件未提交？）:${PROJECT_DIR}" >&2
    exit 1
fi
PKG_NAME="ipc_rtspserver"
ARCHIVE_NAME="${PKG_NAME}.tar.gz"

OUT_DIR=""
while [ $# -gt 0 ]; do
    case "$1" in
        --out)
            [ $# -ge 2 ] || { echo "错误:--out 缺少参数" >&2; exit 2; }
            OUT_DIR="$2"; shift 2 ;;
        -h|--help)
            grep '^#' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *)
            echo "未知参数:$1" >&2; exit 2 ;;
    esac
done

# 投递目录必填:每人本地仓库路径不同,不做工作区布局猜测
if [ -z "${OUT_DIR}" ]; then
    echo "错误:缺少 --out <DIR>(libIpcRtspServer 组件目录)" >&2
    echo "示例:tools/pack_release.sh --out ~/workdir/camera/cam_opensource_library/libIpcRtspServer" >&2
    exit 2
fi

# 1. 发布纪律:库源码路径必须 clean,revision 才能对得上归档
if [ -n "$(git -C "${REPO_ROOT}" status --porcelain -- "${SRC_REL}")" ]; then
    echo "错误:${SRC_REL} 存在未提交变更,先 commit 再打包(归档 revision 纪律,见 27 号文档 §27.3)" >&2
    git -C "${REPO_ROOT}" status --short -- "${SRC_REL}" >&2
    exit 1
fi
REVISION="$(git -C "${REPO_ROOT}" rev-parse --short=12 HEAD)"

if [ ! -d "${OUT_DIR}" ]; then
    echo "错误:投递目录不存在:${OUT_DIR}(首次使用请先创建,或用 --out 指定)" >&2
    exit 1
fi
if [ ! -f "${OUT_DIR}/compile.sh" ]; then
    echo "错误:${OUT_DIR} 不是 libIpcRtspServer 组件目录(缺少 compile.sh)" >&2
    exit 1
fi

# 2. 从已提交内容解包(天然排除 build/ 等未跟踪产物)
WORK_DIR="$(mktemp -d /tmp/rtsp_pack.XXXXXX)"
trap 'rm -rf "${WORK_DIR}"' EXIT

git -C "${REPO_ROOT}" archive --format=tar HEAD -- "${SRC_REL}" | tar -x -C "${WORK_DIR}"
mv "${WORK_DIR}/${SRC_REL}" "${WORK_DIR}/${PKG_NAME}"

# 3. 剔除 host 自测专用 libevent 镜像,写入包内 revision
rm -rf "${WORK_DIR}/${PKG_NAME}/third_party/libevent"
printf '%s\n' "${REVISION}" > "${WORK_DIR}/${PKG_NAME}/REVISION"

[ -f "${WORK_DIR}/${PKG_NAME}/CMakeLists.txt" ] || {
    echo "错误:包内容异常,缺少 CMakeLists.txt" >&2
    exit 1
}

# 4. 打包并计算指纹
tar -C "${WORK_DIR}" --sort=name --owner=0 --group=0 --numeric-owner \
    -czf "${WORK_DIR}/${ARCHIVE_NAME}" "${PKG_NAME}"
ARCHIVE_SHA256="$(sha256sum "${WORK_DIR}/${ARCHIVE_NAME}" | awk '{print $1}')"

# 5. 投递(覆盖旧包,包名固定,compile.sh 不用改)
cp "${WORK_DIR}/${ARCHIVE_NAME}" "${OUT_DIR}/${ARCHIVE_NAME}"
printf '%s  %s\n' "${ARCHIVE_SHA256}" "${ARCHIVE_NAME}" > "${OUT_DIR}/${ARCHIVE_NAME}.sha256"

ARCHIVE_SIZE="$(stat -c '%s' "${OUT_DIR}/${ARCHIVE_NAME}")"
echo "打包完成:revision=${REVISION}"
echo "  投递:${OUT_DIR}/${ARCHIVE_NAME}(${ARCHIVE_SIZE} bytes)"
echo "  sha256:${ARCHIVE_SHA256}"
echo "下一步:"
echo "  cd ${OUT_DIR} && ./compile.sh <cross-prefix> [--sync-ipc-platform <ipc_platform/lib/rtsp_smol>]"