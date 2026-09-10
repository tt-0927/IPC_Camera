/**
 * @FilePath     : log_convert.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-16 17:00:39
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-16 17:04:54
 * @Description  : 日志信息转换
 */

#include "log_convert.h"

#include "convert.h" /* 这个要放在XxxConvert的后面 */

void Convert::deal(Json::Object *pRootJson, Log::Info_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);

    convert.field(pRootJson, "StartTime", stInfo.startTime);
    convert.field(pRootJson, "Type", stInfo.nType);
    convert.field(pRootJson, "Action", stInfo.nAction);
    convert.field(pRootJson, "ChnName", stInfo.chnName);
    convert.field(pRootJson, "User", stInfo.user);
    convert.field(pRootJson, "Host", stInfo.host);
    convert.field(pRootJson, "Context", stInfo.context);
}
void Convert::deal(Json::Object *pRootJson, std::vector<Log::Info_S> &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "LogInfos", stInfo);
}

void Convert::deal(Json::Object *pRootJson, Log::RetrievalCond_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);

    convert.field(pRootJson, "Type", (int &)stInfo.enType);
    convert.field(pRootJson, "Action", (int &)stInfo.enAction);
    convert.field(pRootJson, "StartTime", stInfo.startTime);
    convert.field(pRootJson, "EndTime", stInfo.endTime);
}
