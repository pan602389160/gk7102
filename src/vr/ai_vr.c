#include <stdio.h>
#include <unistd.h>

#include "json-c/json.h"
#include "gk_app.h"
#include "ray_app.h"
#include "ai_vr.h"
#include "content.h"

const char *vr_domain_str[] = {
	[DOMAIN_NULL] = "DOMAIN_NULL",
	[DOMAIN_MOVIE] = "DOMAIN_MOVIE",
	[DOMAIN_MUSIC] = "DOMAIN_MUSIC",
	[DOMAIN_WEATHER] = "DOMAIN_WEATHER",
	[DOMAIN_COMMAND] = "DOMAIN_COMMAND",
	[DOMAIN_CALENDAR] = "DOMAIN_CALENDAR",
	[DOMAIN_REMINDER] = "DOMAIN_REMINDER",
	[DOMAIN_NETFM] = "DOMAIN_NETFM",
	[DOMAIN_CHAT] = "DOMAIN_CHAT",
	[DOMAIN_STORY] = "DOMAIN_STORY",
	[DOMAIN_JOKE] = "DOMAIN_JOKE",
	[DOMAIN_POETRY] = "DOMAIN_POETRY",
	[DOMAIN_COOKBOOK] = "DOMAIN_COOKBOOK",
	[DOMAIN_STOCK] = "DOMAIN_STOCK",
	[DOMAIN_TRANSLATE] = "DOMAIN_TRANSLATE",
	[DOMAIN_MAX] = "DOMAIN_UNSUPPORY"
};




vr_info_t ai_vr_info;
vr_callback ai_vr_callback;
pthread_mutex_t ai_lock;
#define TMP_BUFFER_SZ 1024
struct aiengine *sds_agn = NULL;
int ai_vr_init(vr_callback _callback)
{
	int ret = start_dds();
	if(ret)
		goto ERROR;
	ret = ray_wakeup_new(); 
	if(ret)
		goto ERROR;
	ret = ray_vad_start();
	if(ret)
		goto ERROR;
	ret = ray_echo_start();
	if(ret)
		goto ERROR;
	#if 1	

	ai_mutex_lock();
	ai_vr_callback = _callback;
	ai_mutex_unlock();
	DEBUG("SUCCESSFULLY init vr!!\n");
	return 0;
ERROR:
	return -1;
	#endif
}



int ai_vr_uninit(void)
{
	#if 1
	stop_dds();
	if(ai_vr_info.asr.sds.recordId)
		free(ai_vr_info.asr.sds.recordId);
	ai_vr_info.asr.sds.recordId = NULL;
	if (ai_vr_info.content.state != CONTENT_IDLE)
		content_free(&ai_vr_info.content);
	if (ai_vr_info.asr.state != ASR_IDLE)
		slot_free(&ai_vr_info.asr);
	ray_aec_delete();
	ray_wakeup_delete();
	ray_vad_stop();
	ai_mutex_lock();
	if (ai_vr_callback){
		ai_vr_callback    = NULL;
	}
	ai_mutex_unlock();
	return 0;
	#endif
}

int ai_vr_authorization(void)
{
	#if 0
	struct aiengine *agn;
	char buf[TMP_BUFFER_SZ] = {0};

	agn = aiengine_new(sds_conf);
	if (NULL == agn) {
		printf("Create sds engine error.\n");
		return -1;
	}

	aiengine_opt(agn, 11, buf, TMP_BUFFER_SZ);
	printf("%s\n", buf);

	aiengine_delete(agn);

	return 0;
	#endif
}

