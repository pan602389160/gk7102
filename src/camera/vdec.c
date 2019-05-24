
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>

#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <basetypes.h>

#include <signal.h>
#include <pthread.h>
#include <error.h>

#include "adi_types.h"
#include "adi_sys.h"
#include "adi_vdec.h"
#include "vdec.h"
#include "shell.h"
#include "parser.h"

#define VERSION    0x00000005

GADI_SYS_HandleT vdecHandle = NULL;
extern GADI_SYS_HandleT voHandle;
extern  GADI_SYS_HandleT vencHandle;
extern  GADI_SYS_HandleT viHandle;

static GADI_SYS_ThreadHandleT    vdecThreadHandle = 0;
static GADI_U32 runVdecThread  = 0;
static void vdec_run(void *v);
#define AUDIO_RECORD_THREAD_PRIORITY    4
#define AUDIO_RECORD_THREAD_STACKSIZE   2048
#define AUDIO_RECORD_THREAD_NAME        "vdec_decode"
static GADI_U16 gWidth;
static GADI_U16 gHeight;




static const char *shortOptions = "hSPIOCEf:H:W:";
static struct option longOptions[] =
{
    {"help",     0, 0, 'h'},
    {"start",     0, 0, 'S'},
    {"stop",     0, 0, 'P'},
    {"init",     0, 0, 'I'},
    {"open",     0, 0, 'O'},
    {"close",     0, 0, 'C'},
    {"exit",     0, 0, 'E'},
    {"filename", 1, 0, 'f'},
    {"height",     1, 0, 'H'},
    {"width",     1, 0, 'W'},
    {0,          0, 0, 0}
};
/*************************************************************************
  *                                                                           *
  *     H264 frame analysis program start.                                          *
  *                                                                           *
  *************************************************************************/
static unsigned char NALHeader[] = {0x00, 0x00, 0x00, 0x01};
typedef enum {
    H264_NAL = 0,
    H264_SLICE,
    H264_SLICE_DPA,
    H264_SLICE_DPB,
    H264_SLICE_DPC,
    H264_SLICE_IDR,
    H264_SEI,
    H264_SPS,
    H264_PPS
} FrameTypeEnumT;

typedef struct {
    int size;
    FrameTypeEnumT type;
} frameInfoT;
typedef struct {
    int fd;
    int findIDR;
    int writeOffset;
    int readOffset;
    frameInfoT frameArry[1024];
    GADI_SYS_ThreadHandleT analyerThread;
    pthread_rwlock_t rwlock;
    int analyerThreadFlag;
} GADI_VDEC_H264FrameAnalyzerT,* AnalyzerHandleT;

int CreateH264FrameAnalyzer(AnalyzerHandleT *analyzer)
{
    GADI_VDEC_H264FrameAnalyzerT *analyzerObj = NULL;
    if(analyzer == NULL)
        return -1;
    analyzerObj = malloc(sizeof(GADI_VDEC_H264FrameAnalyzerT));
    if(analyzerObj == NULL)
        return -1;
    memset(analyzerObj, 0, sizeof(GADI_VDEC_H264FrameAnalyzerT));
    *analyzer = (AnalyzerHandleT)analyzerObj;
    analyzerObj->writeOffset = 0;
    analyzerObj->readOffset = 0;
    analyzerObj->fd = -1;
    pthread_rwlock_init(&(analyzerObj->rwlock), NULL);
    return 0;
}

int DestroyH264FrameAnalyzer(AnalyzerHandleT analyzer)
{
    if(analyzer == NULL)
        return -1;
    pthread_rwlock_destroy(&(analyzer->rwlock));
    free(analyzer);
    return 0;
}

int SetNextFrameSize(AnalyzerHandleT analyzer, int size, FrameTypeEnumT type)
{
    if(analyzer == NULL)
        return -1;
    if(analyzer->writeOffset >= (sizeof(analyzer->frameArry)/sizeof(analyzer->frameArry[0]))
        || analyzer->writeOffset < 0)
        analyzer->writeOffset = 0;
    while((analyzer->writeOffset + 1) == analyzer->readOffset) {
        usleep(1000);
    };
    pthread_rwlock_wrlock(&(analyzer->rwlock));
    analyzer->frameArry[analyzer->writeOffset].size = size;
    analyzer->frameArry[analyzer->writeOffset].type = type;
    analyzer->writeOffset++;
    pthread_rwlock_unlock(&(analyzer->rwlock));
    return 0;
}

