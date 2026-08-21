/*
 * @FilePath     : Behavior.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-08-02 11:15:19
 * @LastEditors: weigl 308241951@qq.com
 * @LastEditTime: 2025-02-26 17:01:03
 * @Description  :
 */

#include "Behavior.hpp"

#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

using namespace Ai0630_NS;

/* 获取当前行为类型 */
int Behavior::getType(
    std::vector<Inference_NS::Point_S> vecInfo,
    BehaviorParam_S                    stParam)
{

    if (vecInfo.size() != 26)
    {
        return -1;
    }

    Inference_NS::Point_S& stNose      = vecInfo[0];  /* 鼻子 */
    Inference_NS::Point_S& stREye      = vecInfo[1];  /* 右眼 */
    Inference_NS::Point_S& stLEye      = vecInfo[2];  /* 左眼 */
    Inference_NS::Point_S& stREar      = vecInfo[3];  /* 右耳朵 */
    Inference_NS::Point_S& stLEar      = vecInfo[4];  /* 左耳朵 */
    Inference_NS::Point_S& stRShoulder = vecInfo[5];  /* 右肩 */
    Inference_NS::Point_S& stLShoulder = vecInfo[6];  /* 左肩 */
    Inference_NS::Point_S& stRElbow    = vecInfo[7];  /* 右手肘 */
    Inference_NS::Point_S& stLElbow    = vecInfo[8];  /* 左手肘 */
    Inference_NS::Point_S& stRWrist    = vecInfo[9];  /* 右手腕 */
    Inference_NS::Point_S& stLWrist    = vecInfo[10]; /* 左手腕 */
    Inference_NS::Point_S& stRHip      = vecInfo[11]; /* 右臀 */
    Inference_NS::Point_S& stLHip      = vecInfo[12]; /* 左臀 */
    Inference_NS::Point_S& stRKnee     = vecInfo[13]; /* 右膝盖 */
    Inference_NS::Point_S& stLKnee     = vecInfo[14]; /* 左膝盖 */
    Inference_NS::Point_S& stRAnkle    = vecInfo[15]; /* 右脚踝 */
    Inference_NS::Point_S& stLAnkle    = vecInfo[16]; /* 左脚踝 */
    Inference_NS::Point_S& stHead      = vecInfo[17]; /* 头顶 */
    Inference_NS::Point_S& stNeck      = vecInfo[18]; /* 颈部 */
    Inference_NS::Point_S& stHip       = vecInfo[19]; /* 臀部 */

/* 数据优化 */
#if 1
    double nShow  = 0.8;
    /* 手臂特征点只要有二个超过阈值，全部设置为阈值以上 */
    /* 计算大于阈值的数的数量 */
    int    nCount = (stLShoulder.nShow > nShow) +
        (stLElbow.nShow > nShow) +
        (stLWrist.nShow > nShow);
    if (nCount >= 2)
    {
        stLShoulder.nShow = (nShow + 0.1);
        stLElbow.nShow    = (nShow + 0.1);
        stLWrist.nShow    = (nShow + 0.1);
    }

    nCount = (stRShoulder.nShow > nShow) +
        (stRElbow.nShow > nShow) +
        (stRWrist.nShow > nShow);
    if (nCount >= 2)
    {
        stRShoulder.nShow = (nShow + 0.1);
        stRElbow.nShow    = (nShow + 0.1);
        stRWrist.nShow    = (nShow + 0.1);
    }

    /* 耳朵特征点超过阈值，眼睛特征点设置为阈值以上 */
    if (stREar.nShow > nShow && stLEar.nShow > nShow)
    {
        stREye.nShow = (nShow + 0.1);
        stLEye.nShow = (nShow + 0.1);
    }

#endif

    /* 旧的
    种类，{
    0:'低头',
    1:'抬头',
    2:'转头',
    3:'举手',
    4:'站立',
    5:'转身',
    6:'趴桌'
    }
    */

    // enum class StudentBehavior_E 新的
    // {
    //     ACTION_NULL = -1, /* 未识别到 */
    //     LISTEN      = 0,  /* 听讲，抬头（专注） */
    //     PRACTICE    = 1,  /* 实践，举手（专注） */
    //     DEMO        = 2,  /* 演示，站立（专注） */
    //     READ        = 3,  /* 阅读，低头（涣散） */
    //     DISCUSS     = 4,  /* 讨论，转头（涣散） */
    //     DOWN_DESK   = 5,  /* 趴桌（涣散） */
    // };

    /* 趴桌判断 */
    if (isLyingOnDesk(vecInfo, stParam.stLyingOnDesk))
    {
        return 5;
    }

    /* 转身判断 */
    if (isTurningBody(vecInfo, stParam.stTurningBody))
    {
        return 4;
    }

    /* 站立判断 */
    if (isStanding(vecInfo, stParam.stStanding))
    {
        return 2;
    }

    /* 举手判断 */
    if (isRaisingHand(vecInfo, stParam.stRaisingHand))
    {
        return 1;
    }

    /* 低头判断 */
    if (isBendingHead(vecInfo, stParam.stBendingHead))
    {
        return 4;
    }
    /* 转头判断 */
    else if (isTurningHead(vecInfo, stParam.stTurningHead))
    {
        return 4;
    }
    /* 抬头判断 */
    else if (isRaisingHead(vecInfo, stParam.stRaisingHead))
    {
        return 0;
    }

    return -1;
}

