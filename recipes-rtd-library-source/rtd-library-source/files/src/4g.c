#define _GNU_SOURCE
#include "4g.h"

#define WAIT_PPP_UP 3

static int loc_status = 0;
int sim_status = OBD2_LIB_FAILURE;
static char cpin_user_input[50];

/*Read GSM IMEI No.*/
int get_gsm_imei(char *imei_no, int length )
{
	char val[100] = {0};
	int rc = OBD2_LIB_SUCCESS;
	char buff_imei[30] = {0};
	char *p = NULL;
	int len = 0;
	rc = gsm_at_cmd(READ_IMEI, val, sizeof( val ), 500000);
	if(rc == OBD2_LIB_SUCCESS)
	{
		p = strstr(val,"8");
		if( p != NULL )
		{
			strncpy(buff_imei, p, 15);
			len = strlen(buff_imei);
			if( length < len+1 )
			{
				return E_BUFFER_READ_OVERFLOW;
			}
			memset(imei_no, 0x0, length);
			strcpy(imei_no, buff_imei);
		}
		else
		{
			rc = E_GSM_IMEI_READ_TIMEOUT;
		}
	}
	return rc;
}

/*Check GSM Network Connection*/
/* BUG ID 5281 */
int check_gsm_nw_connection()
{
	int ret = OBD2_LIB_SUCCESS;
	int sim_info = 0;
	ret = get_gsm_sim_status(&sim_info);
	CHK_ERR (ret, stderr, "Error: check_gsm_nw_connection() Read SIM Status");
	if (sim_info != E_GSM_SIM_DETECTED)
		return sim_status;

	ret = CheckLink("ppp0");
	if (ret != OBD2_LIB_SUCCESS){
		ret = E_GSM_NW_CONNECTION_DOWN;
		CHK_ERR (ret, stderr, "Error: GSM Network Connection Down");
	}
	return ret;
}

/*Set GSM Flight Mode ON*/
int set_gsm_flight_mode_on()
{
	int ret = 0;
	char val[50] = {0};

	if( network_enable_status == 0 )
	{
		ret = gsm_at_cmd(SET_FLIGHT_MODE_ON, val, sizeof( val ), 300000);
		CHK_ERR (ret, stderr, "Error: Enable GSM Flight Mode");
	}
	else
	{
		ret = E_GSM_NETWORK_MODE_ENABLED;
	}

	return ret;
}

/*Set GSM Flight Mode OFF*/
int set_gsm_flight_mode_off()
{
	int ret = 0;
	char val[150] = {0};
	struct timespec time_out;
	struct timeval start_modem_on, end_modem_on;
	int duration_modem_on = 0;

	if( network_enable_status == 0 )
	{
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
	}
	else
	{
		ret = E_GSM_NETWORK_MODE_ENABLED;
	}

exit:
	return ret;
}

