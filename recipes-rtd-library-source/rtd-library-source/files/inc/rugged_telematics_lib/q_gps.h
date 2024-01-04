#ifndef __QGPS__
#define __QGPS__

#include <termios.h>
#include <stddef.h>
#include "serial.h"
#ifdef __cplusplus
extern "C" {
#endif
#define TIME_STAMP		1
#define FIX_STATUS		2
#define CUR_LATITUDE		3
#define HEMISPHERE		4
#define CUR_LONGITUDE		5
#define GREENWICH		6
#define KNOT_SPEED		7
#define DIRECTION		8
#define FIELD_SIZE		100
#define GPRS_POWER_ON		5
#define GPS_INIT		"AT+QGPS=1"
#define GPS_DEINIT		"AT+QGPS=0"

#define AGPS_STATUS		"AT+QGPSXTRADATA?"
#define AGPS_ENABLE		"AT+QGPSXTRA=1"
#define AGPS_DEL_EXIST_DATA	"AT+QFDEL=\"xtra2.bin\""
#define AGPS_HTTP_CFG		"AT+QHTTPCFG=\"contextid\",1"
#define AGPS_QIACT_ENABLE	"AT+QIACT=1"
#define AGPS_URL_SIZE		"AT+QHTTPURL=40,80"
#define AGPS_URL		"http://xtrapath4.izatcloud.net/xtra2.bin"
#define AGPS_DOWNLOAD_DATA	"at+qhttpget"
#define AGPS_READ_DOWNLOAD_DATA	"at+qhttpreadfile=\"xtra2.bin\""
#define AGPS_LOAD_DOWNLOAD_DATA	"at+qgpsxtradata=\"xtra2.bin\""
#define AGPS_QFLST		"AT+QFLST=\"*\""
#define AGPS_QNTP		"AT+QNTP=1,\"in.pool.ntp.org\""
/* NaN : This is required to pass from hw_deinit */

int board_init_gps();
#ifdef __cplusplus
}
#endif
#endif
