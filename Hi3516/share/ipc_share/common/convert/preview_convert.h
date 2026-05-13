/*** 
 * @FilePath     : preview_convert.h
 * @Author       : 严泽辉 (yanzeh@kfb.cn)
 * @Date         : 2024-10-07 17:30:52
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-07-16 14:23:03
 * @Description  : 预览定义数据的转换
 */

#pragma once

#include "Json.h"
#include "preview_define.h"

namespace Convert
{
    void deal(Json::Object *pRootJson, Preview::RtspUrl_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Preview::ImageParam_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Preview::PreviewInfo_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Preview::CollectAudioInfo_S&stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Preview::IntercomInfo_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Preview::BroadcastInfo_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Preview::BeepAlarm_S &stInfo, bool bOutStruct);

} // namespace Convert
