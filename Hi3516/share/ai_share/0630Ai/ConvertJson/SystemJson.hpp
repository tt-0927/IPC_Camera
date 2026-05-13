#pragma once

#include "Intern.hpp"
#include "JsonInterfase.h"
#include "PerfManageExtern.hpp"

namespace Ai0630_NS
{

    /* 登录 */
    void dealJson(Json::Object* pRootJson, LoginInfo_S& stInfo, bool bOutStruct);

    /* 同步时间 */
    void dealJson(Json::Object* pRootJson, AdjuestTime_S& stInfo, bool bOutStruct);

    /* 网络信息 */
    void dealJson(Json::Object* pRootJson, SysNetwork_S& stInfo, bool bOutStruct);

    /* 注册 */
    void dealJson(Json::Object* pRootJson, RegisterInfo_S& stInfo, bool bOutStruct);

    /* 系统信息 */
    void dealJson(Json::Object* pRootJson, SystemInfo_S& stInfo, bool bOutStruct);

    /* 系统状态 */
    void dealJson(Json::Object* pRootJson, CpuInfo_S& stInfo, bool bOutStruct);
    void dealJson(Json::Object* pRootJson, MemoryInfo_S& stInfo, bool bOutStruct);
    void dealJson(Json::Object* pRootJson, GpuInfo_S& stInfo, bool bOutStruct);
    void dealJson(Json::Object* pRootJson, GrapMemoryInfo_S& stInfo, bool bOutStruct);
    void dealJson(Json::Object* pRootJson, PerfInfo_S& stInfo, bool bOutStruct);

    /* 升级信息 */
    void dealJson(Json::Object* pRootJson, UpgradeInfo_S& stInfo, bool bOutStruct);

}    // namespace Ai0630_NS