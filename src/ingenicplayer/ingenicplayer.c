#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "json-c/json.h"
#include "utils_interface.h"

#include "linklist_interface.h"
#include "ingenicplayer.h"
#include "mozart_musicplayer.h"
#include "ray_key_function.h"
#include "debug_print.h"
/* #define MOZART_INGENICPLAYER_DEBUG */
#ifdef MOZART_INGENICPLAYER_DEBUG
#define pr_debug(fmt, args...)				\
	printf("[INGENICPLAYER] %s %d: "fmt, __func__, __LINE__, ##args)
#else  /* MOZART_INGENICPLAYER_DEBUG */
#define pr_debug(fmt, args...)			\
	do {} while (0)
#endif	/* MOZART_INGENICPLAYER_DEBUG */

#define pr_info(fmt, args...)						\
	printf("[INGENICPLAYER] [INFO] "fmt, ##args)

#define pr_err(fmt, args...)						\
	fprintf(stderr, "[INGENICPLAYER] [ERROR] %s %d: "fmt, __func__, __LINE__, ##args)

#define DEVICENAME "/usr/data/ingenicplayer/devicename"
#define DEVICE_INGENICPLAYER_DIR "/usr/data/ingenicplayer/"

extern int music_vol;


enum {
	INGENICPLAYER_PLAY_PAUSE = 0,
	INGENICPLAYER_PREV,
	INGENICPLAYER_NEXT,
	INGENICPLAYER_PLAY_MODE,
	INGENICPLAYER_INC_VOLUME,
	INGENICPLAYER_DEC_VOLUME,
	INGENICPLAYER_SET_VOLUME,
	INGENICPLAYER_GET_VOLUME,
	INGENICPLAYER_GET_SONG_INFO,
	INGENICPLAYER_GET_SONG_QUEUE,
	INGENICPLAYER_ADD_QUEUE,
	INGENICPLAYER_REPLACE_QUEUE,
	INGENICPLAYER_DEL_QUEUE,
	INGENICPLAYER_PLAY_QUEUE,
	INGENICPLAYER_GET_DEVICE_INFO,
	INGENICPLAYER_HEART_PULSATE,
	INGENICPLAYER_SET_SEEK,
	INGENICPLAYER_GET_POSITION,
	INGENICPLAYER_GET_SD_MUSIC,
	INGENICPLAYER_SET_SHORTCUT,
	INGENICPLAYER_GET_SHORTCUT,
	INGENICPLAYER_PLAY_SHORTCUT,
};

char *ingenicplayer_cmd[] = {
	[INGENICPLAYER_PLAY_PAUSE] = "toggle", /* play/pause */
	[INGENICPLAYER_PREV] = "prefix", /* previous */
	[INGENICPLAYER_NEXT] = "skip", /* next */
	[INGENICPLAYER_PLAY_MODE] = "set_queue_mode", /* play mode */
	[INGENICPLAYER_INC_VOLUME] = "increase_volume",
	[INGENICPLAYER_DEC_VOLUME] = "decrease_volume",
	[INGENICPLAYER_SET_VOLUME] = "set_volume",
	[INGENICPLAYER_GET_VOLUME] = "get_volume",
	[INGENICPLAYER_GET_SONG_INFO] = "get_song_info",
	[INGENICPLAYER_GET_SONG_QUEUE] = "get_song_queue",
	[INGENICPLAYER_ADD_QUEUE] = "add_queue_to_play",
	[INGENICPLAYER_REPLACE_QUEUE] = "replace_queue",
	[INGENICPLAYER_DEL_QUEUE] = "delete_from_queue",
	[INGENICPLAYER_PLAY_QUEUE] = "play_from_queue",
	[INGENICPLAYER_GET_DEVICE_INFO] = "get_device_info",
	[INGENICPLAYER_HEART_PULSATE] = "heart_pulsate",
	[INGENICPLAYER_SET_SEEK] = "set_seek",
	[INGENICPLAYER_GET_POSITION] = "get_position",
	[INGENICPLAYER_GET_SD_MUSIC] = "get_sd_music",
	[INGENICPLAYER_SET_SHORTCUT] = "set_shortcut",
	[INGENICPLAYER_GET_SHORTCUT] = "get_shortcut",
	[INGENICPLAYER_PLAY_SHORTCUT] = "play_shortcut",
};

struct cmd_info_c {
	struct appserver_cli_info *owner;
	char *command;
	char *data;
};

static List cmd_list;
static bool die_out = false;
static pthread_t ingenicplayer_thread;
static pthread_mutex_t cmd_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_var = PTHREAD_COND_INITIALIZER;

/* TODO: snd_source need lock */
static int mozart_module_ingenicplayer_start(void)
{
	#if 0
	if (snd_source != SND_SRC_INGENICPLAYER) {
		if (mozart_module_stop())
			return -1;
		if (mozart_musicplayer_start(mozart_musicplayer_handler))
			return -1;
		snd_source = SND_SRC_INGENICPLAYER;
	} else if (!mozart_musicplayer_is_active(mozart_musicplayer_handler)) {
		if (mozart_musicplayer_start(mozart_musicplayer_handler))
			return -1;
	} else {
		mozart_musicplayer_musiclist_clean(mozart_musicplayer_handler);
	}
	#endif
	return 0;
}

static void free_cmd_list(void *cmd_list_info)
{
	struct cmd_info_c *cmd_info = (struct cmd_info_c *)cmd_list_info;
	if (cmd_info) {
		if (cmd_info->owner) {
			free(cmd_info->owner);
			cmd_info->owner = NULL;
		}
		if (cmd_info->command) {
			free(cmd_info->command);
			cmd_info->command = NULL;
		}
		if (cmd_info->data) {
			free(cmd_info->data);
			cmd_info->data = NULL;
		}
		free(cmd_info);
	}
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

static char *sync_devicename(char *newname)
{
	int namelen = 0;
	char *devicename = NULL;
	FILE *fd = NULL;

	if ((fd = fopen(DEVICENAME, "a+")) == NULL) {
		pr_err("fopen failed %s\n", strerror(errno));
		return NULL;
	}
	if (newname) {
		truncate(DEVICENAME, 0);
		fwrite(newname, sizeof(char), strlen(newname), fd);
	} else {
		fseek(fd, 0, SEEK_END);
		namelen = ftell(fd);
		fseek(fd, 0, SEEK_SET);
		if (namelen) {
			devicename = (char *)calloc(namelen, sizeof(char) + 1);
			if (devicename) {
				fread(devicename, sizeof(char), namelen, fd);
				pr_debug("get devicename %s\n", devicename);
			}
		}
	}
	fclose(fd);

	return devicename;
}

static int ingenicplayer_play_pause(void)
{
	return mozart_musicplayer_play_pause(mozart_musicplayer_handler);
}

static void get_song_json_object(json_object *object, struct music_info *info)
{
	json_object *reply_object = object;

	if (object == NULL || info == NULL)
		return ;

	if (info->id)
		json_object_object_add(reply_object, "song_id",
				json_object_new_int(info->id));
	if (info->music_name)
		json_object_object_add(reply_object, "song_name",
				json_object_new_string(info->music_name));
	if (info->music_url)
		json_object_object_add(reply_object, "songurl",
				json_object_new_string(info->music_url));
	if (info->music_picture)
		json_object_object_add(reply_object, "picture",
				json_object_new_string(info->music_picture));
	if (info->albums_name)
		json_object_object_add(reply_object, "albumsname",
				json_object_new_string(info->albums_name));
	if (info->artists_name)
		json_object_object_add(reply_object, "artists_name",
				json_object_new_string(info->artists_name));
	if (info->data)
		json_object_object_add(reply_object, "data",
				json_object_new_string(info->data));
}

static void get_player_status(json_object *object)
{
	json_object *reply_object = object;

	player_status_t status = mozart_musicplayer_get_status(mozart_musicplayer_handler);
	if (status == PLAYER_PLAYING)
		json_object_object_add(reply_object, "status", json_object_new_string("play"));
	else if (status == PLAYER_PAUSED || status == PLAYER_TRANSPORT)
		json_object_object_add(reply_object, "status", json_object_new_string("pause"));
	else if (status == PLAYER_STOPPED)
		json_object_object_add(reply_object, "status", json_object_new_string("stop"));
}

char *ingenicplayer_get_song_info(char *command,char *timestamp)
{
	int index = -1;
	int length = -1;
	int duration = 0;
	enum play_mode mode = -1;
	char *reply_json = NULL;
	json_object *reply_object;
	struct music_info *info = NULL;

	reply_object = json_object_new_object();
	if (!reply_object)
		return reply_object;
	json_object_object_add(reply_object, "command", json_object_new_string(command));
	get_player_status(reply_object);
	mode = mozart_musicplayer_musiclist_get_play_mode(mozart_musicplayer_handler);
	length = mozart_musicplayer_musiclist_get_length(mozart_musicplayer_handler);
	index = mozart_musicplayer_musiclist_get_current_index(mozart_musicplayer_handler);
	info = mozart_musicplayer_musiclist_get_current_info(mozart_musicplayer_handler);
	if (info) {
		get_song_json_object(reply_object, info);
		duration = mozart_musicplayer_get_duration(mozart_musicplayer_handler);
		json_object_object_add(reply_object, "duration", json_object_new_int(duration));
	}
	json_object_object_add(reply_object, "queue_mode", json_object_new_int(mode));
	json_object_object_add(reply_object, "queue_index", json_object_new_int(index));
	json_object_object_add(reply_object, "queue_num", json_object_new_int(length));
	json_object_object_add(reply_object, "timestamp", json_object_new_string(timestamp));

	reply_json = strdup(json_object_get_string(reply_object));
	json_object_put(reply_object);

	return reply_json;
}

char *ingenicplayer_get_position(char *command,char *timestamp)
{
	int i = 0;
	int pos = 0;
	int duration = 0;
	char *reply_json = NULL;
	json_object *reply_object = json_object_new_object();
	if (!reply_object)
		return NULL;

	for (i = 0; i < 30; i++) {
		pos = mozart_musicplayer_get_pos(mozart_musicplayer_handler);
		if (pos != -1 && pos != 0)
			break;
		usleep(100000);
	}
	duration = mozart_musicplayer_get_duration(mozart_musicplayer_handler);

	json_object_object_add(reply_object, "command", json_object_new_string(command));
	json_object_object_add(reply_object, "timestamp", json_object_new_string(timestamp));
	json_object_object_add(reply_object, "get_current_position",json_object_new_int(pos));
	json_object_object_add(reply_object, "get_duration",json_object_new_int(duration));
	reply_json = strdup(json_object_get_string(reply_object));
	json_object_put(reply_object);

	return reply_json;
}

static void ingenicplayer_play_mode(struct cmd_info_c *cmd_info)
{
	enum play_mode mode;

	mode = get_object_int("mode", cmd_info->data);
	mozart_musicplayer_musiclist_set_play_mode(mozart_musicplayer_handler, mode);

	mode = mozart_musicplayer_musiclist_get_play_mode(mozart_musicplayer_handler);
	mozart_ingenicplayer_notify_play_mode(mode);

	return;
}

char *ingenicplayer_get_volume(char *command,char *timestamp)
{
	json_object *reply_object;
	char *reply_json = NULL;
	//int volume = mozart_musicplayer_get_volume(mozart_musicplayer_handler);

	//if (volume < 0)
	//	return NULL;

	reply_object = json_object_new_object();
	if (reply_object == NULL)
		return NULL;
	json_object_object_add(reply_object, "command", json_object_new_string(command));
	json_object_object_add(reply_object, "volume", json_object_new_int(music_vol));
	json_object_object_add(reply_object, "timestamp", json_object_new_string(timestamp));
	reply_json = strdup(json_object_get_string(reply_object));
	json_object_put(reply_object);

	return reply_json;
}

void *h5_musiclist_delete_index(char *data)
{
	int index = -1;
	json_object *object = NULL;

	object = json_tokener_parse(data);
	if (!object) {
		pr_err("json parse failed %s\n", strerror(errno));
		return NULL;
	}
	index = get_object_int("index", data);
	mozart_musicplayer_musiclist_delete_index(mozart_musicplayer_handler, index);
	return NULL;
}


char *h5_get_queue(char *timestamp)
{
	
	int i = 0;
	int index = -1;
	int length = -1;
	char *reply_json = NULL;
	json_object *reply_object;
	json_object *songs;
	struct music_info *info = NULL;

	reply_object = json_object_new_object();
	if (!reply_object)
		goto object_err;
	json_object_object_add(reply_object, "command", json_object_new_string("get_song_queue"));
	json_object_object_add(reply_object, "timestamp", json_object_new_string(timestamp));
	
	songs = json_object_new_array();
	if (!songs)
		goto object_err;
	index = mozart_musicplayer_musiclist_get_current_index(mozart_musicplayer_handler);
	json_object_object_add(reply_object, "index", json_object_new_int(index));
	json_object_object_add(reply_object, "songs", songs);
	length = mozart_musicplayer_musiclist_get_length(mozart_musicplayer_handler);
	//printf("h5_get_queue---------length = %d\n",length);
	for (i = 0; i < length; i++) {
		json_object *songs_object = json_object_new_object();
		if (!songs_object)
			goto object_err;

		info = mozart_musicplayer_musiclist_get_index_info(mozart_musicplayer_handler, i);
		if (info != NULL) {
			get_song_json_object(songs_object, info);
			json_object_array_add(songs, songs_object);
		}
	}
	//printf("h5_get_queue---------111\n");
	reply_json = (char *)calloc(strlen(json_object_get_string(reply_object))+ 1, sizeof(char));
	if (!reply_json) {
		pr_err("calloc %s\n", strerror(errno));
		goto object_err;
	}
	//printf("h5_get_queue---------2222\n");

	reply_json = strdup(json_object_get_string(reply_object));
	//printf("h5_get_queue---------444  reply_json = \n %s\n");

	return reply_json;
object_err:
	json_object_put(reply_object);
	return -1;

}


void ingenicplayer_get_queue(char *command, struct appserver_cli_info *owner)
{
	int i = 0;
	int index = -1;
	int length = -1;
	char *reply_json = NULL;
	json_object *reply_object;
	json_object *songs;
	struct music_info *info = NULL;

	reply_object = json_object_new_object();
	if (!reply_object)
		goto object_err;
	songs = json_object_new_array();
	if (!songs)
		goto object_err;

	index = mozart_musicplayer_musiclist_get_current_index(mozart_musicplayer_handler);
	json_object_object_add(reply_object, "index", json_object_new_int(index));
	json_object_object_add(reply_object, "songs", songs);

	length = mozart_musicplayer_musiclist_get_length(mozart_musicplayer_handler);
	for (i = 0; i < length; i++) {
		json_object *songs_object = json_object_new_object();
		if (!songs_object)
			goto object_err;

		info = mozart_musicplayer_musiclist_get_index_info(mozart_musicplayer_handler, i);
		if (info != NULL) {
			get_song_json_object(songs_object, info);
			json_object_array_add(songs, songs_object);
		}
	}
	reply_json = (char *)calloc(strlen(json_object_get_string(reply_object))
			+ 1, sizeof(char));
	if (!reply_json) {
		pr_err("calloc %s\n", strerror(errno));
		goto object_err;
	}
	strcpy(reply_json, (const char *)json_object_get_string(reply_object));
	//mozart_appserver_response(command, reply_json, owner);
	free(reply_json);

object_err:
	json_object_put(reply_object);
	return;
}

static struct music_info *create_music_info_by_json(json_object *object)
{
	int id = 0;
	char *music_name = NULL;
	char *music_url = NULL;
	char *music_picture = NULL;
	char *albums_name = NULL;
	char *artists_name = NULL;
	char *data = NULL;
	json_object *tmp = NULL;
	struct music_info *info = NULL;

	if (object == NULL)
		return NULL;

	if (json_object_object_get_ex(object, "song_id", &tmp))
		id = json_object_get_int(tmp);
	if (json_object_object_get_ex(object, "song_name", &tmp))
		music_name = strdup(json_object_get_string(tmp));
	if (json_object_object_get_ex(object, "songurl", &tmp))
		music_url = strdup(json_object_get_string(tmp));
	if (json_object_object_get_ex(object, "picture", &tmp))
		music_picture = strdup(json_object_get_string(tmp));
	if (json_object_object_get_ex(object, "albumsname", &tmp))
		albums_name = strdup(json_object_get_string(tmp));
	if (json_object_object_get_ex(object, "artists_name", &tmp))
		artists_name = strdup(json_object_get_string(tmp));
	if (json_object_object_get_ex(object, "data", &tmp))
		data = strdup(json_object_get_string(tmp));

	info = mozart_musiclist_get_info(id, music_name, music_url,
					 music_picture, albums_name, artists_name, data);

	free(music_name);
	free(music_url);
	free(music_picture);
	free(albums_name);
	free(artists_name);

	return info;
}

static int ingenicplayer_insert_song(json_object *array)
{
	int index = -1;
	struct music_info *info = NULL;

	if (array == NULL)
		return index;

	info = create_music_info_by_json(array);

	if (info)
		index = mozart_musicplayer_musiclist_insert(mozart_musicplayer_handler, info);

	return index;
}

char *ingenicplayer_add_queue(char *data)
{
	int index = -1;
	json_object *object = NULL;
	json_object *array = NULL;
	json_object *songs = NULL;

	char *command = NULL;
	char *timestamp = NULL;


	object = json_tokener_parse(data);
	if (!object) {
		pr_err("json parse failed %s\n", strerror(errno));
		return NULL;
	}


	command = get_object_string("command",NULL,data);
	timestamp = get_object_string("timestamp",NULL,data);

	if (json_object_object_get_ex(object, "songs", &songs)) {
		if (json_object_array_length(songs) > 0) {
			array = json_object_array_get_idx(songs, 0);
			index = ingenicplayer_insert_song(array);
		}
	}

	mozart_musicplayer_play_index(mozart_musicplayer_handler, index);
	json_object_put(object);

	return ingenicplayer_get_song_info(command,timestamp);
}

char *ingenicplayer_replace_queue(char *data, struct appserver_cli_info *owner)
{
	int i = 0;
	int index = -1;
	int length = -1;
	json_object *cmd_object = NULL;
	json_object *array = NULL;
	json_object *songs = NULL;
	json_object *tmp = NULL;

	cmd_object = json_tokener_parse(data);
	if (!cmd_object) {
		pr_err("json parse failed %s\n", strerror(errno));
		return NULL;
	}

	if (json_object_object_get_ex(cmd_object, "songs", &songs))
		length = json_object_array_length(songs);

	for (i = 0; i < length ; i++) {
		array = json_object_array_get_idx(songs, i);
		ingenicplayer_insert_song(array);
	}

	if (mozart_musicplayer_musiclist_get_length(mozart_musicplayer_handler) <= 0) {
		json_object_put(cmd_object);
		ingenicplayer_get_queue("delete_from_queue", owner);
		mozart_musicplayer_stop_playback(mozart_musicplayer_handler);
		return NULL;
	} else {
		if (json_object_object_get_ex(cmd_object, "index", &tmp) &&
				json_object_get_int(tmp) >= 0)
			index = json_object_get_int(tmp);
		else
			index = 0;
		mozart_musicplayer_play_index(mozart_musicplayer_handler, index);
	}

	json_object_put(cmd_object);

	return ingenicplayer_get_song_info("add_queue_to_play",NULL);
}

static void ingenicplayer_delete_queue(char *data, struct appserver_cli_info *owner)
{
	int idx = -1;
	int index = -1;
	int length = -1;

	if ((idx = get_object_int("index", data)) >= 0)
		mozart_musicplayer_musiclist_delete_index(mozart_musicplayer_handler, idx);

	ingenicplayer_get_queue("delete_from_queue", owner);

	index = mozart_musicplayer_musiclist_get_current_index(mozart_musicplayer_handler);
	length = mozart_musicplayer_musiclist_get_length(mozart_musicplayer_handler);

	if (index == idx && length > 0)
		mozart_musicplayer_play_index(mozart_musicplayer_handler, index);
	else if (length <= 0)
		mozart_musicplayer_stop_playback(mozart_musicplayer_handler);

	return;
}

char *ingenicplayer_play_queue(char *data)
{
	int index = -1;
	char *command = NULL;
	char *timestamp = NULL;

	command = get_object_string("command",NULL,data);
	timestamp = get_object_string("timestamp",NULL,data);

	if ((index = get_object_int("index", data)) >= 0)
		mozart_musicplayer_play_index(mozart_musicplayer_handler, index);

	return ingenicplayer_get_song_info(command,timestamp);
}

char *ingenicplayer_get_device_info(void)
{
	int volume;
	char *devicename = NULL;
	char *reply_json = NULL;
	json_object *reply_object = NULL;
	json_object *song_info = NULL;
	struct music_info *info = NULL;

	volume = mozart_musicplayer_get_volume(mozart_musicplayer_handler);
	if (volume < 0)
		return NULL;

	reply_object = json_object_new_object();
	if (!reply_object)
		return NULL;
	song_info = json_object_new_object();
	if (!song_info)
		goto get_device_info_err;

	json_object_object_add(reply_object, "volume", json_object_new_int(volume));

	if ((devicename = sync_devicename(NULL)) != NULL) {
		json_object_object_add(reply_object, "name",
				json_object_new_string(devicename));
		free(devicename);
	}

	json_object_object_add(reply_object, "song_info", song_info);

	get_player_status(song_info);

	info = mozart_musicplayer_musiclist_get_current_info(mozart_musicplayer_handler);
	if (info)
		get_song_json_object(song_info, info);
	reply_json = strdup(json_object_get_string(reply_object));

get_device_info_err:
	json_object_put(reply_object);
	return reply_json;
}

static char *ingenicplayer_heart_pulsate(void)
{
	char *reply_json = NULL;
	json_object *reply_object = json_object_new_object();
	if (!reply_object)
		return NULL;

	reply_json = strdup(json_object_get_string(reply_object));
	json_object_put(reply_object);

	return reply_json;
}

void ingenicplayer_set_seek(int *position)
{
	int ret = -1;
	char *reply_json = NULL;

	mozart_musicplayer_set_seek(mozart_musicplayer_handler, position);
	
	return;
}

char *ingenicplayer_get_sd_music(void)
{
	
	char *sdmusic = NULL;
	char *reply_json = NULL;
	#if 0
	json_object *songs = NULL;
	json_object *reply_object = json_object_new_object();
	if (!reply_object)
		return NULL;

	if ((sdmusic = mozart_localplayer_get_musiclist()) == NULL) {
		json_object_object_add(reply_object, "insert",
				json_object_new_int(0));
		json_object_object_add(reply_object, "songs",
				json_object_new_array());
	} else {
		json_object_object_add(reply_object, "insert",
				json_object_new_int(1));
		if ((songs = json_tokener_parse(sdmusic)) != NULL)
			json_object_object_add(reply_object, "songs", songs);
		free(sdmusic);
	}
	reply_json = strdup(json_object_get_string(reply_object));
	json_object_put(reply_object);
	#endif
	return reply_json;
}

static char *ingenicplayer_set_shortcut(char *data)
{
	FILE *fp = NULL;
	char *name = NULL;
	char shortcut_name[64] = {};

	if (!data)
		return NULL;

	name = get_object_string("name", NULL, data);
	if (name == NULL)
		return NULL;

	sprintf(shortcut_name, "%s%s", DEVICE_INGENICPLAYER_DIR, name);

	fp = fopen(shortcut_name, "w+");
	if (fp == NULL) {
		pr_err("fopen %s\n", strerror(errno));
		free(name);
		return NULL;
	}

	fwrite(data, strlen(data), sizeof(char), fp);
	fclose(fp);
	free(name);

	return NULL;
}

static char *ingenicplayer_get_shortcut(char *data)
{
	FILE *fp = NULL;
	char *name = NULL;
	char shortcut_name[64] = {};
	char *shortcut = NULL;
	int length = 0;
	json_object *object = NULL;

	if (!data)
		return NULL;

	name = get_object_string("name", NULL, data);
	if (name == NULL)
		return NULL;

	sprintf(shortcut_name, "%s%s", DEVICE_INGENICPLAYER_DIR, name);

	if ((fp = fopen(shortcut_name, "r")) != NULL) {
		fseek(fp, 0, SEEK_END);
		length = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		if (length) {
			shortcut = (char *)calloc(length, sizeof(char) + 1);
			if (shortcut) {
				fread(shortcut, sizeof(char), length, fp);
				pr_debug("get shortcut %s\n", shortcut);
			}
		}
		fclose(fp);
	}
	if (shortcut == NULL) {
		if ((object = json_tokener_parse(data)) != NULL) {
			json_object_object_add(object, "data", json_object_new_array());
			shortcut = strdup(json_object_get_string(object));
			json_object_put(object);
		}
	}
	free(name);

	return shortcut;
}

static char *ingenicplayer_play_shortcut(char *data)
{
	FILE *fp = NULL;
	char shortcut_name[64] = {};
	char *shortcut = NULL;
	char *songs = NULL;
	char *reply_json = NULL;
	int length = 0;

	if (data == NULL)
		return NULL;

	sprintf(shortcut_name, "%s%s", DEVICE_INGENICPLAYER_DIR, data);

	if ((fp = fopen(shortcut_name, "r")) != NULL) {
		fseek(fp, 0, SEEK_END);
		length = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		if (length) {
			shortcut = (char *)calloc(length, sizeof(char) + 1);
			if (shortcut) {
				fread(shortcut, sizeof(char), length, fp);
				pr_debug("get shortcut %s\n", shortcut);

				if ((songs = get_object_string("data", NULL, shortcut)) != NULL)
					reply_json = ingenicplayer_replace_queue(songs, NULL);

				free(shortcut);
			}
		}
		fclose(fp);
	}

	return reply_json;
}

int ingenicplayer_handle_command(struct cmd_info_c *cmd_info)
{
	int index = -1;
	int volume = 0;
	char *name = NULL;
	char *command = NULL;
	char *reply_json = NULL;

	for (index = 0; index < sizeof(ingenicplayer_cmd) /
			sizeof(ingenicplayer_cmd[0]); index++) {
		if (strcmp(ingenicplayer_cmd[index], cmd_info->command) == 0)
			break;
	}

	pr_debug("receive %s\n", cmd_info->command);

	switch (index) {
	case INGENICPLAYER_PLAY_PAUSE:
		ingenicplayer_play_pause();
		break;
	case INGENICPLAYER_PREV:
		mozart_musicplayer_prev_music(mozart_musicplayer_handler);
		break;
	case INGENICPLAYER_NEXT:
		mozart_musicplayer_next_music(mozart_musicplayer_handler);
		break;
	case INGENICPLAYER_PLAY_MODE:
		ingenicplayer_play_mode(cmd_info);
		break;
	case INGENICPLAYER_INC_VOLUME:
		mozart_musicplayer_volume_up(mozart_musicplayer_handler);
		break;
	case INGENICPLAYER_DEC_VOLUME:
		mozart_musicplayer_volume_down(mozart_musicplayer_handler);
		break;
	case INGENICPLAYER_SET_VOLUME:
		volume = get_object_int("volume", cmd_info->data);
		mozart_musicplayer_set_volume(mozart_musicplayer_handler, volume);
		break;
	case INGENICPLAYER_GET_VOLUME:
		reply_json = ingenicplayer_get_volume(NULL,NULL);
		command = strdup("get_volume");
		break;
	case INGENICPLAYER_GET_SONG_INFO:
		reply_json = ingenicplayer_get_song_info(NULL,NULL);
		command = strdup("get_song_info");
		break;
	case INGENICPLAYER_GET_SONG_QUEUE:
		ingenicplayer_get_queue("get_song_queue", cmd_info->owner);
		break;
	case INGENICPLAYER_ADD_QUEUE:
		mozart_module_ingenicplayer_start();
		reply_json = ingenicplayer_add_queue(cmd_info->data);
		command = strdup("get_song_info");
		break;
	case INGENICPLAYER_REPLACE_QUEUE:
		mozart_module_ingenicplayer_start();
		reply_json = ingenicplayer_replace_queue(cmd_info->data, cmd_info->owner);
		command = strdup("get_song_info");
		break;
	case INGENICPLAYER_DEL_QUEUE:
		ingenicplayer_delete_queue(cmd_info->data, cmd_info->owner);
		break;
	case INGENICPLAYER_PLAY_QUEUE:
		reply_json = ingenicplayer_play_queue(cmd_info->data);
		command = strdup("get_song_info");
		break;
	case INGENICPLAYER_GET_DEVICE_INFO:
		reply_json = ingenicplayer_get_device_info();
		command = strdup("get_device_info");
		break;
	case INGENICPLAYER_HEART_PULSATE:
		reply_json = ingenicplayer_heart_pulsate();
		command = strdup("heart_pulsate");
		break;
	case INGENICPLAYER_SET_SEEK:
		//ingenicplayer_set_seek(cmd_info->data);
		break;
	case INGENICPLAYER_GET_POSITION:
		reply_json = ingenicplayer_get_position(NULL,NULL);
		command = strdup("get_position");
		break;
	case INGENICPLAYER_GET_SD_MUSIC:
		reply_json = ingenicplayer_get_sd_music();
		command = strdup("get_sd_music");
		break;
	case INGENICPLAYER_SET_SHORTCUT:
		reply_json = ingenicplayer_set_shortcut(cmd_info->data);
		command = strdup("set_shortcut");
		break;
	case INGENICPLAYER_GET_SHORTCUT:
		reply_json = ingenicplayer_get_shortcut(cmd_info->data);
		command = strdup("get_shortcut");
		break;
	case INGENICPLAYER_PLAY_SHORTCUT:
		mozart_module_ingenicplayer_start();
		name = get_object_string("name", NULL, cmd_info->data);
		if (name) {
			mozart_ingenicplayer_notify_play_shortcut(name);
			free(name);
		}
		break;
	default:
		pr_err("Unknow ingenic player cmd %d %s\n", index, cmd_info->command);
		break;
	}

	//if (reply_json && command)
		//mozart_appserver_response(command, reply_json, cmd_info->owner);

	free(reply_json);
	free(command);

	free_cmd_list(cmd_info);

	return index;
}

int ray_appserver_notify(char *data)
{
	char *cmd = NULL;
	cmd = (char *)calloc(strlen(data) + 128,sizeof(char));
	if(!cmd){
		printf("EEROR :cmd calloc \n");
		return -1;
	}
	sprintf(cmd,"mosquitto_pub -h 119.23.207.229 -p 1883 -u leden -P ray123456 -t /app/%s -m '%s'","RSPythonTest",data);
	//printf("SYSCMD = %s\n",cmd);
	system(cmd);
	free(cmd);


	return 0;
}


int mozart_ingenicplayer_notify_play_shortcut(char *shortcut_name)
{
	char *reply_json = NULL;

	if ((reply_json = ingenicplayer_play_shortcut(shortcut_name)) == NULL)
		return -1;

	//mozart_appserver_notify("get_song_info", reply_json);
	free(reply_json);

	return 0;
}

int mozart_ingenicplayer_notify_volume(int volume)
{
	json_object *reply_object = NULL;

	if ((reply_object = json_object_new_object()) == NULL)
		return -1;

	json_object_object_add(reply_object, "volume", json_object_new_int(volume));
	//mozart_appserver_notify("set_volume", (char *)json_object_get_string(reply_object));
	json_object_put(reply_object);

	return 0;
}

int mozart_ingenicplayer_notify_play_mode(enum play_mode mode)
{
	json_object *reply_object = NULL;

	if ((reply_object = json_object_new_object()) == NULL)
		return -1;

	json_object_object_add(reply_object, "mode", json_object_new_int(mode));
	//mozart_appserver_notify("set_queue_mode", (char *)json_object_get_string(reply_object));
	json_object_put(reply_object);

	return 0;
}

int mozart_ingenicplayer_notify_pos(void)
{
	char *reply_json = NULL;

	reply_json = ingenicplayer_get_position(NULL,NULL);
	if (reply_json) {
		//mozart_appserver_notify("get_position", reply_json);
		free(reply_json);
	}

	return 0;
}

int mozart_ingenicplayer_notify_song_info(void)
{
	struct tm  *tp;
	char cur_time[16] = {0};
 	time_t timep ;
 	timep = time(NULL);
	sprintf(cur_time,"%ld",timep);

	char *reply_json = NULL;

	reply_json = ingenicplayer_get_song_info("get_song_info","-1");
	if (reply_json) {
		ray_appserver_notify( reply_json);
		free(reply_json);
	}

	return 0;
}

int mozart_ingenicplayer_response_cmd(char *command, char *data, struct appserver_cli_info *owner)
{
	struct cmd_info_c *cmd_info = NULL;

	cmd_info = (struct cmd_info_c *)calloc(sizeof(struct cmd_info_c), sizeof(char));

	cmd_info->owner = (struct appserver_cli_info *)calloc(
			sizeof(struct appserver_cli_info), sizeof(char));
	if (!cmd_info->owner) {
		pr_err("calloc %s\n", strerror(errno));
		free(cmd_info);
		return 0;
	}
	memcpy(cmd_info->owner, owner, sizeof(struct appserver_cli_info));
	cmd_info->command = strdup(command);
	cmd_info->data = strdup(data);

	pthread_mutex_lock(&cmd_mutex);
	list_insert_at_tail(&cmd_list, (struct cmd_info_c *)cmd_info);
	pthread_cond_signal(&cond_var);
	pthread_mutex_unlock(&cmd_mutex);

	return 0;
}

static void *ingenicplayer_func(void *arg)
{
	char devicename[64] = {};
	char macaddr[] = "255.255.255.255";
	struct stat devicename_stat;

	system("mkdir -p /usr/data/ingenicplayer");

	stat(DEVICENAME, &devicename_stat);
	if (devicename_stat.st_size == 0) {
		memset(macaddr, 0, sizeof(macaddr));
		get_mac_addr("wlan0", macaddr, "");
		sprintf(devicename, "SmartAudio-%s", macaddr + 4);
		pr_debug("devicename %s\n", devicename);
		sync_devicename(devicename);
	}

	pthread_mutex_lock(&cmd_mutex);
	while (die_out) {
		if (!is_empty(&cmd_list)) {
			struct cmd_info_c *cmd_info;
			cmd_info = list_delete_at_index(&cmd_list, 0);

			if (cmd_info) {
				pthread_mutex_unlock(&cmd_mutex);
				ingenicplayer_handle_command(cmd_info);
				pthread_mutex_lock(&cmd_mutex);
			}
		} else {
			pthread_cond_wait(&cond_var, &cmd_mutex);
		}
	}

	list_destroy(&cmd_list, free_cmd_list);
	pthread_mutex_unlock(&cmd_mutex);

#ifdef SUPPORT_SONG_SUPPLYER
	if (snd_source == SND_SRC_CLOUD)
		mozart_musicplayer_stop(mozart_musicplayer_handler);
#endif

	return NULL;
}

int mozart_ingenicplayer_startup(void)
{
	pthread_mutex_lock(&cmd_mutex);
	if (die_out) {
		pthread_mutex_unlock(&cmd_mutex);
		return 0;
	}

	die_out = true;
	list_init(&cmd_list);

	if (pthread_create(&ingenicplayer_thread, NULL, ingenicplayer_func, NULL)) {
		pr_err("create ingenicplayer_thread failed\n");
		return -1;
	}
	pthread_detach(ingenicplayer_thread);

	pthread_mutex_unlock(&cmd_mutex);

	return 0;
}

int mozart_ingenicplayer_shutdown(void)
{
	pthread_mutex_lock(&cmd_mutex);
	if (die_out) {
		die_out = false;
		pthread_cond_signal(&cond_var);
	}
	pthread_mutex_unlock(&cmd_mutex);

	return 0;
}
