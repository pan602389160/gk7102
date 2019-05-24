//
// Created by 邱模武 on 2019-02-26.
//

#ifndef TURINGIMAGE_MOTION_DETECT_C_H
#define TURINGIMAGE_MOTION_DETECT_C_H


#include "tl_imgproc.h"

typedef struct _MotionDetection_{
    int dstHeight;
    int min_thr;
    int max_thr;
    int minThrWaitTime;
    int fps;
    int number_of_changes;
    int state;                  //  0-before max thr  1-after max thr 2 after min thr
    int sever_wait_state;
    int send_state;
    int cropRate;
    int serverWaitTime;
    int lastSend;
    int lossFrame;
    int lossFrameThr;
    long long compareTimer;
    long long lastTimer;
    long long lastMdTimer;
    IplImageTL *frame1;
}MotionDetection;


#ifdef __cplusplus
extern "C"{
#endif // __cplusplus

void initMotionDetection(MotionDetection *md, int min_thr, int max_thr, int minThrWaitTime, int dstHeight, int cropRate,
                         int serverWaitTime, int timer, long long nowTime);
int getMotionResult(MotionDetection *md, const unsigned char *image, int width, int height, int channels, TL_IMAGE_TYPE imageType, long long nowTime);
void setInputImage(MotionDetection *md, IplImageTL *frameL,TL_IMAGE_TYPE imageType);

void updateState(MotionDetection *md);
void imageCompare(MotionDetection *md, IplImageTL *frame2);
void getDifference(MotionDetection *md, IplImageTL *motion);
int getNumOfChanges(MotionDetection *md);
int getResult(MotionDetection *md);


#ifdef __cplusplus
}
#endif

#endif //TURINGIMAGE_MOTION_DETECT_C_H
