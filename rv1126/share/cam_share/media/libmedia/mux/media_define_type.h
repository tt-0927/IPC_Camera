#pragma once

#include <stdint.h>

typedef enum Capture_Data_Type
{
	CAPTURE_AUDIO_TYPE = 0,
	CAPTURE_VIDEO_TYPE = 1,
    CAPTURE_VIDEO_ALL = 2
}Capture_Data_Type_t;

typedef enum
{
	/* 流类型视频流/复合流 */
	STREAM_MUX_TYPE_COMPOSITE,
	STREAM_MUX_TYPE_VIDEO,
	STREAM_MUX_TYPE_BUTT,
}STREAM_MUX_TYPE_E;


typedef struct Capture_CallBack_Data
{
	unsigned char *data;
	int size;
	int keyframe;
	Capture_Data_Type_t data_type;
	STREAM_MUX_TYPE_E streamType;
    int64_t nPts;
    int64_t nDts;
    int nTimebase;
}Capture_CallBack_Data_t;
