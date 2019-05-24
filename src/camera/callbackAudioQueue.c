

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "callbackAudioQueue.h"


#if 1
#define dprintf printf
#else
#define dprintf
#endif


// 40ms per frame.
#define FRAME_INTERVAL  40

static int s_iDevIDTemp = 0;

static unsigned int GET_TIMESTAMP() {
	struct timeval stTime;
	gettimeofday(&stTime, 0);
	return (stTime.tv_sec * 1000 + stTime.tv_usec / 1000);
}

static void Sleep(unsigned int uMilliSecond)
{
	usleep(uMilliSecond * 1000);
}

static void AudioQueueWait(PG_DEV_CALLBACK_AUDIO_QUEUE_S* lpstQueue)
{
	if (lpstQueue != 0) {
		pthread_mutex_lock(&lpstQueue->tThreadMutex); 
		if (!lpstQueue->uThreadSignal) {
			lpstQueue->uThreadWait = 1;
	    	pthread_cond_wait(&lpstQueue->tThreadCond, &lpstQueue->tThreadMutex);
	    	lpstQueue->uThreadWait = 0;
	    }
		lpstQueue->uThreadSignal = 0;
		pthread_mutex_unlock(&lpstQueue->tThreadMutex);
	}
}

static void AudioQueueSignal(PG_DEV_CALLBACK_AUDIO_QUEUE_S* lpstQueue)
{
	if (lpstQueue != 0) {
		pthread_mutex_lock(&lpstQueue->tThreadMutex); 
		lpstQueue->uThreadSignal = 1;
		if (lpstQueue->uThreadWait) {
			pthread_cond_signal(&lpstQueue->tThreadCond);
		}
		pthread_mutex_unlock(&lpstQueue->tThreadMutex);
	}
}


///
// In this demo, we use a thread to simulate Audio output callback.
static void* AudioOutputThreadProc(void* lp)
{
	dprintf("AudioOutputThreadProc\n");

	if (pthread_detach(pthread_self()) != 0) {
		dprintf("AudioOutThreadProc, err=%d\n", errno);
	}

	PG_DEV_CALLBACK_AUDIO_QUEUE_S* pstQueue = (PG_DEV_CALLBACK_AUDIO_QUEUE_S*)lp;
	if (pstQueue == 0){
 		dprintf("AudioOutThreadProc, queue insten is null\n");
		return NULL;
	}
	
	unsigned int uPlayedSize = 0;
	unsigned int uStampStart = 0;
	unsigned int uCallbackCount = 0;

	while (pstQueue->uThreadRunning != 0) {

		if (pstQueue->iCvtIDIn > 0) {
			// Not message in queue, wait.
			AudioQueueWait(pstQueue);

			while (1) {
				// Pop record audio data from queue.
				unsigned char ucDataPop[2048];
				int iPopSize = pgDevAudioConvertPopS(pstQueue->iCvtIDIn, ucDataPop, sizeof(ucDataPop));
				if (iPopSize <= 0) {
					break;
				}

				// Report audio play progress to SDK.
				uPlayedSize += pstQueue->uFrameSizeOut;
				pgDevAudioOutPlayedProc(pstQueue->iDevIDOut, uPlayedSize, 0);

				// Input audio record data to SDK.
				pgDevAudioInRecordProc(pstQueue->iDevIDIn, ucDataPop,
					iPopSize, pstQueue->uFormatIn, pstQueue->uDelayIn);
			}
			
			uCallbackCount = 0;
		}
		else {
			// No record, return played size by timer.
			if (pstQueue->uPlayCount <= uCallbackCount) {
				Sleep(5);
				continue;
			}

			// Get start stamp.
			if (uCallbackCount == 0) {
	 			uStampStart = GET_TIMESTAMP();
	 		}

			unsigned int uStamp = GET_TIMESTAMP();
			unsigned int uDeltaReal = (uStamp < uStampStart) ? (0xffffffff - uStampStart + uStamp) : (uStamp - uStampStart);
			unsigned int uDeltaCalc = FRAME_INTERVAL * uCallbackCount;
			if (uDeltaReal < uDeltaCalc || (uDeltaReal - uDeltaCalc) < FRAME_INTERVAL) {
				Sleep(5);
				continue;
			}

			uPlayedSize += pstQueue->uFrameSizeOut;
			pgDevAudioOutPlayedProc(pstQueue->iDevIDOut, uPlayedSize, 0);
			uCallbackCount++;
		}
	}

	dprintf("AudioOutputThreadProc end\n");
 	pthread_exit(NULL);
}


/// --------------------------------------------------------
// Record functions
int pgDevAudioQueueRecordOpen(PG_DEV_CALLBACK_AUDIO_QUEUE_S* lpstQueue,
	unsigned int uPopFormat, unsigned int uPushSampleRate,
	unsigned int uPushSampleSize, unsigned int uDelayMs)
{
	if (lpstQueue != 0) {
		int iCvtIDIn = pgDevAudioConvertAlloc(0,
			uPopFormat, uPushSampleRate, uPushSampleSize);
		if (iCvtIDIn > 0) {
			s_iDevIDTemp++;
			lpstQueue->uFormatIn = uPopFormat;
			lpstQueue->uDelayIn = ((uDelayMs != 0) ? uDelayMs : 160);
			lpstQueue->iCvtIDIn = iCvtIDIn;
			lpstQueue->iDevIDIn = s_iDevIDTemp;		
			return lpstQueue->iDevIDIn;
		}
		dprintf("pgDevAudioQueueRecordOpen: Alloc audio convert failed.\n");
	}
	return -1;
}

