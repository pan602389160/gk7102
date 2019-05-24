/*************************************************************************
  copyright   : Copyright (C) 2014-2017, www.peergine.com, All rights reserved.
              :
  filename    : pgCppLiveMultiCapture.h
  discription : 
  modify      : create, chenbichao, 2017/05/10

*************************************************************************/
#ifndef _PG_CPP_LIVE_H
#define _PG_CPP_LIVE_H

#include "pgCPP.h"


///
// Live mode
enum PG_CPP_LIVE_MODE_E {
	PG_CPP_LIVE_MODE_RENDER  = 0,
	PG_CPP_LIVE_MODE_CAPTURE = 1,
};


///
// Log output callback.
typedef void (*TfnCppLiveLogOut)(unsigned int uLevel, const char* lpszOut);

PG_DLL_API
void pgCppLiveSetLogCallback(TfnCppLiveLogOut pfnLogout);


///
// Live event interface
class PG_DLL_API IPGCppLiveEvent {
public:
	virtual void OnEvent(CPGCPPString& sAction, CPGCPPString& sData, CPGCPPString& sRender) = 0;
};


///
// Live event interface
class PG_DLL_API IPGCppLiveEventHook {
public:
	virtual int OnExtRequest(CPGCPPString& sObj, int uMeth, CPGCPPString& sData, unsigned int uHandle, CPGCPPString& sPeer) = 0;
	virtual int OnReply(CPGCPPString& sObj, int uErr, CPGCPPString& sData, CPGCPPString& sParam) = 0;
};


///
// CPGCppLive class
class PG_DLL_API CPGCppLive
{
public:
	// Set the event callback proc.
	void SetEventProc(IPGCppLiveEvent* lpEvent);
	void SetEventHook(IPGCppLiveEventHook* lpEventHook);

	// Initialize and cleanup.
	unsigned int Initialize(int iMode, const char* sUser, const char* sPass,
		const char* sSvrAddr, const char* sRelayAddr, int iP2PTryTime, const char* sVideoParam);

	unsigned int InitializeEx(int iMode, const char* sUser, const char* sPass,
		const char* sSvrAddr, const char* sRelayAddr, int iP2PTryTime,
		const char* sInitParam, const char* sVideoParam, const char* sAudioParam);

	void Cleanup();
	
	void EventEnable(unsigned int bEnable);
	
	// Get peergine node
	IPGCPPNode* GetNode();
	CPGCPPString GetSelfPeer();
	
	// Start and stop the live.
	unsigned int Start(const char* sID);
	void Stop();
	unsigned int Connected();
	
	// Render handle
	unsigned int RenderReject(const char* sRenID);
	unsigned int RenderAccess(const char* sRenID, unsigned int bVideo, unsigned int bAudio);
	CPGCPPString RenderEnum(int iIndex);
	unsigned int RenderConnected(const char* sRenID);

	// Start and stop video.
	unsigned int VideoStart();
	void VideoStop();
	
	// Start and stop audio.
	unsigned int AudioStart();
	void AudioStop();
	
	// Mutilcast notify to all render.
	unsigned int NotifySend(const char* sMsg);
	
	// Send message to one render.
	unsigned int MessageSend(const char* sData, const char* sRender);

	// Pull one MJPEG frame.
	unsigned int FramePull();

	// Scan the captures in the same lan.
	unsigned int LanScanStart();

	// Video control
	unsigned int VideoSource(int iCameraNo);
	unsigned int VideoCamera(const char* sJpgPath);
	unsigned int VideoModeSize(int iMode, int iWidth, int iHeight);
	unsigned int VideoShowMode(int iMode);
	unsigned int VideoParam(const char* sVideoParam);
	unsigned int VideoRecordStart(const char* sAviPath);
	void VideoRecordStop();

	// Audio control
	unsigned int AudioSpeech(unsigned int bEnable);
	unsigned int AudioParam(const char* sParam);
	unsigned int AudioMute(unsigned int uInput, unsigned int uOutput);
	unsigned int AudioSyncDelay();

	// Record video and audio
	unsigned int RecordStart(const char* sAviPath, unsigned int bVideo, unsigned int bAudio);
	void RecordStop();

	// Live forward alloc and free
	int ForwardAlloc();
	int ForwardFree();
	
	// File transfer functions
	int FileGetRequest(const char* sPeer, const char* sPath, const char* sPeerPath);
	int FilePutRequest(const char* sPeer, const char* sPath, const char* sPeerPath);
	int FileAccept(const char* sPeer, const char* sPath);
	int FileReject(const char* sPeer);
	int FileCancel(const char* sPeer);

	// Server request.
	unsigned int SvrRequest(const char* sData);

	void Version(char* lpszVersion, unsigned int uSize);

	CPGCppLive();
	~CPGCppLive();
	
private:
	unsigned int m_uMode;
	unsigned int m_uInstID;
	CPGCPPString m_sVideoParam;
	CPGCPPString m_sAudioParam;
	CPGCPPString m_sCapID;
	unsigned int m_uViewID;

	IPGCppLiveEvent* m_pEvent;
	IPGCppLiveEventHook* m_pEventHook;
};

#endif
