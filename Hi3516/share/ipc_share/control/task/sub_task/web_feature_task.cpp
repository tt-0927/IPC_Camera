/**
 * @FilePath     : web_feature_task.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-10-28 15:00:00
 * @Description  : 网页功能可见性配置任务
 */

#include "web_feature_task.h"
#include "web_feature_convert.h"
#include "convert_interface.h"

void Task::WebFeature::GetFeatureConfig::handle()
{
    ::WebFeature::FeatureConfig_S stConfig;
    result(Convert::to_string(stConfig));
}
