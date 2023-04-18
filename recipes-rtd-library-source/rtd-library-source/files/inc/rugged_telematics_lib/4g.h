#ifndef __4G_HEADER__
#define __4G_HEADER__
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "gsm.h"
#include "gps.h"
#include "q_gps.h"
#include "init.h"
#include "obd2lib.h"
#include "thread.h"
#include "error_nos.h"

#define MXC_0				"/dev/ttymxc0"
#define GSM_GPS_PORT			"/dev/ttyUSB0"
#define GSM_LOG_PORT			"/dev/ttyUSB1"
#define GSM_AT_PORT			"/dev/ttyUSB2"
#define GSM_CONNECTION_PORT		"/dev/ttyUSB3"
#define GSM_AT_PORT_BAUDRATE		115200

#define READ_MODULE_ID			"AT+CGMM"
#define READ_SIMSTATUS			"AT+CPIN?"
#define SET_DISABLE_ECHO		"ate0" 
#define SET_FLIGHT_MODE_ON		"AT+CFUN=0"
#define SET_FLIGHT_MODE_OFF		"AT+CFUN=1"
#define READ_IMEI			"at+gsn"
#define READ_ICCID			"at+qccid"
#define READ_GSM_SIGNAL_STRENGTH	"at+csq"
#define READ_GSM_SIM_REGISTRATION	"at+creg?"
#define SET_GSM_SIM_REG_WITH_LOC_ON	"at+creg=2"
#define SET_GSM_SIM_REG_WITH_LOC_OFF	"at+creg=0"
#define SET_MODE_2G			"at+qcfg=\"nwscanmode\",1,1"
#define SET_MODE_3G			"at+qcfg=\"nwscanmode\",2,1"
#define SET_MODE_4G			"at+qcfg=\"nwscanmode\",3,1"
#define SET_AUTO_MODE			"at+qcfg=\"nwscanmode\",0,1"

#define LENGTH				25
#define MIN_FD				2
#define COMMAND_LEN			20
#define MEM_CLR				0x0
#define MAX_MSG_LEN			160
#define CTRL_Z				26

sem_t* gsm_at_cmd_lock;

enum sim_mode{
	MODE_TYPE_AUTO = 1,
	MODE_TYPE_2G,
	MODE_TYPE_3G,
	MODE_TYPE_4G,
};

extern int sim_status;
extern int sim_status_monitor;

int check_gsm_sim_registration();
int set_gsm_nw_reg_loc_on();
int set_gsm_nw_reg_loc_off();
int establish_nw_connection();

/* Network Monitoring APIs*/
int check_gsm_nw_connection_monitor();
int set_gsm_flight_mode_on_monitor();
int set_gsm_flight_mode_off_monitor();
int gsm_modem_on_monitor(char*, int);
int gsm_modem_off_monitor();
int get_gsm_sim_status_monitor();
int check_gsm_sim_registration_monitor();
int set_gsm_network_mode_monitor (int);
int get_gsm_signal_strength_monitor(char *, int);
int get_gsm_nw_reg_monitor(char *, int, char *, int);
int set_gsm_nw_reg_loc_on_monitor();
int set_gsm_nw_reg_loc_off_monitor();
int check_gsm_modem_status_monitor();
int establish_nw_connection_monitor();
int gsm_apn_config_read(char* apn_val);
int gsm_at_port_close( );
int read_event( );
int link_listener(void);

#endif //__4G_HEADER__
