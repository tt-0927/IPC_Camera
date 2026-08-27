/**
 * @FilePath     : custom_define.h
 * @Author       : 17343431340@163.com
 * @Date         : 2026-02-27 13:40:43
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-19 15:58:15
 * @Description  : RTSP媒体服务通用数据类型和默认配置
 */
#ifndef __CUSTOM_DEFINE_H
#define __CUSTOM_DEFINE_H


typedef void * RtSpServerHandle_t;
typedef void * RtSpClientHandle_t;
#ifdef __cplusplus
extern "C" {
#endif
#define RTSP_CLIENT_MAX 32
#define STREAM_NAME_MAX 64
#define RTSP_IP_MAX 16
/* RTSP 连接策略默认值：业务编译时可通过能力宏覆盖。 */
#ifndef CAP_RTSP_HIGH_CONCURRENCY
#define CAP_RTSP_HIGH_CONCURRENCY 0               /* 高并发能力开关：1 表示总连接数翻倍 */
#endif
#ifndef RTSP_DEFAULT_GLOBAL_MAX_CLIENT
#define RTSP_DEFAULT_GLOBAL_MAX_CLIENT 4          /* 全局 RTSP 总连接数默认值 */
#endif
#ifndef RTSP_DEFAULT_STREAM_MAX_CLIENT
#define RTSP_DEFAULT_STREAM_MAX_CLIENT 4          /* 单路媒体会话默认连接数 */
#endif
#ifndef RTSP_MAIN_CLIENT_LIMIT_8M
#define RTSP_MAIN_CLIENT_LIMIT_8M 2               /* 主码流达到 8 Mbps 档时的连接数 */
#endif
#ifndef RTSP_MAIN_CLIENT_LIMIT_16M
#define RTSP_MAIN_CLIENT_LIMIT_16M 1              /* 主码流达到 16 Mbps 档时的连接数 */
#endif
/* 默认帧率 */
#define DEFAULT_VIDEO_FPS 25.0f

/* 最大帧大小 */
#define MAX_FRAME_SIZE (1572864)  // 1.5 MB
/* OutPacketBuffer缓存大小 2.5MB */
#define REV_BUF_SIZE  (2621440)
/*
 * RTSP OutPacketBuffer 默认缓存大小
 * 注意：这些是库层默认值，应用层可通过 rtsp_server.h 覆盖
 * 实际使用的值由应用层根据设备能力动态计算
 */
#define RTSP_MAIN_OUT_PACKET_BUFFER_SIZE  (2U * 1024U * 1024U)   /* 2 MiB 默认值 */
#define RTSP_SUB_OUT_PACKET_BUFFER_SIZE   (1U * 1024U * 1024U)   /* 1 MiB 默认值 */
#define RTSP_AUDIO_OUT_PACKET_BUFFER_SIZE (64U * 1024U)          /* 64 KiB 默认值 */

/*封装printf*/
#define live_log(fmt...) \
    do { \
        printf("\033[1;34m[RtspServer][Func]:%s [Line]:%d ", __FUNCTION__, __LINE__); \
        printf(fmt); \
        printf("\033[0;39m\n"); \
        fflush(stdout); \
    } while (0)

typedef enum
{
	RTSPCLIENT_START = 0,
	RTSPCLIENT_PAUSE,
	RTSPCLIENT_STOP,
	RTSPCLIENT_FINISH,
	/* 首个 SETUP 绑定媒体会话前的准入检查，不改变既有状态值。 */
	RTSPCLIENT_ADMISSION,
}Rtsp_Status_t;
typedef enum
{
	RTSP_FRAMEPROTOL_H264 = 0,
	RTSP_FRAMEPROTOL_H265 = 1,
	RTSP_FRAMEPROTOL_MJPEG = 2,
	RTSP_FRAMEPROTOL_NULL = 3,
}Rtsp_Frame_ProtolType_E;
typedef struct Rtsp_Client_State_Info
{
	Rtsp_Status_t status;
	void *param;//由用户自己传进去标识
}Rtsp_ClientStream_State_t;
typedef struct
{
	char ip [RTSP_CLIENT_MAX][RTSP_IP_MAX];
	int nNumClient;
}Rtsp_Client_Info_t;
typedef enum
{
	VIDEO_TYPE = 0,
	AUDIO_TYPE,

}VideoOrAudio_t;
typedef struct fream_Info
{
	int frameSize;
	unsigned int width;
	unsigned int height;
	unsigned int bakfps;
	unsigned int fps;
	float fFps;
	unsigned char *data;
	int iFrame;
	int audio_type;
	unsigned int sample_rate;
	VideoOrAudio_t type;
	int channel;
	void *param;//由用户初始化，自己传进去的标识
	struct timeval presentationTime;
	int videolistsize;
	int audiolistsize;
	Rtsp_Frame_ProtolType_E enVideoDataType;
}Fream_Info_t;
typedef struct
{
	int turnAudio;
}Rtsp_Inparam;//主要用于扩展

typedef int (*ClientStreamStatus)(Rtsp_ClientStream_State_t * param);
typedef int (*RtspClientStreamStatus)(Rtsp_ClientStream_State_t * param, void* rtspdata);
typedef int (*FrameCallBack)(Fream_Info_t* frame);


typedef struct
{
	FrameCallBack dataGetfun;
	ClientStreamStatus clientFun;
	void *Videoindex;//视频流标识,索引
	void *Audioindex;//音频流标识,索引
	char streamName[STREAM_NAME_MAX];
	int nProtolType; /* 视频流类型Rtsp_Frame_ProtolType_E */
	int nAudioType; //0:aac 1:g711u 2:g711a 3:g726
	int nAuidoSamplingFreqIndex; // zhouzr 新增字段，用于设置采样率索引，基于AAC
	int nAudioChannel;  // zhouzr 新增字段，用于设置通道个数
	int nAudioBitWidth;	//zhouzr 新增字段，用于设置采样位宽
	int param1;//扩展参数
	void* param2;//扩展参数
	/* 每个RTPSink实例的输出缓存上限，0表示沿用live555全局默认值。 */
	unsigned int outPacketBufferSize;
	/* 每个音频RTPSink实例的输出缓存上限，0表示沿用live555全局默认值。 */
	unsigned int audioOutPacketBufferSize;
}Rtsp_Create_Info_t;

typedef struct
{
	ClientStreamStatus clientFun;
	FrameCallBack dataGetfun;
	void *audioindex;
	char streamName[STREAM_NAME_MAX];
	int samplingFreqIndex;  // zhouzr 新增字段，用于设置采样率索引，基于AAC
	int channel;  // zhouzr 新增字段，用于设置通道个数
	int bitWidth;	//zhouzr 新增字段，用于设置采样位宽
	/* 音频RTPSink实例的输出缓存上限。 */
	unsigned int outPacketBufferSize;
}Audio_Source_Info_t;

typedef struct
{
	FrameCallBack dataGetfun;
	ClientStreamStatus clientFun;
	void *videoindex;
	char streamName[STREAM_NAME_MAX];
	int nAudio;
	/* 视频RTPSink实例的输出缓存上限。 */
	unsigned int outPacketBufferSize;
}Video_Source_Info_t;

#ifdef __cplusplus
}
#endif
#endif
