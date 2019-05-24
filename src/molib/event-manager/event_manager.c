#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>
#include <pthread.h>
#include <linux/input.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <linux/netlink.h>
#include <sys/types.h>
#include "event_interface.h"
#include "common.h"

#define EVENT_FD_SIZE		32	/* size of listen fd */
#define CONN_FD_SIZE		32	/* size of connect fd */
#define UEVENT_MSG_LEN 1024

struct uevent {
	const char *action;
	const char *path;
	const char *subsystem;
	const char *firmware;
	int major;
	int minor;
};

static void openKeyboard();
static void closeKeyboard();
static void eventLoop(void);

static int event_fd[EVENT_FD_SIZE] = {[0 ... EVENT_FD_SIZE-1] = -1};
static int conn_fd[CONN_FD_SIZE] = {[0 ... CONN_FD_SIZE-1] = -1};

static int device_fd = -1;
static int max_fd = -1;

static int start_tcp_server(char *ipaddr, int port)
{
	int sockfd;
	int opt = 1;
	struct sockaddr_in seraddr;

	memset(&seraddr, 0, sizeof(struct sockaddr_in));
	seraddr.sin_family = AF_INET;
	seraddr.sin_port = htons(port);
	seraddr.sin_addr.s_addr = inet_addr(ipaddr); //htonl(INADDR_ANY);

	while ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("[EventManager - Error] socket() for processing event hanlder failed: %s,"
			   "Re-try after 500ms.\n", strerror(errno));
		usleep(500 * 1000);
	}

	if (-1 == setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
		printf("[EventManager - Warning] set SO_REUSEADDR failed, Ignore it.\n");

	if (-1 == bind(sockfd, (struct sockaddr *)&seraddr, sizeof(seraddr))) {
		printf("[EventManager - Error] bind for event handler socket failed: %s,"
			   "kernel uevent support is disabled!!!\n", strerror(errno));
		close(sockfd);
		return -1;
	}

	return sockfd;
}

static int distribute_event(mozart_event event)
{
	int i = 0;

	for (i = 0; i < CONN_FD_SIZE; i++) {
		if (conn_fd[i] >= 0)
			send(conn_fd[i], &event, sizeof(event), 0); /* Ignore error here. */
	}

	return 0;
}

static int openevent(char *dev)
{
	int fd = 0;
	int i = 0;

	fd = open(dev, O_RDWR);
	if (fd < 0) {
		printf("[EventManager - Warning] open %s failed: %s, try next.\n",
			   dev, strerror(errno));
	} else {
		for (i = 0; i < EVENT_FD_SIZE; i++) {
			if (event_fd[i] < 0) {
				printf("[EventManager - info] new input device added: %s\n", dev);
				event_fd[i++] = fd;
				break;
			}
		}
		if (i == EVENT_FD_SIZE) {
			printf("[EventManager - Warning] Too many file opened already, Will not monitor more.\n");
			return -1;
		}
	}

	return 0;
}

static void openKeyboard(void)
{
	struct dirent *ptr = NULL;
	char eventfile[512] = "";

	DIR *dir = opendir("/dev/input");
	if (!dir) {
		printf("[EventManager - Warning] open /dev/input failed: %s,"
			   "key event support is disabled!!!\n", strerror(errno));
		return;
	}

	while (NULL != (ptr = readdir(dir))) {
		if (0 == strncmp(ptr->d_name, "event", 5)) {
			memset(eventfile, 0, sizeof(eventfile));
			sprintf(eventfile, "/dev/input/%s", ptr->d_name);
			openevent(eventfile);
		}
	}

	closedir(dir);
	return;
}

static int openUeventSocket(void)
{
	int i = 0;
	int s = -1;
	int ret = 0;
	int buffersize = 1024;
	struct sockaddr_nl addr;

	memset(&addr, 0, sizeof(addr));
	addr.nl_family = AF_NETLINK;
	addr.nl_pid = getpid();
	addr.nl_groups = 0xffffffff;

	while ((s = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT)) == -1) {
		printf("[EventManager - Error] socket() for uevent failed: %s,"
			   "try after 500ms.\n", strerror(errno));
		usleep(500 * 1000);
	}

	if (-1 == setsockopt(s, SOL_SOCKET, SO_RCVBUFFORCE, &buffersize, sizeof(buffersize))) {
		printf("[EventManager - Error] SO_RCVBUFFORCE failed: %s,"
			   "kernel uevent support is disabled!!!\n", strerror(errno));
		ret = -1;
		goto err_exit;
	}

	if (-1 == bind(s, (struct sockaddr *)&addr, sizeof(addr))) {
		printf("[EventManager - Error] bind for kernel uevent failed: %s,"
			   "kernel uevent support is disabled!!!\n", strerror(errno));
		ret = -1;
		goto err_exit;
	}

	for (i = 0; i < EVENT_FD_SIZE; i++) {
		if (event_fd[i] < 0) {
			event_fd[i] = s;
			break;
		}
	}
	device_fd = s;

	return ret;

