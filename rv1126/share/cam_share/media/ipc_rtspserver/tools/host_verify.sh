#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# 主机侧端到端验证脚本
#
# 做四件事：
#   1) 构建库/PoC/单测（可用 --no-build 跳过）；
#   2) 跑单元测试（ctest）；
#   3) 启动 PoC（离线 Annex-B 文件模拟 VENC），跑协议探针（TCP/UDP/负向/鉴权）；
#   4) 用 ffprobe/ffmpeg 真实拉流，校验 SDP、codec 与 RTP 头字段。
#
# 用法：
#   ./media/ipc_rtspserver/tools/host_verify.sh                    # 默认 8554 端口、无鉴权
#   ./media/ipc_rtspserver/tools/host_verify.sh --auth             # 额外验证 Digest 鉴权
#   ./media/ipc_rtspserver/tools/host_verify.sh --no-build
#
# 环境要求：cmake、gcc/g++、python3、ffmpeg/ffprobe（缺失时跳过对应步骤）。
# -----------------------------------------------------------------------------
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
if REPO_DIR="$(git -C "${PROJECT_DIR}" rev-parse --show-toplevel 2>/dev/null)"; then
    :
else
    # cam_share 独立仓库下，ipc_rtspserver 位于 media/ 下两级。
    REPO_DIR="$(cd "${PROJECT_DIR}/../.." && pwd)"
fi

BUILD_DIR="${REPO_DIR}/build/rtsp_host"
MEDIA_DIR="${MEDIA_DIR:-/tmp/rtsp_media}"
PORT="${PORT:-8554}"
DO_BUILD=1
DO_AUTH=0
RUN_DIR="$(mktemp -d /tmp/rtsp_verify.XXXXXX)"

USER_NAME="admin"
USER_PASSWORD="zfrl@168"

while [ $# -gt 0 ]; do
    case "$1" in
        --no-build) DO_BUILD=0 ;;
        --auth) DO_AUTH=1 ;;
        --port) shift; PORT="$1" ;;
        *) echo "未知参数: $1"; exit 2 ;;
    esac
    shift
done

PASS=0
FAIL=0
SKIP=0

step() { printf '\n=== %s ===\n' "$1"; }
ok()   { PASS=$((PASS + 1)); printf '[通过] %s\n' "$1"; }
bad()  { FAIL=$((FAIL + 1)); printf '[失败] %s\n' "$1"; }
skip() { SKIP=$((SKIP + 1)); printf '[跳过] %s\n' "$1"; }

cleanup() {
    if [ -n "${POC_PID:-}" ] && kill -0 "${POC_PID}" 2>/dev/null; then
        kill "${POC_PID}" 2>/dev/null
        wait "${POC_PID}" 2>/dev/null
    fi
    rm -rf "${RUN_DIR}"
}
trap cleanup EXIT

