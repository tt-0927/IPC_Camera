/*
 * @FilePath     : TrackerAlgorithmV1.hpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-08 15:13:45
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-02-23 17:49:13
 * @Description  : 第一代跟踪模块
 */
#pragma once

#include "TrackerAlgorithmBase.hpp"

namespace TA_NS
{
    class CTrackerAlgorithmV1 : public CTrackerAlgorithmBase
    {
    public:

        CTrackerAlgorithmV1(TrackerAlgorithmInParam_S stInParam);
        ~CTrackerAlgorithmV1();

        /**
         * @brief 分析数据
         * @param [MediaDataInfo_S] stMediaDataInfo: 媒体数据信息
         * @param [CountingAnalyzerResult_S&] stOutInfo: 分析后的结果信息
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note 不会释放stMediaDataInfo空间。
         */
        BlError_E dataAnalysis(
            MediaDataInfo_S           stMediaDataInfo,
            TrackerAlgorithmResult_S& stOutInfo);

    private:
        /* 算法实例 */
        void* m_pAlgorithm = nullptr;
    };

}    // namespace TA_NS
