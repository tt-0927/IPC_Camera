/*
 * @Descripttion:
 * @version: v1.0
 * @Author: zhangjunbin
 * @Date: 2021-09-01 13:45:30
 */

#ifndef SOURCE_CORE_OSA_MEDIA_AV_BUFFER_INCLUDE_H_
#define SOURCE_CORE_OSA_MEDIA_AV_BUFFER_INCLUDE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include "buffer_ref.h"

typedef enum PACKET_TYPE_E_
{
    PACKET_TYPE_VIDEO,
    PACKET_TYPE_AUDIO,
    PACKET_TYPE_SUBTITLE,
    PACKET_TYPE_POLICE_AUDIO,
    PACKET_TYPE_BUTT
}PACKET_TYPE_E;

typedef enum PACKET_FORMAT_E_
{
    PACKET_FORMAT_H264,
    PACKET_FORMAT_H265,
    PACKET_FORMAT_AAC,

    PACKET_FORMAT_BUTT
}PACKET_FORMAT_E;

typedef struct AV_MEDIA_PACKET_S
{
	/* 引用技术管理内存 */
	bufferRefHndl_S* bufRef;
	unsigned char* pData;
	int nSize;
	PACKET_TYPE_E type;
	PACKET_FORMAT_E format;
	int nKeyFrame;
	int nWidth;
	int nHeight;
	double frameRate;
	int bitRate;
	int audioSampleRate;
	int audioBitRate;
    int64_t pts;
    int64_t dts;
	int isLostPacke;
    int nTimebase;
    int nDuration;
}AVMediaPacket_S;


/* 创建packet内存 */
int avMedia_Packet_create(
		AVMediaPacket_S* packet,\
		unsigned char* data,int size,\
		int nKeyFrame,int nWidth,int nHeight,\
        PACKET_TYPE_E type, PACKET_FORMAT_E format, \
		double frameRate,int bitRate,\
		int audioSampleRate,int audioBitRate, \
	    int64_t pts,int64_t dts,int isLostPacke,\
		bufferRefFreeInterface freeInterface,void *user);

/* 解引用packet内存,
 * @param[in] packet:引用的packet对象
 * 注意:如果是最后一个使用,则会使用packet里面的内存,
 * */
int avMedia_packet_unref(AVMediaPacket_S* packet);

/* 引用packet内存
 * @param[in] dst:引用之后的packet对象
 * @param[in] src:被引用的packet对象
 * @param[out] return :0-success ,<0 - 失败
 *  */
int avMedia_packet_ref(AVMediaPacket_S *dst, \
		const AVMediaPacket_S *src);


///////////////////////frame//////////////////////////

/* 视频数据里面最多多少个通道 */
#define VIDEO_FRAME_MAX_CHN (4)

typedef enum _PIXEL_FORMAT_E_
{
    /*RGB*/
    PIXEL_FORMAT_RGB_888,//packed RGB 8:8:8, 24bpp, RGBRGB...(AV_PIX_FMT_RGB24)
    PIXEL_FORMAT_ARGB_8888,//packed ARGB 8:8:8:8, 32bpp, ARGBARGB...(AV_PIX_FMT_ARGB)
    PIXEL_FORMAT_RGBA_8888,//packed RGBA 8:8:8:8, 32bpp, RGBARGBA...(AV_PIX_FMT_RGBA)

    PIXEL_FORMAT_YUV_PLANAR_422,//planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples)(AV_PIX_FMT_YUV422P)
    PIXEL_FORMAT_YUV_PLANAR_420,//planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)(AV_PIX_FMT_YUV420P)
    PIXEL_FORMAT_YUV_PLANAR_444,//planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples)(AV_PIX_FMT_YUV444P)

    //interleaved chroma YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples)
    PIXEL_FORMAT_YUV_SEMIPLANAR_422,    //(AV_PIX_FMT_NV16)
    //planar YUV 4:2:0, 12bpp, 1 plane for Y and 1 plane for the UV components, which are interleaved (first byte U and the following byte V)
    PIXEL_FORMAT_YUV_SEMIPLANAR_420,    //(AV_PIX_FMT_NV12)
    PIXEL_FORMAT_YUV_SEMIPLANAR_444,    //(AV_PIX_FMT_NV24)

    PIXEL_FORMAT_BUTT           //保留，1.知道枚举的大小，2.颜色空间格式保留源格式，即不进行颜色空间转换
}PIXEL_FORMAT_E;



typedef struct AV_MEDIA_FRAME_S
{
	/* 引用技术管理内存 */
	bufferRefHndl_S* bufRef;
    unsigned char* pVirAddr[VIDEO_FRAME_MAX_CHN];
    int lineSize[VIDEO_FRAME_MAX_CHN];
    int width;
    int height;
    int format;	/* 音频跟视频的格式 PIXEL_FORMAT_E */
	int audioSampleRate;
    int64_t pts;
    int64_t dts;

}AVMediaFrame_S;


/* 创建packet内存 */
int avMedia_frame_create(
		AVMediaFrame_S* frame,\
		unsigned char* pVirAddr[], int addrCount, \
		int lineSize[], int lineSizeCount, \
		int nWidth,int nHeight,\
		int format, \
		int audioSampleRate,\
	    int64_t pts,int64_t dts,\
		bufferRefFreeInterface freeInterface,void *user);

/* 解引用frame内存,
 * @param[in] frame:引用的frame对象
 * 注意:如果是最后一个使用,则会使用frame里面的内存,
 * */
int avMedia_frame_unref(AVMediaFrame_S* frame);

/* 引用packet内存
 * @param[in] dst:引用之后的frame对象
 * @param[in] src:被引用的frame对象
 * @param[out] return :0-success ,<0 - 失败
 *  */
int avMedia_frame_ref(AVMediaFrame_S *dst, \
		const AVMediaFrame_S *src);


#ifdef __cplusplus
}
#endif


#endif //SOURCE_CORE_OSA_MEDIA_AV_BUFFER_INCLUDE_H_
