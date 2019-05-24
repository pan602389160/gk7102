#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ai_vr.h"
#include "slot.h"
#include "content.h"

#include "dds.h"
#include "general.h"
#include "asr.h"
#include "ray_app.h"
#include "gk_app.h"
#include "duilite.h"
#include "ai_vr.h"
#include "asoundlib.h"


static const char *_content_param =
"\
{\
    \"coreProvideType\": \"cloud\",\
    \"vadEnable\": 0,\
    \"app\": {\
        \"userId\": \"wifiBox\"\
    },\
    \"audio\": {\
        \"audioType\": \"wav\",\
        \"sampleBytes\": 2,\
        \"sampleRate\": 16000,\
        \"channel\": 1,\
		\"compress\": \"speex\"\
    },\
    \"request\": {\
        \"coreType\": \"cn.sds\",\
        \"res\": \"aihome\",\
 		\"sdsExpand\": {\
        	\"prevdomain\": \"\",\
			\"lastServiceType\": \"cloud\"\
		}\
    }\
}";

static bool is_end_flag = false;
static bool is_work_flag = false;


static enum _status content_status;



void content_free(content_info_t *content_info){
	content_info->errId = 0;
	free(content_info->error);
	content_info->error = NULL;
	free_sds(&content_info->sds);
	ai_vr_info.content.state = CONTENT_IDLE;
}




int _content_resolve(json_object *sem_json, content_info_t *content_info)
{
	#if 0
	json_object *sds = NULL;
	/*  free all */

	if (json_object_object_get_ex(sem_json, "sds", &sds)){
		get_sds(sds, &content_info->sds);
		return 0;
 	}
	content_free(content_info);
	return -1;
	#endif
	
	int i = 0;
	json_object *dm = NULL;
		json_object *shouldEndSession = NULL;
		json_object *widget = NULL;
		json_object *count = NULL;
		json_object *content = NULL;
			json_object *song = NULL;
				json_object *linkUrl = NULL;
				json_object *title = NULL;
				json_object *subTitle = NULL;
	json_object *skillId = NULL;
	json_object *recordId = NULL;
	json_object *contextId = NULL;
	json_object *sessionId = NULL;
	json_object *speakUrl = NULL;

	if (json_object_object_get_ex(sem_json, "dm", &dm)){
		printf("_content_resolve dm\n");
		if (json_object_object_get_ex(dm, "widget", &widget)){
			printf("_content_resolve widget\n");
			if (json_object_object_get_ex(widget, "count", &count)){
				printf("_content_resolve widget\n");
				content_info->sds.music.number = json_object_get_int(count);

			}
			if (json_object_object_get_ex(widget, "content", &content)){
				printf("_content_resolve content\n");
				for(i = 0;i < json_object_get_int(count);i++)
				{
					song = json_object_array_get_idx(content,i);
					if(song){
						if (json_object_object_get_ex(song, "linkUrl", &linkUrl)){
							content_info->sds.music.data[i].url = strdup(json_object_get_string(linkUrl));
						}
						if (json_object_object_get_ex(song, "title", &title)){
							content_info->sds.music.data[i].title = strdup(json_object_get_string(title));
						}
						if (json_object_object_get_ex(song, "subTitle", &subTitle)){
							content_info->sds.music.data[i].artist= strdup(json_object_get_string(subTitle));
						}
					}
				}	
			}
		}
		if (json_object_object_get_ex(dm, "shouldEndSession", &shouldEndSession)){//是否是多轮对话
			content_info->sds.is_mult_sds = json_object_get_boolean(shouldEndSession);
		}
	}

	if (json_object_object_get_ex(sem_json, "skillId", &skillId)){	 //获取对话意图
		if (json_object_get_string(skillId)){
			if(!strcmp(json_object_get_string(skillId),"2018040200000004"))
				content_info->sds.domain = DOMAIN_WEATHER;
			if(!strcmp(json_object_get_string(skillId),"2018040200000012"))//新版故事，与音乐分一起
				content_info->sds.domain = DOMAIN_STORY;
			if(!strcmp(json_object_get_string(skillId),"2018112200000078"))//成语故事，与音乐分一起
				content_info->sds.domain = DOMAIN_STORY;
			if(!strcmp(json_object_get_string(skillId),"2018112200000065"))
				content_info->sds.domain = DOMAIN_MUSIC;
			if(!strcmp(json_object_get_string(skillId),"2018112200000035"))
				content_info->sds.domain = DOMAIN_MUSIC;
			if(!strcmp(json_object_get_string(skillId),"2018060500000011"))  //国学，跟故事分一起
				content_info->sds.domain = DOMAIN_STORY;
			if(!strcmp(json_object_get_string(skillId),"2018042400000005"))
				content_info->sds.domain = DOMAIN_STOCK;
			if(!strcmp(json_object_get_string(skillId),"2018042400000004")) //诗词
				content_info->sds.domain = DOMAIN_POETRY;
			if(!strcmp(json_object_get_string(skillId),"2018112200000068")) //成语字典，与chat分一起
				content_info->sds.domain = DOMAIN_CHAT;
			if(!strcmp(json_object_get_string(skillId),"2018042800000002")) //新版计算器，与chat分一起 
				content_info->sds.domain = DOMAIN_CHAT;
			if(!strcmp(json_object_get_string(skillId),"2018050400000026")) 
				content_info->sds.domain = DOMAIN_CHAT;
		}
	}
	if (json_object_object_get_ex(sem_json, "speakUrl", &speakUrl)){ //https to http
		if (json_object_get_string(speakUrl)){
			char https[1024] = {0};
			char http[1024] = {0};
			strcpy(https,json_object_get_string(speakUrl));
			sprintf(http,"http%s",&https[5]);
			content_info->sds.output = strdup(http);
		}
	}
	//content_free(content_info);
}


