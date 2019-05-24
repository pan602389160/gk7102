#ifndef __INGENICPLAYER_H__
#define __INGENICPLAYER_H__

#include "appserver.h"
#include "mozart_musicplayer.h"

//#include "musiclist_interface.h"

char *ingenicplayer_get_device_info(void);

char *ingenicplayer_get_sd_music(void);

char *ingenicplayer_get_song_info(char *command,char *cur_time);

char *ingenicplayer_get_position(char *command,char *cur_time);

void ingenicplayer_get_queue(char *command, struct appserver_cli_info *owner);

void ingenicplayer_set_seek(int  *position);


char *ingenicplayer_add_queue(char *data);

char *ingenicplayer_replace_queue(char *data, struct appserver_cli_info *owner);

char *ingenicplayer_play_queue(char *data);


char *h5_get_queue(char *timestamp);

int h5_play_mode(char *data);

void *h5_insert_voice(char *url);

void *h5_play_msg(void);

void *h5_play_keep_msg(void);

void *check_msg(void);

int h5_post_msg(void);


#if (SUPPORT_INGENICPLAYER == 1)
extern int mozart_ingenicplayer_notify_play_shortcut(char *shortcut_name);
extern int mozart_ingenicplayer_notify_pos(void);
extern int mozart_ingenicplayer_notify_volume(int volume);
extern int mozart_ingenicplayer_notify_play_mode(enum play_mode mode);
extern int mozart_ingenicplayer_notify_song_info(void);
extern int mozart_ingenicplayer_response_cmd(char *command, char *data, struct appserver_cli_info *owner);
extern int mozart_ingenicplayer_startup(void);
extern int mozart_ingenicplayer_shutdown(void);
#else
static inline int mozart_ingenicplayer_notify_play_shortcut(char *shortcut_name) { return 0; }
static inline int mozart_ingenicplayer_notify_pos(void) { return 0; }
static inline int mozart_ingenicplayer_notify_volume(int volume) { return 0; }
static inline int mozart_ingenicplayer_notify_play_mode(enum play_mode mode) { return 0; }
static inline int mozart_ingenicplayer_notify_song_info(void) { return 0; }
static inline int mozart_ingenicplayer_response_cmd(char *command,
						    char *data, struct appserver_cli_info *owner) { return 0; }
static inline int mozart_ingenicplayer_startup(void) { return 0; }
static inline int mozart_ingenicplayer_shutdown(void) { return 0; }
#endif

#endif /* __INGENICPLAYER_H__ */
