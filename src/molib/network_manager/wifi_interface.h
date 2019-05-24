/**
 * @file wifi_interface.h
 * @brief network_manager interface file
 * @author ylguo <yonglei.guo@ingenic.com>
 * @version 0.1
 * @date 2015-03-23
 *
 * Copyright (C) 2014 Ingenic Semiconductor Co., Ltd.
 *
 * The program is not free, Ingenic without permission,
 * no one shall be arbitrarily (including but not limited
 * to: copy, to the illegal way of communication, display,
 * mirror, upload, download) use, or by unconventional
 * methods (such as: malicious intervention Ingenic data)
 * Ingenic's normal service, no one shall be arbitrarily by
 * software the program automatically get Ingenic data
 * Otherwise, Ingenic will be investigated for legal responsibility
 * according to law.
 */

#ifndef __WIFI_INTERFACE_H__
#define __WIFI_INTERFACE_H__

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * @brief network event type
 */
typedef enum EVENT{
	/*
	 * @brief wifi mode changes(example: AP, STA, AIRKISS, WIFI_NULL).
	 */
	WIFI_MODE,
	/*
	 * @brief sta mode status(example: SCANNING, ASSOCIATED, COMPLETED,etc).
	 */
	STA_STATUS,
	/*
	 * @brief ap mode status(example: CLIENT CONNECT or DISCONNECT,etc).
	 */
	AP_STATUS,
	/*
	 * @brief network configure status(exp:AIRKISS_SUCCESS or AIRKISS_FAILED).
	 */
	NETWORK_CONFIGURE,
	/*
	 * @brief ethernet mode changes(example: ETHER_CONNECT, ETHER_DISCONECT).
	 */
	ETHER_STATUS,
	/*
	 * @brief sta ip addr change.
	 */
	STA_IP_CHANGE,
	/*
	 * @brief ethernet ip addr change.
	 */
	ETH_IP_CHANGE,
	/*
	 * @brief invalid event.
	 */
	INVALID
} event_notice_t;
/**
 * @brief string format of event type, for readable
 */


/**
 * @brief wpa_supplicant config file status.
 */
typedef enum sta_error{
	/*
	 * @brief wpa configure file no exist.
	 */
	NONEXISTENCE_WPAFILE,
	/*
	 * @brief Non effective ssid for wpa configure file.
	 */
	NON_VALID_SSID,
	/*
	 * @brief Station connect timeout.
	 */
	CONNECT_TIMEOUT,
	/*
	 * @brief wrong password.
	 */
	WRONG_KEY,
	/*
	 * @brief sta disconnected
	 */
	STA_DISCONNECTED,
	/*
	 * @briief scan ssid failed.
	 */
	SCAN_SSID_FAILED,
	/*
	 * @brief obtain ip addr timeout.
	 */
	GET_IP_TIMEOUT
} sta_error_t;
/**
 * @brief string format of station error, for readable
 */
extern const char *sta_error_str[];

/**
 * @brief network config failed reason.
 */
typedef enum netcfg_error{
	/*
	 * @brief Invalid passphrase length(expected: 8..63)
	 */
	INVALID_PWD_LEN,
	/*
	 * @brief network configure timeout
	 */
	CONFIGURE_TIMEOUT
} netcfg_error_t;
/**
 * @brief string format of netcfg error, for readable
 */


/**
 * @brief network configure mode status
 */
typedef enum airkiss_operation {
	AIRKISS_STARTING,
	AIRKISS_RUNNING,
	AIRKISS_SUCCESS,
	AIRKISS_FAILED,
	AIRKISS_SUCCESS_OVER,
	AIRKISS_FAILED_OVER,
	AIRKISS_NULL,
	AIRKISS_CANCEL
} airkiss_operate_t;
/**
 * @brief string format of network configure, for readable
 */


/**
 * @brief network configure info for success.
 */
typedef struct netcfg_info {
	/*
	 *@brief ssid.
	 */
	char ssid[64];
	/*
	 *@brief password.
	 */
	char passwd[65];
	/*
	 *@brief sender ip address.
	 */
	char ip[16];
} netcfg_info_t;

/**
 * @brief wifi mode status
 */
typedef enum wifi_mode {
	/**
	 * @brief wifi is in AP mode
	 */
	AP,
	/**
	 * @brief wifi is in STA mode
	 */
	STA,
	/**
	 * @brief wifi is in AIRKISS mode
	 */
	AIRKISS,
	/**
	 * @brief wifi is in STA(with internet) mode
	 */
	STANET,
	/**
	 * @brief wifi is connecting to a STAtion.
	 */
	STA_WAIT,
	/**
	 * @brief wifi is creating AP.
	 */
	AP_WAIT,
	/**
	 * @brief wifi is not in AP mode or STA mode.
	 */
	WIFI_NULL
} wifi_mode_t;
/**
 * @brief readable string format of wifi_mode, NOTE: maintainer of network_manager should keep pace with `enum wifi_mode`!!!
 */
