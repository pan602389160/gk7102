#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ai_vr.h"
#include "slot_sem.h"
#include "gk_app.h"

void _free_sem_movie(sem_movie_t *movie){
	free(movie->name);
	movie->name = NULL;
	free(movie->player);
	movie->player = NULL;
	free(movie->director);
	movie->director = NULL;
	free(movie->type);
	movie->type = NULL;
	free(movie->area);
	movie->area = NULL;

	free(movie->language);
	movie->language = NULL;
	free(movie->crowd);
	movie->crowd = NULL;
	free(movie->series);
	movie->series = NULL;
	free(movie->sequence);
	movie->sequence = NULL;
	free(movie->channel);
	movie->channel = NULL;
	free(movie->clarity);
	movie->clarity = NULL;
}

void _free_sem_music(sem_music_t *music){
	free(music->artist);
	music->artist = NULL;
	free(music->title);
	music->title = NULL;
	free(music->style);
	music->style= NULL;
	free(music->type);
	music->type= NULL;

	free(music->album);
	music->album = NULL;
	free(music->number);
	music->number = NULL;
	free(music->operation);
	music->operation= NULL;
	free(music->volume);
	music->volume= NULL;
	free(music->tgt);
	music->tgt= NULL;
}

void _free_sem_weather(sem_weather_t *weather){
	free(weather->provinces);
	weather->provinces = NULL;
	free(weather->city);
	weather->city = NULL;
	free(weather->district);
	weather->district = NULL;
	free(weather->date);
	weather->date = NULL;
	free(weather->time);
	weather->time = NULL;
	free(weather->lunar);
	weather->lunar = NULL;
	free(weather->meteorology);
	weather->meteorology = NULL;
}

void _free_sem_command(sem_command_t *command){

	free(command->device);
	command->device = NULL;
	free(command->number);
	command->number = NULL;
	free(command->operation);
	command->operation = NULL;
	free(command->position);
	command->position = NULL;
	free(command->floor);
	command->floor = NULL;

	free(command->mode);
	command->mode = NULL;
	free(command->color);
	command->color = NULL;
	free(command->timer);
	command->timer = NULL;
	free(command->channel);
	command->channel = NULL;
	free(command->tv_station);
	command->tv_station = NULL;

	free(command->volume);
	command->volume = NULL;
	free(command->luminance);
	command->luminance = NULL;
	free(command->temperature);
	command->temperature = NULL;
	free(command->humidity);
	command->humidity = NULL;
	free(command->wind_speed);
	command->wind_speed = NULL;

	free(command->contrast);
	command->contrast = NULL;
	free(command->color_temperature);
	command->color_temperature = NULL;
	free(command->time);
	command->time = NULL;
	free(command->position);
	command->date = NULL;
	free(command->group);
	command->group = NULL;

	free(command->quantity);
	command->quantity = NULL;
}

void _free_sem_reminder(sem_reminder_t *reminder){
	free(reminder->time);
	reminder->time = NULL;
	free(reminder->date);
	reminder->date = NULL;
	free(reminder->event);
	reminder->event = NULL;
	free(reminder->object);
	reminder->object = NULL;
	free(reminder->quantity);
	reminder->quantity = NULL;
	free(reminder->operation);
	reminder->operation = NULL;
	free(reminder->repeat);
	reminder->repeat = NULL;

}

void _free_sem_netfm(sem_netfm_t *netfm){
	free(netfm->type);
	netfm->type = NULL;
	free(netfm->column);
	netfm->column = NULL;

	free(netfm->item);
	netfm->item = NULL;

	free(netfm->artist);
	netfm->artist = NULL;

	free(netfm->number);
	netfm->number = NULL;
	free(netfm->operation);
	netfm->operation = NULL;
	free(netfm->radio);
	netfm->radio = NULL;
	free(netfm->channel);
	netfm->channel = NULL;
}

void _free_sem_story(sem_story_t *story){
	free(story->object);
	story->object = NULL;
	free(story->name);
	story->name = NULL;
	free(story->operation);
	story->operation = NULL;
	free(story->type);
	story->type = NULL;
}

void _free_sem_poetry(sem_poetry_t *poetry){
	free(poetry->object);
	poetry->object = NULL;
	free(poetry->name);
	poetry->name = NULL;
	free(poetry->operation);
	poetry->operation = NULL;
	free(poetry->verse);
	poetry->verse = NULL;
	free(poetry->place);
	poetry->place = NULL;
	free(poetry->writer);
	poetry->writer = NULL;
	free(poetry->search);
	poetry->search = NULL;
}

void _free_sem_cookbook(sem_cookbook_t *cookbook){
	free(cookbook->dish);
	cookbook->dish = NULL;
}

