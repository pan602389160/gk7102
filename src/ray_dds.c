#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <json-c/json.h>

#include "dds.h"
#include "general.h"
#include "asr.h"
#include "ray_app.h"
#include "gk_app.h"
#include "duilite.h"
#include "ai_vr.h"
#include "asoundlib.h"
#include "debug_print.h"

#define buf_size 1024

static int is_get_asr_result = 0;
static int is_get_tts_url = 0;
static int is_dui_response = 0;

struct dds_msg *start_msg = NULL;

static pthread_t start_tid;

//VAD
struct duilite_vad *vad;
char *vad_cfg = "{\"resBinPath\": \"/usr/data/vad/vad_aihome_v0.7.bin\",\"pauseTime\": 500}";


//char *vad_auth_cfg ="{\"productId\":\"278576719\",\"savedProfile\":\"/usr/data/provision\"}";
char *vad_auth_cfg ="{\"productId\":\"278579663\",\"savedProfile\":\"/usr/data/provision\"}";

//char *vad_auth_cfg ="{\"productId\":\"278579663\",\"productKey\":\"c933f76b362a938e02b116b99789b202\",\"productSecret\":\"146263a252081f641894b2192df06173\"}";


static int is_txt_input = 0;


#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>
#include "echocloud_post.h"

#define    AUDIO_FILE   		"/tmp/record_tmp.pcm"
#define    STREAM_URL   		"http://api.echocloud.com/stream"
#define    AUDIO_BUFF_SIZE		(4 * 1024)

#define CONTENT_TYPE			"audio/pcm;bit=16;rate=16000"
/**< 用户和人工智能对话，说话时发送该消息。*/
#define AI_DIALOG_RECOGNIZE  	"ai_dialog.recognize.device" 
/**< 用户输入文字和人工智能对话 
 * 一般也可以用于调试或者设备端有ASR处理模块，可以直接把文字传进来。
 */
#define AI_DIALOG_INPUT			"ai_dialoginput.device" 


#define E_VERSION				"3"
#define	CHANNLE_UUID		"2d779f20-6201-4e2c-b553-cc78d4943dc8"
#define DEVICE_ID			"499c060078534b7eb499c06007853bsd"
#define HASH_KEY			"deS99U7qBWTNQx7fjDHEMGAcdrA4xKqfJtTtj6mv"
#define MAC_ADDR			"00:80:e1:29:e8:02"//@note：MAC_ADDR宏是设备在发送device.online消息时附带的mac_address信息，需要在获得设备mac地址后再来修改这个宏值。
#define	FIRMWARE_VERSION	"0.0.0.1"//@note：FIRMWARE_VERSION为设备固件版本信息。

static stream_info_t stream_info = {
	.content_type = CONTENT_TYPE,
	.x_echocloud_type = AI_DIALOG_RECOGNIZE,
	.x_echocloud_version = E_VERSION
};

static device_info_t gdev_info = {
	.mac_addr = MAC_ADDR,
	.fw_version = FIRMWARE_VERSION,
	.ch_uuid = CHANNLE_UUID,
	.dev_id = DEVICE_ID,
	.hash_key = HASH_KEY
};




CURL *curl;
struct curl_slist *headers = NULL;

static FILE *gfp = NULL;

static size_t get_audioBuff_callback(void *ptr,size_t size,size_t nmemb,void *stream)
{
	curl_off_t nread;	
	size_t retcode = fread(ptr, 1, AUDIO_BUFF_SIZE, gfp);
	//printf("read audio size %ld\n", retcode);
    return retcode;
}


