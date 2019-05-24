#ifndef __CURL_ECHOCLOUD_POST_H
#define __CURL_ECHOCLOUD_POST_H

//int libcurl_main(void);
//int mqtt_main(void);


typedef struct stream_info 
{
	char content_type[32];
	char x_echocloud_type[64];
	char x_echocloud_version[8];
}stream_info_t;

typedef struct device_info {
	char mac_addr[24];
	char fw_version[24];
	char ch_uuid[40];
	char dev_id[40];
	char hash_key[64];
}device_info_t;


#endif