extern const char *wifi_mode_str[];



typedef enum ethernet_status {
	/*
	 *@brief ethernet line is offline
	 */
	ETHER_NULL,
	/*
	 *@brief ethernet line is online, but ethernet is disconnected
	 */
	ETHER_DISCONNECT,
	/*
	 *@brief ethernet is obtaining ip
	 */
	ETHER_WAIT,
	/*
	 *@brief ethernet is connected and have already obtained ipaddr
	 */
	ETHER_CONNECT
} ether_status_t;

extern const char *ether_status_str[];


typedef struct wifi_info{
	/*
	 * Concurrent WiFi Working mode.
	 */
	wifi_mode_t wifi_mode;
	/*
	 * current ehternet working status.
	 */
	ether_status_t ether_status;
	/*
	 * Ap or Softap ssid name.
	 */
	char ssid[64];
	/*
	 * @brief bssid of wifi router to be connected,
	 */
	char bssid[17];
} wifi_info_t;

/**
 * @brief From Network-Manager Notice Event.
 */
typedef struct event_info{
	/*
	 * Event name, wifi is usually.
	 */
	char name[8];
	/*
	 * Event type.
	 */
	char type[32];
	/*
	 * Event content.
	 */
	char content[32];
	char reason[32];
	/*
	 * Additional information.
	 */
	void *param;
} event_info_t;

/**
 * @brief wifi-get and wifi-set command.
 */
typedef enum wifi_cmd {
	/**
	 * @brief get current wifi mode(refer to wifi_mode_t).
	 */
	CHECKMODE,
	/**
	 * @brief switch to ap mode.
	 */
	SW_AP,
	/**
	 * @brief switch to sta mode.
	 */
	SW_STA,
	/**
	 * @brief switch to low priority network for wpaconfig file.
	 */
	SW_NEXT_NET,
	/**
	 * @brief switch to airkiss mode.
	 */
	SW_NETCFG,
	/**
	 * @brief close wifi.
	 */
	STOP_WIFI,
	/*
	 *@brief connect to ethernet and obtain ip for ehternet
	 */
	STARTUP_ETHERNET,
	/*
	 *@brief disconnect ethernet and release ip and delete route of ehternet
	 */
	STOP_ETHERNET,
	/*
	 *@brief set hostname when obtaining ip.
	 */
	SET_HOSTNAME,
	/**
	 * @brief create wifi virtual device.
	 */
	SW_VIRTUAL_DEV
} wifi_cmd_t;
/**
 * @brief readable string format of wifi_cmd, NOTE: maintainer of network_manager should keep pace with `enum wifi_cmd`!!!
 *
 */
extern const char *wifi_cmd_str[];

typedef struct switch_station_cmd_param {
	/**
	 * @brief ssid of wifi to be connected;
	 * ONLY used in ap/sta config mode; memset to 0 on ap/sta switch mode.
	 */
	char ssid[64];
	/**
	 * @brief password of wifi to be connected,
	 * ONLY used in ap/sta config mode, memset to 0 on ap/sta switch mode.
	 */
	char psk[128];
	/**
	 * @brief bssid of wifi router to be connected,
	 * ONLY used in sta config mode, memset to 0 on ap/sta switch mode.
	 */
	char bssid[17];
	/*
	 * @brief sta mode timeout(unit: Second), ONLY used in sta switch/config mode.
	 */
	int sta_timeout;
	/*
	 * @brief obtain ip address timeout(unit: Second), ONLY used in sta switch/config mode.
	 */
	int ip_timeout;
} station_t;

typedef struct switch_softap_cmd_param {
	/**
	 * @brief ssid of wifi to be connected;
	 * ONLY used in ap/sta config mode; memset to 0 on ap/sta switch mode.
	 */
	char ssid[64];
	/**
	 * @brief password of wifi to be connected,
	 * ONLY used in ap/sta config mode, memset to 0 on ap/sta switch mode.
	 */
	char psk[128];
	/**
	 * @brief bssid of wifi router to be connected,
	 * ONLY used in sta config mode, memset to 0 on ap/sta switch mode.
	 */
	char bssid[17];
} softap_t;

