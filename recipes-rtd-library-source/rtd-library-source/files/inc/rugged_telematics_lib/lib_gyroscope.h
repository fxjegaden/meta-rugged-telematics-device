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
#include <math.h>
#include <semaphore.h>
#include <signal.h>
#include "gyroscope.h"
#include "iio_utils.h"
#include "init.h"
#include "thread.h"
#include "error_nos.h"
#ifdef __cplusplus
extern "C" {
#endif
#define GYRO_MAIN_PATH 		"/sys/bus/iio/devices/iio:device"
#define GYRO_SUB_PATH_SCAN	"/scan_elements"
#define GYRO_SUB_PATH_BUF	"/buffer"
#define GYRO_EVENT_NAME		"gyro"
#define GYRO_INTERRUPT_X_AXIS	"in_anglvel_x_en"
#define GYRO_INTERRUPT_Y_AXIS	"in_anglvel_y_en"
#define GYRO_INTERRUPT_Z_AXIS	"in_anglvel_z_en"
#define GYRO_BUFFER_ENABLE	"enable"

int gyro_enable();
int gyro_disable();
#ifdef __cplusplus
}
#endif