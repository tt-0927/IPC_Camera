
#ifndef _USERCONVERT_H
#define _USERCONVERT_H

#include <string>
#include <vector>
#include <set>

#include "Json.h"

// 库通用头文件
#ifdef NET_TV_SDK_SERVER_API
    #include "NetTVSDKServerInterface.h"
#elif defined(NET_TV_SDK_CLIENT_API)
    #include "NetTVSDKClientInterface.h"
#else
    #include "NetTVSDKCommon.h"
#endif

namespace SDKConvert
{
    void deal(Json::Object* pRootJson, NET_TV_AUDIO_CFG_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_ENTER_REGION_ALARM_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_LEAVE_REGION_ALARM_INFO_S& stInfo, bool bOutStruct);

    void deal(Json::Object* pRootJson, NET_TV_DEVICE_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_DEVICE_BASICINFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_SYSTEM_NTP_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_USER_PASSWORD_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_NETWORKCFG_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_WIFI_STA_CFG_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_WIFI_WEP_KEY_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_WIFI_STA_CONNECT_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_4G_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_HOTSPOT_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_HOTSPOT_CONN_DEVICE_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_HOTSPOT_CONN_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_PAGE_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_LOGIN_LOCK_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_PWD_POLICY_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_SSH_ADMIN_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_SECURITY_SERVICES_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_SSH_COUNTDOWN_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_LOG_SERVER_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_LOG_RETRIEVAL_COND_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_LOG_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_LOG_LIST_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_RECORD_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_RECORD_STATUS_INFO_S& stInfo, bool bOutStruct);
     /**
     * @brief 在 JSON 与 SDK SD 卡状态结构体之间转换。
     * @author ITC
     * @param [in,out] pRootJson 根据 bOutStruct 作为源 JSON 或目标 JSON 对象。
     * @param [in,out] stInfo 根据 bOutStruct 作为源或目标 SD 卡状态结构体。
     * @param [in] bOutStruct 为 TRUE 时将 JSON 解析到 stInfo；为 FALSE 时将 stInfo 序列化到 JSON。
     * @return 无。
     */
    void deal(Json::Object* pRootJson, NET_SdCardStatus_S& stInfo, bool bOutStruct);

    
    /**
     * @brief 在 JSON 与 SDK 自定义声音告警音频信息之间转换。
     * @author ITC
     * @param [in,out] pRootJson 根据 bOutStruct 作为源或目标 JSON 对象。
     * @param [in,out] stInfo 根据 bOutStruct 作为源或目标自定义音频结构体。
     * @param [in] bOutStruct 为 TRUE 时将 JSON 解析到结构体；为 FALSE 时将结构体序列化到 JSON。
     * @return 无。
     */
    void deal(Json::Object* pRootJson, NET_AudibleAlarmCustomAudio_S& stInfo, bool bOutStruct);

    /**
     * @brief 在 JSON 与 SDK 声音告警配置之间转换。
     * @author ITC
     * @param [in,out] pRootJson 根据 bOutStruct 作为源或目标 JSON 对象。
     * @param [in,out] stInfo 根据 bOutStruct 作为源或目标声音告警配置结构体。
     * @param [in] bOutStruct 为 TRUE 时将 JSON 解析到结构体；为 FALSE 时将结构体序列化到 JSON。
     * @return 无。
     */
    void deal(Json::Object* pRootJson, NET_AudibleAlarmInfo_S& stInfo, bool bOutStruct);

    /**
     * @brief 在 JSON 与 SDK 单路报警输入配置之间转换。
     * @author ITC
     * @param [in,out] pRootJson 根据 bOutStruct 作为源或目标 JSON 对象。
     * @param [in,out] stInfo 根据 bOutStruct 作为源或目标报警输入配置结构体。
     * @param [in] bOutStruct 为 TRUE 时将 JSON 解析到结构体；为 FALSE 时将结构体序列化到 JSON。
     * @return 无。
     */
    void deal(Json::Object* pRootJson, NET_AlarmInputInfo_S& stInfo, bool bOutStruct);

    /**
     * @brief 在 JSON 与 SDK 报警输入配置集合之间转换。
     * @author ITC
     * @param [in,out] pRootJson 根据 bOutStruct 作为源或目标 JSON 对象。
     * @param [in,out] stInfo 根据 bOutStruct 作为源或目标报警输入配置集合结构体。
     * @param [in] bOutStruct 为 TRUE 时将 JSON 解析到结构体；为 FALSE 时将结构体序列化到 JSON。
     * @return 无。
     */
    void deal(Json::Object* pRootJson, NET_AlarmInputInfoList_S& stInfo, bool bOutStruct);

    /**
     * @brief 在 JSON 与 SDK 单路报警输出配置之间转换。
     * @author ITC
     * @param [in,out] pRootJson 根据 bOutStruct 作为源或目标 JSON 对象。
     * @param [in,out] stInfo 根据 bOutStruct 作为源或目标报警输出配置结构体。
     * @param [in] bOutStruct 为 TRUE 时将 JSON 解析到结构体；为 FALSE 时将结构体序列化到 JSON。
     * @return 无。
     */
    void deal(Json::Object* pRootJson, NET_AlarmOutputInfo_S& stInfo, bool bOutStruct);

