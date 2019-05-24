#ifndef __APPSERVER_H__
#define __APPSERVER_H__

#include "appserver_interface.h"

extern int appserver_cmd_callback(char *scope, char *command, char *data, struct appserver_cli_info *owner);

extern int mozart_device_response_cmd(char *command, char *data, struct appserver_cli_info *owner);

#endif
