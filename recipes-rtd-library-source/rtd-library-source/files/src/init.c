#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <string.h>
#include <linux/i2c-dev.h>
#include <stdint.h>
#include <errno.h>
#include "netinet/in.h"
#include "init.h"
#include "lib_common.h"
#include "q_gps.h"
#include "obd2lib.h"
#include "thread.h"
#include "lib_battery.h"
#include "pm.h"
#include "gps.h"
#include "lib_gyroscope.h"

int g_i2c_fd = 0;

void at_cmd_sem_init (void) __attribute__ ((constructor));
void at_cmd_sem_deinit (void) __attribute__ ((destructor));

void at_cmd_sem_init (void)
{
	int ret;
	/*!< Init Semaphore for modem*/
	gsm_at_cmd_lock = sem_open("/gsm_at_cmd_lock", O_CREAT | EEXIST | O_EXCL, 0777, 1);
	if (gsm_at_cmd_lock == SEM_FAILED){
		ret = E_GSM_SEM_INIT;
		CHK_ERR (ret, stderr, "Error: at_cmd_sem_init() sem_init");
	}else
		sem_unlink("/gsm_at_cmd_lock");
}

void at_cmd_sem_deinit (void)
{
	int ret;

	if (gsm_at_cmd_lock != SEM_FAILED)
		sem_destroy(gsm_at_cmd_lock);
}

/*AT commands*/
int gsm_at_cmd(char *at_cmd, char *resp, int length, int max_resp_time)
{
	char *wrt_cmd = NULL;
	int numberofbytesent, numberofbytesrcvd;
	char buff[1024] = {0};
	int rc = OBD2_LIB_SUCCESS;
	int serial_fd = 0;

	rc = check_gsm_modem_status();
	if( rc == OBD2_LIB_SUCCESS )
	{
		if (gsm_at_cmd_lock != SEM_FAILED)
		{
			sem_wait(gsm_at_cmd_lock);
		}
		else
		{
			printf("%s - gsm_at_cmd_lock failed!!! \n", __FUNCTION__ );
		}

		serial_fd = iW_Serial_Init(SERIAL_PORT_ACM0,115200, USB_2);
		if(serial_fd <= 0)
		{

			if (gsm_at_cmd_lock != SEM_FAILED)
			{
				sem_post(gsm_at_cmd_lock);
			}

			CHK_ERR (E_GSM_AT_SERIAL_INIT, stderr, "Error: GSM AT Port Init");
			return E_GSM_AT_SERIAL_INIT;
		}
		tcflush(serial_fd,TCIOFLUSH);

		if (strcmp(at_cmd, "dummy_buf") ==0)
		{
			goto read;
		}

		rc = asprintf(&wrt_cmd,"%s\r",at_cmd);
		if(rc < 0)
		{
			rc = -ENOMEM;
			goto end;
		}

		numberofbytesent = write (serial_fd ,wrt_cmd, strlen(wrt_cmd));
		if(numberofbytesent <= 0)
		{
			rc= E_GSM_AT_SERIAL_WRITE;
			if (wrt_cmd != NULL)
			{
				free(wrt_cmd);
				wrt_cmd = NULL;
			}
			CHK_ERR (rc, stderr, "Error: GSM AT Port Write");
			goto end;
		}
		usleep(max_resp_time);
read:

		memset( buff, 0, sizeof( buff ) );
		numberofbytesrcvd = read(serial_fd, buff, sizeof(buff)-1);
		if(numberofbytesrcvd <= 0)
		{
			rc = E_GSM_AT_SERIAL_READ;
			CHK_ERR (rc, stderr, "Error: GSM AT Port Read");
		}
		else if(numberofbytesrcvd > length - 1)
		{
			rc = E_BUFFER_READ_OVERFLOW;
			CHK_ERR (rc, stderr, "Error: GSM AT Port Read Overflow");
		}
		else
		{
			memset( resp, 0, length );
			strcpy( resp, buff );
			rc = OBD2_LIB_SUCCESS;
#ifdef ENABLE_OBD_DEBUG
			int i = 0;
			printf("\nAT Command Response for [%s]={ ", wrt_cmd);
			for(i =0; i < numberofbytesrcvd; i++){
				printf ("%c",buff[i]);
			}
			printf("}\n");
			i =0;
#endif
		}

end:
		if( serial_fd )
		{
			tcflush(serial_fd, TCIOFLUSH);
			close(serial_fd);
			serial_fd = 0;
		}

		if (wrt_cmd != NULL)
		{
			free(wrt_cmd);
			wrt_cmd = NULL;
		}

		if (gsm_at_cmd_lock != SEM_FAILED)
		{
			sem_post(gsm_at_cmd_lock);
		}
	}
	else
	{
		// Do Nothing
	}

	return rc;
}

