/*!
*****************************************************************************
** \file        applications/netcam/onvif/src/onvif_adapter.c
**
** \version     $Id$
**
** \brief       onvif adapter
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**              ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**              OMMISSIONS
**
** (C) Copyright 2012-2013 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/
//*****************************************************************************
//*****************************************************************************
//** Local Defines
//*****************************************************************************
//*****************************************************************************
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if_arp.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <dirent.h>

#include "onvif_adapter.h"
#if 0
#include "cfg_com.h"
#include "sdk_enc.h"
#include "sdk_isp.h"
#include "sdk_network.h"
#include "netcam_api.h"
#endif
#include "adi_sys.h"
#include "adi_venc.h"
#include "adi_audio.h"
#include "venc.h"
#include "vi.h"
#include "ircut.h"
#include "isp.h"

#define LINUX_ONVIF_CFG_DIR                 "/etc/videoconfig/"
#define LINUX_ONVIF_CFG_DEF_DIR             "/etc/videoconfig/"
#define IMAGING_XML_DEF_PATH		        "imaging.xml"
#define MEDIASTATIC_XML_DEF_PATH	        "mediastatic.xml"
#define MEDIADYNAMIC_XML_DEF_PATH	        "mediadynamic.xml"
#define DEVICE_XML_DEF_PATH					"device.xml"
#define DISCOVER_XML_DEF_PATH				"discover.xml"
#define PTZ_XML_DEF_PATH                    "ptz.xml"
#define PTZ_PRESET_CFG_DEF_PATH             "ptz_preset.cfg"


char onvif_file[][32] =
			            {
			                "device.xml",
			                "discover.xml",
			                "imaging.xml",
			                "mediadynamic.xml",
			                "mediastatic.xml",
			                "ptz.xml",
			            };


static int onvif_get_resolution(GONVIF_MEDIA_VideoResolution_S **optArr, int streamID, int *maxOpt);

//device
static int Device_Network_GetMac(char *netCardName, char *macStr);
static int Device_Network_GetIP(char *netCardName, struct sockaddr *addr);
static int Device_Network_SetIP(char *netCardName, char *ipStr);
static int Device_Network_SetMask(char *netCardName, char *maskStr);
static int Device_Network_SetDNS(char *netCardName, ONVIF_DeviceNet_S netInfor);
static int Device_Network_GetNTP(ONVIF_DeviceNTP_S *ntpInfor);
static int Device_Network_SetNTP(ONVIF_DeviceNTP_S ntpInfor);
static int Device_Network_SetGateway(char *netCardName, char *gwStr);
static int Device_Network_GetMTU(char *netCardName, int *mtu);
static int Device_Network_SetMTU(char *netCardName, int mtu);
static int Device_Network_SetDHCP(char *netCardName, int isDHCP);
static int Device_Network_GetIsRunning(char *netCardName);
static int Device_Network_GetWifiNICName(char *netCardName, int nameLen);
static int Device_Network_GetInfor(char *netCardName, ONVIF_DeviceNet_S *pstNicInfor);
static int Device_Network_SetInfor(char *netCardName, ONVIF_DeviceNet_S stNicInfor);
static int Device_System_SetFactoryDefault(ONVIF_DeviceFactory_E enFactory);
static int Device_System_GetDeviceInformation(char *netCardName, GONVIF_DEVMNG_GetDeviceInformation_Res_S *pstADP_GetDeviceInformationRes);
static int Device_system_GetDateAndTime(char *timeBuf, int *timeZone);
static int Device_System_SetDateAddTime(char *timeBuf, int timeZone);
static int Device_System_SetReboot(int delayTime);
static int Device_System_GetUsers(GONVIF_DEVMNG_GetUsers_Res_S *pstUsersCfg);
static int Device_System_CreateUsers(GONVIF_DEVMNG_User_S userCfg);
static int Device_System_DeleteUsers(GK_CHAR *userName);
static int Device_System_SetUser(GONVIF_DEVMNG_User_S userCfg);

static int Device_System_SaveFile(ONVIF_FILE_TYPE type,char *file_name);

static int Device_System_GetFile(ONVIF_FILE_TYPE type,char *buffer,int *len);

//media
static int Media_GetVideoEncoderInfor(ONVIF_MediaVideoEncoderInfor_S *pstEncodeInfor, int encodeNum);
static int Media_GetAudioEncoderInfor(ONVIF_MediaAudioEncoderInfor_S *pstEncodeInfor, int encodeNum);
static int Media_GetStreamUri(unsigned char streamID, GONVIF_MEDIA_StreamType_E enStreamType, char *pstStreamUri, int uriLen);
static int Media_SetResolution(int streamID, int width, int height, int encodetype);
static int Media_SetFrameRate(int streamID, int frameRate);
static int Media_SetBitRate(int streamID, int bitRate);
static int Media_SetInterval(int streamID, int interval);
static int Media_SetQuality(int streamID, float quality);
static int Media_SetGop(int streamID, int gop);
static int Media_Snapshot(void);
static int Media_SetAudioEncodingType(int value);
static int Media_SetAudioBitrate(int value);
static int Media_SetAudioSampleRate(int value);
static int Media_SynVideoEncoderOptions(GONVIF_MEDIA_VideoEncoderConfigurationOptions_S *pstStaticCfg, GK_S32 sizeOptions);
static int Media_SynAudioEncoderOptions(GONVIF_MEDIA_AudioEncoderConfigurationOptions_S *pstStaticCfg, GK_S32 sizeOptions);
static void Media_SaveCfg(void);
//imaging
static int Imaging_GetImagingInfor(ONVIF_ImagingInfor_S *imagingInfor);
static int Imaging_SetBrightness(float brightness);
static int Imaging_SetSaturation(float saturation);
static int Imaging_SetContrast(float contrast);
static int Imaging_SetIrCutFilter(int mode);
static int Imaging_SetSharpness(float sharpdess);
static int Imaging_SetWbMode(int mode);
static void Imaging_SaveCfg(void);

//ptz
static int PTZ_ContinuousMove(ONVIF_PTZContinuousMove_S stContinuousMove);
static int PTZ_AbsoluteMove(ONVIF_PTZAbsoluteMove_S stRelativeMove);
static int PTZ_RelativeMove(ONVIF_PTZRelativeMove_S stRelativeMove);
static int PTZ_Stop(ONVIF_PTZStop_S stStop);
static int PTZ_GetPresets(ONVIF_PTZ_GetPresets_S *pstAllPresets);
static int PTZ_SetPreset(ONVIF_PTZ_Preset_S stPreset);
static int PTZ_GotoPreset(int presetNum);
static int PTZ_RemovePreset(int presetNum);
//event

//analytics

//*****************************************************************************
//*****************************************************************************
//** Local structures
//*****************************************************************************
//*****************************************************************************



//*****************************************************************************
//*****************************************************************************
//** Global Data
//*****************************************************************************
//*****************************************************************************



//*****************************************************************************
//*****************************************************************************
//** Local Data
//*****************************************************************************
//*****************************************************************************



//*****************************************************************************
//*****************************************************************************
//** Local Functions Declaration
//*****************************************************************************
//*****************************************************************************


GONVIF_ADAPTER_Media_S g_stMediaAdapter =
{
	.GetVideoEncoderInfor   = Media_GetVideoEncoderInfor,
	.GetAudioEncoderInfor   = Media_GetAudioEncoderInfor,

    .GetStreamUri           = Media_GetStreamUri,
    .SetResolution          = Media_SetResolution,
    .SetFrameRate           = Media_SetFrameRate,
    .SetBitRate             = Media_SetBitRate,
    .SetInterval            = Media_SetInterval,
    .SetQuality             = Media_SetQuality,
    .SetGovLength           = Media_SetGop,
    .Snapshot               = Media_Snapshot,

    .SetAudioEncodingType   = Media_SetAudioEncodingType,
    .SetAudioBitrate        = Media_SetAudioBitrate,
    .SetAudioSampleRate     = Media_SetAudioSampleRate,
	.SynVideoEncoderOptions	= Media_SynVideoEncoderOptions,	// If SynVideoEncoderOptions == NULL, options use private configuration.
	.SynAudioEncoderOptions = Media_SynAudioEncoderOptions,	// If SynAudioEncoderOptions == NULL, options use private configuration.
    .SaveCfg                = Media_SaveCfg,
};


ONVIF_ADAPTER_Imaging_S g_stImagingAdapter =
{
	.GetImagingInfor	= Imaging_GetImagingInfor,
    .SetBrightness      = Imaging_SetBrightness,
    .SetColorSaturation	= Imaging_SetSaturation,
    .SetContrast        = Imaging_SetContrast,
    .SetIrCutFilter     = Imaging_SetIrCutFilter,
    .SetSharpness       = Imaging_SetSharpness,
    .SetWbMode		    = Imaging_SetWbMode,
    .SaveCfg            = Imaging_SaveCfg,
#if 0
    .imagingSetBlcMode       	= imagingSetBlcMode,
    .imagingSetExposureMode    	= imagingSetExposureMode,
    .imagingSetAeShutterTimeMin = imagingSetAeShutterTimeMin,
    .imagingSetAeShutterTimeMax	= imagingSetAeShutterTimeMax,
    .imagingSetAeGainMin        = imagingSetAeGainMin,
    .imagingSetAeGainMax     	= imagingSetAeGainMax,
    .imagingSetMeShutterTime	= imagingSetMeShutterTime,
	.imagingSetMeGain			= imagingSetMeGain,
    .imagingSetFocusMode        = imagingSetFocusMode,
    .imagingSetAfNearLimit      = imagingSetAfNearLimit,
    .imagingSetAfFarLimit       = imagingSetAfFarLimit,
    .imagingSetMfDefaultSpeed   = imagingSetMfDefaultSpeed,
    .imagingSetWdrMode          = imagingSetWdrMode,
    .imagingSetWdrLevel         = imagingSetWdrLevel,
    .imagingSetMwbRGain   	    = imagingSetMwbRGain,
    .imagingSetMwbBGain         = imagingSetMwbBGain,
    .imagingSetForcePersistence = imagingSetForcePersistence,
#endif
};


ONVIF_ADAPTER_PTZ_S g_stPTZAdapter =
{
	.ContinuousMove  = PTZ_ContinuousMove,
    .AbsoluteMove    = PTZ_AbsoluteMove,
    .RelativeMove    = PTZ_RelativeMove,
    .Stop            = PTZ_Stop,
    .GetPresets		 = PTZ_GetPresets,
    .SetPreset       = PTZ_SetPreset,
    .GotoPreset		 = PTZ_GotoPreset,
    .RemovePreset	 = PTZ_RemovePreset,
};

ONVIF_ADAPTER_Device_S g_stDeviceAdapter =
{
	//network
    .GetNetworkInfor         = Device_Network_GetInfor,
    .SetNetworkInfor	     = Device_Network_SetInfor,
    .GetNetworkMac           = Device_Network_GetMac,
    .SetNetworkMac           = NULL,
    .GetNetworkIP            = Device_Network_GetIP,
    .SetNetworkIP            = Device_Network_SetIP,
    .GetNetworkMask          = NULL,
    .SetNetworkMask          = Device_Network_SetMask,
    .GetNetworkDNS           = NULL,
    .SetNetworkDNS           = Device_Network_SetDNS,
    .GetNetworkNTP		     = Device_Network_GetNTP,
    .SetNetworkNTP		     = Device_Network_SetNTP,
    .GetNetworkGateway	     = NULL,
    .SetNetworkGateway	     = Device_Network_SetGateway,
    .GetNetworkMTU           = Device_Network_GetMTU,
    .SetNetworkMTU           = Device_Network_SetMTU,
    .SetNetworkDHCP          = Device_Network_SetDHCP,
    .GetNetworkIsRunning     = Device_Network_GetIsRunning,
    .GetNetworkWifiNICName   = Device_Network_GetWifiNICName,
    //system
	.SetSystemFactoryDefault = Device_System_SetFactoryDefault,
    .GetSystemDeviceInfor    = Device_System_GetDeviceInformation,
	.GetSystemDateAddTime    = Device_system_GetDateAndTime,
	.SetSystemDateAddTime    = Device_System_SetDateAddTime,
	.SetSystemReboot	     = Device_System_SetReboot,
	.SystemGetUsers		 	 = Device_System_GetUsers,
	.SystemCreateUsers	 	 = Device_System_CreateUsers,
	.SystemDeleteUsers	 	 = Device_System_DeleteUsers,
	.SystemSetUser		 	 = Device_System_SetUser,
};

ONVIF_XML_FILE XmlFileHandle =
{
	.OnvifSaveFile           = Device_System_SaveFile,
    .OnvifGetFile            = Device_System_GetFile,
};

extern GADI_SYS_HandleT vencHandle;
extern GADI_SYS_HandleT viHandle;
extern GADI_SYS_HandleT ispHandle;

int  sdk_net_get_gateway(const char *adapter_name, char *gateway)
{
    FILE *fp;
    char buf[1024];
    char iface[16];
    unsigned char tmp[100]={'\0'};
    unsigned int dest_addr=0, gate_addr=0;
    if(NULL == gateway)
    {
        GADI_ERROR("gateway is NULL \n");
        return -1;
    }
    fp = fopen("/proc/net/route", "r");
    if(fp == NULL)
    {
        GADI_ERROR("fopen route error \n");
        return -1;
    }

    fgets(buf, sizeof(buf), fp);
    while(fgets(buf, sizeof(buf), fp))
    {
        if((sscanf(buf, "%s\t%X\t%X", iface, &dest_addr, &gate_addr) == 3)
            && (memcmp(adapter_name, iface, strlen(adapter_name)) == 0)
            && gate_addr != 0)
        {
                memcpy(tmp, (unsigned char *)&gate_addr, 4);
                sprintf(gateway, "%d.%d.%d.%d", (unsigned char)*tmp, (unsigned char)*(tmp+1), (unsigned char)*(tmp+2), (unsigned char)*(tmp+3));
                break;
        }
    }

    fclose(fp);
    return 0;
}

static int Device_Network_GetInfor(char *netCardName, ONVIF_DeviceNet_S *pstNicInfor)
{

    if (netCardName == NULL || pstNicInfor == NULL)
    {
        GADI_ERROR("Invalid parameters.\n");
        return -1;
    }

    struct ifreq ifr;
    const char *ifname = netCardName;
    int skfd;

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(skfd < 0)
    {
        GADI_ERROR("socket create error\n");
        return -1;
    }
    memset(&ifr, 0, sizeof(struct ifreq));
    strcpy(ifr.ifr_name, ifname);
    ifr.ifr_addr.sa_family = AF_INET;

    if (ioctl(skfd, SIOCGIFHWADDR, &ifr) < 0)
    {
        close(skfd);
        GADI_ERROR("get mac failed\n");
        GADI_ERROR("error!!:%s,device:%s\n",strerror(errno),netCardName);
        return -1;
    }

    pstNicInfor->isDHCP = 1;
    memcpy(pstNicInfor->macAddr, ifr.ifr_hwaddr.sa_data, 8);

    ifr.ifr_addr.sa_family = AF_INET;

    if (ioctl(skfd, SIOCGIFADDR, &ifr) != 0)
    {
        close(skfd);
        GADI_ERROR("error!!:%s,device:%s\n",strerror(errno),netCardName);
        return -1;
    }

    strncpy(pstNicInfor->ipAddr,
        inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
        MAX_16_LENGTH-1);

    if (ioctl(skfd, SIOCGIFNETMASK, &ifr) >= 0) {
        strncpy(pstNicInfor->maskAddr,
            inet_ntoa(((struct sockaddr_in *)&ifr.ifr_netmask)->sin_addr),
            MAX_16_LENGTH-1);
    }

    sdk_net_get_gateway(netCardName, pstNicInfor->gwAddr);

    strncpy(pstNicInfor->dnsAddr1, pstNicInfor->gwAddr, MAX_16_LENGTH-1);
    pstNicInfor->mtu = 1500;

    close(skfd);

    return 0;
}

static int Device_Network_SetInfor(char *netCardName, ONVIF_DeviceNet_S stNicInfor)
{
	if (netCardName == NULL)
	{
		GADI_ERROR("Invalid parameters.\n");
		return -1;
	}


	return 0;
}

static int Device_Network_GetMac(char *netCardName, char *macStr)
{

    if (netCardName == NULL || macStr == NULL)
    {
        GADI_ERROR("Invalid parameters.\n");
        return -1;
    }

    struct ifreq ifr;
    const char *ifname = netCardName;
    int skfd;

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(skfd < 0)
    {
        GADI_ERROR("socket create error\n");
        return -1;
    }
    memset(&ifr, 0, sizeof(struct ifreq));
    strcpy(ifr.ifr_name, ifname);
    ifr.ifr_addr.sa_family = AF_INET;

    if (ioctl(skfd, SIOCGIFHWADDR, &ifr) < 0)
    {
        close(skfd);
        GADI_ERROR("get mac failed\n");
        GADI_ERROR("error!!:%s,device:%s\n",strerror(errno),netCardName);
        return -1;
    }

    memcpy(macStr, ifr.ifr_hwaddr.sa_data, 8);

    close(skfd);

    return 0;
}

static int Device_Network_GetIP(char *netCardName, struct sockaddr *addr)
{

    if (netCardName == NULL || addr == NULL)
    {
        GADI_ERROR("Invalid parameters.");
        return -1;
    }

    struct ifreq ifr;
    const char *ifname = netCardName;
    int skfd;

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(skfd < 0)
    {
        GADI_ERROR("socket create error\n");
        return -1;
    }
    memset(&ifr, 0, sizeof(struct ifreq));
    strcpy(ifr.ifr_name, ifname);
    ifr.ifr_addr.sa_family = AF_INET;

    if (ioctl(skfd, SIOCGIFADDR, &ifr) != 0)
    {
        close(skfd);
        GADI_ERROR("error!!:%s,device:%s\n",strerror(errno),netCardName);
        return -1;
    }

    memcpy(addr, &ifr, sizeof(struct sockaddr));
    close(skfd);

    return 0;
}


static int Device_Network_SetIP(char *netCardName, char *ipStr)
{
    if(netCardName == NULL || ipStr == NULL)
    {
        GADI_ERROR("Invalid parameters.");
        return -1;
    }

    return 0;
}

static int Device_Network_SetMask(char *netCardName, char *maskStr)
{
    if (netCardName == NULL || maskStr == NULL)
    {
        GADI_ERROR("Invalid parameters.");
        return -1;
    }


    return 0;
}

static int Device_Network_SetDNS(char *netCardName, ONVIF_DeviceNet_S netInfor)
{
    if (netCardName == NULL)
    {
		GADI_ERROR("Invalid NIC name.");
		return -1;
    }

    return 0;
}

static int Device_Network_GetNTP(ONVIF_DeviceNTP_S *ntpInfor)
{
	if (ntpInfor == NULL)
	{
		GADI_ERROR("Invalid parameters.");
		return -1;
	}

	return 0;
}

static int Device_Network_SetNTP(ONVIF_DeviceNTP_S ntpInfor)
{

	return 0;
}

static int Device_Network_SetGateway(char *netCardName, char *gwStr)
{
    if (netCardName == NULL || gwStr == NULL)
    {
        GADI_ERROR("Invalid parameters.");
        return -1;
    }

	return 0;
}

static int Device_Network_GetMTU(char *netCardName, int *mtu)
{

    if (netCardName == NULL || mtu == NULL)
    {
        GADI_ERROR("Invalid parameters.");
        return -1;
    }
    (void)(netCardName);

    *mtu = 1500;
    return 0;
}


static int Device_Network_SetMTU(char *netCardName, int mtu)
{

    if (netCardName == NULL)
    {
        GADI_ERROR("Invalid parameters.");
        return -1;
    }
    (void)(netCardName);
    (void)(mtu);
    return 0;
}

static int Device_System_SetFactoryDefault(ONVIF_DeviceFactory_E enFactory)
{

    return 0;
}

static int Device_System_GetDeviceInformation(char *netCardName, GONVIF_DEVMNG_GetDeviceInformation_Res_S *pstADP_GetDeviceInformationRes)
{

    if (pstADP_GetDeviceInformationRes == NULL || netCardName == NULL)
    {
        GADI_ERROR("Invalid parameters.");
        return -1;
    }

    int ret = 0;

    strncpy(pstADP_GetDeviceInformationRes->aszManufacturer, "aszManufacturer", MAX_MANUFACTURER_LENGTH-1);
    strncpy(pstADP_GetDeviceInformationRes->aszModel, "aszModel", MAX_MODEL_LENGTH-1);
    strncpy(pstADP_GetDeviceInformationRes->aszFirmwareVersion, "aszFirmwareVersion", MAX_FIRMWARE_VERSION_LENGTH-1);
    strncpy(pstADP_GetDeviceInformationRes->aszHardwareId, "aszHardwareId", MAX_HARDWARE_ID_LENGTH-1);
#if 1	//private CMS
    char mac[16] = {0};
    Device_Network_GetMac(netCardName, mac);

    strncpy(pstADP_GetDeviceInformationRes->aszSerialNumber,  mac, MAX_SERIAL_NUMBER_LENGTH-1);
#else	//standard ONVIF
    strncpy(pstADP_GetDeviceInformationRes->aszSerialNumber, stDevInfo.serialNumber, MAX_SERIAL_NUMBER_LENGTH-1);
#endif
    return ret;
}

static int Device_system_GetDateAndTime(char *timeBuf, int *timeZone)
{

    if (timeBuf == NULL || timeZone == NULL)
    {
        GADI_ERROR("Invalid parameters.");
        return -1;
    }
	int secZone = 0;

    *timeZone = secZone/60;

	return 0;
}

static int Device_System_SetDateAddTime(char *timeBuf, int timeZone)
{

    if (timeBuf == NULL)
    {
        GADI_ERROR("Invalid parameters.");
        return -1;
    }

    return 0;
}

static int Device_System_SetReboot(int delayTime)
{

    return 0;
}

static int Device_System_GetUsers(GONVIF_DEVMNG_GetUsers_Res_S *pstUsersCfg)
{
	if (pstUsersCfg == NULL)
	{
        GADI_ERROR("Invalid parameters.");
        return -1;
	}

    pstUsersCfg->sizeUser = 0;
    strncpy(pstUsersCfg->stUser[pstUsersCfg->sizeUser].aszUsername, "admin", MAX_USERNAME_LENGTH-1);
	strncpy(pstUsersCfg->stUser[pstUsersCfg->sizeUser].aszPassword, "admin", MAX_PASSWORD_LENGTH-1);
	pstUsersCfg->stUser[pstUsersCfg->sizeUser].enUserLevel = UserLevel_Administrator;
	pstUsersCfg->sizeUser++;

	return 0;
}

static int Device_System_CreateUsers(GONVIF_DEVMNG_User_S userCfg)
{


	return 0;
}

static int Device_System_DeleteUsers(GK_CHAR *userName)
{
	if (userName == NULL)
	{
        GADI_ERROR("Invalid parameters.");
        return -1;
	}


	return 0;
}

static int Device_System_SetUser(GONVIF_DEVMNG_User_S userCfg)
{



	return 0;
}

static int Device_Network_SetDHCP(char *netCardName, int isDHCP)
{
    if (netCardName == NULL)
    {
        GADI_ERROR("Invalid parameters.");
        return -1;
    }



    return 0;
}

static int Device_Network_GetIsRunning(char *netCardName)
{
	if (netCardName == NULL)
	{
		GADI_ERROR("Invalid name of NIC.");
		return -1;
	}

    if (strcmp(netCardName, "eth0") != 0)
        return 0;

	return 1;
}

static int Device_Network_GetWifiNICName(char *netCardName, int nameLen)
{
	if (netCardName == NULL)
	{
		GADI_ERROR("Invalid name buffer of NIC");
		return -1;
	}

	strncpy(netCardName, "ra0", nameLen-1);

	return 0;
}

int copy_file(char *src_name, char *des_name)
{

	int fd_old, fd_new, ret;
	char bufer[1024];
	fd_old = open(src_name, O_RDONLY);
	if(fd_old < 0)
	{
		GADI_ERROR("open %s fail", src_name);
		return -1;
	}
	fd_new = open(des_name, O_WRONLY|O_TRUNC|O_CREAT, S_IRWXU|S_IRWXG|S_IRWXO);
	if(fd_new < 0)
	{
		close(fd_old);
		GADI_ERROR("open %s fail", des_name);
		return -1;
	}
	while((ret = read(fd_old, bufer, sizeof(bufer))) > 0)
	{
		if(write(fd_new, bufer, ret) <= 0)
		{
			close(fd_old);
			close(fd_new);
			remove(des_name);
			return -1;
		}
	}
	close(fd_old);
	close(fd_new);
	return 0;
}

static int Device_System_GetFile(ONVIF_FILE_TYPE type,char *buffer,int *len)
{

    char name[128];
    char namedef[128];
    FILE *fp;
    int file_len;
    int ret;
	if (access(LINUX_ONVIF_CFG_DIR, R_OK))
    {
        if(mkdir(LINUX_ONVIF_CFG_DIR, S_IRWXU | S_IRWXG| S_IROTH | S_IXOTH))
           GADI_ERROR("mkdir %s fail\n", LINUX_ONVIF_CFG_DIR);
    }
    if (type < DEVICES || type > PTZ)
    {
        GADI_ERROR("Bad file type\n");
        return -1;
    }

    if (buffer == NULL || *len < 0)
    {
        GADI_ERROR("Bad parameter\n");
        return -1;
    }

    sprintf(name,"%s%s",LINUX_ONVIF_CFG_DIR,onvif_file[type]);
    fp = fopen(name, "r");
    if (fp == NULL)
	{
        sprintf(namedef,"%s%s",LINUX_ONVIF_CFG_DEF_DIR,onvif_file[type]);
        if (copy_file(namedef, name) != 0)
        {
            GADI_ERROR("copy file failed: src:%s,dst:%s\n",namedef,name);
            return -1;
        }
		GADI_INFO("fopen def xml file!\n");
        fp = fopen(name, "r");
        if (fp == NULL)
        {
            GADI_ERROR("open file failed: file:%s",name);
            return -1;
        }
    }

	fseek(fp,0,SEEK_END);
	file_len = ftell(fp);
	if (*len < file_len)
	{
		GADI_ERROR("Get file len is small!,name:%s\n",name);
        fclose(fp);
		return -1;
	}
	fseek(fp,0,SEEK_SET);
    memset(buffer,0,*len);
    ret = fread(buffer,1,file_len,fp);
    if (ret != file_len)
    {
		GADI_ERROR("Read file length error!,read:%d,size:%d,name:%s\n",ret,file_len,name);
        fclose(fp);
		return -1;
    }
    fclose(fp);
	*len = file_len;
	return 0;
}

static int Device_System_SaveFile(ONVIF_FILE_TYPE type,char *file_name)
{

    if (type < DEVICES || type > PTZ)
    {
        GADI_ERROR("Bad file type\n");
        return -1;
    }

    sprintf(file_name,"%s%s",LINUX_ONVIF_CFG_DIR,onvif_file[type]);
	return 0;

}


static int Media_GetVideoEncoderInfor(ONVIF_MediaVideoEncoderInfor_S *pstEncodeInfor, int encodeNum)
{

    if (pstEncodeInfor == NULL)
    {
        GADI_ERROR("Invalid parameters.\n");
        return -1;
    }

    int ret = 0;
	int sizeEncoder = 0;

    if (encodeNum > 4 || encodeNum <= 0)
        sizeEncoder = 4;
	else
		sizeEncoder = encodeNum;

    int i = 0;
    GADI_VENC_StreamFormatT formatPar;
    GADI_VENC_H264ConfigT configPar;

    for (i = 0; i < sizeEncoder; i++)
    {
        formatPar.streamId = i;
        gadi_venc_get_stream_format(vencHandle, &formatPar);
        pstEncodeInfor[i].streamid = formatPar.streamId;
        pstEncodeInfor[i].encode_type = formatPar.encodeType;
        pstEncodeInfor[i].encode_width = formatPar.width;
        pstEncodeInfor[i].encode_height = formatPar.height;
        pstEncodeInfor[i].framerate = formatPar.fps;

        configPar.streamId = i;
        ret = gadi_venc_get_h264_config(vencHandle, &configPar);
        if (ret == GADI_OK) {
            pstEncodeInfor[i].bitrate = configPar.cbrAvgBps;
            pstEncodeInfor[i].quality = 0;
            pstEncodeInfor[i].govLength = configPar.gopN;
            pstEncodeInfor[i].h264Profile = configPar.profile;
        } else {
            pstEncodeInfor[i].bitrate = 0;
            pstEncodeInfor[i].quality = 0;
            pstEncodeInfor[i].govLength = 0;
            pstEncodeInfor[i].h264Profile = 0;
        }
    }

    return ret;
}

static int Media_GetAudioEncoderInfor(ONVIF_MediaAudioEncoderInfor_S *pstEncodeInfor, int encodeNum)
{

    if (pstEncodeInfor == NULL)
    {
        GADI_ERROR("Invalid parameters.");
        return -1;
    }
    int ret = 0;
	int sizeEncoder = 0;
    if (encodeNum > 1 || encodeNum <= 0)
        sizeEncoder = 1;
	else
		sizeEncoder = encodeNum;

	(void)sizeEncoder;// ignore for the moment

    pstEncodeInfor[0].encode_type = 0;
    pstEncodeInfor[0].bitrate     = 16;
    pstEncodeInfor[0].sampleRate  = 8;

    return ret;
}


static int Media_GetStreamUri(unsigned char streamID, GONVIF_MEDIA_StreamType_E enStreamType, char *pstStreamUri, int uriLen)
{

    if (pstStreamUri == NULL)
    {
        GADI_ERROR("Invalid parameters.");
        return -1;
    }

    int ret = 0;
    char stream[MAX_VIDEOENCODE_NUM][32] = {"stream0", "stream1", "stream2", "stream3"};

    snprintf(pstStreamUri, uriLen, "rtsp://%s:554/%s", g_GkIpc_OnvifInf.discov.hostip, stream[streamID]/*, enStreamType ? "false":"true"*/);

    return ret;
}