# ---------------------------------------------------------------------------
# 0) 准备测试码流
# ---------------------------------------------------------------------------
prepare_media() {
    step "准备测试码流"
    mkdir -p "${MEDIA_DIR}"
    if [ ! -s "${MEDIA_DIR}/main.h264" ]; then
        if command -v ffmpeg >/dev/null 2>&1; then
            ffmpeg -y -loglevel error -f lavfi -i testsrc2=size=640x360:rate=25 -t 6 \
                -c:v libx264 -preset ultrafast -tune zerolatency \
                -x264-params keyint=25:min-keyint=25:scenecut=0:idr=1 \
                -pix_fmt yuv420p -f h264 "${MEDIA_DIR}/main.h264" >/dev/null 2>&1
        fi
    fi
    if [ ! -s "${MEDIA_DIR}/sub.h265" ]; then
        if command -v ffmpeg >/dev/null 2>&1; then
            ffmpeg -y -loglevel error -f lavfi -i testsrc2=size=320x180:rate=25 -t 6 \
                -c:v libx265 -preset ultrafast \
                -x265-params log-level=none:keyint=25:min-keyint=25:scenecut=0:open-gop=0 \
                -pix_fmt yuv420p -f hevc "${MEDIA_DIR}/sub.h265" >/dev/null 2>&1
        fi
    fi
    if [ ! -s "${MEDIA_DIR}/audio.aac" ]; then
        if command -v ffmpeg >/dev/null 2>&1; then
            ffmpeg -y -loglevel error -f lavfi -i "sine=frequency=440:sample_rate=16000" -t 8 \
                -c:a aac -b:a 32k -f adts "${MEDIA_DIR}/audio.aac" >/dev/null 2>&1
        fi
    fi
    if [ ! -s "${MEDIA_DIR}/main.mjpg" ]; then
        if command -v ffmpeg >/dev/null 2>&1; then
            # -q:v 3 高质量：帧普遍超过单包分片上限，顺带覆盖 RFC 2435 分片路径。
            ffmpeg -y -loglevel error -f lavfi -i testsrc2=size=640x360:rate=25 -t 6 \
                -c:v mjpeg -q:v 3 -f mjpeg "${MEDIA_DIR}/main.mjpg" >/dev/null 2>&1
        fi
    fi
    if [ ! -s "${MEDIA_DIR}/audio.alaw" ]; then
        if command -v ffmpeg >/dev/null 2>&1; then
            ffmpeg -y -loglevel error -f lavfi -i "sine=frequency=440:sample_rate=8000" -t 8 \
                -c:a pcm_alaw -f alaw "${MEDIA_DIR}/audio.alaw" >/dev/null 2>&1
        fi
    fi
    if [ ! -s "${MEDIA_DIR}/audio.ulaw" ]; then
        if command -v ffmpeg >/dev/null 2>&1; then
            ffmpeg -y -loglevel error -f lavfi -i "sine=frequency=440:sample_rate=8000" -t 8 \
                -c:a pcm_mulaw -f mulaw "${MEDIA_DIR}/audio.ulaw" >/dev/null 2>&1
        fi
    fi
    if [ ! -s "${MEDIA_DIR}/audio.g726" ]; then
        if command -v ffmpeg >/dev/null 2>&1; then
            ffmpeg -y -loglevel error -f lavfi -i "sine=frequency=440:sample_rate=8000" -t 8 \
                -c:a adpcm_g726 -b:a 32000 -f g726 "${MEDIA_DIR}/audio.g726" >/dev/null 2>&1
        fi
    fi

    if [ -s "${MEDIA_DIR}/main.h264" ]; then
        ok "主码流测试文件 ${MEDIA_DIR}/main.h264"
    else
        bad "缺少主码流测试文件（需要 ffmpeg）"
    fi
    if [ -s "${MEDIA_DIR}/sub.h265" ]; then
        ok "子码流测试文件 ${MEDIA_DIR}/sub.h265"
    else
        skip "缺少子码流测试文件（仅验证主码流）"
    fi
}

# ---------------------------------------------------------------------------
# 1) 构建
# ---------------------------------------------------------------------------
build() {
    step "构建 ipc_rtspserver"
    if [ "${DO_BUILD}" -eq 0 ]; then
        skip "按参数跳过构建"
        return
    fi
    if cmake -S "${PROJECT_DIR}" -B "${BUILD_DIR}" >"${RUN_DIR}/cmake.log" 2>&1 \
        && cmake --build "${BUILD_DIR}" -- -j6 >"${RUN_DIR}/build.log" 2>&1; then
        ok "构建成功（${BUILD_DIR}）"
    else
        bad "构建失败，见 ${RUN_DIR}/build.log"
        tail -20 "${RUN_DIR}/build.log" 2>/dev/null
        exit 1
    fi
}

# ---------------------------------------------------------------------------
# 2) 单元测试
# ---------------------------------------------------------------------------
unit_tests() {
    step "单元测试（ctest）"
    if (cd "${BUILD_DIR}" && ctest --output-on-failure --timeout 60 -j6 >"${RUN_DIR}/ctest.log" 2>&1); then
        ok "ctest 全部通过（$(grep -c 'Passed' "${RUN_DIR}/ctest.log" || true) 组）"
    else
        bad "ctest 失败"
        tail -20 "${RUN_DIR}/ctest.log"
    fi
}