err_exit:
	if (s >= 0)
		close(s);
	return ret;
}

static void closeKeyboard()
{
	int i = 0;

	for (i = 0; i < EVENT_FD_SIZE; i++) {
		if (event_fd[i] > 0) {
			close(event_fd[i]);
			event_fd[i] = -1;
		}
	}
}

static int handle_device(int fd)
{
	int i = 0;
	char msg[UEVENT_MSG_LEN + 2] = {};
	char cred_msg[CMSG_SPACE(sizeof(struct ucred))] = {};
	struct sockaddr_nl snl;
	struct iovec iov = {msg, sizeof(msg)};
	struct msghdr hdr = {&snl, sizeof(snl), &iov, 1, cred_msg, sizeof(cred_msg), 0};
	mozart_event event;

	ssize_t n = recvmsg(fd, &hdr, 0);
	if (n == 0) { /* peer has performed an orderly shutdown */
		printf("[EventManager - Error] kernel uevent server shutdown,"
			   "kernel uevent support will disable soon!!!\n");
		return EBADFD;
	} else if (n < 0 || n >= UEVENT_MSG_LEN) { /* recv error || recv overflow */
		printf("[EventManager - Error] recvmsg kernel uevent failed: %s,"
			   "Abandon this time.\n", strerror(errno));
		return -1;
	}

	/* ignoring non-kernel netlink multicast message */
	if ((snl.nl_groups != 1) || (snl.nl_pid != 0)) {
		printf("[EventManager - Error] bad kernel uevent,"
			   "Abandon this time.\n");
		return -1;
	}

	/* get the whole sting: replace all '\0\ to '\n' */
	for (i = 0; i < n; i++)
		if (*(msg + i) == '\0')
			msg[i] = '\n';

	memset(&event, 0, sizeof(event));
	event.type = EVENT_MISC;
	if (strstr(msg, "change@/devices/platform/jz-linein")) { // linein in M150
		strcpy(event.event.misc.name, "linein");
		if (strstr(msg, "LINEIN_STATE=IN") != NULL)
			strcpy(event.event.misc.type, "plugin");
		else if (strstr(msg, "LINEIN_STATE=OUT"))
			strcpy(event.event.misc.type, "plugout");
	} else if (strstr(msg, "change@/devices/virtual/switch/linein")) { // linein in X1000
		strcpy(event.event.misc.name, "linein");
		if (strstr(msg, "SWITCH_STATE=1") != NULL)
			strcpy(event.event.misc.type, "plugin");
		else if (strstr(msg, "SWITCH_STATE=0"))
			strcpy(event.event.misc.type, "plugout");
	} else if (strstr(msg, "change@/devices/virtual/switch/h2w")) {
		strcpy(event.event.misc.name, "headset");
		if (strstr(msg, "SWITCH_STATE=1") != NULL)
			strcpy(event.event.misc.type, "plugin");
		else if (strstr(msg, "SWITCH_STATE=0"))
			strcpy(event.event.misc.type, "plugout");
	} else if (strstr(msg, "change@/devices/virtual/switch/spdif")) {
		strcpy(event.event.misc.name, "spdif");
		if (strstr(msg, "SWITCH_STATE=1") != NULL)
			strcpy(event.event.misc.type, "plugin");
		else if (strstr(msg, "SWITCH_STATE=0"))
			strcpy(event.event.misc.type, "plugout");
	} else if (strstr(msg, "add@/devices/platform/jzmmc.1") ||
			   strstr(msg, "add@/devices/platform/jzmmc_v1.2.0")) {
		strcpy(event.event.misc.name, "tfcard");
		strcpy(event.event.misc.type, "plugin");
	} else if (strstr(msg, "remove@/devices/platform/jzmmc.1") ||
			   strstr(msg, "remove@/devices/platform/jzmmc_v1.2.0")) {
		strcpy(event.event.misc.name, "tfcard");
		strcpy(event.event.misc.type, "plugout");
	} else if (strstr(msg, "add@/devices/platform/jz-dwc2/dwc2/usb1/1-1/1-1:1.0/host") && strstr(msg, "/block/sd")) {
		strcpy(event.event.misc.name, "udisk");
		strcpy(event.event.misc.type, "plugin");
	} else if (strstr(msg, "remove@/devices/platform/jz-dwc2/dwc2/usb1/1-1/1-1:1.0/host") && strstr(msg, "/block/sd")) {
		strcpy(event.event.misc.name, "udisk");
		strcpy(event.event.misc.type, "plugout");
	} else if (strstr(msg, "change@/devices/virtual/android_usb/android0")) {
		strcpy(event.event.misc.name, "usb");
		if (strstr(msg, "USB_STATE=CONNECTED"))
			strcpy(event.event.misc.type, "connected");
		else if (strstr(msg, "USB_STATE=DISCONNECTED"))
			strcpy(event.event.misc.type, "disconnected");
		else if (strstr(msg, "USB_STATE=CONFIGURED"))
			strcpy(event.event.misc.type, "configured");
	} else if (strstr(msg,"change@/devices/platform/jz-dwc2")){
		strcpy(event.event.misc.name, "uaudio");
		if (strstr(msg, "USB_STATE=CONNECTED")){
			strcpy(event.event.misc.type, "connected");
		}else if (strstr(msg, "USB_STATE=DISCONNECTED")){
			strcpy(event.event.misc.type, "disconnected");
		}
	} else if (strstr(msg, "add@/devices/virtual/input/")) { /* new input device added, select it */
		char new_input_dev[32] = {};
		char *tmp = NULL;
		int num = 0;
		tmp = strstr(msg, "DEVNAME=input/event");
		if (tmp) {
			sscanf(tmp + strlen("DEVNAME=input/event"), "%d", &num);
			sprintf(new_input_dev, "/dev/input/event%d", num);
			num = 100;
			while (num--) {
				if (!access(new_input_dev, 0)) {
					openevent(new_input_dev);
					break;
				}
				usleep(10 *1000);
			}
			if (num <= 0)
				printf("[EventManager - Error] %s NOT create after 1 Second\n", new_input_dev);
		}
	} else if (!strncmp(msg, "change@", 7)) {
		char *tmp = strrchr(msg, '/');
		if (tmp && !strncmp(tmp, "/battery", 8)) {
			strcpy(event.event.misc.name, "battery");
			strcpy(event.event.misc.type, "change");
		} else {
			return 0;
		}
	} else {
#if 0 // ONLY for debug.
		printf("Unsupport uevent, Ignore it.\n");
		printf("\n\n---------msg--------\n"
			   "%s"
			   "\n----------msg-------\n\n", msg);
#endif
		return 0;
	}

	return distribute_event(event);
}