static int Media_SetResolution(int streamID, int width, int height, int encodetype)
{
    int ret = 0;

    if (encodetype != VideoEncoding_H264)
    {
        GADI_ERROR("unsupported encodingType.");
        return -1;
    }

    gdm_venc_set_resolution(streamID, width, height);
    isp_restart();

    return ret;
}

static int Media_SetFrameRate(int streamID, int frameRate)
{
    int ret = 0;

    GADI_VENC_FrameRateT pstFrameRate;
    pstFrameRate.streamId = streamID;
    pstFrameRate.fps = frameRate;
    ret = gdm_venc_set_framerate(&pstFrameRate);
    if (ret != 0)
    {
        GADI_ERROR("onvif adapter: get video parameters failed.");
        return -1;
    }

    return ret;
}

static int Media_SetBitRate(int streamID, int bitRate)
{

   int ret = 0;
   GADI_VENC_BitRateRangeT bitrate;

   gdm_venc_get_bitrate(&bitrate);
   bitrate.cbrAvgBps = bitRate;
   ret = gdm_venc_set_bitrate(&bitrate);
   if (ret != 0)
   {
       GADI_ERROR("onvif adapter: get video parameters failed.");
       return -1;
   }

    return ret;
}

static int Media_SetInterval(int streamID, int interval)
{

   int ret = 0;
   GADI_VENC_H264GopConfigT h264GopNconfig;

   h264GopNconfig.streamId = streamID;
   h264GopNconfig.gopN = interval;
   ret = gdm_venc_set_gopn(&h264GopNconfig);
   if (ret != 0)
   {
       GADI_ERROR("onvif adapter: get video parameters failed.");
       return -1;
   }

    return ret;
}

