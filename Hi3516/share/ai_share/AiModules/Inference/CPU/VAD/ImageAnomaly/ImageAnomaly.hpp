/*
 *  File Name: ImageAnomaly.h
 *  Created on: 2024年9月5日
 *  Author: wcp
 *  description : 视频异常分析算法
 *  Modify date: 2024年9月5日
 */

#ifndef __RK_IMAGEANOMALY_H__
#define __RK_IMAGEANOMALY_H__

#include <dlfcn.h>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/time.h>

/* 自定义的预处理头文件 */
#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"

using namespace std;

namespace Inference_NS
{
    class cImageAnomaly
    {
    public:
        /**
         * @brief 过滤掉检测底背景
         * @param [cv::Mat] aFrame: 原始图片
         * @param [cv::Scalar] aLowerColor: yuv数据，纯背景的下限值
         * @param [cv::Scalar] aUpperColor: yuv数据，纯背景的上限值
         * @param [cv::Mat&] aColorMask: 底背景掩码图
         * @param [cv::Mat&] aAnomalyMask: 前景掩码图
         * @param [cv::Mat&] aAnomalyFrame: 前景图
         * @return
         * @note
         */
        void detectRedAnomaly(cv::Mat aFrame, cv::Scalar aLowerColor, cv::Scalar aUpperColor, cv::Mat &aColorMask, cv::Mat &aAnomalyMask, cv::Mat &aAnomalyFrame);

        /**
         * @brief 前后两帧，做差
         * @param aFrame 原始图片
         * @param aColorMask 底背景掩码图
         * @param aAnomalyMask 前景掩码图
         * @param aAnomalyFrame 前景图
         */
        void detectRedAnomaly(cv::Mat aFrame, cv::Mat &aColorMask, cv::Mat &aAnomalyMask, cv::Mat &aAnomalyFrame);

        /**
         * @brief 条纹检测算法
         * @param [cv::Mat&] aAnomalyFrame: 过滤掉检测底背景后的前景图
         * @param [std::vector<cv::Vec4i>&] vLines: 检测到的直线两个点的坐标集合
         * @return
         * @note
         */
        void detectStripes(cv::Mat &aAnomalyFrame, std::vector<cv::Vec4i> &vLines);

        /**
         * @brief 亮暗检测算法
         * @param [cv::Mat&] aFrame: 原始图片
         * @param [cv::Mat&] aColorMask: 底背景掩码图
         * @return
         * @note
         */
        double getLight(cv::Mat &aFrame, cv::Mat &aColorMask);

        /**
         * @brief 噪点检测算法
         * @param [cv::Mat&] aAnomalyMask: 前景掩码图
         * @return
         * @note
         */
        int countNoise(cv::Mat &aAnomalyMask);

        /**
         * @brief 图片模糊度检测
         * @param [cv::Mat&] aAnomalyMask: 原始图片
         * @return
         * @note
         */
        double assessImageSharpness(cv::Mat &aImage);

    private:
        /* 上一帧数据 */
        cv::Mat aLastFrame;
        /* 上一帧的亮度 */
        double dLastLight = 0;

    public:
        /* 构造函数 */
        cImageAnomaly();
        /* 析构函数 */
        ~cImageAnomaly();
    };
}; // namespace Inference_NS
#endif // __RK_IMAGEANOMALY_H__
