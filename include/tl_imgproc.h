//
// Created by turing on 2019-02-21.
//

#ifndef TURINGIMAGE_TL_IMGPROC_H
#define TURINGIMAGE_TL_IMGPROC_H

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus

typedef struct _IplImageTuling {
    int nSize;             /**< sizeof(IplImage) */
    int nChannels;         /**< Most of OpenCV functions support 1,2,3 or 4 channels */
    int depth;             /**< Pixel depth in bits: IPL_DEPTH_8U, IPL_DEPTH_8S, IPL_DEPTH_16S,
                               IPL_DEPTH_32S, IPL_DEPTH_32F and IPL_DEPTH_64F are supported.  */
    int width;             /**< Image width in pixels.                           */
    int height;            /**< Image height in pixels.                          */
    int imageSize;         /**< Image data size in bytes
                               (==image->height*image->widthStep
                               in case of interleaved data)*/
    char *imageData;        /**< Pointer to aligned image data.         */
    int widthStep;         /**< Size of aligned image row in bytes.    */
} IplImageTL;


enum {
    TL_CV_GRAY = 0,
    TL_CV_RGB = 1,
    TL_CV_BGR = 2,
    //420
    TL_CV_YUV_NV21 = 10,
    TL_CV_YUV_NV12 = 11,
    TL_CV_YUV_YV12 = 12,
    TL_CV_YUV_IYUV = 13,
    TL_CV_YUV_I420 = 14,
    // 422
    TL_CV_YUV_UYVY = 20,
    TL_CV_YUV_YUY2 = 21,
    TL_CV_YUV_Y422 = 22,
    TL_CV_YUV_UYNV = 23,
    TL_CV_YUV_YVYU = 24,
    TL_CV_YUV_YUYV = 25,
    TL_CV_YUV_YUNV = 26
};

typedef enum _TL_IMAGE_TYPE_ {

    TL_CV_GRAY2GRAY = 0,
//    TL_CV_BGR2RGB = 4,
    TL_CV_BGR2GRAY = 6,
    TL_CV_RGB2GRAY = 7,
//    TL_CV_RGB2BGR = TL_CV_BGR2RGB,
    TL_CV_YUV2GRAY_420 = 106,
    TL_CV_YUV2GRAY_NV21 = TL_CV_YUV2GRAY_420,
    TL_CV_YUV2GRAY_NV12 = TL_CV_YUV2GRAY_420,
    TL_CV_YUV2GRAY_YV12 = TL_CV_YUV2GRAY_420,
    TL_CV_YUV2GRAY_IYUV = TL_CV_YUV2GRAY_420,
    TL_CV_YUV2GRAY_I420 = TL_CV_YUV2GRAY_420,
    TL_CV_YUV2GRAY_Y422P = TL_CV_YUV2GRAY_I420,

    TL_CV_YUV2GRAY_UYVY = 123,
    TL_CV_YUV2GRAY_YUY2 = 124,
    TL_CV_YUV2GRAY_Y422 = TL_CV_YUV2GRAY_UYVY,
    TL_CV_YUV2GRAY_UYNV = TL_CV_YUV2GRAY_UYVY,
    TL_CV_YUV2GRAY_YVYU = TL_CV_YUV2GRAY_YUY2,
    TL_CV_YUV2GRAY_YUYV = TL_CV_YUV2GRAY_YUY2,
    TL_CV_YUV2GRAY_YUNV = TL_CV_YUV2GRAY_YUY2

}TL_IMAGE_TYPE;

enum {
    TL_CV_INTER_NN = 0,
    TL_CV_INTER_LINEAR = 1,
    TL_CV_INTER_CUBIC = 2,
    TL_CV_INTER_AREA = 3,
    TL_CV_INTER_LANCZOS4 = 4
};

/** Threshold types */
enum {
    TL_CV_THRESH_BINARY = 0,  /**< value = value > threshold ? max_value : 0       */
    TL_CV_THRESH_BINARY_INV = 1,  /**< value = value > threshold ? 0 : max_value       */
    TL_CV_THRESH_TRUNC = 2,  /**< value = value > threshold ? threshold : value   */
    TL_CV_THRESH_TOZERO = 3,  /**< value = value > threshold ? value : 0           */
    TL_CV_THRESH_TOZERO_INV = 4,  /**< value = value > threshold ? 0 : value           */
    TL_CV_THRESH_MASK = 7,
    TL_CV_THRESH_OTSU = 8, /**< use Otsu algorithm to choose the optimal threshold value;
                                 combine the flag with one of the above CV_THRESH_* values */
    TL_CV_THRESH_TRIANGLE = 16  /**< use Triangle algorithm to choose the optimal threshold value;
                                 combine the flag with one of the above CV_THRESH_* values, but not
                                 with CV_THRESH_OTSU */
};


IplImageTL *cvCreateImageTL(int width, int height, int nChannels,TL_IMAGE_TYPE imageType);
IplImageTL *cvCreateImageTLTest(int width, int height, int nChannels, TL_IMAGE_TYPE imageType);
IplImageTL *cvCreateImageTLByUChar(const unsigned char *src, int width, int height, int nChannels,TL_IMAGE_TYPE imageType);
IplImageTL *cvCreateImageGRAYTLByUChar(const unsigned char *srcData, int srcWidth, int srcHeight, int srcChannel, TL_IMAGE_TYPE code);

void cvFreeImageTL(struct _IplImageTuling *imageTuling);

// cvCopy
void cvCopyImageTL(struct _IplImageTuling *src, struct _IplImageTuling *dst);

// cvResizeTL
void cvResizeTL(struct _IplImageTuling *src, struct _IplImageTuling *dst, int interpolation);

void cvtColorTL(struct _IplImageTuling *src, struct _IplImageTuling *dst, TL_IMAGE_TYPE code);

void cvAbsDiffTL(struct _IplImageTuling *src1, struct _IplImageTuling *src2, struct _IplImageTuling *dst);

void cvThresholdTL(struct _IplImageTuling *src, struct _IplImageTuling *dst, double threshold, double max_value,
                   int threshold_type);

void cvErodeTL(struct _IplImageTuling *src, struct _IplImageTuling *dst, int x, int iterations);
void cvCutTL(struct _IplImageTuling *src, struct _IplImageTuling *dst,int targetHeight);

unsigned char rgbToGray(const unsigned char R, const unsigned char G, const unsigned char B);

void cvGrayToGray(const unsigned char *srcData, unsigned char *dstData, int srcWidthStep, int srcHeight);
void cvYuvToGray_420(const unsigned char * srcData,unsigned char * dstData,int srcWidth,int srcHeight);

void cvRgbToGray(const unsigned char *srcData, unsigned char *dstData, int srcWidthStep, int srcHeight, int srcChannels,int dstWidthStep);
void cvBgrToGray(const unsigned char *srcData, unsigned char *dstData, int srcWidthStep, int srcHeight, int srcChannels,int dstWidthStep);

void cvYuvToGray_UYVY(const unsigned char *srcData, unsigned char *dstData,int srcHeight, int srcWidth, int dstWidthStep);
void cvYuvToGray_YUYV(const unsigned char *srcData, unsigned char *dstData,int srcHeight, int srcWidth, int dstWidthStep);



void write_log_file(const char *filename, const char *buffer, unsigned int buf_size);


#ifdef __cplusplus
}
#endif

void printfTL(IplImageTL *src, int type);
#endif //TURINGIMAGE_TL_IMGPROC_H
