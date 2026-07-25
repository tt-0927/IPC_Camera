/*
 * @FilePath     : LicensePlateCognitionV2_0.hpp
 * @Author       : 廖尔涛 liaoet@kfb.cn
 * @Date         : 2024-10-09 13:59:59
 * @LastEditors: 梁浩尧 lianghaoyao@kfb.cn
 * @LastEditTime: 2025-11-21 14:39:15
 * @Description  : 
 */

#pragma once

#include <unordered_map>


#include "LicensePlateCognitionExt.hpp"
#include "YoloUltralytics.hpp"
#include "LicensePlateRec.hpp"

namespace LicensePlateCognition_NS
{
    class CLicensePlateCognitionV1_0
    {
    public:

        CLicensePlateCognitionV1_0(InParam_S stInParam);
        ~CLicensePlateCognitionV1_0();

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
        bool process(InData_S stInData, std::vector<Result_S>& vecResult);

    private:
        /**
         * @brief 等比例缩放图片
         * @param [CVData_S] inputImage: 传入的图片数据
         * @param [char*&] pchOutData: 输出的缩放后的图片
         * @return [*]
         * @note
         */
        bool resizeAndPadImage(cv::Mat inputImage, cv::Mat& outputImage);
    
    private:
        /**
         * @brief 提取车牌
         * @param [cv::Point] topLeft: 车牌的目标框左上角坐标
         * @param [cv::Point] bottomRight: 车牌的目标框右下角坐标
         * @param [cv::Mat] inImage: 输入原始图片
         * @param [cv::Mat&] outImage: 输出的提取的车牌图片
         * @return [*]
         * @note
         */
        bool licensePlateExtraction(cv::Point topLeft, cv::Point bottomRight, cv::Mat inImage, cv::Mat& outImage);
    
    private:
        /**
         * @brief 双层车牌分割
         * @param [cv::Mat] inImage: 输入原始图片
         * @param [cv::Mat&] outImage: 输出的提取的车牌图片
         * @return [*]
         * @note
         */
        bool getSplitMerge(cv::Mat inImage, cv::Mat& outImage);


    private:

        /* 初始化参数 */
        InParam_S m_stInParam;

        Inference_NS::CYoloUltralytics* m_pLicensePlateDetect = nullptr;
        Inference_NS::CLicensePlateRec* m_pLicensePlateRec = nullptr;


        // std::vector<std::string> m_vLicensePlateTypes = {"单层车牌", "双层车牌"};
        std::vector<std::string> m_vLicensePlateTypes = {"Single", "Double"};
        /* 车牌检测算法输入参数限制 */
        int m_nLimitHeight  = 0;
        int m_nLimitWidth   = 0;
        int m_nLimitChannel = 0;

        /* 车牌识别算法输入参数限制 */
        int m_nRecLimitHeight  = 0;
        int m_nRecLimitWidth   = 0;
        int m_nRecLimitChannel = 0;

        /* 缩放填充后左上角的坐标 */
        int   m_nXOffset     = 0;
        int   m_nYOffset     = 0;
        /* 缩放比例 */
        float m_fResizeScale = 0.0;
        /* 提取车牌的参数 */
        // std::vector<cv::Point2f> m_vdstPoints = {cv::Point2f(0.0f, 0.0f),
                                                //  cv::Point2f(168.0f, 0.0f),
                                                //  cv::Point2f(168.0f, 48.0f),
                                                //  cv::Point2f(0.0f, 48.0f)};


    };

}    // namespace LicensePlateCognition_NS
