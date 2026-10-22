/*
 * @Descripttion:
 * @version: v1.0
 * @Author: zhangjunbin
 * @Date: 2021-09-01 13:45:30
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mediaAVbuffer.h"


/* 创建packet内存 */
int avMedia_Packet_create(
		AVMediaPacket_S* packet,\
		unsigned char* data,int size,\
		int nKeyFrame,int nWidth,int nHeight,\
        PACKET_TYPE_E type, PACKET_FORMAT_E format, \
		double frameRate,int bitRate,\
		int audioSampleRate,int audioBitRate, \
		int64_t pts,int64_t dts,int isLostPacke,\
		bufferRefFreeInterface freeInterface,void *user)
{
	if(packet == NULL)
	{
		printf("this packet is null. error!!!\n");
		return -1;
	}
	memset(packet,0,sizeof(AVMediaPacket_S));

	packet->pData = data;
	packet->nSize = size;
	packet->nKeyFrame = nKeyFrame;
	packet->nWidth = nWidth;
	packet->nHeight = nHeight;
	packet->type = type;
	packet->format = format;
	packet->frameRate = frameRate;
	packet->bitRate = bitRate;
	packet->audioSampleRate = audioSampleRate;
	packet->audioBitRate = audioBitRate;
	packet->pts = pts;
	packet->dts = dts;
	packet->isLostPacke = isLostPacke;
	/* 创建引用 */
	packet->bufRef = bufferRef_create(data,size,freeInterface,user);
	if(packet->bufRef == NULL)
	{
		printf("bufferRef_create error!!\n");
		return -1;
	}

	return 0;
}

/* 解引用packet内存 */
int avMedia_packet_unref(AVMediaPacket_S* packet)
{
	if(packet == NULL){
		return -1;
	}

	if(packet->bufRef)
	{
		bufferRef_unref(&(packet->bufRef));
		packet->bufRef = NULL;
	}

	return 0;
}

/* 引用packet内存 */
int avMedia_packet_ref(AVMediaPacket_S *dst, \
		const AVMediaPacket_S *src)
{
	if((dst == NULL) || (src == NULL)){
		return -1;
	}

	dst->pData = src->pData;
	dst->nSize = src->nSize;
	dst->nKeyFrame = src->nKeyFrame;
	dst->nWidth = src->nWidth;
	dst->nHeight = src->nHeight;
	dst->type = src->type;
	dst->format = src->format;
	dst->frameRate = src->frameRate;
	dst->bitRate = src->bitRate;
	dst->audioSampleRate = src->audioSampleRate;
	dst->audioBitRate = src->audioBitRate;
    dst->pts = src->pts;
    dst->dts = src->dts;
    dst->nTimebase = src->nTimebase;
	/* 引用内存  */
	if(src->bufRef)
	{
		dst->bufRef = bufferRef_ref(src->bufRef);
	}
	return 0;
}


/////////////////////////////////////////////


int avMedia_frame_create(
		AVMediaFrame_S* frame,\
		unsigned char* pVirAddr[], int addrCount, \
		int lineSize[], int lineSizeCount, \
		int nWidth,int nHeight,\
		int format, \
		int audioSampleRate,\
	    int64_t pts,int64_t dts,\
		bufferRefFreeInterface freeInterface,void *user)
{
	if(frame == NULL)
	{
		printf("this frame is null. error!!!\n");
		return -1;
	}
	memset(frame,0,sizeof(AVMediaFrame_S));

	int i = 0;
	for(i = 0; i < VIDEO_FRAME_MAX_CHN;i++)
	{
		if(i < addrCount){
			frame->pVirAddr[i] = pVirAddr[i];
		}else{
			frame->pVirAddr[i] = NULL;
		}

		if(i < lineSizeCount){
			frame->lineSize[i] = lineSize[i];
		}else{
			frame->lineSize[i] = 0;
		}
	}

	frame->width = nWidth;
	frame->height = nHeight;
	frame->format = format;
	frame->audioSampleRate = audioSampleRate;
	frame->pts = pts;
	frame->dts = dts;

	/* 创建引用 */
	frame->bufRef = bufferRef_create(\
			pVirAddr[0],lineSize[0],\
			freeInterface,user);
	if(frame->bufRef == NULL)
	{
		printf("bufferRef_create error!!\n");
		return -1;
	}

	return 0;
}


int avMedia_frame_unref(AVMediaFrame_S* frame)
{
	if(frame == NULL){
		return -1;
	}

	if(frame->bufRef)
	{
		bufferRef_unref(&(frame->bufRef));
		frame->bufRef = NULL;
	}

	return 0;
}


int avMedia_frame_ref(AVMediaFrame_S *dst, \
		const AVMediaFrame_S *src)
{
	if((dst == NULL) || (src == NULL)){
		return -1;
	}

	int i = 0;
	for(i = 0; i < VIDEO_FRAME_MAX_CHN;i++)
	{
		dst->pVirAddr[i] = src->pVirAddr[i];
		dst->lineSize[i] = src->lineSize[i];
	}
	dst->width = src->width;
	dst->height = src->height;
	dst->format = src->format;
	dst->audioSampleRate = src->audioSampleRate;

	/* 引用内存  */
	if(src->bufRef)
	{
		dst->bufRef = bufferRef_ref(src->bufRef);
	}
	return 0;
}




















