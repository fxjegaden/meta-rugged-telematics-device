#ifndef __OBD2LIB__
#define __OBD2LIB__
#include "serial.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _serial_intf{
	int tty_fd;
	fd_set read_fds;
	fd_set write_fds;
	fd_set except_fds;
}_serial_intf;

typedef struct _exe_time{
	int duration;
	int gps_duration;
	int car_duration;
	int acc_duration;
	int dtc_duration;
	int gyro_duration;
	int mag_duration;
}_exe_time;

typedef struct _libClient{
	sem_t sys_wake;
	sem_t ign_off_restart;
	sem_t ign_of_sem;
	sem_t ign_of_dis;
	sem_t ign_t_sem;
	sem_t btry_drain;
	sem_t sem_ppp0_link_down;
	sem_t sem_board_init;
	sem_t gps_sem;
	sem_t ign_sem;
	sem_t obd_sem;
	sem_t ign_off_sem;
	mqd_t pwr_mgmt_qid;
	int state;
	int init_connect;
	int powermgmt;
	int car_mode;
	int lab_mode;
	int obdflag;
	int ppp0_link_status;
	int system_wake_timer;
	int fresh_boot;
	int at_fd;
	int gyro_mounted;
	int gps_init_fix;
	int dev_sleep;
	int check_raw_can;
	int set_raw_can;
	int system_wake_acc;
	int system_wake_can;
	int ign_pin_status;
	int disconn_pin_status;
#ifdef _XLS__EN_
	_xls xls_elem;
#else
	_xml xml_elem;
#endif	
	_exe_time run_time;
	_serial_intf serial_intf;	
	IGN_PTRS ign_fptr;
	SLEEP_WAKE s_w; 
	pthread_t tid[10];//0-pm, 1-link_thread, 2-link_status, 3-key_event, 4-ignition_status, 5-board
	sem_t sem_board_init_complete;
}_libClient;

_libClient libClient;

#define DEV_WAKE 0
#define DEV_SLEEP 1

#define LINK_DOWN 0
#define LINK_UP 1
#ifdef __cplusplus
}
#endif
#endif
