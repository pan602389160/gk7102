/* ************************************************************************
 *       Filename:  sharememory.c
 *    Description:
 *        Version:  1.0
 *        Created:  01/05/2015 12:10:59 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xllv (),
 *        Company:
 * ************************************************************************/

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/file.h>
#include <sys/shm.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <error.h>
#include <errno.h>
#include <unistd.h>
#include <sys/shm.h>

#include <fcntl.h>                          // �ṩopen()����  
//#include <sys/types.h>                      // �ṩmode_t����  
#include <sys/stat.h>                       // �ṩopen()�����ķ���  
//#include <unistd.h>                         // �ṩclose()����  
#include <stdio.h> 


#include "sharememory_interface.h"

#define SHAREMEM_KEY		"/"
#define SHAREMEM_ID			100
#define SHAREMEM_SIZE		128
#define SHAREMEM_LOCK       "/var/run/sharemem.lock"
#define SHAREMEM_DEBUG(x,y...)	//{printf("[ %s : %s : %d] ",__FILE__, __func__, __LINE__); printf(x,##y); printf("\n");}
#define SHAREMEM_ERROR(x,y...)	{printf("[ %s : %s : %d] ",__FILE__, __func__, __LINE__); printf(x,##y); printf("\n");}

static int shm_id = 0;
static int *shm_addr = NULL;

/**
 * @brief 通过状态值枚举值获取对应枚举名字符串,增强可读性
 */
char *module_status_str[] = {
	[STATUS_SHUTDOWN] = "STATUS_SHUTDOWN",
	[STATUS_RUNNING] = "STATUS_RUNNING",
	[STATUS_EXTRACT] = "STATUS_EXTRACT",
	[STATUS_INSERT] = "STATUS_INSERT",
	[STATUS_PLAYING] = "STATUS_PLAYING",
	[STATUS_PAUSE] = "STATUS_PAUSE",
	[WAIT_RESPONSE] = "WAIT_RESPONSE",
	[RESPONSE_DONE] = "RESPONSE_DONE",
	[RESPONSE_PAUSE] = "RESPONSE_PAUSE",
	[RESPONSE_CANCEL] = "RESPONSE_CANCEL",
	[STATUS_MAX] = "STATUS_MAX",
	NULL,
};

/**
 * @brief 通过共享内存域索号引获取对应域名字的字符串,增强可读性
 */
char *memory_domain_str[] = {
	[UNKNOWN_DOMAIN] = "UNKNOWN_DOMAIN",
	[SDCARD_DOMAIN] = "SDCARD_DOMAIN",
	[UDISK_DOMAIN] = "UDISK_DOMAIN",
	[RENDER_DOMAIN] = "RENDER_DOMAIN",
	[AIRPLAY_DOMAIN] = "AIRPLAY_DOMAIN",
	[ATALK_DOMAIN] = "ATALK_DOMAIN",
	[LOCALPLAYER_DOMAIN] = "LOCALPLAYER_DOMAIN",
	[VR_DOMAIN] = "VR_DOMAIN",
	[BT_HS_DOMAIN] = "BT_HS_DOMAIN",
	[BT_AVK_DOMAIN] = "BT_AVK_DOMAIN",
	[LAPSULE_DOMAIN] = "LAPSULE_DOMAIN",
	[TONE_DOMAIN] = "TONE_DOMAIN",
	[CUSTOM_DOMAIN] = "CUSTOM_DOMAIN",
	[MUSICPLAYER_DOMAIN] = "MUSICPLAYER_DOMAIN",
	[MAX_DOMAIN] = "MAX_DOMAIN",
	NULL,
};

int get_domain_cnt(void)
{
	return MAX_DOMAIN;
}

int get_status_cnt(void)
{
	return STATUS_MAX;
}

