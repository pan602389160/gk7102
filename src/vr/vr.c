#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

#include "vr_interface.h"

#include "aec.h"
#include "asr.h"

#define CNT1000 1000
#define MS10 (10*1000)

extern bool asr_break_flag ;

static pthread_t vr_logic_thread;

static ray_vr_callback callback_to_mozart = NULL;

bool vr_init_flag = false;

bool vr_working_flag = false;

vr_status_t vr_status = VR_IDLE;

static vr_target_t vr_next = VR_TO_NULL;

static vr_info_t *info = NULL;

static bool fast_aec_wakeup_mode;


static void *vr_logic_func(void *params)
{
	printf("=========vr_logic_func========\n");
	#if 1
	vr_result_t res;
	int cnt = 0;
	
	while (vr_init_flag) {
		while (vr_working_flag) {
			cnt = 0;
			switch (vr_next) {
			case VR_TO_AEC:
				vr_status = VR_AEC;
				ray_aec_start();
				break;
			case VR_TO_ASR:
				vr_status = VR_ASR;
				ray_asr_start(false);
				break;
			case VR_TO_ASR_SDS:
				vr_status = VR_ASR;
				ray_asr_start(true);
				break;
			case VR_TO_WAIT:
				if (vr_working_flag) {
					usleep(5 * 1000);
					continue;
				} else {
					break;
				}
			default:
				printf("Unsupport vr_target: %d.\n", vr_next);
				break;
			}
			#if 1
			if (!vr_working_flag) {
				vr_status = VR_IDLE;
				printf("vr stopped!\n");
			} else if (info->from == VR_FROM_ASR) {
				/* waiting for asr result;
				 * asr callback #1 tells us speak end and begin recognition.
				 * asr callback #2 is asr result, then asr.state will become to ASR_SUCCESS */
				while (info->asr.state != ASR_SUCCESS &&
					   info->asr.state != ASR_FAIL &&
					   info->asr.state != ASR_BREAK &&
					   cnt++ < CNT1000) {
					usleep(1000*1000);
					printf("looooooop info->asr.state = %d\n",info->asr.state);
				}

				if (info->asr.state == ASR_BREAK || cnt == CNT1000) {
					printf("ASR_BREAK ASR_BREAK ASR_BREAK ASR_BREAK ASR_BREAK ASR_BREAK\n");
					/* get result timeout or break, cancel current asr connect */
					//asr_cancel();

					/* then, goto AEC*/
					vr_next = VR_TO_AEC;
				} else {
					printf("-----------ASR_SUCCESS--------------\n");
					/* got result */
					if (callback_to_mozart) {
						res = callback_to_mozart(info);  //b---->process_vr_callback
						if(!asr_break_flag)
							vr_next = res.vr_next;
						else{
							printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
							vr_next = VR_TO_AEC;
						}
					} else {
						vr_next = VR_TO_AEC;
					}
					printf("vr_next = %d\n",vr_next);
				}
			} else if (info->from == VR_FROM_AEC && !fast_aec_wakeup_mode) {
				/* TODO: error handle */
				if (callback_to_mozart) {
					printf("------>>>>vr_logic_func----->VR_FROM_AEC\n");
					res = callback_to_mozart(info);  // b---->process_vr_callback
					vr_next = res.vr_next;
				} else {
					/* default next action: AEC */
					vr_next = VR_TO_AEC;
				}
			}
			#endif
		}
		usleep(10 * 1000);

	}
	#endif
	// make compile happy.
	return NULL;
}



static int callback_from_vr(vr_info_t *vr_info)
{
	#if 1
	if (vr_info->from == VR_FROM_AEC) {	
		if (fast_aec_wakeup_mode) {
			vr_result_t res;
			if (callback_to_mozart) {
				/*	ai_vr_info.from    = VR_FROM_AEC;
				 *	ai_vr_info.aec.wakeup = true;
				 */
				res = callback_to_mozart(vr_info);   //process_vr_callback
				//result.vr_next = VR_TO_ASR;
				vr_next = res.vr_next;
			} else {
				vr_next = VR_TO_AEC;
			}
		}
		info = vr_info;
		ray_aec_stop();   // B vr_logic_func
		printf("--------------------vr_next = %d\n",vr_next);
	} else if (vr_info->from == VR_FROM_ASR) {
		printf("------>callback_from_vr  VR_FROM_ASR vr_info->asr.state = %d\n",vr_info->asr.state );
		/*		ai_vr_info.from    = VR_FROM_ASR;
		  *		ai_vr_info.asr.state = ASR_SPEAK_END;
		  */
		if (vr_info->asr.state == ASR_SPEAK_END)
			ray_asr_stop(0);
		else if (vr_info->asr.state == ASR_BREAK)
			ray_asr_stop(1);
		info = vr_info;              // B --->vr_logic_func
	} else if (vr_info->from == VR_FROM_CONTENT) {
		if (callback_to_mozart)
			callback_to_mozart(vr_info);  //b-->process_vr_callback
		return 0;
	}
	#endif
	return 0;
}


int ray_vr_init(ray_vr_callback callback)
{
	printf("===========ray_vr_init===================\n");
	int ret = 0;

	callback_to_mozart = callback;

	ret = ai_vr_init(callback_from_vr);
	if (ret) {
		printf("ai vr init error\n");
		goto err_vr_init_error;
	}

	vr_init_flag = true;
	ret = pthread_create(&vr_logic_thread, NULL, vr_logic_func, NULL);
	if (ret) {
		printf("create thread for vr logic error: %s\n", strerror(errno));
	}
	return 0;
	
err_create_thread_out:
	callback_to_mozart = NULL;

err_vr_init_error:
	ai_vr_uninit();

	return ret;
}

int ray_vr_start(void)
{
	printf("========ray_vr_start=============\n");
	if (!vr_init_flag) {
		printf("vr not init, %s fail.\n", __func__);
		return -1;
	}

	// wakeup vr machine logic
	vr_working_flag = true;

	// start aec deault.
	vr_next = VR_TO_AEC;

	return 0;
}
int ray_vr_stop(void)
{
	printf("==================================================ray_vr_stop\n");
	if (!vr_init_flag || !vr_working_flag) {
		printf("---------------->warning: vr not init or not start, %s fail.\n", __func__);
		return -1;
	}
	
	vr_working_flag = false;
	ray_aec_stop();
	ray_asr_stop(0);
	return 0;
}

int ray_vr_uninit(void)
{
	if (!vr_init_flag) {
		printf("warning: vr not init, do not need %s.\n", __func__);
		return 0;
	}

	ray_vr_stop();

	/* notify vr machine logic pthread stop */
	vr_init_flag = false;

	/* wait vr machine logic pthread stop */
	pthread_join(vr_logic_thread, NULL);

	/* uninit engine */
	ai_vr_uninit();

	return 0;
}

vr_status_t ray_vr_get_status(void)
{
	return vr_status;
}



