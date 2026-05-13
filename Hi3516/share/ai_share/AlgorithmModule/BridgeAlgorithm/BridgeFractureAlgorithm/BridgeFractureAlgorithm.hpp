/*
 * @FilePath     : BridgeFractureAlgorithm.hpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-08 15:13:45
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-03-14 15:27:02
 * @Description  : 桥梁裂缝检测
 */
#pragma once

#include "BridgeAlgorithmBase.hpp"

namespace BDGA_NS
{
    class CBridgeFractureAlgorithm : public CBridgeAlgorithmBase
    {
    public:

        CBridgeFractureAlgorithm(InParam_S stInParam);
        ~CBridgeFractureAlgorithm();

    protected:

        /**
         * @brief 分析数据
         * @param [MediaDataInfo_S] stMediaDataInfo: 媒体数据信息
         * @param [AnalyzerResult_S&] stOutInfo: 分析后的结果信息
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note 如果设置了stMediaDataInfo.freeDataFunc; 内部使用完会自动释放stMediaDataInfo.pchData。
         */
        BlError_E dataAnalysis(MediaDataInfo_S&  stMediaDataInfo,
                               AnalyzerResult_S& stOutInfo);


    private:

        /* 算法实例 */
        void* m_pAlgorithm = nullptr;
    };

}    // namespace BDGA_NS
