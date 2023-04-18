#define _GNU_SOURCE
#include "4g.h"

int nl_socket = OBD2_LIB_SUCCESS;
static char cpin_user_input_monitor[50];
/* BUG ID 5281 */
/*Check GSM Network Connection*/
int check_gsm_nw_connection_monitor()
{
	int ret = OBD2_LIB_SUCCESS;
	int sim_info_monitor = 0;
	ret = get_gsm_sim_status_monitor(&sim_info_monitor);
	CHK_ERR (ret, stderr, "Error: check_gsm_nw_connection_monitor() Read SIM Status");
	if(sim_info_monitor != E_GSM_SIM_DETECTED)
		return sim_status_monitor;

	ret = CheckLink("ppp0");
	if (ret != OBD2_LIB_SUCCESS){
		ret = E_GSM_NW_CONNECTION_DOWN;
		CHK_ERR (ret, stderr, "Error: GSM Network Connection Down");
	}

	return ret;
}

/*Set GSM Flight Mode ON*/
int set_gsm_flight_mode_on_monitor()
{
	int ret = 0;
	char val[50] = {0};
	ret = gsm_at_cmd(SET_FLIGHT_MODE_ON, val, sizeof( val ), 300000);
	CHK_ERR (ret, stderr, "Error: Enable GSM Flight Mode");
	return ret;
}

/*Set GSM Flight Mode OFF*/
int set_gsm_flight_mode_off_monitor()
{
	int ret = 0;
	char val[150] = {0};
	struct timespec time_out;
	struct timeval start_modem_on, end_modem_on;
	int duration_modem_on = 0;

	if (clock_gettime(CLOCK_REALTIME, &time_out) == -1)
		IOBD_DEBUG_LEVEL2 ("clock_gettime error");
	time_out.tv_sec += 1;

	gettimeofday(&start_modem_on, NULL);

	ret = gsm_at_cmd(SET_FLIGHT_MODE_OFF, val, sizeof( val ), 3000000);
	CHK_ERR (ret, stderr, "Error: Disable GSM Flight Mode");
	if( ret != OBD2_LIB_SUCCESS )
	{
		goto exit;
	}

	while(1){
		gettimeofday(&end_modem_on, NULL);
		duration_modem_on = ((end_modem_on.tv_usec-start_modem_on.tv_usec)+(end_modem_on.tv_sec-start_modem_on.tv_sec)*1000000)/1000000;

		if (strstr (val,"CPIN")){
			if (strstr(val,"READY")){
			}
			else if (strstr(val,"NOT")){
				printf ("%s - sim status NOT INSERTED\n", __FUNCTION__ );
				ret = E_GSM_SIM_NOT_DETECTED;
				break;
			}
		}
		if (strstr(val,"PB DONE"))
			break;
		if( duration_modem_on > 10 )
		{
			break;
		}
	}

exit:
	return ret;
}
/* BUG ID 5281 */
/*GSM Module ON*/
int gsm_modem_on_monitor(char *cpin, int length)
{
	int ret = OBD2_LIB_SUCCESS;
	int error = OBD2_LIB_SUCCESS;
	int count = 0;
	char val[120] = {0};
	char *cpin_cmd = NULL;
	memset(cpin_user_input_monitor,0,sizeof(cpin_user_input_monitor));
	if( length > sizeof( cpin_user_input_monitor ) - 1 )
	{
		return E_BUFFER_READ_OVERFLOW;
	}
	strncpy(cpin_user_input_monitor, cpin, sizeof(cpin));
	ret = check_gsm_modem_status_monitor();
	if (ret == OBD2_LIB_SUCCESS)
		return ret;

	ret = set_gpio_value(90, ON);
	CHK_ERR (ret, stderr, "Error: gsm_modem_on_monitor() set CELLULAR MODULE SWITCH Highh");
	sleep(1);

	ret = set_gpio_value(78, ON);
	CHK_ERR (ret, stderr, "Error: gsm_modem_on_monitor() set USB SWITCH FOR CELLULAR MODULE HIGH");
	sleep(1);

	ret = set_gpio_value(88, OFF);
	CHK_ERR (ret, stderr, "Error: gsm_modem_on_monitor() set CELLULAR MODULE PWRKEY LOW");
	sleep(1);

	ret = set_gpio_value(88, ON);
	CHK_ERR (ret, stderr, "Error: gsm_modem_on_monitor() set CELLULAR MODULE PWRKEY HIGH");
	sleep(1);

	ret = set_gpio_value(88, OFF);
	CHK_ERR (ret, stderr, "Error: gsm_modem_on_monitor() set CELLULAR MODULE PWRKEY LOW");
	sleep(1);

	while (1)
	{
		ret = check_gsm_modem_status_monitor();
		if (ret == OBD2_LIB_SUCCESS)
		{
			break;
		}
		else
		{
			sleep(1);
			printf("PORT NOT Connected\n");
			if (count > 10){
				break;
			}
			count++;
		}
	}

	sleep( 2 );
	sim_status_monitor = OBD2_LIB_FAILURE;

	if (ret == OBD2_LIB_SUCCESS)
	{
		memset(val,0,sizeof(val));
		ret = gsm_at_cmd("dummy_buf", val, sizeof( val ), 5000000);
		if (ret == OBD2_LIB_SUCCESS)
		{
			if (strstr (val,"CPIN"))
			{
				if (strstr(val,"READY"))
				{
					sim_status_monitor = E_GSM_SIM_DETECTED;
				}
				if (strstr(val,"NOT"))
				{
					sim_status_monitor = E_GSM_SIM_NOT_DETECTED;
				}
				if (strstr(val,"SIM PIN"))
				{
					memset(val,0,sizeof(val));
					ret = asprintf(&cpin_cmd, "AT+CPIN=%s",cpin_user_input_monitor);
					if(cpin_cmd == NULL)
					{
						ret = -ENOMEM;
						goto exit;
					}
					ret = gsm_at_cmd(cpin_cmd,val, sizeof( val ), 5000000);
					if( ret != OBD2_LIB_SUCCESS )
					{
						goto exit;
					}
					else
					{
						if (strstr (val,"READY"))
						{
							sim_status_monitor = E_GSM_SIM_DETECTED;
						}
						else
						{
							memset(val,0,sizeof(val));
							ret = gsm_at_cmd(READ_SIMSTATUS, val, sizeof( val ), 5000000);
							if( ret != OBD2_LIB_SUCCESS )
							{
								goto exit;
							}
							else
							{
								if (strstr(val,"READY"))
								{
									sim_status_monitor = E_GSM_SIM_DETECTED;
								}
								if (strstr(val,"NOT"))
								{
									sim_status_monitor = E_GSM_SIM_NOT_DETECTED;
								}
								if (strstr(val,"NOT") || (strstr(val,"+CME ERROR")))
								{
									sim_status = E_GSM_SIM_NOT_DETECTED;
									CHK_ERR (sim_status, stderr, "Error: get_gsm_sim_status() Sim Not Detectdd");
								}


							}
						}
					}
				}
			}
		}
	}
	else
	{
		return E_GSM_USB_DISCONNECT;
	}
	ret = set_gsm_network_mode_monitor(1);
	ret = check_gsm_modem_status_monitor();

	sleep (10);
exit:
	if(cpin_cmd != NULL)
	{
		free(cpin_cmd);
		cpin_cmd = NULL;
	}
	return ret;
}

