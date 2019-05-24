/* ************************************************************************
 *       Filename:  utils_config.c
 *    Description:
 *        Version:  1.0
 *        Created:  6/23/2017 10:54:29 AM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  fshang (),
 *        Company:
 * ************************************************************************/

#include <string.h>
#include "ini_interface.h"
#include "utils_interface.h"

#define SECTION_PRODUCT   "product"
#define KEY_CPU           "cpu"
#define KEY_NAME          "name"

#define SECTION_AUDIO     "audio"
#define KEY_TYPE          "type"
#define KEY_CODEC         "codec"
#define KEY_DEV_PLAYBACK  "dev_playback"
#define KEY_DEV_RECORD    "dev_record"
#define KEY_DEV_PCM       "dev_pcm"
#define KEY_PLAYBACK      "playback"
#define KEY_RECORD        "record"
#define KEY_VOLUME        "volume"
#define KEY_MUTE          "mute"
#define KEY_LINEIN        "linein"

#define SECTION_NV        "nv"
#define KEY_STORAGE       "storage"
#define KEY_BLKNAME       "blkname"

#define SECTION_USB_AUDIO "usb_audio"
#define KEY_USE_USB_AUDIO "storage"

#define PATH_LEN          128
static char system_ini_conf_path[PATH_LEN] = "/usr/data/system.ini";

char* mozart_ini_get_system_conf_path(void)
{
	return system_ini_conf_path;
}
int mozart_ini_set_system_conf_path(char *path)
{
	if(path && strlen(path) < PATH_LEN - 1){
		strcpy(system_ini_conf_path, path);
		return 0;

	}else{
		return -1;
	}
}
int mozart_ini_get_product_name(char *name)
{
	return mozart_ini_getkey(system_ini_conf_path,
			SECTION_PRODUCT, KEY_NAME, name);
}
int mozart_ini_set_product_name(char *name)
{
	return mozart_ini_setkey(system_ini_conf_path,
			SECTION_PRODUCT, KEY_NAME, name);
}
int mozart_ini_get_product_cpu(char *cpu)
{
	return mozart_ini_getkey(system_ini_conf_path,
			SECTION_PRODUCT, KEY_CPU, cpu);
}
int mozart_ini_set_product_cpu(char *cpu)
{
	return mozart_ini_setkey(system_ini_conf_path,
			SECTION_PRODUCT, KEY_CPU, cpu);
}
int mozart_ini_get_audio_type(char *type)
{
	return mozart_ini_getkey(system_ini_conf_path,
			SECTION_AUDIO, KEY_TYPE, type);
}
int mozart_ini_set_audio_type(char *type)
{
	return mozart_ini_setkey(system_ini_conf_path,
			SECTION_AUDIO, KEY_TYPE, type);
}
int mozart_ini_get_audio_codec(char *codec)
{
	return mozart_ini_getkey(system_ini_conf_path,
			SECTION_AUDIO, KEY_CODEC, codec);
}
int mozart_ini_set_audio_codec(char *codec)
{
	return mozart_ini_setkey(system_ini_conf_path,
			SECTION_AUDIO, KEY_CODEC, codec);
}
int mozart_ini_get_audio_dev_pcm(char *dev_pcm)
{
	return mozart_ini_getkey(system_ini_conf_path,
			SECTION_AUDIO, KEY_DEV_PCM, dev_pcm);
}
int mozart_ini_set_audio_dev_pcm(char *dev_pcm)
{
	return mozart_ini_setkey(system_ini_conf_path,
			SECTION_AUDIO, KEY_DEV_PCM, dev_pcm);
}
int mozart_ini_get_audio_dev_playback(char *dev_playback)
{
	return mozart_ini_getkey(system_ini_conf_path,
			SECTION_AUDIO, KEY_DEV_PLAYBACK, dev_playback);
}
int mozart_ini_set_audio_dev_playback(char *dev_playback)
{
	return mozart_ini_setkey(system_ini_conf_path,
			SECTION_AUDIO, KEY_DEV_PLAYBACK, dev_playback);
}
int mozart_ini_get_audio_dev_record(char *dev_record)
{
	return mozart_ini_getkey(system_ini_conf_path,
			SECTION_AUDIO, KEY_DEV_RECORD, dev_record);
}
int mozart_ini_set_audio_dev_record(char *dev_record)
{
	return mozart_ini_setkey(system_ini_conf_path,
			SECTION_AUDIO, KEY_DEV_RECORD, dev_record);
}
int mozart_ini_get_audio_record(char *record_dev)
{
	return mozart_ini_getkey(system_ini_conf_path,
			SECTION_AUDIO, KEY_RECORD, record_dev);
}
int mozart_ini_set_audio_record(char *record_dev)
{
	return mozart_ini_setkey(system_ini_conf_path,
			SECTION_AUDIO, KEY_RECORD, record_dev);
}
int mozart_ini_get_audio_playback(char *playback)
{
	return mozart_ini_getkey(system_ini_conf_path,
			SECTION_AUDIO, KEY_PLAYBACK, playback);
}
int mozart_ini_set_audio_playback(char *playback)
{
	return mozart_ini_setkey(system_ini_conf_path,
			SECTION_AUDIO, KEY_PLAYBACK, playback);
}
int mozart_ini_get_audio_volume(char *audio_volume)
{
	return mozart_ini_getkey(system_ini_conf_path,
			SECTION_AUDIO, KEY_VOLUME, audio_volume);
}
int mozart_ini_set_audio_volume(char *audio_volume)
{
	return mozart_ini_setkey(system_ini_conf_path,
			SECTION_AUDIO, KEY_VOLUME, audio_volume);
}
int mozart_ini_get_audio_mute(char *mute)
{
	return mozart_ini_getkey(system_ini_conf_path,
			SECTION_AUDIO, KEY_MUTE, mute);
}
int mozart_ini_set_audio_mute(char *mute)
{
	return mozart_ini_setkey(system_ini_conf_path,
			SECTION_AUDIO, KEY_MUTE, mute);
}
int mozart_ini_get_nv_blkname(char *blkname)
{
	return mozart_ini_getkey(system_ini_conf_path,
			SECTION_NV, KEY_BLKNAME, blkname);
}
int mozart_ini_set_nv_blkname(char *blkname)
{
	return mozart_ini_setkey(system_ini_conf_path,
			SECTION_NV, KEY_BLKNAME, blkname);
}
int mozart_ini_get_usb_audio_use_usb_audio(char *use_usb_audio)
{
	return mozart_ini_getkey(system_ini_conf_path,
			SECTION_USB_AUDIO, KEY_USE_USB_AUDIO, use_usb_audio);
}
int mozart_ini_set_usb_audio_use_usb_audio(char *use_usb_audio)
{
	return mozart_ini_setkey(system_ini_conf_path,
			SECTION_USB_AUDIO, KEY_USE_USB_AUDIO, use_usb_audio);
}
