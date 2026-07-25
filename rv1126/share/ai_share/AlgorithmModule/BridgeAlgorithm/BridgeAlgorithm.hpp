/*
 * @FilePath     : BridgeAlgorithm.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-03-08 10:36:45
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-03-08 11:25:26
 * @Description  : 桥梁分析模块使用类
 */
#pragma once

#include "BridgeCollapseAlgorithm.hpp"
#include "BridgeFractureAlgorithm.hpp"

namespace BDGA_NS
{
    class CBridgeAlgorithm
    {
    public:

        static CBridgeAlgorithmBase* createBridgeAlgorithm(InParam_S stInfo)
        {
            switch (stInfo.stNeedParam.enType)
            {
                case BDGA_CE:
                {
                    return new CBridgeCollapseAlgorithm(stInfo);
                }
                case BDGA_FE:
                {
                    return new CBridgeFractureAlgorithm(stInfo);
                }
                default:
                {
                    return nullptr;
                }
            }
        }
    };

}    // namespace BDGA_NS
