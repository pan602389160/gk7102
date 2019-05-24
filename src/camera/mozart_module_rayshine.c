#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include "pgLibLiveMultiError.h"
#include "pgLibLiveMultiCapture.h"
#include "callbackVideo.h"
#include "callbackAudio.h"
#include "debug_print.h"


unsigned int uVideoMode = 3;	//0x2 is 320x240 [ 3 is 640*480, 10 is 1280*720 ]
unsigned int g_set_definetion = 0;
unsigned int g_get_definetion = 0;
char szVideoParam[256] = { 0 };
int g_video_start = 0;//æ ‡è¯†å½•åƒæ˜¯å¦å¼€å¯ï¼Œé¿å…å¤šé‡å½•åƒå¼€å¯
int g_video_restart = 0;
int g_RenderLeave = 1;


void LogOutput(unsigned int uLevel, const char* lpszOut)
{
	//printf(lpszOut);
}

void GetInput(char* lpszBuf)
{
	gets(lpszBuf);

	// trim string.
	char* pszTemp0 = lpszBuf;
	while ((*pszTemp0) != '\0') {
		if (strchr(" \r\n\t", (*pszTemp0)) == 0) {
			break;
		}
		pszTemp0++;
	}

	char* pszTemp1 = lpszBuf + strlen(lpszBuf);
	while (pszTemp1 > pszTemp0) {
		if (strchr(" \r\n\t", (*(pszTemp1 - 1))) == 0) {
			break;
		}
		pszTemp1--;
	}

	int iSize = (pszTemp1 - pszTemp0);
	memmove(lpszBuf, pszTemp0, iSize);
	lpszBuf[iSize] = '\0';
}

void EventProc(unsigned int uInstID, const char* lpszAction, const char* lpszData, const char* lpszRenID)
{
	if (strcmp(lpszAction, "Message") == 0) {
		printf("MESSAGE: szData=%s, szRender=%s\n", lpszData, lpszRenID);

		int iRet = pgLiveMultiCaptureMessageSend(uInstID, lpszRenID, "Done !");
		if (iRet != PG_LIVE_MULTI_ERR_Normal) {
			printf("pgLiveMultiCaptureMessage: iErr=%d\n", iRet);
		}
	}
	else if (strcmp(lpszAction, "RenderJoin") == 0) {
		printf("RENDER_JOIN: szRender=%s\n", lpszRenID);
		purple_pr("RENDER_JOIN: szRender=%s\n", lpszRenID);
		stopall();	
		g_RenderLeave = 0;

		rayshine_set_camera();

		// Open video  
		//code:1ÎªMJPEG¡¢2ÎªVP8¡¢3ÎªH264¡¢4ÎªH265  
		//Rate: ÊÓÆµµÄÖ¡¼ä¼ä¸ô£¨ºÁÃë£©
		sprintf(szVideoParam, "(Code){3}(Mode){3}(Rate){50}(BitRate){500}(CameraNo){%s}(MaxStream){%s}",
			"0", "2");
		printf("############################start Video############################## \n");
		int iErr = pgLiveMultiCaptureVideoStart(uInstID, 0, szVideoParam, 0);
		if (iErr != PG_LIVE_MULTI_ERR_Normal) {
			printf("pgLiveVideoStart. iErr=%d\n", iErr);
		}

		// Open audio.
		iErr = pgLiveMultiCaptureAudioStart(uInstID, 0, "");
		if (iErr != PG_LIVE_MULTI_ERR_Normal) {
			printf("pgLiveAudioStart. iErr=%d\n", iErr);
		}

	}
	else if (strcmp(lpszAction, "RenderLeave") == 0) {
		printf("RENDER_LEAVE: szRender=%s\n", lpszRenID);
		purple_pr("RENDER_LEAVE: szRender=%s\n", lpszRenID);

		printf("############################stop audio##############################\n");
		pgLiveMultiCaptureAudioStop(uInstID, 0);
		printf("############################stop video##############################\n");
		pgLiveMultiCaptureVideoStop(uInstID, 0);
		startall();
		g_RenderLeave = 1;
		
	}
	else if (strcmp(lpszAction, "VideoStatus") == 0) {
		//printf("VIDEO_STATUS: szData=%s\n", lpszData);
	}
	else if (strcmp(lpszAction, "VideoCamera") == 0) {
		printf("CAMERA_FILE: szData=%s\n", lpszData);
	}
	else if (strcmp(lpszAction, "VideoFrameStat") == 0) {
		printf("FRAME_STAT: szData=%s, szRender=%s\n", lpszData, lpszRenID);
	}
	else if (strcmp(lpszAction, "Login") == 0) {
		printf("SVR_LOGIN: szData=%s\n", lpszData);
	}
	else if (strcmp(lpszAction, "Logout") == 0) {
		printf("SVR_LOGOUT: szData=%s\n", lpszData);
	}
	else if (strcmp(lpszAction, "SvrReply") == 0) {
		printf("SVR_REPLY: szData=%s\n", lpszData);
	}
	else if (strcmp(lpszAction, "SvrReplyError") == 0) {
		printf("SVR_ERROR: szData=%s\n", lpszData);
	}
	else if (strcmp(lpszAction, "SvrNotify") == 0) {
		printf("SVR_NOTIFY: szData=%s\n", lpszData);
	}
	else if (strcmp(lpszAction, "KickOut") == 0) {
		printf("SVR_KICK_OUT: szData=%s\n", lpszData);
	}
	else if (strcmp(lpszAction, "ForwardAllocReply") == 0) {
		printf("FORWARD_ALLOC_REPLY: szData=%s, szRender=%s\n", lpszData, lpszRenID);
	}
	else if (strcmp(lpszAction, "ForwardFreeReply") == 0) {
		printf("FORWARD_FREE_REPLY: szData=%s, szRender=%s\n", lpszData, lpszRenID);
	}
	else if (strcmp(lpszAction, "FilePutRequest") == 0) {
		printf("FILE_PUT_REQUEST: szData=%s, szRender=%s\n", lpszData, lpszRenID);
		int iRet = pgLiveMultiCaptureFileAccept(uInstID, lpszRenID, "testput.txt");
		if (iRet != PG_LIVE_MULTI_ERR_Normal) {
			printf("pgLiveFileAccept: iRet=%d\n", iRet);
		}
	}
	else if (strcmp(lpszAction, "FileGetRequest") == 0) {
		printf("FILE_GET_REQUEST: szData=%s, szRender=%s\n", lpszData, lpszRenID);
		int iRet = pgLiveMultiCaptureFileAccept(uInstID, lpszRenID, "testget.txt");
		if (iRet != PG_LIVE_MULTI_ERR_Normal) {
			printf("pgLiveFileAccept: iRet=%d\n", iRet);
		}
	}
	else if (strcmp(lpszAction, "FileAccept") == 0) {
		printf("FILE_ACCEPT: szData=%s, szRender=%s\n", lpszData, lpszRenID);
	}
	else if (strcmp(lpszAction, "FileReject") == 0) {
		printf("FILE_REJECT: szData=%s, szRender=%s\n", lpszData, lpszRenID);
	}
	else if (strcmp(lpszAction, "FileProgress") == 0) {
		printf("FILE_PROGRESS: szData=%s, szRender=%s\n", lpszData, lpszRenID);
	}
	else if (strcmp(lpszAction, "FileFinish") == 0) {
		printf("FILE_FINISH: szData=%s, szRender=%s\n", lpszData, lpszRenID);
	}
	else if (strcmp(lpszAction, "FileAbort") == 0) {
		printf("FILE_ABORT: szData=%s, szRender=%s\n", lpszData, lpszRenID);
	}
	else if (strcmp(lpszAction, "PeerInfo") == 0) {
		printf("PeerInfo: szData=%s\n", lpszData);
	}
}


