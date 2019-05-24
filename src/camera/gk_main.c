/******************************************************************************
** \file        adi/test/src/main.c
**
** \brief       ADI layer test.
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
#include <getopt.h>
#include <signal.h>

#include "adi_sys.h"
#include "adi_pwm.h"
#include "adi_gpio.h"
#include "adi_pda.h"
#include "vi.h"
#include "vout.h"
#include "venc.h"
#include "isp.h"
#include "shell.h"
#include "gpio.h"
#include "spi.h"
#include "ir.h"
#include "osd.h"
#include "pm.h"
#ifdef AUDIO_I2S_MODE
#include "i2s.h"
#else
#include "audio.h"
#endif
#include "pda.h"
#include "pwm.h"
#include "fb.h"
#include "vdec.h"
#include "interrupts.h"
#include "rtsp.h"
#include "web.h"
#include "tuning.h"
#include "ircut.h"
#include "onvif.h"
#include "zbar.h"
#include "pda.h"
#include "shell.h"
#include "venc.h"
#include "osd.h"
#include "osd_functions.h"
#include "adi_pda.h"

#include "ray_app.h"

//*****************************************************************************
//*****************************************************************************
//** Local Defines
//*****************************************************************************
//*****************************************************************************
#define ADI_TEST_DEBUG_LEVEL    GADI_SYS_LOG_LEVEL_INFO

#define APP_VIDEO_CONFIGURATION_FILE_PATH "/usr/local/bin/video.xml"

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

GADI_SYS_HandleT pwmHandle = NULL;

//*****************************************************************************
//*****************************************************************************
//** Local Functions Declaration
//*****************************************************************************
//*****************************************************************************

static const char *shortOptions = "hadvbc";
static struct option longOptions[] =
{
    {"help",            0, 0, 'h'},
    {"all",             0, 0, 'a'},
    {"audio",           0, 0, 'd'},
    {"vdec",            0, 0, 'v'},
    {"background",      0, 0, 'b'},
    {"client",          0, 0, 'c'},
    {"disable-module",  1, 0, 1},
    {"enable-module",   1, 0, 2},
    {0,                 0, 0, 0}
};
static GADI_S32 work_mode = 0;
static GADI_S32 daemon_flag = 0;

static GADI_U32 module_flag = 0x0;

#define MODULE_AUDIO_ENABLE             (0x1)
#define MODULE_ISP_ENABLE               (0x1<<1)
#define MODULE_VOUT_ENABLE              (0x1<<2)
#define MODULE_VENC_ENABLE              (0x1<<3)
#define MODULE_SHELL_SERVER_ENABLE      (0x1<<7)
#define MODULE_SHELL_CLIENT_ENABLE      (0x1<<8)
#define MODULE_SHELL_CONSOLE_ENABLE     (0x1<<9)
#define MODULE_TUNNING_SERVER_ENABLE    (0x1<<10)

#define MODULE_FLAG_CLEAR(n)        (module_flag &= (~(n)))
#define MODULE_FLAG_SET(n)          (module_flag |= (n))

/* enable shell client, this function is connect to shelle server. */
#define shell_client_module         "shell-client"
#define shell_client_enable()       ({MODULE_FLAG_CLEAR(MODULE_SHELL_SERVER_ENABLE); \
                                    MODULE_FLAG_SET(MODULE_SHELL_CLIENT_ENABLE);})
#define shell_client_disable()      ({MODULE_FLAG_CLEAR(MODULE_SHELL_CLIENT_ENABLE);})
#define is_shell_client_enbale()    (module_flag&MODULE_SHELL_CLIENT_ENABLE)

/* enable shell server, this function is build one local server, wait only one client connect. */
#define shell_server_module         "shell-server"
#define shell_server_enable()       ({MODULE_FLAG_CLEAR(MODULE_SHELL_CLIENT_ENABLE); \
                                    MODULE_FLAG_SET(MODULE_SHELL_SERVER_ENABLE);})
#define shell_server_disable()      ({MODULE_FLAG_CLEAR(MODULE_SHELL_SERVER_ENABLE);})
#define is_shell_server_enbale()    (module_flag&MODULE_SHELL_SERVER_ENABLE)

/* enable shell console,  is accept cmd input. */
#define shell_console_module         "shell-console"
#define shell_console_enable()      ({MODULE_FLAG_SET(MODULE_SHELL_CONSOLE_ENABLE);})
#define shell_console_disable()     ({MODULE_FLAG_CLEAR(MODULE_SHELL_CONSOLE_ENABLE);})
#define is_shell_console_enbale()   (module_flag&MODULE_SHELL_CONSOLE_ENABLE)

/* enable auto ISP function, define enable, when open tunning module and use option '-a'. */
#define isp_module                  "isp"
#define isp_enable()                ({MODULE_FLAG_SET(MODULE_ISP_ENABLE);})
#define isp_disable()               ({MODULE_FLAG_CLEAR(MODULE_ISP_ENABLE);})
#define is_isp_enbale()             (module_flag&MODULE_ISP_ENABLE)

