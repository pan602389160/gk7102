#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/stat.h>
#include <linux/input.h>
#include <fcntl.h>
//#include <execinfo.h>
#include <sys/syscall.h>
#include <curl/curl.h>


#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include <net/if.h>
#include <net/route.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <linux/netlink.h>




#include "debug_print.h"

#include "ray_app.h"
//#include "mozart_app.h"

#include "event_interface.h"
//#include "wifi_interface.h"
//#include "volume_interface.h"
#include "player_interface.h"
//#include "tips_interface.h"
#include "json-c/json.h"
#include "utils_interface.h"
//#include "power_interface.h"
#include "sharememory_interface.h"
//#include "ini_interface.h"
//#include "nvrw_interface.h"
//#include "updater_interface.h"
//#include "localplayer_interface.h"


//#include "lapsule_control.h"
//#include "battery_capacity.h"
//#include "ingenicplayer.h"
#include "wifi_interface.h"
#include "network_common.h"
#include "mozart_musicplayer.h"
#include "ray_key_function.h"
#include "vr_interface.h"



int fd_playctr_out_fifo;					//创建有名管道，用于向mplayer发送命令
int fd_playctr_in_fifo;                    //接收播放控制命令


pthread_t wait_wifi_tid;
int wait_wifi_flag = 0;

event_handler *keyevent_handler = NULL;
event_handler *miscevent_handler = NULL;

int KEY_ALL = 0;
int time_cnt1 = 0;
static int null_cnt = 0;


#define NETLINK_GK_IPC      22
#define UEVENT_BUFFER_SIZE  1024

#define MAX_MSG_SIZE        (16-2)
typedef union
{
    unsigned int all[MAX_MSG_SIZE];
    struct
    {
        unsigned char id;
        unsigned char val;
    }gk_nl_gpio_data;
    struct
    {
        unsigned char key;
    }gk_nl_ir_data;
    struct
    {
        unsigned char insert;
    }gk_nl_sd_data;
    struct
    {
        unsigned char data[56];
    }gk_nl_433_data;
    struct
    {
        int link;
        int speed;
        int duplex;
    }gk_nl_eth_data;
}GK_MSG_DATA_S;

typedef struct
{
    unsigned int module;
    unsigned int len;
    GK_MSG_DATA_S data;
}GK_NET_LINK_MSG_S;
typedef enum
{
    GK_NL_GPIO          = 0x0000005A,
    GK_NL_IR            = 0x000000A5,
    GK_NL_IR_433_KEY    = 0x00005A00,
    GK_NL_IR_433_GATE   = 0x0000A500,
    GK_NL_PMU_GPIO      = 0x00005A5A,
    GK_NL_SD            = 0x0000A5A5,
    GK_NL_ETH           = 0x005A0000,
}GK_MSG_TYPE_E;

	const char *wifi_module_str[] = {
		[BROADCOM] = "BROADCOM",
		[REALTEK] = "REALTEK",
		[SOUTHSV] = "SOUTHSV"
	};


	const char *event_type_str[] = {
		[WIFI_MODE] = "WIFI_MODE",
		[STA_STATUS] = "STA_STATUS",
		[AP_STATUS]  =  "AP_STATUS",
		[NETWORK_CONFIGURE] = "NETWORK_CONFIGURE",
		[ETHER_STATUS] = "ETHER_STATE",
		[AP_STATUS] = "AP_STATUS",
		[STA_IP_CHANGE] = "IP_CHANGE",
		[INVALID] = "INVALID",
	};

/**
 * @brief string format of network configure, for readable
 */
	const char *network_configure_status_str[] = {
		[AIRKISS_STARTING] = "AIRKISS_STARTING",
		[AIRKISS_RUNNING] = "AIRKISS_RUNNING",
		[AIRKISS_SUCCESS] = "AIRKISS_SUCCESS",
		[AIRKISS_FAILED] = "AIRKISS_FAILED",
		[AIRKISS_SUCCESS_OVER] = "AIRKISS_SUCCESS_OVER",
		[AIRKISS_FAILED_OVER] = "AIRKISS_FAILED_OVER",
		[AIRKISS_NULL] = "AIRKISS_NULL",
		[AIRKISS_CANCEL] = "AIRKISS_CANCEL",
	};

