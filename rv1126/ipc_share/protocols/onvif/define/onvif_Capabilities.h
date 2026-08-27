/**
 * @file onvif_Capabilities.h
 * @author tianl (tianl@kfb.cn)
 * @date 2025-09-28
 * 
 * @brief onvif 能力集定义 
 */
#pragma once

//==========================	设备能力 Device Capabilities		==========================

//==========================	媒体能力 Media Capabilities			==========================
#define ONVIF_MEDIA_PROFILE_NUM     2       //媒体个数 

/* 视频质量 */
#define VIDEO_QUALITY_MAX 6
#define VIDEO_QUALITY_MIN 1

#define ONVIF_OSD_TYPE_NUM          4       //OSD 配置类型个数
#define ONVIF_OSD_MAX_NUM           6       //OSD 最大配置个数
#define ONVIF_OSD_MAX_NUM_STR       "9"     //OSD 最大配置个数
#define ONVIF_OSD_TEXT_NUM          4       //OSD 最大文本个数
#define ONVIF_OSD_TEXT_NUM_STR      "8"     //OSD 最大文本个数
#define ONVIF_OSD_DATE_NUM          1       //OSD 最大日期个数
#define ONVIF_OSD_DATE_NUM_STR     "1"     //OSD 最大日期个数

#define ONVIF_OSD_DATEFORMAT_TYPE_NUM          4       //OSD 日期配置类型个数
#define ONVIF_OSD_TIMEFORMAT_TYPE_NUM          2       //OSD 时间配置类型个数
//OSD字体大小范围
#define ONVIF_OSD_FONT_MIN          16
#define ONVIF_OSD_FONT_MAX          64
//OSD颜色空间大小范围
#define ONVIF_OSD_COLOR_MIN     0.000000f           
#define ONVIF_OSD_COLOR_MAX     255.000000f

//==========================	图像能力 Iming Capabilities			==========================
/* 图像服务能力XML描述 */
#define IMING_CAPABILITIES_MACRO \
    "<timg:Capabilities ImageStabilization=\"false\">\r\n " \     
    "</timg:Capabilities>\r\n"


#define BRIGHTNESS_RANGE_MIN 0   /* 亮度最小值 */
#define BRIGHTNESS_RANGE_MAX 100 /* 亮度最大值 */

#define CONTRAST_RANGE_MIN 0   /* 对比度最小值 */
#define CONTRAST_RANGE_MAX 100 /* 对比度最大值 */

#define SATURATION_RANGE_MIN 0   /* 饱和度最小值 */
#define SATURATION_RANGE_MAX 100 /* 饱和度最大值 */

#define SHARPNESS_RANGE_MIN 0   /* 锐度最小值 */
#define SHARPNESS_RANGE_MAX 100 /* 锐度最大值 */

//==========================	事件能力 Event Capabilities			==========================
#define ONVIF_SUPPORT_WS_SUB_POLICY        1   // 支持 WS-SubscriptionPolicy
#define ONVIF_SUPPORT_PULLPOINT            1   // 支持 PullPoint 模式
#define ONVIF_SUPPORT_PAUSABLE_SUB         0   // 不支持暂停订阅
#define ONVIF_MAX_NOTIFICATION_PRODUCERS   20  // 最大通知生产者数量
#define ONVIF_MAX_PULLPOINTS               20  // 最大 PullPoint 数量

//==========================	分析能力 Analytics Capabilities		==========================
/* 分析服务能力XML描述 */
#define ANALYTICS_CAPABILITIES_MACRO \
    "<tan:Capabilities\r\n " \
    "RuleSupport=\"true\"\r\n " \
    "AnalyticsModuleSupport=\"true\"\r\n " \
    "CellBasedSceneDescriptionSupported=\"true\"\r\n " \
    "RuleOptionsSupported=\"true\"\r\n " \
    "AnalyticsModuleOptionsSupported=\"false\" />\r\n"
/* 分析规则能力描述 */
#define MOTION_REGION_CONFIG_OPTIONS \
    "<axt:MotionRegionConfigOptions>\r\n" \
    "          <axt:MaxRegions>8</axt:MaxRegions>\r\n" \
    "          <axt:DisarmSupport>true</axt:DisarmSupport>\r\n" \
    "          <axt:PolygonSupport>false</axt:PolygonSupport>\r\n" \
    "          <axt:SingleSensitivitySupport>false</axt:SingleSensitivitySupport>\r\n" \
    "          <axt:RuleNotification>false</axt:RuleNotification>\r\n" \
    "        </axt:MotionRegionConfigOptions>\r\n"

#define ONVIF_ANALYTICS_SUPPORT_NUM                 20  //支持分析模块数量   
#define ONVIF_ANALYTICS_RULE_SUPPORT_NUM            20  //支持分析规则数量    
//==========================	设备IO DeviceIO Capabilities		==========================