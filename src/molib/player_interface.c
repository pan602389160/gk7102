#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "tcp.h"
#include "command.h"
#include "player_interface.h"
//#include "volume_interface.h"
#include "player_ipc_interface.h"

#define DEBUG(x,y...)

#define ERROR(x,y...)	(printf("ERROR [ %s : %s : %d] "x"\n", __FILE__, __func__, __LINE__, ##y))

static pthread_mutex_t player_internal_mutex = PTHREAD_MUTEX_INITIALIZER;

char *player_status_str[] = {
	[PLAYER_UNKNOWN] = "PLAYER_UNKNOWN",
	[PLAYER_TRANSPORT] = "PLAYER_TRANSPORT",
	[PLAYER_PLAYING] = "PLAYER_PLAYING",
	[PLAYER_PAUSED] = "PLAYER_PAUSED",
	[PLAYER_STOPPED] = "PLAYER_STOPPED",
	[AOSWITCH_FAIL] = "AOSWITCH_FAIL",
};

static int register_connection(char *method)
{
	int ret = -1;
	int fd = -1;
	char buf[2] = {};

	fd = connect_to_unix_socket(PLAYER_SOCK);
	if (fd < 0) {
		ERROR("connect to player server failed: %s.\n", strerror(errno));
		fd = -1;
		goto err_exit;
	}

	ret = send(fd, method, strlen(method) + 1, 0);
	if (ret != strlen(method) + 1) {
		ERROR("Send %s connection request error: %s.\n", method, strerror(errno));
		goto err_exit;
	}

	ret = recv(fd, buf, 2, 0);
	printf("register_connection recv buf = %s,len = %d\n",buf,ret);
	
	if (ret != 2) {
		ERROR("Register %s connection error: %s.\n", method, strerror(errno));
	} else if (buf[0] != '1') {  // #define REGISTER_OK '1'
		ERROR("Register %s connection error: %s.\n", method, "Bad connection protocols");
		goto err_exit;
	}

	return fd;

err_exit:
	if (fd >= 0) {
		close(fd);
		fd = -1;
	}
	return fd;
}

static bool recv_result_feadback(player_handler_t *handler)
{
	bool feadback = false;
	int rs = -1;

	if (!handler)
		return feadback;

	rs = recv(handler->fd_sync, &feadback, sizeof(feadback), 0);
	if (rs != sizeof(feadback)) {
		if (rs == 0)
			ERROR("connection peer has shutdown, please check and handle it!!!!!\n");
		else if (rs == -1)
			ERROR("recv status error:%s", strerror(errno));
	}

	return feadback;
}

int mozart_player_start_newchannel(player_handler_t *handler)
{
	int rs = -1;

	if (!handler)
		return -1;

	player_command *cmd = create_player_command(handler->uuid, handler->ch_num, START_CH, NULL);
	if (!cmd) {
		ERROR("Invalid cmd, Do nothing.\n");
		return -1;
	}

	rs = send_cmd(handler, cmd);

	destory_player_command(&cmd);

	return rs;
}


int mozart_player_stop_newchannel(player_handler_t *handler)
{
	int ret = -1;
	bool feadback = false;
	bool is_sync = true;

	if (!handler)
		return -1;

	player_command *cmd = create_player_command(handler->uuid, handler->ch_num, STOP_CH, (void *)is_sync);
	if (!cmd) {
		ERROR("Invalid cmd, Do nothing.\n");
		return -1;
	}

	pthread_mutex_lock(&player_internal_mutex);
	ret = send_cmd(handler, cmd);
	if (!ret && is_sync)
		feadback = recv_result_feadback(handler);
	pthread_mutex_unlock(&player_internal_mutex);

	destory_player_command(&cmd);

	return feadback ? 0 : -1;
}


int mozart_player_playurl(player_handler_t *handler, char *url)
{
	int rs = -1;

	if (!handler)
		return -1;

	player_command *cmd = create_player_command(handler->uuid, handler->ch_num, PLAY_URL, url);
	if (!cmd) {
		ERROR("Invalid cmd, Do nothing.\n");
		return -1;
	}
	printf(">>>>>>>>>>>mozart_player_playurl\n");
	rs = send_cmd(handler, cmd);

	destory_player_command(&cmd);

	return rs;
}


