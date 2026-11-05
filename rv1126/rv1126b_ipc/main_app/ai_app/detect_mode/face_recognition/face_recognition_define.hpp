/*
 * @Author: zhengxh
 * @Date: 2026-10-21 09:34:36
 * @LastEditors: zhengxh
 * @LastEditTime: 2026-10-21 09:34:36
 * @Description: 人脸识别模块公共数据结构
 */

#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include "common_process.h"          /* pointScaleUp, fillRGBToCenter */
#include "face_detect_model.hpp"    /* FaceDetectedTarget_S, cv::Mat */

/* 基础管线目标：检测+质量评分（主检测器产出），特化分析结果由子检测器自持 */
struct FaceBaseTarget_S
{
    explicit FaceBaseTarget_S(FaceDetectedTarget_S stResult)
        : stDetectResult(std::move(stResult))
    {
    }

    FaceDetectedTarget_S stDetectResult; /* 人脸检测结果 */
    float fQualityScore = 0.0F;          /* 人脸质量分数 */
};

/**
 * @brief 人脸框外扩1.5倍后等比缩放居中到指定尺寸黑底画布，产出模型输入小图
 * @note 纯图像工具，属性/表情等特化分析共用
 */
inline bool cropFaceForModel(const cv::Mat &stRgbImage,
                             const FaceDetectedTarget_S &stResult,
                             int nWidth,
                             int nHeight,
                             cv::Mat &stFaceImage)
{
    if (stRgbImage.empty() || nWidth <= 0 || nHeight <= 0)
    {
        return false;
    }

    int nX1 = std::max(0, std::min(stRgbImage.cols, static_cast<int>(stResult.fX1)));
    int nY1 = std::max(0, std::min(stRgbImage.rows, static_cast<int>(stResult.fY1)));
    int nX2 = std::max(0, std::min(stRgbImage.cols, static_cast<int>(stResult.fX2)));
    int nY2 = std::max(0, std::min(stRgbImage.rows, static_cast<int>(stResult.fY2)));
    pointScaleUp(nX1, nY1, nX2, nY2, stRgbImage.cols, stRgbImage.rows, 1.5);
    if (nX2 <= nX1 || nY2 <= nY1)
    {
        return false;
    }

    const cv::Rect stFaceRoi(nX1, nY1, nX2 - nX1, nY2 - nY1);
    stFaceImage = cv::Mat(nHeight, nWidth, CV_8UC3, cv::Scalar(0, 0, 0));
    return fillRGBToCenter(stRgbImage, stFaceRoi, stFaceImage);
}

/**
 * @brief 当前检测帧对应的高分辨率特写源
 */
struct CloseupSource_S
{
    bool isValid() const
    {
        return pNv12 &&
               nDataSize > 0 &&
               nWidth > 0 &&
               nHeight > 0 &&
               nVirWidth > 0 &&
               nVirHeight > 0;
    }

    std::shared_ptr<char[]> pNv12;    /* NV12帧数据 */
    std::size_t nDataSize = 0;        /* 数据长度 */
    int nWidth = 0;                   /* 可见宽 */
    int nHeight = 0;                  /* 可见高 */
    int nVirWidth = 0;                /* 虚拟宽 */
    int nVirHeight = 0;               /* 虚拟高 */
};

/**
 * @brief 单帧人脸识别上下文，封装通道、时间戳与算法输入图像
 */
struct FrameContext_S
{
    int nChannelId = 0;               /* 通道号 */
    long long llTimestamp = 0;        /* 毫秒时间戳 */
    cv::Mat matPanoramaImage;         /* RGB全景大图，算法输入 */
    CloseupSource_S stCloseupSource;  /* 同步高分辨率特写源 */
};

/**
 * @brief 单帧检测上下文：帧源信息与基础管线结果（检测+质量），
 *        子检测器作为纯消费者读取，特化分析在子检测器内部自建数据
 */
struct DetectContext_S
{
    FrameContext_S stFrame;                        /* 帧源信息 */
    std::vector<FaceBaseTarget_S> vstDetectResult; /* 基础管线目标结果 */
};