/* BUG ID 5280 */
int gsm_at_port_close( )
{
	int rc = 0;

	/* De-initializing gps */
	rc = gps_deinit( );

	rc = gsm_modem_off_monitor();
	printf("%s - gsm_modem_off_monitor rc value 0x%x\n", __FUNCTION__, rc );


	return rc;
}

int send_sms( char *msg_response, char *sender_number, int max_resp_time )
{
	int ret = GSM_set_to_message_init ( );
	if( ret < 0 )
	{
		ret = E_GSM_SIM_SMS_INIT;
		return ret;
	}

	int serial_fd = 0;
	int rc = OBD2_LIB_SUCCESS;
	int count = 0;
	int idx = 0;
	int numberofbytesent, numberofbytesrcvd;
	bool overrun = false;
	char p = '"';
	/*command to set GSM to text messages mode*/
	char resp_buffer[ 1024 ] = {0};
	char *send_command = NULL;
	char response[COMMAND_LEN] = {0};
	char *msg_buf = NULL;
	char *msg_buf_sub = NULL;
	char *res_ptr = NULL;
	int sz = 0;
	/*checking for the received parameter is not equal to NULL*/
	if (msg_response != NULL)
		sz = strlen(msg_response);
	else
		return E_GSM_SIM_SMS_SEND_MSG;

	if (sz > MAX_MSG_LEN)
	{
		overrun = true;
	}

	if(access(GSM_AT_PORT, F_OK))
		return E_GSM_USB_DISCONNECT;

	if (gsm_at_cmd_lock != SEM_FAILED)
		sem_wait(gsm_at_cmd_lock);

	serial_fd = iW_Serial_Init(SERIAL_PORT_ACM0,115200, USB_2);
	if(serial_fd <= 0)
	{
		if (gsm_at_cmd_lock != SEM_FAILED)
			sem_post(gsm_at_cmd_lock);

		CHK_ERR (E_GSM_AT_SERIAL_INIT, stderr, "Error: GSM AT Port Init");
		return E_GSM_AT_SERIAL_INIT;
	}
	tcflush(serial_fd,TCIOFLUSH);

	for (idx = 1; idx < 5; idx++)
	{
		if (overrun == true && idx < 3)
		{
			rc = asprintf (&send_command,"AT+QCMGS=%c%s%c,153,%d,2\r",p, sender_number, p, idx);
			if(rc < 0)
			{
				rc = -ENOMEM;
				goto exit;
			}
		}
		else
		{
			rc = asprintf (&send_command,"AT+CMGS=%c%s%c\r",p, sender_number, p );
			if(rc < 0)
			{
				rc = -ENOMEM;
				goto exit;
			}
		}

		numberofbytesent = write (serial_fd ,send_command, strlen( send_command ) );
		if(numberofbytesent <= 0)
		{
			rc= E_GSM_AT_SERIAL_WRITE;
			CHK_ERR (rc, stderr, "Error: GSM AT Port Write");
			goto exit;
		}
		else
		{
			usleep(max_resp_time);
			numberofbytesrcvd = read(serial_fd, resp_buffer, sizeof(resp_buffer)-1 );
			if(numberofbytesrcvd <= 0)
			{
				rc = E_GSM_AT_SERIAL_READ;
				CHK_ERR (rc, stderr, "Error: GSM AT Port Read");
				goto exit;
			}
			else
			{
#ifdef ENABLE_OBD_DEBUG
				printf("\nAT Command Response for [%s]={ ", send_command );
				int i = 0;
				for(i =0; i < numberofbytesrcvd; i++){
					printf ("%c", resp_buffer[i]);
				}
				printf("\n");
				printf("}\n");
				i =0;
#endif
			}
			if( send_command != NULL )
			{
				free( send_command );
				send_command = NULL;
			}
		}

		if (sz > MAX_MSG_LEN){
			msg_buf_sub = malloc( sizeof(char) * 95 );
			strncpy(msg_buf_sub, msg_response, 90);
			printf("%s - msg_buf_sub value is %s\n", __FUNCTION__, msg_buf_sub );
			rc = asprintf (&msg_buf, "%s%c", msg_buf_sub, CTRL_Z);//in sas-token value starts from 15th byte.
			printf("%s - msg_buf value is %s\n", __FUNCTION__, msg_buf );
			if(rc < 0)
			{
				rc = -ENOMEM;
				goto exit;
			}
			if( msg_buf_sub != NULL )
			{
				free( msg_buf_sub );
				msg_buf_sub = NULL;
			}
			sz = sz - 90;
			msg_response = msg_response + 90;
		}
		else{
			rc = asprintf (&msg_buf, "%s%c", msg_response, CTRL_Z);//in sas-token value starts from 15th byte.
			if(rc < 0)
			{
				rc = -ENOMEM;
				goto exit;
			}
			sz = sz - MAX_MSG_LEN;
		}

		/*write system to enter the framed response in command line*/
		numberofbytesent = write (serial_fd, msg_buf, strlen( msg_buf ) );
		if (numberofbytesent <= 0)
		{
			perror ("send_sms");
			rc = E_GSM_AT_SERIAL_WRITE;
			goto exit;
		}else{
			if (msg_buf != NULL)
			{
				free(msg_buf);
				msg_buf = NULL;
			}
			/*maximum delay for at command response 300ms*/
			while (1){
				sleep (1);
				memset (response, MEM_CLR, sizeof(response));
				/*read system call to read the response get from the at command*/
				numberofbytesrcvd = read (serial_fd, response, COMMAND_LEN);
				if ((strstr (response, "OK")) == NULL && count != 15){
					count++;
				}else{
					count = 0;
					break;
				}
			}
			if (numberofbytesrcvd <= 0)
			{
				perror ("2.send_sms");
				rc = E_GSM_AT_SERIAL_READ;
				goto exit;
			}else{
				printf( "recieved response is : %s %d\n",response,sz);
			}
		}
		/*if at command execute successfully then the response is OK otherwise ERROR
		  check for OK response */
		if ((strstr (response,"OK")) != NULL && sz < 0){
			printf("%s: Successfully sent with idx is %d\n",__func__, idx);
			break;
		}
	}
	if (idx == 5)
		rc = E_GSM_SIM_SMS_SEND;
	else
		rc = OBD2_LIB_SUCCESS;

exit:
	if( serial_fd )
	{
		ret = tcflush(serial_fd, TCIOFLUSH);
		close(serial_fd);
		serial_fd = 0;
	}
	if(send_command != NULL)
	{
		free(send_command);
		send_command = NULL;
	}
	if(msg_buf != NULL)
	{
		free(msg_buf);
		msg_buf = NULL;
	}
	if (gsm_at_cmd_lock != SEM_FAILED)
		sem_post(gsm_at_cmd_lock);

	return rc;
}

