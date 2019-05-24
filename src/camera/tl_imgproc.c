//
// Created by turing on 2019-02-21.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "tl_imgproc.h"

int cvFloorTL(double value) {
    int i = (int) value;
    return i - (i > value);
}

short saturate_cast_short(float v) {
    return (short) ((unsigned long int) ((long long int) v - SHRT_MIN) <= (unsigned long int) USHRT_MAX ? v : v > 0
                                                                                                              ? SHRT_MAX
                                                                                                              : SHRT_MIN);
}

IplImageTL *cvCreateImageTL(int width, int height, int nChannels, TL_IMAGE_TYPE imageType) {
    IplImageTL *image;
    int imageSize;
    if (nChannels == 1) {
        imageSize = width * height;
    } else if (imageType == TL_CV_YUV2GRAY_420) {
        imageSize = width * height * 3 / 2;
    } else if (imageType == TL_CV_YUV2GRAY_UYVY || imageType == TL_CV_YUV2GRAY_YUY2) {
        imageSize = width * height * 2;
    } else {
        imageSize = width * height * nChannels + width % 4 * height;
    }
    image = (IplImageTL *) malloc(sizeof(IplImageTL));
//    printf("imageSize:%d\n", imageSize);
//    printf("imageSize:%d\twidth:%d\theight:%d\tnChannels:%d\n", imageSize, width, height, nChannels);
    image->imageData = (char*)malloc(imageSize);
    memset(image->imageData,0,imageSize);
    image->width = width;//cols
    image->height = height;//rows
    image->widthStep = width * nChannels;
    image->nChannels = nChannels;
    image->imageSize = imageSize;
    return image;
}

//unsigned char 转image
IplImageTL *cvCreateImageTLByUChar(const unsigned char *src, int width, int height, int nChannels, TL_IMAGE_TYPE imageType) {
    int i = 0;
    IplImageTL *image;
    image = cvCreateImageTL(width, height, nChannels, imageType);
//    int allSize = width * height * nChannels;
//    printf("allSize:%d\twidth:%d\theight:%d\tnChannels:%d\n", allSize, width, height, nChannels);
    for (i = 0; i < image->imageSize; i++) {
        image->imageData[i] = src[i];
    }
    return image;
}

IplImageTL *cvCreateImageGRAYTLByUChar(const unsigned char *srcData, int srcWidth, int srcHeight, int srcChannel, TL_IMAGE_TYPE code) {

    int srcWidthStep = srcWidth * srcChannel;

    IplImageTL *imageGray = cvCreateImageTL(srcWidth, srcHeight, 1, code);

    unsigned char * dstData = (unsigned char *)imageGray->imageData;
    int dstWidthStep = imageGray->widthStep;

    if (code == TL_CV_RGB2GRAY) {
        cvRgbToGray(srcData, dstData, srcWidthStep, srcHeight, srcChannel,dstWidthStep);
    } else if (code == TL_CV_BGR2GRAY) {
        cvBgrToGray(srcData, dstData, srcWidthStep, srcHeight, srcChannel,dstWidthStep);
    } else if (TL_CV_GRAY2GRAY == code) {
        cvGrayToGray(srcData, dstData, srcWidthStep, srcHeight);
    } else if (code == TL_CV_YUV2GRAY_420) {
        memcpy(dstData, srcData, srcWidth * srcHeight);
//        cvYuvToGray_420(srcData, dstData, srcWidth, srcHeight);
    } else if (code == TL_CV_YUV2GRAY_UYVY) {
        cvYuvToGray_UYVY(srcData, dstData, srcHeight,srcWidth, dstWidthStep);
    } else if (code == TL_CV_YUV2GRAY_YUYV) {
        cvYuvToGray_YUYV(srcData, dstData,srcHeight, srcWidth, dstWidthStep);
    }


    return imageGray;
}

void cvFreeImageTL(struct _IplImageTuling *image) {
    if (image != NULL) {
        free(image->imageData);
        free(image);
    }
}

// cvCopy
void cvCopyImageTL(struct _IplImageTuling *src, struct _IplImageTuling *dst) {
    int i = 0, j = 0;
    char *srcPixel;
    char *dstPixel;
    if (src->widthStep != dst->widthStep || src->height != dst->height || src->nChannels != dst->nChannels) {
        printf("error copyImage shape is different\n");
        return;
    }
    for (i = 0; i < src->height; i++) {
        srcPixel = src->imageData + i * src->widthStep;
        dstPixel = dst->imageData + i * dst->widthStep;
        for (j = 0; j < src->widthStep; j++) {
            dstPixel[j] = srcPixel[j];
        }
    }
}