static bool keyevent_is_valid(struct input_event event)
{
	if (event.type == EV_KEY || event.type == EV_REL || event.type == EV_ABS)
		return true;
	return false;
}

static int handle_key(int fd)
{
	int i = 0;
	struct mozart_event event;

	event.type = EVENT_KEY;
	if (read(fd, &event.event.key.key, sizeof(event.event.key.key)) !=
		sizeof(event.event.key.key)) {
		if (errno == ENODEV) {
			close(fd);
			for (i = 0; i < EVENT_FD_SIZE; i++) {
				if (event_fd[i] == fd) {
					event_fd[i] = -1;
					break;
				}
			}
		} else {
			printf("[EventManager - Error] read keyboard failed: %s,"
				   "Abandon this time.\n", strerror(errno));
		}
		return -1;
	}

	if (keyevent_is_valid(event.event.key.key)) {
#if 0 // ONLY for debug.
		printf("type: %d, code: %d, value: %d.\n",
			   event.event.key.key.type,
			   event.event.key.key.code,
			   event.event.key.key.value);
#endif
		return distribute_event(event);
	} else {
		return 0;
	}
}

static int handle_fd(int fd)
{
#if 0 // ONLY for debug.
	printf("fd %d can be read.\n", fd);
#endif
	if (device_fd >= 0 && fd == device_fd)
		return handle_device(fd);
	else
		return handle_key(fd);
}