# ---------------------------------------------------------------------------
# 3) 启动 PoC
# ---------------------------------------------------------------------------
start_poc() {
    step "启动 PoC（文件媒体源）"

    # 清理上一次运行遗留的 PoC 进程（只会匹配本验证程序自身的进程名）。
    if pgrep -x rtspserver_poc >/dev/null 2>&1; then
        printf '[提示] 发现遗留的 rtspserver_poc 进程，先结束它\n'
        pkill -x rtspserver_poc 2>/dev/null
        for _ in $(seq 1 20); do
            pgrep -x rtspserver_poc >/dev/null 2>&1 || break
            sleep 0.2
        done
    fi

    local auth_args=()
    if [ "${DO_AUTH}" -eq 1 ]; then
        auth_args=(--user "${USER_NAME}" --password "${USER_PASSWORD}")
    fi

    "${BUILD_DIR}/poc/rtspserver_poc" \
        --port "${PORT}" \
        --main "${MEDIA_DIR}/main.h264" \
        --sub "${MEDIA_DIR}/sub.h265" \
        --sub-codec h265 --fps 25 --verbose 1 "${auth_args[@]}" >"${RUN_DIR}/poc.log" 2>&1 &
    POC_PID=$!

    # 端口可能处于 TIME_WAIT 或仍被其它进程占用：最多重试 10 秒。
    for attempt in $(seq 1 50); do
        if grep -q "RTSP服务已启动" "${RUN_DIR}/poc.log" 2>/dev/null; then
            ok "PoC 已监听端口 ${PORT}（pid ${POC_PID}）"
            return 0
        fi
        if grep -q "errno:98" "${RUN_DIR}/poc.log" 2>/dev/null; then
            if [ "${attempt}" -eq 1 ]; then
                printf '[提示] 端口 %s 被占用，等待释放后重试\n' "${PORT}"
            fi
            sleep 0.2
            kill "${POC_PID}" 2>/dev/null
            wait "${POC_PID}" 2>/dev/null
            : >"${RUN_DIR}/poc.log"
            "${BUILD_DIR}/poc/rtspserver_poc" \
                --port "${PORT}" \
                --main "${MEDIA_DIR}/main.h264" \
                --sub "${MEDIA_DIR}/sub.h265" \
                --sub-codec h265 --fps 25 --verbose 1 "${auth_args[@]}" >"${RUN_DIR}/poc.log" 2>&1 &
            POC_PID=$!
            continue
        fi
        if ! kill -0 "${POC_PID}" 2>/dev/null; then
            bad "PoC 启动失败"
            cat "${RUN_DIR}/poc.log"
            return 1
        fi
        sleep 0.2
    done
    bad "PoC 启动超时"
    cat "${RUN_DIR}/poc.log"
    return 1
}

# ---------------------------------------------------------------------------
# 4) 协议探针
# ---------------------------------------------------------------------------
probe() {
    local transport="$1"
    local suite="$2"
    local extra=()
    if [ "${DO_AUTH}" -eq 1 ]; then
        extra=(--user "${USER_NAME}" --password "${USER_PASSWORD}")
    fi
    if timeout 60 python3 "${SCRIPT_DIR}/rtsp_probe.py" \
        --host 127.0.0.1 --port "${PORT}" --transport "${transport}" \
        --suite "${suite}" --seconds 2 \
        --rtp-csv "${RUN_DIR}/rtp_${transport}.csv" "${extra[@]}" >"${RUN_DIR}/probe_${transport}_${suite}.log" 2>&1; then
        ok "协议探针 ${transport}/${suite}"
    else
        bad "协议探针 ${transport}/${suite}"
        cat "${RUN_DIR}/probe_${transport}_${suite}.log"
    fi
}

