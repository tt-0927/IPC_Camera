/**
 * @file FaceLandmarkV2_0.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-11-29
 *
 * @brief
 */
#pragma once

#include <unordered_map>
#include "Scrfd.hpp"
#include "FaceLandmark1000.hpp"
#include "FaceLandmarkExt.hpp"

namespace FaceLandmark_NS
{
    class CFaceLandmarkV2_0
    {
    public:
        CFaceLandmarkV2_0(InParam_S stInParam);
        ~CFaceLandmarkV2_0();

        /**
         * @brief 初始化
         * @return true
         * @return false
         */
        bool init();

        /**
         * @brief 反初始化
         * @return true
         * @return false
         */
        bool unInit();

        /**
         * @brief 处理数据
         * @param stInData 传入的视频数据
         * @param stOutData 分析后的结果
         * @return true
         * @return false
         */
        bool process(InData_S stInData, 
            std::vector<OutData_S>& stOutData
        );

    private:
        /**
         * @brief 等比例缩放图片
         * @param [CVData_S] inputImage: 传入的图片数据
         * @param [int] nLimitWidth:  缩放宽
         * @param [int] nLimitHeight: 缩放高
         * @param [char*&] pchOutData: 输出的缩放后的图片
         * @return [*]
         * @note
         */
        bool resizeAndPadImage(
                cv::Mat inputImage,
                int nLimitWidth,
                int nLimitHeight,
                int &nXOffset,
                int &nYOffset,
                float &fResizeScale,
                cv::Mat &outputImage
        );

    private:
        /* 初始化参数 */
        InParam_S m_stInParam;

        /* 人脸检测算法输入参数限制 */
        Inference_NS::CScrfd *m_pFaceDetect = nullptr;

        int m_nDetectLimitHeight = 0;
        int m_nDetectLimitWidth = 0;
        int m_nDetectLimitChannel = 0;
        /* 缩放填充后左上角的坐标、缩放比例 */
        int m_nDetectXOffset = 0;
        int m_nDetectYOffset = 0;
        float m_fDetectResizeScale = 1.0;

        /* 人脸特征点提取算法输入参数限制 */
        Inference_NS::CFaceLandmark1000 *m_pFaceLandmark = nullptr;
        int m_nLandmarkLimitHeight = 0;
        int m_nLandmarkLimitWidth = 0;
        int m_nLandmarkLimitChannel = 0;
        /* 缩放填充后左上角的坐标、缩放比例 */
        int m_nLandmarkXOffset = 0;
        int m_nLandmarkYOffset = 0;
        float m_fLandmarkResizeScale = 1.0;
    };

} // namespace FaceLandmark_NS
