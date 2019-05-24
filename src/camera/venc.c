/******************************************************************************
** \file        adi/test/src/venc.c
**
** \brief       ADI layer venc module test.
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**              ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**              OMMISSIONS
**
** (C) Copyright 2013-2014 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/prctl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <assert.h>

#include "adi_types.h"
#include "adi_sys.h"
#include "adi_vi.h"
#include "adi_isp.h"
#include "adi_vout.h"

#include "basetypes.h"
#include "venc.h"
#include "shell.h"
#include "parser.h"
#include "media_fifo.h"
#include "rtsp.h"
#include "ircut.h"
#include "debug.h"
#include "isp.h"
//*****************************************************************************
//*****************************************************************************
//** Local Defines
//*****************************************************************************
//*****************************************************************************
#define VIDEO_ENCODE_MODE  GADI_VENC_MODE_NORMAL
#define SNAP_PICTRUE_PATH	"/tmp/snap.jpeg"
#define GK7101_VENC_JPEG_MIN_WIDTH  (64)
#define GK7101_VENC_JPEG_MIN_HEIGHT (64)
#define GK7101_VENC_MJPEG_FPS       (3)
#define GK7101_JPEG_STREAM          (5)

//*****************************************************************************
//*****************************************************************************
//** Local structures
//*****************************************************************************
//*****************************************************************************

//*****************************************************************************
//*****************************************************************************
//** Global Data
//*****************************************************************************
//*****************************************************************************
GADI_SYS_HandleT vencHandle  = NULL;
extern GADI_SYS_HandleT ispHandle;
extern GADI_BOOL        ispStart;
extern GADI_SYS_HandleT viHandle;
extern GADI_SYS_HandleT voHandle;
video_encode_stream streams[GADI_VENC_STREAM_NUM];

//*****************************************************************************
//*****************************************************************************
//** Local Data
//*****************************************************************************
//*****************************************************************************
static const char *vencShortOptions = "hb:f:g:I:S:P:q:Q:l:T:r:F:c:B:R:E:C:M:";

static struct option vencLongOptions[] =
{
	{"help",	    0, 0, 'h'},
	{"start",	    1, 0, 'S'},
	{"stop",	    1, 0, 'P'},
	{"h264qp",	    1, 0, 'q'},
	{"h264quality",	1, 0, 'Q'},
	{"idr",	        1, 0, 'I'},
	{"look",	    1, 0, 'l'},
	{"test",	    1, 0, 'T'},
    {"resolution",	1, 0, 'r'},
    {"framerate",   1, 0, 'f'},
    {"gopn",        1, 0, 'g'},
    {"flip",	    1, 0, 'F'},
	{"snap",	    1, 0, 'c'},
	{"bitrate",     1, 0, 'b'},
	{"bias",        1, 0, 'B'},
	{"roi",         1, 0, 'R'},
    {"reenc",       1, 0, 'E'},
    {"ictl",        1, 0, 'C'},
    {"mbqpctl",     1, 0, 'M'},
	{0, 		    0, 0, 0}
};

static parser_map vencStreamsMap[GADI_VENC_STREAM_NUM][MAX_STREAM_CONFIGNUM] =
{
    {
        {"id",                &streams[0].streamFormat.streamId,   DATA_TYPE_U32},
        {"type",              &streams[0].streamFormat.encodeType, DATA_TYPE_U8    },
        {"width",             &streams[0].streamFormat.width,      DATA_TYPE_U16},
        {"height",            &streams[0].streamFormat.height,     DATA_TYPE_U16},
        {"fps",               &streams[0].streamFormat.fps,        DATA_TYPE_U32},
        {"rotate",            &streams[0].streamFormat.flipRotate, DATA_TYPE_U8 },
        {"keep_aspect",       &streams[0].streamFormat.keepAspRat, DATA_TYPE_U8 },
        {"h264_id",           &streams[0].h264Conf.streamId,       DATA_TYPE_U32},
        {"h264_gop_mode",     &streams[0].h264Conf.gopModel,       DATA_TYPE_U8 },
        {"h264_gop_M",        &streams[0].h264Conf.gopM,           DATA_TYPE_U8 },
        {"h264_gop_N",        &streams[0].h264Conf.gopN,           DATA_TYPE_U8 },
        {"h264_idr_interval", &streams[0].h264Conf.idrInterval,    DATA_TYPE_U8 },
        {"h264_profile",      &streams[0].h264Conf.profile,        DATA_TYPE_U8 },//0: (default)CABAC-Main Profile, 1: CAVLC- Baseline Profile
        {"h264_bcr",          &streams[0].h264Conf.brcMode,        DATA_TYPE_U8 },//0:cbr, 1:vbr, 2: cbr keep quality, 3:vbr keep quality
        {"h264_cbr_avg_bps",  &streams[0].h264Conf.cbrAvgBps,      DATA_TYPE_U32},
        {"h264_vbr_min_bps",  &streams[0].h264Conf.vbrMinbps,      DATA_TYPE_U32},
        {"h264_vbr_max_bps",  &streams[0].h264Conf.vbrMaxbps,      DATA_TYPE_U32},
        {"h264_reenc_mode",   &streams[0].h264Conf.reEncMode,      DATA_TYPE_U8},
        {"mjpeg_id",          &streams[0].mjpegConf.streamId ,     DATA_TYPE_U32},
        {"mjpeg_quality",     &streams[0].mjpegConf.quality,       DATA_TYPE_U8 },
        {"mjpeg_chromaformat",&streams[0].mjpegConf.chromaFormat,  DATA_TYPE_U8 },
        {NULL,                        NULL,                        DATA_TYPE_U32},
    },
    {
        {"id",                &streams[1].streamFormat.streamId,   DATA_TYPE_U32},
        {"type",              &streams[1].streamFormat.encodeType, DATA_TYPE_U8    },
        {"width",             &streams[1].streamFormat.width,      DATA_TYPE_U16},
        {"height",            &streams[1].streamFormat.height,     DATA_TYPE_U16},
        {"fps",               &streams[1].streamFormat.fps,        DATA_TYPE_U32},
        {"rotate",            &streams[1].streamFormat.flipRotate, DATA_TYPE_U8 },
        {"keep_aspect",       &streams[1].streamFormat.keepAspRat, DATA_TYPE_U8 },
        {"h264_id",           &streams[1].h264Conf.streamId,       DATA_TYPE_U32},
        {"h264_gop_mode",     &streams[1].h264Conf.gopModel,       DATA_TYPE_U8 },
        {"h264_gop_M",        &streams[1].h264Conf.gopM,           DATA_TYPE_U8 },
        {"h264_gop_N",        &streams[1].h264Conf.gopN,           DATA_TYPE_U8 },
        {"h264_idr_interval", &streams[1].h264Conf.idrInterval,    DATA_TYPE_U8 },
        {"h264_profile",      &streams[1].h264Conf.profile,        DATA_TYPE_U8 },//0: (default)CABAC-Main Profile, 1: CAVLC- Baseline Profile
        {"h264_bcr",          &streams[1].h264Conf.brcMode,        DATA_TYPE_U8 },//0:cbr, 1:vbr, 2: cbr keep quality, 3:vbr keep quality
        {"h264_cbr_avg_bps",  &streams[1].h264Conf.cbrAvgBps,      DATA_TYPE_U32},
        {"h264_vbr_min_bps",  &streams[1].h264Conf.vbrMinbps,      DATA_TYPE_U32},
        {"h264_vbr_max_bps",  &streams[1].h264Conf.vbrMaxbps,      DATA_TYPE_U32},
        {"h264_reenc_mode",   &streams[1].h264Conf.reEncMode,      DATA_TYPE_U8},
        {"mjpeg_id",          &streams[1].mjpegConf.streamId ,     DATA_TYPE_U32},
        {"mjpeg_quality",     &streams[1].mjpegConf.quality,       DATA_TYPE_U8 },
        {"mjpeg_chromaformat",&streams[1].mjpegConf.chromaFormat,  DATA_TYPE_U8 },
        {NULL,                        NULL,                        DATA_TYPE_U32},
    },
    {
        {"id",                &streams[2].streamFormat.streamId,   DATA_TYPE_U32},
        {"type",              &streams[2].streamFormat.encodeType, DATA_TYPE_U8    },
        {"width",             &streams[2].streamFormat.width,      DATA_TYPE_U16},
        {"height",            &streams[2].streamFormat.height,     DATA_TYPE_U16},
        {"fps",               &streams[2].streamFormat.fps,        DATA_TYPE_U32},
        {"rotate",            &streams[2].streamFormat.flipRotate, DATA_TYPE_U8 },
        {"keep_aspect",       &streams[2].streamFormat.keepAspRat, DATA_TYPE_U8 },
        {"h264_id",           &streams[2].h264Conf.streamId,       DATA_TYPE_U32},
        {"h264_gop_mode",     &streams[2].h264Conf.gopModel,       DATA_TYPE_U8 },
        {"h264_gop_M",        &streams[2].h264Conf.gopM,           DATA_TYPE_U8 },
        {"h264_gop_N",        &streams[2].h264Conf.gopN,           DATA_TYPE_U8 },
        {"h264_idr_interval", &streams[2].h264Conf.idrInterval,    DATA_TYPE_U8 },
        {"h264_profile",      &streams[2].h264Conf.profile,        DATA_TYPE_U8 },//0: (default)CABAC-Main Profile, 1: CAVLC- Baseline Profile
        {"h264_bcr",          &streams[2].h264Conf.brcMode,        DATA_TYPE_U8 },//0:cbr, 1:vbr, 2: cbr keep quality, 3:vbr keep quality
        {"h264_cbr_avg_bps",  &streams[2].h264Conf.cbrAvgBps,      DATA_TYPE_U32},
        {"h264_vbr_min_bps",  &streams[2].h264Conf.vbrMinbps,      DATA_TYPE_U32},
        {"h264_vbr_max_bps",  &streams[2].h264Conf.vbrMaxbps,      DATA_TYPE_U32},
        {"h264_reenc_mode",   &streams[2].h264Conf.reEncMode,      DATA_TYPE_U8},
        {"mjpeg_id",          &streams[2].mjpegConf.streamId ,     DATA_TYPE_U32},
        {"mjpeg_quality",     &streams[2].mjpegConf.quality,       DATA_TYPE_U8 },
        {"mjpeg_chromaformat",&streams[2].mjpegConf.chromaFormat,  DATA_TYPE_U8 },
        {NULL,                        NULL,                        DATA_TYPE_U32},
    },
    {
        {"id",                &streams[3].streamFormat.streamId,   DATA_TYPE_U32},
        {"type",              &streams[3].streamFormat.encodeType, DATA_TYPE_U8    },
        {"width",             &streams[3].streamFormat.width,      DATA_TYPE_U16},
        {"height",            &streams[3].streamFormat.height,     DATA_TYPE_U16},
        {"fps",               &streams[3].streamFormat.fps,        DATA_TYPE_U32},
        {"rotate",            &streams[3].streamFormat.flipRotate, DATA_TYPE_U8 },
        {"keep_aspect",       &streams[3].streamFormat.keepAspRat, DATA_TYPE_U8 },
        {"h264_id",           &streams[3].h264Conf.streamId,       DATA_TYPE_U32},
        {"h264_gop_mode",     &streams[3].h264Conf.gopModel,       DATA_TYPE_U8 },
        {"h264_gop_M",        &streams[3].h264Conf.gopM,           DATA_TYPE_U8 },
        {"h264_gop_N",        &streams[3].h264Conf.gopN,           DATA_TYPE_U8 },
        {"h264_idr_interval", &streams[3].h264Conf.idrInterval,    DATA_TYPE_U8 },
        {"h264_profile",      &streams[3].h264Conf.profile,        DATA_TYPE_U8 },//0: (default)CABAC-Main Profile, 1: CAVLC- Baseline Profile
        {"h264_bcr",          &streams[3].h264Conf.brcMode,        DATA_TYPE_U8 },//0:cbr, 1:vbr, 2: cbr keep quality, 3:vbr keep quality
        {"h264_cbr_avg_bps",  &streams[3].h264Conf.cbrAvgBps,      DATA_TYPE_U32},
        {"h264_vbr_min_bps",  &streams[3].h264Conf.vbrMinbps,      DATA_TYPE_U32},
        {"h264_vbr_max_bps",  &streams[3].h264Conf.vbrMaxbps,      DATA_TYPE_U32},
        {"h264_reenc_mode",   &streams[3].h264Conf.reEncMode,      DATA_TYPE_U8},
        {"mjpeg_id",          &streams[3].mjpegConf.streamId ,     DATA_TYPE_U32},
        {"mjpeg_quality",     &streams[3].mjpegConf.quality,       DATA_TYPE_U8 },
        {"mjpeg_chromaformat",&streams[3].mjpegConf.chromaFormat,  DATA_TYPE_U8 },
        {NULL,                        NULL,                        DATA_TYPE_U32},
    }
};

static parser_map vencStreamsQpMap[GADI_VENC_STREAM_NUM][MAX_STREAM_CONFIGNUM] =
{
    {
        {"id",                &streams[0].streamFormat.streamId,    DATA_TYPE_U32},
        {"h264_adapt_qp",     &streams[0].h264QpConf.adaptQp,       DATA_TYPE_U8},
        {"h264_qp_min_I",     &streams[0].h264QpConf.qpMinOnI,      DATA_TYPE_U8},
        {"h264_qp_min_P",     &streams[0].h264QpConf.qpMinOnP,      DATA_TYPE_U8},
        {"h264_qpIWeight",    &streams[0].h264QpConf.qpIWeight,     DATA_TYPE_U8},
        {"h264_qpPWeight",    &streams[0].h264QpConf.qpPWeight,     DATA_TYPE_U8},
        {NULL,                        NULL,                         DATA_TYPE_U32},
    },
    {
        {"id",                &streams[1].streamFormat.streamId,    DATA_TYPE_U32},
        {"h264_adapt_qp",     &streams[1].h264QpConf.adaptQp,       DATA_TYPE_U8},
        {"h264_qp_min_I",     &streams[1].h264QpConf.qpMinOnI,      DATA_TYPE_U8},
        {"h264_qp_min_P",     &streams[1].h264QpConf.qpMinOnP,      DATA_TYPE_U8},
        {"h264_qpIWeight",    &streams[1].h264QpConf.qpIWeight,     DATA_TYPE_U8},
        {"h264_qpPWeight",    &streams[1].h264QpConf.qpPWeight,     DATA_TYPE_U8},
        {NULL,                        NULL,                         DATA_TYPE_U32},
    },
    {
        {"id",                &streams[2].streamFormat.streamId,    DATA_TYPE_U32},
        {"h264_adapt_qp",     &streams[2].h264QpConf.adaptQp,       DATA_TYPE_U8},
        {"h264_qp_min_I",     &streams[2].h264QpConf.qpMinOnI,      DATA_TYPE_U8},
        {"h264_qp_min_P",     &streams[2].h264QpConf.qpMinOnP,      DATA_TYPE_U8},
        {"h264_qpIWeight",    &streams[2].h264QpConf.qpIWeight,     DATA_TYPE_U8},
        {"h264_qpPWeight",    &streams[2].h264QpConf.qpPWeight,     DATA_TYPE_U8},
        {NULL,                        NULL,                         DATA_TYPE_U32},
    },
    {
        {"id",                &streams[3].streamFormat.streamId,    DATA_TYPE_U32},
        {"h264_adapt_qp",     &streams[3].h264QpConf.adaptQp,       DATA_TYPE_U8},
        {"h264_qp_min_I",     &streams[3].h264QpConf.qpMinOnI,      DATA_TYPE_U8},
        {"h264_qp_min_P",     &streams[3].h264QpConf.qpMinOnP,      DATA_TYPE_U8},
        {"h264_qpIWeight",    &streams[3].h264QpConf.qpIWeight,     DATA_TYPE_U8},
        {"h264_qpPWeight",    &streams[3].h264QpConf.qpPWeight,     DATA_TYPE_U8},
        {NULL,                        NULL,                         DATA_TYPE_U32},
    }
};

static pthread_mutex_t snapshotMutex;
GADI_SYS_SemHandleT jpegCapSem;
static GADI_BOOL    snapshot_inited = GADI_FALSE;
static GADI_BOOL    exit_snapshot = GADI_FALSE;
static GADI_U16     env_stream_count = 0;
static u16 mbQpThrdsLow[15]  =  {18,35,56,80,108,140,175,212,251,293,339,390,448,520,617};
static u16 mbQpThrdsMid[15]  =  {1, 4, 16, 32, 64, 128, 256, 512, 1024, 2048,  4096,
                        8192, 16384, 32768};
static u16 mbQpThrdsHigh[15] =  {65535, 65535, 65535, 65535, 65535, 65535, 65535 ,
                          65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535};

static s8 mbQpDeltaLow[16]  =  {-3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
static s8 mbQpDeltaMid[16]  =  {1,   1,  2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4,  4, 4};
static s8 mbQpDeltaHigh[16] =  {0,   0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0};


//*****************************************************************************
//*****************************************************************************
//** Local Functions Declaration
//*****************************************************************************
//*****************************************************************************
static GADI_ERR check_encode_streams_params( GADI_U32 viWidth, GADI_U32 viHeight);
static GADI_ERR calculate_encode_size_order(GADI_U32 *order, GADI_U32 streamsNum);
static GADI_ERR get_channels_params(GADI_VENC_ChannelsParamsT* chansParams);
static GADI_ERR handle_venc_command(int argc, char* argv[]);
static void venc_usage(void);
static GADI_VOID _snapshot_init(GADI_VOID);
static GADI_ERR _snapshot_set_source_streamid(GADI_U32 streamid,
    GADI_U8 quality, GADI_U16 width, GADI_U16 height);
static GADI_ERR _snapshot_cancel_source_streamid(GADI_VOID);
static int _snapshot_to_file(GADI_S32 streamId, GAPP_ENC_SNAPSHOT_QualityEnumT quality,
        GAPP_ENC_SNAPSHOT_ImageSizeEnumT width, GAPP_ENC_SNAPSHOT_ImageSizeEnumT height, FILE* filename);
static void venc_daytonight(GADI_U32 value);
static void venc_nighttoday(GADI_U32 value);

//*****************************************************************************
//*****************************************************************************
//** API Functions
//*****************************************************************************
//*****************************************************************************
static void video_test(GADI_U32 operaNum)
{
	//VI test
	if(operaNum == 0){
		GADI_ERR retVal = GADI_OK;
        GADI_U32 viWidth = 0,	viHeight = 0;
        GADI_VI_SettingParamsT viAttr;
        static GADI_U32 OperaMode = 0;

		//gadi_vi_get_resolution tesst
		retVal = gadi_vi_get_resolution(viHandle, &viWidth, &viHeight);
		if (retVal != GADI_OK) {
			GADI_ERROR("gadi_vi_get_resolution fail\n");
		} else {
			GADI_INFO("viWidth:%d viHeight:%d\n", viWidth, viHeight);
		}

		//gadi_vi_get_params test
		retVal = gadi_vi_get_params(viHandle, &viAttr);
		if (retVal != GADI_OK) {
			GADI_ERROR("gadi_vi_get_params fail\n");
		} else {
			GADI_INFO("gadi_vi_get_params ok!\n");
			GADI_INFO("reslution mode:%d frameRate:%d\n", viAttr.resoluMode, viAttr.frameRate);
		}

		//gadi_vi_get_mirror_mode test
		retVal = gadi_vi_get_mirror_mode(viHandle, &viAttr.mirrorMode);
		if (retVal != GADI_OK) {
			GADI_ERROR("gadi_vi_get_mirror_mode fail\n");
		} else {
			GADI_INFO("gadi_vi_get_mirror_mode ok!\n");
		}
		//gadi_vi_get_operation_mode test
		retVal = gadi_vi_get_operation_mode(viHandle, &OperaMode);
		if (retVal != GADI_OK) {
			GADI_ERROR("gadi_vi_get_operation_mode fail\n");
		} else {
			GADI_INFO("gadi_vi_get_operation_mode ok!\n");
		}
	}
	//venc test
	else if(operaNum == 1) {
		GADI_ERR retVal = GADI_OK;
		GADI_VENC_ChannelStateT vencChannelStat;
		GADI_VENC_ChannelsParamsT vencChannelsParm;
		GADI_VENC_StreamFormatT vencStreamFormat;
        GADI_VENC_H264ConfigT vencH264;
        GADI_VENC_MjpegConfigT vencMJPEG;
        GADI_VENC_DptzParamT dptzPar;
        GADI_VENC_DptzOrgParamT vencDptzOrgPar;
        GADI_CHN_AttrT chnAttr;

		//gadi_venc_get_channel_state test
		retVal = gadi_venc_get_channel_state(vencHandle,
				&vencChannelStat);
		if (retVal != GADI_OK) {
			GADI_ERROR("gadi_venc_get_channel_state fail\n");
		} else {
			GADI_INFO("gadi_venc_get_channel_state ok!\n");
		}
		//gadi_venc_get_channels_params test
		retVal = gadi_venc_get_channels_params(vencHandle,
				&vencChannelsParm);
		if (retVal != GADI_OK) {
			GADI_ERROR("gadi_venc_get_channels_params fail\n");
		} else {
			GADI_INFO("gadi_venc_get_channels_params ok!\n");
		}
		//gadi_venc_get_channels_params test
		vencStreamFormat.streamId = 0;
		retVal = gadi_venc_get_stream_format(vencHandle,
				&vencStreamFormat);
		if (retVal != GADI_OK) {
			GADI_ERROR("gadi_venc_get_stream_format fail\n");
		} else {
			GADI_INFO("gadi_venc_get_stream_format ok!\n");
		}
        //gadi_venc_get_h264_config test
        vencH264.streamId = 0;
		retVal = gadi_venc_get_h264_config(vencHandle, &vencH264);
		if (retVal != GADI_OK) {
			GADI_ERROR("gadi_venc_get_h264_config fail\n");
		} else {
			GADI_INFO("gadi_venc_get_h264_config ok!\n");
            //gadi_venc_set_bitrate test
            GADI_VENC_BitRateRangeT vencBitRateRange;
            vencBitRateRange.brcMode = vencH264.brcMode;
            vencBitRateRange.cbrAvgBps = vencH264.cbrAvgBps;
            vencBitRateRange.streamId = vencH264.streamId;
            vencBitRateRange.vbrMaxbps = vencH264.vbrMaxbps;
            vencBitRateRange.vbrMinbps = vencH264.vbrMinbps;

            vencBitRateRange.streamId = 0;
    		retVal = gadi_venc_set_bitrate(vencHandle, &vencBitRateRange);
    		if (retVal != GADI_OK) {
    			GADI_ERROR("gadi_venc_set_bitrate fail\n");
    		} else {
    			GADI_INFO("gadi_venc_set_bitrate ok!\n");
    		}
		}
        //gadi_venc_get_mjpeg_config test
        vencMJPEG.streamId = 0;
		retVal = gadi_venc_get_mjpeg_config(vencHandle, &vencMJPEG);
		if (retVal != GADI_OK) {
			GADI_ERROR("gadi_venc_get_mjpeg_config fail\n");
		} else {
			GADI_INFO("gadi_venc_get_mjpeg_config ok!\n");
		}
        //gadi_venc_get_dptz_param test
		retVal = gadi_venc_get_dptz_param(vencHandle, &dptzPar);
		if (retVal != GADI_OK) {
			GADI_ERROR("gadi_venc_get_dptz_param fail\n");
		} else {
			GADI_INFO("gadi_venc_get_dptz_param ok!\n");
            GADI_INFO("id:%d factor:%d offsetX:%d offsetY:%d\n",
                    dptzPar.channelId, dptzPar.zoomFactor, dptzPar.offsetX, dptzPar.offsetY);

            //gadi_venc_get_dptz_org_param test
    		retVal = gadi_venc_get_dptz_org_param(vencHandle,&dptzPar,&vencDptzOrgPar);
    		if (retVal != GADI_OK) {
    			GADI_ERROR("gadi_venc_get_dptz_org_param fail\n");
    		} else {
    			GADI_INFO("gadi_venc_get_dptz_org_param ok!\n");
                GADI_INFO("id:%d, zoomX:%d zoomY:%d offsetX:%d offsetY:%d\n",
                    vencDptzOrgPar.channelId, vencDptzOrgPar.zoomFactorX, vencDptzOrgPar.zoomFactorY,
                    vencDptzOrgPar.offsetX, vencDptzOrgPar.offsetY);
    		}
            //gadi_venc_get_dptz_org_param test
    		retVal = gadi_venc_set_dptz_param(vencHandle,&dptzPar);
    		if (retVal != GADI_OK) {
    			GADI_ERROR("gadi_venc_get_dptz_org_param fail\n");
    		} else {
    			GADI_INFO("gadi_venc_get_dptz_org_param ok!\n");
    		}
		}

        //gadi_venc_query test
        chnAttr.id = 0;
		retVal = gadi_venc_query(vencHandle, 0, &chnAttr);
		if (retVal != GADI_OK) {
			GADI_ERROR("gadi_venc_query fail\n");
		} else {
			GADI_INFO("gadi_venc_query ok!\n");
            GADI_INFO("stream id:%d, type:%d, state:%d\n",
               chnAttr.id, chnAttr.type, chnAttr.state);
		}


	}


}

GADI_ERR gdm_venc_init(void)
{
    GADI_ERR retVal = GADI_OK;

    retVal = gadi_venc_init();
	/*init the snapshot mutex*/
    pthread_mutex_init(&snapshotMutex, NULL);
	jpegCapSem = gadi_sys_sem_create(0);

    return retVal;
}

