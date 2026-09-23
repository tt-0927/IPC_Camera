#!/usr/bin/env bash
# 校验 libevent-openipc staging SDK 是否可以作为 RTSP 第一阶段依赖。
# 用法：./verify_libevent_sdk.sh <sdk-dir> [expected-target]

set -euo pipefail

if (($# < 1 || $# > 2)); then
    echo "用法：$0 <libevent-sdk-dir> [expected-target]" >&2
    exit 2
fi

SDK_DIR="$1"
EXPECTED_TARGET="${2:-}"
MANIFEST="${SDK_DIR}/manifest.json"
CORE_ARCHIVE="${SDK_DIR}/lib/libevent_core.a"
CONFIG_HEADER="${SDK_DIR}/include/event2/event-config.h"

for required in "$MANIFEST" "$CORE_ARCHIVE" "$CONFIG_HEADER"; do
    if [[ ! -f "$required" ]]; then
        echo "缺少 SDK 文件：$required" >&2
        exit 1
    fi
done

python3 - "$MANIFEST" "$EXPECTED_TARGET" <<'PY'
import json
import sys
from pathlib import Path

manifest_path = Path(sys.argv[1])
expected_target = sys.argv[2]
data = json.loads(manifest_path.read_text(encoding="utf-8"))

if data.get("name") != "libevent-openipc":
    raise SystemExit("manifest name 不是 libevent-openipc")
if data.get("commit") != "694decef35717d8955aa34ba4d2baaaf61c9e4a9":
    raise SystemExit("manifest commit 与 OpenIPC fork pin 不一致")
if data.get("library_type") != "STATIC":
    raise SystemExit("第一阶段要求 STATIC libevent")
if expected_target and data.get("target") != expected_target:
    raise SystemExit(
        f"SDK target 不匹配：实际 {data.get('target')!r}，期望 {expected_target!r}"
    )
if data.get("target") in ("hi3516cv610", "arm-v01c02-linux-musleabi") and data.get("broken_mmap") is not True:
    raise SystemExit("Hi3516 SDK 未启用 broken_mmap profile")

print(
    "SDK OK:",
    data.get("target"),
    "commit=", data.get("commit"),
    "broken_mmap=", data.get("broken_mmap"),
)
PY

if ! ar t "$CORE_ARCHIVE" | grep -q .; then
    echo "libevent_core.a 为空" >&2
    exit 1
fi

echo "libevent SDK 校验通过：$SDK_DIR"
