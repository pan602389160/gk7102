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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>

#include <getopt.h>

#include "audio.h"
#include "shell.h"
#ifdef AUDIO_ECHO_CANCELLATION_SUPPORT
#include "ap.h"
#endif

//*****************************************************************************
//*****************************************************************************
//** Local Defines
//*****************************************************************************
//*****************************************************************************
#define DEFAULT_CHANNEL_NUMBER              1
#define DEFAULT_RECORD_STREAM_FILE          "record_audio.dat"
#define DEFAULT_PLAYBACK_STREAM_FILE        "playback_audio.dat"

#define AUDIO_PLAYBACK_THREAD_PRIORITY      3
#define AUDIO_PLAYBACK_THREAD_STACKSIZE     2048
#define AUDIO_PLAYBACK_THREAD_NAME          "audio_playback"

#define AUDIO_RECORD_THREAD_PRIORITY        4
#define AUDIO_RECORD_THREAD_STACKSIZE       2048
#define AUDIO_RECORD_THREAD_NAME            "audio_record"

#define AUDIO_LOOPBACK_THREAD_PRIORITY      4
#define AUDIO_LOOPBACK_THREAD_STACKSIZE     2048
#define AUDIO_LOOPBACK_THREAD_NAME          "audio_loopback"

#define PRINT_RECORD_INFO printf("\nRecord Info:\n"); \
printf("Bit    Width   --->  %d\n",(int)aio_attr.bitWidth); \
printf("Sound  Mode    --->  %d(0:LEFT 1:RIGHT 2:STEREO 3:MONO 4:SINGLE)\n",(int)aio_attr.soundMode); \
printf("Sample Rate    --->  %d\n",(int)aio_attr.sampleRate); \
printf("Frame  Samples --->  %d\n",(int)aio_attr.frameSamples); \
printf("Frame  Number  --->  %d\n",(int)aio_attr.frameNum); \
printf("Sample Format  --->  %d(0:PCM 1:mu_law 2:a_law)\n",(int)sample_format); \
printf("Frame  Size    --->  %d\n",frame_size);

#define PRINT_PLAYBACK_INFO printf("\nPlayback Info:\n"); \
printf("Bit    Width   --->  %d\n",(int)aio_attr.bitWidth); \
printf("Sound  Mode    --->  %d(0:LEFT 1:RIGHT 2:STEREO 3:MONO 4:SINGLE)\n",(int)aio_attr.soundMode); \
printf("Sample Rate    --->  %d\n",(int)aio_attr.sampleRate); \
printf("Frame  Samples --->  %d\n",(int)aio_attr.frameSamples); \
printf("Frame  Number  --->  %d\n",(int)aio_attr.frameNum); \
printf("Sample Format  --->  %d(0:PCM 1:mu_law 2:a_law)\n",(int)sample_format); \
printf("Frame  Size    --->  %d\n",audio_frame.len);


#define PRINT_LOOPBACK_INFO printf("\nLoopback Info:\n"); \
printf("Bit    Width   --->  %d\n",(int)aio_attr.bitWidth); \
printf("Sound  Mode    --->  %d(0:LEFT 1:RIGHT 2:STEREO 3:MONO 4:SINGLE)\n",(int)aio_attr.soundMode); \
printf("Sample Rate    --->  %d\n",(int)aio_attr.sampleRate); \
printf("Frame  Samples --->  %d\n",(int)aio_attr.frameSamples); \
printf("Frame  Number  --->  %d\n",(int)aio_attr.frameNum); \


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
static const GADI_CHAR *shortOptions = "hPRLS:g:v:s:n:r:m:";
static struct option longOptions[] =
{
    {"help",     0, 0, 'h'},
    {"playback", 0, 0, 'P'},
    {"record",   0, 0, 'R'},
    {"loopback", 0, 0, 'L'},
    {"stop",     1, 0, 'S'},
    {"gain",     1, 0, 'g'},
    {"volume",   1, 0, 'v'},
    {"samples",   1, 0, 's'},
    {"number",   1, 0, 'n'},
    {"sampleRate",   1, 0, 'r'},
    {"soundMode",   1, 0, 'm'},
    {0,             0, 0, 0}
};

