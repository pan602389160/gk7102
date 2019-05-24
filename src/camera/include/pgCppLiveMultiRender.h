/*************************************************************************
  copyright   : Copyright (C) 2014-2017, www.peergine.com, All rights reserved.
              :
  filename    : pgCppLiveMultiRender.h
  discription : 
  modify      : create, chenbichao, 2017/05/10

*************************************************************************/
#ifndef _PG_CPP_LIVE_MULTI_RENDER_H
#define _PG_CPP_LIVE_MULTI_RENDER_H

#include "pgCPP.h"


///
// Set log output
typedef void (*TpfnPGCPPLiveMultiRenderLogOutput)(unsigned int uLevel, const char* sOut);

PG_DLL_API
void pgCppLiveMultiRenderSetLogOutput(TpfnPGCPPLiveMultiRenderLogOutput pfnLogOutput);


///
// Live event callback interface
class PG_DLL_API IPGCppLiveMultiRenderEvent {
public:
	virtual void OnEvent(CPGCPPString& sAction, CPGCPPString& sData, CPGCPPString& sCapID) = 0;
};


///
// Live event hook interface
class PG_DLL_API IPGCppLiveMultiRenderEventHook {
public:
	virtual int OnExtRequest(CPGCPPString& sObj, int uMeth, CPGCPPString& sData, unsigned int uHandle, CPGCPPString& sPeer) = 0;
	virtual int OnReply(CPGCPPString& sObj, int uErr, CPGCPPString& sData, CPGCPPString& sParam) = 0;
};


class PG_DLL_API CPGCppLiveMultiRender
{
public:
	// Set the event callback proc.
	void SetEventProc(IPGCppLiveMultiRenderEvent* lpEvent);
	void SetEventHook(IPGCppLiveMultiRenderEventHook* lpEventHook);
	
	int Initialize(const char* sUser, const char* sPass,
		const char* sSvrAddr, const char* sRelayAddr,
		int iP2PTryTime, const char* sInitParam);
	void Cleanup();
	
	// Get peergine node
	IPGCPPNode* GetNode();
	CPGCPPString GetSelfPeer();

	int LanScanStart();

	int Connect(const char* sCapID);
	void Disconnect(const char* sCapID);
	int Connected(const char* sCapID);

	int MessageSend(const char* sCapID, const char* sData);
	
	int VideoModeSize(int iMode, int iWidth, int iHeight);
	int VideoStart(const char* sCapID, int iVideoID, const char* sParam, unsigned int uViewID);
	void VideoStop(const char* sCapID, int iVideoID);
	int VideoFramePull(const char* sCapID, int iVideoID);
	int VideoCamera(const char* sCapID, int iVideoID, const char* sJpgPath);
	int VideoParam(const char* sCapID, int iVideoID, const char* sParam);
	int VideoShowMode(int iMode);
	
	int AudioStart(const char* sCapID, int iAudioID, const char* sParam);
	void AudioStop(const char* sCapID, int iAudioID);
	int AudioSpeech(const char* sCapID, int iAudioID, unsigned int bEnable);
	int AudioParam(const char* sCapID, int iAudioID, const char* sParam);
	int AudioMute(const char* sCapID, int iAudioID, unsigned int bInput, unsigned int bOutput);
	int AudioSyncDelay(const char* sCapID, int iAudioID, int iVideoID);
	
	int RecordStart(const char* sCapID, const char* sAviPath, int iVideoID, int iAudioID);
	void RecordStop(const char* sCapID);

	int FilePutRequest(const char* sCapID, const char* sPath, const char* sPeerPath);
	int FileGetRequest(const char* sCapID, const char* sPath, const char* sPeerPath);
	int FileAccept(const char* sCapID, const char* sPath);
	int FileReject(const char* sCapID, int iErrCode);
	int FileCancel(const char* sCapID);

	// Server request.
	int SvrRequest(const char* sData, const char* sParam);

	CPGCPPString Version();

	CPGCppLiveMultiRender();
	~CPGCppLiveMultiRender();

private:
	unsigned int m_uInstID;
	IPGCppLiveMultiRenderEvent* m_pEvent;
	IPGCppLiveMultiRenderEventHook* m_pEventHook;
};

#endif
