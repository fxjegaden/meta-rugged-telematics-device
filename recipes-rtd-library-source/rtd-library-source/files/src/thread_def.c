#define _GNU_SOURCE
#include "thread.h"
#include "init.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "lib_common.h"

sem_t *sem_ser;
static int mod_init = 0;
IO_PIN_STATE gpio_pin;
int timer = 3;
int ignition_status_value = 0;
int ign_status = 0;
int timer_thread_status = 0;

/* BUG ID 5289 */
int CheckLink(char *ifname)
{
	int ret =0;
	int socId = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (socId < 0)
		return OBD2_LIB_FAILURE;

	struct ifreq if_req;
	(void) strncpy(if_req.ifr_name, ifname, sizeof(if_req.ifr_name));
	int rv = ioctl(socId, SIOCGIFFLAGS, &if_req);
	close(socId);

	if ( rv == -1)
		return E_GSM_NW_CONNECTION_DOWN;

	ret = (if_req.ifr_flags & IFF_UP) && (if_req.ifr_flags & IFF_RUNNING);
	if (ret == 1)
		return OBD2_LIB_SUCCESS;
	else
		return E_GSM_NW_CONNECTION_DOWN;
}

/* BUG ID 5270 */
/*Function used to check whether ethernet interface up or not*/
int Check_eth_Link( )
{
	int ret = OBD2_LIB_FAILURE;
	FILE *fp_eth0 = NULL;
	char eth0_buf[ 2 ] = {0};

	if( access( ETH0_CARRIER_PATH, F_OK ) == OBD2_LIB_SUCCESS )
	{
		fp_eth0 = fopen( ETH0_CARRIER_PATH, "r" );
		if( fp_eth0 == NULL )
		{
			printf("%s - failed to open %s with errno %d\n", __FUNCTION__, ETH0_CARRIER_PATH, errno );
			ret = E_IF_OPEN;
			goto end;
		}

		ret = fread( &eth0_buf, 1, 1, fp_eth0 );
		if( ret >= 0 )
		{
			if( strncmp( eth0_buf, "0", 1 ) == 0 )
			{
				ret = OBD2_LIB_SUCCESS; //Ethernet interface is up
			}
			else
			{
				ret = E_IF_INVALID; //Ethernet interface down
			}
		}
		else
		{
			ret = E_IF_READ;
		}
		fclose( fp_eth0 );
		memset(eth0_buf,0,sizeof(eth0_buf));
	}
	else
	{
		ret = OBD2_LIB_FAILURE;
	}

end:
	return ret;
}

int GPIO_config(void)
{
	int ret, rc = OBD2_LIB_SUCCESS;
	int status = 0;

	/* GPIO to turn on/off Status LED */
	ret = gpio_export(LED_GPIO, OUTPUT);
	CHK_ERR (ret, stderr, "Error in LED_GPIO gpio_export ()");

	/*!< Export TWELVE_V_REG*/
	ret = gpio_export(137, OUTPUT);
	CHK_ERR (ret, stderr, "Error in gpio_export ()-137");

	/*!< Export Battery Status GPIO*/
	ret = gpio_export(118, INPUT);
	CHK_ERR (ret, stderr, "Error in gpio_export ()-118");

	/*Battery Power good*/
	ret = gpio_export(64, INPUT);
	CHK_ERR (ret, stderr, "Error in gpio_export ()-64");

	/*Battery Battery ADC gpio*/
	ret = gpio_export(5, INPUT);
	CHK_ERR (ret, stderr, "Error in gpio_export ()-5");

	ret = gpio_export(120, OUTPUT);
	CHK_ERR (ret, stderr, "Error in gpio_export ()-120");
	ret = set_gpio_value(73, ON);
	CHK_ERR (ret, stderr, "Error in set_gpio_value ()-73");
	ret = set_gpio_value(137, OFF);
	CHK_ERR (ret, stderr, "Error in set_gpio_value ()-137");


	ret = gpio_export(90, OUTPUT);
	CHK_ERR (ret, stderr, "Error in gsm_modem_on - CELLULAR MODULE SWITCH export");

	ret = gpio_export(78, OUTPUT);
	CHK_ERR (ret, stderr, "Error in gpio_export () USB SWITCH FOR CELLULAR MODULE export");

	ret = gpio_export(88, OUTPUT);
	CHK_ERR (ret, stderr, "Error in gpio_export () CELLULAR MODULE PWRKEY export");

	rc = get_gpio( 68, &status );
	if(rc == 0)
	{
		ret = set_gpio_value(68, ON );
		CHK_ERR (ret, stderr, "Error in set_gpio_value ()-68");
		sleep( 2 );
	}
	else
	{
	}

	rc = get_gpio( 68, &status );
	if(rc == 0)
	{
		ret = set_gpio_value(68, OFF );
		CHK_ERR (ret, stderr, "Error in set_gpio_value ()-68");
		sleep( 2 );
	}
	else
	{
		// Do Nothing	
	}

	rc = get_gpio( 68, &status );
	if(rc == 0)
	{
		ret = set_gpio_value(68, ON );
		CHK_ERR (ret, stderr, "Error in set_gpio_value ()-68");
		sleep( 2 );
	}
	else
	{
		// Do Nothing	
	}

	rc = get_gpio( 71, &status );
	if(rc == 0)
	{
		ret = set_gpio_value(71, ON );
		CHK_ERR (ret, stderr, "Error in set_gpio_value ()-71");
		sleep( 2 );
	}
	else
	{
		// Do Nothing	
	}

	return ret;
}

