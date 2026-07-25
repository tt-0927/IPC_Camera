
#pragma once

#include "DevManageExtern.hpp"
#include "JsonInterfase.h"

namespace Ai0630_NS
{
    /* 设备管理 */
    void dealJson(Json::Object* pRootJson, GetReqDevInfo_S& stInfo, bool bOutStruct);
    void dealJson(Json::Object* pRootJson, AddDevInfo_S& stInfo, bool bOutStruct);
    void dealJson(Json::Object* pRootJson, EditDevInfo_S& stInfo, bool bOutStruct);
    void dealJson(Json::Object* pRootJson, DelDevInfo_S& stInfo, bool bOutStruct);
    void dealJson(Json::Object* pRootJson, DevDataInfo_S& stInfo, bool bOutStruct);

}    // namespace Ai0630_NS