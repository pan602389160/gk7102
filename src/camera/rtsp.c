/*
****************************************************************************
** \file      /applications/adidemo/src/rtsp.c
**
** \version   $Id: rtsp.c 0 2016-03-24 11:48:50Z 	dengbiao $
**
** \brief     videc abstraction layer header file.
**
** \attention THIS SAMPLE CODE IS PROVIDED AS IS. GOFORTUNE SEMICONDUCTOR
**            ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**            OMMISSIONS.
**
** (C) Copyright 2015-2016 by GOKE MICROELECTRONICS CO.,LTD
**
****************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <assert.h>
#include <sys/prctl.h>

#include "hardware_api.h"

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
#include "pgLibDevVideoIn.h"

#ifdef AUTO_TEST_ENABLE
#include "venc_test.h"
#endif
//**************************************************************************
//**************************************************************************
//** Local Defines
//**************************************************************************
//**************************************************************************

//**************************************************************************
//**************************************************************************
//** Local structures
//**************************************************************************
//**************************************************************************

//**************************************************************************
//**************************************************************************
//** Global Data
//**************************************************************************
//**************************************************************************

extern GADI_SYS_HandleT vencHandle;
extern video_encode_stream streams[GADI_VENC_STREAM_NUM];

GADI_VENC_StreamT v_stream;


//**************************************************************************
//**************************************************************************
//** Local Data
//**************************************************************************
//**************************************************************************

static const char *shortOptions = "hSP";
static struct option longOptions[] =
{
    {"help",     0, 0, 'h'},
    {"start",    0, 0, 'S'},
    {"stop",     0, 0, 'P'},
    {0,          0, 0, 0}
};
static GADI_BOOL g_rtsp_start = GADI_FALSE;
static int video_frame_index[4] = {0,0,0,0};
static int audio_frame_index = 0;
static MEDIABUF_HANDLE video_writer[GADI_VENC_STREAM_NUM] =
    {[0 ... (GADI_VENC_STREAM_NUM-1)] = NULL};
static MEDIABUF_HANDLE audio_writer[GADI_VENC_STREAM_NUM] =
    {[0 ... (GADI_VENC_STREAM_NUM-1)] = NULL};
static unsigned int video_pts[4] = {0,0,0,0};
static unsigned int audio_pts = 0;
static GADI_SYS_ThreadHandleT read_video_data_pid;
static GADI_SYS_ThreadHandleT read_audio_data_pid;
static GADI_BOOL read_video_loop = GADI_FALSE;
static GADI_BOOL read_audio_loop = GADI_FALSE;
static RTSP_PARAM_S g_rtsp_param;

//**************************************************************************
//**************************************************************************
//** Local Functions Declaration
//**************************************************************************
//**************************************************************************
static GADI_ERR handle_rtsp_command(int argc, char* argv[]);
static void read_video_data(void *argc);
static void read_audio_data(void *argc);
static int set_rtsp_start_param(RTSP_PARAM_S *rtsp_param);

//**************************************************************************
//**************************************************************************
//** API Functions
//**************************************************************************
//**************************************************************************
int rtsp_server_start(void)
{
    int ret = 0;

    if(g_rtsp_start) {
        GADI_INFO("RTSP already start...\n");
        return 0;
    }
    /* configure rtsp params */
    set_rtsp_start_param(&g_rtsp_param);

    /* rtsp start running */
    ret = rtsp_start(&g_rtsp_param);
    if(ret < 0){
        GADI_ERROR("Rtsp start failed.");
        return -1;
    }

    /* fill video data stream. */
    read_video_loop = GADI_TRUE;
    ret = gadi_sys_thread_create(read_video_data, NULL, GADI_SYS_THREAD_PRIO_DEFAULT,
        GADI_SYS_THREAD_STATCK_SIZE_DEFAULT, "read_video_data", &read_video_data_pid);
    if(ret < 0){
        GADI_ERROR("Create [rtsp] pthread failed.");
        read_video_loop = GADI_FALSE;
    }
