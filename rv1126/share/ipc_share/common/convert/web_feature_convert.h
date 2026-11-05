/**
 * @FilePath     : web_feature_convert.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-10-28 15:00:00
 * @Description  : 网页功能可见性配置转换
 */

#pragma once
#include "web_feature_define.h"

#include "Json.h"
namespace Convert
{
void deal(Json::Object *pRootJson, WebFeature::FeatureConfig_S &stConfig, bool bOutStruct);
}
