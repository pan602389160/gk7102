/*!
*****************************************************************************
** \file        ./adi/test/src/venc.h
**
** \brief       adi test venc module header file.
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**              ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**              OMMISSIONS
**
** (C) Copyright 2013-2014 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/

#ifndef _VENC_H_
#define _VENC_H_

#include "adi_venc.h"

//*****************************************************************************
//*****************************************************************************
//** Defines and Macros
//*****************************************************************************
//*****************************************************************************
#define VENC_STATISTIC_FRAME_NUM_MAX 300


//*****************************************************************************
//*****************************************************************************
//** Enumerated types
//*****************************************************************************
//*****************************************************************************




//*****************************************************************************
//*****************************************************************************
//** Data Structures
//*****************************************************************************
//*****************************************************************************
typedef enum {
    ENC_SNAPSHOT_QUALITY_HIGHEST,
    ENC_SNAPSHOT_QUALITY_HIGH,
    ENC_SNAPSHOT_QUALITY_MEDIUM,
    ENC_SNAPSHOT_QUALITY_LOW,
    ENC_SNAPSHOT_QUALITY_NUM
} GAPP_ENC_SNAPSHOT_QualityEnumT;

typedef enum {
    ENC_SNAPSHOT_SIZE_MIN,
    ENC_SNAPSHOT_SIZE_MAX,
    ENC_SNAPSHOT_SIZE_AUTO,
    ENC_SNAPSHOT_SIZE_NUM
} GAPP_ENC_SNAPSHOT_ImageSizeEnumT;

typedef struct
{
    GADI_VENC_StreamFormatT streamFormat;
    GADI_VENC_H264ConfigT   h264Conf;
    GADI_VENC_H264QpConfigT h264QpConf;
    GADI_VENC_MjpegConfigT  mjpegConf;
}video_encode_stream;

//*****************************************************************************
//*****************************************************************************
//** API Functions
//*****************************************************************************
//*****************************************************************************

#ifdef __cplusplus
extern "C" {
#endif

GADI_ERR gdm_venc_init(void);
GADI_ERR gdm_venc_exit(void);
GADI_ERR gdm_venc_open(void);
GADI_ERR gdm_venc_close(void);
GADI_ERR gdm_venc_parse_config_file(char *path);
GADI_ERR gdm_venc_setup(void);
GADI_ERR gdm_venc_start_encode_stream(GADI_U32 streamId);
GADI_ERR gdm_venc_stop_encode_stream(GADI_U32 streamId);
GADI_ERR gdm_venc_get_stream_format(GADI_VENC_StreamFormatT *formatPar);
GADI_ERR gdm_venc_register_testcase(void);
GADI_ERR gdm_venc_get_bitrate(GADI_VENC_BitRateRangeT *bitrate);
GADI_ERR gdm_venc_set_bitrate(GADI_VENC_BitRateRangeT *bitrate);
GADI_ERR gdm_venc_set_framerate(GADI_VENC_FrameRateT *frameRate);
GADI_ERR gdm_venc_get_framerate(GADI_VENC_FrameRateT *pstFrameRate);
GADI_ERR gdm_venc_force_idr(int streamid);
GADI_ERR gdm_venc_set_gopn(GADI_VENC_H264GopConfigT *h264GopNconfig);
GADI_ERR gdm_venc_get_gopn(GADI_VENC_H264GopConfigT *h264GopNconfig);
GADI_ERR gdm_venc_set_h264_qp(GADI_VENC_H264QpConfigT  *h264QpConfig);
GADI_ERR gdm_venc_set_flip_rotate(int streamid, GADI_U8 flipRotate);
GADI_ERR gdm_venc_get_flip_rotate(int streamid, GADI_U8 *pstFlipRotate);
GADI_ERR gdm_venc_set_resolution(int streamid, int width, int height);
GADI_ERR gdm_venc_capture_jpeg(int streamid, char *path);
GADI_ERR gdm_venc_stop_encode_all_stream(void);
GADI_ERR gdm_venc_start_encode_all_stream(void);

#ifdef __cplusplus
    }
#endif


#endif /* _VENC_H_ */
