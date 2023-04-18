#include "obd2lib.h"
#include "q_gps.h"
#include "4g.h"
#include "lib_common.h"

int uart_init (const char *device, int baudrate)
{
	struct termios tty_cfg;
	struct termios *tty = &tty_cfg;
	speed_t tty_speed;

	libClient.serial_intf.tty_fd = open(device, O_RDWR | O_NOCTTY );
	if(libClient.serial_intf.tty_fd < 0)
	{
		return -1;
	}
	memset(tty, 0, sizeof(struct termios));
	if (tcgetattr (libClient.serial_intf.tty_fd, tty) != 0) {
		return -1;
	}

	switch(baudrate) {
		case 0:		tty_speed = B0;		break;
		case 50:	tty_speed = B50;	break;
		case 75:	tty_speed = B75;	break;
		case 110:	tty_speed = B110;	break;
		case 134:	tty_speed = B134;	break;
		case 150:	tty_speed = B150;	break;
		case 200:	tty_speed = B200;	break;
		case 300:	tty_speed = B300;	break;
		case 600:	tty_speed = B600;	break;
		case 1200:	tty_speed = B1200;	break;
		case 1800:	tty_speed = B1800;	break;
		case 2400:	tty_speed = B2400;	break;
		case 4800:	tty_speed = B4800;	break;
		case 9600:	tty_speed = B9600;	break;
		case 19200:	tty_speed = B19200;	break;
		case 38400:	tty_speed = B38400;	break;
		case 57600:	tty_speed = B57600;	break;
		case 115200:	tty_speed = B115200;	break;
		case 230400:	tty_speed = B230400;	break;
		case 460800:	tty_speed = B460800;	break;
		case 576000:	tty_speed = B576000;	break;
		case 921600:	tty_speed = B921600;	break;
		case 1000000:	tty_speed = B1000000;	break;
		case 2000000:	tty_speed = B2000000;	break;
		case 4000000:	tty_speed = B4000000;	break;

		default:
				return -1;
	}

	cfsetospeed (tty, tty_speed);
	cfsetispeed (tty, tty_speed);

	/* Serial Port Settings for RS485 */
	/* control modes */
	tty->c_cflag |= ( CLOCAL | CREAD | HUPCL );

	/* input modes */
	tty->c_iflag &= ~( IGNBRK| BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON );

	/* output modes */
	tty->c_oflag &= ~( OPOST );

	/* local modes */
	tty->c_lflag &= ~( ECHO | ECHONL | ICANON | ISIG | IEXTEN );

	/* XON and XOFF values */
	tty->c_cc[ VSTART ] = 0x00;
	tty->c_cc[ VSTOP ] =  0x00;

	/* Minimum Data present in the port */
	tty->c_cc[ VMIN ] = 1;

	/* Wait Time for data to check for the presence of data in the port */
	tty->c_cc[ VTIME ] = 0;

	/* Make raw */
	cfmakeraw(tty);

	/* Flush Port, then applies attributes */
	tcflush(libClient.serial_intf.tty_fd, TCIFLUSH );

	if (tcsetattr(libClient.serial_intf.tty_fd, TCSANOW, tty) != 0)
	{
		return -1;
	}

	FD_ZERO(&libClient.serial_intf.read_fds);
	FD_ZERO(&libClient.serial_intf.write_fds);
	FD_ZERO(&libClient.serial_intf.except_fds);
	FD_SET(libClient.serial_intf.tty_fd, &libClient.serial_intf.read_fds);
	return libClient.serial_intf.tty_fd;
}


int iW_Serial_SetDatabits( struct termios *TermiosPtr, int databits)
{
	TermiosPtr->c_cflag &= ~CSIZE;
	/* Select 8 data bits */
	if(databits == 5) TermiosPtr->c_cflag |= CS5;
	else if(databits == 6) TermiosPtr->c_cflag |= CS6;
	else if(databits == 7) TermiosPtr->c_cflag |= CS7;
	else TermiosPtr->c_cflag |= CS8;

	return 0;
}

int iW_Serial_SetStopbits( struct termios *TermiosPtr, int Stopbits)
{
	if(Stopbits == 2)
		TermiosPtr->c_cflag |= CSTOPB;
	else
		TermiosPtr->c_cflag &= ~CSTOPB;
	return 0;
}