#define tunning_module              "tuning"
#define tunning_enable()            ({MODULE_FLAG_SET(MODULE_TUNNING_SERVER_ENABLE);})
#define tunning_disable()           ({MODULE_FLAG_CLEAR(MODULE_TUNNING_SERVER_ENABLE);})
#define is_tunning_enbale()         (module_flag&MODULE_TUNNING_SERVER_ENABLE)

#define set_daemon_mode()           ({daemon_flag = 1;})
#define set_audio_mode()            ({work_mode = 3;})
#define set_vdec_mode()            ({work_mode = 4;})
#define set_all_work_mode()         ({work_mode = 1;})
#define set_module_mode()           ({work_mode = 2;})

#define is_daemon_mode()            (daemon_flag == 1)
#define is_audio_mode()             (work_mode == 3)
#define is_vdec_mode()             (work_mode == 4)
#define is_all_work_mode()          (work_mode == 1)
#define is_module_mode()            (work_mode == 2)

//*****************************************************************************
//*****************************************************************************
//** Local Functions
//*****************************************************************************
//*****************************************************************************


zbar_image_scanner_t *scanner = NULL;

typedef struct BIT_MAP_FILE_HEADER
{
    GADI_U8 type[2]; // "BM" (0x4d42)
    GADI_U32 file_size;
    GADI_U32 reserved_zero;
    GADI_U32 off_bits; // data area offset to the file set (unit. byte)
    GADI_U32 info_size;
    GADI_U32 width;
    GADI_U32 height;
    GADI_U16 planes; // 0 - 1
    GADI_U16 bit_count; // 0 - 1
    GADI_U32 compression; // 0 - 1
    GADI_U32 size_image; // 0 - 1
    GADI_U32 xpels_per_meter;
    GADI_U32 ypels_per_meter;
    GADI_U32 clr_used;
    GADI_U32 clr_important;
}__attribute__((packed)) BIT_MAP_FILE_HEADER_t; //

extern char saoma_buf_ssid[32];
extern char saoma_buf_psk[32];


int rayshine_parse_wifi_info(char *data,char *ssid,char *psk,int *flag)
{
	printf("---------rayshine_parse_wifi_info------------\n");
	char buf[64] = {0};
	strcpy(buf,data);
	char *tmp = buf;
	char *result = NULL;
	result = strsep(&tmp,"#");
	if(result){
		printf("result = %s,tmp = %s\n",result,tmp);
		strcpy(ssid,result);
		strcpy(psk,tmp);
		*flag = 1;
		return 0;
	}else{
		*flag = 0;
		return -1;
	}

	
}

static int sdk_vin_convert_zbar(int width,int height,char *data,char *ssid,char *pw,int *flag)
{
	int ret = 0;
	//printf(" (c) Goke Microelectronics China 2009 - 2016   \n");
	scanner = zbar_image_scanner_create();
	/* configure the reader */
	zbar_image_scanner_set_config(scanner, 0, ZBAR_CFG_ENABLE, 1);
	/* obtain image data */
	//printf("width:%d,height:%d\n",width,height);
	void *raw = NULL;
	raw = malloc(width*height);
	memset(raw, 0, width*height);
	memcpy(raw, data, width*height);
	//printf("size: %d\n", strlen(raw));
	//get_data(argv[1], width, height, &raw); 
	/* wrap image data */
	zbar_image_t *image = zbar_image_create();
	zbar_image_set_format(image, *(int*)"Y800");
	zbar_image_set_size(image, width, height); 
	zbar_image_set_data(image, raw, width * height, zbar_image_free_data);
	/* scan the image for barcodes */
	ret = zbar_scan_image(scanner, image);

    #if 1
    //printf("nsyms size: %d\n", ret);
	if (ret == 0){
		zbar_image_destroy(image);
		zbar_image_scanner_destroy(scanner);
		return -1;	}

    #endif
    /* extract results */
	const zbar_symbol_t *symbol = zbar_image_first_symbol(image);
	for(; symbol; symbol = zbar_symbol_next(symbol))
	{
		/* do something useful with results */ 
		//memset(saoma_buf_ssid,0,sizeof(saoma_buf_ssid));
		//memset(saoma_buf_psk,0,sizeof(saoma_buf_psk));
		zbar_symbol_type_t typ = zbar_symbol_get_type(symbol);
		const char *data = zbar_symbol_get_data(symbol);
		printf("======>>>>>>>decoded %s symbol \"%s\"\n",zbar_get_symbol_name(typ), data);
		ret = rayshine_parse_wifi_info(data,ssid,pw,flag);
		//ret = sdk_vin_parse_wifi_info(data,ssid,pw,enMode);
    }
	zbar_image_destroy(image);
	zbar_image_scanner_destroy(scanner);
	return 0;
}

