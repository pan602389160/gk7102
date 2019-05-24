/*!
*****************************************************************************
** \file        e/adi/test/src/ir.c
**
** \version     $Id$
**
** \brief       ir testcase
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**              ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**              OMMISSIONS
**
** (C) Copyright 2012-2013 by GOKE MICROELECTRONICS CO.,LTD
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
#include <errno.h>
#include <sys/epoll.h>
    
#include "adi_types.h"
#include "shell.h"
#include "adi_sys.h"
#include "adi_ir.h"
#include "ir.h"

//*****************************************************************************
//*****************************************************************************
//** Local Defines
//*****************************************************************************
//*****************************************************************************

#define ADI_TEST_DEBUG_LEVEL    GADI_SYS_LOG_LEVEL_INFO

#define DEVICE_ID_FUJITSU  1

#define RUC_KEY_MAP_ONE    0x0001
#define RUC_KEY_MAP_TWO    0x0002 
#define RUC_KEY_MAP_THREE  0x0003
#define RUC_KEY_MAP_FOUR   0x0004
#define RUC_KEY_MAP_FIVE   0x0005
#define RUC_KEY_MAP_SIX    0x0006
#define RUC_KEY_MAP_SEVEN  0x0007
#define RUC_KEY_MAP_EIGHT  0x0008
#define RUC_KEY_MAP_NINE   0x0009
#define RUC_KEY_MAP_ZERO   0x0000
#define RUC_KEY_MAP_POWER  0x000a

#define SYSTEM_RCU_TABLE_FUJITSU \
{ 0x001b, RUC_KEY_MAP_ONE   }, \
{ 0x000f, RUC_KEY_MAP_TWO   }, \
{ 0x0003, RUC_KEY_MAP_THREE }, \
{ 0x0019, RUC_KEY_MAP_FOUR  }, \
{ 0x0011, RUC_KEY_MAP_FIVE  }, \
{ 0x0001, RUC_KEY_MAP_SIX   }, \
{ 0x0009, RUC_KEY_MAP_SEVEN }, \
{ 0x001d, RUC_KEY_MAP_EIGHT }, \
{ 0x000d, RUC_KEY_MAP_NINE  }, \
{ 0x000c, RUC_KEY_MAP_ZERO  }, \
{ 0x000b, RUC_KEY_MAP_POWER }, \


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
static GADI_SYS_HandleT irHandle = NULL;

static GADI_IR_KeycodeMapT irTestKeyTable[] = { SYSTEM_RCU_TABLE_FUJITSU };

//*****************************************************************************
//*****************************************************************************
//** Local Functions Declaration
//*****************************************************************************
//*****************************************************************************

static GADI_ERR ir_p_handle_cmd(int argc, char *argv[]);

static GADI_ERR test_ir_open(char *nodePath);

static void ir_user_function(GADI_SYS_HandleT irHandle,
                                    GADI_U16 keyValue, GADI_U16 rcmCode,
                                    GADI_U32 deviceId, GADI_U32 *userOptDataPtr);
                                    

//*****************************************************************************
//*****************************************************************************
//** API Functions
//*****************************************************************************
//*****************************************************************************

GADI_ERR ir_register_testcase(void)
{

    GADI_ERR   retVal =  GADI_OK;
    (void)shell_registercommand(
        "ir",
        ir_p_handle_cmd,
        "ir command",
        "---------------------------------------------------------------------\n"
        "ir open [nodePath]\n"
        "   brief : open ir device and set parameter.\n"
        "   param : nodePath    -- device node path\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "ir close \n"
        "   brief : close ir.\n"
        "\n"
    );
    
    return retVal;
    
}

//*****************************************************************************
//*****************************************************************************
//** Local Functions
//*****************************************************************************
//*****************************************************************************

static GADI_ERR ir_p_handle_cmd(int argc, char *argv[])
{

    GADI_ERR errorCode = GADI_OK;
    
    if(argc == 3)
    {
        if(strcmp(argv[1], "open") == 0)
        {
            char *nodePath;
            nodePath = argv[2];
            errorCode = test_ir_open(nodePath);
            if (errorCode != GADI_OK)
            {
                printf("ir open failed.\n");
                return errorCode;
            }
        }
        else
        {
            goto bad_parameter;
        }
    }
    else if(argc == 2)
    {
        if(strcmp(argv[1], "close") == 0)
        {
            errorCode = gadi_ir_close(irHandle);
            if(errorCode != GADI_OK)
            {
                printf("gadi_ir_close() failed.\n");
                return errorCode;
            }
            else
            {
                printf("ir closed.\n");
                irHandle = NULL;
            }
        }
        else
        {
            goto bad_parameter;
        }
            
    }
    else
    {
         goto bad_parameter;
    }

    return 0;

bad_parameter:
    return -1;
}

static GADI_ERR test_ir_open(char *nodePath)
{

    GADI_IR_OpenParamsT openParams;
    GADI_ERR            errorCode = GADI_OK;

    if(irHandle) 
    {
        GADI_ERROR("ir opened already!\n");
        return GADI_OK;
    }

    gadi_sys_memset(&openParams, 0, sizeof(GADI_IR_OpenParamsT));
    
    openParams.nodePath        = nodePath;
    openParams.type            = GADI_IR_TYPE_RECEIVER;
    openParams.mode            = GADI_IR_HARDWARE_MODE;
    openParams.protocol        = GADI_IR_PROTOCOL_NEC;
    openParams.userFunctionPtr = ir_user_function;
    openParams.userOptDataPtr  = NULL;
    openParams.deviceID        = DEVICE_ID_FUJITSU;

    gadi_ir_init();

    irHandle = gadi_ir_open(&openParams, &errorCode);
    if (irHandle == NULL)
        goto open_failed;
        
    errorCode = gadi_ir_register_map_table(DEVICE_ID_FUJITSU,
                sizeof(irTestKeyTable)/sizeof(irTestKeyTable[0]),
                irTestKeyTable);
    if(errorCode != GADI_OK)
    {
        gadi_ir_close(irHandle);
        irHandle = NULL;
        goto open_failed;
    }

    GADI_INFO("ir open");
    return GADI_OK;

open_failed:
    GADI_ERROR("ir open failed.\n");
    return errorCode;

}

static void ir_user_function(GADI_SYS_HandleT irHandle,
                                    GADI_U16 keyValue, GADI_U16 rcmCode,
                                    GADI_U32 deviceId, GADI_U32 *userOptDataPtr)
{

    switch(rcmCode)
    {
        case RUC_KEY_MAP_ONE: 
            printf("RUC_KEY_MAP_ONE 0x0001\n");    
            break;
        case RUC_KEY_MAP_TWO:
            printf("RUC_KEY_MAP_TWO 0x0002\n");      
            break;
        case RUC_KEY_MAP_THREE:
            printf("RUC_KEY_MAP_THREE 0x0003\n");         
            break;
        case RUC_KEY_MAP_FOUR:
            printf("RUC_KEY_MAP_FOUR 0x0004\n");         
            break;
        case RUC_KEY_MAP_FIVE:
            printf("RUC_KEY_MAP_FIVE 0x0005\n");         
            break;
        case RUC_KEY_MAP_SIX:
            printf("RUC_KEY_MAP_SIX 0x0006\n");         
            break;
        case RUC_KEY_MAP_SEVEN:
            printf("RUC_KEY_MAP_SEVEN 0x0007\n");         
            break;
        case RUC_KEY_MAP_EIGHT:
            printf("RUC_KEY_MAP_EIGHT 0x0008\n");         
            break;
        case RUC_KEY_MAP_NINE:
            printf("RUC_KEY_MAP_NINE 0x0009\n");         
            break;
        case RUC_KEY_MAP_ZERO:
            printf("RUC_KEY_MAP_ZERO 0x00000\n");         
            break;
        case RUC_KEY_MAP_POWER:
            printf("RUC_KEY_MAP_POWER 0x000a\n");         
            break;
        default: 
            printf("GADI_IR_KEY_UNDEFINED\n");
            break;
    }

    GADI_INFO("keyValue[0x%.4x], rcmCode[0x%.4x], deviceId[%d]\n",
            keyValue, rcmCode, deviceId);
            
}



