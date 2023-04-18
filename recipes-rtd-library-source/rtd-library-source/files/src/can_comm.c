#include "lib_can.h"

#define NLMSG_TAIL(nmsg) \
	((struct rtattr *) (((void *) (nmsg)) + NLMSG_ALIGN((nmsg)->nlmsg_len)))

#define IFLA_CAN_MAX		(__IFLA_CAN_MAX - 1)
#define IF_UP			1
#define IF_DOWN			2

#define GET_STATE		1
#define GET_RESTART_MS		2
#define GET_BITTIMING		3
#define GET_CTRLMODE		4
#define GET_CLOCK		5
#define GET_BITTIMING_CONST	6
#define GET_BERR_COUNTER	7
#define GET_XSTATS		8
#define CAN_SFF_FLAG_11_BIT	0x000007E8

sem_t* can_lock;
struct can_filter *rfilter;
volatile int no_of_mask = 0;
struct sockaddr_can addr_read_can0;
struct sockaddr_can addr_read_can1;
struct sockaddr_can addr_read_can2;

int sread_can0 = 0;
int sread_can1 = 0;
int sread_can2 = 0;

volatile int reqst_flag = 0;
int can_sem_init (void) __attribute__ ((constructor));
void can_sem_deinit (void) __attribute__ ((destructor));

int can_sem_init (void)
{
	int ret;
	/*!< Init Semaphore for CAN*/
	can_lock = sem_open("/can_lock", O_CREAT | EEXIST | O_EXCL, 0777, 1);
	if (can_lock == SEM_FAILED){
		ret = E_CAN_SEM_INIT;
		CHK_ERR (ret, stderr, "Error: can_init() sem init");
	}else
		sem_unlink("/can_lock");
	return ret;
}

void can_sem_deinit (void)
{
	if (can_lock != SEM_FAILED)
		sem_destroy(can_lock);

}


struct get_req {
	struct nlmsghdr n;
	struct rtgenmsg g;
};

struct set_req {
	struct nlmsghdr n;
	struct ifinfomsg i;
	char buf[1024];
};

struct req_info {
	__u8 restart;
	__u8 disable_autorestart;
	__u32 restart_ms;
	struct can_ctrlmode *ctrlmode;
	struct can_bittiming *bittiming;
	struct can_bittiming *dbittiming;
};

static int addattr32(struct nlmsghdr *n, size_t maxlen, int type, __u32 data)
{
	int len = RTA_LENGTH(4);
	struct rtattr *rta;

	if (NLMSG_ALIGN(n->nlmsg_len) + len > maxlen) {
		fprintf(stderr,
				"addattr32: Error! max allowed bound %zu exceeded\n",
				maxlen);
		return -1;
	}

	rta = NLMSG_TAIL(n);
	rta->rta_type = type;
	rta->rta_len = len;
	memcpy(RTA_DATA(rta), &data, 4);
	n->nlmsg_len = NLMSG_ALIGN(n->nlmsg_len) + len;

	return 0;
}

static int addattr_l(struct nlmsghdr *n, size_t maxlen, int type,
		const void *data, int alen)
{
	int len = RTA_LENGTH(alen);
	struct rtattr *rta;

	if (NLMSG_ALIGN(n->nlmsg_len) + RTA_ALIGN(len) > maxlen) {
		fprintf(stderr,
				"addattr_l ERROR: message exceeded bound of %zu\n",
				maxlen);
	}

	rta = NLMSG_TAIL(n);
	rta->rta_type = type;
	rta->rta_len = len;
	memcpy(RTA_DATA(rta), data, alen);
	n->nlmsg_len = NLMSG_ALIGN(n->nlmsg_len) + RTA_ALIGN(len);

	return 0;
}

/**
 * @ingroup intern
 * @brief send_mod_request - send a linkinfo modification request
 *
 * @param fd decriptor to a priorly opened netlink socket
 * @param n netlink message containing the request
 *
 * sends a request to setup the the linkinfo to netlink layer and awaits the
 * status.
 *
 * @return 0 if success
 * @return negativ if failed
 */