/*GSM Module ON*/
int gsm_modem_on(char *cpin, int length)
{
	int ret = OBD2_LIB_SUCCESS;
	int error = OBD2_LIB_SUCCESS;
	int count = 0;
	char val[120] = {0};
	char *cpin_cmd = NULL;

	if( network_enable_status == 0 )
	{
		memset(cpin_user_input,0,sizeof(cpin_user_input));
		if( length > sizeof( cpin_user_input ) - 1 )
		{
			return E_BUFFER_READ_OVERFLOW;
		}
		memset( cpin_user_input, 0, sizeof( cpin_user_input ) );
		strncpy(cpin_user_input, cpin, length);
		ret = check_gsm_modem_status();
		if (ret == OBD2_LIB_SUCCESS)
			return ret;

		ret = set_gpio_value(90, ON);
		CHK_ERR (ret, stderr, "Error: gsm_modem_on() set CELLULAR MODULE SWITCH Highh");
		sleep(1);

		ret = set_gpio_value(78, ON);
		CHK_ERR (ret, stderr, "Error: gsm_modem_on() set USB SWITCH FOR CELLULAR MODULE HIGH");
		sleep(1);

		ret = set_gpio_value(88, OFF);
		CHK_ERR (ret, stderr, "Error: gsm_modem_on() set CELLULAR MODULE PWRKEY LOW");
		sleep(1);

		ret = set_gpio_value(88, ON);
		CHK_ERR (ret, stderr, "Error: gsm_modem_on() set CELLULAR MODULE PWRKEY HIGH");
		sleep(1);

		ret = set_gpio_value(88, OFF);
		CHK_ERR (ret, stderr, "Error: gsm_modem_on() set CELLULAR MODULE PWRKEY LOW");
		sleep(1);

		while (1)
		{
			ret = check_gsm_modem_status();
			if (ret == OBD2_LIB_SUCCESS)
			{
				break;
			}
			else
			{
				sleep(1);
				printf("PORT NOT Connected\n");
				if (count > 30){
					break;
				}
				count++;
			}
		}

		sleep( 2 );

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
						sim_status = E_GSM_SIM_DETECTED;
					}
					if (strstr(val,"NOT"))
					{
						sim_status = E_GSM_SIM_NOT_DETECTED;
					}
					if (strstr(val,"SIM PIN"))
					{
						memset(val,0,sizeof(val));
						ret = asprintf(&cpin_cmd, "AT+CPIN=%s",cpin_user_input);
						if(cpin_cmd < 0)
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
								sim_status = E_GSM_SIM_DETECTED;
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
										sim_status = E_GSM_SIM_DETECTED;
									}
									if (strstr(val,"NOT"))
									{
										sim_status = E_GSM_SIM_NOT_DETECTED;
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
		ret = set_gsm_network_mode(1);
		ret = check_gsm_modem_status();

exit:
		if(cpin_cmd != NULL)
		{
			free(cpin_cmd);
			cpin_cmd = NULL;
		}
	}
	else
	{
		ret = E_GSM_NETWORK_MODE_ENABLED;
	}
	return ret;
}


