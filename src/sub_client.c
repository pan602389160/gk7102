/*
Copyright (c) 2009-2014 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License v1.0
and Eclipse Distribution License v1.0 which accompany this distribution.
 
The Eclipse Public License is available at
   http://www.eclipse.org/legal/epl-v10.html
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.
 
Contributors:
   Roger Light - initial implementation and documentation.
*/
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <fcntl.h>
#include <stdbool.h>

#ifndef MAX_PATH
#define MAX_PATH 256
#endif

#ifndef WIN32
#include <unistd.h>
#else
#include <process.h>
#include <winsock2.h>
#define snprintf sprintf_s
#endif

#include <mosquitto.h>
#include "client_shared.h"

#include "sharememory_interface.h"

#include "json-c/json.h"
//#include "utils_interface.h"
#include "linklist_interface.h"
//#include "localplayer_interface.h"

#include "mozart_musicplayer.h"
#include "ray_key_function.h"
#include "debug_print.h"

bool process_messages = true;
int msg_count = 0;
struct mosq_config sub_cfg;
struct mosq_config pub_cfg;
struct mosquitto *mosq_pub = NULL;

#define STATUS_CONNECTING 0
#define STATUS_CONNACK_RECVD 1
#define STATUS_WAITING 2

char *MQTT_host = NULL;
char *MQTT_port = NULL;
char *MQTT_usr = NULL;
char *MQTT_passwd = NULL;
char *MQTT_topic = NULL;


static char *topic = NULL;
static char *message = NULL;
static long msglen = 0;
static int qos = 0;
static int retain = 0;
static int mode = MSGMODE_NONE;
static int status = STATUS_CONNECTING;
static int mid_sent = 0;
static int last_mid = -1;
static int last_mid_sent = -1;
static bool connected = true;
static char *username = NULL;
static char *password = NULL;
static bool disconnect_sent = false;
static bool quiet = false;

#define VERSION "1.4.12"


const char * base64char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char * base64_encode( const unsigned char * bindata, char * base64, int binlength )
{
    int i, j;
    unsigned char current;

    for ( i = 0, j = 0 ; i < binlength ; i += 3 )
    {
        current = (bindata[i] >> 2) ;
        current &= (unsigned char)0x3F;
        base64[j++] = base64char[(int)current];

        current = ( (unsigned char)(bindata[i] << 4 ) ) & ( (unsigned char)0x30 ) ;
        if ( i + 1 >= binlength )
        {
            base64[j++] = base64char[(int)current];
            base64[j++] = '=';
            base64[j++] = '=';
            break;
        }
        current |= ( (unsigned char)(bindata[i+1] >> 4) ) & ( (unsigned char) 0x0F );
        base64[j++] = base64char[(int)current];

        current = ( (unsigned char)(bindata[i+1] << 2) ) & ( (unsigned char)0x3C ) ;
        if ( i + 2 >= binlength )
        {
            base64[j++] = base64char[(int)current];
            base64[j++] = '=';
            break;
        }
        current |= ( (unsigned char)(bindata[i+2] >> 6) ) & ( (unsigned char) 0x03 );
        base64[j++] = base64char[(int)current];

        current = ( (unsigned char)bindata[i+2] ) & ( (unsigned char)0x3F ) ;
        base64[j++] = base64char[(int)current];
    }
    base64[j] = '\0';
    return base64;
}













static int get_object_int(char *object, char *cmd_json)
{
	int ret = -1;
	json_object *cmd_object = NULL;
	json_object *tmp = NULL;

	cmd_object = json_tokener_parse(cmd_json);
	if (cmd_object) {
		if (json_object_object_get_ex(cmd_object, object, &tmp))
			ret = json_object_get_int(tmp);
	}
	json_object_put(cmd_object);
	return ret;
}

static char *get_object_string(char *object, char *content, char *cmd_json)
{
	char *content_object = NULL;
	json_object *cmd_object = NULL;
	json_object *tmp = NULL;

	cmd_object = json_tokener_parse(cmd_json);
	if (cmd_object) {
		if (json_object_object_get_ex(cmd_object, object, &tmp)) {
			if (content)
				json_object_object_get_ex(tmp, content, &tmp);
			content_object = strdup(json_object_get_string(tmp));
		}
	}
	json_object_put(cmd_object);
	return content_object;
}