int GPIO_off(void)
{
	int ret = 0;

	/*Export Ethernet reset gpio*/
	ret = gpio_export(68, OUTPUT);
	CHK_ERR (ret, stderr, "Error in gpio_export ()-91");

	/*Export Ethernet activation gpio*/
	ret = gpio_export(71, OUTPUT);
	CHK_ERR (ret, stderr, "Error in gpio_export ()-71");

	ret = set_gpio_value(73, OFF);
	CHK_ERR (ret, stderr, "Error in set_gpio_value ()-73");

	ret = set_gpio_value(68, OFF);
	CHK_ERR (ret, stderr, "Error in set_gpio_value ()-68");

	ret = set_gpio_value(71, OFF);
	CHK_ERR (ret, stderr, "Error in set_gpio_value ()-71");

	return ret;
}

int get_gpio_event(char *path, char *event)
{
	int event_no = -1;	
	int ret = 0;
	FILE *fp = NULL;
	char command[100] = {0};
	char *intf_conf;
	char *file_name = "/name";
	char *event_file = NULL;
	int i;

	for (i = 0; i < 10; i++){
		ret = asprintf (&event_file, "%s%d%s",path,i,file_name);	
		if (ret < 0 )
		{
			ret = -ENOMEM;
			goto exit;
		}
		fp = fopen(event_file, "r");
		if (fp == NULL){
			perror (event_file);
			if (event_file != NULL)
			{
				free(event_file);
				event_file = NULL;
			}
			continue;
		}
		else
		{
			if (event_file != NULL)
			{
				free(event_file);
				event_file = NULL;
			}
			fread(&command, 30, 1, fp);
			if( fp != NULL )
			{
				fclose (fp);
				fp = NULL;
			}
		}
		intf_conf = strstr(command,event);
		if (intf_conf){
			ret = i;
			break;
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

int timer_wakeup_monitor()
{
	int fd = 0;
	int rd;
	unsigned long data;
	struct timeval timeout;
	int retval;
	fd_set rdfs;
	const char *file = "/dev/rtc0";

	if ((fd = open (file, O_RDWR)) < 0) {
		printf("timer_wakeup_monitor - Error Occured while opening Device File.!!\n");
		return -1;
	}
	else {
	}


	FD_ZERO(&rdfs);
	FD_SET(fd, &rdfs);

	/* timeout wait for 500ms */
	timeout.tv_sec = 0;
	timeout.tv_usec = 10000;
	timer_thread_status = 1;

	while(1) {
		if( timer_thread_status == -1 )
		{
			timer_thread_status = 0;
			break;
		}
		retval = select(fd + 1, &rdfs, NULL, NULL, &timeout);

		if(retval) {
			rd = read(fd, &data, sizeof( unsigned long ));
			if (rd < sizeof( unsigned long )) {
				printf("expected %d bytes, got %d\n", sizeof( unsigned long ), rd);
				return 1;
			}
			else
			{
				timer_state = 1;
				printf("timer wakeup %d\n", timer_state );
			}
		}
		FD_SET(fd, &rdfs);
		/* timeout wait for 35ms */
		timeout.tv_sec = 0;
		timeout.tv_usec = 10000;
	}

	timer_state = 0;
	close(fd); // Close the device
}


int ignition_pin_status_check()
{
	int ret = OBD2_LIB_SUCCESS;
	int fd = 0;
	struct input_event ev[64];
	int i, rd;
	struct timeval timeout;
	int retval;
	fd_set rdfs;
	int event_no;
	char *file = NULL;
	event_no = get_gpio_event(GPIO_EVENT_PATH, GPIO_EVENT_NAME);

	ret = asprintf(&file, "/dev/input/event%d", event_no);
	if(ret < 0)
	{
		ret = -ENOMEM;
		goto exit;
	}
	if ((fd = open (file, O_RDWR)) < 0) {
		printf("ignition_pin_status_check - Error Occured while opening Device File.!!\n");
		ret = -1;
		goto exit;
	}
	else {
	}


	FD_ZERO(&rdfs);
	FD_SET(fd, &rdfs);

	/* timeout wait for 500ms */
	timeout.tv_sec = 0;
	timeout.tv_usec = 10000;
	ign_status = 1;
	ign_sleep = 0;

	while(1) {
		if( ign_status == -1 )
		{
			printf("ignition status = %d\n",ign_status);
			ign_status = 0;
			break;
		}
		retval = select(fd + 1, &rdfs, NULL, NULL, &timeout);

		if(retval) {
			rd = read(fd, ev, sizeof(ev));
			if (rd < (int) sizeof(struct input_event)) {
				printf("expected %d bytes, got %d\n", (int) sizeof(struct input_event), rd);
				return 1;
			}

			for (i = 0; i < rd / sizeof(struct input_event); i++) {
				if(ev[i].code == 29 && ev[i].value == 0 ) {
					ignition_status_value = 1;
				}
				else if (ev[i].code == 29 && ev[i].value == 1 ) {
					ignition_status_value = -1;
				}
			}
		}
		FD_SET(fd, &rdfs);
		/* timeout wait for 35ms */
		timeout.tv_sec = 0;
		timeout.tv_usec = 10000;
	}

	ignition_status_value = 0;
	close(fd); // Close the device
exit:
	if(file != NULL)
	{
		free(file);
		file = NULL;
	}
	return ret;
}

int ign_pin_status_check_enable( )
{
	int ret = OBD2_LIB_SUCCESS;
	pthread_t ign_tid;

	if( ign_status == 0 )
	{
		if(pthread_create( &ign_tid, NULL, (void *) ignition_pin_status_check, NULL) != 0) {
			printf(" pthread_create for ignition_status_pin_check failed %d\r\n",errno);
			return errno;
		}
		else
		{
			if (pthread_detach ( ign_tid ) != OBD2_LIB_SUCCESS)
			{
				return errno;
			}
			else
			{
			}
		}
	}
	else
	{
		printf("ign_pin_status_check_enable - thread is already running\n" );
		ret = OBD2_LIB_FAILURE;
	}
	return ret;
}

int ign_pin_status_check_disable( )
{
	ign_status = -1;
	sleep( 2 );
	return OBD2_LIB_SUCCESS;
}

int timer_wakeup_check_enable( )
{
	int ret = OBD2_LIB_SUCCESS;
	pthread_t timer_tid;

	if( timer_thread_status == 0 )
	{
		if(pthread_create( &timer_tid, NULL, (void *) timer_wakeup_monitor, NULL) != 0) {
			printf(" pthread_create for timer_wakeup_monitor failed %d\r\n",errno);
			return errno;
		}
		else
		{
			printf("timer_wakeup_monitor created\n");
			if (pthread_detach ( timer_tid ) != OBD2_LIB_SUCCESS)
			{
				return errno;
			}
			else
			{
			}
		}
	}
	else
	{
		printf("timer_wakeup_monitor - thread is already running\n" );
		ret = OBD2_LIB_FAILURE;
	}
	return ret;
}

int timer_wakeup_check_disable( )
{
	timer_thread_status = -1;
	sleep( 2 );
	return OBD2_LIB_SUCCESS;
}


int timer_wakeup_monitor_status( )
{
	return timer_state;
}

int get_ignition_status_with_gpio()
{
	int ret = FAILURE;
	FILE *fp1 = NULL;
	char *command = NULL;
	char *ptr = NULL;
	char *line = NULL;
	char ign_on[] = "lo IRQ";
	char ign_off[] = "hi IRQ";
	size_t len = 0;
	ssize_t read = 0;

	ret = asprintf(&command, "cat /sys/kernel/debug/gpio | grep gpio-67");
	if(ret < 0)
	{
		ret = -ENOMEM;
		goto exit;
	}else{
		ret = SUCCESS;
	}

	fp1 = popen(command,"r");
	if (fp1 != NULL )
	{
		/* Reading msb of CPU Unique ID*/
		while ((read = getline(&line, &len, fp1)) != -1) {
			//Do Nothing
		}
		pclose(fp1);
	}
	else
	{
		ret = FAILURE;
	}
	ptr = strstr(line, ign_on);
	if(ptr)
	{
		ret = SUCCESS;
		ignition_status_value = 1;
		goto exit;
	}
	ptr = strstr(line, ign_off);
	if(ptr)
	{
		ret = SUCCESS;
		ignition_status_value = -1;
		goto exit;
	}

exit:
	if(command != NULL){
		free(command);
		command = NULL;
		free(line);
		ptr = NULL;
	}
	return ret;
}
int ignition_pin_status( )
{
	int ret = OBD2_LIB_FAILURE;
	float voltage_value;
	if(ignition_status_value == 0)
	{
		ret = get_ignition_status_with_gpio();
		if(ret != SUCCESS)
		{
			printf("return value = %d\n", ret);
		}
	}

	return ignition_status_value;
}
