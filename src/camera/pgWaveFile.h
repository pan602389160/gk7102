/****************************************************************
  copyright   : Copyright (C) 2011-2018, chenbichao,
                All rights reserved.
  filename    : pgWaveFile.H
  discription : 
  modify      : create, chenbichao, 2018/05/14

*****************************************************************/
#ifndef _PG_WAVE_FILE_H
#define _PG_WAVE_FILE_H


typedef enum _tagPG_WAVE_FORMAT_E {
	PG_WAVE_FORMAT_Unknown = 0x0,
	PG_WAVE_FORMAT_PCM     = 0x1,
	PG_WAVE_FORMAT_ALAW    = 0x6,
	PG_WAVE_FORMAT_MULAW   = 0x7,
} PG_WAVE_FORMAT_E;


typedef struct _tagPG_WAVE_FILE_CTX_S {
	FILE* pFile;
	unsigned int uDataSize;
} PG_WAVE_FILE_CTX_S;


unsigned int pgWaveFileOpen(PG_WAVE_FILE_CTX_S* lpCtx,
	const char* lpszPath, unsigned int uFormat, unsigned int uSampleRate);

void pgWaveFileClose(PG_WAVE_FILE_CTX_S* lpCtx);

int pgWaveFileWrite(PG_WAVE_FILE_CTX_S* lpCtx, const void* lpData, unsigned int uSize);

void pgWaveFileZero(PG_WAVE_FILE_CTX_S* lpCtx);

#endif