struct cmd_info_c {
	struct appserver_cli_info *owner;
	char *command;
	char *data;
};

void pub_connect_callback(struct mosquitto *mosq, void *obj, int result)
{
	int rc = MOSQ_ERR_SUCCESS;

	if(!result){
		switch(mode){
			case MSGMODE_CMD:
			case MSGMODE_FILE:
			case MSGMODE_STDIN_FILE:
				rc = mosquitto_publish(mosq, &mid_sent, topic, msglen, message, qos, retain);
				break;
			case MSGMODE_NULL:
				rc = mosquitto_publish(mosq, &mid_sent, topic, 0, NULL, qos, retain);
				break;
			case MSGMODE_STDIN_LINE:
				status = STATUS_CONNACK_RECVD;
				break;
		}
		if(rc){
			if(!quiet){
				switch(rc){
					case MOSQ_ERR_INVAL:
						fprintf(stderr, "Error: Invalid input. Does your topic contain '+' or '#'?\n");
						break;
					case MOSQ_ERR_NOMEM:
						fprintf(stderr, "Error: Out of memory when trying to publish message.\n");
						break;
					case MOSQ_ERR_NO_CONN:
						fprintf(stderr, "Error: Client not connected when trying to publish.\n");
						break;
					case MOSQ_ERR_PROTOCOL:
						fprintf(stderr, "Error: Protocol error when communicating with broker.\n");
						break;
					case MOSQ_ERR_PAYLOAD_SIZE:
						fprintf(stderr, "Error: Message payload is too large.\n");
						break;
				}
			}
			mosquitto_disconnect(mosq);
		}
	}else{
		if(result && !quiet){
			fprintf(stderr, "%s\n", mosquitto_connack_string(result));
		}
	}
}

void pub_disconnect_callback(struct mosquitto *mosq, void *obj, int rc)
{
	connected = false;
}


void my_publish_callback(struct mosquitto *mosq, void *obj, int mid)
{
	last_mid_sent = mid;
	if(mode == MSGMODE_STDIN_LINE){
		if(mid == last_mid){
			mosquitto_disconnect(mosq);
			disconnect_sent = true;
		}
	}else if(disconnect_sent == false){
		mosquitto_disconnect(mosq);
		disconnect_sent = true;
	}
}

char *del_blank(char *ptr)
{
	char *buf = NULL;
	buf = (char *)calloc(strlen(ptr),sizeof(char));
	if(!buf){
		printf("EEROR : calloc \n");
		return NULL;
	}
	char *buf_p = buf;
	

	char *pTmp = ptr;  

	while (*pTmp != '\0')   
	{
		if (*pTmp != ' ')  
		{  
		    *buf_p++ = *pTmp;  
		}  
		pTmp++;
	}
	char *ret = NULL;
	ret = (char *)calloc(strlen(ptr),sizeof(char));
	if(!ret){
		printf("EEROR : calloc \n");
		return NULL;
	}
	strcpy(ret,buf);
	free(buf);
	return ret;
}

void sub_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
	
	struct mosq_config *cfg;
	int i;
	bool res;

	char *tmp = NULL;
	char *command = NULL;
	char *timestamp = NULL;
	if(process_messages == false) return;

	assert(obj);
	cfg = (struct mosq_config *)obj;
	if(message->retain && cfg->no_retain) return;
	if(cfg->filter_outs){
		for(i=0; i<cfg->filter_out_count; i++){
			mosquitto_topic_matches_sub(cfg->filter_outs[i], message->topic, &res);
			if(res) return;
		}
	}
	if(cfg->verbose){
		if(message->payloadlen){
			printf("%s ", message->topic);
			printf("receive : ");
			fwrite(message->payload, 1, message->payloadlen, stdout);
			if(cfg->eol){
				printf("\n");
			}
		}else{
			if(cfg->eol){
				printf("%s (null)\n", message->topic);
			}
		}
		fflush(stdout);
	}else{
		purple_pr("receive : ");
		if(message->payloadlen){
			fwrite(message->payload, 1, message->payloadlen, stdout);
			if(cfg->eol){
				printf("\n");
			}
			fflush(stdout);
		}
	}

	if(cfg->msg_count>0){
		msg_count++;
		if(cfg->msg_count == msg_count){
			process_messages = false;
			mosquitto_disconnect(mosq);
		}
	}

	timestamp = get_object_string("timestamp",NULL,(char *)message->payload);
	command = get_object_string("command",NULL,(char *)message->payload);
	if(command == NULL)
		return NULL;
	
	//purple_pr("============>my_message_callback get the command : [%s] \ntimestamp = [%s]\n",command,timestamp);

	int send_flag = 0;
	struct tm  *tp;
	char cur_time[16] = {0};
 	time_t timep ;
 	timep = time(NULL);

	//printf("timep = %ld\n",timep);
	
	sprintf(cur_time,"%ld",timep);
	//printf("cur_time = %s\n",cur_time);

	tp = localtime(&timep);
	//printf("=============>%d/%d/%d\n",tp->tm_mon+1,tp->tm_mday,tp->tm_year+1900);  
	//printf("=============>%d:%d:%d\n",tp->tm_hour,tp->tm_min,tp->tm_sec);


	char *reply_json = NULL;
	char *reply_json_x = NULL;
	json_object *reply_object = NULL;
	char *song = NULL;
	int index = 0;
	enum play_mode mode;
