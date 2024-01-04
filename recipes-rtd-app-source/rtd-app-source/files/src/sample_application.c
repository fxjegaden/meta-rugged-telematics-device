#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "common.h"
#include "gps.h"
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <stdint.h>
#include <linux/netlink.h>
#include <linux/if_link.h>
#include <linux/input.h>
#include <linux/rtnetlink.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <unistd.h>
#include <errno.h>
#include "gsm.h"
#include <can.h>
#include "gyroscope.h"
#include "accelerometer.h"
#include "error_nos.h"
#include "magnetometer.h"
#include "battery.h"
#include <time.h>

#define ENABLE 1
#define DISABLE 0
#define WIFI_HOSTAPD_MODE 1
#define WIFI_STATION_MODE 2
#define CAN_250 250000
#define CAN_500 500000

int sleep_counter = 0;

int can_initialization()
{
	printf ("Inside can_initialization \n");
	int rc = -1;
	struct canfd_frame frame;
	int i =0;
	int ret = -1;
	const char can_name[] = "can0";

	rc= can_init(can_name, CAN_500);
	printf (" return value of can_init 500 is %x \n", rc);
	if (rc == 0)
	{

		for ( i = 0; i < 5; i++)
		{
			memset(&frame, 0, sizeof(frame));
			rc = can_read("can0", &frame);
			printf (" return value of can_read is %x \n", rc);
			if( rc >= 8 )
			{
				ret = 0;
				break;
			}
		}
		if(rc < 0)
		{
			can_deinit("can0");
			printf(" 500kbps no data, checking 250kbps\n");
			goto check_250;
		}
	}
	else
	{
check_250:
		can_deinit("can0");
		rc= can_init("can0", CAN_250);
		printf (" return value of can_init 250 is %x \n", rc);

		if (rc == 0)
		{
			for ( i=0; i<5; i++)
			{
				memset(&frame,0,sizeof(frame));
				rc = can_read("can0", &frame);
				printf (" return value of can_read is %x \n", rc);
				if( rc >= 8 )
				{
					ret = 0;
					break;
				}
			}
			if(rc < 0)
			{
				can_deinit("can0");
			}

		}
	}
	if(ret == 0)
	{
		rc = ret;
	}

	return rc;
}