int mozart_player_cacheurl(player_handler_t *handler, char *url)
{
	int rs = -1;

	if (!handler)
		return -1;

	player_command *cmd = create_player_command(handler->uuid, handler->ch_num, CACHE_URL, url);
	if (!cmd) {
		ERROR("Invalid cmd, Do nothing.\n");
		return -1;
	}

	rs = send_cmd(handler, cmd);

	destory_player_command(&cmd);

	return rs;
}




static void* recv_player_snapshot_async_func(void *arg)
{
	int ret = -1;
	player_handler_t *handler = (player_handler_t *)arg;
	fd_set rfds;
	struct timeval timeout;

	player_snapshot_t snapshot;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

	while (1) {
		memset(&snapshot, 0, sizeof(snapshot));

		FD_ZERO(&rfds);
		if (handler->fd_async < 0)
			break;
		else
			FD_SET(handler->fd_async, &rfds);

		timeout.tv_sec = 0;
		timeout.tv_usec = 200 * 1000; // check exit flag each 200ms.
		ret = select(handler->fd_async + 1, &rfds, NULL, NULL, &timeout);
		if (ret == -1) {
			if (handler->fd_async < 0 && handler->_service_stop) {
				break;
			} else {
				printf("[player - Error] select() for recv player msg failed: %s,"
				       "Re-try after 500ms.\n", strerror(errno));
				usleep(500 * 1000);
				continue;
			}
		} else if (ret == 0) { // select timeout, check exit flag.
			if (handler->_service_stop)
				break;
			else
				continue;
		}

		// if waiting feedback in other process, SHOULD NOT recv msg here.
		if (pthread_mutex_trylock(&player_internal_mutex)) {
			DEBUG("sync mode, Will not recv player msg.\n");
			usleep(100 * 1000);
			continue;
		}
		ret = recv(handler->fd_async, &snapshot, sizeof(snapshot), 0);
		printf("----->>>  recv_player_snapshot_async_func ret = %d\n",ret);
		pthread_mutex_unlock(&player_internal_mutex);
		if (ret == sizeof(snapshot)) {
			
			if ((handler->ch_num == snapshot.ch_num) && handler->callback)
				handler->callback(&snapshot, handler, handler->param);
		} else if (ret == 0) {
			printf("[Player - Error] player disconnected(Dead?), Re-connect after 500ms.\n");
			usleep(500 * 1000);

			if (handler->fd_async)
				close(handler->fd_async);

			while ((handler->fd_async = register_connection("async")) < 0) {
				printf("[Player - Error] Register async connection error, Re-try after 200ms.\n");
				usleep(200 * 1000);
			}
		} else {
			printf("[Player - Error] recieve data from player failed: %s, Re-try after 500ms.\n",
			       strerror(errno));
			usleep(500 * 1000);

			if (handler->fd_async)
				close(handler->fd_async);

			while ((handler->fd_async = register_connection("async")) < 0) {
				printf("[Player - Error] Register async connection error, Re-try after 200ms.\n");
				usleep(200 * 1000);
			}
		}
	}

	return NULL; // make compile happy.
}


