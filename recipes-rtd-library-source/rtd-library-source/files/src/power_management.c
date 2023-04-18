#define _GNU_SOURCE
#include "thread.h"
#include <stdio.h>
#include "init.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "lib_common.h"
#include "obd2lib.h"
#include "lib_accelerometer.h"
#include "pm.h"

/*
 * API		: disable_all_wakeup_sources()
 * Description	: API to disable all the wakeup interrupts.
 * Arguments	: None
 * Return Value	: 0 for success and error code for failure.
 * */
int disable_all_wakeup_sources()
{
	int ret = OBD2_LIB_SUCCESS;

	ret = config_mcu_wakeup( DISABLE );
	if( ret == OBD2_LIB_SUCCESS )
	{
		if(access(RTC_WAKEUPALARM_FILE_NAME, F_OK ) == 0)
		{
			ret = config_rtc_wakeup( DISABLE, DISABLE );
		}
		else
		{
			// Do Nothing
		}

		if( ret == OBD2_LIB_SUCCESS )
		{
			ret = config_timer_wakeup( DISABLE, 0 );
			if( ret == OBD2_LIB_SUCCESS )
			{
				ret = config_ignition_wakeup( DISABLE );
				if( ret == OBD2_LIB_SUCCESS )
				{
					ret = config_acc_wakeup( DISABLE );
					if( ret == OBD2_LIB_SUCCESS )
					{
						ret = config_can_wakeup( CAN0, DISABLE );
						if( ret == OBD2_LIB_SUCCESS )
						{
							ret = config_can_wakeup( CAN1, DISABLE );
							if( ret == OBD2_LIB_SUCCESS )
							{
								ret = config_can_wakeup( CAN2, DISABLE );
								if( ret == OBD2_LIB_SUCCESS )
								{
									// Do Nothing
								}
								else
								{
									// Do Nothing
								}							}
							else
							{
								// Do Nothing
							}
						}
						else
						{
							// Do Nothing
						}
					}
					else
					{
						// Do Nothing
					}
				}
				else
				{
					// Do Nothing
				}
			}
			else
			{
				// Do Nothing
			}
		}
		else
		{
			// Do Nothing
		}
	}
	else
	{
		// Do Nothing
	}

	return ret;
}

int config_timer_wakeup( int option, int timer )
{
	int ret = OBD2_LIB_SUCCESS;
	FILE *fp_timer = NULL;
	char *timer_buf = NULL;

	if( option == TIMER_WAKEUP_ENABLE )
	{
		ret = asprintf( &timer_buf, "+%d", timer );
		if(ret < 0)
		{
			ret = -ENOMEM;
			goto end;
		}
	}
	else
	{
		ret = asprintf( &timer_buf, "+%d", DISABLE );
		if(ret < 0)
		{
			ret = -ENOMEM;
			goto end;
		}
	}

	fp_timer = fopen( TIMER_WAKEUPALARM_FILE_NAME, "w" );
	if( fp_timer == NULL )
	{
		printf("%s - failed to open %s with errno %d\n", __FUNCTION__, TIMER_WAKEUPALARM_FILE_NAME, errno );
		ret = OBD2_LIB_FAILURE;
		goto end;
	}

	ret = fwrite( timer_buf, 1, strlen( timer_buf ), fp_timer );
	if( ret < 0 )
	{
		printf("%s - fwrite failed %d\n", __FUNCTION__, errno );
		ret = OBD2_LIB_FAILURE;
	}
	else
	{
		ret = OBD2_LIB_SUCCESS;
	}

	if( fp_timer != NULL )
	{
		fclose( fp_timer );
		fp_timer = NULL;
	}

end:
	if(timer_buf != NULL){
		free( timer_buf );
		timer_buf = NULL;
	}

	return ret;
}

uint8_t bin2bcd(const uint8_t bin) {
	return ((bin / 10) << 4) + (bin % 10);
}

/*
 * API		: config_rtc_wakeup( int option, int timer )
 * Description	: API to enble ot disable the RTC interrupt.
 * Arguments	: timer = Timer in seconds to be passed to this function. \
 * 			Pass 0 to disable the wakeup.
 * Return Value	: 0 for success and error code for failure.
 * */
