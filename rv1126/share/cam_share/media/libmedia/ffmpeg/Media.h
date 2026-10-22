#ifndef __MEDIEA_H__
#define __MEDIEA_H__
#define __STDC_CONSTANT_MACROS
#include <math.h>

#ifdef __cplusplus
extern "C"
{
#endif
#include <libavutil/mathematics.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
	
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <pthread.h>
#define MAX_PATH 512
#endif

#ifdef WIN32

#ifdef MEDIALIB_EXPORTS
#define MEDIA_API  __declspec (dllexport)
#else
#define MEDIA_API	//__declspec (dllimport)
#endif

#else
#include <unistd.h>
#include <time.h>
#define MEDIA_API 
#endif

	
#define STREAM_FRAME_RATE 10000
#define DEFAULT_AUDIO_SAMPLE_RATE 44100
#define MAX_STREAM_NUM 128
	
	enum H264_PROFILE
	{
		PROFILE_BASELINE = 1,
		PROFILE_MAIN,
		PROFILE_HIGH
	};
	//#ultrafast、superfast、veryfast、faster、fast、medium、slow、slower、veryslow、placebo
	enum PRESET
	{
		PRESET_ULTRAFAST = 1,
		PRESET_SUPERFAST,
		PRESET_VERYFAST,
		PRESET_FASTER,
		PRESET_FAST,
		PRESET_MEDIUM,
		PRESET_SLOW,
		PRESET_SLOWER,
		PRESET_VERYSLOW,
		PRESET_PLACEBO
	};
	
	typedef struct _MuxWriter
	{
		AVFormatContext * oc;
		AVBitStreamFilterContext * bsfc;
		AVCodecContext * pAudioCodecCtx;
		AVCodecContext * apVideoCodecCtx[MAX_STREAM_NUM];
		enum AVCodecID eAudioCodecID;
		enum AVCodecID eVideoCodecID;
		char acFileName[MAX_PATH];
		int bMp4File;
		int bAsfFile;
		int abGetSyncFrame[MAX_STREAM_NUM];
		int bStartWriteFrame;
		int bHaveVideo;
#ifdef WIN32
		CRITICAL_SECTION cs;
#else
		pthread_mutex_t mutex;
#endif
	}MuxWriter;
	
	
	typedef struct _DemuxReader
	{
		AVFormatContext * ic;
		AVBitStreamFilterContext * aBsfc[MAX_STREAM_NUM];
		AVCodecContext * apVideoCodecCtx[MAX_STREAM_NUM];
		AVCodecContext * pAudioCodecCtx;
		int aiFrameNum[MAX_STREAM_NUM];
		int64_t aiDuration[MAX_STREAM_NUM];
		int iVideoNum;
		int aiWidth[MAX_STREAM_NUM];
		int aiHeight[MAX_STREAM_NUM];
		int iVideoStreamIdx;
		enum AVCodecID eAudioCodecID;
		int iAudioStreamIdx;
		int iSampleRate;
		int iChannel;
		int iHE_AAC;
		enum AVSampleFormat eAudioFmt;
		int iBitRate;
		int64_t iDuration;
		int bStreamEnd;
		int bMp4H264BitStream;
		int bNeedAACAdtsHeader;
#ifdef WIN32
		CRITICAL_SECTION cs;
#else
		pthread_mutex_t mutex;
#endif
	}DemuxReader;
	
	typedef struct _MediaPacket
	{
		int iStreamIdx; //从文件中读出帧的流索引，从0开始
		int bAudio;     //表示此帧是音频还是视频，为1时是音频，否则是视频
		int bKeyFrame;  //表示此帧是否是关键帧
		int64_t pts;    //表示此帧的pts,单位是ms
		int64_t dts;
		int64_t prev_vdts;
		int64_t prev_adts;
		int iLen;       //表示帧长
		uint8_t* pData;//帧的数据指针
		unsigned int sample_rate;
		unsigned int frame_rate;
		AVPacket pkt;
	}MediaPacket;


#define MAX_AUDIO_PCM_BUF (2 * 192000)
#define MAX_AUDIO_ES_LEN  (32 * 1024)
#define MAX_VIDEO_ES_LEN  (512 * 1024)


	typedef struct _TransContext
	{
		DemuxReader DemuxRdr;
		int bOpenInput;
		MuxWriter MuxWtr;
		int bOpenOutput;


		AVCodecContext * pVideoDecCtx;
		enum AVCodecID eVideoDecID;
		int iDecVideoStreamIdx;
		int iDecOutWidth,iDecOutHeight;
		enum AVPixelFormat DecPixFmt;
		AVCodecContext * pVideoEncCtx;
		enum AVCodecID eVideoEncID;
		int iEncVideoStreamIdx;
		double dVideoPts;
		int iEncWitdth;
		int iEncHeight;
		int bGetVideoDecFrame;
		AVFrame * pVideoDecFrame;
		AVFrame * pVideoEncFrame;
		AVFrame * pAudioDecFrame;
		AVFrame * pAudioEncFrame;
		struct SwsContext * img_convert_ctx;
	


		AVCodecContext * pAudioDecCtx;
		AVCodecContext * pAudioEncCtx;
		enum AVCodecID eAudioDecID;
		enum AVCodecID eAudioEncID;
		int iBitRate;
		int iSampleRate;
		int iChannels;
		int iChannel_layout;
		enum AVSampleFormat eSampleFmt;
		double dAudioPts;
		double dAudioStartPts;
		int bGetAudioDecFrame;
		int iDecAudioStreamIdx;
		int iEncAudioStreamIdx;
		int bAudioResample;
#ifdef USE_SWRESAMPLE
		struct SwrContext *swr;
#else
		AVFilterContext *abuffersink_ctx;
		AVFilterContext *abuffersrc_ctx;
		AVFilterGraph *audio_filter_graph;
#endif
        uint8_t  ucAudioResamplePcmBuf[MAX_AUDIO_PCM_BUF];
		int iResamplePcmLen;

		AVFilterContext *vbuffersink_ctx;
		AVFilterContext *vbuffersrc_ctx;
		AVFilterGraph *video_filter_graph;


	}TransContext;


	/*
	功能:支技多路视频和一路音频mp4或者asf文件读和写，并且支持只写音频的mp4和asf文件，同时支持windows和linux平台.
	需要注意的问题：
	1. VideoStreamAdd，AudioStreamAdd 添加的流，如果添加成功，会返回流索引，这个流索引在调用MediaWriteFrame写入时使用，即 iStreamIdx
	2. 应用需要保证调用MediaWriteFrame 时给的第一路流的pts 是递增的，不然此帧写不进去
	*/
	//注册ffmpeg format和codec
	MEDIA_API void MediaSysInit(); 
	
	//媒体文件复用结构初化化
	MEDIA_API void MediaWriterInit(MuxWriter * pMuxWriter);
	//file 写媒体文件名称, bHaveVideo 表示创建的媒体文件是否有视频
	MEDIA_API int MediaFileCreate(MuxWriter * pMuxWriter,char * file,int bHaveVideo);
	//视频流添加，返回值即为此流的索引，如果是创建多视频流文件，需要多次调用，codec_id 编码视频ID,h.264是CODEC_ID_H264,frame_rate 设置为 STREAM_FRAME_RATE
	//gop 是I帧间隔，profile,表示h.264的profile,preset表示 x264 预置参数
	MEDIA_API int VideoStreamAdd(MuxWriter * pMuxWriter,enum AVCodecID codec_id,int width,int height,int bitrate,int frame_rate);
	//音频流添加，返回值即为此流的索引
	MEDIA_API int AudioStreamAdd(MuxWriter * pMuxWriter,enum AVCodecID codec_id,int sample_rate,int bit_rate,int channel,enum AVSampleFormat sample_fmt);
	//写帧函数，pts 单位是ms, bAudio 表示是否是音频
	MEDIA_API int MediaWriteFrame(MuxWriter * pMuxWriter,MediaPacket * pMediaPkt);
	MEDIA_API int MediaWrite265Frame(MuxWriter * pMuxWriter,MediaPacket * pMediaPkt);
	//媒体文件写尾，在写完所有帧后，最后调用
	MEDIA_API int MediaWriteTrailer(MuxWriter * pMuxWriter);
	MEDIA_API void MediaWriterClose(MuxWriter * pMuxWriter);
	
	//媒体文件解复用初始化
	MEDIA_API void MediaReaderInit(DemuxReader * pDemuxReader);
	//file 输入的文件名
	MEDIA_API int MediaOpenFile(DemuxReader * pDemuxReader,char * file);
	//pMediaPkt 是传入的 MediaPacket 结构指针，函数内部会分配内存来保存读的帧，所以如果调用成功，使用完后，需要释放pMediaPkt->pData指向的内存
	MEDIA_API int MediaReadFrame(DemuxReader * pDemuxReader,MediaPacket * pMediaPkt);
	//iSeekTime 文件绝对时间，单位是ms
	MEDIA_API int MediaSeek(DemuxReader * pDemuxReader,int iSeekTime);
	MEDIA_API void MediaReaderClose(DemuxReader * pDemuxReader);
	
	MEDIA_API void MediaPacketFree(MediaPacket * pkt);
	
	MEDIA_API AVStream *add_audio_stream(AVFormatContext *oc, enum AVCodecID codec_id,int sample_rate,int bit_rate,int channel,enum AVSampleFormat sample_fmt);

	MEDIA_API int MediaTranscodeInit(TransContext * pTransContext,char * InputFile,char * OutputFile);
	MEDIA_API int MediaTranscodeClose(TransContext * pTransContext);

	MEDIA_API int AudioDecInfoInit(TransContext * pTransContext,enum AVCodecID eCodecID,int samplerate,int bitrate,int channels,enum AVSampleFormat eFmt);
	MEDIA_API int AudioEncInfoInit(TransContext * pTransContext,enum AVCodecID eCodecID,int samplerate,int bitrate,int channels,enum AVSampleFormat eFmt);
	MEDIA_API int AudioDeocde(TransContext * pTransContext,MediaPacket * InPkt);
	MEDIA_API int AudioEncode(TransContext * pTransContext,MediaPacket * OutPkt);


	MEDIA_API int VideoDecInfoInit(TransContext * pTransContext,enum AVCodecID eCodecID,int width,int height,int iOutWidth,int iOutHeight,enum AVPixelFormat OuntFmt);
	MEDIA_API int VideoEncInfoInit(TransContext * pTransContext,enum AVCodecID eCodecID,int width,int height,int bitrate,int fps,int gop,int profile,int preset);
	MEDIA_API int VideoDeocde(TransContext * pTransContext,MediaPacket * InPkt);
	MEDIA_API int VideoEncode(TransContext * pTransContext,MediaPacket * OutPkt);
	MEDIA_API int h264_encode_para_set(AVCodecContext * c,int gop,int profile,int preset);
	MEDIA_API int AudioPcmInfoInit(TransContext * pTransContext,int samplerate,int channels,enum AVSampleFormat eFmt);
	MEDIA_API int AudioAddPcmData(TransContext * pTransContext,char * pPcmBuf,int iPcmLen);
	MEDIA_API int VideoFilterInit(TransContext * pTransContext,const char *filters_descr);
	MEDIA_API int VideoFilterInputFrame(TransContext * pTransContext);
	MEDIA_API int VideoFilterOutputFrame(TransContext * pTransContext);

	MEDIA_API int Mp4Recovery(char * RecoveryFile,char * InFile,char * OutFile);
	MEDIA_API int MediaGenIndexImage(char * InFile,char * image,int iInternal,int iStartNum,int iMaxImageNum);//iStartNum need ffmpeg 1.0 
	MEDIA_API void media_yuv_scaler2bgr(int h, int w, int data_len, char *srcData, char * desData);
//	MEDIA_API int media_writeJpeg(AVFrame* pFrame, int width, int height, char *out_file); 
#ifdef __cplusplus
}
#endif


#endif
