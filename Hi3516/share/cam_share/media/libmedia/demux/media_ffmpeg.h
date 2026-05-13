

#ifndef _CORE_SOURCE_PLAYER_MEDIA_FFMPEG_INCLUDE_
#define _CORE_SOURCE_PLAYER_MEDIA_FFMPEG_INCLUDE_




#ifdef __cplusplus
extern "C" {
#endif

#include "os_debug.h"
#include "mediaAVbuffer.h"
#include "libavcodec/avcodec.h"
#include "libavutil/dict.h"
#include "libavutil/error.h"
#include "libavutil/opt.h"
#include "libavformat/avio.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "media_clock.h"

typedef int(*decCallback)(AVFrame *frame,void* user);

#define AUDIO_BIT_SIZE 16
#define AUDIO_SAMPLERATE_48K   48000
#define AUDIO_SAMPLERATE_441K  44100

#define G726_BIT_RATE_16K  16000
#define G726_BIT_RATE_24K  24000
#define G726_BIT_RATE_32K  32000
#define G726_BIT_RATE_40K  40000





typedef enum
{
	/* packet类型 */
	MEDIA_TYPE_NULL = 0,
    MEDIA_TYPE_VIDEO,
    MEDIA_TYPE_AUDIO,
    MEDIA_TYPE_SUBTITLE,
	MEDIA_TYPE_NB_T		/* 保留字段，最后一个属性，便于确定枚举的大小 */
}mediaType_t;

typedef enum
{
	/* 流类型视频流/复合流 */
	STREAM_DEMUX_TYPE_VIDEO = 0x1,
	STREAM_DEMUX_TYPE_COMPOSITE = 0x3,
} STREAM_DEMUX_TYPE_E;

struct timeBase_S
{
    int num; ///< Numerator
    int den; ///< Denominator
};

/* 打开的流媒体信息 */
typedef struct _MEIDA_PARAM_INFO_S
{
	char url[2048];		/* 视频路径 */
    double frameRate;   //video fps
    int width;			//video width
    int height;			//video height
    int sampleRate;		//audio sample rate
    int channel;		//audio channels
    int track;			//How many tracks
    struct timeBase_S time_base[MEDIA_TYPE_NB_T];
    int ntrack[MEDIA_TYPE_NB_T];
    long long duration;	//总时长（单位：微秒us，转换为秒需要除以1000000）

}mediaParam_S;

typedef struct timeClock_S_
{
    mediaClock_S viclk;
    mediaClock_S auclk;
    mediaClock_S extclk;
    int syn_master_clock;	//以哪个时钟为主时钟
}timeClock_S;

typedef struct _MEDIA_FFMPEG_INCLUDE_
{
	AVFormatContext* formatCtx;
	AVCodecContext* pVideoCodecCtx;
	AVCodecContext* pAudioCodecCtx;
	AVBitStreamFilterContext* videoBsfc;
	AVBitStreamFilterContext* audioBsfc;
	AVCodecParserContext *pAudioParser;

	STREAM_DEMUX_TYPE_E streamType;

	AVFrame * pAudioDecFrame;
	int bGetAudioDecFrame;

	/* 音频重采样 */
	struct SwrContext *swr_ctx;
	int64_t audioResampleLayout;
	int audioResampleFormat;
	int audioResampleSampleRate;
	int audioMaxDstNbSamples;
	AVFrame* pAudioResampleFrame;

	decCallback audioDecSink;
	void* audioDecUser;

	double videofps;
	int audio_stream;
	int video_stream;
	int subtitle_stream;

	unsigned int startTime;

	/* 解析的媒体参数 */
	mediaParam_S demuxParam;

	/* 时钟 */
	timeClock_S clock;

	void* user;
	OS_log log;
}mediaFfmpeg_t;


typedef struct _MEDIA_PACKET_INCLUDE_
{
    int64_t pts;
    int64_t dts;
    uint8_t *data;
    int   size;
    mediaType_t type;
    int64_t duration;
    int64_t pos;                 ///< byte position in stream, -1 if unknown
    //方便解码的时候选择解码器
    enum AVCodecID     codec_id;
	int isKeyFrame;
}mediaPacket_t;


mediaFfmpeg_t* media_open_url(char* url,long long startTime,OS_log log,void* user,int (*interrupt_callback)(void*));

int media_get_frame(mediaFfmpeg_t* handle,mediaPacket_t* paket);

int media_unpaket(mediaFfmpeg_t* handle,mediaPacket_t* pkt);

/* 获取数据，packet采用引用技术机制，
 * 不使用后需要调用avMedia_packet_unref(paket)函数释放
 * */
int media_get_packetRef(mediaFfmpeg_t* handle,AVMediaPacket_S* paket);

/* 关闭媒体
 * @param[in] handle:媒体句柄
 *  */
int media_close_url(mediaFfmpeg_t* handle);

/* 获取当前的媒体信息
 * @param[in] handle:媒体句柄
 *  */
int media_get_demuxInfo(mediaFfmpeg_t* handle,\
		mediaParam_S* demuxParam);

/* 获取当前播放的时间
 * @param[in] handle:媒体句柄
 * */
double media_get_time(mediaFfmpeg_t* handle);

/* 快进快退
 * @param[in] handle:媒体句柄
 * @param[in] seek_pos:跳到第几秒
 * */
int media_seek(mediaFfmpeg_t* handle,double seek_pos);


/* 设置音频解码的回调接口 */
int media_setDecoderSink(mediaFfmpeg_t* handle,decCallback sink,void *user);

/* 送音频数据解码，
 * @[in] handle:模块句柄
 * @[in] pMediaPkt:待解码的数据
 * @[in] isResample:是否需要重采样，1-是，0-否
 * @[in] layout:需要重采样的音频数据布局，如：AV_CH_LAYOUT_MONO，AV_CH_LAYOUT_STEREO
 * @[in] sampleRate:需要重采样的音频采样率：如：48000
 * @[in] format:需要重采样的音频格式，如：AV_SAMPLE_FMT_S16P
 *  */
int media_decode_audio(mediaFfmpeg_t* handle,mediaPacket_t* pMediaPkt,\
		int isResample,\
		int layout,int sampleRate,enum AVSampleFormat format);

/* pcm音频数据重采样，
 * @[in] handle:模块句柄
 * @[in] pMediaPkt:待解码的数据
 * @[in] isResample:是否需要重采样，1-是，0-否
 * @[in] layout:需要重采样的音频数据布局，如：AV_CH_LAYOUT_MONO，AV_CH_LAYOUT_STEREO
 * @[in] sampleRate:需要重采样的音频采样率：如：48000
 * @[in] format:需要重采样的音频格式，如：AV_SAMPLE_FMT_S16P
 *  */
int media_pcm_audio(mediaFfmpeg_t* handle,mediaPacket_t* pMediaPkt,\
		int isResample,\
		int layout,int sampleRate,enum AVSampleFormat format);



void media_writeAudioFile(const AVFrame *frame, const char *pPath);



/* demo */
int media_test_demo(char* url);

#ifdef __cplusplus
}
#endif
#endif //_CORE_SOURCE_PLAYER_MEDIA_FFMPEG_INCLUDE_

