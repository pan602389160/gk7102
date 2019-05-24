/******************************************************************************
** \file        adi/test/src/audio.c
**
** \brief       ADI layer audio(record/play) test.
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**              ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**              OMMISSIONS
**
** (C) Copyright 2013-2014 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#include <getopt.h>


#include "i2s.h"
#include "shell.h"

//*****************************************************************************
//*****************************************************************************
//** Local Defines
//*****************************************************************************
//*****************************************************************************
#define I2S_THREAD_PRIORITY  3
#define I2S_THREAD_STACKSIZE 2048
#define I2S_THREAD_NAME      "i2s"

#define DEFAULT_RECORD_STREAM_FILE       "record_i2s.dat"
#define DEFAULT_PLAYBACK_STREAM_FILE       "playback_i2s.dat"

#define I2S_PLAYBACK_THREAD_PRIORITY  3
#define I2S_PLAYBACK_THREAD_STACKSIZE 2048
#define I2S_PLAYBACK_THREAD_NAME      "i2s_playback"

#define I2S_RECORD_THREAD_PRIORITY  3
#define I2S_RECORD_THREAD_STACKSIZE 2048
#define I2S_RECORD_THREAD_NAME      "i2s_record"

#define I2S_LOOPBACK_THREAD_PRIORITY    4
#define I2S_LOOPBACK_THREAD_STACKSIZE   2048
#define I2S_LOOPBACK_THREAD_NAME        "i2s_loopback"

//*****************************************************************************
//*****************************************************************************
//** Local structures
//*****************************************************************************
//*****************************************************************************



//*****************************************************************************
//*****************************************************************************
//** Global Data
//*****************************************************************************
//*****************************************************************************


//*****************************************************************************
//*****************************************************************************
//** Local Data
//*****************************************************************************
//*****************************************************************************
static int i2sRxFd = 0;
static int i2sTxFd = 0;

static const char *shortOptions = "hPRLS:m:";
static struct option longOptions[] =
{
    {"help",        0, 0, 'h'},
    {"playback",    0, 0, 'P'},
    {"record",      0, 0, 'R'},
    {"loopback",    0, 0, 'L'},
    {"stop",        1, 0, 'S'},
    {"mute/umute",  0, 0, 'm'},
    {0,             0, 0, 0}
};

char record_file_name[256];
char playback_file_name[256];


static GADI_SYS_ThreadHandleT    i2sPlaybackThreadHandle = 0;
static GADI_SYS_ThreadHandleT    i2sRecordThreadHandle   = 0;
static GADI_SYS_ThreadHandleT    i2sLoopbackThreadHandle   = 0;


static int i2sPlaybackRunning = 0;
static int i2sRecordRunning   = 0;
static int i2sLoopbackRunning  = 0;

//*****************************************************************************
//*****************************************************************************
//** Local Functions Declaration
//*****************************************************************************
//*****************************************************************************
static GADI_ERR handle_i2s_command(int argc, char* argv[]);


//*****************************************************************************
//*****************************************************************************
//** API Functions
//*****************************************************************************
//*****************************************************************************
int gdm_i2s_init(void)
{
    GADI_I2S_devAttrT i2sAttr;
    GADI_I2S_FrameChnParamT channelParam;
    GADI_ERR retVal;
    
    i2sAttr.i2sMode   = GADI_I2S_MODE_I2S;
    i2sAttr.workMode = GADI_I2S_WORK_MODE_MASTER;
    i2sAttr.channel= GADI_I2S_CHANNEL_COUNT_2;
    i2sAttr.wlen   = GADI_I2S_WORD_LENGTH_16BIT;
    i2sAttr.speed  = GADI_I2S_SPEED_8000;

    retVal = gadi_i2s_init();
    if(retVal != GADI_OK) {
        return retVal;
    }
    
    retVal = gadi_i2s_set_attr(&i2sAttr);
    if(retVal != GADI_OK) {
        return retVal;
    }
    
    i2sRxFd = gadi_i2s_rx_get_fd();
    if(i2sRxFd <= 0) {
        return -1;
    }
    i2sTxFd = gadi_i2s_tx_get_fd();
    if(i2sTxFd <= 0) {
        return -1;
    }

    channelParam.frameNum = 30;
    channelParam.frameSize = 1024;

    retVal = gadi_i2s_dev_set_frame_channel_param(i2sRxFd,&channelParam);
    if(retVal != GADI_OK) {
        return retVal;
    }

    channelParam.frameNum = 30;
    channelParam.frameSize = 1024;
    
    retVal = gadi_i2s_dev_set_frame_channel_param(i2sTxFd,&channelParam);
    if(retVal != GADI_OK) {
        return retVal;
    }
    

    strcpy(record_file_name, DEFAULT_RECORD_STREAM_FILE);
    strcpy(playback_file_name, DEFAULT_PLAYBACK_STREAM_FILE);
    
    return GADI_OK;    
}

int gdm_i2s_exit(void)
{
    gadi_i2s_exit();
    GADI_INFO("i2s_exit ok\n");
    return GADI_OK;
}

int gdm_i2s_register_testcase(void)
{
    int   retVal = 0;
    (void)shell_registercommand (
        "i2s",
        handle_i2s_command,
        "i2s command",
        "---------------------------------------------------------------------\n"
        "i2s -R file_name\n"
        "   brief : start record i2s stream.\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "i2s -L\n"
        "   brief : Loopback(rx to tx)  i2s stream.\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "i2s -P file_name\n"
        "   brief : start play i2s stream.\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "i2s -S\n"
        "   brief : stop record i2s stream.\n"
        "\n"

    );

    return retVal;
}

static void i2s_record_thread(void *opt)
{
    const GADI_CHAR *name = record_file_name;
    GADI_S32 fd = -1;
    GADI_I2S_FrameT      rx_frame;
    GADI_BOOL block = GADI_TRUE;
    GADI_ERR retVal;

    /* open a new file to write */
    remove(name);
    if ((fd = open(name, O_WRONLY | O_CREAT, 0644)) == -1)
    {
        GADI_ERROR("open %s failed.\n", name);
        goto exit;
    }
       
    retVal = gadi_i2s_dev_enable(i2sRxFd);
    if(retVal != GADI_OK) {
        goto exit1;
    }
    
    while(i2sRecordRunning)
    {
        gadi_i2s_get_frame(&rx_frame,block);
        if (write(fd, rx_frame.data_addr, rx_frame.data_length) != rx_frame.data_length)
        {
           GADI_ERROR("audio_record: write file error, len=%d\n",rx_frame.data_length);
        }
        /*non-blocking (flag is GADI_FALSE), please sleep here*/
        //gadi_sys_thread_sleep(1);
    }

    gadi_i2s_dev_disable(i2sRxFd);