#if 0
    read_audio_loop = GADI_TRUE;

    ret = gadi_sys_thread_create(read_audio_data, NULL, GADI_SYS_THREAD_PRIO_DEFAULT,
        GADI_SYS_THREAD_STATCK_SIZE_DEFAULT, "read_audio_data", &read_audio_data_pid);
    if(ret < 0){
        GADI_ERROR("Create [rtsp] pthread failed.");
        read_audio_loop = GADI_FALSE;
    }
#endif
    g_rtsp_start = GADI_TRUE;

    return 0;
}

int rtsp_server_stop(void)
{
    if(!g_rtsp_start) {
        GADI_INFO("RTSP no start...\n");
        return 0;
    }
	read_video_loop = GADI_FALSE;
    rtsp_exit();
    if (read_video_loop) {
        read_video_loop = GADI_FALSE;
        gadi_sys_wait_end_thread(read_video_data_pid);
    }
    if (read_audio_loop) {
        read_audio_loop = GADI_FALSE;
        gadi_sys_wait_end_thread(read_audio_data_pid);
    }
    g_rtsp_start = GADI_FALSE;
    return 0;
}

int rtsp_audio_output_stream_on(void)
{
    GADI_ERR ret = 0;
    if(!g_rtsp_start) {
        GADI_INFO("RTSP no start...\n");
        return -1;
    }
    if (g_rtsp_param.audio.enable == 0) {
        GADI_INFO("Audio is no enable.\n");
        return -1;
    }
    if (read_audio_loop) {
        GADI_INFO("Audio output already on...\n");
        return 0;
    }
    read_audio_loop = GADI_TRUE;
    ret = gadi_sys_thread_create(read_audio_data, NULL, GADI_SYS_THREAD_PRIO_DEFAULT,
        GADI_SYS_THREAD_STATCK_SIZE_DEFAULT, "read_audio_data", &read_audio_data_pid);
    if(ret < 0){
        GADI_ERROR("Create [rtsp] pthread failed.");
        read_audio_loop = GADI_FALSE;
        return -1;
    }
    return 0;
}

int rtsp_audio_output_stream_off(void)
{
    if(!g_rtsp_start) {
        GADI_INFO("RTSP no start...\n");
        return -1;
    }
    if (g_rtsp_param.audio.enable == 0) {
        GADI_INFO("Audio is no enable.\n");
        return -1;
    }
    if(!read_audio_loop) {
        GADI_INFO("Audio output already on...\n");
        return 0;
    }
    read_audio_loop = GADI_FALSE;
    gadi_sys_wait_end_thread(read_audio_data_pid);
    return 0;
}


int rtsp_create_media_stream(int stream_id, int buf_size)
{
    int ret = 0;
    if(video_writer[stream_id] != NULL)
        return 0;

    ret = mediabuf_init(stream_id, buf_size);
    if(ret != 0){
        GADI_ERROR("init media buffer failed");
        return ret;
    }
    video_writer[stream_id] = mediabuf_add_writer(stream_id);
    audio_writer[stream_id] = mediabuf_add_writer(stream_id);
    return 0;
}

int rtsp_destory_media_stream(int stream_id)
{
    int ret = 0;
    if(video_writer[stream_id] == NULL)
        return 0;

    mediabuf_del_writer(audio_writer[stream_id]);
    audio_writer[stream_id] = NULL;
    mediabuf_del_writer(video_writer[stream_id]);
    video_writer[stream_id] = NULL;
    ret = mediabuf_uninit(stream_id);
    if(ret != 0){
        GADI_ERROR("uninit media buffer failed");
        return ret;
    }
    return 0;
}

GADI_ERR rtsp_register_testcase(void)
{
    GADI_ERR   retVal =  GADI_OK;
    (void)shell_registercommand (
        "rtsp",
        handle_rtsp_command,
        "rtsp command",
        NULL
    );

    return retVal;
}