int config_rtc_wakeup( int option, int timer )
{
	int ret = OBD2_LIB_FAILURE;
	uint8_t i2c_command[10] = {0};
	time_t tmp_timer;
	struct tm *rt;
	struct tm *wt;

	if(access(RTC_WAKEUPALARM_FILE_NAME, F_OK ) == 0)
	{
		if(timer >= 0)
		{
			ret = system("hwclock -w");
			if(ret == OBD2_LIB_SUCCESS)
			{
				ret = system("hwclock -s");
				if(ret == OBD2_LIB_SUCCESS)
				{
					time(&tmp_timer);
					rt = localtime(&tmp_timer);
#if DEBUG_PRINTS
					printf("\tCurr time_from_epoch = %ld\n\trt->tm_sec = %d\n\trt->tm_min = %d\n\trt->tm_hour = %d\n\trt->tm_mday = %d\n\trt->tm_mon = %d\n\n", \
							tmp_timer, rt->tm_sec, rt->tm_min, rt->tm_hour, rt->tm_mday, rt->tm_mon);
#endif
					if(tmp_timer >= 0)
					{
						timer += tmp_timer;
						wt = localtime(&timer);
#if DEBUG_PRINTS
						printf("\tAlarm time_from_epoch = %ld\n\twt->tm_sec = %d\n\twt->tm_min = %d\n\twt->tm_hour = %d\n\twt->tm_mday = %d\n\twt->tm_mon = %d\n\n", \
								timer, wt->tm_sec, wt->tm_min, wt->tm_hour, wt->tm_mday, wt->tm_mon);
#endif

						/* Set Alarm */
						i2c_command[0] = REG_RTC_ALARM1_SECOND;
						if((wt->tm_sec >= 0) && (wt->tm_sec < 60))
						{
							i2c_command[1] = bin2bcd(wt->tm_sec);
							i2c_command[9] |= REG_RTC_ALARM_ENABLE_A1E_SECOND;
						}

						if((wt->tm_min >= 0) && (wt->tm_min < 60))
						{
							i2c_command[2] = bin2bcd(wt->tm_min);
							i2c_command[9] |= REG_RTC_ALARM_ENABLE_A1E_MINUTE;
						}

						if((wt->tm_hour >= 0) && (wt->tm_hour < 24))
						{
							i2c_command[3] = bin2bcd(wt->tm_hour);
							i2c_command[9] |= REG_RTC_ALARM_ENABLE_A1E_HOUR;
						}

						if((wt->tm_mday >= 1) && (wt->tm_mday <= 31))
						{
							i2c_command[4] = bin2bcd(wt->tm_mday);
							i2c_command[9] |= REG_RTC_ALARM_ENABLE_A1E_DAY;
						}

						if((wt->tm_mon >= 1) && (wt->tm_mon <= 12))
						{
							/* Months are counted from 1 to 12 in PCF85263A */
							i2c_command[5] = bin2bcd((wt->tm_mon)+1);
							i2c_command[9] |= REG_RTC_ALARM_ENABLE_A1E_MONTH;
						}

						/* Set Interrupt mode output */
						ret = i2c_write(RTC_BUS_NO, RTC_SLAVE_ADDR, REG_PIN_IO, 0x02);
						if( ret == OBD2_LIB_SUCCESS)
						{
							/* Enable the Alarm interrupt pulse */
							ret = i2c_write(RTC_BUS_NO, RTC_SLAVE_ADDR, REG_INTA_ENABLE, 0x18);
							if( ret == OBD2_LIB_SUCCESS)
							{
								/* Clear alarm enable bits */
								ret = i2c_write(RTC_BUS_NO, RTC_SLAVE_ADDR, REG_RTC_ALARM_ENABLE, DISABLE);
								if( ret == OBD2_LIB_SUCCESS)
								{
									/* Set alarm and enable bits */
									ret = i2c_write(RTC_BUS_NO, RTC_SLAVE_ADDR, REG_FLAGS, DISABLE);
									if( ret == OBD2_LIB_SUCCESS)
									{
										/* Write the Alarm to the registers of RTC chip */
										ret = mcu_i2c_write( RTC_BUS_NO, RTC_SLAVE_ADDR, i2c_command );
									} /* i2c_write : REG_FLAGS */
								} /* i2c_write : REG_RTC_ALARM_ENABLE */
							} /* i2c_write : REG_INTA_ENABLE */
						} /* i2c_write : REG_PIN_IO */
					}
					else /* Alarm time is already expired */
					{
						ret = E_OBD2_LIB_SLP_RTC_ERR;
					}
				} /* system("hwclock -s") */
			} /* system("hwclock -w") */
		}
		else
		{
			ret = E_OBD2_LIB_SLP_RTC_INV_ERR;
		} /* end of else - Timer is not valid */
	}
	else
	{
		printf("%s : Failed to open \"%s\" with errno %d\n", __FUNCTION__, RTC_WAKEUPALARM_FILE_NAME, errno );
		ret = E_OBD2_LIB_SLP_RTC_ERR;
	}

	return ret;
}