char record_file_name[256];
char playback_file_name[256];


static GADI_SYS_ThreadHandleT    audioPlaybackThreadHandle = 0;
static GADI_SYS_ThreadHandleT    audioRecordThreadHandle   = 0;
static GADI_SYS_ThreadHandleT    audioLoopbackThreadHandle   = 0;


static GADI_S32 audioPlaybackRunning = 0;
static GADI_S32 audioRecordRunning   = 0;
static GADI_S32 audioLoopbackRunning  = 0;

static GADI_U32 frameSamples = 160;
static GADI_U32 frameNum = 30;
static GADI_AUDIO_SampleRateEnumT sampleRate = GADI_AUDIO_SAMPLE_RATE_8000;
static GADI_AUDIO_SoundModeEnumT soundMode = GADI_AUDIO_SOUND_MODE_SINGLE;
#ifdef AUDIO_ECHO_CANCELLATION_SUPPORT
static GADI_S32 aecHandle;
#endif
//*****************************************************************************
//*****************************************************************************
//** Local Functions Declaration
//*****************************************************************************
//*****************************************************************************
static GADI_VOID usage(GADI_VOID);
static GADI_ERR handle_audio_command(GADI_S32 argc, GADI_CHAR* argv[]);
static GADI_VOID audio_playback_thread(GADI_VOID *optDataPtr);
static GADI_VOID audio_record_thread(GADI_VOID *optDataPtr);
static GADI_VOID audio_loopback_thread(GADI_VOID *optDataPtr);
static GADI_AUDIO_VolumeLevelEnumT  get_volume_level(GADI_S32 value);
static GADI_AUDIO_GainLevelEnumT  get_gain_level(GADI_S32 value);
static GADI_S32 get_volume_value(GADI_AUDIO_VolumeLevelEnumT volumeLevel);
static GADI_S32 get_gain_value(GADI_AUDIO_GainLevelEnumT gainLevel);
static GADI_AUDIO_SoundModeEnumT get_sound_mode(GADI_S32 value);
static GADI_AUDIO_SampleRateEnumT get_sample_rate(GADI_S32 value);

//*****************************************************************************
//*****************************************************************************
//** API Functions
//*****************************************************************************
//*****************************************************************************
GADI_ERR gdm_audio_register_testcase(GADI_VOID)
{
    GADI_ERR   retVal = GADI_OK;
    (void)shell_registercommand (
        "audio",
        handle_audio_command,
        "audio command",
        "---------------------------------------------------------------------\n"
        "audio -R file_name\n"
        "   brief : start record audio stream.\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "audio -L\n"
        "   brief : Loopback(ai to ao)  audio stream.\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "audio -P file_name\n"
        "   brief : start play audio stream.\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "audio -S\n"
        "   brief : stop record audio stream.\n"
        "\n"
    );

    return retVal;
}

GADI_ERR gdm_audio_init(GADI_VOID)
{
    GADI_ERR retVal;

    retVal = gadi_audio_init();
    if(retVal != GADI_OK){
        GADI_ERROR("gadi_audio_init fail\n");
        return retVal;
    }
#ifdef AUDIO_ECHO_CANCELLATION_SUPPORT
    retVal = ap_aec_register(&aecHandle);
    if(retVal != GADI_OK){
        GADI_ERROR("aec register fail\n");
    }
#endif

    return retVal;
}