static GADI_S32 pda_capture_bmp(GADI_U32 channelIndex,char *ssid,char *psk,int *flag)
{
	int  width = 0;
	int  height = 0;
	size_t  stride = 0;
	size_t y = 0;
	unsigned char* byteY = NULL;
	unsigned char* py ;
	int i,ii,ret;

    GADI_PDA_Frame_InfoT video_frame_info;
    GADI_PDA_CAP_Buffer_TypeT buffer = GADI_PDA_CAP_MAIN_BUFFER;
	#if 0
    FILE *bitmap_stream = fopen("/opt/pda_111.bmp","wb");
    if(bitmap_stream == NULL)
    {
        GADI_ERROR("pda_capture_bmp: open file error\n");
        return -1;
    }
	#endif
    if (0 == channelIndex)
    {
        buffer = GADI_PDA_CAP_MAIN_BUFFER;
    }
    else if (1 == channelIndex)
    {
        buffer = GADI_PDA_CAP_SECOND_BUFFER;
    }
    else if (3 == channelIndex)
    {
        buffer = GADI_PDA_CAP_FOURTH_BUFFER;
    }

    if(gadi_pda_capture_buffer(buffer, &video_frame_info) != GADI_OK)
    {
        printf("pda_capture_bmp: failed\n");
		return -1;
    }
    else
    {
        //printf("gadi_pda_capture_buffer ok\n");

		width = video_frame_info.width;
		height = video_frame_info.height;
		stride = video_frame_info.stride;
		
		y = width * height ;
		byteY = malloc(y);
		//printf("[ %d x %d] stride %d, y = %d\n",  width, height, stride); //[ 1280 x 720] stride 1280
		for(i = 0; i < height; ++i)
		{
			py	= (uint8_t*)(video_frame_info.yAddr+ stride * i);
			for(ii = 0; ii < width; ++ii)
			{
				if(py[ii] == 0){
					byteY[i*width+ii] = py[ii]+1;
				}
				else{
					byteY[i*width+ii] = py[ii];
				}
			}
		}

		ret = sdk_vin_convert_zbar(width,height,byteY,ssid,psk,flag);
		free(byteY);
		return ret;
    }
    return -1;
}





void* saoma_func(void *args)
{
	printf("---------saoma_func---------\n");
	int ret = 0;
	char psd[256] = {0};
    char ssid[256] = {0};


	SAOMA_INFO_S *info = (SAOMA_INFO_S *)args;


	int retVal=gadi_pda_init(GADI_PDA_MD_FIRST_BUFFER);
	if(retVal != GADI_OK)
	{
		printf("[INFO] gadi_pda_init: failed %d\n",retVal);
	}else{
		printf("[INFO] gadi_pda_init ok\n");
		#if 0
		formatPar.streamId = 0;
		retVal = gdm_venc_get_stream_format(&formatPar);
		#endif
		while(1){
			ret = pda_capture_bmp(0,info->saoma_buf_ssid,info->saoma_buf_psk,&info->flag);
			printf("---------saoma_func---------ret = %d\n",ret);
			if(ret == 0)
				break;
			//usleep(50*1000);
		}
		printf("----------saoma_func OVER------------------\n");
	}

	return NULL;
}

int create_saoma_func(SAOMA_INFO_S *info)
{
	printf("---------create_saoma_func-------1111\n");
	memset(info->saoma_buf_ssid,0,sizeof(info->saoma_buf_ssid));
	memset(info->saoma_buf_psk,0,sizeof(info->saoma_buf_psk));
	info->flag = 0;
	
	if (pthread_create(&info->tid, NULL, saoma_func, (void *)info) == -1) 
	{
		printf("Create saoma_t pthread failed");
	}
	pthread_detach(info->tid);
	printf("---------create_saoma_func-------2222\n");
	return 0;
}


// dataÊÇ´Ó¶þÎ¬ÂëÖÐ»ñÈ¡µÄ×Ö·û´®£¬¸Ãº¯Êý¸ù¾Ý×Ô¶¨ÒåµÄ×Ö·û´®¸ñÊ½È¥½âÎössidºÍpwd.
//date    ×Ó´®¸ñÊ½:
//"XXXX:\r ssid:\r xxxx:\r passwd:\r enmode:\r"
static int sdk_vin_parse_wifi_info(char *data,char *ssid,char *pw,char *enMode)
{	
    printf("=================>>>>[%s]\n",data);
	#if 0
	int ret = 0;	char *p = ":";
	char *q;	int i = 0;
	q = strtok(data,":");
    if(q)
    {
        strcpy(ssid,q);
        printf("ssid >%s<\n",ssid);
    }
    
	q = strtok(NULL,":");
    if(q)
    {
        strcpy(pw,q);
        printf("pwd >%s<\n",pw);
    }

	q = strtok(NULL,":");
    if(q)
    {
        strcpy(enMode,q);
        printf("enMode >%s<\n",enMode);
    }

	#endif
    
    #if 0
	if (strcmp(q,"S"))
		return -1;
  
	printf("%s\n",q);	while((q = strtok(NULL,p)))	{
		if (i ==0)
		{
			strcpy(ssid,q);
			printf("ssid >%s<\n",ssid);
		}
		else if (i == 2)
		{
			strcpy(pw,q);
			printf("passwd >%s<\n",pw);
		}
		else if (i == 3)
		{
			strcpy(enMode,q);
			printf("enMode >%c<\n",*enMode);
		}
		i++;
	} 
	#endif
	return 0;
}

#ifdef MODULE_SUPPORT_TURING
#include "turing_api.h"
#endif

