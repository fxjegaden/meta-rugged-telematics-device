/* headers */
#define _GNU_SOURCE
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <linux/types.h>
#include <string.h>
#include <poll.h>
#include <endian.h>
#include <getopt.h>
#include <inttypes.h>
#include <math.h>
#include <semaphore.h>
#include "iio_utils.h"
#include "obd2lib.h"
#include "lib_magnetometer.h"
#include <signal.h>
#include "init.h"
#include "thread.h"
#include <malloc.h>
#include "error_nos.h"

static magnetometer_api_priv mdata;
static int init_state = 0;
static struct iio_channel_info *channels;
static int num_channels;
static int mag_event_no = 0;
int scan_size;
pthread_t mag_thread_id;
sem_t* mag_lock;
static char *event_file = NULL;
static int interrupt;
void process_scan_mag(char *, struct iio_channel_info *, int);
void print2byte_mag(int, struct iio_channel_info *, int);
int mag_sem_init (void) __attribute__ ((constructor));
void mag_sem_deinit (void) __attribute__ ((destructor));

int mag_sem_init (void)
{
	int ret = OBD2_LIB_SUCCESS;
	/*!< Init Semaphore for accelerometer*/
	mag_lock = sem_open("/mag_lock", O_CREAT | EEXIST | O_EXCL, 0777, 1);
	if (mag_lock == SEM_FAILED){
		ret = OBD2_LIB_FAILURE;
		CHK_ERR (ret, stderr, "Error: acc_init() sem_init");
	}else
		sem_unlink("/mag_lock");
	return ret;

}

void mag_sem_deinit (void)
{
	if (mag_lock != SEM_FAILED)
		sem_destroy(mag_lock);
}