static int Media_SetQuality(int streamID, float quality)
{


    return 0;
}

static int Media_SetGop(int streamID, int gop)
{

    int ret = 0;
   GADI_VENC_H264GopConfigT h264GopNconfig;

   h264GopNconfig.streamId = streamID;
   h264GopNconfig.gopN = gop;
   ret = gdm_venc_set_gopn(&h264GopNconfig);
   if (ret != 0)
   {
       GADI_ERROR("onvif adapter: get video parameters failed.");
       return -1;
   }

    return ret;
}

static int Media_Snapshot(void)
{

    int ret = 0;

    ret = gdm_venc_capture_jpeg(1, "/opt/resource/web/snapshot/onvif.jpg");
    if (ret != 0)
    {
        GADI_ERROR("onvif adapter: snap shot failed.");
        return -1;
    }

    return 0;
}

static int Media_SetAudioEncodingType(int value)
{
    int ret = 0;


    return ret;
}

static int Media_SetAudioSampleRate(int value)
{
    int ret = 0;


    return ret;
}

static int Media_SynVideoEncoderOptions(GONVIF_MEDIA_VideoEncoderConfigurationOptions_S *pstStaticCfg, GK_S32 sizeOptions)
{

	if (pstStaticCfg == NULL || sizeOptions <= 0)
	{
		GADI_ERROR("onvif adapter: invalid parameters.");
		return -1;
	}
	int i = 0;
	int ret = 0;
	int options = 0;
    if (sizeOptions > GADI_VENC_STREAM_NUM)
        options = GADI_VENC_STREAM_NUM;
    else
        options = sizeOptions;

	for (i = 0; i < options; i++)
	{
        if (pstStaticCfg[i].stH264.pstResolutionsAvailable != NULL)
            free(pstStaticCfg[i].stH264.pstResolutionsAvailable);
        pstStaticCfg[i].stH264.pstResolutionsAvailable = NULL;
        ret = onvif_get_resolution(&pstStaticCfg[i].stH264.pstResolutionsAvailable, i, &pstStaticCfg[i].stH264.sizeResolutionsAvailable);
        if (ret != 0)
        {
            GADI_ERROR("onvif adapter: fail to get resolution options.");
            return -1;
        }
        pstStaticCfg[i].stQualityRange.min = 1;
        pstStaticCfg[i].stQualityRange.max = 5;
        pstStaticCfg[i].stH264.stGovLengthRange.min = 1;
        pstStaticCfg[i].stH264.stGovLengthRange.max = 5;
        pstStaticCfg[i].stH264.stFrameRateRange.min = 10;
        pstStaticCfg[i].stH264.stFrameRateRange.max = 30;

        pstStaticCfg[i].stH264.sizeH264ProfilesSupported = 1;
        if (pstStaticCfg[i].stH264.penH264ProfilesSupported == NULL) {
            pstStaticCfg[i].stH264.penH264ProfilesSupported =
            (GONVIF_MEDIA_H264Profile_E *)malloc(sizeof(GONVIF_MEDIA_H264Profile_E));
            if (pstStaticCfg[i].stH264.penH264ProfilesSupported == NULL)
            {
                GADI_ERROR("onvif adapter: fail to alloc memory.");
                return -1;
            }
        }
        *pstStaticCfg[i].stH264.penH264ProfilesSupported = 0;
        pstStaticCfg[i].stExtension.stH264.stBitrateRange.min = 1000000;
        pstStaticCfg[i].stExtension.stH264.stBitrateRange.max = 2000000;
	}

	return ret;
}