#include <json-c/json.h>
#include <curl/curl.h>
#include <stdlib.h> /* exit, atoi, malloc, free */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h> /* struct hostent, gethostbyname */
#include <sys/time.h>
#include <ctype.h>
#include <time.h>



#define	TURING_RD_BOOK_TITLE "http://smartdevice.ai.tuling123.com/speech/chat"


static char upload_head[] = 
	"POST /speech/chat HTTP/1.1\r\n"
    "Host: %s\r\n"
    "Connection: keep-alive\r\n"
	"Content-Length: %d\r\n"
    "Cache-Control: no-cache\r\n"
    "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36\r\n"
	"Content-Type: multipart/form-data; boundary=%s\r\n"
    "Accept: */*\r\n"
   // "Accept-Encoding: gzip, deflate\r\n"
    "Accept-Language: en-US,en;q=0.8,zh-CN;q=0.6,zh;q=0.4,zh-TW;q=0.2,es;q=0.2\r\n"
    "\r\n";

static char upload_parameters[] = 
	"Content-Disposition: form-data; name=\"parameters\"\r\n\r\n%s";

static char upload_speech[] = 
    "Content-Disposition: form-data; name=\"speech\"; filename=\"turing.jpg\"\r\n"
    "Content-Type: application/octet-stream\r\n\r\n";

void error(const char *msg) { perror(msg); exit(0); }


long getCurrentTime()    
{
   struct timeval tv;    
   gettimeofday(&tv,NULL);    
   return tv.tv_sec * 1000 + tv.tv_usec / 1000;    
}
/* * »ñÈ¡Ëæ»ú×Ö·û´® */

void get_rand_str(char s[], int number)
{
    char *str = "abcdefghijklmnopqrstuvwxyzzyxwvutsrqponmlkjhgfedcba";
    int i,lstr;
    char ss[2] = {0};
    lstr = strlen(str);//¼ÆËã×Ö·û´®³¤¶È
    srand((unsigned int)time((time_t *)NULL));//Ê¹ÓÃÏµÍ³Ê±¼äÀ´³õÊ¼»¯Ëæ»úÊý·¢ÉúÆ÷
    for(i = 1; i <= number; i++){//°´Ö¸¶¨´óÐ¡·µ»ØÏàÓ¦µÄ×Ö·û´®
       sprintf(ss,"%c",str[(rand()%lstr)]);//rand()%lstr ¿ÉËæ»ú·µ»Ø0-71Ö®¼äµÄÕûÊý, str[0-71]¿ÉËæ»úµÃµ½ÆäÖÐµÄ×Ö·û
       strcat(s,ss);//½«Ëæ»úÉú³ÉµÄ×Ö·û´®Á¬½Óµ½Ö¸¶¨Êý×éºóÃæ
    }
}


struct resp_header{    
	int status_code;//HTTP/1.1 '200' OK    
    char content_type[128];//Content-Type: application/gzip    
	long content_length;//Content-Length: 11683079    
};

static void get_resp_header(const char *response,struct resp_header *resp)
{        
	char *pos = strstr(response, "HTTP/");    
	if (pos)        
		sscanf(pos, "%*s %d", &resp->status_code);//è¿”å›žçŠ¶æ€ç     
		pos = strstr(response, "Content-Type:");//è¿”å›žå†…å®¹ç±»åž‹    
		if (pos)        
			sscanf(pos, "%*s %s", resp->content_type);    
		pos = strstr(response, "Content-Length:");//å†…å®¹çš„é•¿åº¦(å­—èŠ‚)    
		if (pos)        
			sscanf(pos, "%*s %ld", &resp->content_length);    
}


/* * ¹¹ÔìÇëÇó£¬²¢·¢ËÍÇëÇó */