int init_mode_pid(car_parameters *carparams)
{
	int rc = 0;
	char value [5];
	char pid_no[10];
	int i;	
	carparams->modepid = (mode_pid *)calloc(10,sizeof(mode_pid));
	for (i = 0;i<10;i++){
		bzero (pid_no,sizeof(pid_no));
		bzero (value,sizeof(value));
		sprintf(pid_no,"%s%d","pid",i+1);
		strcpy (carparams->modepid[i].pid, value);
		carparams->modepid[i].mode = 1;
	}
	carparams->no_of_pids = i;
	return rc;

}


int set_default_freq_values(struct frequency *freq)
{
	strcpy(freq[0].name,"GPSUPDATE");
	strcpy(freq[0].select,"Yes");
	freq[0].value = 1;

	strcpy(freq[1].name,"CARSTATUS");
	strcpy(freq[1].select,"Yes");
	freq[1].value = 2;

	strcpy(freq[2].name,"ACCELEROMETER");
	strcpy(freq[2].select,"Yes");
	freq[2].value = 2;

	strcpy(freq[3].name,"DTCCODE");
	strcpy(freq[3].select,"Yes");
	freq[3].value = 2;

	strcpy(freq[4].name,"GYROSCOPE");
	strcpy(freq[4].select,"Yes");
	freq[4].value = 2;

	return 0;
}

