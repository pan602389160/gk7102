/*************************************************************************
  copyright   : Copyright (C) 2014-2017, www.peergine.com, All rights reserved.
              :
  filename    : pgLiveMultiCapture.h
  discription : 
  modify      : create, chenbichao, 2017/05/10

*************************************************************************/
#ifndef _PG_LIB_LIVE_MULTI_CAPTURE_H
#define _PG_LIB_LIVE_MULTI_CAPTURE_H


#ifdef _PG_DLL_EXPORT
#define PG_DLL_API __declspec(dllexport)
#else
#define PG_DLL_API
#endif


#ifdef __cplusplus
extern "C" {
#endif


typedef void (*TpfnPGLiveMultiCaptureLogOutput)(unsigned int uLevel, const char* sOut);


typedef void (*TpfnPGLiveMultiCaptureEventProc)(unsigned int uInstID,
	const char* sAction, const char* sData, const char* sRenID);


///
// Node Event hook
typedef int (*TpfnPGLiveMultiCaptureEventHookOnExtRequest)(unsigned int uInstID,
	const char* sObj, int uMeth, const char* sData, unsigned int uHandle, const char* sPeer);

typedef int (*TpfnPGLiveMultiCaptureEventHookOnReply)(unsigned int uInstID,
	const char* sObj, int uErr, const char* sData, const char* sParam);


PG_DLL_API
void pgLiveMultiCaptureSetCallback(TpfnPGLiveMultiCaptureLogOutput pfnLogOutput,
	TpfnPGLiveMultiCaptureEventProc pfnEventProc);


PG_DLL_API
void pgLiveMultiCaptureSetEventHook(TpfnPGLiveMultiCaptureEventHookOnExtRequest pfnOnExtRequest,
	TpfnPGLiveMultiCaptureEventHookOnReply pfnOnReply);


PG_DLL_API
int pgLiveMultiCaptureInitialize(unsigned int* lpuInstID,
	const char* sUser, const char* sPass, const char* sSvrAddr,
	const char* sRelayAddr, int iP2PTryTime, const char* sInitParam);


PG_DLL_API
void pgLiveMultiCaptureCleanup(unsigned int uInstID);


PG_DLL_API
int pgLiveMultiCaptureGetSelfPeer(unsigned int uInstID, char* sPeer, unsigned int uSize);


PG_DLL_API
int pgLiveMultiCaptureRenderReject(unsigned int uInstID, const char* sRenID);


PG_DLL_API
int pgLiveMultiCaptureRenderAccess(unsigned int uInstID,
	const char* sRenID, unsigned int bVideo, unsigned int bAudio);


PG_DLL_API
int pgLiveMultiCaptureRenderEnum(unsigned int uInstID, int iIndex, char* sRenID, unsigned int uSize);


PG_DLL_API
int pgLiveMultiCaptureRenderConnected(unsigned int uInstID, const char* sRenID);


PG_DLL_API
int pgLiveMultiCaptureNotifySend(unsigned int uInstID, const char* sData);


PG_DLL_API
int pgLiveMultiCaptureMessageSend(unsigned int uInstID,
	const char* sRenID, const char* sData);


PG_DLL_API
int pgLiveMultiCaptureVideoModeSize(unsigned int uInstID,
	int iMode, int iWidth, int iHeight);


PG_DLL_API
int pgLiveMultiCaptureVideoStart(unsigned int uInstID,
	int iVideoID, const char* sParam, unsigned int uViewID);


PG_DLL_API
void pgLiveMultiCaptureVideoStop(unsigned int uInstID, int iVideoID);


PG_DLL_API
int pgLiveMultiCaptureVideoCamera(unsigned int uInstID, int iVideoID, const char* sJpgPath);


PG_DLL_API
int pgLiveMultiCaptureVideoParam(unsigned int uInstID, int iVideoID, const char* sParam);


PG_DLL_API
int pgLiveMultiCaptureVideoForwardAlloc(unsigned int uInstID, int iVideoID, const char* sParam);


PG_DLL_API
int pgLiveMultiCaptureVideoForwardFree(unsigned int uInstID, int iVideoID, const char* sParam);


PG_DLL_API
int pgLiveMultiCaptureAudioStart(unsigned int uInstID, int iAudioID, const char* sParam);


PG_DLL_API
void pgLiveMultiCaptureAudioStop(unsigned int uInstID, int iAudioID);


PG_DLL_API
int pgLiveMultiCaptureAudioSpeech(unsigned int uInstID,
	int iAudioID, const char* sRenID, unsigned int bEnable);


PG_DLL_API
int pgLiveMultiCaptureAudioParam(unsigned int uInstID, int iAudioID, const char* sParam);


PG_DLL_API
int pgLiveMultiCaptureAudioMute(unsigned int uInstID,
	int iAudioID, unsigned int bInput, unsigned int bOutput);


PG_DLL_API
int pgLiveMultiCaptureRecordStart(unsigned int uInstID,
	const char* sTag, const char* sAviPath, int iVideoID, int iAudioID);


PG_DLL_API
void pgLiveMultiCaptureRecordStop(unsigned int uInstID, const char* sTag);


PG_DLL_API
int pgLiveMultiCaptureFilePutRequest(unsigned int uInstID,
	const char* sRenID, const char* sPath, const char* sPeerPath);


PG_DLL_API
int pgLiveMultiCaptureFileGetRequest(unsigned int uInstID,
	const char* sRenID, const char* sPath, const char* sPeerPath);


PG_DLL_API
int pgLiveMultiCaptureFileAccept(unsigned int uInstID,
	const char* sRenID, const char* sPath);


PG_DLL_API
int pgLiveMultiCaptureFileReject(unsigned int uInstID,
	const char* sRenID, int iErrCode);


PG_DLL_API
int pgLiveMultiCaptureFileCancel(unsigned int uInstID, const char* sRenID);


PG_DLL_API
int pgLiveMultiCaptureSvrRequest(unsigned int uInstID,
	const char* sData, const char* sParam);


PG_DLL_API
void pgLiveMultiCaptureVersion(char* lpszVersion, unsigned int uSize);


#ifdef __cplusplus
}
#endif

#endif