int mqtt_start_flag = 0;

void *mplayer_rec_ctrmsg(void *arg)
{
	int size = 0;
	char buf[100];
	while(1){		
		size = read(fd_playctr_in_fifo,buf,sizeof(buf));
		printf("mplayer_rec_ctrmsg READ msg = [%s],len = %d\n",buf,size);
		if(write(fd_playctr_out_fifo,buf,strlen(buf))!=strlen(buf))
			perror("write");
	}

}


void *mqttsub_func(void *arg)
{

	char topic1[32] = {0}; 
	char topic2[32] = {0};	
	//memset(macaddr, 0, sizeof(macaddr));
	sprintf(topic1,"/dev/%s","RSPythonTest");
	sprintf(topic2,"/dev/voice/%s","RSPythonTest");
	printf("-----topic1 = %s\n",topic1);
	printf("-----topic2 = %s\n",topic2);
	mqtt_start_flag = 1;
	//start_mqtt("120.24.75.220","61613","admin","password",topic1,topic2);	
	start_mqtt("119.23.207.229","1883","leden","ray123456",topic1,topic2);
	
	return NULL;
}

int create_mqttsub_pthread(void)
{
	printf("----------%s-------------\n",__func__);
	if(mqtt_start_flag)
		return 0;
	pthread_t mqttsub_pthread;
	if (pthread_create(&mqttsub_pthread, NULL, mqttsub_func, NULL) == -1) {
		printf("Create wifi blink pthread failed: %s.\n", strerror(errno));
		return -1;
	}
	pthread_detach(mqttsub_pthread);

	return 0;
}

static int init_hotplug_sock()
{
	const int buffersize = UEVENT_BUFFER_SIZE;
	int ret;

	struct sockaddr_nl snl;
	bzero(&snl, sizeof(struct sockaddr_nl));
	snl.nl_family = AF_NETLINK;
	snl.nl_pid = getpid();
	snl.nl_groups = 1;

	int s = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_GK_IPC);
	if (s == -1)
	{
		perror("socket");
		return -1;
	}
	setsockopt(s, SOL_SOCKET, SO_RCVBUF, &buffersize, sizeof(buffersize));
	ret = bind(s, (struct sockaddr *)&snl, sizeof(struct sockaddr_nl));
	if (ret < 0)
	{
		perror("bind");
		close(s);
		return -1;
	}
	return s;
}

void *timeout_func(void *args)
{
	void *flag = args;
	time_cnt1 = 3;
	*(int *)flag = 1;
	while(time_cnt1){
		time_cnt1--;
		//printf("time_cnt1 = %d\n",time_cnt1);
		usleep(100*1000);
	}
	*(int *)flag = 0;
	return NULL;
}


int press_timeout(int *flag)
{	
	pthread_t timeout;
	if (pthread_create(&timeout, NULL, timeout_func,(void *)flag) == -1) {
		printf("Create press_timeout pthread failed: %s.\n", strerror(errno));
	}
	pthread_detach(timeout);

}

void *wait_wifi_func(void *args)  //是否做网络配置超时呢   ?
{
	printf("--------wait_wifi_func-------------\n");


	wait_wifi_flag = 1;
	while(!saoma_info.flag){
		sleep(1);
	}
	wait_wifi_flag = 0;
	mozart_play_key_sync("/usr/data/tone/recv_wifi.mp3");
	wifi_ctl_msg_t new_mode;

	memset(&new_mode, 0, sizeof(new_mode));
	new_mode.cmd = SW_STA;// SW_STA 2
	
	strcpy(new_mode.param.switch_sta.ssid,saoma_info.saoma_buf_ssid);//saoma_info.saoma_buf_ssid
	strcpy(new_mode.param.switch_sta.psk,saoma_info.saoma_buf_psk);//saoma_info.saoma_buf_psk
	new_mode.force = true;
	new_mode.param.switch_sta.sta_timeout = 6;
	strcpy(new_mode.name, app_name);

	if(request_wifi_mode(new_mode) != true)
		printf("ERROR: [%s SW_STA] Request Network Failed, Please Register First!!!!\n", app_name);

	return NULL;
}


