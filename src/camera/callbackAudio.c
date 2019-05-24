// callbackAudio.cpp 
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "callbackAudio.h"
#include "callbackAudioQueue.h"
#include "hardware_api.h"	
#include "pgWaveFile.h"	

#include "adi_audio.h"
#include "adi_sys_error.h"
#include "adi_types.h"
#include "asoundlib.h"


// define the queue instance.
static PG_DEV_CALLBACK_AUDIO_QUEUE_S s_stQueue = PG_DEV_AUDIO_QUEUE_INIT();


//------------------------------------------------------------------------------
// Audio input callback

// This macro is enable or disable the recording wave file.
#define _PG_AUDIO_IN_WAVE   0

#if _PG_AUDIO_IN_WAVE
static PG_WAVE_FILE_CTX_S s_WaveCtxIn;
#endif


static int HardwareAudioRecordCallback(void *context, void *data, int len)
{
	//printf("------>>Dev to App,len = %d\n",len);

	if (pgDevAudioQueueRecordPush(&s_stQueue, PG_DEV_AUDIO_CVT_FMT_PCM16, data, len) <= 0) {
		return 0;
	}

	return 0;
}

GADI_AUDIO_AioAttrT aio_attr;
GADI_AUDIO_SampleFormatEnumT sampleFormat = GADI_AUDIO_SAMPLE_FORMAT_RAW_PCM;
struct pcm_config in_config;
struct pcm *in_pcm;


static int AudioInOpen(unsigned int uDevNO, unsigned int uSampleBits,
	unsigned int uSampleRate, unsigned int uChannels, unsigned int uPackBytes)
{
	printf("AudioInOpen: uDevNO=%u, uSampleBits=%u, uSampleRate=%u, uChannels=%u,uPackBytes=%u\n",
		uDevNO, uSampleBits, uSampleRate, uChannels,uPackBytes);
	
	int iDevIDIn = pgDevAudioQueueRecordOpen(&s_stQueue, PG_DEV_AUDIO_CVT_FMT_PCM16, 16000, 640, 0);
	if (iDevIDIn <= 0) {
		printf("AudioInOpen: Alloc audio convert failed.\n");
		return -1;
	}

	// Open the audio record device.
	aio_attr.bitWidth = GADI_AUDIO_BIT_WIDTH_16;
    aio_attr.soundMode = GADI_AUDIO_SOUND_MODE_SINGLE;
    aio_attr.sampleRate = GADI_AUDIO_SAMPLE_RATE_16000;
    aio_attr.frameSamples = 640;
    aio_attr.frameNum = 30;

    in_config.channels = 2;
	in_config.rate = 16000;
	in_config.period_size = 320;
	in_config.period_count = 4;
	in_config.format = PCM_FORMAT_S16_LE;
	in_config.start_threshold = 0;
	in_config.stop_threshold = 0;
	in_config.silence_threshold = 0;
	in_pcm = pcm_open(0, 0, PCM_IN, &in_config);
	if (!in_pcm || !pcm_is_ready(in_pcm)) {
		//printf("--------------------->???111  IN_pcm_open FAILED\n");
		fprintf(stderr, "Unable to open PCM device (%s)\n",pcm_get_error(in_pcm));
		return 0;
	}
	//printf("--------------------->???111  OUT_pcm_open SUCCESSFULLY\n");
	
	if (!hardware_audio_record_open(0, uDevNO, uSampleRate,
		(uPackBytes / 2), HardwareAudioRecordCallback))
	{
		pgDevAudioQueueRecordClose(&s_stQueue);
		iDevIDIn = -1;
	}
	
	return iDevIDIn;
}

static void AudioInClose(int iDevID)
{
	printf("AudioInClose: iDevID=%d\n", iDevID);

	// Close the audio record device.
	hardware_audio_record_close(0);

	pgDevAudioQueueRecordClose(&s_stQueue);
	pcm_close(in_pcm);

}

static PG_DEV_AUDIO_IN_CALLBACK_S s_stCallback_AudioIn = {
	AudioInOpen,
	AudioInClose
};
 

//------------------------------------------------------------------------------
// Audio output callback

// This macro is enable or disable the recording wave file.
#define _PG_AUDIO_OUT_WAVE   0

#if _PG_AUDIO_OUT_WAVE
static PG_WAVE_FILE_CTX_S s_WaveCtxOut;
#endif
static struct pcm_config out_config;
static struct pcm *out_pcm;


