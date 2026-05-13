#!/bin/sh

# mute pop
echo 60 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio60/direction
echo 0 > /sys/class/gpio/gpio60/value

cd /komod/load3516cv610

./load3516cv610_20s_debug -i -sensor0 sc533hai

echo 1 > /sys/class/gpio/gpio60/value

# sensor pwdn
echo 59 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio59/direction
echo 1 > /sys/class/gpio/gpio59/value