void cvtColorTL(struct _IplImageTuling *src, struct _IplImageTuling *dst, TL_IMAGE_TYPE code) {

    unsigned char *srcData;
    unsigned char *dstData;
    int srcWidthStep = src->widthStep;
    int srcHeight = src->height;
    int srcChannel = src->nChannels;
    int srcWidth = src->width;
    int dstWidthStep = dst->widthStep;
    srcData = (unsigned char *) src->imageData;
    dstData = (unsigned char *) dst->imageData;

    if (code == TL_CV_RGB2GRAY) {
        cvRgbToGray(srcData, dstData, srcWidthStep, srcHeight, srcChannel,dstWidthStep);
    } else if (code == TL_CV_BGR2GRAY) {
        cvBgrToGray(srcData, dstData, srcWidthStep, srcHeight, srcChannel,dstWidthStep);
    } else if (TL_CV_GRAY2GRAY == code) {
        cvGrayToGray(srcData, dstData, srcWidthStep, srcHeight);
    } else if (code == TL_CV_YUV2GRAY_420) {
        memcpy(dstData, srcData, srcWidth * srcHeight);
//        cvYuvToGray_420(srcData, dstData, srcWidth, srcHeight);
    } else if (code == TL_CV_YUV2GRAY_UYVY) {
        cvYuvToGray_UYVY(srcData, dstData, srcHeight, srcWidth, dstWidthStep);
    } else if (code == TL_CV_YUV2GRAY_YUYV) {
        cvYuvToGray_YUYV(srcData, dstData, srcHeight, srcWidth, dstWidthStep);
    }

}

unsigned char rgbToGray(const unsigned char R, const unsigned char G, const unsigned char B) {
    unsigned char re;
    re = (unsigned char) (((int) R * 19595 + (int) G * 38469 + (int) B * 7472) >> 16);
    return re;
}

void cvBgrToGray(const unsigned char *srcData, unsigned char *dstData, int srcWidthStep, int srcHeight, int srcChannels,int dstWidthStep) {
    int i, j,k;
    unsigned char *srcPixel;
    unsigned char *dstPixel;
    for (i = 0; i < srcHeight; i++) {
        srcPixel = (unsigned char *) (srcData + i * srcWidthStep);
        dstPixel = (dstData + i * dstWidthStep);
        for (j = 0, k = 0; j < srcWidthStep; j += srcChannels, k++) {
//            dstPixel[k] = rgbToGray(srcPixel[j + 2], srcPixel[j + 1], srcPixel[j]);
            dstPixel[k] = (unsigned char) (
                    ((int) srcPixel[j + 2] * 19595 + (int) srcPixel[j + 1] * 38469 + (int) srcPixel[j] * 7472) >> 16);
        }
    }
}

void cvRgbToGray(const unsigned char *srcData, unsigned char *dstData, int srcWidthStep, int srcHeight, int srcChannels,int dstWidthStep) {
    int i, j,k;
    unsigned char *srcPixel;
    unsigned char *dstPixel;
    for (i = 0; i < srcHeight; i++) {
        srcPixel = (unsigned char *) (srcData + i * srcWidthStep);
        dstPixel = (dstData + i * dstWidthStep);
        for (j = 0, k = 0; j < srcWidthStep; j += srcChannels, k++) {
//            dstPixel[k] = rgbToGray(srcPixel[j], srcPixel[j + 1], srcPixel[j + 2]);
            dstPixel[k] = (unsigned char) (
                    ((int) srcPixel[j] * 19595 + (int) srcPixel[j + 1] * 38469 + (int) srcPixel[j + 2] * 7472) >> 16);
        }
    }
}

void cvGrayToGray(const unsigned char *srcData, unsigned char *dstData, int srcWidthStep, int srcHeight) {
    memcpy(dstData, srcData, srcWidthStep * srcHeight);
}

void cvYuvToGray_420(const unsigned char *srcData, unsigned char *dstData, int srcWidth, int srcHeight) {
    memcpy(dstData, srcData, srcWidth * srcHeight);
}

void cvYuvToGray_UYVY(const unsigned char *srcData, unsigned char *dstData,int srcHeight, int srcWidth, int dstWidthStep) {
    int i, j, k, y;
    unsigned char *srcPixel;
    unsigned char *dstPixel;
    y = srcWidth * 2;
    for (i = 0; i < srcHeight; i++) {
        srcPixel = (unsigned char *) (srcData + i * srcWidth * 2);
        dstPixel = (dstData + i * dstWidthStep);
        for (j = 0, k = 0; j < y; j += 2, k++) {
            dstPixel[k] = srcPixel[j + 1];
        }
    }
}