//	struct appserver_cli_info *sub_owner = NULL;
//	sub_owner = (struct cmd_info_c *)(message->payload);
	if(strcmp(command,"ping") == 0){ 
		reply_object = json_object_new_object();
		json_object_object_add(reply_object, "command", json_object_new_string(command));
		json_object_object_add(reply_object, "timestamp", json_object_new_string(timestamp));
		reply_json = strdup(json_object_get_string(reply_object));
		send_flag = 1;
	}else if(strcmp(command,"prefix") == 0){ 
		mozart_previous_song();
		reply_json = del_blank(ingenicplayer_get_song_info(command,timestamp));
		send_flag = 1;
	}else if(strcmp(command,"skip") == 0){	
		mozart_next_song();
		reply_json = del_blank(ingenicplayer_get_song_info(command,timestamp));
		send_flag = 1;
	}else if(strcmp(command,"toggle") == 0){
		mozart_play_pause();
		reply_json = del_blank(ingenicplayer_get_song_info(command,timestamp));
		send_flag = 1;
	}else if(strcmp(command,"get_volume") == 0){
		reply_json = del_blank(ingenicplayer_get_volume(command,timestamp));
		send_flag = 1;
	}
	else if(strcmp(command,"set_volume") == 0){
		int volume = -1;
		volume = get_object_int("volume", (char *)message->payload);
		//mozart_musicplayer_set_volume(mozart_musicplayer_handler,value);
		mozart_volume_set(volume);
		reply_object = json_object_new_object();
		if(command)
			json_object_object_add(reply_object, "command", json_object_new_string(command));
		
		json_object_object_add(reply_object, "volume", json_object_new_int(volume));
		
		json_object_object_add(reply_object, "timestamp", json_object_new_string(timestamp));
		reply_json = del_blank(strdup(json_object_get_string(reply_object)));
		send_flag = 1;
	}
	else if(strcmp(command,"increase_volume") == 0){
		mozart_volume_up();
		send_flag = 1;
	}else if(strcmp(command,"decrease_volume") == 0){
		mozart_volume_down();
		send_flag = 1;
	}else if(strcmp(command,"get_device_info") == 0){	
		//reply_json = ingenicplayer_get_device_info();
		//reply_json = del_blank(ingenicplayer_get_device_info());
		send_flag = 1;
	}else if(strcmp(command,"upgrade_ota") == 0){//--------------------------

	}else if(strcmp(command,"get_status") == 0){ //---------------------
		reply_object = json_object_new_object();
		int stat = 1;
		json_object_object_add(reply_object, "status", json_object_new_int(stat));
		json_object_object_add(reply_object, "timestamp", json_object_new_string(timestamp));
		reply_json = del_blank(strdup(json_object_get_string(reply_object)));
		send_flag = 1;
	}else if(strcmp(command,"get_sd_music") == 0){//-------------------
		//reply_json = ingenicplayer_get_sd_music();
		//reply_json = del_blank(ingenicplayer_get_sd_music());
		send_flag = 1;
	}else if(strcmp(command,"get_song_info") == 0){
		reply_json = del_blank(ingenicplayer_get_song_info(command,timestamp));
		tmp = h5_get_queue(timestamp);
		reply_json_x = del_blank(tmp);
		send_flag = 2;
	}else if(strcmp(command,"get_position") == 0){
		reply_json = del_blank(ingenicplayer_get_position(command,timestamp));
		send_flag = 1;
	}else if(strcmp(command,"get_song_queue") == 0){
		tmp = h5_get_queue(timestamp);
		reply_json = del_blank(tmp);
		send_flag = 1;

	}else if(strcmp(command,"add_queue_to_tail") == 0){//--------------------
		ingenicplayer_add_queue((char *)message->payload);
		
		char *tmp = h5_get_queue();
		reply_json = del_blank(tmp);
		send_flag = 1;
	}else if(strcmp(command,"delete_from_queue") == 0){//------------------------
		h5_musiclist_delete_index((char *)message->payload);
		char *tmp = h5_get_queue();
		reply_json = del_blank(tmp);
		send_flag = 1;
	}
	else if(strcmp(command,"add_queue_to_play") == 0){
		char *tmp  = ingenicplayer_replace_queue((char *)message->payload, NULL);
		reply_json = del_blank(tmp);
		send_flag = 1;
	}else if(strcmp(command,"play_from_queue") == 0){
		char *tmp  = ingenicplayer_play_queue((char *)message->payload);
		purple_pr("tmp = %s\n",tmp);
		reply_json = del_blank(tmp);

		send_flag = 1;
	}else if(strcmp(command,"set_queue_mode") == 0){
		//ingenicplayer_play_mode((char *)message->payload);
		#if 0
		int value = h5_play_mode((char *)message->payload);
		reply_object = json_object_new_object();
		if(command)
			json_object_object_add(reply_object, "command", json_object_new_string(command));
		
		json_object_object_add(reply_object, "value", json_object_new_int(value));
		if(cur_time)	
			json_object_object_add(reply_object, "timestamp", json_object_new_string(cur_time));
		
		reply_json = del_blank(strdup(json_object_get_string(reply_object)));
		printf("reply_json = %s\n",reply_json);
		send_flag = 1;
		#endif
	}else if(strcmp(command,"set_seek") == 0){//------------------------
		//ingenicplayer_set_seek((char *)message->payload);
		int value = -1;
		value = get_object_int("value", (char *)message->payload);
		ingenicplayer_set_seek(value);
			
		reply_object = json_object_new_object();
		if(command)
			json_object_object_add(reply_object, "command", json_object_new_string(command));
		
		json_object_object_add(reply_object, "value", json_object_new_int(value));
		
		json_object_object_add(reply_object, "timestamp", json_object_new_string(timestamp));
		reply_json = del_blank(strdup(json_object_get_string(reply_object)));
		send_flag = 1;
	}




	if(send_flag){
		char *cmd = NULL;
		//printf("len = %d,reply_json = %s\n",strlen(reply_json),reply_json);
		
		cmd = (char *)calloc(strlen(reply_json) + 128,sizeof(char));
		if(!cmd){
			printf("EEROR :cmd calloc \n");
			
		}
		sprintf(cmd,"mosquitto_pub -h 119.23.207.229 -p 1883 -u leden -P ray123456 -t /app/%s -m '%s'","RSPythonTest",reply_json);
		//printf("SYSCMD = %s\n",cmd);
		system(cmd);
		free(cmd);
		cmd = NULL;
		//free(send_msg);
		if(send_flag == 2){
			cmd = (char *)calloc(strlen(reply_json_x) + 128,sizeof(char));
			if(!cmd){
					printf("EEROR :cmd calloc \n");		
			}
			sprintf(cmd,"mosquitto_pub -h 119.23.207.229 -p 1883 -u leden -P ray123456 -t /app/%s -m '%s'","RSPythonTest",reply_json_x);
			purple_pr("cmd = %s\n",cmd);
			system(cmd);
			free(cmd);
			cmd = NULL;
		}

	}
	if(reply_json)
		free(reply_json);
	if(reply_json_x)
		free(reply_json_x);

}

