#define _GNU_SOURCE
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include "serial.h"
#include "4g.h"

sem_t* gps_lock;
static int gps_fd = -1;

void gps_sem_init (void) __attribute__ ((constructor));
void gps_sem_deinit (void) __attribute__ ((destructor));

void gps_sem_init (void)
{
	int ret;
	/*!< Init Semaphore for GPS*/
	gps_lock = sem_open("/gps_lock", O_CREAT | EEXIST | O_EXCL, 0777, 1);
	if (gps_lock == SEM_FAILED){
		ret = E_GPS_SEM_INIT;
		CHK_ERR (ret, stderr, "Error: gps_sem_init sem_init");
	}else
		sem_unlink("/gps_lock");
}

void gps_sem_deinit (void)
{
	if (gps_lock != SEM_FAILED)
		sem_destroy(gps_lock);
}

/* BUG ID 5310 */
int gps_init()
{
	char val[300] = {0};	
	int rc = 0;
	char dummy_buf[500] = {0};

	rc = gsm_at_cmd("AT+QGPS?", val, sizeof(val), 500000);
	if(strstr(val,"1") != NULL){
		rc = gps_deinit();
	}
	if (gps_lock != SEM_FAILED)
	{
		sem_wait(gps_lock);
	}

	if( gps_fd >= 0 )
	{
		rc = E_GPS_PORT_EXIST;
		goto end;
	}

	rc = gsm_at_cmd(GPS_INIT, val, sizeof(val), 300000);
	if (rc < 0)
	{
		gps_fd = rc;
		goto end;
	}

	if(strstr(val,"OK") != NULL)
	{
		gps_fd = iW_Serial_Init(SERIAL_PORT_ACM0, 115200, GPS_NODE);
		if(gps_fd < 0)	
		{
			rc = E_GPS_USB_INIT;
			CHK_ERR (rc, stderr, "Error: GPS Port Init");
		}
		else
		{
			read(gps_fd, &dummy_buf, 500);
			rc = OBD2_LIB_SUCCESS;
		}
	}
	else
	{
		CHK_ERR (rc, stderr, "Error: GPS Enable");
	}

end:
	if (gps_lock != SEM_FAILED)
	{
		sem_post(gps_lock);
	}

	return rc;
}

int gps_deinit( )
{
	int ret = 0;
	char val[300];

	if (gps_lock != SEM_FAILED)
	{
		sem_wait(gps_lock);
	}


	if(gps_fd < 0)
	{
		ret = E_GPS_USB_DISCONNECT;
		CHK_ERR (ret, stderr, "Error: GPS Port DeInit");
	}
	else
	{
		if( gps_fd >= 0 )
		{
			ret = tcflush(gps_fd,TCIOFLUSH);
			ret = close(gps_fd);
			if (ret == 0)
				ret = OBD2_LIB_SUCCESS;
			else{
				ret = E_GPS_USB_DEINIT;
			}
			gps_fd = -1;
		}
		else
		{
			ret = E_GPS_USB_DEINIT;
		}
	}

	ret = gsm_at_cmd("AT+QGPS?", val, sizeof(val), 500000);
	if(strstr(val,"1") != NULL)
	{

		ret = gsm_at_cmd("AT+QGPSEND", val, sizeof(val), 500000);
	}


	if (gps_lock != SEM_FAILED)
	{
		sem_post(gps_lock);
	}

	return ret;
}

