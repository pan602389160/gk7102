#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "mozart_musicplayer.h"

#include "vr_interface.h"
#include "ray_module_vr.h"

#include "debug_print.h"


static bool need_resume_after_asr = false;

extern bool asr_break_flag;
static vr_result_t process_vr_aec_callback(aec_info_t *aec)
{
	printf("-------------------process_vr_aec_callback----------------------\n");
	vr_result_t result;
	memset(&result, 0, sizeof(result));

	if (aec->wakeup) {
		purple_pr("--------------->aec wakeup.\n");

		mozart_module_pause();
		system("localplay /usr/data/tone/shenmeshi.pcm");
		result.vr_next = VR_TO_ASR_SDS;// VR_TO_ASR_SDS
	} else {
		/* aec error, back to AEC again. */
		result.vr_next = VR_TO_AEC;
	}

	return result;
}


static void process_vr_asr_command_command(asr_info_t *asr)
{
	sem_command_t *command = &asr->sem.request.command;

	/* response volume first. */
	if (command->volume) {
		if (!strcmp(command->volume, "+")) {
			printf("----------volume_up\n");
			mozart_volume_up();
			mozart_play_key_sync("/usr/data/tone/volume_up.mp3");	
		} else if (!strcmp(command->volume, "-")) {
			printf("----------volume_down\n");
			mozart_volume_down();
			mozart_play_key_sync("/usr/data/tone/volume_down.mp3");	
		} else if (!strcmp(command->volume, "max")) {
			printf("----------volume_max\n");
			mozart_volume_set(100);
			mozart_play_key_sync("/usr/data/tone/volume_up.mp3");	
		} else if (!strcmp(command->volume, "min")) {
			printf("----------volume_min\n");
			mozart_volume_set(10);
			mozart_play_key_sync("/usr/data/tone/volume_down.mp3");	
		} else if (command->volume[0] >= '0' && command->volume[0] <= '9') {
			printf("----------set volume %d\n",atoi(command->volume));
			mozart_volume_set(atoi(command->volume));
		} else {
			printf("TODO: unsupport volume set format: %s.\n", command->volume);
		}
	}

	/* then response operation */
	if (command->operation) {
		if (strcmp(command->operation, "暂停") == 0) {
			printf("---->[pause]\n");
			player_status_t status;
			status = mozart_player_getstatus(mozart_musicplayer_handler->player_handler);
			if (status == PLAYER_PLAYING)
				mozart_musicplayer_play_pause(mozart_musicplayer_handler);
			mozart_play_key_sync("/usr/data/tone/pause.mp3");
	
			//FIXME: maybe not paused here.
			mozart_play_key_sync("paused");
		} else if (strcmp(command->operation, "播放") == 0) {
			//mozart_play_key_sync("resume");
			mozart_play_pause();
		} else if (strcmp(command->operation, "继续") == 0) {
			printf("---->[keep playing]\n");
			//mozart_play_key_sync("resume");
			mozart_play_pause();
			need_resume_after_asr = false;
		} else if (strcmp(command->operation, "停止") == 0) {
			//mozart_module_stop();
			//FIXME: maybe not stop here.s
			//mozart_play_key_sync("stopped");
		} else if (strcmp(command->operation, "上一个") == 0) {
			//mozart_play_key_sync("previous_song");
			printf("---->[previous_song]\n");
			mozart_previous_song();
		} else if (strcmp(command->operation, "下一个") == 0) {
			//mozart_play_key_sync("next_song");
			printf("---->[next_song]\n");
			mozart_next_song();
		} else if (strcmp(command->operation, "退出") == 0) {
			//mozart_play_key_sync("exit");
		} else if (strcmp(command->operation, "结束") == 0) {
			//mozart_play_key_sync("exit");
		} else {
			printf("unsurport operation：%s\n", command->operation);
		}
	}

	return;
}

static void process_vr_asr_command(asr_info_t *asr)
{
	printf("sem.request.domain: %s\n", vr_domain_str[asr->sem.request.domain]);

	switch (asr->sem.request.domain) {
	case DOMAIN_MUSIC:
		printf("unsurport operation%s\n", vr_domain_str[asr->sem.request.domain]);
		break;
	case DOMAIN_MOVIE:
		printf("unsurport operation%s\n", vr_domain_str[asr->sem.request.domain]);
		break;
	case DOMAIN_NETFM:
		printf("unsurport operation：%s\n", vr_domain_str[asr->sem.request.domain]);
		break;
	case DOMAIN_COMMAND:
		process_vr_asr_command_command(asr);
		break;
	default:
		printf("unsurport operation : %d\n", asr->sem.request.domain);
		break;
	}

	return;
}

