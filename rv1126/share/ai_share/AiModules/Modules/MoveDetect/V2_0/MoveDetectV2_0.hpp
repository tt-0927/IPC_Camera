#pragma once

#include "MoveDetectExt.hpp"
#include "MoveDetect.hpp"

namespace MoveDetect_NS
{
    class CMoveDetectV2_0
    {
    public:

        CMoveDetectV2_0(InParam_S stInParam);
        ~CMoveDetectV2_0();

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
         * @param [Result_S&] vecResult: 输出的处理结果
         * @return [*]
         * @note
         */
        bool process(InData_S stInData,  std::vector<std::vector<int>> &vecResult);

        void set_resolution(int nWith, int nHeight)
        {
            m_nLimitHeight = nHeight;
            m_nLimitWidth  = nWith;
        }

        /**
         * @brief 获取算法限制尺寸宽
         * @return [m_nLimitWidth]
         * @note
         */
        int getLimitWidth() const 
        { return m_nLimitWidth; }

        /**
         * @brief 获取算法限制尺寸高
         * @return [m_nLimitHeight]
         * @note
         */
        int getLimitHeight() const 
        { return m_nLimitHeight; }

    public:

        int erode_size = 4;  /* 默认敏感度阈值：阈值越小，变化检测越敏感 */
        int dilate_size = 40; /* 默认框融合阈值：阈值越大，相距越远的变化融合在一起 */
        int nSkipFrame = 1; /* 默认检测间隔，每N帧检测一次，检测对象为周期起始帧与周期结束帧 */  
        cv::Mat LastFrameCache; /* 上一个检测帧的缓存 */

    private:

        /* 初始化参数 */
        InParam_S m_stInParam;
        Inference_NS::CMoveDetect* m_pMoveDetect = nullptr;

        /* 算法输入参数限制 */
        int m_nLimitHeight  = 720;
        int m_nLimitWidth   = 1280;
        int m_nLimitChannel = 3;
    };

}    // namespace MoveDetect_NS
