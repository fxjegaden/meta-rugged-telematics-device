/* headers */
#define _GNU_SOURCE
#include "lib_accelerometer.h"

double acc_sensor_sensitivity = 0;
static int init_state = 0;
static struct iio_channel_info *channels;
static int num_channels;
static accelerometer_api_priv adata;
static accelerometer_api_priv prev_adata;
static int acc_init_flag = 0;
static int acc_event_no = -1;
int scan_size;
sem_t* acc_lock;
static char *event_file = NULL;
static int interrupt;
int acc_init_success;

void acc_sem_init (void)
{
	int ret;
	acc_init_flag = 0;

	/*!< Init Semaphore for accelerometer*/
	acc_lock = sem_open("/acc_lock", O_CREAT | EEXIST | O_EXCL, 0777, 1);
	if (acc_lock == SEM_FAILED){
		ret = E_ACCELEROMETER_SEM_INIT;
		CHK_ERR (ret, stderr, "Error: acc_init() sem_init");
	}else
		sem_unlink("/acc_lock");

}

void acc_sem_deinit (void)
{
	if (acc_lock != SEM_FAILED)
		sem_destroy(acc_lock);
}

int acc_init()
{
	acc_init_success = 0;
	int ret = OBD2_LIB_SUCCESS;
	acc_init_flag = 0;
	event_file = NULL;

	if (acc_event_no == -1)
		acc_event_no = get_gpio_event(ACC_MAIN_PATH, ACC_EVENT_NAME);

	ret = asprintf (&event_file, "%s%d%s", ACC_MAIN_PATH, acc_event_no, ACC_SUB_PATH_SCAN);
	if(ret < 0)
	{
		ret = -ENOMEM;
		goto exit;
	}

	ret = read_sysfs_posint(ACC_INTERRUPT_X_AXIS,event_file);
	if (ret != ENABLE){
		/*!< Acclerometer x-axis interupt enable*/
		ret = write_sysfs_int (ACC_INTERRUPT_X_AXIS, event_file, ENABLE);
		if (ret < OBD2_LIB_SUCCESS)
		{
			init_state = 0;
			ret = E_ACCELEROMTER_X_AXIS_INIT;
			CHK_ERR (ret, stderr, "Error: acc_init() Enable ACC_INTERRUPT_X_AXIS");
			goto exit;
		}
	}
	else if(ret < 0)
	{
		goto exit;
	}
	else
	{
	}

	ret = read_sysfs_posint(ACC_INTERRUPT_Y_AXIS,event_file);
	if (ret != ENABLE){
		/*!< Acclerometer y-axis interupt enable*/
		ret = write_sysfs_int (ACC_INTERRUPT_Y_AXIS, event_file, ENABLE);
		if (ret < OBD2_LIB_SUCCESS)
		{
			init_state = 0;
			ret = E_ACCELEROMTER_Y_AXIS_INIT;
			CHK_ERR (ret, stderr, "Error: acc_init() Enable ACC_INTERRUPT_Y_AXIS");
			goto exit;
		}
	}
	else if(ret < 0)
	{
		goto exit;
	}
	else
	{
	}

	ret = read_sysfs_posint(ACC_INTERRUPT_Z_AXIS,event_file);
	if (ret != ENABLE){
		/*!< Acclerometer z-axis interupt enable*/
		ret = write_sysfs_int (ACC_INTERRUPT_Z_AXIS, event_file, ENABLE);
		if (ret < OBD2_LIB_SUCCESS)
		{
			init_state = 0;
			ret = E_ACCELEROMTER_Z_AXIS_INIT;
			CHK_ERR (ret, stderr, "Error: acc_init() Enable ACC_INTERRUPT_Z_AXIS");
			goto exit;
		}
	}
	else if(ret < 0)
	{
		goto exit;
	}
	else
	{
	}

	if(event_file != NULL)
	{
		free(event_file);
		event_file = NULL;
	}

	ret = asprintf (&event_file, "%s%d",ACC_MAIN_PATH,acc_event_no);
	if(ret < 0)
	{
		ret = -ENOMEM;
		goto exit;
	}

	ret = build_channel_array (event_file, &channels, &num_channels);	
	if (ret < OBD2_LIB_SUCCESS)
	{
		init_state = 0;
		ret = E_ACCELEROMETER_BULD_CHANNEL;
		CHK_ERR (ret, stderr, "Error: acc_init() Get event_file");
		goto exit;
	}

	scan_size = size_from_channelarray (channels, num_channels);
	if(scan_size < OBD2_LIB_SUCCESS)
	{
		init_state = 0;
		ret = E_ACCELEROMETER_SCAN_SIZE;
		CHK_ERR (ret, stderr, "Error: acc_init() Get channels");
		goto exit;
	}	
	else
	{
		if(event_file != NULL)
		{
			free(event_file);
			event_file = NULL;
		}

		ret = asprintf (&event_file, "%s%d%s",ACC_MAIN_PATH, acc_event_no, ACC_SUB_PATH_BUF);
		if(ret < 0)
		{
			ret = -ENOMEM;
			goto exit;
		}

		ret = read_sysfs_posint(ACC_BUFFER_ENABLE,event_file);
		if (ret != ENABLE){
			ret = write_sysfs_int (ACC_BUFFER_ENABLE, event_file, ENABLE);
			if (ret < OBD2_LIB_SUCCESS)
			{
				init_state = 0;
				ret = E_ACCELEROMTER_BUFFER_INIT;
				CHK_ERR (ret, stderr, "Error: acc_init() Enable ACC_BUFFER_ENABLE");
				goto exit;
			}
		}
		else if (ret < 0)
		{
			goto exit;
		}
		else
		{
		}

		ret = i2c_write(ACC_I2C_NO, ACC_SLAVE_ADDR, SENSOR_CTRL_REG1, 0x70);
		if(ret < OBD2_LIB_SUCCESS)
		{
			ret = E_ACCELEROMETER_I2C_REG_CONFIG;
			CHK_ERR (ret, stderr, "Error: acc_init() Set SENSOR_CTRL_REG1");
		}	

		ret = i2c_write(ACC_I2C_NO, ACC_SLAVE_ADDR, SENSOR_CTRL_REG8, 0x80);
		if(ret < OBD2_LIB_SUCCESS)
		{
			ret = E_ACCELEROMETER_I2C_REG_CONFIG;
			CHK_ERR (ret, stderr, "Error: acc_init() Set SENSOR_CTRL_REG8,");
		}

		ret = i2c_write(ACC_I2C_NO, ACC_SLAVE_ADDR, ACC_THRESHOLD_REG, 0x81);
		if(ret < OBD2_LIB_SUCCESS)
		{
			ret = E_ACCELEROMETER_I2C_REG_CONFIG;
			CHK_ERR (ret, stderr, "Error: acc_init() Set ACC_THRESHOLD_REG");
		}

		ret = i2c_write(ACC_I2C_NO, ACC_SLAVE_ADDR, SENSOR_INT_DUR2, 0x06);
		if(ret < OBD2_LIB_SUCCESS)
		{
			ret = E_ACCELEROMETER_I2C_REG_CONFIG;
			CHK_ERR (ret, stderr, "Error: acc_init() Set SENSOR_INT_DUR2");
		}

		ret = i2c_write(ACC_I2C_NO, ACC_SLAVE_ADDR, SENSOR_WAKE_UP_THS, 0x00);
		if(ret < OBD2_LIB_SUCCESS)
		{
			ret = E_ACCELEROMETER_I2C_REG_CONFIG;
			CHK_ERR (ret, stderr, "Error: acc_init() Set SENSOR_WAKE_UP_THS");
		}

		ret = i2c_write(ACC_I2C_NO, ACC_SLAVE_ADDR, SENSOR_MD1_CFG, 0x40);
		if(ret < OBD2_LIB_SUCCESS)
		{
			ret = E_ACCELEROMETER_I2C_REG_CONFIG;
			CHK_ERR (ret, stderr, "Error: acc_init() Set SENSOR_MD1_CFG");
		}

		/* Enable data ready interrupt */
		ret = i2c_write(ACC_I2C_NO, LSM6DSL_SLAVE_ADDR, LSM6DSL_INT1_CTRL, 0x01);
		if(ret == OBD2_LIB_FAILURE)
		{
			ret = OBD2_LIB_FAILURE;
			CHK_ERR (ret, stderr, "Error: acc_init() Set LSM6DSL_INT1_CTRL");
		}

		/*Disable acc interrupt */
		ret = i2c_write(ACC_I2C_NO, LSM6DSL_SLAVE_ADDR, LSM6DSL_TAP_CFG, 0x0e);
		if(ret == OBD2_LIB_FAILURE)
		{
			ret = OBD2_LIB_FAILURE;
			CHK_ERR (ret, stderr, "Error: acc_init() Set LSM6DSL_TAP_CFG");
		}

		set_acc_sampling_frequency(0xB0);

		if (acc_init_flag == 1)
		{
			acc_init_flag = 1;
			init_state = 1;
		}

		ret = acc_sensitivity();
	}

exit:
	if(event_file != NULL)
	{
		free(event_file);
		event_file = NULL;
	}

	if( ret == OBD2_LIB_SUCCESS )
		acc_init_success = 1;

	return ret;
}

