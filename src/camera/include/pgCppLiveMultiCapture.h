/*************************************************************************
  copyright   : Copyright (C) 2014-2017, www.peergine.com, All rights reserved.
              :
  filename    : pgCppLiveMultiCapture.h
  discription : 
  modify      : create, chenbichao, 2017/05/10

*************************************************************************/
#ifndef _PG_CPP_LIVE_MULTI_CAPTURE_H
#define _PG_CPP_LIVE_MULTI_CAPTURE_H

#include "pgCPP.h"


///
// Set log output
typedef void (*TpfnPGCPPLiveMultiCaptureLogOutput)(unsigned int uLevel, const char* sOut);

PG_DLL_API
void pgCppLiveMultiCaptureSetLogOutput(TpfnPGCPPLiveMultiCaptureLogOutput pfnLogOutput);


///
// Live event interface
class PG_DLL_API IPGCppLiveMultiCaptureEvent {
public:
	virtual void OnEvent(CPGCPPString& sAction, CPGCPPString& sData, CPGCPPString& sRenID) = 0;
};


///
// Live event hook interface
class PG_DLL_API IPGCppLiveMultiCaptureEventHook {
public:
	virtual int OnExtRequest(CPGCPPString& sObj, int uMeth, CPGCPPString& sData, unsigned int uHandle, CPGCPPString& sPeer) = 0;
	virtual int OnReply(CPGCPPString& sObj, int uErr, CPGCPPString& sData, CPGCPPString& sParam) = 0;
};


class PG_DLL_API CPGCppLiveMultiCapture
{
public:
	// Set the event callback proc.
	void SetEventProc(IPGCppLiveMultiCaptureEvent* lpEvent);
	void SetEventHook(IPGCppLiveMultiCaptureEventHook* lpEventHook);
	
	// Initialize and cleanup.
	int Initialize(const char* sUser, const char* sPass,
		const char* sSvrAddr, const char* sRelayAddr,
		int iP2PTryTime, const char* lpszInitParam);
	void Cleanup();
	
	// Get peergine node
	IPGCPPNode* GetNode();
	CPGCPPString GetSelfPeer();

	// Render control method.
	int RenderReject(const char* sRenID);
	int RenderAccess(const char* sRenID, unsigned int bVideo, unsigned int bAudio);
	CPGCPPString RenderEnum(int iIndex);
	int RenderConnected(const char* sRenID);

	// Mutilcast notify to all render.
	int NotifySend(const char* sData);
	
	// Send message to one render.
	int MessageSend(const char* sRenID, const char* sData);
	
	// Video method
	int VideoModeSize(int iMode, int iWidth, int iHeight);
	int VideoStart(int iVideoID, const char* sParam, unsigned int uViewID);
	void VideoStop(int iVideoID);
	int VideoCamera(int iVideoID, const char* sJpgPath);
	int VideoParam(int iVideoID, const char* sParam);
	int VideoForwardAlloc(int iVideoID, const char* sParam);
	int VideoForwardFree(int iVideoID, const char* sParam);

	// Audio method
	int AudioStart(int iAudioID, const char* sParam);
	void AudioStop(int iAudioID);
	int AudioSpeech(int iAudioID, const char* sRenID, unsigned int bEnable);
	int AudioParam(int iAudioID, const char* sParam);
	int AudioMute(int iAudioID, unsigned int bInput, unsigned int bOutput);

	int RecordStart(const char* sTag, const char* sAviPath, int iVideoID, int iAudioID);
	void RecordStop(const char* sTag);

	// File transfer functions
	int FilePutRequest(const char* sRenID, const char* sPath, const char* sPeerPath);
	int FileGetRequest(const char* sRenID, const char* sPath, const char* sPeerPath);
	int FileAccept(const char* sRenID, const char* sPath);
	int FileReject(const char* sRenID, int iErrCode);
	int FileCancel(const char* sRenID);

	// Server request.
	int SvrRequest(const char* sData, const char* sParam);

	CPGCPPString Version();

	CPGCppLiveMultiCapture();
	~CPGCppLiveMultiCapture();

private:
	unsigned int m_uInstID;
	IPGCppLiveMultiCaptureEvent* m_pEvent;
	IPGCppLiveMultiCaptureEventHook* m_pEventHook;
};

#endif