static int send_mod_request(int fd, struct nlmsghdr *n)
{
	int status;
	struct sockaddr_nl nladdr;
	struct nlmsghdr *h;

	struct iovec iov = {
		.iov_base = (void *)n,
		.iov_len = n->nlmsg_len
	};
	struct msghdr msg = {
		.msg_name = &nladdr,
		.msg_namelen = sizeof(nladdr),
		.msg_iov = &iov,
		.msg_iovlen = 1,
	};
	char buf[16384];

	memset(&nladdr, 0, sizeof(nladdr));

	nladdr.nl_family = AF_NETLINK;
	nladdr.nl_pid = 0;
	nladdr.nl_groups = 0;

	n->nlmsg_seq = 0;
	n->nlmsg_flags |= NLM_F_ACK;

	status = sendmsg(fd, &msg, 0);

	if (status < 0) {
		perror("Cannot talk to rtnetlink");
		return -1;
	}

	iov.iov_base = buf;
	while (1) {
		iov.iov_len = sizeof(buf);
		status = recvmsg(fd, &msg, 0);
		for (h = (struct nlmsghdr *)buf; (size_t) status >= sizeof(*h);) {
			int len = h->nlmsg_len;
			int l = len - sizeof(*h);
			if (l < 0 || len > status) {
				if (msg.msg_flags & MSG_TRUNC) {
					fprintf(stderr, "Truncated message\n");
					return -1;
				}
				fprintf(stderr,
						"!!!malformed message: len=%d\n", len);
				return -1;
			}

			if (h->nlmsg_type == NLMSG_ERROR) {
				struct nlmsgerr *err =
					(struct nlmsgerr *)NLMSG_DATA(h);
				if ((size_t) l < sizeof(struct nlmsgerr)) {
					fprintf(stderr, "ERROR truncated\n");
				} else {
					errno = -err->error;
					if (errno == 0)
						return 0;

					perror("RTNETLINK answers");
				}
				return -1;
			}
			status -= NLMSG_ALIGN(len);
			h = (struct nlmsghdr *)((char *)h + NLMSG_ALIGN(len));
		}
	}

	return 0;
}

/**
 * @ingroup intern
 * @brief open_nl_sock - open a netlink socket
 *
 * opens a netlink socket and returns the socket descriptor
 *
 * @return 0 if success
 * @return negativ if failed
 */
static int open_nl_sock()
{
	int fd;
	int sndbuf = 32768;
	int rcvbuf = 32768;
	unsigned int addr_len;
	struct sockaddr_nl local;

	fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (fd < 0) {
		perror("Cannot open netlink socket");
		return -1;
	}

	setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (void *)&sndbuf, sizeof(sndbuf));

	setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void *)&rcvbuf, sizeof(rcvbuf));

	memset(&local, 0, sizeof(local));
	local.nl_family = AF_NETLINK;
	local.nl_groups = 0;

	if (bind(fd, (struct sockaddr *)&local, sizeof(local)) < 0) {
		perror("Cannot bind netlink socket");
		return -1;
	}

	addr_len = sizeof(local);
	if (getsockname(fd, (struct sockaddr *)&local, &addr_len) < 0) {
		perror("Cannot getsockname");
		return -1;
	}
	if (addr_len != sizeof(local)) {
		fprintf(stderr, "Wrong address length %u\n", addr_len);
		return -1;
	}
	if (local.nl_family != AF_NETLINK) {
		fprintf(stderr, "Wrong address family %d\n", local.nl_family);
		return -1;
	}
	return fd;
}

/**
 * @ingroup intern
 * @brief do_set_nl_link - setup linkinfo
 *
 * @param fd socket file descriptor to a priorly opened netlink socket
 * @param if_state state of the interface we want to put the device into. this
 * parameter is only set if you want to use the callback to driver up/down the
 * device
 * @param name name of the can device. This is the netdev name, as ifconfig -a shows
 * in your system. usually it contains prefix "can" and the numer of the can
 * line. e.g. "can0"
 * @param req_info request parameters
 *
 * This callback can do two different tasks:
 * - bring up/down the interface
 * - set up a netlink packet with request, as set up in req_info
 * Which task this callback will do depends on which parameters are set.
 *
 * @return 0 if success
 * @return -1 if failed
 */
static int do_set_nl_link(int fd, __u8 if_state, const char *name,
		struct req_info *req_info)
{
	struct set_req req;

	const char *type = "can";

	memset(&req, 0, sizeof(req));

	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
	req.n.nlmsg_type = RTM_NEWLINK;
	req.i.ifi_family = 0;