int iW_Serial_SetParity( struct termios *TermiosPtr, char Parity)
{
	if(Parity == 'n' || Parity == 'N')
		TermiosPtr->c_cflag &= ~PARENB;
	else if(Parity == 'o' || Parity == 'O')
	{
		TermiosPtr->c_cflag |= PARENB;
		TermiosPtr->c_cflag |= PARODD;
	}
	else
	{
		TermiosPtr->c_cflag |= PARENB;
		TermiosPtr->c_cflag &= ~PARODD;
	}
	return 0;
}


int iW_Serial_SetBaudrate( struct termios* TermiosPtr, int Baudrate)
{
	/* Baud rate */
	speed_t Speed = B50;
	/* Error handling variable */
	int status = 0;

	/* Select the Baud rate */
	if(Baudrate == 460800) Speed = B460800;
	if(Baudrate == 230400) Speed = B230400;
	if(Baudrate == 115200) Speed = B115200;
	if(Baudrate == 57600) Speed = B57600;
	if(Baudrate == 38400) Speed = B38400;
	if(Baudrate == 19200) Speed = B19200;
	if(Baudrate == 9600) Speed = B9600;
	if(Baudrate == 4800) Speed = B4800;
	if(Baudrate == 2400) Speed = B2400;
	if(Baudrate == 1200) Speed = B1200;
	if(Baudrate == 600) Speed = B600;
	if(Baudrate == 300) Speed = B300;
	if(Baudrate == 150) Speed = B150;
	if(Baudrate == 110) Speed = B110;
	if(Baudrate == 75) Speed = B75;

	/* Set the Output baud rate */
	if(cfsetospeed( TermiosPtr, Speed) == -1)
	{
		perror("iW_Serial_SetBaudrate:\nError while \
				setting output baud rate of serial device");
		IOBD_DEBUG_LEVEL3("errno = %d \r\n",errno);

		status = -1;
	}

	/* Set the Input baud rate */
	if(cfsetispeed( TermiosPtr, Speed) == -1)
	{
		perror("iW_Serial_SetBaudrate:\nError while setting input \
				baud rate of serial device");
		IOBD_DEBUG_LEVEL3("errno = %d \r\n",errno);

		status = -1;
	}

	return status;
}

int iW_Serial_Init(int Serial_Port_Number, int baudrate, const char *node)
{
	/*Argument Validation*/
	if (Serial_Port_Number != 0)
		return E_OBD2_LIB_INVALID_ARG;
	/* Serial handle and maximum number of discriptors to watch */
	int Serial_FD = 0, serial_maxfd, ret = 0;
	/* Serial device private informations */
	struct termios OldTermiosPtr;
	struct termios NewTermiosPtr;

	/* Set of serial read descriptors */
	fd_set serial_readfs;
	/* Open the serial device */
	switch(Serial_Port_Number)
	{
		case 0:
		case 1: Serial_FD = open(node, O_RDWR | O_NONBLOCK );
			break;
		default:
			IOBD_DEBUG_LEVEL3("\niW_Serial_Init: Invalid Serial port number\r\n");
			break;
	}

	if(Serial_FD <= 0)
	{
		IOBD_ERR("iW_Serial_Init: Error while opening the serial device [%s] with errno = %d", node, errno);
		ret = OBD2_LIB_FAILURE;
	}
	else
	{
		//serial_maxfd = Serial_FD+1;
		/* maximum bit entry (fd1) to test */
		FD_ZERO(&serial_readfs);
		/* set testing for source 1 */
		FD_SET(Serial_FD, &serial_readfs);
		tcgetattr(Serial_FD, &OldTermiosPtr);
		memcpy( &NewTermiosPtr, &OldTermiosPtr,\
				sizeof(NewTermiosPtr));
		NewTermiosPtr = OldTermiosPtr;
		/* Set the parameters for the port */
		/* control modes */
		NewTermiosPtr.c_cflag |= (CLOCAL |CREAD | HUPCL );
		/* input modes */
		NewTermiosPtr.c_iflag &= ~( IGNBRK | BRKINT | PARMRK | ISTRIP| INLCR|IGNCR|ICRNL|IXON);
		/* output modes */
		NewTermiosPtr.c_oflag &= ~(OPOST | ONLCR);
		/* local modes */
		NewTermiosPtr.c_lflag &= ~(ECHO | ECHONL |ICANON |ISIG|IEXTEN);
		/* XON and XOFF values */
		NewTermiosPtr.c_cc[VSTART] = 0x00;
		NewTermiosPtr.c_cc[VSTOP] = 0x00;
		/* Minimum Data present in the port */
		NewTermiosPtr.c_cc[VMIN] = 1;
		/* Wait Time for data to check for the presence of data in the port */
		NewTermiosPtr.c_cc[VTIME]= 0;
		ret = iW_Serial_SetBaudrate( &NewTermiosPtr, baudrate );
		if (ret == OBD2_LIB_FAILURE)
		{
			IOBD_ERR ("iW_Serial_SetBaudrate : failed");
		}
		else
		{
			iW_Serial_SetDatabits( &NewTermiosPtr, DATABITS );
			iW_Serial_SetStopbits( &NewTermiosPtr, STOPBITS );
			iW_Serial_SetParity( &NewTermiosPtr, PARITY );
			if((ret = tcsetattr(Serial_FD, TCSANOW, &NewTermiosPtr)) == -1)
			{
				if (CHK_ERR (ret, ERROR_FILE, "iW_Serial_Init_tcsetattr") != OBD2_LIB_SUCCESS)
				{
					ret = OBD2_LIB_FAILURE;
				}
			}
			else
			{
				ret = Serial_FD;
			}
		}
	}

	return ret;
}

