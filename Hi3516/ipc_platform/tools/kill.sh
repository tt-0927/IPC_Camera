#!/bin/sh

# 检查是否传入了参数
if [ -z "$1" ]; then
    echo "Usage: $0 <process_name> [signal_number]"
    echo "Default signal is 15 (SIGTERM)"
    exit 1
fi

PROC_NAME=$1
SIGNAL_NUM=${2:-15} # 使用参数替换设置默认值为15 (SIGTERM)

# 验证信号是否为数字
if ! echo "$SIGNAL_NUM" | grep -q '^[0-9]\+$'; then
    echo "Error: Signal number must be an integer"
    exit 1
fi

# 查找进程ID并发送信号（排除 grep 本身）
PIDS=$(pgrep "$PROC_NAME")

if [ -z "$PIDS" ]; then
    echo "No process found with name: $PROC_NAME"
    exit 0
fi

for PID in $PIDS; do
    echo "Sending signal $SIGNAL_NUM to PID $PID ($PROC_NAME)"
    if ! kill -"$SIGNAL_NUM" "$PID"; then
        echo "Failed to send signal $SIGNAL_NUM to PID $PID" >&2
    fi
done