frameInfoT GetNextFrameSize(AnalyzerHandleT analyzer, int millisecond)
{
    frameInfoT frameInfo;
    if(analyzer == NULL){
        frameInfo.size = -1;
        return frameInfo;
    }
    while(analyzer->readOffset == analyzer->writeOffset){
        if(--millisecond < 0){
            frameInfo.size = -1;
            return frameInfo;
        }
        usleep(1000);
    }
    if(analyzer->readOffset >= (sizeof(analyzer->frameArry)/sizeof(analyzer->frameArry[0]))
        || analyzer->readOffset < 0)
        analyzer->readOffset = 0;
    pthread_rwlock_rdlock(&(analyzer->rwlock));
    frameInfo = analyzer->frameArry[analyzer->readOffset];
    analyzer->frameArry[analyzer->readOffset].size = 0;
    analyzer->frameArry[analyzer->readOffset].type = H264_NAL;
    analyzer->readOffset++;
    pthread_rwlock_unlock(&(analyzer->rwlock));
    return frameInfo;
}

static void AnalysisH264FrameThread(void *arg)
{
    AnalyzerHandleT analyzer = (AnalyzerHandleT)arg;
    unsigned char buffer[1024*1024];
    int readSize = 0;
    int preOffset, nextOffset;
    int moreData = 0;
    FrameTypeEnumT curType;

    analyzer->analyerThreadFlag = 1;
    memset(buffer, 0, sizeof(buffer));
    while(analyzer->analyerThreadFlag) {
        preOffset = 0;
        nextOffset = preOffset + sizeof(NALHeader);
        readSize = read(analyzer->fd, buffer + moreData, sizeof(buffer) - moreData);
        if(readSize <= 0) {
            printf("file read complete.\n");
            break;
        }
        readSize += moreData;
        while(nextOffset <= (readSize - sizeof(NALHeader))) {
            while(nextOffset <= (readSize - sizeof(NALHeader)) && (buffer[nextOffset] != NALHeader[0]
                || buffer[nextOffset+1] != NALHeader[1]
                || buffer[nextOffset+2] != NALHeader[2]
                || buffer[nextOffset+3] != NALHeader[3])) {
                nextOffset++;
            }
            if(nextOffset > readSize - sizeof(NALHeader)){
                moreData = readSize - preOffset;
                memmove(buffer, &(buffer[preOffset]), moreData);
                continue;
            } else {//set normal frame size
                curType = (buffer[preOffset + sizeof(NALHeader)]&0x1f);
                SetNextFrameSize(analyzer, nextOffset - preOffset, curType);
                preOffset = nextOffset;
                nextOffset = preOffset + sizeof(NALHeader);
            }
        }
    }
}

int StartAnalysisH264Frame(AnalyzerHandleT analyzer, const char *h264File)
{
    analyzer->fd = open(h264File, O_RDONLY);
    if(analyzer->fd < 0)
        return -1;
    return gadi_sys_thread_create(AnalysisH264FrameThread,(void *)analyzer, GADI_SYS_THREAD_PRIO_DEFAULT,
        GADI_SYS_THREAD_STATCK_SIZE_DEFAULT, "AnalysisH264", &(analyzer->analyerThread));
}

int StopAnalysisH264Frame(AnalyzerHandleT analyzer)
{
    if(analyzer == NULL)
        return -1;
    if(analyzer->analyerThreadFlag) {
        analyzer->analyerThreadFlag = 0;
        gadi_sys_wait_end_thread(analyzer->analyerThread);
        analyzer->analyerThread = 0;
    }
    if(analyzer->fd >= 0) {
        close(analyzer->fd);
        analyzer->fd = -1;
    }
    return 0;
}

/*************************************************************************
  *                                                                           *
  *     H264 frame analysis program  end.                                           *
  *                                                                           *
  *************************************************************************/

void vdec_init(void)
{
   GADI_ERR retVal;
   retVal =  gadi_vdec_init();
   if(retVal < 0){
        GADI_ERROR("gadi_vdec_init fail");
        return;
   }
   GADI_INFO("gadi_vdec_init ok\n");
}

