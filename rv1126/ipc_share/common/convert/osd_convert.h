/***
 * @FilePath     : osd_convert.h
 * @Author       : huangjunda
 * @Date         : 2025-05-27 11:13:05
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-05-27 11:13:06
 * @Description  : OSD配置转换处理
 */

#pragma once

#include "Json.h"
#include "osd_define.h"

namespace Convert
{
    void deal(Json::Object *pRootJson, Osd::CoordinateInfo_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, std::vector<Osd::CoordinateInfo_S> &vecInfo, bool bOutStruct);
    
    void deal(Json::Object *pRootJson, Osd::Info_S &stInfo, bool bOutStruct);

    void deal(Json::Object *pRootJson, Osd::Overplay_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Osd::OverplayInfo_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, std::vector<Osd::OverplayInfo_S> &vecInfo, bool bOutStruct);

    void deal(Json::Object *pRootJson, Osd::Cover_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Osd::CoverInfo_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, std::vector<Osd::CoverInfo_S> &vecInfo, bool bOutStruct);

    void deal(Json::Object *pRootJson, Osd::OsdAttribute_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Osd::OsdNameInfo_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Osd::OsdTimeInfo_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Osd::OsdInfo_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, std::vector<Osd::OsdInfo_S> &vecInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Osd::OsdConfig_S &stInfo, bool bOutStruct);

    void deal(Json::Object *pRootJson, Osd::CoverAttribute_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, Osd::CoverConfig_S &stInfo, bool bOutStruct);
}