exit1:
    close(fd);
exit:
    i2sRecordRunning = 0;
    
    return;
}

static void i2s_playback_thread(void *opt)
{
    const GADI_CHAR *name = playback_file_name;
    GADI_I2S_FrameChnParamT channelParam;
    GADI_S32 fd = -1;
    GADI_I2S_FrameT      tx_frame;
    GADI_BOOL block = GADI_TRUE;
    GADI_U32  count  = 0;
    GADI_ERR retVal;

    if ((fd = open(name, O_RDONLY, 0)) == -1)
    {
        GADI_ERROR("open %s failed.\n",name);
        goto exit;
    }
    
    retVal = gadi_i2s_dev_enable(i2sTxFd);
    if(retVal != GADI_OK) {
        goto exit1;
    }

    gadi_i2s_dev_get_frame_channel_param(i2sTxFd,&channelParam);

    tx_frame.data_length = channelParam.frameSize;
    tx_frame.data_addr = (GADI_CHAR*)gadi_sys_malloc(tx_frame.data_length);

    /* playback */
    while (i2sPlaybackRunning )
    {
        if(!retVal) {
            count = read(fd, tx_frame.data_addr, tx_frame.data_length);
        }
        if (count == 0) {
            lseek(fd, 0, SEEK_SET);
        }
        if(count == tx_frame.data_length) {
            retVal = gadi_i2s_send_frame(&tx_frame,block);
        }
        /*non-blocking (flag is GADI_FALSE), please sleep here*/
        //gadi_sys_thread_sleep(1);
    }
    
    free(tx_frame.data_addr);

    gadi_i2s_dev_disable(i2sTxFd);  
exit1:
    close(fd);
exit:
    i2sPlaybackRunning = 0;
    return;

}

static void i2s_loopback_thread(void *opt)
{
    GADI_I2S_FrameT      rx_frame;
    GADI_I2S_FrameT      tx_frame;
    GADI_BOOL block = GADI_TRUE;
    GADI_ERR retVal;

    
    retVal = gadi_i2s_dev_enable(i2sRxFd);
    if(retVal != GADI_OK) {
        goto exit;
    }
    
    retVal = gadi_i2s_dev_enable(i2sTxFd);
    if(retVal != GADI_OK) {
        goto exit1;
    }
    
    while(i2sLoopbackRunning)
    {
        gadi_i2s_get_frame(&rx_frame,block);
        gadi_i2s_send_frame(&tx_frame,block);
        memcpy(tx_frame.data_addr, rx_frame.data_addr, rx_frame.data_length);
    }
    
    gadi_i2s_dev_disable(i2sTxFd); 
    
exit1:
    gadi_i2s_dev_disable(i2sRxFd);  

exit:
    i2sLoopbackRunning = 0;

    return;

}
//*****************************************************************************
//*****************************************************************************
//** Local Functions
//*****************************************************************************
//*****************************************************************************
static void usage(void)
{
    printf("\nusage: i2s [OPTION]...[FILE]...\n");
    printf("\t-h, --help            help.\n"
           "\t-P, --playback        playback file.\n"
           "\t-L, --loopback        loopback(rx to tx).\n"
           "\t-R, --record          record file.\n"
           "\t-S, --stop            stop.\n"  
           "\t-m, --mute           mute\\unmute.\n"
           );
    printf("\n");
}

