#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

#include <assert.h>

#include "ray_module_vr.h"
#include "duilite.h"
#include "asoundlib.h"
#include "ray_app.h"
bool vr_started = false;


int start(app_t app)
{
	switch(app)
	{
		case MUSICPLAYER:
			mozart_musicplayer_startup();
			
			break;
		case VR:
			if (ray_vr_init(process_vr_callback)) {
				printf("init error, vr disabled!!\n");
				return -1;
			}	
			sleep(1);
			if (ray_vr_start()) {  //需要等待开机连上网络开始播放开机资源才开始
				printf("start error, vr disabled!!\n");
				ray_vr_uninit();
				return -1;
			}
			
			vr_started = true;
			
		default:
			break;
	}

	return 0;
}


int stop(app_t app)
{
	
	switch(app)
	{
		case MUSICPLAYER:
			mozart_musicplayer_shutdown();
			break;
		case VR:

			ray_vr_uninit();
			vr_started = false;
			break;
		default:
			break;
	}


	return 0;
}


int startall()
{
	
	start(MUSICPLAYER);
	start(VR);
	mozart_module_cloud_music_startup();  //这里需要   ray_vr_init 注册的回调函数
	
	
	return 0;
}


int stopall()
{
	printf("======stopall=========\n");
	stop(MUSICPLAYER);
	stop(VR);

	printf("======stopall=========ZZZZZZZZZZZZZZ\n");
	return 0;
}