void vdec_exit(void)
{
   GADI_ERR retVal;
   retVal =  gadi_vdec_exit();
   if(retVal < 0){
        GADI_ERROR("gadi_vdec_exit fail");
        return;
   }
   GADI_INFO("gadi_vdec_exit ok\n");
}

void vdec_open(void)
{
    GADI_ERR errVal;
    if(viHandle)
        gadi_vi_enable(viHandle, 0);
    vdecHandle = gadi_vdec_open(&errVal);
    if(errVal < 0 || vdecHandle == NULL){
        GADI_ERROR("gadi_vdec_open fail");
        return;
    }
    GADI_INFO("gadi_vdec_open ok\n");
}

void vdec_close(void)
{
    GADI_ERR retVal;
    retVal = gadi_vdec_close(vdecHandle);
    if(retVal < 0){
        GADI_ERROR("gadi_vdec_close fail");
        return;
    }
    GADI_INFO("gadi_vdec_close ok");
}

static GADI_U8 map_flag = 0;
GADI_VDEC_DecBufT MapInfo;

static void vdec_map_buffer(void)
{
    GADI_ERR retVal;
    if(map_flag == 1){
        GADI_INFO("vdec_map_buffer already init\n");
        return;
    }

    retVal = gadi_vdec_map_buf(vdecHandle, &MapInfo);
    if(retVal < 0){
        GADI_ERROR("vdec_map_buffer fail");
        return ;
    }
	map_flag = 1;
    //GADI_INFO("vdec_map_buffer ok\n");
    // GADI_INFO("# bsbAddr:%p bsbSize:%d\n", MapInfo.bsbAddr, MapInfo.bsbSize);
}

static void vdec_unmap_buffer(void)
{
    GADI_ERR retVal;
    if(map_flag == 0){
        GADI_INFO("vdec_unmap_buffer already init\n");
        return;
    }
    retVal = gadi_vdec_unmap_buf(vdecHandle);
    if(retVal < 0){
        GADI_ERROR("vdec_unmap_buffer fail");
        return ;
    }
    map_flag = 0;
    GADI_INFO("vdec_unmap_buffer ok\n");
}


void vdec_start(char *filename, GADI_U16 width, GADI_U16 height)
{
    GADI_ERR retVal = GADI_OK;
    GADI_VOUT_SettingParamsT voParams;

    vdec_map_buffer();
	sleep(1);

    voParams.voutChannel = GADI_VOUT_B;
    retVal = gadi_vout_get_params(voHandle, &voParams);
    printf("dev:%d mod:%d\n", voParams.deviceType, voParams.resoluMode);
    sleep(1);

    retVal = gadi_vdec_select_channel(vdecHandle, 0);
    if(retVal < 0){
        GADI_ERROR("gadi_vdec_select_channel fail");
        return ;
    }
    GADI_INFO("gadi_vdec_select_channel ok\n");
    sleep(1);

    retVal = gadi_vdec_start(vdecHandle);
    if(retVal < 0){
        GADI_ERROR("gadi_vdec_start fail");
        return ;
    }
    gWidth = width;
    gHeight = height;
    runVdecThread = 1;
    gadi_sys_thread_create(vdec_run, (void *)filename,
        AUDIO_RECORD_THREAD_PRIORITY,
        AUDIO_RECORD_THREAD_STACKSIZE,
        AUDIO_RECORD_THREAD_NAME,
        &vdecThreadHandle);
}