void cvYuvToGray_YUYV(const unsigned char *srcData, unsigned char *dstData,int srcHeight, int srcWidth, int dstWidthStep) {
    int i, j, k, y;
    unsigned char *srcPixel;
    unsigned char *dstPixel;
    y = srcWidth * 2;
    for (i = 0; i < srcHeight; i++) {
        srcPixel = (unsigned char *) (srcData + i * srcWidth * 2);
        dstPixel = (dstData + i * dstWidthStep);
        for (j = 0, k = 0; j < y; j += 2, k++) {
            dstPixel[k] = srcPixel[j];
        }
    }
}

void cvResizeTL(struct _IplImageTuling *src, struct _IplImageTuling *dst, int interpolation) {
    float fy;
    int sy;
    short cbufy[2];
    short cbufx[2];
    float fx;
    int i, j, k, sx, stepDst, stepSrc, iWidthSrc, iHeightSrc;
    unsigned char *dataDst;
    unsigned char *dataSrc;
    double xRatio;
    double yRatio;

    xRatio = (double) src->width / dst->width;
    yRatio = (double) src->height / dst->height;
    dataDst = (unsigned char *) dst->imageData;
    dataSrc = (unsigned char *) src->imageData;
    stepDst = dst->widthStep;
    stepSrc = src->widthStep;
    iWidthSrc = src->width;
    iHeightSrc = src->height;
    i = 0;
    j = 0;
    k = 0;

    for (i = 0; i < dst->height; ++i) {
//        float fy = (float) ((1000*i + 500) * yRatio - 500)/1000;
        fy = (float) ((i + 0.5) * yRatio - 0.5);
        sy = (int) fy - ((int) fy > fy);
        fy -= sy;
        sy = sy < (iHeightSrc - 2) ? sy : (iHeightSrc - 2);
        sy = 0 > sy ? 0 : sy;


        cbufy[0] = saturate_cast_short(((1.f - fy) * 2048));
        cbufy[1] = (short) (2048 - cbufy[0]);

        for (j = 0; j < dst->width; ++j) {
            fx = (float) ((j + 0.5) * xRatio - 0.5);
            sx = (int) fx - ((int) fx > fx);
            fx -= sx;

            if (sx < 0) {
                fx = 0, sx = 0;
            }

            if (sx >= iWidthSrc - 1) {
                fx = 0, sx = iWidthSrc - 2;
            }


            cbufx[0] = saturate_cast_short((1.f - fx) * 2048);
            cbufx[1] = (short) (2048 - cbufx[0]);

            for (k = 0; k < src->nChannels; ++k) {

                *(dataDst + i * stepDst + src->nChannels * j + k) =
                        (unsigned char) (
                                ((short) (*(dataSrc + sy * stepSrc + src->nChannels * sx + k)) * cbufx[0] * cbufy[0] +
                                (short) (*(dataSrc + (sy + 1) * stepSrc + src->nChannels * sx + k)) * cbufx[0] *cbufy[1] +
                                (short) (*(dataSrc + sy * stepSrc + src->nChannels * (sx + 1) + k)) * cbufx[1] *cbufy[0] +
                                (short) (*(dataSrc + (sy + 1) * stepSrc + src->nChannels * (sx + 1) + k)) * cbufx[1] *cbufy[1]) >> 22);

            }
        }
    }
}

void cvAbsDiffTL(struct _IplImageTuling *src1, struct _IplImageTuling *src2, struct _IplImageTuling *dst) {
    int i, j, k;
    unsigned char *src1Pixel;
    unsigned char *src2Pixel;
    unsigned char *dstPixel;
    if (src1 == NULL || src2 == NULL || dst == NULL) {
        printf("tl_error cvAbsDiffTL src1 or src2 or dst is NULL \n");
        return;
    }
    if (src1->widthStep != src2->widthStep || src1->height != src2->height || src1->nChannels != src2->nChannels) {
        printf("tl_error cvAbsDiffTL src1 src2 shape is different\n");
        return;
    }

    if (src1->widthStep != dst->widthStep || src1->height != dst->height || src1->nChannels != dst->nChannels) {
        printf("tl_error cvAbsDiffTL src1 dst shape is different\n");
        return;
    }

    for (i = 0; i < src1->height; i++) {
        src1Pixel = (unsigned char *) (src1->imageData + i * src1->widthStep);
        src2Pixel = (unsigned char *) (src2->imageData + i * src2->widthStep);
        dstPixel = (unsigned char *) dst->imageData + i * dst->widthStep;
        for (j = 0; j < src1->widthStep; j+=src1->nChannels) {
            for(k=0;k<src1->nChannels;k++){
                dstPixel[j+k] = (unsigned char) (abs((int) src1Pixel[j+k] - (int) src2Pixel[j+k]));
            }
        }
    }

}