void buildRequest(int socket_fd, char *file_data, int len, int asr_type, int realtime, int index, char *identify, char *host)
{
    char *boundary_header = "------AiWiFiBoundary";
    char* end = "\r\n"; 			
	char* twoHyphens = "--";		
    char s[20] = {0};
    get_rand_str(s,19);
  printf("%s\n",s);
    char *boundary = malloc(strlen(boundary_header)+strlen(s) +1);
	//char boundary[strlen(boundary_header)+strlen(s) +1];
	memset(boundary, 0, strlen(boundary_header)+strlen(s) +1);
    strcat(boundary, boundary_header);
    strcat(boundary, s);
    printf("boundary is : %s\n", boundary);
    char firstBoundary[128]={0};
    char secondBoundary[128]={0};
    char endBoundary[128]={0};
    sprintf(firstBoundary, "%s%s%s", twoHyphens, boundary, end);
    sprintf(secondBoundary, "%s%s%s%s", end, twoHyphens, boundary, end);
    sprintf(endBoundary, "%s%s%s%s%s", end, twoHyphens, boundary, twoHyphens, end);
    
    cJSON *root = cJSON_CreateObject();
    //cJSON *seceneCodes = cJSON_CreateObject();
	//cJSON_AddItemToObject(root, "seceneCodes", seceneCodes = cJSON_CreateArray());
	//cJSON_AddNumberToObject(seceneCodes,"seceneCodes", 20028);
    cJSON_AddStringToObject(root,"ak","9836726864a94d0da2b275b92346fe56");
	cJSON_AddNumberToObject(root,"asr", asr_type);
	cJSON_AddNumberToObject(root,"type", 4);
	cJSON_AddNumberToObject(root,"tts", 2);
    cJSON_AddNumberToObject(root,"flag", 3);
	cJSON_AddNumberToObject(root,"tone", 20);
	cJSON_AddStringToObject(root,"uid","cfe5a80d2b5bc08099fd8494f542f156");
	cJSON_AddStringToObject(root,"token","ddad61fabd9f41b2b58db862f3bb21af");	

	#if 1
	cJSON *extra = cJSON_CreateObject();

	cJSON_AddNumberToObject(extra,"bookHeight",250);
	cJSON_AddNumberToObject(extra,"cameraAngle",50);
	cJSON_AddNumberToObject(extra,"cameraHeight",125);
	cJSON_AddNumberToObject(extra,"fl",252.5);
	cJSON_AddNumberToObject(extra,"imageHeight",240);
	cJSON_AddNumberToObject(extra,"imageWidth",320);
	cJSON_AddNumberToObject(extra,"imageQuality",90);
	cJSON_AddStringToObject(extra,"imgFlagId","187148b529c147a298c160ae71f2a6d2");
	cJSON_AddNumberToObject(extra,"innerUrlFlag",1);
	cJSON_AddStringToObject(extra,"bookName","187148b529c147a298c160ae71f2a6d2");//
	//char* extra_str = cJSON_PrintUnformatted(extra);
	//cJSON_AddStringToObject(root,"extra",extra_str);
	cJSON_AddItemReferenceToObject(root,"extra",extra);
    #else

	json_object *extra = NULL;
	extra = json_object_new_object();
	json_object_object_add(extra, "imgFlagId", json_object_new_string("187148b529c147a298c160ae71f2a6d2"));

	printf("---------->>> : %s\n",json_object_get_string(extra));

	cJSON_AddStringToObject(root,"extra",json_object_get_string(extra));
	#endif


	char* str_js = cJSON_Print(root);
    cJSON_Delete(root);
	cJSON_Delete(extra);
    printf("parameters is : %s\n", str_js);
    char *parameter_data = malloc(strlen(str_js)+ strlen(upload_parameters) + strlen(boundary) + strlen(end)*2 + strlen(twoHyphens) +1);
    sprintf(parameter_data, upload_parameters, str_js);
    strcat(parameter_data, secondBoundary);

    int content_length = len+ strlen(boundary)*2 + strlen(parameter_data) + strlen(upload_speech) + strlen(end)*3 + strlen(twoHyphens)*3;
    //printf("content length is %d \n", content_length);
    
    char header_data[4096] = {0};
    int ret = snprintf(header_data,4096, upload_head, host, content_length, boundary);

    //printf("Request header is : %s\n", header_data);

    //header_data,boundary,parameter_data,boundary,upload_speech,fileData,end,boundary,boundary_end
    send(socket_fd, header_data, ret,0);
    printf("%s\n", header_data);
    send(socket_fd, firstBoundary, strlen(firstBoundary),0);
    printf("%s\n", firstBoundary);
    send(socket_fd, parameter_data, strlen(parameter_data),0);
    printf("%s\n", parameter_data);

    send(socket_fd, upload_speech, strlen(upload_speech),0);
    printf("%s\n", upload_speech);
    int w_size=0,pos=0,all_Size=0;
	while(1){
		//printf("ret = %d\n",ret);
		pos =send(socket_fd,file_data+w_size,len-w_size,0);
		w_size +=pos;
		all_Size +=len;
		if( w_size== len){
			w_size=0;
			break;
		}
	}

    send(socket_fd, endBoundary, strlen(endBoundary),0);

    free(boundary);
    free(parameter_data);
    free(str_js);
}


/* * »ñÈ¡ÏìÓ¦½á¹û */

void getResponse(int socket_fd, char **text)
{
    /* receive the response */
    char response[4096];
    memset(response, 0, sizeof(response));
    int length = 0,mem_size=4096;
    struct resp_header resp;
    int ret=0;
    while (1)	{	
		ret = recv(socket_fd, response+length, 1,0);
		if(ret<=0)
			break;
		//ÕÒµ½ÏìÓ¦Í·µÄÍ·²¿ÐÅÏ¢, Á½¸ö"\r\n"Îª·Ö¸îµã		  
		int flag = 0;	
		int i;			
		for (i = strlen(response) - 1; response[i] == '\n' || response[i] == '\r'; i--, flag++);
		if (flag == 4)			  
			break;		  
		length += ret;	
		if(length>=mem_size-1){
			break;
		}
	}
	get_resp_header(response,&resp); //	/*»ñÈ¡ÏìÓ¦Í·µÄÐÅÏ¢*/  
    printf("resp.content_length = %ld status_code = %d\n",resp.content_length,resp.status_code);
	if(resp.status_code!=200||resp.content_length==0){
		return;
	}
	char *code = (char *)calloc(1,resp.content_length+1);
	if(code==NULL){
		return;
	}
	ret=0;
	length=0;
	while(1){
		ret = recv(socket_fd, code+length, resp.content_length-length,0);
		if(ret<=0){
			free(code);
			break;
		}
		length+=ret;
		//printf("result = %s len=%d\n",code,len);		
		if(length==resp.content_length)
			break;
	}
    printf("response is %s\n",code);
    
    *text = code;
}