static int AudioOutOpen(unsigned int uDevNO, unsigned int uSampleBits,
	unsigned int uSampleRate, unsigned int uChannels, unsigned int uPackBytes)
{
	printf("AudioOutOpen: uDevNO=%u, uSampleBits=%u, uSampleRate=%u, uChannels=%u,uPackBytes=%u\n",
		uDevNO, uSampleBits, uSampleRate, uChannels,uPackBytes);
	
	aio_attr.bitWidth = GADI_AUDIO_BIT_WIDTH_16;
    aio_attr.soundMode = GADI_AUDIO_SOUND_MODE_SINGLE;
    aio_attr.sampleRate = GADI_AUDIO_SAMPLE_RATE_16000;
    aio_attr.frameSamples = 640;
    aio_attr.frameNum = 30;
	

	int iDevIDOut = pgDevAudioQueuePlayOpen(&s_stQueue, PG_DEV_AUDIO_CVT_FMT_PCM16, 16000, 640);
	if (iDevIDOut <= 0) {
		printf("AudioOutOpen: Alloc audio convert failed.\n");
		return -1;
	}

	// Open the audio playing device.
	#if 0  //goke API
	int volume = -1;
	sample_audio_ao_start(&aio_attr, sampleFormat);
	gdm_audio_ao_get_volume(&volume);
 	printf("-->>>11 get volume = %d\n",volume);
	volume = 4;
	gdm_audio_ao_set_volume(volume);
	volume = -1;
	gdm_audio_ao_get_volume(&volume);
 	printf("-->>>22 get volume = %d\n",volume);
	#else
    out_config.channels = 2;
	out_config.rate = 16000;
	out_config.period_size = 320;
	out_config.period_count = 4;
	out_config.format = PCM_FORMAT_S16_LE;
	out_config.start_threshold = 0;
	out_config.stop_threshold = 0;
	out_config.silence_threshold = 0;
	out_pcm = pcm_open(0, 0, PCM_OUT, &out_config);
	if (!out_pcm || !pcm_is_ready(out_pcm)) {
		//printf("--------------------->???222  OUT_pcm_open FAILED\n");
	 	fprintf(stderr, "Unable to open PCM device 0 (%s)\n", pcm_get_error(out_pcm));
		return -1;
	}
	//printf("--------------------->???222  OUT_pcm_open SUCCESSFULLY\n");
	#endif
	
 	return iDevIDOut;
}

static void AudioOutClose(int iDevID)
{
 	printf("AudioOutClose: iDevID=%d\n", iDevID);
 
	pgDevAudioQueuePlayClose(&s_stQueue);
	
	// Close the audio playing device.
	//sample_audio_ao_stop();
	pcm_close(out_pcm);
}
 
static int AudioOutPlay(int iDevID, const void* lpData, unsigned int uDataSize, unsigned int uFormat)
{
	int iRet = pgDevAudioQueuePlayPush(&s_stQueue, uFormat, lpData, uDataSize);// 把待转换的音频数据压入到队列中
	if (iRet <= 0) {
		printf("AudioOutPlay: Audio convert push failed.\n");
 	  	return -1;
	}
	GADI_AUDIO_AioFrameT audio_frame;
	GADI_BOOL flag = GADI_TRUE;
	unsigned char ucDataPop[1280] = {0};
	unsigned char transfomer_data[2560] = {0};
	int inside_loop = 0;
	int inside_index = 0;
	int outside_loop = 0;
	int loop_index = 0;
	int index = 0;
	
	int iPopSize = pgDevAudioQueuePlayPop(&s_stQueue, ucDataPop, sizeof(ucDataPop)); // 把转换后的数据从队列弹出
	if (iPopSize > 0) {
		// Output data to device.
	#if 0
		printf("====>>>>[%d] [%d] [%d] [%d] ----------------->iPopSize = [%d]\n",ucDataPop[0],ucDataPop[1],ucDataPop[2],ucDataPop[3],iPopSize);

	#endif
		#if 0
		audio_frame.len = iPopSize;
	    audio_frame.virAddr = ucDataPop;
		gadi_audio_ao_send_frame(&audio_frame,flag);
		#else
		for(index = 0;index < 2560;index++)//--------
		{
			inside_index = index/2; 
			loop_index = index%2;

			if(inside_index%2)
				transfomer_data[index] = transfomer_data[index - 2];
			else
				transfomer_data[index] = ucDataPop[inside_index + loop_index];
		}
		iRet = pcm_write(out_pcm, transfomer_data, sizeof(transfomer_data));
		if(iRet)
			printf("####pcm_write %d\n",iRet);
		#endif
	}
	
 	return uDataSize;

}

static PG_DEV_AUDIO_OUT_CALLBACK_S s_stACallback_AudioOut = {
	AudioOutOpen,
	AudioOutClose,
	AudioOutPlay
};


void RegisterAudioCallback(void)
{
	pgDevAudioInSetCallback(&s_stCallback_AudioIn);
	pgDevAudioOutSetCallback(&s_stACallback_AudioOut);
}
