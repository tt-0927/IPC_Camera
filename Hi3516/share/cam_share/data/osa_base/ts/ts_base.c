
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#include "os.h"
#include "ts_base.h"

/*打印函数*/
#define ts_printf_log(logFun,format,args...)\
	if(logFun)\
	{\
		logFun("\033[31m[%s:%d]\033[0m" format "\r\n",__func__, __LINE__,##args);\
	}else{\
		printf("\033[31m[%s:%d]\033[0m" format "\r\n",__func__, __LINE__,##args);\
	}


inline const char* ts_ftimestamp(uint32_t t, char* buf)
{
	sprintf(buf, "%02u:%02u:%02u.%03u", t / 3600000, (t / 60000) % 60, (t / 1000) % 60, t % 1000);
	return buf;
}

static void* ts_alloc(void* param, size_t bytes)
{
	ts_info_t *tshandle = (ts_info_t *)param;
	if(bytes > 188)
	{
		ts_printf_log(tshandle->inparam.log,"buff > 188!!");
		return NULL;
	}

	//不允许多线程同时操作同一块静态内存
	char *s_buffer = NULL;
	s_buffer = tshandle->buffer;
	return s_buffer;
}

static void ts_free(void* param, void* packet)
{
	return;
}

static void ts_write(void* param, const void* packet, size_t bytes)
{
	ts_info_t *tshandle = (ts_info_t *)param;

	if(tshandle->inparam.tsWriteCallback)
	{
		/* 将ts流文件上抛 */
		tshandle->inparam.tsWriteCallback(tshandle->inparam.user,packet,bytes);
	}else
	{
		if(tshandle->recfd)
		{
			int startT = OS_getSysTimeInMsec();
			//写文件
			fwrite(packet, bytes, 1, tshandle->recfd);

			int endT = OS_getSysTimeInMsec();
			if((endT - startT) >= 10)
			{
				printf(">>>>>>>>>>>> fwrite bytes[%d]=================== time[%d]\n",bytes,endT - startT);
			}
		}
	}
}



void* ts_init(ts_inparam_t input)
{
	struct mpeg_ts_func_t tshandler;
	ts_info_t *handle = (ts_info_t*)malloc(sizeof(ts_info_t));
	if(handle == NULL)
	{
		ts_printf_log(handle->inparam.log,"malloc error!!");
		return NULL;
	}

	memset(handle,0,sizeof(ts_info_t));
	memset(&tshandler,0,sizeof(struct mpeg_ts_func_t));

	memcpy(&(handle->inparam),&input,sizeof(ts_inparam_t));

	tshandler.alloc = ts_alloc;
	tshandler.write = ts_write;
	tshandler.free = ts_free;

	if(handle->inparam.tsWriteCallback == NULL)
	{
		/* 只写文件 */
		handle->recfd = fopen(handle->inparam.fileName, "wb");
		if(handle->recfd == NULL)
		{
			ts_printf_log(handle->inparam.log,"fopen file[%s] error!!",handle->inparam.fileName);
			if(handle)
			{
				free(handle);	//释放资源
				handle = NULL;
			}
			return NULL;
		}
	}

	handle->tsHandle = mpeg_ts_create(&tshandler, handle);
	if(handle->tsHandle == NULL)
	{
		ts_printf_log(handle->inparam.log,"ts create error!!");
		fclose(handle->recfd);
		if(handle)
		{
			free(handle);	//释放资源
			handle = NULL;
		}
		return NULL;
	}

	return handle;
}

int ts_unInit(void *handle)
{
	if(handle == NULL)
	{
		ts_Log log = NULL;
		ts_printf_log(log,"this argument is NULL!!");
		return -1;
	}

	int ret = 0;
	ts_info_t *tshandle = (ts_info_t *)handle;

	//关闭文件
	if(tshandle->recfd)
	{
		fclose(tshandle->recfd);
		tshandle->recfd = NULL;
	}

	//关闭ts
	mpeg_ts_destroy(tshandle->tsHandle);

	//释放资源
	if(tshandle)
	{
		free(tshandle);	//释放资源
		tshandle = NULL;
	}

	return 0;
}


//添加视频流索引
int ts_add_StreamID(void* handle,int avtype,const void* extra_data, size_t extra_data_size)
{
	int ret = 0;
	ts_info_t *tshandle = (ts_info_t *)handle;

	switch(avtype)
	{
	/*视频*/
	case STREAM_VIDEO_MPEG4:
	case STREAM_VIDEO_H264:		//H264
	case STREAM_VIDEO_H265:
	case STREAM_VIDEO_SVAC:
		tshandle->stream_video = mpeg_ts_add_stream(tshandle->tsHandle, avtype, extra_data, extra_data_size);
		if(tshandle->stream_video < 0)
		{
			ts_printf_log(tshandle->inparam.log,"[Video] mpeg_ts_add_stream error!!");
			return -1;
		}
		break;

	/*音频*/
	case STREAM_AUDIO_MP3:
	case STREAM_AUDIO_AAC:
	case STREAM_AUDIO_G711:
	case STREAM_AUDIO_G722:
	case STREAM_AUDIO_G723:
	case STREAM_AUDIO_G729:
	case STREAM_AUDIO_SVAC:
		tshandle->stream_audio = mpeg_ts_add_stream(tshandle->tsHandle, avtype, extra_data, extra_data_size);
		if(tshandle->stream_audio < 0)
		{
			ts_printf_log(tshandle->inparam.log,"[Audio] mpeg_ts_add_stream error!!");
			return -1;
		}
		break;

	default:
		ts_printf_log(tshandle->inparam.log,"this avtype[%d] is not support!!!",avtype);
		ret = -1;
		break;
	}

	return ret;
}




//iskeyFrame: 1-关键帧，0-非关键帧
//avtype:video:STREAM_VIDEO_H264 audio:STREAM_AUDIO_AAC
//pts == dts
//video pts: pts_v = (video_cnt) * 1000 / fps;
//audio pts: pts_v = (audio_cnt) * (framesize * 1000) / samplerate;((audio_cnt) * (1024 * 1000) / 44100)
int ts_write_frame(void* handle,int avtype,char* data,int bytes,int64_t pts,int64_t dts,int iskeyFrame)
{
	int ret = 0;
	ts_info_t *tshandle = (ts_info_t *)handle;

	ret = mpeg_ts_write(tshandle->tsHandle, STREAM_VIDEO_H264 == avtype ? tshandle->stream_video :tshandle->stream_audio, \
			iskeyFrame, pts * 90, dts * 90, data, bytes);
	if(ret < 0)
	{
		ts_printf_log(tshandle->inparam.log,"mpeg_ts_write error!!");
		return -1;
	}

	return 0;
}


/**************************************/

void* ts_demux_init(ts_demuxInparam_S input)
{
	ts_demux_S* handle = (ts_demux_S*)malloc(sizeof(ts_demux_S));
	if(handle == NULL)
	{
		ts_printf_log(input.log,"malloc error!!!\n");
		return NULL;
	}
	memset(handle,0,sizeof(ts_demux_S));
	memcpy(&(handle->input),&input,sizeof(ts_demuxInparam_S));

	handle->tsHandle = ts_demuxer_create(handle->input.onpacket, \
			handle->input.user);
	if(handle->tsHandle == NULL)
	{
		ts_printf_log(input.log,"ts_demuxer_create error!!!\n");
		free(handle);
		handle = NULL;
		return NULL;
	}

	return handle;
}


int ts_demux_uninit(void *handle)
{
	if(handle == NULL)
	{
		printf("this argument is NULL!!!\n");
		return -1;
	}

	ts_demux_S* tshandle = (ts_demux_S*)handle;
	if(tshandle->tsHandle)
	{
		ts_demuxer_flush((struct ts_demuxer_t*)tshandle->tsHandle);
		ts_demuxer_destroy((struct ts_demuxer_t*)tshandle->tsHandle);
	}

    free(tshandle);
    tshandle = NULL;
	return 0;
}

int ts_demux_send(void* handle, const uint8_t* data, size_t bytes)
{
	if(handle == NULL)
	{
		printf("this argument is NULL!!!\n");
		return -1;
	}

	int ret = 0;
	ts_demux_S* tshandle = (ts_demux_S*)handle;
	if(bytes != 188)
	{
		ts_printf_log(tshandle->input.log,"this data size is no 188!!!\n");
		return -1;
	}

	if(tshandle->tsHandle)
	{
		ret = ts_demuxer_input(tshandle->tsHandle, data, bytes);
	}
	return ret;
}














