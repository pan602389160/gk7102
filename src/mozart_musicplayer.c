#include <stdio.h>
#include <string.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <dirent.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/input.h>
#include <sys/time.h>
#include <sys/select.h>
#include <errno.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include <stdbool.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "sharememory_interface.h"
#include "event_interface.h"
#include "utils_interface.h"

//#include "mozart_config.h"
//#include "ingenicplayer.h"
#include "mozart_musicplayer.h"
#include "debug_print.h"
#define DEBUG(format, ...) printf("[%s : %s : %d] ",__FILE__,__func__,__LINE__); printf(format, ##__VA_ARGS__);

#define ERROR(x,y...)	(printf("ERROR [ %s : %s : %d] "x"\n", __FILE__, __func__, __LINE__, ##y))

#define pr_debug(fmt, args...)			\
	do {} while (0)

#define pr_info(fmt, args...)						\
	printf("[MUSICPLAYER] [INFO] "fmt, ##args)

#define pr_err(fmt, args...)						\
	fprintf(stderr, "[MUSICPLAYER] [ERROR] {%s, %d}: "fmt, __func__, __LINE__, ##args)

#define musicplayer_lock(lock)						\
	do {                                                            \
		int i = 0;                                              \
									\
		while (pthread_mutex_trylock(lock)) {                   \
			if (i++ >= 100) {                               \
				pr_err("#### {%s, %s, %d} dead lock (last: %d)####\n", \
				       __FILE__, __func__, __LINE__, last_lock_line); \
				i = 0;                                  \
			}                                               \
			usleep(100 * 1000);                             \
		}                                                       \
		last_lock_line = __LINE__;				\
	} while (0)

#define musicplayer_unlock(lock)					\
	do {								\
		pthread_mutex_unlock(lock);				\
	} while (0)

#define VOLUME_VARIATION 10

typedef enum {
	NO_STORAGE = 0,
	SDCARD,
	UDISK,
} storage_type;

typedef struct music_item_s {
	char *musicUrl;
	storage_type storage_type;
	struct music_item_s *next;
	struct music_item_s *prev;
} music_item;

typedef struct dir_item_s {
	char dir_name[1024];
	struct dir_item_s *next;
} dir_item;
typedef music_item *music_list;
typedef dir_item *dir_list;


musicplayer_handler_t *mozart_musicplayer_handler;

static bool autoplay_flag;
static int in_musicplayer;
static int last_lock_line;
static player_handler_t current_player_handler;
static player_status_t musicplayer_status;
static struct music_list *musicplayer_list;
static pthread_mutex_t musicplayer_mutex = PTHREAD_MUTEX_INITIALIZER;

static mozart_event mozart_musicplayer_event = {
	.event = {
		.misc = {
			.name = "musicplayer",
		},
	},
	.type = EVENT_MISC,
};



static int check_handler_uuid(musicplayer_handler_t *handler)
{
	if (handler == NULL || handler->player_handler == NULL) {
		pr_err("handler is null\n");
		return -1;
	}

	if (strcmp(current_player_handler.uuid, handler->player_handler->uuid)) {
		DEBUG("%s can't control, controler is %s\n",
			handler->player_handler->uuid,current_player_handler.uuid);
		return -1;
	}

	return 0;
}
bool mozart_musicplayer_is_active(musicplayer_handler_t *handler)
{
	int ret;

	musicplayer_lock(&musicplayer_mutex);
	ret = check_handler_uuid(handler);
	musicplayer_unlock(&musicplayer_mutex);

	return ret ? false : true;
}

static void free_data(void *arg)
{
	return;
}

int mozart_musicplayer_musiclist_insert(musicplayer_handler_t *handler, struct music_info *info)
{
	int ret;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		pr_err("check handler uuid failed\n");
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	ret = mozart_musiclist_insert(musicplayer_list, info);
	//printf("mozart_musicplayer_musiclist_insert ret = %d,musicplayer_list = %d\n",ret,mozart_musiclist_get_length(musicplayer_list));
	musicplayer_unlock(&musicplayer_mutex);

	/* TODO-tjiang: notify song queue ? */

	return ret;
}


int mozart_musicplayer_musiclist_delete_index(musicplayer_handler_t *handler, int index)
{
	int ret;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		pr_err("check handler uuid failed\n");
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	ret = mozart_musiclist_delete_index(musicplayer_list, index);
	musicplayer_unlock(&musicplayer_mutex);

	/* TODO-tjiang: notify song queue ? */

	return ret;
}


enum play_mode mozart_musicplayer_musiclist_get_play_mode(musicplayer_handler_t *handler)
{
	enum play_mode mode;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		pr_debug("check handler uuid failed\n");
		musicplayer_unlock(&musicplayer_mutex);
		return play_mode_order;
	}

	mode = mozart_musiclist_get_play_mode(musicplayer_list);
	musicplayer_unlock(&musicplayer_mutex);

	return mode;
}

int mozart_musicplayer_musiclist_set_play_mode(musicplayer_handler_t *handler, enum play_mode mode)
{
	int ret;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		pr_err("check handler uuid failed\n");
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	ret = mozart_musiclist_set_play_mode(musicplayer_list, mode);
	musicplayer_unlock(&musicplayer_mutex);

	//mozart_ingenicplayer_notify_play_mode(mode);

	return ret;
}

int mozart_musicplayer_musiclist_get_length(musicplayer_handler_t *handler)
{
	int ret;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		pr_debug("check handler uuid failed\n");
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	ret = mozart_musiclist_get_length(musicplayer_list);
	musicplayer_unlock(&musicplayer_mutex);

	return ret;
}

struct music_info *mozart_musicplayer_musiclist_get_current_info(musicplayer_handler_t *handler)
{
	struct music_info *info;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		pr_debug("check handler uuid failed\n");
		musicplayer_unlock(&musicplayer_mutex);
		return NULL;
	}

	info = mozart_musiclist_get_current(musicplayer_list);
	musicplayer_unlock(&musicplayer_mutex);

	return info;
}