int init_frequency(struct frequency * freq)
{
	int rc = 0;
	char value [10];

	int i;
	char sub_node[][24] = {"sampling_frequency", "can_sign", "duration"};
	return rc;
}

int ext_bat_test()
{
	int bat_read;
	float voltage_value;

	bat_read = prgst_read_car_bat_volt(&voltage_value);
	CHK_ERR (bat_read, stderr, "Error in prgst_read_car_bat_volt");

	return bat_read;
}

int prgst_read_car_bat_volt(float *voltage_value)
{
	FILE *fp = NULL;
	int raw_value = 0;
	char buf[4] = {0};

	fp = fopen(PRGST_CAR_BAT_NODE, "r");
	if(fp == NULL){
		printf("Error opening file\r\n");
		return OBD2_LIB_FAILURE;
	}

	memset( buf, 0, sizeof( buf ) );
	fread(&buf, 4, 1, fp);

	raw_value = (int)strtol(buf, NULL, 10);
	*voltage_value = ((float)(raw_value * 0.0057 ));
	if( fp )
	{
		fclose(fp);
		fp = NULL;
	}

	if (*voltage_value > 10.5)
		return OBD2_LIB_SUCCESS;
	else{
		return OBD2_LIB_FAILURE;
	}
}

int init ( int network_enable )
{
	int ret = 0;
	int rc = 0;
	int error = 0;
	pthread_t network_tid;

	network_enable_status = 0;
	printf ("\t###################################################################\n");
	printf ("\tLibrary Revision : iW-PRGST-SC-R1.0-REL2.4.1_RuggedTelematics_Library\n");
	printf ("\t###################################################################\n");

	if(network_enable==1 || network_enable==0)
	{
		network_enable_status = network_enable;
	}
	else
	{
		return OBD2_LIB_FAILURE;
	}

	ret = ext_bat_test( );
	if( ret == OBD2_LIB_SUCCESS )
	{
		rc = system("lsmod | grep tcan4x5x > /dev/null");
		if (rc != OBD2_LIB_SUCCESS){
			rc = system("insmod /iwtest/kernel-module/tcan4x5x.ko");
			CHK_ERR (rc, stderr, "Error: can_sem_init() load module tcan4x5x.ko");
		}
		usleep(250000);
	}
	else
	{
		// Do Nothing
	}

	if ((rc = GPIO_config()) != OBD2_LIB_SUCCESS){
		IOBD_DEBUG_LEVEL3 ("GPIO_config failed");
		error = OBD2_LIB_FAILURE;
	}
	else
	{
		/*
		 * Disable all the interrupts in the device. This will \
		 * ensure that the device will not wakeup from sources \
		 * which are not enabled by the user.
		 * */
		ret = disable_all_wakeup_sources();
	}

	rc = Check_eth_Link( );
	if (rc == OBD2_LIB_SUCCESS)
	{
		printf("CheckLink eth0 success\n" );
		sleep( 1 );
	}
	else
	{
		rc = system ("ifconfig eth0 up");
		rc = Check_eth_Link();
		sleep( 1 );
		if (rc != OBD2_LIB_SUCCESS)
		{
			printf("Ethernet up failed\n");
		}
		else
		{
			printf("Ethernet up\n");
		}
	}

	if( network_enable_status == 1 )
	{
		if( network_status == 0 )
		{
			if(pthread_create( &network_tid, NULL, (void *) network_manager, NULL) != 0) {
				printf(" pthread_create for network_status_pin_check failed %d\r\n",errno);
				return errno;
			}
			else
			{
				network_status = 1;
				printf("network_manager thread created\n");
				if (pthread_detach ( network_tid ) != OBD2_LIB_SUCCESS)
				{
					return errno;
				}
				else
				{
				}
			}
		}
		else
		{
			printf("network_manager - thread is already running \n" );
			ret = OBD2_LIB_FAILURE;
		}
	}

	return error;
}

