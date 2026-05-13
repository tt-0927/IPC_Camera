

#ifndef _LIBMEDIA_SOURCE_CODE_TS_INCLUDE_
#define _LIBMEDIA_SOURCE_CODE_TS_INCLUDE_

#include <stdio.h>

#include "ts_base.h"
#include "mpeg-ts.h"
#include "mpeg-ps.h"
#include "mpeg-ts-proto.h"

#ifdef __cplusplus
extern "C" {
#endif

/*用于输出调试信息的函数指针*/
typedef int(*ts_Log)(const char *format, ...);

/* 复用ts后上抛ts数据 */
typedef void (*ts_writeCallback)(void* user, const void* packet, size_t bytes);

typedef struct _TS_PARAME_INPUT_
{
	/* 录制ts文件名,若赋值下面的回调指针，则不将ts写文件了 */
	char fileName[512];
	/* 复用ts后上抛ts数据,有该回调函数后，就不写文件了，全部异步上抛数据 */
	ts_writeCallback tsWriteCallback;
	/* 回调的上层带入的数据 */
	void *user;
	/* 日志句柄 */
	ts_Log log;
}ts_inparam_t;


typedef struct _TS_HANDLE_T_
{
	void *tsHandle;		//ts句柄
	char buffer[188];

	int stream_video;	//视频流id
	int stream_audio;	//音频流id

	FILE* recfd;		//文件句柄
	ts_inparam_t inparam;	//传入的参数

}ts_info_t;




void* ts_init(ts_inparam_t input);	//初始化句柄

int ts_unInit(void *handle);	//关闭文件

//添加流索引ID
int ts_add_StreamID(void* handle,int avtype,const void* extra_data, size_t extra_data_size);

//写帧数据
int ts_write_frame(void* handle,int avtype,char* data,int bytes,int64_t pts,int64_t dts,int iskeyFrame);


/************ts demux*********************/

typedef struct _TS_DEMUX_PARAME_
{
	/*日志句柄*/
	ts_Log log;
	/* 解封装数据上抛的回调函数 */
	ts_dumuxer_onpacket onpacket;
	void *user;
}ts_demuxInparam_S;

typedef struct _TS_DEMUX_S_
{
	void *tsHandle;		//ts句柄
	char buffer[188];
	ts_demuxInparam_S input;
}ts_demux_S;

void* ts_demux_init(ts_demuxInparam_S input);

int ts_demux_uninit(void *handle);

int ts_demux_send(void* handle, const uint8_t* data, size_t bytes);

#ifdef __cplusplus
}
#endif

#endif	//_LIBMEDIA_SOURCE_CODE_TS_INCLUDE_


