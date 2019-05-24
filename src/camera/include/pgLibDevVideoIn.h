/*************************************************************************
  copyright   : Copyright (C) 2015, chenbichao, 
              : www.pptun.com, www.peergine.com, All rights reserved.
  filename    : pgLibDevVideoIn.h
  discription : 
  modify      : Create, chenbichao, 2015/02/19

*************************************************************************/
#ifndef _PG_LIB_DEV_VIDEO_IN_H
#define _PG_LIB_DEV_VIDEO_IN_H

#ifdef _PG_DLL_EXPORT
#define PG_DLL_API __declspec(dllexport)
#else
#define PG_DLL_API
#endif


#ifdef	__cplusplus
extern	"C"	{
#endif

///
// Video capture format enum.
typedef enum tagPG_DEV_VIDEO_IN_FMT_E {
	PG_DEV_VIDEO_IN_FMT_RGB24,
	PG_DEV_VIDEO_IN_FMT_YUV422SP,
	PG_DEV_VIDEO_IN_FMT_YUV420SP,
	PG_DEV_VIDEO_IN_FMT_YUYV,
	PG_DEV_VIDEO_IN_FMT_YV12,
	PG_DEV_VIDEO_IN_FMT_MJPEG,
	PG_DEV_VIDEO_IN_FMT_VP8,
	PG_DEV_VIDEO_IN_FMT_H264,
	PG_DEV_VIDEO_IN_FMT_H265,
	PG_DEV_VIDEO_IN_FMT_NV21,
	PG_DEV_VIDEO_IN_FMT_BUTT
} PG_DEV_VIDEO_IN_FMT_E;

///
// Video input flag
typedef enum tagPG_DEV_VIDEO_IN_FLAG_E {
	PG_DEV_VIDEO_IN_FLAG_KEY_FRAME = 0x0001,
} PG_DEV_VIDEO_IN_FLAG_E;

///
// Video capture control command.
typedef enum tagPG_DEV_VIDEO_IN_CTRL_E {
	PG_DEV_VIDEO_IN_CTRL_PULL_KEY_FRAME,
	PG_DEV_VIDEO_IN_CTRL_BUTT
} PG_DEV_VIDEO_IN_CTRL_E;


///
// Video input api callback functions.
typedef struct tagPG_DEV_VIDEO_IN_CALLBACK_S {
	
	///
	// Return value:
	// >= 0: the video capture device id.
	// < 0: failed.
	int (*pfnVideoInOpen)(unsigned int uDevNO, unsigned int uPixBytes,
		unsigned int uWidth, unsigned int uHeight, unsigned int uBitRate,
		unsigned int uFrmRate, unsigned int uKeyFrmRate);

	void (*pfnVideoInClose)(int iDevID);

	///
	// Control the video input.
	// uCtrl: [in] See the enum 'PG_DEV_VIDEO_IN_CTRL_E'
	// uParam: [in] Reserved.
	void (*pfnVideoInCtrl)(int iDevID, unsigned int uCtrl, unsigned int uParam);

} PG_DEV_VIDEO_IN_CALLBACK_S;


///
// Set api callback functions
PG_DLL_API
void pgDevVideoInSetCallback(const PG_DEV_VIDEO_IN_CALLBACK_S* lpstCallback);

///
// Device write the video capture data to peergine.
// iDevID: [in] Device id return by 'pfnVideoInOpen'.
// lpData: [in] Video frame data.
// uDataSize: [in] Video frame data size(byte).
// uFormat: [in] Video frame format, see the enum 'PG_DEV_VIDEO_IN_FMT_E'.
// uFlag: [in] Flag. see the enum 'tagPG_DEV_VIDEO_IN_FLAG_E'.
PG_DLL_API
void pgDevVideoInCaptureProc(int iDevID, const void* lpData,
	unsigned int uDataSize, unsigned int uFormat, unsigned int uFlag);


#ifdef __cplusplus
}
#endif

#endif //_PG_LIB_DEV_VIDEO_IN_H
