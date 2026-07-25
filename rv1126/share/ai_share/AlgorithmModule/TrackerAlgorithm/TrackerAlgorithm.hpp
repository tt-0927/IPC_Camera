/*
 * @FilePath     : TrackerAlgorithm.hpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-08 15:11:10
 * @LastEditors  : yanzeh yanzeh@kfb.cn
 * @LastEditTime : 2024-01-18 15:50:40
 * @Description  : 跟踪模块使用类
 */
#pragma once

#include "TrackerAlgorithmV1.hpp"

namespace TA_NS
{
    class CTrackerAlgorithm
    {
    public:

        static CTrackerAlgorithmBase* createTrackerAlgorithm(TrackerAlgorithmInParam_S stInfo)
        {
            switch (stInfo.stNeedParam.enType)
            {
                case TA_V1:
                {
                    return new CTrackerAlgorithmV1(stInfo);
                }
                default:
                {
                    return nullptr;
                }
            }
        }
    };

}    // namespace TA_NS
