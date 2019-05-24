/* ************************************************************************
 *       Filename:  utils_interface.h
 *    Description:
 *        Version:  1.0
 *        Created:  10/28/2014 06:57:57 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xllv (),
 *        Company:
 * ************************************************************************/

#ifndef __UTILS_H__
#define __UTILS_H__

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef  __cplusplus
extern "C" {
#endif

typedef enum {
	IDLE = 0,
	READING,
	WRITEING,
	BUSY,
} dsp_status;

typedef enum {
	AUDIO_UNKNOWN,
	AUDIO_OSS,
	AUDIO_ALSA,
} audio_type;

typedef enum {
	UNKNOWN_CODEC,
	INTER_CODEC,
	EXTER_CODEC,
} codec_type;

typedef struct sdcard_info{
	int totalSize;       //总的空间
	int availableSize;   //可用空间
	int usedSize;        //剩余空间
} sdinfo;

/**
 * @brief F_SETOWN
 *
 * @param fd operate fd
 * @param pid owner pid
 */
extern void set_owner(int fd, pid_t pid);

/**
 * @brief F_SETFL
 *
 * @param fd file fd
 * @param flags flags
 */
extern void set_flag(int fd, int flags);

extern void clr_flag(int fd, int flags);

/**
 * @brief get /dev/dsp status
 *
 * @param dsp_stat
 */
extern void get_dsp_status(dsp_status *dsp_stat);

/**
 * @brief get ip address
 *
 * @param ifname [in] interface name, such as wlan0
 *
 * @return return ip address(DO NOT need free) if got,
 */
extern char *get_ip_addr(char *ifname);

/**
 * @brief get mac address
 *
 * @param ifname [in] interface name
 * @param macaddr [out] buffer for
 * @param separator [in] separator
 *
 * @return return param macaddr if get, return NULL otherwise.
 */
extern char *get_mac_addr(char *ifname, char *macaddr, char *separator);

/**
 * @brief a secure system
 *
 * @param cmd [in] cmd string
 *
 * @return the same as system().
 */
extern int mozart_system(const char *cmd);

/**
 * @brief digits to string
 *
 * @param value [in] digits
 * @param str [out] target string
 *
 * @return return str if success convert
 */
extern char *mozart_itoa(int value, char *str);

/**
 * @brief check path is mounted?
 *
 * @param path [in] path to check
 *
 * @return return true if mount, false otherwise.
 */
extern bool mozart_path_is_mount(const char *path);

/**
 * @brief get sdcard size info
 *
 * @param sdpath sdcard mount dir
 *
 * @return return sdcard info.
 */
extern sdinfo mozart_get_sdcard_info(const char *sdpath);

/**
 * @brief check dsp open flag.
 *
 * @param opt [in] open flag: O_RDONLY, O_WRONLY, or O_RDWR
 *
 * @return return 1 if can open with opt, return 0 otherwise.
 */
int check_dsp_opt(int opt);

/**
 * @brief get audio type from /usr/data/system.ini
 *
 * @return retrun audio type
 */
extern audio_type get_audio_type(void);

/**
 * @brief get codec type
 *
 * @return return codec type
 */
extern codec_type get_audio_codec_type(void);

/**
 * @brief get config file's path
 *
 * @return return pointer of the path
 */
extern char* mozart_ini_get_system_conf_path(void);
/**
 * @brief set system config file's path
 *
 * @param path the path of config file
 *
 * @return 0 on success otherwise negative error code.
 */
extern int mozart_ini_set_system_conf_path(char *path);

extern int mozart_ini_get_product_name(char *name);
extern int mozart_ini_set_product_name(char *name);
extern int mozart_ini_get_product_cpu(char *cpu);
extern int mozart_ini_set_product_cpu(char *cpu);
extern int mozart_ini_get_audio_type(char *type);
extern int mozart_ini_set_audio_type(char *type);
extern int mozart_ini_get_audio_codec(char *codec);
extern int mozart_ini_set_audio_codec(char *codec);
extern int mozart_ini_get_audio_dev_pcm(char *dev_pcm);
extern int mozart_ini_set_audio_dev_pcm(char *dev_pcm);
extern int mozart_ini_get_audio_dev_playback(char *dev_playback);
extern int mozart_ini_set_audio_dev_playback(char *dev_playback);
extern int mozart_ini_get_audio_dev_record(char *dev_record);
extern int mozart_ini_set_audio_dev_record(char *dev_record);
extern int mozart_ini_get_audio_record(char *record_dev);
extern int mozart_ini_set_audio_record(char *record_dev);
extern int mozart_ini_get_audio_playback(char *playback);
extern int mozart_ini_set_audio_playback(char *playback);
extern int mozart_ini_get_audio_volume(char *audio_volume);
extern int mozart_ini_set_audio_volume(char *audio_volume);
extern int mozart_ini_get_audio_mute(char *mute);
extern int mozart_ini_set_audio_mute(char *mute);
extern int mozart_ini_get_nv_blkname(char *blkname);
extern int mozart_ini_set_nv_blkname(char *blkname);
extern int mozart_ini_get_usb_audio_use_usb_audio(char *use_usb_audio);
extern int mozart_ini_set_usb_audio_use_usb_audio(char *use_usb_audio);

#ifdef  __cplusplus
}
#endif

#endif /** __UTILS_H__ **/
