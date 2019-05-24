#ifndef __ASR_INTERFACE_H__
#define __ASR_INTERFACE_H__

#include <stdlib.h>
#include "ai_vr.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef enum {

	VR_IDLE,
	VR_AEC,

	VR_ASR,

	VR_CONTENT_GET,
} vr_status_t;


typedef enum {

	VR_TO_NULL,

	VR_TO_AEC,

	VR_TO_ASR,

	VR_TO_ASR_SDS,

	VR_TO_WAIT,
} vr_target_t;

typedef struct {
	vr_target_t vr_next;
} vr_result_t;


typedef vr_result_t (*ray_vr_callback)(vr_info_t *info);

extern int ray_vr_init(ray_vr_callback callback);

extern int ray_vr_start(void);

extern int ray_vr_stop(void);

extern int ray_vr_uninit(void);

extern int ray_vr_aec_wakeup(void);

extern int ray_vr_asr_break(void);

extern vr_status_t ray_vr_get_status(void);

extern int ray_vr_content_get(char *keyword);

extern int ray_vr_content_get_interrupt(void);

extern char *ray_aispeech_tts(char *word);

extern void ray_aispeech_tts_stop(void);





#ifdef __cplusplus
}
#endif

#endif 

