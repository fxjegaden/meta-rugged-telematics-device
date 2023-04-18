#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "common.h"
#include "gyroscope.h"
#include "accelerometer.h"
#include "magnetometer.h"
#include "gps.h"
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <stdint.h>

void INThandler(int);

int main(int argc, char *argv[])
{
	int rc = 0;
	signal(SIGINT, INThandler);
	signal(SIGSTOP, INThandler);
	accelerometer_api_priv g_adata;
	gyroscope_api_priv g_data;
	magnetometer_api_priv mag;
	int scale = 0;
	uint8_t value = 0;

	rc = acc_init();
	if (rc != 0)
	{
		printf ("acc_on failed");
	}

	value = 0x80;
	set_acc_low_pass_filter(value);
	value = 0x81;
	set_acc_wakeup_threshold(value);

	acc_init();
	gyro_init();
	mag_init();
	while(1)
	{
		accelerometer_read(&g_adata);
		printf ("acc: x-axis: %f\ty-axis: %f\tz-axis: %f\n",g_adata.x,g_adata.y,g_adata.z);
		gyroscope_read(&g_data);
		printf ("gyro: x-axis: %f\ty-axis: %f\tz-axis: %f\n",g_data.x,g_data.y,g_data.z);
		magnetometer_read(&mag);
		printf ("Mag x-axis: %f\ty-axis: %f\tz-axis: %f\n",mag.x,mag.y,mag.z);
		usleep(250000);
	}

	return 0;
}

void INThandler(int sig)
{
	gyro_deinit();
	acc_deinit();
	mag_deinit();
	printf("\n\n");

	exit(0);
}