int get_socket_fd(char *host)
{
    int portno = 80;
    int sockfd;
    struct hostent *server;
    struct sockaddr_in serv_addr;
    
    /* create the socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("ERROR opening socket");

    /* lookup the ip address */
    server = gethostbyname(host);
    if (server == NULL) error("ERROR, no such host");

    /* fill in the structure */
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    memcpy(&serv_addr.sin_addr.s_addr,server->h_addr,server->h_length);

    /* connect the socket */
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");
    return sockfd;
    
}

/*******************************************
@º¯Êý¹¦ÄÜ:	json½âÎö·þÎñÆ÷Êý¾Ý
@²ÎÊý:	pMsg	·þÎñÆ÷Êý¾Ý
***********************************************/

static void parseJson_string(const char * pMsg){
	if(NULL == pMsg){
		return;
    }
    cJSON * pJson = cJSON_Parse(pMsg);
	if(NULL == pJson){
       	return;
    }
	cJSON *pSub = cJSON_GetObjectItem(pJson, "token");//èŽ·å–tokençš„å€¼ï¼Œç”¨äºŽä¸‹ä¸€æ¬¡è¯·æ±‚æ—¶ä¸Šä¼ çš„æ ¡éªŒå€¼
	if(pSub!=NULL){
		printf("TokenValue = %s\n",pSub->valuestring);
		//updateTokenValue(pSub->valuestring);
	}
    pSub = cJSON_GetObjectItem(pJson, "code");
    if(NULL == pSub){
		goto exit;
	}
	switch(pSub->valueint){
		case 40001:
        case 40002:
		case 40003:
		case 40004:
		case 40005:		
		case 40006:	
		case 40007:
		case 305000:
		case 302000:
		case 200000:
			goto exit;
	}
	pSub = cJSON_GetObjectItem(pJson, "asr");
    if(NULL == pSub){
		goto exit;
    }
	printf("info: %s \n",pSub->valuestring);
	pSub = cJSON_GetObjectItem(pJson, "tts");
	if(NULL != pSub){
        printf("tts: %s \n",pSub->valuestring);
	}
#if 0	
    if (cJSON_HasObjectItem(pJson, "nlp")) {
        cJSON *item = cJSON_GetObjectItem(pJson, "nlp");
        cJSON *subitem = cJSON_GetArrayItem(item, 0);
        printf("url is   %s\n", subitem->valuestring);
        char *urlname = malloc(strlen(subitem->valuestring) + 1);
        memset(urlname, 0, strlen(subitem->valuestring) + 1);
        sprintf(urlname, "play %s", subitem->valuestring);
        //system(urlname);
        if (urlname) {
            free(urlname);
            urlname = NULL;
        }

    }
    if (cJSON_HasObjectItem(pJson, "func")) {
        cJSON *func = cJSON_GetObjectItem(pJson, "func");
        cJSON *url = cJSON_GetObjectItem(func, "url");
        printf("url is   %s\n", url->valuestring);
        char *urlname = malloc(strlen(url->valuestring) + 1);
        memset(urlname, 0, strlen(url->valuestring) + 1);
        sprintf(urlname, "play %s", url->valuestring);
        //system(urlname);
        if (urlname) {
            free(urlname);
            urlname = NULL;
        }
    }
#endif
exit:
    if (pJson) {
        cJSON_Delete(pJson);
        pJson = NULL;
    }
}


int readFile(const char *file_name, char **file_data, int * length)
{
    if(access(file_name,F_OK) != 0){
		printf("file %s not exsit!\n",file_name);
		return -1;
	}
	FILE *fp = fopen(file_name,"rb");
	if(fp == NULL){
		printf("open file %s error!\n",file_name);
		return -1;
	}
	fseek(fp,0,SEEK_END);
	
	int file_len = ftell(fp);
	fseek(fp,0,SEEK_SET);
	char *fileData = (char *)calloc(1,file_len+1);
	if(fileData==NULL)
		return -1;
	int len = fread(fileData,1,file_len,fp);
    *file_data = fileData;
    *length = len;
	fclose(fp);
    return 0;
}


/* * ·¢ËÍÇëÇó£¬¶ÁÈ¡ÏìÓ¦ */

void requestAndResponse(int socket_fd, char *file_data, int len, int asr_type, int realtime, int index, char *identify, char *host)
{
    char *text = NULL;
    buildRequest(socket_fd,file_data,len,asr_type,realtime,index,identify,host);
    long start_time = getCurrentTime();
    printf("Send finish at %ld.\r\n", start_time); 
    getResponse(socket_fd, &text);
    long end_time = getCurrentTime();
    printf("Response finish at %ld.\r\n", end_time);
    printf("COST TIME £º%ld\n", end_time-start_time);
    if(text){
        parseJson_string((const char * )text);
        free(text);
    }else{
        printf("req failed\n");
    }
}

