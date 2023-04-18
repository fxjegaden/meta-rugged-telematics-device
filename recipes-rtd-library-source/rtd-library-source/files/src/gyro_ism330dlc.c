/* headers */
#define _GNU_SOURCE
#include "lib_gyroscope.h"

static struct iio_channel_info *channels;
static int num_channels;
static gyroscope_api_priv gdata;
static int gyro_init_flag = 0;
static int gyro_event_no = -1;
int scan_size;
double gyro_sensor_sensitivity;
sem_t* gyro_lock;
static char *event_file = NULL;
void process_scan_gyro (char *, struct iio_channel_info *, int);
void print2byte_gyro (int, struct iio_channel_info *, int);
static int interrupt;
void gyro_sem_init (void) __attribute__ ((constructor));
void gyro_sem_deinit (void) __attribute__ ((destructor));

void gyro_sem_init (void)
{
	int ret;
	/*!< Init Semaphore for Gyro*/
	gyro_lock = sem_open("/gyro_lock", O_CREAT | EEXIST | O_EXCL, 0777, 1);
	if (gyro_lock == SEM_FAILED){
		ret = E_GYRO_SEM_INIT;
		CHK_ERR (ret, stderr, "Error: gyro_init() sem init");
	}else
		sem_unlink("/gyro_lock");
}

void gyro_sem_deinit (void)
{
	if (gyro_lock != SEM_FAILED)
		sem_destroy (gyro_lock);
}

void signal_gyro_h(int signo)
{
	interrupt = 1;
}