int set_acc_sampling_frequency(uint8_t value)
{
	int ret = OBD2_LIB_SUCCESS;
	ret = i2c_write(ACC_I2C_NO,LSM6DSL_SLAVE_ADDR, LSM6DSL_CTRL_REG1, value);
	acc_sensitivity();
	return ret;
}

int set_acc_low_pass_filter(uint8_t value)
{
	int ret = OBD2_LIB_SUCCESS;
	ret = i2c_write(ACC_I2C_NO,LSM6DSL_SLAVE_ADDR,LSM6DSL_CTRL_REG8,value);
	return ret;
}

int set_acc_wakeup_threshold(uint8_t value)
{
	int ret = OBD2_LIB_SUCCESS;
	ret = i2c_write(ACC_I2C_NO,LSM6DSL_SLAVE_ADDR,LSM6DSL_ACC_THRESHOLD_REG,value);
	if(ret < OBD2_LIB_SUCCESS)
	{
		ret = E_ACCELEROMETER_I2C_REG_CONFIG;
		CHK_ERR (ret, stderr, "Error: acc_init() Set SENSOR_WAKE_UP_THS");
	}
	return ret;
}

int acc_deinit()
{
	int ret;
	int error = 0;
	event_file = NULL;

	if( !acc_init_success )
	{
		/*
		 * Initializing the Accelerometer before the deinitialisation \
		 * will avoid the Kernel Panic after sleep wakeup. \
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

	if( acc_init_success )
	{
		ret = asprintf (&event_file, "%s%d%s",ACC_MAIN_PATH, acc_event_no, ACC_SUB_PATH_BUF);
		if(ret < 0)
		{
			error = -ENOMEM;
			goto exit;
		}else{
			ret = OBD2_LIB_SUCCESS;
		}

		/* Disable the buffer */
		ret = write_sysfs_int (ACC_BUFFER_ENABLE, event_file, DISABLE);
		if (ret < OBD2_LIB_SUCCESS)
		{
			error = OBD2_LIB_FAILURE;
			goto exit;
		}else{
			ret = OBD2_LIB_SUCCESS;
		}

		if (event_file != NULL)
		{
			free(event_file);
			event_file = NULL;
		}else{
			ret = OBD2_LIB_SUCCESS;
		}

		ret = asprintf (&event_file, "%s%d%s", ACC_MAIN_PATH, acc_event_no, ACC_SUB_PATH_SCAN);
		if(ret < 0)
		{
			error = -ENOMEM;
			goto exit;
		}else{
			ret = OBD2_LIB_SUCCESS;
		}

		ret = write_sysfs_int (ACC_INTERRUPT_X_AXIS, event_file, DISABLE);
		if (ret < OBD2_LIB_SUCCESS)
		{
			error = OBD2_LIB_FAILURE;
			goto exit;
		}else{
			ret = OBD2_LIB_SUCCESS;
		}

		ret = write_sysfs_int (ACC_INTERRUPT_Y_AXIS, event_file, DISABLE);
		if (ret < OBD2_LIB_SUCCESS)
		{
			error = OBD2_LIB_FAILURE;
			goto exit;
		}else{
			ret = OBD2_LIB_SUCCESS;
		}

		ret = write_sysfs_int (ACC_INTERRUPT_Z_AXIS, event_file, DISABLE);
		if (ret < OBD2_LIB_SUCCESS)
		{
			error = OBD2_LIB_FAILURE;
			goto exit;
		}else{
			ret = OBD2_LIB_SUCCESS;
		}

		acc_event_no = -1;
	}

exit:
	if(event_file != NULL)
	{
		free(event_file);
		event_file = NULL;
	}

	if( channels != NULL )
	{
		free( channels );
		channels = NULL;
	}

	if( ret == OBD2_LIB_SUCCESS )
		acc_init_success = 0;

	return error;
}

