#ifndef __APPSERVER_INTERFACE_H__
#define __APPSERVER_INTERFACE_H__

#include <netinet/in.h>
struct appserver_cli_info {
	struct sockaddr_in cli_in;
	int client_fd;
};

/**
 * @brief CMD callback.
 *
 * @param scope [in] function name
 * @param command [in] command name
 * @param data [in] command info
 * @param client_fd [in] connect client fd
 *
 * @return On success returns 0, return -1 if an error occurred.
 */
typedef int (*appserver_callback)(char *scope, char *command, char *data,
		struct appserver_cli_info *owner);

/**
 * @brief register appserver callback(and create broadcast deviceinfo thread).
 *
 * @param max_cmd_len [in] max length of command
 * @param callback [in] cmd callback
 *
 * @return On success returns 0, return -1 if an error occurred.
 */
extern int mozart_appserver_register_callback(long max_cmd_len, appserver_callback callback);

/**
 * @brief close appserver.
 *
 * @return On success returns 0, return -1 if an error occurred.
 */
extern int mozart_appserver_shutdown(void);

/**
 * @brief cmd response.
 *
 * @param command [in] command name
 * @param data [in] command info
 * @param client_fd [in] connect client fd
 *
 * @return On success returns 0, return -1 if an error occurred.
 */
extern int mozart_appserver_response(char *command, char *data, struct appserver_cli_info *owner);

/**
 * @brief cmd notify.
 *
 * @param command [in] command name
 * @param data [in] command info
 *
 * @return On success returns 0, return -1 if an error occurred.
 */
extern int mozart_appserver_notify(char *command, char *data);

/**
 * @brief custom devicename, default format: { "name": "SmartAudio-a2848fc6", "ip": "192.168.40.94", "mac": "94:a1:a2:84:8f:c6" }
 *
 * @param devicename [in] new device name, devicename should < 32Bytes.
 *
 * @return return the whole deviceinfo on success, return NULL otherwise.
 */
extern char *mozart_appserver_rename(char *devicename);

#endif /** __APPSERVER_INTERFACE_H__ **/
