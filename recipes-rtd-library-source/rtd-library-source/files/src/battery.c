#define _GNU_SOURCE
#include "lib_battery.h"

int i_battery_get_voltage(double *i_bat_volt)
{
	int ret_read = OBD2_LIB_FAILURE;
	float voltage = 0;
	static char *event_file = NULL;

	ret_read = asprintf (&event_file, "%s", IBATTERY_VOLTAGE);
	if(ret_read < 0)
	{
		ret_read = -ENOMEM;
		goto end;
	}
	ret_read = read_sysfs_float("in_voltage5_raw",event_file,&voltage);
	if(ret_read < 0)
	{
		CHK_ERR (E_IBAT_GET_VOL_NODE_READ, stderr, "Error: i_battery_get_voltage() Read Internal Battery Volatage");
		ret_read = E_IBAT_GET_VOL_NODE_READ;
		goto end;
	}
	else
	{
		*i_bat_volt = ((((voltage * 3300) / 4096) / 1000) * 1.5);
		ret_read = OBD2_LIB_SUCCESS;
	}
end:
	if(event_file != NULL){
		free(event_file);
		event_file = NULL;
	}
	return ret_read;
}

int i_battery_get_health()
{
	int fd = 0;
	int ret_read;
	char var[50] = {0};
	int health = 0;
	int rc = OBD2_LIB_FAILURE;

	fd = open(IBATTERY_NODE "/health", O_RDONLY);
	if (fd > 0)
	{
		lseek(fd, 0, SEEK_SET);
		ret_read = read(fd, var, sizeof(var)-1);
		if( ret_read < 0 )
		{
			if( fd )
			{
				close(fd);
				fd = 0;
			}

			CHK_ERR (E_IBAT_GET_HEALTH_NODE_READ, stderr, "Error: i_battery_get_health() Read Internal Battery Health");
			rc = E_IBAT_GET_HEALTH_NODE_READ;
		}
		else{
			health = strncmp(var, "Good", 4);

			if( fd )
			{
				close(fd);
				fd = 0;
			}
			if (health == 0)
				rc = IBATTERY_STRENGTH_GOOD;
			else
				rc = IBATTERY_STRENGTH_NOT_GOOD;
		}
	}
	else{
		CHK_ERR (E_IBAT_GET_HEALTH_NODE_READ, stderr, "Error: i_battery_get_health() Read Internal Battery Health");
		rc = E_IBAT_GET_HEALTH_NODE_READ;
	}

	return rc;
}

int i_battery_init()
{
	int ret = OBD2_LIB_FAILURE;

	ret = i2c_write(IBAT_I2C_NO, BATTERY_SLAVE_ADDR, BATTERY_CNTRL_REG1, 0x17);
	CHK_ERR (ret, stderr, "Error: i_battery_init() input current limiter for battery charger IC");

	/*charging current limited to 120mA*/
	ret = i2c_write(IBAT_I2C_NO, BATTERY_SLAVE_ADDR, BATTERY_CNTRL_REG2, 0x85);
	CHK_ERR (ret, stderr, "Error: i_battery_init() charging current limited to 120mA");

	/*pre-charging and terminal charging current limited to 60mA*/
	ret = i2c_write(IBAT_I2C_NO, BATTERY_SLAVE_ADDR, BATTERY_CNTRL_REG3, 0x11);
	CHK_ERR (ret, stderr, "Error: i_battery_init() pre-charging and terminal charging current limited to 60mA");

	/*watchdog timer is disabled*/
	ret = i2c_write(IBAT_I2C_NO, BATTERY_SLAVE_ADDR, BATTERY_CNTRL_REG4, 0x8F);
	CHK_ERR (ret, stderr, "Error: i_battery_init() watchdog timer is disabled");

	return ret;
}


