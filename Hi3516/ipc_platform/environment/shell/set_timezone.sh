#!/bin/sh
# 设置系统时区，必须用 source 方式调用：. /opt/cam/shell/set_timezone.sh
# POSIX 时区偏移符号和 UTC 展示符号相反，例如东八区使用 CST-8。

TIMEZONE_CONF="/opt/cam/shell/timezone.conf"
DEFAULT_TZ="CST-8"

if [ -n "$1" ]; then
    TIMEZONE="$1"
    echo "$TIMEZONE" > "$TIMEZONE_CONF"
else
    if [ -f "$TIMEZONE_CONF" ]; then
        TIMEZONE=$(cat "$TIMEZONE_CONF" 2>/dev/null)
    fi

    if [ -z "$TIMEZONE" ]; then
        TIMEZONE="$DEFAULT_TZ"
        echo "$TIMEZONE" > "$TIMEZONE_CONF"
    fi
fi

export TZ="$TIMEZONE"
echo "System timezone set to: $TIMEZONE"
