/******************************************************************************
** \file        adi/test/src/isp.c
**
** \brief       ADI layer ISP test.
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

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <getopt.h>


#include "isp.h"
#include "shell.h"
#include "parser.h"
#include "venc.h"
#include "ircut.h"
#include "adi_vi.h"

//*****************************************************************************
//*****************************************************************************
//** Local Defines
//*****************************************************************************
//*****************************************************************************
#define CHECK_ARG_COUNT(argc, req)  {if (argc < req) goto bad_parameter;}
#define TEST_OK(format, args...)    printf("\033[1;32m" format "\033[0m", ##args)
#define TEST_FAIL(format, args...)  printf("\033[1;31m" format "\033[0m", ##args)

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
extern GADI_SYS_HandleT viHandle;

//*****************************************************************************
//*****************************************************************************
//** Local Data
//*****************************************************************************
//*****************************************************************************
GADI_SYS_HandleT ispHandle   = NULL;
GADI_BOOL        ispStart    = GADI_FALSE;
static const char *shortOptions = "hIOECPSeMwsagmnrbpiolkcLxyURf";
static struct option longOptions[] =
{
    {"help",     0, 0, 'h'},
    {"init",     0, 0, 'I'},
    {"open",     0, 0, 'O'},
    {"exit",     0, 0, 'E'},
    {"close",    0, 0, 'C'},
    {"start",    0, 0, 'S'},
    {"stop",     0, 0, 'P'},
    {"ae",       0, 0, 'e'},
    {"me",       0, 0, 'M'},
    {"aw",       0, 0, 'w'},
    {"slow",     0, 0, 's'},
    {"antif",    0, 0, 'a'},
    {"gamma",    0, 0, 'g'},
    {"mw",       0, 0, 'm'},
    {"satur",    0, 0, 'n'},
    {"hue",      0, 0, 'r'},
    {"brigh",    0, 0, 'b'},
    {"sharpen",  0, 0, 'p'},
    {"shading",  0, 0, 'i'},
    {"denoise",  0, 0, 'o'},
    {"black",    0, 0, 'l'},
    {"back",     0, 0, 'k'},
    {"contrast", 0, 0, 'c'},
    {"look",     0, 0, 'L'},
    {"extype",   0, 0, 'x'},
    {"wbtype",   0, 0, 'y'},
    {"status",   0, 0, 'U'},
    {"reset",    0, 0, 'R'},
    {"fps",      0, 0, 'f'},
    {0,          0, 0, 0}
};
static GADI_U8 openFirst = 0;
static GADI_U8 ispFactor = 0;
static GADI_ISP_AntiFlickerParamT ispAntFlickerAttr;
static GADI_ISP_GammaAttrT ispGammaAttr;
static GADI_ISP_AwbAttrT ispAWAttr;
static GADI_ISP_MwbAttrT ispMWAttr;
static GADI_S32 ispSaturationAttr;
static GADI_ISP_ContrastAttrT ispContrastAttr;
static GADI_S32 ispHueAttr;
static GADI_S32 ispBrightnessAttr;
static GADI_ISP_SharpenAttrT ispSharpenAttr;
static GADI_ISP_ShadingAttrT ispShadingAttr;
static GADI_ISP_DenoiseAttrT ispDenoiseAttr;
static GADI_ISP_BlackLevelAttrT ispBlackLevelAttr;
static GADI_ISP_BacklightAttrT ispBackLightAttr;
static GADI_ISP_AeAttrT ispAEAttr;
static GADI_ISP_MeAttrT ispMEAttr;
static GADI_ISP_ExpTypeEnumT ispExpType;
static GADI_ISP_WbTypeEnumT ispWbType;
static GADI_U8 ispIsDay;
static GADI_S32 ispFps;
static char ispPrintStr[32];
static char *printStr = ispPrintStr;

//*****************************************************************************
//*****************************************************************************
//** Local Functions Declaration
//*****************************************************************************
//*****************************************************************************
static GADI_ERR isp_p_handle_cmd(int argc, char* argv[]);
static void isp_usage(void);
static void isp_nighttoday(GADI_U32 value);
static void isp_daytonight(GADI_U32 value);

static void isp_print_slow_framerate(void)
{
    GADI_INFO("[slow frametate] factor = %02x\n", ispFactor);
}

static void isp_print_antiflicker(void)
{
    GADI_INFO("[   antiflicker] enable: %s\n",
        ispAntFlickerAttr.enableDisable?"on":"off");
    GADI_INFO("[   antiflicker] freq: %s Hz\n",
        ispAntFlickerAttr.freq?"60":"50");
}

static void isp_print_gamma(void)
{
    GADI_U32 i,j;
    GADI_INFO("[         gamma] enable: %s\n",
        ispGammaAttr.enableDisable?"on":"off");
    GADI_INFO("[         gamma] table\n");
    for(i = 0; i < GADI_ISP_GAMMA_LUT_SIZE/16; i++) {
        printf("\t\t\t");
        for(j = 0; j < 16; j++) {
        printf("%02x ", ispGammaAttr.gammaTable[i*16 +j]);
        }
        printf("\n");
    }
}

static void isp_print_awb(void)
{
    static const char  colorTemp[][32] = {
        "GADI_ISP_AWB_COLOR_TEMP_AUTO",
        "GADI_ISP_AWB_COLOR_TEMP_2800",
        "GADI_ISP_AWB_COLOR_TEMP_3200",
        "GADI_ISP_AWB_COLOR_TEMP_4500",
        "GADI_ISP_AWB_COLOR_TEMP_5500",
        "GADI_ISP_AWB_COLOR_TEMP_6500",
        "GADI_ISP_AWB_COLOR_TEMP_7500",
        "GADI_ISP_AWB_COLOR_TEMP_NUM"};
    GADI_INFO("[           awb] color temp: %s\n",
        colorTemp[ispAWAttr.colorTemp]);
}

static void isp_print_mwb(void)
{
    GADI_INFO("[           mwb] gainRed   = %04x\n",ispMWAttr.gainRed);
    GADI_INFO("[           mwb] gainGreen = %04x\n",ispMWAttr.gainGreen);
    GADI_INFO("[           mwb] gainBlue  = %04x\n",ispMWAttr.gainBlue);
}

static void isp_print_saturation(void)
{
    GADI_INFO("[    saturation] saturation = %02x\n",ispSaturationAttr);
}

static void isp_print_contrast(void)
{
    GADI_INFO("[   antiflicker] enableAuto: %s\n",
        ispContrastAttr.enableAuto?"Auto":"Manual");
    GADI_INFO("[   antiflicker] manualStrength = %02x \n",
        ispContrastAttr.manualStrength);
    GADI_INFO("[   antiflicker] aotuStrength   = %02x \n",
        ispContrastAttr.autoStrength);
}

static void isp_print_hue(void)
{
    GADI_INFO("[           hue] hue = %02x\n", ispHueAttr);
}

static void isp_print_brightness(void)
{
    GADI_INFO("[    brightness] brightness = %02x\n", ispBrightnessAttr);
}

static void isp_print_sharpen(void)
{
    GADI_INFO("[       sharpen] enable: %s\n",
        ispSharpenAttr.enableDisable?"on":"off");
    GADI_INFO("[       sharpen] enableManual = %s \n",
        ispSharpenAttr.enableManual?"enable":"disable");
    GADI_INFO("[       sharpen] level   = %02x \n",
        ispSharpenAttr.level);
}

static void isp_print_shading(void)
{
    GADI_INFO("[       shading] enable: %s\n",
        ispShadingAttr.enableDisable?"on":"off");
}

static void isp_print_denoise(void)
{
    GADI_INFO("[       denoise] enable: %s\n",
        ispDenoiseAttr.enableDisable?"on":"off");
    GADI_INFO("[       denoise] level   = %02x \n",
        ispDenoiseAttr.level);
}

static void isp_print_blacklevel(void)
{
    GADI_INFO("[    blacklevel] blackLevel[0] = %02x \n",
        ispBlackLevelAttr.blackLevel[0]);
    GADI_INFO("[    blacklevel] blackLevel[1] = %02x \n",
        ispBlackLevelAttr.blackLevel[1]);
    GADI_INFO("[    blacklevel] blackLevel[2] = %02x \n",
        ispBlackLevelAttr.blackLevel[2]);
    GADI_INFO("[    blacklevel] blackLevel[3] = %02x \n",
        ispBlackLevelAttr.blackLevel[3]);
}

static void isp_print_backlight(void)
{
    GADI_INFO("[     backlight] enable: %s\n",
        ispBackLightAttr.enableDisable?"on":"off");
    GADI_INFO("[     backlight] level   = %02x \n",
        ispBackLightAttr.level);
}

static void isp_print_ae(void)
{
    GADI_INFO("[            ae] speed   = %02x \n",ispAEAttr.speed);
    GADI_INFO("[            ae] shutmin = %d \n",ispAEAttr.shutterTimeMin);
    GADI_INFO("[            ae] shutmax = %d \n",ispAEAttr.shutterTimeMax);
    GADI_INFO("[            ae] gainmin = %02x \n",ispAEAttr.gainMin);
    GADI_INFO("[            ae] gainmax = %02x \n",ispAEAttr.gainMax);
    GADI_INFO("[            ae] tagetRatio = %02x \n",ispAEAttr.tagetRatio);
}

static void isp_print_me(void)
{
    GADI_INFO("[            me] gain    = %02x \n",ispMEAttr.gain);
    GADI_INFO("[            me] shutmin = %d \n",ispMEAttr.shutterTime);
}

static void isp_print_shutter(void)
{
    double shutter = 0.0;
    gadi_isp_get_current_shutter(ispHandle, &shutter);
    GADI_INFO("[            ae] shutter = %lf \n", shutter);
}

static void isp_print_gain(void)
{
    GADI_U32 gain = 0;
    gadi_isp_get_current_gain(ispHandle, &gain);
    GADI_INFO("[            ae] gain = %d \n", gain);
}

static void isp_print_sensor_model(void)
{
    GADI_ERR retVal = GADI_OK;
    GADI_ISP_SensorModelEnumT model;
    retVal = gadi_isp_get_sensor_model(ispHandle, &model);
    if(retVal != GADI_OK)
        GADI_ERROR("gadi_isp_get_sensor_model faill\n");
    else{
        switch(model)
        {
            case GADI_ISP_SENSOR_UNKNOWN:
                GADI_INFO("[        sensor] model: %s \n", "Unkown");
                break;
            case GADI_ISP_SENSOR_AR0130:
                GADI_INFO("[        sensor] model: %s \n", "AR0130");
                break;
            case GADI_ISP_SENSOR_AR0330:
                GADI_INFO("[        sensor] model: %s \n", "AR0330");
                break;
            case GADI_ISP_SENSOR_GC1004:
                GADI_INFO("[        sensor] model: %s \n", "GC1004");
                break;
//add by WRD
           case GADI_ISP_SENSOR_GC0328:
                GADI_INFO("[        sensor] model: %s \n", "GC0328");
                break;
//end
            case GADI_ISP_SENSOR_IMX122:
                GADI_INFO("[        sensor] model: %s \n", "IMX222");
                break;
            case GADI_ISP_SENSOR_IMX238:
                GADI_INFO("[        sensor] model: %s \n", "IMX238");
                break;
            case GADI_ISP_SENSOR_OV2710:
                GADI_INFO("[        sensor] model: %s \n", "OV2710");
                break;
            case GADI_ISP_SENSOR_OV9710:
                GADI_INFO("[        sensor] model: %s \n", "OV9710");
                break;
            case GADI_ISP_SENSOR_JXH42:
                GADI_INFO("[        sensor] model: %s \n", "JXH42");
                break;
//add by WRD
			case GADI_ISP_SENSOR_JXH65:
                GADI_INFO("[        sensor] model: %s \n", "JXH65");
                break;
      case GADI_ISP_SENSOR_SC1235:
                GADI_INFO("[        sensor] model: %s \n", "SC1235");
                break;
//end
            case GADI_ISP_SENSOR_BG0701:
                GADI_INFO("[        sensor] model: %s \n", "BG0701");
                break;
            default:
                GADI_INFO("[        sensor] model: %s \n", "Unkown");
                break;

        }
    }
}


static void isp_print_all(void)
{
    isp_print_antiflicker();
    isp_print_awb();
    isp_print_backlight();
    isp_print_blacklevel();
    isp_print_brightness();
    isp_print_contrast();
    isp_print_denoise();
    isp_print_gamma();
    isp_print_hue();
    isp_print_mwb();
    isp_print_saturation();
    isp_print_shading();
    isp_print_sharpen();
    isp_print_slow_framerate();
    isp_print_ae();
    isp_print_me();
    isp_print_shutter();
    isp_print_gain();
    isp_print_sensor_model();
}

//*****************************************************************************
//*****************************************************************************
//** API Functions
//*****************************************************************************
//*****************************************************************************
GADI_ERR isp_init(void)
{
    GADI_ERR errorCode = GADI_OK;

    errorCode = gadi_isp_init();
    GADI_INFO("gadi_isp_init ok\n");

    return errorCode;
}


GADI_ERR isp_exit(void)
{
    return gadi_isp_exit();
}

GADI_ERR isp_open(void)
{
    GADI_ERR             errorCode = GADI_OK;
    GADI_ISP_OpenParamsT openParams;

    if (openFirst == 0) {
        gadi_sys_memset(&openParams, 0, sizeof(GADI_ISP_OpenParamsT));
        openParams.denoiseMode = GADI_ISP_VPS_MERGE_MODE;
        ispHandle = gadi_isp_open(&openParams, &errorCode);
        if (ispHandle == NULL || errorCode != GADI_OK) {
            GADI_ERROR("gadi_isp_open error %d\n", errorCode);
        }

        ircut_register_daytonight(0, &isp_daytonight);
        ircut_register_nighttoday(0, &isp_nighttoday);
        openFirst = 1;
    }
    return errorCode;
}

GADI_ERR isp_close(void)
{
    if (openFirst == 1) {
        openFirst = 0;
        return gadi_isp_close(ispHandle);
    }
    return 0;
}


GADI_ERR isp_start(void)
{
    GADI_ERR        errorCode = GADI_OK;

    errorCode = gadi_isp_load_param(ispHandle, NULL);
    if (errorCode != GADI_OK)
    {
        if (GADI_ISP_ERR_FEATURE_NOT_SUPPORTED != errorCode)
            GADI_ERROR("gadi_isp_load_param() failed! ret = %d\n", errorCode);
        else {
            GADI_INFO("Current sensor no support ISP function.\n");
            return errorCode;
        }
    }

    errorCode = gadi_isp_start(ispHandle);
    if (errorCode != GADI_OK)
    {
        GADI_ERROR("gadi_isp_start() failed! ret = %d\n", errorCode);
    }
    ispStart = GADI_TRUE;
    return errorCode;
}

GADI_ERR isp_restart(void)
{
    GADI_ERR        errorCode = GADI_OK;

    if (ispHandle == NULL)
        return GADI_FALSE;
    errorCode = gadi_isp_reset_3a_static(ispHandle);
    if (errorCode != GADI_OK)
    {
        GADI_ERROR("gadi_isp_reset_3a_static() failed! ret = %d\n", errorCode);
    }
    else
    {
        GADI_INFO("gadi_isp_reset_3a_static ok\n");
    }

    return errorCode;
}

GADI_ERR isp_stop(void)
{
    GADI_ERR        errorCode = GADI_OK;

    errorCode = gadi_isp_stop(ispHandle);
    if (errorCode != GADI_OK)
    {
        GADI_ERROR("gadi_isp_start() failed! ret = %d\n", errorCode);
    }
    GADI_INFO("gadi_isp_stop ok\n");

    ispStart = GADI_FALSE;
    return errorCode;
}


GADI_ERR isp_register_testcase(void)
{
    GADI_ERR   retVal =  GADI_OK;

    (void)shell_registercommand (
        "isp",
        isp_p_handle_cmd,
        "isp command",
        "---------------------------------------------------------------------\n"
        "   Need help, please input '#isp -h'.\n"
        "---------------------------------------------------------------------\n"
        /******************************************************************/
    );

    return retVal;
}

