/**
 * @FilePath     : log_convert.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-16 17:00:39
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-16 17:04:45
 * @Description  : 日志信息转换
 */

#pragma once
#include <vector>
#include "log_define.h"

#include "Json.h"
namespace Convert
{
    void deal(Json::Object *pRootJson, Log::Info_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, std::vector<Log::Info_S> &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Log::RetrievalCond_S &stInfo, bool bOutStruct);
}