GADI_ERR gdm_audio_exit(GADI_VOID)
{
    GADI_ERR retVal;

#ifdef AUDIO_ECHO_CANCELLATION_SUPPORT
    retVal = ap_aec_unregister(aecHandle);
    if(retVal != GADI_OK){
        GADI_ERROR("aec unregister fail\n");
        return retVal;
    } else {
        GADI_INFO("aec unregister ok\n");
    }
#endif

    retVal = gadi_audio_exit();
     if(retVal != GADI_OK){
        GADI_ERROR("gadi_audio_exit fail\n");
    } else {
        GADI_INFO("gadi_audio_exit ok\n");
    }

    return retVal;
}

GADI_ERR gdm_audio_ai_set_gain(GADI_S32 value)
{
    GADI_AUDIO_GainLevelEnumT gain_level;
    GADI_ERR retVal;

    gain_level = get_gain_level(value);
    retVal = gadi_audio_ai_set_gain(&gain_level);
    if(retVal != GADI_OK){
        GADI_ERROR("gadi_audio_ai_set_gain failed!\n");
        return retVal;
    }

    return GADI_OK;
}

GADI_ERR gdm_audio_ai_get_gain(GADI_S32 *value)
{
    GADI_AUDIO_GainLevelEnumT gain_level;
    GADI_ERR retVal;

    retVal = gadi_audio_ai_get_gain(&gain_level);
    if(retVal != GADI_OK){
        GADI_ERROR("gadi_audio_ai_set_gain failed!\n");
        return retVal;
    }
    *value = get_gain_value(gain_level);

    return GADI_OK;
}

GADI_ERR gdm_audio_ao_set_volume(GADI_S32 value)
{
    GADI_AUDIO_VolumeLevelEnumT volume_level;
    GADI_ERR retVal;

    volume_level = get_volume_level(value);
    retVal = gadi_audio_ao_set_volume(&volume_level);
    if(retVal != GADI_OK){
        GADI_ERROR("gadi_audio_ao_set_volume failed!\n");
        return retVal;
    }

    return GADI_OK;
}

GADI_ERR gdm_audio_ao_get_volume(GADI_S32 *value)
{
    GADI_AUDIO_VolumeLevelEnumT volume_level;
    GADI_ERR retVal;

    retVal = gadi_audio_ao_get_volume(&volume_level);
    if(retVal != GADI_OK){
        GADI_ERROR("gadi_audio_ao_get_volume failed!\n");
        return retVal;
    }
    *value = get_volume_value(volume_level);

    return GADI_OK;
}

//*****************************************************************************
//*****************************************************************************
//** Local Functions
//*****************************************************************************
//*****************************************************************************
static GADI_VOID usage(GADI_VOID)
{
    printf("\nusage: audio [OPTION]...[FILE]...\n");
    printf("\t-h, --help            help.\n"
           "\t-P, --playback        playback file.\n"
           "\t-L, --loopback        loopback(ai to ao).\n"
           "\t-R, --record          record file.\n"
           "\t-S, --stop            stop.\n"
           "\t-v, --volume=#        set ao volume level(0~12)\n"
           "\t-g, --gain=#          set ai gain level(0~12)\n"
           "\t-s, --samples=#       set frame Samples(32 ~ 1024(4 align))\n"
           "\t-n, --number=#        set frame number(8 ~ 128)\n"
           "\t-r, --sampleRate=#    set sample rate(8000,11025,12000,16000,22050,24000,32000,44100,48000)\n"
           "\t-m, --mode=#          set sound mode(0:LEFT 1:RIGHT 2:STEREO 3:MONO 4:SINGLE)\n");
    printf("\n");
}

