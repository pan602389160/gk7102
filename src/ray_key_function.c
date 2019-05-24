#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/input.h>
#include <fcntl.h>

#include "sharememory_interface.h"
#include "ray_key_function.h"
#include "ray_module_vr.h"
#include "mozart_musicplayer.h"
#include "player_interface.h"

player_handler_t *tiny_handler = NULL;
player_status_t tiny_status;
char tiny_uuid[64] = "tiny_play";
int quit_flag = 0;
int music_vol = 90;

const char *snd_source_str[] = {
	[SND_SRC_CLOUD] = "cloud_music",
	[SND_SRC_BT_AVK] = "bluetooth",
	[SND_SRC_LOCALPLAY] = "localplay",
};


void mozart_previous_song(void)
{
	memory_domain domain;
	int ret = -1;

	ret = share_mem_get_active_domain(&domain);
	if (ret) {
		printf("get active domain error in %s:%s:%d, do nothing.\n",
		       __FILE__, __func__, __LINE__);
		return;
	}

	switch (domain) {
	case MUSICPLAYER_DOMAIN:
		mozart_musicplayer_prev_music(mozart_musicplayer_handler);
		break;
	case UNKNOWN_DOMAIN:
		printf("[%s, %d]: idle mode\n", __func__, __LINE__);
		break;
	default:
		if (domain > 0 && domain < MAX_DOMAIN)
			printf("[%s, %d]: %s domain is active\n", __func__, __LINE__,
			       memory_domain_str[domain]);
		else
			printf("[%s, %d]: Not support domain is active\n", __func__, __LINE__);
		break;
	}
}

void mozart_next_song(void)
{
	memory_domain domain;
	int ret = -1;

	ret = share_mem_get_active_domain(&domain);
	if (ret) {
		printf("get active domain error in %s:%s:%d, do nothing.\n",
		       __FILE__, __func__, __LINE__);
		return;
	}

	switch (domain) {
	case MUSICPLAYER_DOMAIN:
		mozart_musicplayer_next_music(mozart_musicplayer_handler);
		break;
	case UNKNOWN_DOMAIN:
		printf("[%s, %d]: idle mode\n", __func__, __LINE__);
		break;
	default:
		if (domain > 0 && domain < MAX_DOMAIN)
			printf("[%s, %d]: %s domain is active\n", __func__, __LINE__,
			       memory_domain_str[domain]);
		else
			printf("[%s, %d]: Not support domain is active\n", __func__, __LINE__);
		break;
	}
}

void mozart_play_pause(void)
{
	memory_domain domain;
	int ret = -1;

	ret = share_mem_get_active_domain(&domain);
	if (ret) {
		printf("get active domain error in %s:%s:%d, do nothing.\n",
		       __FILE__, __func__, __LINE__);
		return;
	}

	switch (domain) {
	case UNKNOWN_DOMAIN:
		printf("system is in idle mode in %s:%s:%d.\n",
		       __FILE__, __func__, __LINE__);
#if (SUPPORT_LOCALPLAYER == 1)
		if (snd_source != SND_SRC_LINEIN) {
			printf("start localplayer playback...\n");
			mozart_module_local_music_startup();
		}
#endif
		break;
	case MUSICPLAYER_DOMAIN:
		if (ray_vr_get_status() == VR_ASR) {
			printf("ASR mode, interrupt it.\n");
			ray_vr_asr_break();
		}

		mozart_musicplayer_play_pause(mozart_musicplayer_handler);
		break;
	default:
		if (domain > 0 && domain < MAX_DOMAIN)
			printf("[%s, %d]: %s domain is active\n", __func__, __LINE__,
			       memory_domain_str[domain]);
		else
			printf("[%s, %d]: Not support domain is active\n", __func__, __LINE__);
		break;
	}
}

int mozart_module_pause(void)
{
	printf("========mozart_module_pause=========\n");
	int ret = -1;
	int i = 60;
	memory_domain domain;
	module_status status;

	ret = share_mem_get_active_domain(&domain);
	if (ret) {
		printf("get active domain error in %s:%s:%d, do nothing.\n",
		       __FILE__, __func__, __LINE__);
		return 0;
	}
	printf("========mozart_module_pause=========domain = %d,get status = %d\n",
		domain,mozart_musicplayer_get_status(mozart_musicplayer_handler));
	
	domain = MUSICPLAYER_DOMAIN;
	
	switch (domain) {
	case MUSICPLAYER_DOMAIN:
		if (mozart_musicplayer_get_status(mozart_musicplayer_handler) == PLAYER_PLAYING)
			mozart_musicplayer_play_pause(mozart_musicplayer_handler);
		while (i--) {
			if (PLAYER_PAUSED == mozart_musicplayer_get_status(mozart_musicplayer_handler))
				break;
			usleep(50*1000);
		}
		if (i == 0)
			printf("pause musicplayer timeout(>1s).\n");
		break;

	case UNKNOWN_DOMAIN:
		printf("[%s, %d]: idle mode\n", __func__, __LINE__);
		break;
	default:
		if (domain > 0 && domain < MAX_DOMAIN)
			printf("[%s, %d]: %s domain is active\n", __func__, __LINE__,
			       memory_domain_str[domain]);
		else
			printf("[%s, %d]: Not support domain is active\n", __func__, __LINE__);
		break;
	}
	return 0;
}


