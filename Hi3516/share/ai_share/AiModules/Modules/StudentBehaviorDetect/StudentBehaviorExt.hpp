/*
 * @Author: zhengxh zhengxh@kfb.cn
 * @Date: 2026-03-30 19:19
 * @LastEditors: zhengxh zhengxh@kfb.cn
 * @LastEditTime: 2026-03-30 19:19
 * @FilePath: ai_share/AiModules/Modules/StudentBehaviorDetect/StudentBehaviorExt.hpp
 * @Description: 学生行为检测
 */

#pragma once

#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"
#include <vector>

namespace StudentBehavior_NS {

    /* 学生行为定义 */
    const int BendingHead   = 0;    /* 低头 */
    const int RaisingHead   = 1;    /* 抬头 */
    const int TurningHead   = 2;    /* 转头 */
    const int RaisingHand   = 3;    /* 举手 */
    const int Standing      = 4;    /* 站立 */
    const int TurningBody   = 5;    /* 转身 */
    const int LyingOnDesk   = 6;    /* 趴桌 */

    /* 人体关键点信息的结构体 */
    typedef struct _KeyPosInfo_
    {
        double dX;     /* x 坐标 */
        double dY;     /* y 坐标 */
        double dScore; /* 置信度 */

        void clear()
        {
            dX     = 0.0f;
            dY     = 0.0f;
            dScore = 0.0f;
        }

        _KeyPosInfo_()
        {
            clear();
        }
    } KeyPosInfo_S;

    /* 低头识别参数 */
    typedef struct _BendingHead_
    {
        double dKeyScore; /* 用到的关键点最低得分 */
        double dRatio;    /* 眉心的垂点到鼻子 / 眉心的垂点颈部 = 系数比 */

        _BendingHead_()
        {
            dKeyScore = 0.5;
            dRatio    = 0.75;
        }
    } BendingHead_S;

    /* 抬头识别参数 */
    typedef struct _RaisingHead_
    {
        double dKeyScore;   /* 用到的关键点最低得分 */
        double dRaiseAngle; /* 左右眼和鼻子的夹角 */

        _RaisingHead_()
        {
            dKeyScore   = 0.8;
            dRaiseAngle = 120.0;
        }
    } RaisingHead_S;

    /* 转头识别参数 */
    typedef struct _TurningHead_
    {
        double dKeyScore; /* 用到的关键点最低得分 */

        _TurningHead_()
        {
            dKeyScore = 0.5;
        }
    } TurningHead_S;

    /* 转身识别参数 */
    typedef struct _TurningBody_
    {
        double dKeyScore; /* 用到的关键点最低得分 */

        _TurningBody_()
        {
            dKeyScore = 0.7;
        }
    } TurningBody_S;

    /* 站立识别参数 */
    typedef struct _Standing_
    {
        double dKeyScore;    /* 用到的关键点最低得分 */
        double dYRaiseAngle; /* 膝盖到臀部与y轴的夹角 */
        double dRaiseAngle;  /* 手肘到手腕和手肘到肩膀的角度 */

        _Standing_()
        {
            dKeyScore    = 0.8;
            dYRaiseAngle = 20.0;
            dRaiseAngle  = 165.0;
        }
    } Standing_S;

    /* 举手识别参数 */
    typedef struct _RaisingHand_
    {
        double dKeyScore;    /* 用到的关键点最低得分 */
        double dYRaiseAngle; /* 手腕和手肘与Y轴的夹角 */

        _RaisingHand_()
        {
            dKeyScore    = 0.8;
            dYRaiseAngle = 30.0;
        }
    } RaisingHand_S;

    /* 趴桌识别参数 */
    typedef struct _LyingOnDesk_
    {
        double dKeyScore; /* 用到的关键点最低得分 */

        _LyingOnDesk_()
        {
            dKeyScore = 0.1;
        }
    } LyingOnDesk_S;

    /* 行为识别参数 */
    typedef struct _BehaviorParam_
    {
        BendingHead_S stBendingHead; /* 低头 */
        RaisingHead_S stRaisingHead; /* 抬头 */
        TurningHead_S stTurningHead; /* 转头 */
        TurningBody_S stTurningBody; /* 转身 */
        Standing_S    stStanding;    /* 站立 */
        RaisingHand_S stRaisingHand; /* 举手 */
        LyingOnDesk_S stLyingOnDesk; /* 趴桌子 */
    } BehaviorParam_S;

    /* 人头识别参数 */
    typedef struct _HeadParam_
    {
        float confidence = 0.25;    /* 人头识别阈值 */
    } HeadParam_S;

    /* 初始化参数 */
    typedef struct _InParam_
    {
        std::string strHModelPath;   /* 人头分析模型路径 */
        std::string strBModelPath;       /* 学生行为分析模型路径 */

        /* 调试功能 */
        bool        bDebug = false;      /* 是否开启调试功能 */
        std::string strOriginalDataPath; /* 原始数据保存路径, 文件夹路径 */
        std::string strHeadDataPath;        /* 人头数据保存路径 */
        std::string strBodyDataPath;        /* 人体数据保存路径 */
    } InParam_S;

    /* 输入数据 */
    typedef struct _InData_
    {
        int            nChnId;  /* 通道 */
        cv::Mat        inMat;   /* 图片 */
        BehaviorParam_S stParam; /* 参数 */
        HeadParam_S stHeadParam;
    } InData_S;

    /* =========================================================================== */

    typedef struct __Behavior__
    {
        int nBehavior;
        std::vector<float> vFeature;
    } Behavior_S;

    /* 检测结果 */
    typedef struct _OutData_
    {
        int nChnId; /* 通道 */
        std::vector<Behavior_S> vStBehavior;
    } OutData_S;

    /* 结果 */
    typedef struct _Result_
    {
        float       fX1 = 0;        /* 左上角x坐标 */
        float       fY1 = 0;        /* 左上角y坐标 */
        float       fX2 = 0;        /* 右下角x坐标 */
        float       fY2 = 0;        /* 右下角y坐标 */
        float       fBoxConfidence; /* 置信度 */
        int         nId = 0;        /* 框ID */
        int         nClassId = -1;       /* 种类ID: */
    } Result_S;

}  // namespace Group2Detect_NS