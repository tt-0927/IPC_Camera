/*
 * @FilePath     : AiPlatformFactory.hpp
 * @Author       : lih lih@kfb.cn
 * @Date         : 2024-05-06 15:38:33
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-05-06 19:12:09
 * @Description  :
 */
#pragma once

#include "PlatformManage.hpp"
#include "PlatformExtern.hpp"

namespace PlatformManage_NS
{

    class CAiPlatformFactory
    {
    public:

        static CAiPlatformBase* createClient(AiPlatformInParam_S stInfo)
        {
            switch (stInfo.stNeedParam.enType)
            {
                case AiPlatformType_E::ITC_PLATFORM:
                {
                    return new CPlatformManage(stInfo);
                }
                default:
                {
                    return nullptr;
                }
            }
        }
    };

}