//http://smartdevice.ai.tuling123.com/speech/chat
int func_turing_readbook()
{
	//tur_read_book_test("/usr/data/turing.jpg");
    //asr_type 0:pcm_16K_16bit; 1:pcm_8K_16bit; 2:amr_8K_16bit; 3:amr_16K_16bit; 4:opus

    int asr_type = 5;
    char *host = "smartdevice.ai.tuling123.com";

	 char *file_name= "/usr/data/turing_1.jpg";
	 char *fileData = NULL;
	 int len = 0;
	 if(readFile(file_name, &fileData, &len) != 0)
		 return -1;
	 int sockfd = get_socket_fd(host);
	 
	 int size = 5*1024* 1024;  
	 printf("len is %d\n",len);
	 if(len > size)
	 {
		 //Á÷Ê½Ê¶±ð
		 int piece = (len % size != 0) ? (len / size + 1) : (len / size);
		 char identify[33] = {0};
		 get_rand_str(identify,32);
		 
		 int i=0;
		 for(i=0;i<piece;i++)
		 {
			 requestAndResponse(sockfd,fileData+size*i,(i == (piece - 1)) ? (len - size * i) : size,asr_type,1,(i==(piece - 1))?(-i-1):(i+1),identify,host);
		 }
	 }
	 else 
	 {
		//·ÇÁ÷Ê½Ê¶±ð
		 requestAndResponse(sockfd,fileData,len,asr_type,0,0,NULL,host);
	 }
	 
	 free(fileData);
	 close(sockfd);

	return 0;
}


int tmp_turing_turn_page()
{
#if 0
	MotionDetection mt;
	int min_thr = 40;
	int max_thr = 100;
	int minThrWaitTime = 100;
	int dstHeight = 100;
	int cropRate = 33;
	int serverWaitTime = 0;
	int timer = 100;
	unsigned char image[MAX_CAP_FRAEM_SIZE] = {0};
	int result = -1;
	long long nowTime = getCurrentTime();

	
	initMotionDetection(&mt,min_thr,max_thr,minThrWaitTime,dstHeight,cropRate,serverWaitTime);

	memset(image,0,sizeof(image));
	turing_pda_get_y_12(0,image);
	getMotionResult(&mt,image,640,360,2,TL_CV_YUV2GRAY_420,getCurrentTime())
#endif
	return 0;
}
int tmp_get_y_picture()
{
	printf("---------tmp_get_y_picture---------\n");	
	int retVal=gadi_pda_init(GADI_PDA_MD_FIRST_BUFFER);
	
	if(retVal != GADI_OK)
	{
		printf("[INFO] gadi_pda_init: failed %d\n",retVal);
	}else{
		printf("[INFO] gadi_pda_init ok\n");

			GADI_VENC_StreamFormatT stream_format;
			stream_format.width = 640,stream_format.height = 360;
			gdm_venc_set_resolution(stream_format.channelId, stream_format.width, stream_format.height);

			int return_page = -1;
			int playing_page = -2;
			//ret = pda_capture_bmp(0);
			char cmd[128] = {0};
			
			while(1){
				printf("$$$$$$$$$$$$$$$$$$\n");
				return_page = pda_cap_yuv_12(0);
				printf(">>>>>>>>>>>>>>>>>>>playing_page = %d,return_page = %d\n",playing_page,return_page);
				if((return_page >= 0)&&(playing_page != return_page)){
					printf("************************PLAYING*********************\n");
					//mozart_module_pause();
					playing_page = return_page;
					memset(cmd,0,sizeof(0));
				
					sprintf(cmd,"echo \"loadfile http://file.rayshine.cc/books/book0/%d.mp3\" > /var/run/mplayer0/infifo",playing_page);
					printf("cmd = %s\n",cmd);
					system(cmd);
				}

				usleep(150*1000);
			}
	}

	return 0;
}


