#define _GNU_SOURCE
#include "mcu_i2c_framing.h"

/*
 * API		: void print_frame_details( i2c_cmd_frame i2c_frame_buf )
 * Description	: API to print the i2c command frame. Set the DEBUG_EN macro to 1 for enabling the prints.
 * Arguments	: i2c_cmd_frame i2c_frame_buf - The typedef structure with i2c command frame.
 * Return Value : None
 * */
void print_frame_details( i2c_cmd_frame i2c_frame_buf )
{
#if DEBUG_EN
	int i = 0;
	printf("************ i2c Frame Details *************\n");
	printf("\ti2c_frame_buf.i2c_addr = |%x|\n\ti2c_frame_buf.reg = |%x|\n\ti2c_frame_buf.command_id = |%x|\n\ti2c_frame_buf.length = |%x|\n", \
			i2c_frame_buf.i2c_addr, i2c_frame_buf.reg, i2c_frame_buf.command_id, i2c_frame_buf.length);
	printf("\ti2c_frame_buf.data :\t|");
	while( ( i < i2c_frame_buf.length ))
	{
		printf("%x|", i2c_frame_buf.data[i]);
		i ++;
	}
	printf("\n\n");
#endif
}

/*
 * API		: void print_decoded_frame_details( i2c_cmd_decode i2c_decode_buf )
 * Description	: API to print the i2c command frame. Set the DEBUG_EN macro to 1 for enabling the prints.
 * Arguments	: i2c_cmd_decode i2c_decode_buf - The typedef structure with i2c command frame.
 * Return Value	: None
 * */
void print_decoded_frame_details( i2c_cmd_decode i2c_decode_buf )
{
#if DEBUG_EN
	int i = 0;
	printf("************ Decoded i2c Frame Details *************\n");
	printf("\ti2c_decode_buf.response_id : |%x|\n\ti2c_decode_buf.length : |%x|\n", \
			i2c_decode_buf.response_id, i2c_decode_buf.length);

	i = 0;
	printf("\ti2c_decode_buf.err_code : |");
	while( i < (sizeof( i2c_decode_buf.err_code )))
	{
		printf("%x|", i2c_decode_buf.err_code[i]);
		i ++;
	}
	printf("\n");

	i = 0;
	printf("\ti2c_decode_buf.data : |");
	while( ( i < i2c_decode_buf.length ))
	{
		printf("%x|", i2c_decode_buf.data[i]);
		i ++;
	}
	printf("\n\n");
#endif
}

/*
 * API		: int decToHex(int n, uint8_t *hexNum)
 * Description	: API to convert the decimal values to hexadecimal. \
 * 			This API can be used to find the size of the input integers.
 * Arguments	: uint32_t dec - The decimal value to be converted to HexaDecimal.
 * 		: uint8_t *hexNum - An empty pointer to store the HexaDecimal value.
 * Return Value	: Size of the Hexadecimal Number
 * */
int decToHex(uint32_t dec, uint8_t *hex)
{
	uint32_t temp = 0;
	int i = 0;
	int j = 0;
	uint8_t temp_hex[100] = {0};

	while (dec != 0)
	{
		temp = (dec % 16);
		if(temp < 10)
		{
			temp_hex[i] = (temp + 48);
			i++;
		}
		else
		{
			temp_hex[i] = (temp + 55);
			i++;
		}
		dec = (dec / 16);
	}

	i = 0;
	j = ( strlen(temp_hex) - 1);
	while( (i < strlen(temp_hex)) && (j >= 0) )
	{
		hex[i] = temp_hex[j];
		i ++;
		j --;
	}

	temp = (strlen(hex) - 1)/2;
	if(temp < 1)
		temp = 1;
	else if(temp % 2)
		temp += 1;

#if DEC_TO_HEX_DEBUG_EN
	printf("\t%s : After decToHex exec\n\tret = |%d|\n\tdec : |%d|\n\thex : |%x|%d|\n\n\n", __func__, temp, dec, *hex, *hex);
#endif

	return temp;
}

/*
 * API		: int i2c_buf_frame( i2c_cmd_frame i2c_frame_buf, uint8_t *i2c_command )
 * Description	: API for framing the i2c commands.
 * Arguments	: i2c_cmd_frame i2c_frame_buf - Typedef structure with the i2c request details filled.
 * 		: uint8_t *i2c_command - An empty pointer to store the framed i2c command. \
 * Return Value	: 0 for success and error code for failure.
 * */
