#ifndef _CONTENT_H_
#define _CONTENT_H_

#include "asr.h"

#define CONTENT_TIMEOUT 10


#define CONTENT_RANDOM_MUSIC "我要听歌"



typedef enum {

	CONTENT_IDLE,

	CONTENT_START,

	CONTENT_SUCCESS,

	CONTENT_FAIL,

	CONTENT_INTERRUPT,

	CONTENT_MAX
} content_state_e;


typedef struct content_info_t {

	content_state_e state;

	int errId;

	char *error;

	sds_info_t sds;

} content_info_t;


extern void content_stop(void);
extern void content_interrupt(void);


extern int content_get(char *text);

extern void content_free(content_info_t *content_info);

#endif