int _content_callback(void *usrdata, const char *id, int type, const void *message, int size)
{
	#if 0
//    printf("resp data: %.*s\n", size, (char *) message);
	json_object *out = NULL;
    json_object *result = NULL;
	json_object *errId = NULL;
	json_object *error = NULL;

    out = json_tokener_parse((char*) message);
    if (!out)
    {
        return -1;
    }

	if (json_object_object_get_ex(out, "result", &result)){
		content_free(&ai_vr_info.content);

		if(_content_resolve(result, &ai_vr_info.content) == -1){
			ai_vr_info.content.state = CONTENT_FAIL;
		} else {
			ai_vr_info.content.state = CONTENT_SUCCESS;
		}
		is_end_flag = true;
		ai_mutex_lock();
		ai_vr_info.from    = VR_FROM_CONTENT;
		ai_vr_info.content.state = CONTENT_SUCCESS;
		if (ai_vr_callback)
			ai_vr_callback(&ai_vr_info);
		ai_mutex_unlock();
    }

	if (json_object_object_get_ex(out, "error", &error)){
		content_free(&ai_vr_info.content);
		if (json_object_get_string(error)){
			ai_vr_info.content.error = strdup(json_object_get_string(error));
		}

		if (json_object_object_get_ex(out, "errId", &errId)){
			ai_vr_info.content.errId = json_object_get_int(errId);
		}

		is_end_flag = true;
		ai_mutex_lock();
		ai_vr_info.from    = VR_FROM_CONTENT;
		ai_vr_info.content.state = CONTENT_FAIL;
		if (ai_vr_callback)
			ai_vr_callback(&ai_vr_info);
		ai_mutex_unlock();
	}

//exit_error:
	if(out){
		json_object_put(out);
	}
    return 0;
	#endif
}

void _content_stop(void)
{
	#if 0
	if (is_work_flag){
		if (sds_agn){
			aiengine_cancel(sds_agn);
		}
		is_work_flag = false;
	}
	#endif
}

void *content_wait_func(void *arg)
{
	#if 0
	int timeout = CONTENT_TIMEOUT * 1000;
	int timeused = 0;

	pthread_detach(pthread_self());

    while (is_end_flag == false) {
        usleep(1000);
		timeused++;
		if (timeused > timeout) {
			PERROR("ERROR: get content timeout!\n");
			_content_stop();
			ai_mutex_lock();
			ai_vr_info.from    = VR_FROM_CONTENT;
			ai_vr_info.content.state = CONTENT_FAIL;
			ai_vr_info.content.errId = -1;
			ai_vr_info.content.error = strdup("get content timeout");
			if (ai_vr_callback)
				ai_vr_callback(&ai_vr_info);
			ai_mutex_unlock();
			return NULL;
		}
    }

    /* make compile happy. */
    return NULL;
	#endif
}

int content_get(char *text)
{
	#if 0
	char uuid[64] = {0};
	const void *usrdata  ;//= NULL;
	char *_param = NULL;
	pthread_t content_wait_pthread;

	json_object *request = NULL;
	json_object *param = NULL;

	if (ai_vr_callback == NULL) {
		printf("ai_vr_callback is null\n");
		goto exit_error;
	}

	ai_vr_info.content.state = CONTENT_START;
	if (text == NULL){
		printf("text   = null !\n");
		goto exit_error;
	}

	param = json_tokener_parse(_content_param);
	if(!param) {
		printf("get param_js faild !\n");
		goto exit_error;
	}

	is_end_flag = false;
	is_work_flag = true;

	if (json_object_object_get_ex(param, "request", &request)){
		json_object_object_add(request, "refText", json_object_new_string(text));
		_param = (char *)json_object_to_json_string(param);
	}
	if (_param == NULL){
		PERROR("Error: get param faild !\n");
		goto exit_error;
	}

	aiengine_start(sds_agn,_param,
			uuid, _content_callback, &usrdata);
	aiengine_stop(sds_agn);

	if (pthread_create(&content_wait_pthread, NULL, content_wait_func, NULL)) {
		PERROR("create content_wait_pthread error: %s.\n", strerror(errno));
		return -1;
	}

	if(param){
		json_object_put(param);
	}
	return 0;

exit_error:
	if(param){
		json_object_put(param);
	}
	return -1;
	#endif
}