int i2c_buf_frame( i2c_cmd_frame i2c_frame_buf, uint8_t *i2c_command )
{
	int ret = OBD2_LIB_FAILURE;
	uint32_t frame_length = 0;
	uint8_t i = 0;
	uint8_t j = 0;

	if( i2c_frame_buf.length <= (MAX_I2C_FRAME_SIZE - MCU_I2C_CMD_DATA_BYTE) )
	{
		(void) memset(i2c_command, 0, (i2c_frame_buf.length + MCU_I2C_CMD_DATA_BYTE));
#if MCU_I2C_BUF_FRAME_DEBUG_EN
		print_frame_details( i2c_frame_buf );
#endif
		i2c_command[MCU_I2C_REG_ADDR_BYTE] = i2c_frame_buf.reg;
		i2c_command[MCU_I2C_CMD_ID_BYTE] = i2c_frame_buf.command_id;
		i2c_command[MCU_I2C_CMD_LENGTH_BYTE] = i2c_frame_buf.length;

		frame_length = MIN_I2C_FRAME_SIZE;

		j = 0;
		i = MCU_I2C_CMD_DATA_BYTE;
		while( j < frame_length )
		{
			i2c_command[i] = i2c_frame_buf.data[j];
			i ++;
			j ++;
		}

#if MCU_I2C_BUF_FRAME_DEBUG_EN
		printf("************ i2c_command is: *************\n");
		i = 0;
		while( i <= (i2c_command[MCU_I2C_CMD_LENGTH_BYTE] + MCU_I2C_CMD_LENGTH_BYTE))
		{
			printf("%x|", i2c_command[i]);
			i ++;
		}
		printf("\n\n");
#endif
		ret = OBD2_LIB_SUCCESS;
	}
	else
	{
		ret = E_LIB_LENGTH_OVERFLOW;
	}

	return ret;
}

/*
 * API		: int i2c_buf_decode( i2c_cmd_frame *i2c_decode_buf, uint8_t *i2c_command )
 * Description	: API for decoding the i2c frame received. Expected i2c frame format is as below:
 * 			<Response ID(1 byte)>, <Return/Error Code(4 bytes)>, <Length(1 byte)>, <Data(Length bytes)>
 * Arguments	: i2c_cmd_decode - Typedef structure to store the received i2c frame.
 * 		: uint8_t *i2c_frame - Pointer to the i2c frame received. \
 * Return Value	: 0 for success and error code for failure.
 * */
int i2c_buf_decode( i2c_cmd_decode *i2c_decode_buf, uint8_t *i2c_command )
{
	int ret = OBD2_LIB_FAILURE;
	uint8_t i = 0;
	uint8_t j = 0;

	i2c_decode_buf->data = (uint8_t *) calloc ( i2c_command[MCU_I2C_RES_LENGTH_BYTE], sizeof(uint8_t));
	if( i2c_command[MCU_I2C_RES_LENGTH_BYTE] <= (MAX_I2C_FRAME_SIZE - MCU_I2C_RES_DATA_BYTE) )
	{
		if( (i2c_command[MCU_I2C_RES_ID_BYTE] >= CPU_MCU_WAKEUP_SOURCE_RESPONSE) && (i2c_command[MCU_I2C_RES_ID_BYTE] < NUM_OF_RESPONSE_COMMAND_ID) )
		{
			i2c_decode_buf->response_id = i2c_command[MCU_I2C_RES_ID_BYTE];
#if MCU_I2C_BUF_FRAME_DEBUG_EN
			printf("i2c_decode_buf->response_id = |%x|\n", i2c_decode_buf->response_id);
#endif
			for( i = 0; i < sizeof(i2c_decode_buf->err_code); i ++ )
			{
#if MCU_I2C_BUF_FRAME_DEBUG_EN
				printf("i2c_command.err_code[%d] = |%x|\n", i, i2c_command[MCU_I2C_RES_ERR_CODE_BYTE+i]);
#endif
				i2c_decode_buf->err_code[i] = i2c_command[MCU_I2C_RES_ERR_CODE_BYTE+i];
			}
			i2c_decode_buf->length = i2c_command[MCU_I2C_RES_LENGTH_BYTE];

			i = 0;
			j = MCU_I2C_RES_DATA_BYTE;
			while( i < i2c_decode_buf->length )
			{
				i2c_decode_buf->data[i] = i2c_command[j];
				i ++;
				j ++;
			}
#if MCU_I2C_BUF_FRAME_DEBUG_EN
			print_decoded_frame_details( *i2c_decode_buf );
#endif
			ret = OBD2_LIB_SUCCESS;
		}
		else
		{
			ret = E_LIB_MCU_INVALID_RES_ID;
		}
	}
	else
	{
		ret = E_LIB_LENGTH_OVERFLOW;
	}

	return ret;
}

/*
 * API		: int MCU_sleep_mode( uint8_t sleep_mode, uint8_t wakeup_source )
 * Description	: API to initiate the MCU sleep Mode
 * Arguments	: uint8_t sleep_mode - The sleep mode to be performed.
 *		: uint8_t wakeup_source - This byte will store the wakeup sources to be configured.
 * Return Value	: 0 for success and error code for failure.
 * */
