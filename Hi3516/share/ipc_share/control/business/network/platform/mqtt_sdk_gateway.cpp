/**
 * @FilePath     : mqtt_sdk_gateway.cpp
 * @Description  : MQTT-SDK 命令转发网关实现
 * @Note         : 命令映射表覆盖 NetTVSDKCommon.h 中所有已定义的 SDK 命令码，
 *                 执行层复用 tvsdk_callbacks.cpp 的 execute_get_result 同步模式
 */

#if CAP_GARBAGE_STATION_PLATFORM
#include "mqtt_sdk_gateway.h"
#include "action_code.h"
#include "dlog.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>

/* TaskManage 头文件 */
#include "task_manage.h"
#include "task.h"

/* cJSON 用于 SET 数据包装（与 tvsdk_callbacks.cpp 的 wrap_data_json 对齐） */
#include "cJSON.h"

/* ==================== 静态成员 ==================== */
static CTaskManage *s_pTaskManage = nullptr;

/* ==================== 命令映射表 ==================== */

/**
 * @brief SDK 命令映射项
 * @note  将 SDK 命令名、SDK 命令码、内部 ActionCode 三元组关联
 */
struct SdkCommandMap_S
{
    const char *pCommandName;   /* SDK 命令名（大写，无空格） */
    int nSdkCommand;            /* SDK 命令码（NetTVSDKCommon.h 定义） */
    int nActionCode;            /* 内部 ActionCode（action_code.h 定义） */
    bool bIsGet;                /* true=GET 查询类，false=SET 设置类 */
    const char *pDescription;   /* 命令描述 */
};

/**
 * @brief 完整的 SDK 命令映射表
 * @note  每个 SDK 命令码在 NetTVSDKCommon.h 中定义，ActionCode 在 action_code.h 中定义
 *         命令名支持多种别名以兼容不同调用方的命名风格
 */