GADI_ERR gdm_venc_exit(void)
{
    GADI_ERR retVal = GADI_OK;

    retVal = gadi_venc_exit();
	pthread_mutex_destroy(&snapshotMutex);

    return retVal;
}

GADI_ERR gdm_venc_open(void)
{
    GADI_ERR retVal = GADI_OK;
    GADI_VENC_OpenParamsT vencOpenParams;
	GADI_VENC_DspMapInfoT info;

    gadi_sys_memset(&vencOpenParams, 0, sizeof(GADI_VENC_OpenParamsT));
    vencOpenParams.viHandle   = viHandle;
    vencOpenParams.voutHandle = voHandle;
    vencHandle = gadi_venc_open(&vencOpenParams, &retVal);
    if(retVal != GADI_OK)
    {
        GADI_ERROR("gadi_venc_open error\n");
        return retVal;
    }
    retVal = gadi_venc_map_bsb(vencHandle);
    if(retVal != GADI_OK)
    {
        GADI_ERROR("gadi_venc_map_bsb fail\n");
    }

    retVal = gadi_venc_map_dsp(vencHandle, &info);
    if(retVal != GADI_OK)
    {
        GADI_ERROR("gadi_venc_map_dsp fail\n");
    }

    ircut_register_daytonight(0, &venc_daytonight);
    ircut_register_nighttoday(0, &venc_nighttoday);

    return retVal;
}

