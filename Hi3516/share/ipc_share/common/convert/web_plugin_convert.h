/**
 * @FilePath     : web_plugin_convert.h
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2025-06-11 15:26:10
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-11 16:47:14
 * @Description  : PC端的插件配置参数转换
 */

#pragma once
#include "web_plugin_define.h"

#include "Json.h"
namespace Convert
{
    void deal(Json::Object *pRootJson, WebPlugin::StreamParam_S &stPermissions, bool bOutStruct);
    void deal(Json::Object *pRootJson, WebPlugin::FileSavePath_S &stFileSavePath, bool bOutStruct);
    void deal(Json::Object *pRootJson, WebPlugin::RecordParam_S &stRecordParam, bool bOutStruct);
    void deal(Json::Object *pRootJson, WebPlugin::Param_S &stParam, bool bOutStruct);
}