static GADI_ERR handle_audio_command(GADI_S32 argc, GADI_CHAR* argv[])
{
    GADI_S32 option_index;
    GADI_S32 ch;
    GADI_S32 value;
    GADI_AUDIO_StreamDirectEnumT stream_direct = GADI_AUDIO_STREAM_LAST;
    GADI_CHAR *file_name;
    GADI_ERR retVal;

    strcpy(record_file_name, DEFAULT_RECORD_STREAM_FILE);
    strcpy(playback_file_name, DEFAULT_PLAYBACK_STREAM_FILE);

    file_name = record_file_name;

	optind = 1;

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
                stream_direct = GADI_AUDIO_STREAM_CAPTURE;
                break;

            case 'P':
                file_name = playback_file_name;
                stream_direct = GADI_AUDIO_STREAM_PLAYBACK;
                break;

            case 'L':
                stream_direct = GADI_AUDIO_STREAM_LOOPBACK;
                break;

            case 'g':
                retVal = gdm_audio_ai_set_gain(strtol(optarg,NULL,0));
                if(retVal != GADI_OK) {
                    goto bad_parameter;
                }
                retVal = gdm_audio_ai_get_gain(&value);
                if(retVal != GADI_OK) {
                    goto bad_parameter;
                }
                GADI_INFO("Current AI gain: %d Lv\n",value);
                break;

            case 'v':
                retVal = gdm_audio_ao_set_volume(strtol(optarg,NULL,0));
                if(retVal != GADI_OK) {
                    goto bad_parameter;
                }
                retVal = gdm_audio_ao_get_volume(&value);
                if(retVal != GADI_OK) {
                    goto bad_parameter;
                }
                GADI_INFO("Current AO volume: %d Lv\n",value);
                break;

            case 's':
                frameSamples = strtol(optarg,NULL,0);
                break;
            case 'n':
                frameNum = strtol(optarg,NULL,0);
                break;
            case 'r':
                sampleRate = get_sample_rate(strtol(optarg,NULL,0));
                break;
            case 'm':
                soundMode = get_sound_mode(strtol(optarg,NULL,0));
                break;
            case 'S':
                if(strcmp(optarg, "play") == 0)
                {
                    if(audioPlaybackThreadHandle != 0)
                    {
                        audioPlaybackRunning = 0;
                        retVal = gadi_sys_wait_end_thread(audioPlaybackThreadHandle);
                        if (retVal != GADI_OK)
                        {
                            printf("audio: destroy playback thread failed %d\n", retVal);
                            goto command_exit;
                        }
                        printf("audio: stop playback successfully!!!\n");
                        audioPlaybackThreadHandle = 0;
                    }
                    goto command_exit;
                }
                else if(strcmp(optarg, "record") == 0)
                {
                    if(audioRecordThreadHandle != 0)
                    {
                        audioRecordRunning = 0;
                        retVal = gadi_sys_wait_end_thread(audioRecordThreadHandle);
                        if (retVal != GADI_OK)
                        {
                            printf("audio: destroy record thread failed %d\n", retVal);
                            goto command_exit;
                        }
                        printf("audio: stop record successfully!!!\n");
                        audioRecordThreadHandle = 0;
                    }
                    goto command_exit;
                }
                else if(strcmp(optarg, "loopback") == 0)
                {
                    if(audioLoopbackThreadHandle != 0)
                    {
                        audioLoopbackRunning = 0;
                        retVal = gadi_sys_wait_end_thread(audioLoopbackThreadHandle);
                        if (retVal != GADI_OK)
                        {
                            printf("audio: destroy record thread failed %d\n", retVal);
                            goto command_exit;
                        }
                        printf("audio: stop loop successfully!!!\n");
                        audioLoopbackThreadHandle = 0;
                    }
                    goto command_exit;
                }
                else
                {
                    printf("please intput what to stop -S (record/play/loopback)?\n");
                    goto command_exit;
                }
                break;

            default:
                printf("type '--help' for more usage.\n");
                goto command_exit;
        }
    }

    if(optind <= argc -1) {
        strcpy(file_name, argv[optind]);
        GADI_INFO("audio test: file name=%s\n",file_name);
    }

    if(stream_direct == GADI_AUDIO_STREAM_PLAYBACK)
    {
        if (audioPlaybackRunning == 0)
        {
            audioPlaybackRunning = 1;
            gadi_sys_thread_create(audio_playback_thread,
                                   NULL,
                                   AUDIO_PLAYBACK_THREAD_PRIORITY,
                                   AUDIO_PLAYBACK_THREAD_STACKSIZE,
                                   AUDIO_PLAYBACK_THREAD_NAME,
                                   &audioPlaybackThreadHandle);
        }
    }
    else if(stream_direct == GADI_AUDIO_STREAM_CAPTURE)
    {
        if(audioRecordRunning == 0)
        {
            audioRecordRunning = 1;
            gadi_sys_thread_create(audio_record_thread,
                                   NULL,
                                   AUDIO_RECORD_THREAD_PRIORITY,
                                   AUDIO_RECORD_THREAD_STACKSIZE,
                                   AUDIO_RECORD_THREAD_NAME,
                                   &audioRecordThreadHandle);
        }

    }
    else if(stream_direct == GADI_AUDIO_STREAM_LOOPBACK)
    {
        if(audioLoopbackRunning == 0)
        {
            audioLoopbackRunning = 1;
            gadi_sys_thread_create(audio_loopback_thread,
                                   NULL,
                                   AUDIO_LOOPBACK_THREAD_PRIORITY,
                                   AUDIO_LOOPBACK_THREAD_STACKSIZE,
                                   AUDIO_LOOPBACK_THREAD_NAME,
                                   &audioLoopbackThreadHandle);
        }
    }