/*
unsigned int SendSvrPush(unsigned int uInstID, const char* lpszRenID, const char* lpszMsg)
{
	char szBuf[256] = {0};
	sprintf(szBuf, "Forward?(User){_RND_%s}(Msg){%s}", lpszRenID, lpszMsg);

	int iErr = pgLiveMultiCaptureSvrRequest(uInstID, szBuf, "");
	return (iErr == PG_LIVE_MULTI_ERR_Normal);
}
*/


int mozart_start_camera_rayshine()
{

	char szVer[32] = {0};
	pgLiveMultiCaptureVersion(szVer, sizeof(szVer));
	printf("pgLibLive version: %s\n\n", szVer);

	char szDevID[128] = {0};
	char szSvrAddr[128] = {0};
	char szCameraNo[128] = {0};
	char szMaxStream[128] = {0};

	int fd = open("/usr/data/mac.txt",O_RDONLY);
	if(fd > 0){
		char id_buf[32] = {0};
		read(fd,id_buf,sizeof(id_buf));
		printf("id_buf = %s\n",id_buf);
		strncpy(szDevID, id_buf,strlen(id_buf));
		szDevID[strlen(id_buf)] = '\0';
		close(fd);
	}else{
		strcpy(szDevID, "lskj102202");
	}
	
	//strcpy(szSvrAddr, "connect.peergine.com:7781");
	strcpy(szSvrAddr, "120.77.12.202:7781"); // GK 120.77.12.202:7781  bitman 120.55.42.83:7781
	strcpy(szCameraNo, "0");
	strcpy(szMaxStream, "2");
	
	purple_pr("\n*************************************\n");
	purple_pr("szDevID : %s\n",szDevID);
	purple_pr("szSvrAddr : %s\n",szSvrAddr);
	purple_pr("szCameraNo : %s\n",szCameraNo);
	purple_pr("szMaxStream : %s\n",szMaxStream);
	purple_pr("*************************************\n");

	// Register the video input callback interface.
	RegisterVideoCallback();
	RegisterAudioCallback();
	//gk init
	//app_initialize();
	pgLiveMultiCaptureSetCallback(LogOutput, EventProc);

	unsigned int uInstID = 0;
	if (pgLiveMultiCaptureInitialize(&uInstID, szDevID, "", szSvrAddr, "", 2, "") != PG_LIVE_MULTI_ERR_Normal) {
		printf("Init peergine module failed.\n");
		return 0;
	}

	printf("Init peergine module success.\n");
//------------------------------------------------------
	#if 0
	int iErr = -1;
	while(1){

		//while (g_RenderLeave) {
		//	usleep(2000*1000);
		//}

		printf("############################start Video############################## \n");
		// Open video
		//sprintf(szVideoParam, "(Code){1}(Mode){%u}(Rate){80}(BitRate){500}(CameraNo){1}", uVideoMode);
		sprintf(szVideoParam, "(Code){3}(Mode){3}(Rate){50}(BitRate){500}(CameraNo){%s}(MaxStream){%s}",
		szCameraNo, szMaxStream);
		
		iErr = pgLiveMultiCaptureVideoStart(uInstID, 0, szVideoParam, 0);
		if (iErr != PG_LIVE_MULTI_ERR_Normal) {
			printf("pgLiveVideoStart. iErr=%d\n", iErr);
			return 0;
		}

		// Open audio.
		printf("############################start Audio############################## \n");
		//char szAudioParam[256] = { 0 };
		//strcpy(szAudioParam,"(Reliable){0}(MuteInput){0}(MuteOutput){0}(EchoCancel){1}");
		//printf("szAudioParam = %s\n",szAudioParam);
		//iErr = pgLiveMultiCaptureAudioStart(uInstID, 0, szAudioParam);
		iErr = pgLiveMultiCaptureAudioStart(uInstID, 0, "");
		if (iErr != PG_LIVE_MULTI_ERR_Normal) {
			printf("pgLiveAudioStart. iErr=%d\n", iErr);
			return 0;
		}
		

		if(!g_video_start)
			g_video_start = 1;
		while (!g_RenderLeave) {
			usleep(2000*1000);
		}
		
		printf("############################stop audio##############################\n");
		pgLiveMultiCaptureAudioStop(uInstID, 0);
		printf("############################stop video##############################\n");
		pgLiveMultiCaptureVideoStop(uInstID, 0);
		
	}
	pgLiveMultiCaptureCleanup(uInstID);
	#endif
//-------------------------------------------------------3-24
#if 0
while(1){
		
		while (g_RenderLeave) {
			usleep(2000*1000);
		}
		rayshine_set_camera();

		// Open video  
		//code:1ÎªMJPEG¡¢2ÎªVP8¡¢3ÎªH264¡¢4ÎªH265  
		//Rate: ÊÓÆµµÄÖ¡¼ä¼ä¸ô£¨ºÁÃë£©
		sprintf(szVideoParam, "(Code){3}(Mode){3}(Rate){50}(BitRate){500}(CameraNo){%s}(MaxStream){%s}",
			szCameraNo, szMaxStream);
		printf("############################start Video############################## \n");
		int iErr = pgLiveMultiCaptureVideoStart(uInstID, 0, szVideoParam, 0);
		if (iErr != PG_LIVE_MULTI_ERR_Normal) {
			printf("pgLiveVideoStart. iErr=%d\n", iErr);
			break;
		}

		// Open audio.
		iErr = pgLiveMultiCaptureAudioStart(uInstID, 0, "");
		if (iErr != PG_LIVE_MULTI_ERR_Normal) {
			printf("pgLiveAudioStart. iErr=%d\n", iErr);
			break;
		}
		while (!g_RenderLeave) {
			usleep(2000*1000);
		}
		
		printf("############################stop audio##############################\n");
		pgLiveMultiCaptureAudioStop(uInstID, 0);
		printf("############################stop video##############################\n");
		pgLiveMultiCaptureVideoStop(uInstID, 0);

}
	#endif
	//pgLiveMultiCaptureRecordStart(uInstID,"bbb","./test.avi",0,-1);
	int cnt = 0;
	while (1) {
		usleep(2000*1000);
		//if(cnt++ > 10){
			//printf("****pgLiveMultiCaptureRecordStop\n");
			// pgLiveMultiCaptureRecordStop(uInstID,"bbb");
		//}		
	}

	pgLiveMultiCaptureAudioStop(uInstID, 0);
	pgLiveMultiCaptureVideoStop(uInstID, 0);

	pgLiveMultiCaptureCleanup(uInstID);
	printf("Clean peergine module.");

	getchar();

	return 0;
}