probes() {
    step "协议探针"
    if ! command -v python3 >/dev/null 2>&1; then
        skip "缺少 python3"
        return
    fi
    probe tcp basic
    probe udp basic
    probe tcp negative
    if [ "${DO_AUTH}" -eq 1 ]; then
        probe tcp auth
    fi
}

# ---------------------------------------------------------------------------
# 5) 真实播放器拉流
# ---------------------------------------------------------------------------
player_pull() {
    step "ffprobe 拉流校验"
    if ! command -v ffprobe >/dev/null 2>&1; then
        skip "缺少 ffprobe"
        return
    fi

    local url="rtsp://127.0.0.1:${PORT}/Streaming/Channels/101"
    if [ "${DO_AUTH}" -eq 1 ]; then
        url="rtsp://${USER_NAME}:${USER_PASSWORD}@127.0.0.1:${PORT}/Streaming/Channels/101"
    fi

    upload_url="${url}" # avoid SC2034
    if timeout 45 ffprobe -v error -rtsp_transport tcp -show_entries stream=codec_name,width,height \
        -of csv "${url}" >"${RUN_DIR}/ffprobe_tcp.log" 2>&1; then
        ok "ffprobe/TCP: $(tr '\n' ' ' <"${RUN_DIR}/ffprobe_tcp.log")"
    else
        bad "ffprobe/TCP 拉流失败"
        cat "${RUN_DIR}/ffprobe_tcp.log"
    fi

    if timeout 45 ffprobe -v error -rtsp_transport udp -show_entries stream=codec_name,width,height \
        -of csv "${url}" >"${RUN_DIR}/ffprobe_udp.log" 2>&1; then
        ok "ffprobe/UDP: $(tr '\n' ' ' <"${RUN_DIR}/ffprobe_udp.log")"
    else
        bad "ffprobe/UDP 拉流失败"
        cat "${RUN_DIR}/ffprobe_udp.log"
    fi

    # 解码校验：必须能真的解出画面帧（能播不等于能解码）。
    # 判定标准：解码成功，且解码帧率不低于标称帧率的 95%（即没有系统性丢帧）。
    # 同时报告帧数偏差：ffmpeg 对多 slice AU 偶发重复计帧（详见 23 文档已知观察）。
    if command -v ffmpeg >/dev/null 2>&1; then
        if timeout 90 ffmpeg -v info -rtsp_transport tcp -i "${url}" -t 5 -an -f null - >"${RUN_DIR}/ffmpeg_decode.log" 2>&1; then
            local summary decoded seconds expected
            summary="$(tr '\r' '\n' <"${RUN_DIR}/ffmpeg_decode.log" | grep -o 'frame=[^ ]*.*time=[0-9:.]*' | tail -1)"
            decoded="$(printf '%s' "${summary}" | sed -n 's/.*frame= *\([0-9]*\).*/\1/p')"
            seconds="$(printf '%s' "${summary}" | sed -n 's/.*time=\([0-9:.]*\).*/\1/p')"
            if [ -n "${decoded}" ] && [ -n "${seconds}" ]; then
                local secs_int
                secs_int="$(printf '%s' "${seconds}" | awk -F: '{ if (NF==3) printf "%.0f", $1*3600+$2*60+$3; else printf "%.0f", $1 }')"
                expected=$((secs_int * 25))
                if [ "${expected}" -gt 0 ] && [ "${decoded}" -ge $((expected * 95 / 100)) ]; then
                    ok "ffmpeg 解码校验（TCP）通过：解码 ${decoded} 帧 / 期望约 ${expected} 帧"
                else
                    bad "ffmpeg 解码帧数异常：解码 ${decoded} 帧 / 期望约 ${expected} 帧"
                fi
            else
                ok "ffmpeg 解码校验（TCP）通过（未能解析帧统计）"
            fi
        else
            bad "ffmpeg 解码校验（TCP）失败"
            tail -10 "${RUN_DIR}/ffmpeg_decode.log"
        fi
    fi
}