player_handler_t *mozart_player_ch_handler_get(char *uuid, int ch, player_callback callback, void *param)
{
	player_handler_t *handler = NULL;

	if ((ch < 0) && (ch >= 2)) {
		ERROR("ch(%d) out of range", ch);
		return NULL;
	}

	handler = (player_handler_t *)malloc(sizeof(player_handler_t));
	if (!handler) {
		printf("Can not alloc memory for player handler: %s.\n", strerror(errno));
		goto err_exit;
	}
	memset(handler, 0, sizeof(player_handler_t));

	// register sync connection.
	handler->fd_sync = register_connection("sync");
	if (handler->fd_sync < 0) {
		printf("Register sync connection error.\n");
		goto err_exit;
	}

	// register async connection.
	handler->fd_async = register_connection("async");
	if (handler->fd_async < 0) {
		printf("Register async connection error.\n");
		goto err_exit;
	}

	strncpy(handler->uuid, uuid, sizeof(handler->uuid) - 1);
	handler->ch_num = ch;
	handler->callback = callback;
	handler->param = param;

	if (pthread_create(&handler->handler_thread, NULL, recv_player_snapshot_async_func, (void *)handler)){
		ERROR("create thread to wait player event error: %s.\n", strerror(errno));
		goto err_exit;
	}
	handler->_service_stop = false;

	if(handler->ch_num != 0)
		mozart_player_start_newchannel(handler);

	return handler;

err_exit:
	if (handler) {
		if (handler->fd_async >= 0) {
			close(handler->fd_async);
			handler->fd_async = -1;
		}
		if (handler->fd_sync >= 0) {
			close(handler->fd_sync);
			handler->fd_sync = -1;
		}
		if (handler->callback)
			handler->callback = NULL;

		free(handler);
		handler = NULL;
	}
	return NULL;
}


player_handler_t *mozart_player_handler_get(char *uuid, player_callback callback, void *param)
{
	return mozart_player_ch_handler_get(uuid, 0, callback, param);
}

void mozart_player_handler_put(player_handler_t *handler)
{
	if (!handler)
		return;

	if(handler->ch_num != 0)
		mozart_player_stop_newchannel(handler);

	handler->_service_stop = true;

	pthread_join(handler->handler_thread, NULL);

	handler->_service_stop = false;

	if (handler->fd_async >= 0) {
		close(handler->fd_async);
		handler->fd_async = -1;
	}

	if (handler->fd_sync >= 0) {
		close(handler->fd_sync);
		handler->fd_sync = -1;
	}

	if (handler->callback)
		handler->callback = NULL;

	free(handler);

	return;
}

int mozart_player_aoswitch(player_handler_t *handler, char *ao)
{
	int rs = -1;

	if (!handler)
		return -1;

	player_command *cmd = create_player_command(handler->uuid, handler->ch_num, AO_SWITCH, ao);
	if (!cmd) {
		ERROR("Invalid cmd, Do nothing.\n");
		return -1;
	}

	rs = send_cmd(handler, cmd);

	destory_player_command(&cmd);

	return rs;
}

static player_snapshot_t *recv_player_snapshot(player_handler_t *handler)
{
	int rs = -1;
	player_snapshot_t *snapshot = NULL;

	if (!handler)
		goto err_exit;

	snapshot = malloc(sizeof(*snapshot));
	if (!snapshot) {
		printf("Can not alloc memory for recv snapshot: %s.\n", strerror(errno));
		goto err_exit;
	}
	memset(snapshot, 0, sizeof(*snapshot));

	rs = recv(handler->fd_sync, snapshot, sizeof(*snapshot), 0);
	if (rs != sizeof(*snapshot)) {
		if (rs == 0)
			ERROR("connection peer has shutdown, please check and handle it!!!!!\n");
		else if (rs == -1)
			ERROR("recv snapshot error:%s", strerror(errno));
		goto err_exit;
	}

	return snapshot;

err_exit:
	if (snapshot) {
		free(snapshot);
		snapshot = NULL;
	}

	return snapshot;
}


player_snapshot_t *mozart_player_getsnapshot(player_handler_t *handler)
{
	int ret = -1;

	player_snapshot_t *snapshot = NULL;

	if (!handler)
		return NULL;

	player_command *cmd = create_player_command(handler->uuid, handler->ch_num, GET_SNAPSHOT, NULL);
	if (!cmd) {
		ERROR("Invalid cmd, Do nothing.\n");
		return NULL;
	}

	pthread_mutex_lock(&player_internal_mutex);
	ret = send_cmd(handler, cmd);
	if (!ret)
		snapshot = recv_player_snapshot(handler);
	pthread_mutex_unlock(&player_internal_mutex);

	destory_player_command(&cmd);

	return snapshot;
}