GADI_ERR app_initialize(void)
{	printf("-----------app_initialize--------------\n");
    GADI_ERR   err      = GADI_OK;
    GADI_U32   streamId = 0;
    GADI_U32   shellflag = 0;

    /* set debug level*/
	printf("----------set debug level---------------\n");
    if (is_daemon_mode()) {
        err = gadi_sys_set_log_level(GADI_SYS_LOG_LEVEL_ERROR);
    } else {
        err = gadi_sys_set_log_level(ADI_TEST_DEBUG_LEVEL);
    }
	
    /* system module init.*/
	printf("----------- system module init--------------\n");
    err |= gadi_sys_init();
    if(!is_audio_mode()) {
        err = gadi_sys_load_firmware();
    }


        /*load video configuration file.*/
		printf("----------load video configuration file.---------------\n");
        err = gdm_vi_parse_config_file(APP_VIDEO_CONFIGURATION_FILE_PATH);
        if(err != 0) {
            printf("load vi configuration file:%s failed.\n", APP_VIDEO_CONFIGURATION_FILE_PATH);
        }
        err = gdm_vout_parse_config_file(APP_VIDEO_CONFIGURATION_FILE_PATH);
        if(err != 0) {
            printf("load vout configuration file:%s failed.\n", APP_VIDEO_CONFIGURATION_FILE_PATH);
        }
        err = gdm_venc_parse_config_file(APP_VIDEO_CONFIGURATION_FILE_PATH);
        if(err != 0) {
            printf("load venc configuration file:%s failed.\n", APP_VIDEO_CONFIGURATION_FILE_PATH);
        }
		
        /* init video modules.*/
		printf("----------init video modules---------------\n");
        err = gdm_vi_init();
        err |= gdm_vout_init();
        err |= gdm_venc_init();
        err |= gdm_vi_open();
        err |= gdm_vout_open();
        err |= gdm_venc_open();

        if (err != GADI_OK) {
            printf("Applicantions initialize failed!\n");
        }

        gdm_vi_setup();
        gdm_vout_setup();
        gdm_venc_setup();
        gdm_vout_setup_after();

        for (streamId = 0; streamId < GADI_VENC_STREAM_NUM; streamId++)
        {
            err =  gdm_venc_start_encode_stream(streamId);
            if(err != 0)
            {
                printf("start stream[%d]\n",streamId);
            }
        }

        /* init isp modules. */
		printf("-----------init isp modules--------------\n");
        if (is_isp_enbale()) {
            err = isp_init();
            if (err == 0) {
                err = isp_open();
                if (err != 0) {
                    isp_exit();
                }
            }

            if (err == 0) {
                err = isp_start();
                if (err != 0) {
                    isp_close();
                    isp_exit();
                } else {
                    isp_custom_code();
                }
            }
            if (is_tunning_enbale()) {
                gadi_isp_tuning_start();
            }
        }

        /*init osd modules.*/
        //err = osd_init();
        //err = osd_open();

        /*init pm modules. */
        //err = pm_init();
        //err = pm_open();

        /*init audio modules.*/
#ifdef AUDIO_I2S_MODE
        //err = gdm_i2s_init();
#else
        //err = gdm_audio_init();
#endif
	#if 0
		GADI_VENC_FrameRateT framerate;
		GADI_VENC_StreamFormatT stream_format;
		framerate.streamId = 0;
		
        if(gdm_venc_get_framerate(&framerate)) 
           printf(" gdm_venc_get_framerate failed\n");
        else
			printf("streamId = 0, fps = %d\n",framerate.fps);
		framerate.fps = GADI_VENC_FPS_15;
		gdm_venc_set_framerate(&framerate);
		
		framerate.streamId = 1;
		gdm_venc_set_framerate(&framerate);
		
		stream_format.channelId = 0;
		//gdm_venc_get_stream_format(&stream_format);
		//printf("get_resolution BEFORE width = %d,height = %d\n",stream_format.width,stream_format.height);//640 * 480


		stream_format.width = 640,stream_format.height = 480;
		gdm_venc_set_resolution(stream_format.channelId, stream_format.width, stream_format.height);
	#endif
	#if 0	
        err = rtsp_server_start();

		//tmp_get_y_picture();

		 //exit(0);

//        err = web_open();

//        err = onvif_open();

		 
        framerate.streamId = 0;
		if(gdm_venc_get_framerate(&framerate)) 
           printf(" gdm_venc_get_framerate failed\n");
        else
			printf("streamId = 0 fps_set = %d\n",framerate.fps);
		
		framerate.streamId = 1;
        if(gdm_venc_get_framerate(&framerate)) 
           printf("111 gdm_venc_get_framerate failed\n");
        else
			printf("streamId = 1 fps_set = %d\n",framerate.fps);

	
		//gdm_venc_get_stream_format(&stream_format);
		//printf("get_resolution AFTER width = %d,height = %d\n",stream_format.width,stream_format.height);
		
		//Media_SetBitRate
		GADI_VENC_BitRateRangeT bitrate;
		bitrate.streamId = 0;
   		gdm_venc_get_bitrate(&bitrate);
		printf("streamId = %d,brcMode = %d,cbrAvgBps = %d,vbrMinbps = %d,vbrMaxbps = %d\n",
			bitrate.streamId,bitrate.brcMode,bitrate.cbrAvgBps,bitrate.vbrMinbps,bitrate.vbrMaxbps);
	#endif
		

    return err;
}

int rayshine_set_camera()
{
	GADI_VENC_FrameRateT framerate;
	GADI_VENC_StreamFormatT stream_format;
	framerate.streamId = 0;
	
	if(gdm_venc_get_framerate(&framerate)) 
	   printf(" gdm_venc_get_framerate failed\n");
	else
		printf("streamId = 0, fps = %d\n",framerate.fps);
	framerate.fps = GADI_VENC_FPS_15;
	gdm_venc_set_framerate(&framerate);
	
	framerate.streamId = 1;
	gdm_venc_set_framerate(&framerate);
	
	stream_format.channelId = 0;

	stream_format.width = 640,stream_format.height = 480;
	gdm_venc_set_resolution(stream_format.channelId, stream_format.width, stream_format.height);

	return 0;

}









