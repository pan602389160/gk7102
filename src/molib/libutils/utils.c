/* ************************************************************************
 *       Filename:  utils.c
 *    Description:
 *        Version:  1.0
 *        Created:  10/28/2014 06:55:04 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xllv (),
 *        Company:
 * ************************************************************************/

#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <string.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/statfs.h>

#include "ini_interface.h"
#include "utils_interface.h"

#define DEBUG(x,y...)				{printf("[ %s : %s : %d] ",__FILE__, __func__, __LINE__); printf(x,##y); printf("\n");}

int rc = -1;

void set_owner(int fd, pid_t pid)
{
	fcntl(fd, F_SETOWN, getpid());
}

void set_flag(int fd, int flags)
{
	int val;
	val = fcntl(fd, F_GETFL, 0);
	if (val == -1)
		DEBUG("fcntl get flag error:%s", strerror(errno));
	val |= flags;
	if (fcntl(fd, F_SETFL, val) < 0)
		DEBUG("fcntl get flag error:%s", strerror(errno));
}

void clr_flag(int fd, int flags)
{
	int val;
	val = fcntl(fd, F_GETFL, 0);
	if (val == -1)
		DEBUG("fcntl get flag error:%s", strerror(errno));
	val &= ~flags;
	if (fcntl(fd, F_SETFL, val) < 0)
		DEBUG("fcntl get flag error:%s", strerror(errno));
}

static mode_t get_link_stat(char *link)
{
	struct stat file_stat;
	memset(&file_stat, 0, sizeof(file_stat));
	DEBUG("file is %s\n", link);
	if(lstat(link, &file_stat) < 0){
		DEBUG("lstat:%s", strerror(errno));
		return 0;
	}
	return file_stat.st_mode;
}

static void get_playback_dev(char *dev)
{
	char tmp[16] = {};
        // Parse playback dsp device.
	if (mozart_ini_get_audio_dev_playback(tmp)) {
		printf("[player/libaudio - OSS] Can't get record dsp device, force to /dev/dsp.\n");
		strcpy(dev, "/dev/dsp");
	} else {
		strcpy(dev, tmp);
	}
}



void process(int pid, dsp_status *dsp_status)
{
	DIR *dir;
    struct dirent *ptr;
	char item[256] = {0};
	char buff[1024];
	char link[256];
	char dev_dsp[16] = {};
	mode_t mode;

	get_playback_dev(dev_dsp);

	sprintf(item, "/proc/%d/fd", pid);
    if (!(dir = opendir(item))){
		DEBUG("opendir:%s", strerror(errno));
		return;
	}
    //读取目录
    while ((ptr = readdir(dir))) {
		//循环读取出所有的进程文件
		strcpy(link, item);
		strcat(link, "/");
		strcat(link, ptr->d_name);
		//DEBUG("link is %s\n", link);
		memset(buff, 0, sizeof(buff));
		if(-1 == readlink(link, buff, sizeof(buff))){
			DEBUG("readlink:%s", strerror(errno));
			continue;
		}
		if(strcmp(buff, dev_dsp) == 0){
			DEBUG("buff is %s\n", buff);
			mode = get_link_stat(link);
			if(mode & S_IRUSR){
				if(*dsp_status == WRITEING)
					*dsp_status = BUSY;
				else if(*dsp_status == IDLE)
					*dsp_status = READING;
			}
			if(mode & S_IWUSR){
				if(*dsp_status == READING)
					*dsp_status = BUSY;
				else if(*dsp_status == IDLE)
					*dsp_status = WRITEING;
			}
		}
		if(BUSY == *dsp_status)
			break;
    }
	closedir(dir);
}

void get_dsp_status(dsp_status *dsp_status)
{
	*dsp_status = IDLE;
	char dev_dsp[16] = {};
	char cmd[32] = "fuser ";
	int pid;

	get_playback_dev(dev_dsp);
	strcat(cmd, dev_dsp);

	FILE *file = popen(cmd, "r");      /*子进程执行 ls -la ,并把输出写入管道中*/
	if(NULL == file)
		DEBUG("popen fail");
	while (!feof(file)){
		fscanf(file, "%d", &pid);
		DEBUG("pid = %d\n", pid);
		process(pid, dsp_status);
		if(BUSY == *dsp_status)
			break;
	}
	pclose(file);
}

int check_dsp_opt(int opt)
{
	int ret = -1;
	char dev_dsp[16] = {};

	get_playback_dev(dev_dsp);

	ret = open(dev_dsp, opt);
	if(-1 == ret)
		return 0;
	else
		close(ret);
	return 1;
}

char *get_ip_addr(char *ifname)
{
	char *ip = NULL;

	if(!ifname)
		return NULL;

	struct ifaddrs *addrs = NULL;
	struct ifaddrs *tmp;

	if(getifaddrs(&addrs)){
		perror("getifaddrs");
		return NULL;
	}

	tmp = addrs;
	while (tmp)
	{
		if (!strcmp(tmp->ifa_name, ifname) && tmp->ifa_addr->sa_family == AF_INET)
		{
			struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
			ip = inet_ntoa(pAddr->sin_addr);
			freeifaddrs(addrs);
			return ip;
		}
		tmp = tmp->ifa_next;
	}

	freeifaddrs(addrs);
	return NULL;
}

