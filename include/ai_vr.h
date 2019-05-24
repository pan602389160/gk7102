#ifndef __AI_VR_H__
#define __AI_VR_H__
#include <stdbool.h>


#include "aec.h"

#include "asr.h"
//#include "tts.h"
#include "content.h"


typedef enum vr_from_e{
	VR_FROM_AEC,
	VR_FROM_ASR,
	VR_FROM_CONTENT
} vr_from_e;

typedef struct vr_info_t {
	bool stop;

	vr_from_e from;
	union{
		aec_info_t aec;
		asr_info_t asr;
		content_info_t content;
	};

} vr_info_t;

extern vr_info_t ai_vr_info;

typedef int (*vr_callback)(vr_info_t *vr_info);

extern vr_callback ai_vr_callback;

extern int ai_vr_init(vr_callback _callback);


extern int ai_vr_uninit(void);

extern const char *vr_domain_str[];

#endif