static int share_mem_lock(void)
{
    int ret = -1;
    int sharemem_lock_fd = -1;

    sharemem_lock_fd = open(SHAREMEM_LOCK, O_RDWR | O_CREAT,
                            S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if (sharemem_lock_fd < 0) {
        fprintf(stderr, "%s: open %s fail: %s\n", __func__, SHAREMEM_LOCK, strerror(errno));
        return sharemem_lock_fd;
    }

    ret = flock(sharemem_lock_fd, LOCK_EX);
    if (ret) {
        fprintf(stderr, "%s: flock failed.\n", __func__);
        close(sharemem_lock_fd);
        sharemem_lock_fd = -1;
        return sharemem_lock_fd;
    }

    return sharemem_lock_fd;
}

static int share_mem_unlock(int lock)
{
    if (lock >= 0)
        close(lock);

    return 0;
}

static void *share_mem_addr(void)
{
	key_t ipckey;
	ipckey = ftok(SHAREMEM_KEY, SHAREMEM_ID);

	shm_id = shmget(ipckey, SHAREMEM_SIZE, IPC_CREAT | 0666);
	if (-1 == shm_id) {
		SHAREMEM_ERROR("shmget error %s", strerror(errno));
		return (void *)-1;
	}

	shm_addr = shmat(shm_id, 0, 0);
	if ((int *)-1 == shm_addr) {
		SHAREMEM_ERROR("shmat error %s", strerror(errno));
		if (-1 == shmctl(shm_id, IPC_RMID, NULL)) {
			SHAREMEM_ERROR("shmctl error %s", strerror(errno));
			return (void *)-1;
		}
		return (void *)-1;
	}
	return shm_addr;
}

int share_mem_init(void)
{
    int tmpfd = -1;

    // file is not exist, make it.
    if (access(SHAREMEM_LOCK, 0)) {
        tmpfd = creat(SHAREMEM_LOCK, S_IRWXU | S_IROTH | S_IXOTH | S_ISUID);
        if (tmpfd < 0) {
            printf("%s: Create %s FAILED: %s.\n", __func__, SHAREMEM_LOCK, strerror(errno));
            return -1;
        } else {
            close(tmpfd);
        }
    }

    if (NULL != shm_addr) {
        SHAREMEM_DEBUG("Has already been initialized");
        return 0;
    }

    shm_addr = share_mem_addr();
    if ((int *)-1 == shm_addr) {
        return -1;
    }

	return 0;
}

int share_mem_clear(void)
{
    int lock = -1;
    int i = 0;

    if ((lock = share_mem_lock()) < 0) {
        SHAREMEM_ERROR("sharemem lock error.\n");
        return -1;
    }

	if (NULL == shm_addr) {
		SHAREMEM_ERROR("The shared memory is not initialized");
        share_mem_unlock(lock);
		return -1;
	}

    share_mem_init();

    for (i = 0; i < get_domain_cnt(); i++) {
        if (memory_domain_str[i])
            shm_addr[i] = (int)STATUS_SHUTDOWN;
    }

    share_mem_unlock(lock);

	return 0;
}

int share_mem_get(memory_domain domain, module_status * status)
{
    int lock = -1;
    if ((lock = share_mem_lock()) < 0) {
        SHAREMEM_ERROR("sharemem lock error.\n");
        return -1;
    }

	if (NULL == shm_addr) {
		SHAREMEM_ERROR("The shared memory is not initialized");
        share_mem_unlock(lock);
		return -1;
	}

	*status = (module_status) shm_addr[domain];

    share_mem_unlock(lock);

	return 0;
}

module_status share_mem_statusget(memory_domain domain)
{
	module_status status = STATUS_ERROR;

	share_mem_get(domain, &status);

	return status;
}

int share_mem_set(memory_domain domain, module_status status)
{
    int lock = -1;
    if ((lock = share_mem_lock()) < 0) {
        SHAREMEM_ERROR("sharemem lock error.\n");
        return -1;
    }

	if (NULL == shm_addr) {
		SHAREMEM_ERROR("The shared memory is not initialized");
        share_mem_unlock(lock);
		return -1;
	}

	shm_addr[domain] = (int)status;

    share_mem_unlock(lock);

	return 0;
}

int share_mem_destory(void)
{
	struct shmid_ds shm_status;
    int lock = -1;

    if ((lock = share_mem_lock()) < 0) {
        SHAREMEM_ERROR("sharemem lock error.\n");
        return -1;
    }

	if (NULL == shm_addr) {
		SHAREMEM_DEBUG("The shared memory is not initialized");
        share_mem_unlock(lock);
		return -1;
	}

	if (-1 == shmdt(shm_addr)) {
		SHAREMEM_ERROR("shmdt error %s", strerror(errno));
        share_mem_unlock(lock);
		return -1;
	}
	if (-1 == shmctl(shm_id, IPC_STAT, &shm_status)) {
		SHAREMEM_ERROR("shmdt error %s", strerror(errno));
        share_mem_unlock(lock);
		return -1;
	}
	if (0 == shm_status.shm_nattch) {
		if (-1 == shmctl(shm_id, IPC_RMID, NULL)) {
			SHAREMEM_ERROR("shmctl error %s", strerror(errno));
            share_mem_unlock(lock);
			return -1;
		}
	}
	shm_addr = NULL;

    share_mem_unlock(lock);

	return 0;
}

int share_mem_get_active_domain(memory_domain * domain)
{
	module_status status;
	*domain = UNKNOWN_DOMAIN;
	int count = 0;
	if (0 == share_mem_get(BT_HS_DOMAIN, &status)) {
		if (STATUS_PLAYING == status || STATUS_PAUSE == status) {
			*domain = BT_HS_DOMAIN;
			count++;
		}
	} else {
		return -1;
	}

	if (0 == share_mem_get(BT_AVK_DOMAIN, &status)) {
		if (STATUS_PLAYING == status || STATUS_PAUSE == status) {
			*domain = BT_AVK_DOMAIN;
			count++;
		}
	} else {
		return -1;
	}

	if (0 == share_mem_get(AIRPLAY_DOMAIN, &status)) {
		if (STATUS_PLAYING == status || STATUS_PAUSE == status) {
			*domain = AIRPLAY_DOMAIN;
			count++;
		}
	} else {
		return -1;
	}

	if (0 == share_mem_get(ATALK_DOMAIN, &status)) {
		if (STATUS_PLAYING == status || STATUS_PAUSE == status) {
			*domain = ATALK_DOMAIN;
			count++;
		}
	} else {
		return -1;
	}

	if (0 == share_mem_get(RENDER_DOMAIN, &status)) {
		if (STATUS_PLAYING == status || STATUS_PAUSE == status) {
			*domain = RENDER_DOMAIN;
			count++;
		}
	} else {
		return -1;
	}

	if (0 == share_mem_get(LOCALPLAYER_DOMAIN, &status)) {
		if (STATUS_PLAYING == status || STATUS_PAUSE == status) {
			*domain = LOCALPLAYER_DOMAIN;
			count++;
		}
	} else {
		return -1;
	}

	if (0 == share_mem_get(VR_DOMAIN, &status)) {
		if (STATUS_PLAYING == status || STATUS_PAUSE == status) {
			*domain = VR_DOMAIN;
			count++;
		}
	} else {
		return -1;
	}

	if (0 == share_mem_get(LAPSULE_DOMAIN, &status)) {
		if (STATUS_PLAYING == status || STATUS_PAUSE == status) {
			*domain = LAPSULE_DOMAIN;
			count++;
		}
	} else {
		return -1;
	}

	if (0 == share_mem_get(CUSTOM_DOMAIN, &status)) {
		if (STATUS_PLAYING == status || STATUS_PAUSE == status) {
			*domain = CUSTOM_DOMAIN;
			count++;
		}
	} else {
		return -1;
	}

	if (0 == share_mem_get(MUSICPLAYER_DOMAIN, &status)) {
		//printf("???????????share_mem_get  status = %d\n",status);
		if (STATUS_PLAYING == status || STATUS_PAUSE == status) {
			*domain = MUSICPLAYER_DOMAIN;
			count++;
		}
	} else {
		return -1;
	}
	//printf("share_mem_get_active_domain count = %d\n",count);
	if (count > 1) {
		SHAREMEM_ERROR("More than one application is active");
		return -1;
	}
	return 0;
}
