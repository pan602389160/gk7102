#ifndef __RAY_KEY_FUNCTION_H__
#define __RAY_KEY_FUNCTION_H__

//#include "mozart_config.h"

typedef enum snd_source_t{
	SND_SRC_NONE = -1,
	SND_SRC_CLOUD,
	SND_SRC_BT_AVK,
	SND_SRC_LOCALPLAY,
	SND_SRC_MAX,
} snd_source_t;

enum mozart_module_play_status {
	mozart_module_status_stop = 0,
	mozart_module_status_play,
	mozart_module_status_pause,
};

 snd_source_t snd_source;
 char *keyvalue_str[];
 char *keycode_str[];
 int tfcard_status;

 char *app_name;
 snd_source_t snd_source;
 int mozart_module_pause(void);
 int mozart_module_stop(void);
 void mozart_wifi_mode(void);
 void mozart_config_wifi(void);
 void mozart_previous_song(void);
 void mozart_next_song(void);
 void mozart_play_pause(void);
 void mozart_volume_up(void);
 void mozart_volume_down(void);
 void mozart_linein_on(void);
 void mozart_linein_off(void);
 void mozart_snd_source_switch(void);
 void mozart_music_list(int);
 enum mozart_module_play_status mozart_module_get_play_status(void);

#endif	

