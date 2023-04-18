#ifndef __PM__
#define __PM__

#include "lib_can.h"
#include "lib_common.h"
#include "lib_accelerometer.h"
#include "mcu_i2c_framing.h"

#define TIMER_WAKEUPALARM_FILE_NAME	"/sys/class/rtc/rtc0/wakealarm"
#define RTC_WAKEUPALARM_FILE_NAME	"/dev/rtc1"
#define ACC_WAKEUP_FILE_NAME		"/sys/bus/i2c/devices/i2c-1/1-006a/power/wakeup"
#define CAN0_WAKEUP_FILE_NAME		"/sys/devices/soc0/soc/2000000.aips-bus/2090000.flexcan/power/wakeup"
#define CAN1_WAKEUP_FILE_NAME		"/sys/devices/soc0/soc/2000000.aips-bus/2094000.flexcan/power/wakeup"
#define CAN2_WAKEUP_FILE_NAME		"/sys/devices/soc0/soc/2000000.aips-bus/2000000.spba-bus/2008000.spi/spi_master/spi0/spi0.0/power/wakeup"
#define IGN_WAKEUP_FILE_NAME		"/sys/devices/soc0/gpio-keys/power/wakeup"
#define MCU_WAKEUP_FILE_NAME		"/sys/devices/soc0/gpio-keys1/power/wakeup"

#define USB_WAKEUP_FILE_PATH		"/sys/bus/platform/drivers/imx_usb/2184200.usb/"
#define USB_WAKEUP_FILE_NAME		"/power/wakeup"
#define LSUSB_CMD			"lsusb"
#define QUECTEL_ID			" ID 2c7c"

#define AT_GSM_MSG_ENABLE		"at+cmgf=1"
#define AT_GSM_MSG_DISABLE		"at+cmgf=0"
#define AT_GSM_SLP_ENABLE		"at+qsclk=1"
#define AT_GSM_SLP_DISABLE		"at+qsclk=0"

#define MCU_RESET_GPIO		129
#define MCU_SBL_MODE_ADDR	0x18
#define MCU_SBL_MODE_REG_ADDR	0xA3

#define TIMER_WAKEUP_ENABLE	1 
#define ACC_WAKEUP_ENABLE	1
#define CAN_WAKEUP_ENABLE	1
#define IGN_WAKEUP_ENABLE	1
#define MCU_WAKEUP_ENABLE	1
#define SMS_WAKEUP_ENABLE	1

/* Configuration for the NXP RTC chip */
#define RTC_BUS_NO		1
#define RTC_SLAVE_ADDR		0x51
#define TIME_SEC_REG		0x01
#define TIME_MIN_REG		0x02
#define TIME_HOUR_REG		0x03
#define TIME_DAY_REG		0x04
#define TIME_WEEKDAY_REG	0x05
#define TIME_MONTH_REG		0x06
#define TIME_YEAR_REG		0x07
#define DATE_PATH		"/sys/class/rtc/rtc1/date"
#define TIME_PATH		"/sys/class/rtc/rtc1/time"

#define REG_RTC_ALARM_ENABLE	0x10
#define REG_FLAGS		0x2B

#define REG_RTC_ALARM1_SECOND	0x08
#define REG_RTC_ALARM1_MINUTE	0x09
#define REG_RTC_ALARM1_HOUR	0x0A
#define REG_RTC_ALARM1_DAY	0x0B
#define REG_RTC_ALARM1_MONTH	0x0C
#define REG_PIN_IO		0x27
#define REG_INTA_ENABLE		0x29

#define REG_RTC_ALARM_ENABLE_A1E_MASK	0b00011111
#define REG_RTC_ALARM_ENABLE_A1E_SECOND	0b00000001
#define REG_RTC_ALARM_ENABLE_A1E_MINUTE	0b00000010
#define REG_RTC_ALARM_ENABLE_A1E_HOUR	0b00000100
#define REG_RTC_ALARM_ENABLE_A1E_DAY	0b00001000
#define REG_RTC_ALARM_ENABLE_A1E_MONTH	0b00010000

#endif