void vdec_stop(void)
{
    GADI_ERR retVal = GADI_OK;
    retVal = gadi_vdec_stop(vdecHandle);
    if(retVal < 0){
        GADI_ERROR("gadi_vdec_stop fail");
        return ;
    }
    GADI_INFO("gadi_vdec_stop ok");
    runVdecThread = 0;
    vdec_unmap_buffer();
}
static void vdec_run(void *filename)
{
    GADI_ERR retVal = GADI_OK;
    static u8 eos[] = {0x00, 0x00, 0x00, 0x01, 0x0A};
    GADI_VDEC_WaitInfoT waitInfo;
    GADI_VDEC_FeedH264InfoT  streamInfo;
    GADI_VDEC_ConfigInfoT configInfo;
    AnalyzerHandleT analyzerHandle = NULL;
    int fd = open(filename, O_RDONLY);
    if(fd < 0){
        printf("open filename:%s fail\n", (char *)filename);
        return;
    }
    printf("open filename:%s \n", (char *)filename);

    if(gWidth != 0 && gHeight != 0) {
        configInfo.picWidth = gWidth;
        configInfo.picHeight = gHeight;
    } else {
        configInfo.picWidth = 1920;
        configInfo.picHeight = 1080;
    }
    printf("set video width:%4d height:%4d\n", configInfo.picWidth, configInfo.picHeight);
    retVal = gadi_vdec_config(vdecHandle, &configInfo);
    if(retVal < 0){
        GADI_ERROR("gadi_vdec_config fail");
        return;
    }
    unsigned char *pStr = MapInfo.bsbAddr;
    unsigned char *pEnd = MapInfo.bsbAddr;
    int i = 1;
    CreateH264FrameAnalyzer(&analyzerHandle);
    StartAnalysisH264Frame(analyzerHandle, filename);
    frameInfoT frameSize;
    long allSize = 0;
    int fileoffset = 0;
    /* ²»ÊÇIÖ¡¾Í¶ªÆú*/
    do{
        frameSize = GetNextFrameSize(analyzerHandle, 300);
        if(frameSize.size < 0) {
            printf("get one frame timeout.\n");
            goto exit;
        }
        if(frameSize.type < H264_SLICE_IDR) {
            fileoffset += frameSize.size;
        }
    }while(frameSize.type < H264_SLICE_IDR);
    printf("put away data %08x\n", fileoffset);
    lseek(fd, fileoffset, SEEK_SET);
    int first = 1;
    while(runVdecThread){
        allSize += frameSize.size;
        printf("===>No.%d size:%4d allsize:%08lx", i++, frameSize.size, allSize);
        pStr = pEnd;
        fflush(stdout);
        while(1){
            waitInfo.addr = pStr;
            waitInfo.flag = GADI_VDEC_WAIT_BSB;
            waitInfo.size = frameSize.size;
            retVal = gadi_vdec_wait(vdecHandle, &waitInfo);
            if(retVal < 0){
                if (errno != EAGAIN)
                {
                    perror("GK_MEDIA_IOC_WAIT_DECODER");
                    goto exit;
                }
                   break;
               }
            if(waitInfo.flag == GADI_VDEC_WAIT_BSB)
                break;
        }
        printf(", OK!\n");

        if (pStr + frameSize.size <= MapInfo.bsbAddr + MapInfo.bsbSize)
        {
            retVal = read(fd, pStr, frameSize.size);
            if (retVal < frameSize.size)
                goto exit;
            pEnd += frameSize.size;
        }
        else
        {
            GADI_U8 size = (MapInfo.bsbAddr + MapInfo.bsbSize) - pStr;
            if (read(fd, pStr, size) < size)
                goto exit;
            size = frameSize.size - size;
            if (read(fd, MapInfo.bsbAddr, size) < size)
                goto exit;
            pEnd = MapInfo.bsbAddr + size;
        }
        if(first){
            printf("%02x %02x %02x %02x %02x \n",
                pStr[0],pStr[1],pStr[2],pStr[3],pStr[4]);
            first = 0;
        }
        //gadi_vdec_get_dec_info(vdecHandle, &decInfo);
        //printf("== frame NO. : %d, PTS : %08d HPTS:%08d .\n",
        //decInfo.decodedFrames, decInfo.currPts, decInfo.currPtsHigh);
        streamInfo.startAddr = pStr;
        streamInfo.endAddr = pEnd;
        streamInfo.firstPTS = 0;
        streamInfo.nextSize = 0;
        streamInfo.picHeight = 0;
        streamInfo.picWidth = 0;
        streamInfo.picNums = 1;
        retVal = gadi_vdec_feed_h264(vdecHandle, &streamInfo);
        if(retVal < 0){
            GADI_ERROR("gadi_vdec_feed_h264 fail\n");
            goto exit;
        }
        //gadi_vdec_get_frame(vdecHandle, &decFrame);
        //GADI_INFO("uv: %d,y:%d,picnum:%d\n",
        //    decFrame.yuv422_uv_addr, decFrame.yuv422_y_addr, decFrame.pic_num);
        /* get next frame info */
        frameSize = GetNextFrameSize(analyzerHandle, 300);
        if(frameSize.size < 0) {
            printf("get one frame timeout.\n");
            goto exit;
        }
    }
exit:
    StopAnalysisH264Frame(analyzerHandle);
    DestroyH264FrameAnalyzer(analyzerHandle);
    memcpy(pStr, eos, sizeof(eos));
    pEnd += sizeof(eos);
    streamInfo.startAddr = pStr;
    streamInfo.endAddr = pEnd;
    streamInfo.firstPTS = 0;
    streamInfo.nextSize = 0;
    streamInfo.picHeight = 0;
    streamInfo.picWidth = 0;
    streamInfo.picNums = 1;
    gadi_vdec_feed_h264(vdecHandle, &streamInfo);
    free(filename);
    close(fd);
    vdec_unmap_buffer();
    GADI_INFO("exit vdec_run\n");
    return;
}
static void Usage(void)
{
    printf("Usage: vdec [option]\n");
    printf("    -h help.\n");
    printf("    -I init video decode\n");
    printf("    -E exit video decode\n");
    printf("    -O open video decode\n");
    printf("    -C close video decode\n");
    printf("    -S start video decode\n");
    printf("    -P stop video decode\n");
    printf("    -H set video height\n");
    printf("    -W set video width\n");
    printf("    -f set decoder video file\n");
    printf("Example:\n");
    printf("    #vdec -IO\n");
    printf("    #vdec -S -f /h264/dec_video.h264 -W 1920 -H 1080\n");
}

