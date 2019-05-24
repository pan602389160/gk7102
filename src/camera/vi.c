/******************************************************************************
** \file        adi/test/src/vi.c
**
** \brief       ADI layer vi test.
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
#include <fcntl.h>
#include <unistd.h>

#include <sys/prctl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <assert.h>

#include "basetypes.h"
#include "vi.h"
#include "venc.h"
#include "shell.h"
#include "parser.h"
#include "ircut.h"
#include "debug.h"

//*****************************************************************************
//*****************************************************************************
//** Local Defines
//*****************************************************************************
//*****************************************************************************
#define GK_VIDEO_MODE(W,H,F,PI)         (((PI<<31)&0x80000000)|((F<<24)&0x3F000000)|(((W/4)<<12)&0x00FFF000)|((H/2)&0x00000FFF))
#define GK_VIDEO_MODE_GET_WIDTH(mode)   ((mode&0x00FFF000)>>10)
#define GK_VIDEO_MODE_GET_HEIGTH(mode)  ((mode&0x00000FFF)<<1)
#define GK_VIDEO_MODE_GET_FPS(mode)     ((mode&0x3F000000)>>24)
#define GK_VIDEO_MODE_GET_PI(mode)      ((mode&0x80000000)>>31)
#define GK_VIDEO_MODE_GET_MODE(mode)    (mode&0x00FFFFFF)

#define VI_PATH_MAX_LENTH    128
#define VI_AUTO_TEST_PATH	        "/usr/local/test/vi"
#define VI_REPORT_PATH	            "/usr/local/test/vi/report"
#define VI_REPORT_FILE_PATH	        "/usr/local/test/vi/report/vi_test_log"
#define VI_MIRROR_PATH	            "/usr/local/test/vi/mirror"
#define VI_IRCUT_PATH	            "/usr/local/test/vi/ircut"

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

GADI_SYS_HandleT viHandle = NULL;

//*****************************************************************************
//*****************************************************************************
//** Local Data
//*****************************************************************************
//*****************************************************************************
static GADI_VI_SettingParamsT   viParams;
//static GADI_VI_SensorHwInfoT    viSensorHw;
static const char *viShortOptions = "hSPo:s:i:F:f:";

static struct option viLongOptions[] =
{
	{"help",	    0, 0, 'h'},
    {"start test",  0, 0, 'S'},
    {"stop test",   0, 0, 'P'},
	{"operation",	1, 0, 'o'},
	{"slowshutter",	1, 0, 's'},
	{"ircut",	    1, 0, 'i'},
	{"flip",	    1, 0, 'F'},
    {"framerate",   1, 0, 'f'},
	{0, 		    0, 0, 0}
};

static parser_map viMap[] =
{
    {"vi_mode",        &viParams.resoluMode,                 DATA_TYPE_U32},
    {"vi_framerate",   &viParams.frameRate,                  DATA_TYPE_U32},
    {"vi_mirror",      &viParams.mirrorMode.mirrorPattern,   DATA_TYPE_U32},
    {"vi_bayer",       &viParams.mirrorMode.bayerPattern,    DATA_TYPE_U32},
    {NULL,             NULL,                                 DATA_TYPE_U32},
};

//*****************************************************************************
//*****************************************************************************
//** Local Functions Declaration
//*****************************************************************************
//*****************************************************************************
static GADI_ERR flip_then_cap();
static GADI_ERR switch_ircut_then_cap();
static GADI_ERR handle_vi_command(int argc, char* argv[]);
static void     vi_usage(void);
static GADI_ERR vi_auto_test_flip(void);
static GADI_ERR vi_auto_test_slowshutter(void);
static GADI_ERR vi_auto_test_fps(void);
static GADI_ERR vi_auto_test_ircut(void);
static void vi_daytonight(GADI_U32 value);
static void vi_nighttoday(GADI_U32 value);

//*****************************************************************************
//*****************************************************************************
//** API Functions
//*****************************************************************************
//*****************************************************************************
GADI_ERR gdm_vi_init(void)
{
    GADI_ERR retVal = GADI_OK;

    retVal = gadi_vi_init();

    return retVal;
}

GADI_ERR gdm_vi_exit(void)
{
    GADI_ERR retVal = GADI_OK;

    retVal = gadi_vi_exit();

    return retVal;
}

GADI_ERR gdm_vi_open(void)
{
    GADI_ERR retVal = GADI_OK;
    viHandle = gadi_vi_open(&retVal);
    if(retVal != GADI_OK)
    {
        GADI_ERROR("gadi_vi_open error\n");
        return retVal;
    }

    //defaut config IR-cut (board: gk710xs evb).
    ircut_configurate(0, 900, 50, GADI_IRCUT_OverShresholdTrigger);
    //ircut_configurate(1, 250, 5, GADI_IRCUT_OverShresholdTrigger);

    //defaut setup IR cut is day mode.
    gdm_vi_set_ircut_control(GADI_VI_IRCUT_DAY);
    gadi_sys_thread_sleep(100);
    gdm_vi_set_ircut_control(GADI_VI_IRCUT_CLEAR);

    //register ircut switch options.
    ircut_register_daytonight(0, &vi_daytonight);
    ircut_register_nighttoday(0, &vi_nighttoday);

    return retVal;
}

GADI_ERR gdm_vi_close(void)
{
    GADI_ERR retVal = GADI_OK;

    retVal = gadi_vi_close(viHandle);
    if(retVal != GADI_OK)
    {
        GADI_ERROR("gadi_vi_close error\n");
        return retVal;
    }

    return retVal;
}

GADI_ERR gdm_vi_parse_config_file(char *path)
{
    GADI_ERR retVal = GADI_OK;

    if(path == NULL)
        return -1;

    /*parse vi parameters.*/
    retVal =  parse_viInfo(viMap, path);
    DBG_PRINTF("resoluMode:%d,frameRate:%d,mirrorPattern:%d,bayerPattern:%d\n",
                                            viParams.resoluMode,
                                            viParams.frameRate,
                                            viParams.mirrorMode.mirrorPattern,
                                            viParams.mirrorMode.bayerPattern);

    return retVal;
}

