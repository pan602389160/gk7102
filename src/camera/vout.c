/******************************************************************************
** \file        adi/test/src/vout.c
**
** \brief       ADI layer vout test.
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
#include "vout.h"
#include "shell.h"
#include "parser.h"
#include "debug.h"

//*****************************************************************************
//*****************************************************************************
//** Local Defines
//*****************************************************************************
//*****************************************************************************

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
GADI_SYS_HandleT voHandle    = NULL;

//*****************************************************************************
//*****************************************************************************
//** Local Data
//*****************************************************************************
//*****************************************************************************
static GADI_VOUT_SettingParamsT   voParams[GADI_VOUT_NUMBER];

static parser_map voMap[] =
{
    {"vout_mode_a",     &voParams[GADI_VOUT_A].resoluMode,   DATA_TYPE_U32},
    {"vout_dev_a",      &voParams[GADI_VOUT_A].deviceType,   DATA_TYPE_U32},
    {"vout_mode_b",     &voParams[GADI_VOUT_B].resoluMode,   DATA_TYPE_U32},
    {"vout_dev_b",      &voParams[GADI_VOUT_B].deviceType,   DATA_TYPE_U32},
    {NULL,              NULL,                                DATA_TYPE_U32},
};

//*****************************************************************************
//*****************************************************************************
//** Local Functions Declaration
//*****************************************************************************
//*****************************************************************************

//*****************************************************************************
//*****************************************************************************
//** API Functions
//*****************************************************************************
//*****************************************************************************
GADI_ERR gdm_vout_init(void)
{
    GADI_ERR retVal = GADI_OK;

    retVal = gadi_vout_init();

    return retVal;
}

GADI_ERR gdm_vout_exit(void)
{
    GADI_ERR retVal = GADI_OK;

    retVal = gadi_vout_exit();

    return retVal;
}

GADI_ERR gdm_vout_open(void)
{
    GADI_ERR retVal = GADI_OK;

    voHandle = gadi_vout_open(&retVal);
    if(retVal != GADI_OK)
    {
        GADI_ERROR("gadi_vo_open error\n");
        return retVal;
    }

    return retVal;
}


GADI_ERR gdm_vout_close(void)
{
    GADI_ERR retVal = GADI_OK;


    retVal = gadi_vout_close(voHandle);
    if(retVal != GADI_OK)
    {
        GADI_ERROR("gadi_vo_close error\n");
        return retVal;
    }

    return retVal;
}

GADI_ERR gdm_vout_parse_config_file(char *path)
{
    GADI_ERR retVal = GADI_OK;

    if(path == NULL)
        return -1;

    /*parse vo parameters.*/
    retVal =  parse_voInfo(voMap, path);
    DBG_PRINTF("VoutA:resoluMode:%d,deviceType:%d\n",
        voParams[GADI_VOUT_A].resoluMode, voParams[GADI_VOUT_A].deviceType);
    DBG_PRINTF("VoutB:resoluMode:%d,deviceType:%d\n",
        voParams[GADI_VOUT_B].resoluMode, voParams[GADI_VOUT_B].deviceType);

    return retVal;
}

GADI_ERR gdm_vout_setup(void)
{
    GADI_S32 i;
    GADI_ERR retVal = GADI_OK;

    /*video out module: set video output resolution and output device.*/
    for(i = GADI_VOUT_A; i < GADI_VOUT_NUMBER; i++) {
        voParams[i].voutChannel = i;
        retVal = gadi_vout_set_params(voHandle, &voParams[i]);
        if(retVal != GADI_OK)
        {
            GADI_ERROR("gadi_vo_set_params error\n");
            return retVal;
        }
    }

    return retVal;
}


GADI_ERR gdm_vout_setup_after(void)
{
    GADI_S32 i;
    GADI_ERR retVal = GADI_OK;
    GADI_VOUT_I80_CmdParaT   i80CmdParams;

    /*video out module: set video output resolution and output device.*/
    for(i = GADI_VOUT_A; i < GADI_VOUT_NUMBER; i++) {
        if(voParams[i].deviceType == GADI_VOUT_DEVICE_I80)
        {
            i80CmdParams.voId       = i;
            i80CmdParams.cmdParaNum = 1;//command total
            i80CmdParams.rdParaNum  = 0; //read command total
            i80CmdParams.cmdPara[0] = LCD_WRITE_CMD(0x2c);//I80 write pixel
            retVal =  gadi_vout_set_i80_params(voHandle, &i80CmdParams);
            if(retVal != GADI_OK)
            {
               GADI_ERROR("gadi_vout_set_i80_params error\n");
               return retVal;
            }
        }
    }

    return retVal;
}


//*****************************************************************************
//*****************************************************************************
//** Local Functions
//*****************************************************************************
//*****************************************************************************