int config_acc_wakeup( int option )
{
	int ret = OBD2_LIB_SUCCESS;
	FILE *fp_acc = NULL;
	char *acc_buf = NULL;
	int sensor_int1_value = 0;
	int sensor_tap_value = 0;

	if( !acc_init_success )
	{
		/*
		 * Initializing the Accelerometer before disabling the interrupt \
		 * will avoid the Kernel Panic after sleep wakeup \
		 * */
		ret = acc_init();
		if(ret == OBD2_LIB_SUCCESS)
		{
			acc_init_success = 1;
		}
		else
		{
			acc_init_success = 0;
		}
	}
	else
	{
		ret = OBD2_LIB_SUCCESS;
		acc_init_success = 1;
	}

	if( ret == OBD2_LIB_SUCCESS )
	{
		if( option == ACC_WAKEUP_ENABLE )
		{
			ret = asprintf( &acc_buf, "%s", "enabled" );
			if(ret < 0)
			{
				ret = -ENOMEM;
				goto exit;
			}
			sensor_int1_value = 0x00;
			sensor_tap_value = 0x8e;
		}
		else
		{
			ret = asprintf( &acc_buf, "%s", "disabled" );
			if(ret < 0)
			{
				ret = -ENOMEM;
				goto exit;
			}
			sensor_int1_value = 0x01;
			sensor_tap_value = 0x0e;
		}

		ret = i2c_write( ACC_I2C_NO, ACC_SLAVE_ADDR, SENSOR_INT1_CTRL, sensor_int1_value );
		if( ret < OBD2_LIB_SUCCESS )
		{
			ret = E_ACCELEROMETER_I2C_REG_CONFIG;
			CHK_ERR( ret, stderr, "Error: acc_init() Set SENSOR_INT1_CTRL" );
			goto exit;
		}

		ret = i2c_write( ACC_I2C_NO, ACC_SLAVE_ADDR, SENSOR_TAP_CFG, sensor_tap_value );
		if( ret < OBD2_LIB_SUCCESS )
		{
			ret = E_ACCELEROMETER_I2C_REG_CONFIG;
			CHK_ERR( ret, stderr, "Error: acc_init() Set SENSOR_TAP_CFG" );
			goto exit;
		}

		fp_acc = fopen( ACC_WAKEUP_FILE_NAME, "w" );
		if( fp_acc == NULL )
		{
			printf("%s - failed to open %s with errno %d\n", __FUNCTION__, ACC_WAKEUP_FILE_NAME, errno );
			ret = OBD2_LIB_FAILURE;
			goto exit;
		}

		ret = fwrite( acc_buf, 1, strlen( acc_buf ), fp_acc );
		if( ret < 0 )
		{
			printf("%s - fwrite failed %d\n", __FUNCTION__, errno );
			ret = OBD2_LIB_FAILURE;
		}
		else
		{
			ret = OBD2_LIB_SUCCESS;
		}

		if( fp_acc != NULL )
		{
			fclose( fp_acc );
			fp_acc = NULL;
		}
	}

exit:
	if(acc_buf != NULL){
		free( acc_buf );
		acc_buf = NULL;
	}

	return ret;
}

