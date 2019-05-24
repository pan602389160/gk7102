#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>


//#include "tips_interface.h"

#include "ray_key_function.h"
//#include "mozart_tts.h"

#include "mozart_musicplayer.h"
//#include "modules/mozart_module_cloud_music.h"
#include "sharememory_interface.h"
#include "ray_module_vr.h"
#include <json-c/json.h>

#define PORT 12477


static char *artists[] = {"成语故事"};//{"???Ф???","????","????"};

//{"寓言故事","成语故事","十二生肖的故亿,"名人伿,"少儿百科","童话故事","中国历史","国学经典"}


static int mozart_module_cloud_music_start(void)
{
	if (snd_source != SND_SRC_CLOUD) {
		if (mozart_module_stop())
			return -1;
		if (mozart_musicplayer_start(mozart_musicplayer_handler))
			return -1;
		snd_source = SND_SRC_CLOUD;
	} else if (!mozart_musicplayer_is_active(mozart_musicplayer_handler)) {
		if (mozart_musicplayer_start(mozart_musicplayer_handler))
			return -1;
	} else {
		mozart_musicplayer_musiclist_clean(mozart_musicplayer_handler);
	}

	return 0;
}

void *local_search(void *args)
{	
	printf("==============%s==============\n", __func__);
    setvbuf(stdout, NULL, _IONBF, 0);   
    fflush(stdout);  

  
    struct sockaddr_in addrto;  
    bzero(&addrto, sizeof(struct sockaddr_in));  
    addrto.sin_family = AF_INET;  
    addrto.sin_addr.s_addr = htonl(INADDR_ANY);  
    addrto.sin_port = htons(PORT);  
      
 


	struct sockaddr_in reply_addr;
	json_object *reply_object = NULL;
	json_object *id = NULL;
	json_object *mac = NULL;
	json_object *type = NULL;

	char macaddr[] = "255.255.255.255";
	memset(macaddr, 0, sizeof(macaddr));
	get_mac_addr("eth0", macaddr, "");

	char buf[16] = {0};
	sprintf(buf,"%c%c:%c%c:%c%c:%c%c:%c%c:%c%c",
		macaddr[0],macaddr[1],macaddr[2],macaddr[3],macaddr[4],macaddr[5],
		macaddr[6],macaddr[7],macaddr[8],macaddr[9],macaddr[10],macaddr[11]);
	printf("-------------------->>>>buf = %s\n",buf);
	int reply_fd;
	int send_cnt;
	
    int sock = -1;  
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)   
    {     
        printf("socket error\n");   
        return -1;  
    } 

    const int opt = 1;  

    int nb = 0;  
    nb = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *)&opt, sizeof(opt));  
    if(nb == -1)  
    {  
        printf("set socket error...\n");  
        return -1;  
    }  
  
    if(bind(sock,(struct sockaddr *)&(addrto), sizeof(struct sockaddr_in)) == -1)   
    {     
        printf("bind error...\n");  
        return -1;  
    }  
  
    int len = sizeof(struct sockaddr_in);  
    char smsg[100] = {0};  
  	char send_msg[] = "{\"id\": \"RSPythonTest\",\"mac\": \"3e:97:0e:22:e1:14\", \"type\": \"ESP32\"}";
    while(1)  
    {  
       
        int ret=recvfrom(sock, smsg, 100, 0, (struct sockaddr*)&addrto,(socklen_t*)&len);  
        if(ret<=0)  
        {  
            printf("read error....\n");  
        }  
        else  
        {   
        	printf("local_search recv from [%s:%d] : %s\n",inet_ntoa( addrto.sin_addr),ntohs(addrto.sin_port),smsg);
			memset(&reply_addr, 0, sizeof(reply_addr));
			reply_addr.sin_family=AF_INET;  
			reply_addr.sin_port = htons(12477);//htons(ntohs(addrto.sin_port));  htons(PORT)
			reply_addr.sin_addr.s_addr = inet_addr(inet_ntoa( addrto.sin_addr));//inet_addr(inet_ntoa( addrto.sin_addr));	

		//printf("IP : %s,%d\n",inet_ntoa( addrto.sin_addr),PORT);
			#if 0
			reply_object = json_object_new_object();
			if (!reply_object)
				continue;
			json_object_object_add(reply_object, "id", json_object_new_string("RSPythonTest"));
			json_object_object_add(reply_object, "mac", json_object_new_string(buf));
			json_object_object_add(reply_object, "type", json_object_new_string("story.gk7102"));

			char *reply_string = strdup(json_object_get_string(reply_object));
			printf("len = %d,reply_string = %s\n",strlen(reply_string),reply_string);
			#endif
			if((reply_fd = socket(AF_INET,SOCK_DGRAM,0)) < 0){  
					printf("create sock error!\n"); 
					continue;
			}


			send_cnt = sendto(reply_fd,send_msg,sizeof(send_msg),0,(struct sockaddr *)&reply_addr,sizeof(reply_addr));
			if(send_cnt)
				printf("reply %d !!\n",send_cnt);
			free(reply_object);
        }  
  
        sleep(1);  
    }		
	return NULL;
}



int creat_local_search()
{
	pthread_t wx_deviceId_find_t;

	if(pthread_create(&wx_deviceId_find_t,NULL,local_search,NULL) == -1)
	{
		printf("pthread_create check_deviceId_t failed\n");
		return -1;
	}
	pthread_detach(wx_deviceId_find_t);

	printf("creat_wx_deviceId_find recvfrom  OVER ####################\n");
	return 0;

}


/* TODO: cache url */
int mozart_module_cloud_music_startup(void)
{
	memory_domain domain;
	int ret;
	char buf[1024] = {};
	int idx = random() % (sizeof(artists) / sizeof(artists[0]));

	creat_local_search();
	
	printf("==========mozart_module_cloud_music_startup=====PLAY: %s\n",artists[idx]);
	return speech_cloudmusic_play(artists[idx]);


}