void _free_sem_stock(sem_stock_t *stock){
	free(stock->name);
	stock->name = NULL;
	free(stock->tickers);
	stock->tickers = NULL;
	free(stock->index);
	stock->index = NULL;
	free(stock->qualification);
	stock->qualification = NULL;
	free(stock->industries);
	stock->industries = NULL;
}

void _free_sem_translate(sem_translate_t *translate){
	free(translate->target_language);
	translate->target_language = NULL;
	free(translate->source_language);
	translate->source_language = NULL;
	free(translate->content);
	translate->content = NULL;
}


void _free_sem_one(sem_one_t *sem){
	switch (sem->domain){
	case DOMAIN_MOVIE:
		_free_sem_movie(&sem->movie);
		break;
	case DOMAIN_MUSIC:
		_free_sem_music(&sem->music);
		break;
	case DOMAIN_WEATHER:
		_free_sem_weather(&sem->weather);
		break;
	case DOMAIN_COMMAND:
		_free_sem_command(&sem->command);
		break;
	case DOMAIN_REMINDER:
		_free_sem_reminder(&sem->reminder);
		break;
	case DOMAIN_NETFM:
		_free_sem_netfm(&sem->netfm);
		break;
	case DOMAIN_STORY:
		_free_sem_story(&sem->story);
		break;
	case DOMAIN_JOKE:
		/* _free_sem_joke(&sem->joke); */
		break;
	case DOMAIN_POETRY:
		_free_sem_poetry(&sem->poetry);
		break;
	case DOMAIN_COOKBOOK:
		_free_sem_cookbook(&sem->cookbook);
		break;
	case DOMAIN_STOCK:
		_free_sem_stock(&sem->stock);
		break;
	case DOMAIN_TRANSLATE:
		_free_sem_translate(&sem->translate);
		break;
	default:
		break;
	}
	sem->domain = DOMAIN_NULL;
}

void free_sem(sem_info_t *sem)
{
	int i;
	sem_one_t *p_sem;
	p_sem = &sem->request;
	_free_sem_one(p_sem);
	//for(i=0; i<sem->nbest_number; i++){
	//	p_sem = &sem->nbest[i];
	//	_free_sem_one(p_sem);
	//}
	sem->nbest_number = 0;
}


void _get_sem_domain(json_object *sem, sem_one_t *sem_one){
	json_object *domain = NULL;
	//----------------------------------------------------------------- domain , env
	if (json_object_object_get_ex(sem, "domain", &domain)){
		if (json_object_get_string(domain)){
			if(strcmp(json_object_get_string(domain), "电影搜索") == 0){
				sem_one->domain = DOMAIN_MOVIE;
			} else if(strcmp(json_object_get_string(domain), "音乐") == 0){
			 	sem_one->domain = DOMAIN_MUSIC;
			} else if(strcmp(json_object_get_string(domain), "天气") == 0){
				sem_one->domain = DOMAIN_WEATHER;
			} else if(strcmp(json_object_get_string(domain), "中控") == 0){
				sem_one->domain = DOMAIN_COMMAND;
			} else if(strcmp(json_object_get_string(domain), "提醒") == 0){
				sem_one->domain = DOMAIN_REMINDER;
			} else if(strcmp(json_object_get_string(domain), "日历") == 0){
				sem_one->domain = DOMAIN_CALENDAR;
			} else if(strcmp(json_object_get_string(domain), "电台") == 0){
				sem_one->domain = DOMAIN_NETFM;
			} else if(strcmp(json_object_get_string(domain), "聊天") == 0){
				sem_one->domain = DOMAIN_CHAT;
			} else if(strcmp(json_object_get_string(domain), "故事") == 0){
				sem_one->domain = DOMAIN_STORY;
			} else if(strcmp(json_object_get_string(domain), "笑话") == 0){
				sem_one->domain = DOMAIN_JOKE;
			} else if(strcmp(json_object_get_string(domain), "古诗") == 0){
				sem_one->domain = DOMAIN_POETRY;
			} else if(strcmp(json_object_get_string(domain), "菜谱") == 0){
				sem_one->domain = DOMAIN_COOKBOOK;
			} else if(strcmp(json_object_get_string(domain), "股票") == 0){
				sem_one->domain = DOMAIN_STOCK;
			} else if(strcmp(json_object_get_string(domain), "翻译") == 0){
				sem_one->domain = DOMAIN_TRANSLATE;
			} else {
				sem_one->domain = DOMAIN_MAX;
			}
		}
	}
}