/* BUG ID 5298 */
int deinit( )
{
	printf("Inside deinit func() \n" );
	int rc = 0;
	int error = 0;
	if( network_enable_status == 1 )
	{
		network_monitor_disable();
	}

	rc = system("lsmod | grep tcan4x5x > /dev/null");
	if (rc == OBD2_LIB_SUCCESS)
	{
		rc = system("rmmod tcan4x5x");
		CHK_ERR (rc, stderr, "Error: can_sem_deinit() unload module tcan4x5x");
		usleep(250000);
	}

	rc = eth_deinit();
	if (rc == OBD2_LIB_SUCCESS)
	{
		printf("Disable eth0 success\n");
	}
	else
	{
		printf("Disable eth0 failed with |%x|\n", rc);
	}
	sleep( 1 );

	if ((rc = GPIO_off()) != OBD2_LIB_SUCCESS)
	{
		IOBD_DEBUG_LEVEL3 ("GPIO_off failed");
		error = OBD2_LIB_FAILURE;
	}
	else
	{
		error = OBD2_LIB_SUCCESS;
	}

	return error;
}

void sys_sleep_completed(void)
{
	IOBD_DEBUG_LEVEL3("sys_sleep_completed \n");
	return;
}

void sys_wake_completed(void)
{
	/* NaNC : Clear all the ignition flags */
	IOBD_DEBUG_LEVEL3(" sys_wake_completed + \n");
	sem_post(&libClient.sys_wake);
	IOBD_DEBUG_LEVEL3(" sys_wake_completed - \n");

	return;
}

int led_enable()
{
	int ret = OBD2_LIB_SUCCESS;

	ret = set_gpio_value(LED_GPIO, 1);

	return ret;
}

int led_disable()
{
	int ret = OBD2_LIB_SUCCESS;

	ret = set_gpio_value(LED_GPIO, 0);

	return ret;

}

int wake_up_device()
{
	int ret = OBD2_LIB_SUCCESS;
	int error = OBD2_LIB_SUCCESS;
	/* Add Wifi Driver */
	ret = system("modprobe brcmfmac");
	if(ret == OBD2_LIB_FAILURE)
	{
		error = OBD2_LIB_FAILURE;
	}
	return error;
}

/*
 * Common API for i2c register writing.
 * i2cget command is used(i2c APIs cannot be used because i2c nodes are already handled by iio utils so opening node will give error
 * need to disable gyro before executing i2c commands and enable after execution
 * */
/* BUG ID 5275 */
int i2c_write(int bus_num,uint8_t slave_addr, uint8_t data_addr, uint8_t value)
{
	int ret;
	int error = OBD2_LIB_SUCCESS;
	char *command = NULL;
	FILE *fp = NULL;

	ret = asprintf (&command, "%s %d 0x%x 0x%x 0x%x",I2C_WRITE,bus_num,slave_addr, data_addr, value);
	if (ret < 0)
	{
		error = -ENOMEM;
		goto end;
	}else{
		error = OBD2_LIB_SUCCESS;
	}
	fp = popen( command, "w" );
	if( fp != NULL )
	{
		error = OBD2_LIB_SUCCESS;
	}
	else
	{
		IOBD_DEBUG_LEVEL3("File open error \r\n");
		error = OBD2_LIB_FAILURE;
		goto end;
	}
	error = pclose( fp );
	if( error == -1 )
	{
		printf("Error reported by pclose() %d\n", errno );
	}
	else
	{
		// printf("log_user - status of pclose() %d\n", status );
		error = OBD2_LIB_SUCCESS;
	}
end:
	if(command != NULL){
		free(command);
		command = NULL;
	}

	return error;
}