int create_wait_wifi_func()
{

	if(wait_wifi_flag == 1){
		pthread_cancel(wait_wifi_tid);
	}

	if (pthread_create(&wait_wifi_tid, NULL, wait_wifi_func, NULL) == -1) 
	{
		printf("Create saoma_t pthread failed");
	}
	pthread_detach(wait_wifi_tid);

	return 0;
}



void *key_callback_func(void *arg)
{
	int hotplug_sock = init_hotplug_sock();
	int len = 0;
	int i=0;
	GK_NET_LINK_MSG_S msg;
	player_status_t status;
	
	time_t timep ;
	time_t button_5_press_time = 0;
	time_t button_5_release_time = 0;

	time_t button_10_press_time = 0;
	time_t button_10_release_time = 0;

	time_t button_13_press_time = 0;
	time_t button_13_release_time = 0;

	time_t button_17_press_time = 0;
	time_t button_17_release_time = 0;

	time_t button_21_press_time = 0;
	time_t button_21_release_time = 0;

	time_t button_24_press_time = 0;
	time_t button_24_release_time = 0;
	
	while(1)
	{
		/* Netlink message buffer */

		len = recv(hotplug_sock, &msg, sizeof(msg), 0);
		timep = time(NULL);
		#if 1
		if(msg.module == GK_NL_GPIO&&msg.data.gk_nl_gpio_data.val == 0){
			if(KEY_ALL){	
				time_cnt1 = 10;
				continue;
			}
			press_timeout(&KEY_ALL);

		}
		#endif
		switch(msg.module)
		{
		case GK_NL_GPIO:
			printf("PRESS goio%02d-%d  time: %ld\n",msg.data.gk_nl_gpio_data.id,msg.data.gk_nl_gpio_data.val,timep);
			switch(msg.data.gk_nl_gpio_data.id)
			{
			case 22:  //  short_press:pause    long_press: wifi_config
				printf(">>>>>>button 22  [S5] ");   //  下右  S5
				if(msg.data.gk_nl_gpio_data.val == 0)
					button_5_press_time = timep;
				if(msg.data.gk_nl_gpio_data.val == 1){
					button_5_release_time = timep;
					
				if(button_5_release_time - button_5_press_time <= 1 && button_5_press_time > 0){
					printf("short press\n");
					if(mozart_musicplayer_handler)
						status = mozart_player_getstatus(mozart_musicplayer_handler->player_handler);
					printf("[short press pause]  111 status = %d,vr_status = %d\n",status,ray_vr_get_status());

					if (ray_vr_get_status() == VR_ASR) {
						printf("-----ASR mode, interrupt it.....mozart_play_pause\n");
						ray_vr_asr_break();
						system("echo \"quit\" > /tmp/playctr_in_fifo");

					}else{
						if (status == PLAYER_PLAYING){
							printf("cccc\n");
							mozart_musicplayer_play_pause(mozart_musicplayer_handler);
							i = 60;
							while (i--) {
								if (PLAYER_PAUSED == mozart_musicplayer_get_status(mozart_musicplayer_handler))
									break;
								usleep(50*1000);
							}
							mozart_play_key_sync("/usr/data/tone/pause.mp3");
						}else{
							mozart_musicplayer_play_pause(mozart_musicplayer_handler);
						}
					}
						
				}
				if(button_5_release_time - button_5_press_time > 1&&button_5_press_time > 0){
					printf("long press\n");
					if(mozart_musicplayer_handler)
						status = mozart_player_getstatus(mozart_musicplayer_handler->player_handler);
					if (status == PLAYER_PLAYING)
							mozart_musicplayer_play_pause(mozart_musicplayer_handler);
					system("killall wpa_supplicant");
					mozart_play_key_sync("/usr/data/tone/enter_network_config.mp3"); //提示进入网络配置模式
					stopall();
					create_saoma_func(&saoma_info);
					create_wait_wifi_func();
				}
				button_5_press_time = 0;
				button_5_release_time = 0;
				}
				break;
			case 5:   // short_press:volume  -    long_press: pre
				printf(">>>>>>button 5 [S7]");  // 上左  S2
				if(msg.data.gk_nl_gpio_data.val == 0)
					button_10_press_time = timep;
				if(msg.data.gk_nl_gpio_data.val == 1){
					button_10_release_time = timep;
					if(button_10_release_time - button_10_press_time <= 1&&button_10_press_time > 0){
						printf("short press\n");
						mozart_volume_down();
					}
					if(button_10_release_time - button_10_press_time > 1&&button_10_press_time > 0){
						printf("long press\n");
						status = mozart_player_getstatus(mozart_musicplayer_handler->player_handler);
						if (status == PLAYER_PLAYING)
								mozart_musicplayer_play_pause(mozart_musicplayer_handler);
						i = 60;
						while (i--) {
							if (PLAYER_PAUSED == mozart_musicplayer_get_status(mozart_musicplayer_handler))
								break;
							usleep(50*1000);
						}
						mozart_play_key_sync("/usr/data/tone/prev.mp3");	
						mozart_previous_song();
					}
					button_10_press_time = 0;
					button_10_release_time = 0;
				}
				break;
			case 20: // short_press:null  -    long_press: null
				printf(">>>>>>button 20  [**]");//下左 S6
				if(msg.data.gk_nl_gpio_data.val == 0)
					button_13_press_time = timep;
				if(msg.data.gk_nl_gpio_data.val == 1){
					button_13_release_time = timep;
					if(button_13_release_time - button_13_press_time <= 1&&button_13_press_time > 0){
						printf("short press\n");
					}
					if(button_13_release_time - button_13_press_time > 1&&button_13_press_time > 0){
						printf("long press\n");

					}
					button_13_press_time = 0;
					button_13_release_time = 0;
				}
				break;
			case 14: // short_press:null  -    long_press: null
				printf(">>>>>>button 14 [**]"); // 中左 S4
				if(msg.data.gk_nl_gpio_data.val == 0)
					button_17_press_time = timep;
				if(msg.data.gk_nl_gpio_data.val == 1){
					button_17_release_time = timep;
					if(button_17_release_time - button_17_press_time <= 1&&button_17_press_time > 0){
						printf("short press\n");
						
					}
					if(button_17_release_time - button_17_press_time > 1&&button_17_press_time > 0){
						printf("long press\n");

					}
					button_17_press_time = 0;
					button_17_release_time = 0;
				}
				break;
			case 15: // short_press:null  -    long_press: null
				printf(">>>>>>button 15 [**]"); // 中左 S4
				if(msg.data.gk_nl_gpio_data.val == 0)
					button_17_press_time = timep;
				if(msg.data.gk_nl_gpio_data.val == 1){
					button_17_release_time = timep;
					if(button_17_release_time - button_17_press_time <= 1&&button_17_press_time > 0){
						printf("short press\n");
						
					}
					if(button_17_release_time - button_17_press_time > 1&&button_17_press_time > 0){
						printf("long press\n");
			
					}
					button_17_press_time = 0;
					button_17_release_time = 0;
				}
				break;

			case 23: // short_press:null  -    long_press: null
				printf(">>>>>>button 23 [23]"); // 中左 S4
				if(msg.data.gk_nl_gpio_data.val == 0)
					button_17_press_time = timep;
				if(msg.data.gk_nl_gpio_data.val == 1){
					button_17_release_time = timep;
					if(button_17_release_time - button_17_press_time <= 1&&button_17_press_time > 0){
						printf("short press\n");
						
					}
					if(button_17_release_time - button_17_press_time > 1&&button_17_press_time > 0){
						printf("long press\n");
			
					}
					button_17_press_time = 0;
					button_17_release_time = 0;
				}
				break;

			case 21:
				printf(">>>>>>button 21 [S4]"); //short_press:wakeup   long_press: null
				if(msg.data.gk_nl_gpio_data.val == 0)
					button_21_press_time = timep;
				if(msg.data.gk_nl_gpio_data.val == 1){
					button_21_release_time = timep;
					if(button_21_release_time - button_21_press_time <= 1&&button_21_press_time > 0){
						printf("short press\n");
						aec_key_wakeup();
					}
					if(button_21_release_time - button_21_press_time > 1&&button_21_press_time > 0){
						printf("long press\n");

					}
					button_21_press_time = 0;
					button_21_release_time = 0;
				}
				break;
			case 13:
				printf(">>>>>>button 13 [S8]");  // short_press:volum +    long_press: next
				if(msg.data.gk_nl_gpio_data.val == 0)
					button_24_press_time = timep;
				if(msg.data.gk_nl_gpio_data.val == 1){
					button_24_release_time = timep;
					if(button_24_release_time - button_24_press_time <= 1&&button_24_press_time > 0){
						printf("short press\n");
						mozart_volume_up();
					}
					if(button_24_release_time - button_24_press_time > 1&&button_24_press_time > 0){
						printf("long press\n");
						status = mozart_player_getstatus(mozart_musicplayer_handler->player_handler);
						if (status == PLAYER_PLAYING)
								mozart_musicplayer_play_pause(mozart_musicplayer_handler);
						i = 60;
						while (i--) {
							if (PLAYER_PAUSED == mozart_musicplayer_get_status(mozart_musicplayer_handler))
								break;
							usleep(50*1000);
						}
						mozart_play_key_sync("/usr/data/tone/next.mp3");	
						mozart_next_song();
					}
					button_24_press_time = 0;
					button_24_release_time = 0;
				}
				break;


			}
			break;
		case GK_NL_PMU_GPIO:
			printf("GK_NL_PMU_GPIO pmu goio%02d-%d!\n",
				msg.data.gk_nl_gpio_data.id,
				msg.data.gk_nl_gpio_data.val);
			break;
		case GK_NL_IR:
			printf("GK_NL_IR key=%02x!\n", msg.data.gk_nl_ir_data.key);
			break;
		case GK_NL_IR_433_KEY:
			printf("GK_NL_IR_433_KEY:\n");
			for(i=0;i<msg.len;i++)
				printf("0x%02x ", msg.data.gk_nl_433_data.data[i]);
			printf("\n");
			break;
		case GK_NL_IR_433_GATE:
			printf("GK_NL_IR_433_GATE:\n");
			for(i=0;i<msg.len;i++)
				printf("0x%02x ", msg.data.gk_nl_433_data.data[i]);
			printf("\n");
			break;
		case GK_NL_SD:
			printf("GK_NL_SD: %s\n", msg.data.gk_nl_sd_data.insert?"card insert" : "card eject");
			break;
		case GK_NL_ETH:
			if(msg.data.gk_nl_eth_data.link)
			{
				printf("GK_NL_ETH: Link is Up speed:%dMbps duplex:%s\n",
					msg.data.gk_nl_eth_data.speed,
					msg.data.gk_nl_eth_data.duplex ? "full" : "half");
			}
			else
			{
				printf("GK_NL_ETH: Link is Down\n", msg.data.gk_nl_eth_data.link);
			}
			break;
		default :
			printf("net link recieve unkown data:%08x %d!\n", msg.module, msg.len);
			for(i=0;i<msg.len;i++)
				printf("0x%02x ", msg.data.gk_nl_433_data.data[i]);
			printf("\n");
			break;
		}
	}
	return 0;
}