GADI_ERR gdm_vi_register_testcase(void)
{
	printf("\n");

    GADI_ERR   retVal =  GADI_OK;
    (void)shell_registercommand (
        "vi",
        handle_vi_command,
        "vi command",
        "---------------------------------------------------------------------\n"
        "vi -h \n"
        "   brief : help\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "vi -o \n"
        "   brief :  set operation mode\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "vi -s\n"
        "   brief :  set slowshutter mode(set vi fps).\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "vi -i\n"
        "   brief :  set ircut.\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "vi -F\n"
        "   brief :  set vi flip\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "vi -f \n"
        "   brief : set vi framerate(invalid foe the moment).\n"
        "\n"
        /******************************************************************/
    );

    return retVal;
}

GADI_ERR gdm_vi_set_flip(GADI_VI_MirrorPatternEnumT rotate)
{
    GADI_VI_MirrorModeParamsT mirrorModePtr;
    GADI_ERR retVal = GADI_OK;

    if((retVal = gdm_venc_stop_encode_all_stream()) < 0)
    {
        GADI_ERROR("gdm_venc_stop_encode_all_stream error.\n");
        goto exit;
    }

    if((retVal = gadi_vi_enable(viHandle, 0)) < 0)
    {
        GADI_ERROR("gadi_vi_enable error.\n");
        goto exit_stream;
    }

    retVal = gadi_vi_get_mirror_mode(viHandle, &mirrorModePtr);
    if(retVal != GADI_OK){
        GADI_ERROR("gadi_vi_get_mirror_mode failed!\n");
        goto exit_enable;
    }

    mirrorModePtr.mirrorPattern = rotate;

    retVal = gadi_vi_set_mirror_mode(viHandle, &mirrorModePtr);
    if(retVal != GADI_OK){
        GADI_ERROR("gadi_vi_set_mirror_mode failed!\n");
        goto exit_enable;
    }

    viParams.mirrorMode = mirrorModePtr;

exit_enable:
    if(gadi_vi_enable(viHandle, 1) < 0)
    {
        GADI_ERROR("gadi_vi_enable error.\n");
    }

exit_stream:
    if((retVal = gdm_venc_start_encode_all_stream()) < 0)
    {
        GADI_ERROR("gdm_venc_start_encode_all_stream error.\n");
    }

exit:
    return retVal;
}

