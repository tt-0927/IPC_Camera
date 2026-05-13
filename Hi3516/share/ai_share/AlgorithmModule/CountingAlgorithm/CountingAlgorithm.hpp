/*
 * @FilePath     : CountingAlgorithm.hpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-08 15:11:10
 * @LastEditors  : yanzeh yanzeh@kfb.cn
 * @LastEditTime : 2024-01-23 15:04:54
 * @Description  : 人数统计模块使用类
 */
#pragma once

#include "CountingAlgorithmV1.hpp"
#include "CountingAlgorithmV2.hpp"
#include "CountingExtern.hpp"

namespace CA_NS
{
    class CCountingAlgorithm
    {
    public:

        static CCountingAlgorithmBase* createCountingAlgorithm(CountingAnalyzerInParam_S stInfo)
        {
            switch (stInfo.stNeedParam.enType)
            {
                case CA_V1:
                {
                    return new CCountingAlgorithmV1(stInfo);
                }
                case CA_V2:
                {
                    return new CCountingAlgorithmV2(stInfo);
                }
                default:
                {
                    return nullptr;
                }
            }
        }
    };

}    // namespace CA_NS