GADI_ERR gdm_venc_close(void)
{
    GADI_ERR retVal = GADI_OK;

    if(env_stream_count != 0){
        int i;
        for(i = 0; i < GADI_VENC_STREAM_NUM; i++){
            gdm_venc_stop_encode_stream(i);
        }
    }

    retVal = gadi_venc_close(vencHandle);
    if(retVal != GADI_OK)
    {
        GADI_ERROR("gadi_venc_close error\n");
        return retVal;
    }


    return retVal;
}

GADI_ERR gdm_venc_parse_config_file(char *path)
{
    GADI_ERR retVal = GADI_OK;
    int streamNum = 0;
    int i = 0;

    if(path == NULL){
        return -1;
    }

    /*parse encode streams parameters.*/
    streamNum = parse_streamNum(path);
    if(streamNum <= 0 || streamNum > GADI_VENC_STREAM_NUM){
        return -1;
    }

    for(i = 0; i < streamNum; i++){
        retVal = parse_streamInfo(vencStreamsMap[i], path, i);
        if(retVal != 0){
            GADI_ERROR("load encode info failed!\n");
            return -1;
        }
    }

    for(i = 0; i < streamNum; i++){
        retVal = parse_streamInfo(vencStreamsQpMap[i], path, i);
        if(retVal != 0){
            GADI_ERROR("load encode info failed!\n");
            return -1;
        }
    }

    for(i = 0; i < 4; i++)
    {
        DBG_PRINTF("streamFormat.streamId:%d,encodeType:%d,width:%d,height:%d,Fps:%d,flipRotate:%d,keepAspRat:%d,h264Conf.streamId:%d,gopModel:%d,gopM:%d,gopN:%d\n",
                                                            streams[i].streamFormat.streamId,
                                                            streams[i].streamFormat.encodeType,
                                                            streams[i].streamFormat.width,
                                                            streams[i].streamFormat.height,
                                                            streams[i].streamFormat.fps,
                                                            streams[i].streamFormat.flipRotate,
                                                            streams[i].streamFormat.keepAspRat,
                                                            streams[i].h264Conf.streamId,
                                                            streams[i].h264Conf.gopModel,
                                                            streams[i].h264Conf.gopM,
                                                            streams[i].h264Conf.gopN
                                                            );

        DBG_PRINTF("idrInterval:%d,profile:%d,brcMode:%d,cbrAvgBps:%d,vbrMinbps:%d,vbrMaxbps:%d,mjpegConf.streamId:%d,quality:%d,chromaFormat:%d\n",
                                                            streams[i].h264Conf.idrInterval,
                                                            streams[i].h264Conf.profile,
                                                            streams[i].h264Conf.brcMode,
                                                            streams[i].h264Conf.cbrAvgBps,
                                                            streams[i].h264Conf.vbrMinbps,
                                                            streams[i].h264Conf.vbrMaxbps,
                                                            streams[i].mjpegConf.streamId,
                                                            streams[i].mjpegConf.quality,
                                                            streams[i].mjpegConf.chromaFormat
                                                            );
    }

    return retVal;
}

GADI_ERR gdm_venc_setup(void)
{
    GADI_VENC_ChannelsParamsT chansParams;
	GADI_U32 enable;
	GADI_U32 cnt;
    GADI_U32 viWidth;
	GADI_U32 viHeight;
    GADI_U8  encodeType;
	GADI_ERR retVal         = GADI_OK;

    /*video input module: ensure exit preview status.*/
    enable = 0;
    retVal = gadi_vi_enable(viHandle, enable);
    if(retVal != GADI_OK)
    {
        GADI_ERROR("gadi_vi_enable error\n");
        return retVal;
    }

    /*check encode streams parameters.*/
    retVal = gadi_vi_get_resolution(viHandle, &viWidth, &viHeight);
    if(retVal != GADI_OK)
    {
        GADI_ERROR("gadi_vi_get_resolution error\n");
        return retVal;
    }
    retVal = check_encode_streams_params(viWidth, viHeight);
    if(retVal != GADI_OK)
    {
        GADI_ERROR("check_encode_streams_params error\n");
        return retVal;
    }

    /*get encode channels parameters.*/
    retVal = get_channels_params(&chansParams);
    if(retVal != GADI_OK)
    {
        GADI_ERROR("get_channels_params error\n");
        return retVal;
    }

    /*set encode channels parameters.*/
    retVal = gadi_venc_set_channels_params(vencHandle, &chansParams);
    if(retVal != GADI_OK)
    {
        GADI_ERROR("set channel parameters error.\n");
        return retVal;
    }

    /* if stream four is no used, use to capture picture. */
    _snapshot_init();

    for (cnt = 0; cnt < GADI_VENC_STREAM_NUM; cnt++)
    {
        encodeType = streams[cnt].streamFormat.encodeType;
        if(encodeType != 0)
        {
            if (gadi_venc_set_stream_format(vencHandle, &streams[cnt].streamFormat) < 0)
            {
                GADI_ERROR("gadi_venc_set_stream_format error, steamId=%d\n",cnt);
                return -1;
            }
            GADI_VENC_StreamFormatT vencStreamFormat;
            vencStreamFormat.streamId = cnt;
            if (gadi_venc_get_stream_format(vencHandle, &vencStreamFormat) != GADI_OK)
            {
                GADI_ERROR("gadi_venc_get_stream_format fail\n");
                return -1;
            }
            streams[cnt].streamFormat.fps = vencStreamFormat.fps;
            if(encodeType == GADI_VENC_TYPE_H264)
            {
                if(gadi_venc_set_h264_config(vencHandle, &streams[cnt].h264Conf) < 0)
                {
                    GADI_ERROR("gadi_venc_set_h264_config error, steamId=%d\n",cnt);
                    return -1;
                }
                streams[cnt].h264QpConf.streamId = cnt;
                streams[cnt].h264QpConf.qpMaxOnI = 51;
                streams[cnt].h264QpConf.qpMaxOnP = 51;
                if(gadi_venc_set_h264_qp_config(vencHandle, &streams[cnt].h264QpConf) < 0)
                {
                    GADI_ERROR("gadi_venc_set_h264_qp_config error, steamId=%d\n",cnt);
                    return -1;
                }
            }
            else if(encodeType == GADI_VENC_TYPE_MJPEG)
            {
                if(gadi_venc_set_mjpeg_config(vencHandle, &streams[cnt].mjpegConf) < 0)
                {
                    GADI_ERROR("gadi_venc_set_mjpeg_config error, steamId=%d\n",cnt);
                    return -1;
                }
            }
        }
    }

    enable = 1;
    if(gadi_vi_enable(viHandle, enable) < 0)
    {
        GADI_ERROR("gadi_vi_enable error.\n");
        return -1;
    }

    _snapshot_cancel_source_streamid();

    return retVal;
}

GADI_ERR gdm_venc_start_encode_stream(GADI_U32 streamId)
{
    GADI_ERR retVal = GADI_OK;
    GADI_U8  encodeType;

    if (streamId < GADI_VENC_STREAM_NUM) {
        encodeType = streams[streamId].streamFormat.encodeType;
        if(encodeType != 0)
        {
            if(gadi_venc_start_stream(vencHandle, streamId) < 0)
            {
                GADI_ERROR("gadi_venc_start_stream error, streams:%d\n", streamId);

                if(gadi_venc_check_encoder_state(vencHandle) < 0)
                {
                    GADI_ERROR("encoder state error.\n");
                }
                return -1;
            }

            assert(env_stream_count >= 0);
            /* reset 3a algo */
            if(env_stream_count == 0) {
                if(ispStart) {
                    gadi_isp_reset_3a_static(ispHandle);
                }
            }
            if(encodeType == GADI_VENC_TYPE_H264){
                rtsp_create_media_stream(streamId,
                    streams[streamId].streamFormat.width * streams[streamId].streamFormat.height);
            }
            env_stream_count |= 1 << streamId;
        }
    } else {
        GADI_ERROR("No this video encode stream! streamId[0~4]\n");
    }

    return retVal;
}

GADI_ERR gdm_venc_stop_encode_stream(GADI_U32 streamId)
{
    GADI_ERR   retVal =  GADI_OK;
    GADI_U8  encodeType;

    if (streamId < GADI_VENC_STREAM_NUM) {
        encodeType = streams[streamId].streamFormat.encodeType;
        if(encodeType != 0)
        {
            if(gadi_venc_stop_stream(vencHandle, streamId) < 0)
            {
                GADI_ERROR("gadi_venc_stop_stream error, streams:%d\n", streamId);
                return -1;
            }
            GADI_INFO("stop stream[%d]\n",streamId);
            env_stream_count &= ~(1 << streamId);
        }
        if(encodeType == GADI_VENC_TYPE_H264){
            rtsp_destory_media_stream(streamId);
        }
    }
    assert(env_stream_count >= 0);

    return retVal;
}



