#ifndef __LIB_BATTERY_H__
#define __LIB_BATTERY_H__

#include <unistd.h>
#include <poll.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <linux/i2c-dev.h>
#include "battery.h"
#include "init.h"
#include "error_nos.h"
#include "iio_utils.h"
#include "lib_common.h"
#include "lib_accelerometer.h"
#ifdef __cplusplus
extern "C" {
#endif
#define IBAT_I2C_NO			1
#define IBATTERY_NODE			"/sys/class/power_supply/bq25601-battery"
#define IBATTERY_VOLTAGE		"/sys/bus/iio/devices/iio:device0"
#define BATTERY_STATUS_GPIO		118
#define BATTERY_PG_GPIO			64
#define BATTERY_CE_GPIO			120

/* For Changing the Battery Discharge */
#define BAT_OFF_WITH_TIMER		0
#define BAT_ON				1
#define BAT_OFF_WITHOUT_TIMER		2

#define i2C_BUS_1			1

#define BATTERY_SLAVE_ADDR		0x6b
#define BATTERY_STATUS_REG_ADDR		0x08
#define BATTERY_TEMP_STATUS_REG_ADDR	0x09
#define BATTERY_CNTRL_REG1		0x00
#define BATTERY_CNTRL_REG2		0x02
#define BATTERY_CNTRL_REG3		0x03
#define BATTERY_CNTRL_REG4		0x05
#define IBATTERY_STRENGTH_GOOD		0x1001
#define IBATTERY_STRENGTH_NOT_GOOD	0x1002
#define IBATTERY_NOT_CONNECTED		0x1003
#define IBATTERY_FULLY_CHARGED		0x1004
#define IBATTERY_CHARGING		0x1005
#define IBATTERY_NOT_CHARGING		0x1006
#define IBATTERY_UNSTABLE_STATE		0x1007
#define EXTERNAL_POWER			0x1008
#define INTERNAL_POWER			0x1009
#define IBATTERY_NORMAL_TEMP		0x100A
#define IBATTERY_WARM_TEMP		0x100B
#define IBATTERY_COOL_TEMP		0x100C
#define IBATTERY_COLD_TEMP		0x100D
#define IBATTERY_HOT_TEMP		0x100E

/* API to control the Battery based on the temperature */
int battery_control_monitoring( uint32_t battery_monitoring_interval );
#ifdef __cplusplus
}
#endif
#endif