static const SdkCommandMap_S g_astCommandMap[] = {
    /* ==================== 设备基础信息 (100-109) ==================== */
    {"NET_TV_GET_DEVICECFG",              100, AC_GET_DEVICE_CONFIG,          true,  "获取设备基本信息"},
    {"NET_TV_GET_DEVICE_CONFIG",          100, AC_GET_DEVICE_CONFIG,          true,  "获取设备基本信息"},
    {"NET_TV_SET_DEVICECFG",              101, AC_SET_DEVICE_CONFIG,          false, "设置设备基本信息"},
    {"NET_TV_SET_DEVICE_CONFIG",          101, AC_SET_DEVICE_CONFIG,          false, "设置设备基本信息"},
    {"NET_TV_GET_UPGRADESTATUS",          102, AC_GET_UPGRADE_STATUS,         true,  "获取升级状态"},
    {"NET_TV_GET_UPGRADE_STATUS",         102, AC_GET_UPGRADE_STATUS,         true,  "获取升级状态"},
    {"NET_TV_SET_UPGRADE",                103, AC_DO_UPGRADE,                 false, "执行系统升级"},
    {"NET_TV_GET_UPGRADEVERSION",         104, AC_CHECK_UPGRADE,              true,  "获取升级版本信息"},
    {"NET_TV_GET_UPGRADE_VERSION",        104, AC_CHECK_UPGRADE,              true,  "获取升级版本信息"},

    /* ==================== NTP 时间配置 (110-119) ==================== */
    {"NET_TV_GET_NTPCFG",                 110, AC_GET_TIME_INFO,              true,  "获取NTP配置"},
    {"NET_TV_GET_NTP_CONFIG",             110, AC_GET_TIME_INFO,              true,  "获取NTP配置"},
    {"NET_TV_SET_NTPCFG",                 111, AC_SET_TIME_INFO,              false, "设置NTP配置"},
    {"NET_TV_SET_NTP_CONFIG",             111, AC_SET_TIME_INFO,              false, "设置NTP配置"},

    /* ==================== 视频编码 (120-129) ==================== */
    {"NET_TV_GET_STREAMCFG",              120, AC_GET_VIDEO_CONFIG,           true,  "获取视频编码配置"},
    {"NET_TV_GET_STREAM_CONFIG",          120, AC_GET_VIDEO_CONFIG,           true,  "获取视频编码配置"},
    {"NET_TV_SET_STREAMCFG",              121, AC_SET_VIDEO_CONFIG,           false, "设置视频编码配置"},
    {"NET_TV_SET_STREAM_CONFIG",          121, AC_SET_VIDEO_CONFIG,           false, "设置视频编码配置"},
    {"NET_TV_GET_RTSPURLCFG",             122, AC_GET_PREVIEW_INFO,           true,  "获取RTSP流地址"},
    {"NET_TV_GET_RTSP_URL_CONFIG",        122, AC_GET_PREVIEW_INFO,           true,  "获取RTSP流地址"},
    {"NET_TV_GET_REPLAY_URLCFG",          123, AC_GET_REPLAY_MEDIA_INFO,      true,  "获取回放地址"},
    {"NET_TV_GET_REPLAY_URL_CONFIG",      123, AC_GET_REPLAY_MEDIA_INFO,      true,  "获取回放地址"},
    {"NET_TV_GET_REPLAY_RECORD_LIST",     124, AC_GET_REPLAY_IPC_LIST,        true,  "获取回放录像列表"},
    {"NET_TV_SET_REPLAY_CTRL",            125, AC_SET_REPLAY_PLAY,            false, "控制回放"},

    /* ==================== 音频编码 (130-139) ==================== */
    {"NET_TV_GET_AUDIOCFG",               130, AC_GET_AUDIO_CONFIG,           true,  "获取音频编码配置"},
    {"NET_TV_GET_AUDIO_CONFIG",           130, AC_GET_AUDIO_CONFIG,           true,  "获取音频编码配置"},
    {"NET_TV_SET_AUDIOCFG",               131, AC_SET_AUDIO_CONFIG,           false, "设置音频编码配置"},
    {"NET_TV_SET_AUDIO_CONFIG",           131, AC_SET_AUDIO_CONFIG,           false, "设置音频编码配置"},

    /* ==================== OSD 配置 (140-149) ==================== */
    {"NET_TV_GET_OSDCAPCFG",              140, AC_GET_OSD_CONFIG,             true,  "获取OSD能力集配置"},
    {"NET_TV_GET_OSD_CAP_CONFIG",         140, AC_GET_OSD_CONFIG,             true,  "获取OSD能力集配置"},
    {"NET_TV_SET_OSDCAPCFG",              141, AC_SET_OSD_CONFIG,             false, "设置OSD能力集配置"},
    {"NET_TV_SET_OSD_CAP_CONFIG",         141, AC_SET_OSD_CONFIG,             false, "设置OSD能力集配置"},
    {"NET_TV_GET_OSDCFG",                 142, AC_GET_OSD_CONFIG,             true,  "获取OSD配置"},
    {"NET_TV_GET_OSD_CONFIG",             142, AC_GET_OSD_CONFIG,             true,  "获取OSD配置"},
    {"NET_TV_SET_OSDCFG",                 143, AC_SET_OSD_CONFIG,             false, "设置OSD配置"},
    {"NET_TV_SET_OSD_CONFIG",             143, AC_SET_OSD_CONFIG,             false, "设置OSD配置"},

    /* ==================== 图像配置 (160-169) ==================== */
    {"NET_TV_GET_IMAGECFG",               160, AC_GET_VIDEO_EFFECT_INFO,      true,  "获取图像配置"},
    {"NET_TV_GET_IMAGE_CONFIG",           160, AC_GET_VIDEO_EFFECT_INFO,      true,  "获取图像配置"},
    {"NET_TV_SET_IMAGECFG",               161, AC_SET_VIDEO_EFFECT_INFO,      false, "设置图像配置"},
    {"NET_TV_SET_IMAGE_CONFIG",           161, AC_SET_VIDEO_EFFECT_INFO,      false, "设置图像配置"},

    /* ==================== 网络配置 (170-179) ==================== */
    {"NET_TV_GET_NETWORKCFG",             170, AC_GET_NETWORK_INFO,           true,  "获取网络配置"},
    {"NET_TV_GET_NETWORK_CONFIG",         170, AC_GET_NETWORK_INFO,           true,  "获取网络配置"},
    {"NET_TV_SET_NETWORKCFG",             171, AC_SET_NETWORK_INFO,           false, "设置网络配置"},
    {"NET_TV_SET_NETWORK_CONFIG",         171, AC_SET_NETWORK_INFO,           false, "设置网络配置"},

    /* ==================== 隐私遮盖 (180-189) ==================== */
    {"NET_TV_GET_PRIVACYMASKCFG",         180, AC_GET_SHELTER_INFO,           true,  "获取隐私遮盖配置"},
    {"NET_TV_GET_PRIVACY_MASK_CONFIG",    180, AC_GET_SHELTER_INFO,           true,  "获取隐私遮盖配置"},
    {"NET_TV_SET_PRIVACYMASKCFG",         181, AC_SET_SHELTER_INFO,           false, "设置隐私遮盖配置"},
    {"NET_TV_SET_PRIVACY_MASK_CONFIG",    181, AC_SET_SHELTER_INFO,           false, "设置隐私遮盖配置"},

    /* ==================== 遮挡报警 (190-199) ==================== */
    {"NET_TV_GET_TAMPERALARM",            190, AG_GET_HIDE_ALARM_INFO,        true,  "获取遮挡报警配置"},
    {"NET_TV_GET_TAMPER_ALARM",           190, AG_GET_HIDE_ALARM_INFO,        true,  "获取遮挡报警配置"},
    {"NET_TV_SET_TAMPERALARM",            191, AG_SET_HIDE_ALARM_INFO,        false, "设置遮挡报警配置"},
    {"NET_TV_SET_TAMPER_ALARM",           191, AG_SET_HIDE_ALARM_INFO,        false, "设置遮挡报警配置"},

    /* ==================== 移动侦测 (200-201) ==================== */
    {"NET_TV_GET_MOTIONALARM",            200, AC_GET_MOTION_DETECT_INFO,     true,  "获取移动侦测配置"},
    {"NET_TV_GET_MOTION_ALARM",           200, AC_GET_MOTION_DETECT_INFO,     true,  "获取移动侦测配置"},
    {"NET_TV_SET_MOTIONALARM",            201, AC_SET_MOTION_DETECT_INFO,     false, "设置移动侦测配置"},
    {"NET_TV_SET_MOTION_ALARM",           201, AC_SET_MOTION_DETECT_INFO,     false, "设置移动侦测配置"},

    /* ==================== 越界侦测 (202-203) ==================== */
    {"NET_TV_GET_CROSSLINEALARM",         202, AC_GET_LINE_CROSSING_DETECT_INFO, true,  "获取越界侦测配置"},
    {"NET_TV_GET_CROSS_LINE_ALARM",       202, AC_GET_LINE_CROSSING_DETECT_INFO, true,  "获取越界侦测配置"},
    {"NET_TV_SET_CROSSLINEALARM",         203, AC_SET_LINE_CROSSING_DETECT_INFO, false, "设置越界侦测配置"},
    {"NET_TV_SET_CROSS_LINE_ALARM",       203, AC_SET_LINE_CROSSING_DETECT_INFO, false, "设置越界侦测配置"},

    /* ==================== 区域入侵 (204-205) ==================== */
    {"NET_TV_GET_INTRUSIONALARM",         204, AC_GET_REGIONAL_INTRUSION_DETECT_INFO, true,  "获取区域入侵配置"},
    {"NET_TV_GET_INTRUSION_ALARM",        204, AC_GET_REGIONAL_INTRUSION_DETECT_INFO, true,  "获取区域入侵配置"},
    {"NET_TV_SET_INTRUSIONALARM",         205, AC_SET_REGIONAL_INTRUSION_DETECT_INFO, false, "设置区域入侵配置"},
    {"NET_TV_SET_INTRUSION_ALARM",        205, AC_SET_REGIONAL_INTRUSION_DETECT_INFO, false, "设置区域入侵配置"},

    /* ==================== 徘徊侦测 (206-207) ==================== */
    {"NET_TV_GET_LOITERINGALARM",         206, AC_GET_LOITERING_DETECT_INFO,   true,  "获取徘徊侦测配置"},
    {"NET_TV_GET_LOITERING_ALARM",        206, AC_GET_LOITERING_DETECT_INFO,   true,  "获取徘徊侦测配置"},
    {"NET_TV_SET_LOITERINGALARM",         207, AC_SET_LOITERING_DETECT_INFO,   false, "设置徘徊侦测配置"},
    {"NET_TV_SET_LOITERING_ALARM",        207, AC_SET_LOITERING_DETECT_INFO,   false, "设置徘徊侦测配置"},

    /* ==================== 抓图计划 (208-211) ==================== */
    {"NET_TV_GET_CAPTURE_PLAN_INFO",      208, AC_GET_CAPTURE_PLAN_INFO,      true,  "获取抓图计划"},
    {"NET_TV_SET_CAPTURE_PLAN_INFO",      209, AC_SET_CAPTURE_PLAN_INFO,      false, "设置抓图计划"},
    {"NET_TV_GET_CAPTURE_PARAM_INFO",     210, AC_GET_CAPTURE_PARAM_INFO,     true,  "获取抓图参数"},
    {"NET_TV_SET_CAPTURE_PARAM_INFO",     211, AC_SET_CAPTURE_PARAM_INFO,     false, "设置抓图参数"},

    /* ==================== 曝光/日夜/背光/降噪/白平衡 (212-221) ==================== */
    {"NET_TV_GET_EXPOSURE_INFO",          212, AC_GET_EXPOSURE_INFO,          true,  "获取曝光配置"},
    {"NET_TV_SET_EXPOSURE_INFO",          213, AC_SET_EXPOSURE_INFO,          false, "设置曝光配置"},
    {"NET_TV_GET_DAYNIGHT_INFO",          214, AC_GET_DAY_NIGHT_INFO,         true,  "获取日夜转换配置"},
    {"NET_TV_GET_DAY_NIGHT_INFO",         214, AC_GET_DAY_NIGHT_INFO,         true,  "获取日夜转换配置"},
    {"NET_TV_SET_DAYNIGHT_INFO",          215, AC_SET_DAY_NIGHT_INFO,         false, "设置日夜转换配置"},
    {"NET_TV_SET_DAY_NIGHT_INFO",         215, AC_SET_DAY_NIGHT_INFO,         false, "设置日夜转换配置"},
    {"NET_TV_GET_BACKLIGHT_INFO",         216, AC_GET_BACK_LIGHT_INFO,        true,  "获取背光配置"},
    {"NET_TV_SET_BACKLIGHT_INFO",         217, AC_SET_BACK_LIGHT_INFO,        false, "设置背光配置"},
    {"NET_TV_GET_DENOISE_INFO",           218, AC_GET_NOISE_REMOVE_INFO,      true,  "获取降噪配置"},
    {"NET_TV_SET_DENOISE_INFO",           219, AC_SET_NOISE_REMOVE_INFO,      false, "设置降噪配置"},
    {"NET_TV_GET_WHITEBALANCE_INFO",      220, AC_GET_WHITE_BALANCE_INFO,     true,  "获取白平衡配置"},
    {"NET_TV_SET_WHITEBALANCE_INFO",      221, AC_SET_WHITE_BALANCE_INFO,     false, "设置白平衡配置"},

    /* ==================== 音频异常/预览/场景变更 (222-227) ==================== */
    {"NET_TV_GET_AUDIOANOMALYALARM",      222, AC_GET_AUDIO_ANOMALY_DETECT_INFO, true,  "获取音频异常侦测配置"},
    {"NET_TV_GET_AUDIO_ANOMALY_ALARM",    222, AC_GET_AUDIO_ANOMALY_DETECT_INFO, true,  "获取音频异常侦测配置"},
    {"NET_TV_SET_AUDIOANOMALYALARM",      223, AC_SET_AUDIO_ANOMALY_DETECT_INFO, false, "设置音频异常侦测配置"},
    {"NET_TV_SET_AUDIO_ANOMALY_ALARM",    223, AC_SET_AUDIO_ANOMALY_DETECT_INFO, false, "设置音频异常侦测配置"},
    {"NET_TV_GET_PREVIEW_INFO",           224, AC_GET_PREVIEW_INFO,           true,  "获取预览配置"},
    {"NET_TV_SET_PREVIEW_INFO",           225, AC_SET_PREVIEW_INFO,           false, "设置预览配置"},
    {"NET_TV_GET_SCENECHANGEALARM",       226, AC_GET_SCENE_CHANGE_DETECT_INFO, true,  "获取场景变更侦测配置"},
    {"NET_TV_GET_SCENE_CHANGE_ALARM",     226, AC_GET_SCENE_CHANGE_DETECT_INFO, true,  "获取场景变更侦测配置"},
    {"NET_TV_SET_SCENECHANGEALARM",       227, AC_SET_SCENE_CHANGE_DETECT_INFO, false, "设置场景变更侦测配置"},
    {"NET_TV_SET_SCENE_CHANGE_ALARM",     227, AC_SET_SCENE_CHANGE_DETECT_INFO, false, "设置场景变更侦测配置"},

    /* ==================== 人员聚集/停车/物品遗留/物品拿取 (228-235) ==================== */
    {"NET_TV_GET_CROWDGATHERINGALARM",    228, AC_GET_CROWD_GATHERING_DETECT_INFO, true,  "获取人员聚集侦测配置"},
    {"NET_TV_GET_CROWD_GATHERING_ALARM",  228, AC_GET_CROWD_GATHERING_DETECT_INFO, true,  "获取人员聚集侦测配置"},
    {"NET_TV_SET_CROWDGATHERINGALARM",    229, AC_SET_CROWD_GATHERING_DETECT_INFO, false, "设置人员聚集侦测配置"},
    {"NET_TV_SET_CROWD_GATHERING_ALARM",  229, AC_SET_CROWD_GATHERING_DETECT_INFO, false, "设置人员聚集侦测配置"},
    {"NET_TV_GET_PARKINGALARM",           230, AC_GET_PARKING_DETECT_INFO,     true,  "获取停车侦测配置"},
    {"NET_TV_GET_PARKING_ALARM",          230, AC_GET_PARKING_DETECT_INFO,     true,  "获取停车侦测配置"},
    {"NET_TV_SET_PARKINGALARM",           231, AC_SET_PARKING_DETECT_INFO,     false, "设置停车侦测配置"},
    {"NET_TV_SET_PARKING_ALARM",          231, AC_SET_PARKING_DETECT_INFO,     false, "设置停车侦测配置"},
    {"NET_TV_GET_UNATTENDEDOBJECTALARM",  232, AC_GET_UNATTENDED_OBJECT_DETECT_INFO, true,  "获取物品遗留侦测配置"},
    {"NET_TV_GET_UNATTENDED_OBJECT_ALARM", 232, AC_GET_UNATTENDED_OBJECT_DETECT_INFO, true,  "获取物品遗留侦测配置"},
    {"NET_TV_SET_UNATTENDEDOBJECTALARM",  233, AC_SET_UNATTENDED_OBJECT_DETECT_INFO, false, "设置物品遗留侦测配置"},
    {"NET_TV_SET_UNATTENDED_OBJECT_ALARM", 233, AC_SET_UNATTENDED_OBJECT_DETECT_INFO, false, "设置物品遗留侦测配置"},
    {"NET_TV_GET_OBJECTREMOVALALARM",     234, AC_GET_OBJECT_REMOVAL_DETECT_INFO, true,  "获取物品拿取侦测配置"},
    {"NET_TV_GET_OBJECT_REMOVAL_ALARM",   234, AC_GET_OBJECT_REMOVAL_DETECT_INFO, true,  "获取物品拿取侦测配置"},
    {"NET_TV_SET_OBJECTREMOVALALARM",     235, AC_SET_OBJECT_REMOVAL_DETECT_INFO, false, "设置物品拿取侦测配置"},
    {"NET_TV_SET_OBJECT_REMOVAL_ALARM",   235, AC_SET_OBJECT_REMOVAL_DETECT_INFO, false, "设置物品拿取侦测配置"},

    /* ==================== WiFi/4G/热点 (236-248) ==================== */
    {"NET_TV_SET_CONFIG_WIFI_STA",        236, AC_SET_CONFIG_WIFI_STA,        false, "设置WiFi STA配置"},
    {"NET_TV_GET_4G_INFO",                239, AC_GET_4G_INFO,                true,  "获取4G配置"},
    {"NET_TV_SET_4G_INFO",                240, AC_SET_4G_INFO,                false, "设置4G配置"},
    {"NET_TV_SET_HOTSPOT_INFO",           241, AC_SET_HOTSPOT_INFO,           false, "设置热点配置"},
    {"NET_TV_GET_HOTSPOT_CONN",           248, AC_GET_HOTSPOT_CONN,           true,  "获取热点连接设备"},

    /* ==================== 进入/离开区域 (242-245) ==================== */
    {"NET_TV_GET_ENTERREGIONALARM",       242, AC_GET_ENTER_REGION_DETECT_INFO, true,  "获取进入区域侦测配置"},
    {"NET_TV_GET_ENTER_REGION_ALARM",     242, AC_GET_ENTER_REGION_DETECT_INFO, true,  "获取进入区域侦测配置"},
    {"NET_TV_SET_ENTERREGIONALARM",       243, AC_SET_ENTER_REGION_DETECT_INFO, false, "设置进入区域侦测配置"},
    {"NET_TV_SET_ENTER_REGION_ALARM",     243, AC_SET_ENTER_REGION_DETECT_INFO, false, "设置进入区域侦测配置"},
    {"NET_TV_GET_LEAVEREGIONALARM",       244, AC_GET_LEAVE_REGION_DETECT_INFO, true,  "获取离开区域侦测配置"},
    {"NET_TV_GET_LEAVE_REGION_ALARM",     244, AC_GET_LEAVE_REGION_DETECT_INFO, true,  "获取离开区域侦测配置"},
    {"NET_TV_SET_LEAVEREGIONALARM",       245, AC_SET_LEAVE_REGION_DETECT_INFO, false, "设置离开区域侦测配置"},
    {"NET_TV_SET_LEAVE_REGION_ALARM",     245, AC_SET_LEAVE_REGION_DETECT_INFO, false, "设置离开区域侦测配置"},

    /* ==================== 人脸抓拍 (246-247) ==================== */
    {"NET_TV_GET_FACECAPTUREINFO",        246, AC_GET_FACE_CAPTURE_INFO,      true,  "获取人脸抓拍配置"},
    {"NET_TV_GET_FACE_CAPTURE_INFO",      246, AC_GET_FACE_CAPTURE_INFO,      true,  "获取人脸抓拍配置"},
    {"NET_TV_SET_FACECAPTUREINFO",        247, AC_SET_FACE_CAPTURE_INFO,      false, "设置人脸抓拍配置"},
    {"NET_TV_SET_FACE_CAPTURE_INFO",      247, AC_SET_FACE_CAPTURE_INFO,      false, "设置人脸抓拍配置"},

    /* ==================== 人脸比对 (482) ==================== */
    {"NET_TV_SET_FACE_COMPARE_INFO",      482, AC_SET_FACE_COMPARE_INFO,     false, "设置人脸比对配置"},
    {"NET_TV_GET_FACE_COMPARE_INFO",      482, AC_GET_FACE_COMPARE_INFO,     true,  "获取人脸比对配置"},

    /* ==================== 通道信息 (300-301) ==================== */
    {"NET_TV_GET_CHANNEL_INFO",           300, AC_GET_DEVICE_INFO,            true,  "获取通道信息"},
    {"NET_TV_GET_CHANNEL_LIST",           301, AC_GET_DEVICE_INFO,            true,  "获取通道列表"},

    /* ==================== 垃圾站专项 (404-426) ==================== */
    {"NET_TV_GET_GARBAGE_EXPOSURE_CFG",   404, AC_GET_GARBAGE_EXPOSURE_CFG,   true,  "获取垃圾暴露配置"},
    {"NET_TV_SET_GARBAGE_EXPOSURE_CFG",   405, AC_SET_GARBAGE_EXPOSURE_CFG,   false, "设置垃圾暴露配置"},
    {"NET_TV_GET_GARBAGE_OVERFLOW_CFG",   406, AC_GET_GARBAGE_OVERFLOW_CFG,   true,  "获取垃圾满溢配置"},
    {"NET_TV_SET_GARBAGE_OVERFLOW_CFG",   407, AC_SET_GARBAGE_OVERFLOW_CFG,   false, "设置垃圾满溢配置"},
    {"NET_TV_GET_PEOPLE_FLOW_STATISTICS_CFG",     408, AC_GET_PEOPLE_FLOW_STATISTICS_INFO,     true,  "获取人流统计配置"},
    {"NET_TV_SET_PEOPLE_FLOW_STATISTICS_CFG",     409, AC_SET_PEOPLE_FLOW_STATISTICS_INFO,     false, "设置人流统计配置"},
    {"NET_TV_GET_PEOPLE_DENSITY_DETECTION_CFG",   411, AC_GET_PEOPLE_DENSITY_DETECTION_INFO,   true,  "获取人员密度检测配置"},
    {"NET_TV_SET_PEOPLE_DENSITY_DETECTION_CFG",   412, AC_SET_PEOPLE_DENSITY_DETECTION_INFO,   false, "设置人员密度检测配置"},
    {"NET_TV_GET_MANHOLE_COVER_ABNORMAL_CFG",     413, AC_GET_MANHOLE_COVER_ABNORMAL_CFG,     true,  "获取井盖异常检测配置"},
    {"NET_TV_SET_MANHOLE_COVER_ABNORMAL_CFG",     414, AC_SET_MANHOLE_COVER_ABNORMAL_CFG,     false, "设置井盖异常检测配置"},
    {"NET_TV_GET_SLEEP_ON_DUTY_CFG",              415, AC_GET_SLEEP_ON_DUTY_CFG,              true,  "获取睡岗识别配置"},
    {"NET_TV_SET_SLEEP_ON_DUTY_CFG",              416, AC_SET_SLEEP_ON_DUTY_CFG,              false, "设置睡岗识别配置"},
    {"NET_TV_GET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG", 417, AC_GET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG, true, "获取电瓶车进电梯配置"},
    {"NET_TV_SET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG", 418, AC_SET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG, false, "设置电瓶车进电梯配置"},
    {"NET_TV_GET_PERSON_FALL_DOWN_CFG",           419, AC_GET_PERSON_FALL_DOWN_CFG,           true,  "获取人员倒地识别配置"},
    {"NET_TV_SET_PERSON_FALL_DOWN_CFG",           420, AC_SET_PERSON_FALL_DOWN_CFG,           false, "设置人员倒地识别配置"},
    {"NET_TV_GET_CONSTRUCTION_OCCUPY_ROAD_CFG",   421, AC_GET_CONSTRUCTION_OCCUPY_ROAD_CFG,   true,  "获取施工占道识别配置"},
    {"NET_TV_SET_CONSTRUCTION_OCCUPY_ROAD_CFG",   422, AC_SET_CONSTRUCTION_OCCUPY_ROAD_CFG,   false, "设置施工占道识别配置"},
    {"NET_TV_GET_CONGESTION_CFG",                 423, AC_GET_CONGESTION_CFG,                 true,  "获取拥堵识别配置"},
    {"NET_TV_SET_CONGESTION_CFG",                 424, AC_SET_CONGESTION_CFG,                 false, "设置拥堵识别配置"},
    {"NET_TV_GET_LICENSE_PLATE_RECOGNITION_CFG",  425, AC_GET_LICENSE_PLATE_RECOGNITION_CFG,  true,  "获取车牌识别配置"},
    {"NET_TV_SET_LICENSE_PLATE_RECOGNITION_CFG",  426, AC_SET_LICENSE_PLATE_RECOGNITION_CFG,  false, "设置车牌识别配置"},
    {"NET_TV_GET_HIGH_ALTITUDE_SEATBELT_CFG",     427, AC_GET_HIGH_ALTITUDE_SEATBELT_CFG,     true,  "获取高空安全带识别配置"},
    {"NET_TV_SET_HIGH_ALTITUDE_SEATBELT_CFG",     428, AC_SET_HIGH_ALTITUDE_SEATBELT_CFG,     false, "设置高空安全带识别配置"},

    /* ==================== 目标库/人脸库 (483-490) ==================== */
    {"NET_TV_ADD_TARGET_LIB",             483, AC_ADD_TARGET_LIB,             false, "添加目标库"},
    {"NET_TV_DEL_TARGET_LIB",             484, AC_DEL_TARGET_LIB,             false, "删除目标库"},
    {"NET_TV_SET_TARGET_LIB",             485, AC_SET_TARGET_LIB,             false, "修改目标库"},
    {"NET_TV_GET_TARGET_LIB",             486, AC_GET_TARGET_LIB,             true,  "获取目标库"},
    {"NET_TV_ADD_FACE_INFO",              487, AC_ADD_FACE_INFO,              false, "添加人脸"},
    {"NET_TV_DEL_FACE_INFO",              488, AC_DEL_FACE_INFO,              false, "删除人脸"},
    {"NET_TV_SET_FACE_INFO",              489, AC_SET_FACE_INFO,              false, "修改人脸"},
    {"NET_TV_GET_FACE_INFO",              490, AC_GET_FACE_INFO,              true,  "获取人脸"},
};

