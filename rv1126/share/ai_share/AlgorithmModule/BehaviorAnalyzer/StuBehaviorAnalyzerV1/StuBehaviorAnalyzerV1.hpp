/*
 * @FilePath     : StuBehaviorAnalyzerV1.hpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-08 15:13:45
 * @LastEditors  : yanzeh yanzeh@kfb.cn
 * @LastEditTime : 2024-01-23 14:21:47
 * @Description  : 学生分析第一代行为分析模块
 */
#pragma once

#include "BehaviorAnalyzerBase.hpp"

namespace BA_NS
{
    class CStuBehaviorAnalyzerV1 : public CBehaviorAnalyzerBase
    {
    public:

        CStuBehaviorAnalyzerV1(BehaviorAnalyzerInParam_S stInParam);
        ~CStuBehaviorAnalyzerV1();

        /**
         * @brief 分析数据
         * @param [MediaDataInfo_S&] stMediaDataInfo: 媒体数据信息
         * @param [std::list<BehaviorAnalyzerResult_S>&] listOutInfo: 分析后的结果信息
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note 如果设置了stMediaDataInfo.freeDataFunc; 内部使用完会自动释放stMediaDataInfo.pchData。
         */
        BlError_E dataAnalysis(MediaDataInfo_S&                     stMediaDataInfo,
                               std::list<BehaviorAnalyzerResult_S>& listOutInfo);

    private:

        /* 算法实例 */
        void* m_pAlgorithm = nullptr;
    };

}    // namespace BA_NS
