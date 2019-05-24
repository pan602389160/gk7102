//
// Created by turing on 2019-02-26.
//

#include <stdio.h>
#include <string.h>
#include "motion_detect_c.h"


int getMotionResult(MotionDetection *md, const unsigned char *image, int width, int height, int channels,
                    TL_IMAGE_TYPE imageType, long long nowTime) {

    IplImageTL *imageGRAY;
    int isMove;

    if (nowTime - md->lastTimer < md->compareTimer) {
        return -1;
    }
    md->lastTimer = nowTime;

    imageGRAY = cvCreateImageGRAYTLByUChar(image, width, height, channels, imageType);

    setInputImage(md, imageGRAY, imageType);

    cvFreeImageTL(imageGRAY);

    isMove = getResult(md);
    if (isMove > 0 && nowTime - md->lastMdTimer >= (long long) md->serverWaitTime) {
        md->lastMdTimer = nowTime;
        md->lastSend = 1;
        return 1;
    } else {
        md->lastSend = 0;
        return 0;
    }
}

void initMotionDetection(MotionDetection *md, int min_thr, int max_thr, int minThrWaitTime, int dstHeight, int cropRate,
                         int serverWaitTime, int timer, long long nowTime) {
//    int frameThr;
    md->min_thr = min_thr * (100 - cropRate) / 100;
    md->max_thr = max_thr * (100 - cropRate) / 100;
    md->minThrWaitTime = minThrWaitTime;
    md->fps = 5;
    md->dstHeight = dstHeight;
    md->cropRate = cropRate;

//    frameThr = md->minThrWaitTime * fps / 1000;
    // md->send_state = 2 + (1 < frameThr ? 1 : frameThr);
    md->send_state = 2;

    md->number_of_changes = 0;
    md->state = 0;
    md->sever_wait_state = 0;
    md->serverWaitTime = serverWaitTime;
    md->lastSend = 0;
    md->lossFrame = md->fps / md->fps;
    md->lossFrameThr = 1;
    md->compareTimer = timer;
    md->lastTimer = nowTime + md->compareTimer;
    md->lastMdTimer = nowTime + md->serverWaitTime;
    md->frame1 = NULL;

}

void setInputImage(MotionDetection *md, IplImageTL *frameL, TL_IMAGE_TYPE imageType) {
    int cutHeight;
    int flag = 0;
    int imWidth = frameL->width;
    int imHeight = frameL->height;

    int targetWidth = md->dstHeight * imWidth / imHeight;
    int targetHeight = md->dstHeight;

    IplImageTL *frame2 = cvCreateImageTL(targetWidth, targetHeight, frameL->nChannels, imageType);
    cvResizeTL(frameL, frame2, TL_CV_INTER_LINEAR);

    cutHeight = frame2->height * md->cropRate / 100;
    cvCutTL(frame2,frame2,cutHeight);
    frame2->height = frame2->height - cutHeight;

    md->number_of_changes = 0;

    if (md->frame1 == NULL) {
        // first
        cvFreeImageTL(md->frame1);
        md->frame1 = cvCreateImageTL(frame2->width, frame2->height, frame2->nChannels, imageType);
        cvCopyImageTL(frame2, md->frame1);
        flag = 1;
    } else if (md->frame1->width != frame2->width || md->frame1->height != frame2->height) {
        cvFreeImageTL(md->frame1);
        md->frame1 = cvCreateImageTL(frame2->width, frame2->height, frame2->nChannels, imageType);
        cvCopyImageTL(frame2, md->frame1);
        flag = 1;
    }

    if (frame2->width != md->frame1->width || frame2->height != md->frame1->height) {
        printf("error 101 frame1 frame2 shape is diff \n");
        cvFreeImageTL(frame2);
        cvFreeImageTL(md->frame1);
        return;
    }

    imageCompare(md, frame2);

    if (flag == 0 && md->frame1->imageData != NULL) {
        cvFreeImageTL(md->frame1);
        md->frame1 = cvCreateImageTL(frame2->width, frame2->height, frame2->nChannels, imageType);
        cvCopyImageTL(frame2, md->frame1);
    }

    updateState(md);

    cvFreeImageTL(frame2);
}

void imageCompare(MotionDetection *md, IplImageTL *frame2) {
    IplImageTL *temp;
    IplImageTL *motion;
    motion = cvCreateImageTL(md->frame1->width, md->frame1->height, md->frame1->nChannels, TL_CV_RGB2GRAY);
    cvAbsDiffTL(md->frame1, frame2, motion);
    cvThresholdTL(motion, motion, 35, 255, TL_CV_THRESH_BINARY);
    temp = cvCreateImageTL(motion->width, motion->height, motion->nChannels, TL_CV_RGB2GRAY);
    cvCopyImageTL(motion, temp);
    memset(motion->imageData, 0, motion->width * motion->height);

    cvErodeTL(temp, motion, 0, 1);
    cvFreeImageTL(temp);

    getDifference(md,motion);
    cvFreeImageTL(motion);
}


void getDifference(MotionDetection *md, IplImageTL *motion) {
    int i = 0, j = 0;
    unsigned char *pixel_x;
    int pixel;
    for (j = 0; j < motion->height; j++) { // height
        for (i = 0; i < motion->width; i++) { // width
            pixel_x = (unsigned char *) (motion->imageData + j * motion->widthStep + i);
            pixel = (*pixel_x) + 0;
            if (pixel == 255) {
                md->number_of_changes++;
            }
        }
    }
}

void updateState(MotionDetection *md) {
    if (md->state >= md->send_state && md->lastSend == 1) {
        md->state = 0;
    }

    if (md->state == 0 && md->number_of_changes > md->max_thr) {
        md->state = 1;
    } else if (md->state == 1 && md->number_of_changes < md->min_thr) {
        md->state = 2;
    } else if (md->state >= 2) {
        if (md->number_of_changes < md->min_thr) {
            md->state += 1;
        } else {
            md->state = 1;
        }
    }
}

int getNumOfChanges(MotionDetection *md) {
    return md->number_of_changes;
}

int getResult(MotionDetection *md) {
    return md->state >= md->send_state;
}


