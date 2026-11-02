/*
 * @Author: zhengxh
 * @Date: 2026-10-21 09:34:36
 * @LastEditors: zhengxh
 * @LastEditTime: 2026-10-21 09:34:36
 * @Description: 人脸识别模块公共数据结构
 */

#pragma once

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include "face_attribute_model.hpp" /* FaceAttributeTarget_S */
#include "face_detect_model.hpp"    /* FaceDetectedTarget_S, cv::Mat */

/* 人脸管线目标：检测+质量+属性+表情的单目标汇总，模块与事件处理器共用 */
struct FaceTargetResult_S
{
    explicit FaceTargetResult_S(FaceDetectedTarget_S stResult)
        : stDetectResult(std::move(stResult))
    {
    }

    FaceDetectedTarget_S stDetectResult;                 /* 人脸检测结果 */
    float fQualityScore = 0.0F;                          /* 人脸质量分数 */
    bool bAttributeReady = false;                        /* 属性分析是否成功 */
    FaceAttributeTarget_S stAttributeResult{};           /* 人脸属性结果 */
    bool bExpressionReady = false;                       /* 表情分析是否成功 */
    int nExpressionLabel = -1;                           /* 表情类别 0~6，-1=未分析 */
};

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
 * @brief 单帧检测上下文：帧源信息与检测管线结果，模块与事件处理器共用
 */
struct DetectContext_S
{
    FrameContext_S stFrame;                          /* 帧源信息 */
    std::vector<FaceTargetResult_S> vstDetectResult; /* 检测管线目标结果 */
};