player_status_t mozart_player_getstatus(player_handler_t *handler)
{
	player_snapshot_t *snapshot = NULL;
	player_status_t status = PLAYER_UNKNOWN;
	if(!handler)
		return status;
	snapshot = mozart_player_getsnapshot(handler);
	if (snapshot) {
		status = snapshot->status;
		free(snapshot);
	}

	return status;
}

static player_url_t *recv_player_url(player_handler_t *handler)
{
	int rs = -1;
	int len = -1;
	player_url_t *url = NULL;

	if (!handler)
		goto err_exit;

recv_again:
	rs = recv(handler->fd_sync, &len, sizeof(len), 0);
	if (rs != sizeof(len)) {
		if (rs == 0)
			ERROR("connection peer has shutdown, please check and handle it!!!!!\n");
		else if (rs == -1) {
			if (errno == EINTR) {
				goto recv_again;
			} else {
				ERROR("recv url len error:%s", strerror(errno));
				goto err_exit;
			}
		}
	}

	if (len == 0) // url param is NULL, so return directly.
		return NULL;

	url = malloc(sizeof(*url) + len + 1);
	if (!url) {
		printf("Can not alloc memory for recv url: %s.\n", strerror(errno));
		goto err_exit;
	}
	memset(url, 0, sizeof(*url) + len + 1);
	url->len = len;

recv_again1:
	rs = recv(handler->fd_sync, url->url, url->len, 0);
	if (rs != url->len) {
		if (rs == 0)
			ERROR("connection peer has shutdown, please check and handle it!!!!!\n");
		else if (rs == -1) {
			if (errno == EINTR) {
				goto recv_again1;
			} else {
				ERROR("recv url error:%s", strerror(errno));
				goto err_exit;
			}
		}
	}

	return url;

err_exit:
	if (url) {
		free(url);
		url = NULL;
	}

	return url;
}

char *mozart_player_geturl(player_handler_t *handler)
{
	int ret = -1;
	if (!handler)
		return NULL;

	player_url_t *url_recv = NULL;
	char *url = NULL;

	player_command *cmd = create_player_command(handler->uuid, handler->ch_num, GET_URL, NULL);
	if (!cmd) {
		ERROR("Invalid cmd, Do nothing.\n");
		return NULL;
	}

	pthread_mutex_lock(&player_internal_mutex);
	ret = send_cmd(handler, cmd);
	if (!ret)
		url_recv = recv_player_url(handler);
	pthread_mutex_unlock(&player_internal_mutex);

	destory_player_command(&cmd);

	if (url_recv && url_recv->len > 0) {
		url = malloc(url_recv->len + 1);
		if (!url)
			return NULL;
		strncpy(url, url_recv->url, url_recv->len);

		free(url_recv);
		return url;
	}

	return NULL;
}

int mozart_player_getpos(player_handler_t *handler)
{
	player_snapshot_t *snapshot = NULL;
	int position = -1;

	snapshot = mozart_player_getsnapshot(handler);
	if (snapshot) {
		position = snapshot->position;
		//printf("[tinman-debug] run here: %s:%s:%d, getpos: %d.\n", __FILE__, __func__, __LINE__, position);
		free(snapshot);
	}

	return position;
}

int mozart_player_getduration(player_handler_t *handler)
{
	player_snapshot_t *snapshot = NULL;
	int duration = -1;

	snapshot = mozart_player_getsnapshot(handler);
	if (snapshot) {
		duration = snapshot->duration;
		//printf("[tinman-debug] run here: %s:%s:%d, duration: %d.\n", __FILE__, __func__, __LINE__, duration);
		free(snapshot);
	}

	return duration;
}

char *mozart_player_getuuid(player_handler_t *handler)
{
	player_snapshot_t *snapshot = NULL;
	char *uuid = NULL;

	snapshot = mozart_player_getsnapshot(handler);

	if (snapshot && strlen(snapshot->uuid) > 0) {
		uuid = malloc(strlen(snapshot->uuid) + 1);
		if (uuid)
			strcpy(uuid, snapshot->uuid);
		free(snapshot);
	}

	return uuid;
}

