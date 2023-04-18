#define _GNU_SOURCE
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "init.h"
#include "lib_common.h"
#include "serial.h"
#include "iio_utils.h"
#include "obd2lib.h"

void get_time (char *timebuf)
{
	struct timeval tv;
	struct tm* ptm;
	char time_string[40];
	long milliseconds;

	/* Obtain the time of day, and convert it to a tm struct. */
	gettimeofday (&tv, NULL);
	ptm = localtime (&tv.tv_sec);

	/* Format the date and time, down to a single second. */
	strftime (time_string, sizeof (time_string), "%Y-%m-%dT%H:%M:%S", ptm);

	/* Compute milliseconds from microseconds. */
	milliseconds = tv.tv_usec / 1000;

	/* 
	 * Print the formatted time, in seconds, followed by a \
	 * decimal point and the milliseconds.
	 * */
	sprintf (timebuf,"%s.%03ldZ", time_string, milliseconds);
}

int time_diff_seconds(struct timespec *time_end, struct timespec *time_start)
{
	int ret = 0;
	ret = time_end->tv_sec - time_start->tv_sec;
	return ret;
}

int get_cpu_id(char *cpuid,int cpuid_len)
{
	int read_size;
	int ret = SUCCESS;
	FILE *fp0,*fp1 = NULL;
	char buf_msb[9] = {0};
	char buf_lsb[9] = {0};
	char *command = NULL;
	char dest[9] = {0};
	int total_lsb_len = 8;
	int zeroes, len;
	int i = 0;

	/* read unique id lsb*/
	// Execute a process listing
	ret = asprintf(&command, "hexdump /sys/bus/nvmem/devices/imx-ocotp0/nvmem | head -n 1 | awk '{printf $5 $4}'");
	if(ret < 0)
	{
		ret = -ENOMEM;
	}else{
		ret = SUCCESS;
	}
	fp0 = popen(command, "r");
	if (fp0 != NULL){

		/*Reading lsb of CPU Unique ID*/
		if( !( fgets(buf_msb, 9, fp0) ) )
			perror ("read_fp0");

		pclose(fp0);
	}
	else{
		ret = FAILURE;
		goto exit;
	}
	if(command != NULL){
		free(command);
		command = NULL;
	}
	ret = asprintf(&command, "hexdump /sys/bus/nvmem/devices/imx-ocotp0/nvmem | head -n 1 | awk '{printf $7 $6}'");
	if(ret < 0)
	{
		ret = -ENOMEM;
		goto exit;
	}else{
		ret = SUCCESS;
	}

	fp1 = popen(command,"r");
	if (fp1 != NULL )
	{
		/* Reading msb of CPU Unique ID*/
		if( !( fgets(buf_lsb, 9, fp1) ) )
			perror ("read_fp1");

		pclose(fp1);
	}
	else
	{
		ret = FAILURE;
		goto exit;
	}
	if(command != NULL){
		free(command);
		command = NULL;
	}
	puts(buf_lsb);
	/*Pre-pending with 0 incase MSB starts with 0*/
	len = strlen(buf_lsb);
	for(i=0;i<len;i++)
	{
		if((strcmp(&buf_lsb[i],"\n") == 0))
		{
			buf_lsb[i]=0;
			len = len - 1;
		}
	}

	read_size = sizeof(buf_lsb)+sizeof(buf_msb);
	if(cpuid_len > read_size+4){
		memset( cpuid, 0, cpuid_len );
		memcpy(cpuid,"0x",2);
		if(len < total_lsb_len)
		{
			zeroes = total_lsb_len - len;
			memset(dest,'0',zeroes);
			puts(dest);
			strcat(dest,buf_lsb);
			puts(dest);
			len = 0;
			for (i = 0; dest[i] != '\0'; i++)
			{
				len++;
			}
			sprintf(cpuid+2,"%s%s", dest,buf_msb);
		}
		else
		{
			sprintf(cpuid+2,"%s%s", buf_lsb,buf_msb);
		}
	}
	else{
		ret = E_BUFFER_READ_OVERFLOW;
	}
exit:
	if(command != NULL){
		free(command);
		command = NULL;
	}
	return ret;
}

int check_adc_voltage (double *volt)
{
	FILE *fp;
	char buf[10];
	int raw_value;
	double voltage_value;
	int error = OBD2_LIB_SUCCESS;
	bzero (buf,sizeof(buf));
	fp = fopen(ADC_READ_NODE, "r");
	if(fp == NULL){
		printf("ADC_READ_NODE: Error opening file\r\n");
		error = OBD2_LIB_FAILURE;
		goto end;
	}
	if(error == OBD2_LIB_SUCCESS)
	{
		fread(&buf, 10, 1, fp);
		fclose(fp);
		raw_value = (int)strtol(buf, NULL, 10);
		voltage_value = raw_value;
		*volt = (voltage_value * 0.005575);
	}
end:
	return error;
}


int check_in_bt_volt (double *volt)
{
	FILE *fp;
	char buf[10];
	int raw_value;
	double voltage_value;
	int rc = 0;

	bzero (buf,sizeof(buf));

	fp = fopen(IN_BT_READ_NODE, "r");
	if(fp == NULL){
		printf("IN_BT_READ_NODE: Error opening file\r\n");
		rc = -1;
		goto end;
	}

	fread(&buf, 10, 1, fp);
	fclose(fp);

	printf ("In_battery_volt_buf : %s\n",buf);

	raw_value = (int)strtol(buf, NULL, 10);
	voltage_value = raw_value;
	*volt = (voltage_value * 0.005575);
end:
	return rc;
}

