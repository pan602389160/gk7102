/*************************************************************************
  copyright   : Copyright (C) 2015, chenbichao, 
              : www.pptun.com, www.peergine.com, All rights reserved.
  filename    : pgLibDevAudioIn.h
  discription : 
  modify      : Create, chenbichao, 2015/02/19

*************************************************************************/
#ifndef _PG_LIB_DEV_AUDIO_IN_H
#define _PG_LIB_DEV_AUDIO_IN_H

#ifdef _PG_DLL_EXPORT
#define PG_DLL_API __declspec(dllexport)
#else
#define PG_DLL_API
#endif


#ifdef	__cplusplus
extern	"C"	{
#endif


///
// Audio capture format enum.
typedef enum tagPG_DEV_AUDIO_IN_FMT_E {
	PG_DEV_AUDIO_IN_FMT_PCM16,
	PG_DEV_AUDIO_IN_FMT_G711A,
	PG_DEV_AUDIO_IN_FMT_AAC,
	PG_DEV_AUDIO_IN_FMT_BUTT
} PG_DEV_AUDIO_IN_FMT_E;


///
// Audio input api callback functions.
typedef struct tagPG_DEV_AUDIO_IN_CALLBACK_S {
	
	///
	// uDevNO: The microphone's number.
	// Return value:
	// >= 0: The audio input device's file descriptor id.
	// < 0: failed.
	int (*pfnAudioInOpen)(unsigned int uDevNO, unsigned int uSampleBits,
		unsigned int uSampleRate, unsigned int uChannels, unsigned int uPackBytes);

	void (*pfnAudioInClose)(int iDevID);

} PG_DEV_AUDIO_IN_CALLBACK_S;


///
// Set the callback api functions
PG_DLL_API
void pgDevAudioInSetCallback(const PG_DEV_AUDIO_IN_CALLBACK_S* lpstCallback);


///
// Device write the audio record data to peergine.
// uFormat: [in] Audio format, see the enum 'PG_DEV_AUDIO_IN_FMT_E'.
// uDelayMs: [in] Recored audio delay(ms)
PG_DLL_API
void pgDevAudioInRecordProc(int iDevID, const void* lpData,
	unsigned int uDataSize, unsigned int uFormat, unsigned int uDelayMs);


#ifdef __cplusplus
}
#endif

#endif //_PG_LIB_DEV_AUDIO_IN_H