int config_can_wakeup( char *can_name, int option )
{
	int ret = OBD2_LIB_FAILURE;
	FILE *fp_can = NULL;
	char *can_buf = NULL;

	if( option == CAN_WAKEUP_ENABLE )
	{
		ret = asprintf( &can_buf, "%s", "enabled" );
		if(ret < 0)
		{
			ret = -ENOMEM;
			goto end;
		}
	}
	else
	{
		ret = asprintf( &can_buf, "%s", "disabled" );
		if(ret < 0)
		{
			ret = -ENOMEM;
			goto end;
		}
	}

	if( can_name != NULL )
	{
		if(strcmp( can_name, CAN0))
		{
			fp_can = fopen( CAN0_WAKEUP_FILE_NAME, "w" );
			if( fp_can == NULL )
			{
				printf("%s - failed to open %s with errno %d\n", __FUNCTION__, CAN0_WAKEUP_FILE_NAME, errno );
				ret = OBD2_LIB_FAILURE;
				goto end;
			}
			else
			{
				ret = fwrite( can_buf, 1, strlen( can_buf ), fp_can );
				if( ret < 0 )
				{
					printf("%s - fwrite failed %d\n", __FUNCTION__, errno );
					ret = OBD2_LIB_FAILURE;
				}
				else
				{
					ret = OBD2_LIB_SUCCESS;
				}
			}

			ret = fclose( fp_can );
			if( ret == -1 )
			{
				ret = E_CAN0_WAKEUP_FILE_CLOSE_ERROR;
			}
			else
			{
				fp_can = 0;
				ret = OBD2_LIB_SUCCESS;
			}
		}
		else if( strcmp(can_name, CAN1) )
		{
			fp_can = fopen( CAN1_WAKEUP_FILE_NAME, "w" );
			if( fp_can == NULL )
			{
				printf("%s - failed to open %s with errno %d\n", __FUNCTION__, CAN1_WAKEUP_FILE_NAME, errno );
				ret = OBD2_LIB_FAILURE;
				goto end;
			}
			else
			{
				ret = fwrite( can_buf, 1, strlen( can_buf ), fp_can );
				if( ret < 0 )
				{
					printf("%s - fwrite failed %d\n", __FUNCTION__, errno );
					ret = OBD2_LIB_FAILURE;
				}
				else
				{
					ret = OBD2_LIB_SUCCESS;
				}
			}

			ret = fclose( fp_can );
			if( ret == -1 )
			{
				ret = E_CAN1_WAKEUP_FILE_CLOSE_ERROR;
			}
			else
			{
				fp_can = 0;
				ret = OBD2_LIB_SUCCESS;
			}
		}
		else if(strcmp( can_name, CAN2))
		{
			fp_can = fopen( CAN2_WAKEUP_FILE_NAME, "w" );
			if( fp_can == NULL )
			{
				printf("%s - failed to open %s with errno %d\n", __FUNCTION__, CAN2_WAKEUP_FILE_NAME, errno );
				ret = OBD2_LIB_FAILURE;
				goto end;
			}
			else
			{
				ret = fwrite( can_buf, 1, strlen( can_buf ), fp_can );
				if( ret < 0 )
				{
					printf("%s - fwrite failed %d\n", __FUNCTION__, errno );
					ret = OBD2_LIB_FAILURE;
				}
				else
				{
					ret = OBD2_LIB_SUCCESS;
				}
			}

			ret = fclose( fp_can );
			if( ret == -1 )
			{
				ret = E_CAN2_WAKEUP_FILE_CLOSE_ERROR;
			}
			else
			{
				fp_can = 0;
				ret = OBD2_LIB_SUCCESS;
			}
		}
		else
		{
			ret = E_CAN_INTERFACE_NOT_FOUND;
		}
	}
	else
	{
		ret = E_OBD2_LIB_INVALID_ARG;
	}

end:
	if(can_buf != NULL){
		free( can_buf );
		can_buf = NULL;
	}

	return ret;
}

int config_ignition_wakeup( int option )
{
	int ret = OBD2_LIB_SUCCESS;
	FILE *fp_ign = NULL;
	char *ign_buf = NULL;

	if( option == IGN_WAKEUP_ENABLE )
	{
		ret = asprintf( &ign_buf, "%s", "enabled" );
		if(ret < 0)
		{
			ret = -ENOMEM;
			goto end;
		}
	}
	else
	{
		ret = asprintf( &ign_buf, "%s", "disabled" );
		if(ret < 0)
		{
			ret = -ENOMEM;
			goto end;
		}
	}

	fp_ign = fopen( IGN_WAKEUP_FILE_NAME, "w" );
	if( fp_ign == NULL )
	{
		printf("%s - failed to open %s with errno %d\n", __FUNCTION__, IGN_WAKEUP_FILE_NAME, errno );
		ret = OBD2_LIB_FAILURE;
		goto end;
	}

	ret = fwrite( ign_buf, 1, strlen( ign_buf ), fp_ign );
	if( ret < 0 )
	{
		printf("%s - fwrite failed %d\n", __FUNCTION__, errno );
		ret = OBD2_LIB_FAILURE;
	}
	else
	{
		ret = OBD2_LIB_SUCCESS;
	}

	if( fp_ign != NULL )
	{
		fclose( fp_ign );
		fp_ign = NULL;
	}
end:
	if(ign_buf != NULL){
		free( ign_buf );
		ign_buf = NULL;
	}
	return ret;
}

