/**
 * @file InputDataEXT.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-04-27
 *
 * @brief 模型输入的格式
 */
#pragma once

#include <string>
#include <vector>

namespace Inference_NS
{
    /* 分类阈值结构体 */
    typedef struct _ClsThres_
    {
        float fConfidence = 1.0; /* 类别阈值 */
    } ClsThres_S;

    /* 点阈值结构体 */
    typedef struct _PointThres_
    {
        float fBoxConfidence = -1.0; /* 目标框的置信度 */
        float fBoxNms = -1.0;        /* 目标框的nms阈值 */
        float fPointThreshold = 1.0; /* 点是否存在的阈值 */
    } PointThres_S;

    /* 框阈值结构体 */
    typedef struct _BoxThres_
    {
        float fConfidence = -1.0; /* 目标框的置信度 */
        float fNms = -1.0;        /* 目标框的nms阈值 */
    } BoxThres_S;

    /* 模型推理的输入 */
    typedef struct _InputData_
    {
        float *pData = nullptr; /* 输入图片数据 */
        int nDataSize;          /* 输入图片数据的大小 */
        std::string strText;    /* 文本输入 */
        ClsThres_S stCls;       /* 分类阈值 */
        BoxThres_S stBoxs;      /* 框的阈值 */
        PointThres_S stPoints;  /* 点的阈值 */
    } InputData_S;

    /* 音频输入 */
    typedef struct _AVInputData_
    {
        std::vector<float> vFeature; /* 特征 */
        int nFrameIndex = 0;         /* fbank特征下标索引 */
    } AVInputData_S;

    /* =========================================================================== */

} // namespace Inference_NS