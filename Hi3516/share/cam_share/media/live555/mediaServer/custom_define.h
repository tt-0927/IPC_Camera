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

/* 默认帧率 */
#define DEFAULT_VIDEO_FPS 25.0f

/* 最大帧大小 */
#define MAX_FRAME_SIZE (1572864)  // 1.5 MB
/* OutPacketBuffer缓存大小 2.5MB */
#define REV_BUF_SIZE  (2621440)
/* 主码流先保留2MiB安全余量，待VENC基线确认最大I帧后再继续下调。 */
#define RTSP_MAIN_OUT_PACKET_BUFFER_SIZE (2U * 1024U * 1024U)
/* 子码流通常码率和I帧显著较小，单独限制每个RTPSink的缓存。 */
#define RTSP_SUB_OUT_PACKET_BUFFER_SIZE  (1U * 1024U * 1024U)
/* 音频帧远小于视频，避免沿用视频级2.5MiB缓存。 */
#define RTSP_AUDIO_OUT_PACKET_BUFFER_SIZE (64U * 1024U)

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