	req.i.ifi_index = if_nametoindex(name);
	if (req.i.ifi_index == 0) {
		fprintf(stderr, "Cannot find device \"%s\"\n", name);
		return -1;
	}

	if (if_state) {
		switch (if_state) {
			case IF_DOWN:
				req.i.ifi_change |= IFF_UP;
				req.i.ifi_flags &= ~IFF_UP;
				break;
			case IF_UP:
				req.i.ifi_change |= IFF_UP;
				req.i.ifi_flags |= IFF_UP;
				break;
			default:
				fprintf(stderr, "unknown state\n");
				return -1;
		}
	}

	if (req_info != NULL) {
		/* setup linkinfo section */
		struct rtattr *linkinfo = NLMSG_TAIL(&req.n);
		addattr_l(&req.n, sizeof(req), IFLA_LINKINFO, NULL, 0);
		addattr_l(&req.n, sizeof(req), IFLA_INFO_KIND, type,
				strlen(type));
		/* setup data section */
		struct rtattr *data = NLMSG_TAIL(&req.n);
		addattr_l(&req.n, sizeof(req), IFLA_INFO_DATA, NULL, 0);

		if (req_info->restart_ms > 0 || req_info->disable_autorestart)
			addattr32(&req.n, 1024, IFLA_CAN_RESTART_MS,
					req_info->restart_ms);

		if (req_info->restart)
			addattr32(&req.n, 1024, IFLA_CAN_RESTART, 1);

		if (req_info->bittiming != NULL) {
			addattr_l(&req.n, 1024, IFLA_CAN_BITTIMING,
					req_info->bittiming,
					sizeof(struct can_bittiming));
		}

		if (req_info->dbittiming != NULL) {
			addattr_l(&req.n, 1024, IFLA_CAN_DATA_BITTIMING,
					req_info->dbittiming,
					sizeof(struct can_bittiming));
		}

		if (req_info->ctrlmode != NULL) {
			addattr_l(&req.n, 1024, IFLA_CAN_CTRLMODE,
					req_info->ctrlmode,
					sizeof(struct can_ctrlmode));
		}

		/* mark end of data section */
		data->rta_len = (void *)NLMSG_TAIL(&req.n) - (void *)data;

		/* mark end of link info section */
		linkinfo->rta_len =
			(void *)NLMSG_TAIL(&req.n) - (void *)linkinfo;
	}

	return send_mod_request(fd, &req.n);
}

/**
 * @ingroup intern
 * @brief set_link - open a netlink socket and setup linkinfo
 *
 * @param name name of the can device. This is the netdev name, as ifconfig -a
 * shows in your system. usually it contains prefix "can" and the numer of the
 * can line. e.g. "can0"
 * @param if_state state of the interface we want to put the device into. this
 * parameter is only set if you want to use the callback to driver up/down the
 * device
 * @param req_info request parameters
 *
 * This is a wrapper for do_set_nl_link. It opens a netlink socket and sends
 * down the requests.
 *
 * @return 0 if success
 * @return -1 if failed
 */
static int set_link(const char *name, __u8 if_state, struct req_info *req_info)
{
	int err, fd;

	fd = open_nl_sock();
	if (fd < 0)
		return -1;

	err = do_set_nl_link(fd, if_state, name, req_info);
	close(fd);

	return err;
}

/**
 * @ingroup extern
 * can_do_start - start the can interface
 * @param name name of the can device. This is the netdev name, as ifconfig -a shows
 * in your system. usually it contains prefix "can" and the numer of the can
 * line. e.g. "can0"
 *
 * This starts the can interface with the given name. It simply changes the if
 * state of the interface to up. All initialisation works will be done in
 * kernel. The if state can also be queried by a simple ifconfig.
 *
 * @return 0 if success
 * @return -1 if failed
 */
int can_do_start(const char *name)
{
	sleep(1);
	return set_link(name, IF_UP, NULL);
}

/**
 * @ingroup extern
 * can_do_stop - stop the can interface
 * @param name name of the can device. This is the netdev name, as ifconfig -a shows
 * in your system. usually it contains prefix "can" and the numer of the can
 * line. e.g. "can0"
 *
 * This stops the can interface with the given name. It simply changes the if
 * state of the interface to down. Any running communication would be stopped.
 *
 * @return 0 if success
 * @return -1 if failed
 */
