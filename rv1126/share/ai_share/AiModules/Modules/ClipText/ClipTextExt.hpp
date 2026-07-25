/**
 * @file ClipTextExt.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-11-11
 *
 * @brief
 */
#pragma once

#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"

namespace ClipText_NS
{
    /* 视频异常检测分析参数 */
    typedef struct _AnalyseParam_
    {
    } AnalyseParam_S;

    /* =========================================================================== */

    /* 初始化参数 */
    typedef struct _InParam_
    {
        std::string strModelPath; /* 模型路径 */
        std::string sVocabPath; /* 词表路径 */

        /* 调试功能 */
        bool bDebug = false;             /* 是否开启调试功能 */
    } InParam_S;

    /* 输入数据 */
    typedef struct _InData_
    {
        std::string sText;      /* 文字 */
        AnalyseParam_S stParam; /* 参数 */
    } InData_S;

    /* =========================================================================== */

} // namespace NumberOcr_NS