int create_key_callback_pthread(void)
{
	printf("----------%s-------------\n",__func__);

	pthread_t key_callbac_pthread;
	if (pthread_create(&key_callbac_pthread, NULL, key_callback_func, NULL) == -1) {
		printf("Create wifi blink pthread failed: %s.\n", strerror(errno));
		return -1;
	}
	pthread_detach(key_callbac_pthread);

	return 0;
}

void *camera_func(void *args)
{
	mozart_start_camera_rayshine();
	return NULL;
}


int create_camera_pthread(void)
{
	pthread_t camera_pthread;
	if (pthread_create(&camera_pthread, NULL, camera_func, NULL) == -1) {
		printf("Create camera_pthread failed: %s.\n", strerror(errno));
		
		return -1;
	}
	pthread_detach(camera_pthread);
	return 0;
}


void miscevent_callback(mozart_event event, void *param)
{
	switch (event.type) {
	case EVENT_MISC:
		printf("[misc event] %s : %s.\n", event.event.misc.name, event.event.misc.type);
		if (!strcasecmp(event.event.misc.name, "musicplayer")) {
			if (!strcasecmp(event.event.misc.type, "play")) {
				if (mozart_module_stop())
					share_mem_set(MUSICPLAYER_DOMAIN, RESPONSE_CANCEL);
				else
					share_mem_set(MUSICPLAYER_DOMAIN, RESPONSE_DONE);
			} else if (!strcasecmp(event.event.misc.type, "playing")) {
				printf("music player playing.\n");

			} else if (!strcasecmp(event.event.misc.type, "pause")) {
				printf("music player paused.\n");
			} else if (!strcasecmp(event.event.misc.type, "resume")) {
				printf("music player resume.\n");
			} else {
				printf("unhandle music event: %s.\n", event.event.misc.type);
			}
		}   else {
			printf("Unhandle event: %s-%s.\n", event.event.misc.name, event.event.misc.type);
		}
		break;
	case EVENT_KEY:
	default:
		break;
	}

	return;
}




