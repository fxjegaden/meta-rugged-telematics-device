#ifndef __THREAD_H__
#define __THREAD_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/if_link.h>
#include <linux/input.h>
#include <linux/rtnetlink.h>
#include <linux/sched.h> // Required for task states (TASK_INTERRUPTIBLE etc )
#include <linux/kernel.h>
#include <linux/fs.h> // required for various structures related to files liked fops.
#include <sys/socket.h>
#include <sys/types.h>
#include <net/if.h>
#include <assert.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/timerfd.h>
#include <ctype.h>
#include <limits.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <math.h>
#include <time.h>
#include "lib_common.h"
#include "q_gps.h"
#include "error_nos.h"

/*MACROS*/
#define SNAME				"/sync"
#define NODE_EXP_WITHOUT_GYRO		"/dev/input/event2"
#define NODE_EXP_WITH_GYRO		"/dev/input/event3"
#define RESET				20
#define GPIO_EVENT_PATH			"/sys/class/input/input" 
#define GPIO_EVENT_NAME			"gpio-keys"
#define TIMER_WAKEUP_ALARM_ENABLE	"echo +%lu > /sys/class/rtc/rtc0/wakealarm"
#define TIMER_WAKEUP_ALARM_DISABLE	"echo 0 > /sys/class/rtc/rtc0/wakealarm" 

int timer_wakeup_monitor( );
int timer_wakeup_check_enable( );
int timer_wakeup_check_disable( );
int timer_wakeup_monitor_status( );
int ignition_pin_status_check( );

#endif /* __THREAD_H__ */
