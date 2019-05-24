#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#include "command.h"

bool debug_player_command_mode = false;
char *cmd_type_str[] = {
	[PLAY_URL] = "PLAY_URL",
	[CACHE_URL] = "CACHE_URL",
	[AO_SWITCH] = "AO_SWITCH",
	[START_CH] = "START_CH",
	[STOP_CH] = "STOP_CH",
	[DO_RESUME] = "DO_RESUME",
	[DO_PAUSE] = "DO_PAUSE",
	[DO_STOP] = "DO_STOP",
	[DO_SEEK] = "DO_SEEK",
	[SET_VOLUME] = "SET_VOLUME",
	[DO_STOPALL] = "DO_STOPALL",
	[DO_PAUSEALL] = "DO_PAUSEALL",
	[DO_RESUMEALL] = "DO_RESUMEALL",
	[GET_STATUS] = "GET_STATUS",
	[GET_URL] = "GET_URL",
	[GET_POS] = "GET_POS",
	[GET_DURATION] = "GET_DURATION",
	[GET_UUID] = "GET_UUID",
	[GET_SNAPSHOT] = "GET_SNAPSHOT",
	[KILL_MPLAYER] = "KILL_MPLAYER",
	[WAIT_RELEASE] = "WAIT_RELEASE"
};

#if 0
#define DEBUG(x,y...)	(printf("DEBUG [ %s : %s : %d] "x"\n",__FILE__, __func__, __LINE__, ##y))
#else
#define DEBUG(x,y...)
#endif
#define ERROR(x,y...)	(printf("ERROR [ %s : %s : %d] "x"\n", __FILE__, __func__, __LINE__, ##y))

void debug_player_command(player_command *cmd)
{
	if (!debug_player_command_mode)
		return;

	printf("\ndump cmd: {\n"
	       "  cmd: %p.\n"
	       "  cmd->total_size: %d.\n"
	       "  cmd->uuid: %s.\n"
	       "  cmd->ch_num: %d.\n"
	       "  cmd->type: %s.\n", cmd, cmd->total_size, cmd->uuid, cmd->ch_num, cmd_type_str[cmd->type]);

	switch (cmd->type) {
	case PLAY_URL:
	case CACHE_URL:
		printf("  cmd->param.url: %s.\n", cmd->param.url);
		break;
	case AO_SWITCH:
		printf("  cmd->param.ao: %s.\n", cmd->param.ao);
		break;
	case START_CH:
	case DO_RESUME:
	case DO_PAUSE:
	case DO_STOP:
	case GET_STATUS:
	case GET_URL:
	case GET_POS:
	case GET_DURATION:
	case GET_UUID:
	case GET_SNAPSHOT:
	case KILL_MPLAYER:
		break;
	case STOP_CH:
	case DO_STOPALL:
	case DO_PAUSEALL:
	case DO_RESUMEALL:
		printf("  cmd->param.need_feadback: %d.\n", cmd->param.need_feadback);
		break;
	case DO_SEEK:
		printf("  cmd->param.pos: %d.\n", cmd->param.pos);
		break;
	case SET_VOLUME:
	case WAIT_RELEASE:
		printf("  cmd->param.val: %d.\n", cmd->param.val);
		break;
	default:
		printf("  Unknown cmd type!!!!\n");
		break;
	}

	printf("}\n");
}
player_command *create_player_command(char *uuid, int ch, cmd_type type, void *arg)
{
	player_command *cmd = NULL;
	int total_size = -1;

	if (!uuid) {
		DEBUG("No uuid found, do nothing.\n");
		return NULL;
	}

	total_size = sizeof(*cmd);
	cmd = calloc(total_size, 1);
	if (!cmd) {
		DEBUG("alloc memory for player command: %d error: %s.\n", type, strerror(errno));
		return NULL;
	}

	switch (type) {
	case PLAY_URL:
	case CACHE_URL:
		{
			char *url = (char *)arg;
			if (!url) {
				ERROR("No url found, do nothing.\n");
				return NULL;
			}

			/* total size = sizeof(player_command) + size of url + 1 */
			total_size = sizeof(*cmd) + strlen(url) + 1;

			cmd = realloc(cmd, total_size);
			if (!cmd) {
				DEBUG("alloc memory for url: %d error: %s.\n", type, strerror(errno));
				return NULL;
			}

			memset(cmd, 0, total_size);
			strncpy(cmd->uuid, uuid, sizeof(cmd->uuid) - 1);
			cmd->ch_num = ch;
			cmd->total_size = total_size;
			cmd->type = type;
			strcpy(cmd->param.url, url);
		}
		break;
	case AO_SWITCH:
		{
			char *ao = (char *)arg;
			if (!ao) {
				ERROR("No ao found, do nothing.\n");
				return NULL;
			}

			/* total size = sizeof(player_command) + size of ao + 1 */
			total_size = sizeof(*cmd) + strlen(ao) + 1;

			cmd = realloc(cmd, total_size);
			if (!cmd) {
				DEBUG("alloc memory for ao: %d error: %s.\n", type, strerror(errno));
				return NULL;
			}

			memset(cmd, 0, total_size);
			strncpy(cmd->uuid, uuid, sizeof(cmd->uuid) - 1);
			cmd->ch_num = ch;
			cmd->total_size = total_size;
			cmd->type = type;
			strcpy(cmd->param.ao, ao);
		}
		break;
	case START_CH:
	case DO_RESUME: // No param.
	case DO_PAUSE:
	case DO_STOP:
	case GET_STATUS:
	case GET_URL:
	case GET_POS:
	case GET_DURATION:
	case GET_UUID:
	case GET_SNAPSHOT:
	case KILL_MPLAYER:
		strncpy(cmd->uuid, uuid, sizeof(cmd->uuid) - 1);
		cmd->ch_num = ch;
		cmd->total_size = total_size;
		cmd->type = type;
		break;
	case STOP_CH:
	case DO_STOPALL:
	case DO_PAUSEALL:
	case DO_RESUMEALL:
		{
			bool need_feadback = (bool)arg;

			strncpy(cmd->uuid, uuid, sizeof(cmd->uuid) - 1);
			cmd->ch_num = ch;
			cmd->total_size = total_size;
			cmd->type = type;
			cmd->param.need_feadback = need_feadback;
		}
		break;
	case DO_SEEK:
		{
			int pos = (int)arg;

			strncpy(cmd->uuid, uuid, sizeof(cmd->uuid) - 1);
			cmd->ch_num = ch;
			cmd->total_size = total_size;
			cmd->type = type;
			cmd->param.pos= pos;
		}
		break;
	case SET_VOLUME:
	case WAIT_RELEASE:
		{
			int val = (int)arg;

			strncpy(cmd->uuid, uuid, sizeof(cmd->uuid) - 1);
			cmd->ch_num = ch;
			cmd->total_size = total_size;
			cmd->type = type;
			cmd->param.val= val;
		}
		break;
	default:
		ERROR("Not support player cmd type: %d, do nothing.\n", type);
		break;
	}

	return cmd;
}