/* BUG ID 5276 */
int accelerometer_read (accelerometer_api_priv *acc)
{
	int ret;
	char *buffer_access = NULL;
	unsigned long buf_len = 128;
	int fp, i;
	int toread;
	int retval;
	fd_set rdfs;
	struct timeval timeout;
	ssize_t read_size;
	char *data = NULL;

	if (acc_lock != SEM_FAILED)
		sem_wait (acc_lock);

	/* timeout wait for 20ms */
	timeout.tv_sec = 0;
	timeout.tv_usec = 20000;

	data = malloc(scan_size*buf_len);
	if (!data) {
		ret = -ENOMEM;
		goto terminate;
	}

	ret = asprintf(&buffer_access, "/dev/iio:device%d", acc_event_no);
	if (ret < 0) {
		ret = -ENOMEM;
		goto terminate;
	}else{
		ret = OBD2_LIB_SUCCESS;
	}


	toread = buf_len;

	/* Attempt to open non blocking the access dev */
	fp = open(buffer_access, O_RDONLY | O_NONBLOCK);
	if (fp == -1) { /* If it isn't there make the node */
		ret = -errno;
		goto terminate;
	}else{
		ret = OBD2_LIB_SUCCESS;
	}

	FD_ZERO(&rdfs);
	FD_SET(fp, &rdfs);

	retval = select(fp + 1 , &rdfs, NULL, NULL, &timeout);
	if(retval > 0)
	{
		read_size = read(fp, data, toread*scan_size);
		if (read_size == -EAGAIN)
		{
			// Do Nothing
		}
		for (i = 0; i < read_size/scan_size; i++)
		{
			process_scan_acc (data + scan_size * i, channels, num_channels);
			acc -> x = adata.x;
			acc -> y = adata.y;
			acc -> z = adata.z;
			acc -> acc = adata.acc;
		}
	}
	else
	{
		ret = -errno;
	}

	if( fp )
	{
		close(fp);
		fp = 0;
	}

terminate:
	if( data != NULL )
	{
		free( data );
		data = NULL;
	}
	if( buffer_access != NULL )
	{
		free( buffer_access );
		buffer_access = NULL;
	}

	if (acc_lock != SEM_FAILED)
		sem_post (acc_lock);

	return ret;
}

