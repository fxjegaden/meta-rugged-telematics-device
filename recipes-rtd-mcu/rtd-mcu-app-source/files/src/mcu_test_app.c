#include "mcu_i2c_common.h"
#include "accelerometer.h"
#include "battery.h"
#include "common.h"
#include "error_nos.h"
#include "can.h"
#include "gps.h"
#include "gsm.h"

#define I2C_TEST			0
#define RTC_NODE_PATH			"/dev/rtc1"

/* Debug Prints Enable */
#define COMMON_DEBUG_PRINTS		1
#define DEBUG_EN			0
#if DEBUG_EN
#define DEBUG_PRINTS			1
#define DEC_TO_HEX_DEBUG_EN		1
#define MCU_I2C_BUF_FRAME_DEBUG_EN	1
#define MCU_I2C_READ_DEBUG_EN		1
#define MCU_I2C_WRITE_DEBUG_EN	1
#endif

int external_rtc_flag;

/*
 * API		: prepare_sleep_mode()
 * Description	: API to disable all the interfaces before entering the Sleep Mode to \
 * 		: reduce the current consumption.
 * Arguments	: None
 * Return Value	: 0 for success and error code for failure.
 * */
int prepare_sleep_mode()
{
	int ret = OBD2_LIB_FAILURE;

	ret = check_gsm_modem_status();
	if(ret == OBD2_LIB_SUCCESS)
	{
		/* Turning off teh GPS */
		ret = gps_deinit();

		/* Turning Off the GSM modem */
		ret = gsm_modem_off();
	}

	/* Deinitialize the WiFi */
	ret = wifi_deinit();

	/* Deinitialize the Bluetooth */
	ret = ble_deinit();

	/* Deinitialize the interfaces */
	ret = deinit();

	return ret;
}

/*
 * API		: enable_requested_wakeup_sources( uint8_t wakeup_source, uint32_t *timer )
 * Description	: API to enable the requested wakeup sources for MCU sleep wakeup. Other \
 * 		: wakeup sources will be disabled.
 * Arguments	: uint8_t *wake_source - Variable to store the Wakeup source
 * Return Value	: 0 for success and error code for failure.
 * */