void _get_sem_music(json_object *param, sem_music_t *music){

	json_object *artist = NULL;
	json_object *title = NULL;
	json_object *style = NULL;
	json_object *type = NULL;
	json_object *album = NULL;
	json_object *number = NULL;
	json_object *operation = NULL;
	json_object *volume = NULL;
	json_object *act = NULL;
	json_object *tgt = NULL;

	if (json_object_object_get_ex(param, "歌手名", &artist)){
		if (json_object_get_string(artist)){
			music->artist = strdup(json_object_get_string(artist));
		}
	}
	if (json_object_object_get_ex(param, "歌曲名", &title)){
		if (json_object_get_string(title)){
			music->title = strdup(json_object_get_string(title));
		}
	}
	if (json_object_object_get_ex(param, "风格", &style)){
		if (json_object_get_string(style)){
			music->style = strdup(json_object_get_string(style));
		}
	}
	if (json_object_object_get_ex(param, "音乐类型", &type)){
		if (json_object_get_string(type)){
			music->type = strdup(json_object_get_string(type));
		}
	}
	if (json_object_object_get_ex(param, "专辑名", &album)){
		if (json_object_get_string(album)){
			music->album = strdup(json_object_get_string(album));
		}
	}
	if (json_object_object_get_ex(param, "序列号", &number)){
		if (json_object_get_string(number)){
			music->number = strdup(json_object_get_string(number));
		}
	}
	if (json_object_object_get_ex(param, "操作", &operation)){
		if (json_object_get_string(operation)){
			music->operation = strdup(json_object_get_string(operation));
		}
	}
	if (json_object_object_get_ex(param, "音量", &volume)){
		if (json_object_get_string(volume)){
			music->volume = strdup(json_object_get_string(volume));
		}
	}
	if (json_object_object_get_ex(param, "__act__", &act)){
		if (strcmp(json_object_get_string(act), "request") == 0) {
			if (json_object_object_get_ex(param, "__tgt__", &tgt))
				if (json_object_get_string(tgt))
					music->tgt = strdup(json_object_get_string(tgt));
		}
	}
#ifdef DEBUG_SHOW_ALL
	if (music->artist)
		DEBUG("artist = %s\n", music->artist);
	if (music->title)
		DEBUG("title = %s\n", music->title);
	if (music->style)
		DEBUG("style = %s\n", music->style);
	if (music->type)
		DEBUG("type = %s\n", music->type);
	if (music->album)
		DEBUG("album = %s\n", music->album);
	if (music->number)
		DEBUG("number = %s\n", music->number);
	if (music->operation)
		DEBUG("operation = %s\n", music->operation);
	if (music->volume)
		DEBUG("volume = %s\n", music->volume);
	if (music->tgt)
		DEBUG("tgt = %s\n", music->tgt);
#endif
}

void _get_sem_movie(json_object *param, sem_movie_t *movie){
	json_object *name = NULL;
	json_object *player = NULL;
	json_object *director = NULL;
	json_object *type = NULL;
	json_object *area = NULL;
	json_object *language = NULL;
	json_object *crowd = NULL;
	json_object *series = NULL;
	json_object *sequence = NULL;
	json_object *channel = NULL;
	json_object *clarity = NULL;


	if (json_object_object_get_ex(param, "片名", &name)){
		if (json_object_get_string(name)){
			movie->name = strdup(json_object_get_string(name));
		}
	}
	if (json_object_object_get_ex(param, "主演", &player)){
		if (json_object_get_string(player)){
			movie->player = strdup(json_object_get_string(player));
		}
	}
	if (json_object_object_get_ex(param, "导演", &director)){
		if (json_object_get_string(director)){
			movie->director = strdup(json_object_get_string(director));
		}
	}
	if (json_object_object_get_ex(param, "类型", &type)){
		if (json_object_get_string(type)){
			movie->type = strdup(json_object_get_string(type));
		}
	}
	if (json_object_object_get_ex(param, "国家地区", &area)){
		if (json_object_get_string(area)){
			movie->area = strdup(json_object_get_string(area));
		}
	}
	if (json_object_object_get_ex(param, "语言", &language)){
		if (json_object_get_string(language)){
			movie->language = strdup(json_object_get_string(language));
		}
	}
	if (json_object_object_get_ex(param, "系列", &crowd)){
		if (json_object_get_string(crowd)){
			movie->crowd = strdup(json_object_get_string(crowd));
		}
	}
	if (json_object_object_get_ex(param, "系列", &series)){
		if (json_object_get_string(series)){
			movie->series = strdup(json_object_get_string(series));
		}
	}
	if (json_object_object_get_ex(param, "序列号", &sequence)){
		if (json_object_get_string(sequence)){
			movie->sequence = strdup(json_object_get_string(sequence));
		}
	}
	if (json_object_object_get_ex(param, "频道", &channel)){
		if (json_object_get_string(channel)){
			movie->channel = strdup(json_object_get_string(channel));
		}
	}
	if (json_object_object_get_ex(param, "清晰度", &clarity)){
		if (json_object_get_string(clarity)){
			movie->clarity = strdup(json_object_get_string(clarity));
		}
	}

#ifdef DEBUG_SHOW_ALL
	if (movie->name)
		DEBUG("name = %s\n", movie->name);
	if (movie->player)
		DEBUG("player = %s\n", movie->player);
	if (movie->director)
		DEBUG("director = %s\n", movie->director);
	if (movie->type)
		DEBUG("type = %s\n", movie->type);
	if (movie->area)
		DEBUG("area = %s\n", movie->area);

	if (movie->language)
		DEBUG("language = %s\n", movie->language);
	if (movie->crowd)
		DEBUG("crowd = %s\n", movie->crowd);
	if (movie->series)
		DEBUG("series = %s\n", movie->series);
	if (movie->sequence)
		DEBUG("sequence = %s\n", movie->sequence);
	if (movie->channel)
		DEBUG("channel = %s\n", movie->channel);
	if (movie->clarity)
		DEBUG("clarity = %s\n", movie->clarity);
#endif
}