command_exit:
    optind = 1;
    return 0;

bad_parameter:
	optind = 1;
    return -1;
}


static GADI_VOID audio_playback_thread(GADI_VOID *optDataPtr)
{
    const GADI_CHAR *name = playback_file_name;
    GADI_S32 fd = -1;
    GADI_S32 ao_fd = -1;
    unsigned int  count  = 0;
    GADI_AUDIO_AioFrameT audio_frame;
    GADI_AUDIO_SampleFormatEnumT sample_format;
    GADI_AUDIO_AioAttrT aio_attr;
    GADI_BOOL flag = GADI_TRUE;
    GADI_ERR retVal = 0;

    /* config AO dev attr */
    aio_attr.bitWidth = GADI_AUDIO_BIT_WIDTH_16;
    aio_attr.soundMode = soundMode;
    aio_attr.sampleRate = sampleRate;
    aio_attr.frameSamples = frameSamples;
    aio_attr.frameNum = frameNum;

    if ((fd = open(name, O_RDONLY, 0)) == -1)
    {
        GADI_ERROR("open %s failed.\n",name);
        goto exit;
    }

    /* set audio sample format*/
    sample_format = GADI_AUDIO_SAMPLE_FORMAT_RAW_PCM;
    ao_fd = gadi_audio_ao_get_fd();
    gadi_audio_set_sample_format(ao_fd, sample_format);

    /* set AO dev attr */
    retVal = gadi_audio_ao_set_attr(&aio_attr);
    if (retVal){
        GADI_ERROR("Set ao attribute failed \n");
        goto exit1;
    }

    /* enable AO*/
    if (gadi_audio_ao_enable()){
        GADI_ERROR("Enable ao device failed \n");
        goto exit1;
    }

    /* calculate frame size */
    if (sample_format == GADI_AUDIO_SAMPLE_FORMAT_RAW_PCM) {
        audio_frame.len = frameSamples * (aio_attr.bitWidth >> 3);
    } else {
        audio_frame.len = frameSamples * (aio_attr.bitWidth >> 3) / 2;
    }

    audio_frame.virAddr = (unsigned char*)gadi_sys_malloc(audio_frame.len);

    PRINT_PLAYBACK_INFO

    /* playback */
    while (audioPlaybackRunning)
    {
        if(!retVal) {
            count = read(fd, audio_frame.virAddr, audio_frame.len);
        }
        if (count == 0) {
            lseek(fd, 0, SEEK_SET);
        }
        if(count == audio_frame.len) {
            #ifndef AUDIO_ECHO_CANCELLATION_SUPPORT
            retVal = gadi_audio_ao_send_frame(&audio_frame,flag);
            #else
            retVal = gadi_audio_ao_send_frame_aec(&audio_frame,flag);
            #endif
        }
        /*non-blocking (flag is GADI_FALSE), please sleep here*/
        //gadi_sys_thread_sleep(1);
    }

    free(audio_frame.virAddr);

    gadi_audio_ao_disable();
exit1:
    close(fd);
exit:
    audioPlaybackRunning = 0;

    return;
}