static size_t http_receive_callback( char *ptr, size_t size, size_t nmemb, void *userdata)
{
	printf("date:%s\n count:%ld, %ld\n", ptr, size, nmemb);

	int i = 0;
	json_object *json_msg = NULL;
	
	json_object *count = NULL;
	json_object *messageS = NULL;
		json_object *message = NULL;	
		json_object *type = NULL;
		json_object *data = NULL;
			json_object *audio_enabled = NULL;
			json_object *audio_url = NULL;
			json_object *name = NULL;
	
	
	json_msg= json_tokener_parse(ptr);
	
	json_object_object_get_ex(json_msg,"count",&count);
	json_object_object_get_ex(json_msg,"messages",&messageS);

	if(messageS)
	{
		for(i = 0;i < json_object_get_int(count);i++)
		{	
			message = json_object_array_get_idx(messageS,i);
			json_object_object_get_ex(message,"type",&type);
			json_object_object_get_ex(message,"data",&data);
			json_object_object_get_ex(data,"audio_url",&audio_url);
			if(!strcmp(json_object_get_string(type),"ai_dialog.play"))
			{
				printf("------->ai_dialog.play : %s\n",strdup(json_object_get_string(audio_url)));
				#if 1
				ai_vr_info.asr.sds.output = strdup(json_object_get_string(audio_url));
				ai_vr_info.asr.sds.domain = DOMAIN_CHAT;
				ai_vr_info.asr.sds.is_mult_sds = false;
				#endif
			}
			if(!strcmp(json_object_get_string(type),"media_player.start"))
			{	
				printf("------->media_player.start : %s\n",strdup(json_object_get_string(audio_url)));
				#if 1
				ai_vr_info.asr.sds.music.data[ai_vr_info.asr.sds.music.number++].url = strdup(json_object_get_string(audio_url));
				ai_vr_info.asr.sds.domain = DOMAIN_MUSIC;
				ai_vr_info.asr.sds.is_mult_sds = true;
				#endif
			}
			
		}
	}
	#if 1
	ai_vr_info.asr.errId = 0;
	ai_vr_info.from    = VR_FROM_ASR;
	ai_vr_info.asr.state = ASR_SUCCESS;

	ai_mutex_lock();
	printf("------->ai_vr_info.asr.sds.domain =  %d,is_mult_sds = %d\n",ai_vr_info.asr.sds.domain,ai_vr_info.asr.sds.is_mult_sds);
	if (ai_vr_callback)
		ai_vr_callback(&ai_vr_info);  //b---> callback_from_vr
	ai_mutex_unlock();
	#endif	
	return nmemb;
}

static size_t send_hreard(CURL *curl, struct curl_slist *headers)
{
	//char* tmp[512] = {0};
	headers = curl_slist_append(headers, "Connection:close"); 

	char tmp[128] = {0};
	memset(tmp, 0, sizeof(tmp));
	strcat(tmp, "X-Echocloud-Version:");
	strcat(tmp, stream_info.x_echocloud_version);
	headers = curl_slist_append(headers, tmp);

	memset(tmp, 0, sizeof(tmp));
	strcat(tmp, "X-Echocloud-Channel-Uuid:");
	strcat(tmp, gdev_info.ch_uuid);
	headers = curl_slist_append(headers, tmp);

	memset(tmp, 0, sizeof(tmp));
	strcat(tmp, "X-Echocloud-Device-Id:");
	strcat(tmp, gdev_info.dev_id);
	headers = curl_slist_append(headers, tmp);
	
	headers = curl_slist_append(headers, "X-Echocloud-Nonce:3196dc9990eee9f9c7ccaa9a706b4d2b");//TODO
	headers = curl_slist_append(headers, "X-Echocloud-Signature:240d40a6a31132238fb39581c595961c53de9b957ab5eba055ba69fc4aaff6bd");//TODO

	memset(tmp, 0, sizeof(tmp));
	strcat(tmp, "X-Echocloud-Type:");
	strcat(tmp, stream_info.x_echocloud_type);
	headers = curl_slist_append(headers, tmp);

	memset(tmp, 0, sizeof(tmp));
	strcat(tmp, "Content-Type:");
	strcat(tmp, stream_info.content_type);
	headers = curl_slist_append(headers, tmp);
	
	headers = curl_slist_append(headers, "Transfer-Encoding:chunked"); 
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	return 0;
}


int my_init()
{
	printf("=========my_init=============\n");
	gfp = fopen(AUDIO_FILE, "rb");//AUDIO_FILE
	if(gfp == NULL)
	{
	   printf("file can't open!!!!!!");
	   return -1;
	}

	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	if(curl == NULL) {
    	printf("fcurl_easy_init failed!!!!!!");
	   	return -1;
	}

	curl_easy_setopt(curl,CURLOPT_CUSTOMREQUEST,"POST");  //自定义请求方式
	curl_easy_setopt(curl, CURLOPT_POST, 1L);  //设置非0表示本次操作为POST
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); //在使用该选项且第 3 个参数为 1 时，curl 库会显示详细的操作信息

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_receive_callback);//设置接收数据回调函数 

	curl_easy_setopt(curl, CURLOPT_READFUNCTION, get_audioBuff_callback);	 
	curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
	curl_easy_setopt(curl, CURLOPT_URL, STREAM_URL); 

	send_hreard(curl, headers);



	CURLcode res = curl_easy_perform(curl);	 //完成传输任务

	/* Check for errors */
	if(res != CURLE_OK)
		fprintf(stderr, "curl_easy_perform() failed: %s\n",curl_easy_strerror(res));
	/* always cleanup */
	curl_slist_free_all(headers);		  
	curl_easy_cleanup(curl);        //释放内存
	fclose(gfp);
	curl_global_cleanup();   //在结束libcurl使用的时候，用来对curl_global_init做的工作清理。


	return 0;
}


