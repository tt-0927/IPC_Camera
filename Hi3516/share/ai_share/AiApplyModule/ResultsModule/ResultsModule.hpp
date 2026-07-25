/*
 * @FilePath     : ResultsModule.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-03-28 09:21:26
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-04-01 09:17:39
 * @Description  :
 */
#pragma once

#include "ResultsSaveJson.hpp"

namespace ResultsModule_NS
{
    class CResultsModule
    {
    public:

        static CResultsBase* createModule(InParam_S stInfo)
        {
            switch (stInfo.stNeedParam.enType)
            {
                case SAVE_JSON:
                {
                    return new CResultsSaveJson(stInfo);
                }
                default:
                {
                    return nullptr;
                }
            }
        }

        static void releaseModule(CResultsBase*& pResultsBase)
        {
            if (pResultsBase)
            {
                delete pResultsBase;
                pResultsBase = nullptr;
            }
        }
    };



}    // namespace ResultsModule_NS