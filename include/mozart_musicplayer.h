#ifndef __MOZART_MUSICPLAYER_H__
#define __MOZART_MUSICPLAYER_H__

#include <stdbool.h>
//#include "musiclist_interface.h"
#include "player_interface.h"

enum play_mode {
	play_mode_order = 0,
	play_mode_single,
	play_mode_random,
};
/*
typedef enum {
	PLAYER_UNKNOWN,
	PLAYER_TRANSPORT,
	PLAYER_PLAYING,
	PLAYER_PAUSED,
	PLAYER_STOPPED,
	AOSWITCH_FAIL,
} player_status_t;

typedef struct {
	char uuid[32];
	int ch_num;
	player_status_t status;
	int st_code;
	int position;
	int duration;
	bool change_by_seek;
} player_snapshot_t;


typedef int (*player_callback)(player_snapshot_t *snapshot, struct player_handler *handler, void *param);

typedef struct player_handler {
	char uuid[32];
	int ch_num;
	int fd_sync;
	int fd_async;
	player_callback callback;
	pthread_t handler_thread;
	bool _service_stop;
	void *param;
} player_handler_t;




*/

struct music_info {
	int id;
	char *music_name;
	char *music_url;
	char *music_picture;
	char *albums_name;
	char *artists_name;
	void *data;
};


typedef struct mozart_musicplayer_handler {
	player_handler_t *player_handler;
} musicplayer_handler_t;



extern musicplayer_handler_t *mozart_musicplayer_handler;

extern bool mozart_musicplayer_is_active(musicplayer_handler_t *handler);
extern int mozart_musicplayer_musiclist_insert(musicplayer_handler_t *handler,
					       struct music_info *info);
extern int mozart_musicplayer_musiclist_delete_index(musicplayer_handler_t *handler, int index);
extern enum play_mode mozart_musicplayer_musiclist_get_play_mode(musicplayer_handler_t *handler);
extern int mozart_musicplayer_musiclist_set_play_mode(musicplayer_handler_t *handler,
						      enum play_mode mode);
extern int mozart_musicplayer_musiclist_get_length(musicplayer_handler_t *handler);
extern int mozart_musicplayer_musiclist_get_current_index(musicplayer_handler_t *handler);
extern struct music_info *mozart_musicplayer_musiclist_get_current_info(
	musicplayer_handler_t *handler);
extern struct music_info *mozart_musicplayer_musiclist_get_index_info(
	musicplayer_handler_t *handler, int index);
extern int mozart_musicplayer_musiclist_clean(musicplayer_handler_t *handler);
extern int mozart_musicplayer_play_shortcut(musicplayer_handler_t *handler, int index);
extern int mozart_musicplayer_play_index(musicplayer_handler_t *handler, int index);
extern int mozart_musicplayer_prev_music(musicplayer_handler_t *handler);
extern int mozart_musicplayer_next_music_false(musicplayer_handler_t *handler);
extern int mozart_musicplayer_next_music(musicplayer_handler_t *handler);
extern int mozart_musicplayer_get_duration(musicplayer_handler_t *handler);
extern int mozart_musicplayer_get_pos(musicplayer_handler_t *handler);
extern int mozart_musicplayer_set_seek(musicplayer_handler_t *handler, int seek);
extern player_status_t mozart_musicplayer_get_status(musicplayer_handler_t *handler);
extern int mozart_musicplayer_get_volume(musicplayer_handler_t *handler);
extern int mozart_musicplayer_set_volume(musicplayer_handler_t *handler, int volume);
extern int mozart_musicplayer_volume_up(musicplayer_handler_t *handler);
extern int mozart_musicplayer_volume_down(musicplayer_handler_t *handler);
extern int mozart_musicplayer_play_pause(musicplayer_handler_t *handler);
extern int mozart_musicplayer_stop_playback(musicplayer_handler_t *handler);
extern int mozart_musicplayer_play_music_info(musicplayer_handler_t *handler,
					      struct music_info *info);
extern int mozart_musicplayer_start(musicplayer_handler_t *handler);
extern int mozart_musicplayer_stop(musicplayer_handler_t *handler);
extern int mozart_musicplayer_startup(void);
extern int mozart_musicplayer_shutdown(void);

#endif	/* __MOZART_MUSICPLAYER_H__ */