int mozart_musicplayer_musiclist_get_current_index(musicplayer_handler_t *handler)
{
	int ret;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		pr_debug("check handler uuid failed\n");
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	ret = mozart_musiclist_get_current_index(musicplayer_list);
	musicplayer_unlock(&musicplayer_mutex);

	return ret;
}


struct music_info *mozart_musicplayer_musiclist_get_index_info(musicplayer_handler_t *handler,
							       int index)
{
	struct music_info *info;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		pr_debug("check handler uuid failed\n");
		musicplayer_unlock(&musicplayer_mutex);
		return NULL;
	}

	info = mozart_musiclist_get_index(musicplayer_list, index);
	musicplayer_unlock(&musicplayer_mutex);

	return info;
}


int mozart_musicplayer_musiclist_clean(musicplayer_handler_t *handler)
{
	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		pr_err("check handler uuid failed\n");
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	mozart_musiclist_clean(musicplayer_list);
	musicplayer_unlock(&musicplayer_mutex);

	return 0;
}

int mozart_musicplayer_play_shortcut(musicplayer_handler_t *handler, int index)
{
	char shortcut_name[20] = {};

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		pr_err("check handler uuid failed\n");
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	//sprintf(shortcut_name, "shortcut_%d", index);
	//mozart_ingenicplayer_notify_play_shortcut(shortcut_name);
	musicplayer_unlock(&musicplayer_mutex);

	return 0;
}

