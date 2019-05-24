/****************************************************************
  copyright   : Copyright (C) 2014-2016, chenbichao,
              : All rights reserved.
              : www.pptun.com, www.peergine.com
              : 
  filename    : pgLibLiveCap.h
  discription : 
  modify      : create, chenbichao, 2014/07/22
              :
              : Modify, chenbichao, 2014/08/02
              : 1. pgLiveEvent()函数增加lpszRender参数。
              : 2. 增加pgLiveVideoStart()函数，启动视频采集
              : 3. 增加pgLiveVideoStop()函数，停止视频采集
              : 4. 增加pgLiveCamera()函数，对视频拍照
			  : 5. 增加‘PG_LIVE_EVENT_CAMERA_FILE’事件，上报拍照成功事件。
              : 
              : Modify, chenbichao, 2014/11/10
              : 1. 增加pgLiveAudioStart()函数，启动音频采集
              : 2. 增加pgLiveAudioStop()函数，停止音频采集
			  : 
              : Modify, chenbichao, 2015/03/04
              : 1. 增加pgLiveVideoSource()函数，选择视频源
              :
              : Modify, chenbichao, 2015/10/06
              : 1. 增加pgLiveForward* 系列函数，分配和释放直播视频转发服务器资源
              : 2. 增加pgLiveFile* 系列函数，文件传输操作函数。
              : 3. 增加PG_LIVE_EVENT_FORWARD_* 系列事件，分配和释放转发资源的事件。
              : 4. 增加PG_LIVE_EVENT_FILE* 系列事件，文件传输操作的事件。
              :
              : Modify, chenbichao, 2015/10/17
              : 1. PG_LIVE_VIDEO_S结构体增加uMaxVideoStream成员。
              : 2. PG_LIVE_VIDEO_S结构体增加uForward成员。
              :
              : Modify, chenbichao, 2015/11/03
              : 1. 增加pgLiveVideoParam函数，修改视频参数。
              :
			  : modify, chenbichao, 2015/12/12
			  : 1. 增加PG_LIVE_INIT_CFG_S结构的uForwardUse属性，可以设置是否使用第3方P2P节点进行转发。
			  : 2. 修改PG_LIVE_INIT_CFG_S结构的uParam属性为void指针类型，可以兼容64bit的使用场景。
			  :
			  : modify, chenbichao, 2015/12/21
			  : 1. 增加pgLiveAccess函数，控制指定播放端的视频和音频访问权限。
			  :
			  : modify, chenbichao, 2016/01/05
			  : 1. 增加PG_LIVE_EVENT_FRAME_STAT事件，上报视频帧发送失败统计信息。
			  :
			  : modify, chenbichao, 2016/01/21
			  : 1. 增加PG_LIVE_INIT_CFG_S结构的uVideoInExternal属性。
			  : 2. 增加PG_LIVE_INIT_CFG_S结构的uAudioInExternal属性。
			  : 3. 增加PG_LIVE_INIT_CFG_S结构的uAudioOutExternal属性。
			  :
			  : modify, chenbichao, 2016/04/06
			  : 1. 增加pgLiveVideoModeSize()函数，重新定义指定视频模式的尺寸。
			  
			  : modify, chenbichao, 2016/04/06
			  : 1. 增加pgLiveRenderEnum()函数，枚举查询当前连接的Render的P2P ID。
			  : 
*****************************************************************/
#ifndef _PG_LIB_LIVE_CAP_H
#define _PG_LIB_LIVE_CAP_H


#ifdef _PG_DLL_EXPORT
#define PG_DLL_API __declspec(dllexport)
#else
#define PG_DLL_API
#endif