GADI_ERR gdm_vi_set_framerate(GADI_U32 framerate)
{
    GADI_ERR retVal = GADI_OK;

    retVal = gadi_vi_set_framerate(viHandle, &framerate);
    if (retVal != GADI_OK) {
        GADI_ERROR("gadi_vi_set_framerate failed!\n");
        return retVal;
    }

    viParams.frameRate = framerate;

    return retVal;
}

GADI_ERR gdm_vi_set_operation_mode(GADI_U8 mode)
{
    GADI_ERR retVal = GADI_OK;

    retVal = gadi_vi_set_operation_mode(viHandle, mode);
    if(retVal != GADI_OK){
        GADI_ERROR("gadi_vi_set_operation_mode failed!\n");
        return retVal;
    }

    return retVal;
}

GADI_ERR gdm_vi_set_slowshutter_framerate(GADI_U32* framerate)
{
    GADI_ERR retVal = GADI_OK;

    retVal = gadi_vi_set_framerate(viHandle, framerate);
    if(retVal != GADI_OK){
        GADI_ERROR("gadi_vi_set_slowshutter_framerate failed!\n");
        return retVal;
    }

    return retVal;
}

GADI_ERR gdm_vi_set_ircut_control(GADI_VI_IRModeEnumT mode)
{
    GADI_ERR retVal = GADI_OK;

    retVal = gadi_vi_set_ircut_control(viHandle, mode);
    if(retVal != GADI_OK){
        GADI_ERROR("gadi_vi_set_ircut_control failed!\n");
        return retVal;
    }

    return retVal;
}

GADI_ERR gdm_vi_setup(void)
{
    GADI_ERR retVal = GADI_OK;

    /*video input module: set video input frame rate and resolution.*/
    if (viParams.resoluMode != 0) {
        viParams.resoluMode = GK_VIDEO_MODE(viParams.resoluMode/10000,
                                            viParams.resoluMode%10000,
                                            viParams.frameRate,
                                            1);
    }

    retVal = gadi_vi_set_params(viHandle, &viParams);
    if(retVal != GADI_OK)
    {
        GADI_ERROR("gadi_vi_set_params error\n");
        return retVal;
    }

    retVal = gadi_vi_set_mirror_mode(viHandle, &viParams.mirrorMode);
    if(retVal != GADI_OK)
    {
        GADI_ERROR("gadi_vi_set_mirror_mode error\n");
        return retVal;
    }

    return retVal;
}

GADI_ERR gdm_vi_auto_test_init(void)
{
    GADI_ERR ret = GADI_OK;
    GADI_CHAR pathBuf[VI_PATH_MAX_LENTH] = {0};

    // create work dir
    if (!access(VI_AUTO_TEST_PATH, F_OK)) {
        shell_system("rm -rf "VI_AUTO_TEST_PATH);
    }
    snprintf(pathBuf, VI_PATH_MAX_LENTH, "mkdir -p %s", VI_AUTO_TEST_PATH);
    shell_system(pathBuf);

    // create report dir and log file
    memset(pathBuf, 0, sizeof(pathBuf));
    snprintf(pathBuf, VI_PATH_MAX_LENTH, "mkdir -p %s", VI_REPORT_PATH);
    shell_system(pathBuf);
    shell_system("touch "VI_REPORT_FILE_PATH);

    // create mirror dir
    memset(pathBuf, 0, sizeof(pathBuf));
    snprintf(pathBuf, VI_PATH_MAX_LENTH, "mkdir -p %s", VI_MIRROR_PATH);
    shell_system(pathBuf);

    // create ircut dir
    memset(pathBuf, 0, sizeof(pathBuf));
    snprintf(pathBuf, VI_PATH_MAX_LENTH, "mkdir -p %s", VI_IRCUT_PATH);
    shell_system(pathBuf);

    // start
    shell_system("echo \"Start to test vi func automatically.\"           >> " \
        VI_REPORT_FILE_PATH);
    shell_system("echo \"Will generate report vi_test_log.\"                >> " \
        VI_REPORT_FILE_PATH);

    return ret;
}