int can_do_stop(const char *name)
{
	return set_link(name, IF_DOWN, NULL);
}

/**
 * @ingroup extern
 * can_set_restart_ms - set interval of auto restart.
 *
 * @param name name of the can device. This is the netdev name, as ifconfig -a shows
 * in your system. usually it contains prefix "can" and the numer of the can
 * line. e.g. "can0"
 * @param restart_ms interval of auto restart in milliseconds
 *
 * This sets how often the device shall automatically restart the interface in
 * case that a bus_off is detected.
 *
 * @return 0 if success
 * @return -1 if failed
 */
int can_set_restart_ms(const char *name, __u32 restart_ms)
{
	struct req_info req_info = {
		.restart_ms = restart_ms,
	};

	if (restart_ms == 0)
		req_info.disable_autorestart = 1;

	return set_link(name, 0, &req_info);
}

/**
 * @ingroup extern
 * can_set_bittiming - setup the bittiming.
 *
 * @param name name of the can device. This is the netdev name, as ifconfig -a shows
 * in your system. usually it contains prefix "can" and the numer of the can
 * line. e.g. "can0"
 * @param bt pointer to a can_bittiming struct
 *
 * This sets the bittiming of the can device. This is for advantage usage. In
 * normal cases you should use can_set_bitrate to simply define the bitrate and
 * let the driver automatically calculate the bittiming. You will only need this
 * function if you wish to define the bittiming in expert mode with fully
 * manually defined timing values.
 * You have to define the bittiming struct yourself. a can_bittiming struct
 * consists of:
 *
 * @code
 * struct can_bittiming {
 *	__u32 bitrate;
 *	__u32 sample_point;
 *	__u32 tq;
 *	__u32 prop_seg;
 *	__u32 phase_seg1;
 *	__u32 phase_seg2;
 *	__u32 sjw;
 *	__u32 brp;
 * }
 * @endcode
 *
 * to define a customized bittiming, you have to define tq, prop_seq,
 * phase_seg1, phase_seg2 and sjw. See http://www.can-cia.org/index.php?id=88
 * for more information about bittiming and synchronizations on can bus.
 *
 * @return 0 if success
 * @return -1 if failed
 */

int can_set_bittiming(const char *name, struct can_bittiming *bt)
{
	struct req_info req_info = {
		.bittiming = bt,
	};

	return set_link(name, 0, &req_info);
}

/**
 * @ingroup extern
 * can_set_bitrate - setup the bitrate.
 *
 * @param name name of the can device. This is the netdev name, as ifconfig -a shows
 * in your system. usually it contains prefix "can" and the numer of the can
 * line. e.g. "can0"
 * @param bitrate bitrate of the can bus
 *
 * This is the recommended way to setup the bus bit timing. You only have to
 * give a bitrate value here. The exact bit timing will be calculated
 * automatically. To use this function, make sure that CONFIG_CAN_CALC_BITTIMING
 * is set to y in your kernel configuration. bitrate can be a value between
 * 1000(1kbit/s) and 1000000(1000kbit/s).
 *
 * @return 0 if success
 * @return -1 if failed
 */

int can_set_bitrate(const char *name, __u32 bitrate)
{
	struct can_bittiming bt;

	memset(&bt, 0, sizeof(bt));
	bt.bitrate = bitrate;

	return can_set_bittiming(name, &bt);
}

/* BUG ID 5253 and 5250 */
int can_init(const char *name, int bitrate)
{
	int ret = OBD2_LIB_SUCCESS;

	if((bitrate == 250000) || (bitrate == 500000) || (bitrate == 1000000) || (bitrate == 2000000))
	{
		if( ((strcmp(name, CAN0 ) != 0) && ((strcmp(name, CAN1 ) != 0)) ))
		{
			ret = E_CAN_INTERFACE_NOT_FOUND;
		}
		else if(strcmp(name, CAN2 ) == 0)
		{
			ret = E_CAN_INTERFACE_NOT_FOUND;
			printf("%s : CAN FD cannot be initialized with this API\n", __func__);
		}
		else
	{
		ret = CheckLink( (char *) name);
			if (ret == OBD2_LIB_SUCCESS)
			{
			ret = can_deinit (name);
			}

			ret = can_set_bitrate(name, (__u32)bitrate);
			if (ret != 0)
			{
				ret = E_CAN_BITRATE;
		}
			else
			{
				ret = can_do_start(name);
				if (ret != 0)
					ret = E_CAN_INIT;
			}
		}
	}
	else
	{
		ret = E_CAN_INVALID_BITRATE;
	}

	return ret;
}

