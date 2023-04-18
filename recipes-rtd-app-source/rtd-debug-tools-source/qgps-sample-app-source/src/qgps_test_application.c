#include <stdlib.h>
#include <debug.h>
#include <stddef.h>
#include <unistd.h>
#include "common.h"
#include "gsm.h"
#include "gps.h"

int main()
{
	int rc = OBD2_LIB_FAILURE;
	char ts[64] = {0};
	char iccid[100] = {0};
	char cpu_id[100] = {0};
	char device_id[100] = {0};
	char pin_status = 0;
	int gps_fd = 0;
	int count = 0;
	/*<! initialise the call back function pointers*/
	rc = init(0);
	if (rc == OBD2_LIB_FAILURE)
		IOBD_DEBUG_LEVEL1("init failed\n");
#if 0
	system("i2cset -f -y 1 0x6b 0x00 0x17");
	system("i2cset -f -y 1 0x6b 0x03 0x11");
	system("i2cset -f -y 1 0x6b 0x05 0x8f");
	system("i2cset -f -y 1 0x6b 0x02 0x85");
#endif
	rc = check_gsm_modem_status();
        printf("gsm_modem_on() *****Return value = %d\n",rc);
        if(rc != 0)
        {
                rc = gsm_modem_on("0000", 4);
	}
	rc = check_gsm_modem_status();
	if(rc != 0)
	{
		printf("failed to read usb_port\n");
	}
	while (1)
	{
		/*Establishing ppp network connection */
		rc = establish_connection();
		if(rc == 0) 
		{
			/* Checking whether the internet connectivity is active or not */
			rc = check_gsm_nw_connection();
			if (rc == 0 )
				break;
			else
			{
				printf("Trying to Connect!!!!\n");
				sleep(1);
				if (count > 50)
				{
					printf("Network Connection Not Established\n");
					break;
				}
			}
		}
		else
		{
			sleep(1);
			if (count > 50)
			{
				printf("Network Connection Not Established\n");
				break;
			}
		}
		count++;
	}
	/*Updating device time to standard UTC time using ntp time server */
	rc = agps_init();
        if(rc)
                printf("quectel_agps_init() Return value = %d\n",rc);

	rc = gps_init();
	if (rc == OBD2_LIB_FAILURE)
		IOBD_DEBUG_LEVEL3("gps init faild\n");	
	IOBD_DEBUG_LEVEL1("Application exiting with rc %d \n",rc);
	return OBD2_LIB_SUCCESS;
}