GADI_ERR gdm_venc_capture_jpeg(int streamid, char *path)
{
    FILE *file = NULL;
    int retVal = 0;

    if(path == NULL){
        GADI_ERROR("No image path\n");
        return -1;
    }
    file = fopen(path, "w+");
    if(file == NULL){
        GADI_ERROR("can not open %s\n", path);
        return -1;
    }
    //printf("streamid:%d path:%s\n", streamid, path);
    retVal = _snapshot_to_file(streamid, ENC_SNAPSHOT_QUALITY_MEDIUM,
        ENC_SNAPSHOT_SIZE_AUTO, ENC_SNAPSHOT_SIZE_AUTO, file);
    if(retVal < 0)
    {
        fclose(file);
        GADI_ERROR("capture picture failed\n");
        return -1;
    }
    fclose(file);

    return 0;
}

GADI_ERR gdm_venc_get_bitrate(GADI_VENC_BitRateRangeT *bitrate)
{

    GADI_ERR retVal = GADI_OK;

    if(bitrate->streamId >= GADI_VENC_STREAM_NUM){
        return GADI_ERR_BAD_PARAMETER;
    }

    retVal = gadi_venc_get_bitrate(vencHandle, bitrate);
    if(retVal != GADI_OK){
        GADI_ERROR("get bitrate failed. [%d]\n",retVal);
        return retVal;
    }

    streams[bitrate->streamId].h264Conf.cbrAvgBps = bitrate->cbrAvgBps;
    streams[bitrate->streamId].h264Conf.brcMode = bitrate->brcMode;
    streams[bitrate->streamId].h264Conf.vbrMaxbps = bitrate->vbrMaxbps;
    streams[bitrate->streamId].h264Conf.vbrMinbps = bitrate->vbrMinbps;

    return GADI_OK;
}

GADI_ERR gdm_venc_set_bitrate(GADI_VENC_BitRateRangeT *bitrate)
{

    GADI_ERR retVal = GADI_OK;

    if(bitrate->streamId >= GADI_VENC_STREAM_NUM){
        return GADI_ERR_BAD_PARAMETER;
    }

    retVal = gadi_venc_set_bitrate(vencHandle, bitrate);
    if(retVal != GADI_OK){
        GADI_ERROR("set bitrate failed. [%d]\n",retVal);
        return retVal;
    }

    streams[bitrate->streamId].h264Conf.cbrAvgBps = bitrate->cbrAvgBps;
    streams[bitrate->streamId].h264Conf.brcMode = bitrate->brcMode;
    streams[bitrate->streamId].h264Conf.vbrMaxbps = bitrate->vbrMaxbps;
    streams[bitrate->streamId].h264Conf.vbrMinbps = bitrate->vbrMinbps;

    return GADI_OK;
}

GADI_ERR gdm_venc_set_framerate(GADI_VENC_FrameRateT *pstFrameRate)
{

    GADI_VENC_StreamFormatT stream_format;
    GADI_ERR retVal = GADI_OK;

    if(pstFrameRate->streamId >= GADI_VENC_STREAM_NUM){
        return GADI_ERR_BAD_PARAMETER;
    }

    retVal = gadi_venc_set_framerate(vencHandle, pstFrameRate);
    if(retVal != GADI_OK){
        GADI_ERROR("set framerate failed. [%d]\n",retVal);
        return GADI_ERR_FROM_DRIVER;
    }

	stream_format.streamId = pstFrameRate->streamId;

    retVal = gadi_venc_get_stream_format(vencHandle, &stream_format);
    if(retVal != GADI_OK){
        GADI_ERROR("get stream format failed. [%d]\n",retVal);
        return retVal;
    }

    GADI_INFO("streams[%d] fps: %d\n",stream_format.streamId,stream_format.fps);

    streams[pstFrameRate->streamId].streamFormat.fps = stream_format.fps;

    return retVal;
}

GADI_ERR gdm_venc_get_framerate(GADI_VENC_FrameRateT *pstFrameRate)
{

    GADI_VENC_StreamFormatT stream_format;
    GADI_ERR retVal = GADI_OK;

    if(pstFrameRate->streamId >= GADI_VENC_STREAM_NUM){
        return GADI_ERR_BAD_PARAMETER;
    }

	stream_format.streamId = pstFrameRate->streamId;

    retVal = gadi_venc_get_stream_format(vencHandle, &stream_format);
    if(retVal != GADI_OK){
        GADI_ERROR("get stream format failed. [%d]\n",retVal);
        return retVal;
    }

	pstFrameRate->fps = stream_format.fps;

    GADI_INFO("streams[%d] fps: %d\n",stream_format.streamId,stream_format.fps);

    streams[pstFrameRate->streamId].streamFormat.fps = stream_format.fps;

    return retVal;
}

GADI_ERR gdm_venc_force_idr(int streamid)
{
    GADI_ERR retVal = GADI_OK;

    if(streamid >= GADI_VENC_STREAM_NUM){
        return GADI_ERR_BAD_PARAMETER;
    }

    retVal = gadi_venc_force_idr(vencHandle, streamid);
    if(retVal < 0) {
        GADI_ERROR("gadi_venc_force_idr failed!\n");
        return retVal;
    }

    return retVal;
}

GADI_ERR gdm_venc_set_flip_rotate(int streamid, GADI_U8 flipRotate)
{
    GADI_ERR retVal = GADI_OK;
    GADI_VENC_StreamFormatT stream_format;
    GADI_VENC_H264ConfigT h264_config;
    GADI_VENC_MjpegConfigT mjpeg_config;

    if(streamid >= GADI_VENC_STREAM_NUM) {
        return GADI_ERR_BAD_PARAMETER;
    }

    retVal = gdm_venc_stop_encode_all_stream();
    if(retVal < 0) {
        GADI_ERROR("gdm_venc_stop_encode_all_stream failed!\n");
        return retVal;
    }

    retVal = gadi_vi_enable(viHandle, 0);
    if(retVal < 0)
    {
        GADI_ERROR("gadi_vi_enable failed!\n");
        goto exit_start_stream;
    }

    stream_format.streamId = streamid;
    retVal = gadi_venc_get_stream_format(vencHandle, &stream_format);
    if(retVal != GADI_OK){
        GADI_ERROR("gadi_venc_get_stream_format failed!\n");
        goto exit_vi_enable;
    }

    if(stream_format.encodeType == GADI_VENC_TYPE_H264) {

        h264_config.streamId = streamid;
        retVal = gadi_venc_get_h264_config(vencHandle, &h264_config);
	    if (retVal != GADI_OK) {
		    GADI_ERROR("gadi_venc_get_h264_config failed!\n");
            goto exit_vi_enable;
	    }

        stream_format.flipRotate = flipRotate;
        retVal = gadi_venc_set_stream_format(vencHandle, &stream_format);
        if (retVal != GADI_OK)  {
            GADI_ERROR("gadi_venc_set_stream_format failed!\n");
            goto exit_vi_enable;
        }

        retVal = gadi_venc_set_h264_config(vencHandle, &h264_config);
	    if (retVal != GADI_OK) {
		    GADI_ERROR("gadi_venc_set_h264_config fail\n");
            goto exit_vi_enable;
        }
	} else if(stream_format.encodeType == GADI_VENC_TYPE_MJPEG) {

        mjpeg_config.streamId = streamid;
        retVal = gadi_venc_get_mjpeg_config(vencHandle, &mjpeg_config);
        if (retVal != GADI_OK) {
            GADI_ERROR("gadi_venc_get_mjpeg_config failed!\n");
            goto exit_vi_enable;
        }

        stream_format.flipRotate = flipRotate;
        retVal = gadi_venc_set_stream_format(vencHandle, &stream_format);
        if (retVal != GADI_OK)  {
            GADI_ERROR("gadi_venc_set_stream_format failed!\n");
            goto exit_vi_enable;
        }

        retVal = gadi_venc_set_mjpeg_config(vencHandle, &mjpeg_config);
        if (retVal != GADI_OK) {
            GADI_ERROR("gadi_venc_set_h264_config fail\n");
            goto exit_vi_enable;
        }

    }else {
        goto exit_vi_enable;
    }

    streams[streamid].streamFormat.flipRotate = flipRotate;

exit_vi_enable:
    if(gadi_vi_enable(viHandle, 1) < 0) {
        GADI_ERROR("gadi_vi_enable failed!\n");
    }

exit_start_stream:
    if(gdm_venc_start_encode_all_stream() < 0) {
        GADI_ERROR("gdm_venc_start_encode_all_stream failed!\n");
    }

    return retVal;
}

GADI_ERR gdm_venc_get_flip_rotate(int streamid, GADI_U8 *pstFlipRotate)
{
    GADI_ERR retVal = GADI_OK;
    GADI_VENC_StreamFormatT stream_format;

    stream_format.streamId = streamid;
    retVal = gadi_venc_get_stream_format(vencHandle, &stream_format);
    if(retVal != GADI_OK){
        GADI_ERROR("gadi_venc_get_stream_format failed!\n");
    }

    *pstFlipRotate = stream_format.flipRotate;

    return retVal;
}

GADI_ERR gdm_venc_set_resolution(int streamid, int width, int height)
{
    GADI_ERR retVal = GADI_OK;
    GADI_U32 viWidth;
    GADI_U32 viHeight;
    unsigned char cnt;
    GADI_VENC_StreamFormatT *stream_format;
    GADI_VENC_ChannelsParamsT chansParams;


    if(streamid >= GADI_VENC_STREAM_NUM){
        return GADI_ERR_BAD_PARAMETER;
    }

    retVal = gdm_venc_stop_encode_all_stream();
    if(retVal < 0) {
        GADI_ERROR("gdm_venc_stop_encode_all_stream failed!\n");
        return retVal;
    }

    retVal = gadi_vi_enable(viHandle, 0);
    if(retVal < 0)
    {
        GADI_ERROR("gadi_vi_enable failed!\n");
        goto exit_start_stream;
    }

    /*change the stream resolution.*/
    stream_format = &(streams[streamid].streamFormat);
    stream_format->width  = width;
    stream_format->height = height;


    /*check encode streams parameters.*/
    retVal = gadi_vi_get_resolution(viHandle, &viWidth, &viHeight);
    if(retVal != GADI_OK)
    {
        GADI_ERROR("gadi_vi_get_resolution failed!\n");
        return -1;
    }
    retVal = check_encode_streams_params(viWidth, viHeight);
    if(retVal != GADI_OK)
    {
        GADI_ERROR("check_encode_streams_params failed!\n");
        return -1;
    }

    retVal = get_channels_params(&chansParams);
    if(retVal != GADI_OK)
    {
        GADI_ERROR("get_channels_params error\n");
        return -1;
    }

    /*set channels parameters.*/
    retVal = gadi_venc_set_channels_params(vencHandle, &chansParams);
    if(retVal != GADI_OK) {
        GADI_ERROR("gadi_venc_set_channels_params failed!\n");
        goto exit_vi_enable;
    }

    for(cnt = 0; cnt < GADI_VENC_STREAM_NUM; cnt++)
    {
        if(streams[cnt].streamFormat.encodeType != 0)
        {
            retVal = gadi_venc_set_stream_format(vencHandle, &streams[cnt].streamFormat);
            if (retVal != GADI_OK)
            {
                GADI_ERROR("gadi_venc_set_stream_format error, steamId=%d\n",streamid);
                goto exit_vi_enable;
            }
        }
    }

exit_vi_enable:
    if(gadi_vi_enable(viHandle, 1) < 0) {
        GADI_ERROR("gadi_vin_enter_preview enter error.\n");
    }
exit_start_stream:
    if(gdm_venc_start_encode_all_stream() < 0) {
        GADI_ERROR("gdm_venc_start_encode_all_stream failed!\n");
    }

    return retVal;
}

