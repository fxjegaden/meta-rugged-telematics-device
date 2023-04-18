#ifndef __LIB_COMMON_H__
#define __LIB_COMMON_H__

#include <stdio.h>
#include <netinet/in.h>
#include <signal.h>
#include <semaphore.h>
#include <mqueue.h>
#include <sys/time.h>
#include <stdbool.h>
#include "common.h"
#include "error_nos.h"
#include "gpio.h"
#include "debug.h"

#define READ_MAC_NODE		"/sys/class/"

#define MODE1			0x1
#define MODE2			0x2
#define MODE3			0x3
#define MODE4			0x4
#define MODE5			0x5
#define MODE6			0x6
#define MODE7			0x7
#define MODE8			0x8
#define MODE9			0x9

#define FREQ_GPSUPDATE		0x0
#define FREQ_CARSTATUS		0x1
#define FREQ_ACCELEROMETER	0x2
#define FREQ_DTCCODE		0x3
#define FREQ_GYROSCOPE		0x4
#define FREQ_ANALYTICS		0x5

#define FREQ_ELEMENTS 10

typedef struct frequency
{
        char name[64];
        char select[5];
        int value;
}frequency;

typedef struct mode_pid
{
	double data;
	short int mode;
	char pid[10];
	char topic[100];
	char description[40];
	char raw_data[64];
}mode_pid;

typedef struct car_parameters{
	int no_of_pids;
	mode_pid * modepid;
}car_parameters;

#ifdef _XLS__EN_
typedef struct _xls{
	frequency freq[FREQ_ELEMENTS];
	car_parameters car_params;
}_xls;

#else
typedef struct _xml{
	frequency freq[FREQ_ELEMENTS];
	car_parameters car_params;
}_xml;	
#endif

typedef void(*SLEEP)(void);
typedef struct slp_wkp_ptrs
{ 
	SLEEP slp;
	SLEEP wake;
	SLEEP slp_dis;
	SLEEP timer_wake;
}SLEEP_WAKE; 

typedef void(*FPTR)(char *);
typedef void(*FPTR_1)(char *, int);
typedef struct ign_fun_ptrs
{
	FPTR ign_ON;
	FPTR ign_OFF;
	FPTR_1 dis_stat;
	FPTR bat_drain;
	FPTR can_wake;
	FPTR crash_det;
}IGN_PTRS;

typedef struct _IO_PIN_STATUS_{
	short din1;
	short din2;
	short dout1;
	short dout2;
	short ain1;
}IO_PIN_STATE;

#define MAX_MSG_SIZE		256

typedef enum mode1_pid{
	ENGINE_LOAD = 0x04,
	ENGINE_COOLANT_TEMP,
	RPM = 0x0C,
	VEHICLE_SPEED,
	MAF_RATE = 0x10,
	RUNTIME_ENGINE_START = 0x1F,
	FUEL_LEVEL = 0x2F,
	DISTANCE_DTC_CLEAR = 0x31,
	CTRL_MODULE_VOLT = 0x42,
	AIR_TEMP = 0x46,
	OIL_TEMP = 0x5C,
	FUEL_RATE = 0x5E
}mode1_pid;

typedef enum mode9_pid{
	CMD_VIN = 0x02
}mode9_pid;

#define ON			1
#define OFF			0
#define MAX_MESSAGES		100
#define MAX_MSG_SIZE		256

#define IW_OBD_SUCCESS 0
#define IW_OBD_FAILURE -1
#define PRGST_CAR_BAT_NODE "/sys/bus/iio/devices/iio:device0/in_voltage1_raw"

extern int acc_init_success;

int ext_bat_test();
int prgst_read_car_bat_volt(float *voltage_value);
int CheckLink(char *ifname);

/* Ethernet Related APIs */
int read_fused_mac_addr(char *mac_addr);
int Check_eth_Link();

#endif /* __LIB_COMMON_H__ */