int can_fd_init(int bitrate, int dbitrate, int txqueuelen)
{
	int ret = OBD2_LIB_FAILURE;
	int fd;
	char *name = CAN2;
	struct can_bittiming bt;
	struct can_bittiming dbt;
	struct can_ctrlmode cm;
	struct req_info req_info = {0};

	if( (bitrate >= 0) && (dbitrate >= 0) && (txqueuelen >= 0) )
	{
		if( system("lsmod | grep tcan4x5x > /dev/null") )
		{
			ret = system("insmod /iwtest/kernel-module/tcan4x5x.ko");
		}
		else
		{
			ret = OBD2_LIB_SUCCESS;
		}

		if(ret == OBD2_LIB_SUCCESS)
		{
			ret = CheckLink( name );
			if (ret == OBD2_LIB_SUCCESS)
			{
				ret = can_deinit( name );
			}
			else
			{
				// Do Nothing
			}

		if ((bitrate == 250000) || (bitrate == 500000) || (bitrate == 1000000) || (bitrate == 2000000))
		{
				/* Set the Bitrate for CAN FD */
				memset(&bt, 0, sizeof(bt));
				bt.bitrate = bitrate;
				req_info.bittiming = &bt;

				if (dbitrate > 0)
				{
					/* Set the Data Bitrate for CAN FD */
					memset(&dbt, 0, sizeof(dbt));
					dbt.bitrate = dbitrate;
					req_info.dbittiming = &dbt;

					/* Enables the CAN FD Mode */
					memset(&cm, 0, sizeof(cm));
					cm.mask |= CAN_CTRLMODE_FD;
					cm.flags |= CAN_CTRLMODE_FD;
					req_info.ctrlmode = &cm;
				}

				fd = open_nl_sock();
				if (fd < OBD2_LIB_SUCCESS)
				{
					ret = E_CAN_INIT;
				}
				else
				{
					ret = do_set_nl_link(fd, 0, name, &req_info);
					if (ret != OBD2_LIB_SUCCESS)
					{
						ret = E_CAN_BITRATE;
					}
					else
					{
						if (txqueuelen > 0)
						{
							ret = set_qlen(name, txqueuelen);
							if (ret != OBD2_LIB_SUCCESS)
							{
				ret = E_CAN_BITRATE;
			}
			else
			{
				ret = can_do_start(name);
				if (ret != 0)
					ret = E_CAN_INIT;
			}
							close(fd);
						}
						else
						{
							// Do Nothing
						}
					}
				}

				ret = can_do_start(name);
				if (ret != 0)
				{
					ret = E_CAN_INIT;
				}
				else
				{
					// Do Nothing
				}
			}
			else
		{
			ret = E_CAN_INVALID_BITRATE;
		}
	}
		else
		{
			ret = E_CAN_MOD_LOAD_ERROR;
		}
	}
	else
	{
		ret = E_OBD2_LIB_INVALID_ARG;
	}

	return ret;
}

int set_qlen(const char *name, int qlen)
{
	struct ifreq ifr = { .ifr_qlen = qlen };
	int fd;
	int ret = OBD2_LIB_FAILURE;

	fd = socket(PF_INET, SOCK_DGRAM, 0);
	if (fd >= OBD2_LIB_SUCCESS)
	{
		memcpy(ifr.ifr_name, name, strlen(name));
		if (ioctl(fd, SIOCSIFTXQLEN, &ifr) < 0)
		{
			perror("SIOCSIFXQLEN");
			ret = errno;
		}
		close(fd);
	}
	else
	{
		// Do Nothing
	}

	return ret;
}