static GADI_ERR handle_i2s_command(int argc, char* argv[])
{
    int option_index, ch;
    char *file_name = record_file_name;
    GADI_I2S_StreamDirectEnumT stream_direct = GADI_I2S_STREAM_CAPTURE;
    int retVal;


    /*change parameters when giving input options.*/
    while ((ch = getopt_long(argc, argv, shortOptions, longOptions, &option_index)) != -1)
    {
        switch (ch)
        {
            case 'h':
                usage();
                goto command_exit;
                break;

            case 'R':
                file_name = record_file_name;
                stream_direct = GADI_I2S_STREAM_CAPTURE;
                break;
                
            case 'P':
                file_name = playback_file_name;
                stream_direct = GADI_I2S_STREAM_PLAYBACK;
                break;                
            case 'L':
                stream_direct = GADI_I2S_STREAM_LOOPBACK;
                break; 
 
            case 'S':
                if(strcmp(optarg, "play") == 0)
                {
                    if(i2sPlaybackThreadHandle != 0)
                    {
                        i2sPlaybackRunning = 0;
                        retVal = gadi_sys_wait_end_thread(i2sPlaybackThreadHandle);
                        if (retVal != GADI_OK)
                        {
                            printf("i2s: destroy playback thread failed %d\n", retVal);
                            goto command_exit;
                        }
                        printf("i2s: stop playback successfully!!!\n");
                        i2sPlaybackThreadHandle = 0;
                    }
                    goto command_exit;
                }
                else if(strcmp(optarg, "record") == 0)
                {
                    if(i2sRecordThreadHandle != 0)
                    {
                        i2sRecordRunning = 0;
                        retVal = gadi_sys_wait_end_thread(i2sRecordThreadHandle);
                        if (retVal != GADI_OK)
                        {
                            printf("i2s: destroy record thread failed %d\n", retVal);
                            goto command_exit;
                        }
                        printf("i2s: stop record successfully!!!\n");
                        i2sRecordThreadHandle = 0;
                    }
                    goto command_exit;
                }
                else if(strcmp(optarg, "loopback") == 0)
                {
                    if(i2sLoopbackThreadHandle != 0)
                    {
                        i2sLoopbackRunning = 0;
                        retVal = gadi_sys_wait_end_thread(i2sLoopbackThreadHandle);
                        if (retVal != GADI_OK)
                        {
                            printf("i2s: destroy record thread failed %d\n", retVal);
                            goto command_exit;
                        }
                        printf("i2s: stop loop successfully!!!\n");
                        i2sLoopbackThreadHandle = 0;
                    }
                    goto command_exit;
                }
                else
                {
                    printf("please intput what to stop -S (record/play/loopback)?\n");
                    goto command_exit;
                }
                break;


            case 'm':
 
                break;  
           
            default:
                printf("type '--help' for more usage.\n");
                goto command_exit;
        }
    }
    
    if(optind <= argc -1) {
        strcpy(file_name, argv[optind]);
        GADI_INFO("i2s test: file name=%s\n",file_name);
    }

    if(stream_direct == GADI_I2S_STREAM_PLAYBACK)
    {
        if (i2sPlaybackRunning == 0)
        {
            i2sPlaybackRunning = 1;
            gadi_sys_thread_create(i2s_playback_thread,
                                   NULL,
                                   I2S_PLAYBACK_THREAD_PRIORITY,
                                   I2S_PLAYBACK_THREAD_STACKSIZE,
                                   I2S_PLAYBACK_THREAD_NAME,
                                   &i2sPlaybackThreadHandle);
        }
    }
    else if(stream_direct == GADI_I2S_STREAM_CAPTURE)
    {
        if(i2sRecordRunning == 0)
        {
            i2sRecordRunning = 1;
            gadi_sys_thread_create(i2s_record_thread,
                                   NULL,
                                   I2S_RECORD_THREAD_PRIORITY,
                                   I2S_RECORD_THREAD_STACKSIZE,
                                   I2S_RECORD_THREAD_NAME,
                                   &i2sRecordThreadHandle);
        }

    }
    else if(stream_direct == GADI_I2S_STREAM_LOOPBACK)
    {
        if(i2sLoopbackRunning == 0)
        {
            i2sLoopbackRunning = 1;
            gadi_sys_thread_create(i2s_loopback_thread,
                                   NULL,
                                   I2S_LOOPBACK_THREAD_PRIORITY,
                                   I2S_LOOPBACK_THREAD_STACKSIZE,
                                   I2S_LOOPBACK_THREAD_NAME,
                                   &i2sLoopbackThreadHandle);
        }
    }


command_exit:
    optind = 1;
    return 0;
}

