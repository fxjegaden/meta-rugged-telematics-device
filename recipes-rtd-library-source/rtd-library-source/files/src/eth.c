#define _GNU_SOURCE
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "init.h"
#include "lib_common.h"
#include "obd2lib.h"

#define MAC0_ADDR "/sys/fsl_otp/HW_OCOTP_MAC0"
#define MAC1_ADDR "/sys/fsl_otp/HW_OCOTP_MAC1"
#define WAIT_ETH_UP 1

int read_fused_mac_addr(char *mac_addr)
{
	int fp0,fp1;
	size_t sz = 0;

	char buf_msb[11] = {0};
	char buf_lsb[11] = {0};

	unsigned long long msb;
	unsigned long long lsb;

	/* read unique id lsb */
	fp0 = open(MAC0_ADDR, O_RDONLY);
	if (fp0 == -1){
		perror("MAC open_fp0");
		return E_OBD2_LIB_ETH_MAC0_ADDR;
	}

	/*Reading lsb of MAC Addr*/
	sz = read(fp0, buf_msb, 10);
	if ((signed)sz == -1)
		perror ("MAC read_fp0");
	close(fp0);
	printf("MSB : %s\n", buf_msb);
	msb = strtoll(buf_msb, NULL, 0);
	printf("MSB : %llx\n", msb);

	/* read unique id msb */
	fp1 = open(MAC1_ADDR, O_RDONLY);
	if (fp0 == -1){
		perror("MAC open_fp1");
		return E_OBD2_LIB_ETH_MAC1_ADDR;
	}

	/*Reading msb of MAC Addr*/
	sz = read(fp1, buf_lsb, 10);
	if ((signed)sz == -1)
		perror ("MAC read_fp1");
	close(fp1);
	printf("LSB : %s\n", buf_lsb);
	lsb = strtoll(buf_lsb, NULL, 0);
	printf("LSB : %llx\n", lsb);

	sprintf(mac_addr,"%08llx%04llx", msb, lsb);
	printf("mac_addr value %s\n", mac_addr );

	return SUCCESS;
}

/*Function used to bringing up the ethernet interface*/
/* BUG ID 5270 */
int eth_init( )
{
	int ret = OBD2_LIB_FAILURE;

	/* Turn ON the Ethernet Reset GPIO */
	ret = set_gpio_value(68, ON);
	if(!ret)
	{
		ret = Check_eth_Link();
		if(ret)
		{
			ret = system("ifconfig eth0 up");
			if( ret == OBD2_LIB_FAILURE )
			{
				ret = E_OBD2_LIB_ETH_INIT;
			}
			else
			{
				sleep(WAIT_ETH_UP);
				ret = Check_eth_Link();
			}
		}
		else
		{
			ret = OBD2_LIB_SUCCESS;
		}
	}
	else
	{
		CHK_ERR (ret, stderr, "Error in set_gpio_value ()-68");
		ret = E_OBD2_LIB_ETH_INIT;
	}

	return ret;
}

int eth_deinit()
{
	int ret = SUCCESS;
	int error = SUCCESS;

	ret = Check_eth_Link();
	if(!ret)
	{
		ret = system("ifconfig eth0 down");
		if( ret == OBD2_LIB_FAILURE )
		{
			error = E_OBD2_LIB_ETH_DEINIT;
		}
		else
		{
			/* Turn OFF the Ethernet Reset GPIO */
			ret = set_gpio_value(68, OFF);
			if(ret)
			{
				error = E_OBD2_LIB_ETH_DEINIT;
			}
			else
			{
				// Do Nothing
			}
		}
	}
	else
	{
		ret = OBD2_LIB_SUCCESS;
	}

	return error;
}