static constexpr int g_nCommandMapSize = sizeof(g_astCommandMap) / sizeof(g_astCommandMap[0]);

/* ==================== 辅助函数 ==================== */

/**
 * @brief  : 标准化命令名（去空格、转大写）
 */
std::string CMqttSdkGateway::normalize_command_name(const std::string &strCommand)
{
    std::string strResult;
    strResult.reserve(strCommand.size());
    for (char ch : strCommand)
    {
        if (!std::isspace(static_cast<unsigned char>(ch)))
        {
            strResult.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
        }
    }
    return strResult;
}

/**
 * @brief  : 根据命令名查找 SDK 命令码
 */
int CMqttSdkGateway::resolve_sdk_command(const std::string &strCommand)
{
    const std::string strNormalized = normalize_command_name(strCommand);

    /* 先尝试作为数字解析 */
    if (!strNormalized.empty() && std::all_of(strNormalized.begin(), strNormalized.end(),
        [](unsigned char ch) { return std::isdigit(ch) != 0; }))
    {
        return std::atoi(strNormalized.c_str());
    }

    /* 在映射表中查找 */
    for (int i = 0; i < g_nCommandMapSize; ++i)
    {
        if (strNormalized == g_astCommandMap[i].pCommandName)
        {
            return g_astCommandMap[i].nSdkCommand;
        }
    }

    return 0;
}

