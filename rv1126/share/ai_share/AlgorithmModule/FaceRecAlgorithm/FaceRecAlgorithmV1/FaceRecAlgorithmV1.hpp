/*
 * @FilePath     : FaceRecAlgorithmV1.hpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-08 15:13:45
 * @LastEditors  : yanzeh yanzeh@kfb.cn
 * @LastEditTime : 2024-01-18 18:06:27
 * @Description  : 第一代人脸识别模块
 */
#pragma once

#include "FaceRecAlgorithmBase.hpp"

namespace FR_NS
{
    class CFaceRecAlgorithmV1 : public CFaceRecAlgorithmBase
    {
    public:

        CFaceRecAlgorithmV1(FaceRecInParam_S stInParam);
        ~CFaceRecAlgorithmV1();

        /**
         * @brief 获取图片中的人脸特征信息
         * @param [MediaDataInfo_S] stMediaDataInfo: 媒体数据信息
         * @param [std::list<std::vector<float>>&] listOutData: 解析到的人脸链表
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note 不会释放stMediaDataInfo空间。
         */
        BlError_E get_facialFeatures(MediaDataInfo_S stMediaDataInfo, std::list<std::vector<float>>& listOutData);

        /**
         * @brief 获取图片中的人脸特征信息
         * @param [cv::Mat] inMat: 图片数据
         * @param [std::list<std::vector<float>>&] listOutData: 解析到的人脸链表
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note 不会释放stMediaDataInfo空间。
         */
        BlError_E get_facialFeatures(cv::Mat inMat, std::list<std::vector<float>>& listOutData);

        /**
         * @brief 分析数据
         * @param [MediaDataInfo_S] stMediaDataInfo: 媒体数据信息
         * @param [std::list<FaceRecognitionResult_S>&] listOutInfo: 分析后的结果信息
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note 如果设置了stMediaDataInfo.freeDataFunc; 内部使用完会自动释放stMediaDataInfo.pchData。
         */
        BlError_E dataAnalysis(MediaDataInfo_S stMediaDataInfo, std::list<FaceRecognitionResult_S>& listOutInfo);

    private:

        /* 算法实例 */
        void* m_pRetinafaceFacenet = nullptr;
        void* m_pFaceDetect        = nullptr;
        void* m_pFeatureNet        = nullptr;
    };

}    // namespace FR_NS