static int dds_ev_ccb(void *userdata, struct dds_msg *msg) 
{
	int ret = -1;
	int type;
	json_object *out = NULL;
    json_object *nlu = NULL;
	json_object *semantics = NULL;
	json_object *request = NULL;
	json_object *errId = NULL;
	json_object *error = NULL;
	
	if (!dds_msg_get_type(msg, &type)) {
		switch (type) {
		case DDS_EV_OUT_NATIVE_CALL:{
			printf(">>>>>>>>>>>>>>>>>>>>>>>DDS_EV_OUT_NATIVE_CALL\n");
			break;
		}
		case DDS_EV_OUT_COMMAND:{
			printf(">>>>>>>>>>>>>>>>>>>>>>>DDS_EV_OUT_COMMAND\n");
			break;
		}
		case DDS_EV_OUT_MEDIA:{
			printf(">>>>>>>>>>>>>>>>>>>>>>>DDS_EV_OUT_MEDIA\n");
			break;
		}
		case DDS_EV_OUT_DUI_LOGIN:{
			printf(">>>>>>>>>>>>>>>>>>>>>>>DDS_EV_OUT_DUI_LOGIN\n");
			break;
		}
		case DDS_EV_OUT_OAUTH_RESULT:{
			printf(">>>>>>>>>>>>>>>>>>>>>>>DDS_EV_OUT_OAUTH_RESULT\n");
			break;
		}
		case DDS_EV_OUT_PRODUCT_CONFIG_RESULT:{
			printf(">>>>>>>>>>>>>>>>>>>>>>>DDS_EV_OUT_PRODUCT_CONFIG_RESULT\n");
			break;
		}
		case DDS_EV_OUT_WEB_CONNECT:{
			printf(">>>>>>>>>>>>>>>>>>>>>>>DDS_EV_OUT_WEB_CONNECT\n");
			break;
		}
		case DDS_EV_OUT_DUI_DEVICENAME:{
			printf(">>>>>>>>>>>>>>>>>>>>>>>DDS_EV_OUT_DUI_DEVICENAME\n");
			break;
		}
		
		case DDS_EV_OUT_STATUS: {
			printf(">>>>>>>>>>>>>>>>>>>>>>>DDS_EV_OUT_STATUS\n");
			if(!is_txt_input)
			{
				char *value;
				if (!dds_msg_get_string(msg, "status", &value)) {
					printf("dds cur status: %s\n", value);
					if (!strcmp(value, "idle")) {
						dds_status = DDS_STATUS_IDLE;
					} else if (!strcmp(value, "listening")) {
						dds_status = DDS_STATUS_LISTENING;
					} else if (!strcmp(value, "understanding")) {
						dds_status = DDS_STATUS_UNDERSTANDING;
						printf("*****************DDS_STATUS_UNDERSTANDING**********************\n");
						slot_free(&ai_vr_info.asr);
						ai_mutex_lock();
						ai_vr_info.from    = VR_FROM_ASR;
						ai_vr_info.asr.state = ASR_SPEAK_END;
						if (ai_vr_callback)
							ai_vr_callback(&ai_vr_info);   // B --->callback_from_vr
						ai_mutex_unlock();
					}
				}
			}
			break;
		}
		case DDS_EV_OUT_CINFO_RESULT: {
			printf(">>>>>>>>>>>>>>>>>>>>>>>DDS_EV_OUT_CINFO_RESULT\n");
			char *value;
			if (!dds_msg_get_string(msg, "result", &value)) {
				printf("result: %s\n", value);
			}
			if (!dds_msg_get_string(msg, "cinfo", &value)) {
				printf("cinfo: %s\n", value);
			}
			break;
		}
		case DDS_EV_OUT_ASR_RESULT: {
			printf(">>>>>>>>>>>>>>>>>>>>>>>DDS_EV_OUT_ASR_RESULT\n");
			char *value;
			if (!dds_msg_get_string(msg, "var", &value)) {
				printf("\nvar: %s\n", value);
			}
			if (!dds_msg_get_string(msg, "text", &value)) {
				printf("\ntext: %s\n", value);
				is_get_asr_result = 1;
			}
			printf("\nDDS_EV_OUT_ASR_RESULT<<<<<<<<<<<<<<<<<<<\n");
			break;
		}
		case DDS_EV_OUT_TTS: {
			printf(">>>>>>>>>>>>>>>>>>>>>>>DDS_EV_OUT_TTS\n");
			//char *value;
			//-----------------------------------
			#if 0
			ai_mutex_lock();
			ai_vr_info.from    = VR_FROM_ASR;
			ai_vr_info.asr.state = ASR_SUCCESS;

			if (ai_vr_callback)
				ai_vr_callback(&ai_vr_info);  // B --->callback_from_vr
			ai_mutex_lock();
			#endif
			//-----------------------------------
			break;
		}
		case DDS_EV_OUT_DUI_RESPONSE: {
			printf("\n>>>>>>>>>>>>>>>>>>>>>>>DDS_EV_OUT_DUI_RESPONSE is_txt_input = %d\n",is_txt_input);
            char *resp = NULL;
            if(!dds_msg_get_string(msg, "response", &resp)) {  //json_object_new_string(resp) json_object_get_string(json_object_new_string(resp))
            	//printf("\ndui response:\n %s\n\n", resp);
            }
			if(is_txt_input){
				content_free(&ai_vr_info.content);
				if(_content_resolve(json_tokener_parse(resp), &ai_vr_info.content) == -1){
					ai_vr_info.content.state = CONTENT_FAIL;
				} else {
					ai_vr_info.content.state = CONTENT_SUCCESS;
				}
				//is_end_flag = true;  //判断超时用
					
				ai_vr_info.from    = VR_FROM_CONTENT;
				ai_vr_info.content.state = CONTENT_SUCCESS;
				
				is_txt_input = 0;
			}else{
				
				slot_free(&ai_vr_info.asr);
				if(slot_resolve(json_tokener_parse(resp), &ai_vr_info.asr) == -1){
						ai_vr_info.asr.state = ASR_FAIL;
				} else {
						ai_vr_info.asr.state = ASR_SUCCESS;
				}
				//luobo_tech_callback(&ai_vr_info.asr.input);
				ai_vr_info.from    = VR_FROM_ASR;
				ai_vr_info.asr.state = ASR_SUCCESS;
			
			}
			

			ai_mutex_lock();
		
			if (ai_vr_callback)
				ai_vr_callback(&ai_vr_info);  //b---> callback_from_vr
			ai_mutex_unlock();

            is_dui_response = 1;
            break;
        }
		case DDS_EV_OUT_RECORD_AUDIO: {
			//printf(">>>>>>>>>>>>>>>>>>>>>>>DDS_EV_OUT_RECORD_AUDIO\n");
			#if 0
			if (in_asr_pcm && pcm_is_ready(in_asr_pcm)){
				memset(asr_rec_buf,0,sizeof(asr_rec_buf));
				memset(transfor_asr_buf,0,sizeof(transfor_asr_buf));
				
				ret = pcm_read(in_asr_pcm, asr_rec_buf, sizeof(asr_rec_buf));//参考 camera
				//duilite_echo_feed(echo, rec_buf, sizeof(rec_buf));		
				for(buf_index = 0;buf_index < sizeof(asr_rec_buf)/2;buf_index++){	
					inside_index = buf_index/2;
					loop_index = buf_index%2;
					transfor_asr_buf[buf_index] = asr_rec_buf[inside_index*4 + loop_index];
				}
				duilite_vad_feed(vad, transfor_asr_buf, sizeof(asr_rec_buf)/2);
			}
			#endif
			break;
		}
		case DDS_EV_OUT_ERROR: {
			printf(">>>>>>>>>>>>>>>>>>>>>>>DDS_EV_OUT_ERROR\n");
			char *value;
			int errorId = -1;
			if (!dds_msg_get_integer(msg,"errorId",&errorId)) {
				printf("\n--->DDS_EV_OUT_ERROR errorId: %d\n", errorId);
				if(errorId == DDS_ERROR_FATAL){
					
				}
				if(errorId == DDS_ERROR_TIMEOUT){
				
				}
				if(errorId == DDS_ERROR_NETWORK){
					mozart_play_key_sync("/usr/data/tone/badnet.mp3");	
				}
				if(errorId == DDS_ERROR_SERVER){
				
				}
				if(errorId == DDS_ERROR_LOGIC){
				
				}
				if(errorId == DDS_ERROR_INPUT){
				
				}

			}
			if (!dds_msg_get_string(msg, "error", &value)) {
				printf("\n--->DDS_EV_OUT_ERROR: %s\n", value);
			}


			is_dui_response = 1;
			break;
		}
		default:
			break;
		}
	}


	return 0;
}