/* 获取当前行为类型 */
int Behavior::getType(
    std::vector<Inference_NS::Point_S> vecInfo,
    SupClsInsParam_S                   stParam)
{

    if (vecInfo.size() != 26)
    {
        return -1;
    }

    Inference_NS::Point_S& stNose      = vecInfo[0];  /* 鼻子 */
    Inference_NS::Point_S& stREye      = vecInfo[1];  /* 右眼 */
    Inference_NS::Point_S& stLEye      = vecInfo[2];  /* 左眼 */
    Inference_NS::Point_S& stREar      = vecInfo[3];  /* 右耳朵 */
    Inference_NS::Point_S& stLEar      = vecInfo[4];  /* 左耳朵 */
    Inference_NS::Point_S& stRShoulder = vecInfo[5];  /* 右肩 */
    Inference_NS::Point_S& stLShoulder = vecInfo[6];  /* 左肩 */
    Inference_NS::Point_S& stRElbow    = vecInfo[7];  /* 右手肘 */
    Inference_NS::Point_S& stLElbow    = vecInfo[8];  /* 左手肘 */
    Inference_NS::Point_S& stRWrist    = vecInfo[9];  /* 右手腕 */
    Inference_NS::Point_S& stLWrist    = vecInfo[10]; /* 左手腕 */
    Inference_NS::Point_S& stRHip      = vecInfo[11]; /* 右臀 */
    Inference_NS::Point_S& stLHip      = vecInfo[12]; /* 左臀 */
    Inference_NS::Point_S& stRKnee     = vecInfo[13]; /* 右膝盖 */
    Inference_NS::Point_S& stLKnee     = vecInfo[14]; /* 左膝盖 */
    Inference_NS::Point_S& stRAnkle    = vecInfo[15]; /* 右脚踝 */
    Inference_NS::Point_S& stLAnkle    = vecInfo[16]; /* 左脚踝 */
    Inference_NS::Point_S& stHead      = vecInfo[17]; /* 头顶 */
    Inference_NS::Point_S& stNeck      = vecInfo[18]; /* 颈部 */
    Inference_NS::Point_S& stHip       = vecInfo[19]; /* 臀部 */

/* 数据优化 */
#if 1
    double nShow  = 0.8;
    /* 手臂特征点只要有二个超过阈值，全部设置为阈值以上 */
    /* 计算大于阈值的数的数量 */
    int    nCount = (stLShoulder.nShow > nShow) +
        (stLElbow.nShow > nShow) +
        (stLWrist.nShow > nShow);
    if (nCount >= 2)
    {
        stLShoulder.nShow = (nShow + 0.1);
        stLElbow.nShow    = (nShow + 0.1);
        stLWrist.nShow    = (nShow + 0.1);
    }

    nCount = (stRShoulder.nShow > nShow) +
        (stRElbow.nShow > nShow) +
        (stRWrist.nShow > nShow);
    if (nCount >= 2)
    {
        stRShoulder.nShow = (nShow + 0.1);
        stRElbow.nShow    = (nShow + 0.1);
        stRWrist.nShow    = (nShow + 0.1);
    }

    /* 耳朵特征点超过阈值，眼睛特征点设置为阈值以上 */
    if (stREar.nShow > nShow && stLEar.nShow > nShow)
    {
        stREye.nShow = (nShow + 0.1);
        stLEye.nShow = (nShow + 0.1);
    }

#endif

    /*
    种类，{
    7:'玩手机',
    8:'接打电话',
    9:'板书'，
    }
    */

    // enum class TeacherPosture_E
    // {
    //     OTHER         = -1, /* 其他 */
    //     FRONT_EXPLAIN = 0,  /* 正面讲解 */
    //     ARMS_CROSSED = 1,       /* 双手抱臂 */
    //     FACE_BLACKBOARD = 2,    /* 面向黑板 */
    //     HANDS_ON_HIP = 3,       /* 叉腰表达 */
    //     HEAD_DOWN = 4,          /* 低头 */
    //     HEAD_UP = 5,            /* 抬头 */
    //     HAND_UP = 6,            /* 举手示意 */
    //     PLAY_PHONE = 7,         /* 玩手机 */
    //     CALL_PHONE = 8,         /* 接打电话 */
    //     TEABOARD = 9,           /* 板书 */
    // };

    /* 板书判断 */
    if (isTeaBoard(vecInfo, stParam.stTeaBoard))
    {
        return 9;
    }

    /* 面向黑板判断 */
    if (isTurningBody(vecInfo, stParam.stTurningBody))
    {
        return 2;
    }

    /* 举手判断 */
    if (isRaisingHand(vecInfo, stParam.stRaisingHand))
    {
        return 6;
    }

    /* 双手抱臂判断 */
    if (isArmsCrossed(vecInfo, stParam.stArmsCrossed))
    {
        return 1;
    }

    /* 叉腰表达判断 */
    if (isHandsOnHip(vecInfo, stParam.stHandsOnHip))
    {
        return 3;
    }


    /* 低头判断 */
    if (isBendingHead(vecInfo, stParam.stBendingHead))
    {
        return 4;
    }
    /* 抬头判断 */
    else if (isRaisingHead(vecInfo, stParam.stRaisingHead))
    {
        return 5;
    }

    // /* 接打电话判断 */
    // if (isCallPhone(vecInfo, stParam.stCallPhone))
    // {
    //     return 8;
    // }

    // /* 玩手机判断 */
    // if (isPlayPhone(vecInfo, stParam.stPlayPhone))
    // {
    //     return 7;
    // }

    /* 正面判断 */
    return 0;
}