/*
 * API		: config_mcu_wakeup( int option )
 * Description	: API for enabling or disabling the MCU wakeup interrupt
 * Arguments	: int option - 1 to enable and 0 to disable the interrupt
 * Return Value	: 0 for success and error code for failure.
 * */
int config_mcu_wakeup( int option )
{
	int ret = OBD2_LIB_SUCCESS;
	FILE *fp_mcu = NULL;
	char *mcu_buf = NULL;

	if( option == MCU_WAKEUP_ENABLE )
	{
		ret = asprintf( &mcu_buf, "%s", "enabled" );
		if(ret < 0)
		{
			ret = -ENOMEM;
			goto end;
		}
	}
	else
	{
		ret = asprintf( &mcu_buf, "%s", "disabled" );
		if(ret < 0)
		{
			ret = -ENOMEM;
			goto end;
		}
	}

	fp_mcu = fopen( MCU_WAKEUP_FILE_NAME, "w" );
	if( fp_mcu == NULL )
	{
		printf("%s - failed to open %s with errno %d\n", __FUNCTION__, MCU_WAKEUP_FILE_NAME, errno );
		ret = OBD2_LIB_FAILURE;
		goto end;
	}

	ret = fwrite( mcu_buf, 1, strlen( mcu_buf ), fp_mcu );
	if( ret < 0 )
	{
		printf("%s - fwrite failed %d\n", __FUNCTION__, errno );
		ret = OBD2_LIB_FAILURE;
	}
	else
	{
		ret = OBD2_LIB_SUCCESS;
	}

	if( fp_mcu != NULL )
	{
		fclose( fp_mcu );
		fp_mcu = NULL;
	}

end:
	if(mcu_buf != NULL){
		free( mcu_buf );
		mcu_buf = NULL;
	}

	return ret;
}

/*
 * API		: config_sms_wakeup( int option )
 * Description	: API for enabling or disabling the SMS wakeup
 * Arguments	: int option - 1 to enable and 0 to disable the wakeup
 * Return Value	: 0 for success and error code for failure.
 * */