int mozart_player_resume(player_handler_t *handler)
{
	int rs = -1;

	if (!handler)
		return -1;

	player_command *cmd = create_player_command(handler->uuid, handler->ch_num, DO_RESUME, NULL);
	if (!cmd) {
		ERROR("Invalid cmd, Do nothing.\n");
		return -1;
	}

	rs = send_cmd(handler, cmd);

	destory_player_command(&cmd);

	return rs;
}

int mozart_player_pause(player_handler_t *handler)
{
	int rs = -1;

	if (!handler)
		return -1;

	player_command *cmd = create_player_command(handler->uuid, handler->ch_num, DO_PAUSE, NULL);
	if (!cmd) {
		ERROR("Invalid cmd, Do nothing.\n");
		return -1;
	}

	rs = send_cmd(handler, cmd);

	destory_player_command(&cmd);

	return rs;
}

int mozart_player_stop_wait_release(player_handler_t *handler, int val)
{
	int rs = -1;

	if (!handler)
		return -1;

	player_command *cmd = create_player_command(handler->uuid, handler->ch_num, WAIT_RELEASE, (void *)val);
	if (!cmd) {
		ERROR("Invalid cmd, Do nothing.\n");
		return -1;
	}

	rs = send_cmd(handler, cmd);

	destory_player_command(&cmd);

	return rs;
}

int mozart_player_stop(player_handler_t *handler)
{
	int rs = -1;

	if (!handler)
		return -1;

	player_command *cmd = create_player_command(handler->uuid, handler->ch_num, DO_STOP, NULL);
	if (!cmd) {
		ERROR("Invalid cmd, Do nothing.\n");
		return -1;
	}

	rs = send_cmd(handler, cmd);

	destory_player_command(&cmd);

	return rs;
}

int mozart_player_forcestop(player_handler_t *handler)
{
	int rs = -1;

	if (!handler)
		return -1;

	player_command *cmd = create_player_command(handler->uuid, handler->ch_num, KILL_MPLAYER, NULL);
	if (!cmd) {
		ERROR("Invalid cmd, Do nothing.\n");
		return -1;
	}

	rs = send_cmd(handler, cmd);

	destory_player_command(&cmd);

	return rs;
}

int mozart_player_seek(player_handler_t *handler, int position)
{
	printf("=============%s==============\n",__func__);
	int rs = -1;

	if (!handler)
		return -1;

	player_command *cmd = create_player_command(handler->uuid, handler->ch_num, DO_SEEK, (void *)position);
	if (!cmd) {
		ERROR("Invalid cmd, Do nothing.\n");
		return -1;
	}

	rs = send_cmd(handler, cmd);

	destory_player_command(&cmd);

	return rs;
}

int mozart_player_volset(player_handler_t *handler, int volume)
{
	bool feadback = false;
	int rs = -1;

	if (!handler)
		return -1;

	player_command *cmd = create_player_command(handler->uuid, handler->ch_num, SET_VOLUME, (void *)volume);
	if (!cmd) {
		ERROR("Invalid cmd, Do nothing.\n");
		return -1;
	}

	pthread_mutex_lock(&player_internal_mutex);
	rs = send_cmd(handler, cmd);
	if (!rs)
		feadback = recv_result_feadback(handler);
	pthread_mutex_unlock(&player_internal_mutex);

	destory_player_command(&cmd);

	return feadback ? 0 : -1;
}

int mozart_player_volget(player_handler_t *handler)
{
	// TODO: process soft volume.

	//return mozart_volume_get();   //ÔÝÊ±·Å×Å
	return 80;
}

int mozart_player_stopall(player_handler_t *handler, bool is_sync)
{
	int ret = -1;
	bool feadback = false;

	if (!handler)
		return -1;

	player_command *cmd = create_player_command(handler->uuid, handler->ch_num, DO_STOPALL, (void *)is_sync);
	if (!cmd) {
		ERROR("Invalid cmd, Do nothing.\n");
		return -1;
	}

	pthread_mutex_lock(&player_internal_mutex);
	ret = send_cmd(handler, cmd);
	if (!ret && is_sync)
		feadback = recv_result_feadback(handler);
	pthread_mutex_unlock(&player_internal_mutex);

	destory_player_command(&cmd);

	return feadback ? 0 : -1;
}

