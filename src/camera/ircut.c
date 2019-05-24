/*
****************************************************************************
** \file      /applications/adidemo/demo/src/ircut.c
**
** \version   $Id: ircut.c 0 2016-09-19 10:14:14Z dengbiao $
**
** \brief     videc abstraction layer header file.
**
** \attention THIS SAMPLE CODE IS PROVIDED AS IS. GOFORTUNE SEMICONDUCTOR
**            ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**            OMMISSIONS.
**
** (C) Copyright 2015-2016 by GOKE MICROELECTRONICS CO.,LTD
**
****************************************************************************
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

#include "ircut.h"
#include "shell.h"
//**************************************************************************
//**************************************************************************
//** Local Defines
//**************************************************************************
//**************************************************************************

//**************************************************************************
//**************************************************************************
//** Local structures
//**************************************************************************
//**************************************************************************
typedef struct {
    GADI_GPIO_ADC_ChannelT      channel;
    GADI_IRCUT_TriggerModeEnumT trgrmode;
    GADI_U32                    shreshold;
    GADI_U32                    redundancy;
    void                        (*(daytonight[4]))(GADI_U32);
    void                        (*(nighttoday[4]))(GADI_U32);
} IRCUT_HandleT;


//**************************************************************************
//**************************************************************************
//** Global Data
//**************************************************************************
//**************************************************************************

//**************************************************************************
//**************************************************************************
//** Local Data
//**************************************************************************
//**************************************************************************

static IRCUT_HandleT ircut_handle_arry[2] =
{
    [0] = {
        GADI_GPIO_ADC_CHANNEL_ONE,
        GADI_IRCUT_OverShresholdTrigger,
        0,
        0,
        {NULL, NULL, NULL, NULL},
        {NULL, NULL, NULL, NULL},
    },
    [1] = {
        GADI_GPIO_ADC_CHANNEL_TWO,
        GADI_IRCUT_OverShresholdTrigger,
        0,
        0,
        {NULL, NULL, NULL, NULL},
        {NULL, NULL, NULL, NULL},
    },
};

static const char *ircutShortOptions = "hremd";

static struct option ircutLongOptions[] =
{
	{"help",	    0, 0, 'h'},
    {"run",         0, 0, 'r'},
    {"exit",        0, 0, 'e'},
    {"manual",      0, 0, 'm'},
    {"defcfg",      0, 0, 'd'},
	{0, 		    0, 0, 0}
};

static GADI_BOOL ircut_exit_flag = GADI_FALSE;
static GADI_U32 ircutIsDay[sizeof(ircut_handle_arry)/sizeof(ircut_handle_arry[0])] =
    { [ 0 ... ((sizeof(ircut_handle_arry) / sizeof(ircut_handle_arry[0]) - 1)) ] = 1};

static GADI_U32 ircut_channel;
static GADI_U32 ircut_shreshold;
static GADI_U32 ircut_redundancy;
static GADI_IRCUT_TriggerModeEnumT ircut_trgrmode;

//**************************************************************************
//**************************************************************************
//** Local Functions Declaration
//**************************************************************************
//**************************************************************************

//**************************************************************************
//**************************************************************************
//** API Functions
//**************************************************************************
//**************************************************************************


GADI_ERR ircut_configurate(GADI_U32 channel, GADI_U32 shreshold,
        GADI_U32 redundancy, GADI_IRCUT_TriggerModeEnumT trgrmode)
{
    if (shreshold < redundancy ||
        channel >= sizeof(ircut_handle_arry) / sizeof(ircut_handle_arry[0]))
        return -1;

    ircut_handle_arry[channel].shreshold = shreshold;
    ircut_handle_arry[channel].redundancy = redundancy;
    ircut_handle_arry[channel].trgrmode = trgrmode;

    return 0;
}

GADI_ERR ircut_register_daytonight(GADI_U32 channel, void (*daytonight)(GADI_U32 value))
{
    int i;
    if (daytonight == NULL ||
        channel >= sizeof(ircut_handle_arry) / sizeof(ircut_handle_arry[0]))
        return -1;

    for (i = 0; i < sizeof(ircut_handle_arry[channel].daytonight) /
        sizeof(ircut_handle_arry[channel].daytonight[0]); i++) {
        if (ircut_handle_arry[channel].daytonight[i] == NULL) {
            ircut_handle_arry[channel].daytonight[i] = daytonight;
            return 0;
        }
    }

    return -2;
}

GADI_ERR ircut_register_nighttoday(GADI_U32 channel, void (*nighttoday)(GADI_U32 value))
{
    int i;
    if (nighttoday == NULL ||
        channel >= sizeof(ircut_handle_arry) / sizeof(ircut_handle_arry[0]))
        return -1;

    for (i = 0; i < sizeof(ircut_handle_arry[channel].nighttoday) /
        sizeof(ircut_handle_arry[channel].nighttoday[0]); i++) {
        if (ircut_handle_arry[channel].nighttoday[i] == NULL) {
            ircut_handle_arry[channel].nighttoday[i] = nighttoday;
            return 0;
        }
    }

    return -2;
}


GADI_VOID ircut_switch_thread(GADI_VOID *arg)
{
   GADI_GPIO_AdcValue adc_val[sizeof(ircut_handle_arry) / sizeof(ircut_handle_arry[0])];
   int i, dev_idx;


   GADI_INFO("%s run\n", __func__);
   while(!ircut_exit_flag) {
        for (dev_idx = 0; dev_idx < sizeof(adc_val) / sizeof(adc_val[0]); dev_idx++) {
            adc_val[dev_idx].channel = ircut_handle_arry[dev_idx].channel;
            adc_val[dev_idx].value = 0;
            if (gadi_gpio_read_adc(&adc_val[dev_idx]) != GADI_OK)
                continue;

            if (adc_val[dev_idx].value > ircut_handle_arry[dev_idx].shreshold
                + ircut_handle_arry[dev_idx].redundancy) {

                /* over shreshold, touch option */
                if (ircut_handle_arry[dev_idx].trgrmode == GADI_IRCUT_OverShresholdTrigger && ircutIsDay[dev_idx]) {
                    for (i = 0; i < sizeof(ircut_handle_arry[dev_idx].daytonight) /
                        sizeof(ircut_handle_arry[dev_idx].daytonight[0]); i++) {
                        if (ircut_handle_arry[dev_idx].daytonight[i] != NULL) {
                            ircut_handle_arry[dev_idx].daytonight[i](adc_val[dev_idx].value);
                        }
                    }
                    ircutIsDay[dev_idx] = 0;
                } else if (ircut_handle_arry[dev_idx].trgrmode == GADI_IRCUT_BelowShresholdTrigger && !ircutIsDay[dev_idx]) {
                    for (i = dev_idx; i < sizeof(ircut_handle_arry[dev_idx].nighttoday) /
                        sizeof(ircut_handle_arry[dev_idx].nighttoday[0]); i++) {
                        if (ircut_handle_arry[dev_idx].nighttoday[i] != NULL) {
                            ircut_handle_arry[dev_idx].nighttoday[i](adc_val[dev_idx].value);
                        }
                    }
                    ircutIsDay[dev_idx] = 1;
                }
            } else if (adc_val[dev_idx].value < ircut_handle_arry[dev_idx].shreshold
                - ircut_handle_arry[dev_idx].redundancy) {

                /* below shreshold, touch option */
                if (ircut_handle_arry[dev_idx].trgrmode == GADI_IRCUT_OverShresholdTrigger && !ircutIsDay[dev_idx]) {
                    for (i = dev_idx; i < sizeof(ircut_handle_arry[dev_idx].nighttoday) /
                        sizeof(ircut_handle_arry[dev_idx].nighttoday[0]); i++) {
                        if (ircut_handle_arry[dev_idx].nighttoday[i] != NULL) {
                            ircut_handle_arry[dev_idx].nighttoday[i](adc_val[dev_idx].value);
                        }
                    }
                    ircutIsDay[dev_idx] = 1;
                } else if (ircut_handle_arry[dev_idx].trgrmode == GADI_IRCUT_BelowShresholdTrigger && ircutIsDay[dev_idx]) {
                    for (i = 0; i < sizeof(ircut_handle_arry[dev_idx].daytonight) /
                        sizeof(ircut_handle_arry[dev_idx].daytonight[0]); i++) {
                        if (ircut_handle_arry[dev_idx].daytonight[i] != NULL) {
                            ircut_handle_arry[dev_idx].daytonight[i](adc_val[dev_idx].value);
                        }
                    }
                    ircutIsDay[dev_idx] = 0;
                }
            }
            //printf("channel:%d value:%d\n", dev_idx, adc_val[dev_idx].value);
        }
        gadi_sys_thread_sleep(1000);
    }
    GADI_INFO("%s exit\n", __func__);
}

