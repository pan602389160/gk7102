/*************************************************************************
  copyright   : Copyright (C) 2014-2017, www.peergine.com, All rights reserved.
              :
  filename    : pgLiveMultiRender.h
  discription : 
  modify      : create, chenbichao, 2017/05/10

*************************************************************************/
#ifndef _PG_LIB_LIVE_MULTI_RENDER_H
#define _PG_LIB_LIVE_MULTI_RENDER_H


#ifdef _PG_DLL_EXPORT
#define PG_DLL_API __declspec(dllexport)
#else
#define PG_DLL_API
#endif


#ifdef __cplusplus
extern "C" {
#endif


typedef void (*TpfnPGLiveMultiRenderLogOutput)(unsigned int uLevel, const char* sOut);


typedef void (*TpfnPGLiveMultiRenderEventProc)(unsigned int uInstID,
	const char* sAction, const char* sData, const char* sRenID);


///
// Node Event hook
typedef int (*TpfnPGLiveMultiRenderEventHookOnExtRequest)(unsigned int uInstID,
	const char* sObj, int uMeth, const char* sData, unsigned int uHandle, const char* sPeer);

typedef int (*TpfnPGLiveMultiRenderEventHookOnReply)(unsigned int uInstID,
	const char* sObj, int uErr, const char* sData, const char* sParam);


PG_DLL_API
void pgLiveMultiRenderSetCallback(TpfnPGLiveMultiRenderLogOutput pfnLogOutput,
	TpfnPGLiveMultiRenderEventProc pfnEventProc);


PG_DLL_API
void pgLiveMultiRenderSetEventHook(TpfnPGLiveMultiRenderEventHookOnExtRequest pfnOnExtRequest,
	TpfnPGLiveMultiRenderEventHookOnReply pfnOnReply);


PG_DLL_API
int pgLiveMultiRenderInitialize(unsigned int* lpuInstID,
	const char* sUser, const char* sPass, const char* sSvrAddr,
	const char* sRelayAddr, int iP2PTryTime, const char* sInitParam);


PG_DLL_API
void pgLiveMultiRenderCleanup(unsigned int uInstID);


PG_DLL_API
int pgLiveMultiRenderGetSelfPeer(unsigned int uInstID, char* sPeer, unsigned int uSize);


PG_DLL_API
int pgLiveMultiRenderLanScanStart(unsigned int uInstID);


PG_DLL_API
int pgLiveMultiRenderConnect(unsigned int uInstID, const char* sCapID);


PG_DLL_API
void pgLiveMultiRenderDisconnect(unsigned int uInstID, const char* sCapID);


PG_DLL_API
int pgLiveMultiRenderConnected(unsigned int uInstID, const char* sCapID);


PG_DLL_API
int pgLiveMultiRenderMessageSend(unsigned int uInstID,
	const char* sCapID, const char* sData);


PG_DLL_API
int pgLiveMultiRenderVideoModeSize(unsigned int uInstID,
	int iMode, int iWidth, int iHeight);


PG_DLL_API
int pgLiveMultiRenderVideoStart(unsigned int uInstID,
	const char* sCapID, int iVideoID, const char* sParam, unsigned int uViewID);


PG_DLL_API
void pgLiveMultiRenderVideoStop(unsigned int uInstID,
	const char* sCapID, int iVideoID);


PG_DLL_API
int pgLiveMultiRenderVideoFramePull(unsigned int uInstID,
	const char* sCapID, int iVideoID);


PG_DLL_API
int pgLiveMultiRenderVideoCamera(unsigned int uInstID,
	const char* sCapID, int iVideoID, const char* sJpgPath);


PG_DLL_API
int pgLiveMultiRenderVideoParam(unsigned int uInstID,
	const char* sCapID, int iVideoID, const char* sParam);


PG_DLL_API
int pgLiveMultiRenderVideoShowMode(unsigned int uInstID, int iMode);


PG_DLL_API
int pgLiveMultiRenderAudioStart(unsigned int uInstID,
	const char* sCapID, int iAudioID, const char* sParam);


PG_DLL_API
void pgLiveMultiRenderAudioStop(unsigned int uInstID,
	const char* sCapID, int iAudioID);


PG_DLL_API
int pgLiveMultiRenderAudioSpeech(unsigned int uInstID,
	const char* sCapID, int iAudioID, unsigned int bEnable);


PG_DLL_API
int pgLiveMultiRenderAudioParam(unsigned int uInstID,
	const char* sCapID, int iAudioID, const char* sParam);


PG_DLL_API
int pgLiveMultiRenderAudioMute(unsigned int uInstID,
	const char* sCapID, int iAudioID, unsigned int bInput, unsigned int bOutput);


PG_DLL_API
int pgLiveMultiRenderAudioSyncDelay(unsigned int uInstID,
	const char* sCapID, int iAudioID, int iVideoID);


PG_DLL_API
int pgLiveMultiRenderRecordStart(unsigned int uInstID,
	const char* sCapID, const char* sAviPath, int iVideoID, int iAudioID);


PG_DLL_API
void pgLiveMultiRenderRecordStop(unsigned int uInstID, const char* sCapID);


PG_DLL_API
int pgLiveMultiRenderFilePutRequest(unsigned int uInstID,
	const char* sCapID, const char* sPath, const char* sPeerPath);


PG_DLL_API
int pgLiveMultiRenderFileGetRequest(unsigned int uInstID,
	const char* sCapID, const char* sPath, const char* sPeerPath);


PG_DLL_API
int pgLiveMultiRenderFileAccept(unsigned int uInstID,
	const char* sCapID, const char* sPath);


PG_DLL_API
int pgLiveMultiRenderFileReject(unsigned int uInstID,
	const char* sCapID, int iErrCode);


PG_DLL_API
int pgLiveMultiRenderFileCancel(unsigned int uInstID, const char* sCapID);


PG_DLL_API
int pgLiveMultiRenderSvrRequest(unsigned int uInstID,
	const char* sData, const char* sParam);


PG_DLL_API
void pgLiveMultiRenderVersion(char* lpszVersion, unsigned int uSize);


#ifdef __cplusplus
}
#endif

#endif