/*GSM Module OFF*/
int gsm_modem_off_monitor()
{
	int ret = OBD2_LIB_SUCCESS;
	int error, count = OBD2_LIB_SUCCESS;

	ret = system("ifconfig ppp0 down");
	if (ret != OBD2_LIB_SUCCESS){
		CHK_ERR (ret, stderr, "Error: ppp0 down");
	}

	ret = system("killall pppd");
	if (ret != OBD2_LIB_SUCCESS){
		CHK_ERR (ret, stderr, "Error: killall pppd");
	}

	if (ret != OBD2_LIB_SUCCESS)
	{
		error = ret;
		printf("\nError: Error in deinitialising GSM modules %x",error);
	}

	ret = set_gpio_value(88, OFF);
	CHK_ERR (ret, stderr, "Error: gsm_modem_off_monitor() set gpio-88 value OFF ");
	sleep(1);

	ret = set_gpio_value(88, ON);
	CHK_ERR (ret, stderr, "Error: gsm_modem_off_monitor() set gpio-88 value ON ");
	sleep(1);

	ret = set_gpio_value(88, OFF);
	CHK_ERR (ret, stderr, "Error: gsm_modem_off_monitor() set gpio-88 value OFF ");
	sleep (1);

	ret = set_gpio_value(78, OFF);
	CHK_ERR (ret, stderr, "Error: gsm_modem_off_monitor() set USB SWITCH FOR CELLULAR MODULE OFF ");
	sleep (1);

	ret = set_gpio_value(90, OFF);
	CHK_ERR (ret, stderr, "Error: gsm_modem_off_monitor() set CELLULAR MODULE SWITCH OFF ");
	sleep (1);

	while (1)
	{
		ret = check_gsm_modem_status_monitor();
		printf(" the return val of check_gsm_modem_status_monitor is %x\n ", ret);
		if (ret == OBD2_LIB_SUCCESS)
		{
			sleep(1);
			printf("PORT Connected\n");
			if (count > 30){
				ret = OBD2_LIB_FAILURE;
				break;
			}
			count++;
		}
		else
		{
			ret = OBD2_LIB_SUCCESS;
			break;
		}
	}

	return ret;
}

