#ifndef __TCP_H__
#define __TCP_H__

int get_unix_serv_socket(char *un_path);
int connect_to_unix_socket(char *un_path);

#endif