/**
 * process_scan() - print out the values in SI units
 * @data:	pointer to the start of the scan
 * @channels:	information about the channels. Note \
 * 		size_from_channelarray must have been called first to fill the \
 * 		location offsets.
 * @num_channels:number of channels
 **/
void process_scan_acc(char *data, struct iio_channel_info *channels, int num_channels)
{
	int k;
	int channel_num;

	for (k = 0; k < num_channels; k++){
		switch (channels[k].bytes) {
			/* only a few cases implemented so far */
			case 2:
				channel_num = k;
				print2byte_acc (*(uint16_t *)(data + channels[k].location),
						&channels[k], channel_num);
				break;
			case 4:
				if (!channels[k].is_signed) {
					uint32_t val = *(uint32_t *)
						(data + channels[k].location);
					printf("%05f ", ((float)val +
								channels[k].offset)*
							channels[k].scale);

				}
				break;
			case 8:
				if (channels[k].is_signed) {
					int64_t val = *(int64_t *)(data + channels[k].location);
					if ((val >> channels[k].bits_used) & 1)
						val = (val & channels[k].mask) |
							~channels[k].mask;
					/* special case for timestamp */
					if (channels[k].scale == 1.0f &&
							channels[k].offset == 0.0f)
						printf("%" PRId64 " ", val);
					else
						printf("%05f ", ((float)val + channels[k].offset)*channels[k].scale);
				}
				break;
			default:
				break;
		}
	}
}

