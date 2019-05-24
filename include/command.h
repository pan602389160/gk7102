#ifndef __COMMAND_H__
#define __COMMAND_H__

#include <stdbool.h>
#include "player_interface.h"



typedef enum {
	PLAY_URL = 1,
	CACHE_URL,
	AO_SWITCH,
	START_CH,
	STOP_CH,
	DO_RESUME,
	DO_PAUSE,
	DO_STOP,
	DO_SEEK,
	SET_VOLUME,

	DO_STOPALL,
	DO_PAUSEALL,
	DO_RESUMEALL,

	GET_STATUS,
	GET_URL,
	GET_POS,
	GET_DURATION,
	GET_UUID,
	GET_SNAPSHOT,
	KILL_MPLAYER,
	WAIT_RELEASE,
} cmd_type;
/*
char *cmd_type_str[] = {
	[PLAY_URL] = "PLAY_URL",
	[CACHE_URL] = "CACHE_URL",
	[DO_RESUME] = "DO_RESUME",
	[DO_PAUSE] = "DO_PAUSE",
	[DO_STOP] = "DO_STOP",
	...
};
*/
extern char *cmd_type_str[];

typedef struct {
	int len; // length of url.
	char url[0];
} player_url_t;

typedef struct {
	int len; // length of uuid.
	char uuid[0];
} player_uuid_t;

/**
 * @brief command transport to player.
 */
typedef struct {
	/**
	 * @brief size of total command(append strlen(url) if type is PLAY_URL(_SYNC))
	 */
	int total_size;
	/**
	 * @brief uuid of command owner
	 */
	char uuid[32];
	/**
	 * @brief channel
	 */
	int ch_num;
	/**
	 * @brief command
	 */
	cmd_type type;
	/**
	 * @brief param of command
	 */
	union {
		/**
		 * @brief param of DO_SEEK, ONLY valid on DO_SEEK command
		 */
		int pos;
		/**
		 * @brief param of SET_VOLUME and WAIT_RELEASE, ONLY valid on SET_VOLUME and WAIT_RELEASE command
		 */
		int val;
		/**
		 * @brief param of PLAY_URL(_SYNC), ONLY valid on PLAY_URL(_SYNC) command
		 */
		char url[0];
		/**
		 * @brief param of AO_SWITCH, ONLY valid on AO_SWITCH command
		 */
		char ao[0];
		/**
		 * @brief param of DO_STOPALL/DO_PAUSEALL, ONLY valid on DO_STOPALL/DO_PAUSEALL command
		 */
		bool need_feadback;
	} param;
} player_command;

/**
 * @brief enable/disable debug mode.
 */
extern bool debug_player_command_mode;

/**
 * @brief debug command struct.
 *
 * @param cmd
 */
extern void debug_player_command(player_command *cmd);

/**
 * @brief create a command struct.
 *
 * @param uuid uuid of command owner
 * @param type command type
 * @param arg param of command
 *
 * @return return a command
 */
extern player_command *create_player_command(char *uuid, int ch, cmd_type type, void *arg);

/**
 * @brief destory a command created by create_player_command().
 *
 * @param cmd command to be destoryed.
 */
extern void destory_player_command(player_command **cmd);

/**
 * @brief send cmd to player, used in libplayer.
 *
 * @param player_handler handler of player client.
 * @param cmd command to be send.
 *
 * @return return 0 if success, -1 otherwise.
 */
extern int send_cmd(player_handler_t *player_handler, player_command *cmd);

/**
 * @brief recv a cmd from libplayer, used in player.
 *
 * @param connfd connect fd of player client.
 *
 * @return return 0 if success, -1 otherwise.
 */
extern player_command *recv_cmd(int connfd);

#endif // __COMMAND_H__