static GADI_VOID audio_record_thread(GADI_VOID *optDataPtr)
{
    const GADI_CHAR *name = record_file_name;
    GADI_S32 fd = -1;
    GADI_S32 ai_fd = -1;
    GADI_AUDIO_AioFrameT audio_frame;
    #ifdef AUDIO_ECHO_CANCELLATION_SUPPORT
    GADI_AEC_AioFrameT aecFrame;
    #endif
    GADI_AUDIO_SampleFormatEnumT sample_format;
    GADI_AUDIO_AioAttrT aio_attr;
    GADI_U32 frame_size;
    GADI_BOOL flag = GADI_TRUE;
    GADI_ERR retVal;

    /* config aio dev attr */
    aio_attr.bitWidth = GADI_AUDIO_BIT_WIDTH_16;
    aio_attr.soundMode = GADI_AUDIO_SOUND_MODE_SINGLE;
    aio_attr.sampleRate = sampleRate;
    aio_attr.frameSamples = frameSamples;
    aio_attr.frameNum = frameNum;

    /* open a new file to write */
    remove(name);
    if ((fd = open(name, O_WRONLY | O_CREAT, 0644)) == -1)
    {
        GADI_ERROR("open %s failed.\n", name);
        goto exit;
    }

    /* set audio sample format*/
    sample_format = GADI_AUDIO_SAMPLE_FORMAT_RAW_PCM;
    ai_fd = gadi_audio_ai_get_fd();
    gadi_audio_set_sample_format(ai_fd, sample_format);

    /* set AO dev attr */
    retVal = gadi_audio_ai_set_attr(&aio_attr);
    if (retVal){
        GADI_ERROR("Set ai attribute failed \n");
        goto exit1;
    }

    /* enable AI*/
    if (gadi_audio_ai_enable()){
        GADI_ERROR("Enable ai device failed \n");
        goto exit1;
    }

    /* calculate frame size (Just to show)*/
    if (sample_format == GADI_AUDIO_SAMPLE_FORMAT_RAW_PCM) {
        frame_size = frameSamples * (aio_attr.bitWidth >> 3);
    } else {
        frame_size = frameSamples * (aio_attr.bitWidth >> 3) / 2;
    }

#ifdef AUDIO_ECHO_CANCELLATION_SUPPORT
    if(gadi_audio_ai_aec_enable()) {
        GADI_ERROR("Enable ai aec failed \n");
        goto exit2;
    }
#endif
    PRINT_RECORD_INFO

    /* record */
    while (audioRecordRunning)
    {
#ifdef AUDIO_ECHO_CANCELLATION_SUPPORT
        if(gadi_audio_ai_get_frame_aec(&audio_frame, &aecFrame, flag) == 0)
#else
        if(gadi_audio_ai_get_frame(&audio_frame, flag) == 0)
#endif
        {
           if (write(fd, audio_frame.virAddr, audio_frame.len) != audio_frame.len)
           {
               GADI_ERROR("audio_record: write file error, len=%d\n",audio_frame.len);
           }
        }
        /*non-blocking (flag is GADI_FALSE), please sleep here*/
        //gadi_sys_thread_sleep(1);

    }

#ifdef AUDIO_ECHO_CANCELLATION_SUPPORT
    gadi_audio_ai_aec_disable();
exit2:
#endif

    gadi_audio_ai_disable();
exit1:
    close(fd);

exit:
    audioRecordRunning = 0;

    return;
}