int i_get_battery_status( int *b_chrg_status )
{
	int ret = OBD2_LIB_FAILURE , ret_val = OBD2_LIB_FAILURE;
	uint8_t value;
	value = 0;

	ret = i2c_read(IBAT_I2C_NO, BATTERY_SLAVE_ADDR, BATTERY_STATUS_REG_ADDR,&value);
	if ( ret == OBD2_LIB_SUCCESS )
	{
		if(value == 0x65)
		{
			ret_val = E_IBATTERY_NOT_CONNECTED;
			*b_chrg_status = IBATTERY_NOT_CONNECTED;
		}
		else if(value == 0x7C)
		{
			ret_val = E_IBATTERY_FULLY_CHARGED;
			*b_chrg_status = IBATTERY_FULLY_CHARGED;
		}
		else if(value == 0x74)
		{
			ret_val = E_IBATTERY_CHARGING;
			*b_chrg_status = IBATTERY_CHARGING;
		}
		else if(value == 0x00)
		{
			ret_val = E_IBATTERY_NOT_CHARGING;
			*b_chrg_status = IBATTERY_NOT_CHARGING;
		}
		else
		{
			ret_val = E_IBATTERY_UNSTABLE_STATE;
			*b_chrg_status = IBATTERY_UNSTABLE_STATE;
		}
	}

	printf("b_chrg_status value %x\n", *b_chrg_status );
	return ret_val;
}

/*
 * API used to enable and disable battery charging
 * if state = 1 , charge disable
 * If state = 0 , charge enable
 */
int battery_charge_state_config(int state)
{
	int error = OBD2_LIB_FAILURE;
	int ret = OBD2_LIB_FAILURE;

	if(state == ON)
	{
		ret = set_gpio_value(BATTERY_CE_GPIO, ON);
		if(ret != OBD2_LIB_SUCCESS)
		{
			error = OBD2_LIB_FAILURE;
		}else{
			error = OBD2_LIB_SUCCESS;
		}
	}
	else if(state == OFF)
	{
		ret = set_gpio_value(BATTERY_CE_GPIO, OFF);
		if(ret != OBD2_LIB_SUCCESS)
		{
			error = OBD2_LIB_FAILURE;
		}else{
			error = OBD2_LIB_SUCCESS;

		}

	}else{
		error = INVALID_VALUE ;

	}

	return error;
}

int get_power_source()
{
	int ret = OBD2_LIB_FAILURE;
	int b_conn_status = 0;
	int status = OBD2_LIB_FAILURE;
	/* Get Battery Power inturrupt GPIO value \
	 * if value = 1008 , Using External Power(Even if we connect both value will be zero, but we are disabling battery registers so that device will use 12V power) \
	 * if value = 1009 , Using Battery Power \
	 * */
	ret = get_gpio(BATTERY_PG_GPIO,&status);
	if(ret == 0)
	{
		if (status == 0)
		{
			b_conn_status = EXTERNAL_POWER;
		}
		if( status == 1)
		{
			b_conn_status = INTERNAL_POWER;

		}
	}
	else
	{
		b_conn_status = OBD2_LIB_FAILURE;
	}
	return b_conn_status;
}

/*
 * Battery Disconnect
 * If con_status == BAT_OFF_WITH_TIMER - Shipment mode will be enabled with a timer of 10 seconds. \
 * 			(Even if the battery is connected, no power flow beween device and battery.) \
 * If con_status == BAT_ON - Shipment mode will be disabled. All battery functionalities work accordingly. \
 * If con_status == BAT_OFF_WITHOUT_TIMER - Shipment mode will be enabled instantly. \
 * 			(Even if the battery is connected, no power flow beween device and battery.) \
 */
int battery_connect_config(int con_status)
{
	int ret = OBD2_LIB_FAILURE;
	if(con_status == BAT_OFF_WITH_TIMER)
	{
		ret = i2c_write(i2C_BUS_1,BATTERY_SLAVE_ADDR,0x07,0x6c);
		if( ret != OBD2_LIB_SUCCESS)
		{
			CHK_ERR (ret, stderr, "Error: battery_connect_config()_con_status == BAT_OFF_WITH_TIMER I2C command to disconnect battery");
		}
		else
		{
			// Do Nothing
		}
	}
	else if(con_status == BAT_ON)
	{
		ret = i2c_write(i2C_BUS_1,BATTERY_SLAVE_ADDR,0x07,0x4c);
		if( ret != OBD2_LIB_SUCCESS)
		{
			CHK_ERR (ret, stderr, "Error: battery_connect_config()_con_status == BAT_ON I2C command to enable battery");
		}
		else
		{
			// Do Nothing
		}
	}

	else if(con_status == BAT_OFF_WITHOUT_TIMER)
	{
		ret = i2c_write(i2C_BUS_1,BATTERY_SLAVE_ADDR,0x07,0x64);
		if( ret != OBD2_LIB_SUCCESS )
		{
			CHK_ERR (ret, stderr, "Error: battery_connect_config()_con_status == BAT_OFF_WITHOUT_TIMER I2C command to disconnect battery");
		}
		else
		{
			// Do Nothing
		}
	}
	else
	{
		ret = E_OBD2_LIB_INVALID_ARG;
	}

	return ret;
}