static int __mozart_musicplayer_play_music_info(musicplayer_handler_t *handler,
		struct music_info *info)
{
	printf("================__mozart_musicplayer_play_music_info===============\n");
	int i, ret;
	//in_musicplayer = 0; //调试
	if(in_musicplayer) {
		blue_pr(" in_musicplayer = %d\n",in_musicplayer);
		blue_pr(" info->music_url = %s\n",info->music_url);
		blue_pr(" info->music_name = %s\n",info->music_name);
		blue_pr(" info->artists_name = %s\n",info->artists_name);
		if (mozart_player_playurl(handler->player_handler, info->music_url)) {
			pr_err("player play url failed\n");
			return -1;
		}
	} else {
		printf("in_musicplayer ????? 0000000\n");
		__attribute__((unused)) struct music_info *dump;
		module_status domain_status;

		share_mem_set(MUSICPLAYER_DOMAIN, WAIT_RESPONSE);
		snprintf(mozart_musicplayer_event.event.misc.type,
				sizeof(mozart_musicplayer_event.event.misc.type), "play");
		if (mozart_event_send(mozart_musicplayer_event))
			pr_err("send musicplayer play event failure\n");

		/* wait RENDER_DOMAIN become to RESPONSE_DONE status for 10 seconds. */
		for (i = 0; i < 1000; i++) {
			ret = share_mem_get(MUSICPLAYER_DOMAIN, &domain_status);
			printf("domain_status ... : %s.\n", module_status_str[domain_status]);
			if (!ret)
				if (domain_status == RESPONSE_DONE ||
						domain_status == RESPONSE_PAUSE ||
						domain_status == RESPONSE_CANCEL)
					break;

			usleep(10 * 1000);
		}

		if (i == 1000) {
			pr_err("Wait musicplayer play event reponse done timeout, will not play music\n");
			return -1;
		}

		blue_pr("==========================================mozart_musiclist_get_length :%d\n",mozart_musiclist_get_length(musicplayer_list));
		blue_pr("url: %s, len: %d\n", info->music_url,mozart_musiclist_get_length(musicplayer_list));
		for (i = 0; i < mozart_musiclist_get_length(musicplayer_list); i++) 
		{
			dump = mozart_musiclist_get_index(musicplayer_list, i);
			blue_pr("[%d]: %s, %s\n", i, dump->music_name, dump->music_url);
		}
		blue_pr("========================================================================\n");
		
		switch (domain_status) {
			case RESPONSE_DONE:
				if (mozart_player_playurl(handler->player_handler, info->music_url)) {
					pr_err("player play url failed\n");
					return -1;  
				}
				break;
			case RESPONSE_PAUSE:
				if (mozart_player_cacheurl(handler->player_handler, info->music_url)) {
					pr_err("player play url failed\n");
					return -1; 
				}
				break;
			case RESPONSE_CANCEL:
			default:
				pr_err("musicplayer response cancel\n");
				return -1;
		}
		in_musicplayer = 1;
	}
	
	autoplay_flag = true;
	snprintf(mozart_musicplayer_event.event.misc.type, 16, "playing");
	if (mozart_event_send(mozart_musicplayer_event) < 0)
		pr_err("%s: Send event fail\n", __func__);

	return 0;
}

int mozart_musicplayer_play_index(musicplayer_handler_t *handler, int index)
{
	printf("======================mozart_musicplayer_play_index=======================\n");
	int ret;
	struct music_info *info = NULL;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		pr_err("check handler uuid failed\n");
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	info = mozart_musiclist_set_index(musicplayer_list, index);
	if (info == NULL) {
		pr_err("mozart_musiclist_set_index failed\n");
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	ret = __mozart_musicplayer_play_music_info(handler, info);
	musicplayer_unlock(&musicplayer_mutex);

	return ret;
}


int mozart_musicplayer_prev_music(musicplayer_handler_t *handler)
{
	int ret;
	struct music_info *info = NULL;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		pr_err("check handler uuid failed\n");
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	info = mozart_musiclist_set_prev(musicplayer_list);
	if (info == NULL) {
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	ret = __mozart_musicplayer_play_music_info(handler, info);
	musicplayer_unlock(&musicplayer_mutex);

	return ret;
}

static int __mozart_musicplayer_next_music(musicplayer_handler_t *handler, bool force)
{
	struct music_info *info = NULL;

	info = mozart_musiclist_set_next(musicplayer_list, force);
	if (info == NULL)
		return -1;

	return __mozart_musicplayer_play_music_info(handler, info);
}

/* for stop */
int mozart_musicplayer_next_music_false(musicplayer_handler_t *handler)
{
	int ret;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		pr_err("check handler uuid failed\n");
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	ret = __mozart_musicplayer_next_music(handler, false);
	musicplayer_unlock(&musicplayer_mutex);

	return ret;
}


int mozart_musicplayer_next_music(musicplayer_handler_t *handler)
{
	int ret;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		pr_err("check handler uuid failed\n");
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	ret = __mozart_musicplayer_next_music(handler, true);
	musicplayer_unlock(&musicplayer_mutex);

	return ret;
}


/**********************************************************************************************
 * music player handler
 *********************************************************************************************/
int mozart_musicplayer_get_duration(musicplayer_handler_t *handler)
{
	int ret;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		musicplayer_unlock(&musicplayer_mutex);
		return PLAYER_PAUSED;
	}

	ret = mozart_player_getduration(handler->player_handler);
	musicplayer_unlock(&musicplayer_mutex);

	return ret;
}

static int __mozart_musicplayer_get_pos(musicplayer_handler_t *handler)
{
	return mozart_player_getpos(handler->player_handler);
}

int mozart_musicplayer_get_pos(musicplayer_handler_t *handler)
{
	int ret;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		musicplayer_unlock(&musicplayer_mutex);
		return PLAYER_PAUSED;
	}

	ret = __mozart_musicplayer_get_pos(handler);
	musicplayer_unlock(&musicplayer_mutex);

	return ret;
}