GADI_ERR isp_custom_code(void)
{
    // TODO:  Add custom code
    /* example:
        if(sensorModel == GADI_ISP_SENSOR_IMX122)   //imx222
        {
            gadi_isp_get_ae_attr(ispHandle, &aeAttr);
            aeAttr.shutterTimeMin = 8000;//1024;
            aeAttr.shutterTimeMax = GADI_VI_FPS_10;
            aeAttr.gainMax        = 30;
            aeAttr.gainMin        = 1;
            contrastAttr.enableAuto     = 1;
            contrastAttr.manualStrength = 128;
            contrastAttr.autoStrength   = 96;//96;
            gadi_isp_set_contrast_attr(ispHandle, &contrastAttr);
            gadi_isp_set_ae_attr(ispHandle, &aeAttr);
        }
        */
//add by WRD
    GADI_ISP_SensorModelEnumT sensorModel;
    GADI_ISP_AeAttrT    aeAttr;
    GADI_ISP_ContrastAttrT contrastAttr;

	gadi_isp_get_sensor_model(ispHandle,&sensorModel);

    //gadi_isp_set_day_night_mode(0);

    gadi_isp_set_meter_mode(3);

	if(sensorModel == GADI_ISP_SENSOR_GC0328)	//GC0328
	{
		gadi_isp_get_ae_attr(ispHandle, &aeAttr);
		aeAttr.shutterTimeMin = 8000;//1024;
		aeAttr.shutterTimeMax = GADI_VI_FPS_25;
		aeAttr.gainMax		  = 36;
		aeAttr.gainMin		  = 1;
		contrastAttr.enableAuto 	= 1;
		contrastAttr.manualStrength = 128;
		contrastAttr.autoStrength	= 72;//72;
		gadi_isp_set_contrast_attr(ispHandle, &contrastAttr);
		gadi_isp_set_ae_attr(ispHandle, &aeAttr);
	}
	else if (sensorModel == GADI_ISP_SENSOR_JXH65)	//JXH65
	{
		gadi_isp_get_ae_attr(ispHandle, &aeAttr);
		aeAttr.shutterTimeMin = 8000;//1024;
		aeAttr.shutterTimeMax = GADI_VI_FPS_25;
		aeAttr.gainMax		  = 48;
		aeAttr.gainMin		  = 1;
		contrastAttr.enableAuto 	= 1;
		contrastAttr.manualStrength = 128;
		contrastAttr.autoStrength	= 64;//72;
		gadi_isp_set_contrast_attr(ispHandle, &contrastAttr);
		gadi_isp_set_ae_attr(ispHandle, &aeAttr);
	}
	else if (sensorModel == GADI_ISP_SENSOR_SC1235)	//SC1235
	{
		gadi_isp_get_ae_attr(ispHandle, &aeAttr);
		aeAttr.shutterTimeMin = 8000;//1024;
		aeAttr.shutterTimeMax = GADI_VI_FPS_25;
		aeAttr.gainMax		  = 48;
		aeAttr.gainMin		  = 1;
		contrastAttr.enableAuto 	= 1;
		contrastAttr.manualStrength = 128;
		contrastAttr.autoStrength	= 64;//72;
		gadi_isp_set_contrast_attr(ispHandle, &contrastAttr);
		gadi_isp_set_ae_attr(ispHandle, &aeAttr);
	}
	else if (sensorModel == GADI_ISP_SENSOR_JXK02)	//JXK02
	{
		gadi_isp_get_ae_attr(ispHandle, &aeAttr);
		aeAttr.shutterTimeMin = 8000;//1024;
		aeAttr.shutterTimeMax = GADI_VI_FPS_15;
		aeAttr.gainMax		  = 36;
		aeAttr.gainMin		  = 1;
		contrastAttr.enableAuto 	= 1;
		contrastAttr.manualStrength = 128;
		contrastAttr.autoStrength	= 64;//72;
		gadi_isp_set_contrast_attr(ispHandle, &contrastAttr);
		gadi_isp_set_ae_attr(ispHandle, &aeAttr);
	}
	else if (sensorModel == GADI_ISP_SENSOR_JXK03)	//JXK03
	{
		gadi_isp_get_ae_attr(ispHandle, &aeAttr);
		aeAttr.shutterTimeMin = 8000;//1024;
		aeAttr.shutterTimeMax = GADI_VI_FPS_15;
		aeAttr.gainMax		  = 36;
		aeAttr.gainMin		  = 1;
		contrastAttr.enableAuto 	= 1;
		contrastAttr.manualStrength = 128;
		contrastAttr.autoStrength	= 64;//72;
		gadi_isp_set_contrast_attr(ispHandle, &contrastAttr);
		gadi_isp_set_ae_attr(ispHandle, &aeAttr);
	}
	else if (sensorModel == GADI_ISP_SENSOR_SC3035)	//SC3035
	{
		gadi_isp_get_ae_attr(ispHandle, &aeAttr);
		aeAttr.shutterTimeMin = 8000;//1024;
		aeAttr.shutterTimeMax = GADI_VI_FPS_20;
		aeAttr.gainMax		  = 42;
		aeAttr.gainMin		  = 1;
		contrastAttr.enableAuto 	= 1;
		contrastAttr.manualStrength = 128;
		contrastAttr.autoStrength	= 64;//72;
		gadi_isp_set_contrast_attr(ispHandle, &contrastAttr);
		gadi_isp_set_ae_attr(ispHandle, &aeAttr);
	}
    else if (sensorModel == GADI_ISP_SENSOR_GC1034)	//GC1034
	{
		gadi_isp_get_ae_attr(ispHandle, &aeAttr);
		aeAttr.shutterTimeMin = 8000;//1024;
		aeAttr.shutterTimeMax = GADI_VI_FPS_25;
		aeAttr.gainMax		  = 36;
		aeAttr.gainMin		  = 1;
		contrastAttr.enableAuto 	= 1;
		contrastAttr.manualStrength = 128;
		contrastAttr.autoStrength	= 64;//72;
		gadi_isp_set_contrast_attr(ispHandle, &contrastAttr);
		gadi_isp_set_ae_attr(ispHandle, &aeAttr);
	}
    else	//other
	{
		gadi_isp_get_ae_attr(ispHandle, &aeAttr);
		aeAttr.shutterTimeMin = 8000;//1024;
		aeAttr.shutterTimeMax = GADI_VI_FPS_25;
		aeAttr.gainMax		  = 33;
		aeAttr.gainMin		  = 1;
		contrastAttr.enableAuto 	= 1;
		contrastAttr.manualStrength = 128;
		contrastAttr.autoStrength	= 64;//72;
		gadi_isp_set_contrast_attr(ispHandle, &contrastAttr);
		gadi_isp_set_ae_attr(ispHandle, &aeAttr);
	}

//end
    return 0;
}

