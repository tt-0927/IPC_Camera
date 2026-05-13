/*
 * @FilePath     : ChannelAlgorithm.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-03-08 10:36:45
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-03-08 11:25:26
 * @Description  : 航道分析模块使用类
 */
#pragma once

#include "ChannelAlgorithmV1.hpp"

namespace ChannelAlgorithm_NS
{
    class CChannelAlgorithm
    {
    public:

        static CChannelAlgorithmBase* createChannelAlgorithm(InParam_S stInfo)
        {
            switch (stInfo.stNeedParam.enType)
            {
                case BDGA_CE:
                {
                    return new CChannelAlgorithmV1(stInfo);
                }
                default:
                {
                    return nullptr;
                }
            }
        }
    };

}    // namespace ChannelAlgorithm_NS