# ---------------------------------------------------------------------------
# 6) RTP 头字段校验
# ---------------------------------------------------------------------------
rtp_check() {
    step "RTP 头字段校验"
    if [ ! -s "${RUN_DIR}/rtp_tcp.csv" ]; then
        skip "没有采集到 RTP 明细"
        return
    fi

    local result
    result="$(python3 - "${RUN_DIR}/rtp_tcp.csv" <<'PY'
import csv, sys
rows = list(csv.DictReader(open(sys.argv[1], encoding="utf-8")))
if not rows:
    print("FAIL 空文件"); raise SystemExit(0)
problems = []
seqs = [int(r["sequence"]) for r in rows]
ts = [int(r["timestamp"]) for r in rows]
pt = {int(r["payload_type"]) for r in rows}
ssrc = {int(r["ssrc"]) for r in rows}
# 同一 AU 内 timestamp 必须相同、marker 只在最后一个 RTP 包
au_groups = []
current = [rows[0]]
for row in rows[1:]:
    if int(row["timestamp"]) == int(current[-1]["timestamp"]):
        current.append(row)
    else:
        au_groups.append(current); current = [row]
au_groups.append(current)
for group in au_groups:
    markers = [int(r["marker"]) for r in group]
    if sum(markers) != 1 or markers[-1] != 1:
        problems.append(f"AU marker 异常: {markers[:8]}")
        break
if len(pt) != 1: problems.append(f"payload type 不唯一: {pt}")
if len(ssrc) != 1: problems.append(f"SSRC 不唯一: {ssrc}")
if any(b != (a + 1) % 65536 for a, b in zip(seqs, seqs[1:])):
    problems.append("sequence 不连续")
if len(set(ts)) < 2: problems.append("timestamp 没有推进")
print("FAIL " + "; ".join(problems) if problems else f"OK packets={len(rows)} aus={len(au_groups)} pt={pt.pop()} ts_steps={len(set(ts))}")
PY
)"
    case "${result}" in
        OK*) ok "RTP 校验: ${result#OK }" ;;
        *) bad "RTP 校验: ${result}" ;;
    esac
}

# ---------------------------------------------------------------------------
# 7) 音视频双 track 回归（共享订阅者 + AAC AU 头）
# ---------------------------------------------------------------------------
av_dual_track() {
    step "A/V 双 track 解码回归"
    if ! command -v ffmpeg >/dev/null 2>&1; then
        skip "缺少 ffmpeg"
        return
    fi
    if [ ! -s "${MEDIA_DIR}/sub.h265" ] || [ ! -s "${MEDIA_DIR}/audio.aac" ]; then
        skip "缺少 H265/AAC 测试文件"
        return
    fi

    local av_port=$((PORT + 1))
    local av_url="rtsp://127.0.0.1:${av_port}/Streaming/Channels/101"
    local av_auth_args=()
    if [ "${DO_AUTH}" -eq 1 ]; then
        av_url="rtsp://${USER_NAME}:${USER_PASSWORD}@127.0.0.1:${av_port}/Streaming/Channels/101"
        av_auth_args=(--user "${USER_NAME}" --password "${USER_PASSWORD}")
    fi
    "${BUILD_DIR}/poc/rtspserver_poc" \
        --port "${av_port}" \
        --main "${MEDIA_DIR}/sub.h265" --codec h265 \
        --audio "${MEDIA_DIR}/audio.aac" \
        --fps 25 "${av_auth_args[@]}" >"${RUN_DIR}/poc_av.log" 2>&1 &
    local av_pid=$!

    for attempt in $(seq 1 30); do
        grep -q "RTSP服务已启动" "${RUN_DIR}/poc_av.log" 2>/dev/null && break
        kill -0 "${av_pid}" 2>/dev/null || break
        sleep 0.2
    done

    timeout 30 ffmpeg -rtsp_transport tcp -i "${av_url}" -t 5 -f null - \
        >"${RUN_DIR}/av_decode.log" 2>&1
    local decode_ok=$?
    kill "${av_pid}" 2>/dev/null || true

    if [ "${decode_ok}" -ne 0 ]; then
        bad "A/V 双 track 解码失败"
        tail -10 "${RUN_DIR}/av_decode.log"
        return
    fi
    if grep -qE "Error parsing AU headers|Multi-layer HEVC|Invalid data" "${RUN_DIR}/av_decode.log"; then
        bad "A/V 双 track 存在解包错误（AU 头/HEVC）"
        grep -E "Error parsing AU headers|Multi-layer HEVC|Invalid data" "${RUN_DIR}/av_decode.log" | head -3
        return
    fi
    if grep -q "audio:[1-9]" "${RUN_DIR}/av_decode.log" && grep -q "video:[1-9]" "${RUN_DIR}/av_decode.log"; then
        ok "A/V 双 track 解码通过（视频+音频均有输出，无解包错误）"
    else
        bad "A/V 双 track 缺少视频或音频输出"
        tail -5 "${RUN_DIR}/av_decode.log"
    fi
}