/* 低头检测 */
bool Behavior::isBendingHead(const std::vector<Inference_NS::Point_S>& vstPoint, BendingHead_S stBendingHead)
{
    if (vstPoint.size() != 26)
    {
        dlog(LOG_ERROR, "数据大小不对 [%ld]", vstPoint.size());
        return false;
    }

    const Inference_NS::Point_S& stNose      = vstPoint[0];  /* 鼻子 */
    const Inference_NS::Point_S& stREye      = vstPoint[1];  /* 右眼 */
    const Inference_NS::Point_S& stLEye      = vstPoint[2];  /* 左眼 */
    const Inference_NS::Point_S& stREar      = vstPoint[3];  /* 右耳朵 */
    const Inference_NS::Point_S& stLEar      = vstPoint[4];  /* 左耳朵 */
    const Inference_NS::Point_S& stRShoulder = vstPoint[5];  /* 右肩 */
    const Inference_NS::Point_S& stLShoulder = vstPoint[6];  /* 左肩 */
    const Inference_NS::Point_S& stRElbow    = vstPoint[7];  /* 右手肘 */
    const Inference_NS::Point_S& stLElbow    = vstPoint[8];  /* 左手肘 */
    const Inference_NS::Point_S& stRWrist    = vstPoint[9];  /* 右手腕 */
    const Inference_NS::Point_S& stLWrist    = vstPoint[10]; /* 左手腕 */
    const Inference_NS::Point_S& stRHip      = vstPoint[11]; /* 右臀 */
    const Inference_NS::Point_S& stLHip      = vstPoint[12]; /* 左臀 */
    const Inference_NS::Point_S& stRKnee     = vstPoint[13]; /* 右膝盖 */
    const Inference_NS::Point_S& stLKnee     = vstPoint[14]; /* 左膝盖 */
    const Inference_NS::Point_S& stRAnkle    = vstPoint[15]; /* 右脚踝 */
    const Inference_NS::Point_S& stLAnkle    = vstPoint[16]; /* 左脚踝 */
    const Inference_NS::Point_S& stHead      = vstPoint[17]; /* 头顶 */
    const Inference_NS::Point_S& stNeck      = vstPoint[18]; /* 颈部 */
    const Inference_NS::Point_S& stHip       = vstPoint[19]; /* 臀部 */

    if (stNeck.nShow > stBendingHead.dKeyScore &&
        stNose.nShow > stBendingHead.dKeyScore)
    {
        if (stNeck.nY < stNose.nY)
        {
            return true;
        }
        else
        {
            if (stLEye.nShow > stBendingHead.dKeyScore &&
                stREye.nShow > stBendingHead.dKeyScore)
            {
                /* 获取stNose到眉心的垂点 */
                Inference_NS::Point_S stAjina = getIntersection(stREye, stLEye, stNose);

                double dNumerator   = getDistance(stAjina, stNose);
                double dDenominator = getDistance(stAjina, stNeck);
                if (dDenominator != 0)
                {
                    double dFactor = dNumerator / dDenominator;
                    if (dFactor > stBendingHead.dRatio)
                    {
                        /* 低头 */
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

/* 抬头检测 */
bool Behavior::isRaisingHead(
    const std::vector<Inference_NS::Point_S>& vstPoint,
    RaisingHead_S                             stRaisingHead)
{
    if (vstPoint.size() != 26)
    {
        dlog(LOG_ERROR, "数据大小不对 [%ld]", vstPoint.size());
        return false;
    }

    const Inference_NS::Point_S& stNose      = vstPoint[0];  /* 鼻子 */
    const Inference_NS::Point_S& stREye      = vstPoint[1];  /* 右眼 */
    const Inference_NS::Point_S& stLEye      = vstPoint[2];  /* 左眼 */
    const Inference_NS::Point_S& stREar      = vstPoint[3];  /* 右耳朵 */
    const Inference_NS::Point_S& stLEar      = vstPoint[4];  /* 左耳朵 */
    const Inference_NS::Point_S& stRShoulder = vstPoint[5];  /* 右肩 */
    const Inference_NS::Point_S& stLShoulder = vstPoint[6];  /* 左肩 */
    const Inference_NS::Point_S& stRElbow    = vstPoint[7];  /* 右手肘 */
    const Inference_NS::Point_S& stLElbow    = vstPoint[8];  /* 左手肘 */
    const Inference_NS::Point_S& stRWrist    = vstPoint[9];  /* 右手腕 */
    const Inference_NS::Point_S& stLWrist    = vstPoint[10]; /* 左手腕 */
    const Inference_NS::Point_S& stRHip      = vstPoint[11]; /* 右臀 */
    const Inference_NS::Point_S& stLHip      = vstPoint[12]; /* 左臀 */
    const Inference_NS::Point_S& stRKnee     = vstPoint[13]; /* 右膝盖 */
    const Inference_NS::Point_S& stLKnee     = vstPoint[14]; /* 左膝盖 */
    const Inference_NS::Point_S& stRAnkle    = vstPoint[15]; /* 右脚踝 */
    const Inference_NS::Point_S& stLAnkle    = vstPoint[16]; /* 左脚踝 */
    const Inference_NS::Point_S& stHead      = vstPoint[17]; /* 头顶 */
    const Inference_NS::Point_S& stNeck      = vstPoint[18]; /* 颈部 */
    const Inference_NS::Point_S& stHip       = vstPoint[19]; /* 臀部 */

    if (stLEye.nShow > stRaisingHead.dKeyScore &&
        stLEar.nShow > stRaisingHead.dKeyScore &&
        stREye.nShow > stRaisingHead.dKeyScore &&
        stREar.nShow > stRaisingHead.dKeyScore)
    {

        if (std::max(stLEye.nY, stREye.nY) < std::min(stLEar.nY, stREar.nY))
        {
            return true;
        }

        double dAngle = getAngleBetweenPoints(stNose, stLEar, stREar);
        if (dAngle > stRaisingHead.dRaiseAngle)
        {
            return true;
        }
    }

    return false;
}

/* 转头检测 */
bool Behavior::isTurningHead(const std::vector<Inference_NS::Point_S>& vstPoint, TurningHead_S stTurningHead)
{
    if (vstPoint.size() != 26)
    {
        dlog(LOG_ERROR, "数据大小不对 [%ld]", vstPoint.size());
        return false;
    }

    const Inference_NS::Point_S& stNose      = vstPoint[0];  /* 鼻子 */
    const Inference_NS::Point_S& stREye      = vstPoint[1];  /* 右眼 */
    const Inference_NS::Point_S& stLEye      = vstPoint[2];  /* 左眼 */
    const Inference_NS::Point_S& stREar      = vstPoint[3];  /* 右耳朵 */
    const Inference_NS::Point_S& stLEar      = vstPoint[4];  /* 左耳朵 */
    const Inference_NS::Point_S& stRShoulder = vstPoint[5];  /* 右肩 */
    const Inference_NS::Point_S& stLShoulder = vstPoint[6];  /* 左肩 */
    const Inference_NS::Point_S& stRElbow    = vstPoint[7];  /* 右手肘 */
    const Inference_NS::Point_S& stLElbow    = vstPoint[8];  /* 左手肘 */
    const Inference_NS::Point_S& stRWrist    = vstPoint[9];  /* 右手腕 */
    const Inference_NS::Point_S& stLWrist    = vstPoint[10]; /* 左手腕 */
    const Inference_NS::Point_S& stRHip      = vstPoint[11]; /* 右臀 */
    const Inference_NS::Point_S& stLHip      = vstPoint[12]; /* 左臀 */
    const Inference_NS::Point_S& stRKnee     = vstPoint[13]; /* 右膝盖 */
    const Inference_NS::Point_S& stLKnee     = vstPoint[14]; /* 左膝盖 */
    const Inference_NS::Point_S& stRAnkle    = vstPoint[15]; /* 右脚踝 */
    const Inference_NS::Point_S& stLAnkle    = vstPoint[16]; /* 左脚踝 */
    const Inference_NS::Point_S& stHead      = vstPoint[17]; /* 头顶 */
    const Inference_NS::Point_S& stNeck      = vstPoint[18]; /* 颈部 */
    const Inference_NS::Point_S& stHip       = vstPoint[19]; /* 臀部 */

    if (stLEye.nShow > stTurningHead.dKeyScore &&
        stREye.nShow > stTurningHead.dKeyScore &&
        stHead.nShow > stTurningHead.dKeyScore &&
        stNeck.nShow > stTurningHead.dKeyScore)
    {
        /* 转头 以头颈线为基准，左眼在右边，右眼在左边边都为转头 */

        int nLEye = getPointRelativeOfLine(stHead, stNeck, stLEye);
        if (nLEye <= 0)
        {
            /* 转头 */
            return true;
        }
        else
        {
            int nREye = getPointRelativeOfLine(stHead, stNeck, stREye);
            if (nREye >= 0)
            {
                /* 转头 */
                return true;
            }
        }
    }

    return false;
}

/* 转身检测 */
bool Behavior::isTurningBody(const std::vector<Inference_NS::Point_S>& vstPoint, TurningBody_S stTurningBody)
{
    if (vstPoint.size() != 26)
    {
        dlog(LOG_ERROR, "数据大小不对 [%ld]", vstPoint.size());
        return false;
    }

    const Inference_NS::Point_S& stNose      = vstPoint[0];  /* 鼻子 */
    const Inference_NS::Point_S& stREye      = vstPoint[1];  /* 右眼 */
    const Inference_NS::Point_S& stLEye      = vstPoint[2];  /* 左眼 */
    const Inference_NS::Point_S& stREar      = vstPoint[3];  /* 右耳朵 */
    const Inference_NS::Point_S& stLEar      = vstPoint[4];  /* 左耳朵 */
    const Inference_NS::Point_S& stRShoulder = vstPoint[5];  /* 右肩 */
    const Inference_NS::Point_S& stLShoulder = vstPoint[6];  /* 左肩 */
    const Inference_NS::Point_S& stRElbow    = vstPoint[7];  /* 右手肘 */
    const Inference_NS::Point_S& stLElbow    = vstPoint[8];  /* 左手肘 */
    const Inference_NS::Point_S& stRWrist    = vstPoint[9];  /* 右手腕 */
    const Inference_NS::Point_S& stLWrist    = vstPoint[10]; /* 左手腕 */
    const Inference_NS::Point_S& stRHip      = vstPoint[11]; /* 右臀 */
    const Inference_NS::Point_S& stLHip      = vstPoint[12]; /* 左臀 */
    const Inference_NS::Point_S& stRKnee     = vstPoint[13]; /* 右膝盖 */
    const Inference_NS::Point_S& stLKnee     = vstPoint[14]; /* 左膝盖 */
    const Inference_NS::Point_S& stRAnkle    = vstPoint[15]; /* 右脚踝 */
    const Inference_NS::Point_S& stLAnkle    = vstPoint[16]; /* 左脚踝 */
    const Inference_NS::Point_S& stHead      = vstPoint[17]; /* 头顶 */
    const Inference_NS::Point_S& stNeck      = vstPoint[18]; /* 颈部 */
    const Inference_NS::Point_S& stHip       = vstPoint[19]; /* 臀部 */

    /* 如果左肩大于右肩，则为转身 */
    if (stLShoulder.nShow > stTurningBody.dKeyScore &&
        stRShoulder.nShow > stTurningBody.dKeyScore &&
        stLShoulder.nX > stRShoulder.nX)
    {
        /* 转身 */
        return true;
    }

    return false;
}

/* 站立检测 */
bool Behavior::isStanding(const std::vector<Inference_NS::Point_S>& vstPoint, Standing_S stStanding)
{
    if (vstPoint.size() != 26)
    {
        dlog(LOG_ERROR, "数据大小不对 [%ld]", vstPoint.size());
        return false;
    }

    const Inference_NS::Point_S& stNose      = vstPoint[0];  /* 鼻子 */
    const Inference_NS::Point_S& stREye      = vstPoint[1];  /* 右眼 */
    const Inference_NS::Point_S& stLEye      = vstPoint[2];  /* 左眼 */
    const Inference_NS::Point_S& stREar      = vstPoint[3];  /* 右耳朵 */
    const Inference_NS::Point_S& stLEar      = vstPoint[4];  /* 左耳朵 */
    const Inference_NS::Point_S& stRShoulder = vstPoint[5];  /* 右肩 */
    const Inference_NS::Point_S& stLShoulder = vstPoint[6];  /* 左肩 */
    const Inference_NS::Point_S& stRElbow    = vstPoint[7];  /* 右手肘 */
    const Inference_NS::Point_S& stLElbow    = vstPoint[8];  /* 左手肘 */
    const Inference_NS::Point_S& stRWrist    = vstPoint[9];  /* 右手腕 */
    const Inference_NS::Point_S& stLWrist    = vstPoint[10]; /* 左手腕 */
    const Inference_NS::Point_S& stRHip      = vstPoint[11]; /* 右臀 */
    const Inference_NS::Point_S& stLHip      = vstPoint[12]; /* 左臀 */
    const Inference_NS::Point_S& stRKnee     = vstPoint[13]; /* 右膝盖 */
    const Inference_NS::Point_S& stLKnee     = vstPoint[14]; /* 左膝盖 */
    const Inference_NS::Point_S& stRAnkle    = vstPoint[15]; /* 右脚踝 */
    const Inference_NS::Point_S& stLAnkle    = vstPoint[16]; /* 左脚踝 */
    const Inference_NS::Point_S& stHead      = vstPoint[17]; /* 头顶 */
    const Inference_NS::Point_S& stNeck      = vstPoint[18]; /* 颈部 */
    const Inference_NS::Point_S& stHip       = vstPoint[19]; /* 臀部 */

    /* 如果左肩大于右肩，则为转身 */
    if (stLShoulder.nShow > stStanding.dKeyScore &&
        stRShoulder.nShow > stStanding.dKeyScore &&
        stLShoulder.nX > stRShoulder.nX)
    {
        if (stLHip.nShow > stStanding.dKeyScore &&
            stRHip.nShow > stStanding.dKeyScore)
        {
            /* 转身的站立 */
            return true;
        }
    }

    /* 站立 */
#if 1
    bool bValue = false;
    /* 站立 小腿大腿竖直20度以内，且能检测到身体 */
    if (stLHip.nShow > stStanding.dKeyScore && stLKnee.nShow > stStanding.dKeyScore)
    {
        if (stLHip.nY < stLKnee.nY)
        {
            /* 膝盖和臀部的点与Y轴夹角，小于一定的角度 */
            double dAxis = getAngleWithYAxis(stLKnee, stLHip);
            if (dAxis < stStanding.dYRaiseAngle)
            {
                bValue = true;
            }
        }
    }
    if (stRHip.nShow > stStanding.dKeyScore && stRKnee.nShow > stStanding.dKeyScore)
    {
        if (stRHip.nY < stRKnee.nY)
        {
            double dAxis = getAngleWithYAxis(stRKnee, stRHip);
            if (dAxis < stStanding.dYRaiseAngle)
            {
                bValue = true;
            }
        }
    }

    /* 满足上述条件后，还要判断 */
    if (bValue)
    {
        /* 必须有一个手的手腕低于手肘 */
        if (stLElbow.nShow > stStanding.dKeyScore &&
            stLWrist.nShow > stStanding.dKeyScore &&
            stLWrist.nY > stLElbow.nY)
        {
            /* 且手肘到手腕和手肘到肩膀的角度必须大于某个角度 */
            double dAxis = getAngleBetweenPoints(stLElbow, stLShoulder, stLWrist);
            if (dAxis > stStanding.dRaiseAngle)
            {
                return true;
            }
        }
        if (stRElbow.nShow > stStanding.dKeyScore &&
            stRWrist.nShow > stStanding.dKeyScore &&
            stRWrist.nY > stRElbow.nY)
        {
            /* 且手肘到手腕和手肘到肩膀的角度必须大于某个角度 */
            double dAxis = getAngleBetweenPoints(stRElbow, stRShoulder, stRWrist);
            if (dAxis > stStanding.dRaiseAngle)
            {
                return true;
            }
        }
    }

#endif
    return false;
}

/* 举手检测 */
bool Behavior::isRaisingHand(const std::vector<Inference_NS::Point_S>& vstPoint, RaisingHand_S stRaisingHand)
{
    if (vstPoint.size() != 26)
    {
        dlog(LOG_ERROR, "数据大小不对 [%ld]", vstPoint.size());
        return false;
    }

    const Inference_NS::Point_S& stNose      = vstPoint[0];  /* 鼻子 */
    const Inference_NS::Point_S& stREye      = vstPoint[1];  /* 右眼 */
    const Inference_NS::Point_S& stLEye      = vstPoint[2];  /* 左眼 */
    const Inference_NS::Point_S& stREar      = vstPoint[3];  /* 右耳朵 */
    const Inference_NS::Point_S& stLEar      = vstPoint[4];  /* 左耳朵 */
    const Inference_NS::Point_S& stRShoulder = vstPoint[5];  /* 右肩 */
    const Inference_NS::Point_S& stLShoulder = vstPoint[6];  /* 左肩 */
    const Inference_NS::Point_S& stRElbow    = vstPoint[7];  /* 右手肘 */
    const Inference_NS::Point_S& stLElbow    = vstPoint[8];  /* 左手肘 */
    const Inference_NS::Point_S& stRWrist    = vstPoint[9];  /* 右手腕 */
    const Inference_NS::Point_S& stLWrist    = vstPoint[10]; /* 左手腕 */
    const Inference_NS::Point_S& stRHip      = vstPoint[11]; /* 右臀 */
    const Inference_NS::Point_S& stLHip      = vstPoint[12]; /* 左臀 */
    const Inference_NS::Point_S& stRKnee     = vstPoint[13]; /* 右膝盖 */
    const Inference_NS::Point_S& stLKnee     = vstPoint[14]; /* 左膝盖 */
    const Inference_NS::Point_S& stRAnkle    = vstPoint[15]; /* 右脚踝 */
    const Inference_NS::Point_S& stLAnkle    = vstPoint[16]; /* 左脚踝 */
    const Inference_NS::Point_S& stHead      = vstPoint[17]; /* 头顶 */
    const Inference_NS::Point_S& stNeck      = vstPoint[18]; /* 颈部 */
    const Inference_NS::Point_S& stHip       = vstPoint[19]; /* 臀部 */


    /* 举手，1.手腕高于手肘，并且手肘和手肘连成的直线在纵向小于一定度 */
    if (stLElbow.nShow > stRaisingHand.dKeyScore &&
        stLWrist.nShow > stRaisingHand.dKeyScore)
    {
        if (stLElbow.nY > stLWrist.nY)
        {
            double dAxis = getAngleWithYAxis(stLElbow, stLWrist);
            if (dAxis < stRaisingHand.dYRaiseAngle)
            {
                /* 举左手 */
                return true;
            }
            // dlog(LOG_ERROR, "举左手 [%f] %f(%f,%f)-(%f,%f)",
            //      dAxis,
            //      stRaisingHand.dRaiseAngle,
            //      stLElbow.nX,
            //      stLElbow.nY,
            //      stLWrist.nX,
            //      stLWrist.nY);
        }
    }

    if (stRElbow.nShow > stRaisingHand.dKeyScore &&
        stRWrist.nShow > stRaisingHand.dKeyScore)
    {
        if (stRElbow.nY > stRWrist.nY)
        {
            double dAxis = getAngleWithYAxis(stRElbow, stRWrist);
            if (dAxis < stRaisingHand.dYRaiseAngle)
            {
                /* 举右手 */
                return true;
            }
            // dlog(LOG_ERROR, "举右手 [%f] %f(%f,%f)-(%f,%f)",
            //      dAxis,
            //      stRaisingHand.dRaiseAngle,
            //      stRElbow.nX,
            //      stRElbow.nY,
            //      stRWrist.nX,
            //      stRWrist.nY);
        }
    }

    return false;
}

/* 趴桌检测 */
bool Behavior::isLyingOnDesk(const std::vector<Inference_NS::Point_S>& vstPoint, LyingOnDesk_S stLyingOnDesk)
{
    if (vstPoint.size() != 26)
    {
        dlog(LOG_ERROR, "数据大小不对 [%ld]", vstPoint.size());
        return false;
    }

    const Inference_NS::Point_S& stNose      = vstPoint[0];  /* 鼻子 */
    const Inference_NS::Point_S& stREye      = vstPoint[1];  /* 右眼 */
    const Inference_NS::Point_S& stLEye      = vstPoint[2];  /* 左眼 */
    const Inference_NS::Point_S& stREar      = vstPoint[3];  /* 右耳朵 */
    const Inference_NS::Point_S& stLEar      = vstPoint[4];  /* 左耳朵 */
    const Inference_NS::Point_S& stRShoulder = vstPoint[5];  /* 右肩 */
    const Inference_NS::Point_S& stLShoulder = vstPoint[6];  /* 左肩 */
    const Inference_NS::Point_S& stRElbow    = vstPoint[7];  /* 右手肘 */
    const Inference_NS::Point_S& stLElbow    = vstPoint[8];  /* 左手肘 */
    const Inference_NS::Point_S& stRWrist    = vstPoint[9];  /* 右手腕 */
    const Inference_NS::Point_S& stLWrist    = vstPoint[10]; /* 左手腕 */
    const Inference_NS::Point_S& stRHip      = vstPoint[11]; /* 右臀 */
    const Inference_NS::Point_S& stLHip      = vstPoint[12]; /* 左臀 */
    const Inference_NS::Point_S& stRKnee     = vstPoint[13]; /* 右膝盖 */
    const Inference_NS::Point_S& stLKnee     = vstPoint[14]; /* 左膝盖 */
    const Inference_NS::Point_S& stRAnkle    = vstPoint[15]; /* 右脚踝 */
    const Inference_NS::Point_S& stLAnkle    = vstPoint[16]; /* 左脚踝 */
    const Inference_NS::Point_S& stHead      = vstPoint[17]; /* 头顶 */
    const Inference_NS::Point_S& stNeck      = vstPoint[18]; /* 颈部 */
    const Inference_NS::Point_S& stHip       = vstPoint[19]; /* 臀部 */

    /* 趴桌子 : 头顶，有一个低于肩膀。则说明为趴桌子 */
    if (stLShoulder.nShow > stLyingOnDesk.dKeyScore &&
        stRShoulder.nShow > stLyingOnDesk.dKeyScore &&
        stHead.nShow > stLyingOnDesk.dKeyScore)
    {
        double dMinShoulder = std::min(stLShoulder.nY, stRShoulder.nY);
        if ((stHead.nY) >= dMinShoulder)
        {
            return true;
        }
    }


    return false;
}

/* 玩手机检测 */
bool Behavior::isPlayPhone(const std::vector<Inference_NS::Point_S>& vstPoint, PlayPhone_S stPlayPhone)
{
    if (vstPoint.size() != 26)
    {
        dlog(LOG_ERROR, "数据大小不对 [%ld]", vstPoint.size());
        return false;
    }

    const Inference_NS::Point_S& stNose      = vstPoint[0];  /* 鼻子 */
    const Inference_NS::Point_S& stREye      = vstPoint[1];  /* 右眼 */
    const Inference_NS::Point_S& stLEye      = vstPoint[2];  /* 左眼 */
    const Inference_NS::Point_S& stREar      = vstPoint[3];  /* 右耳朵 */
    const Inference_NS::Point_S& stLEar      = vstPoint[4];  /* 左耳朵 */
    const Inference_NS::Point_S& stRShoulder = vstPoint[5];  /* 右肩 */
    const Inference_NS::Point_S& stLShoulder = vstPoint[6];  /* 左肩 */
    const Inference_NS::Point_S& stRElbow    = vstPoint[7];  /* 右手肘 */
    const Inference_NS::Point_S& stLElbow    = vstPoint[8];  /* 左手肘 */
    const Inference_NS::Point_S& stRWrist    = vstPoint[9];  /* 右手腕 */
    const Inference_NS::Point_S& stLWrist    = vstPoint[10]; /* 左手腕 */
    const Inference_NS::Point_S& stRHip      = vstPoint[11]; /* 右臀 */
    const Inference_NS::Point_S& stLHip      = vstPoint[12]; /* 左臀 */
    const Inference_NS::Point_S& stRKnee     = vstPoint[13]; /* 右膝盖 */
    const Inference_NS::Point_S& stLKnee     = vstPoint[14]; /* 左膝盖 */
    const Inference_NS::Point_S& stRAnkle    = vstPoint[15]; /* 右脚踝 */
    const Inference_NS::Point_S& stLAnkle    = vstPoint[16]; /* 左脚踝 */
    const Inference_NS::Point_S& stHead      = vstPoint[17]; /* 头顶 */
    const Inference_NS::Point_S& stNeck      = vstPoint[18]; /* 颈部 */
    const Inference_NS::Point_S& stHip       = vstPoint[19]; /* 臀部 */

    BendingHead_S stBendingHead;
    /* 玩手机： 低头+两个手靠近（两个肩膀的X轴距离(固定)与两个手腕的X轴距离比例） */
    // dlog(LOG_USER,"stPlayPhone.dKeyScore=%f",stPlayPhone.dKeyScore);
    // dlog(LOG_USER,"stLShoulder.nShow=%f",stLShoulder.nShow);
    // dlog(LOG_USER,"stRShoulder.nShow=%f",stRShoulder.nShow);
    // dlog(LOG_USER,"stLWrist.nShow=%f",stLWrist.nShow);
    // dlog(LOG_USER,"stRWrist.nShow=%f",stRWrist.nShow);
    // dlog(LOG_USER,"isBendingHead(vstPoint,stBendingHead)=%d",isBendingHead(vstPoint,stBendingHead));
    /* 获取两个手腕的斜率 */
    double        dSlope = getSlope(stLWrist, stRWrist);
    if (stLShoulder.nShow > stPlayPhone.dKeyScore &&
        stRShoulder.nShow > stPlayPhone.dKeyScore &&
        stLWrist.nShow > stPlayPhone.dKeyScore &&
        stRWrist.nShow > stPlayPhone.dKeyScore &&
        isBendingHead(vstPoint, stBendingHead) &&
        dSlope > 0 &&
        dSlope < 0.1 &&
        stRShoulder.nX > stLShoulder.nX &&
        stRWrist.nX > stLWrist.nX)
    {
        Standing_S stStanding;
        /* 过滤掉异常动作 - 不符合玩手机的动作 */
        if (stLWrist.nX > stLShoulder.nX &&
            stLWrist.nX < stRShoulder.nX &&
            stRWrist.nX > stLShoulder.nX &&
            stRWrist.nX < stRShoulder.nX &&
            false == isStanding(vstPoint, stStanding))
        {
            /* 除数不能为0 */
            if (stRWrist.nX - stLWrist.nX != 0)
            {
                /* 系数比：手腕宽度 / 肩宽度 = 系数比 */
                double dRatio = (stRWrist.nX - stLWrist.nX) / (stRShoulder.nX - stLShoulder.nX);
                // dlog(LOG_USER,"系数比：stPlayPhone.dRatio=%f",stPlayPhone.dRatio);
                // dlog(LOG_USER,"系数比：dRatio=%f",dRatio);
                if (dRatio <= stPlayPhone.dRatio && dRatio > 0)
                {
                    return true;
                }
            }
        }
    }

    return false;
}

/* 接打电话检测 */
bool Behavior::isCallPhone(
    const std::vector<Inference_NS::Point_S>& vstPoint,
    CallPhone_S                               stCallPhone)
{
    if (vstPoint.size() != 26)
    {
        dlog(LOG_ERROR, "数据大小不对 [%ld]", vstPoint.size());
        return false;
    }

    const Inference_NS::Point_S& stNose      = vstPoint[0];  /* 鼻子 */
    const Inference_NS::Point_S& stREye      = vstPoint[1];  /* 右眼 */
    const Inference_NS::Point_S& stLEye      = vstPoint[2];  /* 左眼 */
    const Inference_NS::Point_S& stREar      = vstPoint[3];  /* 右耳朵 */
    const Inference_NS::Point_S& stLEar      = vstPoint[4];  /* 左耳朵 */
    const Inference_NS::Point_S& stRShoulder = vstPoint[5];  /* 右肩 */
    const Inference_NS::Point_S& stLShoulder = vstPoint[6];  /* 左肩 */
    const Inference_NS::Point_S& stRElbow    = vstPoint[7];  /* 右手肘 */
    const Inference_NS::Point_S& stLElbow    = vstPoint[8];  /* 左手肘 */
    const Inference_NS::Point_S& stRWrist    = vstPoint[9];  /* 右手腕 */
    const Inference_NS::Point_S& stLWrist    = vstPoint[10]; /* 左手腕 */
    const Inference_NS::Point_S& stRHip      = vstPoint[11]; /* 右臀 */
    const Inference_NS::Point_S& stLHip      = vstPoint[12]; /* 左臀 */
    const Inference_NS::Point_S& stRKnee     = vstPoint[13]; /* 右膝盖 */
    const Inference_NS::Point_S& stLKnee     = vstPoint[14]; /* 左膝盖 */
    const Inference_NS::Point_S& stRAnkle    = vstPoint[15]; /* 右脚踝 */
    const Inference_NS::Point_S& stLAnkle    = vstPoint[16]; /* 左脚踝 */
    const Inference_NS::Point_S& stHead      = vstPoint[17]; /* 头顶 */
    const Inference_NS::Point_S& stNeck      = vstPoint[18]; /* 颈部 */
    const Inference_NS::Point_S& stHip       = vstPoint[19]; /* 臀部 */

    /* 不能背对 */
    if (stRShoulder.nX > stLShoulder.nX &&
        stRWrist.nX > stLWrist.nX)
    {
        /* 左边接打电话： 耳朵为圆心，耳朵到肩膀距离为半径，手腕的点在半径范围内为接打电话 */
        if (stLShoulder.nShow > stCallPhone.dKeyScore &&
            stLEar.nShow > stCallPhone.dKeyScore &&
            stLWrist.nShow > stCallPhone.dKeyScore)
        {
            /* 计算左耳到左肩的距离，作为半径 */
            double dRadius     = getDistance(stLShoulder, stLEar);
            /* 计算手腕到左耳的距离 */
            double dEarToWrist = getDistance(stLWrist, stLEar);
            // dlog(LOG_INFO,"左耳坐标[x:%f,y:%f]",stLEar.nX,stLEar.nY);
            // dlog(LOG_INFO,"左肩坐标[x:%f,y:%f]",stLShoulder.nX,stLShoulder.nY);
            // dlog(LOG_INFO,"左手腕坐标[x:%f,y:%f]",stLWrist.nX,stLWrist.nY);
            // dlog(LOG_INFO,"左耳到左肩的距离：dRadius=%f",dRadius);
            // dlog(LOG_INFO,"左手腕到左耳的距离：dEarToWrist=%f",dEarToWrist);
            /* 判断手腕是否在圆内 */
            if (dEarToWrist < dRadius)
            {
                // dlog(LOG_INFO,"符合左边接打电话");
                return true;
            }
        }

        /* 右边接打电话： 耳朵为圆心，耳朵到肩膀距离为半径，手腕的点在半径范围内为接打电话 */
        if (stRShoulder.nShow > stCallPhone.dKeyScore &&
            stREar.nShow > stCallPhone.dKeyScore &&
            stRWrist.nShow > stCallPhone.dKeyScore)
        {
            /* 计算右耳到右肩的距离，作为半径 */
            double dRadius     = getDistance(stRShoulder, stREar);
            /* 计算手腕到右耳的距离 */
            double dEarToWrist = getDistance(stRWrist, stREar);
            // dlog(LOG_INFO,"右耳坐标[x:%f,y:%f]",stREar.nX,stREar.nY);
            // dlog(LOG_INFO,"右肩坐标[x:%f,y:%f]",stRShoulder.nX,stRShoulder.nY);
            // dlog(LOG_INFO,"右手腕坐标[x:%f,y:%f]",stRWrist.nX,stRWrist.nY);
            // dlog(LOG_INFO,"右耳到右肩的距离：dRadius=%f",dRadius);
            // dlog(LOG_INFO,"右手腕到右耳的距离：dEarToWrist=%f",dEarToWrist);
            /* 判断手腕是否在圆内 */
            if (dEarToWrist < dRadius)
            {
                // dlog(LOG_INFO,"符合右边接打电话");
                return true;
            }
        }
    }


    return false;
}

/* 板书检测 */
bool Behavior::isTeaBoard(const std::vector<Inference_NS::Point_S>& vstPoint, TeaBoard_S stTeaBoard)
{
    if (vstPoint.size() != 26)
    {
        dlog(LOG_ERROR, "数据大小不对 [%ld]", vstPoint.size());
        return false;
    }

    const Inference_NS::Point_S& stNose      = vstPoint[0];  /* 鼻子 */
    const Inference_NS::Point_S& stREye      = vstPoint[1];  /* 右眼 */
    const Inference_NS::Point_S& stLEye      = vstPoint[2];  /* 左眼 */
    const Inference_NS::Point_S& stREar      = vstPoint[3];  /* 右耳朵 */
    const Inference_NS::Point_S& stLEar      = vstPoint[4];  /* 左耳朵 */
    const Inference_NS::Point_S& stRShoulder = vstPoint[5];  /* 右肩 */
    const Inference_NS::Point_S& stLShoulder = vstPoint[6];  /* 左肩 */
    const Inference_NS::Point_S& stRElbow    = vstPoint[7];  /* 右手肘 */
    const Inference_NS::Point_S& stLElbow    = vstPoint[8];  /* 左手肘 */
    const Inference_NS::Point_S& stRWrist    = vstPoint[9];  /* 右手腕 */
    const Inference_NS::Point_S& stLWrist    = vstPoint[10]; /* 左手腕 */
    const Inference_NS::Point_S& stRHip      = vstPoint[11]; /* 右臀 */
    const Inference_NS::Point_S& stLHip      = vstPoint[12]; /* 左臀 */
    const Inference_NS::Point_S& stRKnee     = vstPoint[13]; /* 右膝盖 */
    const Inference_NS::Point_S& stLKnee     = vstPoint[14]; /* 左膝盖 */
    const Inference_NS::Point_S& stRAnkle    = vstPoint[15]; /* 右脚踝 */
    const Inference_NS::Point_S& stLAnkle    = vstPoint[16]; /* 左脚踝 */
    const Inference_NS::Point_S& stHead      = vstPoint[17]; /* 头顶 */
    const Inference_NS::Point_S& stNeck      = vstPoint[18]; /* 颈部 */
    const Inference_NS::Point_S& stHip       = vstPoint[19]; /* 臀部 */

    if (stRShoulder.nShow > stTeaBoard.dKeyScore && stLShoulder.nShow > stTeaBoard.dKeyScore)
    {
        // dlog(LOG_INFO,"右肩x坐标：cstRShoulder.nX=%f",stRShoulder.nX);
        // dlog(LOG_INFO,"左肩x坐标：cstRShoulder.nX=%f",stLShoulder.nX);
        /* 背对 */
        if (stRShoulder.nX < stLShoulder.nX)
        {
            // dlog(LOG_INFO,"背对。。。");
            return true;
        }
    }

    return false;
}

/* 双手抱臂判断 */
bool Ai0630_NS::Behavior::isArmsCrossed(
    const std::vector<Inference_NS::Point_S>& vstPoint,
    ArmsCrossed_S                             stInfo)
{
    if (vstPoint.size() != 26)
    {
        dlog(LOG_ERROR, "数据大小不对 [%ld]", vstPoint.size());
        return false;
    }

    const Inference_NS::Point_S& stNose      = vstPoint[0];  /* 鼻子 */
    const Inference_NS::Point_S& stREye      = vstPoint[1];  /* 右眼 */
    const Inference_NS::Point_S& stLEye      = vstPoint[2];  /* 左眼 */
    const Inference_NS::Point_S& stREar      = vstPoint[3];  /* 右耳朵 */
    const Inference_NS::Point_S& stLEar      = vstPoint[4];  /* 左耳朵 */
    const Inference_NS::Point_S& stRShoulder = vstPoint[5];  /* 右肩 */
    const Inference_NS::Point_S& stLShoulder = vstPoint[6];  /* 左肩 */
    const Inference_NS::Point_S& stRElbow    = vstPoint[7];  /* 右手肘 */
    const Inference_NS::Point_S& stLElbow    = vstPoint[8];  /* 左手肘 */
    const Inference_NS::Point_S& stRWrist    = vstPoint[9];  /* 右手腕 */
    const Inference_NS::Point_S& stLWrist    = vstPoint[10]; /* 左手腕 */
    const Inference_NS::Point_S& stRHip      = vstPoint[11]; /* 右臀 */
    const Inference_NS::Point_S& stLHip      = vstPoint[12]; /* 左臀 */
    const Inference_NS::Point_S& stRKnee     = vstPoint[13]; /* 右膝盖 */
    const Inference_NS::Point_S& stLKnee     = vstPoint[14]; /* 左膝盖 */
    const Inference_NS::Point_S& stRAnkle    = vstPoint[15]; /* 右脚踝 */
    const Inference_NS::Point_S& stLAnkle    = vstPoint[16]; /* 左脚踝 */
    const Inference_NS::Point_S& stHead      = vstPoint[17]; /* 头顶 */
    const Inference_NS::Point_S& stNeck      = vstPoint[18]; /* 颈部 */
    const Inference_NS::Point_S& stHip       = vstPoint[19]; /* 臀部 */

    if (stLElbow.nShow > stInfo.dKeyScore &&
        stRElbow.nShow > stInfo.dKeyScore &&
        stLHip.nShow > stInfo.dKeyScore &&
        stRHip.nShow > stInfo.dKeyScore)
    {
        double dL1 = getDistance(stLElbow, stRElbow);
        double dL2 = getDistance(stLElbow, stRHip);
        double dL3 = getDistance(stRElbow, stLHip);

        if (dL2 / dL1 < stInfo.dRatio &&
            dL3 / dL1 < stInfo.dRatio)
        {
            return true;
        }
    }

    return false;
}

/* 叉腰表达 */
bool Ai0630_NS::Behavior::isHandsOnHip(
    const std::vector<Inference_NS::Point_S>& vstPoint,
    HandsOnHip_S                              stInfo)
{
    if (vstPoint.size() != 26)
    {
        dlog(LOG_ERROR, "数据大小不对 [%ld]", vstPoint.size());
        return false;
    }

    const Inference_NS::Point_S& stNose      = vstPoint[0];  /* 鼻子 */
    const Inference_NS::Point_S& stREye      = vstPoint[1];  /* 右眼 */
    const Inference_NS::Point_S& stLEye      = vstPoint[2];  /* 左眼 */
    const Inference_NS::Point_S& stREar      = vstPoint[3];  /* 右耳朵 */
    const Inference_NS::Point_S& stLEar      = vstPoint[4];  /* 左耳朵 */
    const Inference_NS::Point_S& stRShoulder = vstPoint[5];  /* 右肩 */
    const Inference_NS::Point_S& stLShoulder = vstPoint[6];  /* 左肩 */
    const Inference_NS::Point_S& stRElbow    = vstPoint[7];  /* 右手肘 */
    const Inference_NS::Point_S& stLElbow    = vstPoint[8];  /* 左手肘 */
    const Inference_NS::Point_S& stRWrist    = vstPoint[9];  /* 右手腕 */
    const Inference_NS::Point_S& stLWrist    = vstPoint[10]; /* 左手腕 */
    const Inference_NS::Point_S& stRHip      = vstPoint[11]; /* 右臀 */
    const Inference_NS::Point_S& stLHip      = vstPoint[12]; /* 左臀 */
    const Inference_NS::Point_S& stRKnee     = vstPoint[13]; /* 右膝盖 */
    const Inference_NS::Point_S& stLKnee     = vstPoint[14]; /* 左膝盖 */
    const Inference_NS::Point_S& stRAnkle    = vstPoint[15]; /* 右脚踝 */
    const Inference_NS::Point_S& stLAnkle    = vstPoint[16]; /* 左脚踝 */
    const Inference_NS::Point_S& stHead      = vstPoint[17]; /* 头顶 */
    const Inference_NS::Point_S& stNeck      = vstPoint[18]; /* 颈部 */
    const Inference_NS::Point_S& stHip       = vstPoint[19]; /* 臀部 */

    /* 手腕在臀外侧 */
    if (stLWrist.nShow > stInfo.dKeyScore &&
        stRWrist.nShow > stInfo.dKeyScore &&
        stLHip.nShow > stInfo.dKeyScore &&
        stRHip.nShow > stInfo.dKeyScore)
    {
        double dL1 = getDistance(stRHip, stLHip);
        double dL2 = getDistance(stLWrist, stLHip);
        double dL3 = getDistance(stRHip, stRWrist);

        if (dL2 / dL1 < stInfo.dRatio &&
            dL3 / dL1 < stInfo.dRatio)
        {
            return true;
        }
    }

    return false;
}

/* 计算两点之间与y轴的角度 */
double Behavior::getAngleWithYAxis(const Inference_NS::Point_S& stPoint1, const Inference_NS::Point_S& stPoint2)
{
    /* 确保两点的 x 坐标不同，避免除以零错误 */
    if (stPoint1.nX == stPoint2.nX)
    {
        /* 如果两点的 x 坐标相同，则直线垂直于 x 轴，夹角为 90 度 */
        return 0;
    }

    /* 计算直线的斜率 */
    double dSlope = std::abs((stPoint2.nX - stPoint1.nX) / (stPoint2.nY - stPoint1.nY));

    /* 计算斜率的角度（弧度） */
    double dAngleRadians = std::atan(dSlope);

    /* 将弧度转换为角度 */
    double dAngleDegrees = dAngleRadians * 180.0 / M_PI;    // std::to_degrees(dAngleRadians);

    /* 确保角度在0到90度之间 */
    dAngleDegrees = std::min(dAngleDegrees, 90.0d);
    dAngleDegrees = std::max(dAngleDegrees, 0.0d);

    return dAngleDegrees;
}

/* 获取斜率 */
double Behavior::getSlope(const Inference_NS::Point_S& p1, const Inference_NS::Point_S& p2)
{
    if (p1.nX != p2.nX)
    {    // 检查是否为垂直线
        return (p2.nY - p1.nY) / (p2.nX - p1.nX);
    }
    else
    {
        return 0;    // 垂直线斜率设置为0
    }
}

/* 获取垂直交点 */
Inference_NS::Point_S Behavior::getIntersection(const Inference_NS::Point_S& A, const Inference_NS::Point_S& B, const Inference_NS::Point_S& C)
{
    double slopeAB            = getSlope(A, B);
    double slopePerpendicular = -1 / slopeAB;    // Perpendicular slope

    // 垂直线的方程 y - y1 = slopePerpendicular * (x - x1)
    // 线段AB的方程 y = slopeAB * x + (y1 - slopeAB * x1)

    // 交点坐标的计算
    Inference_NS::Point_S stOutInfo;

    if (slopeAB == 0)
    {                           // 线段AB水平
        stOutInfo.nX = A.nX;
        stOutInfo.nY = C.nY;    // 垂直线通过C点
    }
    else if (slopePerpendicular == 0)
    {                           // 垂直线水平
        stOutInfo.nY = C.nY;
        stOutInfo.nX = A.nX;    // 线段AB通过A点
    }
    else
    {
        stOutInfo.nX = (C.nY - (slopePerpendicular * C.nX) - (A.nY - slopeAB * A.nX)) / (slopeAB - slopePerpendicular);
        stOutInfo.nY = slopePerpendicular * stOutInfo.nX + (C.nY - slopePerpendicular * C.nX);
    }
    stOutInfo.nShow = 1.0;

    return stOutInfo;
}

/* 计算两点间的距离 */
double Behavior::getDistance(const Inference_NS::Point_S& stPoint1, const Inference_NS::Point_S& stPoint2)
{
    /* 欧几里得距离公式 */
    return sqrt((stPoint2.nX - stPoint1.nX) * (stPoint2.nX - stPoint1.nX) + (stPoint2.nY - stPoint1.nY) * (stPoint2.nY - stPoint1.nY));
}

/* 获取中心点 */
Inference_NS::Point_S Behavior::getMidpoint(const Inference_NS::Point_S& point1, const Inference_NS::Point_S& point2)
{
    Inference_NS::Point_S stOutInfo;
    stOutInfo.nShow = 1.0;

    /* 计算中心点的x坐标和y坐标 */
    stOutInfo.nX = (point1.nX + point2.nX) / 2.0;
    stOutInfo.nY = (point1.nY + point2.nY) / 2.0;

    return stOutInfo;
}

/* 计算三角形面积的两倍 */
double Behavior::getTwiceTriangleArea(const Inference_NS::Point_S& a, const Inference_NS::Point_S& b, const Inference_NS::Point_S& c)
{
    return a.nX * (b.nY - c.nY) + b.nX * (c.nY - a.nY) + c.nX * (a.nY - b.nY);
}

/* 计算由三个点形成的夹角的角度 a是交点 */
double Behavior::getAngleBetweenPoints(const Inference_NS::Point_S& a, const Inference_NS::Point_S& b, const Inference_NS::Point_S& c)
{
    // 计算三角形面积的两倍，检查是否共线
    double area = getTwiceTriangleArea(a, b, c);
    if (area == 0)
    {
        return 180.0;
    }

    // 计算向量AB和向量AC
    double ABx = b.nX - a.nX;
    double ABy = b.nY - a.nY;
    double ACx = c.nX - a.nX;
    double ACy = c.nY - a.nY;

    // 计算向量AB和向量AC的点积
    double dotProduct = ABx * ACx + ABy * ACy;

    // 计算向量AB和向量AC的模
    double magnitudeAB = sqrt(ABx * ABx + ABy * ABy);
    double magnitudeAC = sqrt(ACx * ACx + ACy * ACy);

    // 计算夹角的余弦值
    double cosAngle = dotProduct / (magnitudeAB * magnitudeAC);

    // 将余弦值转换为角度
    double angleRadians = acos(cosAngle);
    double angleDegrees = angleRadians * (180.0 / M_PI);

    return angleDegrees;
}

/* 计算向量AB和AP的叉积 */
double Behavior::getCrossProduct(const Inference_NS::Point_S& A, const Inference_NS::Point_S& B, const Inference_NS::Point_S& P)
{
    return (B.nX - A.nX) * (P.nY - A.nY) - (B.nY - A.nY) * (P.nX - A.nX);
}

/* 判断点P相对于由A和B定义的直线的位置, 具体返回值为 1 表示在左边，-1 表示在右边，0 表示在直线上 */
int Behavior::getPointRelativeOfLine(const Inference_NS::Point_S& A,
                                     const Inference_NS::Point_S& B,
                                     const Inference_NS::Point_S& P)
{
    double crossProduct = getCrossProduct(A, B, P);

    /* 根据直线方向决定左边的判断 */
    if (A.nY < B.nY)
    {
        /* 如果 A.y < B.y，则交叉积大于等于零在左边 */
        if (crossProduct == 0.0)
        {
            return 0;
        }
        else if (crossProduct > 0.0)
        {
            return 1;
        }
        else
        {
            return -1;
        }
    }
    else
    {
        if (crossProduct == 0.0)
        {
            return 0;
        }
        else if (crossProduct > 0.0)
        {
            return -1;
        }
        else
        {
            return 1;
        }
    }
    return 0;
}