void cvThresholdTL(struct _IplImageTuling *src, struct _IplImageTuling *dst, double threshold, double max_value,
                   int threshold_type) {
    int i;
    if (src == NULL || dst == NULL ) {
        printf("tl_error cvThresholdTL src or dst is NULL \n");
        return;
    }


    for(i=0;i<src->height*src->width*src->nChannels;i++){
        if( TL_CV_THRESH_BINARY == threshold_type ){
            if ((int) src->imageData[i] > (int) threshold) {
                dst->imageData[i] = (unsigned char) (255);
            } else {
                dst->imageData[i] = (unsigned char) (0);
            }
        }
    }

}

void cvErodeTL(struct _IplImageTuling *src, struct _IplImageTuling *dst, int m, int iterations) {
    int x, y, w, h;
    unsigned char *srcData;
    unsigned char *dstData;
    unsigned int t;
    int x2, y2, x3, y3;
    dstData = (unsigned char *) dst->imageData;
    srcData = (unsigned char *) src->imageData;
    w = src->width;
    h = src->height;
    while (iterations-- > 0) {
        for (y = 0; y < h; y++) {
            for (x = 0; x < w; x++) {
                y2 = y - 1;
                if (y2 < 0) y2 = h - 1;
                y3 = y + 1;
                if (y3 >= h) y3 = 0;

                x2 = x - 1;
                if (x2 < 0) x2 = w - 1;
                x3 = x + 1;
                if (x3 >= w) x3 = 0;

                t = (unsigned int) srcData[y * w + x];
                if ((unsigned int) srcData[y2 * w + x] < t) t = (unsigned int) srcData[y2 * w + x];
                if ((unsigned int) srcData[y3 * w + x] < t) t = (unsigned int) srcData[y3 * w + x];
                if ((unsigned int) srcData[y * w + x2] < t) t = (unsigned int) srcData[y * w + x2];
                if ((unsigned int) srcData[y * w + x3] < t) t = (unsigned int) srcData[y * w + x3];
                dstData[y * w + x] = (unsigned char) t;
            }
        }
    }

}

void cvCutTL(struct _IplImageTuling *src, struct _IplImageTuling *dst,int targetHeight){
    int i, j, k, index;
    index = 0;
    for (i = targetHeight; i < src->height; i++) {
        for(j=0;j<src->width;j++){
            for(k=0;k<src->nChannels;k++){
                dst->imageData[index] = src->imageData[i*src->widthStep+src->nChannels*j+k];
                index ++;
            }
        }
    }
}

void printfTL(IplImageTL *src, int type) {
    int i, j;
    unsigned char *data;
    int step;
    int channels;
    int sum_all;
    int count;
    unsigned char *pixel;
    unsigned int x_p;
    char log_info[8];
    printf("printfTL_1\n");
    data = (unsigned char *) src->imageData;
    step = src->widthStep / sizeof(unsigned char);
    channels = src->nChannels;
//	uchar b_pixel, g_pixel, r_pixel;
    printf("printfTL_2\n");
    printf("printfTL_->height:%d\t", src->height);
    printf("printfTL_->width:%d\n", src->width);
    printf("printfTL_->widthStep:%d\n", src->widthStep);
    printf("printfTL_->nChannels:%d\n", src->nChannels);
    sum_all = 0;

    for (i = 0; i < src->height; i += 1) { // height
        count = 0;
        for (j = 0; j < src->widthStep; j++) { // width
            pixel = (unsigned char *) (src->imageData + i * src->widthStep + j);
            x_p = (unsigned int) (*pixel);
            if (x_p == 255) {
                count += 1;
//                printf("%d\t", j);
            }
            if (type == 1) {
                memset(log_info, 0, 8);
                sprintf(log_info, "%d", x_p);
//                if(j==1279){
//                    printf("xxx=%s\n",log_info);
//                    int le = strlen(log_info);
//                    printf("len=%d\n",le);
//                }
                write_log_file("ttt.log", log_info, (unsigned int) strlen(log_info));
                write_log_file("ttt.log", "\n", (unsigned int)strlen("\n"));
//                sprintf(log_info, "%d", x_p);
            }
            if (j >= src->widthStep - 5) {
                printf("\nj=%d\n", j);
            }
//            printf("i=%d-%dmo\t", j,x_p);
            printf("%d\t", x_p);
//            if (j >= 60) {
//                break;
//            }

        }
        printf("count=%d\t", count);
        sum_all += count;
//        break;
    }
    printf("printf_mat_sum_all : %d\n", sum_all);
}

void write_log_file(const char *filename, const char *buffer, unsigned int buf_size) {
    FILE *fp;
    if (filename != NULL && buffer != NULL) {
        {
            fp = fopen(filename, "at+");
            if (fp != NULL) {
//                char now[32];
//                memset(now, 0, sizeof(now));
//                get_local_time(now);
//                fwrite(now, strlen(now)+1, 1, fp);
                fwrite(buffer, buf_size, 1, fp);
                fclose(fp);
                fp = NULL;
            }
        }
    }
}