int network_callback(const char *p)
{
	pthread_t sync_time_pthread;
	struct json_object *wifi_event = NULL;
	char test[64] = {0};
	wifi_ctl_msg_t new_mode;
	event_info_t network_event;
	player_status_t status;
	
	purple_pr("[%s] network event: %s\n", __func__, p);
	wifi_event = json_tokener_parse(p);
	//purple_pr("event.to_string()=%s\n", json_object_to_json_string(event));

	memset(&network_event, 0, sizeof(event_info_t));
	struct json_object *tmp = NULL;
	json_object_object_get_ex(wifi_event, "name", &tmp);
	if(tmp != NULL){
		strncpy(network_event.name, json_object_get_string(tmp), strlen(json_object_get_string(tmp)));
		//printf("name:%s\n", json_object_get_string(tmp));
	}

	json_object_object_get_ex(wifi_event, "type", &tmp);
	if(tmp != NULL){
		strncpy(network_event.type, json_object_get_string(tmp), strlen(json_object_get_string(tmp)));
		//printf("type:%s\n", json_object_get_string(tmp));
	}

	json_object_object_get_ex(wifi_event, "content", &tmp);
	if(tmp != NULL){
		strncpy(network_event.content, json_object_get_string(tmp), strlen(json_object_get_string(tmp)));
		//printf("content:%s\n", json_object_get_string(tmp));
	}

	memset(&new_mode, 0, sizeof(wifi_ctl_msg_t));
	if (!strncmp(network_event.type, event_type_str[STA_STATUS], strlen(event_type_str[STA_STATUS]))) {
		//printf("[%s]: %s\n", network_event.type, network_event.content);
		null_cnt = 0;
		if(!strncmp(network_event.content, "STA_CONNECT_STARTING", strlen("STA_CONNECT_STARTING"))){
			mozart_play_key_sync("/usr/data/tone/network_connecting.mp3");    //正在连接
		}

		if(!strncmp(network_event.content, "STA_CONNECT_FAILED", strlen("STA_CONNECT_FAILED"))){
			printf("======>>>>>STA_CONNECT_FAILED\n");
			json_object_object_get_ex(wifi_event, "reason", &tmp);
			if(tmp != NULL){
				strncpy(test, json_object_get_string(tmp), strlen(json_object_get_string(tmp)));
				printf("STA_CONNECT_FAILED REASON:%s\n", json_object_get_string(tmp));
			}
			{
				mozart_play_key_sync("/usr/data/tone/network_connect_fail.mp3");  //网络连接失败,试试下一个网络
				new_mode.cmd = SW_NEXT_NET;
				new_mode.force = true;
				strcpy(new_mode.name, app_name);
				new_mode.param.network_config.timeout = -1;
				memset(new_mode.param.network_config.key, 0, sizeof(new_mode.param.network_config.key));
				if(request_wifi_mode(new_mode) != true)
					printf("ERROR: [%s SW_NEXT_NET] Request Network Failed, Please Register First!!!!\n", app_name);
			}
		}
		if(!strncmp(network_event.content, "STA_SCAN_OVER", strlen("STA_SCAN_OVER"))){
			//全部试完连不上，需要重新配网
			printf("======>>>>>STA_SCAN_OVER\n");
			new_mode.cmd = SW_NETCFG;
			new_mode.force = true;
			strcpy(new_mode.name, app_name);
			new_mode.param.network_config.timeout = -1;
			memset(new_mode.param.network_config.key, 0, sizeof(new_mode.param.network_config.key));
			new_mode.param.network_config.method |= COOEE;
			strcpy(new_mode.param.network_config.wl_module, wifi_module_str[BROADCOM]);
			if(request_wifi_mode(new_mode) != true)
				printf("ERROR: [%s SW_NETCFG] Request Network Failed, Please Register First!!!!\n", app_name);
			//语音提示进入网络配置模式
			mozart_play_key_sync("/usr/data/tone/please_enter_network_config.mp3"); 
			
		}
	} else if (!strncmp(network_event.type, event_type_str[NETWORK_CONFIGURE], strlen(event_type_str[NETWORK_CONFIGURE]))) {
		//printf("[%s]: %s\n", network_event.type, network_event.content);
		null_cnt = 0;
		if(!strncmp(network_event.content, network_configure_status_str[AIRKISS_STARTING], strlen(network_configure_status_str[AIRKISS_STARTING]))) {
			//mozart_play_key("airkiss_config");
		}

		if(!strncmp(network_event.content, network_configure_status_str[AIRKISS_FAILED], strlen(network_configure_status_str[AIRKISS_FAILED]))) {
			//mozart_play_key("airkiss_config_fail");
			json_object_object_get_ex(wifi_event, "reason", &tmp);
			if(tmp != NULL){
				strncpy(test, json_object_get_string(tmp), strlen(json_object_get_string(tmp)));
				printf("NETWORK CONFIGURE REASON:%s\n", json_object_get_string(tmp));
			}
			new_mode.cmd = SW_STA;
			new_mode.force = true;
			strcpy(new_mode.name, app_name);
			if(request_wifi_mode(new_mode) != true)
				printf("ERROR: [%s SW_AP] Request Network Failed, Please Register First!!!!\n", app_name);
		} else if (!strncmp(network_event.content, network_configure_status_str[AIRKISS_SUCCESS], strlen(network_configure_status_str[AIRKISS_SUCCESS]))) {
			//mozart_play_key("airkiss_config_success");
			{
#if 0
				json_object_object_get_ex(wifi_event, "ssid", &tmp);
				if(tmp != NULL){
					strncpy(test, json_object_get_string(tmp), strlen(json_object_get_string(tmp)));
					printf("ssid:%s\n", json_object_get_string(tmp));
				}
				json_object_object_get_ex(wifi_event, "passwd", &tmp);
				if(tmp != NULL){
					strncpy(test, json_object_get_string(tmp), strlen(json_object_get_string(tmp)));
					printf("passwd:%s\n", json_object_get_string(tmp));
				}
				json_object_object_get_ex(wifi_event, "ip", &tmp);
				if(tmp != NULL){
					strncpy(test, json_object_get_string(tmp), strlen(json_object_get_string(tmp)));
					printf("ip:%s\n", json_object_get_string(tmp));
				}
#endif
			}
		} else if(!strncmp(network_event.content, network_configure_status_str[AIRKISS_CANCEL], strlen(network_configure_status_str[AIRKISS_CANCEL]))) {
			//mozart_play_key("airkiss_config_quit");
		}
	} else if (!strncmp(network_event.type, event_type_str[WIFI_MODE], strlen(event_type_str[WIFI_MODE]))) {
		printf("---------------------network_callback---->>> network_event.type = WIFI_MODE\n");
		wifi_info_t infor;
		infor = get_wifi_mode();
		
#if 0
		struct wifi_client_register wifi_info;
		memset(&new_mode, 0, sizeof(new_mode));
		new_mode.cmd = 2;// SW_STA 2
		new_mode.force = true;
		strcpy(new_mode.name, "rayshine");
		
			request_wifi_mode(new_mode);
#endif		

		if (infor.wifi_mode == AP) {

		} else if (infor.wifi_mode == STA) {
			if(mozart_musicplayer_handler)
				status = mozart_player_getstatus(mozart_musicplayer_handler->player_handler);
			if (status == PLAYER_UNKNOWN){
				null_cnt = 0;
				mozart_play_key_sync("/usr/data/tone/network_connected.mp3"); 
				create_camera_pthread();	
				create_mqttsub_pthread();
				startall(1);
			}
		}else if (infor.wifi_mode == WIFI_NULL) {
			//此处提示就要进行配网了

		}else if (infor.wifi_mode == STA_WAIT){
			
		}else {
			printf("[ERROR]: Unknown event type!!!!!!\n");
		}
	} else if (!strncmp(network_event.type, event_type_str[AP_STATUS], strlen(event_type_str[AP_STATUS]))) {
		//printf("[%s]: %s\n", network_event.type, network_event.content);
		if(!strncmp(network_event.content, "AP-STA-CONNECTED", strlen("AP-STA-CONNECTED"))) {
			printf("\nThe client has the connection is successful.\n");
		} else if (!strncmp(network_event.content, "AP-STA-DISCONNECTED", strlen("AP-STA-DISCONNECTED"))) {
			printf("\nThe client has been disconnected.\n");
		}
	} else if (!strncmp(network_event.type, event_type_str[STA_IP_CHANGE], strlen(event_type_str[STA_IP_CHANGE]))) {
		printf("[WARNING] STA IP ADDR HAS CHANGED!!\n");
	} else {
		printf("Unknown Network Events-[%s]: %s\n", network_event.type, network_event.content);
	}

	json_object_put(wifi_event);
	return 0;
}



