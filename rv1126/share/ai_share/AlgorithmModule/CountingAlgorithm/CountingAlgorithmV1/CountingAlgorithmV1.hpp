/*
 * @FilePath     : CountingAlgorithmV1.hpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-08 15:13:45
 * @LastEditors  : yanzeh yanzeh@kfb.cn
 * @LastEditTime : 2024-01-17 21:03:24
 * @Description  : 第一代人数统计模块
 */
#pragma once

#include "CountingAlgorithmBase.hpp"

namespace CA_NS
{
    class CCountingAlgorithmV1 : public CCountingAlgorithmBase
    {
    public:

        CCountingAlgorithmV1(CountingAnalyzerInParam_S stInParam);
        ~CCountingAlgorithmV1();

        /**
         * @brief 分析数据
         * @param [MediaDataInfo_S] stMediaDataInfo: 媒体数据信息
         * @param [CountingAnalyzerResult_S&] stOutInfo: 分析后的结果信息
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note 如果设置了stMediaDataInfo.freeDataFunc; 内部使用完会自动释放stMediaDataInfo.pchData。
         */
        BlError_E dataAnalysis(
            MediaDataInfo_S           stMediaDataInfo,
            CountingAnalyzerResult_S& stOutInfo);


    private:

        /* 算法实例 */
        void* m_pAlgorithm = nullptr;
    };

}    // namespace CA_NS
