#ifndef __WIFI_BLE_HEADER__
#define __WIFI_BLE_HEADER__

#include <stdio.h>
#include "init.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "lib_common.h"
#include "obd2lib.h"

#define WIFI_HOST_MODE 1
#define WIFI_STA_MODE 0

int wifi_mode;

int set_wifi_hostapd_mode( );
int set_wifi_station_mode( );

#endif
