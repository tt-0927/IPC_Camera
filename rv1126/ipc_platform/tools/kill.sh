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

# 查找进程ID并发送信号（排除 grep 本身）无pgrep命令，使用ps命令
PROCESS_LINES=$(ps | grep "$PROC_NAME" | grep -v grep)

if [ -z "$PROCESS_LINES" ]; then
    echo "No process found with name: $PROC_NAME"
    exit 0
fi

# 遍历每一行进程信息，并使用 awk 提取第一列作为 PID
for PID in $(echo "$PROCESS_LINES" | awk '{print $1}'); do
    echo "Sending signal $SIGNAL_NUM to PID$PID ($PROC_NAME)"
    if ! kill -"$SIGNAL_NUM" "$PID"; then
        echo "Failed to send signal $SIGNAL_NUM to PID$PID" >&2
    fi
done