int can_deinit(const char *name)
{
	int err = 0, can0_ret = 0, can1_ret = 0, can2_ret = 0;
	FILE *fp_can = NULL;
	char can_buf[ 10 ] = { 0 };

	if(name != NULL)
	{
		can0_ret = strcmp( name, "can0" );
		can1_ret = strcmp( name, "can1" );
		can2_ret = strcmp( name, "can2" );

		if ((strcmp(name, "can0" )) == 0){
			if (sread_can0 > 0){
				close(sread_can0);
				sread_can0 = 0;
			}
		}

		if ((strcmp(name, "can1" )) == 0){
			if (sread_can1 > 0){
				close(sread_can1);
				sread_can1 = 0;
			}
		}

		if ((strcmp(name, "can2" )) == 0){
			if (sread_can2 > 0){
				close(sread_can2);
				sread_can2 = 0;
			}
		}

		if( !can0_ret || !can1_ret )
		{
			if( can0_ret == 0 )
			{
				if( access( CAN0_WAKEUP_PATH, F_OK ) == OBD2_LIB_SUCCESS )
				{
					fp_can = fopen( CAN0_WAKEUP_PATH, "r" );
				}
				else
				{
				}
			}
			else if ( can1_ret == 0 )
			{
				if( access( CAN1_WAKEUP_PATH, F_OK ) == OBD2_LIB_SUCCESS )
				{
					fp_can = fopen( CAN1_WAKEUP_PATH, "r" );
				}
				else
				{
				}
			}
			else
			{
			}

			if( fp_can == NULL )
			{
				printf("%s - failed to open %s wakeup path with errno %d\n", __FUNCTION__, name, errno );
				err = OBD2_LIB_FAILURE;
			}
			else
			{
				fread( &can_buf, 10, 1, fp_can );
				if( strncmp( can_buf, "enabled", 7 ) == 0 )
				{
					err = E_CAN_WAKEUP_ENABLED;
				}
				else
				{
					if (can_do_stop(name) != 0)
						err = E_CAN_DEINIT;
				}
			}
		}
		else if( !can2_ret )
		{
			if (can_do_stop(name) != 0)
				err = E_CAN_DEINIT;
		}
		else
		{
			err = E_CAN_INTERFACE_NOT_FOUND;
		}
	}
	else
	{
		err = E_CAN_INTERFACE_NOT_FOUND;
	}

	return err;
}

int read_init(int* s,struct ifreq *ifr,struct sockaddr_can* addr,char * can)
{
	int optval = 0;
	int enable_canfd = 1;
	int errnos = 0;
	struct can_filter raw_filter[2];

	*s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if( *s < 0 )
	{
		return -1;
	}

	optval = 1;
	if(setsockopt( *s, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof( int ) ))
	{
		printf("error when enabling CAN FD support\n");
	}
	else
	{
		printf("enabling CAN FD support Success\n");
	}

	if (setsockopt(*s, SOL_CAN_RAW, CAN_RAW_FD_FRAMES,&enable_canfd, sizeof(enable_canfd)))
	{
		printf("error when enabling CAN FD support\n");
		return -1;
	}

	if(reqst_flag)
	{
		raw_filter[0].can_id = 0x7E8 & CAN_SFF_FLAG_11_BIT;
		raw_filter[0].can_mask = (CAN_EFF_FLAG|CAN_RTR_FLAG|CAN_SFF_FLAG_11_BIT);

		raw_filter[1].can_id = ((0x18DAF100 | CAN_EFF_FLAG) & (CAN_EFF_MASK | CAN_EFF_FLAG));
		raw_filter[1].can_mask = (CAN_EFF_FLAG | CAN_RTR_FLAG | 0x18DAF100);

		setsockopt(*s, SOL_CAN_RAW, CAN_RAW_FILTER, &raw_filter, sizeof(raw_filter));
		strcpy(ifr->ifr_name,can);
		ioctl(*s, SIOCGIFINDEX, ifr);

		addr->can_family = AF_CAN;
		addr->can_ifindex = ifr->ifr_ifindex;

		bind(*s, (struct sockaddr *)addr, sizeof(struct sockaddr_can));
	}
	else if(reqst_flag == 0)
	{
		printf("sizeof filter in read init = %d\n", no_of_mask * sizeof(struct can_filter));

		if((errnos = setsockopt(*s, SOL_CAN_RAW, CAN_RAW_FILTER, rfilter, no_of_mask * sizeof(struct can_filter) )))
		{
			printf("error when enabling the filters return value and error no = %d, %x\n", errnos, errno);
		}
		else
		{
			printf("enabling the filters successfull return value & error number= %d, %x\n", errnos, errno);
		}

		reqst_flag = 0;
		strcpy(ifr->ifr_name,can);
		ioctl(*s, SIOCGIFINDEX, ifr);

		addr->can_family = AF_CAN;
		addr->can_ifindex = ifr->ifr_ifindex;

		bind(*s, (struct sockaddr *)addr, sizeof(struct sockaddr_can));
	}

	return 0;
}