/* BUG ID 5281 */
/*Get GSM SIM Status*/
int get_gsm_sim_status_monitor(int *sim_status_val_monitor)
{
	int ret = OBD2_LIB_SUCCESS;
	int count = OBD2_LIB_SUCCESS;
	char val[ 256 ] = {0};
	char *cpin_cmd = NULL;
	memset (sim_status_val_monitor , 0, sizeof (sim_status_val_monitor));
	if (sim_status_val_monitor == NULL){
		ret = E_NULL_PARAMETER;
		return ret;
	}


	memset(val, 0x0, sizeof (val));
	ret = check_gsm_modem_status_monitor();
	if (ret != OBD2_LIB_SUCCESS){
		sim_status_monitor = E_GSM_SIM_STATUS_UNKNOWN;
		goto exit;
	}


	while (1)
	{
		ret = gsm_at_cmd(READ_SIMSTATUS, val, sizeof( val ), 5000000);
		CHK_ERR (ret, stderr, "Error: Read GSM SIM Status");
		if (ret == OBD2_LIB_SUCCESS)
		{
			if (strstr(val,"READY"))
			{
				sim_status_monitor = E_GSM_SIM_DETECTED;
				break;
			}
			else if (strstr(val,"SIM PIN"))
			{
				ret = asprintf(&cpin_cmd, "AT+CPIN=%s",cpin_user_input_monitor);
				if(ret < 0)
				{
					ret = -ENOMEM;
					goto exit;
				}
				ret = gsm_at_cmd(cpin_cmd,val, sizeof( val ), 5000000);
				if( ret != OBD2_LIB_SUCCESS )
				{
					goto exit;
				}
				else
				{
					if (strstr (val,"READY"))
					{
						sim_status_monitor = E_GSM_SIM_DETECTED;
						break;
					}
					else
					{
						if(cpin_cmd != NULL)
						{
							free(cpin_cmd);
							cpin_cmd = NULL;
						}
					}
				}
			}
			else if (strstr(val,"NOT") || (strstr(val,"+CME ERROR")))
			{
				sim_status_monitor = E_GSM_SIM_NOT_DETECTED;
				CHK_ERR (sim_status_monitor, stderr, "Error: get_gsm_sim_status_monitor() Sim Not Detected");
				break;
			}
			else
			{
				if (count > 5){
					sim_status_monitor = E_GSM_SIM_NOT_DETECTED;
					break;
				}
				count++;
			}
		}
		else
		{
		}
	}
exit:
	*sim_status_val_monitor = sim_status_monitor;
	if(cpin_cmd != NULL)
	{
		free(cpin_cmd);
		cpin_cmd = NULL;
	}

	return ret;
}
/* BUG ID 5281 */
/*Set GSM Network Mode*/
int set_gsm_network_mode_monitor (int type)
{
	char val [64] = {0};
	int ret = OBD2_LIB_SUCCESS;
	int sim_info_monitor = 0;


	sim_status_monitor = get_gsm_sim_status_monitor(&sim_info_monitor);
	CHK_ERR (sim_status_monitor, stderr, "Error: set_gsm_network_mode_monitor() Read SIM Status");
	if(sim_status_monitor != OBD2_LIB_SUCCESS)
		return sim_status_monitor;

	if (type == MODE_TYPE_2G){
		ret = gsm_at_cmd(SET_MODE_2G, val, sizeof( val ), 300000);
		CHK_ERR (ret, stderr, "Error: set_gsm_network_mode_monitor() Set GSM 2G Mode");
	}
	else if (type == MODE_TYPE_3G){
		ret = gsm_at_cmd(SET_MODE_3G, val, sizeof( val ), 300000);
		CHK_ERR (ret, stderr, "Error: set_gsm_network_mode_monitor() Set GSM 3G Mode");
	}
	else if (type == MODE_TYPE_4G){
		ret = gsm_at_cmd(SET_MODE_4G, val, sizeof( val ), 300000);
		CHK_ERR (ret, stderr, "Error: set_gsm_network_mode_monitor() Set GSM 3G Mode");
	}
	else if (type == MODE_TYPE_AUTO){
		ret = gsm_at_cmd(SET_AUTO_MODE, val, sizeof( val ), 300000);
		CHK_ERR (ret, stderr, "Error: set_gsm_network_mode_monitor Set GSM Auto Mode");
	}

	return ret;
}
/* BUG ID 5281 */
/* Read GSM Signal Strength */
int get_gsm_signal_strength_monitor(char *signal_lvl, int length)
{
	char val[100] = {0};
	int ret = OBD2_LIB_SUCCESS;
	char *signal_ptr = NULL;
	int sim_info_monitor = 0;

	ret = get_gsm_sim_status_monitor(&sim_info_monitor);
	CHK_ERR (ret, stderr, "Error: get_gsm_signal_strength_monitor() Read SIM Status");
	if(sim_info_monitor != OBD2_LIB_SUCCESS)
		sim_status_monitor = sim_info_monitor;
	printf ("*********************** get_gsm_signal_strength_monitor **********************\n");
	ret = gsm_at_cmd(READ_GSM_SIGNAL_STRENGTH, val, sizeof( val ), 300000);
	CHK_ERR (ret, stderr, "Error: get_gsm_signal_strength_monitor() Get GSM Singal Strength");
	if (ret == OBD2_LIB_SUCCESS){
		signal_ptr = strtok(val, ":");
		if (signal_ptr != NULL)
		{
			signal_ptr = strtok(NULL, ",");
			if( signal_ptr != NULL )
			{
				if( length < strlen(signal_ptr) + 1 )
				{
					ret = E_BUFFER_READ_OVERFLOW;
					goto exit;
				}
				strcpy(signal_lvl,signal_ptr+1);
			}
			else
			{
				ret = E_GSM_SIM_STRENGTH_ERROR;
				goto exit;
			}
		}
		else
		{
			ret = E_GSM_SIM_STRENGTH_ERROR;
			goto exit;
		}
	}

exit:
	return ret;
}
/* BUG ID 5281 */
/* Read GSM Network Registration */
int get_gsm_nw_reg_monitor(char *cell_id, int cell_id_len, char *lac, int lac_len)
{
	char val[100] = {0};
	int ret = OBD2_LIB_SUCCESS;
	char *lac_ptr = NULL;
	char* c_id_ptr = NULL;
	int sim_info_monitor = 0;

	ret = get_gsm_sim_status_monitor(&sim_info_monitor);
	CHK_ERR (ret, stderr, "Error: get_gsm_nw_reg_monitor() Read SIM Status");
	if(sim_info_monitor != E_GSM_SIM_DETECTED)
	{
		return sim_status_monitor;
	}
	ret = set_gsm_nw_reg_loc_on_monitor();
	CHK_ERR (ret, stderr, "Error: get_gsm_nw_reg_monitor() Get GSM Network Registration");
	sleep (5);
	if(ret != OBD2_LIB_SUCCESS)
	{
		return E_GSM_SIM_REG_ERROR;
	}
	ret = gsm_at_cmd(READ_GSM_SIM_REGISTRATION, val, sizeof( val ), 300000);
	CHK_ERR (ret, stderr, "Error: get_gsm_nw_reg_monitor() Get GSM Network Registration");
	if (ret == OBD2_LIB_SUCCESS){
		c_id_ptr = strtok(val, "\"");
		if ( c_id_ptr != NULL)
		{
			c_id_ptr = strtok(NULL, "\"");
			if( c_id_ptr != NULL )
			{
				if( cell_id_len < strlen( c_id_ptr ) + 1 )
				{
					ret = E_BUFFER_READ_OVERFLOW;
					goto exit;
				}
				strcpy(cell_id, c_id_ptr);
			}
			else
			{
				ret = E_GSM_SIM_REG_ERROR;
				goto exit;
			}
		}
		else
		{
			ret = E_GSM_SIM_REG_ERROR;
			goto exit;
		}

		lac_ptr = strtok(NULL, "\"");
		if(lac_ptr != NULL){
			lac_ptr = strtok(NULL, "\"");
			if( lac_ptr != NULL )
			{
				if( lac_len < strlen( lac_ptr ) + 1 )
				{
					ret = E_BUFFER_READ_OVERFLOW;
					goto exit;
				}
				strcpy(lac, lac_ptr);
			}
			else
			{
				ret = E_GSM_SIM_REG_ERROR;
				goto exit;
			}
		}
		else
		{
			ret = E_GSM_SIM_REG_ERROR;
			goto exit;
		}
	}

exit:
	return ret;
}