void content_stop(void){
	return _content_stop();
}

void content_interrupt(void){
	#if 0
	ai_mutex_lock();
	_content_stop();
	ai_vr_info.from    = VR_FROM_CONTENT;
	ai_vr_info.content.state = CONTENT_INTERRUPT;
	if (ai_vr_callback)
		ai_vr_callback(&ai_vr_info);
	ai_mutex_unlock();
	#endif
}
#if 0
static int is_get_asr_result = 0;
static int is_get_tts_url = 0;
static int is_dui_response = 0;

static int content_dds_ev_ccb(void *userdata, struct dds_msg *msg) 
{
	int type;
	if (!dds_msg_get_type(msg, &type)) {
		switch (type) {
		case DDS_EV_OUT_STATUS: {
			char *value;
			if (!dds_msg_get_string(msg, "status", &value)) {
				if (!strcmp(value, "idle")) {
					content_status = DDS_STATUS_IDLE;
				} else if (!strcmp(value, "listening")) {
					content_status = DDS_STATUS_LISTENING;
				} else if (!strcmp(value, "understanding")) {
					content_status = DDS_STATUS_UNDERSTANDING;
				}
			}
			break;
		}
		case DDS_EV_OUT_CINFO_RESULT: {
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
			printf("\n>>>>>>>>>>>>>>>>>>>>>>>DDS_EV_OUT_DUI_RESPONSE\n");
			char *value;
			if (!dds_msg_get_string(msg, "var", &value)) {
				printf("var: %s\n", value);
			}
			if (!dds_msg_get_string(msg, "text", &value)) {
				printf("text: %s\n", value);
				is_get_asr_result = 1;
			}
			break;
		}
		case DDS_EV_OUT_TTS: {
			char *value;
			if (!dds_msg_get_string(msg, "speakUrl", &value)) {
				printf("speakUrl: %s\n", value);
				is_get_tts_url = 1;
			}
			break;
		}
		case DDS_EV_OUT_DUI_RESPONSE: {
			printf("\n>>>>>>>>>>>>>>>>>>>>>>>DDS_EV_OUT_DUI_RESPONSE\n");
            char *resp = NULL;
            if(!dds_msg_get_string(msg, "response", &resp)) {
            	printf("dui response: %s\n", resp);
            }

			content_free(&ai_vr_info.content);
			if(_content_resolve(json_tokener_parse(resp), &ai_vr_info.content) == -1){
				ai_vr_info.content.state = CONTENT_FAIL;
			} else {
				ai_vr_info.content.state = CONTENT_SUCCESS;
			}
			//is_end_flag = true;  //判断超时用				
			ai_vr_info.from    = VR_FROM_CONTENT;
			ai_vr_info.content.state = CONTENT_SUCCESS;
				
			ai_mutex_lock();		
			if (ai_vr_callback)
				ai_vr_callback(&ai_vr_info);  //b---> callback_from_vr
			ai_mutex_unlock();

            is_dui_response = 1;
            break;
        }
		case DDS_EV_OUT_ERROR: {
			printf("\n>>>>>>>>>>>>>>>>>>>>>>>DDS_EV_OUT_ERROR\n");
			char *value;
			if (!dds_msg_get_string(msg, "error", &value)) {
				printf("DDS_EV_OUT_ERROR: %s\n", value);
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


static void *content_run(void *arg) 
{
	printf("=========%s============\n",__func__);
	struct dds_msg *msg = dds_msg_new();
	dds_msg_set_string(msg, "productId", "278576719"); //productId
	dds_msg_set_string(msg, "aliasKey", "test");
	dds_msg_set_string(msg, "savedProfile", "/usr/data/provision");


	struct dds_opt opt;
	opt._handler = content_dds_ev_ccb;
	opt.userdata = arg;
	dds_start(msg, &opt);
	dds_msg_delete(msg);

	return NULL;
}


static void send_content_request(char *key) 
{
	printf("=========%s============\n",__func__);
	struct dds_msg *msg = NULL;
	int timeout_cnt = 0; 
	is_end_flag = false;
	msg = dds_msg_new();
	dds_msg_set_type(msg, DDS_EV_IN_NLU_TEXT);
	dds_msg_set_string(msg, "text", key);
	dds_send(msg);
	dds_msg_delete(msg);
	msg = NULL;




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

}


int ray_vr_content_get(char *key)  
{
	printf("=========%s============\n",__func__);
	struct dds_msg *msg = NULL;
	pthread_t tid;
	pthread_create(&tid, NULL, content_run, NULL);

	while (1) {
		if (content_status == DDS_STATUS_IDLE) break;
		usleep(10000);
	}
	
	
	send_content_request(key);


	
	return 0;
}
#endif