static GADI_ERR handle_vdec_command(int argc, char* argv[]);

GADI_ERR vdec_register_testcase(void)
{
    GADI_ERR   retVal =  GADI_OK;
    (void)shell_registercommand (
        "vdec",
        handle_vdec_command,
        "vdec command",
        "---------------------------------------------------------------------\n"
        "vdec -I \n"
        "   brief : init video decode handle\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "vdec -E \n"
        "   brief : exit video decode\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "vdec -O\n"
        "   brief : open video decode handle and configure\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "vdec -C\n"
        "   brief : close video decode, clear configure\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "vdec -S \n"
        "   brief : start decode the specified stream.\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "vdec -P \n"
        "   brief : stop decode the specified stream.\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "vdec -H \n"
        "   brief : set video height\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "vdec -W \n"
        "   brief : set video width\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "vdec -f \n"
        "   brief : decoder video file\n"
        "\n"
        /******************************************************************/
    );

    return retVal;
}

static GADI_ERR handle_vdec_command(int argc, char* argv[])
{
    static int start_flag = 0;
    int option_index, ch;
    char *filename =  NULL;
    int width = 0, height = 0;

    /*change parameters when giving input options.*/
    optind = 1;
    while (1)
    {
        option_index = 0;
        ch = getopt_long(argc, argv, shortOptions, longOptions, &option_index);
        if (ch == -1)
            break;

        switch (ch)
        {
            case 'h':
            case '?':
                Usage();
            break;
            case 'I':
                vdec_init();
            break;
            case 'E':
                vdec_exit();
            break;
            case 'O':
                vdec_open();
            break;
            case 'C':
                vdec_close();
            break;

            case 'S':
                start_flag = 1;
            break;

            case 'f':
            if(start_flag){
                if(filename){
                    free(filename);
                    filename = NULL;
                }
                filename = malloc(128);
                if(filename != NULL){
                    memset(filename, 0, 128);
                    strncpy(filename, optarg, 128);
                }
            }
            break;
            case 'W':
                {
                    width = atoi(optarg);
                }
                break;
            case 'H':
                {
                    height = atoi(optarg);
                }
                break;
            case 'P':
                vdec_stop();
                break;

            default:
                GADI_ERROR("bad params\n");
                break;
        }
    }
    if(start_flag && filename != NULL){
        vdec_start(filename,width,height);
        filename = NULL;//free filename space into vdec_run().
        start_flag = 0;
    }
    optind = 1;
    return 0;
}