/*Enable GSM Network Registration With Location*/
int set_gsm_nw_reg_loc_on_monitor()
{
	int rc =0;
	char resp[20] = {0};

	rc = gsm_at_cmd(SET_GSM_SIM_REG_WITH_LOC_ON, resp, sizeof( resp ), 300000);
	CHK_ERR (rc, stderr, "Error: set_gsm_nw_reg_loc_on_monitor() Set GSM Network Registration Location ON");

	return rc;
}

/*Enable GSM Network Registration With Location*/
int set_gsm_nw_reg_loc_off_monitor()
{
	int rc =0;
	char resp[20] = {0};

	rc = gsm_at_cmd(SET_GSM_SIM_REG_WITH_LOC_OFF, resp, sizeof( resp ), 300000);
	CHK_ERR (rc, stderr, "Error: set_gsm_nw_reg_loc_off_monitor() Set GSM Network Registration Location OFF");

	return rc;
}

int check_gsm_modem_status_monitor()
{
	int ret = 0;
	if( ( ( access(GSM_GPS_PORT, F_OK) == OBD2_LIB_SUCCESS ) && ( access(GSM_LOG_PORT, F_OK) == OBD2_LIB_SUCCESS ) &&\
				( access(GSM_AT_PORT, F_OK) == OBD2_LIB_SUCCESS ) && ( access(GSM_CONNECTION_PORT, F_OK) == OBD2_LIB_SUCCESS ) ) == 0 )
	{
		ret = E_GSM_USB_DISCONNECT;
		CHK_ERR (ret, stderr, "Error: check_gsm_modem_status_monitor() Read GSM Status ON/OFF");
	}
	else
	{
		ret = OBD2_LIB_SUCCESS;
	}

	return ret;
}

