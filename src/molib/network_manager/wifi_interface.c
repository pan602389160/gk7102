#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <pthread.h>
#include "network_common.h"
#include "wifi_interface.h"

#include <json-c/json.h>


typedef struct param_from_client {
	int pid;
	int (*ptr)(const char *p);
} PARAM;

static int sockfd = -1;
static struct param_from_client param;

int connect_to_networkmanager(void)
{
	int sockfd;
	int len,result;
	struct sockaddr_un address;

	if((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
		perror("socket");
		return -1;
	}

	address.sun_family = AF_UNIX;
	strcpy(address.sun_path, "/var/run/wifi-server/server_socket");
	len = sizeof(address);

	result = connect(sockfd, (struct sockaddr *)&address,len);
	if(result == -1){
		if(access("/var/run/wifi-server/server_socket", F_OK) != 0){
			printf("[Warning]: Wait for Server socket Creating!!!\n");
			usleep(500 * 1000);
			if(connect(sockfd, (struct sockaddr *)&address,len) == -1){
				perror("connect");
				return -1;
			}
		}
		else{
			perror("connect");
			return -1;
		}
	}
	return sockfd;
}

bool request_wifi_mode(wifi_ctl_msg_t msg)
{
	int sockfd;
	int byte;
	bool status = false;

	sockfd = connect_to_networkmanager();
	if(sockfd == -1){
		printf("[ERROR]: Client connect to Network Manager Server Failed!!!!!\n");
		return false;
	}
	printf("request_wifi_mode 111\n");
	if((byte = write(sockfd, &msg, sizeof(msg))) == -1){
		printf("request_wifi_mode write err\n");
		close(sockfd);
		return false;
	}
	printf("request_wifi_mode 222\n");
	//printf("????????????request_wifi_mode------->byte = %d,sizeof(wifi_ctl_msg_t) = %d\n",byte,sizeof(wifi_ctl_msg_t));
	if((byte = read(sockfd, &status, sizeof(status))) == -1){
		printf("?????????? read error\n");
		perror("read");
		close(sockfd);
		return false;
	}
	printf("request_wifi_mode 333  status = %d\n",status);
	close(sockfd);

	return status;
}

wifi_info_t get_wifi_mode(void)
{
	
	int sockfd;
	int byte;
	wifi_info_t infor;

	wifi_ctl_msg_t check_mode;

	check_mode.cmd = CHECKMODE;
	strcpy(check_mode.name, "Duang");

	sockfd = connect_to_networkmanager();
	//printf("??????get_wifi_mode --->sockfd = %d,sizeof(wifi_ctl_msg_t) = %d\n",sockfd,sizeof(wifi_ctl_msg_t));
	//system("ls /var/run/wifi-server");
	if((byte = write(sockfd, &check_mode, sizeof(check_mode))) == -1){
		
		perror("write");
		infor.wifi_mode = WIFI_NULL;
		goto exit;
	}
	//printf("??????get_wifi_mode ---->byte = %d\n",byte);
	
	
	if((byte = read(sockfd, &infor, sizeof(infor))) == -1){
		perror("read");
		infor.wifi_mode = WIFI_NULL;
		goto exit;
	}
	//printf("***************************\n");
exit:
	close(sockfd);
	return infor;
}

int connect_to_registerserver(void)
{
	int sockfd;
	int len,result;
	struct sockaddr_un address;

	if((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
		perror("socket");
		return -1;
	}

	address.sun_family = AF_UNIX;
	strcpy(address.sun_path, "/var/run/wifi-server/register_socket");
	len = sizeof(address);

	result = connect(sockfd, (struct sockaddr *)&address,len);
	if(result == -1){
		if(access("/var/run/wifi-server/register_socket", F_OK) != 0){
			printf("[Warning]: Wait for Register socket Creating!!!\n");
			usleep(500 * 1000);
			if(connect(sockfd, (struct sockaddr *)&address,len) == -1){
				perror("connect");
				return -1;
			}
		}
		else{
			perror("connect");
			return -1;
		}
	}
	return sockfd;
}

void *manage_network_change(void *arg)
{
	int ret_value;
	int msqid;
	key_t key;
	wifi_notify_msg_t msg_recv;

	key = ftok("/tmp",1);
	if(key < 0){
		printf("To obtain key error\n");
		perror("ftok");
	}

	memset(msg_recv.msgtext, 0, sizeof(msg_recv.msgtext));
	msqid=msgget(key,IPC_EXCL);  /*   检查消息队列是否存在*/
	if(msqid < 0){
		printf("=====>msg is not exis and need create it\n");
		msqid = msgget(key,IPC_CREAT|0666);/*   创建消息队列*/
		if(msqid <0){
			printf("failed to create msq | errno=%d [%s]\n",errno,strerror(errno));
			return NULL;
		}
	}
	while(1){
		//printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
		ret_value = msgrcv(msqid, &msg_recv, sizeof(wifi_notify_msg_t), param.pid, 0);
		//printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
		//printf("@########Recv changing network status msg from Server########@\n");
		if ( ret_value < 0 ) {
			printf("msgrcv() read msg failed,errno=%d[%s]\n",errno,strerror(errno));
			return NULL;
		}
		if(param.ptr)
			param.ptr(msg_recv.msgtext);
	}
	return 0;
}

int register_to_networkmanager(WIFI_CLI_REG register_info,int (*ptr)(const char *p))
{
	printf("--------------------register_to_networkmanager----------------------\n");

	int sockfd;
	int byte;
	bool status = false;
	pthread_t recv_server_thread;
	sockfd = connect_to_registerserver();
	if(sockfd == -1){
		printf("ERROR: Client connect to Network Register Server Failed!!!!!\n");
		return -1;
	}

	if((byte = write(sockfd, &register_info, sizeof(register_info))) == -1){
		perror("write");
		return -1;
	}

	if((byte = read(sockfd, &status, sizeof(status))) == -1){
		perror("read");
		close(sockfd);
		return -1;
	} else {
		if (status == false) {
			printf("ERROR: Client register to Network Server Failed, Please Re-registration!!!!!\n");
			close(sockfd);
			return -1;
		}
	}
	close(sockfd);
	param.pid = register_info.pid;
	param.ptr = ptr;

	if (pthread_create(&recv_server_thread, NULL, manage_network_change,NULL) != 0) {
		printf("Can't create manage_network_change: %s\n",strerror(errno));
		return -1;
	}
	pthread_detach(recv_server_thread);

	return 0;
}

void unregister_from_networkmanager(void)
{
	param.ptr = NULL;

	if(sockfd > 0){
		close(sockfd);
		sockfd = -1;
	}
}

char *get_wifi_scanning_results(void)
{
	FILE *fp = NULL;

	char *info;
	char buf[512] = {};
	char bssid[17] = {};
	char ssid[64] = {};
	json_object *wifi[1024];

	int i = 0, j = 0, size = 0, channel = 0, signal = 0;
	json_object *wifilist, *result;
	if (system("iwlist scan >/tmp/wifilist 2>/dev/null")){
		printf("iwlist scan failed.\n");
		return NULL;
	}
	fp = fopen("/tmp/wifilist", "rb");
	if (fp == NULL) {
		printf("open %s error: %s\n", "/tmp/wifilist", strerror(errno));
		return NULL;
	}

	result = json_object_new_object();
	wifilist = json_object_new_array();
	while(fgets(buf, sizeof(buf), fp)){

		buf[strlen(buf)] = '\0';
		// find a new wifi, alloc a new json_object data.
		if (strstr(buf, "Address: ")) {
			wifi[i] = json_object_new_object();
			memset(bssid, 0, 17);
			strncpy(bssid, strstr(buf, "Address: ") + 9, 17);
			//printf("bssid = %s\n", bssid);
			json_object_object_add(wifi[i], "Bssid", json_object_new_string(bssid));
		}

		//parse ssid.
		if (strstr(buf, "ESSID:\"")) {
			memset(ssid, 0, 64);
			size = strlen(strstr(buf, "ESSID:\"")) - strlen("ESSID:\"") - strlen("\"\n");
			strncpy(ssid, strstr(buf, "ESSID:") + strlen("ESSID:\""), size);
			//printf("ssid = %s\n", ssid);
			json_object_object_add(wifi[i], "SSID", json_object_new_string(ssid));
		}

		//parse channel
		if (strstr(buf, "Channel")) {
			channel = atoi(strstr(buf, "Channel ") + strlen("Channel "));
			json_object_object_add(wifi[i], "Channel", json_object_new_int(channel));
		}

		//parse signal level
		if (strstr(buf,"Signal level:")) {
			signal = atoi(strstr(buf,"Signal level:") + strlen("Signal level:"));
			json_object_object_add(wifi[i], "Signal level", json_object_new_int(signal));
			json_object_array_add(wifilist, wifi[i]);
			i++;
		}
	}

	json_object_object_add(result, "WiFiList", wifilist);
	info = (char *)malloc(strlen(json_object_to_json_string(result)));
	//printf("result.to_string()=%s\n", json_object_to_json_string(result));
	strncpy(info, json_object_to_json_string(result), strlen(json_object_to_json_string(result)));

	json_object_put(result);

	fclose(fp);
	return info;
}