static int vad_callback(void *user_data, int type, char *msg, int len) 
{
	#if 1
	if (type == DUILITE_MSG_TYPE_JSON) {
		printf("vad_callback======>>>>>%.*s\n", len, msg);

		json_object *out = NULL;
		json_object *status = NULL;
		json_object *result = NULL;
		json_object *recordId = NULL;
		json_object *errId = NULL;
		json_object *error = NULL;

		out = json_tokener_parse((char*) msg);
		if (json_object_object_get_ex(out, "status", &status)){
			if (json_object_get_int(status) == 2){
	            printf("*****************vad end**********************\n"); // B --> callback_from_vr
	            #if 1
				slot_free(&ai_vr_info.asr);
				ai_mutex_lock();
				ai_vr_info.from    = VR_FROM_ASR;
				ai_vr_info.asr.state = ASR_SPEAK_END;
				if (ai_vr_callback)
					ai_vr_callback(&ai_vr_info);
				ai_mutex_unlock();
				#endif
	        }
		} else {
		#if 0
			struct dds_msg *m;
			m = dds_msg_new();
			dds_msg_set_type(m, DDS_EV_IN_AUDIO_STREAM);
			dds_msg_set_bin(m, "audio", msg, len);
			dds_send(m);
			dds_msg_delete(m);
			#endif
		}
		#endif
	return 0;
	}
}


