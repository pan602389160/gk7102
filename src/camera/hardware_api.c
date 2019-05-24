
/*
 * It is the demo of hardware I/O api.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>

#include "hardware_api.h"

#include "rtsp.h"
#include "adi_types.h"
#include "adi_sys.h"
#include "adi_audio.h"
#include "adi_venc.h"

#include "basetypes.h"
#include "shell.h"
#include "venc.h"
#include "parser.h"
#include "gk_rtsp.h"
#include "media_fifo.h"
#include "adi_audio.h"
#include "adi_sys_error.h"
#include "adi_types.h"
#include "asoundlib.h"

//----------------------------------------------------------
// Video capture api.

static pthread_t s_threadVideoIn;
static unsigned int s_runningVideoIn = 0;
static video_capture_callback s_videoCaptureCallback = 0;
static unsigned char s_ucData[65536] = {0};

extern GADI_VENC_StreamT v_stream;
extern GADI_AUDIO_AioAttrT aio_attr;
extern GADI_AUDIO_SampleFormatEnumT sampleFormat;

///

static void* hardware_video_capture_thread_proc(void* lp)
{
	printf("hardware_video_capture_thread_proc: start\n");

	if (pthread_detach(pthread_self()) != 0) {
		printf("hardware_video_capture_thread_proc, err=%d", errno);
	}
 	
	int ifrmSize = -1;
	while (s_runningVideoIn != 0) {
		unsigned int uDataSize = 0;
		#if 0
		if (pFile != NULL) {
			fseek(pFile, 0, SEEK_SET);
			int iRead = fread(s_ucData, ifrmSize, 1, pFile);
			if (iRead > 0 || feof(pFile)) {
				uDataSize = ifrmSize; //
			}
			else {
				uDataSize = 1024; // assume the H.264 frame size.
			}
		}
		else {
			uDataSize = 1024; // assume the H.264 frame size.
		}
		#endif
		memset(s_ucData,sizeof(s_ucData),0);
		memcpy(s_ucData,v_stream.addr,v_stream.size);
		uDataSize = v_stream.size;
		
		unsigned int uKeyFrame = 0;
		if (v_stream.pic_type == 1) {
			uKeyFrame = 1;
		}

		// invoke callback function.
		if (s_videoCaptureCallback != 0) {
			s_videoCaptureCallback(0, s_ucData, uDataSize, uKeyFrame);
		}

		usleep(50 * 1000);
	}

	printf("hardware_video_capture_thread_proc: stop\n");
	pthread_exit(NULL);
}



int hardware_video_capture_open(void **context, int camera_no, int width, int height,
	int bit_rate, int frame_rate, int key_frame_rate, video_capture_callback callback_func)
{
	s_videoCaptureCallback = callback_func;

	// TODO: Open the hardware video capture device.

	// Check video size.
	if (width != 640 || height != 480) {
		printf("hardware_video_capture_open: Invalid width or height, width=%d, height=%d\n", width, height);
		return 0;
	}

	pthread_attr_t attr;
	pthread_attr_init(&attr);

	s_runningVideoIn = 1;
	int iRet = pthread_create(&s_threadVideoIn, &attr, hardware_video_capture_thread_proc, 0);
	if (iRet != 0) {
		s_runningVideoIn = 0;
		s_videoCaptureCallback = 0;
	}

	pthread_attr_destroy(&attr);

	return (s_runningVideoIn != 0);
}

int hardware_video_capture_force_key_frame(void *context)
{
	// TODO: Force the hardware encoder to output a key frame immediately.

	return 1;
}

void hardware_video_capture_close(void *context)
{
	// TODO: Close the hardware video capture device.

	// In this demo: break the simulate video capture thread.
	s_runningVideoIn = 0;
}



//-----------------------------------------------------------------------------
// In this demo: we use a audio loopback way to simulate audio output and audio input.
// When the SDK play a audio output data, we copy the audio output data into a buffer.
// And then in the audio record thread, we write the audio output data that in buffer back to SDK.
//
// -------
// |     | ----- hardware_audio_play_write_data() ------------+
// |     |                                  |
// |     |                             +---------+
// | SDK |                             | Buffer  |
// |     |                             +---------+
// |     |                                  |
// |     | <---- s_audioRecordCallback() ---+
// -------

// The buffer use cache the audio output data.
#define HW_AUDIO_BUF_DATA_SIZE  (12288 * 2 * 4)
#define HW_AUDIO_LOOP_BUF_NUM   4

struct HW_AUDIO_BUF_S {
	unsigned char ucData[HW_AUDIO_BUF_DATA_SIZE];
	unsigned int uDataSize;
};

static pthread_mutex_t s_AudioLoopMutex;
static struct HW_AUDIO_BUF_S s_AudioLoopBuf[HW_AUDIO_LOOP_BUF_NUM];
static unsigned int s_uAudioLoopInd = 0;


//----------------------------------------------------------
// Audio record api.

static pthread_t s_threadAudioIn;	
static unsigned int	s_runningAudioIn = 0;
static audio_record_callback s_audioRecordCallback = 0;

/*!
******************************************************************************
** sample: ai start
******************************************************************************
*/
GADI_ERR sample_audio_ai_start(GADI_AUDIO_AioAttrT *pstAioAttr,GADI_AUDIO_SampleFormatEnumT sampleFormat)
{
    GADI_ERR retVal;
    
    retVal = gadi_audio_set_sample_format(gadi_audio_ai_get_fd(), sampleFormat);
    if (retVal != GADI_OK){
        GADI_ERROR("gadi_audio_set_sample_format failed(%d)\n",retVal);
        return -1;
    }

    /* set AI dev attr */
    retVal = gadi_audio_ai_set_attr(pstAioAttr);
    if (retVal != GADI_OK){
        GADI_ERROR("gadi_audio_ai_set_attr failed(%d)\n",retVal);
        return -1;
    }

    /* enable AI*/
    retVal = gadi_audio_ai_enable();
    if (retVal != GADI_OK){
        GADI_ERROR("gadi_audio_ai_enable failed(%d)\n",retVal);
        return -1;
    }

    return GADI_OK;
}
/*!
******************************************************************************
** sample: ai stop
******************************************************************************
*/
GADI_ERR sample_audio_ai_stop()
{
    GADI_ERR retVal;

    /* disable AI*/
    retVal = gadi_audio_ai_disable();
    if (retVal){
        GADI_ERROR("gadi_audio_ai_disable failed(%d)\n",retVal);
        return -1;
    }

    return GADI_OK;
}


