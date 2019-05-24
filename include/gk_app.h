#ifndef _GK_APP_H_
#define _GK_APP_H_
#include <pthread.h>

/**
 * @brief DEBUGæ‰“å°ä¿¡æ¯, ä¸€èˆ¬åœ¨è°ƒè¯•æ—¶æ‰“å¼€ï¼Œ æ­£å¼ç‰ˆæœ¬ä¼šå±è”½æŽ‰.
 */
#define __DEBUG__

#ifdef __DEBUG__
/**
 * @brief æ˜¯å¦æ˜¾ç¤ºæ‰€æœ‰DEBUGæ‰“å°ä¿¡æ¯
 */
/* #define DEBUG_SHOW_ALL */


#define DEBUG(format, ...) printf("[%s : %s : %d] ",__FILE__,__func__,__LINE__); printf(format, ##__VA_ARGS__);

#define DEBUG_FUNC_START printf("++++++++++++++++++++++++++++++++ %s\n",__func__);
#define DEBUG_FUNC_END		   printf("-------------------------------- %s\n",__func__);

#else
#define DEBUG(format, ...)
#define DEBUG_FUNC_START
#define DEBUG_FUNC_END
#endif

/**
 * @brief å¼‚å¸¸æ‰“å°ä¿¡æ¯.
 */
#define PERROR(format, ...) printf("[%s : %s : %d] ",__FILE__,__func__,__LINE__); printf(format, ##__VA_ARGS__);

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief ÓïÒô½»»»µÄ»¥³âËø. \n
 * 	Ä¿Ç°»½ÐÑºÍÊ¶±ð¶¼²ÉÓÃÍ¬Ò»¸öcallback£¬ÎªÁË±£»¤callback·µ»Ø²»±»ÖØÈë£¬ÔÚµ÷ÓÃcallbackÇ°¼ÓËø£¬callbackÍêºó½âËø.
 */

extern pthread_mutex_t ai_lock;
#define ai_mutex_lock()				\
	do {								\
		int i = 0;			\
		while (pthread_mutex_trylock(&ai_lock)) {			\
			if (i++ >= 100) {				\
				PERROR("####dead lock####\n");	\
				i = 0;	\
			}			\
			usleep(100 * 1000);				\
		}							\
	} while (0)

#define ai_mutex_unlock() 	\
	do {								\
		pthread_mutex_unlock(&ai_lock);\
	} while (0)

#ifdef __cplusplus
}
#endif
#endif

