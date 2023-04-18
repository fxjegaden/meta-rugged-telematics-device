#define _GNU_SOURCE
#include <stdlib.h>
#include "lib_common.h"

/*!
 * \brief
 * gpio_export function to export the gpio
 *
 * \details
 * This function is to export the gpio from arguments
 *
 * \param [in] gpio
 * GPIO pin number
 *
 * \param [in] direction
 * GPIO Direction INPUT/OUTPUT
 *
 * \return
 * Success - OBD2_LIB_SUCCESS
 * Error - \
 * 	1. E_GPIO_EXP_OPEN - GPIO Number is not proper \
 * 	2. E_GPIOgpio_path_EXP_WRT - Failed to export the GPIO \
 */
int gpio_export(int gpio, int direction)
{
	FILE * pFile;
	int ret =0;
	int error = 0;
	char *gpio_path = NULL;
	char *gpio_buf = NULL;
	ret = asprintf(&gpio_buf,"%d",gpio);
	if(ret < 0)
	{
		error = -ENOMEM;
		goto end;
	}
	ret = asprintf(&gpio_path,"/sys/class/gpio/gpio%d", gpio);
	if(ret < 0)
	{
		error = -ENOMEM;
		goto end;
	}
	if (access(gpio_path, F_OK ) == 0)
	{
		error = OBD2_LIB_SUCCESS;
	}
	else
	{
		pFile=fopen("/sys/class/gpio/export", "wr");
		if (pFile==NULL)
		{
			IOBD_DEBUG_LEVEL3("File open error \r\n");
			error = E_GPIO_EXP_OPEN;
			goto end;
		}
		ret = fwrite( gpio_buf,1, strlen(gpio_buf) ,pFile);
		if (ret < 0)
		{
			fclose(pFile);
			error = E_GPIO_EXP_WRT;
		}
		else
		{
			fclose(pFile);
			error = set_gpio_direction(gpio, direction );
			CHK_ERR (error, stderr, "Error in gpio_direction()");
		}
	}

end:
	if (gpio_buf != NULL)
	{
		free(gpio_buf);
		gpio_buf = NULL;
	}
	if (gpio_path != NULL)
	{
		free(gpio_path);
		gpio_path = NULL;
	}
	return error;
}

/*!
 * \brief
 * set_gpio_direction function to set the gpio direction
 *
 * \details
 * This function is to set INPUT/OUTPUT gpio direction
 *
 * \param [in] gpio
 * GPIO pin number
 *
 * \param [in] direction
 * GPIO Direction INPUT/OUTPUT
 *
 * \return
 * Success - OBD2_LIB_SUCCESS
 * Error - \
 * 	1. E_GPIO_SET_DIR_OPEN - Failed to open the direction file \
 * 	2. E_GPIO_SET_DIR_WRT - Failed to write the Direction \
 * 	3. E_GPIO_SET_DIR_ACCESS - No GPIO Direction access
 */
int set_gpio_direction(int gpio, int direction)
{
	FILE * pFile;
	char *gpio_path = NULL;
	int ret;
	int error = 0;

	ret = asprintf(&gpio_path, "/sys/class/gpio/gpio%d/direction",gpio);
	if (ret < 0)
	{
		error = -ENOMEM;
		goto end;
	}

	if (access(gpio_path, F_OK ) == 0){
		pFile=fopen(gpio_path, "wr");
		if (pFile==NULL) {
			IOBD_DEBUG_LEVEL3("File open error \r\n");
			error = E_GPIO_SET_DIR_OPEN;
			goto end;
		}
		else{
			if (direction == INPUT)
				ret = fwrite("in",1,2,pFile);
			if (direction == OUTPUT)
				ret = fwrite("out",1,3,pFile);
			if (ret < 0 )
				error = E_GPIO_SET_DIR_WRT;
			else
				error = OBD2_LIB_SUCCESS;
			fclose(pFile);
		}	
	}
	else
		error = E_GPIO_SET_DIR_ACCESS;
end:
	if (gpio_path != NULL)
	{
		free(gpio_path);
		gpio_path = NULL;
	}

	return error;
}