int mag_init()
{
	int ret = OBD2_LIB_SUCCESS;
	int error = OBD2_LIB_SUCCESS;
	interrupt = 0;
	event_file = NULL;

	mag_event_no = get_gpio_event(MAG_MAIN_PATH, MAG_EVENT_NAME);
	printf("MAG EVENT NUMBER IS %d\n",mag_event_no);
	if(mag_event_no > OBD2_LIB_FAILURE)
	{
		ret = asprintf (&event_file, "%s%d%s", MAG_MAIN_PATH, mag_event_no, MAG_SUB_PATH_SCAN);
		if(ret < 0)
		{
			ret = -ENOMEM;
			goto exit;
		}

		/*!< magnetometer x-axis interrupt enable*/
		ret = read_sysfs_posint(MAG_INTERRUPT_X_AXIS, event_file);
		if (ret != ENABLE){
			ret = write_sysfs_int (MAG_INTERRUPT_X_AXIS, event_file, ENABLE);
			if (ret < OBD2_LIB_SUCCESS)
			{
				init_state = 0;
				error = OBD2_LIB_FAILURE;
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

		/*!< magnetometer y-axis interrupt enable*/
		ret = read_sysfs_posint(MAG_INTERRUPT_Y_AXIS, event_file);
		if (ret != ENABLE){
			ret = write_sysfs_int (MAG_INTERRUPT_Y_AXIS, event_file, ENABLE);
			if (ret < OBD2_LIB_SUCCESS)
			{
				init_state = 0;
				error = OBD2_LIB_FAILURE;
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

		/*!< magnetometer z-axis interrupt enable*/
		ret = read_sysfs_posint(MAG_INTERRUPT_Z_AXIS, event_file);
		if (ret != ENABLE){
			ret = write_sysfs_int (MAG_INTERRUPT_Z_AXIS, event_file, ENABLE);
			if (ret < OBD2_LIB_SUCCESS)
			{
				init_state = 0;
				error = OBD2_LIB_FAILURE;
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

		ret = asprintf (&event_file, "%s%d",MAG_MAIN_PATH,mag_event_no);
		if(ret < 0)
		{
			ret = -ENOMEM;
			goto exit;
		}

		ret = build_channel_array (event_file, &channels, &num_channels);
		if (ret < OBD2_LIB_SUCCESS)
		{
			init_state = 0;
			ret = E_MAG_BUILD_CHANNEL;
			goto exit;
		}

		scan_size = size_from_channelarray (channels, num_channels);
		if(scan_size < OBD2_LIB_SUCCESS)
		{
			init_state = 0;
			ret = E_MAG_SCAN_SIZE;
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

			ret = asprintf (&event_file, "%s%d%s",MAG_MAIN_PATH, mag_event_no, MAG_SUB_PATH_BUF);
			if (ret < 0)
			{
				ret = -ENOMEM;
				goto exit;
			}

			if(read_sysfs_posint(MAG_BUFFER_ENABLE, event_file) == DISABLE)
			{
				ret = write_sysfs_int (MAG_BUFFER_ENABLE, event_file, ENABLE);
				if (ret < OBD2_LIB_SUCCESS)
				{
					init_state = 0;
					error = OBD2_LIB_FAILURE;
					goto exit;
				}
				else
				{
					ret = i2c_write(i2C_BUS_0,IIS2MDC_SLAVE_ADDR,IIS2MDC_CFG_REG_A,0x8C);
				}
			}
			else
			{
				ret = i2c_write(i2C_BUS_0,IIS2MDC_SLAVE_ADDR,IIS2MDC_CFG_REG_A,0x8C);
			}
		}

exit:
		if(event_file != NULL)
		{
			free(event_file);
			event_file = NULL;
		}
	}

	return ret;
}

int set_mag_sampling_frequency(uint8_t value)
{
	int ret = OBD2_LIB_SUCCESS;
	ret = i2c_write(i2C_BUS_0,IIS2MDC_SLAVE_ADDR,IIS2MDC_CFG_REG_A,value);
	return ret;
}

int mag_deinit()
{
	int ret = OBD2_LIB_SUCCESS;
	int error = OBD2_LIB_SUCCESS;
	uint8_t value = OBD2_LIB_SUCCESS;
	event_file = NULL;

	ret = asprintf (&event_file, "%s%d%s",MAG_MAIN_PATH, mag_event_no, MAG_SUB_PATH_BUF);
	if(ret < 0)
	{
		error = -ENOMEM;
		goto exit;
	}

	if(read_sysfs_posint(MAG_BUFFER_ENABLE, event_file) == ENABLE)
	{

		/* Magnetometer in IDLE MODE for Accelerometer Wake up to work properly */
		error = i2c_write(i2C_BUS_0,IIS2MDC_SLAVE_ADDR,IIS2MDC_CFG_REG_A,0x03);
		sleep(1);
		if( error == OBD2_LIB_SUCCESS)
		{
			ret = write_sysfs_int (MAG_BUFFER_ENABLE, event_file, DISABLE);
			if (ret < OBD2_LIB_SUCCESS)
			{
				error = OBD2_LIB_FAILURE;
				goto exit;
			}
		}
	}

	if (event_file != NULL)
	{
		free(event_file);
		event_file = NULL;
	}

	ret = asprintf (&event_file, "%s%d%s", MAG_MAIN_PATH, mag_event_no, MAG_SUB_PATH_SCAN);
	if(ret < 0)
	{
		error = -ENOMEM;
		goto exit;
	}

	/*!< magnetometer x-axis interrupt disable*/
	ret = read_sysfs_posint(MAG_INTERRUPT_X_AXIS, event_file);
	if (ret != DISABLE){
		ret = write_sysfs_int (MAG_INTERRUPT_X_AXIS, event_file, DISABLE);
		if (ret < OBD2_LIB_SUCCESS)
		{
			error = OBD2_LIB_FAILURE;
			goto exit;
		}
	}

	/*!< magnetometer y-axis interrupt disable*/
	ret = read_sysfs_posint(MAG_INTERRUPT_Y_AXIS, event_file);
	if (ret != DISABLE){
		ret = write_sysfs_int (MAG_INTERRUPT_Y_AXIS, event_file, DISABLE);
		if (ret < OBD2_LIB_SUCCESS)
		{
			init_state = 0;
			error = OBD2_LIB_FAILURE;
			goto exit;
		}
	}

	/*!< magnetometer z-axis interrupt disable*/
	ret = read_sysfs_posint(MAG_INTERRUPT_Z_AXIS, event_file);
	if (ret != DISABLE){
		ret = write_sysfs_int (MAG_INTERRUPT_Z_AXIS, event_file, DISABLE);
		if (ret < OBD2_LIB_SUCCESS)
		{
			error = OBD2_LIB_FAILURE;
			goto exit;
		}
	}

exit:
	if(event_file != NULL){
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

int magnetometer_read(magnetometer_api_priv *mag)
{
	int ret = OBD2_LIB_SUCCESS;
	char *buffer_mag = NULL;
	unsigned long buf_len = 128;
	int fp, i = 0;
	int toread;
	int retval;
	fd_set rdfs;
	struct timeval timeout;
	ssize_t read_size;
	char *data = NULL;

	if (mag_lock != SEM_FAILED)
		sem_wait (mag_lock);

	/* timeout wait for 20ms */
	timeout.tv_sec = 0;
	timeout.tv_usec = 20000;

	data = malloc(scan_size*buf_len);
	if (!data) {
		ret = -ENOMEM;
		goto terminate;
	}
	else
	{
		ret = asprintf(&buffer_mag, "/dev/iio:device%d", mag_event_no);
		if (ret < 0) {
			ret = -ENOMEM;
			goto terminate;
		}
		else
		{
			toread = buf_len;

			/* Attempt to open non blocking the magess dev */
			fp = open(buffer_mag, O_RDONLY | O_NONBLOCK);
			if (fp <= 0)
			{ /* If it isn't there make the node */
				IOBD_DEBUG_LEVEL3("Failed to open %s\n", buffer_mag);
				ret = -errno;
				goto terminate;
			}
			else
			{
				FD_ZERO(&rdfs);
				FD_SET(fp, &rdfs);
				retval = select(fp + 1 , &rdfs, NULL, NULL, &timeout);
				if(retval > 0)
				{
					read_size = read(fp, data, toread*scan_size);
					if (read_size == -EAGAIN)
					{
						printf("nothing available\n");
					}
					for (i = 0; i < read_size/scan_size; i++)
					{
						process_scan_mag (data + scan_size * i, channels, num_channels);
						mag -> x = mdata.x;
						mag -> y = mdata.y;
						mag -> z = mdata.z;
					}
				}
				else
				{
					ret = -errno;
				}
			}
		}
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
	if( buffer_mag != NULL)
	{
		free( buffer_mag );
		buffer_mag = NULL;
	}
	if (mag_lock != SEM_FAILED)
		sem_post (mag_lock);

	return ret;
	/* NaND : adata must be of type magelerometer_api_priv. dont use char buffer */
}

/**
 * process_scan() - print out the values in SI units
 * @data:	pointer to the start of the scan
 * @channels:	information about the channels. Note
 * 		size_from_channelarray must have been called first to fill the
 * 		location offsets.
 * @num_channels: number of channels
 **/
void process_scan_mag(char *data, struct iio_channel_info *channels, int num_channels)
{
	int k;
	int channel_num;

	for (k = 0; k < num_channels; k++){
		switch (channels[k].bytes) {
			/* only a few cases implemented so far */
			case 2:
				channel_num = k;
				print2byte_mag (*(uint16_t *)(data + channels[k].location),
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

void print2byte_mag(int input, struct iio_channel_info *info, int channel_num)
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
		final_val = (float)(val);
	}
	else
	{
		uint16_t val = input;
		val &= (1 << info->bits_used) - 1;
		final_val = (float)(val);
	}
	if(channel_num == 0)
		mdata.x = (double) final_val;
	if(channel_num == 1)
		mdata.y = (double) final_val;
	if(channel_num == 2)
		mdata.z = (double) final_val;
}