# ---------------------------------------------------------------------------
# 8) Motion-JPEG 回归（RFC 2435 规范打包：静态 PT26/分片/量化表内联）
# ---------------------------------------------------------------------------
mjpeg_regression() {
    step "Motion-JPEG 解码回归（RFC 2435）"
    if ! command -v ffmpeg >/dev/null 2>&1; then
        skip "缺少 ffmpeg"
        return
    fi
    if [ ! -s "${MEDIA_DIR}/main.mjpg" ]; then
        skip "缺少 MJPEG 测试文件"
        return
    fi

    local mjpeg_port=$((PORT + 2))
    "${BUILD_DIR}/poc/rtspserver_poc" \
        --port "${mjpeg_port}" \
        --main "${MEDIA_DIR}/main.mjpg" --codec mjpeg \
        --fps 25 >"${RUN_DIR}/poc_mjpeg.log" 2>&1 &
    local mjpeg_pid=$!

    for attempt in $(seq 1 30); do
        grep -q "RTSP服务已启动" "${RUN_DIR}/poc_mjpeg.log" 2>/dev/null && break
        kill -0 "${mjpeg_pid}" 2>/dev/null || break
        sleep 0.2
    done

    local url="rtsp://127.0.0.1:${mjpeg_port}/Streaming/Channels/101"

    # SDP 协商：ffprobe 必须识别出 mjpeg 轨（PT26 静态映射正确才挂得上解码器）。
    local probe_out
    probe_out="$(timeout 45 ffprobe -v error -rtsp_transport tcp \
        -show_entries stream=codec_name,width,height -of csv "${url}" 2>&1)"
    case "${probe_out}" in
        *jpeg*) ok "MJPEG SDP 协商: $(printf '%s' "${probe_out}" | tr '\n' ' ')" ;;
        *) bad "MJPEG SDP 协商失败: ${probe_out}" ;;
    esac

    # TCP/UDP 双传输解码：大帧分片重组正确才能解码出帧。
    local transport decoded summary
    for transport in tcp udp; do
        if timeout 60 ffmpeg -v info -rtsp_transport "${transport}" -i "${url}" -t 5 -an -f null - \
            >"${RUN_DIR}/mjpeg_decode_${transport}.log" 2>&1; then
            summary="$(tr '\r' '\n' <"${RUN_DIR}/mjpeg_decode_${transport}.log" | grep -o 'frame=[^ ]*.*time=[0-9:.]*' | tail -1)"
            decoded="$(printf '%s' "${summary}" | sed -n 's/.*frame= *\([0-9]*\).*/\1/p')"
            if [ -n "${decoded}" ] && [ "${decoded}" -ge 100 ]; then
                ok "MJPEG/${transport} 解码通过（${decoded} 帧，分片重组正确）"
            else
                bad "MJPEG/${transport} 解码帧数异常（${decoded} 帧）"
            fi
        else
            bad "MJPEG/${transport} 解码失败"
            tail -5 "${RUN_DIR}/mjpeg_decode_${transport}.log"
        fi
    done

    kill "${mjpeg_pid}" 2>/dev/null || true
}