int check_network_connection_monitor()
{
	int ret = 0;
	char tmp[8192];
	struct addrinfo *res, hints;
	struct sockaddr_in sa; /* input */
	socklen_t len;
	memset (&hints, 0, sizeof (hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0; // no enum : possible value can be read in /etc/protocols
	hints.ai_flags = AI_CANONNAME | AI_ALL | AI_ADDRCONFIG;

	ret = getaddrinfo("google.com", NULL, &hints, &res);
	printf("%s - getaddrinfo google.com err return value %d\n", __FUNCTION__, ret );
	if( ret != 0 )
	{

		perror ("getaddrinfo");
		ret = getaddrinfo("pool.ntp.org", NULL, &hints, &res);
		printf("%s - getaddrinfo pool.ntp.org err return value %d\n", __FUNCTION__, ret );
		if( ret != 0 )
		{
			perror ("getaddrinfo");

			memset(&sa, 0, sizeof(struct sockaddr_in));
			memset( tmp, 0, sizeof( tmp ) );
			sa.sin_family = AF_INET;
			sa.sin_addr.s_addr = inet_addr("8.8.8.8");
			len = sizeof(struct sockaddr_in);

			if (getnameinfo((struct sockaddr *) &sa, len, tmp, sizeof(tmp),
						NULL, 0, NI_NAMEREQD))
			{
				IOBD_DEBUG_LEVEL1("%s - could not resolve hostname of DNS Server 8.8.8.8 and host is = %s\n", __FUNCTION__, tmp );
				ret = -1;
			}
			else
			{
				IOBD_DEBUG_LEVEL3("LINK UP!!!!\n");
				ret = 0;
				if( res != NULL )
				{
					freeaddrinfo(res);
					res = NULL;
				}
			}
		}
		else
		{
			IOBD_DEBUG_LEVEL3("LINK UP!!!!\n");
			ret = 0;
			if( res != NULL )
			{
				freeaddrinfo(res);
				res = NULL;
			}
		}
	}
	else
	{
		IOBD_DEBUG_LEVEL3("LINK UP!!!!\n");
		ret = 0;
		if( res != NULL )
		{
			freeaddrinfo(res);
			res = NULL;
		}
	}

	return ret;
}
/* BUG ID 5281 and 5288 */
int establish_nw_connection_monitor()
{
	int sim_info_monitor = OBD2_LIB_FAILURE ;
	int ret = OBD2_LIB_FAILURE;

	struct timespec time_out;


	ret = CheckLink("ppp0");
	if (ret == OBD2_LIB_SUCCESS){
		printf("\n network_manger - Network up\n");
	}
	else{
		ret = system ("pppd call gprs_4g");
	}
	sleep (3);
	ret = CheckLink("ppp0");
	if (ret == OBD2_LIB_SUCCESS){
		printf("\n network_manger - Network up\n");
	} else
	{
		get_gsm_sim_status_monitor(&sim_info_monitor);
		ret=sim_info_monitor;
	}

	return ret;
}
/* BUG ID 5281 and 5288 */
int establish_connection_monitor()
{
	int ret = OBD2_LIB_FAILURE;
	struct timespec time_out;
	int sim_info_monitor = OBD2_LIB_FAILURE;

	int time_diff = 0;
	struct timespec start, end;
	int network_mode = 4;
	int current_mode = 0;
	int status = 1;
	clock_gettime(CLOCK_MONOTONIC, &start);
	ret = get_gsm_sim_status_monitor(&sim_info_monitor);
	if(ret == 0)
	{
		if (sim_info_monitor == E_GSM_SIM_DETECTED){
			while(1)
			{
				ret = establish_nw_connection_monitor();
				if (ret == OBD2_LIB_SUCCESS)
				{
					break;
				}
				else if (ret != E_GSM_SIM_DETECTED)
				{
					break;
				}

				usleep (1000000);
				clock_gettime(CLOCK_MONOTONIC, &end);
				time_diff = time_diff_seconds(&end, &start);
				if(time_diff >= 60)
				{
set_mode:
					ret = CheckLink("ppp0");
					if (ret != OBD2_LIB_SUCCESS){
						system("killall -9 pppd");
					}

					if(network_mode <= 0)
					{
						ret = -1;
						break;
					}
					ret = set_gsm_network_mode_monitor(network_mode);	
					network_mode = network_mode - 1;
					if(ret == 0)
					{
						memset(&start,0,sizeof(start));
						memset(&start,0,sizeof(end));
						clock_gettime(CLOCK_MONOTONIC, &start);
						time_diff = 0;
						continue;
					}
					else
					{
						goto set_mode;
					}
				}
			}
		}else{
			ret = sim_info_monitor;
		}



	}

	return ret;
}

int read_event( )
{
	int rc, status = 0;
	char buf[4096] = {0};
	struct iovec iov = {buf, sizeof buf };
	struct sockaddr_nl snl;
	struct msghdr msg = { (void *) &snl, sizeof snl, &iov, 1, NULL, 0, 0 };
	struct nlmsghdr *h;
	char ifname[1024] = {0};
	struct ifinfomsg *ifi;

recv:
	status = recvmsg ( nl_socket, &msg, 0);

	if (status <= 0) {
		printf("Error has occurred\r\n");
		return -1;
	}
	else {
		for (h = (struct nlmsghdr *) buf; NLMSG_OK (h, (unsigned int) status);h = NLMSG_NEXT (h, status)) {
			if (h->nlmsg_type == NLMSG_DONE) {
				return 0;
			}
			else if (h->nlmsg_type == RTM_NEWLINK) {
				ifi = NLMSG_DATA(h);
				if_indextoname (ifi->ifi_index,ifname);
				printf("iwave: netlink_link_state: Link %s %s\n",ifname,(ifi->ifi_flags & IFF_RUNNING)?"Up":"Down");

				if(((ifi->ifi_flags & IFF_RUNNING) == 0) && (strcmp(ifname, "ppp0") == 0)){
					printf("link_thread - read_event ppp0 LINK_DOWN !!!\n" );
					/* if link status was up before, then kill pppd */
					rc = system("ifconfig ppp0 down");
					rc = system("killall pppd");
					return -1;
				}
				else
				{
					rc = CheckLink("ppp0");
					if (rc != OBD2_LIB_SUCCESS){
						return -1;
					}
					else
					{
						goto recv;
					}
				}
			}
			else
			{
				rc = CheckLink("ppp0");
				if (rc != OBD2_LIB_SUCCESS){
					return -1;
				}
				else
				{
					goto recv;
				}
			}
		}
		return 0;
	}
}

int link_listener (void)
{
	int read_event_ret, optval = 0;
	struct sockaddr_nl addr;
	int ret = 0;

	// Netlink socket creation
	nl_socket = socket (AF_NETLINK, SOCK_DGRAM, IPPROTO_IP);
	printf("%s - nl_socket is %d\n", __FUNCTION__, nl_socket);

	// Socket open error
	if (nl_socket == -1) {
		printf ("Netlink socket open error, error number %d\n", errno);
	}
	else
	{
		memset ((void *) &addr, 0, sizeof (addr));

		/*
		 * setsockopt: Handy debugging trick that lets \
		 * us rerun the server immediately after we kill it; \
		 * otherwise we have to wait about 20 secs. \
		 * Eliminates "ERROR on binding: Address already in use" error. \
		 * */
		optval = 1;
		if( setsockopt( nl_socket, SOL_SOCKET, SO_REUSEADDR,
					(const void *)&optval , sizeof( int ) ) < 0 )
		{
			printf("%s - setsockopt(SO_REUSEADDR) failed with errno %d", __FUNCTION__, errno );
		}
		else
		{
		}

		// Kernel user interface
		addr.nl_family = AF_NETLINK;

		// Get the process ID
		addr.nl_pid = getpid ();

		/* Listen to the RTMGRP_LINK (network interface create/delete/up/down events) \
		 * and RTMGRP_IPV4_IFADDR (IPv4 addresses add/delete events) */
		addr.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR;
		// Bind the socket an address to netlink group
		if (bind (nl_socket, (struct sockaddr *) &addr, sizeof (addr)) < 0){
			printf("Netlink socket bind failed, error number %d\n", errno);
			if( nl_socket > 0 )
			{
				ret = tcflush(nl_socket, TCIOFLUSH);
				close( nl_socket );
				nl_socket = 0;
			}
		}else
		{
			while (1)
			{
				read_event_ret = read_event (nl_socket);
				if (read_event_ret < 0)
					printf("Netlink read_event() error, error number %d\n", errno);
				if( nl_socket )
				{
					printf("%s - closing socket nl_socket %d\n", __FUNCTION__, nl_socket );
					ret = tcflush(nl_socket, TCIOFLUSH);
					close( nl_socket );
					nl_socket = 0;
				}
				return read_event_ret;
			}
		}
	}
	return 0;
}

/* BUG ID 5280 and 5281 */
int network_manager( void )
{
	int rc = 0;
	int sim_reg_count = 0;
	char arr_sig_strngth[100] = {0};
	char cell_id[50] = {0},lac[30] = {0};
	int sig_strength = 0;
	int arr_sig_strength = 0;
	int network_link_count = 0;
	int network_connection_count = 0;
	int ntp_server_update_count = 0;
	int link_status = 0;

	printf("\n############[GSM Modem]##############\n");
	while( 1 )
	{
		if( network_status == -1 )
		{
			rc = gsm_at_port_close();
			printf("%s - gsm_at_port_close rc value 0x%x\n", __FUNCTION__, rc );
			printf("Exiting %s thread!!!\n", __FUNCTION__ );
			if( nl_socket > 0 )
			{
				rc = tcflush(nl_socket, TCIOFLUSH);
				close( nl_socket );
				nl_socket = 0;
			}
			network_status = 0;
			break;
		}
		/* checking whether modem is on or off. */
		rc = check_gsm_modem_status_monitor();
		printf("%s - check_gsm_modem_status_monitor rc value 0x%x\n", __FUNCTION__, rc );
		if(rc != 0)
		{
			/* Turning on the GSM modem */
			rc = gsm_modem_on_monitor( "0000", 4 );
			printf("%s - gsm_modem_on_monitor() Return value = 0x%x\n", __FUNCTION__, rc);
			if( rc != 0 )
			{
				rc = gsm_at_port_close();
				printf("%s - gsm_at_port_close rc value 0x%x\n", __FUNCTION__, rc );
				continue;
			}
			else
			{
				/*BUG ID 5310 */
				rc = gps_init( );
				printf("%s - gps_init rc value 0x%x\n", __FUNCTION__, rc );

				while( 1 )
				{
					/* Getting the SIM registration details */
					memset( cell_id, 0, sizeof( cell_id ) );
					memset( lac, 0, sizeof( lac ) );
					rc = get_gsm_nw_reg_monitor(cell_id, sizeof( cell_id ), lac, sizeof( lac ));
					printf("%s - CELL_ID %s LAC %s\n", __FUNCTION__, cell_id,lac);
					if( ( strlen( cell_id ) != 0 ) && ( strlen( lac ) != 0 ) )
					{
						printf("%s - cell_id return value %s\n", __FUNCTION__, cell_id );
						printf("%s - lac return value %s\n", __FUNCTION__, lac );
						break;
					}
					else
					{
						rc = set_gsm_flight_mode_on_monitor();
						printf("%s - set_gsm_flight_mode_on() Return value = 0x%x\n", __FUNCTION__, rc);
						sleep( 2 );
						rc = set_gsm_flight_mode_off_monitor();
						printf("%s - set_gsm_flight_mode_off() Return value = 0x%x\n", __FUNCTION__, rc);
						sleep( 10 );

						sim_reg_count++;
						if( sim_reg_count == 10 )
						{
							break;
						}
						else
						{
							continue;
						}
					}
				}
				if( sim_reg_count == 10 )
				{
					sim_reg_count = 0;
					continue;
				}else{
				}

				while( 1 )
				{
					/* Getting the signal range details of the network */
					memset( arr_sig_strngth, 0, sizeof( arr_sig_strngth ) );
					rc = get_gsm_signal_strength_monitor(arr_sig_strngth, sizeof( arr_sig_strngth ));
					printf("%s - gsm signal[%s] = 0x%x\n", __FUNCTION__, arr_sig_strngth, rc);
					arr_sig_strength = atoi( arr_sig_strngth );
					printf("%s - gsm signal = %d\n", __FUNCTION__, arr_sig_strength );
					if( rc != 0 )
					{
						sig_strength++;
						if( sig_strength > 3 )
						{
							break;
						}
					}
					else
					{
						if( arr_sig_strength > 10 )
						{
							break;
						}
						else
						{
							sleep( 5 );
							continue;
						}
					}
				}
				if( sig_strength > 3 )
				{
					sig_strength = 0;
					arr_sig_strength = 0;
					continue;
				}
				if( arr_sig_strength < 10 )
				{
					sig_strength = 0;
					arr_sig_strength = 0;
					continue;
				}

				while (1){
					/*Establishing ppp network connection */
					rc = establish_connection_monitor();
					if(rc == 0) {
						/* Checking whether the internet connectivity is active or not */
						rc = check_network_connection_monitor();
						if( rc == 0 )
						{
							break;
						}
						else{
							printf("%s - Trying to Connect!!!!\n", __FUNCTION__ );
							network_link_count++;
							sleep(1);
							if ( network_link_count > 1){
								printf("%s - Network Connection Not Established\n", __FUNCTION__);
								break;
							}
						}
					}
					else
					{
						network_connection_count++;
						sleep(1);
						if (network_connection_count > 1){
							printf("%s - Network Connection Not Established\n", __FUNCTION__ );
							break;
						}
					}
				}
				if( network_connection_count > 1 || network_link_count > 1 )
				{
					network_connection_count = 0;
					network_link_count = 0;
					continue;
				}


				if( network_status == -1 )
				{
					rc = gsm_at_port_close();
					printf("%s - gsm_at_port_close rc value 0x%x\n", __FUNCTION__, rc );
					printf("Exiting %s thread!!!\n", __FUNCTION__ );
					network_status = 0;
					break;
				}

				while( 1 )
				{
					link_status = link_listener( );
					if( link_status < 0 )
					{
						break;
					}
					else
					{
						continue;
					}
				}
			}
		}
		else
		{
			rc = gsm_at_port_close();
			printf("%s - gsm_at_port_close rc value 0x%x\n", __FUNCTION__, rc );
			continue;
		}
	}
	printf("%s - exits!!!\n", __FUNCTION__ );

	return rc;
}

int network_monitor_disable( )
{
	printf("Inside function %s\n", __FUNCTION__ );
	int ret, detach_count = 0;
	if( network_status == 1 )
	{
		ret = CheckLink("ppp0");
		if (ret != OBD2_LIB_SUCCESS){
			ret = E_GSM_NW_CONNECTION_DOWN;
			CHK_ERR (ret, stderr, "Error: GSM Network Connection Down");
			network_status = -1;
			while( 1 )
			{
				if( network_status == 0 )
				{
					break;
				}
				if( nl_socket > 0 )
				{
					ret = tcflush(nl_socket, TCIOFLUSH);
					close( nl_socket );
					nl_socket = 0;

				}
				detach_count++;
				if( detach_count >= 10 )
				{
					detach_count = 0;
					ret = OBD2_LIB_FAILURE;
				}
				sleep( 1 );
			}
			ret = OBD2_LIB_SUCCESS;
		}
		else
		{
			network_status = -1;
			ret = system("ifconfig ppp0 down");
			ret = system("killall pppd");
			while( 1 )
			{
				if( network_status == 0 )
				{
					break;
				}
				detach_count++;
				if( detach_count >= 10 )
				{
					detach_count = 0;
					if( nl_socket > 0 )
					{
						ret = tcflush(nl_socket, TCIOFLUSH);
						close( nl_socket );
						nl_socket = 0;

					}
				}
				sleep( 1 );
			}
			ret = OBD2_LIB_SUCCESS;
		}
	}
	else
	{
		ret = OBD2_LIB_FAILURE;
	}

exit:
	return ret;
}
