/*
 * @FilePath     : ClassroomMoveDetectV1_0.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-09-29 13:59:59
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-09-29 14:16:56
 * @Description  : 
 */

#pragma once

#include <unordered_map>

#include "ClassroomMoveDetectExt.hpp"
#include "HeadDetect.hpp"

namespace ClassroomMoveDetect_NS
{
    class CClassroomMoveDetectV1_0
    {
    public:

        CClassroomMoveDetectV1_0(InParam_S stInParam);
        ~CClassroomMoveDetectV1_0();

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
         * @param [cv::Mat] inMat: 传入的视频数据
         * @param [AnalyseParam_S] stParam: 分析的参数
         * @param [std::vector<Result_S>&] vecResult: 输出的处理结果
         * @return [*]
         * @note
         */
        bool process(InData_S stInData, OutData_S& stOutData);



    private:

        /* 行人数据 */
        typedef struct _Penson_
        {
            int       nId;        /* 人的ID */
            cv::Point startPoint; /* 起始坐标点 */
            cv::Point curPoint;   /* 当前坐标点 */
            int       ndwellTime; /* 放弃跟踪时间 */
            bool      isUsed;     /* 当前数据是否使用 */
        } Penson_S;

        /**
         * @brief 等比例缩放图片
         * @param [CVData_S] inputImage: 传入的图片数据
         * @param [char*&] pchOutData: 输出的缩放后的图片
         * @return [*]
         * @note
         */
        bool resizeAndPadImage(cv::Mat inputImage, cv::Mat& outputImage);

        /**
         * @brief 等比例缩放图片2
         * @param [CVData_S] inputImage: 传入的图片数据
         * @param [char*&] pchOutData: 输出的缩放后的图片
         * @return [*]
         * @note
         */
        bool resizeAndPadImage2(cv::Mat inputImage, cv::Mat& outputImage);

        /**
         * @brief 计算两个目标框的IOU
         * @param [std::vector<int>] box1: 目标框1
         * @param [std::vector<int>] box2: 目标框2
         * @return [*]
         * @note
         */
        double CalculateOverlap(const std::vector<int>& box1, const std::vector<int>& box2);
        
        /**
         * @brief 计算相邻两帧的人头移动比例
         * @param [std::vector<std::vector<int>>] boxes1: 上一帧目标框集合
         * @param [std::vector<std::vector<int>>] boxes2: 当前帧目标框集合
         * @return [*]
         * @note
         */
        double iou_filter(std::vector<std::vector<int>>& boxes1, std::vector<std::vector<int>>& boxes2, const double IouFilterThreshold);



    private:

        /* 初始化参数 */
        InParam_S m_stInParam;

        Inference_NS::CHeadDetect* m_pHeadDetect = nullptr;

        /* 算法输入参数限制 */
        int m_nLimitHeight  = 0;
        int m_nLimitWidth   = 0;
        int m_nLimitChannel = 0;

        /* 缩放填充后左上角的坐标 */
        int   m_nXOffset     = 0;
        int   m_nYOffset     = 0;
        /* 缩放比例 */
        float m_fResizeScale = 1.0;

        /* 计算移动比例的参数 */
        float m_fIouFilterThreshold = 0.8;
        /* 存放上一帧的目标框 */
        std::vector<std::vector<int>> v_nForeboxs;

    };

}    // namespace ClassroomMoveDetect_NS
