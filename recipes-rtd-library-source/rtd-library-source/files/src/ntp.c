#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "init.h"
#include "4g.h"
#include "error_nos.h"
#include "thread.h"

/* BUG ID 5282 */
int ntp_server_update()
{
	int ret = 0;
	int l=0;
	int sim_info = 0;

	ret = get_gsm_sim_status(&sim_info);
	CHK_ERR (ret, stderr, "Error: ntp_server_update() Read SIM Status");
	if (sim_info != E_GSM_SIM_DETECTED){
		return sim_status;
	}

	ret = CheckLink("ppp0");
	printf("ret val is %x \n",ret);
	if (ret != OBD2_LIB_SUCCESS)
		return ret;

	system("unlink /etc/localtime");
	system ("date -s 2018.07.30-00:00:00");

	system("ln -s /usr/share/zoneinfo/UTC /etc/localtime");

	ret = system("ntpdate pool.ntp.org");

	while (ret != OBD2_LIB_SUCCESS){
		++l;
		if (l == 1)
			ret = system("ntpdate jp.pool.ntp.org");
		if (ret == OBD2_LIB_SUCCESS)
			break;
		if (l == 2)
			ret = system("ntpdate id.pool.ntp.org");
		if (ret == OBD2_LIB_SUCCESS)
			break;
		if (l == 3)
			ret = system("ntpdate asia.pool.ntp.org");
		if (ret == OBD2_LIB_SUCCESS)
			break;
		if (l == 4){
			ret = E_GSM_NTP_FAILED;
			break;
		}
	}

	if (ret == OBD2_LIB_SUCCESS){
		ret = system("hwclock -w");
		CHK_ERR (ret, stderr, "Error in ntp_server_update(): set clock Failed");
	}

	return ret;
}
