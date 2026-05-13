/*
 * @FilePath     : BehaviorAnalyzer.hpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-08 15:11:10
 * @LastEditors  : yanzeh yanzeh@kfb.cn
 * @LastEditTime : 2024-01-23 14:33:52
 * @Description  : 行为分析模块使用类
 */
#pragma once

#include "BehaviorAnalyzerExtern.hpp"
#include "StuBehaviorAnalyzerV1.hpp"
#include "StuBehaviorAnalyzerV2.hpp"

namespace BA_NS
{
    class CBehaviorAnalyzer
    {
    public:

        static CBehaviorAnalyzerBase* createBehaviorAnalyzer(BehaviorAnalyzerInParam_S stInfo)
        {
            switch (stInfo.stNeedParam.enType)
            {
                case BA_STU_V1:
                {
                    return new CStuBehaviorAnalyzerV1(stInfo);
                }
                case BA_STU_V2:
                {
                    return new CStuBehaviorAnalyzerV2(stInfo);
                }
                default:
                {
                    return nullptr;
                }
            }
        }
    };

}    // namespace BA_NS
