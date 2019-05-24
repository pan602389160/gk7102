/*!
*****************************************************************************
** \file      $gkprjinc/gk_rtsp.h
**
** \version	$id: inc/gk_rtsp.h 15-08-04  8月:08:1438664577
**
** \brief
**
** \attention   this code is provided as is. goke microelectronics
		accepts no responsibility or liability for any errors
		orommissions
** (c) copyright 2015 by He yongbing co.,ltd
*****************************************************************************
*/
#ifndef GK_RTSP_H_
#define GK_RTSP_H_

#define MAX_VIDEO_ENC_CHN   3
#define MEDIA_CODEC_H264	(0x0)
#define MEDIA_CODEC_PCMA	(0x1)
#define MEDIA_CODEC_PCMU	(0x2)
#define MEDIA_CODEC_NOT_SUPPORT (0x03)



typedef struct video_param{
    int max_ch;//max video channle number
    int enc_type[MAX_VIDEO_ENC_CHN];
    int buffer_id[MAX_VIDEO_ENC_CHN];//video stream buffer id default 0-2
    int fps[MAX_VIDEO_ENC_CHN];
    char rtsp_route[MAX_VIDEO_ENC_CHN][512];
}VIDEO_PARAM_S;
typedef struct audio_patam{
    int enable;// 0 disable; 1 input; 2- input&output
    int enc_type;//audio enc type
    int samplerate;//audio samplerate
    int samplewidth;//audio samplewidth
    int channle_num;
}AUDIO_PATAM_S;
typedef struct rtsp_param{
    VIDEO_PARAM_S video;
    AUDIO_PATAM_S audio;
}RTSP_PARAM_S;


int rtsp_start(RTSP_PARAM_S *rtsp);
int rtsp_exit(void);
int rtsp_getParam(RTSP_PARAM_S *param);
int rtsp_setParam(RTSP_PARAM_S *param);
#endif