void sub_connect_callback(struct mosquitto *mosq, void *obj, int result)
{
	int i;
	struct mosq_config *cfg;

	assert(obj);
	cfg = (struct mosq_config *)obj;

	if(!result){
		for(i=0; i<cfg->topic_count; i++){
			mosquitto_subscribe(mosq, NULL, cfg->topics[i], cfg->qos);
		}
	}else{
		if(result && !cfg->quiet){
			fprintf(stderr, "%s\n", mosquitto_connack_string(result));
		}
	}
}

void sub_subscribe_callback(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos)
{
	int i;
	struct mosq_config *cfg;

	assert(obj);
	cfg = (struct mosq_config *)obj;

	if(!cfg->quiet) printf("Subscribed (mid: %d): %d", mid, granted_qos[0]);
	for(i=1; i<qos_count; i++){
		if(!cfg->quiet) printf(", %d", granted_qos[i]);
	}
	if(!cfg->quiet) printf("\n");
}

void my_log_callback(struct mosquitto *mosq, void *obj, int level, const char *str)
{
	printf("%s\n", str);
}
int start_mqtt(char *host,char *port,char *user,char *passwd,char *topic1,char *topic2)
{	printf("======start_mqtt===== topic = %s,topic2 = %s\n",topic1,topic2);

	char *MQTT_host = host;
	char *MQTT_port = port;
	char *MQTT_usr = user;
	char *MQTT_passwd = passwd;
	char *MQTT_topic = topic1;	

	struct mosquitto *mosq_sub = NULL;
	
	int rc;
	
	if(topic1)
		rc = client_config_load(&sub_cfg, CLIENT_SUB,host,port,user,passwd,topic1,topic2,NULL);
	if(rc){
		client_config_cleanup(&sub_cfg);
		if(rc == 2){
			printf("mqtt arg error\n");
		}else{
			fprintf(stderr, "\nUse 'mosquitto_sub --help' to see usage.\n");
		}
		return 1;
	}
	purple_pr(">>>>>sub_client_config_load successfully\n");
	mosquitto_lib_init();

	if(client_id_generate(&sub_cfg, "mosqsub")){
		return 1;
	}

	mosq_sub = mosquitto_new(sub_cfg.id, sub_cfg.clean_session, &sub_cfg);
	if(!mosq_sub){
		switch(errno){
			case ENOMEM:
				if(!sub_cfg.quiet) fprintf(stderr, "Error: Out of memory.\n");
				break;
			case EINVAL:
				if(!sub_cfg.quiet) fprintf(stderr, "Error: Invalid id and/or clean_session.\n");
				break;
		}
		mosquitto_lib_cleanup();
		return 1;
	}
	purple_pr(">>>>>sub mosquitto_new successfully\n");
	if(client_opts_set(mosq_sub, &sub_cfg)){
		return 1;
	}

	char *reply_json = NULL;
	json_object *reply_object = NULL;
	char *cmd = NULL;

	reply_object = json_object_new_object();
	json_object_object_add(reply_object, "status", json_object_new_int(1));
	json_object_object_add(reply_object, "timestamp", json_object_new_string("-1"));
	reply_json = del_blank(strdup(json_object_get_string(reply_object)));

	printf("??? reply_json = %s\n",reply_json);
	cmd = (char *)calloc(strlen(reply_json) + 128,sizeof(char));
	if(!cmd){
		printf("EEROR :cmd calloc \n");	
	}else{
		sprintf(cmd,"mosquitto_pub -h 119.23.207.229 -p 1883 -u leden -P ray123456 -t /app/%s -m '%s'","RSPythonTest",reply_json);
		system(cmd);
		free(cmd);
	}
	free(reply_json);
	reply_json = NULL;

	
	mosquitto_connect_callback_set(mosq_sub, sub_connect_callback);
	mosquitto_message_callback_set(mosq_sub, sub_message_callback);

	rc = client_connect(mosq_sub, &sub_cfg);

	if(rc) return rc;

	purple_pr(">>>>>sub mosquitto client_connect successfully\n");

	rc = mosquitto_loop_forever(mosq_sub, -1, 1);

	mosquitto_destroy(mosq_sub);
	mosquitto_lib_cleanup();

	if(sub_cfg.msg_count>0 && rc == MOSQ_ERR_NO_CONN){
		rc = 0;
	}
	if(rc){
		fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
	}
	return rc;	
	
}