int agps_xtra_data_download( )
{
	// Declare the file pointer
	FILE *filePointer;

	char *dataToBeRead = NULL;
	char *dataToBeFread = NULL;
	int log_files_read = 0;
	int data_length = 0;
	int i = 0;
	int serial_fd = 0;
	int max_resp_time = 300000;
	char buff[1024] = {0};
	char *sub_buff = NULL;
	char *file_pointer = NULL;
	int numberofbytesent, numberofbytesrcvd;
	int ret = 0 ;

	char AT_test[ 10 ] = { 0 };
	char file_delete[ 500 ] = { 0 };
	char file_open[ 500 ] = { 0 };
	char file_write[ 50 ] = { 0 };
	char file_close[ 50 ] = { 0 };
	char file_upload[ 500 ] = { 0 };
	sleep( 10 );

	filePointer = fopen("xtra2.bin", "rb");

	// Check if this filePointer is null which maybe if the file does not exist
	if ( filePointer == NULL )
	{
		printf( "xtra2.bin file failed to open." );
	}
	else
	{
		fseek(filePointer, 0, SEEK_END);
		data_length = ftell(filePointer); /* Calcule the Size of the file for to allocate memory */
		fseek(filePointer, 0, SEEK_SET);

		dataToBeRead = ( char * )calloc( 1, data_length );
		dataToBeFread = ( char * )( dataToBeRead );

		if( dataToBeRead == NULL )
		{
			printf("alloc error\n");
			goto error;
		}

		// Read the dataToBeRead from the file using fgets() method
		while( 1 )
		{
			log_files_read = fread( dataToBeRead, 1, data_length, filePointer );
			i += log_files_read;
			if( i == data_length )
			{
				i = 0;
				break;
			}
		}

		// Closing the file using fclose()
		fclose(filePointer);
	}

	serial_fd = iW_Serial_Init(SERIAL_PORT_ACM0,115200, USB_2);
	if(serial_fd < 0)
	{
		printf("Error: GSM AT Port Init\n");
		return -1;
	}
	tcflush(serial_fd,TCIOFLUSH);
	sleep( 1 );

	{
		numberofbytesrcvd = read( serial_fd, buff, sizeof( buff ) );
		if(numberofbytesrcvd <= 0)
		{
			printf("Error: GSM AT Port Read\n");
		}
		else
		{
			// Do Nothing
		}
	}
	tcflush(serial_fd,TCIOFLUSH);
	sleep( 1 );

	strcpy( AT_test, "ATE0\r" );
	strcpy( file_delete, "AT+QFDEL=\"xtra2.bin\"\r" );
	strcpy( file_open, "AT+QFOPEN=\"xtra2.bin\",0\r" );


	numberofbytesent = write (serial_fd ,AT_test, strlen( AT_test ) );
	if(numberofbytesent <= 0)
	{
		printf("Error: GSM AT Port Write\n" );
		goto error;
	}
	usleep( max_resp_time );

	memset( buff, 0, sizeof( buff ) );
	numberofbytesrcvd = read( serial_fd, buff, sizeof( buff ) );
	if(numberofbytesrcvd <= 0)
	{
		printf("Error: GSM AT Port Read\n");
		goto error;
	}
	else
	{
		// Do Nothing
	}

	tcflush(serial_fd,TCIOFLUSH);
	sleep( 1 );

	numberofbytesent = write (serial_fd , file_delete, strlen(file_delete));
	if(numberofbytesent <= 0)
	{
		printf("Error: GSM AT Port Write\n" );
		goto error;
	}
	usleep( max_resp_time );

	memset( buff, 0, sizeof( buff ) );
	numberofbytesrcvd = read( serial_fd, buff, sizeof( buff ) );
	if(numberofbytesrcvd <= 0)
	{
		printf("Error: GSM AT Port Read\n");
		goto error;
	}
	else
	{
		// Do Nothing
	}

	tcflush(serial_fd,TCIOFLUSH);
	sleep( 1 );

	numberofbytesent = write (serial_fd , file_open, strlen(file_open));
	if(numberofbytesent <= 0)
	{
		printf("Error: GSM AT Port Write\n" );
		goto error;
	}
	usleep( max_resp_time );

	numberofbytesrcvd = read( serial_fd, buff, sizeof( buff ) );

	if(numberofbytesrcvd <= 0)
	{
		printf("Error: GSM AT Port Read\n");
		goto error;
	}
	else
	{
		// Do Nothing
	}

	sub_buff = strtok ( buff,": ");
	while ( sub_buff != NULL )
	{
		printf("sub_buff[ 0 ] = %c\n", sub_buff[ 0 ] );
		if( sub_buff[ 0 ] >= 48 )
		{
			file_pointer = strdup( sub_buff );
			sprintf( file_write, "%s=%c,%d\r", "AT+QFWRITE", file_pointer[ 0 ], data_length );
		}
		sub_buff = strtok (NULL, " ");
	}


	numberofbytesent = write( serial_fd , file_write, strlen( file_write ) );
	if(numberofbytesent <= 0)
	{
		printf("Error: GSM AT Port Write\n" );
		goto error;
	}
	usleep(max_resp_time);

	memset( buff, 0, sizeof( buff ) );
	numberofbytesrcvd = read(serial_fd, buff, sizeof(buff));
	if(numberofbytesrcvd <= 0)
	{
		printf("Error: GSM AT Port Read\n");
		goto error;
	}
	else
	{
		tcflush(serial_fd,TCIOFLUSH);
		sleep( 1 );

		if( strstr( buff, "CONNECT" ) != NULL )
		{
			while( data_length > 0 )
			{
				numberofbytesent = write( serial_fd, dataToBeRead, data_length );
				if(numberofbytesent <= 0)
				{
					printf("Error: GSM AT Port Write\n" );
					goto error;
				}
				else
				{
					dataToBeRead += numberofbytesent;
					data_length -= numberofbytesent;
					usleep( 6000 );
				}
			}

			usleep(max_resp_time);

			memset( buff, 0, sizeof( buff ) );
			numberofbytesrcvd = read(serial_fd, buff, sizeof(buff));
			if(numberofbytesrcvd <= 0)
			{
				printf("Error: GSM AT Port Read\n");
				goto error;
			}
		}
		else
		{
			printf("Error on file_write!!!\n");
			goto error;
		}
	}

	usleep(max_resp_time);

	sprintf(file_close, "%s=%c\r", "AT+QFCLOSE", file_pointer[ 0 ] );

	numberofbytesent = write (serial_fd , file_close, strlen(file_close));
	if(numberofbytesent <= 0)
	{
		printf("Error: GSM AT Port Write\n" );
		goto error;
	}
	usleep( max_resp_time );

	numberofbytesrcvd = read( serial_fd, buff, sizeof( buff ) );

	if(numberofbytesrcvd <= 0)
	{
		printf("Error: GSM AT Port Read\n");
		goto error;
	}
	else
	{
		// Do Nothing
	}

	if( dataToBeFread != NULL )
	{
		free( dataToBeFread );
		dataToBeFread = NULL;
	}
	if( file_pointer != NULL )
	{
		free( file_pointer );
		file_pointer = NULL;
	}
	ret = tcflush(serial_fd, TCIOFLUSH);
	close(serial_fd);

	return 0;

error:
	if( dataToBeFread != NULL )
	{
		free( dataToBeFread );
		dataToBeFread = NULL;
	}
	if( file_pointer != NULL )
	{
		free( file_pointer );
		file_pointer = NULL;
	}
	ret = tcflush(serial_fd, TCIOFLUSH);
	close(serial_fd);

	return -1;
}