static GADI_VOID audio_loopback_thread (GADI_VOID *optDataPtr)
{
    GADI_AUDIO_AioAttrT aio_attr;

    /* config aio dev attr */
    aio_attr.bitWidth = GADI_AUDIO_BIT_WIDTH_16;
    aio_attr.soundMode = GADI_AUDIO_SOUND_MODE_SINGLE;
    aio_attr.sampleRate = sampleRate;
    aio_attr.frameSamples = 0;
    aio_attr.frameNum = 0;

    if(gadi_audio_ao_bind_ai(&aio_attr) != GADI_OK) {
        audioLoopbackRunning = 0;
        return;
    }

    PRINT_LOOPBACK_INFO

    while (audioLoopbackRunning)
    {
        gadi_sys_thread_sleep(1000);
    }

    gadi_audio_ao_unbind_ai();

    return;
}

static GADI_AUDIO_VolumeLevelEnumT get_volume_level(GADI_S32 value)
{
    GADI_AUDIO_VolumeLevelEnumT volume_level;

    if(value > 12) {
        value = 12;
    }else if (value < 0) {
        value = 0;
    }

    switch(value)
    {
        case 0:
            volume_level = VLEVEL_0;
            break;
        case 1:
            volume_level = VLEVEL_1;
            break;
        case 2:
            volume_level = VLEVEL_2;
            break;
        case 3:
            volume_level = VLEVEL_3;
            break;
        case 4:
            volume_level = VLEVEL_4;
            break;
        case 5:
            volume_level = VLEVEL_5;
            break;
        case 6:
            volume_level = VLEVEL_6;
            break;
        case 7:
            volume_level = VLEVEL_7;
            break;
        case 8:
            volume_level = VLEVEL_8;
            break;
        case 9:
            volume_level = VLEVEL_9;
            break;
        case 10:
            volume_level = VLEVEL_10;
            break;
        case 11:
            volume_level = VLEVEL_11;
            break;
        case 12:
            volume_level = VLEVEL_12;
            break;
        default:
            volume_level = VLEVEL_8;
            break;
    }

    return volume_level;
}

static GADI_S32 get_volume_value(GADI_AUDIO_VolumeLevelEnumT volumeLevel)
{
    GADI_S32 value = -1;

    switch(volumeLevel)
    {
        case VLEVEL_0:
            value = 0;
            break;
        case VLEVEL_1:
            value = 1;
            break;
        case VLEVEL_2:
            value = 2;
            break;
        case VLEVEL_3:
            value = 3;
            break;
        case VLEVEL_4:
            value = 4;
            break;
        case VLEVEL_5:
            value = 5;
            break;
        case VLEVEL_6:
            value = 6;
            break;
        case VLEVEL_7:
            value = 7;
            break;
        case VLEVEL_8:
            value = 8;
            break;
        case VLEVEL_9:
            value = 9;
            break;
        case VLEVEL_10:
            value = 10;
            break;
        case VLEVEL_11:
            value = 11;
            break;
        case VLEVEL_12:
            value = 12;
            break;
        default:
            ;
    }

    return value;
}

static GADI_AUDIO_GainLevelEnumT get_gain_level(GADI_S32 value)
{
    GADI_AUDIO_GainLevelEnumT gain_level;

    if(value > 15) {
        value = 15;
    }else if (value < 0) {
        value = 0;
    }

    switch(value)
    {
        case 0:
            gain_level = GLEVEL_0;
            break;
        case 1:
            gain_level = GLEVEL_1;
            break;
        case 2:
            gain_level = GLEVEL_2;
            break;
        case 3:
            gain_level = GLEVEL_3;
            break;
        case 4:
            gain_level = GLEVEL_4;
            break;
        case 5:
            gain_level = GLEVEL_5;
            break;
        case 6:
            gain_level = GLEVEL_6;
            break;
        case 7:
            gain_level = GLEVEL_7;
            break;
        case 8:
            gain_level = GLEVEL_8;
            break;
        case 9:
            gain_level = GLEVEL_9;
            break;
        case 10:
            gain_level = GLEVEL_10;
            break;
        case 11:
            gain_level = GLEVEL_11;
            break;
        case 12:
            gain_level = GLEVEL_12;
            break;
        case 13:
            gain_level = GLEVEL_13;
            break;
        case 14:
            gain_level = GLEVEL_14;
            break;
        case 15:
            gain_level = GLEVEL_15;
            break;
        default:
            gain_level = GLEVEL_12;
            break;
    }

    return gain_level;
}