int config_sms_wakeup( int option )
{
	int ret = OBD2_LIB_SUCCESS;
	int dev_num = 0;
	int sim_info = 0;
	char t;
	char buffer[100] = {0};
	char resp_buffer[1024] = {0};
	char *sms_buf = NULL;
	char *token = NULL;
	FILE *fp_sms = NULL;

	if( option == SMS_WAKEUP_ENABLE )
	{
		ret = asprintf( &sms_buf, "%s", "enabled" );
		if(ret < 0)
		{
			ret = -ENOMEM;
			goto end;
		}
		else
		{
			ret = gsm_at_cmd( AT_GSM_SLP_ENABLE, resp_buffer, sizeof(resp_buffer), 300000);
			if(ret == OBD2_LIB_SUCCESS)
			{
				ret = gsm_at_cmd( AT_GSM_MSG_ENABLE, resp_buffer, sizeof(resp_buffer), 300000);
				if(ret == OBD2_LIB_SUCCESS)
				{
					ret = get_gsm_sim_status(&sim_info);
					if(sim_info != E_GSM_SIM_DETECTED)
					{
						ret = sim_status;
					}
					else
					{
						// Do Nothing
					}
				}
				else
				{
					// Do Nothing
				}
			}
			else
			{
				// Do Nothing
			}
		}
	}
	else
	{
		ret = asprintf( &sms_buf, "%s", "disabled" );
		if(ret < 0)
		{
			ret = -ENOMEM;
			goto end;
		}
		else
		{
			ret = gsm_at_cmd( AT_GSM_SLP_DISABLE, resp_buffer, sizeof(resp_buffer), 300000);
			if(ret == OBD2_LIB_SUCCESS)
			{
				ret = gsm_at_cmd( AT_GSM_MSG_DISABLE, resp_buffer, sizeof(resp_buffer), 300000);
				if(ret == OBD2_LIB_SUCCESS)
				{
					ret = get_gsm_sim_status(&sim_info);
					if(sim_info != E_GSM_SIM_DETECTED)
					{
						ret = sim_status;
					}
					else
					{
						// Do Nothing
					}
				}
				else
				{
					// Do Nothing
				}
			}
			else
			{
				// Do Nothing
			}
		}
	}

	if( ret == OBD2_LIB_SUCCESS )
	{
		fp_sms = popen(LSUSB_CMD, "r");
		if( fp_sms != NULL )
		{
			while(fgets(buffer, 100, fp_sms) != NULL)
			{
				if( token = strstr( buffer, QUECTEL_ID ) )
				{
                                        token = strtok( buffer, "Device");
                                        t = token[strlen(token) - 2];
					sscanf(&t, "%d", &dev_num);
					ret = 1;
					break;
				}
				memset(buffer, 0, strlen(buffer));
			}
			pclose(fp_sms);
		}
		else
		{
			printf("Failed opening file\n");
			ret = E_LIB_FILE_OPEN_ERROR;
		}

		if( ret > OBD2_LIB_SUCCESS )
		{
			memset(buffer, 0, strlen(buffer));
			ret = sprintf(buffer, "%s/ci_hdrc.1/usb%d/%d-1/%s", USB_WAKEUP_FILE_PATH, dev_num, dev_num, USB_WAKEUP_FILE_NAME);
			if(((access(buffer, F_OK) == 0) && ret >= OBD2_LIB_SUCCESS))
			{
				fp_sms = fopen( buffer, "w" );
				if( fp_sms == NULL )
				{
					printf("%s - failed to open %s with errno %d\n", __FUNCTION__, buffer, errno );
					ret = E_LIB_FILE_OPEN_ERROR;
					goto end;
				}
				else
				{
					ret = fwrite( sms_buf, 1, strlen( sms_buf ), fp_sms );
					if( ret < 0 )
					{
						printf("%s - fwrite failed %d\n", __FUNCTION__, errno );
						ret = OBD2_LIB_FAILURE;
					}
					else
					{
						ret = OBD2_LIB_SUCCESS;
					}

					if( fp_sms != NULL )
					{
						fclose( fp_sms );
						fp_sms = NULL;
					}
				}

				memset(buffer, 0, strlen(buffer));
				ret = sprintf(buffer, "%s/ci_hdrc.1/%s", USB_WAKEUP_FILE_PATH, USB_WAKEUP_FILE_NAME);
				if(((access(buffer, F_OK) == 0) && ret >= OBD2_LIB_SUCCESS))
				{
					fp_sms = fopen( buffer, "w" );
					if( fp_sms == NULL )
					{
						printf("%s - failed to open %s with errno %d\n", __FUNCTION__, buffer, errno );
						ret = E_LIB_FILE_OPEN_ERROR;
						goto end;
					}
					else
					{
						ret = fwrite( sms_buf, 1, strlen( sms_buf ), fp_sms );
						if( ret < 0 )
						{
							printf("%s - fwrite failed %d\n", __FUNCTION__, errno );
							ret = OBD2_LIB_FAILURE;
						}
						else
						{
							ret = OBD2_LIB_SUCCESS;
						}

						if( fp_sms != NULL )
						{
							fclose( fp_sms );
							fp_sms = NULL;
						}
					}

					memset(buffer, 0, strlen(buffer));
					ret = sprintf(buffer, "%s/%s", USB_WAKEUP_FILE_PATH, USB_WAKEUP_FILE_NAME);
					if(((access(buffer, F_OK) == 0) && ret >= OBD2_LIB_SUCCESS))
					{
						fp_sms = fopen( buffer, "w" );
						if( fp_sms == NULL )
						{
							printf("%s - failed to open %s with errno %d\n", __FUNCTION__, buffer, errno );
							ret = E_LIB_FILE_OPEN_ERROR;
							goto end;
						}
						else
						{
							ret = fwrite( sms_buf, 1, strlen( sms_buf ), fp_sms );
							if( ret < 0 )
							{
								printf("%s - fwrite failed %d\n", __FUNCTION__, errno );
								ret = OBD2_LIB_FAILURE;
							}
							else
							{
								ret = OBD2_LIB_SUCCESS;
							}

							if( fp_sms != NULL )
							{
								fclose( fp_sms );
								fp_sms = NULL;
							}
						}
						memset(buffer, 0, strlen(buffer));
					}
					else
					{
						ret = E_LIB_FILE_EXIST_ERROR;
					}
				}
				else
				{
					ret = E_LIB_FILE_EXIST_ERROR;
				}
			}
			else
			{
				ret = E_LIB_FILE_EXIST_ERROR;
			}
		}
		else
		{
			// Do Nothing
		}
	}
	else
	{
		// Do Nothing
	}

end:
	if(sms_buf != NULL)
	{
		free( sms_buf );
		sms_buf = NULL;
	}

	return ret;
}
