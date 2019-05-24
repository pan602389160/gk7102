#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

#include "event_interface.h"
#include "common.h"

static int connect_to_tcp_server(char *ipaddr, int port)
{
	int sockfd = -1;
	struct sockaddr_in seraddr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		printf("[EventManager - Error] socket() error for connect_to_tcp_server failed: %s.\n",
			   strerror(errno));
		return -1;
	}

	seraddr.sin_family = AF_INET;
	seraddr.sin_port = htons(port);
	seraddr.sin_addr.s_addr = inet_addr(ipaddr);

	if (-1 == connect(sockfd, (struct sockaddr *)&seraddr, sizeof(seraddr))) {
		printf("[EventManager - Error] connect to event manager failed: %s.\n", strerror(errno));
		return -1;
	}

	return sockfd;
}

static void *recv_event_thread(void *arg)
{	printf("==========%s==============\n",__func__);
	int ret = 0;
	mozart_event event;
	event_handler *handler = (event_handler *)arg;

	while (1) {
		ret = recv(handler->sockfd, &event, sizeof(event), 0);
		if (ret == sizeof(event)) {
			//printf("==========%s==============1111111\n",__func__);
			if (handler->callback)
				handler->callback(event, handler->param);
		} else if (ret == 0){
			printf("[EventManager - Error] event_manager disconnected(event_manager Dead?), Re-connect after 500ms.\n");
			usleep(500 * 1000);
			if (handler->sockfd)
				close(handler->sockfd);
			handler->sockfd = connect_to_tcp_server(SERVER_ADDR, SERVER_REGISTER_PORT);
		} else {
			printf("[EventManager - Error] recieve data from event_manager failed: %s, Re-try after 500ms.\n",
				   strerror(errno));
			usleep(500 * 1000);
		}
	}

	return NULL;
}

event_handler *mozart_event_handler_get(EventCallback callback, void *param)
{	printf("==========%s==============\n",__func__);
	event_handler *handler = malloc(sizeof(*handler));
	if (!handler) {
		printf("[EentManager - Error] alloc memory for handler failed: %s.\n",
			   strerror(errno));
		return NULL;
	}

	handler->callback = callback;
	handler->param = param;
	handler->sockfd = connect_to_tcp_server(SERVER_ADDR, SERVER_REGISTER_PORT);
	if (handler->sockfd < 0) {
		printf("[EventManager - Error] Can not connect to event manager.\n");
		free(handler);
		return NULL;
	}

	pthread_t recv_event_pthread;
	while (0 != pthread_create(&recv_event_pthread, NULL, recv_event_thread, handler)) {
		printf("[EventManager - Error] create thread for listening event failed: %s, Re-try after 500ms.\n",
			   strerror(errno));
		usleep(500 * 1000);
	}

	pthread_detach(recv_event_pthread); /* FIXME: Never mind error here. may memory leak if occur */

	return handler;
}

int mozart_event_send(mozart_event event)
{
	int sockfd = -1;
	int ret = 0;
	char feedback[2] = "";

	sockfd = connect_to_tcp_server(SERVER_ADDR, SERVER_EVENT_PORT);
	if (sockfd < 0) {
		printf("can not connect to event manager, exit..\n");
		ret = -1;
		goto func_exit;
	}

	ret = send(sockfd, &event, sizeof(event), 0);
	if (ret != sizeof(event)) {
		printf("send event error: %s.\n", strerror(errno));
		ret = -1;
		goto func_exit;
	}

	ret = recv(sockfd, feedback, 2, 0);
	printf("mozart_event_send recv :ret = %d,feedback = %s\n",ret,feedback);
	if (ret == 2) {
		if (feedback[0] == '1') {
			ret = 0;
			goto func_exit;
		} else {
			ret = -1;
			goto func_exit;
		}
	} else {
		ret = -1;
		goto func_exit;
	}

func_exit:
	if (sockfd) {
		close(sockfd);
		sockfd = -1;
	}

	return ret;
}

void mozart_event_handler_put(event_handler *handler)
{
	if (!handler)
		return;

	if (handler->sockfd > 0) {
		close(handler->sockfd);
		handler->sockfd = -1;
	}

	handler->callback = NULL;
	free(handler);
}
