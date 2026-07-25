/**
 * @file PostProcessEXT.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-04-27
 *
 * @brief 模型输出配置参数
 */
#pragma once

#include <cstring>
#include <string>
#include <vector>


namespace Inference_NS
{
    /* 分类结构体 */
    typedef struct _Cls_
    {
        int   nLabel      = 0;
        float fConfidence = 1.0;
    } Cls_S;

    /* 点结构体 */
    typedef struct _Point_
    {
        int   nX    = -1;
        int   nY    = -1;
        float nShow = 1.0; /* 点是否可见 */
    } Point_S;

    /* 框结构体 */
    typedef struct _Box_
    {
        int nX1 = -1; /* 左上角坐标 */
        int nY1 = -1;
        int nX2 = -1; /* 右下角坐标 */
        int nY2 = -1;

        void clear()
        {
            nX1 = 0;
            nY1 = 0;
            nX2 = 0;
            nY2 = 0;
        }
    } Box_S;

    /* =========================================================================== */

    /* 分类/特征提取网络结果 */
    typedef struct _ClsData_
    {
        Cls_S              stCls;    /* 单分类结果 */
        std::vector<Cls_S> vCls;     /* 多分类结果,用于属性识别和字符串检测 */
        std::vector<float> vFeature; /* 特征提取网络结果 */
    } ClsData_S;

    /* 目标框结果 */
    typedef struct _BoxData_
    {
        Box_S stBoxs;      /* 框位置 */
        int   nLabel;      /* 标签 */
        float fConfidence; /* 置信度 */
    } BoxData_S;

    /* 关键点结果 */
    typedef struct _PointData_
    {
        Box_S                stBoxs;      /* 框位置 */
        int                  nLabel;      /* 框标签 */
        float                fConfidence; /* 框置信度 */
        std::vector<Point_S> vPoints;     /* 点信息 */
    } PointData_S;

    /* ASR结果 */
    typedef struct _ASRData_
    {
        std::string              strText;     /* 识别的文本 */
        float                    fTime;       /* 时间戳 */
        float                    fConfidence; /* 识别置信度 */
        bool                     bSpeech;     /* VAD端点检测 */
        std::vector<std::string> vTexts;      /* 文本容器 */
        std::vector<float>       vTimestamp;  /* 文本对应的时间戳容器 */
    } ASRData_S;

}    // namespace Inference_NS