START_FILE=/var/run/ssh_service_start.time
EXPIRY_SECONDS=28800 # 8小时
CURRENT_TIME=$(date +%s)
START_TIME=$(cat "$START_FILE" 2>/dev/null || echo 0)
ELAPSED=$((CURRENT_TIME - START_TIME))
if [ "$ELAPSED" -ge "$EXPIRY_SECONDS" ]; then
    killall dropbear
    # 清理文件
    rm -f "$START_FILE"
    rm -f "$0"
fi