int i_get_battery_temp( int *b_temp_status )
{
	int ret = OBD2_LIB_FAILURE , ret_val = OBD2_LIB_FAILURE;
	uint8_t temp, value = 0;
	temp = 0;
	unsigned int mask = 0x7;

	ret = i2c_read(IBAT_I2C_NO, BATTERY_SLAVE_ADDR, BATTERY_TEMP_STATUS_REG_ADDR, &value);
	if ( ret == OBD2_LIB_SUCCESS )
	{
		temp |= value & mask;
		printf("The battery temperature is %X\n", temp);
		if(temp == 0)
		{
			ret_val = E_IBATTERY_NORMAL_TEMP;
			*b_temp_status = IBATTERY_NORMAL_TEMP;
		}
		else if(temp == 2)
		{
			ret_val = E_IBATTERY_WARM_TEMP;
			*b_temp_status = IBATTERY_WARM_TEMP;
		}
		else if(temp == 3)
		{
			ret_val = E_IBATTERY_COOL_TEMP;
			*b_temp_status = IBATTERY_COOL_TEMP;
		}
		else if(temp == 5)
		{
			ret_val = E_IBATTERY_COLD_TEMP;
			*b_temp_status = IBATTERY_COLD_TEMP;
		}
		else
		{
			ret_val = E_IBATTERY_HOT_TEMP;
			*b_temp_status = IBATTERY_HOT_TEMP;
		}
	}

	printf("b_temp_status value %x\n", *b_temp_status );
	return ret_val;
}

/*
 * API		: int battery_control_monitoring()
 * Description	: API to control the battery charging or discharging based \
 * 			on the input tempearature. \
 * Arguments	: battery_monitoring_interval - The interval in seconds to monitor the battery temperature.
 * Return Value	: 0 on Success & Error Code on Failure
 * */
int battery_control_monitoring( uint32_t battery_monitoring_interval )
{
	int ret = OBD2_LIB_FAILURE;
	float_t temperature;

	if( battery_monitoring_interval > OBD2_LIB_SUCCESS )
	{
		while( 1 )
		{
			ret = acc_temp_read(&temperature );
			printf("|%d| Accelerometer temp is |%f|\n", ret, temperature);
			if( ret == OBD2_LIB_SUCCESS )
			{
				if( (temperature <= 0) || (temperature >= 45) )
				{
					/* Disable the Battery Charging */
					ret = battery_charge_state_config( ON );
					if( ret == OBD2_LIB_SUCCESS )
					{
						// Do Nothing
					}
					else
					{
						/* Error while controlling the battery */
						break;
					}
				}
				else if( (temperature > 0) && (temperature < 45) )
				{
					/* Enable the Battery Charging */
					ret = battery_charge_state_config( OFF );
					if( ret == OBD2_LIB_SUCCESS )
					{
						// Do Nothing
					}
					else
					{
						/* Error while controlling the battery */
						break;
					}
				}

				if( (temperature <= -20) || (temperature >= 60) )
				{
					/* Disable the battery Discharging */
					ret = battery_connect_config( BAT_OFF_WITHOUT_TIMER );
					if( ret == OBD2_LIB_SUCCESS )
					{
						// Do Nothing
					}
					else
					{
						/* Error while controlling the battery */
						break;
					}
				}
				else if( (temperature > -20) || (temperature < 60) )
				{
					/* Enable the battery Discharging */
					ret = battery_connect_config( BAT_ON );
					if( ret == OBD2_LIB_SUCCESS )
					{
						// Do Nothing
					}
					else
					{
						/* Error while controlling the battery */
						break;
					}
				}
			}
			else
			{
				/* Error while reading the temperature */
				break;
			}
			sleep( battery_monitoring_interval );
		}
	}
	else
	{
		ret = E_OBD2_LIB_INVALID_ARG;
	}
	return ret;
}
