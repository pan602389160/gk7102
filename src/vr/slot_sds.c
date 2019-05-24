#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ai_vr.h"
#include "slot_sds.h"
#include "gk_app.h"
const char *slot_state[]= {
				"null",
				"do",
				"query",
				"offernone",
				"offer",
				"interact",
				"exit"};

extern const char *domain_type;

void _free_sds_music(sds_music_t *music){
	int i =0;
	for (i=0; i<music->number; i++){
		free(music->data[i].url);
		music->data[i].url= NULL;
		free(music->data[i].artist);
		music->data[i].artist = NULL;
		free(music->data[i].title);
		music->data[i].title= NULL;
	}
	music->number = 0;
}


void _free_sds_netfm(sds_netfm_t *netfm){
	int i =0;
	for (i=0; i<netfm->number; i++){
		free(netfm->data[i].url);
		netfm->data[i].url= NULL;
		free(netfm->data[i].track);
		netfm->data[i].track = NULL;
	}
	netfm->number = 0;
}


void _free_sds_weather(sds_weather_t *weather){
	free(weather->meteorology);
	weather->meteorology = NULL;
	free(weather->temperature);
	weather->temperature = NULL;
	free(weather->wind);
	weather->wind = NULL;
	free(weather->area);
	weather->area = NULL;

}

void _free_sds_reminder(sds_reminder_t *reminder){
	free(reminder->date);
	reminder->date = NULL;
	free(reminder->time);
	reminder->time = NULL;
	free(reminder->event);
	reminder->event = NULL;
}

void _free_sds_chat(sds_chat_t *chat){
	free(chat->url);
	chat->url = NULL;
}


void free_sds(sds_info_t *sds){
	if (sds->env){
		free(sds->env);
		sds->env = NULL;
	}
	if (sds->contextId){
		free(sds->contextId);
		sds->contextId = NULL;
	}
	if (sds->output){
		free(sds->output);
		sds->output = NULL;
	}

	switch (sds->domain){
		case DOMAIN_MUSIC:
			_free_sds_music(&sds->music);
			break;
		case DOMAIN_NETFM:
			_free_sds_netfm(&sds->netfm);
			break;
		case DOMAIN_WEATHER:
			_free_sds_weather(&sds->weather);
			break;
		case DOMAIN_REMINDER:
			_free_sds_reminder(&sds->reminder);
			break;
		case DOMAIN_CHAT:
			_free_sds_chat(&sds->chat);
			break;
		default:
			break;
	}

	sds->domain = DOMAIN_NULL;
	sds->is_mult_sds = false;
	sds->state = SDS_STATE_NULL;
}


void _get_sds_domain(json_object *sds_j, sds_info_t *sds){
	json_object *domain = NULL;
	//----------------------------------------------------------------- domain , env
	if (json_object_object_get_ex(sds_j, "domain", &domain)){
		if (json_object_get_string(domain)){
			free(sds->env);
			sds->env = NULL;
			if(strcmp(json_object_get_string(domain), "music") == 0){
				sds->domain = DOMAIN_MUSIC;
				sds->env = strdup("dlg_domain=\"音乐\";");
			} else if(strcmp(json_object_get_string(domain), "netfm") == 0){
				sds->domain = DOMAIN_NETFM;
				sds->env = strdup("dlg_domain=\"电台\";");
			} else if(strcmp(json_object_get_string(domain), "chat") == 0){
				sds->domain = DOMAIN_CHAT;
				sds->env = strdup("dlg_domain=\"聊天\";");
			} else if(strcmp(json_object_get_string(domain), "weather") == 0){
				sds->domain = DOMAIN_WEATHER;
				sds->env = strdup("dlg_domain=\"天气\";");
			} else if(strcmp(json_object_get_string(domain), "calendar") == 0){
				sds->domain = DOMAIN_CALENDAR;
				sds->env = strdup("dlg_domain=\"日历\";");
			} else if(strcmp(json_object_get_string(domain), "reminder") == 0){
				sds->domain = DOMAIN_REMINDER;
				sds->env = strdup("dlg_domain=\"提醒\";");
			} else if(strcmp(json_object_get_string(domain), "stock") == 0){
				sds->domain = DOMAIN_STOCK;
				sds->env = strdup("dlg_domain=\"股票\";");
			} else if(strcmp(json_object_get_string(domain), "poetry") == 0){
				sds->domain = DOMAIN_POETRY;
				sds->env = strdup("dlg_domain=\"古诗\";");
			} else if(strcmp(json_object_get_string(domain), "command") == 0){
				sds->domain = DOMAIN_COMMAND;
				sds->env = strdup("dlg_domain=\"中控\";");
			}
		}
	}
}

