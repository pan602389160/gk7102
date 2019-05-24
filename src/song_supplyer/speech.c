#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <pthread.h>


#include "mozart_musicplayer.h"
#include "vr_interface.h"

int speech_cloudmusic_playmusic(sds_music_t *music, int index)
{
	printf("==============speech_cloudmusic_playmusic==================music->number = %d\n",music->number);
	int i = 0;
	int ret = 0;
	struct music_info *info = NULL;

	if (!music)
		return -1;

	mozart_musicplayer_start(mozart_musicplayer_handler);

	mozart_musicplayer_musiclist_clean(mozart_musicplayer_handler);

	for (i = 0; i < music->number; i++) {
		info = mozart_musiclist_get_info(-1,/* songid ignore */
										 music->data[i].title,
										 music->data[i].url,
										 NULL,/* song picture ignore */
										 NULL,/* album name ignore */
										 music->data[i].artist,
										 NULL/* private data ignore */);
		if (info){
			mozart_musicplayer_musiclist_insert(mozart_musicplayer_handler, info);
		}
	}

	ret = mozart_musicplayer_play_index(mozart_musicplayer_handler, index);

	return ret;
}

int speech_cloudmusic_playfm(sds_netfm_t *netfm, int index)
{
	int i = 0;
	int ret = 0;
	struct music_info *info = NULL;

	if (!netfm)
		return -1;

	mozart_musicplayer_musiclist_clean(mozart_musicplayer_handler);

	for (i = 0; i < netfm->number; i++) {
		info = mozart_musiclist_get_info(-1,/* songid ignore */
										 netfm->data[i].track,
										 netfm->data[i].url,
										 NULL,/* song picture ignore */
										 NULL,/* album name ignore */
										 NULL, /*artist ignore*/
										 NULL/* private data ignore */);
		if (info)
			mozart_musicplayer_musiclist_insert(mozart_musicplayer_handler, info);
	}

	ret = mozart_musicplayer_play_index(mozart_musicplayer_handler, index);

	return ret;
}

int speech_cloudmusic_playjoke(char *url)
{
	int ret = 0;
	struct music_info *info = NULL;

	if (!url)
		return -1;

	mozart_musicplayer_musiclist_clean(mozart_musicplayer_handler);

	info = mozart_musiclist_get_info(-1,/* songid ignore */
									 NULL,
									 url,
									 NULL,/* song picture ignore */
									 NULL,/* album name ignore */
									 NULL, /*artist ignore*/
									 NULL/* private data ignore */);
	if (info)
		mozart_musicplayer_musiclist_insert(mozart_musicplayer_handler, info);

	ret = mozart_musicplayer_play_index(mozart_musicplayer_handler, 0);

	return ret;
}

int speech_cloudmusic_play(char *key)
{
	printf("=========%s============\n",__func__);
	/* get result in asr callback */
	return ray_vr_content_get(key);
}