    /**
     * @brief 在 JSON 与 SDK 报警输出配置集合之间转换。
     * @author ITC
     * @param [in,out] pRootJson 根据 bOutStruct 作为源或目标 JSON 对象。
     * @param [in,out] stInfo 根据 bOutStruct 作为源或目标报警输出配置集合结构体。
     * @param [in] bOutStruct 为 TRUE 时将 JSON 解析到结构体；为 FALSE 时将结构体序列化到 JSON。
     * @return 无。
     */
    void deal(Json::Object* pRootJson, NET_AlarmOutputInfoList_S& stInfo, bool bOutStruct);

    /**
     * @brief 在 JSON 与 SDK 闪光灯告警配置之间转换。
     * @author ITC
     * @param [in,out] pRootJson 根据 bOutStruct 作为源或目标 JSON 对象。
     * @param [in,out] stInfo 根据 bOutStruct 作为源或目标闪光灯告警配置结构体。
     * @param [in] bOutStruct 为 TRUE 时将 JSON 解析到结构体；为 FALSE 时将结构体序列化到 JSON。
     * @return 无。
     */
    void deal(Json::Object* pRootJson, NET_FlashingLightAlarmInfo_S& stInfo, bool bOutStruct);

    /**
     * @brief 在 JSON 与 SDK 手动声光报警触发请求之间转换。
     * @author ITC
     * @param [in,out] pRootJson 根据 bOutStruct 作为源或目标 JSON 对象。
     * @param [in,out] stInfo 根据 bOutStruct 作为源或目标手动声光报警触发请求结构体。
     * @param [in] bOutStruct 为 TRUE 时将 JSON 解析到结构体；为 FALSE 时将结构体序列化到 JSON。
     * @return 无。
     */
    void deal(Json::Object* pRootJson, NET_SoundLightAlarmTrigger_S& stInfo, bool bOutStruct);

    /**
     * @brief 在 JSON 与 SDK PIR 告警配置之间转换。
     * @author ITC
     * @param [in,out] pRootJson 根据 bOutStruct 作为源或目标 JSON 对象。
     * @param [in,out] stInfo 根据 bOutStruct 作为源或目标 PIR 告警配置结构体。
     * @param [in] bOutStruct 为 TRUE 时将 JSON 解析到结构体；为 FALSE 时将结构体序列化到 JSON。
     * @return 无。
     */
    void deal(Json::Object* pRootJson, NET_PirAlarmInfo_S& stInfo, bool bOutStruct);
    
    void deal(Json::Object* pRootJson, NET_TV_RECORD_TIME_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_RECORD_DAY_SCHEDULE_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_RECORD_SCHEDULE_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_RECORD_ADVANCED_PARAM_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_RECORD_FIND_COND_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_RECORD_VIDEO_TIME_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_RECORD_FIND_RESULT_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_RECORD_FILE_LIST_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_RECORD_DOWNLOAD_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_RECORD_DOWNLOAD_PROGRESS_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_RECORD_DOWNLOAD_LIST_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_VIDEO_OSD_CFG_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_RTSP_URL_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_DEVICE_CONTROL_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_RECORD_FRAME_STREAM_COND_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_RECORD_FRAME_STREAM_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_RECORD_FRAME_STOP_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_REPLAY_URL_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_REPLAY_CTRL_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_REPLAY_RECORD_TIME_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_REPLAY_RECORD_LIST_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_CHANNEL_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_CHANNEL_LIST_S& stInfo, bool bOutStruct);


    void deal(Json::Object* pRootJson, NET_TV_PREVIEW_RTSP_URL_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_PREVIEW_IMAGE_PARAM_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_IMAGE_SETTING_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_PREVIEW_INFO_S& stInfo, bool bOutStruct);

    void deal(Json::Object* pRootJson, NET_TV_TALKBACK_STATE_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_TALKBACK_STREAM_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_REPLAY_TALKBACK_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_VOICECOM_AUDIO_CFG_S& stInfo, bool bOutStruct);


    // 系统升级相关
    void deal(Json::Object*& pRootJson, tagNETTVUpgradeInfo*& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_UPGRADE_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_UPGRADE_STATUS_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_UPGRADE_VERSION_S& stInfo, bool bOutStruct);
    
    // 布防时间和联动相关
    void deal(Json::Object* pRootJson, NET_TV_SCHED_TIME_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_ALARM_SCHEDULE_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_LINKAGE_LIST_S& stInfo, bool bOutStruct);
    
    // 移动侦测相关
    void deal(Json::Object* pRootJson, NET_TV_MOTION_REGION_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_MOTION_EXPERT_MODE_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_MOTION_NORMAL_MODE_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_MOTION_ALARM_INFO_S& stInfo, bool bOutStruct);
    