/*!
 * \brief
 * set_gpio_value function to set the gpio value
 *
 * \details
 * This function is to set gpio value
 *
 * \param [in] gpio
 * GPIO pin number
 *
 * \param [in] value
 * GPIO value ON/OFF
 *
 * \return
 * Success - OBD2_LIB_SUCCESS
 * Error - \
 * 	1. E_GPIO_SET_OPEN - Failed to open the value file \
 * 	2. E_GPIO_SET_WRT - Failed to write the value \
 * 	3. E_GPIO_SET_VAL_ACCESS - No GPIO value file access
 */
int set_gpio_value(int gpio, int value)
{
	int ret;
	FILE *pFile = NULL;
	char *gpio_path = NULL;
	char *gpio_buf = NULL;
	int error = 0;

	ret = asprintf(&gpio_buf,"%d",value);
	if (ret < 0)
	{
		ret = -ENOMEM;
		goto end;
	}

	asprintf(&gpio_path, "/sys/class/gpio/gpio%d/value",gpio);
	if (ret < 0)
	{
		ret = -ENOMEM;
		goto end;
	}

	if (access(gpio_path, F_OK ) == 0)
	{
		// Do Nothing
	}
	else
	{
		gpio_export(gpio, OUTPUT);
	}

	if (access(gpio_path, F_OK ) == 0)
	{
		pFile=fopen(gpio_path, "wr");
		if (pFile==NULL) {
			IOBD_DEBUG_LEVEL3("File open error \r\n");
			error = E_GPIO_SET_OPEN;
			goto end;
		}

		ret = fwrite( gpio_buf,1,strlen( gpio_buf ),pFile);
		if (ret < 0 )
		{
			error = E_GPIO_SET_WRT;
			fclose(pFile);
			goto end;
		}
		else
			error = OBD2_LIB_SUCCESS;

		fclose(pFile);
	}
	else
	{
		error = E_GPIO_SET_VAL_ACCESS;
	}
end:
	if (gpio_path != NULL)
	{
		free(gpio_path);
		gpio_path = NULL;
	}
	if (gpio_buf != NULL)
	{
		free(gpio_buf);
		gpio_buf = NULL;
	}

	return error;
}

/*!
 * \brief
 * get_gpio function to get the gpio value
 *
 * \details
 * This function is to get the gpio value
 *
 * \param [in] gpio
 * GPIO pin number
 *
 * \param [in] value
 * GPIO value
 *
 * \return
 * Success - OBD2_LIB_SUCCESS
 * Error - \
 * 	1. E_GPIO_GET_OPEN - Error to open the get file \
 * 	2. E_GPIO_GET_RD - Failed to read the value \
 * 	3. E_GPIO_GET_VAL_ACCESS - No GPIO get value file access
 */
int get_gpio(int gpio, int * value)
{
	int ret =0;
	FILE * pFile;
	char gpio_buf[10] = {0};
	char *gpio_path = NULL;
	int error = 0;

	ret = asprintf(&gpio_path, "/sys/class/gpio/gpio%d/value",gpio);
	if (ret < 0)
	{
		error = -ENOMEM;
		goto end;
	}
	if (access(gpio_path, F_OK ) == 0)
	{
		pFile=fopen(gpio_path, "r");
		if (pFile==NULL) {
			IOBD_DEBUG_LEVEL3("File open error \r\n");
			error = E_GPIO_GET_OPEN;;
			goto end;
		}

		ret = fread(gpio_buf, 1, sizeof(gpio_buf), pFile);
		if (ret == strlen(gpio_buf))
		{
			*value = atoi(gpio_buf);
		}
		else{
			error = E_GPIO_GET_RD;
		}

		fclose(pFile);

	}
	else
	{
		error = E_GPIO_GET_VAL_ACCESS;
	}
end:
	if (gpio_path != NULL)
	{
		free(gpio_path);
		gpio_path = NULL;
	}

	return error;
}
