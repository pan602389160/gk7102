#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define DEBUG(x,y...)	//(printf("DEBUG [ %s : %s : %d] "x"\n",__FILE__, __func__, __LINE__, ##y))
#define ERROR(x,y...)	(printf("ERROR [ %s : %s : %d] "x"\n", __FILE__, __func__, __LINE__, ##y))

#define SERVER_REGISTER_PORT		65430
#define SERVER_EVENT_PORT		65431
#define SERVER_ADDR		"127.0.0.1"

#endif // __COMMON_H__