//*****************************************************************************
//*****************************************************************************
//** Local Functions
//*****************************************************************************
//*****************************************************************************
static void isp_daytonight(GADI_U32 value)
{
    GADI_ISP_SensorModelEnumT sensorModel;
    GADI_ISP_AeAttrT    aeAttr;
    GADI_ISP_ContrastAttrT contrastAttr;

	gadi_isp_get_sensor_model(ispHandle,&sensorModel);

    gadi_isp_set_day_night_mode(0);
//added by Hu Yin
    gadi_isp_set_meter_mode(3);

    if(sensorModel == GADI_ISP_SENSOR_IMX122)   //imx222
    {
        gadi_isp_get_ae_attr(ispHandle, &aeAttr);
        aeAttr.shutterTimeMin = 8000;//1024;
        aeAttr.shutterTimeMax = GADI_VI_FPS_10;
        aeAttr.gainMax        = 30;
        aeAttr.gainMin        = 1;
        contrastAttr.enableAuto     = 1;
        contrastAttr.manualStrength = 128;
        contrastAttr.autoStrength   = 96;//96;
        gadi_isp_set_contrast_attr(ispHandle, &contrastAttr);
        gadi_isp_set_ae_attr(ispHandle, &aeAttr);
    }
//add by WRD
    else if(sensorModel == GADI_ISP_SENSOR_GC0328)   //gc0328
    {
        gadi_isp_get_ae_attr(ispHandle, &aeAttr);
        aeAttr.shutterTimeMin = 8000;//1024;
        aeAttr.shutterTimeMax = GADI_VI_FPS_10;
        aeAttr.gainMax        = 36;
        aeAttr.gainMin        = 1;
        contrastAttr.enableAuto     = 1;
        contrastAttr.manualStrength = 128;
        contrastAttr.autoStrength   = 72;//96;
        gadi_isp_set_contrast_attr(ispHandle, &contrastAttr);
        gadi_isp_set_ae_attr(ispHandle, &aeAttr);
    }
    else if(sensorModel == GADI_ISP_SENSOR_JXH65)   //jxh65
    {
        gadi_isp_get_ae_attr(ispHandle, &aeAttr);
        aeAttr.shutterTimeMin = 8000;//1024;
        aeAttr.shutterTimeMax = GADI_VI_FPS_10;
        aeAttr.gainMax        = 48;
        aeAttr.gainMin        = 1;
        contrastAttr.enableAuto     = 1;
        contrastAttr.manualStrength = 128;
        contrastAttr.autoStrength   = 64;//96;
        gadi_isp_set_contrast_attr(ispHandle, &contrastAttr);
        gadi_isp_set_ae_attr(ispHandle, &aeAttr);
    }
    else if(sensorModel == GADI_ISP_SENSOR_SC1235)   //SC1235
    {
        gadi_isp_get_ae_attr(ispHandle, &aeAttr);
        aeAttr.shutterTimeMin = 8000;//1024;
        aeAttr.shutterTimeMax = GADI_VI_FPS_10;
        aeAttr.gainMax        = 48;
        aeAttr.gainMin        = 1;
        contrastAttr.enableAuto     = 1;
        contrastAttr.manualStrength = 128;
        contrastAttr.autoStrength   = 64;//96;
        gadi_isp_set_contrast_attr(ispHandle, &contrastAttr);
        gadi_isp_set_ae_attr(ispHandle, &aeAttr);
    }
    else if(sensorModel == GADI_ISP_SENSOR_JXK02)   //JXK02
    {
        gadi_isp_get_ae_attr(ispHandle, &aeAttr);
        aeAttr.shutterTimeMin = 8000;//1024;
        aeAttr.shutterTimeMax = GADI_VI_FPS_15;
        aeAttr.gainMax        = 36;
        aeAttr.gainMin        = 1;
        contrastAttr.enableAuto     = 1;
        contrastAttr.manualStrength = 128;
        contrastAttr.autoStrength   = 64;//96;
        gadi_isp_set_contrast_attr(ispHandle, &contrastAttr);
        gadi_isp_set_ae_attr(ispHandle, &aeAttr);
    }
    else if(sensorModel == GADI_ISP_SENSOR_JXK03)   //JXK03
    {
        gadi_isp_get_ae_attr(ispHandle, &aeAttr);
        aeAttr.shutterTimeMin = 8000;//1024;
        aeAttr.shutterTimeMax = GADI_VI_FPS_15;
        aeAttr.gainMax        = 36;
        aeAttr.gainMin        = 1;
        contrastAttr.enableAuto     = 1;
        contrastAttr.manualStrength = 128;
        contrastAttr.autoStrength   = 64;//96;
        gadi_isp_set_contrast_attr(ispHandle, &contrastAttr);
        gadi_isp_set_ae_attr(ispHandle, &aeAttr);
    }
    else if(sensorModel == GADI_ISP_SENSOR_SC3035)   //SC3035
    {
        gadi_isp_get_ae_attr(ispHandle, &aeAttr);
        aeAttr.shutterTimeMin = 8000;//1024;
        aeAttr.shutterTimeMax = GADI_VI_FPS_10;
        aeAttr.gainMax        = 42;
        aeAttr.gainMin        = 1;
        contrastAttr.enableAuto     = 1;
        contrastAttr.manualStrength = 128;
        contrastAttr.autoStrength   = 64;//96;
        gadi_isp_set_contrast_attr(ispHandle, &contrastAttr);
        gadi_isp_set_ae_attr(ispHandle, &aeAttr);
    }
    else//others
    {
         gadi_isp_get_ae_attr(ispHandle, &aeAttr);
         aeAttr.shutterTimeMin = 8000;//1024;
         aeAttr.shutterTimeMax = GADI_VI_FPS_10;
         aeAttr.gainMax        = 33;
         aeAttr.gainMin        = 1;
         contrastAttr.enableAuto     = 1;
         contrastAttr.manualStrength = 128;
         contrastAttr.autoStrength   = 64;//96;
         gadi_isp_set_contrast_attr(ispHandle, &contrastAttr);
         gadi_isp_set_ae_attr(ispHandle, &aeAttr);
     }
//end

}