static void *_run(void *arg) {
	
	struct dds_msg *msg = dds_msg_new();
	//dds_msg_set_boolean(msg, "logEnable", 0);
	dds_msg_set_string(msg, "productId", "278579663"); //productId
	dds_msg_set_string(msg, "aliasKey", "test");
	//dds_msg_set_string(msg, "deviceProfile", deviceProfile);//deviceProfile
	dds_msg_set_string(msg, "savedProfile", "/usr/data/provision");
	//dds_msg_set_string(msg, "productKey", "c933f76b362a938e02b116b99789b202");
	//dds_msg_set_string(msg, "productSecret", "146263a252081f641894b2192df06173");	
	//dds_msg_set_string(msg, "asrRes", "comm");
	struct dds_opt opt;
	opt._handler = dds_ev_ccb;
	opt.userdata = arg;
	dds_start(msg, &opt);
	dds_msg_delete(msg);

	return NULL;
}

void send_request_tts(char *tts_txt) 
{
	if(!tts_txt)
		return -1;
	struct dds_msg *msg = NULL;
	
	msg = dds_msg_new();
	//dds_msg_set_boolean(msg, "logEnable", 0);
	dds_msg_set_type(msg, DDS_EV_IN_CUSTOM_TTS_TEXT);
	dds_msg_set_string(msg, "text", tts_txt);
	dds_msg_set_string(msg, "voiceId", "qianranf");
	dds_send(msg);
	dds_msg_delete(msg);
	msg = NULL;

	while (1) {
		if (is_get_tts_url || is_dui_response)
			break;
		usleep(10000);
	}

}


int ray_tts(char *tts_txt)
{
	if(!tts_txt)
		return -1;
	
	send_request_tts(tts_txt);
	
	return 0;
}

int ray_aitype()
{
	
	return 0;
}