int mozart_player_pauseall(player_handler_t *handler, bool is_sync)
{
	int ret = -1;
	bool feadback = false;

	if (!handler)
		return -1;

	player_command *cmd = create_player_command(handler->uuid, handler->ch_num, DO_PAUSEALL, (void *)is_sync);
	if (!cmd) {
		ERROR("Invalid cmd, Do nothing.\n");
		return -1;
	}

	pthread_mutex_lock(&player_internal_mutex);
	ret = send_cmd(handler, cmd);
	if (!ret && is_sync)
		feadback = recv_result_feadback(handler);
	pthread_mutex_unlock(&player_internal_mutex);

	destory_player_command(&cmd);

	return feadback ? 0 : -1;
}

int mozart_player_resumeall(player_handler_t *handler, bool is_sync)
{
	int ret = -1;
	bool feadback = false;

	if (!handler)
		return -1;

	player_command *cmd = create_player_command(handler->uuid, handler->ch_num, DO_RESUMEALL, (void *)is_sync);
	if (!cmd) {
		ERROR("Invalid cmd, Do nothing.\n");
		return -1;
	}

	pthread_mutex_lock(&player_internal_mutex);
	ret = send_cmd(handler, cmd);
	if (!ret && is_sync)
		feadback = recv_result_feadback(handler);
	pthread_mutex_unlock(&player_internal_mutex);

	destory_player_command(&cmd);

	return feadback ? 0 : -1;
}

static int simple_play_send_request(char *mp3_src, uint32_t opt, int fade_msec, int fade_volume)
{
	union {
		simple_request_t request;
		simple_respond_t respond;
	} msg;

	ssize_t rByte, wByte;
	int sock_fd;
	int err = 0;

	memset(&msg, 0, sizeof(msg));

	sock_fd = connect_to_unix_socket(TINY_SOCK);
	if (sock_fd < 0) {
		ERROR("connect to player server failed: %s.\n", strerror(errno));
		return -1;
	}

	if (mp3_src) {
		if (strlen(mp3_src) < 4096) {
			strcpy(msg.request.url, mp3_src);
		} else {
			ERROR("the length of url limit to 4096,please check!");
			err = -1;
			goto err_send;
		}
	}else
		msg.request.url[0] = 0;

	msg.request.opt		= opt;
	msg.request.fade_msec	= fade_msec;
	msg.request.fade_volume	= fade_volume;

	wByte = send(sock_fd, &msg.request, sizeof(simple_request_t), 0);
	if (wByte < 0 || wByte != sizeof(simple_request_t)) {
		ERROR("Send simple play request: %s", strerror(errno));
		err = -1;
		goto err_send;
	}

	rByte = recv(sock_fd, &msg.respond, sizeof(simple_respond_t), 0);
	if (rByte < 0) {
		ERROR("Receive simple reply: %s", strerror(errno));
		err = -1;
		goto err_recv;
	}

	if (rByte == 0)
		DEBUG("Simple play disconnect by server\n");

	if (msg.respond.ok == -1) {
		ERROR("Simple play report failed");
		err = -1;
	} else {
		err = 0;
	}

err_recv:
err_send:
	close(sock_fd);

	return err;
}

int mozart_player_playmp3(char *mp3_src, int fade_msec, int fade_volume, int is_sync)
{
	uint32_t opt = 0;

	if (is_sync)
		opt |= SO_SYNC;

	return simple_play_send_request(mp3_src, opt, fade_msec, fade_volume);
}

int mozart_player_stopmp3(int is_sync)
{
	uint32_t opt = SO_STOP;

	if (is_sync)
		opt |= SO_SYNC;

	return simple_play_send_request(NULL, opt, 0, 0);
}