static void isp_nighttoday(GADI_U32 value)
{
    GADI_ISP_AeAttrT    aeAttr;
    GADI_ISP_ContrastAttrT contrastAttr;
    GADI_ISP_SensorModelEnumT sensorModel;

	gadi_isp_get_sensor_model(ispHandle,&sensorModel);

    gadi_isp_set_day_night_mode(1);

    gadi_isp_set_meter_mode(2);

    if(sensorModel == GADI_ISP_SENSOR_IMX122)   //imx222
    {
        gadi_isp_get_ae_attr(ispHandle, &aeAttr);
        aeAttr.shutterTimeMin = 8000;//1024;
        aeAttr.shutterTimeMax = 25;
        aeAttr.gainMax        = 36;
        aeAttr.gainMin        = 1;
        contrastAttr.enableAuto     = 1;
        contrastAttr.manualStrength = 128;
        contrastAttr.autoStrength   = 96;//96;
        gadi_isp_set_contrast_attr(ispHandle, &contrastAttr);
        gadi_isp_set_ae_attr(ispHandle, &aeAttr);
    }
//add by WRD
    else if(sensorModel == GADI_ISP_SENSOR_GC0328)   //gc0328
    {
        gadi_isp_get_ae_attr(ispHandle, &aeAttr);
        aeAttr.shutterTimeMin = 8000;//1024;
        aeAttr.shutterTimeMax = GADI_VI_FPS_25;
        aeAttr.gainMax        = 36;
        aeAttr.gainMin        = 1;
        contrastAttr.enableAuto     = 1;
        contrastAttr.manualStrength = 128;
        contrastAttr.autoStrength   = 72;//96;
        gadi_isp_set_contrast_attr(ispHandle, &contrastAttr);
        gadi_isp_set_ae_attr(ispHandle, &aeAttr);
    }
    else if(sensorModel == GADI_ISP_SENSOR_JXH65)   //jxh65
    {
        gadi_isp_get_ae_attr(ispHandle, &aeAttr);
        aeAttr.shutterTimeMin = 8000;//1024;
        aeAttr.shutterTimeMax = GADI_VI_FPS_25;
        aeAttr.gainMax        = 48;
        aeAttr.gainMin        = 1;
        contrastAttr.enableAuto     = 1;
        contrastAttr.manualStrength = 128;
        contrastAttr.autoStrength   = 64;//96;
        gadi_isp_set_contrast_attr(ispHandle, &contrastAttr);
        gadi_isp_set_ae_attr(ispHandle, &aeAttr);
    }
     else if(sensorModel == GADI_ISP_SENSOR_SC1235)   //SC1235
    {
        gadi_isp_get_ae_attr(ispHandle, &aeAttr);
        aeAttr.shutterTimeMin = 8000;//1024;
        aeAttr.shutterTimeMax = GADI_VI_FPS_25;
        aeAttr.gainMax        = 48;
        aeAttr.gainMin        = 1;
        contrastAttr.enableAuto     = 1;
        contrastAttr.manualStrength = 128;
        contrastAttr.autoStrength   = 64;//96;
        gadi_isp_set_contrast_attr(ispHandle, &contrastAttr);
        gadi_isp_set_ae_attr(ispHandle, &aeAttr);
    }
    else if(sensorModel == GADI_ISP_SENSOR_JXK02)   //JXK02
    {
        gadi_isp_get_ae_attr(ispHandle, &aeAttr);
        aeAttr.shutterTimeMin = 8000;//1024;
        aeAttr.shutterTimeMax = GADI_VI_FPS_15;
        aeAttr.gainMax        = 36;
        aeAttr.gainMin        = 1;
        contrastAttr.enableAuto     = 1;
        contrastAttr.manualStrength = 128;
        contrastAttr.autoStrength   = 64;//96;
        gadi_isp_set_contrast_attr(ispHandle, &contrastAttr);
        gadi_isp_set_ae_attr(ispHandle, &aeAttr);
    }
    else if(sensorModel == GADI_ISP_SENSOR_JXK03)   //JXK03
    {
        gadi_isp_get_ae_attr(ispHandle, &aeAttr);
        aeAttr.shutterTimeMin = 8000;//1024;
        aeAttr.shutterTimeMax = GADI_VI_FPS_15;
        aeAttr.gainMax        = 36;
        aeAttr.gainMin        = 1;
        contrastAttr.enableAuto     = 1;
        contrastAttr.manualStrength = 128;
        contrastAttr.autoStrength   = 64;//96;
        gadi_isp_set_contrast_attr(ispHandle, &contrastAttr);
        gadi_isp_set_ae_attr(ispHandle, &aeAttr);
    }
     else if(sensorModel == GADI_ISP_SENSOR_SC3035)   //SC3035
    {
        gadi_isp_get_ae_attr(ispHandle, &aeAttr);
        aeAttr.shutterTimeMin = 8000;//1024;
        aeAttr.shutterTimeMax = GADI_VI_FPS_20;
        aeAttr.gainMax        = 42;
        aeAttr.gainMin        = 1;
        contrastAttr.enableAuto     = 1;
        contrastAttr.manualStrength = 128;
        contrastAttr.autoStrength   = 64;//96;
        gadi_isp_set_contrast_attr(ispHandle, &contrastAttr);
        gadi_isp_set_ae_attr(ispHandle, &aeAttr);
    }
    else//others
    {
         gadi_isp_get_ae_attr(ispHandle, &aeAttr);
         aeAttr.shutterTimeMin = 8000;//1024;
         aeAttr.shutterTimeMax = GADI_VI_FPS_25;
         aeAttr.gainMax        = 33;
         aeAttr.gainMin        = 1;
         contrastAttr.enableAuto     = 1;
         contrastAttr.manualStrength = 128;
         contrastAttr.autoStrength   = 64;//96;
         gadi_isp_set_contrast_attr(ispHandle, &contrastAttr);
         gadi_isp_set_ae_attr(ispHandle, &aeAttr);
     }
//end
}