void _get_sem_weather(json_object *param, sem_weather_t *weather){
	json_object *provinces = NULL;
	json_object *city = NULL;
	json_object *district = NULL;
	json_object *date = NULL;
	json_object *time = NULL;
	json_object *lunar = NULL;
	json_object *meteorology = NULL;

	if (json_object_object_get_ex(param, "省份", &provinces)){
		if (json_object_get_string(provinces)){
			weather->provinces = strdup(json_object_get_string(provinces));
		}
	}
	if (json_object_object_get_ex(param, "城市", &city)){
		if (json_object_get_string(city)){
			weather->city = strdup(json_object_get_string(city));
		}
	}
	if (json_object_object_get_ex(param, "区域", &district)){
		if (json_object_get_string(district)){
			weather->district = strdup(json_object_get_string(district));
		}
	}
	if (json_object_object_get_ex(param, "日期", &date)){
		if (json_object_get_string(date)){
			weather->date = strdup(json_object_get_string(date));
		}
	}
	if (json_object_object_get_ex(param, "时间", &time)){
		if (json_object_get_string(time)){
			weather->time = strdup(json_object_get_string(time));
		}
	}
	if (json_object_object_get_ex(param, "阴历日期", &lunar)){
		if (json_object_get_string(lunar)){
			weather->lunar = strdup(json_object_get_string(lunar));
		}
	}
	if (json_object_object_get_ex(param, "气象", &meteorology)){
		if (json_object_get_string(meteorology)){
			weather->meteorology = strdup(json_object_get_string(meteorology));
		}
	}

#ifdef DEBUG_SHOW_ALL
	if (weather->provinces)
		DEBUG("provinces = %s\n", weather->provinces);
	if (weather->city)
		DEBUG("city = %s\n", weather->city);
	if (weather->district)
		DEBUG("district = %s\n", weather->district);
	if (weather->date)
		DEBUG("date = %s\n", weather->date);
	if (weather->time)
		DEBUG("time = %s\n", weather->time);
	if (weather->lunar)
		DEBUG("lunar = %s\n", weather->lunar);
	if (weather->meteorology)
		DEBUG("meteorology = %s\n", weather->meteorology);
#endif
}