int enable_requested_wakeup_sources( uint8_t wakeup_source, uint32_t *timer )
{
	int ret = OBD2_LIB_SUCCESS;

	if( access( RTC_NODE_PATH, F_OK ) )
	{
		external_rtc_flag = 0;
	}
	else
	{
		external_rtc_flag = 1;
	}

	/* For safer side, we are disabling the CAN0 and CAN1 wakeup */
	ret = config_can_wakeup( CAN0, DISABLE );
	if( ret == OBD2_LIB_SUCCESS )
	{
		ret = config_can_wakeup( CAN1, DISABLE );
		if( ret == OBD2_LIB_SUCCESS )
		{
			// Do Nothing
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

	if( ret == OBD2_LIB_SUCCESS )
	{
		/* Check for the Ignition Interrupt */
		if( wakeup_source & CPU_MCU_IGN_WAKEUP_REQUEST_BIT_MASK )
		{
			ret = config_ignition_wakeup( ENABLE );
#if DEBUG_PRINTS
			printf("%s : config_ignition_wakeup() enable returned |%d|\n", __func__, ret);
#endif
		}
		else
		{
			ret = config_ignition_wakeup( DISABLE );
#if DEBUG_PRINTS
			printf("%s : config_ignition_wakeup() disable returned |%d|\n", __func__, ret);
#endif
		}

		if( ret == OBD2_LIB_SUCCESS )
		{
			/* Check for the Accelerometer Interrupt */
			if( wakeup_source & CPU_MCU_ACC_WAKEUP_REQUEST_BIT_MASK )
			{
				ret = config_acc_wakeup( ENABLE );
#if DEBUG_PRINTS
				printf("%s : config_acc_wakeup() enable returned |%d|\n", __func__, ret);
#endif
			}
			else
			{
				ret = config_acc_wakeup( DISABLE );
				if( ret == OBD2_LIB_SUCCESS )
				{
					ret = acc_deinit();
				}
#if DEBUG_PRINTS
				printf("%s : config_acc_wakeup() disable returned |%d|\n", __func__, ret);
#endif
			}

			if( ret == OBD2_LIB_SUCCESS )
			{
				/* Check for the CAN FD Interrupt */
				if( wakeup_source & CPU_MCU_CAN_WAKEUP_REQUEST_BIT_MASK )
				{
					ret = config_can_wakeup( CAN2, ENABLE );
#if DEBUG_PRINTS
					printf("%s : config_can_wakeup() enable returned |%d|\n", __func__, ret);
#endif
				}
				else
				{
					ret = config_can_wakeup( CAN2, DISABLE );
#if DEBUG_PRINTS
					printf("%s : config_can_wakeup() disable returned |%d|\n", __func__, ret);
#endif
				}

				if( ret == OBD2_LIB_SUCCESS )
				{
					/* Check for the Timer Interrupt */
					if( wakeup_source & CPU_MCU_TIMER_WAKEUP_REQUEST_BIT_MASK )
					{
						ret = config_timer_wakeup( ENABLE, *timer );
#if DEBUG_PRINTS
						printf("%s : config_timer_wakeup() disable returned |%d|\n", __func__, ret);
#endif
					}
					/* Support for the RTC Interrupt */
					else if( wakeup_source & CPU_MCU_RTC_WAKEUP_REQUEST_BIT_MASK )
					{
						if( external_rtc_flag )
						{
#if DEBUG_PRINTS
							printf("External RTC detected. Enabling the RTC wakeup\n");
#endif
							ret = config_rtc_wakeup( ENABLE, *timer );
#if DEBUG_PRINTS
							printf("%s : config_rtc_wakeup() enable returned |%d|\n", __func__, ret);
#endif
						}
						else
						{
							ret = E_IF_INVALID;
							printf("%s : RTC Not found. Error Code is |%x|\n", __func__, ret);
						}
					}
					else
					{
						if( external_rtc_flag )
						{
#if DEBUG_PRINTS
							printf("External RTC detected. Disabling the RTC wakeup\n");
#endif
							ret = config_rtc_wakeup( DISABLE, DISABLE );
#if DEBUG_PRINTS
							printf("%s : config_rtc_wakeup() disable returned |%d|\n", __func__, ret);
#endif
						}
						else
						{
							// Do Nothing
						}

						ret = config_timer_wakeup( DISABLE, DISABLE );
#if DEBUG_PRINTS
						printf("%s : config_timer_wakeup() disable returned |%d|\n", __func__, ret);
#endif
					}

					if( ret == OBD2_LIB_SUCCESS )
					{
						/* Enable the MCU GPIO interrupt for wakeup */
						ret = config_mcu_wakeup( ENABLE );
#if DEBUG_PRINTS
						printf("%s : config_mcu_wakeup() enable returned |%d|\n", __func__, ret);
#endif
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

int main()
{
	int ret = 0, i = 0;
	int choice = 0;
	int adc = 0;
	float voltage = 0;
	uint8_t sleep_mode = 0;
	uint8_t wakeup_source = 0;
	uint8_t wakeup_choice = 0;
	char mcu_fw_version[35] = {0};
	int timer = 0;
	int bitrate = 0;
	int dbitrate = 0;
	int txqueuelen = 250000;
	uint32_t bat_monitor_interval = 0;
	float_t temperature;

	i2c_cmd_frame i2c_frame_buf;
	i2c_cmd_decode i2c_decode_buf;

	uint32_t bus_num = 0;
	uint8_t slave_addr = 0x76;
	uint8_t data_addr = 0x02;
	uint8_t i2c_frame[256] = {0};

	i = 1;
	printf("\n\t%d. MCU Sleep Request\n\t%d. MCU Sleep Source Request\n\t%d. MCU Firmware Version\n\t%d. MCU ADC Read\n\n", \
			i, (i+1), (i+2), (i+3));
	printf("Enter the choice : ");
	ret = scanf("%d", &choice);
	switch( choice )
	{
		case 1:
			/* Sleep Request */
			printf("\t\t1. Normal Sleep\n\t\t2. Deep Power Down\n\n");
			fflush(stdin);
			printf("Enter the sleep_mode: ");
			ret = scanf(" %hhx", &sleep_mode );

			switch(sleep_mode)
			{
				case 1:
					sleep_mode = 1;
					break;
				case 2:
					sleep_mode = 4;
					break;
				default:
					sleep_mode = 0;
					printf("Enter valid Sleep mode\n\n");
					break;
			}

			if( sleep_mode > OBD2_LIB_SUCCESS )
			{
				i = 1;
				fflush(stdin);
				printf("\t\t%d. Timer\n\t\t%d. RTC Wakeup\n\t\t%d. Ignition\n\t\t%d. Accelerometer\n\t\t%d. CAN FD\n\t\t%d. Custom Wakeup sources\n", \
						i, (i+1), (i+2), (i+3), (i+4), (i+5));
				printf("Enter the wakeup_source: ");
				ret = scanf(" %hhx", &wakeup_choice );
				switch(wakeup_choice)
				{
					case 1:
						wakeup_source = CPU_MCU_TIMER_WAKEUP_REQUEST_BIT_MASK;
						break;
					case 2:
						wakeup_source = CPU_MCU_RTC_WAKEUP_REQUEST_BIT_MASK;
						break;
					case 3:
						wakeup_source = CPU_MCU_IGN_WAKEUP_REQUEST_BIT_MASK;
						break;
					case 4:
						wakeup_source = CPU_MCU_ACC_WAKEUP_REQUEST_BIT_MASK;
						break;
					case 5:
						wakeup_source = CPU_MCU_CAN_WAKEUP_REQUEST_BIT_MASK;
						break;
					case 6:
						fflush(stdin);
						printf("Enter the combination of wakeup sources: ");
						ret = scanf(" %hhx", &wakeup_source );
						break;
					default:
						wakeup_source = 0x0;
						printf("Enter valid Wakeup Source\n\n");
						break;
				}

				if( (wakeup_source & CPU_MCU_TIMER_WAKEUP_REQUEST_BIT_MASK) || (wakeup_source & CPU_MCU_RTC_WAKEUP_REQUEST_BIT_MASK) )
				{
					fflush(stdin);
					printf("Enter the timer for wakeup: ");
					ret = scanf(" %d", &timer );
				}
				else
				{
					//Do Nothing
				}

				if(wakeup_source & CPU_MCU_CAN_WAKEUP_REQUEST_BIT_MASK)
				{
					fflush(stdin);
					printf("Enter the Speed for CAN FD interface: ");
					ret = scanf(" %d", &bitrate );
					printf("Enter the DBitrate for CAN FD interface: ");
					ret = scanf(" %d", &dbitrate );

					ret = can_fd_init( bitrate, dbitrate, txqueuelen);
				}
				else
				{
					//Do Nothing
				}

				if( (wakeup_source > 0x0) && !((wakeup_source & CPU_MCU_TIMER_WAKEUP_REQUEST_BIT_MASK) && (wakeup_source & CPU_MCU_RTC_WAKEUP_REQUEST_BIT_MASK)) )
				{
					/*
					 * API to disable all the interfaces before entering the \
					 * Sleep Mode to reduce the current consumption.
					 * */
					ret = prepare_sleep_mode();

					/*
					 * Enable the requested Wakeup Sources only. The wakeup \
					 * sources that are not requested will be disabled.
					 * */
					ret = enable_requested_wakeup_sources( wakeup_source, &timer );
					if( ret == OBD2_LIB_SUCCESS )
					{
						ret = MCU_sleep_mode(sleep_mode, wakeup_source);
						if( (ret == OBD2_LIB_SUCCESS) && (wakeup_source & CPU_MCU_CAN_WAKEUP_REQUEST_BIT_MASK) )
						{
							ret = can_deinit( CAN2 );
						}
						else
						{
							//Do Nothing
						}
						ret = eth_init();
					}
					else
					{
						//Do Nothing
					}
				}
				else
				{
					ret = INVALID_ARGS;
				}
			}
			else
			{
				//Do Nothing
			}
			break;
		case 2:
			/* Wakeup Source Request */
			wakeup_source = 0;
			ret = MCU_Wakeup_Source_Request(&wakeup_source);
			printf("The MCU wakeup source is |%x|\n\n", wakeup_source);
			break;
		case 3:
			/* MCU Firmware Version Read */
			ret = MCU_FW_Version_Read_Request( mcu_fw_version );
			if( ret == OBD2_LIB_SUCCESS )
				printf("The MCU Firmware Version is |%s|\n\n", mcu_fw_version);
			break;
		case 4:
			printf("\n\t1. 12V ADC\n\t2. Battery ADC\n\t3. ADC1\n\t4. ADC2\nEnter the ADC to be read: ");
			ret = scanf(" %d", &adc);
			ret = MCU_ADC_Read_Request( adc, &voltage );
			if( ret == OBD2_LIB_SUCCESS )
				printf("The MCU ADC Voltage is |%f|\n\n", voltage);
			break;
		default:
			ret = OBD2_LIB_FAILURE;
			printf("Not implemented\n\n");
			break;
	}

	if(ret == OBD2_LIB_SUCCESS)
	{
		printf("mcu_test_app : SUCCESS with return |%x|\n", ret);
	}
	else
	{
		printf("mcu_test_app : FAILURE with return |%x|\n", ret);
	}

	return ret;
}