/*GSM Module OFF*/
int gsm_modem_off()
{
	int ret = OBD2_LIB_SUCCESS;
	int error, count = OBD2_LIB_SUCCESS;

	if( network_enable_status == 0 )
	{
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
		CHK_ERR (ret, stderr, "Error: gsm_modem_off() set gpio-88 value OFF ");
		sleep(1);

		ret = set_gpio_value(88, ON);
		CHK_ERR (ret, stderr, "Error: gsm_modem_off() set gpio-88 value ON ");
		sleep(1);

		ret = set_gpio_value(88, OFF);
		CHK_ERR (ret, stderr, "Error: gsm_modem_off() set gpio-88 value OFF ");
		sleep (1);

		ret = set_gpio_value(78, OFF);
		CHK_ERR (ret, stderr, "Error: gsm_modem_off() set USB SWITCH FOR CELLULAR MODULE OFF ");
		sleep (1);

		ret = set_gpio_value(90, OFF);
		CHK_ERR (ret, stderr, "Error: gsm_modem_off() set CELLULAR MODULE SWITCH OFF ");
		sleep (1);

		while (1)
		{
			ret = check_gsm_modem_status();
			if (ret == OBD2_LIB_SUCCESS)
			{
				sleep(1);
				printf("EG95 PORT Connected\n");
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
	}
	else
	{
		ret = E_GSM_NETWORK_MODE_ENABLED;
	}

	return ret;
}

/*Get GSM SIM Status*/
/* BUG ID 5281 */
int get_gsm_sim_status(int *sim_status_val)
{
	int ret = OBD2_LIB_SUCCESS;
	int count = OBD2_LIB_SUCCESS;
	char val[ 256 ] = {0};
	char *cpin_cmd = NULL;

	memset (sim_status_val , 0, sizeof (sim_status_val));
	if (sim_status_val == NULL){
		ret = E_NULL_PARAMETER;
		return ret;
	}
	memset(val, 0x0, sizeof (val));

	ret = check_gsm_modem_status();
	if (ret != OBD2_LIB_SUCCESS){
		sim_status = E_GSM_SIM_STATUS_UNKNOWN;
		goto exit;
	}

	while (1)
	{
		memset( val, 0, sizeof( val ) );
		ret = gsm_at_cmd(READ_SIMSTATUS, val, sizeof( val ), 5000000);
		CHK_ERR (ret, stderr, "Error: Read GSM SIM Status");
		if (ret == OBD2_LIB_SUCCESS)
		{
			if (strstr(val,"READY"))
			{
				sim_status = E_GSM_SIM_DETECTED;
				break;
			}
			else if (strstr(val,"SIM PIN"))
			{
				ret = asprintf(&cpin_cmd, "AT+CPIN=%s",cpin_user_input);
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
						sim_status = E_GSM_SIM_DETECTED;
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
				sim_status = E_GSM_SIM_NOT_DETECTED;
				CHK_ERR (sim_status, stderr, "Error: get_gsm_sim_status() Sim Not Detectdd");
				break;
			}
			else
			{
				if (count > 5){
					sim_status = E_GSM_SIM_NOT_DETECTED;
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
	*sim_status_val = sim_status;
	if(cpin_cmd != NULL)
	{
		free(cpin_cmd);
		cpin_cmd = NULL;
	}
	return ret;
}

/*Read GSM SIM ICCID*/
int get_gsm_sim_iccid(char *iccid, int length)
{
	char *sim_id_ptr = NULL;
	char temp_str[64]={0};
	char val[100] = {0};
	int ret = OBD2_LIB_SUCCESS;
	int count = OBD2_LIB_SUCCESS;
	int sim_info = 0;

	if( network_enable_status == 1 )
	{
		if(sim_status_monitor != OBD2_LIB_SUCCESS)
			return sim_status_monitor;
	}
	else
	{
		ret = get_gsm_sim_status(&sim_info);
		CHK_ERR (ret, stderr, "Error: get_gsm_sim_iccid() Read SIM Status");

		printf(" the sim status in get_gsm_sim_iccid is %x\n", sim_status);
		if (sim_info != E_GSM_SIM_DETECTED)
			return sim_status;
	}

	while (1){
		memset( val, 0, sizeof( val ) );
		memset( temp_str, 0, sizeof( temp_str ) );
		memset(iccid, 0x0, length);

		ret = gsm_at_cmd(READ_ICCID, val, sizeof( val ), 300000);
		CHK_ERR (ret, stderr, "Error: get_gsm_sim_iccid() Read GSM SIM ICCID");

		printf("%s - gsm_at_cmd ret val is %s\n", __FUNCTION__, val );
		if ((ret == OBD2_LIB_SUCCESS)){
			sim_id_ptr = strtok(val, ":");
			if (sim_id_ptr != NULL)
			{
				sim_id_ptr = strtok(NULL, "\n");
				if( sim_id_ptr != NULL )
				{
					if( sizeof( temp_str ) < strlen( sim_id_ptr ) + 1 )
					{
						ret = E_BUFFER_READ_OVERFLOW;
						goto exit;
					}
					strcpy(temp_str, sim_id_ptr+1);
				}
				else
				{
					ret = E_GSM_SIM_ICCID_ERROR;
					goto exit;
				}
			}
			else
			{
				ret = E_GSM_SIM_ICCID_ERROR;
				goto exit;
			}

			if( length < strlen(temp_str) + 1 )
			{
				ret = E_BUFFER_READ_OVERFLOW;
				goto exit;
			}
			strncpy(iccid, temp_str, strlen(temp_str)-1);

			if (strlen (iccid) > 18){
				break;
			}else{
				if (count >= 5){
					memset(iccid, 0x0, length);
					strcpy(iccid, "");
					ret = E_GSM_SIM_NOT_VALID;
					CHK_ERR (ret, stderr, "Error: get_gsm_sim_iccid() Invalid ICCID");
					break;
				}

				count++;
			}
		}
		else
		{
			break;
		}
	}

exit:
	return ret;
}

/*Set GSM Network Mode*/
int set_gsm_network_mode (int type)
{
	int sim_info = 0;
	char val [64] = {0};
	int ret = OBD2_LIB_SUCCESS;

	if( network_enable_status == 0 )
	{
		ret = get_gsm_sim_status(&sim_info);
		CHK_ERR (ret, stderr, "Error: set_gsm_network_mode() Read SIM Status");
		if (sim_info != E_GSM_SIM_DETECTED)
			return sim_status;

		if (type == MODE_TYPE_2G){
			ret = gsm_at_cmd(SET_MODE_2G, val, sizeof( val ), 300000);
			CHK_ERR (ret, stderr, "Error: set_gsm_network_mode() Set GSM 2G Mode");
		}
		else if (type == MODE_TYPE_3G){
			ret = gsm_at_cmd(SET_MODE_3G, val, sizeof( val ), 300000);
			CHK_ERR (ret, stderr, "Error: set_gsm_network_mode() Set GSM 3G Mode");
		}
		else if (type == MODE_TYPE_4G){
			ret = gsm_at_cmd(SET_MODE_4G, val, sizeof( val ), 300000);
			CHK_ERR (ret, stderr, "Error: set_gsm_network_mode() Set GSM 3G Mode");
		}
		else if (type == MODE_TYPE_AUTO){
			ret = gsm_at_cmd(SET_AUTO_MODE, val, sizeof( val ), 300000);
			CHK_ERR (ret, stderr, "Error: set_gsm_network_mode Set GSM Auto Mode");
		}else
			ret = INVALID_VALUE;
	}
	else
	{
		ret = E_GSM_NETWORK_MODE_ENABLED;
	}

	return ret;
}

/* Read GSM Signal Strength */
/* BUG ID 5281 */
int get_gsm_signal_strength(char *signal_lvl, int length)
{
	int sim_info = 0;
	char val[100] = {0};
	int ret = OBD2_LIB_SUCCESS;
	char *signal_ptr = NULL;

	ret = get_gsm_sim_status(&sim_info);
	CHK_ERR (ret, stderr, "Error: get_gsm_signal_strength() Read SIM Status");
	if (sim_info != E_GSM_SIM_DETECTED)
		sim_status = sim_info;
	printf("sim_status in get_gsm_signal_strength is %x\n ",sim_status);
	ret = gsm_at_cmd(READ_GSM_SIGNAL_STRENGTH, val, sizeof( val ), 300000);
	CHK_ERR (ret, stderr, "Error: get_gsm_signal_strength() Get GSM Singal Strength");
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

/* Read GSM Network Registration */
/* BUG ID 5281 */
int get_gsm_nw_reg(char * cell_id, int cell_id_len, char *lac, int lac_len)
{
	int sim_info = 0;
	char val[100] = {0};
	int ret = OBD2_LIB_SUCCESS;
	char *lac_ptr = NULL;
	char *c_id_ptr = NULL;
	ret = get_gsm_sim_status(&sim_info);
	CHK_ERR (sim_status, stderr, "Error: get_gsm_nw_reg() Read SIM Status");
	if (sim_info != E_GSM_SIM_DETECTED)
		return sim_status;

	ret = set_gsm_nw_reg_loc_on();
	loc_status = 1;
	CHK_ERR (ret, stderr, "Error: get_gsm_nw_reg() Get GSM Network Registration");
	if(ret != OBD2_LIB_SUCCESS)
	{
		ret = E_GSM_SIM_REG_ERROR;
		goto exit;
	}

	ret = gsm_at_cmd(READ_GSM_SIM_REGISTRATION, val, sizeof( val ), 300000);
	CHK_ERR (ret, stderr, "Error: get_gsm_nw_reg() Get GSM Network Registration");
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
int set_gsm_nw_reg_loc_on()
{
	int rc =0;
	char resp[20] = {0};

	rc = gsm_at_cmd(SET_GSM_SIM_REG_WITH_LOC_ON, resp, sizeof( resp ), 300000);
	CHK_ERR (rc, stderr, "Error: set_gsm_nw_reg_loc_on() Set GSM Network Registration Location ON");

	return rc;
}

/*Enable GSM Network Registration With Location*/
int set_gsm_nw_reg_loc_off()
{
	int rc =0;
	char resp[20] = {0};

	rc = gsm_at_cmd(SET_GSM_SIM_REG_WITH_LOC_OFF, resp, sizeof( resp ), 300000);
	CHK_ERR (rc, stderr, "Error: set_gsm_nw_reg_loc_off() Set GSM Network Registration Location OFF");

	return rc;
}

int check_gsm_modem_status()
{
	int ret = 0;

	if( ( ( access(GSM_GPS_PORT, F_OK) == OBD2_LIB_SUCCESS ) && ( access(GSM_LOG_PORT, F_OK) == OBD2_LIB_SUCCESS ) &&\
				( access(GSM_AT_PORT, F_OK) == OBD2_LIB_SUCCESS ) && ( access(GSM_CONNECTION_PORT, F_OK) == OBD2_LIB_SUCCESS ) ) == 0 )
	{
		ret = E_GSM_USB_DISCONNECT;
		CHK_ERR (ret, stderr, "Error: check_gsm_modem_status() Read GSM Status ON/OFF");
	}
	else
	{
		ret = OBD2_LIB_SUCCESS;
	}

	return ret;
}

int gsm_apn_config_read(char* apn_val )
{
	FILE *fp;
	char apncontent[] = "OK AT+CGDCONT=1";
	char *content = NULL;

	char* token = NULL;
	int content_length = 0;

	if(access("/etc/ppp/chat/gprs", F_OK) == 0)
	{
		fp = fopen("/etc/ppp/chat/gprs", "r");
		fseek(fp, 0, SEEK_END);
		content_length = ftell(fp); /* Calcule the Size of the file for to allocate memory */
		fseek(fp, 0, SEEK_SET);

		content = ( char * )calloc( 1, content_length );
		if( content == NULL )
		{
			printf("alloc error\n");
			goto error;
		}
		while ( fgets(content, content_length ,fp))
		{
			if( strstr (content , apncontent)!= NULL)
			{
				token = strtok (content, ",");
				token = strtok (NULL, ",");
				token = strtok (NULL, ",");
				strcpy (apn_val , token);
				printf("APN_CONFIG is %s\n",apn_val);
			}else
			{
			}
		}
error:
		if( content != NULL )
		{
			free(content );
			content = NULL;
		}
		if( fp != NULL )
		{
			fclose( fp );
			fp = NULL;
		}
		return OBD2_LIB_SUCCESS;
	}else {
		return OBD2_LIB_FAILURE;
	}

}



void gsm_apn_configuration(char *apn_name, char *atd_num, char *username, char *password)
{
	FILE *fp;
	if(access("/etc/ppp/chat/gprs", F_OK) == 0)
		system ("rm /etc/ppp/chat/gprs");

	fp = fopen("/etc/ppp/chat/gprs", "a");
	fprintf(fp,"TIMEOUT 35\n");
	fprintf(fp,"ECHO ON\n");
	fprintf(fp,"ABORT \'\\nBUSY\\r\'\n");
	fprintf(fp,"ABORT \'\\nERROR\\r\'\n");
	fprintf(fp,"ABORT \'\\nNO ANSWER\\r\'\n");
	fprintf(fp,"ABORT \'\\nNO CARRIER\\r\'\n");
	fprintf(fp,"ABORT \'\\nNO DIALTONE\\r\'\n");
	fprintf(fp,"ABORT \'\\nRINGING\\r\\n\\r\\nRINGING\\r\'\n");
	fprintf(fp,"\'\' \\rAT\n");
	fprintf(fp,"OK \'ATQ0 V1 E1 S0=0 &C1 &D2\'\n");
	fprintf(fp,"OK AT+CGDCONT=1,\"IP\",\"%s\"\n", apn_name);
	fprintf(fp,"OK %s\n", atd_num);
	fprintf(fp,"CONNECT \\c\n");
	fclose(fp);

	if (username != NULL && password != NULL){

		if(access("/etc/ppp/chap-secrets", F_OK) == 0)
			system ("rm /etc/ppp/chap-secrets");

		fp = fopen("/etc/ppp/chap-secrets", "wr");
		fprintf(fp,"%s * %s\n", username, password);
		fclose(fp);

		if(access("/etc/ppp/pap-secrets", F_OK) == 0)
			system ("rm /etc/ppp/pap-secrets");

		fp = fopen("/etc/ppp/pap-secrets", "wr");
		fprintf(fp,"%s * %s\n", username, password);
		fclose(fp);
	}
}
/* BUG ID 5281 */

int check_network_connection()
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
				IOBD_DEBUG_LEVEL1("could not resolve hostname of DNS Server 8.8.8.8\n" );
				ret = -1;

			}
			else
			{
				IOBD_DEBUG_LEVEL3("LINK UP!!!!\n");
				ret = 0;
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
/* BUG ID 5288 */
/*This is an internal API, Used in the API establish_connection()*/
int establish_nw_connection()
{
	int sim_info = OBD2_LIB_FAILURE ;
	int ret = OBD2_LIB_FAILURE;

	if( network_enable_status == 0 )
	{
		ret = CheckLink("ppp0");
		if (ret == OBD2_LIB_SUCCESS)
		{
			printf("\nNetwork up\n");
		}
		else{
			ret = system ("pppd call gprs_4g");
			sleep (WAIT_PPP_UP);
			ret = CheckLink("ppp0");
			if (ret == OBD2_LIB_SUCCESS){
				printf("\nNetwork up\n");
			}
			else
			{
				get_gsm_sim_status(&sim_info);
				ret=sim_info;
			}
		}

	}
	else
	{
		ret = E_GSM_NETWORK_MODE_ENABLED;
	}

	return ret;
}

/*
 * Function used to establish the GSM network connection. \
 * It checks the GSM SIM status, Modem OFF/ON status. \
 * It will change the network from auto mode to 4G mode, 4G mode to 3G mode and 3G mode to 2G mode. \
 * if network connection is not establishing even if SIM present*/

/* BUG ID 5288 */
int establish_connection()
{
	int ret = OBD2_LIB_FAILURE;
	int time_diff = 0;
	struct timespec start, end;
	int network_mode = 4;
	int current_mode = 0;
	int status = 1;
	int sim_info = OBD2_LIB_FAILURE;
	if( network_enable_status == 0 )
	{
		clock_gettime(CLOCK_MONOTONIC, &start);
		ret = get_gsm_sim_status(&sim_info);
		printf (" SIM status is %x\n ", sim_info);
		if(ret == 0)
		{
			if (sim_info == E_GSM_SIM_DETECTED){
				while(1)
				{
					ret = establish_nw_connection();
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
						ret = set_gsm_network_mode(network_mode);	

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
				ret = sim_info;
			}
		}
	}
	else
	{
		ret = E_GSM_NETWORK_MODE_ENABLED;
	}

	return ret;
}

/*
 * function to read messages using AT commands \
 * AT+CMGF=1 => this command will set the GSM to text mode \
 * AT+CSMP=17,167,0,241 => To set the additional text mode parameters \
 * AT+CSCS = \"IRA\" => selects the TE character set "IRA" \
 * */
int GSM_set_to_message_init ( )
{
	int ret_value = 0;
	int count = 0;
	/*command to set GSM to text messages mode*/
	char *tmp_buf[] = {"ATE0\r","AT+CMGF=1\r", "AT+CSMP=17,167,0,241\r", "AT+CSCS=\"IRA\"\r"};
	char resp_buffer[1024] = {0};

	/*calculating the number of commands executed*/
	int max_count = sizeof(tmp_buf)/sizeof(tmp_buf[0]);

	for (count = 0; count < max_count; count++)
	{
		memset( resp_buffer, 0, sizeof( resp_buffer ) );
		ret_value = gsm_at_cmd(tmp_buf[count], resp_buffer, sizeof( resp_buffer ), 800000 );
		if (ret_value < 0)
		{
			perror ("GSM_set_to_message_init");
			return ret_value;
		}
		else
		{
		}
		sleep( 1 );
	}

	return OBD2_LIB_SUCCESS;
}

int unread_message (char *msg_buf, int length, int max_resp_time )
{
	int ret = GSM_set_to_message_init ( );
	if( ret < 0 )
	{
		ret = E_GSM_SIM_SMS_INIT;
		return ret;
	}

	int ret_value;
	char tmp_buf[] ={"AT+CMGL=\"REC UNREAD\"\r"};
	char *res_ptr = NULL;

	ret_value = gsm_at_cmd(tmp_buf, msg_buf, length, max_resp_time );
	if (ret_value < 0)
	{
		perror ("unread_message");
		return ret_value;
	}else{
		if ((res_ptr = strstr (msg_buf, "OK")) == NULL)
			return E_GSM_SIM_SMS_UNREAD;
	}
	return OBD2_LIB_SUCCESS;
}

int read_message (char *msg_buf, int length, int max_resp_time )
{
	int ret = GSM_set_to_message_init ( );
	if( ret < 0 )
	{
		ret = E_GSM_SIM_SMS_INIT;
		return ret;
	}

	int ret_value;
	char tmp_buf[] ={"AT+CMGL=\"REC READ\"\r"};
	char *res_ptr = NULL;

	ret_value = gsm_at_cmd(tmp_buf, msg_buf, length, max_resp_time );
	printf(" read_message resp_buffer %s\n", msg_buf );
	if (ret_value < 0)
	{
		perror ("read_message");
		return ret_value;
	}else{
		if ((res_ptr = strstr (msg_buf, "OK")) == NULL)
			return E_GSM_SIM_SMS_READ;
	}
	return OBD2_LIB_SUCCESS;
}

int delete_message( int index, int max_resp_time )
{
	char *tmp_buf = NULL;
	char resp_buffer[ 1024 ] = {0};
	int ret = OBD2_LIB_SUCCESS;
	ret = GSM_set_to_message_init ( );
	if( ret < 0 )
	{
		ret = E_GSM_SIM_SMS_INIT;
		return ret;
	}

	ret = asprintf(&tmp_buf, "AT+CMGD=%d\r", index );
	if(ret < 0)
	{
		ret = -ENOMEM;
		goto end;
	}
	memset( resp_buffer, 0, sizeof( resp_buffer ) );
	ret = gsm_at_cmd( tmp_buf, resp_buffer, sizeof( resp_buffer ), max_resp_time );
	if (ret < 0)
	{
		perror ("delete_message");
		goto end;
	}
	else{
		printf("delete_message resp_buffer %s\n", resp_buffer );
		if (strstr (resp_buffer, "OK") == NULL)
		{
			ret = E_GSM_SIM_SMS_DELETE;
			goto end;
		}
		else
		{
		}
	}


end:	
	if(tmp_buf != NULL)
	{
		free(tmp_buf);
		tmp_buf = NULL;
	}
	return ret;	
}

int delete_all_messages( int max_resp_time )
{
	int ret = GSM_set_to_message_init ( );
	if( ret < 0 )
	{
		ret = E_GSM_SIM_SMS_INIT;
		return ret;
	}

	int ret_value;
	/* Command to set GSM to text messages mode */
	char tmp_buf[ 1024 ] = { "AT+CMGD=1,1\r" };
	char resp_buffer[ 1024 ] = {0};

	memset( resp_buffer, 0, sizeof( resp_buffer ) );
	ret_value = gsm_at_cmd( tmp_buf, resp_buffer, sizeof( resp_buffer ), max_resp_time );
	if (ret_value < 0)
	{
		perror ("delete_message");
		return OBD2_LIB_FAILURE;
	}else{
		printf("delete_all_messages resp_buffer %s\n", resp_buffer );
		if (strstr (resp_buffer, "OK") == NULL)
			return E_GSM_SIM_SMS_DELETE_ALL;
	}
	return OBD2_LIB_SUCCESS;
}
