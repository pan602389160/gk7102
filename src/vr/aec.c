#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <json-c/json.h>

#include <assert.h>

#include "duilite.h"
#include "asoundlib.h"
#include "gk_app.h"
#include "ai_vr.h"
#include "debug_print.h"

#include "adi_types.h"
#include "adi_sys.h"
#include "adi_audio.h"
#include "adi_venc.h"

#include "basetypes.h"

#include "adi_audio.h"
#include "adi_sys_error.h"
#include "adi_types.h"
#include "kws.h"


static bool aec_stop_flag = false;
static bool aec_running = false;

static bool auth_flag = false;

char *aec_cfg = "{\"resBinPath\":\"/usr/data/aec/aec_20180510_v0.9.4.bin\",\"channels\": 2, \"micNum\": 1, \"sampleFormat\": 16}";
char *wakeup_cfg = "{\"resBinPath\":\"/usr/data/wakeup_aifar_comm_20180104.bin\"}";
char *wakeup_param = "{\"env\": \"words=ni hao xiao le;thresh=0.06;major=1;\"}";
//char *auth_cfg ="{\"productId\":\"278576719\",\"savedProfile\":\"/usr/data/provision\"}";
char *auth_cfg ="{\"productId\":\"278579663\",\"savedProfile\":\"/usr/data/provision\"}";

//char *auth_cfg = "{\"productId\":\"278579663\",\"productKey\":\"c933f76b362a938e02b116b99789b202\",\"productSecret\":\"146263a252081f641894b2192df06173\"}";


static int buf_index = 0;
static int inside_loop = 0;
static int inside_index = 0;
static int outside_loop = 0;
static int loop_index = 0;
#if 1
char transfor_aec_buf[1600] = {0};
char aec_rec_buf[3200] = {0};
#else
char transfor_aec_buf[320] = {0};
char aec_rec_buf[640] = {0};
#endif

struct duilite_wakeup *wakeup = NULL;
struct pcm_config in_aec_config;
struct pcm *in_aec_pcm;



struct duilite_echo *echo = NULL;

/*
{
	"version": "wakeup_aifar_comm_20180104.bin",
	"lib_version": "1.4.6",
	"vad_version": "wakeup_aifar_vad.v0.6",
	"status": 1,
	"wakeupWord": "ni hao xiao le",
	"major": 1,
	"confidence": 0.065517,
	"frame": 163,
	"wakeup_frame": 32,
	"vprint": 0,
	"words": {
		"ni hao xiao le": 0.0500
	}
}

*/

static int wakeup_callback(void *user_data, int type, char *msg, int len) 
{
	//printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>wakeup_callback>>%.*s\n", len, msg);
	printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>wakeup_callback>>\n");
	json_object *out = NULL;
    json_object *result = NULL;
    json_object *status = NULL;
	out = json_tokener_parse((char*) msg);
	if (!out)
    {
        return -1;
    }
	if (json_object_object_get_ex(out, "status", &status))
	{
		if (json_object_get_int(status) == 1)
		{
			ai_mutex_lock();
			ai_vr_info.from    = VR_FROM_AEC;
			ai_vr_info.aec.wakeup = true;
			if (ai_vr_callback)
				ai_vr_callback(&ai_vr_info);  //callback_from_vr
			ai_mutex_unlock();
		}
	}
	json_object_put(out);
	//aec_stop_flag = true;
	return 0;
}

static int echo_callback(void *user_data, int type, char *msg, int len) 
{
	
	if (type == DUILITE_MSG_TYPE_JSON) 
	{
		printf("%.*s\n", len, msg);
	} else {
		
		duilite_wakeup_feed(wakeup, msg, len);
	}
	return 0;
}



int ray_wakeup_new()
{
	int ret = -1;
	ret = duilite_library_load(auth_cfg);
	if(ret){
		purple_pr("---->>>>??????????????? duilite_library_load FAILED\n");
		return -1;
	}
	printf("---->>>> duilite_library_load SUCCESSFULLY\n");
	
	wakeup = duilite_wakeup_new(wakeup_cfg, wakeup_callback, NULL);
	if(wakeup)
		return 0;
	else
		return -1;

}

int ray_echo_start()
{
	echo = duilite_echo_new(aec_cfg, echo_callback, NULL);
	if(echo)
		return 0;
	else
		return -1;
}


int ray_wakeup_delete()
{
	duilite_library_release();
	if(wakeup)
		duilite_wakeup_delete(wakeup);
	return 0;
}

int aec_start()
{
   //speech wakeup
	if(wakeup){
		duilite_wakeup_start(wakeup, wakeup_param);
		duilite_echo_start(echo, NULL);
		return 0;
	}else{
		return -1;
	}

    
	return 0;
}

