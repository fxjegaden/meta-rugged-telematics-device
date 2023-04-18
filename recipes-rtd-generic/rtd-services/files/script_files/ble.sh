#!/bin/sh
echo 133 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio133/direction
echo 0 > /sys/class/gpio/gpio133/value
sleep 1
echo 1 > /sys/class/gpio/gpio133/value
sleep 1
hciattach /dev/ttymxc3 bcm43xx 3000000 noflow -t 20
sleep 2
hciconfig hci0 up
pulseaudio --start
sleep 1
systemctl start bluetooth.service
sleep 1
hciconfig hci0 piscan
sleep 1
sdptool add --channel=2 SP
rfcomm --raw listen /dev/rfcomm0 2 &
