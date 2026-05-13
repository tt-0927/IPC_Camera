/*
 * @FilePath     : FaceExpressV1_0.hpp
 * @Author       : 吴才朋 wucp@kfb.cn
 * @Date         : 2024-08-01 16:29:52
 * @LastEditors: 李辉 lihui@kfb.cn
 * @LastEditTime: 2024-09-03 10:41:28
 * @Description  : 表情识别
 */
#pragma once

#include "CVInferenceBase.hpp"
#include "ScenarioBase.hpp"

namespace Scenario_NS
{
    class CFaceExpressV1_0 : public CScenarioBase
    {
    public:

        CFaceExpressV1_0(AiScenario_NS::InParam_S stInParam);
        ~CFaceExpressV1_0();

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
         * @param [CVData_S] stInData: 传入的图片数据
         * @param [char*&] pchOutData: 输出的处理结果
         * @param [int&] nDataSize: 数据大小
         * @return [*]
         * @note
         */
        bool process(AiScenario_NS::CVData_S stInData, char*& pchOutData, int& nDataSize);

        /**
         * @brief 处理数据
         * @param [CAData_S] stInData: 传入的音频数据
         * @param [char*&] pchOutData: 输出的处理结果
         * @param [int&] nDataSize: 数据大小
         * @return [*]
         * @note
         */
        bool process(AiScenario_NS::CAData_S stInData, char*& pchOutData, int& nDataSize);


        /**
         * @brief 释放处理结果
         * @param [char*&] pchOutData: 处理结果指针
         * @return [*]
         * @note
         */
        bool releaseData(char*& pchOutData);

    private:
        /**
         * @brief json输出数据
         * @param [std::vector<float>] vecBoxPos: 人脸框图数据
         * @param [std::vector<float>] vecFeaturePos: 人脸特征数据
         * @param [char**] pchOutData: 输出数据
         * @return [*]
         * @note
         */
        bool convertToJson(std::vector<float> vecBoxPos, std::vector<float> vecFeaturePos, char** pchOutData, int& nDataSize);

        /**
         * @brief 人脸矫正
         * @param [cv::Mat &] aFace: 人脸数据
         * @param [float] fX: X
         * @param [float] fY: Y
         * @return [*]
         * @note
         */
        void faceAlignment(cv::Mat &aFace,float fX,float fY);

    private:

        /* 人脸检测算法 */
        InferenceV1_0_NS::CCVInferenceBase* m_pFaceDetectInference = nullptr;
        /* 表情识别算法 */
        InferenceV1_0_NS::CCVInferenceBase* m_pFaceExpressInference = nullptr;
        /* 模型处理数据限制 */
        int m_nFaceDetectLimitHeight  = 0;
        int m_nFaceDetectLimitWidth   = 0;
        int m_nFaceDetectLimitChannel = 0;

        int m_nFaceExpressLimitHeight  = 0;
        int m_nFaceExpressLimitWidth   = 0;
        int m_nFaceExpressLimitChannel = 0;
        
    };

}   // namespace Scenario_NS