void eventLoop(void)
{
	fd_set rfds;
	int i = 0;
	int ret = 0;

	while (1) {
		FD_ZERO(&rfds);
		for (i = 0; i < EVENT_FD_SIZE; i++) {
			if (event_fd[i] >= 0) {
				FD_SET(event_fd[i], &rfds);
			} else {
				break;
			}
		}

		max_fd = -1;
		for (i = 0; i < EVENT_FD_SIZE; i++) {
			if (event_fd[i] > 0) {
				max_fd = event_fd[i] > max_fd ? event_fd[i] : max_fd;
			}
		}

#if 0 // ONLY for debug.
		printf("max_fd = %d.\n", max_fd);
		printf("device_fd = %d.\n", device_fd);
		for (i = 0; i < EVENT_FD_SIZE; i++)
			if (event_fd[i] > 0)
				printf("event[%d] = %d.\n", i, event_fd[i]);
#endif

		ret = select(max_fd + 1, &rfds, NULL, NULL, NULL);
		if (ret < 0) {
			printf("[EventManager - Error] select() for key_manager failed: %s,"
				   "Re-try after 500ms.\n", strerror(errno));
			usleep(500 * 1000);
			continue;
		}

		for (i = 0; i < EVENT_FD_SIZE; i++) {
			if (event_fd[i] >= 0 && FD_ISSET(event_fd[i], &rfds))
				if (handle_fd(event_fd[i]) == EBADFD) {
					close(event_fd[i]);
					event_fd[i] = -1;
				}
		}
	}
}

void *process_handler_request(void *data)
{
	int i = 0;
	int sockfd = -1, connfd = -1;
	struct sockaddr_in cliaddr;
	socklen_t addrlen = sizeof(struct sockaddr_in);

	sockfd = start_tcp_server(SERVER_ADDR, SERVER_REGISTER_PORT);

	if (sockfd == -1) {
		printf("start tcp server failed, Protocol event support is disabled!!!.\n");
		return NULL;
	}

	if (-1 == listen(sockfd, 10)) {
		printf("[EventManager - Error] listen for handle handler request failed: %s,"
			   "Protocol event support is disabled!!!.\n", strerror(errno));
		close(sockfd);
		return NULL;
	}

	while (1) {
		connfd = accept(sockfd, (struct sockaddr *)&cliaddr, &addrlen);
		if (-1 == connfd) {
			printf("[EventManager - Warning] accept recv protocol connect failed: %s,"
				   "Ignore this time!!!.\n", strerror(errno));
			continue;
		}

		for (i = 0; i < CONN_FD_SIZE; i++) {
			if (conn_fd[i] < 0) {
				conn_fd[i] = connfd;
				break;
			}
		}
		if (i == CONN_FD_SIZE) {
			printf("[EventManager - Warning] Too many connection already, Will not accept new request.\n");
			break;
		}
	}

	for (i = 0; i < CONN_FD_SIZE; i++) {
		if (conn_fd[i] > 0)
			close(conn_fd[i]);
	}
	close(sockfd);
	return NULL;
}

static void *process_protocol_event(void *args)
{
	int sockfd = -1;
	int connfd = -1;
	int ret = 0;
	struct mozart_event event;
	struct sockaddr_in cliaddr;
	socklen_t addrlen = sizeof(struct sockaddr_in);

	sockfd = start_tcp_server(SERVER_ADDR, SERVER_EVENT_PORT);

	if (-1 == listen(sockfd, 10)) {
		printf("[EventManager - Error] listen for recv protocol event failed: %s,"
			   "Protocol event support is disabled!!!.\n", strerror(errno));
		return NULL;
	}

	while (1) {
		connfd = accept(sockfd, (struct sockaddr *)&cliaddr, &addrlen);
		if (-1 == connfd) {
			printf("[EventManager - Warning] accept recv protocol connect failed: %s,"
				   "Ignore this time!!!.\n", strerror(errno));
			continue;
		}
		ret = read(connfd, &event, sizeof(event));
		if (ret != sizeof(event)) {
			if (ret == 0) {
				printf("[EventManager - Warning ] connection peer shutdown, close it.\n");
				continue;
			} else {
				printf("[EventManager - Error] recv protocol failed: %s,"
					   "Abandon this time.\n", strerror(errno));
				write(connfd, "0", 2);
				continue;
			}
		}

		/* feedback the process result */
		write(connfd, "1", 2);
		close(connfd);

		distribute_event(event);
	}

	return NULL;

}

