
#ifndef _CALLBACK_AUDIO_QUEUE_H
#define _CALLBACK_AUDIO_QUEUE_H

#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

#include "pgLibDevAudioIn.h"
#include "pgLibDevAudioOut.h"
#include "pgLibDevAudioConvert.h"


#define PG_DEV_AUDIO_QUEUE_INIT() {-1, -1, -1, -1}


typedef struct {

	int iDevIDIn;
	int iDevIDOut;

	int iCvtIDIn;
	int iCvtIDOut;
	
	unsigned int uFormatIn;
	unsigned int uDelayIn;
	
	unsigned int uPlayCount;
	unsigned int uFrameSizeOut;

	pthread_t tThread;
	unsigned int uThreadRunning;
	pthread_cond_t tThreadCond;
	pthread_mutex_t tThreadMutex;
	unsigned int uThreadSignal;
	unsigned int uThreadWait;

} PG_DEV_CALLBACK_AUDIO_QUEUE_S;


/// --------------------------------------------------------
// Record functions
int pgDevAudioQueueRecordOpen(PG_DEV_CALLBACK_AUDIO_QUEUE_S* lpstQueue,
	unsigned int uPopFormat, unsigned int uPushSampleRate,
	unsigned int uPushSampleSize, unsigned int uDelayMs);

void pgDevAudioQueueRecordClose(PG_DEV_CALLBACK_AUDIO_QUEUE_S* lpstQueue);

int pgDevAudioQueueRecordPush(PG_DEV_CALLBACK_AUDIO_QUEUE_S* lpstQueue,
	unsigned int uFormat, const void* lpData, unsigned int uDataSize);

int pgDevAudioQueueRecordPop(PG_DEV_CALLBACK_AUDIO_QUEUE_S* lpstQueue,
	void *lpData, unsigned int uDataSize);


/// --------------------------------------------------------
// Play functions
int pgDevAudioQueuePlayOpen(PG_DEV_CALLBACK_AUDIO_QUEUE_S* lpstQueue,
	unsigned int uPopFormat, unsigned int uPopSampleRate, unsigned int uPopSampleSize);

void pgDevAudioQueuePlayClose(PG_DEV_CALLBACK_AUDIO_QUEUE_S* lpstQueue);

int pgDevAudioQueuePlayPush(PG_DEV_CALLBACK_AUDIO_QUEUE_S* lpstQueue,
	unsigned int uFormat, const void* lpData, unsigned int uDataSize);

int pgDevAudioQueuePlayPop(PG_DEV_CALLBACK_AUDIO_QUEUE_S* lpstQueue,
	void *lpData, unsigned int uDataSize);


#endif