//**************************************************************************
//**************************************************************************
//** Local Functions
//**************************************************************************
//**************************************************************************
static int set_rtsp_start_param(RTSP_PARAM_S *rtsp_param)
{
    int i;

    memset(rtsp_param, 0, sizeof(RTSP_PARAM_S));
    rtsp_param->video.max_ch = 3;
    if(rtsp_param->video.max_ch > 4)//max venc channle is 4
        rtsp_param->video.max_ch = 4;
	
    for(i = 0; i < rtsp_param->video.max_ch; i ++)
    {
        if(1 == streams[i].streamFormat.encodeType)/*0: none, 1: H.264, 2: MJPEG*/
            rtsp_param->video.enc_type[i] = MEDIA_CODEC_H264;
        else
            rtsp_param->video.enc_type[i] = MEDIA_CODEC_NOT_SUPPORT;
        rtsp_param->video.buffer_id[i] = i;
        rtsp_param->video.fps[i] = streams[i].streamFormat.fps;
        sprintf(rtsp_param->video.rtsp_route[i], "/stream%d", i);
    }
    rtsp_param->audio.enable = 1;
    rtsp_param->audio.enc_type = MEDIA_CODEC_PCMA;

    rtsp_param->audio.samplerate = 16000;
    rtsp_param->audio.samplewidth = 16;
    rtsp_param->audio.channle_num = 1;

    return 0;
}

static inline void cal_video_pts(unsigned int*pts, GADI_VENC_StreamT *stream)
{
    unsigned int ch = stream->stream_id;
    *pts = video_pts[ch];
    switch(streams[ch].streamFormat.fps)
    {
        case GADI_VENC_FPS_1:
            video_pts[ch] += 90000;
            break;
        case GADI_VENC_FPS_2:
            video_pts[ch] += 45000;
            break;
        case GADI_VENC_FPS_3:
            video_pts[ch] += 30000;
            break;
        case GADI_VENC_FPS_4:
            video_pts[ch] += 22500;
            break;
        case GADI_VENC_FPS_5:
            video_pts[ch] += 18000;
            break;
        case GADI_VENC_FPS_6:
            video_pts[ch] += 15000;
            break;
        case GADI_VENC_FPS_7:
            video_pts[ch] += 12857;
            break;
        case GADI_VENC_FPS_8:
            video_pts[ch] += 11250;
            break;
        case GADI_VENC_FPS_9:
            video_pts[ch] += 10000;
            break;
        case GADI_VENC_FPS_10:
            video_pts[ch] += 9000;
            break;
        case GADI_VENC_FPS_11:
            video_pts[ch] += 8181;
            break;
        case GADI_VENC_FPS_12:
            video_pts[ch] += 7500;
            break;
        case GADI_VENC_FPS_13:
            video_pts[ch] += 6923;
            break;
        case GADI_VENC_FPS_14:
            video_pts[ch] += 6428;
            break;
        case GADI_VENC_FPS_15:
            video_pts[ch] += 6000;
            break;
        case GADI_VENC_FPS_16:
            video_pts[ch] += 5625;
            break;
        case GADI_VENC_FPS_17:
            video_pts[ch] += 5294;
            break;
        case GADI_VENC_FPS_18:
            video_pts[ch] += 5000;
            break;
        case GADI_VENC_FPS_19:
            video_pts[ch] += 4736;
            break;
        case GADI_VENC_FPS_20:
            video_pts[ch] += 4500;
            break;
        case GADI_VENC_FPS_21:
            video_pts[ch] += 4285;
            break;
        case GADI_VENC_FPS_22:
            video_pts[ch] += 4090;
            break;
        case GADI_VENC_FPS_23:
            video_pts[ch] += 3913;
            break;
        case GADI_VENC_FPS_24:
            video_pts[ch] += 3750;
            break;
        case GADI_VENC_FPS_25:
            video_pts[ch] += 3600;
            break;
        case GADI_VENC_FPS_26:
            video_pts[ch] += 3461;
            break;
        case GADI_VENC_FPS_27:
            video_pts[ch] += 3333;
            break;
        case GADI_VENC_FPS_28:
            video_pts[ch] += 3214;
            break;
        case GADI_VENC_FPS_29:
            video_pts[ch] += 3103;
            break;
        case GADI_VENC_FPS_23_976:  /*fps:23.976.*/
        case GADI_VENC_FPS_29_97:   /*fps:29.97.*/
        case GADI_VENC_FPS_30:
            video_pts[ch] += 3000;
            break;
        case GADI_VENC_FPS_50:
            video_pts[ch] += 1800;
            break;
        case GADI_VENC_FPS_59_94:   /*fps:59.94.*/
        case GADI_VENC_FPS_60:
            video_pts[ch] += 1500;
            break;
        case GADI_VENC_FPS_120:
            video_pts[ch] += 750;
            break;
        case GADI_VENC_FPS_3_125:   /*fps:3.125.*/
            video_pts[ch] += 28000;
            break;
        case GADI_VENC_FPS_3_75:    /*fps:3.75.*/
            video_pts[ch] += 24000;
            break;
        case GADI_VENC_FPS_6_25:    /*fps:6.25.*/
            video_pts[ch] += 14400;
            break;
        case GADI_VENC_FPS_7_5:     /*fps:7.5.*/
            video_pts[ch] += 12000;
            break;
        case GADI_VENC_FPS_12_5:    /*fps:12.5.*/
            video_pts[ch] += 7200;
            break;
        case GADI_VENC_FPS_AUTO:
        default:
            video_pts[ch] =  stream->PTS;
    }

}