static void ircut_run(void)
{
    ircut_exit_flag = GADI_FALSE;
    /* running auto IR-cut*/
    gadi_sys_thread_create(ircut_switch_thread, NULL, GADI_SYS_THREAD_PRIO_DEFAULT,
        GADI_SYS_THREAD_STATCK_SIZE_DEFAULT, "IRcut-SW", NULL);
}

static void ircut_exit(void)
{
    ircut_exit_flag = GADI_TRUE;
}

void ircut_switch_manual(GADI_U32 isday)
{
    int i, dev_idx;
    int adc_total;

    if (!ircut_exit_flag) {
        ircut_exit();
        gadi_sys_thread_sleep(1000);
    }

    adc_total = sizeof(ircut_handle_arry) / sizeof(ircut_handle_arry[0]);

    if (isday != 0) {
        for (dev_idx = 0; dev_idx < adc_total; dev_idx++) {
            for (i = 0; i < sizeof(ircut_handle_arry[dev_idx].nighttoday) /
                sizeof(ircut_handle_arry[dev_idx].nighttoday[0]); i++) {
                if (ircut_handle_arry[dev_idx].nighttoday[i] != NULL) {
                    ircut_handle_arry[dev_idx].nighttoday[i](0);
                }
            }
            ircutIsDay[dev_idx] = 1;
        }
    } else {
        for (dev_idx = 0; dev_idx < adc_total; dev_idx++) {
            for (i = 0; i < sizeof(ircut_handle_arry[dev_idx].daytonight) /
                sizeof(ircut_handle_arry[dev_idx].daytonight[0]); i++) {
                if (ircut_handle_arry[dev_idx].daytonight[i] != NULL) {
                    ircut_handle_arry[dev_idx].daytonight[i](0);
                }
            }
            ircutIsDay[dev_idx] = 0;
        }
    }
}