GADI_ERR gdm_venc_get_stream_format(GADI_VENC_StreamFormatT *formatAttr)
{
	GADI_ERR retVal = GADI_OK;

	retVal = gadi_venc_get_stream_format(vencHandle, formatAttr);
	if (retVal != GADI_OK) {
		GADI_ERROR("gadi_venc_get_stream_format fail!\n");
		return retVal;
	}

	return retVal;
}

GADI_ERR gdm_venc_set_gopn(GADI_VENC_H264GopConfigT *h264Gopconfig)
{
    GADI_ERR retVal = GADI_OK;

    if(h264Gopconfig->streamId >= GADI_VENC_STREAM_NUM){
        return GADI_ERR_BAD_PARAMETER;
    }

    retVal = gadi_venc_set_h264_gop(vencHandle, h264Gopconfig);
    if (retVal != GADI_OK) {
        GADI_ERROR("gadi_venc_set_h264_gop fail!\n");
        return retVal;
    }

    if(retVal == GADI_OK) {
		streams[h264Gopconfig->streamId].h264Conf.gopN = h264Gopconfig->gopN;
	}

    return retVal;
}

GADI_ERR gdm_venc_get_gopn(GADI_VENC_H264GopConfigT *h264Gopconfig)
{
    GADI_ERR retVal = GADI_OK;

    if(h264Gopconfig->streamId >= GADI_VENC_STREAM_NUM){
        GADI_ERROR("gdm_venc_get_gopn error streamId :%d!\n", h264Gopconfig->streamId);
        return GADI_ERR_BAD_PARAMETER;
    }

    retVal = gadi_venc_get_h264_gop(vencHandle, h264Gopconfig);
    if (retVal != GADI_OK) {
        GADI_ERROR("gadi_venc_get_h264_gop fail!\n");
        return retVal;
    }

    if(retVal == GADI_OK) {
		streams[h264Gopconfig->streamId].h264Conf.gopN = h264Gopconfig->gopN;
	}

    return retVal;
}

GADI_ERR gdm_venc_set_h264_qp(GADI_VENC_H264QpConfigT  *h264QpConfig)
{
    GADI_ERR retVal = GADI_OK;

    if(h264QpConfig->streamId >= GADI_VENC_STREAM_NUM){
        return GADI_ERR_BAD_PARAMETER;
    }

    retVal = gadi_venc_set_h264_qp_config(vencHandle, h264QpConfig);
    if (retVal != GADI_OK) {
        GADI_ERROR("gadi_venc_set_h264_qp_config fail!\n");
        return retVal;
    }

    return retVal;
}

GADI_ERR gdm_venc_get_h264_qp(GADI_VENC_H264QpConfigT  *h264QpConfig)
{
    GADI_ERR retVal = GADI_OK;

    if(h264QpConfig->streamId >= GADI_VENC_STREAM_NUM){
        return GADI_ERR_BAD_PARAMETER;
    }

    retVal = gadi_venc_get_h264_qp_config(vencHandle, h264QpConfig);
    if (retVal != GADI_OK) {
        GADI_ERROR("gadi_venc_get_h264_qp_config fail!\n");
        return retVal;
    }

    return retVal;
}

GADI_ERR gdm_venc_register_testcase(void)
{
    GADI_ERR   retVal =  GADI_OK;
    (void)shell_registercommand (
        "venc",
        handle_venc_command,
        "venc command",
        "---------------------------------------------------------------------\n"
        "venc -h \n"
        "   brief : help\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "venc -b \n"
        "   brief : set bitratae\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "venc -f\n"
        "   brief : set venc framerate\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "venc -S\n"
        "   brief :  start video stream\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "venc -P\n"
        "   brief : stop video stream\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "venc -l \n"
        "   brief : look video attributes.\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "venc -F \n"
        "   brief : set stream flip\n"
        "---------------------------------------------------------------------\n"
        "venc -r \n"
        "   brief : force idr\n"
                "---------------------------------------------------------------------\n"
        "venc -c \n"
        "   brief : snap picture \n"
        "\n"
        /******************************************************************/
    );

    return retVal;
}

GADI_ERR gdm_venc_start_encode_all_stream(void)
{
    GADI_ERR     retVal =  GADI_OK;
    GADI_U8      encodeType;
    GADI_U32     cnt;

    /*start all streams encoding*/
    for (cnt=0; cnt < GADI_VENC_STREAM_NUM; cnt++) {
        encodeType = streams[cnt].streamFormat.encodeType;
        if(encodeType != 0) {
            retVal = gadi_venc_start_stream(vencHandle, cnt);
            if(retVal < 0) {
                GADI_ERROR("gadi_venc_start_stream failed!\n");
                return retVal;
            }
            else {
                GADI_INFO("stream[%d] start OK!\n",cnt);
            }
        }
    }

    return retVal;
}

GADI_ERR gdm_venc_stop_encode_all_stream(void)
{
    GADI_ERR     retVal =  GADI_OK;
    GADI_U8      encodeType;
    GADI_U32     cnt;

    for (cnt=0; cnt < GADI_VENC_STREAM_NUM; cnt++) {
        encodeType = streams[cnt].streamFormat.encodeType;
        if(encodeType != 0) {
            retVal = gadi_venc_stop_stream(vencHandle, cnt);
            if(retVal < 0) {
                GADI_ERROR("gadi_venc_stop_stream failed!\n");
                return retVal;
            }
            else {
                    GADI_INFO("stream[%d] stop OK!\n",cnt);
            }
        }
    }

    return retVal;
}

//*****************************************************************************
//*****************************************************************************
//** Local Functions
//*****************************************************************************
//*****************************************************************************
static int bk_fps[GADI_VENC_STREAM_NUM] = {-1, -1, -1, -1};
static void venc_daytonight(GADI_U32 value)
{
    GADI_VENC_FrameRateT stFrameRate;
    GADI_U32 streamFps = 0;
    GADI_U32 cnt;

    for (cnt = 0; cnt < GADI_VENC_STREAM_NUM; cnt++)
    {
        /*only set shutter framerate, donot change the config parameters.*/
        streamFps = streams[cnt].streamFormat.fps;
        if(streamFps > 0)
        {
            bk_fps[cnt] = streamFps;
            if(GADI_VI_FPS_10 > streamFps){
                stFrameRate.streamId = cnt;
                stFrameRate.fps = streamFps;
                gadi_venc_set_framerate(vencHandle, &stFrameRate);
            }
            else{
                stFrameRate.streamId = cnt;
                stFrameRate.fps = GADI_VI_FPS_10;
                gadi_venc_set_framerate(vencHandle, &stFrameRate);
            }
        }
    }
}

static void venc_nighttoday(GADI_U32 value)
{
    GADI_U32 streamFps = 0;
    GADI_VENC_FrameRateT stFrameRate;
    GADI_U32 cnt;

    for (cnt = 0; cnt < GADI_VENC_STREAM_NUM; cnt++)
    {
        /*restore encode encode framerate by config parameters.*/
        streamFps = streams[cnt].streamFormat.fps;
        if(streamFps > 0)
        {
            stFrameRate.streamId = cnt;
            if (bk_fps[cnt] > 0) {
                stFrameRate.fps = bk_fps[cnt];
            } else {
                stFrameRate.fps = streamFps;
            }
            bk_fps[cnt] = -1;
            gadi_venc_set_framerate(vencHandle, &stFrameRate);
        }
    }
}

static GADI_ERR check_encode_streams_params( GADI_U32 viWidth, GADI_U32 viHeight)
{
    GADI_ERR retVal = GADI_OK;
    GADI_U8  cnt;
    GADI_VENC_StreamFormatT *format = NULL;

    for(cnt=0; cnt<GADI_VENC_STREAM_NUM; cnt++)
    {
        format = &(streams[cnt].streamFormat);
        if((format->width > viWidth) ||
           (format->height> viHeight) ||
           (format->width + format->xOffset > viWidth) ||
           (format->height + format->yOffset > viHeight))
        {
            DBG_PRINTF("encode stream[%d] resolution:%dX%d,x offset:%d, y offset:%d \
                is bigger than video input:%dX%d\n",
                cnt, format->width, format->height, format->xOffset, format->yOffset,
                viWidth, viHeight);

            format->xOffset = 0;
            format->yOffset = 0;
            format->width   = viWidth;
            format->height  = viHeight;
        }
    }

    return retVal;
}

static GADI_ERR calculate_encode_size_order(GADI_U32 *order, GADI_U32 streamsNum)
{
    GADI_U32 tmp, resolution[GADI_VENC_STREAM_NUM];
    GADI_VENC_StreamFormatT *format;
    GADI_U32 i, j;

    for (i = 0; i < streamsNum; ++i)
    {
        format = &(streams[i].streamFormat);
        order[i] = i;
        if (format->encodeType == GADI_VENC_TYPE_OFF)
            resolution[i] = 0;
        else
            resolution[i] = (format->width << 16) + format->height;
    }
    for (i = 0; i < streamsNum; ++i)
    {
        for (j = i + 1; j < streamsNum; ++j)
        {
            if (resolution[i] < resolution[j])
            {
                tmp = resolution[j];
                resolution[j] = resolution[i];
                resolution[i] = tmp;
                tmp = order[j];
                order[j] = order[i];
                order[i] = tmp;
            }
        }
    }

    DBG_PRINTF("resolution order of streams : ");
    for (i = 0; i < streamsNum; ++i)
    {
        DBG_PRINTF("%d, ", order[i]);
    }
    DBG_PRINTF("\n");

    return GADI_OK;
}