static void isp_usage(void)
{
    printf("Usage:\n");
        printf("\t-h,--help         print usage manual.\n");
        printf("\t-I,--init         init isp.\n");
        printf("\t-E,--exit         exit isp.\n");
        printf("\t-O,--open         open isp.\n");
        printf("\t-C,--close        close isp.\n");
        printf("\t-S,--start        start isp.\n");
        printf("\t-P,--stop         stop isp.\n");

        printf("\t-s,--slow         set isp slow framerate factor.\n");
        printf("\t-a,--antiflicker  set isp antiflicker req params.\n");
        printf("\t-g,--gamma        set isp gamma.\n");
        printf("\t-w,--aw           set isp AWB color tmp params.\n");
        printf("\t-m,--mw           set isp MWB gain params.\n");
        printf("\t-e,--ae           set isp AE params.\n");
        printf("\t-M,--me           set isp ME gain params.\n");
        printf("\t-n,--saturation   set isp saturation params.\n");
        printf("\t-c,--contrast     set isp contrast params.\n");
        printf("\t-r,--hue          set isp hue params.\n");
        printf("\t-b,--brightness   set isp brightness params.\n");
        printf("\t-p,--sharpen      set isp sharpen params.\n");
        printf("\t-i,--shading      set isp shading params.\n");
        printf("\t-o,--denoise      set isp denoise params.\n");
        printf("\t-l,--blacklevel   set isp blacklevel params.\n");
        printf("\t-k,--backlight    set isp backlight params.\n");
        printf("\t-x,--extype       set exposure type.\n");
        printf("\t-y,--wbtype       set white blance type.\n");
        printf("\t-U,--status       set day or night work model.\n");
        printf("\t-f,--fps          set vin fps.\n");
        printf("\t-R,--reset        reset ISP.\n");
        printf("\t-L,--look         look isp params.\n");
        printf("\t-t,--test         test isp API and function.\n");
}
/*
    isp -s
*/
static GADI_ERR isp_p_handle_cmd(int argc, char* argv[])
{
    GADI_ERR errorCode = GADI_OK;
    GADI_S32 option_index = 0;
    GADI_S8  ch;

    optind = 1;
    while ((ch = getopt_long(argc, argv, shortOptions, longOptions, &option_index)) != -1)
    {
    switch (ch)
    {
    case 'h':
        isp_usage();
        break;
    case 'I':
        errorCode = isp_init();
        if(errorCode != GADI_OK) {
            GADI_ERROR("init isp process fail\n");
            goto exit;
        }
        GADI_INFO("init isp process ok\n");
        break;
    case 'O':
        errorCode = isp_open();
        if(errorCode != GADI_OK) {
            GADI_ERROR("open isp process fail\n");
            goto exit;
        }

        GADI_INFO("open isp process ok\n");
        break;
    case 'E':
        errorCode = isp_exit();
        if(errorCode != GADI_OK) {
            GADI_ERROR("exit isp process fail\n");
            goto exit;
        }
        GADI_INFO("exit isp process ok\n");
        break;
    case 'C':
        errorCode = isp_close();
        if(errorCode != GADI_OK) {
            GADI_ERROR("close isp process fail\n");
            goto exit;
        }
        GADI_INFO("close isp process ok\n");
        break;
    case 'S':
        errorCode = isp_start();
        if(errorCode != GADI_OK) {
            GADI_ERROR("start isp process fail\n");
            goto exit;
        }

        errorCode = gadi_isp_get_slow_framerate(ispHandle, &ispFactor);
        if(errorCode != GADI_OK) {
            GADI_ERROR("gadi_isp_get_slow_framerate fail\n");
        }
        GADI_INFO("gadi_isp_get_slow_framerate ok\n");
        errorCode = gadi_isp_get_antiflicker(ispHandle, &ispAntFlickerAttr);
        if(errorCode != GADI_OK) {
            GADI_ERROR("gadi_isp_get_antiflicker fail\n");
        }
        GADI_INFO("gadi_isp_get_antiflicker ok\n");
        errorCode = gadi_isp_get_gamma_attr(ispHandle, &ispGammaAttr);
        if(errorCode != GADI_OK) {
            GADI_ERROR("gadi_isp_get_gamma_attr fail\n");
        }
        GADI_INFO("gadi_isp_get_gamma_attr ok\n");

        errorCode = gadi_isp_get_saturation(ispHandle, &ispSaturationAttr);
        if(errorCode != GADI_OK) {
            GADI_ERROR("gadi_isp_get_saturation fail\n");
        }
        GADI_INFO("gadi_isp_get_saturation ok\n");
        errorCode = gadi_isp_get_contrast_attr(ispHandle, &ispContrastAttr);
        if(errorCode != GADI_OK) {
            GADI_ERROR("gadi_isp_get_contrast_attr fail\n");
        }
        GADI_INFO("gadi_isp_get_contrast_attr ok\n");
        errorCode = gadi_isp_get_hue(ispHandle, &ispHueAttr);
        if(errorCode != GADI_OK) {
            GADI_ERROR("gadi_isp_get_hue fail\n");
        }
        GADI_INFO("gadi_isp_get_hue ok\n");
        errorCode = gadi_isp_get_brightness(ispHandle, &ispBrightnessAttr);
        if(errorCode != GADI_OK) {
            GADI_ERROR("gadi_isp_get_brightness fail\n");
        }
        GADI_INFO("gadi_isp_get_brightness ok\n");
        errorCode = gadi_isp_get_sharpen_attr(ispHandle, &ispSharpenAttr);
        if(errorCode != GADI_OK) {
            GADI_ERROR("gadi_isp_get_sharpen_attr fail\n");
        }
        GADI_INFO("gadi_isp_get_sharpen_attr ok\n");
        errorCode = gadi_isp_get_shading_attr(ispHandle, &ispShadingAttr);
        if(errorCode != GADI_OK) {
            GADI_ERROR("gadi_isp_get_shading_attr fail\n");
        }
        GADI_INFO("gadi_isp_get_shading_attr ok\n");
        errorCode = gadi_isp_get_denoise_attr(ispHandle, &ispDenoiseAttr);
        if(errorCode != GADI_OK) {
            GADI_ERROR("gadi_isp_get_denoise_attr fail\n");
        }
        GADI_INFO("gadi_isp_get_denoise_attr ok\n");
        errorCode = gadi_isp_get_blacklevel_attr(ispHandle, &ispBlackLevelAttr);
        if(errorCode != GADI_OK) {
            GADI_ERROR("gadi_isp_get_blacklevel_attr fail\n");
        }
        GADI_INFO("gadi_isp_get_blacklevel_attr ok\n");
        errorCode = gadi_isp_get_backlight_attr(ispHandle, &ispBackLightAttr);
        if(errorCode != GADI_OK) {
            GADI_ERROR("gadi_isp_get_backlight_attr fail\n");
        }
        GADI_INFO("gadi_isp_get_backlight_attr ok\n");

        errorCode = gadi_isp_get_awb_attr(ispHandle, &ispAWAttr);
        if(errorCode != GADI_OK) {
            GADI_ERROR("gadi_isp_get_awb_attr fail\n");
        }
        GADI_INFO("gadi_isp_get_awb_attr ok\n");
        errorCode = gadi_isp_get_mwb_attr(ispHandle, &ispMWAttr);
        if(errorCode != GADI_OK) {
            GADI_ERROR("gadi_isp_get_mwb_attr fail\n");
        }
        GADI_INFO("gadi_isp_get_mwb_attr ok\n");
        errorCode = gadi_isp_get_ae_attr(ispHandle, &ispAEAttr);
        if(errorCode != GADI_OK) {
            GADI_ERROR("gadi_isp_get_ae_attr fail\n");
        }
        GADI_INFO("gadi_isp_get_ae_attr ok\n");
        errorCode = gadi_isp_get_me_attr(ispHandle, &ispMEAttr);
        if(errorCode != GADI_OK) {
            GADI_ERROR("gadi_isp_get_me_attr fail\n");
        }
        GADI_INFO("gadi_isp_get_me_attr ok\n");

        GADI_INFO("start isp process\n");
        break;
    case 'P':
        errorCode = isp_stop();
        if(errorCode != GADI_OK) {
            GADI_ERROR("stop isp process fail\n");
            goto exit;
        }
        GADI_INFO("stop isp process\n");
        break;
    case 'x':
        CREATE_INPUT_MENU(ispExpType) {
            ADD_SUBMENU(ispExpType,
                "Setup ISP exposure type [0:auto 1:manual]."),
            CREATE_INPUT_MENU_COMPLETE();

            if (DISPLAY_MENU() == 0) {
                errorCode = gadi_isp_set_exposure_type(ispHandle, ispExpType);
                if(errorCode != GADI_OK)
                    GADI_ERROR("gadi_isp_set_exposure_type fail\n");
                else
                    GADI_INFO("gadi_isp_set_exposure_type ok\n");
            }
        }
        break;
    case 'y':
        CREATE_INPUT_MENU(ispWbType) {
            ADD_SUBMENU(ispWbType,
                "Setup ISP white banlance type [0:auto 1:manual]."),
            CREATE_INPUT_MENU_COMPLETE();
            if (DISPLAY_MENU() == 0) {
                errorCode = gadi_isp_set_wb_type(ispHandle, ispWbType);
                if(errorCode != GADI_OK)
                    GADI_ERROR("gadi_isp_set_wb_type fail\n");
                else
                    GADI_INFO("gadi_isp_set_wb_type ok\n");
            }
        }
        break;
    case 'U':
        CREATE_INPUT_MENU(ispIsDay) {
            ADD_SUBMENU(ispIsDay,
                "Setup ISP is day or night mode [0:night 1:day]."),
            CREATE_INPUT_MENU_COMPLETE();

            if (DISPLAY_MENU() == 0) {
                errorCode = gadi_isp_set_day_night_mode(ispIsDay);
                if(errorCode != GADI_OK)
                    GADI_ERROR("gadi_isp_set_day_night_mode fail\n");
                else
                    GADI_INFO("gadi_isp_set_day_night_mode ok\n");
            }
        }
        break;
    case 'f':
        CREATE_INPUT_MENU(ispFps) {
            ADD_SUBMENU(ispFps,
                "set vi fps."),
            CREATE_INPUT_MENU_COMPLETE();

            if (DISPLAY_MENU() == 0) {
                errorCode = GADI_OK;
                if(errorCode != GADI_OK)
                    GADI_ERROR("gadi_isp_set_vi_frame fail\n");
                else
                    GADI_INFO("gadi_isp_set_vi_frame ok\n");
            }
        }
        break;
    case 'R':
        errorCode = gadi_isp_reset_3a_static(ispHandle);
        if(errorCode != GADI_OK)
            GADI_ERROR("gadi_isp_reset_3a_static fail\n");
        else
            GADI_INFO("gadi_isp_reset_3a_static ok\n");
        break;
    /* auto exposure  attribute setup */
    case 'e':
        errorCode = gadi_isp_get_ae_attr(ispHandle, &ispAEAttr);
        if(errorCode != GADI_OK)
            GADI_ERROR("gadi_isp_set_ae_attr fail\n");
        else
            GADI_INFO("gadi_isp_set_ae_attr ok\n");

        CREATE_INPUT_MENU(ispAEAttr) {
            ADD_SUBMENU(ispAEAttr.speed, "Auto exposure convergence speed[0], def:0."),
            ADD_SUBMENU(ispAEAttr.shutterTimeMin,
                "Auto exposure shutter time min [1-8000], def:8000."),
            ADD_SUBMENU(ispAEAttr.shutterTimeMax,
                "Auto exposure shutter time max [1-8000], def:60."),
            ADD_SUBMENU(ispAEAttr.gainMin, "Auto exposure gain min [1-80], def:1."),
            ADD_SUBMENU(ispAEAttr.gainMax, "Auto exposure gain max [1-80], def:80."),
            ADD_SUBMENU(ispAEAttr.tagetRatio,
                "Auto exposure target ratio level [1-255], def:128."),
            CREATE_INPUT_MENU_COMPLETE();

            if (DISPLAY_MENU() == 0) {
                errorCode = gadi_isp_set_ae_attr(ispHandle, &ispAEAttr);
                if(errorCode != GADI_OK)
                    GADI_ERROR("gadi_isp_set_ae_attr fail\n");
                else
                    GADI_INFO("gadi_isp_set_ae_attr ok\n");
            }
        }
        break;
    /* manual exposure  attribute setup */
    case 'M':
        errorCode = gadi_isp_get_me_attr(ispHandle, &ispMEAttr);
        if(errorCode != GADI_OK)
            GADI_ERROR("gadi_isp_set_me_attr fail\n");
        else
            GADI_INFO("gadi_isp_set_me_attr ok\n");

        CREATE_INPUT_MENU(ispMEAttr) {
            ADD_SUBMENU(ispMEAttr.gain, "Manual exposure gain [0-64]."),
            ADD_SUBMENU(ispMEAttr.shutterTime,
                "Manual exposure shutter time min [1-8000]."),
            CREATE_INPUT_MENU_COMPLETE();

            if (DISPLAY_MENU() == 0) {
                errorCode = gadi_isp_set_me_attr(ispHandle, &ispMEAttr);
                if(errorCode != GADI_OK)
                    GADI_ERROR("gadi_isp_set_me_attr fail\n");
                else
                    GADI_INFO("gadi_isp_set_me_attr ok\n");
            }
        }
        break;
    /* auto white banlance  attribute setup */
    case 'w':
        errorCode = gadi_isp_get_awb_attr(ispHandle, &ispAWAttr);
        if(errorCode != GADI_OK)
            GADI_ERROR("gadi_isp_get_awb_attr fail\n");
        else
            GADI_INFO("gadi_isp_get_awb_attr ok\n");

        CREATE_INPUT_MENU(ispAWAttr) {
            ADD_SUBMENU(ispAWAttr.colorTemp,
                "Auto white banlance colorTemp [0-7]."),
            ADD_SUBMENU(ispAWAttr.algo,
                "Auto white banlance algo [0:auto 1:gray world]."),
            ADD_SUBMENU(ispAWAttr.speed,
                "Auto white banlance speed."),
            CREATE_INPUT_MENU_COMPLETE();

            if (DISPLAY_MENU() == 0) {
                errorCode = gadi_isp_set_awb_attr(ispHandle, &ispAWAttr);
                if(errorCode != GADI_OK)
                    GADI_ERROR("gadi_isp_set_awb_attr fail\n");
                else
                    GADI_INFO("gadi_isp_set_awb_attr ok\n");
            }
        }
        break;
    /* manual white banlance  attribute setup */
    case 'm':
        errorCode = gadi_isp_get_mwb_attr(ispHandle, &ispMWAttr);
        if(errorCode != GADI_OK)
            GADI_ERROR("gadi_isp_get_mwb_attr fail\n");
        else
            GADI_INFO("gadi_isp_get_mwb_attr ok\n");

        CREATE_INPUT_MENU(ispMWAttr) {
            ADD_SUBMENU(ispMWAttr.gainBlue, "Blue gain range [0-262144]."),
            ADD_SUBMENU(ispMWAttr.gainGreen, "Green gain range [0-262144]."),
            ADD_SUBMENU(ispMWAttr.gainRed, "Red gain range [0-262144]."),
            CREATE_INPUT_MENU_COMPLETE();

            if (DISPLAY_MENU() == 0) {
                errorCode = gadi_isp_set_mwb_attr(ispHandle, &ispMWAttr);
                if(errorCode != GADI_OK)
                    GADI_ERROR("gadi_isp_set_mwb_attr fail\n");
                else
                    GADI_INFO("gadi_isp_set_mwb_attr ok\n");
            }
        }
        break;
    /* saturation setup */
    case 'n':
        errorCode = gadi_isp_get_saturation(ispHandle, &ispSaturationAttr);
        if(errorCode != GADI_OK)
            GADI_ERROR("gadi_isp_set_saturation fail\n");
        else
            GADI_INFO("gadi_isp_set_saturation ok\n");

        CREATE_INPUT_MENU(ispSaturationAttr) {
            ADD_SUBMENU(ispSaturationAttr, "Saturation range [0-127]."),
            CREATE_INPUT_MENU_COMPLETE();

            if (DISPLAY_MENU() == 0) {
                errorCode = gadi_isp_set_saturation(ispHandle, ispSaturationAttr);
                if(errorCode != GADI_OK)
                    GADI_ERROR("gadi_isp_set_saturation fail\n");
                else
                    GADI_INFO("gadi_isp_set_saturation ok\n");
            }
        }
        break;
    /* contrast attribute setup */
    case 'c':
        errorCode = gadi_isp_get_contrast_attr(ispHandle, &ispContrastAttr);
        if(errorCode != GADI_OK)
            GADI_ERROR("gadi_isp_set_contrast_attr fail\n");
        else
            GADI_INFO("gadi_isp_set_contrast_attr ok\n");

        CREATE_INPUT_MENU(ispContrastAttr) {
            ADD_SUBMENU(ispContrastAttr.enableAuto,
                "Contrast enable [0:disable 1:enable]."),
            ADD_SUBMENU(ispContrastAttr.autoStrength,
                "Contrast auto strength [0-256]."),
            ADD_SUBMENU(ispContrastAttr.manualStrength,
                "Contrast manual strength [0-256]."),
            CREATE_INPUT_MENU_COMPLETE();

            if (DISPLAY_MENU() == 0) {
                errorCode = gadi_isp_set_contrast_attr(ispHandle, &ispContrastAttr);
                if(errorCode != GADI_OK)
                    GADI_ERROR("gadi_isp_set_contrast_attr fail\n");
                else
                    GADI_INFO("gadi_isp_set_contrast_attr ok\n");
            }
        }
        break;
    /* slow attribute setup */
    case 's':
        errorCode = gadi_isp_get_slow_framerate(ispHandle, &ispFactor);
        if(errorCode != GADI_OK)
            GADI_ERROR("gadi_isp_get_slow_framerate fail\n");
        else
            GADI_INFO("gadi_isp_get_slow_framerate ok\n");

        CREATE_INPUT_MENU(ispFactor) {
            ADD_SUBMENU(ispFactor,
                "ISP slow framerate factor[>8]."),
            CREATE_INPUT_MENU_COMPLETE();

            if (DISPLAY_MENU() == 0) {
                errorCode = gadi_isp_set_slow_framerate(ispHandle, ispFactor);
                if(errorCode != GADI_OK)
                    GADI_ERROR("gadi_isp_set_slow_framerate fail\n");
                else
                    GADI_INFO("gadi_isp_set_slow_framerate ok\n");
            }
        }
        break;
    case 'a':
        errorCode = gadi_isp_get_antiflicker(ispHandle, &ispAntFlickerAttr);
        if(errorCode != GADI_OK)
            GADI_ERROR("gadi_isp_get_antiflicker fail\n");
        else
            GADI_INFO("gadi_isp_get_antiflicker ok\n");

        CREATE_INPUT_MENU(ispAntFlickerAttr) {
            ADD_SUBMENU(ispAntFlickerAttr.enableDisable,
                "ISP antflicker enable [0:disable 1:enable]."),
            ADD_SUBMENU(ispAntFlickerAttr.freq,
                "ISP antflicker frequency [0:50Hz 1:60Hz]."),
            CREATE_INPUT_MENU_COMPLETE();

            if (DISPLAY_MENU() == 0) {
                errorCode = gadi_isp_set_antiflicker(ispHandle, &ispAntFlickerAttr);
                if(errorCode != GADI_OK)
                    GADI_ERROR("gadi_isp_set_antiflicker fail\n");
                else
                    GADI_INFO("gadi_isp_set_antiflicker ok\n");
            }
        }
        break;
    case 'g':
        errorCode = gadi_isp_get_gamma_attr(ispHandle, &ispGammaAttr);
        if(errorCode != GADI_OK)
            GADI_ERROR("gadi_isp_get_gamma_attr fail\n");
        else
            GADI_INFO("gadi_isp_get_gamma_attr ok\n");

        CREATE_INPUT_MENU(ispGammaAttr) {
            ADD_SUBMENU(ispGammaAttr.enableDisable,
                "ISP gamma enable [0:disable 1:enable]."),
            ADD_SUBMENU(ispGammaAttr.gammaTable[0],
                "ISP gamma table [0]."),
            ADD_SUBMENU(ispGammaAttr.gammaTable[1],
                "ISP gamma table [1]."),
            ADD_SUBMENU(ispGammaAttr.gammaTable[2],
                "ISP gamma table [2]."),
            ADD_SUBMENU(ispGammaAttr.gammaTable[3],
                "ISP gamma table [3]."),
            ADD_SUBMENU(ispGammaAttr.gammaTable[4],
                "ISP gamma table [4]."),
            ADD_SUBMENU(ispGammaAttr.gammaTable[5],
                "ISP gamma table [5]."),
            ADD_SUBMENU(ispGammaAttr.gammaTable[6],
                "ISP gamma table [6]."),
            ADD_SUBMENU(ispGammaAttr.gammaTable[7],
                "ISP gamma table [7]."),
            ADD_SUBMENU(ispGammaAttr.gammaTable[8],
                "ISP gamma table [8]."),
            CREATE_INPUT_MENU_COMPLETE();

            if (DISPLAY_MENU() == 0) {
                errorCode = gadi_isp_set_gamma_attr(ispHandle, &ispGammaAttr);
                if(errorCode != GADI_OK)
                    GADI_ERROR("gadi_isp_set_gamma_attr fail\n");
                else
                    GADI_INFO("gadi_isp_set_gamma_attr ok\n");
            }
        }
        break;
    case 'r':
        errorCode = gadi_isp_get_hue(ispHandle, &ispHueAttr);
        if(errorCode != GADI_OK)
            GADI_ERROR("gadi_isp_get_hue fail\n");
        else
            GADI_INFO("gadi_isp_get_hue ok\n");

        CREATE_INPUT_MENU(ispHueAttr) {
            ADD_SUBMENU(ispHueAttr,
                "ISP hue params[0-255]."),
            CREATE_INPUT_MENU_COMPLETE();

            if (DISPLAY_MENU() == 0) {
                errorCode = gadi_isp_set_hue(ispHandle, ispHueAttr);
                if(errorCode != GADI_OK)
                    GADI_ERROR("gadi_isp_set_hue fail\n");
                else
                    GADI_INFO("gadi_isp_set_hue ok\n");
            }
        }
        break;
    case 'b':
        errorCode = gadi_isp_get_brightness(ispHandle, &ispBrightnessAttr);
        if(errorCode != GADI_OK)
            GADI_ERROR("gadi_isp_get_brightness fail\n");
        else
            GADI_INFO("gadi_isp_get_brightness ok\n");

        CREATE_INPUT_MENU(ispBrightnessAttr) {
            ADD_SUBMENU(ispBrightnessAttr,
                "ISP brightness params [0-255]."),
            CREATE_INPUT_MENU_COMPLETE();

            if (DISPLAY_MENU() == 0) {
                errorCode = gadi_isp_set_brightness(ispHandle, ispBrightnessAttr);
                if(errorCode != GADI_OK)
                    GADI_ERROR("gadi_isp_set_brightness fail\n");
                else
                    GADI_INFO("gadi_isp_set_brightness ok\n");
            }
        }
        break;
    case 'p':
        errorCode = gadi_isp_get_sharpen_attr(ispHandle, &ispSharpenAttr);
        if(errorCode != GADI_OK)
            GADI_ERROR("gadi_isp_get_sharpen_attr fail\n");
        else
            GADI_INFO("gadi_isp_get_sharpen_attr ok\n");

        CREATE_INPUT_MENU(ispSharpenAttr) {
            ADD_SUBMENU(ispSharpenAttr.enableDisable,
                "ISP sharpen enable [0:disable 1:enable]."),
            ADD_SUBMENU(ispSharpenAttr.enableManual,
                "ISP sharpen auto/manual [0:auto 1:manual]."),
            ADD_SUBMENU(ispSharpenAttr.level,
                "ISP sharpen level [0-255]."),
            CREATE_INPUT_MENU_COMPLETE();

            if (DISPLAY_MENU() == 0) {
                errorCode = gadi_isp_set_sharpen_attr(ispHandle, &ispSharpenAttr);
                if(errorCode != GADI_OK)
                    GADI_ERROR("gadi_isp_set_sharpen_attr fail\n");
                else
                    GADI_INFO("gadi_isp_set_sharpen_attr ok\n");
            }
        }
        break;
    case 'i':
        errorCode = gadi_isp_get_shading_attr(ispHandle, &ispShadingAttr);
        if(errorCode != GADI_OK)
            GADI_ERROR("gadi_isp_get_shading_attr fail\n");
        else
            GADI_INFO("gadi_isp_get_shading_attr ok\n");

        CREATE_INPUT_MENU(ispShadingAttr) {
            ADD_SUBMENU(ispShadingAttr.enableDisable,
                "ISP shading enable [0:disable 1:enable]."),
            CREATE_INPUT_MENU_COMPLETE();

            if (DISPLAY_MENU() == 0) {
                errorCode = gadi_isp_set_shading_attr(ispHandle, &ispShadingAttr);
                if(errorCode != GADI_OK)
                    GADI_ERROR("gadi_isp_set_shading_attr fail\n");
                else
                    GADI_INFO("gadi_isp_set_shading_attr ok\n");
            }
        }
        break;
    case 'o':
        errorCode = gadi_isp_get_denoise_attr(ispHandle, &ispDenoiseAttr);
        if(errorCode != GADI_OK)
            GADI_ERROR("gadi_isp_get_denoise_attr fail\n");
        else
            GADI_INFO("gadi_isp_get_denoise_attr ok\n");

        CREATE_INPUT_MENU(ispDenoiseAttr) {
            ADD_SUBMENU(ispDenoiseAttr.enableDisable,
                "ISP denoise enable [0:disable 1:enable]."),
            ADD_SUBMENU(ispDenoiseAttr.level,
                "ISP denoise level [0-255]."),
            CREATE_INPUT_MENU_COMPLETE();

            if (DISPLAY_MENU() == 0) {
                errorCode = gadi_isp_set_denoise_attr(ispHandle, &ispDenoiseAttr);
                if(errorCode != GADI_OK)
                    GADI_ERROR("gadi_isp_set_denoise_attr fail\n");
                else
                    GADI_INFO("gadi_isp_set_denoise_attr ok\n");
            }
        }
        break;
    case 'l':
        errorCode = gadi_isp_get_blacklevel_attr(ispHandle, &ispBlackLevelAttr);
        if(errorCode != GADI_OK)
            GADI_ERROR("gadi_isp_get_blacklevel_attr fail\n");
        else
            GADI_INFO("gadi_isp_get_blacklevel_attr ok\n");

        CREATE_INPUT_MENU(ispBlackLevelAttr) {
            ADD_SUBMENU(ispBlackLevelAttr.blackLevel[0],
                "ISP black level of R [0-65535]."),
            ADD_SUBMENU(ispBlackLevelAttr.blackLevel[1],
                "ISP black level of Gr [0-65535]."),
            ADD_SUBMENU(ispBlackLevelAttr.blackLevel[2],
                "ISP black level of Gb [0-65535]."),
            ADD_SUBMENU(ispBlackLevelAttr.blackLevel[3],
                "ISP black level of B [0-65535]."),
            CREATE_INPUT_MENU_COMPLETE();

            if (DISPLAY_MENU() == 0) {
                errorCode = gadi_isp_set_blacklevel_attr(ispHandle, &ispBlackLevelAttr);
                if(errorCode != GADI_OK)
                    GADI_ERROR("gadi_isp_set_blacklevel_attr fail\n");
                else
                    GADI_INFO("gadi_isp_set_blacklevel_attr ok\n");
            }
        }
        break;
    case 'k':
        errorCode = gadi_isp_get_backlight_attr(ispHandle, &ispBackLightAttr);
        if(errorCode != GADI_OK)
            GADI_ERROR("gadi_isp_get_backlight_attr fail\n");
        else
            GADI_INFO("gadi_isp_get_backlight_attr ok\n");

        CREATE_INPUT_MENU(ispBackLightAttr) {
            ADD_SUBMENU(ispBackLightAttr.enableDisable,
                "ISP back light enable [0:disable 1:enable]."),
            ADD_SUBMENU(ispBackLightAttr.level,
                "ISP back light level [0-255]."),
            CREATE_INPUT_MENU_COMPLETE();

            if (DISPLAY_MENU() == 0) {
                errorCode = gadi_isp_set_backlight_attr(ispHandle, &ispBackLightAttr);
                if(errorCode != GADI_OK)
                    GADI_ERROR("gadi_isp_set_backlight_attr fail\n");
                else
                    GADI_INFO("gadi_isp_set_backlight_attr ok\n");
            }
        }
        break;
    case 'L':
        CREATE_INPUT_MENU(printStr) {
            ADD_SUBMENU(printStr,
                "[all] [slow] [antiflicker] [gamma] [awb]\n"
                "\t\t\t\t\t[mwb] [saturation] [contrast] [hue]\n"
                "\t\t\t\t\t[brightness] [sharpen] [shading] [denoise]\n"
                "\t\t\t\t\t[blacklevel] [backlight] [ae] [me] [sensor]\n"
                "\t\t\t\t\t[shutter] [gain]."),
            CREATE_INPUT_MENU_COMPLETE();

            if (DISPLAY_MENU() == 0) {
                if (strcmp(ispPrintStr, "all") == 0)
                    isp_print_all();
                else if (strcmp(ispPrintStr, "slow") == 0){
                    isp_print_slow_framerate();
                    option_index += 4;
                } else if (strcmp(ispPrintStr, "antiflicker") == 0) {
                    isp_print_antiflicker();
                    option_index += 11;
                } else if (strcmp(ispPrintStr, "gamma") == 0) {
                    isp_print_gamma();
                    option_index += 5;
                } else if (strcmp(ispPrintStr, "awb") == 0) {
                    isp_print_awb();
                    option_index += 3;
                } else if (strcmp(ispPrintStr, "mwb") == 0) {
                    isp_print_mwb();
                    option_index += 3;
                } else if (strcmp(ispPrintStr, "saturation") == 0) {
                    isp_print_saturation();
                    option_index += 10;
                } else if (strcmp(ispPrintStr, "contrast") == 0) {
                    isp_print_contrast();
                    option_index += 8;
                } else if (strcmp(ispPrintStr, "hue") == 0) {
                    isp_print_hue();
                    option_index += 3;
                } else if (strcmp(ispPrintStr, "brightness") == 0) {
                    isp_print_brightness();
                    option_index += 10;
                } else if (strcmp(ispPrintStr, "sharpen") == 0) {
                    isp_print_sharpen();
                    option_index += 7;
                } else if (strcmp(ispPrintStr, "shading") == 0) {
                    isp_print_shading();
                    option_index += 7;
                } else if (strcmp(ispPrintStr, "denoise") == 0) {
                    isp_print_denoise();
                    option_index += 7;
                } else if (strcmp(ispPrintStr, "blacklevel") == 0) {
                    isp_print_blacklevel();
                    option_index += 10;
                } else if (strcmp(ispPrintStr, "backlight") == 0) {
                    isp_print_backlight();
                    option_index += 9;
                } else if (strcmp(ispPrintStr, "ae") == 0) {
                    isp_print_ae();
                    option_index += 2;
                } else if (strcmp(ispPrintStr, "me") == 0) {
                    isp_print_me();
                    option_index += 2;
                } else if (strcmp(ispPrintStr, "sensor") == 0) {
                    isp_print_sensor_model();
                    option_index += 6;
                } else if (strcmp(ispPrintStr, "shutter") == 0) {
                    isp_print_shutter();
                    option_index += 7;
                } else if (strcmp(ispPrintStr, "gain") == 0) {
                    isp_print_gain();
                    option_index += 4;
                }
            }
        }
        break;
    default:
        GADI_ERROR("no params\n");
        break;
    }
    }

exit:
    optind = 1;
    return 0;
}