/* BUG ID 5310 */
int agps_init()
{
	char val[300];
	char at_cmd[1024];
	struct timeval tv;
	static int r_count=0;
	char *date_str;
	char *check;
	int xtra_gps_valid = 0;
	char time_string[40];
	char valid_buf[100];
	char *buf = NULL;
	int count = 0;
	int rc = 0;
	int i = 0;
	char apn_name[50];
	char* qntp_content = NULL;
	char apn_command[50];
	char AGPS_CONTEXT[] = "AT+QICSGP=1,1,";
	char xtra_path[50] = {0};
	char *ret;
	char ch = '-';
	system ("rm -r xtra2.bin");
	while(1)
	{

		rc = gsm_at_cmd("AT+QGPS?", val, sizeof(val), 500000);
		if(strstr(val,"1") != NULL)
		{
			rc = gps_deinit ();
		}
		rc = CheckLink("ppp0");
		if (rc == OBD2_LIB_SUCCESS){
			system ("wget -O xtra2.bin http://xtrapath4.izatcloud.net/xtra2.bin?raw=true");
			break;
		}else if (i < 10){
			i ++;
		}
		else
		{
			rc = OBD2_LIB_FAILURE;
			break;
		}
	}
	if (i == 10)
	{
		if (gps_lock != SEM_FAILED)
		{
			sem_post(gps_lock);
		}

		return rc;
	}
	system("chmod +x xtra2.bin");
	if (gps_lock != SEM_FAILED)
	{
		sem_wait(gps_lock);
	}
	sprintf(xtra_path,"/home/root/xtra2.bin");
	if (access(xtra_path, F_OK ) == 0){

		gsm_apn_config_read(apn_name);
		strcat (AGPS_CONTEXT,apn_name);

		memset(valid_buf, 0, sizeof(valid_buf));
		for (i = 0 ; i <3 ; i++){
			rc = gsm_at_cmd(AGPS_CONTEXT, val, sizeof(val), 500000);

			CHK_ERR (rc, stderr, "Error: agps_init() QICSGP for AGPS");
			if(strstr(val,"OK") == NULL)
			{
				printf("%s - E_AGPS_QICSGP\n",val);
				rc = E_AGPS_QICSGP;
			}
		}
	}
	memset(val, 0x0, 200);
	rc = gsm_at_cmd(AGPS_QIACT_ENABLE, val, sizeof(val), 500000);

	CHK_ERR (rc, stderr, "Error: agps_init() Enable QIACT for AGPS");
	if(strstr(val,"OK") == NULL)
	{
		printf("%s - E_AGPS_QIACT_ENABLE\n",val);
		rc = E_AGPS_QIACT_ENABLE;
	}

	rc = gsm_at_cmd(AGPS_ENABLE, val, sizeof(val), 500000);

	CHK_ERR (rc, stderr, "Error: agps_init() Enable AGPS");

	if(strstr(val,"OK") == NULL)
	{
		printf("%s - E_AGPS_ENABLE\n", __func__);
		rc = E_AGPS_ENABLE;

		CHK_ERR (rc, stderr, "Error: agps_init() enable agps");
	}

	rc = gsm_at_cmd(AGPS_STATUS, val, sizeof(val), 500000);

	CHK_ERR (rc, stderr, "Error: agps_init() check agps_status");

	check = strstr(val,"+QGPSXTRADATA:");
	if(check) {
		date_str = strstr(check,",");

		if(date_str) {
			strncpy(valid_buf,&check[15],(date_str - (check + 15)));
		}
		xtra_gps_valid = atof(valid_buf);
	}else
	{
		printf("%s - AGPS_STATUS_INVALID\n",val);
	}

	/*QFLST*/
	memset(val, 0x0, 200);
	rc = gsm_at_cmd(AGPS_QFLST, val, sizeof(val), 500000);

	CHK_ERR (rc, stderr, "Error: agps_init() List AGPS Exist Data");
	if(strstr(val,"xtra2.bin") == NULL)
	{
		printf("%s - xtra2.bin does not exist\n",val);
		rc = E_AGPS_LIST_EXST_DATA;


	}else{
		/*QFDEL*/
		memset(val, 0x0, 200);
		rc = gsm_at_cmd(AGPS_DEL_EXIST_DATA, val, sizeof(val), 500000);

		CHK_ERR (rc, stderr, "Error: agps_init() Delete AGPS Exist Data");
		if(strstr(val,"OK") == NULL)
		{
			printf("%s - E_AGPS_DEL_EXST_DATA\n",val);
			rc = E_AGPS_DEL_EXST_DATA;

		}
	}

	rc = agps_xtra_data_download();
	/*QFLST*/
	memset(val, 0x0, 200);
	rc = gsm_at_cmd(AGPS_QFLST, val, sizeof(val), 500000);

	CHK_ERR (rc, stderr, "Error: agps_init() List AGPS Exist Data");
	if(strstr(val,"xtra2.bin") == NULL)
	{
		printf("%s - xtra2.bin does not exist after transfer\n", __func__ );
		rc = E_AGPS_LIST_EXST_DATA;
	}

QNTP:
	/*QNTP*/
	memset(val, 0x0, 200);
	rc = gsm_at_cmd(AGPS_QNTP, val, sizeof(val), 500000);

	CHK_ERR (rc, stderr, "Error: agps_init() AGPS QNTP");
	sleep (2);
	if(strstr(val,"+QNTP") == NULL)
	{
		printf("%s - AGPS_QNTP\n",val);
		goto QNTP;
	}
	else
	{
		/*+QNTP: 0,"2022/02/08,02:34:41+28*/
		ret = strchr(val, ch);
		if ( ret != NULL){
			qntp_content= strtok(val, ",");
			qntp_content = strtok (NULL,"-");
			printf ("negative qntp content is %s\n",qntp_content);

		}else{
			char ch = '+';
			/*+QNTP: 0,"2022/02/08,02:34:41-28*/
			ret = strchr(val, ch);
			if ( ret != NULL){

				qntp_content= strtok(val,",");
				qntp_content = strtok (NULL,"+");
				printf ("positive qntp content is %s\n",qntp_content);
			}
		}
	}
	rc = asprintf(&buf, "AT+QGPSXTRATIME=0,%s\",1,1,5\r", qntp_content);
	if (rc < 0)
	{
		rc = -ENOMEM;

		goto exit;
	}
	memset(val, 0x0, 200);
	memset(at_cmd, 0x0, 200);
	strcpy(at_cmd,buf);
	sleep(5);
	rc = gsm_at_cmd(at_cmd, val, sizeof(val), 5000000);
	CHK_ERR (rc, stderr, "Error: agps_init() set local time to get Xtradata");
	if (buf != NULL)
	{
		free(buf);
		buf = NULL;
	}
	if((strstr(val,"OK") == NULL))
	{
		printf("%s - QGPSXTRATIME\n",val);
		rc = E_AGPS_SET_TIME;

		goto exit;
	}
	else
	{
		// Do Nothing
	}

	/*QGPSXTRADATA*/
	memset(val, 0x0, 200);
	rc = gsm_at_cmd(AGPS_LOAD_DOWNLOAD_DATA, val, sizeof(val), 1000000);
	CHK_ERR (rc, stderr, "Error: agps_init() Load the Downloaded AGPS Data");

	if(strstr(val,"OK") == NULL)
	{
		printf("%s - E_AGPS_LOAD_DATA_INVALID\n " , val);
		rc = E_AGPS_LOAD_DATA_INVALID;

	}
	else
	{
		// Do Nothing
	}

exit:
	if (buf != NULL)
	{
		free(buf);
		buf = NULL;
	}
	if (gps_lock != SEM_FAILED)
	{
		sem_post(gps_lock);
	}

	return rc;
}