GADI_ERR gdm_vi_auto_test(void)
{
    GADI_ERR ret = GADI_OK;

    // mirror
    ret = vi_auto_test_flip();
    if (ret) {
        GADI_ERROR("Fail to test mirror mode(ret = %d).", ret);
        return -1;
    }

    // slowshutter
    ret = vi_auto_test_slowshutter();
    if (ret) {
        GADI_ERROR("Fail to test slowshutter mode(ret = %d).", ret);
        return -1;
    }

    // fps
    ret = vi_auto_test_fps();
    if (ret) {
        GADI_ERROR("Fail to test fps(ret = %d).", ret);
        return -1;
    }

    // ircut
    ret = vi_auto_test_ircut();
    if (ret) {
        GADI_ERROR("Fail to test ircut(ret = %d).", ret);
        return -1;
    }

    // operation mode

    // agc

    // shutter time

    return ret;
}

GADI_ERR gdm_vi_auto_test_exit(void)
{
    GADI_ERR ret = GADI_OK;
    if (!access(VI_AUTO_TEST_PATH, F_OK)) {
        shell_system("rm -rf "VI_AUTO_TEST_PATH);
    }

    GADI_INFO("Stop to auto test, delete all report.\n");

    return ret;
}

//*****************************************************************************
//*****************************************************************************
//** Local Functions
//*****************************************************************************
//*****************************************************************************

static void vi_daytonight(GADI_U32 value)
{
    GADI_U32 fps;

    gdm_vi_set_ircut_control(GADI_VI_IRCUT_NIGHT);
    gadi_sys_thread_sleep(100);
    gdm_vi_set_ircut_control(GADI_VI_IRCUT_CLEAR);

    fps = GADI_VI_FPS_10; // use the default fps
    gadi_vi_set_framerate(viHandle, &fps);
}

static void vi_nighttoday(GADI_U32 value)
{
    GADI_U32 fps;

    gdm_vi_set_ircut_control(GADI_VI_IRCUT_DAY);
    gadi_sys_thread_sleep(100);
    gdm_vi_set_ircut_control(GADI_VI_IRCUT_CLEAR);

    fps = GADI_VI_FPS_AUTO; // use the default fps
    gadi_vi_set_framerate(viHandle, &fps);
}

static GADI_ERR vi_auto_test_flip(void)
{
    GADI_ERR  retVal = GADI_OK;
    GADI_CHAR viSnapPath[VI_PATH_MAX_LENTH] = {0};
    GADI_VI_MirrorModeParamsT preMode;
    GADI_VI_MirrorModeParamsT curMode;

    if (!viHandle) {
        GADI_ERROR("Not init VI.");
        return -1;
    }

    shell_system("echo \"=======================================\"        >> " \
        VI_REPORT_FILE_PATH);
    shell_system("echo \"##### mirror mode.\"                             >> " \
        VI_REPORT_FILE_PATH);
    shell_system("echo \"##### gadi_vi_get_mirror_mode()\"                >> " \
        VI_REPORT_FILE_PATH);
    shell_system("echo \"##### gadi_vi_set_mirror_mode()\"                >> " \
        VI_REPORT_FILE_PATH);

    // none
    retVal = gadi_vi_get_mirror_mode(viHandle, &preMode);
    if (retVal != GADI_OK) {
        GADI_ERROR("gadi_vi_get_mirror_mode failed.");
        return retVal;
    }
    usleep(1);
	snprintf(viSnapPath, VI_PATH_MAX_LENTH, "%s/%s", VI_MIRROR_PATH, "mirror_n.jpeg");
    remove(viSnapPath);
	retVal = gdm_venc_capture_jpeg(0, viSnapPath);
	if (retVal) {
        GADI_ERROR("Fail to capture %s.", viSnapPath);
        return retVal;
	}

    // H&V test
    curMode.mirrorPattern = GADI_VI_MIRROR_HORRIZONTALLY_VERTICALLY;
    curMode.bayerPattern  = preMode.bayerPattern;
    retVal = flip_then_cap(&curMode);
    if (retVal) {
        GADI_ERROR("Fail to capture hv.");
        return retVal;
    }

    // H test
    curMode.mirrorPattern = GADI_VI_MIRROR_HORRIZONTALLY;
    curMode.bayerPattern  = preMode.bayerPattern;
    retVal = flip_then_cap(&curMode);
    if (retVal) {
        GADI_ERROR("Fail to capture h.");
        return retVal;
    }

    // V test
    curMode.mirrorPattern = GADI_VI_MIRROR_VERTICALLY;
    curMode.bayerPattern  = preMode.bayerPattern;
    retVal = flip_then_cap(&curMode);
    if (retVal) {
        GADI_ERROR("Fail to capture v.");
        return retVal;
    }

    // revert
    retVal = gadi_vi_set_mirror_mode(viHandle, &preMode);
    if (retVal != GADI_OK) {
        GADI_ERROR("Fail to revert flip status automatically.\n");
        return retVal;
    }

    shell_system("echo \"##### Picture had been saved, please check.\"    >> " \
        VI_REPORT_FILE_PATH);
    shell_system("echo \"##### Finish to test mirror mode.\"              >> " \
        VI_REPORT_FILE_PATH);
    shell_system("echo \"=======================================\"        >> " \
        VI_REPORT_FILE_PATH);

    return retVal;
}


