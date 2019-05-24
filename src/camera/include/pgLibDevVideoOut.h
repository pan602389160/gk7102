/*************************************************************************
  copyright   : Copyright (C) 2015, chenbichao, 
              : www.pptun.com, www.peergine.com, All rights reserved.
  filename    : pgLibDevVideoOut.h
  discription : 
  modify      : Create, chenbichao, 2015/02/19

*************************************************************************/
#ifndef _PG_LIB_DEV_VIDEO_OUT_H
#define _PG_LIB_DEV_VIDEO_OUT_H

#ifdef _PG_DLL_EXPORT
#define PG_DLL_API __declspec(dllexport)
#else
#define PG_DLL_API
#endif


#ifdef	__cplusplus
extern	"C"	{
#endif


///
// Video output format enum.
typedef enum tagPG_DEV_VIDEO_OUT_FMT_E {
	PG_DEV_VIDEO_OUT_FMT_RGB24,
	PG_DEV_VIDEO_OUT_FMT_MJPEG,
	PG_DEV_VIDEO_OUT_FMT_VP8,
	PG_DEV_VIDEO_OUT_FMT_H264,
	PG_DEV_VIDEO_OUT_FMT_H265,
	PG_DEV_VIDEO_OUT_FMT_BUTT
} PG_DEV_VIDEO_OUT_FMT_E;

///
// Video output flag
typedef enum tagPG_DEV_VIDEO_OUT_FLAG_E {
	PG_DEV_VIDEO_OUT_FLAG_KEY_FRAME = 0x0001,
} PG_DEV_VIDEO_OUT_FLAG_E;

///
// Video output event
typedef enum tagPG_DEV_VIDEO_OUT_EVENT_E {
	PG_DEV_VIDEO_OUT_EVENT_PAINT,
} PG_DEV_VIDEO_OUT_EVENT_E;

///
// Video fill mode
typedef enum tagPG_DEV_VIDEO_OUT_FILL_MODE_E {
	PG_DEV_VIDEO_OUT_FILL_MODE_DST_IN_SRC,
	PG_DEV_VIDEO_OUT_FILL_MODE_SRC_IN_DST,
	PG_DEV_VIDEO_OUT_FILL_MODE_SRC_FIT_DST,
} PG_DEV_VIDEO_OUT_FILL_MODE_E;

///
// Video rotate.
typedef enum tagPG_DEV_VIDEO_OUT_ROTATE_E {
	PG_DEV_VIDEO_OUT_ROTATE_0,
	PG_DEV_VIDEO_OUT_ROTATE_90,
	PG_DEV_VIDEO_OUT_ROTATE_180,
	PG_DEV_VIDEO_OUT_ROTATE_270,
} PG_DEV_VIDEO_OUT_ROTATE_E;


///
// Video output api callback functions.
typedef struct tagPG_DEV_VIDEO_OUT_CALLBACK_S {
	
	///
	// Return value:
	// >= 0: the video output device id.
	// < 0: failed.
	int (*pfnVideoOutOpen)(unsigned int uDevNO);

	///
	// iDevID: the return value of pfnVideoOutOpen
	void (*pfnVideoOutClose)(int iDevID);

	///
	// iDevID: the return value of pfnVideoOutOpen
	void (*pfnVideoOutImage)(int iDevID, const void* lpData,
		unsigned int uDataSize, unsigned int uFormat, unsigned int uFlag,
		int iPosX, int iPosY, unsigned int uWidth, unsigned int uHeight,
		unsigned int uFillMode, unsigned int uRotate);

	///
	// iDevID: the return value of pfnVideoOutOpen
	void (*pfnVideoOutClean)(int iDevID);

} PG_DEV_VIDEO_OUT_CALLBACK_S;


///
// Set callback api functions
PG_DLL_API
void pgDevVideoOutSetCallback(const PG_DEV_VIDEO_OUT_CALLBACK_S* lpstCallback);

/// 
// Process the video output event.
// iDevID: the return value of pfnVideoOutOpen
// uEvent: enum of 'PG_DEV_VIDEO_OUT_EVENT_E'
PG_DLL_API
void pgDevVideoOutEventProc(int iDevID, unsigned int uEvent, const void* lpParam);


#ifdef __cplusplus
}
#endif

#endif //_PG_LIB_DEV_VIDEO_H
