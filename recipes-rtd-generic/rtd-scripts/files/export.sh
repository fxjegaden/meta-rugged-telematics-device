#12v register
echo 137 > /sys/class/gpio/export
#Battery Status
echo 118 > /sys/class/gpio/export
#Battery Charge Enable
echo 120 > /sys/class/gpio/export
#LED
echo 73 > /sys/class/gpio/export
#Battery Power Good
echo 64 > /sys/class/gpio/export
#Modem
echo 90 > /sys/class/gpio/export
echo 78 > /sys/class/gpio/export
echo 88 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio137/direction
echo out > /sys/class/gpio/gpio120/direction
echo out > /sys/class/gpio/gpio73/direction
echo in > /sys/class/gpio/gpio118/direction
echo in > /sys/class/gpio/gpio64/direction
echo out > /sys/class/gpio/gpio90/direction
echo out > /sys/class/gpio/gpio78/direction
echo out > /sys/class/gpio/gpio88/direction