static inline void cal_audio_pts(unsigned int*pts)
{
    *pts = audio_pts;
    audio_pts += 320;
}


extern int s_iDevID_VideoIn;

static void read_video_data(void *argc)
{
    int chn, i;
    GADI_CHN_AttrT  chn_attr;
    unsigned int uFlag = 0;
    GADI_BOOL encodingState = GADI_FALSE;
    struct timeval tv;

    while(read_video_loop) {
        encodingState = GADI_FALSE;
        /* wait video encode enable */
        while(vencHandle == NULL && read_video_loop) {
            sleep(1);
        }

        /* check video encode stream status */
        for(i = 0; i < GADI_VENC_STREAM_NUM; i ++)
        {
            chn = i;
            if (gadi_venc_query(vencHandle, chn, &chn_attr) < 0)
            {
                continue;
            }
			
            if(chn_attr.state == GADI_VENC_STREAM_STATE_ENCODING)
            {
            	
                encodingState = GADI_TRUE;
                break;
            }
			 if(chn_attr.state == GADI_VENC_STREAM_STATE_STOPPING)
            {
            	
                encodingState = GADI_FALSE;
                break;
            }
        }
		//printf("[%d] : chn_attr.state = %d,encodingState = %d\n",i,chn_attr.state,encodingState);
        /* have encode stream, get video data to rtsp data buffer. */
        if(encodingState){
            if (gadi_venc_get_stream(vencHandle, i, &v_stream)<0)//BLOCK
            {
        
                continue;
            }
            /*stream end, stream size & stream addr is invalid.*/
            if(v_stream.stream_end == 1)
            {

                continue;
            }
			#if 0
            GK_NET_FRAME_HEADER header = {0};
            gettimeofday(&tv, NULL);
            header.frame_size = stream.size;
            if (stream.pic_type == 1)
                header.frame_type = GK_NET_FRAME_TYPE_I;
            else
                header.frame_type = GK_NET_FRAME_TYPE_P;
            header.sec = tv.tv_sec;
            header.usec = tv.tv_usec;
            header.video_reso = (streams[stream.stream_id].streamFormat.width << 16) + (streams[stream.stream_id].streamFormat.height);
            //header.pts = stream.PTS;
            cal_video_pts(&(header.pts), &stream);

            header.frame_no = video_frame_index[stream.stream_id] ++;

			#endif
			
			if (v_stream.pic_type == 1)
				uFlag |= PG_DEV_VIDEO_IN_FLAG_KEY_FRAME;
			else
				uFlag = 0;
			if(read_video_loop)
				pgDevVideoInCaptureProc(s_iDevID_VideoIn, v_stream.addr, v_stream.size, PG_DEV_VIDEO_IN_FMT_H264, uFlag);

            //if(video_writer[v_stream.stream_id] != NULL)
            //    mediabuf_write_frame(video_writer[v_stream.stream_id], v_stream.addr, v_stream.size, &header);
        }/*end if(encodingState) */
        else
        {
            sleep(1);
        }
    }/*end while(read_loop) */
}