int iW_Serial_Write(int fd, char *buf, size_t sz)
{
	int ret, bytes_written = 0, bytes_to_write;

	IOBD_DEBUG_LEVEL3("iW_Serial_Write : fd = %d\tbuf = %s\tsz = %d\n", fd, buf, sz);

	/*!< Check for Invalid Arguments */
	if ((fd < 0) && (buf == NULL) &&(sz ==0))
	{
		ret = E_OBD2_LIB_INVALID_ARG;
	}
	else
	{
		IOBD_DEBUG_LEVEL3("iW_Serial_Write : Valid Arguments\n");
		bytes_to_write = sz;
		while (bytes_written < sz)
		{
			ret = write (fd, buf, bytes_to_write);
			usleep(10000);
			if (ret == -1)
			{
				ret = errno;
			}
			else
			{
				bytes_written += ret;
			}

			IOBD_DEBUG_LEVEL3("iW_Serial_Write : bytes_written = %d\tret = %d\n", bytes_written, ret);
			buf += ret;
			bytes_to_write = (sz - bytes_written);
		}
	}

	return ret;
}

int iW_Serial_Read(int fd , char *buf, long int sz)
{
	unsigned int bytes_read = 0;
	int ret = 0;
	fd_set set;
	struct timeval timeout;

	/*!< Check for Invalid Arguments */
	if ((fd < 0) || (buf == NULL) || (sz == 0))
	{
		ret = E_OBD2_LIB_INVALID_ARG;
	}
	else
	{
		/* clear the set */
		FD_ZERO(&set);
	       	/* add file descriptor to the set */
		FD_SET(fd, &set);

		timeout.tv_sec = 0;
		timeout.tv_usec = 2000000;

		ret = select(fd + 1, &set, NULL, NULL, &timeout);
		if(ret > OBD2_LIB_SUCCESS)
		{
			ret = read(fd, buf, sz);
			if (ret == -1)
			{
				IOBD_DEBUG_LEVEL3 ("iW_Serial_Read Error: ret = %d", ret);
				ret = errno;
			}
			else
			{
				bytes_read += ret;
				IOBD_DEBUG_LEVEL3("iW_Serial_Read : bytes_read = %d\tret = %d\n", bytes_read, ret);
			}
		}
		else if(ret == OBD2_LIB_SUCCESS)
		{
			printf("Timeout Occured\n");
			ret = E_SERIAL_READ_TIMEOUT;
		}
		else
		{
			ret = E_SERIAL_READ;
		}
	}

	return ret;
}