static GADI_ERR vi_auto_test_slowshutter(void)
{
    GADI_ERR   retVal = GADI_OK;
    GADI_U32   preFps = 0;
    GADI_U32   curFps = 0;
    GADI_ULONG timesA = 0;
    GADI_ULONG timesB = 0;
    GADI_CHAR  cmd[128] = {0};

    if (!viHandle) {
        GADI_ERROR("Not init VI.");
        return -1;
    }

    shell_system("echo \"=======================================\"        >> " \
        VI_REPORT_FILE_PATH);
    shell_system("echo \"##### slowshutter mode.\"                        >> " \
        VI_REPORT_FILE_PATH);
    shell_system("echo \"##### gdm_vi_set_slowshutter_framerate()\"       >> " \
        VI_REPORT_FILE_PATH);
    shell_system("echo \"##### gadi_vi_get_interrupt()\"                  >> " \
        VI_REPORT_FILE_PATH);

    // set slowshutter mode 1, 20 fps

    // set slowshutter mode 2, 15 fps
    curFps = GADI_VI_FPS_15;
    retVal = gdm_vi_set_slowshutter_framerate(&curFps);
    if (retVal != GADI_OK) {
        GADI_ERROR("Fail to set slowshutter framerate.");
        return retVal;
    }
    GADI_INFO("slowshutter framerate: %d.\n", curFps);
    retVal = gadi_vi_get_interrupt(viHandle, &timesA);
    if (retVal != GADI_OK) {
        GADI_ERROR("Fail to get interrupt of VI.");
        return retVal;
    }
    sleep(1);
    retVal = gadi_vi_get_interrupt(viHandle, &timesB);
    if (retVal != GADI_OK) {
        GADI_ERROR("Fail to get interrupt of VI.");
        return retVal;
    }
    // check
	snprintf(cmd, 128, "echo \"##### slowshutter framerate: %d, VI interrupt: %d.\" >> %s\n",
	    curFps, (int)(timesB - timesA), VI_REPORT_FILE_PATH);
    shell_system(cmd);
    if ((timesB - timesA) > 17 || (timesB - timesA) < 13) {
        shell_system("echo \"##### Fail to test slow shutter mode 2.\"    >> " \
            VI_REPORT_FILE_PATH);
    } else {
        shell_system("echo \"##### Success to test slow shutter mode 2.\" >> " \
            VI_REPORT_FILE_PATH);
    }

    // set slowshutter mode 3, 10 fps
    // check

    // set slowshutter mode 4, 5 fps
    curFps = GADI_VI_FPS_5;
    retVal = gdm_vi_set_slowshutter_framerate(&curFps);
    if (retVal != GADI_OK) {
        GADI_ERROR("Fail to set slowshutter framerate.");
        return retVal;
    }
    GADI_INFO("slowshutter frame: %d.\n", curFps);
    retVal = gadi_vi_get_interrupt(viHandle, &timesA);
    if (retVal != GADI_OK) {
        GADI_ERROR("Fail to get interrupt of VI.");
        return retVal;
    }
    sleep(1);
    retVal = gadi_vi_get_interrupt(viHandle, &timesB);
    if (retVal != GADI_OK) {
        GADI_ERROR("Fail to get interrupt of VI.");
        return retVal;
    }
    // check
	snprintf(cmd, 128, "echo \"##### slowshutter framerate: %d, VI interrupt: %d.\" >> %s\n",
	    curFps, (int)(timesB - timesA), VI_REPORT_FILE_PATH);
    shell_system(cmd);
    if ((timesB - timesA) > 7 || (timesB - timesA) < 3) {
        shell_system("echo \"##### Fail to test slow shutter mode 4.\"    >> " \
            VI_REPORT_FILE_PATH);
    } else {
        shell_system("echo \"##### Success to test slow shutter mode 4.\" >> " \
            VI_REPORT_FILE_PATH);
    }

    // set slowshutter mode 0, revert fps
    preFps = GADI_VI_FPS_25;
    retVal = gadi_vi_set_framerate(viHandle, &preFps);
    if (retVal != GADI_OK) {
        GADI_ERROR("Fail to set slowshutter framerate.");
        return retVal;
    }

    shell_system("echo \"##### Finish to test slowshutter mode.\"         >> " \
        VI_REPORT_FILE_PATH);
    shell_system("echo \"=======================================\"        >> " \
        VI_REPORT_FILE_PATH);

    return retVal;
}

