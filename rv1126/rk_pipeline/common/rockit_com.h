/*** 
 * @FilePath     : rockit_com.h
 * @Author       : luoyk 
 * @Date         : 2022-12-08 08:48:16
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-03-14 17:59:04
 * @Description  : 
 */

#ifndef _ROCKIT_COM_H_
#define _ROCKIT_COM_H_
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "rk_mpi_mb.h"
#include "rk_comm_video.h"
#include "rk_mpi_cal.h"
#include "rk_mpi_mmz.h"
#include "rockit_venc.h"
#include "rk_mpi_sys.h"
#include "rockit_tde.h"
/*分配帧缓冲*/
VIDEO_FRAME_INFO_S * alloc_nocache_tdeFrame( int nWidth, int nHeight, PIXEL_FORMAT_E enPixFormat, COMPRESS_MODE_E enCompressMode  );
VIDEO_FRAME_INFO_S * alloc_tdeFrame( int nWidth, int nHeight, PIXEL_FORMAT_E enPixFormat, COMPRESS_MODE_E enCompressMode  );
/*释放帧缓冲*/
void free_tdeFrame( VIDEO_FRAME_INFO_S* pFrame );
/*vgs-分配帧缓冲*/
VIDEO_FRAME_INFO_S* alloc_vgsFrame(int nWidth, int nHeight, PIXEL_FORMAT_E enPixFormat, COMPRESS_MODE_E enCompressMode);
/*vgs-释放帧缓冲*/
void free_vgsFrame(VIDEO_FRAME_INFO_S* pFrame);
/*TDE缩放*/
int resize_frame( VIDEO_FRAME_INFO_S* pSrcFrame, VIDEO_FRAME_INFO_S* pDstFrame );
/*TDE缩放或拷贝到指定的位置*/
int resize_rect_frame( VIDEO_FRAME_INFO_S* pSrcFrame, VIDEO_FRAME_INFO_S* pDstFrame, int nX, int nY, int nW, int nH );
int quickRect_frame( VIDEO_FRAME_INFO_S* pDstFrame);
/*虚拟地址转视频帧*/
VIDEO_FRAME_INFO_S* dataToFrame( char* pData, int nSize, PIXEL_FORMAT_E enPixForMat, int nWidth, int nHeight);

/*虚拟地址编码成MJPEG*/
int dataToMjpeg(int nChn, int nWidth, int nHeight, PIXEL_FORMAT_E eFormat, char* pInData, int nInSize, char** pOutData, int* pOutSize);
/*将frame编码成MJPEG*/
int frameToMjpeg(int nChn, VIDEO_FRAME_INFO_S* pVFrame, char** pOutData, int* pOutSize);

/**
 * @brief       : 拷贝视频帧结构体数据
 * @author      : zhouzirui
 * @param        {VIDEO_FRAME_INFO_S} *pstDst	目标视频帧指针
 * @param        {VIDEO_FRAME_INFO_S} *pstSrc	源视频帧指针
 * @return       {*}
 */
void copyVideoFrame(VIDEO_FRAME_INFO_S *pstDst, const VIDEO_FRAME_INFO_S *pstSrc);

/**
 * @brief       : 释放视频帧结构体数据
 * @author      : zhouzirui
 * @param        {VIDEO_FRAME_INFO_S} *pstFrame	视频帧指针
 * @return       {*}
 */
void freeVideoFrame(VIDEO_FRAME_INFO_S *pstFrame);

#endif