static void read_audio_data(void *argc)
{
    int ai_fd = -1, i;
    GADI_AUDIO_AioFrameT audio_frame;
    GADI_AUDIO_SampleFormatEnumT sample_format;
    GADI_AUDIO_AioAttrT aio_attr;
    struct timeval tv;
    GADI_ERR ret = GADI_OK;
    while((ai_fd = gadi_audio_ai_get_fd()) < 0 && read_audio_loop) {
        sleep(1);
    }
    sample_format = GADI_AUDIO_SAMPLE_FORMAT_A_LAW;
    aio_attr.bitWidth = g_rtsp_param.audio.samplewidth;
    aio_attr.soundMode = GADI_AUDIO_SOUND_MODE_SINGLE;
    aio_attr.sampleRate = g_rtsp_param.audio.samplerate;
    aio_attr.frameSamples = 1024;
    aio_attr.frameNum = 30;
    ret = gadi_audio_set_sample_format(ai_fd, sample_format);
    if (ret != GADI_OK) {
         GADI_ERROR("Set ai sample failed, error code[%d]", ret);
         return;
    }
    ret = gadi_audio_ai_set_attr(&aio_attr);
    if (ret != GADI_OK) {
         GADI_ERROR("Set ai attribute failed, error code[%d]", ret);
         return;
    }
    ret = gadi_audio_ai_enable();
    if (ret != GADI_OK) {
         GADI_ERROR("Enable ai failed, error code[%d]", ret);
         return;
    }

    while(read_audio_loop)
    {
        GK_NET_FRAME_HEADER header = {0};
        while((ai_fd = gadi_audio_ai_get_fd()) < 0 && read_audio_loop) {
            sleep(1);
        }
        /* wait video encode enable */
        gettimeofday(&tv, NULL);
        header.frame_type = GK_NET_FRAME_TYPE_A;
        header.sec = tv.tv_sec;
        header.usec = tv.tv_usec;
        if(gadi_audio_ai_get_frame(&audio_frame, GADI_TRUE) == 0){
            header.frame_size = audio_frame.len;
            header.pts = audio_frame.seq;
            cal_audio_pts(&(header.pts));
            header.frame_no = audio_frame_index++;
            for(i = 0; i < GADI_VENC_STREAM_NUM; i++){
                if(audio_writer[i] != NULL) {
                    mediabuf_write_frame(audio_writer[i], audio_frame.virAddr, audio_frame.len, &header);
                }
            }
        } else {
            sleep(1);
        }
    }
}

static void rtsp_usage(void)
{
    printf("\nusage: rtsp [OPTION]...\n");
    printf("\t-h, --help            help.\n"
           "\t-S, --start           running rtsp.\n"
           "\t-P, --stop             close rtsp.\n");
    printf("\n");
}

static GADI_ERR handle_rtsp_command(int argc, char* argv[])
{
    int option_index, ch;
	int retVal;

    /*change parameters when giving input options.*/
    while ((ch = getopt_long(argc, argv, shortOptions, longOptions, &option_index)) != -1)
    {
        switch (ch)
        {
        case 'h':
            rtsp_usage();
            break;

        case 'S':
            retVal = rtsp_server_start();
            if(retVal != 0){
                GADI_ERROR("rtsp start failed.");
                goto bad_parameter;
            }
            GADI_INFO("rtsp_start OK!");
            break;

        case 'P':
            retVal = rtsp_server_stop();
            if(retVal != 0){
                GADI_ERROR("rtsp stop failed.");
                goto bad_parameter;
            }
            GADI_INFO("rtsp_stop OK!");
            break;

        default:
            GADI_ERROR("bad params\n");
            goto bad_parameter;
			break;
        }
    }
	optind = 1;
    return 0;

bad_parameter:
	optind = 1;
    return -1;
}