static char *signal_str[] = {
	[1] = "SIGHUP",       [2] = "SIGINT",       [3] = "SIGQUIT",      [4] = "SIGILL",      [5] = "SIGTRAP",
	[6] = "SIGABRT",      [7] = "SIGBUS",       [8] = "SIGFPE",       [9] = "SIGKILL",     [10] = "SIGUSR1",
	[11] = "SIGSEGV",     [12] = "SIGUSR2",     [13] = "SIGPIPE",     [14] = "SIGALRM",    [15] = "SIGTERM",
	[16] = "SIGSTKFLT",   [17] = "SIGCHLD",     [18] = "SIGCONT",     [19] = "SIGSTOP",    [20] = "SIGTSTP",
	[21] = "SIGTTIN",     [22] = "SIGTTOU",     [23] = "SIGURG",      [24] = "SIGXCPU",    [25] = "SIGXFSZ",
	[26] = "SIGVTALRM",   [27] = "SIGPROF",     [28] = "SIGWINCH",    [29] = "SIGIO",      [30] = "SIGPWR",
	[31] = "SIGSYS",      [34] = "SIGRTMIN",    [35] = "SIGRTMIN+1",  [36] = "SIGRTMIN+2", [37] = "SIGRTMIN+3",
	[38] = "SIGRTMIN+4",  [39] = "SIGRTMIN+5",  [40] = "SIGRTMIN+6",  [41] = "SIGRTMIN+7", [42] = "SIGRTMIN+8",
	[43] = "SIGRTMIN+9",  [44] = "SIGRTMIN+10", [45] = "SIGRTMIN+11", [46] = "SIGRTMIN+12", [47] = "SIGRTMIN+13",
	[48] = "SIGRTMIN+14", [49] = "SIGRTMIN+15", [50] = "SIGRTMAX-14", [51] = "SIGRTMAX-13", [52] = "SIGRTMAX-12",
	[53] = "SIGRTMAX-11", [54] = "SIGRTMAX-10", [55] = "SIGRTMAX-9",  [56] = "SIGRTMAX-8", [57] = "SIGRTMAX-7",
	[58] = "SIGRTMAX-6",  [59] = "SIGRTMAX-5",  [60] = "SIGRTMAX-4",  [61] = "SIGRTMAX-3", [62] = "SIGRTMAX-2",
	[63] = "SIGRTMAX-1",  [64] = "SIGRTMAX",
};

void sig_handler(int signo)
{
	char cmd[64] = {};
	printf("\n\n[%s: %d] event_manager crashed by signal %s.\n", __func__, __LINE__, signal_str[signo]);

	if (signo == SIGSEGV || signo == SIGBUS ||
		signo == SIGTRAP || signo == SIGABRT) {
		sprintf(cmd, "cat /proc/%d/maps", getpid());
		printf("Process maps:\n");
		system(cmd);
	}

	closeKeyboard();

	exit(-1);
}
#if 0
int main(int argc, char *argv[])
{
	pthread_t process_request_thread;
	pthread_t process_protocol_thread;;

	// signal hander,
	signal(SIGINT, sig_handler);
	signal(SIGUSR1, sig_handler);
	signal(SIGUSR2, sig_handler);
	signal(SIGTERM, sig_handler);
	signal(SIGBUS, sig_handler);
	signal(SIGSEGV, sig_handler);
	signal(SIGABRT, sig_handler);
	signal(SIGPIPE, SIG_IGN);

	openKeyboard();
	openUeventSocket();

	while (0 != pthread_create(&process_request_thread, NULL, process_handler_request, NULL)) {
		printf("[EventManager - Error] create thread for key_manager failed: %s,"
			   "Re-try after 500ms.\n", strerror(errno));
		usleep(500 * 1000);
	}
	pthread_detach(process_request_thread); /* Never mind error here. may memory leak if occur */

	while (0 != pthread_create(&process_protocol_thread, NULL, process_protocol_event, NULL)) {
		printf("[EventManager - Error] create thread for key_manager failed: %s,"
			   "Re-try after 500ms.\n", strerror(errno));
		usleep(500 * 1000);
	}
	pthread_detach(process_protocol_thread); /* Never mind error here. may memory leak if occur */

	eventLoop();

	return 0;
}
#endif