void _get_sds_state(json_object *sds_j, sds_info_t *sds){
	json_object *state = NULL;
	//----------------------------------------------------------------- domain , env
	if (json_object_object_get_ex(sds_j, "state", &state)){
		if (json_object_get_string(state)){
			if(strcmp(json_object_get_string(state), "do") == 0){
				sds->state = SDS_STATE_DO;
			} else if(strcmp(json_object_get_string(state), "query") == 0){
				sds->state = SDS_STATE_QUERY;
			} else if(strcmp(json_object_get_string(state), "interact") == 0){
				sds->state = SDS_STATE_INTERACT;
			} else if(strcmp(json_object_get_string(state), "offer") == 0){
				sds->state = SDS_STATE_OFFER;
			} else if(strcmp(json_object_get_string(state), "offernone") == 0){
				sds->state = SDS_STATE_OFFERNONE;
			} else if(strcmp(json_object_get_string(state), "exit") == 0){
				sds->state = SDS_STATE_EXIT;
			} else {
				sds->state = SDS_STATE_NULL;
			}
		}
	}
}

void _get_sds_music(json_object *dbdata, sds_music_t *music){
	int i = 0;
	int number = 0;
	json_object *dbdata_i = NULL;
	json_object *url = NULL;
	json_object *artist = NULL;
	json_object *title = NULL;

	if((dbdata == NULL)||(music == NULL))
		return;


	number = json_object_array_length(dbdata);
	if (number > MUSIC_MAX){
		number = MUSIC_MAX;
	}

	DEBUG("music num = %d\n", number);
	music->number = number;

	for (i = 0; i < number; i++){
		dbdata_i = json_object_array_get_idx(dbdata, i);
		if(dbdata_i){
			if (json_object_object_get_ex(dbdata_i, "url", &url)){
				if (json_object_get_string(url)){
					music->data[i].url = strdup(json_object_get_string(url));
				}
				if (json_object_object_get_ex(dbdata_i, "artist", &artist)){
					if (json_object_get_string(artist)){
						music->data[i].artist = strdup(json_object_get_string(artist));
					}
				}
				if (json_object_object_get_ex(dbdata_i, "title", &title)){
					if (json_object_get_string(title)){
						music->data[i].title = strdup(json_object_get_string(title));
					}
				}
			}
#ifdef DEBUG_SHOW_ALL
			DEBUG("[%d].url = %s\n", i, music->data[i].url);
			DEBUG("[%d].artist = %s\n", i, music->data[i].artist);
			DEBUG("[%d].title = %s\n", i, music->data[i].title);
#endif
		}
	}
}

void _get_sds_netfm(json_object *dbdata, sds_netfm_t *netfm){
	int i = 0;
	int number = 0;
	json_object *dbdata_i = NULL;
	json_object *url = NULL;
	json_object *track = NULL;

	if((dbdata == NULL)||(netfm == NULL))
		return;

	number = json_object_array_length(dbdata);
	if (number > NETFM_MAX){
		number = NETFM_MAX;
	}

	DEBUG("netfm num = %d\n", number);
	netfm->number = number;

	for (i = 0; i < number; i++){
		dbdata_i = json_object_array_get_idx(dbdata, i);
		if(dbdata_i){
			if (json_object_object_get_ex(dbdata_i, "playUrl32", &url)){
				if (json_object_get_string(url)){
					netfm->data[i].url = strdup(json_object_get_string(url));
				}
				if (json_object_object_get_ex(dbdata_i, "track", &track)){
					if (json_object_get_string(track)){
						netfm->data[i].track = strdup(json_object_get_string(track));
					}
				}
			}
			DEBUG("[%d].url = %s\n", i, netfm->data[i].url);
			DEBUG("[%d].track = %s\n", i, netfm->data[i].track);
		}

	}
}

void _get_sds_weather(json_object *dbdata, sds_weather_t *weather){
	json_object *dbdata_1 = NULL;
	json_object *area = NULL;
	json_object *today = NULL;
	json_object *meteorology = NULL;
	json_object *wind = NULL;
	json_object *temperature = NULL;
	if(dbdata == NULL)
		return;

	if (json_object_array_length(dbdata) == 1){
		dbdata_1 = json_object_array_get_idx(dbdata, 0);
		if(dbdata_1){
			if (json_object_object_get_ex(dbdata_1, "today", &today)){
				if (json_object_object_get_ex(today, "weather", &meteorology)){
					if (json_object_get_string(meteorology)){
						weather->meteorology = strdup(json_object_get_string(meteorology));
					}
				}
				if (json_object_object_get_ex(today, "wind", &wind)){
					if (json_object_get_string(wind)){
						weather->wind = strdup(json_object_get_string(wind));
					}
				}
				if (json_object_object_get_ex(today, "temperature", &temperature)){
					if (json_object_get_string(temperature)){
						weather->temperature = strdup(json_object_get_string(temperature));
					}
				}
			}
			if (json_object_object_get_ex(dbdata_1, "area", &area)){
				if (json_object_get_string(area)){
					weather->area = strdup(json_object_get_string(area));
				}
			}
		}
	}

#ifdef DEBUG_SHOW_ALL
	if (weather->meteorology)
		DEBUG("meteorology : %s\n", weather->meteorology);
	if (weather->temperature)
		DEBUG("temperature : %s\n", weather->temperature);
	if (weather->wind)
		DEBUG("wind : %s\n", weather->wind);
	if (weather->area)
		DEBUG("area : %s\n", weather->area);
#endif
}