void _get_sem_command(json_object *param, sem_command_t *command){
	json_object *device = NULL;
	json_object *number = NULL;
	json_object *operation = NULL;
	json_object *position = NULL;
	json_object *floor = NULL;
	json_object *mode = NULL;
	json_object *color = NULL;
	json_object *timer = NULL;
	json_object *channel = NULL;
	json_object *tv_station = NULL;
	json_object *volume = NULL;
	json_object *luminance = NULL;
	json_object *temperature = NULL;
	json_object *humidity = NULL;
	json_object *wind_speed = NULL;
	json_object *contrast = NULL;
	json_object *color_temperature = NULL;
	json_object *time = NULL;
	json_object *date = NULL;
	json_object *group = NULL;
	json_object *quantity = NULL;

	if (json_object_object_get_ex(param, "设备名称", &device)){
		if (json_object_get_string(device)){
			command->device = strdup(json_object_get_string(device));
		}
	}
	if (json_object_object_get_ex(param, "设备编号", &number)){
		if (json_object_get_string(number)){
			command->number = strdup(json_object_get_string(number));
		}
	}
	if (json_object_object_get_ex(param, "操作", &operation)){
		if (json_object_get_string(operation)){
			command->operation = strdup(json_object_get_string(operation));
		}
	}
	if (json_object_object_get_ex(param, "位置", &position)){
		if (json_object_get_string(position)){
			command->position = strdup(json_object_get_string(position));
		}
	}
	if (json_object_object_get_ex(param, "楼层", &floor)){
		if (json_object_get_string(floor)){
			command->floor = strdup(json_object_get_string(floor));
		}
	}
	if (json_object_object_get_ex(param, "模式", &mode)){
		if (json_object_get_string(mode)){
			command->mode = strdup(json_object_get_string(mode));
		}
	}
	if (json_object_object_get_ex(param, "颜色", &color)){
		if (json_object_get_string(color)){
			command->color = strdup(json_object_get_string(color));
		}
	}
	if (json_object_object_get_ex(param, "定时", &timer)){
		if (json_object_get_string(timer)){
			command->timer = strdup(json_object_get_string(timer));
		}
	}
	if (json_object_object_get_ex(param, "频道", &channel)){
		if (json_object_get_string(channel)){
			command->channel = strdup(json_object_get_string(channel));
		}
	}
	if (json_object_object_get_ex(param, "电视台", &tv_station)){
		if (json_object_get_string(tv_station)){
			command->tv_station = strdup(json_object_get_string(tv_station));
		}
	}
	if (json_object_object_get_ex(param, "音量", &volume)){
		if (json_object_get_string(volume)){
			command->volume = strdup(json_object_get_string(volume));
		}
	}
	if (json_object_object_get_ex(param, "亮度", &luminance)){
		if (json_object_get_string(luminance)){
			command->luminance = strdup(json_object_get_string(luminance));
		}
	}
	if (json_object_object_get_ex(param, "温度", &temperature)){
		if (json_object_get_string(temperature)){
			command->temperature = strdup(json_object_get_string(temperature));
		}
	}
	if (json_object_object_get_ex(param, "湿度", &humidity)){
		if (json_object_get_string(humidity)){
			command->humidity = strdup(json_object_get_string(humidity));
		}
	}
	if (json_object_object_get_ex(param, "风速", &wind_speed)){
		if (json_object_get_string(wind_speed)){
			command->wind_speed = strdup(json_object_get_string(wind_speed));
		}
	}
	if (json_object_object_get_ex(param, "对比度", &contrast)){
		if (json_object_get_string(contrast)){
			command->contrast = strdup(json_object_get_string(contrast));
		}
	}
	if (json_object_object_get_ex(param, "色温", &color_temperature)){
		if (json_object_get_string(color_temperature)){
			command->color_temperature = strdup(json_object_get_string(color_temperature));
		}
	}
	if (json_object_object_get_ex(param, "时间", &time)){
		if (json_object_get_string(time)){
			command->time = strdup(json_object_get_string(time));
		}
	}
	if (json_object_object_get_ex(param, "日期", &date)){
		if (json_object_get_string(date)){
			command->date = strdup(json_object_get_string(date));
		}
	}
	if (json_object_object_get_ex(param, "分组", &group)){
		if (json_object_get_string(group)){
			command->group = strdup(json_object_get_string(group));
		}
	}
	if (json_object_object_get_ex(param, "数量", &quantity)){
		if (json_object_get_string(quantity)){
			command->quantity = strdup(json_object_get_string(quantity));
		}
	}
#ifdef DEBUG_SHOW_ALL
	if (command->device)
		DEBUG("device = %s\n", command->device);
	if (command->number)
		DEBUG("number = %s\n", command->number);
	if (command->operation)
		DEBUG("operation = %s\n", command->operation);
	if (command->position)
		DEBUG("position = %s\n", command->position);
	if (command->floor)
		DEBUG("floor = %s\n", command->floor);
	if (command->mode)
		DEBUG("mode = %s\n", command->mode);
	if (command->color)
		DEBUG("color = %s\n", command->color);
	if (command->timer)
		DEBUG("timer = %s\n", command->timer);
	if (command->tv_station)
		DEBUG("tv_station = %s\n", command->tv_station);
	if (command->volume)
		DEBUG("volume = %s\n", command->volume);
	if (command->luminance)
		DEBUG("luminance = %s\n", command->luminance);
	if (command->temperature)
		DEBUG("temperature = %s\n", command->temperature);
	if (command->humidity)
		DEBUG("humidity = %s\n", command->humidity);
	if (command->wind_speed)
		DEBUG("wind_speed = %s\n", command->wind_speed);
	if (command->contrast)
		DEBUG("contrast = %s\n", command->contrast);
	if (command->color_temperature)
		DEBUG("color_temperature = %s\n", command->color_temperature);
	if (command->time)
		DEBUG("time = %s\n", command->time);
	if (command->date)
		DEBUG("date = %s\n", command->date);
	if (command->group)
		DEBUG("group = %s\n", command->group);
	if (command->quantity)
		DEBUG("quantity = %s\n", command->quantity);
#endif
}