int MCU_sleep_mode( uint8_t sleep_mode, uint8_t wakeup_source)
{
	int ret = OBD2_LIB_SUCCESS;
	uint8_t i2c_command[MIN_I2C_FRAME_SIZE];
	i2c_cmd_frame i2c_frame_buf;

	if( sleep_mode == SLEEP_MODE || sleep_mode == DEEP_POWER_DOWN_MODE )
	{
		if( (wakeup_source & CPU_MCU_TIMER_WAKEUP_REQUEST_BIT_MASK) || (wakeup_source & CPU_MCU_IGN_WAKEUP_REQUEST_BIT_MASK) \
				|| (wakeup_source & CPU_MCU_ACC_WAKEUP_REQUEST_BIT_MASK) || (wakeup_source & CPU_MCU_CAN_WAKEUP_REQUEST_BIT_MASK) \
				|| (wakeup_source & CPU_MCU_RTC_WAKEUP_REQUEST_BIT_MASK) )
		{
			/* Timer Wakeup is not supported in Deep Power Down Mode */
			if( !(((wakeup_source & CPU_MCU_TIMER_WAKEUP_REQUEST_BIT_MASK) || (wakeup_source & CPU_MCU_RTC_WAKEUP_REQUEST_BIT_MASK)) && (sleep_mode == DEEP_POWER_DOWN_MODE)))
			{
				i2c_frame_buf.length = 2;
				i2c_frame_buf.data = (uint8_t *) calloc ( MIN_I2C_FRAME_SIZE, sizeof(uint8_t));
				if( i2c_frame_buf.data != NULL )
				{
					i2c_frame_buf.i2c_addr = MCU_SLAVE_ADDRESS;
					i2c_frame_buf.reg = MCU_REG_ADDRESS;
					i2c_frame_buf.command_id = CPU_MCU_SLEEP_REQUEST;
					i2c_frame_buf.data[0] = sleep_mode;
					i2c_frame_buf.data[1] = wakeup_source;

					if( (wakeup_source & CPU_MCU_ACC_WAKEUP_REQUEST_BIT_MASK) && (sleep_mode == DEEP_POWER_DOWN_MODE) )
					{
						/*
						 * Disable the Accelerometer interrupt from the CPU. \
						 * This is done to avoid the i2c line hanging condition \
						 * when the Interrupt level is changed for MCU DPDM IRQ detection. \
						 * The accelerometer and gyroscope sensors cannot be \
						 * used for data reading once the interrupt is disabled. \
						 */
						ret = set_gpio_value( ACC_INTERRUPT_GPIO, OFF );
						if( ret == OBD2_LIB_SUCCESS )
						{
							sleep(1);
							ret = i2c_write(ACC_I2C_NO, ACC_SLAVE_ADDR, LSM6DSL_CTRL3_C_REG, 0x64);
							if(ret != OBD2_LIB_SUCCESS)
							{
								ret = E_ACCELEROMETER_I2C_REG_CONFIG;
#if COMMON_DEBUG_PRINTS
								printf("%s : Failed to change the Accelerometer Interrupt level\n", __func__);
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

					if( ret == OBD2_LIB_SUCCESS )
					{
						ret = i2c_buf_frame( i2c_frame_buf, i2c_command);
						if(ret == OBD2_LIB_SUCCESS)
						{
							// i2c send
							ret = mcu_i2c_write( MCU_BUS_ADDRESS, MCU_SLAVE_ADDRESS, i2c_command );
							if(ret == OBD2_LIB_SUCCESS)
							{
#if DEBUG_PRINTS
								printf("\nArguments for %s:\n\tbus_num = %x\n\tslave_addr = %x\n\n", __func__, MCU_BUS_ADDRESS, MCU_SLAVE_ADDRESS);
#endif

								switch(sleep_mode)
								{
									case DEEP_POWER_DOWN_MODE:
										/* Disable the battery Discharge instantly. Only applicable for Deep Power Down Mode */
										ret = battery_connect_config( BAT_OFF_WITHOUT_TIMER );
										if(ret == OBD2_LIB_SUCCESS)
										{
											//sleep(3);
											printf("\n###########################################\n########## Shutting down the CPU ##########\n###########################################\n");
											ret = system("poweroff");
										}
										else
										{
											// Do Nothing
										}
										break;
									case SLEEP_MODE:
										/* Enable the MCU Wakeup Interrupt before going to Sleep Mode */
										ret = config_mcu_wakeup( ENABLE );
										if(ret == OBD2_LIB_SUCCESS)
										{
											ret = push_device_to_sleep( );
											if(ret == OBD2_LIB_SUCCESS)
											{
												if( wakeup_source & CPU_MCU_TIMER_WAKEUP_REQUEST_BIT_MASK )
												{
													ret = system("echo out > /sys/class/gpio/gpio129/direction");
													if( ret == OBD2_LIB_SUCCESS )
													{
														ret = system("echo 1 > /sys/class/gpio/gpio129/value");
														if( ret == OBD2_LIB_SUCCESS )
														{
															system("i2cset -f -y 0 0x18 0xA3");
														}
													}
												}
											}
										}
										else
										{
											// Do Nothing
										}
										break;
									default:
										{
											ret = E_LIB_MCU_INVALID_SL_MODE;
											break;
										}
								}
							}
							else
							{
								printf("mcu_i2c_write : FAILURE with return |%x|\n", ret);
							}
						}
						else
						{
							printf("i2c_buf_frame : FAILURE with return |%x|\n", ret);
						}
					}
					else
					{
						// Do Nothing
					}
				}
				else
				{
					ret = E_LIB_MEM_ALLOC_ERROR;
				}
			}
			else
			{
#if COMMON_DEBUG_PRINTS
				printf("%s : Timer Wakeup is not supported in Deep Power Down Mode\n\n", __func__ );
#endif
				ret = E_LIB_MCU_INVALID_SL_MODE;
			}
		}
		else
		{
			ret = E_LIB_MCU_INVALID_WAKE_SOURCE;
		}
	}
	else
	{
		ret = E_LIB_MCU_INVALID_SL_MODE;
	}

	return ret;
}

/*
 * API		: MCU_Wakeup_Source_Request( uint8_t *wake_source )
 * Description	: API for requesting the MCU wakeup source
 * Arguments	: uint8_t *wake_source - Variable to store the Wakeup source
 * Return Value	: 0 for success and error code for failure.
 * */
int MCU_Wakeup_Source_Request( uint8_t *wake_source )
{
	int ret = OBD2_LIB_FAILURE;
	uint8_t i2c_command[MIN_I2C_FRAME_SIZE];
	i2c_cmd_frame i2c_frame_buf;
	i2c_cmd_decode i2c_decode_buf;

	i2c_frame_buf.i2c_addr = MCU_SLAVE_ADDRESS;
	i2c_frame_buf.reg = MCU_REG_ADDRESS;
	i2c_frame_buf.command_id = CPU_MCU_WAKEUP_SOURCE_REQUEST;
	i2c_frame_buf.length = 0x00;

	i2c_frame_buf.data = (uint8_t *) calloc ( MIN_I2C_FRAME_SIZE, sizeof(uint8_t));
	if( i2c_frame_buf.data != NULL )
	{
		ret = i2c_buf_frame( i2c_frame_buf, i2c_command);
		if(ret != OBD2_LIB_SUCCESS)
		{
			printf("i2c_buf_frame : FAILURE with return |%x|\n", ret);
		}
		else
		{
			// i2c send & read
			ret = mcu_i2c_read( MCU_BUS_ADDRESS, MCU_SLAVE_ADDRESS, i2c_command );
			if(ret == OBD2_LIB_SUCCESS)
			{
				ret = i2c_buf_decode( &i2c_decode_buf, i2c_command );
				if(ret == OBD2_LIB_SUCCESS)
				{
					if( i2c_decode_buf.response_id == CPU_MCU_WAKEUP_SOURCE_RESPONSE )
					{
						if(i2c_decode_buf.data[0] != '\0')
						{
							*wake_source = i2c_decode_buf.data[0];
						}
						else
						{
							ret = E_LIB_NULL_DATA;
						}
					}
					else
					{
						ret = E_LIB_MCU_INVALID_RES_ID;
					}
				}
				else
				{
					printf("mcu_i2c_read : FAILURE with return |%x|\n", ret);
				}
			}
			else
			{
				printf("mcu_i2c_read : FAILURE with return |%x|\n", ret);
			}
		}
	}
	else
	{
		ret = E_LIB_MEM_ALLOC_ERROR;
	}

	return ret;
}

/*
 * API		: MCU_Keep_Alive_Request()
 * Description	: API for the MCU KeepAlive
 * Arguments	: None
 * Return Value	: The sleep wakeup source or error
 * */
int MCU_Keep_Alive_Request()
{
	int ret = OBD2_LIB_FAILURE;
	uint8_t i2c_command[MIN_I2C_FRAME_SIZE];
	i2c_cmd_frame i2c_frame_buf;
	i2c_cmd_decode i2c_decode_buf;

	i2c_frame_buf.i2c_addr = MCU_SLAVE_ADDRESS;
	i2c_frame_buf.reg = MCU_REG_ADDRESS;
	i2c_frame_buf.command_id = CPU_MCU_KEEPALIVE_REQUEST;
	i2c_frame_buf.length = 0x00;

	i2c_frame_buf.data = (uint8_t *) calloc ( MIN_I2C_FRAME_SIZE, sizeof(uint8_t));
	if( i2c_frame_buf.data != NULL )
	{
		ret = i2c_buf_frame( i2c_frame_buf, i2c_command);
		if(ret != OBD2_LIB_SUCCESS)
		{
			printf("i2c_buf_frame : FAILURE with return |%x|\n", ret);
		}
		else
		{
			// i2c send & read
			ret = mcu_i2c_read( MCU_BUS_ADDRESS, MCU_SLAVE_ADDRESS, i2c_command );
			if(ret == OBD2_LIB_SUCCESS)
			{
				ret = i2c_buf_decode( &i2c_decode_buf, i2c_command );
				if(ret == OBD2_LIB_SUCCESS)
				{
					if( i2c_decode_buf.response_id != CPU_MCU_KEEPALIVE_RESPONSE )
					{
						ret = E_LIB_MCU_INVALID_RES_ID;
					}
					else
					{
						// Do Nothing
					}
				}
				else
				{
					printf("i2c_buf_decode : FAILURE with return |%x|\n", ret);
				}
			}
			else
			{
				printf("mcu_i2c_read : FAILURE with return |%x|\n", ret);
			}
		}
	}
	else
	{
		ret = E_LIB_MEM_ALLOC_ERROR;
	}

	return ret;
}

/*
 * API		: MCU_FW_Version_Read_Request( char *mcu_fw_version )
 * Description	: API for reading the MCU Firmware Version
 * Arguments	: *mcu_fw_version - Character pointer to hold the MCU Firmware Version
 * Return Value	: Returns 0 on Success and other error code on Failure
 * */
int MCU_FW_Version_Read_Request( char *mcu_fw_version )
{
	int ret = OBD2_LIB_FAILURE;
	uint8_t i2c_command[MIN_I2C_FRAME_SIZE];
	i2c_cmd_frame i2c_frame_buf;
	i2c_cmd_decode i2c_decode_buf;

	i2c_frame_buf.i2c_addr = MCU_SLAVE_ADDRESS;
	i2c_frame_buf.reg = MCU_REG_ADDRESS;
	i2c_frame_buf.command_id = CPU_MCU_FIRMWARE_VERSION_READ_REQUEST;
	i2c_frame_buf.length = 0x00;

	i2c_frame_buf.data = (uint8_t *) calloc ( MIN_I2C_FRAME_SIZE, sizeof(uint8_t));
	if( i2c_frame_buf.data != NULL )
	{
		ret = i2c_buf_frame( i2c_frame_buf, i2c_command);
		if(ret != OBD2_LIB_SUCCESS)
		{
			printf("i2c_buf_frame : FAILURE with return |%x|\n", ret);
		}
		else
		{
			// i2c send & read
			ret = mcu_i2c_read( MCU_BUS_ADDRESS, MCU_SLAVE_ADDRESS, i2c_command );
			if(ret == OBD2_LIB_SUCCESS)
			{
				ret = i2c_buf_decode( &i2c_decode_buf, i2c_command );
				if(ret == OBD2_LIB_SUCCESS)
				{
					if( i2c_decode_buf.response_id != CPU_MCU_FIRMWARE_VERSION_READ_RESPONSE )
					{
						ret = E_LIB_MCU_INVALID_RES_ID;
					}
					else
					{
						if((i2c_decode_buf.data[0] != '\0') && (i2c_decode_buf.data[1] != '\0') )
						{
							if(i2c_decode_buf.length == 2)
							{
								ret = sprintf( mcu_fw_version, "iW-PRGST-SC-R1.0-REL%d.%d_MCU_FW", \
										i2c_decode_buf.data[0], i2c_decode_buf.data[1]);
							}
							else if(i2c_decode_buf.length == 3)
							{
								ret = sprintf( mcu_fw_version, "iW-PRGST-SC-R1.0-REL%d.%d.%d_MCU_FW", \
										i2c_decode_buf.data[0], i2c_decode_buf.data[1], i2c_decode_buf.data[2]);
							}
							else
							{
								ret = E_LIB_MCU_INVALID_FW_VER;
							}

							if(ret > OBD2_LIB_SUCCESS)
							{
#if DEBUG_PRINTS
								printf("%s : mcu_fw_version is |%s|\n\n", __func__, mcu_fw_version);
#endif
								ret = OBD2_LIB_SUCCESS;
							}
							else
							{
								ret = E_LIB_MEM_WRITE_ERROR;
							}
						}
						else
						{
							ret = E_LIB_NULL_DATA;
						}
					}
				}
				else
				{
					printf("i2c_buf_decode : FAILURE with return |%x|\n", ret);
				}
			}
			else
			{
				printf("mcu_i2c_read : FAILURE with return |%x|\n", ret);
			}
		}
	}
	else
	{
		ret = E_LIB_MEM_ALLOC_ERROR;
	}

	return ret;
}

/*
 * API		: MCU_ADC_Read_Request( int adc, float *voltage )
 * Description	: API for the MCU KeepAlive
 * Arguments	: adc - ADC name in numbers:
 * 			1. 12V ADC
 * 			2. Battery ADC
 * 			3. ADC1
 * 			4. ADC2
 * 		: *voltage - Floating variable to hold the ADC voltage
 * Return Value	: Returns 0 on Success and other error code on Failure
 * */
int MCU_ADC_Read_Request( int adc, float *voltage )
{
	int ret = OBD2_LIB_FAILURE;
	uint16_t raw_value = 0;
	float calc = 0;
	uint8_t i2c_command[MIN_I2C_FRAME_SIZE];
	i2c_cmd_frame i2c_frame_buf;
	i2c_cmd_decode i2c_decode_buf;

	i2c_frame_buf.i2c_addr = MCU_SLAVE_ADDRESS;
	i2c_frame_buf.reg = MCU_REG_ADDRESS;
	i2c_frame_buf.command_id = CPU_MCU_ADC_READ_REQUEST;
	i2c_frame_buf.length = 0x01;

	i2c_frame_buf.data = (uint8_t *) calloc ( MIN_I2C_FRAME_SIZE, sizeof(uint8_t));
	if( i2c_frame_buf.data != NULL )
	{
		if( adc == 1 )
		{
			/* 12V ADC */
			i2c_frame_buf.data[0] = 0x07;
			calc = 0.0088623046875;
		}
		else if( adc == 2 )
		{
			/* Battery ADC */
			i2c_frame_buf.data[0] = 0x02;
			calc = 0.00169189453125;
		}
		else if( adc == 3 )
		{
			/* ADC1 */
			i2c_frame_buf.data[0] = 0x05;
			calc = 0.0088623046875;
		}
		else if( adc == 4 )
		{
			/* ADC2 */
			i2c_frame_buf.data[0] = 0x04;
			calc = 0.0088623046875;
		}
		else
		{
			ret = E_OBD2_LIB_INVALID_ARG;
		}

		if(ret != E_OBD2_LIB_INVALID_ARG)
		{
			ret = i2c_buf_frame( i2c_frame_buf, i2c_command);
			if(ret != OBD2_LIB_SUCCESS)
			{
				printf("i2c_buf_frame : FAILURE with return |%x|\n", ret);
			}
			else
			{
				// i2c send & read
				ret = mcu_i2c_read( MCU_BUS_ADDRESS, MCU_SLAVE_ADDRESS, i2c_command );
				if(ret == OBD2_LIB_SUCCESS)
				{
					ret = i2c_buf_decode( &i2c_decode_buf, i2c_command );
					if(ret == OBD2_LIB_SUCCESS)
					{
						if( i2c_decode_buf.response_id != CPU_MCU_ADC_READ_RESPONSE )
						{
							ret = E_LIB_MCU_INVALID_RES_ID;
						}
						else
						{
							raw_value = i2c_decode_buf.data[0] & 0xFF;
							raw_value = raw_value << 8;
							raw_value |= i2c_decode_buf.data[1];
							if(adc == 3 || adc == 4) /* Analog 1 & 2 */
							{
								if(raw_value < 2300) /* OFFSET breaking from 0- 19 & 20 - 36 */
									raw_value = raw_value - 118; /* OFFSET 118 for the range of input 0 - 19 */
								else
									raw_value = raw_value - 65; /* OFFSET 65 for the range of input 20- 36 */
							}
							else if (adc == 1) /* 12 V ADC */
							{
								if(raw_value < 2575) /* OFFSET breaking from 0-22 , 23-31 & 32-36 */
									raw_value = raw_value - 65; /* OFFSET 65 for the range of input 0- 22 */
								else if(raw_value < 3540)
									raw_value = raw_value - 10; /* OFFSET 10 for the range of input 23- 31 */
								else
									raw_value = raw_value - 40; /* OFFSET 40 for the range of input 32- 36 */
							}
							*voltage = calc * raw_value;
						}
					}
					else
					{
						printf("i2c_buf_decode : FAILURE with return |%x|\n", ret);
					}
				}
				else
				{
					printf("mcu_i2c_read : FAILURE with return |%x|\n", ret);
				}
			}
		}
		else
		{
			ret = E_LIB_MEM_ALLOC_ERROR;
		}
	}

	return ret;
}

/*
 * API		: int set_i2c_register_values( uint32_t i2c_file, uint8_t addr, uint8_t reg, uint8_t *values, uint32_t length )
 * Description	: Common API for CPU to MCU i2c Write. \
 * Arguments	: i2c_file - Socket opened for i2c.
 * 		: i2c_addr - The i2c Slave address.
 * 		: i2c_frame - This is the i2c command frame to be send to the MCU.
 * 		: bus_num - The i2c bus to which the slave is connected.
 * Return Value : 0 on Success & ioctl Error Code on Failure
 * */
int set_i2c_register_values( uint32_t i2c_file, uint8_t i2c_addr, uint8_t *i2c_frame, uint32_t length )
{
	int ret = OBD2_LIB_FAILURE;
	int i = 0;
	uint8_t outbuf[length];
	struct i2c_rdwr_ioctl_data packets;
	struct i2c_msg messages[1];

#if MCU_I2C_WRITE_DEBUG_EN
	printf("%s :\n\ti2c_file : |%x|\n\ti2c_addr : %x\n\tlength : %x\n\t", __func__, i2c_file, i2c_addr, length );
	printf("i2c_frame : |");
	for(i = 0; i < length; i++)
	{
		printf("%x|", i2c_frame[i]);
	}
	printf("\n");
#endif

	messages[0].addr = i2c_addr;
	messages[0].flags = 0;
	messages[0].len = sizeof(outbuf);
	messages[0].buf = outbuf;

	/*
	 * The first byte indicates which register we'll write.
	 * The second byte indicates the value to write. Note that for many
	 * devices, we can write multiple, sequential registers at once by
	 * simply making outbuf bigger.
	 */
	for(i = 0; i < length; i++)
	{
		outbuf[i] = i2c_frame[i];
	}

	/* Transfer the i2c packets to the kernel and verify it worked */
	packets.msgs = messages;
	packets.nmsgs = 1;

	if(ioctl(i2c_file, I2C_RDWR, &packets) < 0)
	{
		perror("Unable to send data");
		ret = E_LIB_MCU_I2C_WRITE_ERR;
	}
	else
	{
		ret = OBD2_LIB_SUCCESS;
	}

	return ret;
}

/*
 * API		: int mcu_i2c_write(int bus_num, uint8_t i2c_addr, uint8_t *i2c_frame)
 * Description	: Common API for CPU to MCU i2c Write. \
 * Arguments	: bus_num - The i2c bus to which the slave is connected.
 * 		: i2c_addr - The i2c address to which the data to be send.
 * 		: i2c_frame - This is the i2c command frame to be send to the MCU. \
 * 			The i2c frame should be in below format: \
 * 			For MCU:
 * 				<i2c_register_address>, <i2c_command_id>, <i2c_data_length>, <i2c_data>
 * 			For Other i2c transfer:
 * 				<i2c_register_address>, <i2c_data>
 * Return Value : 0 on Success & Error Code on Failure
 * */
int mcu_i2c_write( int bus_num, uint8_t i2c_addr, uint8_t *i2c_frame )
{
	uint32_t i2c_file;
	int ret = OBD2_LIB_FAILURE;
	int i = 0;
	char i2c_file_name[20] = {0};

	(void) memset(i2c_file_name, 0, sizeof(i2c_file_name));
	sprintf( i2c_file_name, "%s%d", I2C_FILE_NAME, bus_num );

#if MCU_I2C_WRITE_DEBUG_EN
	printf("\t%s : i2c_file_name : |%s|\n", __func__, i2c_file_name);
	printf("\t%s : i2c_frame length : |%d|\n\n", __func__, (i2c_frame[MCU_I2C_CMD_LENGTH_BYTE] + (MCU_I2C_CMD_LENGTH_BYTE + 1)));
	printf("i2c_frame :\n\t|");
	for( i = 0; i <= (i2c_frame[MCU_I2C_CMD_LENGTH_BYTE] + (MCU_I2C_CMD_LENGTH_BYTE + 1)); i ++ )
	{
		printf("%x|", i2c_frame[i]);
	}
	printf("\n\n");
#endif

	/* Open a connection to the I2C userspace control file */
	if((i2c_file = open( i2c_file_name, O_RDWR)) < 0 )
	{
		perror("Unable to open i2c control file");
	}
	else
	{
		ret = set_i2c_register_values(i2c_file, i2c_addr, i2c_frame, MIN_I2C_FRAME_SIZE);
		usleep(500);
		close(i2c_file);
	}

	return ret;
}

/*
 * API		: int mcu_i2c_read( uint32_t bus_num, uint8_t i2c_addr, uint8_t *i2c_frame)
 * Description	: Common API for CPU to MCU i2c Read. \
 *			mcu_i2c_read() will wait for a particular time if no data is available.
 * Arguments	: bus_num - The i2c bus to which the slave is connected.
 * 		: i2c_addr - The i2c address to which the data to be send.
 * 		: i2c_frame - An empty pointer to read the i2c command frame sent by the MCU.
 * Return Value	: 0 on Success & Error Code on Failure
 * */
int get_i2c_register_values( uint32_t i2c_file, uint8_t i2c_addr, uint8_t *i2c_frame, uint32_t length )
{
	int ret = OBD2_LIB_FAILURE;
	int i = 0;
	uint8_t outbuf[length]; // For i2c write
	uint8_t inbuf[length]; // For Reading the i2c frame
	struct i2c_rdwr_ioctl_data packets;
	struct i2c_msg messages[2];
	struct timeval timeout;
	fd_set read_set;

	(void) memset( inbuf, 0, sizeof(inbuf) );

#if MCU_I2C_WRITE_DEBUG_EN
	printf("%s :\n\ti2c_file : |%x|\n\ti2c_addr : %x\n\tlength : %x\n\t\n\n", __func__, i2c_file, i2c_addr, length );
	printf("i2c_frame : |");
	for(i = 0; i < length; i++)
	{
		printf("%x|", i2c_frame[i]);
	}
	printf("\n");
#endif

	for( i = 0; i < length; i ++ )
	{
		outbuf[i] = i2c_frame[i];
	}

	/* The first byte indicates which register to write dummy data */
	messages[0].addr = i2c_addr;
	messages[0].flags = 0;
	messages[0].len = sizeof(outbuf);
	messages[0].buf = &outbuf[0];

	(void) memset( i2c_frame, 0, sizeof(inbuf) );
	messages[1].addr = i2c_addr;
	messages[1].flags = I2C_M_RD /* | I2C_M_NOSTART*/;
	messages[1].len = sizeof(inbuf);
	messages[1].buf = &inbuf[0];

	/*
	 * The second byte indicates the value to write. Note that for many
	 * devices, we can write multiple, sequential registers at once by
	 * simply making outbuf bigger.
	 */

	/* Transfer the i2c packets to the kernel and verify it worked */
	packets.msgs = messages;
	packets.nmsgs = 2;

	/* timeout wait for 2000ms */
	timeout.tv_sec = 5;
	timeout.tv_usec = 200000000;

	if(ioctl(i2c_file, I2C_RDWR, &packets) < 0)
	{
		perror("Unable to read data");
		ret = E_LIB_MCU_I2C_READ_ERR;
	}
	else
	{
		for( i = 0; (i < inbuf[MCU_I2C_RES_LENGTH_BYTE] + MCU_I2C_RES_DATA_BYTE); i ++)
		{
			i2c_frame[i] = inbuf[i];
		}
		ret = OBD2_LIB_SUCCESS;

#if MCU_I2C_READ_DEBUG_EN
		printf("inbuf read is :\n\t|");
		for( i = 0; i < sizeof(inbuf); i ++)
		{
			printf("%x|", inbuf[i]);
		}
		printf("\n");

		printf("i2c_frame read is :\n\t|");
		for( i = 0; i < sizeof(inbuf); i ++)
		{
			printf("%x|", i2c_frame[i]);
		}
		printf("\n\n");
#endif
		ret = OBD2_LIB_SUCCESS;
	}

	return ret;
}

int mcu_i2c_read( uint32_t bus_num, uint8_t i2c_addr, uint8_t *i2c_frame )
{
	uint32_t i2c_file;
	int ret = OBD2_LIB_FAILURE;
	char i2c_file_name[20] = {0};
	int i = 0;

	(void) memset(i2c_file_name, 0, sizeof(i2c_file_name));
	sprintf( i2c_file_name, "%s%d", I2C_FILE_NAME, bus_num );

	/* Open a connection to the I2C userspace control file */
	if((i2c_file = open( i2c_file_name, O_RDWR )) < 0 )
	{
		perror("Unable to open i2c control file");
		ret = errno;
	}
	else
	{
#if MCU_I2C_READ_DEBUG_EN
		printf("\t%s : i2c_addr is : |%x|\n", __func__, i2c_addr);
		printf("\t%s : bus_num is : |%x|\n", __func__, bus_num);
		printf("\t%s : i2c_file_name : |%s|\n", __func__, i2c_file_name);
		printf("\t%s : i2c_file is : |%x|\n", __func__, i2c_file);
		printf("\t%s : i2c_frame length : |%d|\n\n", __func__, (i2c_frame[MCU_I2C_CMD_LENGTH_BYTE] + (MCU_I2C_CMD_LENGTH_BYTE + 1)));
		printf("\t%s : i2c_frame :\n\t|", __func__ );
		for( i = 0; i < MIN_I2C_FRAME_SIZE; i ++ )
		{
			printf("%x|", i2c_frame[i]);
		}
		printf("\n\n");
#endif

		ret = get_i2c_register_values( i2c_file, i2c_addr, i2c_frame, MIN_I2C_FRAME_SIZE);
		usleep(500);
		close(i2c_file);
		if( ret != OBD2_LIB_SUCCESS )
		{
			ret = E_LIB_MCU_I2C_READ_ERR;
		}
		else
		{
#if MCU_I2C_READ_DEBUG_EN
			printf("\t%s : i2c_frame length : |%d|\n\n", __func__, (i2c_frame[MCU_I2C_CMD_LENGTH_BYTE] + (MCU_I2C_CMD_LENGTH_BYTE + 1)));
			printf("\t%s : i2c_frame length : |%d|\n\n", __func__, (i2c_frame[MCU_I2C_CMD_LENGTH_BYTE] + MCU_I2C_RES_LENGTH_BYTE));
			printf("\t%s : i2c_frame :\n\t|", __func__ );
			for( i = 0; i < (i2c_frame[MCU_I2C_CMD_LENGTH_BYTE] + MCU_I2C_RES_DATA_BYTE); i ++ )
			{
				printf("%x|", i2c_frame[i]);
			}
			printf("\n\n");
#endif
		}
	}

	return ret;
}