int main(int argc, char **argv)
{
	red_pr("               #######             ########\n");
	red_pr("              #######             ########\n");
	red_pr("             #######             ########\n");
	red_pr("            #######             ########\n");
	red_pr("           #######             ########\n");
	red_pr("          #######             ########\n");
	red_pr("         #######             ########\n");
	red_pr("        #######             ########\n");
	red_pr("       #######             ########\n");
	red_pr("      #######             ########\n");
	red_pr("     #######             ########\n");
	red_pr("     ##########################\n");
	red_pr("     #######################\n");
	red_pr("     #####################                                 @ Rayshine \n\n");
	printf(">>>>>make time:%s-%s\n",__DATE__,__TIME__);


	app_name = "gk_test";
	struct wifi_client_register wifi_info;
	wifi_ctl_msg_t new_mode;
	

	create_key_callback_pthread();
	miscevent_handler = mozart_event_handler_get(miscevent_callback, app_name);


	app_initialize();
	//create_saoma_func(&saoma_info);
	
	memset(&wifi_info, 0, sizeof(wifi_info));
	wifi_info.pid = getpid();
	register_to_networkmanager(wifi_info, network_callback);

	#if 0   //开机wifi连网的，有线调试下暂时不开
	// register network manager
	memset(&wifi_info, 0, sizeof(wifi_info));
	wifi_info.pid = getpid();
	if(register_to_networkmanager(wifi_info, network_callback) != 0) {
		printf("ERROR: [%s] register to Network Server Failed!!!!\n", app_name);
	} else if(!access("/usr/data/wpa_supplicant.conf", R_OK)) {
		memset(&new_mode, 0, sizeof(new_mode));
		new_mode.cmd = SW_STA;// SW_STA 2
		new_mode.force = true;
		new_mode.param.switch_sta.sta_timeout = 6;
		strcpy(new_mode.name, app_name);

		if(request_wifi_mode(new_mode) != true)
			printf("ERROR: [%s SW_STA] Request Network Failed, Please Register First!!!!\n", app_name);

	}
	#endif

	
	//create_camera_pthread();	   //摄像头功能
	//tmp_get_y_picture();       //测试抓拍y 图片
	//func_turing_readbook();    //  图灵绘本的测试

	printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
	unlink("/tmp/playctr_out_fifo");
	unlink("/tmp/playctr_in_fifo");
	mkfifo("/tmp/playctr_out_fifo",O_CREAT|0666);
	mkfifo("/tmp/playctr_in_fifo",O_CREAT|0666);
	perror("mkfifo");

	startall();
	//create_mqttsub_pthread();
	
	while(1) {

		sleep(60);
		system("echo 3 > /proc/sys/vm/drop_caches");
	}

	return 0;

}














