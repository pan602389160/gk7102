/******************************************************************************
** \file        adidemo/miniweb/src/web.c
**
**
** \brief       web demo test.
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
#include <sys/mman.h>
#include <getopt.h>
#include <sys/ioctl.h>

#include "adi_vi.h"
#include "adi_vout.h"
#include "venc.h"
#include "web.h"
#include "ircut.h"
//*****************************************************************************
//*****************************************************************************
//** Local Defines
//*****************************************************************************
//*****************************************************************************
#define   RESOURCE_DIR "/opt/resource"
//#define   RESOURCE_FONT_DIR  RESOURCE_DIR"/font"
#define   RESOURCE_WEB_DIR  RESOURCE_DIR"/web"
#define	  RESOURCE_SNAPSHOT_DIR	RESOURCE_WEB_DIR"/snapshot"

#define   MAX_JSON_LENGTH       8196
#define   ENABLE_BITRATE_SET    1
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

extern GADI_VI_SettingParamsT   viParams;
extern GADI_VOUT_SettingParamsT voParams;
extern video_encode_stream      streams[GADI_VENC_STREAM_NUM];


//*****************************************************************************
//*****************************************************************************
//** Local Data
//*****************************************************************************
//*****************************************************************************

//static const char shortOptions[] = "hosrc";
static const char shortOptions[] = "hsrc";

static struct option longOptions[] =
{
    {"help",     0, 0, 'h'},
    //{"open",     0, 0, 'o'},
    {"start",    0, 0, 's'},
    {"close",    0, 0, 'c'},
    {0,          0, 0, 0}
};

int webInit = 0;

//*****************************************************************************
//*****************************************************************************
//** Local Functions Declaration
//*****************************************************************************
//*****************************************************************************
static void  web_usage(void);
static int   handle_app_command(int argc, char* argv[]);

static void  http_cgi_register(void);
static int   http_cgi_snapshotjpg(HTTP_OPS* ops, void* arg);

#if ENABLE_BITRATE_SET
static int   http_cgi_stream_control(HTTP_OPS* ops, void* arg);
static char *json_stream_control_string(int id);
#endif
//*****************************************************************************
//*****************************************************************************
//** API Functions
//*****************************************************************************

int web_open(void)
{
    int i = 0;
    NETCAM_VIDEO_STREAM_Porpety videoPro[4];

    if(webInit)
        return -2;

    if (http_mini_server_init("0.0.0.0", 80, RESOURCE_WEB_DIR) < 0)
    {
        return -1;
        printf("Create http server failed\n");
    }
    http_cgi_register();
    http_mini_server_run();
    for(i = 0;i < 3; i++)
    {
        videoPro[i].bufId = i;
        videoPro[i].streamId = i;
        videoPro[i].maxStreamNum = 3;
    }
    netcam_video_web_stream_init(videoPro, 3);

    webInit = 1;
    return 0;
}

int web_close(void)
{
    if(!webInit)
        return -2;

    // release media resource
    netcam_video_web_stream_exit();
    // release web resource
    //http_mini_server_exit();

    webInit = 0;
    return 0;
}

int web_register_testcase(void)
{
    int   retVal = 0;
    (void)shell_registercommand (
        "web",
        handle_app_command,
        "web command",
        "---------------------------------------------------------------------\n"
        "web -s\n"
        "   brief : start miniweb.\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "web -c\n"
        "   brief : close miniweb.\n"
        "\n"
    );

    return retVal;
}

//*****************************************************************************
//*****************************************************************************
//** Local Functions
//*****************************************************************************
//*****************************************************************************
static void web_usage(void)
{
    printf("\nusage: web [OPTION]\n");

    printf("\t-h, --help            help\n"
           "\t-s, --start         open  miniweb.\n"
           "\t-c, --close         close miniweb.\n");
    printf("\n");
}

static GADI_ERR handle_app_command(int argc, char* argv[])
{
    int option_index, ch;
    int ret = 0;
    /*change parameters when giving input options.*/
    while ((ch = getopt_long(argc, argv, shortOptions, longOptions, &option_index)) != -1)
    {
        switch (ch)
        {
            case 'h':
                web_usage();
                goto command_exit;

            case 's':
                // start miniweb
                ret = web_open();
                if(ret == -2)
                    printf("miniweb had been opened.\n");
                else if(ret == -1)
                {
                    printf("Fail to open miniweb.\n");
                    goto command_exit;
                }
                break;

            case 'c':
                // close miniweb
                ret = web_close();
                if(ret == -2)
                    printf("miniweb is not open or had been closed.\n");
                else if(ret == -1)
                {
                    printf("Fail to close miniweb.\n");
                    goto command_exit;
                }
                break;

            default:
                printf("type '--help' for more usage.\n");
                goto command_exit;
        }
    }

    optind = 1;
    return 0;