int set_can_mask_and_filter(uint32_t *mask, uint32_t *filter, int no_of_filter)
{
	int ret = 0;
	int i = 0;

	no_of_mask = no_of_filter;
	rfilter = (struct can_filter *) malloc (no_of_filter * sizeof(struct can_filter));
	if( rfilter != NULL )
	{
		printf("adding filters for socket\n");
		for (i = 0; i < no_of_filter; i++)
		{
			printf("filter id [%d] = %x\n", i, filter[i]);
			if(filter[i] & 0xFF00000)
			{
				(rfilter + i)->can_id = ((filter[i] | CAN_EFF_FLAG) & (CAN_EFF_MASK | CAN_EFF_FLAG));
				(rfilter + i)->can_mask = (CAN_EFF_FLAG | CAN_RTR_FLAG | mask[i]);
				printf("29 bit mask and filters: %x, %x\n", (rfilter + i)->can_mask, (rfilter + i)->can_id);
			}
			else
			{
				(rfilter + i)->can_id = filter[i];
				(rfilter + i)->can_mask = (CAN_EFF_FLAG | CAN_RTR_FLAG | mask[i] );
				printf("11 bit mask and filters: %x, %x\n", (rfilter + i)->can_mask, (rfilter + i)->can_id);
			}
		}
		printf("sizeof filter = %d\n", sizeof(rfilter));
	}
	else
	{
		ret = E_LIB_MEM_ALLOC_ERROR;
	}
	return ret;
}

int can_write(char *name, char *data)
{
	int s; /* can raw socket */ 
	int required_mtu;
	int mtu;
	int enable_canfd = 1;
	struct sockaddr_can addr;
	struct canfd_frame frame;
	struct ifreq ifr;

	if ((strcmp(name, "can0" )) == 0){
		if (sread_can0 > 0){
			close(sread_can0);
			sread_can0 = 0;
		}
	}

	if ((strcmp(name, "can1" )) == 0){
		if (sread_can1 > 0){
			close(sread_can1);
			sread_can1 = 0;
		}
	}

	if ((strcmp(name, "can2" )) == 0){
		if (sread_can2 > 0){
			close(sread_can2);
			sread_can2 = 0;
		}
	}

	/* parse CAN frame */
	required_mtu = parse_canframe(data, &frame);
	if (!required_mtu){
		fp = fopen("/home/root/can_log.txt", "a");
		fprintf(fp, "%s%s\n" , "Wrong CAN-frame:",data);
		fclose(fp);
		return -1;
	}

	/* open socket */
	if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		perror("socket");
		return -1;
	}

	strncpy(ifr.ifr_name, name, IFNAMSIZ - 1);
	ifr.ifr_name[IFNAMSIZ - 1] = '\0';
	ifr.ifr_ifindex = if_nametoindex(ifr.ifr_name);
	if (!ifr.ifr_ifindex) {
		perror("if_nametoindex");
		return -1;
	}

	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	if (required_mtu > CAN_MTU) {

		/* check if the frame fits into the CAN netdevice */
		if (ioctl(s, SIOCGIFMTU, &ifr) < 0) {
			perror("SIOCGIFMTU");
			return -1;
		}
		mtu = ifr.ifr_mtu;

		if (mtu != CANFD_MTU) {
			printf("CAN interface is not CAN FD capable - sorry.\n");
			return -1;
		}

		/* interface is ok - try to switch the socket into CAN FD mode */
		if (setsockopt(s, SOL_CAN_RAW, CAN_RAW_FD_FRAMES,
					&enable_canfd, sizeof(enable_canfd))){
			printf("error when enabling CAN FD support\n");
			return -1;
		}

		/* ensure discrete CAN FD length values 0..8, 12, 16, 20, 24, 32, 64 */
		frame.len = can_dlc2len(can_len2dlc(frame.len));
	}

	/* disable default receive filter on this RAW socket */
	/* This is obsolete as we do not read from the socket at all, but for */
	/* this reason we can remove the receive list in the Kernel to save a */
	/* little (really a very little!) CPU usage. */
	setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER, NULL, 0);

	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		return -1;
	}
	if(reqst_flag == 0)
	{
		/* send frame */
		if (write(s, &frame, required_mtu) != required_mtu) {
			perror("write");
			printf("request status flag = %d\n", reqst_flag);
			return -1;
		}
		else
		{
			reqst_flag = 1;
		}
	}
	else
	{
		reqst_flag = 0;
		return -1;
	}
	printf("request status flag = %d\n", reqst_flag);
	close(s);

	return 0;
}