    // 隐私遮盖配置相关
    void deal(Json::Object* pRootJson, NET_TV_PRIVACY_MASK_AREA_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_PRIVACY_MASK_CFG_S& stInfo, bool bOutStruct);

    // 遮挡报警相关
    void deal(Json::Object* pRootJson, NET_TV_TAMPER_ALARM_INFO_S& stInfo, bool bOutStruct);
    
    // 越界检测相关
    void deal(Json::Object* pRootJson, NET_TV_BOUNDARY_PLANE_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_CROSS_LINE_ALARM_INFO_S& stInfo, bool bOutStruct);
    
    // 入侵检测相关
    void deal(Json::Object* pRootJson, NET_TV_INTRUSION_RULE_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_INTRUSION_ALARM_INFO_S& stInfo, bool bOutStruct);

    // 徘徊侦测相关
    void deal(Json::Object* pRootJson, NET_TV_LOITERING_RULE_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_LOITERING_ALARM_INFO_S& stInfo, bool bOutStruct);

    void deal(Json::Object* pRootJson, NET_TV_AUDIO_ANOMALY_ALARM_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_SCENE_CHANGE_ALARM_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_CROWD_GATHERING_RULE_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_CROWD_GATHERING_ALARM_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_PARKING_RULE_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_PARKING_ALARM_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_UNATTENDED_OBJECT_RULE_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_UNATTENDED_OBJECT_ALARM_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_OBJECT_REMOVAL_RULE_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_OBJECT_REMOVAL_ALARM_INFO_S& stInfo, bool bOutStruct);

    // 垃圾检测配置
    void deal(Json::Object* pRootJson, NET_TV_GARBAGE_EXPOSURE_RULE_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_GARBAGE_EXPOSURE_CFG_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_GARBAGE_OVERFLOW_RULE_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_GARBAGE_OVERFLOW_CFG_S& stInfo, bool bOutStruct);

    // 单规则智能检测配置
    void deal(Json::Object* pRootJson, NET_TV_AI_SIMPLE_RULE_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_MANHOLE_COVER_ABNORMAL_CFG_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_SLEEP_ON_DUTY_CFG_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_PERSON_FALL_DOWN_CFG_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_CONSTRUCTION_OCCUPY_ROAD_CFG_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_CONGESTION_CFG_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_LICENSE_PLATE_RECOGNITION_CFG_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_HIGH_ALTITUDE_SEATBELT_CFG_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_SAFETY_HELMET_CFG_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_PERSON_FALL_CFG_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_PHONE_USAGE_CFG_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_SMOKING_CFG_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_OPEN_FLAME_CFG_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_BARE_SOIL_CFG_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_HOLE_PROTECTION_BAR_CFG_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_REFLECTIVE_CLOTHING_CFG_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_SMART_REGION_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_SMART_REGION_RULE_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_SMART_LINE_RULE_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_PET_RECOGNITION_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_CLIMB_FENCE_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_DIMISSION_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_ILLEGAL_LANE_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_RETROGRADE_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_NONMOTOR_VEHICLE_INTRUSION_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_OCCUPATION_EMERGENCY_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_PEDESTRIAN_INTRUSION_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_SMOKE_FIRE_CFG_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_ROAD_PONDING_CFG_S& stInfo, bool bOutStruct);
    
    // 抓图计划相关
    void deal(Json::Object *pRootJson, NET_TV_CAPTURE_TIME_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, NET_TV_CAPTURE_DAY_SCHEDULE_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, NET_TV_CAPTURE_PLAN_INFO_S &stInfo, bool bOutStruct);

    // 抓图参数相关
    void deal(Json::Object *pRootJson, NET_TV_CAPTURE_CONFIG_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, NET_TV_CAPTURE_PARAM_INFO_S &stInfo, bool bOutStruct);

    // ISP params
    void deal(Json::Object *pRootJson, NET_TV_EXPOSURE_INFO_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, NET_TV_DAYNIGHT_INFO_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, NET_TV_BACKLIGHT_INFO_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, NET_TV_DENOISE_INFO_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, NET_TV_WHITEBALANCE_INFO_S &stInfo, bool bOutStruct);

    void deal(Json::Object* pRootJson, NET_TV_FACE_CAPTURE_REGION_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_FACE_CAPTURE_RULE_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_FACE_CAPTURE_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_FACE_CAPTURE_OVERLAY_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_FACE_COMPARE_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_FACE_LIB_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_FACE_LIB_LIST_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_FACE_ID_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_FACE_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_FACE_INFO_LIST_S& stInfo, bool bOutStruct);

    // 人流统计与人员密度检测相关
    void deal(Json::Object* pRootJson, NET_TV_PEOPLE_FLOW_RULE_LINE_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_PEOPLE_ALARM_RULE_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_PEOPLE_ALARM_CONFIG_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_STATISTICS_RESET_CONFIG_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_PEOPLE_FLOW_STATISTICS_CFG_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_PEOPLE_DENSITY_DETECTION_CFG_S& stInfo, bool bOutStruct);
};

#endif