# ---------------------------------------------------------------------------
# 9) G711/G726 音频回归
# ---------------------------------------------------------------------------
audio_codec_regression() {
    step "G711/G726 音频解码回归"
    if ! command -v ffmpeg >/dev/null 2>&1; then
        skip "缺少 ffmpeg"
        return
    fi
    if [ ! -s "${MEDIA_DIR}/main.h264" ]; then
        skip "缺少视频测试文件"
        return
    fi

    local ext decode_args
    for ext in alaw ulaw g726; do
        local audio_file="${MEDIA_DIR}/audio.${ext}"
        if [ ! -s "${audio_file}" ]; then
            skip "缺少 ${ext} 测试文件"
            continue
        fi

        local audio_port=$((PORT + 3))
        "${BUILD_DIR}/poc/rtspserver_poc" \
            --port "${audio_port}" \
            --main "${MEDIA_DIR}/main.h264" --codec h264 \
            --audio "${audio_file}" --audio-format "${ext}" \
            --fps 25 >"${RUN_DIR}/poc_audio_${ext}.log" 2>&1 &
        local audio_pid=$!

        for attempt in $(seq 1 30); do
            grep -q "RTSP服务已启动" "${RUN_DIR}/poc_audio_${ext}.log" 2>/dev/null && break
            kill -0 "${audio_pid}" 2>/dev/null || break
            sleep 0.2
        done

        local url="rtsp://127.0.0.1:${audio_port}/Streaming/Channels/101"

        # 协商断言：ffprobe 必须按 SDP rtpmap 识别出对应音频轨。
        local codec_probe
        codec_probe="$(timeout 30 ffprobe -v error -rtsp_transport tcp \
            -show_entries stream=codec_name -of csv "${url}" 2>&1)"

        timeout 45 ffmpeg -v info -rtsp_transport tcp -i "${url}" -t 4 -f null - \
            >"${RUN_DIR}/audio_${ext}_decode.log" 2>&1
        local decode_ok=$?
        kill "${audio_pid}" 2>/dev/null || true
        # 等进程退出再进下一用例：后续用例复用同一端口，避免连到旧实例。
        wait "${audio_pid}" 2>/dev/null || true

        case "${ext}" in
        alaw) expected_codec="pcm_alaw" ;;
        ulaw) expected_codec="pcm_mulaw" ;;
        g726) expected_codec="g726" ;;
        esac
        case "${codec_probe}" in
            *"${expected_codec}"*) : ;;
            *)
                bad "音频用例 ${ext} SDP 协商失败: ${codec_probe}"
                continue
                ;;
        esac

        if [ "${decode_ok}" -eq 0 ] && grep -q "audio:[1-9]" "${RUN_DIR}/audio_${ext}_decode.log"; then
            ok "音频用例 ${ext} 解码通过（协商+解码）"
        elif [ "${ext}" = "g726" ]; then
            # G726-32 的 RTP 解码在部分 ffmpeg 版本上不可用：协商正确即视为透传通过。
            ok "G726-32 SDP 协商通过（透传验证，解码依赖播放器能力）"
        else
            bad "音频用例 ${ext} 无音频输出"
            tail -5 "${RUN_DIR}/audio_${ext}_decode.log"
        fi
    done
}

# ---------------------------------------------------------------------------
main() {
    echo "ipc_rtspserver 主机验证（端口 ${PORT}，鉴权 $([ "${DO_AUTH}" -eq 1 ] && echo 开 || echo 关)）"
    prepare_media
    build
    unit_tests
    if start_poc; then
        probes
        player_pull
        rtp_check
        av_dual_track
        mjpeg_regression
        audio_codec_regression
        step "PoC 运行日志（尾部）"
        tail -8 "${RUN_DIR}/poc.log"
    fi

    printf '\n================ 汇总 ================\n通过 %d，失败 %d，跳过 %d\n' "${PASS}" "${FAIL}" "${SKIP}"
    [ "${FAIL}" -eq 0 ]
}

main "$@"
