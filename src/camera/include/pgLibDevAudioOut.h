/*************************************************************************
  copyright   : Copyright (C) 2015, chenbichao, 
              : www.pptun.com, www.peergine.com, All rights reserved.
  filename    : pgLibDevAudioOut.h
  discription : 
  modify      : Create, chenbichao, 2015/02/19

*************************************************************************/
#ifndef _PG_LIB_DEV_AUDIO_OUT_H
#define _PG_LIB_DEV_AUDIO_OUT_H

#ifdef _PG_DLL_EXPORT
#define PG_DLL_API __declspec(dllexport)
#else
#define PG_DLL_API
#endif


#ifdef	__cplusplus
extern	"C"	{
#endif


///
// Audio output format enum.
typedef enum tagPG_DEV_AUDIO_OUT_FMT_E {
	PG_DEV_AUDIO_OUT_FMT_PCM16,
	PG_DEV_AUDIO_OUT_FMT_G711A,
	PG_DEV_AUDIO_OUT_FMT_AAC,
	PG_DEV_AUDIO_OUT_FMT_BUTT
} PG_DEV_AUDIO_OUT_FMT_E;


///
// Audio out api callback functions.
typedef struct tagPG_DEV_AUDIO_OUT_CALLBACK_S {

	///
	// uDevNO: The speaker's number.
	// Return value:
	// >= 0: The audio output device's file descriptor id.
	// < 0: failed.
	int (*pfnAudioOutOpen)(unsigned int uDevNO, unsigned int uSampleBits,
		unsigned int uSampleRate, unsigned int uChannels, unsigned int uPackBytes);

	void (*pfnAudioOutClose)(int iDevID);

	///
	// uFormat: [in] Audio format, see the enum 'PG_DEV_AUDIO_OUT_FMT_E'.
	int (*pfnAudioOutPlay)(int iDevID, const void* lpData,
		unsigned int uDataSize, unsigned int uFormat);

} PG_DEV_AUDIO_OUT_CALLBACK_S;


///
// Set the callback api functions
PG_DLL_API
void pgDevAudioOutSetCallback(const PG_DEV_AUDIO_OUT_CALLBACK_S* lpstCallback);


///
// Device notify the audio data size(bytes) has played.
// uDelayMs: [in] Play audio delay(ms)
PG_DLL_API
void pgDevAudioOutPlayedProc(int iDevID, unsigned int uPlayedSize, unsigned int uDelayMs);


///
// Enable to play the silent data.
// uDevNO: [in] The speaker NO to enable
// uEnable: [in] Enable play silent. 1: enable, 0: disable.
// return: 1: success, 0: failed.
PG_DLL_API
unsigned int pgDevAudioOutPlaySilent(unsigned int uDevNO, unsigned int uEnable);


#ifdef __cplusplus
}
#endif

#endif //_PG_LIB_DEV_AUDIO_OUT_H