void pgDevAudioQueueRecordClose(PG_DEV_CALLBACK_AUDIO_QUEUE_S* lpstQueue)
{
	if (lpstQueue != 0) {
		if (lpstQueue->iCvtIDIn > 0) {
			pgDevAudioConvertFree(lpstQueue->iCvtIDIn);
			lpstQueue->iCvtIDIn = -1;
		}
		lpstQueue->iDevIDIn = -1;
	}
}

int pgDevAudioQueueRecordPush(PG_DEV_CALLBACK_AUDIO_QUEUE_S* lpstQueue,
	unsigned int uFormat, const void* lpData, unsigned int uDataSize)
{
	if (lpstQueue != 0) {
		int iRet = pgDevAudioConvertPush(lpstQueue->iCvtIDIn, uFormat, lpData, uDataSize);
		if (iRet > 0) {
			AudioQueueSignal(lpstQueue);
		}
		return iRet;
	}
	return -1;
}

int pgDevAudioQueueRecordPop(PG_DEV_CALLBACK_AUDIO_QUEUE_S* lpstQueue,
	void *lpData, unsigned int uDataSize)
{
	if (lpstQueue != 0) {
		return pgDevAudioConvertPopS(lpstQueue->iCvtIDIn, lpData, uDataSize);
	}
	return -1;
}


/// --------------------------------------------------------
// Play functions
int pgDevAudioQueuePlayOpen(PG_DEV_CALLBACK_AUDIO_QUEUE_S* lpstQueue,
	unsigned int uPopFormat, unsigned int uPopSampleRate, unsigned int uPopSampleSize)
{
	if (lpstQueue != 0) {
		int iCvtIDOut = pgDevAudioConvertAlloc(1, uPopFormat, uPopSampleRate, uPopSampleSize);
		if (iCvtIDOut > 0) {

			// In this demo, we use a thread to simulate video playing.
		 	pthread_attr_t attr;
		 	pthread_attr_init(&attr);
		 	lpstQueue->uThreadRunning = 1;

		 	int iRet = pthread_create(&lpstQueue->tThread, &attr, AudioOutputThreadProc, lpstQueue);
		 	pthread_attr_destroy(&attr);
		 	if (iRet == 0) {
		 		
				pthread_cond_init(&lpstQueue->tThreadCond, 0);
				pthread_mutex_init(&lpstQueue->tThreadMutex, 0);
				lpstQueue->uThreadSignal = 0;
				lpstQueue->uThreadWait = 0;
		 		
				s_iDevIDTemp++;
				lpstQueue->uPlayCount = 0;
				lpstQueue->iCvtIDOut = iCvtIDOut;
				lpstQueue->iDevIDOut = s_iDevIDTemp;
				return lpstQueue->iDevIDOut;
		 	}

	 		lpstQueue->uThreadRunning = 0;
		 	pgDevAudioConvertFree(iCvtIDOut);

			dprintf("AudioOutOpen: start thread failed.\n");
		}

		dprintf("AudioOutOpen: Alloc audio convert failed.\n");
	} 

	return -1;
}

void pgDevAudioQueuePlayClose(PG_DEV_CALLBACK_AUDIO_QUEUE_S* lpstQueue)
{
	if (lpstQueue != 0) {
		lpstQueue->uThreadRunning = 0;
		AudioQueueSignal(lpstQueue);
		Sleep(100);

	 	if (lpstQueue->iCvtIDOut > 0) {
		 	pgDevAudioConvertFree(lpstQueue->iCvtIDOut);
			lpstQueue->iCvtIDOut = -1;
		}
		lpstQueue->iDevIDOut = -1;

		pthread_cond_destroy(&lpstQueue->tThreadCond);
		pthread_mutex_destroy(&lpstQueue->tThreadMutex);
	}
}

int pgDevAudioQueuePlayPush(PG_DEV_CALLBACK_AUDIO_QUEUE_S* lpstQueue,
	unsigned int uFormat, const void* lpData, unsigned int uDataSize)
{
	if (lpstQueue != 0) {
		lpstQueue->uFrameSizeOut = uDataSize;
		int iRet = pgDevAudioConvertPush(lpstQueue->iCvtIDOut, uFormat, lpData, uDataSize);
		if (iRet > 0) {
			lpstQueue->uPlayCount++;
		}
		return iRet;
	}
 	return -1;
}

int pgDevAudioQueuePlayPop(PG_DEV_CALLBACK_AUDIO_QUEUE_S* lpstQueue,
	void *lpData, unsigned int uDataSize)
{
	if (lpstQueue != 0) {
		return pgDevAudioConvertPopS(lpstQueue->iCvtIDOut, lpData, uDataSize);
	}
 	return -1;
}