static GADI_S32 get_gain_value(GADI_AUDIO_GainLevelEnumT gainLevel)
{
    GADI_S32 value = -1;

    switch(gainLevel)
    {
        case GLEVEL_0:
            value = 0;
            break;
        case GLEVEL_1:
            value = 1;
            break;
        case GLEVEL_2:
            value = 2;
            break;
        case GLEVEL_3:
            value = 3;
            break;
        case GLEVEL_4:
            value = 4;
            break;
        case GLEVEL_5:
            value = 5;
            break;
        case GLEVEL_6:
            value = 6;
            break;
        case GLEVEL_7:
            value = 7;
            break;
        case GLEVEL_8:
            value = 8;
            break;
        case GLEVEL_9:
            value = 9;
            break;
        case GLEVEL_10:
            value = 10;
            break;
        case GLEVEL_11:
            value = 11;
            break;
        case GLEVEL_12:
            value = 12;
            break;
        case GLEVEL_13:
            value = 13;
            break;
        case GLEVEL_14:
            value = 14;
            break;
        case GLEVEL_15:
            value = 15;
            break;
        default:
            ;
    }

    return value;
}

static GADI_AUDIO_SoundModeEnumT get_sound_mode(GADI_S32 value)
{
    GADI_AUDIO_SoundModeEnumT sound_mode;

    switch(value)
    {
        case 0:
            sound_mode = GADI_AUDIO_SOUND_MODE_LEFT;
            break;
        case 1:
            sound_mode = GADI_AUDIO_SOUND_MODE_RIGHT;
            break;
        case 2:
            sound_mode = GADI_AUDIO_SOUND_MODE_STEREO;
            break;
        case 3:
            sound_mode = GADI_AUDIO_SOUND_MODE_MONO;
            break;
        case 4:
            sound_mode = GADI_AUDIO_SOUND_MODE_SINGLE;
            break;
        default:
            sound_mode = GADI_AUDIO_SOUND_MODE_NUM;
    }

    return sound_mode;
}

static GADI_AUDIO_SampleRateEnumT get_sample_rate(GADI_S32 value)
{
    GADI_AUDIO_SampleRateEnumT sample_rate;

    switch(value)
    {
        case 8000:
            sample_rate = GADI_AUDIO_SAMPLE_RATE_8000;
            break;
        case 11025:
            sample_rate = GADI_AUDIO_SAMPLE_RATE_11025;
            break;
        case 16000:
            sample_rate = GADI_AUDIO_SAMPLE_RATE_16000;
            break;
        case 22050:
            sample_rate = GADI_AUDIO_SAMPLE_RATE_22050;
            break;
        case 24000:
            sample_rate = GADI_AUDIO_SAMPLE_RATE_24000;
            break;
        case 32000:
            sample_rate = GADI_AUDIO_SAMPLE_RATE_32000;
            break;
        case 44100:
            sample_rate = GADI_AUDIO_SAMPLE_RATE_44100;
            break;
        case 48000:
            sample_rate = GADI_AUDIO_SAMPLE_RATE_48000;
            break;
        default:
            sample_rate = GADI_AUDIO_SAMPLE_RATE_NUM;
    }

    return sample_rate;
}
