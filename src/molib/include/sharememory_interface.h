/**
 * @file sharememory_interface.h
 * @brief For the operation of the shared memory API
 * @author xllv <xinliang.lv@ingenic.com>
 * @version 1.0.0
 * @date 2015-01-12
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
#ifndef __SHARE_MEMORY_HEAD__
#define __SHARE_MEMORY_HEAD__

#ifdef  __cplusplus
extern "C" {
#endif
	/**
	 * @brief  Said the state value of the corresponding domain.
	 */
	typedef enum {
		/**
		 * @brief error
		 */
		STATUS_ERROR = -1,

		/**
		 * @brief Indicate the application is closed  [0]
		 */
		STATUS_SHUTDOWN, //0 

		/**
		 * @brief The application has been launched, waiting for client  [1]
		 * connection.
		 */
		STATUS_RUNNING,  // 1

		/**
		 * @brief Device in the extracted state. [2]
		 */
		STATUS_EXTRACT,  // 2

		/**
		 * @brief Device in the inserted state. [3]
		 */
		STATUS_INSERT,   // 3

		/**
		 * @brief Links have been established with the client, in the playing state. [4]
		 */
		STATUS_PLAYING, // 4

		/**
		 * @brief Links have been established with the client, in the pause state. [5]
		 */
		STATUS_PAUSE, //5

		/**
		 * @brief wait for event response done.
		 */
		WAIT_RESPONSE, // 6

		/**
		 * @brief event response done.
		 */
		RESPONSE_DONE,  // 7

		/**
		 * @brief event response done, but event source should pause.
		 */
		RESPONSE_PAUSE,

		/**
		 * @brief event response done, but failure.
		 */
		RESPONSE_CANCEL,

		/**
		 * @brief status cnt
		 */
		STATUS_MAX,
	} module_status;

	/**
	 * @brief  The index for the shared memory operations.
	 */
	typedef enum {
		UNKNOWN_DOMAIN,		/**< unknown domain */

		SDCARD_DOMAIN,		/**< SD card plug state domain */

		UDISK_DOMAIN,		/**< UDISK plug state domain */

		RENDER_DOMAIN,		/**< render state domain */

		AIRPLAY_DOMAIN,		/**< airplay state domain */

		ATALK_DOMAIN,		/**< atalk state domain */

		LOCALPLAYER_DOMAIN,	/**< localplayer state domain */

		VR_DOMAIN,		/**< voice recognition state domain*/

		BT_HS_DOMAIN,		/**< bluetooth phone call domain */

		BT_AVK_DOMAIN,		/**< bluetooth music domain */

		LAPSULE_DOMAIN,		/**< lapsule domain */

		TONE_DOMAIN,		/**< tone domain(tone cnt, not play status) */

		MUSICPLAYER_DOMAIN,	/**< musicplayer state domain */

		CUSTOM_DOMAIN,		/**< custom domain */

		MAX_DOMAIN,			/**< domain cnt */

	} memory_domain;

	/**
	 * @brief By enumeration value obtain the corresponding enumeration string
	 * Generally, it used only for debug.
	 */
	extern char *module_status_str[];

	/**
	 * @brief how many status now.
	 * Generally, it used only for debug.
	 */
	extern int get_status_cnt(void);

	/**
	 * @brief According to the index number of shared memory, to get the string of corresponding domain
	 * Generally, it used only for debug.
	 */
	extern char *memory_domain_str[];

	/**
	 * @brief how many domains now.
	 * Generally, it used only for debug.
	 */
	extern int get_domain_cnt(void);

	/**
	 * @brief  Shared memory initialization, before operating the shared memory
	 * and must be called once.
	 *
	 * @return On success returns 0, return -1 if an error occurred.
	 */
	extern int share_mem_init(void);

	/**
	 * @brief  Empty shared memory, shared memory for all domains are set to zero.
	 *
	 * @return On success returns 0, return -1 if an error occurred.
	 */
	extern int share_mem_clear(void);

	/**
	 * @brief  Destruction of shared memory, after each call share_mem_init,
	 * when no longer in use sharememory API, call the function.
	 *
	 * @return On success returns 0, return -1 if an error occurred.
	 */
	extern int share_mem_destory(void);

	/**
	 * @brief  Gets the value of the specified index field.
	 *
	 * @param domain [in]	The specified index
	 * @param status [out]	State value
	 *
	 * @return On success returns 0, return -1 if an error occurred.
	 */
	extern int share_mem_get(memory_domain domain, module_status * status);

	/**
	 * @brief  Gets the value of the specified index field.
	 *
	 * @param domain [in]	The specified index
	 *
	 * @return On success returns module status, return STATUS_ERROR if an error occurred.
	 */
	extern module_status share_mem_statusget(memory_domain domain);

	/**
	 * @brief  Set the value of the specified index field.
	 *
	 * @param domain [in]	The specified index
	 * @param status [in]	State value
	 *
	 * @return On success returns 0, return -1 if an error occurred.
	 */
	extern int share_mem_set(memory_domain domain, module_status status);

	/**
	 * @brief  Get the current active application.
	 * @attention Applications in pause or application is using DSP
	 * are all active. If no application is active,
	 * you will get an unknown domain
	 *
	 * @param domain [out]	Application of active
	 *
	 * @return On success returns 0, return -1 if an error occurred.
	 */
	extern int share_mem_get_active_domain(memory_domain * domain);
#ifdef  __cplusplus
}
#endif
#endif				/* __SHARE_MEMORY_HEAD__ */