int can_read(char *name, struct canfd_frame *frame)
{
	struct ifreq ifr_read;
	struct sockaddr_can addr_read;
	struct canfd_frame frames;
	int ret;
	char ctrlmsg[CMSG_SPACE(sizeof(struct timeval)) + CMSG_SPACE(sizeof(__u32))];
	struct iovec iov;
	struct msghdr msg;
	int nbytes, sread;
	struct timeval timeout = { 0, 0 };

	if (can_lock != SEM_FAILED)
		sem_wait(can_lock);
	if ((strcmp(name, "can0" )) == 0){
		if (sread_can0 <= 0 || reqst_flag == 1)
		{
			close(sread_can0);
			read_init(&sread_can0,&ifr_read,&addr_read_can0, name);
		}
		sread = sread_can0;
		addr_read = addr_read_can0;
	}

	else if ((strcmp(name, "can1" )) == 0){
		if (sread_can1 <= 0 || reqst_flag == 1)
		{
			close(sread_can1);
			read_init(&sread_can1,&ifr_read,&addr_read_can1, name);
		}
		sread = sread_can1;
		addr_read = addr_read_can1;
	}

	else if ((strcmp(name, "can2" )) == 0){
		if ( sread_can2 <= 0 || reqst_flag == 1)
		{
			close(sread_can2);
			read_init(&sread_can2,&ifr_read,&addr_read_can2, name);
		}
		sread = sread_can2;
		addr_read = addr_read_can2;
	}
	else
	{
	}

	iov.iov_base = frame;
	msg.msg_name = &addr_read;
	msg.msg_iovlen = 1;
	msg.msg_control = &ctrlmsg;

	iov.iov_len = sizeof(frames);
	msg.msg_iov = &iov;
	msg.msg_namelen = sizeof(addr_read);
	msg.msg_controllen = sizeof(ctrlmsg);
	msg.msg_flags = 0;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	if( setsockopt( sread, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
				sizeof( timeout ) ) < 0 )
	{
		perror("setsockopt failed\n");
	}
	else
	{
	}

	nbytes = recvmsg( sread, &msg, 0 );
	if (nbytes < 0)
	{
		printf("CAN Read failed with error code %d \n", errno);
		if (can_lock != SEM_FAILED)
			sem_post(can_lock);

		if ((strcmp(name, "can0" )) == 0){
			if (sread_can0 > 0){
				close(sread_can0);
				sread_can0 = 0;
			}
		}

		if ((strcmp(name, "can1" )) == 0){
			if (sread_can1 > 0){
				close(sread_can1);
				sread_can1 = 0;
			}
		}

		if ((strcmp(name, "can2" )) == 0){
			if (sread_can2 > 0){
				close(sread_can2);
				sread_can2 = 0;
			}
		}
		return E_CAN_READ_TIMEOUT;
	}
	else
	{
		if(frame->can_id & CAN_EFF_FLAG)
		{
			frame->can_id = frame->can_id & CAN_EFF_MASK;
		}
		printf("response can_id = %x\n", frame->can_id);

	}

	if(reqst_flag == 1)
	{
		if(sread_can0)
		{
			close(sread_can0);
			sread_can0 = -1;
		}
		if(sread_can1)
		{
			close(sread_can1);
			sread_can1 = -1;
		}
		if(sread_can2)
		{
			close(sread_can2);
			sread_can2 = -1;
		}
		reqst_flag = 0;
	}

	if (can_lock != SEM_FAILED)
		sem_post(can_lock);

exit:
	return frame->len;
}
