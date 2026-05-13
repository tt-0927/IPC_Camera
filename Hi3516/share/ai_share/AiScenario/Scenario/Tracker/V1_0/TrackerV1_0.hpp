/*
 * @FilePath     : TrackerV1_0.hpp
 * @Author       : lih lih@kfb.cn
 * @Date         : 2024-05-31 15:31:40
 * @LastEditors: 李辉 lihui@kfb.cn
 * @LastEditTime: 2024-09-03 10:40:51
 * @Description  :
 */
#pragma once

#include "CVInferenceBase.hpp"
#include "ScenarioBase.hpp"

namespace Scenario_NS
{
    struct PRIORITYFEATUES
    {
        /* 优先级，0-99（0最大） */
        int Priority;
        /* 人体提取特征 */
        float* vPriorityFeatures;
    };

    struct BoundingBox 
    {
        float fX;
        float fY;
        float fW;
        float fH;
    };

    class CTrakcerV1_0 : public CScenarioBase
    {
    public:

        CTrakcerV1_0(AiScenario_NS::InParam_S stInParam);
        ~CTrakcerV1_0();

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

        bool convertToJson(std::vector<float> vTargetXY, std::vector<float> vAllXY, char** pchOutData, int& nDataSize);

        /**
         * @brief 判断框选坐标是否存在人
         * @param [std::vector<float>] vResultPoints: 框选信息
         * @param [std::vector<float>] vSelectPoints: 人头检测出来的数据
         * @return [*]
         * @note
         */
        bool peopleChoose(std::vector<float> vResultPoints, std::vector<float> vSelectPoints);

        /**
         * @brief 更新滑动窗口的信息
         * @param [cv::Mat] aFrame: 图片信息
         * @param [std::vector<float>] vPoints: 数据
         * @param [std::vector<float>] vNewPoints: 输出数据
         * @return [*]
         * @note
         */
        void nextImg(cv::Mat aFrame, std::vector<float> vPoints, std::vector<float> &vNewPoints);

        /**
         * @brief 余弦相识度
         * @param [const float *] vec1: 数据一
         * @param [const float *] vec2: 数据二
         * @param [int] size: 大小
         * @return [*]
         * @note
         */
        float cosineSimilarity(const float *vec1, const float *vec2, int size);

        /**
         * @brief 
         * @param [cv::Mat &] image: 图片数据
         * @param [int] nTargetWidth: 模板宽度
         * @param [int] nTargetHeight: 目标高度
         * @return [*]
         * @note
         */
        void resizeAndPad(cv::Mat &image, int nTargetWidth, int nTargetHeight);

        /**
         * @brief 清空保存的特征点
         * @return [*]
         * @note
         */
        void clearFeatures();

    private:
        /* 人头检测算法 */
        InferenceV1_0_NS::CCVInferenceBase* m_pHCInference = nullptr;
        /* 跟踪算法 */
        InferenceV1_0_NS::CCVInferenceBase* m_pTInference = nullptr;

        /* 框选框 */
        std::vector<float> m_vTrackerPoints;

        /* 优先级人的特征链表（包含人脸识别+页面点击的） */
		std::vector<PRIORITYFEATUES> m_vPriorityDatas;

        /* 用于“多少帧识别不到自动删除被跟踪人的特征”的计数 */
		int m_nChangeNum = 0;

        /* 缓存多少帧特征 */
		int m_nNumFeatures = 2;

        /* 人体特征缓存人 -- 用于第一次特征提取*/
		bool m_bFlag = true;
 		float* m_pMyFeatures = new float[512];

        /* 上一帧的位置信息 */
		BoundingBox lastBox;

        int m_nIndexFeature = 0;

        std::vector<float> m_vNewPoints;
        /* 连续5帧的特征信息 */
		std::vector<float*> m_vFeatures;

        /* 相识度阈值 */
		float m_fSimilarityThreshold = 0.9;
		
		/* 是否启动人头框选功能 */
		bool bChoose = false;

        /*上一次是不是一个人的标志位*/
		int m_nOneFlg = 0;

        /* 模型处理数据限制 */
        int m_nHeadDetecttLimitHeight  = 0;
        int m_nHeadDetectLimitWidth   = 0;
        int m_nHeadDetectLimitChannel = 0;

        int m_nBodyFeatureLimitHeight  = 0;
        int m_nBodyFeatureLimitWidth   = 0;
        int m_nBodyFeatureLimitChannel = 0;

        /* yolo的阈值 */
        float m_fBoxThreshold = 0.75;
        float m_fNmsThreshold = 0.25;
        
    };

}    // namespace Scenario_NS