command_exit:
    optind = 1;
    return -1;
}

static int http_cgi_snapshotjpg(HTTP_OPS* ops, void* arg)
{
    int ret = 0;

    ret = gdm_venc_capture_jpeg(0, "/tmp/web_snapshot2.jpg");
    if(ret != 0)
    {
        printf("Fail to still capture.\n");
        return HPE_RET_FAIL;
    }

    return HPE_RET_SUCCESS;
}


#if ENABLE_BITRATE_SET
static int http_cgi_stream_control(HTTP_OPS* ops, void* arg)
{
    int method = ops->get_method(ops);
    char *buf;
    int streamId;
	char buffer[9096]={0};

    //printf("Method: %s\n", http_get_method_string(method));
#if 0   // unauthoized
	if(netcam_http_service(ops)==0)
		return HPE_RET_UNAUTHORIZED;
#endif
	memset(buffer,0,sizeof(buffer));
    if(method == METHOD_GET)
    {
        buf = (char *)ops->get_param_string(ops,"streamId");
        if(buf == NULL)
        {
            printf("Cant not get stream ID.");
            return HPE_RET_FAIL;
        }
        streamId = atoi(buf);
        if(streamId < 0 || streamId >4)
        {
            printf("steamid error:%d", streamId);
            return HPE_RET_FAIL;
        }
        char *jsonBuf;
        jsonBuf = json_stream_control_string(streamId);
        if(jsonBuf == NULL)
        {
            printf("Cant not get stream ID.");
            return HPE_RET_FAIL;
        }
	    gbk_to_utf8(jsonBuf, buffer, sizeof(buffer));
        ops->set_body_ex(ops, buffer, strlen(buffer));
        free(jsonBuf);
    }
    else
    {
        cJSON *opt, *tmp;
        char retData[256];
        int ret = 0;
        int streamId;
        int bodyLen = 0;

        buf = (char *)ops->get_body(ops,&bodyLen);
        if(buf == NULL)
        {
            printf("No http body\n");
            return HPE_RET_FAIL;

        }
		utf_to_gbk(buf, buffer, sizeof(buffer));
        // parse cjson
        tmp = cJSON_Parse(buffer);
        if(tmp == NULL)
        {
            printf("http body json error\n");
            return HPE_RET_FAIL;
        }
        //change bps
        if ((opt=cJSON_GetObjectItem(tmp,"rate_change")) != NULL) {
            int rateMode = 0;
            int cbrbps = 0;
            int vbrbps_max = 0;
            int vbrbps_min = 0;

            streamId = cJosn_Read_Int(opt, "id");
            rateMode = cJosn_Read_Int(opt,"h264_brcMode");
            cbrbps = cJosn_Read_Int(opt,"h264_cbrAvgBps");
            vbrbps_max = cJosn_Read_Int(opt,"h264_vbrBpsMax");
            vbrbps_min = cJosn_Read_Int(opt,"h264_vbrBpsMin");

            GADI_VENC_BitRateRangeT bitrate;

            bitrate.streamId = streamId;
            gdm_venc_get_bitrate(&bitrate);

            if ((rateMode&0x1) == 0) {
                bitrate.brcMode = rateMode;
                bitrate.cbrAvgBps = cbrbps*1000;
            } else {
                bitrate.brcMode = rateMode;
                bitrate.vbrMaxbps = vbrbps_max*1000;
                bitrate.vbrMinbps = vbrbps_min*1000;
            }
            ret = gdm_venc_set_bitrate(&bitrate);
            if(ret != 0)
            {
                printf("Fail to set h264 bitrate.\n");
                //return HPE_RET_FAIL;
            }
        }
        //change resolution
        else if ((opt=cJSON_GetObjectItem(tmp,"res_change")) != NULL) {
            int res_width = 0;
            int res_heigh = 0;

            streamId = cJosn_Read_Int(opt, "id");
            res_width = cJosn_Read_Int(opt,"res_width");
            res_heigh = cJosn_Read_Int(opt,"res_heigh");

            ret = gdm_venc_set_resolution(streamId, res_width, res_heigh);
            if(ret != 0)
            {
                printf("Fail to set resolution [%dx%d].\n", res_width, res_heigh);
                //return HPE_RET_FAIL;
            }
            isp_restart();
        }
        //change day/night work mode work_mod
        else if ((opt=cJSON_GetObjectItem(tmp,"work_mode")) != NULL) {
            int workmode = 0;

            workmode = opt->valueint;
            ircut_switch_manual(workmode);
        }
        else {
            ret = -1;
        }

        cJSON_Delete(tmp);
        sprintf(retData, "{\"statusCode\": \"%d\"}", ret);
        ops->set_body_ex(ops, retData, strlen(retData));
    }

    return HPE_RET_SUCCESS;
}
#endif