void _get_sem_reminder(json_object *param, sem_reminder_t *reminder){
	json_object *time = NULL;
	json_object *date = NULL;
	json_object *event = NULL;
	json_object *object = NULL;
	json_object *quantity = NULL;
	json_object *operation = NULL;
	json_object *repeat = NULL;

	if (json_object_object_get_ex(param, "时间", &time)){
		if (json_object_get_string(time)){
			reminder->time = strdup(json_object_get_string(time));
		}
	}
	if (json_object_object_get_ex(param, "日期", &date)){
		if (json_object_get_string(date)){
			reminder->date = strdup(json_object_get_string(date));
		}
	}
	if (json_object_object_get_ex(param, "事件", &event)){
		if (json_object_get_string(event)){
			reminder->event = strdup(json_object_get_string(event));
		}
	}
	if (json_object_object_get_ex(param, "对象", &object)){
		if (json_object_get_string(object)){
			reminder->object = strdup(json_object_get_string(object));
		}
	}
	if (json_object_object_get_ex(param, "数量", &quantity)){
		if (json_object_get_string(quantity)){
			reminder->quantity = strdup(json_object_get_string(quantity));
		}
	}
	if (json_object_object_get_ex(param, "操作", &operation)){
		if (json_object_get_string(operation)){
			reminder->operation = strdup(json_object_get_string(operation));
		}
	}
	if (json_object_object_get_ex(param, "repeat", &repeat)){
		if (json_object_get_string(repeat)){
			reminder->repeat = strdup(json_object_get_string(repeat));
		}
	}
#ifdef DEBUG_SHOW_ALL
	if (reminder->time)
		DEBUG("time = %s\n", reminder->time);
	if (reminder->date)
		DEBUG("date = %s\n", reminder->date);
	if (reminder->event)
		DEBUG("event = %s\n", reminder->event);
	if (reminder->object)
		DEBUG("object = %s\n", reminder->object);
	if (reminder->quantity)
		DEBUG("quantity = %s\n", reminder->quantity);
	if (reminder->operation)
		DEBUG("operation = %s\n", reminder->operation);
	if (reminder->repeat)
		DEBUG("repeat = %s\n", reminder->repeat);
#endif
}

void _get_sem_netfm(json_object *param, sem_netfm_t *netfm){

	json_object *type = NULL;
	json_object *column = NULL;
	json_object *item = NULL;
	json_object *artist = NULL;
	json_object *number = NULL;
	json_object *operation = NULL;
	json_object *radio = NULL;
	json_object *channel = NULL;

	if (json_object_object_get_ex(param, "类别", &type)){
		if (json_object_get_string(type)){
			netfm->type = strdup(json_object_get_string(type));
		}
	}
	if (json_object_object_get_ex(param, "栏目", &column)){
		if (json_object_get_string(column)){
			netfm->column = strdup(json_object_get_string(column));
		}
	}
	if (json_object_object_get_ex(param, "节目", &item)){
		if (json_object_get_string(item)){
			netfm->item = strdup(json_object_get_string(item));
		}
	}
	if (json_object_object_get_ex(param, "艺人", &artist)){
		if (json_object_get_string(artist)){
			netfm->artist = strdup(json_object_get_string(artist));
		}
	}
	if (json_object_object_get_ex(param, "序列号", &number)){
		if (json_object_get_string(number)){
			netfm->number = strdup(json_object_get_string(number));
		}
	}
	if (json_object_object_get_ex(param, "操作", &operation)){
		if (json_object_get_string(operation)){
			netfm->operation = strdup(json_object_get_string(operation));
		}
	}
	if (json_object_object_get_ex(param, "电台", &radio)){
		if (json_object_get_string(radio)){
			netfm->radio = strdup(json_object_get_string(radio));
		}
	}
	if (json_object_object_get_ex(param, "频道", &channel)){
		if (json_object_get_string(channel)){
			netfm->channel = strdup(json_object_get_string(channel));
		}
	}
#ifdef DEBUG_SHOW_ALL
	if (netfm->type)
		DEBUG("type = %s\n", netfm->type);
	if (netfm->column)
		DEBUG("column = %s\n", netfm->column);
	if (netfm->item)
		DEBUG("item = %s\n", netfm->item);
	if (netfm->artist)
		DEBUG("artist = %s\n", netfm->artist);
	if (netfm->number)
		DEBUG("number = %s\n", netfm->number);
	if (netfm->operation)
		DEBUG("operation = %s\n", netfm->operation);
	if (netfm->radio)
		DEBUG("radio = %s\n", netfm->radio);
	if (netfm->channel)
		DEBUG("channel = %s\n", netfm->channel);
#endif
}


