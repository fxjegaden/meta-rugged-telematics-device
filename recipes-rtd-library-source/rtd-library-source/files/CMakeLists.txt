cmake_minimum_required(VERSION 3.10)
project(RuggedTelematics C)

set(CMAKE_C_STANDARD 11)

include_directories(
    inc/libcommon
    inc/rugged_telematics_lib
    inc/libxml
)

set(SOURCES
    src/4g.c
    src/4g_monitor.c
    src/acc_ism330dlc.c
    src/battery.c
    src/ble.c
    src/can_comm.c
    src/can_lib.c
    src/eth.c
    src/get.c
    src/gpio.c
    src/gyro_ism330dlc.c
    src/i2c_framing.c
    src/iio_utils.c
    src/init.c
    src/mag.c
    src/ntp.c
    src/power_management.c
    src/rs232.c
    src/serial.c
    src/thread_def.c
    src/u_gps.c
    src/wifi.c
)

add_library(RuggedTelematics STATIC ${SOURCES})
target_link_libraries(RuggedTelematics m dl pthread)