/******************************************************************************
** \file        adi/test/src/pm.c
**
**
** \brief       ADI layer privacy mask test.
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

#include "shell.h"
#include "pm.h"
#include "venc.h"
//*****************************************************************************
//*****************************************************************************
//** Local Defines
//*****************************************************************************
//*****************************************************************************


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


//*****************************************************************************
//*****************************************************************************
//** Local Data
//*****************************************************************************
//*****************************************************************************
GADI_SYS_HandleT pmHandle;

static const char shortOptions[] = "hu:x:y:w:H:c:a:e:";
static struct option longOptions[] =
{
    {"help",     0, 0, 'h'},
    {"unit",     0, 0, 'u'},
    {"offsetX",  0, 0, 'x'},
    {"offsetY",  0, 0, 'y'},
    {"width",    0, 0, 'w'},
    {"height",   0, 0, 'H'},
    {"colour",   0, 0, 'c'},
    {"action",   0, 0, 'a'},
    {"enable",   0, 0, 'e'},
    {0,          0, 0, 0}
};

GADI_PM_MallocParamsT pmPars =
{
    .unit    = 1,
    .offsetX = 0,
    .offsetY = 0,
    .width   = 20,
    .height  = 20,
    .colour  = 0,
    .action  = 0,
};

GADI_U32 enable = 0;

//*****************************************************************************
//*****************************************************************************
//** Local Functions Declaration
//*****************************************************************************
//*****************************************************************************
static void usage(void);
static GADI_ERR handle_pm_command(int argc, char* argv[]);

//*****************************************************************************
//*****************************************************************************
//** API Functions
//*****************************************************************************
//*****************************************************************************
int pm_init(void)
{
    int retVal;

    retVal = gadi_pm_init();

    return retVal;
}

int pm_exit(void)
{
    int retVal;

    retVal = gadi_pm_exit();

    return retVal;
}

int pm_open(void)
{
    GADI_ERR retVal = GADI_OK;

    pmHandle = gadi_pm_open(&retVal);
    if(retVal != GADI_OK)
    {
        printf("gadi_pm_open error\n");
    }

    return retVal;
}

int pm_close(void)
{
    int retVal;

    retVal = gadi_pm_close(pmHandle);

    return retVal;
}

int pm_register_testcase(void)
{
    int   retVal = 0;
    (void)shell_registercommand (
        "pm",
        handle_pm_command,
        "pm command",
        "---------------------------------------------------------------------\n"
        "pm -u 1 -x 0 -y 0 -w 10 -H 10 -c 0 -a 0 -e 1\n"
        "   brief : add include region privacy mask.\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "pm -u 0 -x 0 -y 0 -w 10 -H 10 -c 0 -a 1 -e 1\n"
        "   brief : add exclude region privacy mask.\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "pm -u 1 -x 0 -y 0 -w 10 -H 10 -c 0 -a 3 -e 1\n"
        "   brief : remove all privacy mask.\n"
        "\n"
    );

    return retVal;
}

//*****************************************************************************
//*****************************************************************************
//** Local Functions
//*****************************************************************************
//*****************************************************************************
static void usage(void)
{
    printf("\nusage: osd [OPTION]...[Text String]...\n");
    printf("\t-h, --help            help\n"
           "\t-u, --unit            coordinate unite: 0:precent, 1-pixel\n"
           "\t-x, --xoffset         coordinate x offset.\n"
           "\t-y, --yoffset         coordinate y offset.\n"
           "\t-w, --width           privacy mask width.\n"
           "\t-H, --height          privacy mask height.\n"
           "\t-c, --colour          privacy mask colour,RGB value.\n"
           "\t-a, --action          privacy mask action.\n"
           "\t-e, --enable          enable/disable this action.\n"
           "\t-t, --test            test PM function.\n");

    printf("example:\n");
    printf("\tpm \n"
           " --- add include region privacy mask:\n"
           "\tpm -u 1 -x 0 -y 0 -c 0 -a 0 -e 1\n\n"
           " --- add exclude region privacy mask:\n"
           "\tpm -u 0 -x 0 -y 0 -c 0 -a 1 -e 1\n"
           " --- remove all privacy mask:\n"
           "\tpm -u 1 -x 0 -y 0 -c 0 -a 3 -e 1\n");

    printf("\n");
}

static GADI_ERR handle_pm_command(int argc, char* argv[])
{
    int option_index, ch;
    int retVal;
    unsigned char pmIndex;

    /*change parameters when giving input options.*/
    while ((ch = getopt_long(argc, argv, shortOptions, longOptions, &option_index)) != -1)
    {
        switch (ch)
        {
            case 'h':
                usage();
                goto command_exit;

            case 'u':
                pmPars.unit = atoi(optarg);
                if((pmPars.unit != 0) && (pmPars.unit != 1))
                {
                    printf("bad input unit :%d\n", pmPars.unit);
                    goto command_exit;
                }
                break;

            case 'x':
                pmPars.offsetX = atoi(optarg);
                break;

            case 'y':
                pmPars.offsetY = atoi(optarg);
                break;

            case 'w':
                pmPars.width = atoi(optarg);

                break;
            case 'H':
                pmPars.height = atoi(optarg);
                break;

            case 'c':
                pmPars.colour = atoi(optarg);
                break;
            case 'a':
                pmPars.action = atoi(optarg);
                if(pmPars.action >= GADI_PM_ACTIONS_NUM)
                {
                    printf("bad input action :%d\n", pmPars.action);
                    goto command_exit;
                }
                break;

            case 'e':
                enable = atoi(optarg);
                if((enable != 0) && (enable != 1))
                {
                    printf("bad input enable :%d\n", enable);
                    goto command_exit;
                }
                break;
            default:
                printf("type '--help' for more usage.\n");
                goto command_exit;
        }
    }

    retVal = gadi_pm_malloc(pmHandle, &pmPars, &pmIndex);
    if(retVal != 0)
    {
        printf("gadi_pm_malloc error:%d\n",retVal);
        goto command_exit;
    }

    retVal =  gadi_pm_enable(pmHandle, pmIndex, enable);
    if(retVal != 0)
    {
        printf("gadi_pm_enable error:%d\n",retVal);
        goto command_exit;
    }

command_exit:
    optind = 1;
    return 0;
}