void _get_sem_story(json_object *param, sem_story_t *story){
	json_object *object = NULL;
	json_object *name = NULL;
	json_object *operation = NULL;
	json_object *type = NULL;

	if (json_object_object_get_ex(param, "对象", &object)){
		if (json_object_get_string(object)){
			story->object = strdup(json_object_get_string(object));
		}
	}
	if (json_object_object_get_ex(param, "故事名", &name)){
		if (json_object_get_string(name)){
			story->name = strdup(json_object_get_string(name));
		}
	}
	if (json_object_object_get_ex(param, "操作", &operation)){
		if (json_object_get_string(operation)){
			story->operation = strdup(json_object_get_string(operation));
		}
	}
	if (json_object_object_get_ex(param, "故事类型", &type)){
		if (json_object_get_string(type)){
			story->type = strdup(json_object_get_string(type));
		}
	}
#ifdef DEBUG_SHOW_ALL
	if (story->object)
		DEBUG("object = %s\n", story->object);
	if (story->name)
		DEBUG("name = %s\n", story->name);
	if (story->operation)
		DEBUG("operation = %s\n", story->operation);
	if (story->type)
		DEBUG("type = %s\n", story->type);
#endif
}

void _get_sem_poetry(json_object *param, sem_poetry_t *poetry){
	json_object *object = NULL;
	json_object *name = NULL;
	json_object *operation = NULL;
	json_object *verse = NULL;
	json_object *place = NULL;
	json_object *writer = NULL;
	json_object *search = NULL;

	if (json_object_object_get_ex(param, "对象", &object)){
		if (json_object_get_string(object)){
			poetry->object = strdup(json_object_get_string(object));
		}
	}

	if (json_object_object_get_ex(param, "古诗名", &name)){
		if (json_object_get_string(name)){
			poetry->name = strdup(json_object_get_string(name));
		}
	}
	if (json_object_object_get_ex(param, "操作", &operation)){
		if (json_object_get_string(operation)){
			poetry->operation = strdup(json_object_get_string(operation));
		}
	}
	if (json_object_object_get_ex(param, "诗句", &verse)){
		if (json_object_get_string(verse)){
			poetry->verse = strdup(json_object_get_string(verse));
		}
	}
	if (json_object_object_get_ex(param, "位置", &place)){
		if (json_object_get_string(place)){
			poetry->place = strdup(json_object_get_string(place));
		}
	}
	if (json_object_object_get_ex(param, "作者", &writer)){
		if (json_object_get_string(writer)){
			poetry->writer = strdup(json_object_get_string(writer));
		}
	}
	if (json_object_object_get_ex(param, "查询对象", &search)){
		if (json_object_get_string(search)){
			poetry->search = strdup(json_object_get_string(search));
		}
	}

#ifdef DEBUG_SHOW_ALL
	if (poetry->object)
		DEBUG("object = %s\n", poetry->object);
	if (poetry->name)
		DEBUG("name = %s\n", poetry->name);
	if (poetry->operation)
		DEBUG("operation = %s\n", poetry->operation);
	if (poetry->verse)
		DEBUG("verse = %s\n", poetry->verse);
	if (poetry->place)
		DEBUG("place = %s\n", poetry->place);
	if (poetry->writer)
		DEBUG("writer = %s\n", poetry->writer);
	if (poetry->search)
		DEBUG("search = %s\n", poetry->search);
#endif
}



void _get_sem_cookbook(json_object *param, sem_cookbook_t *cookbook){
	json_object *dish = NULL;

	if (json_object_object_get_ex(param, "菜名", &dish)){
		if (json_object_get_string(dish)){
			cookbook->dish = strdup(json_object_get_string(dish));
		}
	}

#ifdef DEBUG_SHOW_ALL
	if (cookbook->dish)
		DEBUG("dish = %s\n", cookbook->dish);
#endif
}

