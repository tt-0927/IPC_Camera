/*
 *  File Name: rk_human_count_detect.cpp
 *  Created on: 2023年7月13日
 *  Author: wcp
 *  Modify date: 2023年7月18日
 */


#ifndef _RKNN_HUMAN_COUNT_POSTPROCESS_H_
#define _RKNN_HUMAN_COUNT_POSTPROCESS_H_

#include <iostream>
#include <set>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/time.h>
#include <vector>

// #include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"

/******************************** 外部调用参数（可调） ******************************/
typedef struct _POINTPARAM_
{
    /* 绘制关键点的半径大小 */
    int nRadius = 5;
    /* 绘制关键点的颜色 */
    int nB = 255, nG = 0, nR = 0;
} PointParam_S;

typedef struct _TEXTPARAM_
{
    /* 文字内容 */
    char* strContextTxt = "";
    /* 绘制文字的位置 */
    int   nDrawWidth    = 10;
    int   nDrawHeight   = 100;
    /* 绘制关键点的颜色 */
    int   nB = 0, nG = 255, nR = 255;
} TextParam_S;

typedef struct _TUNABLEPARAM_
{
    /* 是否需要绘制关键点和人数
     *img_draw为：（通过对象.img_draw=xx,即可设置改变了）
     *0 直接返回原图
     *1 绘制关键点和人数
     */
    short        nImgDraw = 1;
    /* opencv连通器中的最小面积参数 */
    int          nMinArea = 3;
    /* 绘制关键点 */
    PointParam_S stPointParam;
    /* 绘制文字 */
    TextParam_S  stTextParam;

} TunableParam_S;

/* 对输出的图片进行后处理
 * pInput0： 神经网络的第一个输出
 * pInput1： 神经网络的第二个输出
 * aSrc_img： 输入的视频流图片数据，用于绘制文字等
 * nHeight,nWidth：推理图片的宽高
 * aImgNew,aLabels,aStats, aCentroids：opencv连通组件的缓存变量，无实际意义
 * nImgDraw：是否需要对输入的src_img绘制文字等；0-不绘制，1-绘制人数和关键点
 * nPepleNum：图片识别到的人数
 */
int ctrlNetoutputProcess(float*              pInput0,
                         float*              pInput1,
                         int                 nHeight,
                         int                 nWidth,
                         TunableParam_S      stTunableParam,
                         int&                nPepleNum,
                         std::vector<float>& vPointsXY);

#endif    //_RKNN_HUMAN_COUNT_POSTPROCESS_H_
