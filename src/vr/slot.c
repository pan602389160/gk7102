#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ai_vr.h"
#include "slot.h"
#include "gk_app.h"
#include "asr.h"

#include "debug_print.h"


const char *domain_type[DOMAIN_MAX]= {
				"null",
				"music",
				"netfm",
				"chat",
				"weather",
				"calendar",
				"reminder",
				"stock",
				"poetry",
				"movie"
				"command"
				};




void slot_free(asr_info_t *asr_info)
{
	free(asr_info->input);
	asr_info->input = NULL;
	asr_info->errId = 0;
	free(asr_info->error);
	asr_info->error = NULL;
	free_sem(&asr_info->sem);
	free_sds(&asr_info->sds);
}


int slot_resolve(json_object *sem_json, asr_info_t *asr_info)
{
	#if 0
	json_object *input = NULL;
	json_object *semantics = NULL;
	json_object *sds = NULL;

	/*  free all */
	slot_free(asr_info);

	if (json_object_object_get_ex(sem_json, "input", &input)){
		if (json_object_get_string(input)){
			asr_info->input = strdup(json_object_get_string(input));
		}
	}

//----------------------------------------------------------------- sds
	if (json_object_object_get_ex(sem_json, "sds", &sds)){
		get_sds(sds, &asr_info->sds);
 	}

	if (json_object_object_get_ex(sem_json, "semantics", &semantics)){
		get_sem(semantics, &asr_info->sem);
	}
	goto exit_slot;
//exit_error:
	slot_free(asr_info);
	return -1;
exit_slot:
		DEBUG("input : %s\n", asr_info->input);
	return 0;
#else
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
	
	json_object *nlu = NULL;
		json_object *input = NULL;
		json_object *semantics = NULL;
			json_object *request = NULL;
				json_object *slots = NULL;
					json_object *slot = NULL;
					json_object *value = NULL;
		json_object *sds = NULL;
	json_object *error = NULL;
		json_object *errId = NULL;
	/*	free all */
	slot_free(asr_info);

	json_object *status = NULL;

	if (json_object_object_get_ex(sem_json, "dm", &dm)){
		//printf("slot_resolve dm\n");
		if (json_object_object_get_ex(dm, "widget", &widget)){
			printf("slot_resolve widget\n");
			if (json_object_object_get_ex(widget, "count", &count)){
				//printf("slot_resolve widget\n");
				asr_info->sds.music.number = json_object_get_int(count);

			}
			if (json_object_object_get_ex(widget, "content", &content)){
				//printf("slot_resolve content\n");
				for(i = 0;i < json_object_get_int(count);i++)
				{
					song = json_object_array_get_idx(content,i);
					if(song){
						if (json_object_object_get_ex(song, "linkUrl", &linkUrl)){
							asr_info->sds.music.data[i].url = strdup(json_object_get_string(linkUrl));
						}
						if (json_object_object_get_ex(song, "title", &title)){
							asr_info->sds.music.data[i].title = strdup(json_object_get_string(title));
						}
						if (json_object_object_get_ex(song, "subTitle", &subTitle)){
							asr_info->sds.music.data[i].artist= strdup(json_object_get_string(subTitle));
						}
					}
				}	
			}
		}
		if (json_object_object_get_ex(dm, "shouldEndSession", &shouldEndSession)){
			//printf("slot_resolve shouldEndSession\n");
			asr_info->sds.is_mult_sds = json_object_get_boolean(shouldEndSession);
		}
	}

	if (json_object_object_get_ex(sem_json, "skillId", &skillId)){  
		if (json_object_get_string(skillId)){
			printf("skillId = %s\n",json_object_get_string(skillId));
			if(!strcmp(json_object_get_string(skillId),"2018112200000023"))
				asr_info->sds.domain = DOMAIN_WEATHER;
			if(!strcmp(json_object_get_string(skillId),"2018040200000004"))
				asr_info->sds.domain = DOMAIN_WEATHER;
			if(!strcmp(json_object_get_string(skillId),"2018040200000012"))
				asr_info->sds.domain = DOMAIN_STORY;
			if(!strcmp(json_object_get_string(skillId),"2018112200000078"))
				asr_info->sds.domain = DOMAIN_STORY;
			if(!strcmp(json_object_get_string(skillId),"2018112200000065"))
				asr_info->sds.domain = DOMAIN_MUSIC;
			if(!strcmp(json_object_get_string(skillId),"2018112200000035"))
				asr_info->sds.domain = DOMAIN_MUSIC;
			if(!strcmp(json_object_get_string(skillId),"2018060500000011")) 
				asr_info->sds.domain = DOMAIN_STORY;
			if(!strcmp(json_object_get_string(skillId),"2018042400000005"))
				asr_info->sds.domain = DOMAIN_STOCK;
			if(!strcmp(json_object_get_string(skillId),"2018042400000004")||!strcmp(json_object_get_string(skillId),"2018112200000025")) //??
				asr_info->sds.domain = DOMAIN_POETRY;
			if(!strcmp(json_object_get_string(skillId),"2018112200000068")) 
				asr_info->sds.domain = DOMAIN_CHAT;
			if(!strcmp(json_object_get_string(skillId),"2018042800000002")) 
				asr_info->sds.domain = DOMAIN_CHAT;
			if(!strcmp(json_object_get_string(skillId),"2018050400000026")) 
				asr_info->sds.domain = DOMAIN_CHAT;
			if(!strcmp(json_object_get_string(skillId),"2018072300000026"))
				asr_info->sds.domain = DOMAIN_COMMAND;
			if(!strcmp(json_object_get_string(skillId),"2018111600000003"))
				asr_info->sds.domain = DOMAIN_COMMAND;
			if(!strcmp(json_object_get_string(skillId),"2018111600000036")) 
				asr_info->sds.domain = DOMAIN_COMMAND;
		}
	}
	if (json_object_object_get_ex(sem_json, "speakUrl", &speakUrl)){ //https to http
		if (json_object_get_string(speakUrl)){
			char https[1024] = {0};
			char http[1024] = {0};
			strcpy(https,json_object_get_string(speakUrl));
			sprintf(http,"http%s",&https[5]);
			asr_info->sds.output = strdup(http);
		}
	}
	if (json_object_object_get_ex(sem_json, "contextId", &contextId)){
		if (json_object_get_string(contextId)){
			asr_info->sds.contextId = strdup(json_object_get_string(contextId));
		}
	}
	
	asr_info->sem.request.command.volume = NULL;
	if (json_object_object_get_ex(sem_json, "nlu", &nlu)){
		if (json_object_object_get_ex(nlu, "input", &input)){
			asr_info->input = strdup(json_object_get_string(input));
		}
		if (json_object_object_get_ex(nlu, "contextId", &contextId)){
			if (json_object_get_string(contextId)){
				asr_info->sds.contextId = strdup(json_object_get_string(contextId));
			}
		}
		if (json_object_object_get_ex(nlu, "semantics", &semantics)){
			//printf("slot_resolve semantics\n");
			if (json_object_object_get_ex(semantics, "request", &request)){
				//printf("slot_resolve request\n");
				if (json_object_object_get_ex(request, "slots", &slots)){
					//printf("slot_resolve slots\n");
					slot = json_object_array_get_idx(slots,0);
					if (json_object_object_get_ex(slot, "value", &value)){
						//printf("value = %s\n",json_object_get_string(value));
						//printf("strcmp1 = %d\n",strcmp(json_object_get_string(value),"下一个"));
						//printf("strcmp2 = %d\n",strcmp(json_object_get_string(value),"上一个"));
						//printf("strcmp3 = %d\n",strcmp(json_object_get_string(value),"+"));
						//printf("strcmp4 = %d\n",strcmp(json_object_get_string(value),"-"));

						if(!strcmp(json_object_get_string(value),"下一个"))
							asr_info->sem.request.command.operation = strdup(json_object_get_string(value));
						if(!strcmp(json_object_get_string(value),"上一个"))
							asr_info->sem.request.command.operation = strdup(json_object_get_string(value));
						if(!strcmp(json_object_get_string(value),"+"))
							asr_info->sem.request.command.volume = strdup(json_object_get_string(value));
						if(!strcmp(json_object_get_string(value),"-"))
							asr_info->sem.request.command.volume = strdup(json_object_get_string(value));
						if(!strcmp(json_object_get_string(value),"min"))
							asr_info->sem.request.command.volume = strdup(json_object_get_string(value));
						if(!strcmp(json_object_get_string(value),"max"))
							asr_info->sem.request.command.volume = strdup(json_object_get_string(value));
					}
				}	
			}
			
		}

	}
	if (json_object_object_get_ex(sem_json, "error", &error)){
		if (json_object_object_get_ex(error, "errId", &errId)){
			asr_info->errId = json_object_get_int(errId);
		}
	}else{
		asr_info->errId = 0;
	}
/*----------------------------------------------------------------- sds
	if (json_object_object_get_ex(sem_json, "sds", &sds)){
		get_sds(sds, &asr_info->sds);
	}
*/
	
	goto exit_slot;
//exit_error:
	slot_free(asr_info);
	return -1;
exit_slot:
		purple_pr("input : %s\n", asr_info->input);
	return 0;

#endif
}
