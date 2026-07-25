/*
 * @FilePath     : ParseData.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-04-01 15:12:15
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-04-01 15:20:00
 * @Description  :
 */
#pragma once

#include "ParseJson.hpp"

namespace ParseData_NS
{
    class CParseData
    {
    public:

        static CParseBase* create(InParam_S stInfo)
        {
            switch (stInfo.stNeedParam.enType)
            {
                case PARSE_JSON:
                {
                    return new CParseJson(stInfo);
                }
                default:
                {
                    return nullptr;
                }
            }
        }

        static void release(CParseBase*& pBase)
        {
            if (pBase)
            {
                delete pBase;
                pBase = nullptr;
            }
        }
    };

}    // namespace ParseData_NS