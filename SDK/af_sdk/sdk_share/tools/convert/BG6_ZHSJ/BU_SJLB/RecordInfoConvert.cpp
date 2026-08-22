/**
 * @file RecordInfoConvert.cpp
 * @author ITC
 * @date 2026-08-20
 * @LastEditors  : ITC
 * @LastEditTime : 2026-08-20
 *
 * @brief 录播部门（BU_SJLB）专用结构体 JSON 转换实现
 * 功能说明：
 * 1. 实现注册信息 NET_RegisterInfo_S 的结构体与 JSON 双向转换
 * 2. 对应命令码 NET_GET_REGISTERINFO(520) / NET_SET_REGISTERINFO(521)
 */

#include "RecordInfoConvert.h"
#include "SDKConvert.h"

#include <cstdio>
#include <cstdlib>

void SDKConvert::deal(Json::Object* pRootJson, NET_RegisterInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "MachinSn", stInfo.strMachinSn);
    convert.field(pRootJson, "RegisterEg", stInfo.strRegisterEg);
    convert.field(pRootJson, "StartTime", stInfo.strStartTime);
    convert.field(pRootJson, "UsableTime", stInfo.nUsableTimer);
    /* ActionTime 为枚举类型，Json::get/add 无枚举重载，需手动转换 */
    if (bOutStruct)
    {
        int nActionTime = 0;
        Json::get(pRootJson, "ActionTime", nActionTime);
        stInfo.enActionTime = (NET_ActivationTime_E)nActionTime;
    }
    else
    {
        Json::add(pRootJson, "ActionTime", (int)stInfo.enActionTime);
    }
}
