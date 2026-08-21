#!/bin/sh

# mute pop
echo 10 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio10/direction
echo 0 > /sys/class/gpio/gpio10/value

cd /komod/load3516cv610

#改为load3516cv610_00s_debug
./load3516cv610_00s_debug -i -sensor0 sc533hai

echo 10 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio10/direction
echo 1 > /sys/class/gpio/gpio10/value

# sensor pwdn
echo 59 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio59/direction
echo 1 > /sys/class/gpio/gpio59/value