#ifdef __cplusplus
extern "C" {
#endif



/**
 *  错误码定义
 */
typedef enum tagPG_LIVE_ERROR_E {
	PG_LIVE_ERROR_OK = 0,             // 成功
	PG_LIVE_ERROR_INIT = -1,          // 没有调用pgLiveInitialize()或者已经调用pgLiveCleanup()清理模块。
	PG_LIVE_ERROR_CLOSE = -2,         // 会话已经关闭（会话已经不可恢复）。
	PG_LIVE_ERROR_BADPARAM = -3,      // 传递的参数错误。
	PG_LIVE_ERROR_BADRENDER = -4,     // 指定的Render不存在。
	PG_LIVE_ERROR_NOBUF = -5,         // 会话发送缓冲区已满。
	PG_LIVE_ERROR_NOSPACE = -6,       // 传递的接收缓冲区太小。
	PG_LIVE_ERROR_TIMEOUT = -7,       // 操作超时。
	PG_LIVE_ERROR_BUSY = -8,          // 系统正忙。
	PG_LIVE_ERROR_NOLOGIN = -9,       // 还没有登录到P2P服务器。
	PG_LIVE_ERROR_MAXINST = -11,      // 最大实例数。
	PG_LIVE_ERROR_NOCONNECT = -12,    // Render还未建立P2P连接
	PG_LIVE_ERROR_BADSTATUS = -13,    // 状态不正确
	PG_LIVE_ERROR_NOIMP = -14,        // 该接口没有实现
	PG_LIVE_ERROR_SYSTEM = -127,      // 系统错误。
} PG_LIVE_ERROR_E;


/**
 *  数据发送/接收优先级
 */
typedef enum tagPG_PRIORITY_E {
	PG_PRIORITY_0,         // 优先级0, 最高优先级。
	PG_PRIORITY_1,         // 优先级1
	PG_PRIORITY_2,         // 优先级2
	PG_PRIORITY_3,         // 优先级3, 最低优先级
	PG_PRIORITY_BUTT,
} PG_PRIORITY_E;


/**
 *  pgLiveEvent()函数等待的事件定义
 */
typedef enum tagPG_LIVE_EVENT_E {
	PG_LIVE_EVENT_NULL,                 // NULL
	PG_LIVE_EVENT_MESSAGE,              // 接收到Render发送的消息。
	PG_LIVE_EVENT_RENDER_JOIN,          // Render加入视频直播组。
	PG_LIVE_EVENT_RENDER_LEAVE,         // Render离开视频直播组。
	PG_LIVE_EVENT_VIDEO_STATUS,         // 上报视频采集的状态。
	PG_LIVE_EVENT_CAMERA_FILE,          // 抓拍视频照片保存到文件成功。
	PG_LIVE_EVENT_FRAME_STAT,           // 上报视频帧发送失败统计信息。
	PG_LIVE_EVENT_INFO,                 /// 上报Render的连接通道信息

	PG_LIVE_EVENT_SVR_LOGIN = 16,       // 登录到P2P服务器成功（上线）
	PG_LIVE_EVENT_SVR_LOGOUT,           // 从P2P服务器注销或掉线（下线）
	PG_LIVE_EVENT_SVR_REPLY,            // P2P服务器应答事件。
	PG_LIVE_EVENT_SVR_NOTIFY,           // P2P服务器推送事件。
	PG_LIVE_EVENT_SVR_ERROR,            // pgLiveSvrRequest返回错误。
	PG_LIVE_EVENT_SVR_KICK_OUT,         // 被服务器踢出，因为有另外一个相同ID的节点登录了。
	
	PG_LIVE_EVENT_FORWARD_ALLOC_REPLY = 32,   // 分配视频转发资源返回结果
	PG_LIVE_EVENT_FORWARD_FREE_REPLY,         // 释放视频转发资源返回结果

	PG_LIVE_EVENT_FILE_PUT_REQUEST = 48,      // 播放端请求上传文件
	PG_LIVE_EVENT_FILE_GET_REQUEST,           // 播放端请求下载文件
	PG_LIVE_EVENT_FILE_ACCEPT,                // 播放端接收了本采集端的文件传输请求
	PG_LIVE_EVENT_FILE_REJECT,                // 播放端决绝了本采集端的文件传输请求
	PG_LIVE_EVENT_FILE_PROGRESS,              // 文件传输进度上报
	PG_LIVE_EVENT_FILE_FINISH,                // 文件传输完成
	PG_LIVE_EVENT_FILE_ABORT,                 // 文件传输中断

} PG_LIVE_EVENT_E;


/**
 *  通道的连接类型
 */
typedef enum tagPG_LIVE_CNNT_E {
	PG_LIVE_CNNT_Unknown = 0,            // 未知，还没有检测到连接类型

	PG_LIVE_CNNT_IPV4_Pub = 4,           // 公网IPv4地址
	PG_LIVE_CNNT_IPV4_NATConeFull,       // 完全锥形NAT
	PG_LIVE_CNNT_IPV4_NATConeHost,       // 主机限制锥形NAT
	PG_LIVE_CNNT_IPV4_NATConePort,       // 端口限制锥形NAT
	PG_LIVE_CNNT_IPV4_NATSymmet,         // 对称NAT

	PG_LIVE_CNNT_IPV4_Private = 12,      // 私网直连
	PG_LIVE_CNNT_IPV4_NATLoop,           // 私网NAT环回

	PG_LIVE_CNNT_IPV4_TunnelTCP = 16,    // TCPv4转发
	PG_LIVE_CNNT_IPV4_TunnelHTTP,        // HTTPv4转发

	PG_LIVE_CNNT_IPV4_PeerFwd = 24,      // 借助P2P节点转发

	PG_LIVE_CNNT_IPV6_Pub = 32,          // 公网IPv6地址

	PG_LIVE_CNNT_IPV6_TunnelTCP = 40,    // TCPv6转发
	PG_LIVE_CNNT_IPV6_TunnelHTTP,        // HTTPv6转发

	PG_LIVE_CNNT_Offline = 0xffff,       // 对端不在线

} PG_LIVE_CNNT_E;


/**
 *  初始化参数。
 */
typedef struct tagPG_LIVE_INIT_CFG_S {

	// 4个优先级的发送缓冲区长度，单位为：K字节。
	// uBufSize[0] 为优先级0的发送缓冲区长度，传0则使用缺省值，缺省值为128(K)
	// uBufSize[1] 为优先级1的发送缓冲区长度，传0则使用缺省值，缺省值为128(K)
	// uBufSize[2] 为优先级2的发送缓冲区长度，传0则使用缺省值，缺省值为256(K)
	// uBufSize[3] 为优先级3的发送缓冲区长度，传0则使用缺省值，缺省值为256(K)
	// 提示：缓冲区的内存不是初始化时就分配好，要用的时候才分配。
	//       例如，配置了256(K)，但当前只使用了16(K)，则只分配16(K)的内存。
	//       如果网络带宽大，发送的数据不在缓冲区中滞留，则缓冲区实际使用的长度不会增长。
	// 注意：缓冲区的长度值不能超过32768。
	unsigned int uBufSize[PG_PRIORITY_BUTT];

	// 尝试P2P穿透的时间。这个时间到达后还没有穿透，则切换到转发通信。
	// (uTryP2PTimeout == 0)：使用缺省值，缺省值为6秒。
	// (uTryP2PTimeout > 0 && uTryP2PTimeout <= 3600)：超时值为所传的uTryP2PTimeout
	// (uTryP2PTimeout > 3600)：禁用P2P穿透，直接用转发。
	unsigned int uTryP2PTimeout;

	// 允许其他P2P节点借助本节点转发流量。非0：转发速率（字节/秒），0：不允许转发。
	// 建议配置: 32K (字节/秒) 以上。
	unsigned int uForwardSpeed;

	// 是否借助第3方P2P节点转发流量。非0：是，0：否。
	unsigned int uForwardUse;

	// 传递初始化参数（目前在Android系统传入Java VM的指针。）
	// 在JNI模块中实现JNI_Onload接口，获取到Java VM的指针，并在pgLiveInitialize()传入给P2P模块。
	void* pvParam;

	// 视频外部采集/压缩接口选项。非0：外部采集/压缩，0：内部自带的采集/压缩。
	// 接口定义参考pgLibDevVideoIn.h
	unsigned int uVideoInExternal;
	
	// 音频外部采集/压缩接口选项。非0：外部采集/压缩，0：内部自带的采集/压缩。
	// 接口定义参考pgLibDevAudioIn.h
	unsigned int uAudioInExternal;

	// 音频外部解压/播放接口选项。非0：外部解压/播放，0：内部自带的解压/播放。
	// 接口定义参考pgLibDevAudioOut.h
	unsigned int uAudioOutExternal;
								
} PG_LIVE_INIT_CFG_S;


/**
 *  视频采集参数。
 */
typedef struct tagPG_LIVE_VIDEO_S {
	unsigned int uCode;             // 视频编解码类型：1为MJPEG，2为VP8，3为H264, 4为H265
	unsigned int uMode;             // 视频尺寸模式。0: 80x60, 1: 160x120, 2: 320x240, 3: 640x480, 4: 800x600, 5: 1024x768, 6: 176x144, 7: 352x288, 8: 704x576, 9: 854x480, 10: 1280x720, 11: 1920x1080
	unsigned int uRate;             // 视频的帧速：这里指定的帧间隔（毫秒）
	unsigned int uCameraNo;         // 指定摄像头(视频源)编号(0, 1, 2, ...)
	unsigned int uBitRate;          // 视频流的码流大小(针对H264和VP8)，单位为Kbps
	unsigned int uMaxVideoStream;   // 允许同时发送视频流的最大条数，默认为2条。
	unsigned int uForward;          // 转发直播视频流。1为启用转发，0为禁用转发。
} PG_LIVE_VIDEO_S;


/**
 *  Render的地址信息。
 */
typedef struct tagPG_LIVE_INFO_S {
	char szPeerID[128];         // Render的P2P ID
	char szAddrPub[64];         // Render的公网IP地址
	char szAddrPriv[64];        // Render的私网IP地址
	unsigned int uCnntType;     // Render连接通道的连接类型（NAT类型），见枚举“PG_LIVE_CNNT_E”
} PG_LIVE_INFO_S;


///
// Node Event hook
typedef int (*TpfnPGLiveEventHookOnExtRequest)(unsigned int uInstID,
	const char* sObj, int uMeth, const char* sData, unsigned int uHandle, const char* sPeer);

typedef int (*TpfnPGLiveEventHookOnReply)(unsigned int uInstID,
	const char* sObj, int uErr, const char* sData, const char* sParam);


PG_DLL_API
void pgLiveSetEventHook(TpfnPGLiveEventHookOnExtRequest pfnOnExtRequest,
	TpfnPGLiveEventHookOnReply pfnOnReply);


/**
 *  日志输出回调函数
 *
 *  uLevel：[IN] 日志级别
 *
 *  lpszOut：[IN] 日志输出内容
 */
typedef void (*TfnLogOut)(unsigned int uLevel, const char* lpszOut);

/**
 *  描述：P2P直播模块初始化函数
 *
 *  阻塞方式：非阻塞，立即返回。
 *
 *  lpuInstID：[OUT] 实例ID。P2P模块支持多实例，初始化时分配实例ID。
 *
 *  lpszUser：[IN] 客户端时为帐号用户名（侦听端时通常为设备ID）
 *
 *  lpszPass：[IN] 客户端时为帐号密码
 *
 *  lpszSvrAddr：[IN] P2P服务器的地址端口，例如：“127.0.0.1:3333”
 * 
 *  lpszRelayList：[IN] 中继服务器地址列表，P2P无法穿透的情况下通过中继服务器转发。
 *      格式示例："type=0&load=0&addr=127.0.0.1:8000;type=1&load=100&addr=192.168.0.1:443"
 *      每个中继服务器有type、load和addr三个参数，多个中继服务器之间用分号‘;’隔开。
 * 
 *  lpstInitCfg：[IN] 初始化参数，见结构“PG_LIVE_INIT_CFG_S”的定义。
 *
 *  lpstVideo：[IN] 视频采集参数，见结构“PG_lIVE_VIDEO_S”的定义
 *
 *  pfnLogOut：[IN] 日志输出回调函数的指针。回调函数原型见‘TfnLogOut’定义。
 *
 *  返回值：见枚举‘PG_LIVE_ERROR_E’的定义
 */
PG_DLL_API
int pgLiveInitialize(unsigned int* lpuInstID, const char* lpszUser,
	const char* lpszPass, const char* lpszSvrAddr, const char* lpszRelayList,
	PG_LIVE_INIT_CFG_S *lpstInitCfg, PG_LIVE_VIDEO_S* lpstVideo, TfnLogOut pfnLogOut);


PG_DLL_API
int pgLiveInitializeEx(unsigned int* lpuInstID, const char* lpszUser,
	const char* lpszPass, const char* lpszSvrAddr, const char* lpszRelayAddr,
	int iP2PTryTime, const char* lpszInstParam, const char* lpszVideoParam,
	const char* lpszAudioParam, TfnLogOut pfnLogOut);


/**
 *  描述：P2P直播模块清理，释放所有资源。
 * 
 *  阻塞方式：非阻塞，立即返回。
 *
 *  uInstID：[IN] 实例ID，调用pgLiveInitialize()时输出。
 */
PG_DLL_API
void pgLiveCleanup(unsigned int uInstID);


/**
 *  描述：选择视频源。
 *
 *  阻塞方式：非阻塞，立即返回。
 *
 *  uInstID：[IN] 实例ID，调用pgLiveInitialize()时输出。
 *
 *  uCameraNo：[IN] 摄像头编号(0, 1, 2, ...)。
 *
 *  返回值：等于0为成功，小于0为错误码
 */
PG_DLL_API
int pgLiveVideoSource(unsigned int uInstID, unsigned int uCameraNo);


/**
 *  描述：重新定义指定视频模式的尺寸。
 *        此函数调用时机：在pgLiveInitialize()调用之后，pgLiveVideoStart()调用之前。
 *
 *  阻塞方式：非阻塞，立即返回。
 *
 *  uInstID：[IN] 实例ID，调用pgLiveInitialize()时输出。
 *
 *  uVideoMode [IN] 需要重新定义尺寸的视频模式。
 *
 *  uWidth [IN] 视频的宽（像素）。
 *
 *  uHeight [IN] 视频的高（像素）。
 *
 *  返回值：等于0为成功，小于0为错误码
 */
PG_DLL_API
int pgLiveVideoModeSize(unsigned int uInstID, unsigned int uVideoMode,
	unsigned int uWidth, unsigned int uHeight);


/**
 *  描述：修改视频参数。
 *
 *  阻塞方式：非阻塞，立即返回。
 *
 *  uInstID：[IN] 实例ID，调用pgLiveInitialize()时输出。
 *
 *  lpstVideo：[IN] 视频参数。
 *
 *  返回值：等于0为成功，小于0为错误码
 */
PG_DLL_API
int pgLiveVideoParam(unsigned int uInstID, const PG_LIVE_VIDEO_S* lpstVideo);


/**
 *  描述：开始视频采集。
 *
 *  阻塞方式：非阻塞，立即返回。
 *
 *  uInstID：[IN] 实例ID，调用pgLiveInitialize()时输出。
 *
 *  返回值：等于0为成功，小于0为错误码
 */
PG_DLL_API
int pgLiveVideoStart(unsigned int uInstID);


/**
 *  描述：停止视频采集。
 *
 *  阻塞方式：非阻塞，立即返回。
 *
 *  uInstID：[IN] 实例ID，调用pgLiveInitialize()时输出。
 */
PG_DLL_API
void pgLiveVideoStop(unsigned int uInstID);


/**
 *  描述：抓拍照片。通过‘PG_LIVE_EVENT_CAMERA_FILE’事件上报拍照结果。
 *
 *  阻塞方式：非阻塞，立即返回。
 *
 *  uInstID：[IN] 实例ID，调用pgLiveInitialize()时输出。
 *
 *  lpszPath：[IN] 保存照片文件的路径，文件名后缀必须为*.jpg。
 *
 *  返回值：等于0为成功，小于0为错误码
 */
PG_DLL_API
int pgLiveCamera(unsigned int uInstID, const char* lpszPath);


/**
 *  描述：开始音频采集。
 *
 *  阻塞方式：非阻塞，立即返回。
 *
 *  uInstID：[IN] 实例ID，调用pgLiveInitialize()时输出。
 *
 *  返回值：等于0为成功，小于0为错误码
 */
PG_DLL_API
int pgLiveAudioStart(unsigned int uInstID);


/**
 *  描述：停止音频采集。 
 *
 *  阻塞方式：非阻塞，立即返回。
 *
 *  uInstID：[IN] 实例ID，调用pgLiveInitialize()时输出。
 */
PG_DLL_API
void pgLiveAudioStop(unsigned int uInstID);


/**
*  描述：设置音频参数。
*
*  阻塞方式：非阻塞，立即返回。
*
*  sParam：[IN] 音频参数。
*
*  uInstID：[IN] 实例ID，调用pgLiveInitialize()时输出。
*
*  返回值：等于0为成功，小于0为错误码
*/
PG_DLL_API
int pgLiveAudioParam(unsigned int uInstID, const char* sParam);


/**
 *  描述：等待底层事件的函数，可以同时等待多个会话上的多种事件。事件发生后函数返回。
 *
 *  阻塞方式：阻塞，有事件达到或等待超时后返回。
 *
 *  uInstID：[IN] 实例ID，调用pgLiveInitialize()时输出。
 *
 *  lpuEvent：[OUT] 输出当前发生的事件类型，见枚举‘PG_LIVE_EVENT_E’定义
 *
 *  lpszData：[OUT] 接受当前发生事件的内容缓冲区。
 *
 *  uDataSize：[IN] 缓冲区长度。当缓冲区不够长时，超过缓冲区的内容将被丢弃。
 *
 *  lpuParam：[OUT] 输出当前发生事件的参数。
 *      在‘PG_LIVE_EVENT_SVR_REPLY’事件时使用此参数。
 *
 *  lpszRender：[OUT] 接收触发当前事件的Render的P2P节点ID，长度必须大于等于128字符。
 *
 *  uTimeout：[IN] 等待超时时间(毫秒)。传0为不等待，立即返回。
 *
 *  返回值：见枚举‘PG_LIVE_ERROR_E’的定义。
 */
PG_DLL_API
int pgLiveEvent(unsigned int uInstID, unsigned int* lpuEvent, char* lpszData,
	unsigned int uDataSize, unsigned int* lpuParam, char* lpszRender, unsigned int uTimeout);


/**
 *  描述：组播发送通知给各个Render
 *
 *  阻塞方式：非阻塞，立即返回。
 *
 *  uInstID：[IN] 实例ID，调用pgLiveInitialize()时输出。
 *
 *  lpszData：[IN] 组播通知的内容
 *
 *  返回值：大于0为发送的数据长度。小于0为错误码（见枚举‘PG_LIVE_ERROR_E’的定义）
 */
PG_DLL_API
int pgLiveNotify(unsigned int uInstID, const char* lpszData);


/**
 *  描述：发送消息给一个指定的Render.
 *
 *  阻塞方式：非阻塞，立即返回。
 *
 *  uInstID：[IN] 实例ID，调用pgLiveInitialize()时输出。
 *
 *  lpszRender：[IN] 发送目的Render的P2P节点ID
 *
 *  lpszData：[IN] 发送消息的内容
 *
 *  返回值：大于0为接收的数据长度。小于0为错误码（见枚举‘PG_LIVE_ERROR_E’的定义）
 */
PG_DLL_API
int pgLiveMessage(unsigned int uInstID, const char* lpszRender, const char* lpszData);


/**
 *  描述：获取一个指定Render的地址信息。
 *
 *  阻塞方式：非阻塞，立即返回。
 *
 *  uInstID：[IN] 实例ID，调用pgLiveInitialize()时输出。
 *
 *  lpszRender：[IN] 指定Render的P2P节点ID。
 *
 *  lpstInfo：[OUT] 获取到的地址信息，见‘PG_LIVE_INFO_S’结构定义
 *
 *  返回值：0为成功。小于0为错误码（见枚举‘PG_LIVE_ERROR_E’的定义）
 */
PG_DLL_API
int pgLiveInfo(unsigned int uInstID, const char* lpszRender, PG_LIVE_INFO_S* lpstInfo);


/**
 *  描述：拒绝一个指定的Render。
 *
 *  阻塞方式：非阻塞，立即返回。
 *
 *  uInstID：[IN] 实例ID，调用pgLiveInitialize()时输出。
 *
 *  lpszRender：[IN] 指定Render的P2P节点ID
 *
 *  返回值：0为成功。小于0为错误码（见枚举‘PG_LIVE_ERROR_E’的定义）
 */
PG_DLL_API
int pgLiveReject(unsigned int uInstID, const char* lpszRender);


/**
 *  描述：开启和关闭指定的Render的视频和音频访问权限。
 *
 *  阻塞方式：非阻塞，立即返回。
 *
 *  uInstID：[IN] 实例ID，调用pgLiveInitialize()时输出。
 *
 *  lpszRender：[IN] 指定Render的P2P节点ID
 *
 *  uVideoEnable：[IN] 非0：允许视频访问，0：禁止视频访问。默认为允许访问。
 *
 *  uAudioEnable：[IN] 非0：允许音频访问，0：禁止音频访问。默认为允许访问。
 *
 *  返回值：0为成功。小于0为错误码（见枚举‘PG_LIVE_ERROR_E’的定义）
 */
PG_DLL_API
int pgLiveAccess(unsigned int uInstID, const char* lpszRender,
	unsigned int uVideoEnable, unsigned int uAudioEnable);


/**
 *  描述：枚举查询当前连接的Render的P2P ID。
 *
 *  阻塞方式：非阻塞，立即返回。
 *
 *  uInstID：[IN] 实例ID，调用pgLiveInitialize()时输出。
 *
 *  iIndex: [IN] 枚举查询的索引值：0, 1, 2, 3, ...
 *
 *  sRenID: [OUT] 接收Render的P2P ID的缓冲区。
 *
 *  uSize: [IN] 缓冲区长度。
 *
 *  返回值：0为成功。小于0为错误码（见枚举‘PG_LIVE_ERROR_E’的定义）
 */
PG_DLL_API
int pgLiveRenderEnum(unsigned int uInstID, int iIndex, char* sRenID, unsigned int uSize);


/**
 *  描述：判断一个指定的Render是否已经连接到本采集端。
 *
 *  阻塞方式：非阻塞，立即返回。
 *
 *  uInstID：[IN] 实例ID，调用pgLiveInitialize()时输出。
 *
 *  lpszRender：[IN] 指定Render的P2P节点ID
 *
 *  返回值：0为成功。小于0为错误码（见枚举‘PG_LIVE_ERROR_E’的定义）
 */
PG_DLL_API
int pgLiveRenderConnected(unsigned int uInstID, const char* lpszRender);


/**
 *  描述：开始录制本端视频和音频。
 *
 *  阻塞方式：非阻塞，立即返回。
 *
 *  uInstID：[IN] 实例ID，调用pgLiveInitialize()时输出。
 *
 *  sAviPath：[IN] 录制的媒体文件路径，文件名的扩展名为 ".avi", ".mp4", "mov"。
 *
 *  bVideo：[IN] 是否录制视频。1为录制，0为不录制
 *
 *  bAudio：[IN] 是否录制音频。1为录制，0为不录制
 *
 *  返回值：0为成功。小于0为错误码（见枚举‘PG_LIVE_ERROR_E’的定义）
 */
PG_DLL_API
int pgLiveRecordStart(unsigned int uInstID, const char* sAviPath, unsigned int bVideo, unsigned int bAudio);


/**
 *  描述：停止录制本端视频和音频。
 *
 *  阻塞方式：非阻塞，立即返回。
 *
 *  uInstID：[IN] 实例ID，调用pgLiveInitialize()时输出。
 *
 *  返回值：无
 */
PG_DLL_API
void pgLiveRecordStop(unsigned int uInstID);


/**
 *  描述：向P2P服务器发送一个请求。
 *
 *  阻塞方式：非阻塞，立即返回。
 *
 *  uInstID：[IN] 实例ID，调用pgLiveInitialize()时输出。
 *
 *  lpszData：[IN] 请求发送的内容，任意字符串。
 *
 *  uParam：[IN] 自定义参数。（目的是使请求和应答能够相匹配）。
 *
 *  返回值：0为成功。小于0为错误码（见枚举‘PG_LIVE_ERROR_E’的定义）
 */
PG_DLL_API
int pgLiveSvrRequest(unsigned int uInstID, const char* lpszData, unsigned int uParam);


/**
 *  描述：请求分配视频转发资源。
 *
 *  阻塞方式：非阻塞，立即返回。
 *
 *  uInstID：[IN] 实例ID，调用pgLiveInitialize()时输出。
 *
 *  返回值：0为成功。小于0为错误码（见枚举‘PG_LIVE_ERROR_E’的定义）
 */
PG_DLL_API
int pgLiveForwardAlloc(unsigned int uInstID);


/**
 *  描述：请求释放视频转发资源。
 *
 *  阻塞方式：非阻塞，立即返回。
 *
 *  uInstID：[IN] 实例ID，调用pgLiveInitialize()时输出。
 *
 *  返回值：0为成功。小于0为错误码（见枚举‘PG_LIVE_ERROR_E’的定义）
 */
PG_DLL_API
int pgLiveForwardFree(unsigned int uInstID);


/**
 *  描述：请求上传文件给播放端。
 *
 *  阻塞方式：非阻塞，立即返回。
 *
 *  uInstID：[IN] 实例ID，调用pgLiveInitialize()时输出。
 * 
 *  lpszRender：[IN] 播放端的P2P ID
 *
 *  lpszPath：[IN] 指定上传的文件路径（全路径）
 *  
 *  lpszPeerPath：[IN] 文件在播放端存储的相对路径。
 *                如果此参数传空，则SDK自动从lpszPath参数中截取文件名作为本参数。
 *
 *  返回值：0为成功。小于0为错误码（见枚举‘PG_LIVE_ERROR_E’的定义）
 */
PG_DLL_API
int pgLiveFilePutRequest(unsigned int uInstID, const char* lpszRender,
	const char* lpszPath, const char* lpszPeerPath);


/**
 *  描述：请求从播放端下载文件。
 *
 *  阻塞方式：非阻塞，立即返回。
 *
 *  uInstID：[IN] 实例ID，调用pgLiveInitialize()时输出。
 * 
 *  lpszRender：[IN] 播放端的P2P ID
 *
 *  lpszPath：[IN] 指定保存此下载文件的路径（全路径）
 *  
 *  lpszPeerPath：[IN] 文件在播放端存储的相对路径。
 *                如果此参数传空，则SDK自动从lpszPath参数中截取文件名作为本参数。
 *
 *  返回值：0为成功。小于0为错误码（见枚举‘PG_LIVE_ERROR_E’的定义）
 */
PG_DLL_API
int pgLiveFileGetRequest(unsigned int uInstID, const char* lpszRender,
	const char* lpszPath, const char* lpszPeerPath);


/**
 *  描述：接受播放端的文件上传或文件下载请求。
 *
 *  阻塞方式：非阻塞，立即返回。
 *
 *  uInstID：[IN] 实例ID，调用pgLiveInitialize()时输出。
 * 
 *  lpszRender：[IN] 播放端的P2P ID
 *
 *  lpszPath：[IN] 指定保存此上传文件的路径，或指定下载文件的路径（全路径）
 *  
 *  返回值：0为成功。小于0为错误码（见枚举‘PG_LIVE_ERROR_E’的定义）
 */
PG_DLL_API
int pgLiveFileAccept(unsigned int uInstID,
	const char* lpszRender, const char* lpszPath);


/**
 *  描述：拒绝播放端的文件上传或文件下载请求。
 *
 *  阻塞方式：非阻塞，立即返回。
 *
 *  uInstID：[IN] 实例ID，调用pgLiveInitialize()时输出。
 * 
 *  lpszRender：[IN] 播放端的P2P ID
 *  
 *  返回值：0为成功。小于0为错误码（见枚举‘PG_LIVE_ERROR_E’的定义）
 */
PG_DLL_API
int pgLiveFileReject(unsigned int uInstID, const char* lpszRender);


/**
 *  描述：取消（中断）正在进行的文件传输。
 *
 *  阻塞方式：非阻塞，立即返回。
 *
 *  uInstID：[IN] 实例ID，调用pgLiveInitialize()时输出。
 * 
 *  lpszRender：[IN] 播放端的P2P ID
 *  
 *  返回值：0为成功。小于0为错误码（见枚举‘PG_LIVE_ERROR_E’的定义）
 */
PG_DLL_API
int pgLiveFileCancel(unsigned int uInstID, const char* lpszRender);


/**
 *  描述：设置日志输出的级别。
 *
 *  阻塞方式：非阻塞，立即返回。
 *
 *  uLevel：[IN] 日志输出级别：
 *          0：重要的日志信息（默认开启），
 *          1：数量较多但次要的日志信息（默认关闭）。
 *
 *  uEnable：[IN] 0：关闭，非0：开启。
 *
 *  返回值：等于0为成功，小于0为错误码
 */
PG_DLL_API
int pgLiveLevel(unsigned int uLevel, unsigned int uEnable);


/**
 *  描述：获取本模块的版本。
 *
 *  阻塞方式：非阻塞，立即返回。
 *
 *  lpszVersion：[OUT] 接受版本信息的缓冲区。
 *
 *  uSize：[IN] 缓冲区的长度（建议大于等于16字节）
 */
PG_DLL_API
void pgLiveVersion(char* lpszVersion, unsigned int uSize);



#ifdef __cplusplus
}
#endif


#endif //_PG_LIB_LIVE_CAP_H
