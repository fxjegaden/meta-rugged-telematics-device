#ifndef __INIT__
#define __INIT__
#include "lib_common.h"
#include "obd2lib.h"
#include "4g.h"
#ifdef __cplusplus
extern "C" {
#endif
#define CAR
#define CPU_UNIQUE_ID_LO	"/sys/fsl_otp/HW_OCOTP_CFG0"
#define CPU_UNIQUE_ID_HI	"/sys/fsl_otp/HW_OCOTP_CFG1"
#define ADC_READ_NODE		"/sys/bus/iio/devices/iio:device0/in_voltage1_raw"
#define IN_BT_READ_NODE		"/sys/bus/iio/devices/iio:device0/in_voltage5_raw"
#define I2C_READ		"i2cget -f -y"
#define I2C_WRITE		"i2cset -f -y"
#define i2C_BUS_1		1
#define i2C_BUS_0		0
#ifdef ENABLE_OBD_DEBUG
#define IOBD_DEBUG(...)\
{\
	printf("DEBUG: %s L#%d ", __func__, __LINE__); \
	printf(__VA_ARGS__); \
	printf("\n"); \
}			
#else		
#define IOBD_DEBUG(...)
#endif

#define DEVICE_SLEEP_PATH		"/sys/power/state"
#define ETH0_CARRIER_PATH		"/sys/class/net/eth0/carrier"
#define ETH0_ADDRESS_PATH		"/sys/class/net/eth0/address"
#define DEBUG_PRINT			0
#define DEBUG_PRINT_RES			1

#define MAX_MESSAGES			100
#define MAX_MSG_SIZE			256
#define SUCCESS				0
#define FAILURE				-1
#define HW_INIT_ALL			1
#define HW_NO_INIT_ALL			0
#define HW_DEINIT_ALL			1
#define HW_NO_DEINIT_ALL 		0
#define LED_GPIO			73
#define THREE_V3_GPIO			91
#define ACC_ERR_CRASH_DETECTED		1
#define LSM6DSL_SLAVE_ADDR		0x6a
#define LSM6DSL_CTRL_REG1		0x10
#define LSM6DSL_CTRL_REG8		0x17
#define LSM6DSL_ACC_THRESHOLD_REG	0x59
#define LSM6DSL_INT1_CTRL		0x0d
#define LSM6DSL_INT2_CTRL		0x0e
#define LSM6DSL_TAP_CFG			0x58
#define LSM6DSL_CTRL_REG2		0x11
#define LSM6DSL_CTRL_REG6		0x15
#define LSM6DSL_FIFO_CTRL5		0x0A
#define LSM6DSM_OUT_TEMP_L		0x20U
#define LSM6DSM_OUT_TEMP_H		0x21U
#define LSM6DSL_CTRL3_C_REG		0x12
#define ACC_INTERRUPT_GPIO		65

#define REBOOT_GPIO			137
#define TEMPERATURE_OFFSET		25

int ign_sleep;
int timer_state;
int network_status;
int network_enable_status;
int sim_status_monitor;

struct ign_stat
{
	int msg_type;
	char data[MAX_MSG_SIZE];
};

int send_ign_q(_libClient * libclient,struct ign_stat *);
int init_ign_dis_handler(void);
int hw_init(int, char *, int, int*);
int hw_deinit(int, char *, int);
int hw_init_interfaces(int , int);
int hw_deinit_interfaces(int);
int start_pm_client();
int init_ign_q(_libClient *);
int obd_init();
char *trim(char *);
void sigHandler(int);
void get_imei(char *);
int frame_imei(char *,int *,char *);
int check_mode(void);
int check_protocol(void);
int check_ign_status();
int check_connection();
void ignition_thread(void);
void dtc_code(char *);
void car_status(char *);
char *get_disconnect_status(void);
int get_serial_no(void);
int check_in_bt_volt(double *);
void sys_sleep_completed(void);
void sys_wake_completed(void);
void server_connection_complete(void);
int check_if_modem_on();
int wakeup_interrupt_config(int);
int put_device_to_sleep(int, char *, int);
int wake_up_device();
int three_v3_on();
int three_v3_off();
void enable_only_rmc();
int time_diff_seconds(struct timespec *time_end, struct timespec *time_start);
int network_manager();
#ifdef __cplusplus
}
#endif
#endif