int mozart_module_stop(void)
{
	int ret = 0;
	memory_domain domain;

	ret = share_mem_get_active_domain(&domain);
	if (ret) {
		printf("get active domain error in %s:%s:%d, do nothing.\n",
		       __FILE__, __func__, __LINE__);
		return -1;
	}



	switch (domain) {
	case MUSICPLAYER_DOMAIN:
		mozart_musicplayer_stop(mozart_musicplayer_handler);
		ret = 0;
		break;

	case UNKNOWN_DOMAIN:
			printf("[%s, %d]: idle mode\n", __func__, __LINE__);
		return 0;
	default:
		if (domain > 0 && domain < MAX_DOMAIN) {
			printf("[%s, %d]: %s domain is active\n", __func__, __LINE__,
			       memory_domain_str[domain]);
			ret = 0;
		} else {
			printf("[%s, %d]: Not support domain is active\n", __func__, __LINE__);
			ret = -1;
		}
		break;
	}

	//if (ret == 0)
		//ret = mozart_check_dsp();

	return ret;
}


void mozart_volume_set(int set_vol)
{
	memory_domain domain;
	int ret = -1;

	ret = share_mem_get_active_domain(&domain);
	if (ret) {
		printf("get active domain error in %s:%s:%d, do nothing.\n",
		       __FILE__, __func__, __LINE__);
		return;
	}

	music_vol = set_vol;

	switch (domain) {
	case MUSICPLAYER_DOMAIN:
		mozart_musicplayer_set_volume(mozart_musicplayer_handler, music_vol);
		break;
	default:
		//mozart_volume_set(vol, MUSIC_VOLUME);
		break;
	}
}


void mozart_volume_up(void)
{
	int vol = 0;
	char vol_buf[8] = {};
	memory_domain domain;
	int ret = -1;

	ret = share_mem_get_active_domain(&domain);
	if (ret) {
		printf("get active domain error in %s:%s:%d, do nothing.\n",
		       __FILE__, __func__, __LINE__);
		return;
	}
#if 0
	if (mozart_ini_getkey("/usr/data/system.ini", "volume", "music", vol_buf)) {
		printf("failed to parse /usr/data/system.ini, set music volume to 20.\n");
		vol = 20;
	} else {
		vol = atoi(vol_buf);
	}

	if (vol == 100) {
		printf("max volume already, do nothing.\n");
		return;
	}
#endif
	
	music_vol += 4;
	if (music_vol > 100)
		music_vol = 100;
	else if(music_vol < 60)
		music_vol = 60;

	switch (domain) {
	case MUSICPLAYER_DOMAIN:
		mozart_musicplayer_set_volume(mozart_musicplayer_handler, music_vol);
		break;
	default:
		mozart_volume_set(80);
		break;
	}
}


void mozart_volume_down(void)
{
	int vol = 0;
	char vol_buf[8] = {};
	memory_domain domain;
	int ret = -1;

	ret = share_mem_get_active_domain(&domain);
	if (ret) {
		printf("get active domain error in %s:%s:%d, do nothing.\n",
		       __FILE__, __func__, __LINE__);
		return;
	}
#if 0
	if (mozart_ini_getkey("/usr/data/system.ini", "volume", "music", vol_buf)) {
		printf("failed to parse /usr/data/system.ini, set music volume to 20.\n");
		vol = 20;
	} else {
		vol = atoi(vol_buf);
	}

	if (vol == 0) {
		printf("min volume already, do nothing.\n");
		return;
	}
#endif
	music_vol -= 4;
	if(music_vol > 100)
		music_vol = 100;
	else if(music_vol < 60)
		music_vol = 60;

	switch (domain) {
	case MUSICPLAYER_DOMAIN:
		mozart_musicplayer_set_volume(mozart_musicplayer_handler, music_vol);
		break;
	default:
		mozart_volume_set(80);
		break;
	}
}


int tiny_callback(player_snapshot_t *snapshot, struct player_handler *handler, void *param)
{
	printf("%s uuid: %s, status: %s, pos is %d, duration is %d.\n",__func__,
			snapshot->uuid, player_status_str[snapshot->status], snapshot->position, snapshot->duration);

	switch (snapshot->status) {
		case PLAYER_TRANSPORT:
		case PLAYER_PLAYING:
		case PLAYER_PAUSED:
		case PLAYER_UNKNOWN:
			break;
		case PLAYER_STOPPED:
			if (!strcmp(handler->uuid, snapshot->uuid)) {
				if(!quit_flag) {
					printf("PLAYER_STOPPED received, replay file!\n");
					//mozart_player_playurl(handler, file);
				}else {
					printf("end!\n");
					quit_flag = 0;
				}
			}
			break;
		default:
			break;
	}

	return 0;
}


int mozart_play_key_sync(char *name)
{
	#if 0
	char buf[128] = {0};
	int size = 0;
	int fd_fifo;
	char msg[128] = {0};
	
	fd_fifo=open("/tmp/playctr_in_fifo",O_RDWR);
	if(fd_fifo < 0){
			printf("open failed\n");
			return -1;
	}
	memset(msg,0,sizeof(msg));
	sprintf(msg,"loadfile %s",name);
	msg[strlen(msg)]='\n';
	printf("send---->> mozart_play_key_sync msg = [%s],len = %d\n",msg,strlen(msg));
	write(fd_fifo,msg,strlen(msg));
		//perror("write fd_fifo");
	
	close(fd_fifo);
	#endif
	if(!name)
		return -1;
	char cmd [512] = {0};
	sprintf(cmd,"mplayer -quiet -slave %s -input file=/tmp/playctr_in_fifo",name);
	//sprintf(cmd,"echo \"loadfile %s\" > /var/run/playctr_in_fifo",name);
	printf("---->mozart_play_key_sync cmd  = %s\n",cmd);
	system(cmd);
	return 0;
}








