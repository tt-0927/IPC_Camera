/*
 * @FilePath     : ShipRouteAnalyzer.hpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-08 15:13:45
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-03-14 19:18:16
 * @Description  : 船只航线检测
 */
#pragma once

#include "ShipAnalyzerBase.hpp"

namespace ShipAnalyzer_NS
{
    class CShipRouteAnalyzer : public CShipAnalyzerBase
    {
    public:

        CShipRouteAnalyzer(InParam_S stInParam);
        ~CShipRouteAnalyzer();

    protected:

        /**
         * @brief 分析数据
         * @param [MediaDataInfo_S] stMediaDataInfo: 媒体数据信息
         * @param [std::list<AnalyzerResult_S>&] listOutInfo: 分析后的结果信息
         * @return [*] 成功 >= BlError_E::OK   其他失败
         * @note 如果设置了stMediaDataInfo.freeDataFunc; 内部使用完会自动释放stMediaDataInfo.pchData。
         */
        BlError_E dataAnalysis(MediaDataInfo_S&             stMediaDataInfo,
                               std::list<AnalyzerResult_S>& listOutInfo);


    private:

        /* 算法实例 */
        void* m_pAlgorithm = nullptr;
    };

}    // namespace ShipAnalyzer_NS