static GADI_ERR get_channels_params(GADI_VENC_ChannelsParamsT* chansParams)
{
    GADI_ERR retVal = GADI_OK;
    GADI_U32 cnt;
    GADI_U32 chanId;
    GADI_U32 usedFlag[GADI_VENC_STREAM_NUM];
    GADI_U32 order[GADI_VENC_STREAM_NUM];
    GADI_VENC_StreamFormatT *format = NULL;
    GADI_VOUT_SettingParamsT voParams;

    if(chansParams == NULL)
    {
        return -1;
    }

    for (cnt = 0; cnt < GADI_VENC_STREAM_NUM; cnt++)
    {
        usedFlag[cnt] = 0;
    }

    calculate_encode_size_order(order, GADI_VENC_STREAM_NUM);

    retVal = gadi_venc_get_channels_params(vencHandle, chansParams);
    if(retVal != GADI_OK)
    {
        return retVal;
    }

    for (cnt = 0; cnt < GADI_VENC_STREAM_NUM; cnt++)
    {
        format = &(streams[order[cnt]].streamFormat);
        chanId = GADI_VENC_CHANNEL_1;

        if (format->encodeType == GADI_VENC_TYPE_OFF)
        {
            format->channelId = GADI_VENC_CHANNEL_1;
            continue;
        }

        switch (format->height)
        {
            case 1080 ... 1536:
                chansParams->chan1Width  = format->width;
                chansParams->chan1Height = format->height;
                chansParams->chan1Type   = GADI_VENC_CHANNEL_TYPE_ENCODE;
                chanId = GADI_VENC_CHANNEL_1;
                break;

            case 1024:
            case 960:
            case 720:
            case 600:
                if ((usedFlag[GADI_VENC_CHANNEL_1] == 0) ||
                    (chansParams->chan1Height ==  format->height))
                {
                    chansParams->chan1Width  = format->width;
                    chansParams->chan1Height = format->height;
                    chansParams->chan1Type   = GADI_VENC_CHANNEL_TYPE_ENCODE;
                    chanId = GADI_VENC_CHANNEL_1;
                }
                else
                {
                    chansParams->chan3Width  = format->width;
                    chansParams->chan3Height = format->height;
                    chansParams->chan3Type   = GADI_VENC_CHANNEL_TYPE_ENCODE;
                    chanId = GADI_VENC_CHANNEL_3;
                }
                break;

            case 576:
            case 480:
            case 360:
                if ((usedFlag[GADI_VENC_CHANNEL_1] == 0) ||
                    (chansParams->chan1Height ==  format->height))
                {
                    chansParams->chan1Width  = format->width;
                    chansParams->chan1Height = format->height;
                    chansParams->chan1Type   = GADI_VENC_CHANNEL_TYPE_ENCODE;
                    chanId = GADI_VENC_CHANNEL_1;
                }
                else
                {
                    chansParams->chan2Width  = format->width;
                    chansParams->chan2Height = format->height;
                    chansParams->chan2Type   = GADI_VENC_CHANNEL_TYPE_ENCODE;
                    chanId = GADI_VENC_CHANNEL_2;
                }
                break;

            case 288:
            case 240:
            case 144:
            case 120:
                if ((usedFlag[GADI_VENC_CHANNEL_1] == 0) ||
                    (chansParams->chan1Height ==  format->height))
                {
                    chansParams->chan1Width  = format->width;
                    chansParams->chan1Height = format->height;
                    chansParams->chan1Type   = GADI_VENC_CHANNEL_TYPE_ENCODE;
                    chanId = GADI_VENC_CHANNEL_1;
                }
                else if ((usedFlag[GADI_VENC_CHANNEL_2] == 0) ||
                    (chansParams->chan2Height ==  format->height))
                {
                    chansParams->chan2Width  = format->width;
                    chansParams->chan2Height = format->height;
                    chansParams->chan2Type   = GADI_VENC_CHANNEL_TYPE_ENCODE;
                    chanId = GADI_VENC_CHANNEL_2;
                }
                else if((usedFlag[GADI_VENC_CHANNEL_3] == 0) ||
                    (chansParams->chan3Height ==  format->height))
                {
                    chansParams->chan3Width  = format->width;
                    chansParams->chan3Height = format->height;
                    chansParams->chan3Type   = GADI_VENC_CHANNEL_TYPE_ENCODE;
                    chanId = GADI_VENC_CHANNEL_3;
                }
                else
                {
                    chansParams->chan4Width  = format->width;
                    chansParams->chan4Height = format->height;
                    chansParams->chan4Type   = GADI_VENC_CHANNEL_TYPE_ENCODE;
                    chanId = GADI_VENC_CHANNEL_4;
                }

                break;

            default:
                chansParams->chan1Width  = format->width;
                chansParams->chan1Height = format->height;
                chansParams->chan1Type   = GADI_VENC_CHANNEL_TYPE_ENCODE;
                chanId = GADI_VENC_CHANNEL_1;
                GADI_DEBUG("special encode stream resolution:%dx%d\n",format->width, format->height);
                break;

        }

        /*the buffer size != stream resolution???*/
        //format->xOffset??
        //format->yOffset??
        format->channelId = chanId;
        usedFlag[chanId] = 1;
    }

    /*check 2nd channel. if not used, turn it off.*/
    if (usedFlag[GADI_VENC_CHANNEL_2] == 0)
    {
        chansParams->chan2Width  = 0;
        chansParams->chan2Height = 0;
        chansParams->chan2Type   = GADI_VENC_CHANNEL_TYPE_OFF;
    }

    /*if 3rd channel not used, set it as preview.*/
    if (usedFlag[GADI_VENC_CHANNEL_3] == 0)
    {
        #ifdef CHIP_GK7102C
        chansParams->chan3Width  = 0;
        chansParams->chan3Height = 0;
        chansParams->chan3Type   = GADI_VENC_CHANNEL_TYPE_OFF;
        #else
        voParams.voutChannel = GADI_VOUT_B;
        retVal = gadi_vout_get_params(voHandle, &voParams);
        if(retVal != GADI_OK)
        {
            return retVal;
        }
        if ((voParams.deviceType == GADI_VOUT_DEVICE_AUTO) ||
            (voParams.deviceType == GADI_VOUT_DEVICE_DISABLE))
        {
            chansParams->chan3Width  = 0;
            chansParams->chan3Height = 0;
            chansParams->chan3Type   = GADI_VENC_CHANNEL_TYPE_OFF;
        }
        else
        {
            chansParams->chan3Width  = 720;
            chansParams->chan3Height = 480;
            chansParams->chan3Type   = GADI_VENC_CHANNEL_TYPE_PREVIEW;
            usedFlag[GADI_VENC_CHANNEL_3] = 1;
        }
        #endif
    }

    /*if 4th channel not used, set it as preview.*/
    if (usedFlag[GADI_VENC_CHANNEL_4] == 0)
    {
        voParams.voutChannel = GADI_VOUT_A;
        retVal = gadi_vout_get_params(voHandle, &voParams);
        if(retVal != GADI_OK)
        {
            return retVal;
        }

        if ((voParams.deviceType == GADI_VOUT_DEVICE_BT1120) ||
             (voParams.deviceType == GADI_VOUT_DEVICE_RGB) ||
             (voParams.deviceType == GADI_VOUT_DEVICE_I80))
        {
            #ifdef CHIP_GK7102C
            chansParams->chan4Width  = 720;
            chansParams->chan4Height = 576;
            chansParams->chan4Type   = GADI_VENC_CHANNEL_TYPE_PREVIEW;
            usedFlag[GADI_VENC_CHANNEL_4] = 1;
            #else
            chansParams->chan4Width  = 1280;
            chansParams->chan4Height = 720;
            chansParams->chan4Type   = GADI_VENC_CHANNEL_TYPE_PREVIEW;
            usedFlag[GADI_VENC_CHANNEL_4] = 1;
            #endif
        } else {
            chansParams->chan4Width  = 0;
            chansParams->chan4Height = 0;
            chansParams->chan4Type   = GADI_VENC_CHANNEL_TYPE_OFF;
        }
    }

    format = &(streams[0].streamFormat);
    if (format->width == 1920 &&
        format->height == 1080 &&
        (format->fps == GADI_VENC_FPS_60 ||
        format->fps  == GADI_VENC_FPS_59_94))
    {
        chansParams->intlcScan = 1;
    }
    else
    {
        chansParams->intlcScan = 0;
    }

    return retVal;

}
static void venc_usage(void)
{
    printf("\nusage: venc [OPTION]...\n");
    printf("\t-h, --help             help.\n"
           "\t-b, --bitratae         set bitratae(streamId 0~3).\n"
           "\t-f, --framerate        set vi framerate params.(streamId 0~3)\n"
           "\t-g, --gopn             set h264 gopn params.(streamId 0~3)\n"
           "\t-S, --start            start video stream.(streamId 0~3)\n"
           "\t-P, --stop             stop video stream.(streamId 0~3)\n"
           "\t-q, --h264qp           set h264 qp.(streamId 0~3)\n"
           "\t-Q, --h264 quality     set h264 quality.(streamId 0~3)\n"
           "\t-l, --look             look video attributes.(streamId 0~3)\n"
           "\t-F, --flip             set stream flip.(streamId 0~3)\n"
           "\t-I, --idr              force idr.(streamId 0~3)\n"
           "\t-r, --resolution       set resolution.(streamId 0~3)\n"
           "\t-c, --snap             snap picture ["SNAP_PICTRUE_PATH"].(streamId 0~3)\n"
           "\t-B, --bias             set bias params.(streamId 0~3)\n"
           "\t-R, --roi              set roi config.(streamId 0~3)\n"
           "\t-E, --reenc            set h264 reencode config.(streamId 0~3)\n"
           "\t-C, --ictl             set h264 iframe size control config.(streamId 0~3)\n"
           "\t-M, --mbqpctl          set h264 MB QP control config.(streamId 0~3)\n"
          );
	printf("\n");
}