void print2byte_acc(int input, struct iio_channel_info *info, int channel_num)
{
	float final_val;
	/* First swap if incorrect endian */
	if (info->be)
		input = be16toh((uint16_t)input);
	else
		input = le16toh((uint16_t)input);

	/*
	 * Shift before conversion to avoid sign extension
	 * of left aligned data
	 */
	input = input >> info->shift;
	if (info->is_signed)
	{
		int16_t val = input;
		val &= (1 << info->bits_used) - 1;
		val = (int16_t)(val << (16 - info->bits_used)) >>
			(16 - info->bits_used);
		final_val = (float)((val * acc_sensor_sensitivity) * 0.0098);
	}
	else
	{
		uint16_t val = input;
		val &= (1 << info->bits_used) - 1;
		final_val = (float)((val * acc_sensor_sensitivity) * 0.0098);

	}
	if(channel_num == 0)
		adata.x = (double) final_val;
	if(channel_num == 1)
		adata.y = (double) final_val;
	if(channel_num == 2)
		adata.z = (double) final_val;
}

int acc_sensitivity()
{
	uint8_t value;
	int acc_sen;
	int ret = OBD2_LIB_SUCCESS;
	ret = i2c_read(ACC_I2C_NO,LSM6DSL_SLAVE_ADDR, LSM6DSL_CTRL_REG1,&value);
	if(ret == OBD2_LIB_SUCCESS)
	{
		acc_sen = value & 0x0F;
		if(acc_sen == 0 || acc_sen == 1 || acc_sen == 2 || acc_sen == 3)
		{
			acc_sensor_sensitivity = 0.061;
		}
		if(acc_sen == 4 || acc_sen == 5 || acc_sen == 6 || acc_sen == 7)
		{
			acc_sensor_sensitivity = 0.488;
		}
		if(acc_sen == 8 || acc_sen == 9 || acc_sen == 10 || acc_sen == 11)
		{
			acc_sensor_sensitivity = 0.122;
		}
		if(acc_sen == 12 || acc_sen == 13 || acc_sen == 14 || acc_sen == 15)
		{
			acc_sensor_sensitivity = 0.244;
		}
	}

	return ret;	
}

/*
 * API		: int acc_temp_read( float_t *temperature )
 * Description	: API for reading the ambient board temperature. \
 * Arguments	: temperature - An empty float_t pointer to store the temperature.
 * Return Value	: 0 on Success & Error Code on Failure
 * */
int acc_temp_read( float_t *temperature )
{
	uint16_t temp = 0;
	float_t f_temp = 0;
	uint8_t buff[2];
	int ret = OBD2_LIB_FAILURE;

	ret = i2c_read(ACC_I2C_NO, LSM6DSL_SLAVE_ADDR, LSM6DSM_OUT_TEMP_H, &buff[0]);
	if(ret == OBD2_LIB_SUCCESS)
	{
		ret = i2c_read(ACC_I2C_NO, LSM6DSL_SLAVE_ADDR, LSM6DSM_OUT_TEMP_L, &buff[1]);
		if(ret == OBD2_LIB_SUCCESS)
		{
			temp = (buff[0]);
			temp = (temp << 8 );
			temp = temp | buff[1];
			if(temp <= 0x7FFF)
			{
				f_temp = ((float_t)(temp / 256.0) + TEMPERATURE_OFFSET);
			}
			else
			{
				temp = ~(temp - 1);
				f_temp = (((float_t)temp / 256.0) - TEMPERATURE_OFFSET);
			}
			*temperature = f_temp;
		}
	}

	return ret;
}