static vr_result_t process_vr_asr_callback(asr_info_t *asr)
{	printf("-------------------process_vr_asr_callback----------------------\n");
	vr_result_t result;
	memset(&result, 0, sizeof(result));

	printf("asr result, domain: %s.\n", vr_domain_str[asr->sds.domain]);

	/* (default)resume music after response done */
	need_resume_after_asr = true;

	switch (asr->sds.domain) {
	case DOMAIN_NULL:
		printf("process_vr_asr_callback-------------->DOMAIN_NULL\n");
		printf(">>>asr->sds.output = %s\n",asr->sds.output);
		mozart_play_key_sync(asr->sds.output);
		asr->sds.is_mult_sds = false;
		break;
	case DOMAIN_STORY:
	case DOMAIN_MUSIC:
		{
			printf("process_vr_asr_callback-------------->DOMAIN_STORY DOMAIN_MUSIC\n");
			mozart_play_key_sync(asr->sds.output);
			//sleep(10);//阻塞播放还未实现，暂时模仿
			//mozart_play_key_sync(asr->sds.music.data[0].url);
			//mozart_tts_sync(tips);
			speech_cloudmusic_playmusic(&asr->sds.music, 0);
			asr->sds.state = SDS_STATE_OFFER;
			
		}
		break;
	case DOMAIN_NETFM:
		printf("process_vr_asr_callback-------------->DOMAIN_NETFM\n");
		if (asr->sds.netfm.number >= 1) {
			int index = 0;
			if (asr->sds.netfm.number > 1) {
				srandom(time(NULL));
				index = random() % asr->sds.netfm.number;
			}
		} else {
			/* TODO: command, such as volume control. */
		}
		break;
	case DOMAIN_CHAT:
		purple_pr("process_vr_asr_callback-------------->DOMAIN_CHAT\n");

			printf("output: %s.\n", asr->sds.output);
			mozart_play_key_sync(asr->sds.output);
			asr->sds.is_mult_sds = false;
		break;
	case DOMAIN_WEATHER:
	case DOMAIN_CALENDAR:
	case DOMAIN_STOCK:
	case DOMAIN_POETRY:
	case DOMAIN_MOVIE:
		printf("process_vr_asr_callback-------------->DOMAIN_WEATHER DOMAIN_CALENDAR DOMAIN_STOCK DOMAIN_POETRY DOMAIN_MOVIE\n");
		printf("output: %s.\n", asr->sds.output);
		//mozart_tts_sync(asr->sds.output);
		mozart_play_key_sync(asr->sds.output);
		//mozart_player_aoswitch(mozart_musicplayer_handler->player_handler,asr->sds.output);
		asr->sds.is_mult_sds = false;
		break;
	case DOMAIN_REMINDER:
		printf("process_vr_asr_callback-------------->DOMAIN_REMINDER\n");
		printf("提醒类功能未实现.\n");
		break;
	case DOMAIN_COMMAND:
		printf("process_vr_asr_callback-------------->DOMAIN_COMMAND\n");
		process_vr_asr_command_command(asr);
		asr->sds.is_mult_sds = true;
		break;
	default:
		//mozart_play_key_sync("error_invalid_domain");
		break;
	}
	if(asr_break_flag){
		asr->sds.is_mult_sds = true;
		purple_pr("$$$$$$$$$$$$$$ ASR_BREAK$$$$$$$$$$$$$$\n");
	}
	if(asr->sds.is_mult_sds){  //is_mult_sds 反逻辑,表示是否退出ASR
		purple_pr(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>QUIT ASR\n");
		mozart_play_pause();
		result.vr_next = VR_TO_AEC;
	}else{
		purple_pr(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>VR_TO_ASR_SDS\n");
		//mozart_play_key_sync("/usr/data/tone/asr_continue.wav");
		system("localplay /usr/data/tone/asr_continue.pcm");
		result.vr_next = VR_TO_ASR_SDS;
	}

	return result;
}


static int process_vr_content_callback(content_info_t *content)
{
	printf("======process_vr_content_callback domain: %s.\n", vr_domain_str[content->sds.domain]);

	printf("-----content->sds.domain = %d\n",content->sds.domain);
	switch (content->sds.domain) {
	case DOMAIN_NULL:
		printf("process_vr_content_callback-------------->DOMAIN_NULL\n");
		//mozart_play_key_sync(content->sds.output);
		break;
	case DOMAIN_STORY:
	case DOMAIN_MUSIC:
		{
			printf("process_vr_content_callback-------------->DOMAIN_STORY DOMAIN_MUSIC\n");
			mozart_play_key_sync(content->sds.output);

			speech_cloudmusic_playmusic(&content->sds.music, 0);
		}
		break;
	case DOMAIN_NETFM:
		printf("process_vr_content_callback-------------->DOMAIN_NETFM\n");
		if (content->sds.netfm.number >= 1) {
			int index = 0;
			if (content->sds.netfm.number > 1) {
				srandom(time(NULL));
				index = random() % content->sds.netfm.number;
			}
			/* play tts */
			//mozart_tts_sync(asr->sds.netfm.data[index].track);

			//speech_cloudmusic_playfm(&asr->sds.netfm, index);

		} else {
			/* TODO: command, such as volume control. */
			//mozart_tts_sync(asr->sds.output);
		}
		break;
	case DOMAIN_CHAT:
		printf("process_vr_content_callback-------------->DOMAIN_CHAT\n");
		if (content->sds.chat.url) {
			/* joke, is offer */
			//speech_cloudmusic_playjoke(asr->sds.chat.url);
			printf("asr->sds.chat.url = %s\n",content->sds.chat.url);


		} else {
			printf("output: %s.\n", content->sds.output);
			//mozart_tts_sync(asr->sds.output);
		}
		break;
	default:
		//mozart_play_key_sync("error_invalid_domain");
		printf("Unhandled domain: %s.\n", vr_domain_str[content->sds.domain]);
		break;
	}

	return 0;
}


static void *process_vr_content_func(void *arg)
{
	pthread_detach(pthread_self());

	vr_info_t *vr_info = (vr_info_t *)arg;

	if (vr_info->content.state == CONTENT_SUCCESS) {
		process_vr_content_callback(&vr_info->content);
	} else if (vr_info->content.state == CONTENT_FAIL) {
		printf("content errId: %d, error: %s\n",
			   vr_info->content.errId, vr_info->content.error);
		switch (vr_info->content.errId) {
		case 70604:
		case 70605:
			//mozart_play_key_sync("error_net_fail");
			break;
		case 70603:
		case 70613:
			//mozart_play_key_sync("error_net_slow_wait");
			break;
		default:
			//mozart_play_key_sync("error_server_busy");
			break;
		}
	} else if (vr_info->content.state == CONTENT_INTERRUPT) {
		printf("get content request has been break.\n");
	}
	content_free(&vr_info->content);

	return NULL;
}


vr_result_t process_vr_callback(vr_info_t *vr_info)
{
	#if 1
	vr_result_t result;
	memset(&result, 0, sizeof(result));

	if (vr_info->from == VR_FROM_AEC) {
		purple_pr("---process_vr_callback-----VR_FROM_AEC---------->>\n");
		return process_vr_aec_callback(&vr_info->aec); /*AEV:ai_vr_info.from    = VR_FROM_AEC;	ai_vr_info.aec.wakeup = true;*/
	} else if (vr_info->from == VR_FROM_ASR) {
		purple_pr("---process_vr_callback-VR_FROM_ASR----------vr_info->asr.errId = %d\n",vr_info->asr.errId);
		switch (vr_info->asr.errId) {
		case 80014:
		case 80004:
		case 10305:
		case 10400:
			vr_info->asr.sds.domain = DOMAIN_NULL;
			return process_vr_asr_callback(&vr_info->asr);
			break;
		case 10507:
		case 0:
			if (vr_info->asr.state == ASR_BREAK) {
				printf("asr interrupt.\n");
				break;
			} else {
				return process_vr_asr_callback(&vr_info->asr);
			}
		default:
			//mozart_play_key_sync("error_server_busy");
			break;
		}
		result.vr_next = VR_TO_AEC;
		return result;
	} else if (vr_info->from == VR_FROM_CONTENT) {
		purple_pr("---process_vr_callback-----VR_FROM_CONTENT---------->>\n");
		result.vr_next = VR_TO_NULL;
		pthread_t process_vr_content_thread;

		if (pthread_create(&process_vr_content_thread, NULL, process_vr_content_func, vr_info) == -1)
			printf("Create process vr content pthread failed: %s.\n", strerror(errno));

		/* return-ed result will be ignored */
		return result;
	} else {
		printf("Unsupport callback source: %d, back to AEC mode.\n", vr_info->from);
		result.vr_next = VR_TO_AEC;
		return result;
	}
	#endif
}

