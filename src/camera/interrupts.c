/*
****************************************************************************
** \file      /applications/adidemo/src/interrupts.c
**
** \version   $Id: interrupts.c 4 2016-03-10 17:44:58Z dengbiao $
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

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/time.h>

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <pthread.h>

#include "adi_types.h"
#include "adi_sys.h"
#include "adi_vi.h"
#include "basetypes.h"
#include "shell.h"
#include "parser.h"




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

static const char *shortOptions = "hSPt";
static struct option longOptions[] =
{
	{"help",	 0, 0, 'h'},
	{"start",	 0, 0, 'S'},
    {"stop",     0, 0, 'P'},
    {"thread",   0, 0, 't'},
	{0, 		 0, 0, 0}
};
extern GADI_SYS_HandleT viHandle;

//*****************************************************************************
//*****************************************************************************
//** Local Functions Declaration
//*****************************************************************************
//*****************************************************************************

static void Usage(void);
static GADI_ERR handle_interrupt_command(int argc, char* argv[]);

//*****************************************************************************
//*****************************************************************************
//** API Functions
//*****************************************************************************
//*****************************************************************************


GADI_ERR interrupt_register_testcase(void)
{
    GADI_ERR   retVal =  GADI_OK;
    (void)shell_registercommand (
        "interrupt",
        handle_interrupt_command,
        "interrupt command",
        "---------------------------------------------------------------------\n"
        "interrupt -S \n"
        "   brief : start look up interrupt speed. \n"
        "\n"
        "---------------------------------------------------------------------\n"
        "interrupt -P \n"
        "   brief : stop look up interrupt speed. \n"
        "\n"
        "---------------------------------------------------------------------\n"
        "interrupt -t \n"
        "   brief : print thread statistics. \n"
        "\n"
        "---------------------------------------------------------------------\n"
        "\n"
    );

    return retVal;
}
//*****************************************************************************
//*****************************************************************************
//** Local Functions
//*****************************************************************************
//*****************************************************************************
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static GADI_U8 calculate_run_flag = 1;

static void interrupt_thread(void *arg)
{
    GADI_ERR ret;
    struct timeval encode_time_pre, encode_time_next;
    struct timeval vi_time_pre, vi_time_next;
    GADI_ULONG encode_num_pre, encode_num_next;
    GADI_ULONG vi_frame_num_pre, vi_framenum_next;
    ret = gadi_sys_get_interrupt(&encode_num_pre);
    if (ret != GADI_OK) {
        calculate_run_flag = GADI_FALSE;
    }
    gettimeofday(&encode_time_pre, NULL);
    ret = gadi_vi_get_interrupt(viHandle, &vi_frame_num_pre);
    if (ret != GADI_OK) {
        calculate_run_flag = GADI_FALSE;
    }
    gettimeofday(&vi_time_pre, NULL);

    while(calculate_run_flag){
        sleep(1);
        gadi_sys_get_interrupt(&encode_num_next);
        gettimeofday(&encode_time_next, NULL);
        if(encode_num_next < encode_num_pre){
            encode_num_pre = encode_num_next;
            continue;
        }
        sleep(1);
        gadi_vi_get_interrupt(viHandle, &vi_framenum_next);
        gettimeofday(&vi_time_next, NULL);
        if(vi_framenum_next < vi_frame_num_pre){
            vi_frame_num_pre = vi_framenum_next;
            continue;
        }
        printf("\033[2J\033[0;0H");
        printf("\t############## interrupts speed test #############\n");
        printf("\t#                                                #\n");
        printf("\t# [dsp:%04.4f per/s] | [vi_core:%04.4f per/s]  #\n",
            (float)(encode_num_next - encode_num_pre)*100
                /((encode_time_next.tv_sec - encode_time_pre.tv_sec)*100 +
                (encode_time_next.tv_usec -  encode_time_pre.tv_usec)/10000),
            (float)(vi_framenum_next - vi_frame_num_pre)*100
                /((vi_time_next.tv_sec - vi_time_pre.tv_sec)*100 +
                (vi_time_next.tv_usec -  vi_time_pre.tv_usec)/10000));
        printf("\t#                                                #\n");
        printf("\t##################################################\n");
        encode_num_pre = encode_num_next;
        encode_time_pre = encode_time_next;
        vi_frame_num_pre = vi_framenum_next;
        vi_time_pre = vi_time_next;
    }
    pthread_mutex_unlock(&mutex);
    gadi_sys_thread_self_destroy();
}

static void start_calculate_interrupt(void)
{
    GADI_ERR ret = GADI_OK;
    ret = pthread_mutex_trylock(&mutex);
    if(ret != 0){
        GADI_ERROR("interrupt_thread is running. ");
        return;
    }
    calculate_run_flag = 1;
    ret = gadi_sys_thread_create(interrupt_thread, NULL, GADI_SYS_THREAD_PRIO_DEFAULT,
        GADI_SYS_THREAD_STATCK_SIZE_DEFAULT, "interrupt_thread", NULL);
    if (ret != 0) {
        pthread_mutex_unlock(&mutex);
        GADI_ERROR("gadi_sys_thread_create interrupt_thread failed");
        return;
    }
}

static void stop_calculate_interrupt(void)
{
    GADI_ERR ret = GADI_OK;
    ret = pthread_mutex_trylock(&mutex);
    if(ret == 0){
        pthread_mutex_unlock(&mutex);
        GADI_ERROR("interrupt_thread is not run. ");
        return;
    }
    calculate_run_flag = 0;
    GADI_INFO("send stop signal to interrupt_thread.");
}

static void Usage(void)
{
    printf("\nusage: video [OPTION]...\n");
    printf("\t-h, --help            help.\n"
           "\t-S, --start           start look interrupts.\n"
           "\t-P, --stop            stop look interrupts.\n"
           "\t-t, --thread          print thread statistics.\n"
          );

	printf("example:\n");
	printf("\tint -S\n"
		   " --- start look interrupts. \n");
	printf("\n");
}

static GADI_ERR handle_interrupt_command(int argc, char* argv[])
{
    int option_index, ch;

    /*change parameters when giving input options.*/
    while ((ch = getopt_long(argc, argv, shortOptions, longOptions, &option_index)) != -1)
    {
        switch (ch)
        {
        case 'h':
            Usage();
            break;
        case 'S':
            start_calculate_interrupt();
            break;
        case 'P':
            stop_calculate_interrupt();
            break;
        case 't':
            gadi_sys_thread_statistics();
            break;
        default:
            GADI_ERROR("bad params\n");
            break;
        }
    }
    optind = 1;
    return 0;
}