static GADI_ERR vi_auto_test_fps(void)
{
    GADI_ERR   retVal = GADI_OK;
    GADI_U32   preFps = 0;
    GADI_U32   curFps = 0;
    GADI_ULONG timesA = 0;
    GADI_ULONG timesB = 0;
    GADI_VI_SettingParamsT params;
    GADI_CHAR  cmd[128] = {0};

    if (!viHandle) {
        GADI_ERROR("Not init VI.");
        return -1;
    }

    shell_system("echo \"=======================================\"        >> " \
        VI_REPORT_FILE_PATH);
    shell_system("echo \"##### frame rate.\"                              >> " \
        VI_REPORT_FILE_PATH);
    shell_system("echo \"##### gadi_vi_get_params()\"                     >> " \
        VI_REPORT_FILE_PATH);
    shell_system("echo \"##### gadi_vi_set_framerate()\"                  >> " \
        VI_REPORT_FILE_PATH);

    // save
    retVal = gadi_vi_get_params(viHandle, &params);
    if (retVal != GADI_OK) {
        GADI_ERROR("Fail to gadi_vi_get_params.");
        return retVal;
    }
    preFps = params.frameRate;

    // set 15 fps
    curFps = GADI_VI_FPS_15;
    retVal = gadi_vi_set_framerate(viHandle, &curFps);
    if (retVal != GADI_OK) {
        GADI_ERROR("Fail to get interrupt of VI.");
        return retVal;
    }
    usleep(1);
    // check
    retVal = gadi_vi_get_interrupt(viHandle, &timesA);
    if (retVal != GADI_OK) {
        GADI_ERROR("Fail to get interrupt of VI.");
        return retVal;
    }
    sleep(1);
    retVal = gadi_vi_get_interrupt(viHandle, &timesB);
    if (retVal != GADI_OK) {
        GADI_ERROR("Fail to get interrupt of VI.");
        return retVal;
    }
	snprintf(cmd, 128, "echo \"##### fps: %d, VI interrupt: %d.\" >> %s\n",
	    curFps, (int)(timesB - timesA), VI_REPORT_FILE_PATH);
    shell_system(cmd);
    if ((timesB - timesA) > 17 || (timesB - timesA) < 13) {
        shell_system("echo \"##### Fail to test 15 fps.\"                 >> " \
            VI_REPORT_FILE_PATH);
    } else {
        shell_system("echo \"##### Success to test 15 fps.\"              >> " \
            VI_REPORT_FILE_PATH);
    }

    // set 5 fps
    curFps = GADI_VI_FPS_5;
    retVal = gadi_vi_set_framerate(viHandle, &curFps);
    if (retVal != GADI_OK) {
        GADI_ERROR("Fail to get interrupt of VI.");
        return retVal;
    }
    usleep(1);
    // check
    retVal = gadi_vi_get_interrupt(viHandle, &timesA);
    if (retVal != GADI_OK) {
        GADI_ERROR("Fail to get interrupt of VI.");
        return retVal;
    }
    sleep(1);
    retVal = gadi_vi_get_interrupt(viHandle, &timesB);
    if (retVal != GADI_OK) {
        GADI_ERROR("Fail to get interrupt of VI.");
        return retVal;
    }
	snprintf(cmd, 128, "echo \"##### fps: %d, VI interrupt: %d.\" >> %s\n",
	    curFps, (int)(timesB - timesA), VI_REPORT_FILE_PATH);
    shell_system(cmd);
    if ((timesB - timesA) > 7 || (timesB - timesA) < 3) {
        shell_system("echo \"##### Fail to test 5 fps.\"                  >> " \
            VI_REPORT_FILE_PATH);
    } else {
        shell_system("echo \"##### Success to test 5 fps.\"               >> " \
            VI_REPORT_FILE_PATH);
    }

    // revert fps
    retVal = gadi_vi_set_framerate(viHandle, &preFps);
    if (retVal != GADI_OK) {
        GADI_ERROR("Fail to revet fps of VI.");
        return retVal;
    }

    shell_system("echo \"##### Finish to test fps.\"                      >> " \
        VI_REPORT_FILE_PATH);
    shell_system("echo \"=======================================\"        >> " \
        VI_REPORT_FILE_PATH);

    return retVal;
}

