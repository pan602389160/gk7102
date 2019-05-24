#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>

#include "vr_interface.h"
#include "asr.h"
#include "asoundlib.h"
#include "dds.h"
#include "ray_app.h"
static bool asr_stop_flag = false;
bool asr_break_flag = false;
//static mic_record *record = NULL;  <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
bool asr_stopped = false;
extern bool vr_init_flag;
extern bool vr_working_flag;
extern vr_status_t vr_status;

//ASR
struct pcm_config in_asr_config;
struct pcm *in_asr_pcm;
char transfor_asr_buf[1600*2] = {0};
char asr_rec_buf[1600*4] = {0};
static int buf_index = 0;
static int inside_loop = 0;
static int inside_index = 0;
static int outside_loop = 0;
static int loop_index = 0;

static FILE *asr_record_fp = NULL;
#define    AUDIO_FILE   		"/tmp/record_tmp.pcm"

static int soundcard_init(void)
{
	#if 0
	int ret;

	record_param dmic_param = {
		.bits = ASR_BIT,
		.rates = ASR_RATE,
		.channels = ASR_CHANNEL,
		.volume = ASR_VOLUME,
	};

	record = mozart_soundcard_init(dmic_param);
	if (record == NULL) {
		printf("error: soundcard asr init fail.\n");
		ret = -1;
		goto err_init;
	}

	return 0;

err_init:
	mozart_soundcard_uninit(record);
	record = NULL;

	return ret;
	#endif
}

static int soundcard_uninit(void)
{
	#if 0
	mozart_soundcard_uninit(record);
	record = NULL;

	return 0;
	#endif
}

static unsigned long soundcard_asr_record(char *dmic_buf, unsigned long len)
{
	#if 0
	if (!record) {
		printf("error: Please init soundcard firstly!\n");
		return 0;
	}

	return mozart_record(record, dmic_buf, ASR_SIZE);
	#endif
}



static int asr_run()
{
	unsigned int size;
	int ret = -1;
	int cnt = 0;

    in_asr_config.channels = 2;
	in_asr_config.rate = 16000;
	in_asr_config.period_size = 400;  //²Î¿¼ camera
	in_asr_config.period_count = 4;
	in_asr_config.format = PCM_FORMAT_S16_LE;
	in_asr_config.start_threshold = 0;
	in_asr_config.stop_threshold = 0;
	in_asr_config.silence_threshold = 0;
	in_asr_pcm = pcm_open(0, 0, PCM_IN, &in_asr_config);

	if (!in_asr_pcm || !pcm_is_ready(in_asr_pcm)) {
		//printf("--------------------->  IN_ASR_pcm_open FAILED\n");
		fprintf(stderr, "Unable to open PCM device (%s)\n",pcm_get_error(in_asr_pcm));
		return NULL;
	}
	
	asr_record_fp = fopen(AUDIO_FILE, "wb+");
	if(asr_record_fp == NULL)
	{
	   printf("file can't open wb+!!!!!!");
	   return -1;
	}


	printf("===> ASR: What can I do for you?\n");
	
	while (!asr_stop_flag && !asr_break_flag) {
		//printf("--->>cnt  = %d\n",cnt++);
		memset(asr_rec_buf,0,sizeof(asr_rec_buf));
		memset(transfor_asr_buf,0,sizeof(transfor_asr_buf));

		
		ret = pcm_read(in_asr_pcm, asr_rec_buf, sizeof(asr_rec_buf));
		//duilite_echo_feed(echo, rec_buf, sizeof(rec_buf));
		
		for(buf_index = 0;buf_index < sizeof(asr_rec_buf)/2;buf_index++){	
			inside_index = buf_index/2;
			loop_index = buf_index%2;
			transfor_asr_buf[buf_index] = asr_rec_buf[inside_index*4 + loop_index];
		}

		ray_send_request(cnt,transfor_asr_buf,sizeof(asr_rec_buf)/2);
		//fwrite(transfor_asr_buf,sizeof(asr_rec_buf)/2,1,asr_record_fp);
	}
	printf("===>>asr_stop_flag:[%d] asr_break_flag[%d] asr record end!!\n",asr_stop_flag,asr_break_flag);
	if (asr_stop_flag) {
		asr_stop();
	}
	asr_stopped = true;
	fclose(asr_record_fp);
	//my_init();

	return 0;
}


int asr_start(bool sds)
{
	printf("-------asr_start--------\n");
	//start_dds();     //in ai_vr_init() 
	//ray_vad_start();
	request_start();
 	asr_run();
	
	return 0;
}

int asr_stop()
{
	printf("---------asr_stop-------------\n");
	if(in_asr_pcm){
		pcm_close(in_asr_pcm);
		in_asr_pcm = NULL;
	}
	request_stop();
	//ray_vad_stop();
	return 0;
}

int asr_delete()
{
	printf("---------asr_stop-------------\n");
	//ray_vad_delete();
	return 0;
}


int ray_asr_start(bool sds)
{
	asr_stop_flag = false;
	asr_break_flag = false;
	asr_stopped = false;

	asr_start(sds);
	//printf("===> ASR: What can I do for you?\n");

	return 0;
}

int ray_asr_stop(int reason)
{
	printf("-----ray_asr_stop-------\n");
	if (reason == 1)
		asr_break_flag = true;
	else
		asr_stop_flag = true;

	return 0;
}




int ray_vr_asr_break(void)
{
	if (!vr_init_flag || !vr_working_flag) {
		printf("warning: vr not init or not start, %s fail.\n", __func__);
		return -1;
	}

	vr_status = VR_IDLE;

	/* break asr process */
	asr_break();

	/* waiting for asr stopped */
	while (!asr_stopped)
		usleep(10 * 1000);

	/* close asr connection */
	//asr_cancel();

	return 0;
}

