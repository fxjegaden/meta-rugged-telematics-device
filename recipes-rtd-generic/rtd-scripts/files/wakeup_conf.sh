echo 1 > /sys/bus/iio/devices/iio\:device1/scan_elements/in_accel_x_en
echo 1 > /sys/bus/iio/devices/iio\:device1/scan_elements/in_accel_y_en
echo 1 > /sys/bus/iio/devices/iio\:device1/scan_elements/in_accel_z_en
echo 1 > /sys/bus/iio/devices/iio\:device1/buffer/enable
i2cset -f -y 1 0x6a 0x0d 0x01
i2cset -f -y 1 0x6a 0x0d 0x00
echo 0 > /sys/bus/iio/devices/iio\:device1/buffer/enable
echo 0 > /sys/bus/iio/devices/iio\:device1/scan_elements/in_accel_z_en
echo 0 > /sys/bus/iio/devices/iio\:device1/scan_elements/in_accel_y_en
echo 0 > /sys/bus/iio/devices/iio\:device1/scan_elements/in_accel_x_en
