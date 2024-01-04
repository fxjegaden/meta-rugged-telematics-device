#include <linux/input.h>
#include <magnetometer.h>
#ifdef __cplusplus
extern "C" {
#endif
#define MAGNETOMETER_ENABLE 	"echo 1 > /sys/devices/virtual/input/input2/enable"
#define MAGNETOMETER_DISABLE 	"echo 0 > /sys/devices/virtual/input/input2/enable"
#define MAG_MAIN_PATH		"/sys/bus/iio/devices/iio:device"
#define MAG_SUB_PATH_SCAN	"/scan_elements"
#define MAG_SUB_PATH_BUF	"/buffer"
#define MAG_EVENT_NAME		"magn"
#define MAG_INTERRUPT_X_AXIS	"in_magn_x_en"
#define MAG_INTERRUPT_Y_AXIS	"in_magn_y_en"
#define MAG_INTERRUPT_Z_AXIS	"in_magn_z_en"
#define MAG_BUFFER_ENABLE	"enable"
#define IIS2MDC_SLAVE_ADDR	0x1e
#define IIS2MDC_CFG_REG_A	0x60
#define PI			3.141592
#ifdef __cplusplus
}
#endif