#ifndef __SERIAL_H_
#define __SERIAL_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <termios.h>
#include "lib_common.h"
#ifdef __cplusplus
extern "C" {
#endif
#define SERIAL_PORT_ACM0	(0)
#define RS232_UART		"/dev/ttymxc5"
#define USB_2			"/dev/ttyUSB2"
#define DATABITS		8
#define PARITY			'N'
#define STOPBITS		1
#define HANDSHAKE		1
#define INFINITE_TIMEOUT	0
#define SERIAL_FAILURE		-1
#define SERIAL_WAIT_TIMEOUT	-2
#define CTRL_Z			26

int uart_init (const char *device, int baudrate);

struct obd_ {
	double res[255];
	float volt;
	char res_buf[255];
};

typedef struct dtccodes{
	char *dtc_str[500];
	int no_codes;	
}dtccodes;

int obd_read_data(char *,char *,struct obd_ *);
int parse_raw_value(char *,char *);
int config_sleep_wake_trigger_off();
int config_sleep_wake_trigger_on();

int iW_Serial_SetBaudrate( struct termios* TermiosPtr, int Baudrate);
int iW_Serial_SetDatabits( struct termios *TermiosPtr, int databits);
int iW_Serial_SetStopbits( struct termios *TermiosPtr, int Stopbits);
int iW_Serial_SetParity( struct termios *TermiosPtr, char Parity);
int iW_Serial_Init(int Serial_Port_Number,int baudrate, const char *node);
int iW_Serial_Read(int fd , char *buf, long int sz);
int iW_Serial_Write(int fd , char *buf, size_t sz);
#ifdef __cplusplus
}
#endif
#endif