int gyro_init()
{
	int ret = OBD2_LIB_SUCCESS;
	signal(SIGUSR1, signal_gyro_h);
	interrupt = 0;	
	event_file = NULL;

	if (gyro_event_no == -1){
		gyro_event_no = get_gpio_event(GYRO_MAIN_PATH, GYRO_EVENT_NAME);
	}

	ret = asprintf (&event_file, "%s%d%s", GYRO_MAIN_PATH, gyro_event_no, GYRO_SUB_PATH_SCAN);
	if( ret < 0 )
	{
		ret = -ENOMEM;
		goto exit;
	}
#ifdef ENABLE_OBD_DEBUG
	printf("Gyro Event file :[%s]\n", event_file);
#endif
	ret = read_sysfs_posint(GYRO_INTERRUPT_X_AXIS, event_file);
	if (ret != ENABLE){
		/*!< Gyroscope x-axis interupt enable*/
		ret = write_sysfs_int (GYRO_INTERRUPT_X_AXIS, event_file, ENABLE);
		if (ret < OBD2_LIB_SUCCESS)
		{
			ret = E_GYRO_X_AXIS_INIT;
			CHK_ERR (ret, stderr, "Error: gyro_init() Enable GYRO_INTERRUPT_X_AXIS");
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

	ret = read_sysfs_posint(GYRO_INTERRUPT_Y_AXIS, event_file);
	if (ret != ENABLE){
		/*!< Gyroscope y-axis interupt enable*/
		ret = write_sysfs_int (GYRO_INTERRUPT_Y_AXIS, event_file, ENABLE);
		if (ret < OBD2_LIB_SUCCESS)
		{
			ret = E_GYRO_Y_AXIS_INIT;
			CHK_ERR (ret, stderr, "Error: gyro_init() Enable GYRO_INTERRUPT_Y_AXIS");
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

	ret = read_sysfs_posint(GYRO_INTERRUPT_Z_AXIS, event_file);
	if (ret != ENABLE){
		/*!< Gyroscope z-axis interupt enable*/
		ret = write_sysfs_int (GYRO_INTERRUPT_Z_AXIS, event_file, ENABLE);
		if (ret < OBD2_LIB_SUCCESS)
		{
			ret = E_GYRO_Z_AXIS_INIT;
			CHK_ERR (ret, stderr, "Error: gyro_init() Enable GYRO_INTERRUPT_Z_AXIS");
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

	if(event_file != NULL)
	{
		free(event_file);
		event_file = NULL;
	}
	ret = asprintf (&event_file, "%s%d",GYRO_MAIN_PATH,gyro_event_no);
	if( ret < 0 )
	{
		ret = -ENOMEM;
		goto exit;
	}
	ret = build_channel_array (event_file, &channels, &num_channels);	
	if (ret < OBD2_LIB_SUCCESS)
	{
		ret = E_GYRO_BULD_CHANNEL;
		CHK_ERR (ret, stderr, "Error: gyro_init() Get event_file");
		goto exit;
	}

	scan_size = size_from_channelarray (channels, num_channels);
	if(scan_size < OBD2_LIB_SUCCESS)
	{
		ret = E_GYRO_SCAN_SIZE;
		CHK_ERR (ret, stderr, "Error: gyro_init() Get channels");
		goto exit;
	}
	else
	{
		if(event_file != NULL)
		{
			free(event_file);
			event_file = NULL;
		}
		ret = asprintf (&event_file, "%s%d%s",GYRO_MAIN_PATH, gyro_event_no, GYRO_SUB_PATH_BUF);
		if(ret < 0)
		{
			ret = -ENOMEM;
			goto exit;
		}
		ret = read_sysfs_posint(GYRO_BUFFER_ENABLE, event_file);
		if (ret != ENABLE){
			/* Enable the buffer */
			ret = write_sysfs_int (GYRO_BUFFER_ENABLE, event_file, ENABLE);
			if (ret < OBD2_LIB_SUCCESS)
			{
				ret = E_GYRO_BUFFER_INIT;
				CHK_ERR (ret, stderr, "Error: gyro_init() Enable GYRO_BUFFER_ENABLE");
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

		ret = i2c_write(i2C_BUS_1, LSM6DSL_SLAVE_ADDR, LSM6DSL_INT1_CTRL, 0x02);
		if(ret == OBD2_LIB_FAILURE)
		{
			ret = OBD2_LIB_FAILURE;
			CHK_ERR (ret, stderr, "Error: gyro_init() Set LSM6DSL_INT1_CTRL");
		}

		ret = i2c_write(i2C_BUS_1,LSM6DSL_SLAVE_ADDR, LSM6DSL_CTRL_REG2, 0x10);
		if (ret != OBD2_LIB_SUCCESS)
		{
			ret = E_GYRO_I2C_REG_CONFIG;
			CHK_ERR (ret, stderr, "Error: gyro_init() Set LSM6DSL_CTRL_REG2");
			goto exit;
		}
	}

exit:
	if(event_file != NULL)
	{
		free(event_file);
		event_file = NULL;
	}
	return ret;
}

int set_gyro_sampling_frequency(uint8_t value)
{
	int ret = OBD2_LIB_SUCCESS;
	ret = i2c_write(i2C_BUS_1,LSM6DSL_SLAVE_ADDR,LSM6DSL_CTRL_REG2,value);
	return ret;
}

int set_gyro_low_pass_filter(uint8_t value)
{
	int ret = OBD2_LIB_SUCCESS;
	ret = i2c_write(i2C_BUS_1,LSM6DSL_SLAVE_ADDR,LSM6DSL_CTRL_REG6,value);
	return ret;
}

int gyro_deinit()
{
	int ret;
	int error = 0;
	event_file = NULL;

	ret = asprintf (&event_file, "%s%d%s",GYRO_MAIN_PATH, gyro_event_no, GYRO_SUB_PATH_BUF);
	if(ret < 0)
	{
		error = -ENOMEM;
		goto exit;
	}else{
		ret = OBD2_LIB_SUCCESS;
	}

	/* Disable the buffer */
	ret = write_sysfs_int (GYRO_BUFFER_ENABLE, event_file, DISABLE);
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

	ret = asprintf (&event_file, "%s%d%s", GYRO_MAIN_PATH, gyro_event_no, GYRO_SUB_PATH_SCAN);
	if(ret < 0)
	{
		error = -ENOMEM;
		goto exit;
	}else{
		ret = OBD2_LIB_SUCCESS;
	}

	ret = write_sysfs_int (GYRO_INTERRUPT_X_AXIS, event_file, DISABLE);
	if (ret < OBD2_LIB_SUCCESS)
	{
		error = OBD2_LIB_FAILURE;
		goto exit;		
	}else{
		ret = OBD2_LIB_SUCCESS;
	}

	ret = write_sysfs_int (GYRO_INTERRUPT_Y_AXIS, event_file, DISABLE);
	if (ret < OBD2_LIB_SUCCESS)
	{
		error = OBD2_LIB_FAILURE;
		goto exit;
	}else{
		ret = OBD2_LIB_SUCCESS;
	}

	ret = write_sysfs_int (GYRO_INTERRUPT_Z_AXIS, event_file, DISABLE);
	if (ret < OBD2_LIB_SUCCESS)
	{
		error = OBD2_LIB_FAILURE;
		goto exit;		
	}else{
		ret = OBD2_LIB_SUCCESS;
	}

	ret = i2c_write(i2C_BUS_1,LSM6DSL_SLAVE_ADDR, LSM6DSL_CTRL_REG2, 0x00);
	if (ret != OBD2_LIB_SUCCESS)
	{
		ret = E_GYRO_I2C_REG_CONFIG;
		CHK_ERR (ret, stderr, "Error: gyro_init() Set LSM6DSL_CTRL_REG2");
		goto exit;
	}

	ret = i2c_write(i2C_BUS_1,LSM6DSL_SLAVE_ADDR, LSM6DSL_INT1_CTRL, 0x00);
	if (ret != OBD2_LIB_SUCCESS)
	{
		ret = E_GYRO_I2C_REG_CONFIG;
		CHK_ERR (ret, stderr, "Error: gyro_init() Set LSM6DSL_CTRL_REG2");
		goto exit;
	}

	ret = i2c_write(i2C_BUS_1,LSM6DSL_SLAVE_ADDR, LSM6DSL_INT2_CTRL, 0x00);
	if (ret != OBD2_LIB_SUCCESS)
	{
		ret = E_GYRO_I2C_REG_CONFIG;
		CHK_ERR (ret, stderr, "Error: gyro_init() Set LSM6DSL_CTRL_REG2");
		goto exit;
	}

	gyro_event_no = -1;

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

	return error;
}

/* API to disable Gyro data buffer and 3 interrup axes
 * Should be called before Gyro and Acc power down
 * */
int gyro_disable()
{
	int ret = OBD2_LIB_SUCCESS;
	int error = OBD2_LIB_SUCCESS;
	event_file = NULL;

	if (gyro_event_no == -1)
		gyro_event_no = get_gpio_event(GYRO_MAIN_PATH, GYRO_EVENT_NAME);

	ret = asprintf (&event_file, "%s%d%s",GYRO_MAIN_PATH, gyro_event_no, GYRO_SUB_PATH_BUF);
	if( ret < 0)
	{
		error = -ENOMEM;
		goto exit;
	}
	ret = write_sysfs_int (GYRO_BUFFER_ENABLE, event_file, DISABLE);
	if (ret < OBD2_LIB_SUCCESS)
	{
		error = OBD2_LIB_FAILURE;
		goto exit;
	}

	if(event_file != NULL)
	{
		free(event_file);
		event_file = NULL;
	}
	ret = asprintf (&event_file, "%s%d%s", GYRO_MAIN_PATH, gyro_event_no, GYRO_SUB_PATH_SCAN);
	if( ret < 0)
	{
		error = -ENOMEM;
		goto exit;
	}
	ret = write_sysfs_int (GYRO_INTERRUPT_X_AXIS, event_file, DISABLE);
	if (ret < OBD2_LIB_SUCCESS)
	{
		error = OBD2_LIB_FAILURE;
		goto exit;
	}

	ret = write_sysfs_int (GYRO_INTERRUPT_Y_AXIS, event_file, DISABLE);
	if (ret < OBD2_LIB_SUCCESS)
	{
		error = OBD2_LIB_FAILURE;
		goto exit;
	}

	ret = write_sysfs_int (GYRO_INTERRUPT_Z_AXIS, event_file, DISABLE);
	if (ret < OBD2_LIB_SUCCESS)
	{
		error = OBD2_LIB_FAILURE;
		goto exit;
	}

exit:
	if(event_file != NULL)
	{
		free(event_file);
		event_file = NULL;
	}
	return error;

}
int gyro_enable()
{
	int ret = OBD2_LIB_SUCCESS;
	int error = OBD2_LIB_SUCCESS;
	event_file = NULL;

	if (gyro_event_no == -1)
		gyro_event_no = get_gpio_event(GYRO_MAIN_PATH, GYRO_EVENT_NAME);

	ret = asprintf (&event_file, "%s%d%s", GYRO_MAIN_PATH, gyro_event_no, GYRO_SUB_PATH_SCAN);
	if( ret < 0)
	{
		error = -ENOMEM;
		goto exit;
	}
	ret = write_sysfs_int (GYRO_INTERRUPT_X_AXIS, event_file, ENABLE);
	if (ret < OBD2_LIB_SUCCESS)
	{
		error = OBD2_LIB_FAILURE;
		goto exit;
	}
	ret = write_sysfs_int (GYRO_INTERRUPT_Y_AXIS, event_file, ENABLE);
	if (ret < OBD2_LIB_SUCCESS)
	{
		error = OBD2_LIB_FAILURE;
		goto exit;
	}
	ret = write_sysfs_int (GYRO_INTERRUPT_Z_AXIS, event_file, ENABLE);
	if (ret < OBD2_LIB_SUCCESS)
	{
		error = OBD2_LIB_FAILURE;
		goto exit;
	}

	if(event_file != NULL)
	{
		free(event_file);
		event_file = NULL;
	}
	ret = asprintf (&event_file, "%s%d%s",GYRO_MAIN_PATH, gyro_event_no, GYRO_SUB_PATH_BUF);
	if(ret < 0)
	{
		error = -ENOMEM;
		goto exit;
	}
	ret = write_sysfs_int (GYRO_BUFFER_ENABLE, event_file, ENABLE);
	if (ret < OBD2_LIB_SUCCESS)
	{
		error = OBD2_LIB_FAILURE;
		goto exit;
	}

exit:
	if(event_file != NULL)
	{
		free(event_file);
		event_file = NULL;
	}
	return error;

}
/* BUG ID 5276 */
int gyroscope_read (gyroscope_api_priv *g_data)
{
	int ret;
	char *buffer_access = NULL;
	unsigned long buf_len = 128;
	int fp, i = 0;
	int toread;
	int retval;
	fd_set rdfs;
	struct timeval timeout;
	ssize_t read_size;
	char *data = NULL;

	if (gyro_lock != SEM_FAILED)
		sem_wait (gyro_lock);

	/* timeout wait for 10ms */
	timeout.tv_sec = 0;
	timeout.tv_usec = 10000;

	data = malloc(scan_size*buf_len);
	if (!data) {
		ret = -ENOMEM;
		goto terminate;
	}else{
		ret = OBD2_LIB_SUCCESS;
	}


	ret = asprintf(&buffer_access, "/dev/iio:device%d", gyro_event_no);
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
	if(retval > 0) {
		read_size = read(fp, data, toread*scan_size);
		if (read_size == -EAGAIN) {
			//IOBD_DEBUG_LEVEL3("nothing available\n");
		}
		for (i = 0; i < read_size/scan_size; i++){
			process_scan_gyro (data + scan_size * i, channels, num_channels);
			g_data -> x = gdata.x;
			g_data -> y = gdata.y;
			g_data -> z = gdata.z;
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
	if (gyro_lock != SEM_FAILED)
		sem_post (gyro_lock);

	return ret;
}

/**
 * process_scan() - print out the values in SI units
 * @data:	pointer to the start of the scan
 * @channels:	information about the channels. Note \
 * 	size_from_channelarray must have been called first to fill the \
 * 	location offsets. \
 * @num_channels: number of channels
 **/
void process_scan_gyro (char *data, struct iio_channel_info *channels, int num_channels)
{
	int k;
	int channel_num;

	for (k = 0; k < num_channels; k++){
		switch (channels[k].bytes) {
			/* only a few cases implemented so far */
			case 2:
				channel_num = k;
				print2byte_gyro (*(uint16_t *)(data + channels[k].location),
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
					int64_t val = *(int64_t *)
						(data +
						 channels[k].location);
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

void print2byte_gyro (int input, struct iio_channel_info *info, int channel_num)
{
	float final_val;
	/* First swap if incorrect endian */
	//printf("print2byte_acc() input %d\n",input);
	if (info->be)
		input = be16toh((uint16_t)input);
	else
		input = le16toh((uint16_t)input);

	/*
	 * Shift before conversion to avoid sign extension
	 * of left aligned data
	 */
	input = input >> info->shift;
	if (info->is_signed) {
		int16_t val = input;
		val &= (1 << info->bits_used) - 1;
		val = (int16_t)(val << (16 - info->bits_used)) >>
			(16 - info->bits_used);
		final_val = (float)(val + info->offset)*info->scale;
		//final_val = (float)(val * gyro_sensor_sensitivity)/1000;
		//printf("%05f ", ((float)val + info->offset)*info->scale);
	} else {
		uint16_t val = input;
		val &= (1 << info->bits_used) - 1;
		//printf("%05f ", ((float)val + info->offset)*info->scale);
		final_val = ((float)val + info->offset)*info->scale;
		//final_val = (float)(val * gyro_sensor_sensitivity)/1000;
	}
	if(channel_num == 0)
		gdata.x = (double) final_val;
	if(channel_num == 1)
		gdata.y = (double) final_val;
	if(channel_num == 2){
		gdata.z = (double) final_val;
		// printf("x: %05f, y: %05f, z: %05f, acc: %05f\r\n", adata.x, adata.y, adata.z, adata.acc);
	}
}
