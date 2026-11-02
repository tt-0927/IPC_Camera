/**
 * @FilePath     : web_feature_define.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-10-28 15:00:00
 * @Description  : 网页功能可见性配置定义
 */

#pragma once

namespace WebFeature
{
/* 网页功能可见性配置（编译期由产品版本宏决定，不落盘：值必须始终跟随固件版本，
   落盘会在跨版本刷机时被旧文件值盖住编译宏） */
typedef struct
{
#if LANG_OVERSEAS_EN
    bool bEnableGb28181 = false; /* GB28181 平台接入（网页"平台接入"页） */
    bool bEnableGb35114 = false; /* GB35114 国标证书（网页"国际证书管理"页） */
#else
    bool bEnableGb28181 = true; /* GB28181 平台接入（网页"平台接入"页） */
    bool bEnableGb35114 = true; /* GB35114 国标证书（网页"国际证书管理"页） */
#endif
} FeatureConfig_S;
}