/*
 * API		: read_digital_in(int din, int state)
 * Description	: API to read the Digital Inputs in Rugged Telematics Device.
 * Arguments	: din - DIN name in numbers: \
 * 		:	1. DIN1 \
 * 		:	2. DIN2 \
 * 		: *state - To store the current status of the Digital Input
 * Return Value	: Returns 0 on Success and other error code on Failure
 * */
int read_digital_in(int din, int *state)
{
	int ret = OBD2_LIB_FAILURE;
	int gpio = 0;
	char *gpio_path = NULL;

	if(din == 1)
	{
		/* DIN1 : GPIO4 */
		gpio = 4;
	}
	else if(din == 2)
	{
		/* DIN2 : GPIO39 */
		gpio = 39;
	}
	else
	{
		ret = E_OBD2_LIB_INVALID_ARG;
	}

	if(ret != E_OBD2_LIB_INVALID_ARG)
	{
		ret = asprintf(&gpio_path, "/sys/class/gpio/gpio%d/value",gpio);
		if (ret < 0)
		{
			ret = -ENOMEM;
		}
		else
		{
			if (access(gpio_path, F_OK ) == 0)
			{
				// Do Nothing
			}
			else
			{
				gpio_export(gpio, OUTPUT);
			}
			ret = get_gpio(gpio, state);
		}
	}
	else
	{
		// Do Nothing
	}

	if (gpio_path != NULL)
	{
		free(gpio_path);
		gpio_path = NULL;
	}

	return ret;
}

/*
 * API		: write_digital_out(int dout, int state)
 * Description	: API to read the Digital Inputs in Rugged Telematics Device.
 * Arguments	: dout - DOUT name in numbers: \
 * 			1. DOUT1 \
 * 			2. DOUT2 \
 * 		: state - The status to write to the Digital Output.
 * 			0. OFF \
 * 			1. ON \
 * Return Value	: Returns 0 on Success and other error code on Failure
 * */
int write_digital_out(int dout, int state)
{
	int ret = OBD2_LIB_FAILURE;
	int gpio = 0;

	if((state == ON) || (state == OFF))
	{
		if(dout == 1)
		{
			/* DOUT1 : GPIO32 */
			gpio = 32;
		}
		else if(dout == 2)
		{
			/* DOUT2 : GPIO38 */
			gpio = 38;
		}
		else
		{
			ret = E_OBD2_LIB_INVALID_ARG;
		}

		if(ret != E_OBD2_LIB_INVALID_ARG)
		{
			ret = set_gpio_value(gpio, state);
		}
	}
	else
	{	
		ret = E_OBD2_LIB_INVALID_ARG;
	}

	return ret;
}

/*Function used to get the Mac Address of the ethernet*/
/* BUG ID 5269 */
int get_mac_address(char *interface,char *mac_address)
{
	char eth0_buf[ 20 ] = {0};
	char buff[25] = {0};
	FILE *fp_eth0 = NULL;
	char *path = NULL;
	int ret = OBD2_LIB_FAILURE;

	if (strcmp("eth0", interface) == OBD2_LIB_SUCCESS)
	{
		if( access( ETH0_ADDRESS_PATH, F_OK ) == OBD2_LIB_SUCCESS )
		{
			fp_eth0 = fopen( ETH0_ADDRESS_PATH, "r" );
			if( fp_eth0 == NULL )
			{
				printf("%s - failed to open %s with errno %d\n", __FUNCTION__, ETH0_ADDRESS_PATH, errno );
				ret = E_IF_OPEN;
				goto end;
			}

			ret = fread( &eth0_buf, 17, 1, fp_eth0 );
			if( ret >= 0 )
			{
				if( eth0_buf[0] != '\0')
				{
					strcpy(mac_address,eth0_buf); //Ethernet mac address is valid
					ret = OBD2_LIB_SUCCESS;
				}
				else
				{
					ret = E_IF_INVALID; //Ethernet mac address is not valid
				}
			}
			else
			{
				ret = E_IF_READ;
			}
			fclose( fp_eth0 );
			memset(eth0_buf,0,sizeof(eth0_buf));
		}
		else
		{
			ret = OBD2_LIB_FAILURE;
		}
	}
	else
	{
		if (strcmp("hci0", interface) == OBD2_LIB_SUCCESS){
			ret = asprintf(&path,"%sbluetooth/%s/", READ_MAC_NODE, interface);
			if (ret < 0)
			{
				ret = -ENOMEM;
				goto end;
			}
		}
		else{
			ret = asprintf(&path,"%snet/%s/", READ_MAC_NODE, interface);
			if (ret < 0)
			{
				ret = -ENOMEM;
				goto end;
			}
		}
		printf("get_mac_address path %s\n", path );

		if( access(path, F_OK) == OBD2_LIB_SUCCESS )
		{
			ret = read_sysfs_string("address",path,buff);
			strncpy(mac_address,buff,sizeof(buff));
			printf("get_mac_address mac_address value %s\n", mac_address );
		}
		else
		{
			printf("get_mac_address mac_address else case\n" );
			CHK_ERR (ret, stderr, "Error in access interface");
			free(path);
			path = NULL;
			ret = E_IF_INVALID;
		}
	}
end:
	if (path != NULL)
	{
		free(path);
		path = NULL;
	}
	return ret;
}
