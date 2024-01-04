#ifndef __LIB_ACCELEROMETER_H__
#define __LIB_ACCELEROMETER_H__

#include <math.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <linux/types.h>
#include <string.h>
#include <poll.h>
#include <endian.h>
#include <getopt.h>
#include <inttypes.h>
#include <semaphore.h>
#include <signal.h>
#include "accelerometer.h"
#include "init.h"
#include "thread.h"
#include "error_nos.h"
#include "iio_utils.h"
#include "obd2lib.h"
#include "lib_common.h"
#ifdef __cplusplus
extern "C" {
#endif
#define ACC_I2C_NO		1

#define ACC_MAIN_PATH		"/sys/bus/iio/devices/iio:device"
#define ACC_SUB_PATH_SCAN	"/scan_elements"
#define ACC_SUB_PATH_BUF	"/buffer"
#define ACC_EVENT_NAME		"accel"
#define ACC_INTERRUPT_X_AXIS	"in_accel_x_en"
#define ACC_INTERRUPT_Y_AXIS	"in_accel_y_en"
#define ACC_INTERRUPT_Z_AXIS	"in_accel_z_en"
#define ACC_BUFFER_ENABLE	"enable"
#define CRASH_THRESHOLD		1
#define ACC_SLAVE_ADDR          0x6A
#define SENSOR_CTRL_REG1        0x10
#define SENSOR_CTRL_REG8        0x17
#define ACC_THRESHOLD_REG       0x59
#define SENSOR_INT_DUR2         0x5A
#define SENSOR_WAKE_UP_THS      0x5B
#define SENSOR_MD1_CFG          0x5E
#define SENSOR_TAP_CFG          0x58
#define SENSOR_INT1_CTRL        0x0D

int acc_sensitivity();
void process_scan_acc(char *, struct iio_channel_info *, int);
void print2byte_acc(int, struct iio_channel_info *, int);
void acc_sem_init (void) __attribute__ ((constructor));
void acc_sem_deinit (void) __attribute__ ((destructor));
#ifdef __cplusplus
}
#endif
#endif
