/*!
*****************************************************************************
** \file        e/adi/test/src/gpio.c
**
** \brief       ADI layer GPIO test
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**               ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**               OMMISSIONS
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


#include "shell.h"
#include "adi_types.h"
#include "adi_sys.h"
#include "adi_gpio.h"
#include "gpio.h"


//*****************************************************************************
//*****************************************************************************
//** Local Defines
//*****************************************************************************
//*****************************************************************************

#define ADI_TEST_DEBUG_LEVEL    GADI_SYS_LOG_LEVEL_INFO

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

static GADI_SYS_HandleT gpioHandle;

//*****************************************************************************
//*****************************************************************************
//** Local Functions Declaration
//*****************************************************************************
//*****************************************************************************

static GADI_ERR gpio_p_handle_cmd(int argc, char *argv[]);


//*****************************************************************************
//*****************************************************************************
//** API Functions
//*****************************************************************************
//*****************************************************************************
GADI_ERR gpio_register_testcase(void)
{

    GADI_ERR   retVal =  GADI_OK;
    (void)shell_registercommand (
        "gpio",
        gpio_p_handle_cmd,
        "gpio command",
        "---------------------------------------------------------------------\n"
        "gpio open [number] [active_low] [direction] [value]\n"
        "   brief : export I/O port and set parameter.\n"
        "   param : number      -- GPIO number, some GPIO can not exported\n"
        "   param : active_low  -- 0 positive logic\n"
        "                       -- 1 negative logic\n"
        "   param : direction   -- 0 input mode\n"
        "                       -- 1 output mode\n"
        "   param : value       -- 0 high or low level in output mode, ignored in input mode\n"
        "                       -- 1 high or low level in output mode, ignored in input mode\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "gpio set   \n"
        "   brief : change output value in output mode.\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "gpio clear \n"
        "   brief : change output value in output mode.\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "gpio read  \n"
        "   brief : Get value of high-low level.\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "gpio close \n"
        "   brief : close I/O port, should close current I/O port before opening other I/O port.\n"
        "\n"
    );

    return retVal;

}

static GADI_ERR gpio_p_handle_cmd(int argc, char *argv[])
{

    GADI_ERR errorCode = GADI_OK;

    if(argc == 6)
    {
        if(strcmp(argv[1], "open") == 0)
        {
            GADI_GPIO_OpenParam config;
            config.num_gpio   = (GADI_U8)(atoi(argv[2]));
            config.active_low = (GADI_U8)(atoi(argv[3]));
            config.direction  = (GADI_U8)(atoi(argv[4]));
            config.value      = (GADI_U8)(atoi(argv[5]));
            gpioHandle = gadi_gpio_open(&errorCode, &config);
            if (errorCode != GADI_OK)
            {
                printf("gadi_gpio_open() failed.\n");
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
        if(strcmp(argv[1], "set") == 0)
        {
            errorCode = gadi_gpio_set(gpioHandle);
            if (errorCode != GADI_OK)
            {
                printf("gadi_gpio_set() failed.\n");
                return errorCode;
            }
        }
        else if(strcmp(argv[1], "clear") == 0)
        {
            errorCode = gadi_gpio_clear(gpioHandle);
            if (errorCode != GADI_OK)
            {
                printf("gadi_gpio_clear() failed.\n");
                return errorCode;
            }
        }
        else if(strcmp(argv[1], "read") == 0)
        {
            GADI_S32 value;
            errorCode = gadi_gpio_read_value(gpioHandle, &value);
            if (errorCode != GADI_OK)
            {
                printf("gadi_gpio_read_value() failed.\n");
                return errorCode;
            }
            GADI_INFO("value is %d\n", value);
        }
        else if(strcmp(argv[1], "close") == 0)
        {
            errorCode = gadi_gpio_close(gpioHandle);
            if (errorCode != GADI_OK)
            {
                printf("gadi_gpio_close() failed.\n");
                return errorCode;
            }
            else
            {
                printf("gpio closed.\n");
                gpioHandle = NULL;
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




//*****************************************************************************
//*****************************************************************************
//** Local Functions
//*****************************************************************************
//*****************************************************************************