int get_gps_data(char *nmea, size_t * g_nbytes, char *recv_data, int length)
{
	char *char_read = NULL;
	char gps_buf[300] = {0};
	char buf[300] = {0};
	int ret = 0, i = 0, j = 0;
	int gpgsv = 0;
	int start_read = 0;
	struct timeval timeout;
	fd_set set;

	memset(buf, '\0', sizeof(buf));
	memset(gps_buf, '\0', sizeof(gps_buf));
	ret = check_gsm_modem_status();
	if (ret == OBD2_LIB_SUCCESS)
	{
		if (gps_lock != SEM_FAILED)
		{
			sem_wait(gps_lock);
		}
		else
		{
			// Do Nothing
		}

		if(gps_fd < 0)
		{
			printf("gps_fd %d\n", gps_fd);
			ret = E_GPS_USB_INIT;
		}
		else
		{
			if(strncmp(nmea, "GPGSV", 5) == 0)
			{
				gpgsv = 1;
			}
			else
			{
				gpgsv = 0;
			}

			ret = asprintf(&char_read, "%s", nmea);
			if(ret > 0)
			{
				i = 0;
				j = 0;

				while(1)
				{
					/* clear the set */
					FD_ZERO(&set);
					/* add file descriptor to the set */
					FD_SET(gps_fd, &set);

					timeout.tv_sec = 1;
					timeout.tv_usec = 100;

					ret = select(gps_fd + 1, &set, NULL, NULL, &timeout);
					if(ret > OBD2_LIB_SUCCESS)
					{
						if(read( gps_fd, &buf[j], sizeof(char)) > 0)
						{
							if(start_read)
							{
								/* In ASCII, 0x0d indicates the Carriage Return */
								if((buf[j] != '\n') && (buf[j] != '\0') && (buf[j] != '$'))
								{
									if(buf[j] == 0x0d)
									{
										// Ignore the Carriage Return Byte
										// Do Nothing
									}
									else
									{
										j ++;
									}
								}
								else
								{
									start_read = 0;
									ret = strncmp(char_read, buf, strlen(char_read));
									if(ret == 0)
									{
										if(gpgsv)
										{
											if( i == 0 )
											{
												memset(gps_buf, '\0', sizeof(gps_buf));
											}
											else
											{
												// Do Nothing
											}

											if(i < 3)
											{
												strcat(gps_buf, buf);
												ret = OBD2_LIB_SUCCESS;
												i ++;
											}
											else
											{
												i = 0;
												ret = OBD2_LIB_FAILURE;
											}
										}
										else
										{
											buf[j] = '\0';
											memset(gps_buf, '\0', sizeof(gps_buf));
											ret = sprintf(gps_buf, "%s", buf);
											if(ret > OBD2_LIB_SUCCESS)
											{
												ret = OBD2_LIB_SUCCESS;
											}
											else
											{
												ret = OBD2_LIB_FAILURE;
											}
										}
									}
									else
									{
										i = 0;
									}

									memset(buf, '\0', sizeof(buf));
									j = 0;
								}
							}
							else
							{
								if(buf[j] == '$')
								{
									start_read = 1;
									j = 0;
								}
								else
								{
									j = 0;
								}
							}
							errno = 0;
						}
					}
					else
					{
						ret = E_GPS_NMEA_TIMEOUT;
						break;
					}
				}

				/* In ASCII, 0x0d indicates the Carriage Return */
				if( (gps_buf[0] != '\0') && (gps_buf[0] != '\n') && (gps_buf[0] != 0x0d))
				{
					if((gps_buf[strlen(gps_buf)-1] == '\n') || (gps_buf[strlen(gps_buf)-1] == 0x0d))
					{
						gps_buf[strlen(gps_buf)-1] = '\0';
					}
					else
					{
						// Do Nothing
					}
					ret = OBD2_LIB_SUCCESS;
				}
				else if( strlen(gps_buf) <= 0)
				{
					ret = E_GPS_NMEA_TIMEOUT;
				}
				else
				{
					ret = E_GPS_NO_REQ_NMEA;
				}

#if DEBUG_EN
				printf("get_gps_data nbytes value is %d and buf is\n|%s|\n\n\n", strlen(gps_buf), gps_buf );
#endif
				*g_nbytes = strlen(gps_buf);
				if( ( strlen( gps_buf ) ) < length )
				{
					strcpy(recv_data, gps_buf);
				}
				else
				{
					ret = E_BUFFER_READ_OVERFLOW;
				}
			}
			else
			{
				ret = E_LIB_MEM_ALLOC_ERROR;
			}
		}

		memset(buf, '\0', sizeof(buf));
		memset(gps_buf, '\0', sizeof(gps_buf));

		if (gps_lock != SEM_FAILED)
		{
			sem_post(gps_lock);
		}
	}
	else
	{
		// Do Nothing
	}

	return ret;
}