/**
 * @brief  : 将 SDK 命令码映射为内部 ActionCode
 */
int CMqttSdkGateway::sdk_command_to_action_code(int nSdkCommand)
{
    for (int i = 0; i < g_nCommandMapSize; ++i)
    {
        if (g_astCommandMap[i].nSdkCommand == nSdkCommand)
        {
            return g_astCommandMap[i].nActionCode;
        }
    }
    return 0;
}

/* ==================== 公开接口 ==================== */

void CMqttSdkGateway::set_task_manage(CTaskManage *pTaskManage)
{
    s_pTaskManage = pTaskManage;
    dlog_info("MQTT SDK 网关：CTaskManage 实例已设置");
}

bool CMqttSdkGateway::is_get_command(const std::string &strCommand)
{
    const std::string strNormalized = normalize_command_name(strCommand);
    for (int i = 0; i < g_nCommandMapSize; ++i)
    {
        if (strNormalized == g_astCommandMap[i].pCommandName)
        {
            return g_astCommandMap[i].bIsGet;
        }
    }
    /* 默认按命令名前缀判断 */
    return strNormalized.find("GET") != std::string::npos;
}

bool CMqttSdkGateway::is_command_supported(const std::string &strCommand)
{
    return resolve_sdk_command(strCommand) != 0;
}

