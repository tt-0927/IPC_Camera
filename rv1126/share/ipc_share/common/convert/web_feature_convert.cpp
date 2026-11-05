/**
 * @FilePath     : web_feature_convert.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-10-28 15:00:00
 * @Description  : 网页功能可见性配置转换
 */

#include "web_feature_convert.h"

#include "convert.h" /* 这个要放在UserDefineConvert的后面 */

void Convert::deal(Json::Object *pRootJson, WebFeature::FeatureConfig_S &stConfig, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "EnableGb28181", stConfig.bEnableGb28181);
    convert.field(pRootJson, "EnableGb35114", stConfig.bEnableGb35114);
}