int mozart_musicplayer_set_seek(musicplayer_handler_t *handler, int seek)
{
	int ret;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		musicplayer_unlock(&musicplayer_mutex);
		return PLAYER_PAUSED;
	}

	ret = mozart_player_seek(handler->player_handler, seek);
	musicplayer_unlock(&musicplayer_mutex);

	//if (ret == 0)
		//mozart_ingenicplayer_notify_pos();

	return ret;
}

player_status_t mozart_musicplayer_get_status(musicplayer_handler_t *handler)
{	//printf("==================mozart_musicplayer_get_status===============\n");
	player_status_t status;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		musicplayer_unlock(&musicplayer_mutex);
		return PLAYER_PAUSED;
	}

	status = mozart_player_getstatus(handler->player_handler);
	musicplayer_unlock(&musicplayer_mutex);

	return status;
}

static int __mozart_musicplayer_get_volume(musicplayer_handler_t *handler)
{
	return mozart_player_volget(handler->player_handler);
}

int mozart_musicplayer_get_volume(musicplayer_handler_t *handler)
{
	int ret;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		musicplayer_unlock(&musicplayer_mutex);
		return 0;
	}

	ret = __mozart_musicplayer_get_volume(handler);
	musicplayer_unlock(&musicplayer_mutex);

	return ret;
}

static int __mozart_musicplayer_set_volume(musicplayer_handler_t *handler, int volume)
{
	int ret;

	ret = mozart_player_volset(handler->player_handler, volume);

	//if (ret == 0)
		//mozart_ingenicplayer_notify_volume(__mozart_musicplayer_get_volume(handler));

	return ret;
}

int mozart_musicplayer_set_volume(musicplayer_handler_t *handler, int volume)
{
	int ret;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}
	purple_pr("------------mozart_musicplayer_set_volume : %d\n",volume);
	ret = __mozart_musicplayer_set_volume(handler, volume);
	musicplayer_unlock(&musicplayer_mutex);

	return ret;
}

int mozart_musicplayer_volume_up(musicplayer_handler_t *handler)
{
	int ret, volume;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	volume = __mozart_musicplayer_get_volume(handler) + VOLUME_VARIATION;
	if (volume > 100)
		volume = 100;
	else if (volume < 0)
		volume = 0;
	ret = __mozart_musicplayer_set_volume(handler, volume);
	musicplayer_unlock(&musicplayer_mutex);

	return ret;
}


int mozart_musicplayer_volume_down(musicplayer_handler_t *handler)
{
	int ret, volume;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	volume = __mozart_musicplayer_get_volume(handler) - VOLUME_VARIATION;
	if (volume > 100)
		volume = 100;
	else if (volume < 0)
		volume = 0;
	ret = __mozart_musicplayer_set_volume(handler, volume);
	musicplayer_unlock(&musicplayer_mutex);

	return ret;
}