/*
 * Common API for i2c register reading.
 * i2cget command is used(i2c APIs cannot be used because i2c nodes are already handled bu iio utils so opening node will give error
 * need to disable gyro before executing i2c commands and enable after execution
 * Stores the i2cget command result to a txt file and reading the txt file for value.
 * */
int i2c_read(int bus_num, uint8_t slave_addr, uint8_t data_addr, uint8_t *value)
{
	int ret;
	int error = OBD2_LIB_SUCCESS;
	FILE *fp = NULL;
	char *command = NULL;
	ret = asprintf (&command, "%s %d 0x%x 0x%x",I2C_READ,bus_num,slave_addr,data_addr);
	if (ret < 0)
	{
		ret = -ENOMEM;
		goto end;
	}

	if( ret > OBD2_LIB_SUCCESS )
	{
		fp = popen( command, "r" );
		if( fp != NULL )
		{
			fscanf( fp, "%p", (void *)value );
		}
		else
		{
			IOBD_DEBUG_LEVEL3("File open error \r\n");
			error = OBD2_LIB_FAILURE;
			goto end;
		}
		error = pclose( fp );
		if( error == -1 )
		{
			printf("Error reported by pclose() %d\n", errno );
		}
		else
		{
			// printf("%s - status of pclose() %d\n", __FUNCTION__, error );
		}
	}
	else
	{
		error = OBD2_LIB_FAILURE;
	}
end:
	if(command != NULL){
		free(command);
		command = NULL;
	}

	return error;
}

int push_device_to_sleep( )
{
	int ret = OBD2_LIB_SUCCESS;
	FILE *fp_sleep = NULL;
	char *sleep_buf = NULL;
	/*Clearing the flag before going to sleep */
	timer_state = 0;

	battery_charge_state_config( 1 );
	sleep( 2 );

	asprintf( &sleep_buf, "%s", "mem" );

	fp_sleep = fopen( DEVICE_SLEEP_PATH, "w" );
	if( fp_sleep == NULL )
	{
		printf("%s - failed to open %s with errno %d\n", __FUNCTION__, DEVICE_SLEEP_PATH, errno );
		ret = OBD2_LIB_FAILURE;
		goto end;
	}

	ret = fwrite( sleep_buf, 1, strlen( sleep_buf ), fp_sleep );
	if( ret < 0 )
	{
		printf("%s - fwrite failed %d\n", __FUNCTION__, errno );
		ret = OBD2_LIB_FAILURE;
	}
	else
	{
		ret = OBD2_LIB_SUCCESS;
	}

end:
	if( fp_sleep != NULL )
	{
		fclose( fp_sleep );
	}

	free( sleep_buf );

	battery_charge_state_config( 0 );
	sleep( 2 );

	return ret;
}

int restart_device()
{
	int ret = OBD2_LIB_SUCCESS;
	int error = OBD2_LIB_SUCCESS;
	int ps = OBD2_LIB_FAILURE;
	ret = gyro_disable();
	usleep(10000);

	if(ret == OBD2_LIB_FAILURE)
	{
		error = OBD2_LIB_FAILURE;
	}
	/*Gyro Power down */
	ret = system("i2cset -f -y 1 0x6a 0x11 0x00");
	if(ret == OBD2_LIB_FAILURE)
	{
		error = OBD2_LIB_FAILURE;
	}
	usleep(250000);
	/*Accelerometer Power down*/
	ret = system("i2cset -f -y 1 0x6a 0x10 0x00");
	if(ret == OBD2_LIB_FAILURE)
	{
		error = OBD2_LIB_FAILURE;
	}
	ps=get_power_source();
	if(ps == 4104)
	{
		printf("External power source \n" );
		battery_connect_config( 0 );
		sleep(2);
		set_gpio_value(REBOOT_GPIO, 1 );
	}
	else if(ps == 4105)
	{
		printf("Internal battery power source \n" );
		system( "reboot" );
	}

	return error;
}
