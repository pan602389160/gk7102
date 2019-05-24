#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if 0
#define DEBUG(x,y...)	(printf("DEBUG [ %s : %s : %d] "x"\n",__FILE__, __func__, __LINE__, ##y))
#else
#define DEBUG(x,y...)
#endif
#define ERROR(x,y...)	(printf("ERROR [ %s : %s : %d] "x"\n", __FILE__, __func__, __LINE__, ##y))


int get_unix_serv_socket(char *un_path)
{
	struct sockaddr_un un;
	int sockfd;
	int opt = 1;

	if (!un_path) {
		ERROR("un_path is NULL");
		return -1;
	}

	if (-1 == (sockfd = socket(PF_UNIX, SOCK_STREAM, 0))) {
		ERROR("socket:%s", strerror(errno));
		return -1;
	}

	if (-1 == setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
		ERROR("setsockopt:%s", strerror(errno));
		close(sockfd);
		return -1;
	}

	unlink(un_path);
	un.sun_family = AF_UNIX;
	strcpy(un.sun_path, un_path);

	if (-1 == bind(sockfd, (struct sockaddr *)&un, sizeof(struct sockaddr_un))) {
		ERROR("bind:%s", strerror(errno));
		close(sockfd);
		return -1;
	}

	return sockfd;
}


int connect_to_unix_socket(char *un_path)
{
	struct sockaddr_un un;
	int sockfd;

	if (-1 == (sockfd = socket(PF_UNIX, SOCK_STREAM, 0))) {
		return -1;
	}

	un.sun_family = AF_UNIX;
	strcpy(un.sun_path, un_path);

	if (-1 == connect(sockfd, (struct sockaddr *)&un, sizeof(struct sockaddr_un))) {
		close(sockfd);
		return -1;
	}
	return sockfd;
}