static GADI_ERR handle_venc_command(int argc, char* argv[])
{
    GADI_VENC_StreamFormatT stream_format;
    static GADI_U32 stream_id;
    int option_index;
    int ch;
	int retVal;


	optind = 1;

    /*change parameters when giving input options.*/
    while ((ch = getopt_long(argc, argv, vencShortOptions, vencLongOptions, &option_index)) != -1)
    {
        switch (ch)
        {
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        case 'h':
            venc_usage();
            break;
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        case 'c':
            stream_id = atoi(optarg);
			gdm_venc_capture_jpeg(stream_id, SNAP_PICTRUE_PATH);
			break;
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        case 'f':
        {	printf("??????gdm_venc_set_framerate??????\n");
            GADI_VENC_FrameRateT framerate;

            stream_id = atoi(optarg);
            framerate.streamId = stream_id;
            if(gdm_venc_get_framerate(&framerate)) {
                break;
            }
			printf("1111111111111 fps = %d\n",framerate.fps);
            CREATE_INPUT_MENU(framerate) {
                ADD_SUBMENU(framerate.fps,"fps"),
                CREATE_INPUT_MENU_COMPLETE();

                if (DISPLAY_MENU() != 0) {
                    GADI_ERROR("get parameter failed!\n");
                    break;
                }
            }
			printf("22222222222222 fps = %d\n",framerate.fps);
            gdm_venc_set_framerate(&framerate);

			break;
        }
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		case 'g':
        {
            GADI_VENC_H264GopConfigT h264_gop_config;

            stream_id = atoi(optarg);
            h264_gop_config.streamId = stream_id;
            if(gdm_venc_get_gopn(&h264_gop_config)) {
                GADI_ERROR("gdm_venc_get_gopn failed!\n");
                break;
            }

            CREATE_INPUT_MENU(h264_gop_config) {
                ADD_SUBMENU(h264_gop_config.gopN,"[1-255]"),
                CREATE_INPUT_MENU_COMPLETE();

                if (DISPLAY_MENU() != 0) {
                    GADI_ERROR("get parameter failed!\n");
                    break;
                }
            }

			gdm_venc_set_gopn(&h264_gop_config);

			break;
        }
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        case 'F':
        {
            GADI_U8 flip_rotate;

            stream_id = atoi(optarg);
            if(gdm_venc_get_flip_rotate(stream_id,&flip_rotate)) {
                break;
            }

            CREATE_INPUT_MENU(flip_rotate) {
                ADD_SUBMENU(flip_rotate,"[0-7]bit0:horizontally bit1:vertically bit2:rotate"),
                CREATE_INPUT_MENU_COMPLETE();

                if (DISPLAY_MENU() != 0) {
                    GADI_ERROR("get parameter failed!\n");
                    break;
                }
            }

            gdm_venc_set_flip_rotate(stream_id,flip_rotate);

            break;
        }
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        case 'I':
            stream_id = atoi(optarg);
            gdm_venc_force_idr(stream_id);
			break;
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        case 'S':
            stream_id = atoi(optarg);
            gdm_venc_start_encode_stream(atoi(optarg));
            break;
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		case 'l':
            stream_format.streamId = atoi(optarg);
			retVal = gdm_venc_get_stream_format(&stream_format);
            if (retVal != GADI_OK) {
                break;
            }
	        GADI_INFO("encode stream[%d] resolution:%dX%d,x offset:%d, y offset:%d \n"
                "encode format:%s fps:%d\n", stream_format.channelId, stream_format.width,
                stream_format.height, stream_format.xOffset, stream_format.yOffset,
                stream_format.encodeType == 1?"H264":"MJPEG/NONE", stream_format.fps);
			break;
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        case 'P':
            stream_id = atoi(optarg);
            gdm_venc_stop_encode_stream(stream_id);
            break;
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        case 'q':
        {
            GADI_VENC_H264QpConfigT  h264_qp_config;

            h264_qp_config.streamId = atoi(optarg);
            if(gdm_venc_get_h264_qp(&h264_qp_config)) {
                break;
            }

            CREATE_INPUT_MENU(h264_qp_config) {
                ADD_SUBMENU(h264_qp_config.qpMinOnI,    "[1-51]minimum QP for I Frame"),
                ADD_SUBMENU(h264_qp_config.qpMaxOnI,    "[1-51]maximum QP for I Frame"),
                ADD_SUBMENU(h264_qp_config.qpMinOnP,    "[1-51]minimum QP for P Frame"),
                ADD_SUBMENU(h264_qp_config.qpMaxOnP,    "[1-51]maximum QP for P Frame"),
                ADD_SUBMENU(h264_qp_config.qpIWeight,   "[1-10]QP weight for I Frame "),
                ADD_SUBMENU(h264_qp_config.qpPWeight,   "[1-5]QP weight for P Frame "),
                ADD_SUBMENU(h264_qp_config.adaptQp,     "[0-2]quality consistency"),
                CREATE_INPUT_MENU_COMPLETE();

                if (DISPLAY_MENU() != 0) {
                    GADI_ERROR("get parameter failed!\n");
                    break;
                }
            }
			gdm_venc_set_h264_qp(&h264_qp_config);

            break;
        }
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        case 'Q':
        {
            int h264_quality = 0;
            GADI_VENC_H264QpConfigT  h264_qp_config;

            h264_qp_config.streamId = atoi(optarg);
            if(gdm_venc_get_h264_qp(&h264_qp_config)) {
                break;
            }

            CREATE_INPUT_MENU(h264_quality) {
                ADD_SUBMENU(h264_quality,"[0-3]h264 quality"),
                CREATE_INPUT_MENU_COMPLETE();

                if (DISPLAY_MENU() != 0) {
                    GADI_ERROR("get parameter failed!\n");
                    break;
                }
            }

            if (0 == h264_quality){         //poor
                h264_qp_config.qpMinOnI = 23;
                h264_qp_config.qpMinOnP = 26;
            }else if (1 == h264_quality){
                h264_qp_config.qpMinOnI = 20;
                h264_qp_config.qpMinOnP = 23;
            }else if (2 == h264_quality){
                h264_qp_config.qpMinOnI = 18;
                h264_qp_config.qpMinOnP = 22;
            }else if (3 == h264_quality){   //best
                h264_qp_config.qpMinOnI = 14;
                h264_qp_config.qpMinOnP = 17;
            }

			gdm_venc_set_h264_qp(&h264_qp_config);

            break;
        }
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        case 'T':
                video_test(atoi(optarg));
                break;

        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        case 'r':
        {
            stream_format.streamId = atoi(optarg);
			if(gdm_venc_get_stream_format(&stream_format)) {
                break;
            }

            CREATE_INPUT_MENU(stream_format) {
                ADD_SUBMENU(stream_format.width,"width"),
                ADD_SUBMENU(stream_format.height,"height"),
                CREATE_INPUT_MENU_COMPLETE();

                if (DISPLAY_MENU() != 0) {
                    GADI_ERROR("get parameter failed!\n");
                    break;
                }
            }

            retVal = gdm_venc_set_resolution(stream_id, stream_format.width, stream_format.height);
            if (retVal != GADI_OK) {
                goto bad_parameter;
            }
            isp_restart();
            break;
        }
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        case 'b':
        {
            GADI_VENC_BitRateRangeT bit_rate;

            bit_rate.streamId = atoi(optarg);
            if(gdm_venc_get_bitrate(&bit_rate)) {
                break;
            }

            CREATE_INPUT_MENU(bit_rate) {
                ADD_SUBMENU(bit_rate.brcMode,"[0-3]0:CBR 1:VBR 2:CBR keep quality 3:VBR keep quality/"),
                ADD_SUBMENU(bit_rate.cbrAvgBps,"cbr mode"),
                ADD_SUBMENU(bit_rate.vbrMinbps,"vbr mode"),
                ADD_SUBMENU(bit_rate.vbrMaxbps,"vbr mode"),
                CREATE_INPUT_MENU_COMPLETE();

                if (DISPLAY_MENU() != 0) {
                    GADI_ERROR("get parameter failed!\n");
                    break;
                }
            }

            gdm_venc_set_bitrate(&bit_rate);

            break;
        }
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        case 'B':
        {
            GADI_VENC_BiasConfigT bias_config = {0};
            GADI_VENC_IConfigT i_config = {0};
            GADI_VENC_ZeromvConfigT zmv_config = {0};

            bias_config.streamId = atoi(optarg);
            if(gadi_venc_get_h264_bias(vencHandle, &bias_config)) {
                GADI_ERROR("get bias param error!\n");
                break;
            }

            i_config.streamId = atoi(optarg);
            if(gadi_venc_get_h264_iconfig(vencHandle, &i_config)) {
                GADI_ERROR("get iconfig param error!\n");
                break;
            }

            zmv_config.streamId = atoi(optarg);
            if(gadi_venc_get_h264_zeromv(vencHandle, &zmv_config)) {
                GADI_ERROR("get zmv param error!\n");
                break;
            }

            CREATE_INPUT_MENU(bias_config) {
                ADD_SUBMENU(bias_config.i16x16Cost,     "[0-128]i16x16Cost"),
                ADD_SUBMENU(bias_config.i4x4Cost,       "[0-128]i4x4Cost"),
                ADD_SUBMENU(bias_config.p16x16Cost,     "[0-128]p16x16Cost"),
                ADD_SUBMENU(bias_config.p8x8Cost,       "[0-128]p8x8Cost"),
                ADD_SUBMENU(bias_config.skipChance,     "[0-12]skipChance"),
                ADD_SUBMENU(bias_config.uvQpOffset,     "[0-24]uvQpOffset"),
                ADD_SUBMENU(bias_config.dbfAlpha,       "[0-12]dbfAlpha"),
                ADD_SUBMENU(bias_config.dbfBeta,        "[0-12]dbfBeta"),
                ADD_SUBMENU(i_config.iChance,           "[0-500]iChance"),
                ADD_SUBMENU(zmv_config.zeroMvTh,        "[0-255]zeroMvTh"),
                CREATE_INPUT_MENU_COMPLETE();

                if (DISPLAY_MENU() != 0) {
                    GADI_ERROR("get parameter failed!\n");
                    break;
                }
            }

			gadi_venc_set_h264_bias(vencHandle, &bias_config);
			gadi_venc_set_h264_iconfig(vencHandle, &i_config);
			gadi_venc_set_h264_zeromv(vencHandle, &zmv_config);

            break;
        }
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        case 'R':
        {
            GADI_VENC_RoiConfigT roi_config;
            GADI_U32 roi_index;

            CREATE_INPUT_MENU(roi_index) {
                ADD_SUBMENU(roi_index,"[0-3]Index of ROI"),
                CREATE_INPUT_MENU_COMPLETE();

                if (DISPLAY_MENU() != 0) {
                    GADI_ERROR("get parameter failed!\n");
                    break;
                }
            }

            roi_config.streamId = atoi(optarg);
            if(gadi_venc_get_qp_roi_config(vencHandle, roi_index, &roi_config)) {
                break;
            }

            CREATE_INPUT_MENU(roi_config) {
                ADD_SUBMENU(roi_config.relativeQpI,"[-51-51]relative QP of I frame"),
                ADD_SUBMENU(roi_config.relativeQpP,"[-51-51]relative QP of P frame"),
                ADD_SUBMENU(roi_config.roiEnable,"[0-1]0:disable this ROI,1:enable this ROI."),
                ADD_SUBMENU(roi_config.roiRegionInfo.height,"height of ROI,16-pixel-aligned."),
                ADD_SUBMENU(roi_config.roiRegionInfo.width,"width of ROI,16-pixel-aligned."),
                ADD_SUBMENU(roi_config.roiRegionInfo.offsetX,"x offset of ROI,16-pixel-aligned."),
                ADD_SUBMENU(roi_config.roiRegionInfo.offsetY,"y offset of ROI,16-pixel-aligned."),
                CREATE_INPUT_MENU_COMPLETE();

                if (DISPLAY_MENU() != 0) {
                    GADI_ERROR("get parameter failed!\n");
                    break;
                }
            }

            gadi_venc_set_qp_roi_config(vencHandle, &roi_config);

            break;
        }
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        case 'E':
        {
            GADI_VENC_ReEncConfigT  reEnc_config;

            reEnc_config.streamId = atoi(optarg);
            if(gadi_venc_get_h264_reenc(vencHandle, &reEnc_config)){
                GADI_ERROR("get h264 reenc param error!\n");
                break;
            }

            CREATE_INPUT_MENU(reEnc_config) {
                ADD_SUBMENU(reEnc_config.threshStC,   "[0-7]threshStC"),
                ADD_SUBMENU(reEnc_config.strengthStC, "[0-4]strengthStC"),
                ADD_SUBMENU(reEnc_config.threshCtS,   "[0-7]threshCtS"),
                ADD_SUBMENU(reEnc_config.strengthCtS, "[0-4]strengthCtS"),

                CREATE_INPUT_MENU_COMPLETE();

                if (DISPLAY_MENU() != 0) {
                    GADI_ERROR("get parameter failed!\n");
                    break;
                }
            }

            gadi_venc_set_h264_reenc(vencHandle, &reEnc_config);

            break;
        }
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        case 'C':
        {
            GADI_VENC_IsizeCtlT Isize_ctl;

            Isize_ctl.streamId = atoi(optarg);
            if(gadi_venc_get_h264_iframe_size(vencHandle, &Isize_ctl)){
                GADI_ERROR("get h264 iframe size param error!\n");
                break;
            }

            CREATE_INPUT_MENU(Isize_ctl) {
                ADD_SUBMENU(Isize_ctl.IsizeCtlThresh,   "[0-5]IsizeCtlThresh"),
                ADD_SUBMENU(Isize_ctl.IsizeCtlStrength, "[0-5]IsizeCtlStrength"),

                CREATE_INPUT_MENU_COMPLETE();

                if (DISPLAY_MENU() != 0) {
                    GADI_ERROR("get parameter failed!\n");
                    break;
                }
            }

            gadi_venc_set_h264_iframe_size(vencHandle, &Isize_ctl);

            break;
        }
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        case 'M':
        {
            GADI_VENC_MbQpCtlT mbQp_ctl;
            u8 bitRatelevel = 1;

            mbQp_ctl.streamId = atoi(optarg);
            if(gadi_venc_get_h264_mb_qp_ctl(vencHandle, &mbQp_ctl)){
                GADI_ERROR("get h264 MB QP control param error!\n");
                break;
            }

            CREATE_INPUT_MENU(bitRatelevel) {
                ADD_SUBMENU(bitRatelevel,   "0:low, 1:middle, 2:high, other:4 qp detla"),
                CREATE_INPUT_MENU_COMPLETE();

                if (DISPLAY_MENU() != 0) {
                    GADI_ERROR("get parameter failed!\n");
                    break;
                }
            }

            switch(bitRatelevel){
                case 0:
                    memcpy(mbQp_ctl.mbQpThrds, mbQpThrdsLow, sizeof(u16)*15);
                    memcpy(mbQp_ctl.mbQpDelta, mbQpDeltaLow, sizeof(s8)*16);
                    mbQp_ctl.mbQpCtlEnalbe = 1;
                    break;

                case 2:
                    memcpy(mbQp_ctl.mbQpThrds, mbQpThrdsHigh, sizeof(u16)*15);
                    memcpy(mbQp_ctl.mbQpDelta, mbQpDeltaHigh, sizeof(s8)*16);\
                    mbQp_ctl.mbQpCtlEnalbe = 1;
                    break;

                case 1:
                    memcpy(mbQp_ctl.mbQpThrds, mbQpThrdsMid, sizeof(u16)*15);
                    memcpy(mbQp_ctl.mbQpDelta, mbQpDeltaMid, sizeof(s8)*16);
                    mbQp_ctl.mbQpCtlEnalbe = 1;
                    break;

                default:
                    mbQp_ctl.mbQpCtlEnalbe = 0;
                    break;
            }

            mbQp_ctl.mbQpCtlEnalbe = 1;
            gadi_venc_set_h264_mb_qp_ctl(vencHandle, &mbQp_ctl);

            break;
        }
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        default:
            GADI_ERROR("bad params\n");
			break;
        }
    }
	optind = 1;
    return 0;

bad_parameter:
	optind = 1;
    return -1;
}

