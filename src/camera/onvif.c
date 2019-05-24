/*
****************************************************************************
** \file      /applications/adidemo/demo/src/onvif.c
**
** \version   $Id: onvif.c du: cannot access ‘onvif.c’: No such file or directory 2016-10-21 11:33:53Z dengbiao $
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
#include <unistd.h>
#include <fcntl.h>
#include <syscall.h>

#include <getopt.h>

#include "shell.h"
#include "adi_types.h"
#include "adi_sys.h"

#include "onvif.h"
#include "web.h"
#include "stdio.h"
#include "string.h"
#include "unistd.h"
#include "http_export.h"

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

//**************************************************************************
//**************************************************************************
//** Global Data
//**************************************************************************
//**************************************************************************
extern int webInit;
static int onvifInit = 0;
typedef enum
{
	NVT_SYS_ERROR = -1,
	NVT_NO_ERROR = 0,
}GONVIF_NVT_Ret_E;

typedef struct
{
    GADI_CHAR *buffer;
    GADI_S32   Len;
}ONVIF_Http_package_S;

extern ONVIF_Http_package_S *soap_proc_GetHttpPackage(void);
extern GONVIF_NVT_Ret_E GK_NVT_SoapProc_InWeb(GADI_S32 clientFd,
    GADI_U32 clientIp, GADI_U32 clientPort, const GADI_CHAR *pszClientStream, GADI_U32 clientSize);
extern GADI_S32 soap_proc_ReleaseHttpPackage(void);

//**************************************************************************
//**************************************************************************
//** Local Data
//**************************************************************************
//**************************************************************************

static pthread_mutex_t g_soapProcMutex;
static const char *shortOptions = "hSP";
static struct option longOptions[] =
{
    {"help",     0, 0, 'h'},
    {"start",    0, 0, 'S'},
    {"stop",     0, 0, 'P'},
    {0,          0, 0, 0}
};

//**************************************************************************
//**************************************************************************
//** Local Functions Declaration
//**************************************************************************
//**************************************************************************
static int handle_onvif_command(int argc, char* argv[]);
int onvif_start(int isFromWeb, int webFd);

//**************************************************************************
//**************************************************************************
//** API Functions
//**************************************************************************
//**************************************************************************
int onvif_register_testcase(void)
{
    int   retVal = 0;
    (void)shell_registercommand (
        "onvif",
        handle_onvif_command,
        "onvif command",
        "---------------------------------------------------------------------\n"
        "onvif -S \n"
        "    brief : start onvif.\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "onvif -P \n"
        "    brief : stop onvif.\n"
        "\n"
    );

    return retVal;
}

//**************************************************************************
//**************************************************************************
//** Local Functions
//**************************************************************************
//**************************************************************************
static void usage(void)
{
    printf("\nusage: onvif [OPTION]...[Text String]...\n");
    printf("\t-h, --help                help\n"
           "\t-S, --start               start onvif\n"
           "\t-P, --stop                stop onvif\n");

    printf("\n");
}

int onvif_proc(HTTP_OPS* ops, void* arg)
{
    pthread_mutex_lock(&g_soapProcMutex);
	GADI_INFO("HTTP: enter CGI(tid: %d).", (pid_t)syscall(SYS_gettid));

    int len;
    ONVIF_Http_package_S *pstHttpPackage;
    const char *content;
    content = ops->get_http_request(ops, &len);
	GADI_INFO("HTTP: enter ONVIF.");
    GK_NVT_SoapProc_InWeb(0, 0, 0, content, len);
	GADI_INFO("HTTP: out ONVIF.");
    pstHttpPackage = soap_proc_GetHttpPackage();
    ops->set_http_respond(ops, pstHttpPackage->buffer, pstHttpPackage->Len);
    soap_proc_ReleaseHttpPackage();

	GADI_INFO("HTTP: out CGI.");

    pthread_mutex_unlock(&g_soapProcMutex);
    return HPE_RET_DISCONNECT;

}
int netcam_http_onvif_init(void)
{
	if(pthread_mutex_init(&g_soapProcMutex, NULL) < 0)
	{
		printf("Fail to initialize soap mutex in CIG.");
		return -1;
	}

    http_mini_add_cgi_callback("onvif", onvif_proc, METHOD_GET|METHOD_POST, NULL);
    http_mini_add_cgi_callback("Subcription", onvif_proc, METHOD_GET|METHOD_POST, NULL);

    return 0;
}

int onvif_open(void)
{
    int ret = 0;

    if (webInit && !onvifInit) {
        ret = netcam_http_onvif_init();
        if (ret != 0)
            return -1;
        ret = onvif_start(GADI_TRUE, 80);
        if (ret != 0)
            return -1;
        onvifInit = 1;
        return 0;
    } else {
        return -1;
    }
}

static int handle_onvif_command(int argc, char* argv[])
{
    int option_index, ch;
    optind = 1;

    /*change parameters when giving input options.*/
    while ((ch = getopt_long(argc, argv, shortOptions, longOptions, &option_index)) != -1)
    {
        switch (ch)
        {
            case 'h':
                usage();
                goto command_exit;
            case 'S':
                onvif_open();
                break;
            case 'P':
                //onvif_stop();
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

