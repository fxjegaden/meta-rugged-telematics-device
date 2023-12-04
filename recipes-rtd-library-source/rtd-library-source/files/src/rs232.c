#include <stdio.h>
#include <unistd.h>
#include "serial.h"
#include "error_nos.h"
#include "common.h"


static int rs232_fd;
/*
 * API		: int rs232_init(int baudrate, const char *node)
 * Description	: API to initialize the RS232 UART.
 * Arguments	: int baudrate - Baudrate for the RS232 UART
 * Return Value	: A non-negative RS232 FD on success. Error code on Failure.
 * */
int rs232_init(int baudrate)
{
	rs232_fd = iW_Serial_Init(SERIAL_PORT_ACM0, baudrate, RS232_UART);
	if(rs232_fd <= 0)
	{
		printf("RS232 INIT FAILED with %d \r\n",rs232_fd);
		rs232_fd = E_SERIAL_INIT;
	}

	usleep(200000);
	tcflush(rs232_fd,TCIOFLUSH);
	usleep(100000);

	return rs232_fd;
}

/*
 * API		: int rs232_deinit()
 * Description	: API to deinitialize the RS232 UART.
 * Arguments	: None
 * Return Value	: 0 on Success. Error code on Failure.
 * */
int rs232_deinit()
{
	int ret;

	ret = close(rs232_fd);
	if (ret < 0)
	{
		printf("RS232 DEINIT FAILED with %d \r\n",ret);
		ret = E_SERIAL_DEINIT;
	}

	return ret;
}

/*
 * API		: int rs232_read( char *buf, long int sz)
 * Description	: API to read the data from RS232 UART.
 * Arguments	: char *buf - To store the RS232 data.
 * 		: long int sz - Maximum number of bytes to be read from RS232 UART.
 * Return Value	: 0 on Success. Error code on Failure.
 * */
int rs232_read( char *buf, long int sz)
{
	char *rs232_read_buf;
	long int rs232_read_size;
	int ret;

	ret = iW_Serial_Read(rs232_fd, buf, sz);
	if(ret < 0)
	{
		ret = E_SERIAL_READ;
	}
	else
	{
		ret = OBD2_LIB_SUCCESS;
	}

	return ret;
}

/*
 * API		: int rs232_write( char *buf, long int sz)
 * Description	: API to write the data to RS232 UART.
 * Arguments	: char *buf - To store the RS232 data.
 * 		: size_t sz - Size of the input buf in bytes.
 * Return Value	: 0 on Success. Error code on Failure.
 * */
int rs232_write( char *buf, size_t sz)
{
	char *rs232_write_buf;
	long int rs232_write_size;
	int ret;

	ret = iW_Serial_Write(rs232_fd, buf, sz);
	if(ret < 0)
	{
		ret = E_SERIAL_WRITE;
	}
	else
	{
		ret = OBD2_LIB_SUCCESS;
	}

	return ret;
}