static int Media_SynAudioEncoderOptions(GONVIF_MEDIA_AudioEncoderConfigurationOptions_S *pstStaticCfg, GK_S32 sizeOptions)
{
	int ret = 0;


	return ret;
}

static int Media_SetAudioBitrate(int value)
{
    int ret = 0;


    return ret;
}

static void Media_SaveCfg(void)
{

    //netcam_audio_cfg_save();
}

static int Imaging_GetImagingInfor(ONVIF_ImagingInfor_S *imagingInfor)
{

    if (imagingInfor == NULL)
    {
        GADI_ERROR("Invalid parameters.");
        return -1;
    }

    GADI_ISP_ContrastAttrT attrPtr;
    GADI_ISP_ShadingAttrT shadattr;

    gadi_isp_get_brightness(ispHandle, &(imagingInfor->brightness));

    gadi_isp_get_contrast_attr(ispHandle, &attrPtr);
    imagingInfor->contrast = attrPtr.enableAuto ? attrPtr.autoStrength : attrPtr.manualStrength;

    gadi_isp_get_saturation(ispHandle, &(imagingInfor->colorSaturation));

    gadi_isp_get_shading_attr(ispHandle, &shadattr);
    imagingInfor->sharpness = 0;

    imagingInfor->irCutFilter = 2;

    return 0;
}
static int Imaging_SetBrightness(float brightness)
{

    int ret = 0;

    gadi_isp_set_brightness(ispHandle, (GADI_S32)brightness);

    return ret;

}
static int Imaging_SetSaturation(float saturation)
{

    int ret = 0;

    gadi_isp_set_saturation(ispHandle, (GADI_S32)(saturation));

    return ret;
}