char *get_mac_addr(char *ifname, char *macaddr, char *separator)
{
	struct ifreq ifreq;
	int sock;

	if(!ifname)
		return NULL;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("in %s:%s:%d, socket error: %s\n", __FILE__, __func__, __LINE__, strerror(errno));
		return NULL;
	}

	strcpy(ifreq.ifr_name, ifname);
	if (ioctl(sock, SIOCGIFHWADDR, &ifreq) < 0) {
		printf("in %s:%s:%d, ioctl error: %s\n", __FILE__, __func__, __LINE__, strerror(errno));
		return NULL;
	}

	if(!separator)
		separator = ":";

	sprintf(macaddr, "%02x%s%02x%s%02x%s%02x%s%02x%s%02x",
	       (unsigned char)ifreq.ifr_hwaddr.sa_data[0], separator,
	       (unsigned char)ifreq.ifr_hwaddr.sa_data[1], separator,
	       (unsigned char)ifreq.ifr_hwaddr.sa_data[2], separator,
	       (unsigned char)ifreq.ifr_hwaddr.sa_data[3], separator,
	       (unsigned char)ifreq.ifr_hwaddr.sa_data[4], separator,
	       (unsigned char)ifreq.ifr_hwaddr.sa_data[5]);
	if(sock >= 0)
		close(sock);

	return macaddr;
}

int mozart_system(const char *cmd)
{
#if 1
	int i = 0;

	if (cmd == NULL) {
		printf("system cmd is NULL!\n");
		return -1;
	}

	for (i = 0; i < sysconf(_SC_OPEN_MAX); i++)
		if (i != STDIN_FILENO && i != STDOUT_FILENO && i != STDERR_FILENO)
			fcntl(i, F_SETFD, FD_CLOEXEC);

	return system(cmd);
#else
	typedef void (*sighandler_t)(int);
	int ret = 0;

	sighandler_t old_handler;

	old_handler = signal(SIGCHLD, SIG_DFL);
	ret = system(cmd);
	signal(SIGCHLD, old_handler);

	return ret;
#endif
}

char *mozart_itoa(int value, char *str)
{
	if(!str)
		return NULL;

	if (value < 0) //如果是负数,则str[0]='-',并把value取反(变成正整数)
	{
		str[0] = '-';
		value = 0-value;
	}

	int i,j;
	for(i=1; value > 0; i++,value/=10) //从value[1]开始存放value的数字字符，不过是逆序，等下再反序过来
		str[i] = value%10+'0'; //将数字加上0的ASCII值(即'0')就得到该数字的ASCII值

	for(j=i-1,i=1; j-i>=1; j--,i++) //将数字字符反序存放
	{
		str[i] = str[i]^str[j];
		str[j] = str[i]^str[j];
		str[i] = str[i]^str[j];
	}

	if(str[0] != '-') //如果不是负数，则需要把数字字符下标左移一位，即减1
	{
		for(i=0; str[i+1]!='\0'; i++)
			str[i] = str[i+1];
		str[i] = '\0';
	}

	return str;
}

static audio_type global_audio_type = AUDIO_UNKNOWN;
audio_type get_audio_type()
{
	if (global_audio_type == AUDIO_UNKNOWN) {
		char type[5] = {0};
		if (!mozart_ini_get_audio_type(type)) {
			if (!strcasecmp(type, "oss"))
				global_audio_type = AUDIO_OSS;
			else if(!strcasecmp(type, "alsa"))
				global_audio_type = AUDIO_ALSA;
		}
	}

	return global_audio_type;
}

static codec_type global_audio_codec_type = INTER_CODEC;
codec_type get_audio_codec_type()
{

	if (global_audio_codec_type == UNKNOWN_CODEC) {
		char type[16] = {0};
		if (!mozart_ini_get_audio_codec(type)) {
			if (!strcasecmp(type, "inter_codec"))
				global_audio_codec_type = INTER_CODEC;
			else if(!strcasecmp(type, "exter_codec"))
				global_audio_codec_type = EXTER_CODEC;
		}
	}

	return global_audio_codec_type;
}

bool mozart_path_is_mount(const char *path)
{
    if (!path)
        return false;

    char *key = malloc(strlen("on ") + strlen(path) + 1);
    char buffer[512] = {};
    sprintf(key, "on %s", path);

	FILE *file = popen("mount", "r");
	if(NULL == file)
		DEBUG("popen fail");

    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        if (strstr(buffer, key)) {
            pclose(file);
            return true;
        }
	}

	pclose(file);

    return false;
}

sdinfo mozart_get_sdcard_info(const char *sdpath)
{
	struct statfs diskInfo;
    sdinfo sdcardInfo;  //返回值，sd卡信息

    statfs(sdpath, &diskInfo);

    unsigned long long blocksize = diskInfo.f_bsize;    //每个block里包含的字节数

    unsigned long long totalsize = blocksize * diskInfo.f_blocks;   //总的字节数，f_blocks为block的数目
    sdcardInfo.totalSize = totalsize/1024/1024;

    unsigned long long availableDisk = diskInfo.f_bavail * blocksize;   //可用空间大小
	sdcardInfo.availableSize = availableDisk/1024/1024;

	sdcardInfo.usedSize = sdcardInfo.totalSize - sdcardInfo.availableSize;

    return sdcardInfo;
}