int aec_stop()
{
	if(in_aec_pcm){
		pcm_close(in_aec_pcm);
		in_aec_pcm = NULL;
	}
	if(echo)
		duilite_echo_stop(echo);
	if(wakeup)
		duilite_wakeup_stop(wakeup);

	return 0;
}

int ray_aec_delete()
{
	if(echo){
		duilite_echo_delete(echo);
	}
	return 0;
}


#if 1
static int aec_run() 
{
	printf("------------aec_run-------------------\n");

	
	//echo = duilite_echo_new(aec_cfg, aec_callback, NULL);
	unsigned int size;
	int ret = -1;
	int cnt = 0;
	aec_stop_flag = false;
	aec_running = true;
	
    in_aec_config.channels = 2;
	in_aec_config.rate = 16000;
	in_aec_config.period_size = 400;  //320
	in_aec_config.period_count = 4;
	in_aec_config.format = PCM_FORMAT_S16_LE;
	in_aec_config.start_threshold = 0;
	in_aec_config.stop_threshold = 0;
	in_aec_config.silence_threshold = 0;
	
	in_aec_pcm = pcm_open(0, 0, PCM_IN, &in_aec_config);
	printf("in_aec_pcm = %p\n",in_aec_pcm);
	if (!in_aec_pcm || !pcm_is_ready(in_aec_pcm)) {
		//printf("--------------------->  IN_AEC_pcm_open FAILED\n");
		fprintf(stderr, "Unable to open PCM device (%s)\n",pcm_get_error(in_aec_pcm));
		return NULL;
	}else{
		//printf("--------------------->  IN_AEC_pcm_open SUCCESSFULLY\n");
	}


	aec_start();
	purple_pr("==================> AEC RECORD: Please wakeup me.\n");

	while(!aec_stop_flag){		
		memset(aec_rec_buf,0,sizeof(aec_rec_buf));
		memset(transfor_aec_buf,0,sizeof(transfor_aec_buf));
		
		ret = pcm_read(in_aec_pcm, aec_rec_buf, sizeof(aec_rec_buf));
		//duilite_echo_feed(echo, aec_rec_buf, sizeof(aec_rec_buf));

		
		for(buf_index = 0;buf_index < sizeof(aec_rec_buf)/2;buf_index++){	
			inside_index = buf_index/2;
			loop_index = buf_index%2;
			transfor_aec_buf[buf_index] = aec_rec_buf[inside_index*4 + loop_index];
		}	

		duilite_wakeup_feed(wakeup, transfor_aec_buf, sizeof(transfor_aec_buf));

	}
	printf("==================>>>>AEC RECORD END!!!\n");
	aec_stop();
	aec_running = false;
	//duilite_library_release();
	return 0;
}
#else
static int aec_run() 
{
	printf("------------aec_run-------------------\n");
	//echo = duilite_echo_new(aec_cfg, aec_callback, NULL);
	unsigned int size;
	int ret = -1;
	int cnt = 0;
	aec_stop_flag = false;
	aec_running = true;

	
	GADI_AUDIO_AioAttrT aio_attr;
	GADI_AUDIO_AioFrameT audio_frame;

	
	sample_audio_ai_start(&aio_attr, 0);

	
	aec_start();
	printf("===> AEC RECORD: Please wakeup me.\n");

	while(!aec_stop_flag){
		gadi_audio_ai_get_frame(&audio_frame, 1);
		duilite_echo_feed(echo, audio_frame.virAddr, audio_frame.len);
		#if 0
		for(buf_index = 0;buf_index < sizeof(aec_rec_buf)/2;buf_index++){	
			inside_index = buf_index/2;
			loop_index = buf_index%2;
			transfor_aec_buf[buf_index] = aec_rec_buf[inside_index*4 + loop_index];
		}		
		duilite_wakeup_feed(wakeup, transfor_aec_buf, sizeof(transfor_aec_buf));
		#endif
	}
	printf("==================>>>>AEC RECORD END!!!\n");
	sample_audio_ai_stop();
	aec_stop();
	aec_running = false;
	//duilite_library_release();
	return 0;
}

#endif

int ray_aec_start(void)
{
	printf("----------------ray_aec_start--------------\n");

	//aec_start();   //ai_vr_init()
	aec_run();	

	return 0;
}

int ray_aec_stop()
{
	aec_stop_flag = true;
	return 0;
}
int aec_key_wakeup(void)
{
	printf(" =======>key wake up SUCCESSFULLY<=======\n");
	// rgb  dd4001
	ai_mutex_lock();
	ai_vr_info.from    = VR_FROM_AEC;
	ai_vr_info.aec.wakeup = true;
	if (ai_vr_callback)
		ai_vr_callback(&ai_vr_info);
	ai_mutex_unlock();

	return 0;
}