/**
 * @brief  : 通过 CTaskManage 同步执行 GET 命令
 * @note   : 复用 tvsdk_callbacks.cpp 的 execute_get_result 模式
 *           通过 lambda 捕获输出引用，在 fnResultCallbacks 中同步赋值
 */
int CMqttSdkGateway::execute_get_result(int nActionCode, const std::string &strData, std::string &strResult)
{
    if (!s_pTaskManage)
    {
        dlog_error("MQTT SDK 网关：CTaskManage 未设置");
        return -1;
    }

    strResult.clear();

    Task::Info_S stInfo;
    stInfo.data = strData;
    stInfo.fnResultCallbacks = [&strResult](const void *pData, int nLen, int /*nActionCode*/, void * /*pHandler*/) -> int {
        if (pData && nLen > 0)
        {
            strResult.assign(static_cast<const char *>(pData), static_cast<size_t>(nLen));
        }
        return 0;
    };

    return s_pTaskManage->execute(nActionCode, stInfo);
}

/**
 * @brief  : 将裸 JSON 配置包装为 {"Data": ...} 格式
 * @note   : 任务系统 set_info() 通过 get_data(stInfo.data) 提取 "Data" 字段赋给 m_taskData，
 *           因此 SET 命令必须将配置对象包装在 "Data" 键下，与 tvsdk_callbacks.cpp 的 wrap_data_json 对齐
 */
