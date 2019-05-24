#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>

#include "json-c/json.h"

#include "appserver.h"
#include "ingenicplayer.h"
#if (SUPPORT_ALARM == 1)
#include "alarm_interface.h"
#endif

#if (SUPPORT_ALARM == 1)
#define BUF_SIZE 128

static char *alarm_cmd[] = {
	[ADD_ALARM] = "add_alarm",
	[DELETE_ALARM] = "delete_alarm",
	[UPDATE_ALARM] = "update_alarm",
	[GET_ALARM_LIST] = "get_alarm_list",
};
#endif

enum {
	INGENICPLAYER_SCOPE = 0,
	ALARM_SCOPE,
	DEVICE_SCOPE,
};

static char *appserver_cmd[] = {
	[INGENICPLAYER_SCOPE] = "ingenicplayer",
	[ALARM_SCOPE] = "alarm",
	[DEVICE_SCOPE] = "device",
};

#if (SUPPORT_ALARM == 1)
static int get_name_by_pid(pid_t pid, char *process_name, int len)
{
	FILE *fp;
	char proc_pid_path[BUF_SIZE];
	char process_cmdline[BUF_SIZE] = {0};

	sprintf(proc_pid_path, "/proc/%d/cmdline", pid);

	fp = fopen(proc_pid_path, "r");
	if (NULL != fp) {
		if (fgets(process_cmdline, BUF_SIZE - 1, fp) == NULL) {
			fclose(fp);
			return -1;
		}
		fclose(fp);
	}

	memset(process_name, 0, len);
	snprintf(process_name, len, basename(process_cmdline));

	return 0;
}

static struct alarm_info *get_alarm_info(char *data)
{
	int len = 0;
	struct alarm_info *alarm = NULL;
	json_object *object = NULL;
	json_object *tmp = NULL;

	if ((object = json_tokener_parse(data)) == NULL)
		return alarm;
	if (json_object_object_get_ex(object, "alarm_private_info", &tmp))
		if (json_object_object_get_ex(tmp, "private_info", &tmp))
			len = strlen(json_object_get_string(tmp));

	alarm = calloc(sizeof(struct alarm_info) + len + 1, 1);

	if (tmp)
		strncpy(alarm->prv_info.info, json_object_get_string(tmp), len);
	alarm->prv_info.len = len;
	if (json_object_object_get_ex(object, "hour", &tmp))
		alarm->hour = json_object_get_int(tmp);
	if (json_object_object_get_ex(object, "minute", &tmp))
		alarm->minute = json_object_get_int(tmp);
	if (json_object_object_get_ex(object, "week_active", &tmp))
		alarm->week_active = json_object_get_int(tmp);
	if (json_object_object_get_ex(object, "weekly_repeat", &tmp))
		alarm->weekly_repeat = json_object_get_int(tmp);
	if (json_object_object_get_ex(object, "enable", &tmp))
		alarm->enable = json_object_get_int(tmp);
	if (json_object_object_get_ex(object, "alarm_id", &tmp))
		alarm->alarm_id = json_object_get_int(tmp);
	if (json_object_object_get_ex(object, "timestamp", &tmp))
		alarm->timestamp = json_object_get_int(tmp);
	get_name_by_pid(getpid(), alarm->name, 32);

	return alarm;
}

static void appserver_alarm_reply(char *command, struct appserver_cli_info *owner)
{
#if 0
	char *alarm_list = NULL;
	char *cmd_json = NULL;

	json_object *reply_object = json_object_new_object();
	if (!reply_object) {
		printf("%s %d : %s\n", __func__, __LINE__, strerror(errno));
		return;
	}

	if (strcmp(alarm_cmd[GET_ALARM_LIST], command) == 0) {
		alarm_list = mozart_alarm_get_list();
		json_object_object_add(reply_object, "alarm_list", json_tokener_parse(alarm_list));
		free(alarm_list);
	}

	cmd_json = strdup(json_object_get_string(reply_object));
	if (cmd_json != NULL) {
		mozart_appserver_response(command, cmd_json, owner);
		free(cmd_json);
	}

	json_object_put(reply_object);
#endif
	return;
}

static int mozart_alarm_response_cmd(char *command, char *data, struct appserver_cli_info *owner)
{
	struct alarm_info *alarm = NULL;

	if (strcmp(alarm_cmd[GET_ALARM_LIST], command) != 0) {
		if ((alarm = get_alarm_info(data)) != NULL) {
			if (strcmp(alarm_cmd[ADD_ALARM], command) == 0)
				mozart_alarm_add(alarm);
			else if (strcmp(alarm_cmd[DELETE_ALARM], command) == 0)
				mozart_alarm_delete(alarm);
			else if (strcmp(alarm_cmd[UPDATE_ALARM], command) == 0)
				mozart_alarm_update(alarm);
			free(alarm);
		}
	}

	appserver_alarm_reply(command, owner);

	return 0;
}
#endif


int appserver_cmd_callback(char *scope, char *command, char *data, struct appserver_cli_info *owner)
{	//printf("=======appserver_cmd_callback===========scope = %s, command = %s,data = %s\n",scope,command,data);
	int ret = 0;
	
	if(strcmp("usercommand", command) == 0){//usercommand
		ret = mozart_ingenicplayer_response_cmd(scope, data, owner);
		return ret;
	}
	return ret;
}
