/****************************************************************
  copyright   : Copyright (C) 2011-2018, chenbichao,
                All rights reserved.
  filename    : pgWaveFile.cpp
  discription : 
  modify      : create, chenbichao, 2018/05/14

*****************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "pgWaveFile.h"


#pragma pack(1)

typedef struct tagPG_WAVEFORMAT_S {
	unsigned short wFormatTag;        /* format type */
	unsigned short nChannels;         /* number of channels (i.e. mono, stereo...) */
	unsigned int nSamplesPerSec;      /* sample rate */
	unsigned int nAvgBytesPerSec;     /* for buffer estimation */
	unsigned short nBlockAlign;       /* block size of data */
	unsigned short wBitsPerSample;    /* Number of bits per sample of mono data */
	unsigned short wSizeByte;
} PG_WAVEFORMAT_S;

#pragma pack()



unsigned int pgWaveFileOpen(PG_WAVE_FILE_CTX_S* lpCtx,
	const char* lpszPath, unsigned int uFormat, unsigned int uSampleRate)
{
	unsigned int uRes = 0;
	
	do {
		lpCtx->pFile = fopen(lpszPath, "wb");
		if (lpCtx->pFile == 0) {
			printf("pgWaveFileOpen: fopen failed, lpszPath=%s\n", lpszPath);
			break;
		}
	
		// riff block  
		int iWrite = fwrite("RIFF", 1, 4, lpCtx->pFile);
		if (iWrite < 4) {
			printf("pgWaveFileOpen: fwrite 'RIFF' failed\n");
			break;
		}
		
		unsigned int uTotalSize = 0;
		iWrite = fwrite(&uTotalSize, 1, 4, lpCtx->pFile);  
		if (iWrite < 4) {
			printf("pgWaveFileOpen: fwrite total size failed\n");
			break;
		}
	
		iWrite = fwrite("WAVE", 1, 4, lpCtx->pFile);  
		if (iWrite < 4) {
			printf("pgWaveFileOpen: fwrite 'WAVE' failed\n");
			break;
		}

		// fmt block  
		iWrite = fwrite("fmt ", 1, 4, lpCtx->pFile);  
		if (iWrite < 4) {
			printf("pgWaveFileOpen: fwrite 'fmt ' failed\n");
			break;
		}
		
		PG_WAVEFORMAT_S stFmt;
		memset(&stFmt, 0, sizeof(stFmt));

		switch (uFormat) {
		case PG_WAVE_FORMAT_Unknown:
		case PG_WAVE_FORMAT_PCM:
			stFmt.wBitsPerSample   = 16;
			break;
		
		case PG_WAVE_FORMAT_ALAW:
		case PG_WAVE_FORMAT_MULAW:
			stFmt.wBitsPerSample   = 8;
			break;

		default:
			break;
		}
		if (stFmt.wBitsPerSample == 0) {
			printf("pgWaveFileOpen: Format invalid\n");
			break;
		}

		stFmt.wFormatTag       = uFormat;
		stFmt.nChannels        = 1;
		stFmt.nSamplesPerSec   = uSampleRate;
		stFmt.nAvgBytesPerSec  = stFmt.nSamplesPerSec * (stFmt.wBitsPerSample / 8);  
		stFmt.nBlockAlign      = stFmt.nChannels * (stFmt.wBitsPerSample / 8);
	
		unsigned int uFmtSize = sizeof(PG_WAVEFORMAT_S);
		iWrite = fwrite(&uFmtSize, 1, 4, lpCtx->pFile);
		if (iWrite < 4) {
			printf("pgWaveFileOpen: fwrite 'format' length failed\n");
			break;
		}
		
		iWrite = fwrite(&stFmt, 1, sizeof(PG_WAVEFORMAT_S), lpCtx->pFile);
		if (iWrite < (int)sizeof(PG_WAVEFORMAT_S)) {
			printf("pgWaveFileOpen: fwrite 'format' failed\n");
			break;
		}
	
		// data block  
		iWrite = fwrite("data", 1, 4, lpCtx->pFile);
		if (iWrite < 4) {
			printf("pgWaveFileOpen: fwrite 'data' failed\n");
			break;
		}

		lpCtx->uDataSize = 0;
		iWrite = fwrite(&(lpCtx->uDataSize), 1, 4, lpCtx->pFile);
		if (iWrite < 4) {
			printf("pgWaveFileOpen: fwrite 'data' length failed\n");
			break;
		}
		
		uRes = 1;
	}
	while (0);
	
	if (!uRes) {
		if (lpCtx->pFile != 0) {
			fclose(lpCtx->pFile);
			lpCtx->pFile = 0;
		}
	}
	
	return uRes;
}

void pgWaveFileClose(PG_WAVE_FILE_CTX_S* lpCtx)
{
	if (lpCtx->pFile != 0) {
		
		unsigned int uTotalSize = (4 * 5) + sizeof(PG_WAVEFORMAT_S) + lpCtx->uDataSize;
		fseek(lpCtx->pFile, 4, SEEK_SET);
		fwrite(&uTotalSize, 1, 4, lpCtx->pFile);  

		fseek(lpCtx->pFile, ((4 * 6) + sizeof(PG_WAVEFORMAT_S)), SEEK_SET);
		fwrite(&(lpCtx->uDataSize), 1, 4, lpCtx->pFile);

		fflush(lpCtx->pFile);

		fclose(lpCtx->pFile);
		lpCtx->pFile = 0;

		printf("pgWaveFileClose: fclose finish\n");
	}
}

int pgWaveFileWrite(PG_WAVE_FILE_CTX_S* lpCtx, const void* lpData, unsigned int uSize)
{
	if (lpCtx->pFile != 0) {
		int iWrite = fwrite(lpData, 1, uSize, lpCtx->pFile);
		if (iWrite > 0) {
			lpCtx->uDataSize += iWrite;
		}
		else {
			printf("pgWavFileWrite: fwrite wave sample failed, uSize=%d\n", uSize);
		}
		return iWrite;
	}
	return -1;
}

void pgWaveFileZero(PG_WAVE_FILE_CTX_S* lpCtx)
{
	memset(lpCtx, 0, sizeof(PG_WAVE_FILE_CTX_S));
}
