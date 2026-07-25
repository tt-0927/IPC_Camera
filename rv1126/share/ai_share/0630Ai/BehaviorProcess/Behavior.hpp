/*
 * @FilePath     : Behavior.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-07-27 11:41:43
 * @LastEditors: weigl 308241951@qq.com
 * @LastEditTime: 2025-02-26 17:02:18
 * @Description  :
 */
#pragma once

#include <vector>

#include "BlError.h"
#include "dlog.h"
#include "OutputDataEXT.hpp"

namespace Ai0630_NS
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
            dRatio    = 0.5;
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

    /* 双手抱臂参数 */
    typedef struct _ArmsCrossed_
    {
        double dKeyScore; /* 用到的关键点最低得分 */
        double dRatio;    /* 系数比 */

        _ArmsCrossed_()
        {
            dKeyScore = 0.2;
            dRatio    = 0.2;
        }
    } ArmsCrossed_S;

    /* 叉腰表达参数 */
    typedef struct _HandsOnHip_
    {
        double dKeyScore; /* 用到的关键点最低得分 */
        double dRatio;    /* 系数比 */

        _HandsOnHip_()
        {
            dKeyScore = 0.2;
            dRatio    = 0.2;
        }
    } HandsOnHip_S;

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
        TurningBody_S stTurningBody; /* 转身 */
        RaisingHand_S stRaisingHand; /* 举手 */
        BendingHead_S stBendingHead; /* 低头 */
        RaisingHead_S stRaisingHead; /* 抬头 */
        ArmsCrossed_S stArmsCrossed; /* 双手抱臂参数 */
        HandsOnHip_S  stHandsOnHip;  /* 叉腰表达参数 */
    } SupClsInsParam_S;

    class Behavior
    {
    public:

        /**
         * @brief 获取当前行为类型
         * @param vecInfo 行为特征点
         * @param stParam 行为参数
         * @return int
         */
        static int getType(
            std::vector<Inference_NS::Point_S> vecInfo,
            BehaviorParam_S                    stParam = BehaviorParam_S());

        /**
         * @brief 获取当前行为类型
         * @param vecInfo 行为特征点
         * @param stParam 行为参数
         * @return int
         */
        static int getType(
            std::vector<Inference_NS::Point_S> vecInfo,
            SupClsInsParam_S                   stParam = SupClsInsParam_S());

    private:

        /* 低头检测 */
        static bool isBendingHead(const std::vector<Inference_NS::Point_S>& vstPoint, BendingHead_S stBendingHead);

        /* 抬头检测 */
        static bool isRaisingHead(const std::vector<Inference_NS::Point_S>& vstPoint, RaisingHead_S stRaisingHead);

        /* 转头检测 */
        static bool isTurningHead(const std::vector<Inference_NS::Point_S>& vstPoint, TurningHead_S stTurningHead);

        /* 转身检测 */
        static bool isTurningBody(const std::vector<Inference_NS::Point_S>& vstPoint, TurningBody_S stTurningBody);

        /* 站立检测 */
        static bool isStanding(const std::vector<Inference_NS::Point_S>& vstPoint, Standing_S stStanding);

        /* 举手检测 */
        static bool isRaisingHand(const std::vector<Inference_NS::Point_S>& vstPoint, RaisingHand_S stRaisingHand);

        /* 趴桌检测 */
        static bool isLyingOnDesk(const std::vector<Inference_NS::Point_S>& vstPoint, LyingOnDesk_S stLyingOnDesk);

        /* 玩手机检测 */
        static bool isPlayPhone(const std::vector<Inference_NS::Point_S>& vstPoint, PlayPhone_S stPlayPhone);

        /* 接打电话检测 */
        static bool isCallPhone(const std::vector<Inference_NS::Point_S>& vstPoint, CallPhone_S stCallPhone);

        /* 板书检测 */
        static bool isTeaBoard(const std::vector<Inference_NS::Point_S>& vstPoint, TeaBoard_S stTeaBoard);

        /* 双手抱臂判断 */
        static bool isArmsCrossed(const std::vector<Inference_NS::Point_S>& vstPoint, ArmsCrossed_S stTeaBoard);

        /* 叉腰表达 */
        static bool isHandsOnHip(const std::vector<Inference_NS::Point_S>& vstPoint, HandsOnHip_S stTeaBoard);

        /* 计算两点之间与y轴的角度 */
        static double getAngleWithYAxis(const Inference_NS::Point_S& stPoint1, const Inference_NS::Point_S& stPoint2);

        /* 获取斜率 */
        static double getSlope(const Inference_NS::Point_S& p1, const Inference_NS::Point_S& p2);

        /* 获取垂直交点 */
        static Inference_NS::Point_S getIntersection(const Inference_NS::Point_S& A, const Inference_NS::Point_S& B, const Inference_NS::Point_S& C);

        /* 计算两点间的距离 */
        static double getDistance(const Inference_NS::Point_S& stPoint1, const Inference_NS::Point_S& stPoint2);

        /* 获取中心点 */
        static Inference_NS::Point_S getMidpoint(const Inference_NS::Point_S& point1, const Inference_NS::Point_S& point2);

        /* 计算三角形面积的两倍 */
        static double getTwiceTriangleArea(const Inference_NS::Point_S& a, const Inference_NS::Point_S& b, const Inference_NS::Point_S& c);

        /* 计算由三个点形成的夹角的角度 a是交点 */
        static double getAngleBetweenPoints(const Inference_NS::Point_S& a, const Inference_NS::Point_S& b, const Inference_NS::Point_S& c);

        /* 计算向量AB和AP的叉积 */
        static double getCrossProduct(const Inference_NS::Point_S& A, const Inference_NS::Point_S& B, const Inference_NS::Point_S& P);

        /* 判断点P相对于由A和B定义的直线的位置, 具体返回值为 1 表示在左边，-1 表示在右边，0 表示在直线上 */
        static int getPointRelativeOfLine(const Inference_NS::Point_S& A, const Inference_NS::Point_S& B, const Inference_NS::Point_S& P);
    };

}    // namespace Ai0630_NS