static void ircut_usage(void)
{
    printf("\nusage: venc [OPTION]...\n");
    printf(
        "\t-h, --help             help.\n"
        "\t-r, --run              run IR-cut auto switch.\n"
        "\t-e, --exit             exit IR-cut auto switch.\n"
        "\t-m, --manual           manual set IR-cut switch.\n"
        "\t-d, --defcfg           configurate auto switch.\n"
          );
	printf("\n");
}

static GADI_ERR handle_ircut_command(int argc, char *argv[])
{
    int option_index;
    int ch;
	GADI_ERR retVal;
    optind = 1;

    /*change parameters when giving input options.*/
    while ((ch = getopt_long(argc, argv, ircutShortOptions, ircutLongOptions, &option_index)) != -1)
    {
        switch (ch)
        {
            case 'h':
            ircut_usage();
            break;
            case 'r':
            ircut_run();
            break;
            case 'e':
            ircut_exit();
            break;
            case 'm':
            CREATE_INPUT_MENU(ircutIsDay) {
                ADD_SUBMENU(ircutIsDay[0],
                    "Setup IR-cut is day or night mode [0:night 1:day]."),
                CREATE_INPUT_MENU_COMPLETE();

                if (DISPLAY_MENU() == 0) {
                    ircut_switch_manual(ircutIsDay[0]);
                }
            }
            break;
            case 'd':
            CREATE_INPUT_MENU(config_IRcut) {
                ADD_SUBMENU(ircut_channel,
                    "Setup ADC data channel."),
                ADD_SUBMENU(ircut_shreshold,
                    "Setup IR-cut trigger shreshold."),
                ADD_SUBMENU(ircut_redundancy,
                    "Setup IR-cut trigger redundancy."),
                ADD_SUBMENU(ircut_trgrmode,
                    "Setup IR-cut trigger mode [0:over 1:below]."),
                CREATE_INPUT_MENU_COMPLETE();

                if (DISPLAY_MENU() == 0) {
                    retVal = ircut_configurate(ircut_channel, ircut_shreshold,
                                ircut_redundancy, ircut_trgrmode);
                    if (retVal != GADI_OK) {
                        GADI_ERROR("configurate_ircut failed\n");
                        goto bad_parameter;
                    } else
                        GADI_INFO("configurate_ircut ok\n");
                }
            }
            break;
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


GADI_ERR ircut_register_testcase(void)
{
    GADI_ERR   retVal =  GADI_OK;
    (void)shell_registercommand (
        "ircut",
        handle_ircut_command,
        "ircut command",
        "---------------------------------------------------------------------\n"
        "ircut -h \n"
        "   brief : help\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "ircut -r \n"
        "   brief : run IR-cut auto switch.\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "ircut -e\n"
        "   brief : exit IR-cut auto switch.\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "ircut -m\n"
        "   brief : manual set IR-cut switch.\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "ircut -d\n"
        "   brief : configurate auto switch.\n"
        "\n"
        /******************************************************************/
    );

    return retVal;
}

//**************************************************************************
//**************************************************************************
//** Local Functions
//**************************************************************************
//**************************************************************************