static GADI_ERR vi_auto_test_ircut(void)
{
    GADI_ERR  retVal = GADI_OK;

    if (!viHandle) {
        GADI_ERROR("Not init VI.");
        return -1;
    }

    shell_system("echo \"=======================================\"        >> " \
        VI_REPORT_FILE_PATH);
    shell_system("echo \"##### ircut mode.\"                              >> " \
        VI_REPORT_FILE_PATH);
    shell_system("echo \"##### gadi_vi_set_ircut_control()\"              >> " \
        VI_REPORT_FILE_PATH);

    // night
    retVal = switch_ircut_then_cap(GADI_VI_IRCUT_NIGHT);
    if (retVal) {
        GADI_ERROR("Fail to switch day night mode.");
        return retVal;
    }

    // day
    retVal = switch_ircut_then_cap(GADI_VI_IRCUT_DAY);
    if (retVal) {
        GADI_ERROR("Fail to switch day day mode.");
        return retVal;
    }

    shell_system("echo \"##### Picture had been saved, please check.\"    >> " \
        VI_REPORT_FILE_PATH);
    shell_system("echo \"##### Finish to test ircut mode.\"               >> " \
        VI_REPORT_FILE_PATH);
    shell_system("echo \"=======================================\"        >> " \
        VI_REPORT_FILE_PATH);

    return retVal;
}


static GADI_ERR flip_then_cap(GADI_VI_MirrorModeParamsT *mirMode)
{
    GADI_ERR   retVal = GADI_OK;
    GADI_CHAR  viSnapPath[VI_PATH_MAX_LENTH] = {0};
    if (!mirMode) {
        GADI_ERROR("Bad params.");
        return -1;
    }

    retVal = gadi_vi_set_mirror_mode(viHandle, mirMode);
    if (retVal != GADI_OK) {
        GADI_ERROR("Fail to auto test H&V flip automatically.\n");
        return retVal;
    }
    sleep(1);

    switch (mirMode->mirrorPattern) {
        case GADI_VI_MIRROR_HORRIZONTALLY_VERTICALLY:
            snprintf(viSnapPath, VI_PATH_MAX_LENTH, "%s/%s", VI_MIRROR_PATH, "mirror_hv.jpeg");
            break;
        case GADI_VI_MIRROR_HORRIZONTALLY:
            snprintf(viSnapPath, VI_PATH_MAX_LENTH, "%s/%s", VI_MIRROR_PATH, "mirror_h.jpeg");
            break;
        case GADI_VI_MIRROR_VERTICALLY:
            snprintf(viSnapPath, VI_PATH_MAX_LENTH, "%s/%s", VI_MIRROR_PATH, "mirror_v.jpeg");
            break;
        case GADI_VI_MIRROR_NONE:
        case GADI_VI_MIRROR_AUTO:
        default:
            snprintf(viSnapPath, VI_PATH_MAX_LENTH, "%s/%s", VI_MIRROR_PATH, "mirror_n.jpeg");
            break;
    }
    remove(viSnapPath);

	retVal = gdm_venc_capture_jpeg(0, viSnapPath);
	if (retVal) {
        GADI_ERROR("Fail to capture %s.", viSnapPath);
        return retVal;
	}

    return retVal;
}

