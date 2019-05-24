/******************************************************************************
** \file        adi/test/src/pwm.c
**
**
** \brief        ADI layer pwm test.
**
** \attention    THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**                ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**                OMMISSIONS
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

#include "shell.h"
#include "adi_pwm.h"
#include "adi_sys.h"
#include "pwm.h"

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

//*****************************************************************************
//*****************************************************************************
//** Local Data
//*****************************************************************************
//*****************************************************************************
static GADI_SYS_HandleT pwmHandle = NULL;

static const char *shortOptions = "hSPIO:CEm:s:d:l";
static struct option longOptions[] =
{
    {"help",     0, 0, 'h'},
    {"start",     0, 0, 'S'},
    {"stop",     0, 0, 'P'},
    {"init",     0, 0, 'I'},
    {"open",     1, 0, 'O'},
    {"close",     0, 0, 'C'},
    {"exit",     0, 0, 'E'},
    {"mode",     1, 0, 'm'},
    {"speed",     1, 0, 's'},
    {"duty",     1, 0, 'd'},
    {"look",     0, 0, 'l'},
    {0,          0, 0, 0}
};


//*****************************************************************************
//*****************************************************************************
//** Local Functions Declaration
//*****************************************************************************
//*****************************************************************************
static void pwm_usage(void);
static int handle_pwm_command(int argc, char* argv[]);

//*****************************************************************************
//*****************************************************************************
//** API Functions
//*****************************************************************************
//*****************************************************************************
GADI_ERR pwm_init(void)
{
    GADI_ERR retVal;

    retVal = gadi_pwm_init();
    if(retVal != GADI_OK)
        GADI_ERROR("gadi_pwm_init() failed!\n");

    return retVal;
}

GADI_ERR pwm_exit(void)
{
    GADI_ERR retVal;

    retVal = gadi_pwm_exit();
    if(retVal != GADI_OK)
        GADI_ERROR("gadi_pwm_exit() failed!\n");

    return GADI_OK;
}

GADI_ERR pwm_open(GADI_U8 channel)
{
    GADI_ERR retVal = GADI_OK;

    pwmHandle = gadi_pwm_open(&retVal, channel);
    if(retVal != GADI_OK || pwmHandle == NULL)
    {
        GADI_ERROR("gadi_pwm_open() failed\n");
    }

    return retVal;
}

GADI_ERR pwm_close(void)
{
    GADI_ERR retVal = GADI_OK;

    retVal = gadi_pwm_close(pwmHandle);
    if(retVal != GADI_OK)
    {
        GADI_ERROR("gadi_pwm_close() failed\n");
    }

    return retVal;
}

GADI_ERR pwm_start(void)
{
    GADI_ERR retVal = GADI_OK;

    retVal = gadi_pwm_start(pwmHandle);
    if(retVal != GADI_OK)
    {
        GADI_ERROR("gadi_pwm_start() failed\n");
    }

    return retVal;
}

GADI_ERR pwm_stop(void)
{
    GADI_ERR retVal = GADI_OK;

    retVal = gadi_pwm_stop(pwmHandle);
    if(retVal != GADI_OK)
    {
        GADI_ERROR("gadi_pwm_stop() failed\n");
    }

    return retVal;
}

GADI_ERR pwm_set_mode(GADI_U8 mode)
{
    GADI_ERR retVal = GADI_OK;

    retVal = gadi_pwm_set_mode(pwmHandle, mode);
    if(retVal != GADI_OK)
    {
        GADI_ERROR("gadi_pwm_get_mode() failed\n");
    }
    return retVal;
}

GADI_ERR pwm_set_speed(GADI_U32 speed)
{
    GADI_ERR retVal = GADI_OK;

    retVal = gadi_pwm_set_speed(pwmHandle, speed);
    if(retVal != GADI_OK)
    {
        GADI_ERROR("gadi_pwm_set_speed() failed\n");
    }
    return retVal;
}

GADI_ERR pwm_set_duty(GADI_U32 duty)
{
    GADI_ERR retVal = GADI_OK;

    retVal = gadi_pwm_set_duty(pwmHandle, duty);
    if(retVal != GADI_OK)
    {
        GADI_ERROR("gadi_pwm_set_duty() failed\n");
    }
    return retVal;
}

GADI_ERR pwm_look(void)
{
    GADI_ERR retVal = GADI_OK;
    GADI_U8 mode = 0;
    GADI_U32 speed = 0;
    GADI_U32 duty = 0;

    retVal = gadi_pwm_get_mode(pwmHandle, &mode);
    if(retVal != GADI_OK)
    {
        GADI_ERROR("gadi_pwm_get_mode() failed\n");
        return retVal;
    }

    retVal = gadi_pwm_get_speed(pwmHandle, &speed);
    if(retVal != GADI_OK)
    {
        GADI_ERROR("gadi_pwm_get_speed() failed\n");
        return retVal;
    }

    retVal = gadi_pwm_get_duty(pwmHandle, &duty);
    if(retVal != GADI_OK)
    {
        GADI_ERROR("gadi_pwm_get_duty() failed\n");
        return retVal;
    }
    GADI_INFO("pwm: mode = %d, speed = %d\n"
                "\tduty = %d.%01d %%\n", mode, speed, duty/10, duty%10);
    return GADI_OK;
}


int pwm_register_testcase(void)
{
    int   retVal = 0;
    (void)shell_registercommand (
        "pwm",
        handle_pwm_command,
        "pwm command",
        "---------------------------------------------------------------------\n"
        "pwm -I -O 0 -m 0 -s 1000 -d 500 -S\n"
        "    brief : produce square wave.\n"
        "\n"
        "---------------------------------------------------------------------\n"
    );

    return retVal;
}

//*****************************************************************************
//*****************************************************************************
//** Local Functions
//*****************************************************************************
//*****************************************************************************
static void pwm_usage(void)
{
    printf("\nusage: pwm [OPTION]...\n");
    printf("\t-h, --help            help\n"
           "\t-I, --init            init pwm\n"
           "\t-O, --open[index]        open pwm[index:0~3]\n"
           "\t-C, --close            close pwm\n"
           "\t-E, --exit            exit pwm\n"
           "\t-S, --start            start pwm\n"
           "\t-P, --stop             stop pwm\n"
           "\t-m, --mode            set pwm mode(0~1)\n"
           "\t-s, --speed            set pwm speed(Hz)\n"
           "\t-d, --duty            set pwm duty rate[1~999]\n");

    printf("example:\n");
    printf("\tpwm \n"
           " --- produce 1000Hz square wave at 0 channel of pwm.\n"
           "\tpwm -I -O 0 -m 0 -s 1000 -d 500 -S\n");

    printf("\n");
}

static int handle_pwm_command(int argc, char* argv[])
{
    int option_index, ch;

    /*change parameters when giving input options.*/
    while ((ch = getopt_long(argc, argv, shortOptions, longOptions, &option_index)) != -1)
    {
        switch (ch)
        {
            case 'h':
                pwm_usage();
                goto command_exit;
            case 'I':
                if (GADI_OK != pwm_init()) {
                    GADI_ERROR("pwm_init failed!\n");
                    goto command_exit;
                }
                GADI_INFO("pwm_init ok!\n");
                break;
            case 'O':
                if (GADI_OK != pwm_open(atoi(optarg))) {
                    GADI_ERROR("pwm_open failed!\n");
                    goto command_exit;
                }
                GADI_INFO("pwm_open ok!\n");
                break;
            case 'S':
                if (GADI_OK != pwm_start()) {
                    GADI_ERROR("pwm_start failed!\n");
                    goto command_exit;
                }
                GADI_INFO("pwm_start ok!\n");
                break;
            case 'P':
                if (GADI_OK != pwm_stop()) {
                    GADI_ERROR("pwm_stop failed!\n");
                    goto command_exit;
                }
                GADI_INFO("pwm_stop ok!\n");
                break;
            case 'C':
                if (GADI_OK != pwm_close()) {
                    GADI_ERROR("pwm_close failed!\n");
                    goto command_exit;
                }
                GADI_INFO("pwm_close ok!\n");
                break;
            case 'E':
                if (GADI_OK != pwm_exit()) {
                    GADI_ERROR("pwm_exit failed!\n");
                    goto command_exit;
                }
                GADI_INFO("pwm_exit ok!\n");
                break;
            case 'm':
                if (GADI_OK != pwm_set_mode(atoi(optarg))) {
                    GADI_ERROR("pwm_set_mode failed!\n");
                    goto command_exit;
                }
                GADI_INFO("pwm_set_mode ok!\n");
                break;
            case 's':
                if (GADI_OK != pwm_set_speed(atoi(optarg))) {
                    GADI_ERROR("pwm_set_speed failed!\n");
                    goto command_exit;
                }
                GADI_INFO("pwm_set_speed ok!\n");
                break;
            case 'd':
                if (GADI_OK != pwm_set_duty(atoi(optarg))) {
                    GADI_ERROR("pwm_set_duty failed!\n");
                    goto command_exit;
                }
                GADI_INFO("pwm_set_duty ok!\n");
                break;
            case 'l':
                if (GADI_OK != pwm_look()) {
                    GADI_ERROR("pwm_look failed!\n");
                    goto command_exit;
                }
                GADI_INFO("pwm_look ok!\n");
                break;

            default:
                printf("type '--help' for more usage.\n");
                goto command_exit;
        }
    }

command_exit:
    optind = 1;
    return 0;
}