void _get_sds_reminder(json_object *sds, sds_reminder_t *reminder){
	json_object *reminder_j = NULL;
	json_object *date = NULL;
	json_object *time = NULL;
	json_object *event = NULL;

	if (json_object_object_get_ex(sds, "nlu", &reminder_j)){

		if (json_object_object_get_ex(reminder_j, "date", &date)){
			if (json_object_get_string(date)){
				reminder->date = strdup(json_object_get_string(date));
			}
		}
		if (json_object_object_get_ex(reminder_j, "time", &time)){
			if (json_object_get_string(time)){
				reminder->time = strdup(json_object_get_string(time));
			}
		}
		if (json_object_object_get_ex(reminder_j, "event", &event)){
			if (json_object_get_string(event)){
				reminder->event = strdup(json_object_get_string(event));
			}
		}
	}
#ifdef DEBUG_SHOW_ALL
	if (reminder->date)
		DEBUG("data : %s\n", reminder->date);
	if (reminder->time)
		DEBUG("time : %s\n", reminder->time);
	if (reminder->event)
		DEBUG("event : %s\n", reminder->event);
#endif
}

void _get_sds_chat(json_object *dbdata, sds_chat_t *chat){
	json_object *dbdata_1 = NULL;
	json_object *url = NULL;

	if(dbdata == NULL)
		return;

	if (json_object_array_length(dbdata) == 1){
		dbdata_1 = json_object_array_get_idx(dbdata, 0);
		if(dbdata_1){
			if (json_object_object_get_ex(dbdata_1, "url", &url)){
				if (json_object_get_string(url)){
					chat->url = strdup(json_object_get_string(url));
				}
			}
		}
	}
#ifdef DEBUG_SHOW_ALL
	if (chat->url)
	DEBUG("url : %s\n", chat->url);
#endif
}

void get_sds(json_object *sds_j, sds_info_t *sds){
	json_object *output = NULL;
	json_object *contextId = NULL;
	json_object *data = NULL;
	json_object *dbdata = NULL;
//----------------------------------------------------------------- sds

	_get_sds_domain(sds_j, sds);
	_get_sds_state(sds_j, sds);

	if (json_object_object_get_ex(sds_j, "output", &output)){
		if (json_object_get_string(output)){
			sds->output = strdup(json_object_get_string(output));
		}
	}
	if (json_object_object_get_ex(sds_j, "contextId", &contextId)){
		if (json_object_get_string(contextId)){
			sds->contextId = strdup(json_object_get_string(contextId));
		}
	}

	if (json_object_object_get_ex(sds_j, "data", &data)){
		json_object_object_get_ex(data, "dbdata", &dbdata);
	}

	switch (sds->domain){
	case DOMAIN_MUSIC:
		_get_sds_music(dbdata, &sds->music);
		break;
	case DOMAIN_NETFM:
		_get_sds_netfm(dbdata, &sds->netfm);
		break;
	case DOMAIN_CHAT:
		_get_sds_chat(dbdata, &sds->chat);
		break;
	case DOMAIN_WEATHER:
		_get_sds_weather(dbdata, &sds->weather);
		break;
	case DOMAIN_CALENDAR:
		break;
	case DOMAIN_REMINDER:
		_get_sds_reminder(sds_j, &sds->reminder);
		break;
	case DOMAIN_STOCK:
		break;
	case DOMAIN_POETRY:
		break;
	case DOMAIN_MOVIE:
		break;
	case DOMAIN_COMMAND:
		break;
	default:
		break;
	}

	goto exit_slot;

//exit_error:
	free_sds(sds);
exit_slot:
#ifdef DEBUG_SHOW_ALL
	if(slot_state[sds->state])
		DEBUG("state : %s\n", slot_state[sds->state]);
	if(sds->env)
		DEBUG("env : %s\n",  sds->env);
	if(sds->contextId)
		DEBUG("contextId : %s\n",  sds->contextId);
#endif
	return;
}


