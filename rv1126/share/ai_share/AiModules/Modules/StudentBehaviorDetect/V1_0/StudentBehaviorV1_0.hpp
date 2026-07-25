/*
 * @Author: zhengxh zhengxh@kfb.cn
 * @Date: 2026-03-30 19:19
 * @LastEditors: zhengxh zhengxh@kfb.cn
 * @LastEditTime: 2026-03-30 19:19
 * @FilePath: ai_share/AiModules/Modules/StudentBehaviorDetect/V1_0/StudentBehaviorV1_0.hpp
 * @Description: 学生行为分析
 */

#pragma once

#include "Yolov5.hpp"
#include "ImageFeature.hpp"
#include "StudentBehaviorExt.hpp"


namespace StudentBehavior_NS
{
    class CStudentBehaviorV1_0
    {
    public:

        CStudentBehaviorV1_0(InParam_S stInParam);
        ~CStudentBehaviorV1_0();

        /**
         * @brief 初始化
         * @return [*]
         * @note
         */
        bool init();

        /**
         * @brief 反初始化
         * @return [*]
         * @note
         */
        bool unInit();

        /**
         * @brief 处理数据
         * @param [CVData_S] stInData: 传入的视频数据
         * @param [char*&] pchOutData: 输出的处理结果
         * @param [int&] nDataSize: 数据大小
         * @return [*]
         * @note
         */
        bool process(InData_S stInData, std::vector<Result_S> &vecResult, OutData_S *stOutData = nullptr);

        int getBehavioralType(BehaviorParam_S stBehaviorParam, std::vector<KeyPosInfo_S> vecInfo);
    private:
        /* 低头检测 */
        bool isBendingHead(const std::vector<KeyPosInfo_S>& vstPoint, BendingHead_S stBendingHead);

        /* 抬头检测 */
        bool isRaisingHead(const std::vector<KeyPosInfo_S>& vstPoint, RaisingHead_S stRaisingHead);

        /* 转头检测 */
        bool isTurningHead(const std::vector<KeyPosInfo_S>& vstPoint, TurningHead_S stTurningHead);

        /* 转身检测 */
        bool isTurningBody(const std::vector<KeyPosInfo_S>& vstPoint, TurningBody_S stTurningBody);

        /* 站立检测 */
        bool isStanding(const std::vector<KeyPosInfo_S>& vstPoint, Standing_S stStanding);

        /* 举手检测 */
        bool isRaisingHand(const std::vector<KeyPosInfo_S>& vstPoint, RaisingHand_S stRaisingHand);

        /* 趴桌检测 */
        bool isLyingOnDesk(const std::vector<KeyPosInfo_S>& vstPoint, LyingOnDesk_S stLyingOnDesk);

        /**
         * @brief 计算两点之间与y轴的角度
         * @param [KeyPosInfo_S&] stPoint1:
         * @param [KeyPosInfo_S&] stPoint2:
         * @return [*]
         * @note
         */
        double getAngleWithYAxis(const KeyPosInfo_S& stPoint1, const KeyPosInfo_S& stPoint2);

        // 计算三角形面积的两倍
        double getTwiceTriangleArea(const KeyPosInfo_S& a, const KeyPosInfo_S& b, const KeyPosInfo_S& c);

        // 计算由三个点形成的夹角的角度 a是交点
        double getAngleBetweenPoints(const KeyPosInfo_S& a, const KeyPosInfo_S& b, const KeyPosInfo_S& c);

        /* 获取斜率 */
        double getSlope(const KeyPosInfo_S& p1, const KeyPosInfo_S& p2);

        /* 获取垂直交点 */
        KeyPosInfo_S getIntersection(const KeyPosInfo_S& A, const KeyPosInfo_S& B, const KeyPosInfo_S& C);

        /* 计算两点间的距离 */
        double getDistance(const KeyPosInfo_S& stPoint1, const KeyPosInfo_S& stPoint2);

        // 判断点P相对于由A和B定义的直线的位置, 具体返回值为 1 表示在左边，-1 表示在右边，0 表示在直线上
        int getPointRelativeOfLine(const KeyPosInfo_S& A, const KeyPosInfo_S& B, const KeyPosInfo_S& P);

        /* 计算向量AB和AP的叉积 */
        double getCrossProduct(const KeyPosInfo_S& A, const KeyPosInfo_S& B, const KeyPosInfo_S& P);

    private:
        /* 人头检测 */
        Inference_NS::CYolov5* m_pHInference = nullptr;
        /* 人体关键点 */
        Inference_NS::CImageFeature* m_pBInference = nullptr;

        /* 人头检测模型处理数据限制 */
        int m_nHeadDetectLimitHeight  = 0;
        int m_nHeadDetectLimitWidth   = 0;
        int m_nHeadDetectLimitChannel = 0;

        /* 人体关键点提取模型处理数据限制 */
        int m_nFastPoseLimitHeight   = 0;
        int m_nFastPoseLimitWidth    = 0;
        int m_nFastPosetLimitChannel = 0;

        /* yolo的阈值 */
        float m_fBoxThreshold = 0.75;
        float m_fNmsThreshold = 0.25;

        /* 人体关键的提取图片宽高 */
        int m_nWidth = 192;
        int m_nHeight = 256;

        /* 初始化参数 */
        InParam_S m_stInParam;
    };

}    // namespace StudentBehavior_NS