int main()
{
	int rc = 0, rs232_fd;
	long int rs232_write_data;
	long int rs232_read_data;
	char buf1[1048576];
	char buf2[1048576]="ABCDEFGHIJ";
	char arr_sig_strngth[100] = {0};
	char cell_id[50] = {0},lac[30] = {0};
	int sig_strength = 0;
	int arr_sig_strength = 0;
	int network_link_count = 0;
	int network_connection_count = 0;
	int ntp_server_update_count = 0;
	int link_status = 0;
	char ts[64] = {0};
	char iccid[ 64 ] = " ";
	char buf[ 100 ] = " ";
	struct timeval tv;
	time_t t;
	struct tm *info;
	char buffer[64];
	char resp_buffer[ 1024 ];
	char imei[100] = {0};
	char recv_data[500] = {0};
	int gps_fd;
	size_t len = 0;
	int n = 0;
	int battery_chrg_status = 0;
	double battery_voltage = 0;
	accelerometer_api_priv g_adata;
	gyroscope_api_priv g_data;
	magnetometer_api_priv mag;
	int cpuid_length = 40;
	char cpu_id[50] = {0};
	char wifi_addr[30] = {0}, eth_addr[30] = {0}, ble_addr[30] = {0};
	char time[64];
	char time_string[40];
	long milliseconds;

init:
	printf("\n#############-: STIMIO RUN :-#############\n");	
	printf("\n############ [INIT] ##############\n");
	int count = 0;
	rc = init(1);
	if(rc == -1)
		printf("init() failed\n");
	else
		printf("*********************init() done\n");

	rc = i_battery_init( );
	printf("i_battery_init rc value %x\n", rc );	

	memset( cpu_id, 0, sizeof( cpu_id ) );
	rc = get_cpu_id(cpu_id,cpuid_length);
	printf("get_cpu_id rc value %x\n", rc );
	printf("get_cpu_id buffer %s\n", cpu_id );

	/*Initializing ethernet interface */
	rc = eth_init( );
	printf("eth_init rc value %x\n", rc );

	/* Getting bluetotth MAC address */
	get_mac_address("eth0",eth_addr);
	printf("ETHERNET MAC ADDRESS %s\n", eth_addr);

	/*Deinitializing ethernet interface */
	rc = eth_deinit( );
	printf("eth_deinit rc value %x\n", rc );

	/* checking whether modem is on or off. */
gsm:
	rc = check_gsm_modem_status();
	if(rc != 0)
	{
		/* Turning on the GSM modem */
		rc = gsm_modem_on( "0000", 4 );
		goto gsm;
	}

	printf("\nLINE %d\n", __LINE__);

	/* Getting the IMEI number of the GSM module. */
	memset( imei, 0, sizeof( imei ) );
	rc = get_gsm_imei(imei, sizeof( imei ) );
	printf("imei[%s] rc = %x\n", imei, rc);

	/* Getting the ICCID of the GSM module */
	memset( iccid, 0, sizeof( iccid ) );
	rc = get_gsm_sim_iccid(iccid, sizeof(iccid));
	printf("iccid[%s] = %x\n", iccid, rc);

	/* Getting the signal range details of the network */
	memset( arr_sig_strngth, 0, sizeof( arr_sig_strngth ) );
	rc = get_gsm_signal_strength(arr_sig_strngth, sizeof( arr_sig_strngth ));
	printf("gsm signal[%s] = %x\n",arr_sig_strngth, rc);

	/* Getting the SIM registration details */
	memset( cell_id, 0, sizeof( cell_id ) );
	memset( lac, 0, sizeof( lac ) );
	rc = get_gsm_nw_reg(cell_id, sizeof( cell_id ), lac, sizeof( lac ) );
	printf("CELL_ID %s LAC %s\n",cell_id,lac);

	/* Changing the network mode */
	rc = set_gsm_network_mode(4);
	printf("rc set_gsm_network_mode() %x \n",rc);

	/* Sending AT commands and receive the response */
	gsm_apn_configuration("airtelgprs.com","ATDT*99***1#","abcd","1234");

	while (1){
		/* Establishing ppp network connection */
		rc = establish_connection();
		if(rc == 0) {
			/* Checking whether the internet connectivity is active or not */
			rc = check_network_connection();
			if (rc == 0 ){
				rc = check_gsm_nw_connection();
				break;
			}
			else{
				printf("Trying to Connect!!!!\n");
				sleep(1);
				if (count > 2){
					printf("Network Connection Not Established\n");
					break;
				}
			}
		}
		else
		{
			sleep(1);
			if (count >= 2 ){
				printf("Network Connection Not Established\n");
				break;
			}
		}
		count++;
	}

	n = 0;
	printf("\n############[LED]##############\n");
	/* Turning on the green LED */
	rc = led_enable();
	printf("led_enable() Return value = %x\n",rc);

	printf("\n############[GPS]##############\n");
	/* Fetching latest xtradata from the server */
	int i = 0;
	while ( i < 2 ){
		printf("Waiting for network link establishment \n");
		sleep (30);
		rc = agps_init();
		printf("agps_init return value %x\n",rc);
		if(rc != 0){
			i++;
		}else{
			break;
		}
	}

	/*Updating device time to standard UTC time using ntp time server */
	printf("######### ntp_server_update ########\n");
	rc = ntp_server_update();
	if (rc == -1){
		printf("NTP FAILED rc %x\n",rc);
	}
	else
		printf("NTP DONE %x\n",rc);


	/* Initializing GPS */
	rc= gps_init();
	printf("gps_init() Return value = %x\n", rc );

	/* Reading the gps data from the OBDII device */
	/* Ignore the first data after GPS Initialization */
	rc = get_gps_data("GPRMC", &len, recv_data, sizeof(recv_data));
	sleep(1);

	memset( recv_data, 0, sizeof( recv_data ) );
	n = 0;

	while (n < 10){
		rc = get_gps_data("GPRMC", &len, recv_data, sizeof(recv_data));
		if(!rc)
		{
			printf("GPRMC DATA:\n[%s]\n\n", recv_data);
		}
		else
		{
			printf("Error with |%x|\n", rc);
		}
		sleep(1);
		n++;
	}

	rc = GSM_set_to_message_init ( );
	printf("GSM_set_to_message_init rc value is %x\n", rc );

	memset( resp_buffer, 0, sizeof( resp_buffer ) );
	rc = unread_message( resp_buffer, sizeof( resp_buffer ), 800000 );
	printf("unread_message resp_buffer %s and rc value %x\n", resp_buffer, rc );

	memset( resp_buffer, 0, sizeof( resp_buffer ) );
	rc = read_message( resp_buffer, sizeof( resp_buffer ), 800000 );
	printf("read_message resp_buffer %s and rc value %x\n", resp_buffer, rc );

	printf("%s - Before delete_message call \n", __FUNCTION__ );
	memset( resp_buffer, 0, sizeof( resp_buffer ) );
	rc = delete_message( 1, 800000 );
	printf("delete_message rc value %x\n", rc );
	printf("%s - After delete_message call \n", __FUNCTION__ );

	memset( resp_buffer, 0, sizeof( resp_buffer ) );
	rc = unread_message( resp_buffer, sizeof( resp_buffer ), 800000 );
	printf("unread_message resp_buffer %s and rc value %x\n", resp_buffer, rc );

	memset( resp_buffer, 0, sizeof( resp_buffer ) );
	rc = read_message( resp_buffer, sizeof( resp_buffer ), 800000 );
	printf("read_message resp_buffer %s and rc value %x\n", resp_buffer, rc );

	memset( resp_buffer, 0, sizeof( resp_buffer ) );
	rc = delete_all_messages( 800000 );
	printf("delete_all_message rc value %x\n", rc );

	memset( resp_buffer, 0, sizeof( resp_buffer ) );
	rc = unread_message( resp_buffer, sizeof( resp_buffer ), 800000 );
	printf("unread_message resp_buffer %s and rc value %x\n", resp_buffer, rc );

	memset( resp_buffer, 0, sizeof( resp_buffer ) );
	rc = read_message( resp_buffer, sizeof( resp_buffer ), 800000 );
	printf("read_message resp_buffer %s and rc value %x\n", resp_buffer, rc );

	rc = set_gsm_flight_mode_on();
	printf("set_gsm_flight_mode_on() return value %x\n",rc);
	sleep(2);
	rc = set_gsm_flight_mode_off();
	printf("set_gsm_flight_mode_off() return value %x\n",rc);

	/* De-initializing gps */
	rc = gps_deinit( );
	printf("gps_deinit rc value 0x%x\n", rc );

	rc = gsm_modem_off();
	printf("gsm_modem_off rc value 0x%x\n", rc );

	n = 0;
	while( n < 10 )
	{
		/* Checking the battery charging status */
		rc = i_get_battery_status( &battery_chrg_status );
		printf("*****************battery Status **************8 = %x\n",rc);
		n++;
		sleep(1);
	}

	rc = i_battery_get_voltage( &battery_voltage );
	printf("i_battery_get_voltage rc value 0x%x and battery_voltage %f\n", rc, battery_voltage );

	rc = i_battery_get_health( );
	printf("i_battery_get_health rc value 0x%x\n", rc );

	rc = i_get_battery_status( &battery_chrg_status );
	printf("i_get_battery_status rc value %x and battery_chrg_status %x\n", rc, battery_chrg_status );
	printf("i_get_battery_status rc value hex 0x%x\n", rc );
	printf("i_get_battery_status rc value hex1 0x%X\n", rc );

	printf("\n############[Accelerometer]##############\n");
	/* Initializing accelerometer */
	rc = acc_init();
	printf("\n Accelerometer Initialisation return value = 0x%x\n",rc);
	n = 0;
	while( n < 10 )
	{
		/* Reading the accelerometer values from the OBDII device */
		accelerometer_read(&g_adata);
		printf ("Acc x-axis: %f\ty-axis: %f\tz-axis: %f\n",g_adata.x,g_adata.y,g_adata.z);
		n++;
	}

	printf("\n############[Gyroscope]##############\n");
	/* Initializing gyroscope */
	rc = gyro_init();
	printf("\n Gyroscope Initialisation return value = 0x%x\n",rc);
	n = 0;
	while( n < 10 )
	{
		/* Reading the gyroscope values from the OBDII device */
		gyroscope_read(&g_data);
		printf ("Gyro x-axis: %f\ty-axis: %f\tz-axis: %f\n",g_data.x,g_data.y,g_data.z);
		n++;
	}

	printf("\n############[CAN TEST]##############\n");
	rc = can_initialization();
	printf("\n CAN Initialisation return value = 0x%x\n",rc);

	rc = set_acc_wakeup_threshold( 0x89 );
	printf("set_acc_wakeup_threshold rc value 0x%x\n", rc );

	set_acc_sampling_frequency(0x70);
	set_acc_low_pass_filter(0x80);
	set_acc_wakeup_threshold(0x81);
	set_gyro_sampling_frequency(0x70);

	printf("\n############[Magnetometer]##############\n");
	/* Initializing magnetometer */
	rc = mag_init();
	printf("\n Magnetometer Initialisation return value = 0x%x\n",rc);
	n = 0;
	while( n < 10 )
	{
		/* Reading the Magnetometer values from the OBDII device */
		magnetometer_read(&mag);
		printf ("Mag x-axis: %f\ty-axis: %f\tz-axis: %f\n",mag.x,mag.y,mag.z);
		n ++;
	}

	/* De-initializing magnetometer */
	rc = mag_deinit();
	printf("mag_deinit rc value %x\n", rc );

	/* De-initializing gyroscope */
	rc = gyro_deinit ();
	printf("gyro_deinit rc value %x\n", rc );

	/* De-initializing accelerometer */
	rc = acc_deinit ();
	printf("acc_deinit rc value %x\n", rc );

	/* De-initializing bluetooth */
	rc = ble_deinit( );
	printf("ble_deinit rc value %x\n", rc );

	/* De-initializing wifi */
	rc = wifi_deinit( WIFI_HOSTAPD_MODE );
	printf("wifi_deinit rc value %x\n", rc );

	while(1){
		rc = network_monitor_disable( );
		printf("Network Monitor Disable rc = %x\n",rc);
		if(rc == 0){
			break;
		}
		else{
			sleep(1);
		}
	}

	rc =  can_deinit("can0");
	printf("CAN_deinit rc value %x\n", rc );

	memset(buf1,0,sizeof(buf1));
	rs232_fd = rs232_init( 115200 );
	if(rs232_fd < 0)
	{
		printf("Error rs232_init |%d|", rs232_fd);
	}
	else
	{
		printf("RS232_init Success |%d|", rs232_fd);
		sleep(1);

		rs232_write_data = rs232_write(buf2, 11);
		if(rs232_write_data < 0)
		{
			printf("Error in writing RS232 Data |%lx|\n", rs232_write_data);
		}
		else
		{
			printf("RS232 Write Success with |%lx|\n", rs232_write_data);
		}
		sleep(1);

		/* RS232 READ */
		printf("RS232 Read Data\r\n");
		rs232_read_data = rs232_read(buf1, sizeof(buf1));
		if(rs232_read_data < 0)
		{
			printf("Error in reading RS232 Data = |%lx|\n", rs232_read_data);
		}
		else
		{
			printf("RS232 READ Success with |%lx|\n", rs232_read_data);
			printf("RS232 Read Data: %s\r\n",buf1);
		}
		sleep(1);

		/* RS232 De-initialization */
		rc = rs232_deinit();
		printf("RS232 Deinit Return value |%x|\n", rc );
	}

	printf("\n############ LED ##############\n");
	/* Turning on the green LED */
	rc = led_disable();
	printf("led_disable() Return value = %x\n",rc);

	rc = config_timer_wakeup( ENABLE, 60 );
	printf("config_timer_wakeup rc value %x\n", rc );

	rc = config_acc_wakeup( ENABLE );
	printf("config_acc_wakeup rc value %x\n", rc );    

	rc = config_ignition_wakeup( ENABLE );
	printf("config_ignition_wakeup rc value %x\n", rc );

	rc = config_can_wakeup( "can0", ENABLE );
	printf("config_can_wakeup rc value %x\n", rc );

	printf("################## DE-INIT #######################\n");
	rc = deinit( );
	if(rc == -1)
		printf("deinit() failed\n");
	else
		printf("*********************deinit() done\n");

	rc = config_timer_wakeup( ENABLE, 60 );
	printf("config_timer_wakeup rc value %x\n", rc );

	rc = push_device_to_sleep( );
	printf("push_device_to_sleep rc value %x\n", rc );

	sleep_counter++;
	printf("Sleep counter value %d\n", sleep_counter );
	if( sleep_counter > 5 )
	{
		printf("Sleep counter reached more than 5 ############################\n");
		rc = restart_device( );
		printf("restart_device rc value %x\n", rc );
	}

	goto init;
}