/*!
******************************************************************************
** sample: ao start
******************************************************************************
*/
GADI_ERR sample_audio_ao_start(GADI_AUDIO_AioAttrT *pstAioAttr,GADI_AUDIO_SampleFormatEnumT sampleFormat)
{
    GADI_ERR retVal;
    
    retVal = gadi_audio_set_sample_format(gadi_audio_ao_get_fd(), sampleFormat);
    if (retVal != GADI_OK){
        GADI_ERROR("gadi_audio_set_sample_format failed(%d)\n",retVal);
        return -1;
    }

    /* set AO dev attr */
    retVal = gadi_audio_ao_set_attr(pstAioAttr);
    if (retVal != GADI_OK){
        GADI_ERROR("gadi_audio_ao_set_attr failed(%d)\n",retVal);
        return -1;
    }

    /* enable AO*/
    retVal = gadi_audio_ao_enable();
    if (retVal != GADI_OK){
        GADI_ERROR("gadi_audio_ao_enable failed(%d)\n",retVal);
        return -1;
    }

    return GADI_OK;
}


/*!
******************************************************************************
** sample: ao stop
******************************************************************************
*/
GADI_ERR sample_audio_ao_stop()
{
    GADI_ERR retVal;

    /* disable AO*/
    retVal = gadi_audio_ao_disable();
    if (retVal != GADI_OK){
        GADI_ERROR("gadi_audio_ao_disable failed(%d)\n",retVal);
        return -1;
    }


    return GADI_OK;
}

///
// In this demo, we use a thread to simulate Audio record.

extern struct pcm_config in_config;
extern struct pcm *in_pcm;
char rec_buf[2560] = {0};
char transfor_buf[1280] = {0};