static std::string wrap_set_data(const std::string &strRawJson)
{
    if (strRawJson.empty())
        return "{}";

    cJSON *pConfig = cJSON_Parse(strRawJson.c_str());
    if (!pConfig)
        return strRawJson;

    cJSON *pRoot = cJSON_CreateObject();
    if (!pRoot)
    {
        cJSON_Delete(pConfig);
        return strRawJson;
    }

    cJSON_AddItemToObject(pRoot, "Data", pConfig);

    char *pOut = cJSON_PrintUnformatted(pRoot);
    std::string strResult = pOut ? pOut : strRawJson;
    free(pOut);
    cJSON_Delete(pRoot);

    return strResult;
}

/**
 * @brief  : 通过 CTaskManage 执行 SET 命令
 */
int CMqttSdkGateway::execute_set_action(int nActionCode, const std::string &strData)
{
    if (!s_pTaskManage)
    {
        dlog_error("MQTT SDK 网关：CTaskManage 未设置");
        return -1;
    }

    Task::Info_S stInfo;
    stInfo.data = wrap_set_data(strData);
    return s_pTaskManage->execute(nActionCode, stInfo);
}

int CMqttSdkGateway::execute_set_action(int nActionCode, const std::string &strData, std::string &strResult)
{
    if (!s_pTaskManage)
    {
        dlog_error("MQTT SDK 网关：CTaskManage 未设置");
        return -1;
    }

    strResult.clear();

    Task::Info_S stInfo;
    stInfo.data = wrap_set_data(strData);
    stInfo.fnResultCallbacks = [&strResult](const void *pData, int nLen, int /*nActionCode*/, void * /*pHandler*/) -> int {
        if (pData && nLen > 0)
        {
            strResult.assign(static_cast<const char *>(pData), static_cast<size_t>(nLen));
        }
        return 0;
    };

    return s_pTaskManage->execute(nActionCode, stInfo);
}