int ray_vr_content_get(char *key)  //回调中区分content类  ????
{
	is_txt_input = 1;
	struct dds_msg *msg = NULL;
	msg = dds_msg_new();
	dds_msg_set_type(msg, DDS_EV_IN_NLU_TEXT);
	dds_msg_set_string(msg, "text", key);
	dds_send(msg);
	dds_msg_delete(msg);
	msg = NULL;

	int timeout_cnt = 0;
	while (!is_dui_response) {
		if (timeout_cnt++ > 50){
			ai_mutex_lock();
			ai_vr_info.from    = VR_FROM_CONTENT;
			ai_vr_info.content.state = CONTENT_FAIL;
			ai_vr_info.content.errId = -1;
			ai_vr_info.content.error = strdup("get content timeout");//b---> callback_from_vr
			if (ai_vr_callback)
				ai_vr_callback(&ai_vr_info);
			ai_mutex_unlock();		
			break;
		}
		
		usleep(100*1000);
	}
	printf("=========%s============timeout_cnt = %d\n",__func__,timeout_cnt);
	is_dui_response = 0;

	
	return 0;
}


void request_start()
{
	struct dds_msg *msg = NULL;
	
	/*告知DDS开始语音*/
	msg = dds_msg_new();
	//dds_msg_set_boolean(msg, "logEnable", 0);
	
	dds_msg_set_type(msg, DDS_EV_IN_SPEECH);
	//dds_msg_set_string(msg, "asrParams", "{\"realBack\":true}");
	dds_msg_set_string(msg, "action", "start");
	dds_send(msg);
	dds_msg_delete(msg);
	msg = NULL;

	duilite_vad_start(vad, NULL);


	return NULL;
}

void request_stop()
{
	printf("------request_stop----------\n\n");

	struct dds_msg *msg = NULL;
	/*告知DDS结束语音*/
	msg = dds_msg_new();
	//dds_msg_set_boolean(msg, "logEnable", 0);
	dds_msg_set_type(msg, DDS_EV_IN_SPEECH);
	dds_msg_set_string(msg, "action", "end");
	dds_send(msg);
	dds_msg_delete(msg);
	msg = NULL;
	
	duilite_vad_stop(vad);
	return NULL;
}

int ray_vad_start(int cnt)
{
	printf("--------------vad_start----------------\n");
	//duilite_library_load(vad_auth_cfg);
	vad = duilite_vad_new(vad_cfg, vad_callback, NULL);
	if(vad){
		purple_pr("duilite_vad_new successfully\n");
		return 0;
	}
	else{
		purple_pr("duilite_vad_new failed\n");
		return -1;
	}

		
}
int ray_vad_stop()
{
	printf("--------------vad_stop------------------\n");

	if(vad)
		duilite_vad_delete(vad);
	//duilite_library_release();
	return 0;
}

void ray_vad_delete()
{
	printf("--------------ray_vad_delete------------------\n");
	duilite_vad_delete(vad);
	//duilite_library_release();
	return NULL;
}


int test()
{
	return 0;
}

void ray_send_request(int time,char *rec_data,int len) 
{
	//printf("------------ray_send_request--------------time = %d\n",time);
	duilite_vad_feed(vad, rec_data, len);   

#if 1
	struct dds_msg *m;
	m = dds_msg_new();

	dds_msg_set_type(m, DDS_EV_IN_AUDIO_STREAM);
	dds_msg_set_bin(m, "audio", rec_data, len);
	
	dds_send(m);
	dds_msg_delete(m);
#endif

	return NULL;

}

int start_dds()
{	
	
	pthread_create(&start_tid, NULL, _run, NULL);
	int cnt = 0;
	while (cnt++ < 50) {
		if (dds_status == DDS_STATUS_IDLE) break;
		usleep(100*1000);
	}
	if(cnt > 49)
		return -1;
	return 0;
}


int stop_dds()
{	
	struct dds_msg *msg = NULL;
	
	msg = dds_msg_new();
	//dds_msg_set_boolean(msg, "logEnable", 0);
	dds_msg_set_type(msg, DDS_EV_IN_EXIT);
	dds_send(msg);
	dds_msg_delete(msg);
	pthread_join(start_tid, NULL);
	return 0;
}


void asr_break(void)
{
	printf("-----------asr_break---------------\n");
	ai_mutex_lock();
	ai_vr_info.from    = VR_FROM_ASR;
	ai_vr_info.asr.state = ASR_BREAK;
	if (ai_vr_callback)
		ai_vr_callback(&ai_vr_info);  //callback_from_vr
	ai_mutex_unlock();
}