static void http_cgi_register(void)
{
#if ENABLE_BITRATE_SET
	http_mini_add_cgi_callback("/video", http_cgi_stream_control, METHOD_GET|METHOD_PUT, (void *)0);
#endif
    http_mini_add_cgi_callback("/snapshotjpg", http_cgi_snapshotjpg, METHOD_GET, (void *)0);
}

static int res_arry[GADI_VENC_STREAM_NUM][8][2] =
{
    //only frist stream support
    {
        {1920, 1080},
        {1280, 960},
        {1280, 720},
        {1024, 600},
        {0, 0},
    },
    {
        {720, 576},
        {720, 480},
        {640, 480},
        {672, 378},
        {640, 360},
        {0, 0},
    },
    {
        {480, 272},
        {352, 288},
        {320, 240},
        {0, 0},
    },
    {
        {0, 0},
    },
};
#if ENABLE_BITRATE_SET
static char *json_stream_control_string(int id)
{
    char *jsonbuf = malloc(MAX_JSON_LENGTH);
    int   offset = 0;
    int i, j;

    //generate resolution option
    char res_buf[1024] = {"\"resProperty\": {\"type\":\"U32\",\"mode\":\"rw\",\"min\":0,\"max\":8192,\"def\":2000,\"opt\":["};
    for (i = id; i < GADI_VENC_STREAM_NUM; i ++) {
        for (j = 0; j < (sizeof(res_arry[0])/sizeof(res_arry[0][0])) && res_arry[i][j][0] != 0; j++) {
            sprintf(res_buf, "%s\"%dx%d\", ", res_buf, res_arry[i][j][0], res_arry[i][j][1]);
        }
    }
    strcat(res_buf, "\"0x0\"]}");

    offset += snprintf(jsonbuf+offset, MAX_JSON_LENGTH-offset, "{");
    offset += snprintf(jsonbuf+offset, MAX_JSON_LENGTH-offset, "\"id\": %d,", id);
    offset += snprintf(jsonbuf+offset, MAX_JSON_LENGTH-offset, "\"idProperty\": {\"type\":\"S32\",\"mode\":\"rw\",\"min\":0,\"max\":3,\"def\":0,\"opt\":\"0-3\"},");
    offset += snprintf(jsonbuf+offset, MAX_JSON_LENGTH-offset, "\"h264_brcMode\": %d,", streams[id].h264Conf.brcMode);
    offset += snprintf(jsonbuf+offset, MAX_JSON_LENGTH-offset, "\"h264_brcModeProperty\": {\"type\":\"U8\",\"mode\":\"rw\",\"min\":0,\"max\":3,\"def\":0,\"opt\":\"0: CBR; 1: VBR; 2: CBR keep quality; 3: VBR keep quality\"},");
    offset += snprintf(jsonbuf+offset, MAX_JSON_LENGTH-offset, "\"h264_cbrAvgBps\": %d,", streams[id].h264Conf.cbrAvgBps/1000);
    offset += snprintf(jsonbuf+offset, MAX_JSON_LENGTH-offset, "\"h264_vbrBpsMin\": %d,", streams[id].h264Conf.vbrMinbps/1000);
    offset += snprintf(jsonbuf+offset, MAX_JSON_LENGTH-offset, "\"h264_vbrBpsMax\": %d,", streams[id].h264Conf.vbrMaxbps/1000);
    offset += snprintf(jsonbuf+offset, MAX_JSON_LENGTH-offset, "\"h264_rateProperty\": {\"type\":\"U32\",\"mode\":\"rw\",\"min\":0,\"max\":40000,\"def\":2000,\"opt\":\"\"},");
    offset += snprintf(jsonbuf+offset, MAX_JSON_LENGTH-offset, "\"res_width\": %d,", streams[id].streamFormat.width);
    offset += snprintf(jsonbuf+offset, MAX_JSON_LENGTH-offset, "\"res_height\": %d,", streams[id].streamFormat.height);
    offset += snprintf(jsonbuf+offset, MAX_JSON_LENGTH-offset, res_buf);
    offset += snprintf(jsonbuf+offset, MAX_JSON_LENGTH-offset, "}");

    return jsonbuf;
}
#endif
