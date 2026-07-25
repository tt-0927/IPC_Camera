/*
 *  File Name: rk_human_count_detect.h
 *  Created on: 2023年7月13日
 *  Author: wcp
 *  description : 人数统计类接口定义
 *       1、通过类.nPepleNum,可以获得视频流中的人数（int）
 *       2、通过类.DetectHumanBgr函数，可以获得处理后的视频流
 *          - stTunableParam.nImgDraw = 0 ，返回不处理的视频流
 *          - stTunableParam.nImgDraw = 1，返回绘制关键点和人数的视频流
 *       3.结构体stTunableParam，可以通过修改其中的绘制参数来调整绘制效果
 *  Modify date: 2023年7月18日
 */

#ifndef __RK_HUMAN_COUNT_DETECT_H__
#define __RK_HUMAN_COUNT_DETECT_H__

#include <dlfcn.h>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/time.h>

/* rga */
// #include "RgaUtils.h"
// #include "im2d.h"
/* rknn的api */
#include "rknn_api.h"
// #include "rk_mpi_mmz.h"
/* 自定义的预处理头文件 */
#include "rk_human_count_process.h"

#define STB_IMAGE_IMPLEMENTATION

using namespace std;

class RK_COUNT_DETECT
{
private:

    /******************************** 默认参数 ******************************/
    /* 返回值 */
    short                 m_nRet;
    int                   m_nModelDataSize;
    /* rk模型地址 */
    const char*           m_pModelName;
    /* 定义网络相关的数据 */
    rknn_context          uCtx;
    rknn_input_output_num stIoNum;
    rknn_tensor_attr      stInputAttrs[1];
    rknn_tensor_attr      stOutputAttrs[3];
    rknn_input            aInputs[1];
    rknn_output           aOutputs[3];
    /* 模型输入图片的缓存 */
    cv::Mat               aSrcImg;
    cv::Mat               aLabels, aStats, aCentroids;
    cv::Mat               aImgNew;
    /* 模型需要数据的宽高和通道  */
    int                   nModelChannel;
    int                   nModelWidth;
    int                   nModelHeight;
    /* 存放模型的二进制文件  */
    unsigned char*        pModelData;
    /* 多帧结果融合算法参数 */
    bool                  bFlag = true;
    std::vector<float>    vPoints_Result;    // 最终合成的人数位置
    std::vector<float>    vPoints_remove;    // 缓存即将移除的人数位置
    std::vector<int>      vCout;             // 用于统计识别到前后两帧人数的次数
    std::vector<int>      vDelCout;          // 用于统计即将删除的缓存容器，前后两帧匹配的次数

    /******************************** 可调参数 ******************************/
    /* 输入视频流数据的宽高和通道
     * 这里视频流数据的宽高和通道，应该和模型宽高（nModelChannel、model_width、model_height）对应相等
     * 模型查看网址：https://netron.app/，可打开rknn即可看到输入和输出的尺寸
     */
    int                nImgWidth;
    int                nImgHeight;
    int                nImgChannel;
    /* 识别到的人数 */
    int                nPepleNum;
    /* 识别到人头的坐标，n*(1+1) */
    std::vector<float> vPointsXY;

    /******************************** 内部函数的定义，无需操作 ******************************/
    /* 载入模型  */
    unsigned char* LoadModel(const char* filename, int* model_size);
    /* rknn模型的定义以及销毁函数 */
    int            RknnDetectInit();
    int            RknnDetectQueryInoutIo();
    int            RknnDetectDestory();
    /* 计算欧式距离，输入两个点(x1,y1),(x2,y2)，输出一个float的距离 */
    float          CalculateEuclideanDistance(float x1, float y1, float x2, float y2);

public:

    /* (动态变化的结构体)向图片绘制的相关可调参数，定义在postprocess.h中 */
    TunableParam_S stTunableParam;

    /* 多帧结果融合算法参数 */
    float fInstance   = 0.3;    // 相邻两点之间距离小于fInstance，说明该人在上一帧中是存在的
    int   nControl    = 1;      // 缓存识别到的多少帧结果，进行结果融合
    /** 缓存几帧图片 **/
    int   nDelControl = 2;    // 上下两帧未匹配到的人点，缓存几次

    /******************************** 外部调用函数 ******************************/

    /**
     * @brief 输入bgr格式的图片数据进行识别
     * @param [char*] pDataBuffer: 格式的图片bgr格式数据
     * @param [int] nLength: 图片数据的长度
     * @return [*] 成功 大于等于 0 ， 失败 小于 0
     * @note 需要768x640 BGR格式数据
     */
    int DetectHumanBgr(char* pDataBuffer, int nLength);

    /* 获取图片识别的人数 */
    void GetPepleNum(int& nPNum)
    {
        nPNum = nPepleNum;
    }

    /* 获取图片识别的坐标点 */
    void GetPeplePoints(std::vector<float>& vPoints)
    {
        vPoints = vPointsXY;
    }

    /* 获取输入图片的大小 */
    void GetImgShape(int& nImgW, int& nImgH, int& nImgC)
    {
        nImgW = nImgWidth;
        nImgH = nImgHeight;
        nImgC = nImgChannel;
    }

    /* 设置输入图片的大小 */
    // void SetImgShape(int nImgW,int nImgH, int nImgC){nImgWidth=nImgW;nImgHeight=nImgH;nImgChannel=nImgC;}
    /* 获取模型需要的图片大小 */
    void GetModelShape(int& nModelW, int& nModelH, int& nModelC)
    {
        nModelW = nModelWidth;
        nModelH = nModelHeight;
        nModelC = nModelChannel;
    }

    /* 多帧结果融合算法,输入识别到的容器vPoints，然后使用一个容器vResultPoints接受识别后的结果
     * vPoints：人数统计算法识别的人数容器
     * vResultPoints：返回多帧结果融合后的结果
     */
    void MultiFrameResultFusionAlgorithm(std::vector<float> vPoints, std::vector<float>& vResultPoints);

    void ClearVector()
    {
        vPoints_Result.clear();
        vPoints_remove.clear();
        vCout.clear();
        vDelCout.clear();
    }

    /** 构造函数
     * 三个不同参数的构造函数
     * pModelPath：模型的路径
     * uInitImgWidth：模型需要图片的宽度
     * uInitImgHeight：模型需要图片的高
     * uInitImgChannel：模型需要图片的通道数
     * */
    RK_COUNT_DETECT();
    RK_COUNT_DETECT(char* pModelPath);
    RK_COUNT_DETECT(char* pModelPath, int uInitImgWidth, int uInitImgHeight, int uInitImgChannel);

    /* 析构函数 */
    ~RK_COUNT_DETECT();
};

#endif    // __RK_HUMAN_COUNT_DETECT_H__