int mozart_musicplayer_play_pause(musicplayer_handler_t *handler)
{
	//printf("=======#====mozart_musicplayer_play_pause================\n");
	int ret = -1;
	player_status_t status;

	if(!handler)
		return -1;
	status = mozart_player_getstatus(handler->player_handler);
	printf(" ??? status = %d\n",status);


	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		printf("check_handler_uuid failed\n");
		musicplayer_unlock(&musicplayer_mutex);
		return PLAYER_PAUSED;
	}

	if (status == PLAYER_PLAYING) {
		ret = mozart_player_pause(handler->player_handler);
		snprintf(mozart_musicplayer_event.event.misc.type, 16, "pause");
	} else if (status == PLAYER_PAUSED) {
		ret = mozart_player_resume(handler->player_handler);
		snprintf(mozart_musicplayer_event.event.misc.type, 16, "resume");
	}

	if (mozart_event_send(mozart_musicplayer_event) < 0)
		pr_err("%s: Send event fail\n", __func__);

	musicplayer_unlock(&musicplayer_mutex);

	return ret;
}


int mozart_musicplayer_stop_playback(musicplayer_handler_t *handler)
{
	printf("===========%s=============\n",__func__);
	autoplay_flag = false;
	if (mozart_player_stop(handler->player_handler)) {
		pr_err("send stop cmd failed\n");
		return -1;
	}

	if (AUDIO_OSS == get_audio_type()) {
		int cnt = 500;
		while (cnt--) {
			if (musicplayer_status == PLAYER_STOPPED)
				break;
			usleep(10 * 1000);
		}
		if (cnt < 0) {
			pr_err("waiting for stopped status timeout.\n");
			return -1;
		}
	}
	if (share_mem_set(MUSICPLAYER_DOMAIN, STATUS_RUNNING))
		pr_err("share_mem_set failure.\n");

	return 0;
}


int mozart_musicplayer_play_music_info(musicplayer_handler_t *handler, struct music_info *info)
{
	int ret;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		pr_err("check handler uuid failed\n");
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	ret = __mozart_musicplayer_play_music_info(handler, info);
	musicplayer_unlock(&musicplayer_mutex);

	return ret;
}

static int musicplayer_status_change_callback(player_snapshot_t *snapshot,
					      struct player_handler *handler, void *param)
{
	printf("===========%s==========status:%s\n",__func__,player_status_str[snapshot->status]);
	musicplayer_handler_t *musicplayer_handler = (musicplayer_handler_t *)param;

	if (strcmp(handler->uuid, snapshot->uuid)) {
		pr_debug("%s can't control, controler is %s\n", handler->uuid, snapshot->uuid);
		if (share_mem_set(MUSICPLAYER_DOMAIN, STATUS_RUNNING))
			pr_err("share_mem_set failure.\n");
		musicplayer_status = PLAYER_STOPPED;
		return 0;
	}

	musicplayer_status = snapshot->status;
	#if 1
	pr_debug("mozart musicplayer recv player status: %d, %s.\n",
		 snapshot->status, player_status_str[snapshot->status]);

	switch (snapshot->status) {
	case PLAYER_TRANSPORT:
	case PLAYER_PLAYING:
		if (share_mem_set(MUSICPLAYER_DOMAIN, STATUS_PLAYING))
			pr_err("share_mem_set failure.\n");
		mozart_ingenicplayer_notify_song_info();
		break;
	case PLAYER_PAUSED:
		if (share_mem_set(MUSICPLAYER_DOMAIN, STATUS_PAUSE))
			pr_err("share_mem_set failure.\n");
		mozart_ingenicplayer_notify_song_info();
		break;
	case PLAYER_STOPPED:
		if (share_mem_set(MUSICPLAYER_DOMAIN, STATUS_RUNNING))
			pr_err("share_mem_set failure.\n");
		if (autoplay_flag)
			mozart_musicplayer_next_music_false(musicplayer_handler);
		else{
			//mozart_ingenicplayer_notify_song_info();
			pr_err("musicplayer_status_change_callback ????????\n");
		}
		break;
	case PLAYER_UNKNOWN:
		musicplayer_status = PLAYER_STOPPED;
		if (share_mem_set(MUSICPLAYER_DOMAIN, STATUS_RUNNING))
			pr_err("share_mem_set failure.\n");
		break;
	default:
		musicplayer_status = PLAYER_STOPPED;
		pr_err("Unsupported status: %d.\n", snapshot->status);
		break;
	}
	#endif
	return 0;
}