void destory_player_command(player_command **cmd)
{
	player_command *tmp = *cmd;

	if (!tmp)
		return;

	free(tmp);

	tmp = NULL;

	return;
}

int send_cmd(player_handler_t *handler, player_command *cmd)
{
	int ret = 0;

	if (!handler)
		return -1;

	if (!cmd) {
		ERROR("send player_command is NULL");
		return -1;
	}

send_again:
	ret = send(handler->fd_sync, cmd, cmd->total_size, 0);
	if (cmd->total_size != ret) {
		if (ret == -1 && errno == EINTR)
			goto send_again;
		else
			ERROR("send player_command error: %s", strerror(errno));
	}

	return 0;
}

player_command *recv_cmd(int connfd)
{
	int ret = -1;

	int total_size = -1;
	player_command *cmd = NULL;

	ret = recv(connfd, &total_size, sizeof(total_size), 0);
	if (ret != sizeof(total_size)) {
		if (ret == 0)
			DEBUG("connection peer has shutdown, please check and handle it!!!!!\n");
		else if (ret == -1)
			ERROR("recv cmd error:%s", strerror(errno));
		goto err_exit;
	}

	cmd = malloc(total_size);
	if (!cmd) {
		ERROR("realloc memory for recv cmd error: %s.\n", strerror(errno));
		goto err_exit;
	}
	cmd->total_size = total_size;

	ret = recv(connfd, (char *)cmd + sizeof(total_size), total_size - sizeof(total_size), 0);
	if (ret != (total_size - sizeof(total_size))) {
		if (ret == 0)
			DEBUG("connection peer has shutdown, please check and handle it!!!!!\n");
		else if (ret == -1)
			ERROR("recv url error:%s", strerror(errno));

		goto err_exit;
	}

	return cmd;

err_exit:

	if (cmd) {
		free(cmd);
		cmd = NULL;
	}

	return NULL;
}