static GADI_ERR switch_ircut_then_cap(GADI_VI_IRModeEnumT irMode)
{
    GADI_ERR retVal = GADI_OK;
    GADI_CHAR  viSnapPath[VI_PATH_MAX_LENTH] = {0};
    retVal = gadi_vi_set_ircut_control(viHandle, irMode);
    if (retVal) {
        GADI_ERROR("gadi_vi_set_ircut_control failed.");
        return retVal;
    }
    sleep(2);
    // cap
    switch (irMode) {
        case GADI_VI_IRCUT_NIGHT:
            snprintf(viSnapPath, VI_PATH_MAX_LENTH, "%s/%s", VI_IRCUT_PATH, "ircut_night.jpeg");
            break;
        case GADI_VI_IRCUT_DAY:
        case GADI_VI_IRCUT_CLEAR:
        default:
            snprintf(viSnapPath, VI_PATH_MAX_LENTH, "%s/%s", VI_IRCUT_PATH, "ircut_day.jpeg");
            break;
    }
    remove(viSnapPath);

	retVal = gdm_venc_capture_jpeg(0, viSnapPath);
	if (retVal) {
        GADI_ERROR("Fail to capture %s.", viSnapPath);
        return retVal;
	}
    return retVal;
}

static void vi_usage(void)
{
    printf("\nusage: vi [OPTION]...\n");
    printf("\t-h, --help              help.\n"
           "\t-o, --operation         set operation mode.\n"
           "\t-s, --slowshutter       set slowshutter mode.\n"
           "\t-i, --ircut             set ircut.\n"
           "\t-F, --flip              set vi flip(0x0:means flip horizontal and flip vertical,\n"
           "\t                        0x01:means flip horizontal,0x02:means flip vertical,0x03:means none flip)\n"
           "\t-S, --auto test         auto test all VI func"
           "\t-P, --auto test         stop auto test"
           "\t-f, --framerate         set vi framerate.\n"
          );
	printf("\n");
}

static GADI_ERR handle_vi_command(int argc, char* argv[])
{
    GADI_U32 viFramerate;
    int option_index, ch;
	int retVal;

	optind = 1;

    /*change parameters when giving input options.*/
    while ((ch = getopt_long(argc, argv, viShortOptions, viLongOptions, &option_index)) != -1)
    {
        switch (ch)
        {
            ////////////////////////////////////////////////////////////////////////
            ////////////////////////////////////////////////////////////////////////
            case 'h':
            {
                vi_usage();
                break;
            }
            ////////////////////////////////////////////////////////////////////////
            ////////////////////////////////////////////////////////////////////////
            case 'o':
            {
    			retVal = gdm_vi_set_operation_mode(atoi(optarg));
                if (retVal != GADI_OK) {
                    goto bad_parameter;
                }

    			break;
            }
            ////////////////////////////////////////////////////////////////////////
            ////////////////////////////////////////////////////////////////////////
            case 's':
            {
                viFramerate = atoi(optarg);
    			retVal = gdm_vi_set_slowshutter_framerate(&viFramerate);
                if(retVal == GADI_OK) {
                    GADI_INFO("vi framerate %d\n", viFramerate);
                } else {
                    goto bad_parameter;
                }

    			break;
            }
            ////////////////////////////////////////////////////////////////////////
            ////////////////////////////////////////////////////////////////////////
            case 'i':
            {
                retVal = gdm_vi_set_ircut_control(atoi(optarg));
                if (retVal != GADI_OK) {
                    goto bad_parameter;
                }

    			break;
            }
            ////////////////////////////////////////////////////////////////////////
            ////////////////////////////////////////////////////////////////////////
            case 'F':
            {
                retVal = gdm_vi_set_flip(atoi(optarg));
                if (retVal != GADI_OK) {
                    goto bad_parameter;
                }
                isp_restart();

                break;
            }
            ////////////////////////////////////////////////////////////////////////
            ////////////////////////////////////////////////////////////////////////
            case 'f':
            {
                retVal = gdm_vi_set_framerate(atoi(optarg));
    		    if(retVal != GADI_OK){
                    goto bad_parameter;
        		}

    			break;
            }
            ////////////////////////////////////////////////////////////////////////
            ////////////////////////////////////////////////////////////////////////
            case 'S':
            {
        		gdm_vi_auto_test_init();
        		gdm_vi_auto_test();
    			break;
            }
            ////////////////////////////////////////////////////////////////////////
            ////////////////////////////////////////////////////////////////////////
            case 'P':
            {
        		gdm_vi_auto_test_exit();
    			break;
            }
            ////////////////////////////////////////////////////////////////////////
            ////////////////////////////////////////////////////////////////////////
            default:
                GADI_ERROR("bad params\n");
    			break;
        }
    }
	optind = 1;
    return 0;

bad_parameter:
	optind = 1;
    return -1;
}

