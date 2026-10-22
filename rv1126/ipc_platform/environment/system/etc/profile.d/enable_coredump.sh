#!/bin/sh
#减小线程栈空间
ulimit -s 3072   # 设3MB

#禁用核心转储
ulimit -c 0
#ulimit -c unlimited
#echo "/data/core-%p-%e" > /proc/sys/kernel/core_pattern
#if [ ! "emmc" = "emmc" ];then
#echo "| /usr/bin/coredump2sdcard.sh %p %e" > /proc/sys/kernel/core_pattern
#fi
