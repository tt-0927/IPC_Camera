/*
 * @FilePath     : CVExtern.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-06-05 15:50:05
 * @LastEditors: weigl 308241951@qq.com
 * @LastEditTime: 2025-02-28 09:45:39
 * @Description  :
 */
#pragma once

#include "Extern.hpp"
#include <variant>
#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"

namespace AiScenario_NS
{

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

    /* 玩手机识别参数 */
    typedef struct _PlayPhone_
    {
        double dKeyScore; /* 用到的关键点最低得分 */
        double dRatio;    /* 手腕宽度 / 肩宽度 = 系数比 */

        _PlayPhone_()
        {
            dKeyScore = 0.2;
            dRatio = 0.5;
        }
    } PlayPhone_S;

    /* 接打电话识别参数 */
    typedef struct _CallPhone_
    {
        double dKeyScore; /* 用到的关键点最低得分 */
        _CallPhone_()
        {
            dKeyScore = 0.2;
        }
    } CallPhone_S;

    /* 板书识别参数 */
    typedef struct _TeaBoard_
    {
        double dKeyScore; /* 用到的关键点最低得分 */
        _TeaBoard_()
        {
            dKeyScore = 0.2;
        }
    } TeaBoard_S;

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

    /* 督导巡课识别参数 */
    typedef struct _SupClsInsParam_
    {
        PlayPhone_S   stPlayPhone;   /* 玩手机 */
        CallPhone_S   stCallPhone;   /* 接打电话 */
        TeaBoard_S    stTeaBoard;    /* 板书识别 */
    } SupClsInsParam_S;

    /* 跟踪参数 */
    typedef struct _TrackerParam_
    {
        std::vector<float> vBoxPoints;  /* 设置框选信息 */
    } TrackerParam_S;

    /* 虚拟抠像参数 */
    /* 抠图背景分类 */
    typedef enum _BgColor_
    {
        BLUE  = 0,     /* 蓝色背景 */
        GREEN = 1      /* 绿色背景 */
    } BgColor_E;
    
	/* 颜色抠图算法需要配置的参数 */
    typedef struct _CutoutParam_
    {
		BgColor_E eBgColor;          /* 幕布颜色 */   
	    float fColorThres;   	     /* 颜色阈值 */
	    float fEdgeThres;       	 /* 边缘细腻度阈值 */
        float fSharp;                /* 锐化值 */
		cv::Mat aInputImage;		 /* 输入RGB图像   */
		cv::Mat aOutputImage;		 /* 输出RGBA图像  */
		
		/* 可选项-开启会添加耗时 */
		bool bCloseMorPh;           /* 闭运算，解决人像内部空缺问题 */
		bool bErodeMode;            /* 膨胀  ，解决边缘问题 */
		bool bOpenMorPh;            /* 开运算，解决边缘问题 */
		bool bGaussianBlur;         /* 高斯模糊，解决边缘问题 */
        bool bSharpEnabled;         /* 锐化使能 */

        _CutoutParam_()
        {
            eBgColor        = BLUE;        /* 默认的蓝幕    */
            fColorThres     = 175.0;        
            fEdgeThres      = 0.9;
            
            bCloseMorPh     = false;
            bErodeMode      = false;
            bOpenMorPh      = false;
            bGaussianBlur   = false;
        }
		
    } CutoutParam_S;

    /* 视觉数据 */
    typedef struct _CVData_
    {
        cv::Mat inMat; /* 图片 */

        std::variant<TrackerParam_S, BehaviorParam_S, CutoutParam_S> varParam; /* 联合结构体 */
    } CVData_S;

}    // namespace AiScenario_NS