static int Imaging_SetContrast(float contrast)
{

    int ret = 0;
    GADI_ISP_ContrastAttrT attrPtr;

    gadi_isp_get_contrast_attr(ispHandle, &attrPtr);

    attrPtr.autoStrength = (GADI_S32)contrast;

    gadi_isp_set_contrast_attr(ispHandle, &attrPtr);

    return ret;

}

static int Imaging_SetIrCutFilter(int mode)
{

    int ret = 0;

    ircut_switch_manual(mode);

    return ret;

}
static int Imaging_SetSharpness(float sharpdess)
{

    int ret = 0;
    GADI_ISP_SharpenAttrT attrPtr;

    gadi_isp_get_sharpen_attr(ispHandle, &attrPtr);

    attrPtr.level = (GADI_S32)sharpdess;

    gadi_isp_set_sharpen_attr(ispHandle, &attrPtr);

    return ret;
}
static int Imaging_SetWbMode(int mode)
{

    int ret = 0;

    gadi_isp_set_wb_type(ispHandle, mode);

    return ret;
}

static void Imaging_SaveCfg(void)
{

}

static int PTZ_ContinuousMove(ONVIF_PTZContinuousMove_S stContinuousMove)
{


    return 0;
}

static int PTZ_AbsoluteMove(ONVIF_PTZAbsoluteMove_S stRelativeMove)
{
    int ret = 0;


	return ret;
}

static int PTZ_RelativeMove(ONVIF_PTZRelativeMove_S stRelativeMove)
{
    return 0;
}


static int PTZ_Stop(ONVIF_PTZStop_S stStop)
{

    return 0;
}

static int PTZ_GetPresets(ONVIF_PTZ_GetPresets_S *pstAllPresets)
{


    return 0;
}

static int PTZ_SetPreset(ONVIF_PTZ_Preset_S stPreset)
{


	return 0;
}

static int PTZ_GotoPreset(int presetNum)
{


	return 0;
}

static int PTZ_RemovePreset(int presetNum)
{

	return 0;
}

static int onvif_get_resolution(GONVIF_MEDIA_VideoResolution_S **optArr, int streamID, int *maxOpt)
{
    GONVIF_MEDIA_VideoResolution_S *tmp;

    tmp = (GONVIF_MEDIA_VideoResolution_S *)malloc(sizeof(GONVIF_MEDIA_VideoResolution_S)*3);

    tmp[0].width = 1920;
    tmp[0].height = 1080;
    tmp[1].width = 720;
    tmp[1].height = 480;
    tmp[2].width = 320;
    tmp[2].height = 240;

    *optArr = tmp;
    *maxOpt = 3;

	return 0;
}