void _get_sem_stock(json_object *param, sem_stock_t *stock){
	json_object *name = NULL;
	json_object *tickers = NULL;
	json_object *index = NULL;
	json_object *qualification = NULL;
	json_object *industries = NULL;

	if (json_object_object_get_ex(param, "名称", &name)){
		if (json_object_get_string(name)){
			stock->name = strdup(json_object_get_string(name));
		}
	}
	if (json_object_object_get_ex(param, "代码", &tickers)){
		if (json_object_get_string(tickers)){
			stock->tickers = strdup(json_object_get_string(tickers));
		}
	}
	if (json_object_object_get_ex(param, "指数", &index)){
		if (json_object_get_string(index)){
			stock->index = strdup(json_object_get_string(index));
		}
	}
	if (json_object_object_get_ex(param, "技术指标", &qualification)){
		if (json_object_get_string(qualification)){
			stock->qualification = strdup(json_object_get_string(qualification));
		}
	}
	if (json_object_object_get_ex(param, "行业", &industries)){
		if (json_object_get_string(industries)){
			stock->industries = strdup(json_object_get_string(industries));
		}
	}

#ifdef DEBUG_SHOW_ALL
	if (stock->name)
		DEBUG("name = %s\n", stock->name);
	if (stock->tickers)
		DEBUG("tickers = %s\n", stock->tickers);
	if (stock->index)
		DEBUG("index = %s\n", stock->index);
	if (stock->qualification)
		DEBUG("qualification = %s\n", stock->qualification);
	if (stock->industries)
		DEBUG("industries = %s\n", stock->industries);
#endif
}

void _get_sem_translate(json_object *param, sem_translate_t *translate){
	json_object *target_language = NULL;
	json_object *source_language = NULL;
	json_object *content = NULL;

	if (json_object_object_get_ex(param, "目标语言", &target_language)){
		if (json_object_get_string(target_language)){
			translate->target_language = strdup(json_object_get_string(target_language));
		}
	}
	if (json_object_object_get_ex(param, "源语言", &source_language)){
		if (json_object_get_string(source_language)){
			translate->source_language = strdup(json_object_get_string(source_language));
		}
	}
	if (json_object_object_get_ex(param, "内容", &content)){
		if (json_object_get_string(content)){
			translate->content = strdup(json_object_get_string(content));
		}
	}

#ifdef DEBUG_SHOW_ALL
	if (translate->target_language)
		DEBUG("target_language = %s\n", translate->target_language);
	if (translate->source_language)
		DEBUG("source_language = %s\n", translate->source_language);
	if (translate->content)
		DEBUG("content = %s\n", translate->content);
#endif
}


void _get_request(json_object *request, sem_one_t *sem_one){
	DEBUG_FUNC_START;
	json_object *param = NULL;

	_get_sem_domain(request, sem_one);
	DEBUG("domain = %s, %d\n", vr_domain_str[sem_one->domain], sem_one->domain);
	if (json_object_object_get_ex(request, "param", &param)){
		switch (sem_one->domain){
		case DOMAIN_MOVIE:
			_get_sem_movie(param ,&sem_one->movie);
			break;
		case DOMAIN_MUSIC:
			_get_sem_music(param ,&sem_one->music);
			break;
		case DOMAIN_WEATHER:
			_get_sem_weather(param ,&sem_one->weather);
			break;
		case DOMAIN_COMMAND:
			_get_sem_command(param ,&sem_one->command);
			break;
		case DOMAIN_REMINDER:
			_get_sem_reminder(param ,&sem_one->reminder);
			break;
		case DOMAIN_NETFM:
			_get_sem_netfm(param ,&sem_one->netfm);
			break;
		case DOMAIN_STORY:
			_get_sem_story(param ,&sem_one->story);
			break;
		case DOMAIN_POETRY:
			_get_sem_poetry(param ,&sem_one->poetry);
			break;
		case DOMAIN_COOKBOOK:
			_get_sem_cookbook(param ,&sem_one->cookbook);
			break;
		case DOMAIN_STOCK:
			_get_sem_stock(param ,&sem_one->stock);
			break;
		case DOMAIN_TRANSLATE:
			_get_sem_translate(param ,&sem_one->translate);
			break;
		default:
			break;
		}
	}

	DEBUG_FUNC_END;
}

void _get_nbast(json_object *nbast, sem_info_t *sem){
	DEBUG_FUNC_START;
	json_object *nbest_one = NULL;
	int number = 0;
	int i =0;
	number = json_object_array_length(nbast);
	if (number > NBEST_MAX){
		number = NBEST_MAX;
	}
	sem->nbest_number = number;
	DEBUG("nbest num = %d\n", number);
	for (i = 0; i < number; i++){
		json_object *request = NULL;
		nbest_one = json_object_array_get_idx(nbast, i);
		if (json_object_object_get_ex(nbest_one, "request", &request)){
			_get_request(request, &sem->nbest[i]);
		}
	}
	DEBUG_FUNC_END;
}

void get_sem(json_object *sem, sem_info_t *sem_info){
	json_object *request = NULL;
	json_object *nbest = NULL;

	if (json_object_object_get_ex(sem, "request", &request)){
		_get_request(request, &sem_info->request);
	}

	if (json_object_object_get_ex(sem, "nbest", &nbest)){
		//-------------------------- nbest
		_get_nbast(nbest, sem_info);
	}

}