/**
 * @brief  : 执行 MQTT GET 命令
 */
int CMqttSdkGateway::execute_get(const std::string &strCommand, const std::string &strData, std::string &strResult)
{
    const int nSdkCommand = resolve_sdk_command(strCommand);
    if (nSdkCommand == 0)
    {
        dlog_error("MQTT SDK 网关：未知命令[%s]", strCommand.c_str());
        return -1;
    }

    const int nActionCode = sdk_command_to_action_code(nSdkCommand);
    if (nActionCode == 0)
    {
        dlog_error("MQTT SDK 网关：命令[%s]无ActionCode映射", strCommand.c_str());
        return -2;
    }

    dlog_info("MQTT SDK 网关：执行 GET 命令[%s] SDK码[%d] ActionCode[%d]",
              strCommand.c_str(), nSdkCommand, nActionCode);

    return execute_get_result(nActionCode, strData, strResult);
}

/**
 * @brief  : 执行 MQTT SET 命令
 */
int CMqttSdkGateway::execute_set(const std::string &strCommand, const std::string &strData)
{
    const int nSdkCommand = resolve_sdk_command(strCommand);
    if (nSdkCommand == 0)
    {
        dlog_error("MQTT SDK 网关：未知命令[%s]", strCommand.c_str());
        return -1;
    }

    const int nActionCode = sdk_command_to_action_code(nSdkCommand);
    if (nActionCode == 0)
    {
        dlog_error("MQTT SDK 网关：命令[%s]无ActionCode映射", strCommand.c_str());
        return -2;
    }

    dlog_info("MQTT SDK 网关：执行 SET 命令[%s] SDK码[%d] ActionCode[%d]",
              strCommand.c_str(), nSdkCommand, nActionCode);

    return execute_set_action(nActionCode, strData);
}

int CMqttSdkGateway::execute_set(const std::string &strCommand, const std::string &strData, std::string &strResult)
{
    const int nSdkCommand = resolve_sdk_command(strCommand);
    if (nSdkCommand == 0)
    {
        dlog_error("MQTT SDK 网关：未知命令[%s]", strCommand.c_str());
        return -1;
    }

    const int nActionCode = sdk_command_to_action_code(nSdkCommand);
    if (nActionCode == 0)
    {
        dlog_error("MQTT SDK 网关：命令[%s]无ActionCode映射", strCommand.c_str());
        return -2;
    }

    dlog_info("MQTT SDK 网关：执行 SET 命令并返回结果[%s] SDK码[%d] ActionCode[%d]",
              strCommand.c_str(), nSdkCommand, nActionCode);

    return execute_set_action(nActionCode, strData, strResult);
}
#endif