static void snapshot_process(void *params)
{
    GADI_VENC_StreamT *stream = (GADI_VENC_StreamT *)params;
    GADI_CHN_AttrT  chn_attr;
    GADI_U32 mjpegStreamId = 0;

    mjpegStreamId = GADI_VENC_STREAM_THIRD;

    if(params == NULL){
        goto snapshot_exit;
    }
    do{
        if (gadi_venc_query(vencHandle, mjpegStreamId, &chn_attr) < 0)
        {
            GADI_INFO("gadi_venc_query chn:%d", mjpegStreamId);
            usleep(1000);
            continue;
        }

        if((chn_attr.state == GADI_VENC_STREAM_STATE_ENCODING) ||
           (chn_attr.state == GADI_VENC_STREAM_STATE_STOPPING))
        {
            if (gadi_venc_get_stream(vencHandle, mjpegStreamId, stream)<0)//BLOCK
            {
                continue;
            }
            /*stream end, stream size & stream addr is invalid.*/
            if(stream->stream_end == 1)
            {
                continue;
            }

            if(stream->pic_type == GK7101_JPEG_STREAM)
            {
                gadi_sys_sem_post(jpegCapSem);
                break;
            }
        }
    }while(!exit_snapshot);

snapshot_exit:
    gadi_sys_thread_self_destroy();
}

static GADI_VOID _snapshot_init(GADI_VOID)
{
    GADI_VENC_StreamFormatT *streamFormat = NULL;
    GADI_VENC_MjpegConfigT  *mjpegConf = NULL;
    GADI_U32 mjpegStreamId = 0;

    if(streams[GADI_VENC_STREAM_THIRD].streamFormat.encodeType == 0) {
        snapshot_inited = GADI_TRUE;
    }
    else
    {
        snapshot_inited = GADI_FALSE;
        return;
    }
    mjpegStreamId = GADI_VENC_STREAM_THIRD;

    streams[mjpegStreamId].streamFormat.width =
        streams[GADI_VENC_STREAM_FIRST].streamFormat.width;
    streams[mjpegStreamId].streamFormat.height =
        streams[GADI_VENC_STREAM_FIRST].streamFormat.height;
    streams[mjpegStreamId].streamFormat.xOffset =
        streams[GADI_VENC_STREAM_FIRST].streamFormat.xOffset;
    streams[mjpegStreamId].streamFormat.yOffset =
        streams[GADI_VENC_STREAM_FIRST].streamFormat.yOffset;
    streams[mjpegStreamId].streamFormat.channelId =
        streams[GADI_VENC_STREAM_FIRST].streamFormat.channelId;

    streamFormat = &streams[mjpegStreamId].streamFormat;
    streamFormat->streamId   = mjpegStreamId;
    streamFormat->encodeType = 2;//mjpeg
    streamFormat->flipRotate = 0;
    streamFormat->keepAspRat = 0;
    streamFormat->fps        = GK7101_VENC_MJPEG_FPS;/*set 3 fps default.*/
    mjpegConf = &streams[mjpegStreamId].mjpegConf;
    mjpegConf->quality = 50;
    mjpegConf->chromaFormat = 1;//YUV420
    mjpegConf->streamId = mjpegStreamId;
}

static GADI_ERR _snapshot_set_source_streamid(GADI_U32 streamid,
    GADI_U8 quality, GADI_U16 width, GADI_U16 height)
{
    GADI_ERR retVal = GADI_OK;
    GADI_VENC_StreamFormatT *streamFormat = NULL;
    GADI_VENC_MjpegConfigT  *mjpegConf = NULL;
    GADI_U32 mjpegStreamId = 0;

    mjpegStreamId = GADI_VENC_STREAM_THIRD;

    if(streamid < 0 || streamid >= GADI_VENC_STREAM_NUM) {
        GADI_ERROR("snapshot set mjpeg config failed.\n");
        return -1;
    }

    if(width >= streams[streamid].streamFormat.width){
        streams[mjpegStreamId].streamFormat.width =
            streams[streamid].streamFormat.width;
        streams[mjpegStreamId].streamFormat.xOffset =
            streams[streamid].streamFormat.xOffset;
    } else {
        streams[mjpegStreamId].streamFormat.width = width;
        streams[mjpegStreamId].streamFormat.xOffset =
            streams[streamid].streamFormat.xOffset
            + (streams[streamid].streamFormat.width - width)/2;
    }

    if(height >= streams[streamid].streamFormat.height){
        streams[mjpegStreamId].streamFormat.height =
            streams[streamid].streamFormat.height;
        streams[mjpegStreamId].streamFormat.yOffset =
            streams[streamid].streamFormat.yOffset;
    } else {
        streams[mjpegStreamId].streamFormat.height = height;
        streams[mjpegStreamId].streamFormat.yOffset =
            streams[streamid].streamFormat.yOffset
            + (streams[streamid].streamFormat.height - height)/2;
    }

    streams[mjpegStreamId].streamFormat.channelId =
        streams[streamid].streamFormat.channelId;

    streamFormat = &streams[mjpegStreamId].streamFormat;
    streamFormat->streamId   = mjpegStreamId;
    streamFormat->encodeType = 2;//mjpeg
    streamFormat->flipRotate = streams[streamid].streamFormat.flipRotate;
    streamFormat->keepAspRat = streams[streamid].streamFormat.keepAspRat;
    streamFormat->fps        = 3;/*set 3 fps default.*/

    retVal = gadi_venc_set_stream_format(vencHandle, streamFormat);
    if (retVal != GADI_OK)
    {
        GADI_ERROR("snapshot set mjpeg stream format failed.\n");
        return -1;
    }

    mjpegConf = &streams[mjpegStreamId].mjpegConf;
    mjpegConf->quality = quality;
    mjpegConf->chromaFormat = 1;//YUV420
    mjpegConf->streamId = mjpegStreamId;

    retVal = gadi_venc_set_mjpeg_config(vencHandle, mjpegConf);
    if (retVal != GADI_OK)
    {
        GADI_ERROR("snapshot set mjpeg config failed.\n");
        return -1;
    }

#if 0
    printf("enc snapshot: streamId = %d, quality:%d%%, width:%d px, height:%d px\n",
              streamid, quality,
              streams[GADI_VENC_STREAM_FORTH].streamFormat.width - streams[GADI_VENC_STREAM_FORTH].streamFormat.xOffset,
              streams[GADI_VENC_STREAM_FORTH].streamFormat.height - streams[GADI_VENC_STREAM_FORTH].streamFormat.yOffset);
#endif
    return retVal;
}

static GADI_ERR _snapshot_cancel_source_streamid(GADI_VOID)
{
    GADI_ERR retVal = GADI_OK;
    GADI_VENC_StreamFormatT *streamFormat = NULL;
    GADI_U32 mjpegStreamId = 0;

    if (!snapshot_inited)
    {
        return retVal;
    }

    mjpegStreamId = GADI_VENC_STREAM_THIRD;

    streamFormat = &streams[mjpegStreamId].streamFormat;
    streamFormat->encodeType = 0;//mjpeg

    return retVal;
}

static int _snapshot_to_file(GADI_S32 streamId, GAPP_ENC_SNAPSHOT_QualityEnumT quality,
                                    GAPP_ENC_SNAPSHOT_ImageSizeEnumT width, GAPP_ENC_SNAPSHOT_ImageSizeEnumT height, FILE* filename)
{
    int retVal;
    signed int jpegQuality = 0;
    unsigned int capWidth, capHeight;
    GADI_VENC_StreamT stream;
    GADI_U32 mjpegStreamId = 0;

    mjpegStreamId = GADI_VENC_STREAM_THIRD;

    /*check input parameters.*/
    if(filename == NULL)
    {
        GADI_ERROR("snapshot: input FILE * is NULL.\n");
    }

    if(!snapshot_inited) {
        GADI_ERROR("snapshot: four stream don't enable.\n");
        return -1;
    }

    pthread_mutex_lock(&snapshotMutex);

    if(quality == ENC_SNAPSHOT_QUALITY_HIGHEST)
    {
        jpegQuality = 99;
    }
    else if(quality == ENC_SNAPSHOT_QUALITY_HIGH)
    {
        jpegQuality = 90;
    }
    else if(quality == ENC_SNAPSHOT_QUALITY_MEDIUM)
    {
        jpegQuality = 80;
    }
    else if(quality == ENC_SNAPSHOT_QUALITY_LOW)
    {
        jpegQuality = 50;
    }
    else
    {
        jpegQuality = 20;
    }

    if((width == ENC_SNAPSHOT_SIZE_AUTO) ||
       (height == ENC_SNAPSHOT_SIZE_AUTO))
    {
        GADI_VENC_StreamFormatT formatPar;
        formatPar.streamId = streamId;
        retVal = gadi_venc_get_stream_format(vencHandle, &formatPar);
        if(retVal != GADI_OK){
            capWidth  = streams[GADI_VENC_STREAM_FIRST].streamFormat.width;
            capHeight = streams[GADI_VENC_STREAM_FIRST].streamFormat.height;
        } else {
            capWidth  = formatPar.width;
            capHeight = formatPar.height;
        }
    }
    else
    {
        if(width == ENC_SNAPSHOT_SIZE_MAX)
        {
            capWidth = streams[GADI_VENC_STREAM_FIRST].streamFormat.width;
        }
        else if(width == ENC_SNAPSHOT_SIZE_MIN)
        {
            capWidth = GK7101_VENC_JPEG_MIN_WIDTH;
        }
        else
        {
            capWidth = (width > GK7101_VENC_JPEG_MIN_WIDTH) ? width : GK7101_VENC_JPEG_MIN_WIDTH;
        }

        if(height == ENC_SNAPSHOT_SIZE_MAX)
        {
            capHeight = streams[GADI_VENC_STREAM_FIRST].streamFormat.height;
        }

        else if(height == ENC_SNAPSHOT_SIZE_MIN)
        {
            capHeight = GK7101_VENC_JPEG_MIN_HEIGHT;
        }
        else
        {
            capHeight = (width > GK7101_VENC_JPEG_MIN_WIDTH) ? width : GK7101_VENC_JPEG_MIN_WIDTH;
        }
    }

    capWidth  = ((capWidth + 3)>> 2) << 2;
    capHeight = ((capHeight + 3)>> 2) << 2;

    retVal = _snapshot_set_source_streamid(streamId, jpegQuality, capWidth, capHeight);
    if(retVal < 0)
    {
        GADI_ERROR("snapshot _snapshot_set_source_streamid error.\n");
        goto snap_exit;
    }

    /*start jpeg encoder.*/
    retVal = gadi_venc_start_stream(vencHandle, mjpegStreamId);
    if(retVal < 0)
    {
        GADI_ERROR("snapshot start MJPEG stream.\n");
        goto snap_exit;
    }

    /*start to wait jpeg data.*/
    GADI_SYS_ThreadHandleT readjpeg;
    exit_snapshot = GADI_FALSE;
    if(gadi_sys_thread_create(snapshot_process, &stream, GADI_SYS_THREAD_PRIO_DEFAULT,
        GADI_SYS_THREAD_STATCK_SIZE_DEFAULT, "snapshot", &readjpeg) != 0)
    {
        GADI_ERROR("snapshot stop MJPEG stream.\n");
        retVal = -1;
        goto snap_second_exit;
    }
    retVal = gadi_sys_sem_wait_timeout(jpegCapSem, 3000);
    if(retVal == GADI_SYS_ERR_TIMEOUT)
    {
        exit_snapshot = GADI_TRUE;
        GADI_ERROR("snapshot wait JPEG data timeout.\n");
        retVal = -1;
        goto snap_second_exit;
    } else if(stream.pic_type == GK7101_JPEG_STREAM) {
        fwrite(stream.addr, 1, stream.size, filename);
        fflush(filename);
        GADI_INFO("write file addr:%p , length:%d\n", stream.addr,stream.size);
    }
    /*capture end, stop jpeg encoder.*/

snap_second_exit:
    if(gadi_venc_stop_stream(vencHandle, mjpegStreamId) < 0)
    {
        GADI_ERROR("snapshot stop MJPEG stream.\n");
    }
snap_exit:
    _snapshot_cancel_source_streamid();
    pthread_mutex_unlock(&snapshotMutex);

    return retVal;
}