typedef enum netcfg_method {
	/**
	 * @brief atalk method for alibaba.
	 */
	ATALK = 0x01,
	/**
	 * @brief softap method for alibaba.
	 */
	SOFTAP = 0x02,
	/**
	 * @brief airkiss(wechat) method for other wifi module(exp:relatek or braodcom, Use WeChat provide airkiss static library).
	 */
	AIRKISS_WE = 0x04,
	/**
	 * @brief cooee and airkiss method for broadcom.
	 */
	COOEE = 0x08,
	/**
	 * @brief simple config method for realtek.
	 */
	SIMPLE_CONFIG = 0x10,
	/**
	 * @brief joylink method for JD.
	 */
	JOYLINK = 0x20,
	/**
	 * @brief Wi-Fi Protected Setup or QSS.
	 */
	WPS = 0x40
} netcfg_method_t;

typedef enum wifi_module {
	/**
	 * @brief Based on the development of broadcom wifi.
	 */
	BROADCOM,
	/**
	 * @brief Based on the development of realtek wifi.
	 */
	REALTEK,
	/**
	 * @brief Based on the development of southsv wifi.
	 */
	SOUTHSV
} wifi_module_t;
/**
 * @brief string format of wifi module.
 */
extern const char *wifi_module_str[];

typedef struct switch_network_config_cmd_param {
	/*
	 * @brief network configure timeout(unit: s, default is 60s).
	 */
	int timeout;
	/*
	 * @brief channel_holding_time is the time interval between channels switch(unit: ms, dafault is 400ms, not very large generally).
	 */
	int channel_holding_time;
	/*
	 * @brief smartlink key for QQ connect or Airkiss.
	 */
	char key[32];
	/*
	 * @brief Product Mode for Alibaba Network Configure.
	 */
	char product_model[64];
	/*
	 * @brief .
	 */
	char wl_module[16];
	/*
	 * @brief the network configuration method in keeping with Manufacture.
	 */
	char method;
	/*
	 * @brief device id for airkiss LAN discover.
	 */
	char device_id[32];
	/*
	 * @brief device type for airkiss LAN discover.
	 */
	char device_type[32];
} network_config_t;

/**
 * @brief wifi request ctrl msg
 */
typedef struct wifi_ctl_msg_s {
	/**
	 * @brief the pid of client, should equal to client_register->pid
	 */
	pid_t pid;
	/**
	 * @brief name of client, should euqal to client_register-->name
	 * in the same client.
	 */
	char name[32];
	/**
	 * @brief wifi request command.
	 */
	wifi_cmd_t cmd;
	/**
	 * @brief if you wish to force to switch wifi mode if the current wifi mode is the same as desired.
	 */
	bool force;
	/**
	 * @brief the different parameters of different commands.
	 */
	union{
		/**
		 * @brief set ip hostname.
		 */
		char hostname[64];
		/**
		 * @brief switch station func param for SW_STA cmd.
		 */
		station_t switch_sta;
		/**
		 * @brief switch softap func param for SW_AP cmd.
		 */
		softap_t switch_ap;
		/**
		 * @brief network configure param for SW_NETCFG cmd.
		 */
		network_config_t network_config;
	} param;
} wifi_ctl_msg_t;

/**
 * @brief register infomation to network_manager.
 */
typedef struct wifi_client_register {
	/**
	 * @brief the pid of client
	 */
	pid_t pid;
	/**
	 * @brief Reserved.
	 */
	int reset;
	/**
	 * @brief priority of client, Reserved.
	 */
	int priority;
	/**
	 * @brief the unique name of the client.
	 */
	char name[32];
	/**
	 * @brief remove from cli_list
	 */
	int remove;
} WIFI_CLI_REG;

/**
 * @brief get current wifi mode,
 * Do not need to register firstly, you can invoke it in any where.
 *
 * @return  return current wifi mode
 */
extern wifi_info_t get_wifi_mode(void);

/**
 * @brief request a wifi mode
 *
 * @param msg [in] wifi request msg
 *
 * @return return true on Success, false on failure.
 */
extern bool request_wifi_mode(wifi_ctl_msg_t msg);

/**
 * @brief register to network_manager
 *
 * @param register_info [in] the client's description.
 * @param ptr [in] callback func on wifi mode changed.
 *
 * @return return 0 on Success, -1 on failure.
 */
extern int register_to_networkmanager(WIFI_CLI_REG register_info, int (*ptr)(const char *p));

/**
 * @brief unregister from network_manager
 */
//extern int unregister_from_networkmanager(void);

/**
 * @brief get wifi scan result.
 *
 * @return wifi info with json string format, should free it with free() after using done.
 */
extern char *get_wifi_scanning_results(void);

#ifdef  __cplusplus
}
#endif

#endif /* __WIFI_INTERFACE_H__ */
