/*
 * @FilePath     : ShipAnalyzer.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-03-14 18:40:52
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-03-14 19:30:34
 * @Description  : 使用船只分析模块接口
 */

#pragma once

#include "ShipAnalyzerExtern.hpp"
#include "ShipRouteAnalyzer.hpp"

namespace ShipAnalyzer_NS
{
    class CShipAnalyzer
    {
    public:

        static CShipAnalyzerBase* createShipAnalyzer(InParam_S stInfo)
        {
            switch (stInfo.stNeedParam.enType)
            {
                case SA_LINE_V1:
                {
                    return new CShipRouteAnalyzer(stInfo);
                }
                default:
                {
                    return nullptr;
                }
            }
        }
    };

}    // namespace ShipAnalyzer_NS