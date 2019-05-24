#ifndef _ASR_H_
#define _ASR_H_

#define ASR_SIZE 3200

#define ASR_CHANNEL	       1

#define ASR_RATE            16000


#define ASR_BIT             16


#define ASR_VOLUME          60

typedef enum {

	ASR_IDLE,

	ASR_START,

	ASR_SPEAK_END,

	ASR_SUCCESS,

	ASR_FAIL,

	ASR_BREAK,

	ASR_MAX
} asr_state_e;

typedef enum sds_state_e{

	SDS_STATE_NULL,

	SDS_STATE_DO,

	SDS_STATE_QUERY,

	SDS_STATE_OFFERNONE,

	SDS_STATE_OFFER,

	SDS_STATE_INTERACT,

	SDS_STATE_EXIT,

	SDS_STATE_MAX
}sds_state_e;


typedef enum domain_e{

	DOMAIN_NULL,

	DOMAIN_MOVIE,

	DOMAIN_MUSIC,

	DOMAIN_WEATHER,

	DOMAIN_COMMAND,

	DOMAIN_CALENDAR,

	DOMAIN_REMINDER,

	DOMAIN_NETFM,

	DOMAIN_CHAT,

	DOMAIN_STORY,

	DOMAIN_JOKE,

	DOMAIN_POETRY,

	DOMAIN_COOKBOOK,

	DOMAIN_STOCK,

	DOMAIN_TRANSLATE,

	DOMAIN_MAX
}domain_e;

typedef struct music_info_t {

	char *url;

	char *title;

	char *artist;
}music_info_t;


typedef struct netfm_info_t {

	char *track;

	char *url;
}netfm_info_t;

typedef struct sem_music_t {

	char *artist;

	char *title;

	char *style;

	char *type;

	char *album;

	char *number;

	char *operation;

	char *volume;

	char *tgt;
}sem_music_t;


typedef struct sem_movie_t {

	char *name;

	char *player;

	char *director;

	char *type;

	char *area;

	char *language;

	char *crowd;

	char *series;

	char *sequence;

	char *channel;

	char *clarity;
}sem_movie_t;


typedef struct sem_weather_t {

	char *provinces;

	char *city;

	char *district;

	char *date;

	char *time;

	char *lunar;

	char *meteorology;
}sem_weather_t;

typedef struct sem_command_t {

	char *device;

	char *number;

	char *operation;

	char *position;

	char *floor;

	char *mode;

	char *color;

	char *timer;

	char *channel;

	char *tv_station;

	char *volume;

	char *luminance;

	char *temperature;

	char *humidity;

	char *wind_speed;

	char *contrast;

	char *color_temperature;

	char *time;

	char *date;

	char *group;

	char *quantity;
}sem_command_t;

typedef struct sem_reminder_t {

	char *time;

	char *date;

	char *event;

	char *object;

	char *quantity;

	char *operation;

	char *repeat;
}sem_reminder_t;

typedef struct sem_netfm_t {

	char *type;

	char *column;

	char *item;

	char *artist;

	char *number;

	char *operation;

	char *radio;

	char *channel;
}sem_netfm_t;

typedef struct sem_story_t {

	char *object;

	char *name;

	char *operation;

	char *type;
}sem_story_t;


typedef struct sem_poetry_t {

	char *object;

	char *name;

	char *operation;

	char *verse;

	char *place;

	char *writer;

	char *search;
}sem_poetry_t;


typedef struct sem_cookbook_t {

	char *dish;
}sem_cookbook_t;


typedef struct sem_stock_t {

	char *name;

	char *tickers;

	char *index;

	char *qualification;

	char *industries;
}sem_stock_t;

typedef struct sem_translate_t {

	char *target_language;

	char *source_language;

	char *content;
}sem_translate_t;


typedef struct sem_one_t {

	domain_e domain;
	union{

		sem_movie_t movie;

		sem_music_t music;

		sem_weather_t weather;

		sem_command_t command;

		sem_reminder_t reminder;

		sem_netfm_t netfm;

		sem_story_t story;

		sem_poetry_t poetry;

		sem_cookbook_t cookbook;

		sem_stock_t stock;

		sem_translate_t translate;
	};
}sem_one_t;

#define NBEST_MAX 3

typedef struct sem_info_t {

	sem_one_t request;

	int nbest_number;

	sem_one_t nbest[NBEST_MAX];
}sem_info_t;

#define MUSIC_MAX 20

typedef struct sds_music_t {

	int number;

	music_info_t data[MUSIC_MAX];
}sds_music_t;

#define NETFM_MAX 10


typedef struct sds_netfm_t {

	int number;

	netfm_info_t data[NETFM_MAX];
}sds_netfm_t;

typedef struct sds_weather_t {

	char *meteorology;

	char *temperature;

	char *wind;

	char *area;
}sds_weather_t;


typedef struct sds_reminder_t {

	char *date;

	char *time;

	char *event;
}sds_reminder_t;


typedef struct sds_chat_t {

	char *url;
}sds_chat_t;


typedef struct sds_info_t {

	sds_state_e state;

	domain_e domain;

	bool is_mult_sds;

	char *contextId;

	char *env;

	char *output;

	char *recordId;
	union{

		sds_music_t music;

		sds_netfm_t netfm;

		sds_weather_t weather;
	
		sds_reminder_t reminder;

		sds_chat_t chat;
	};
}sds_info_t;


typedef struct asr_info_t {

	asr_state_e state;

	int errId;

	char *error;

	char *input;

	sem_info_t sem;

	sds_info_t sds;

}asr_info_t;

extern int asr_start(bool sds);

extern int asr_stop(void);

extern int asr_feed(const void *rec, int size);


extern int asr_cancel(void);

extern void asr_break(void);

#endif