int mozart_musicplayer_start(musicplayer_handler_t *handler)
{
	if (handler == NULL || handler->player_handler == NULL) {
		pr_err("handler is null\n");
		return -1;
	}

	musicplayer_lock(&musicplayer_mutex);

	if (strcmp(current_player_handler.uuid, "invalid")) { 
		 //uuuid 是有效的，失败
		pr_err("current handler isn't invalid\n");
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}
	
	//uuuid 是invalid
	memcpy(current_player_handler.uuid, handler->player_handler->uuid,
	       sizeof(current_player_handler.uuid));
	printf("mozart_musicplayer_start current is %s\n", current_player_handler.uuid);

	musicplayer_unlock(&musicplayer_mutex);

	return 0;
}


int mozart_musicplayer_stop(musicplayer_handler_t *handler)
{
	musicplayer_lock(&musicplayer_mutex);
	in_musicplayer = 0;
	if (!strcmp(current_player_handler.uuid, "invalid")) {
		musicplayer_unlock(&musicplayer_mutex);
		return 0;
	}
	if (strcmp(current_player_handler.uuid, handler->player_handler->uuid)) {
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	pr_debug("stop %s\n", current_player_handler.uuid);
	if (mozart_player_getstatus(handler->player_handler) != PLAYER_STOPPED)
		mozart_musicplayer_stop_playback(handler);
	mozart_musiclist_clean(musicplayer_list);
	strcpy(current_player_handler.uuid, "invalid");
	musicplayer_unlock(&musicplayer_mutex);

	return 0;
}


int mozart_musicplayer_startup(void)
{
	printf("=============mozart_musicplayer_startup===================\n");

	musicplayer_lock(&musicplayer_mutex);
	if (mozart_musicplayer_handler) {
		pr_info("musicplayer is running\n");
		musicplayer_unlock(&musicplayer_mutex);
		return 0;
	}

	musicplayer_list = mozart_musiclist_create(free_data);
	if(!musicplayer_list)
		pr_err("mozart_musiclist_create\n");
	
	if (share_mem_init() != 0)
		pr_err("share_mem_init failed\n");

	if (share_mem_set(MUSICPLAYER_DOMAIN, STATUS_RUNNING))
		pr_err("share_mem_set MUSICPLAYER_DOMAIN failed\n");

	mozart_musicplayer_handler = calloc(sizeof(*mozart_musicplayer_handler), 1);
	if (mozart_musicplayer_handler == NULL) {
		pr_err("calloc musicplayer handler failed\n");
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}
	mozart_musicplayer_handler->player_handler = mozart_player_handler_get(
		"musicplayer", musicplayer_status_change_callback, mozart_musicplayer_handler);
	
	if (mozart_musicplayer_handler->player_handler == NULL) {
		pr_err("get player handler failed\n");
		free(mozart_musicplayer_handler);
		mozart_musicplayer_handler = NULL;
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	strcpy(current_player_handler.uuid, "invalid");
	musicplayer_unlock(&musicplayer_mutex);

	return 0;
}


int mozart_musicplayer_shutdown(void)
{
	printf("########################%s###############################\n",__func__);

	musicplayer_lock(&musicplayer_mutex);
	if (mozart_musicplayer_handler == NULL) {
		pr_info("ingenic player not running\n");
		musicplayer_unlock(&musicplayer_mutex);
		return 0;
	}

	if (check_handler_uuid(mozart_musicplayer_handler) == 0 &&
	    mozart_player_getstatus(mozart_musicplayer_handler->player_handler) != PLAYER_STOPPED)
		mozart_musicplayer_stop_playback(mozart_musicplayer_handler);
	strcpy(current_player_handler.uuid, "invalid");

	mozart_player_handler_put(mozart_musicplayer_handler->player_handler);
	memset(mozart_musicplayer_handler, 0, sizeof(*mozart_musicplayer_handler));
	free(mozart_musicplayer_handler);
	mozart_musicplayer_handler = NULL;

	mozart_musiclist_destory(musicplayer_list);

	if (share_mem_set(MUSICPLAYER_DOMAIN, STATUS_SHUTDOWN))
		pr_err("share_mem_set failed\n");

	musicplayer_unlock(&musicplayer_mutex);

	return 0;
}