static void* hardware_audio_record_thread_proc(void* lp)
{
  	printf("hardware_audio_record_thread_proc: start\n");
	unsigned int ret = 0;
	int index = 0;
	int inside_loop = 0;
	int inside_index = 0;
	int outside_loop = 0;
	int loop_index = 0;
	if (pthread_detach(pthread_self()) != 0) {
		printf("hardware_audio_record_thread_proc, err=%d", errno);
	}
	#if 0
	GADI_ERR retVal;
	GADI_AUDIO_AioFrameT audio_frame;
	GADI_BOOL flag = GADI_TRUE;
	
	
	retVal = sample_audio_ai_start(&aio_attr, sampleFormat);
    if(retVal != GADI_OK) {
        printf("sample_audio_ai_start ERROR\n");
    }else{
		printf("sample_audio_ai_start SUCCESSFULLY\n");
	}
	while (s_runningAudioIn != 0) {
		if(gadi_audio_ai_get_frame(&audio_frame, flag) == GADI_OK){
			if (s_audioRecordCallback != 0) {
					//printf("---->>>>  audio_frame.len = %d\n",audio_frame.len);
					s_audioRecordCallback(0, audio_frame.virAddr, audio_frame.len);
			}
		}
		//usleep(40 * 1000);
	}
	sample_audio_ai_stop();
	#else
	memset(rec_buf,sizeof(rec_buf),0);
	
	while (s_runningAudioIn != 0) {
		ret = pcm_read(in_pcm, rec_buf, sizeof(rec_buf));
		if (s_audioRecordCallback != 0) {
				if(ret)
					printf("---->>>>  pcm_read ret = %d\n",ret);
				for(index = 0;index < 1280;index++){
					inside_index = index/2;
					loop_index = index%2;
					
					transfor_buf[index] = rec_buf[inside_index*4 + loop_index];
				}
				s_audioRecordCallback(0, transfor_buf, sizeof(transfor_buf));
		}
		
		//usleep(40 * 1000);
	}
	#endif
  	printf("hardware_audio_record_thread_proc: stop\n");

	pthread_exit(NULL);
}


int hardware_audio_record_open(void **context, int microphone_no,
	int sample_rate, int frame_len, audio_record_callback callback_func)
{
	s_audioRecordCallback = callback_func;

	// TODO: Open the hardware audio record device.
	
	// In this demo: we use a thread to simulate audio record.
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	s_runningAudioIn = 1;
	int iRet = pthread_create(&s_threadAudioIn, &attr, hardware_audio_record_thread_proc, 0);
	if (iRet != 0) {
		s_runningAudioIn = 0;
		s_audioRecordCallback = 0;
	}

	pthread_attr_destroy(&attr);

	return (s_runningAudioIn != 0);
}

void hardware_audio_record_close(void *context)
{
	// TODO: Close the hardware audio record device.

	// In this demo: break the simulate record thread.
	s_runningAudioIn = 0;
}


//----------------------------------------------------------
// Audio play api.

int hardware_audio_play_open(void **context, int speaker_no, int sample_rate, int frame_len)
{
	// TODO: Open the hardware audio play device.

	// Reset loop buffer.
	pthread_mutex_init(&s_AudioLoopMutex, 0);
	memset(s_AudioLoopBuf, 0, sizeof(s_AudioLoopBuf));
	s_uAudioLoopInd = 0;

	return 1;
}

int hardware_audio_play_write_data(void *context, void *data, int len)
{
	// TODO: Write audio data to hardware audio play device.

	// In this demo: copy the audio data to the lookpback cache.
	int iRet = 0;
	pthread_mutex_lock(&s_AudioLoopMutex); 

	if (s_uAudioLoopInd < HW_AUDIO_LOOP_BUF_NUM) {
		memcpy(s_AudioLoopBuf[s_uAudioLoopInd].ucData, data, len);
		s_AudioLoopBuf[s_uAudioLoopInd].uDataSize = len;
		s_uAudioLoopInd++;
	  	iRet = len;
	}

	pthread_mutex_unlock(&s_AudioLoopMutex);
	return iRet;
}

void hardware_audio_play_close(void *context)
{
	// TODO: Close the hardware audio play device.
	
	pthread_mutex_destroy(&s_AudioLoopMutex);
}
