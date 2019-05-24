/*******************************************************************************
  copyright   : Copyright (C) 2016, www.peergine.com, All rights reserved.
  filename    : pgLibDevAudioConvert.h
  discription : 
  modify      : Create, chenbichao, 2016/06/24

*******************************************************************************/
#ifndef _PG_LIB_DEV_AUDIO_CVT_H
#define _PG_LIB_DEV_AUDIO_CVT_H

#ifdef _PG_DLL_EXPORT
#define PG_DLL_API __declspec(dllexport)
#else
#define PG_DLL_API
#endif


#ifdef	__cplusplus
extern	"C"	{
#endif


///
// Audio convert format enum.
typedef enum tagPG_DEV_AUDIO_CVT_FMT_E {
	PG_DEV_AUDIO_CVT_FMT_PCM16,
	PG_DEV_AUDIO_CVT_FMT_G711A,
	PG_DEV_AUDIO_CVT_FMT_G711U,
	PG_DEV_AUDIO_CVT_FMT_AAC,
} PG_DEV_AUDIO_CVT_FMT_E;


///
// Alloc a audio convert queue.
// uDirect: [in] 0: input, 1: output.
// uDstFormat: [in] The destinstion format.
// uDevSampleRate: [in] The device side sample rate.
//                      Validable value: 8000, 16000, 32000, 22050, 44100.
// uDevSampleSize: [in] The device side sample number of one packet.
// Return value:
//      <0: failed, >0: return the file descriptor id.
PG_DLL_API
int pgDevAudioConvertAlloc(unsigned int uDirect, unsigned int uDstFormat,
	unsigned int uDevSampleRate, unsigned int uDevSampleSize);

///
// Free a audio convert queue.
// iCvtID: [in] The convert file descriptor id.
PG_DLL_API
void pgDevAudioConvertFree(int iCvtID);

///
// Push data into the audio convert queue.
// iCvtID: [in] The convert file descriptor id.
// uSrcFormat: [in] The source format.
// lpSrcData: [in] The source data buffer
// uSrcDataSize: [in] The source data size.
// Return value:
//     <0: failed, >=0: size of data in buffer.
PG_DLL_API
int pgDevAudioConvertPush(int iCvtID, unsigned int uSrcFormat,
	const void* lpSrcData, unsigned int uSrcDataSize);

///
// Pop data out from the audio convert queue.
// (It is not safe, it simply pop out the pointer and size of data.)
// iCvtID: [in] The convert file descriptor id.
// ppDstData: [out] The pointer of the destinstion data.
// lpuDstDataSize: [out] The size of the destinstion data.
// Return value:
//     <0: failed, ==0: packet data has not ready, >0: the size of output valid packet data.
PG_DLL_API
int pgDevAudioConvertPop(int iCvtID, void **ppDstData, unsigned int* lpuDstDataSize);

///
// Pop data out from the audio convert queue.
// (It is safe, it copy the data out from queue to a buffer.)
// iCvtID: [in] The convert file descriptor id.
// lpDstData: [out] The buffer to receivece the output data.
// lpuDstDataSize: [int] The size of the buffer.
// Return value:
//     <0: failed, ==0: packet data has not ready, >0: the size of output valid packet data.
PG_DLL_API
int pgDevAudioConvertPopS(int iCvtID, void *lpDstData, unsigned int uDstDataSize);


#ifdef __cplusplus
}
#endif

#endif //_PG_LIB_DEV_AUDIO_CVT_H
