/*
 * @FilePath     : BehaviorProcess.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-07-27 11:41:43
 * @LastEditors: weigl 308241951@qq.com
 * @LastEditTime: 2025-02-26 17:02:18
 * @Description  :
 */
#pragma once

#include <vector>

#include "BlError.h"
#include "CVExtern.hpp"
#include "dlog.h"
#include "JsonInterfase.h"

class CBehaviorProcess
{

public:

    /* 关键点信息的结构体 */
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

    /**
     * @brief 处理基础行为数据
     * @param [BehaviorParam_S] stBehaviorParam: 行为参数
     * @param [string&] strInJson: 需要处理的基础行为Json数据
     * @param [string&] strOutJson:处理后的Json数据
     * @param [bool] isDelKeyJson: 是否删除关键点Json数据
     * @return [*]
     * @note
     */
    bool process(AiScenario_NS::BehaviorParam_S stBehaviorParam,
                 std::string&                   strInJson,
                 std::string&                   strOutJson,
                 bool                           isDelKeyJson = true);


    /**
     * @brief 处理基础行为数据
     * @param [SupClsInsParam_S] stSupClsInsParam: 行为参数
     * @param [string&] strInJson: 需要处理的基础行为Json数据
     * @param [string&] strOutJson:处理后的Json数据
     * @param [bool] isDelKeyJson: 是否删除关键点Json数据
     * @return [*]
     * @note
     */
    bool process(AiScenario_NS::SupClsInsParam_S stSupClsInsParam,
                 std::string&                  strInJson,
                 std::string&                  strOutJson,
                 bool                          isDelKeyJson = true);

    /**
     * @brief 获取当前行为类型
     * @param [BehaviorParam_S] stBehaviorParam: 行为参数
     * @param [vector<KeyPosInfo_S>] vecInfo: 行为特征点
     * @return [*]
     * @note
     */
    int getBehavioralType(
        AiScenario_NS::BehaviorParam_S              stBehaviorParam,
        std::vector<CBehaviorProcess::KeyPosInfo_S> vecInfo);

    /**
     * @brief 获取当前手机行为类型
     * @param [SupClsInsParam_S] stSupClsInsParam: 行为参数
     * @param [vector<KeyPosInfo_S>] vecInfo: 行为特征点
     * @return [*]
     * @note
     */
    int getBehavioralType(
        AiScenario_NS::SupClsInsParam_S stSupClsInsParam,
        std::vector<KeyPosInfo_S>     vecInfo);

private:

    /* 框坐标信息的结构体 */
    typedef struct _BoxInfo_
    {
        int nX1; /* 左上角 x 坐标 */
        int nY1; /* 左上角 y 坐标 */
        int nX2; /* 右下角 x 坐标 */
        int nY2; /* 右下角 y 坐标 */

        void clear()
        {
            nX1 = 0;
            nY1 = 0;
            nX2 = 0;
            nY2 = 0;
        }

        bool empty()
        {
            return (nX1 == 0) && (nY1 == 0) && (nX2 == 0) && (nY2 == 0);
        }

        _BoxInfo_()
        {
            clear();
        }
    } BoxInfo_S;

    /**
     * @brief 获取框信息
     * @param [Object*&] pArrayObject: 框信息Json数组
     * @param [BoxInfo_S&] stInfo: 数据
     * @return [*]
     * @note
     */
    bool get_boxInfo(Json::Object*& pArrayObject, BoxInfo_S& stInfo);

    /**
     * @brief 获取关键点
     * @param [Object*&] pArrayObject:
     * @param [Object*&] pArrayObject:
     * @param [vector&] vecInfo:
     * @return [*]
     * @note
     */
    bool get_keypoints(Json::Object*& pKeyObject, Json::Object*& pScoreObject, std::vector<KeyPosInfo_S>& vecInfo);


    /* 低头检测 */
    bool isBendingHead(const std::vector<KeyPosInfo_S>& vstPoint, AiScenario_NS::BendingHead_S stBendingHead);

    /* 抬头检测 */
    bool isRaisingHead(const std::vector<KeyPosInfo_S>& vstPoint, AiScenario_NS::RaisingHead_S stRaisingHead);

    /* 转头检测 */
    bool isTurningHead(const std::vector<KeyPosInfo_S>& vstPoint, AiScenario_NS::TurningHead_S stTurningHead);

    /* 转身检测 */
    bool isTurningBody(const std::vector<KeyPosInfo_S>& vstPoint, AiScenario_NS::TurningBody_S stTurningBody);

    /* 站立检测 */
    bool isStanding(const std::vector<KeyPosInfo_S>& vstPoint, AiScenario_NS::Standing_S stStanding);

    /* 举手检测 */
    bool isRaisingHand(const std::vector<KeyPosInfo_S>& vstPoint, AiScenario_NS::RaisingHand_S stRaisingHand);

    /* 趴桌检测 */
    bool isLyingOnDesk(const std::vector<KeyPosInfo_S>& vstPoint, AiScenario_NS::LyingOnDesk_S stLyingOnDesk);

    /* 玩手机检测 */
    bool isPlayPhone(const std::vector<KeyPosInfo_S>& vstPoint, AiScenario_NS::PlayPhone_S stPlayPhone);

    /* 接打电话检测 */
    bool isCallPhone(const std::vector<KeyPosInfo_S>& vstPoint, AiScenario_NS::CallPhone_S stCallPhone);

    /* 板书检测 */
    bool isTeaBoard(const std::vector<KeyPosInfo_S>& vstPoint, AiScenario_NS::TeaBoard_S stTeaBoard);



    /**
     * @brief 计算两点之间与y轴的角度
     * @param [KeyPosInfo_S&] stPoint1:
     * @param [KeyPosInfo_S&] stPoint2:
     * @return [*]
     * @note
     */
    double getAngleWithYAxis(const KeyPosInfo_S& stPoint1, const KeyPosInfo_S& stPoint2);

    /* 获取斜率 */
    double getSlope(const KeyPosInfo_S& p1, const KeyPosInfo_S& p2);

    /* 获取垂直交点 */
    KeyPosInfo_S getIntersection(const KeyPosInfo_S& A, const KeyPosInfo_S& B, const KeyPosInfo_S& C);

    /* 计算两点间的距离 */
    double getDistance(const KeyPosInfo_S& stPoint1, const KeyPosInfo_S& stPoint2);

    /* 获取中心点 */
    KeyPosInfo_S getMidpoint(const KeyPosInfo_S& point1, const KeyPosInfo_S& point2);

    // 计算三角形面积的两倍
    double getTwiceTriangleArea(const KeyPosInfo_S& a, const KeyPosInfo_S& b, const KeyPosInfo_S& c);

    // 计算由三个点形成的夹角的角度 a是交点
    double getAngleBetweenPoints(const KeyPosInfo_S& a, const KeyPosInfo_S& b, const KeyPosInfo_S& c);

    /* 计算向量AB和AP的叉积 */
    double getCrossProduct(const KeyPosInfo_S& A, const KeyPosInfo_S& B, const KeyPosInfo_S& P);

    // 判断点P相对于由A和B定义的直线的位置, 具体返回值为 1 表示在左边，-1 表示在右边，0 表示在直线上
    int getPointRelativeOfLine(const KeyPosInfo_S& A, const KeyPosInfo_S& B, const KeyPosInfo_S& P);
};
