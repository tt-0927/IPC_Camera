/**
 * @FilePath     : action_code.h
 * @Author       : 严泽辉 (yanzeh@kfb.cn)
 * @Date         : 2024-10-06 15:23:25
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-20 14:36:09
 * @Description  : 通讯命令码
 */

#pragma once

//info /*----------------------- 对外通信默认端口 -----------------------*/
/* HTTP默认端口 */
const int OUT_HTTP_DEFAULT_PORT  = 80;
/* HTTPS默认端口 */
const int OUT_HTTPS_DEFAULT_PORT = 443;
/* RTMP默认端口 */
const int OUT_RTMP_DEFAULT_PORT  = 1935;
/* RTSP默认端口 */
const int OUT_RTSP_DEFAULT_PORT  = 554;
/* TCP默认端口 */
const int OUT_TCP_DEFAULT_PORT   = 5678;
/* UDP默认端口 */
const int OUT_UDP_DEFAULT_PORT   = 1259;

/* 外部设备通信 */
/* 烧录软件udp通信端口 */
const int OUT_BURN_MAC_PORT = 50000;

/* 内部通信 */
/* web->control 通信端口 */
const int IN_WEB_CONTROL_PROT      = 9000;
/* ui->ipc 通信端口 */
const int IN_UI_CONTROL_PROT       = 9001;
/* control->stream 通信端口 */
const int IN_CONTROL_STREAM_PROT   = 9002;
/* control->system 通信端口 */
const int IN_CONTROL_SYSTEM_PROT   = 9003;
/* control->record 录制通信端口 */
const int IN_CONTROL_RECORD_PROT   = 9004;
/* 对讲 通信端口 */
const int IN_CONTROL_TALKBACK_PROT = 9005;
/* hikvision->all 通讯端口 */
const int IN_HIKVISION_SERVER_PORT = 9006;
/* control->regIster 通信端口 */
const int IN_CONTROL_REGISTER_PROT = 9007;
/* control->SDK 通讯端口 */
const int IN_CONTROL_SDK_PROT      = 9008;
/* web->control SSL加密通信端口 */
const int IN_WEB_CONTROL_SSL_PROT  = 9009;
/* platform->control SSL加密通信端口 */
const int IN_PLATFORM_CONTROL_PROT = 9010;
/* control->upgrade */
const int IN_CONTROL_UPGRADE_PROT  = 9011;
/* control->replay */
const int IN_CONTROL_REPLAY_PROT  = 9012;
/* control->operation */
const int IN_CONTROL_OPERATION_PROT  = 9013;

/**
 * @brief record 流媒体数据端口
 */
typedef enum _RECORD_STREAM_PORT_E_
{
    /* stream 送音视频数据至 record 录制流媒体数据端口 */
    IN_RECORD_STREAM_PROT_ONE = 10001,

} RecordStreamPort_E;

// /*
//  * @brief AI AI与stream通讯解码后媒体数据信息
//  */
// typedef enum _STREAM_AI_MEDIA_PORT_E_
// {
//     /*  解码数据端口通道1数据端口 */
//     IN_STREAM_AI_MEDIA_PROT_ONE = 11000,
//     /* 解码数据端口通道2数据端口 */
//     IN_STREAM_AI_MEDIA_PROT_TWO = 11001,

//     /* 流媒体通讯端口定义32个.16个主码流，16个子码流 */
//     IN_STREAM_AI_MEDIA_CODE_END = 11031,
// } StreamAiMediaPort_E;

// /*
//  * @brief liveserver 推流数据端口根据定义的流媒体通讯个数定义多少个
//  */
// typedef enum _LIVESERVER_STREAM_PORT_E_
// {
//     /* liveserver 推流数据端口通道1数据端口 */
//     IN_PUSH_LIVESERVER_STREAM_PROT_ONE = 12000,
//     /* liveserver 推流数据端口通道2数据端口 */
//     IN_PUSH_LIVESERVER_STREAM_PROT_TWO = 12001,

// 	/* 流媒体通讯端口定义32个.16个主码流，16个子码流 */
//     IN_PUSH_LIVESERVER_STREAM_CODE_END = 12031,

// 	/*后续补充不是通道数据的，以32个推流为基础往后叠加*/
// 	/* 零通道推流 */
// 	IIN_PUSH_LIVESERVER_PGM_ZRERO_CHANNEL,
// } LiveServerStreamPort_E;

// /**
//  * @brief  国标协议内部转发流的端口
//  * @author EasonLu
//  * @note   
//  */
// typedef enum _GB28181_STREAM_PORT_E_
// {
//     /* GB28181 推流数据端口通道1数据端口 */
//     IN_PUSH_GB28181_STREAM_PROT_START = 13000,
// } GB28181StreamPort_E;

//info /*----------------------- 命令码枚举 -----------------------*/
typedef enum
{
    /**
     * @brief 通用基础命令
     */
    /// @brief 心跳码
    AC_HEARTBIT = 996,
    /// @brief 状态码
    AC_STATUS = 997,

    /**
     * @brief 用户相关
     */
    /// @brief 用户登录
    AC_LOGIN = 1000,
    /// @brief 新增用户
    AC_ADD_USER_INFO = 1001,
    /// @brief 删除用户
    AC_DEL_USER_INFO = 1002,
    /// @brief 修改用户信息
    AC_SET_USER_INFO = 1003,
    /// @brief 获取用户信息
    AC_GET_USER_INFO = 1004,
    /// @brief 获取登录错误信息
    AC_GET_LOGIN_INFO = 1005,
    /// @brief 获取所有用户信息
    AC_GET_ALLUSER_INFO = 1006,
    /// @brief 验证管理员密码
    AC_VER_ADMIN_INFO = 1007,
    /// @brief 获取在线用户信息
    AC_GET_ONLINE_USERS = 1008,
    /// @brief 删除在线用户
    AC_DELETE_ONLINE_USERS = 1009,
    /// @brief 用户权限验证
    AC_USER_PERMISSION_AUTH = 1010,
    /// @brief 发出删除用户退出登录信息
    AC_DEL_AND_EXIT_USER = 1013,
    /// @brief 发出修改用户退出登录信息
    AC_UPDATE_AND_EXIT_USER = 1014,
    /* 发送当前在线用户ID */
    AC_LOCAL_ONLINE_USER = 1015,
    /* 更新用户状态 - 当前仅用于解锁作用 */
    AC_UNLOCK_USER_STATUS = 1016,
    /* 重置密码信息 */
    AC_RESET_USER_PASSWORD = 1017,


    /**
     * @brief 图像相关
     */
    /// @brief 导入摄像机配置
    AC_IMPORT_PIC_CONFIG = 2000,
    /// @brief 导出摄像机配置
    AC_EXPORT_PIC_CONFIG = 2001,
    /// @brief 获取自定义协议
    AC_GET_PIC_CUSTOM_PROTO = 2002,
    /// @brief 设置自定义协议
    AC_SET_PIC_CUSTOM_PROTO = 2003,
    /// @brief ipc的报警通知信息
    AC_NOTICE_PIC_ALARM_INFO = 2010,
    /* 获取OSD配置 */
    AC_GET_OSD_CONFIG = 2011,
    /* 设置OSD配置 */
    AC_SET_OSD_CONFIG = 2012,
    /* 获取COVER配置 */
    AC_GET_COVER_CONFIG = 2013,
    /* 设置COVER配置 */
    AC_SET_COVER_CONFIG = 2014,
    /// @brief 获取图像配置
    AC_GET_VIDEO_EFFECT_INFO = 2023,
    /// @brief 设置图像配置
    AC_SET_VIDEO_EFFECT_INFO = 2024,
    /// @brief 获取曝光参数
    AC_GET_EXPOSURE_INFO = 2025,
    /// @brief 设置曝光参数
    AC_SET_EXPOSURE_INFO = 2026,
    /// @brief 获取日夜转换参数
    AC_GET_DAY_NIGHT_INFO = 2027,
    /// @brief 设置日夜转换参数
    AC_SET_DAY_NIGHT_INFO = 2028,
    /// @brief 获取背光参数
    AC_GET_BACK_LIGHT_INFO = 2029,
    /// @brief 设置背光参数
    AC_SET_BACK_LIGHT_INFO = 2030,
    /// @brief 获取图像降噪信息
    AC_GET_NOISE_REMOVE_INFO = 2031,
    /// @brief 设置图像降噪信息
    AC_SET_NOISE_REMOVE_INFO = 2032,
    /// @brief 获取隐私遮盖信息
    AC_GET_SHELTER_INFO = 2033,
    /// @brief 设置隐私遮盖信息
    AC_SET_SHELTER_INFO = 2034,
    /// @brief 获取白平衡信息
    AC_GET_WHITE_BALANCE_INFO = 2035,
    /// @brief 设置白平衡信息
    AC_SET_WHITE_BALANCE_INFO = 2036,
    /// @brief 获取图像计划配置信息
    AC_GET_IMAGE_SCHEDULE_INFO = 2037,
    /// @brief 设置图像计划配置信息
    AC_SET_IMAGE_SCHEDULE_INFO = 2038,
    /// @brief 获取视频调整信息
    AC_GET_VIDEO_MIRROR_INFO = 2039,
    /// @brief 设置视频调整信息
    AC_SET_VIDEO_MIRROR_INFO = 2040,
    /// @brief 获取场景信息
    AC_GET_VIDEO_SCENE_INFO = 2041,
    /// @brief 设置场景信息
    AC_SET_VIDEO_SCENE_INFO = 2042,
    /// @brief 恢复默认信息
    AC_SET_PIC_DEFAULT_INFO = 2043,


    /**
     * @brief 视频配置
     */
    /* 获取视频配置 */
    AC_GET_VIDEO_CONFIG = 2100,
    /* 设置视频配置 */
    AC_SET_VIDEO_CONFIG = 2101,
    /* 获取视频能力集 */
    AC_GET_VIDEO_CAPABILITY_SET = 2102,

    /**
     * @brief 音频配置
     */
    /* 获取音频配置 */
    AC_GET_AUDIO_CONFIG = 2150,
    /* 设置音频配置 */
    AC_SET_AUDIO_CONFIG = 2151,
    /* 获取音频能力集 */
    AC_GET_AUDIO_CAPABILITY_SET = 2152,

    /**
     * @brief 视频ROI配置
     */
    /* 获取视频ROI配置 */
    AC_GET_VIDEO_ROI_CONFIG = 2200,
    /* 设置视频ROI配置 */
    AC_SET_VIDEO_ROI_CONFIG = 2201,
    /**
     * @brief 视频区域裁剪配置
     */
    /* 获取视频区域裁剪配置 */
    AC_GET_VIDEO_AREA_CROP_CONFIG = 2202,
    /* 设置视频区域裁剪配置 */
    AC_SET_VIDEO_AREA_CROP_CONFIG = 2203,
    /* 获取视频区域裁剪换算分辨率 */
    AC_GET_VIDEO_AREA_CROP_CONVERSION_RESOLUTION = 2204,

    /**
     * @brief   : 事件
     */
    /**
     * @brief   : 普通事件
     */
    /* 获取移动侦测配置 */
    AC_GET_MOTION_DETECT_INFO = 2300,
    /* 设置移动侦测配置 */
    AC_SET_MOTION_DETECT_INFO = 2301,
    /* 获取遮挡报警配置 */
    AG_GET_HIDE_ALARM_INFO = 2302,
    /* 设置遮挡报警配置 */
    AG_SET_HIDE_ALARM_INFO = 2303,
    /* 获取异常报警配置 */
    AC_GET_ANOMALY_ALARM_INFO = 2304,
    /* 设置异常报警配置 */
    AC_SET_ANOMALY_ALARM_INFO = 2305,
    /* 获取声音报警配置 */
    AC_GET_AUDIBLE_ALARM_INFO = 2306,
    /* 设置声音报警配置 */
    AC_SET_AUDIBLE_ALARM_INFO = 2307,
    /* 获取报警输入配置 */
    AC_GET_ALARM_INPUT_INFO = 2308,
    /* 设置报警输入配置 */
    AC_SET_ALARM_INPUT_INFO = 2309,
    /* 获取报警输出配置 */
    AC_GET_ALARM_OUTPUT_INFO = 2310,
    /* 设置报警输出配置 */
    AC_SET_ALARM_OUTPUT_INFO = 2311,
    /* 获取闪光报警配置 */
    AC_GET_FLASHING_LIGHT_ALARM_INFO = 2312,
    /* 设置闪光报警配置 */
    AC_SET_FLASHING_LIGHT_ALARM_INFO = 2313,
    /* 获取PIR报警配置 */
    AC_GET_PIR_ALARM_INFO = 2314,
    /* 设置PIR报警配置 */
    AC_SET_PIR_ALARM_INFO = 2315,
    /* 编辑声音报警自定义信息 */
    AC_EDIT_AUDIO_CUSTOM_INFO = 2316,
    /* 获取声音报警自定义信息 */
    AC_GET_AUDIO_CUSTOM_INFO = 2317,
    /* 设置声音报警自定义信息 */
    AC_SET_AUDIO_CUSTOM_INFO = 2318,

    /**
     * @brief   : 周界事件
     */
    /* 获取越界侦测配置 */
    AC_GET_LINE_CROSSING_DETECT_INFO = 2400,
    /* 设置越界侦测配置 */
    AC_SET_LINE_CROSSING_DETECT_INFO = 2401,
    /* 获取区域入侵侦测配置 */
    AC_GET_REGIONAL_INTRUSION_DETECT_INFO = 2402,
    /* 设置区域入侵侦测配置 */
    AC_SET_REGIONAL_INTRUSION_DETECT_INFO = 2403,
    /* 获取进入区域侦测配置 */
    AC_GET_ENTER_REGION_DETECT_INFO = 2404,
    /* 设置进入区域侦测配置 */
    AC_SET_ENTER_REGION_DETECT_INFO = 2405,
    /* 获取离开区域侦测配置 */
    AC_GET_LEAVE_REGION_DETECT_INFO = 2406,
    /* 设置离开区域侦测配置 */
    AC_SET_LEAVE_REGION_DETECT_INFO = 2407,

    /**
     * @brief   : Smart事件
     */
    /* 获取音频异常侦测配置 */
    AC_GET_AUDIO_ANOMALY_DETECT_INFO = 2500,
    /* 设置音频异常侦测配置 */
    AC_SET_AUDIO_ANOMALY_DETECT_INFO = 2501,
    /* 获取场景变更侦测配置 */
    AC_GET_SCENE_CHANGE_DETECT_INFO = 2502,
    /* 设置场景变更侦测配置 */
    AC_SET_SCENE_CHANGE_DETECT_INFO = 2503,
    /* 获取人脸侦测配置 */
    AC_GET_FACE_DETECT_INFO = 2504,
    /* 设置人脸侦测配置 */
    AC_SET_FACE_DETECT_INFO = 2505,
    /* 获取徘徊侦测配置 */
    AC_GET_LOITERING_DETECT_INFO = 2506,
    /* 设置徘徊侦测配置 */
    AC_SET_LOITERING_DETECT_INFO = 2507,
    /* 获取人员聚集侦测配置 */
    AC_GET_CROWD_GATHERING_DETECT_INFO = 2508,
    /* 设置人员聚集侦测配置 */
    AC_SET_CROWD_GATHERING_DETECT_INFO = 2509,
    /* 获取停车侦测配置 */
    AC_GET_PARKING_DETECT_INFO = 2510,
    /* 设置停车侦测配置 */
    AC_SET_PARKING_DETECT_INFO = 2511,
    /* 获取物品遗留侦测配置 */
    AC_GET_UNATTENDED_OBJECT_DETECT_INFO = 2512,
    /* 设置物品遗留侦测配置 */
    AC_SET_UNATTENDED_OBJECT_DETECT_INFO = 2513,
    /* 获取物品拿取侦测配置 */
    AC_GET_OBJECT_REMOVAL_DETECT_INFO = 2514,
    /* 设置物品拿取侦测配置 */
    AC_SET_OBJECT_REMOVAL_DETECT_INFO = 2515,
    /* 获取宠物识别配置 */
    AC_GET_PET_RECOGNITION_INFO = 2516,
    /* 设置宠物识别配置 */
    AC_SET_PET_RECOGNITION_INFO = 2517,
    /* 获取人脸抓拍配置 */
    AC_GET_FACE_CAPTURE_INFO = 2518,
    /* 设置人脸抓拍配置 */
    AC_SET_FACE_CAPTURE_INFO = 2519,
    /* 获取人脸抓拍叠加信息 */
    AC_GET_FACE_CAPTURE_OVERLAY_INFO_INFO = 2520,
    /* 设置人脸抓拍叠加信息 */
    AC_SET_FACE_CAPTURE_OVERLAY_INFO_INFO = 2521,
    /* 获取音频异常侦测实时音量 */
    AC_GET_AUDIO_ANOMALY_DETECT_CURRENT_DB = 2522,
    /* 推送人脸抓拍信息 */
    AC_PUSH_FACE_CAPTURE_INFO = 2523,
    /* 设置人脸抓拍推送信息 */
    AC_SET_PUSH_FACE_CAPTURE_INFO = 2524,
    
    /* 设置行人抓拍推送信息 */
    AC_PUSH_PERSON_CAPTURE_INFO = 2525,
    /* 设置机动车抓拍推送信息 */
    AC_PUSH_MOTORVEHICLE_CAPTURE_INFO = 2526,
    /* 设置非机动车抓拍推送信息 */
    AC_PUSH_NONMOTORVEHICLE_CAPTURE_INFO = 2527,
    
    /* 设置人脸比对配置 */
    AC_SET_FACE_COMPARE_INFO = 2528,
    /* 获取人脸比对配置 */
    AC_GET_FACE_COMPARE_INFO = 2529,

    /**
    * @brief   : 场景智能分析
    */
    /* 获取画面分析 */
    AC_GET_IMAGE_ANALYSIS_INFO = 2600,
    /* 设置画面分析 */
    AC_SET_IMAGE_ANALYSIS_INFO = 2601,
    /* 获取文字预设任务 */
    AC_GET_TEXT_PRESET_TASK_INFO = 2602,
    /* 设置文字预设任务 */
    AC_SET_TEXT_PRESET_TASK_INFO = 2603,
    /* 获取实时预警推送 */
    AC_GET_REAL_ALARM_PUSH_INFO = 2604,
    /* 设置实时预警推送 */
    AC_SET_REAL_ALARM_PUSH_INFO = 2605,
    /* 操作画面分析记录 */
    AC_OPERATE_IMAGE_ANALYSIS_RECORD = 2606,
    /* 画面分析结果返回 */
    AC_RETURN_IMAGE_ANALYSIS_RESULT = 2607,
    /* 推理分析中断停止 */
    AC_SET_IMAGE_ANALYSIS_STOP = 2608,
    /* 获取实时预警推送处理记录 */
    AC_GET_REAL_ALARM_PROCESS_INFO = 2609,

     /**
     * @brief   ：场景智能
     */
    /// @brief 获取翻越围栏配置
    AC_GET_CLIMB_FENCE_INFO = 2700,
    /// @brief 设置翻越围栏配置
    AC_SET_CLIMB_FENCE_INFO = 2701,
    /// @brief 获取离岗配置
    AC_GET_DIMISSION_INFO = 2702,
    /// @brief 设置离岗配置
    AC_SET_DIMISSION_INFO = 2703,
    /// @brief 获取违规变道配置
    AC_GET_ILLEGAL_LANE_INFO = 2704,
    /// @brief 设置违规变道配置
    AC_SET_ILLEGAL_LANE_INFO = 2705,
    /// @brief 获取逆行配置
    AC_GET_RETROGRADE_INFO = 2706,
    /// @brief 设置逆行配置
    AC_SET_RETROGRADE_INFO = 2707,
    /// @brief 获取非机动车闯入配置
    AC_GET_NONMOROT_VEHIINTRU_INFO = 2708,
    /// @brief 设置非机动车闯入配置
    AC_SET_NONMOROT_VEHIINTRU_INFO = 2709,
    /// @brief 获取应急车道占用识别配置
    AC_GET_OCCUPATION_EMERGENCY_INFO = 2710,
    /// @brief 设置应急车道占用识别配置
    AC_SET_OCCUPATION_EMERGENCY_INFO = 2711,
    /// @brief 获取行人闯入配置
    AC_GET_PEDESTRAN_INTRUSION_INFO = 2712,
    /// @brief 设置行人闯入配置
    AC_SET_PEDESTRAN_INTRUSION_INFO = 2713,

    // ============================= 其他智能事件 ============================
    /// @brief 获取烟火识别配置
    AC_GET_SMOKE_FIRE_CFG = 2714,
    /// @brief 设置烟火识别配置
    AC_SET_SMOKE_FIRE_CFG = 2715,
    /// @brief 获取道路积水检测配置
    AC_GET_ROAD_PONDING_CFG = 2716,
    /// @brief 设置道路积水检测配置
    AC_SET_ROAD_PONDING_CFG = 2717,
    /// @brief 获取垃圾暴露识别配置
    AC_GET_GARBAGE_EXPOSURE_CFG = 2718,
    /// @brief 设置垃圾暴露识别配置
    AC_SET_GARBAGE_EXPOSURE_CFG = 2719,
    /// @brief 获取井盖异常检测配置
    AC_GET_MANHOLE_COVER_ABNORMAL_CFG = 2720,
    /// @brief 设置井盖异常检测配置
    AC_SET_MANHOLE_COVER_ABNORMAL_CFG = 2721,
    /// @brief 获取睡岗识别配置
    AC_GET_SLEEP_ON_DUTY_CFG = 2722,
    /// @brief 设置睡岗识别配置
    AC_SET_SLEEP_ON_DUTY_CFG = 2723,
    /// @brief 获取电瓶车进电梯识别配置
    AC_GET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG = 2724,
    /// @brief 设置电瓶车进电梯识别配置
    AC_SET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG = 2725,
    /// @brief 获取垃圾满溢识别配置
    AC_GET_GARBAGE_OVERFLOW_CFG = 2726,
    /// @brief 设置垃圾满溢识别配置
    AC_SET_GARBAGE_OVERFLOW_CFG = 2727,
    /// @brief 获取人员倒地识别配置
    AC_GET_PERSON_FALL_DOWN_CFG = 2728,
    /// @brief 设置人员倒地识别配置
    AC_SET_PERSON_FALL_DOWN_CFG = 2729,
    /// @brief 获取施工占道识别配置
    AC_GET_CONSTRUCTION_OCCUPY_ROAD_CFG = 2730,
    /// @brief 设置施工占道识别配置
    AC_SET_CONSTRUCTION_OCCUPY_ROAD_CFG = 2731,
    /// @brief 获取拥堵识别配置
    AC_GET_CONGESTION_CFG = 2732,
    /// @brief 设置拥堵识别配置
    AC_SET_CONGESTION_CFG = 2733,
    /// @brief 获取车牌识别配置
    AC_GET_LICENSE_PLATE_RECOGNITION_CFG = 2734,
    /// @brief 设置车牌识别配置
    AC_SET_LICENSE_PLATE_RECOGNITION_CFG = 2735,
    /// @brief 获取高空安全带识别配置
    AC_GET_HIGH_ALTITUDE_SEATBELT_CFG = 2736,
    /// @brief 设置高空安全带识别配置
    AC_SET_HIGH_ALTITUDE_SEATBELT_CFG = 2737,
    /// @brief 获取安全帽识别配置
    AC_GET_SAFETY_HELMET_CFG = 2738,
    /// @brief 设置安全帽识别配置
    AC_SET_SAFETY_HELMET_CFG = 2739,
    /// @brief 获取摔倒识别配置
    AC_GET_PERSON_FALL_CFG = 2740,
    /// @brief 设置摔倒识别配置
    AC_SET_PERSON_FALL_CFG = 2741,
    /// @brief 获取玩手机识别配置
    AC_GET_PHONE_USAGE_CFG = 2742,
    /// @brief 设置玩手机识别配置
    AC_SET_PHONE_USAGE_CFG = 2743,
    /// @brief 获取抽烟识别配置
    AC_GET_SMOKING_CFG = 2744,
    /// @brief 设置抽烟识别配置
    AC_SET_SMOKING_CFG = 2745,
    /// @brief 获取明火识别配置
    AC_GET_OPEN_FLAME_CFG = 2746,
    /// @brief 设置明火识别配置
    AC_SET_OPEN_FLAME_CFG = 2747,
    /// @brief 获取黄土裸露识别配置
    AC_GET_BARE_SOIL_CFG = 2748,
    /// @brief 设置黄土裸露识别配置
    AC_SET_BARE_SOIL_CFG = 2749,
    /// @brief 获取洞口防护栏识别配置
    AC_GET_HOLE_PROTECTION_BAR_CFG = 2750,
    /// @brief 设置洞口防护栏识别配置
    AC_SET_HOLE_PROTECTION_BAR_CFG = 2751,
    /// @brief 获取反光衣识别配置
    AC_GET_REFLECTIVE_CLOTHING_CFG = 2752,
    /// @brief 设置反光衣识别配置
    AC_SET_REFLECTIVE_CLOTHING_CFG = 2753,

    /* 获取人流统计配置 */
    AC_GET_PEOPLE_FLOW_STATISTICS_INFO = 2754,
    /* 设置人流统计配置 */
    AC_SET_PEOPLE_FLOW_STATISTICS_INFO = 2755,
    /* 立即清零人流统计结果 */
    AC_CLEAR_PEOPLE_FLOW_STATISTICS_RESULT = 2756,
    /* 获取人员密度检测配置 */
    AC_GET_PEOPLE_DENSITY_DETECTION_INFO = 2757,
    /* 设置人员密度检测配置 */
    AC_SET_PEOPLE_DENSITY_DETECTION_INFO = 2758,

    /* 设置属性识别 */
    AC_SET_ATTRIBUTE_DETECT_INFO = 2798,
    /* 获取属性识别 */
    AC_GET_ATTRIBUTE_DETECT_INFO = 2799,

    /**
     * @brief 预览相关
     */
    /// @brief 获取预览信息
    AC_GET_PREVIEW_INFO = 3000,
    /// @brief 设置预览信息
    AC_SET_PREVIEW_INFO = 3001,
    /// @brief 获取采集音频信息
    AC_GET_COLLECT_AUDIO_INFO = 3002,
    /// @brief 设置对讲信息
    AC_SET_INTERCOM_INFO = 3003,
    /// @brief 获取对讲信息
    AC_GET_INTERCOM_INFO = 3004,
    /// @brief 设置广播信息
    AC_SET_BROADCAST_INFO = 3005,
    /// @brief 设置蜂鸣器报警
    AC_SET_BEEP_ALARM = 3006,
    /* 获取对讲/广播状态 */
    AC_GET_INTERCOM_AND_BROADCAST_STATUS = 3007,

    /**
     * @brief 回放相关
     */
    /// @brief 设置回放布局信息
    AC_SET_REPLAY_LAYOUT_INFO = 3100,
    /// @brief 设置回放文件
    AC_SET_REPLAY_FILE = 3101,
    /// @brief 设置回放播放/暂停
    AC_SET_REPLAY_PLAY = 3102,
    /// @brief 设置回放时间
    AC_SET_REPLAY_SEEK = 3103,
    /// @brief 设置回放速度
    AC_SET_REPLAY_SPEED = 3104,
    /// @brief 获取回放媒体信息
    AC_GET_REPLAY_MEDIA_INFO = 3105,
    /// @brief 获取回放列表
    AC_GET_REPLAY_IPC_LIST = 3106,
    /// @brief 电子放大
    AC_SET_REPLAY_DIGITAL_ZOOM = 3107,
    /// @brief 设置声音
    AC_SET_REPLAY_VOICE = 3108,
    /// @brief 设置自适应分辨率
    AC_SET_REPLAY_ADAPT_RES = 3109,
    /// @brief 增加标签
    AC_ADD_LABEL = 3110,
    /// @brief 锁定设置
    AC_SET_LOCK = 3111,
    /// @brief 播放指定文件
    AC_PLAY_FILE = 3113,
    /// @brief 获取所选通道某天的视频时间
    AC_GET_VIDEO_TIME = 3114,
    /// @brief 流媒体回放：回放信息
    AC_SET_REPLAY_INFO = 3115,
    /// @brief 流媒体回放：开始播放
    AC_SET_REPLAY_START = 3116,
    /// @brief 流媒体回放：暂停播放
    AC_SET_REPLAY_PAUSE = 3117,
    /// @brief 流媒体回放：停止播放
    AC_SET_REPLAY_STOP = 3118,

    /**
     * @brief 对讲相关
     */
    AC_STATE_TALKBACK = 3200,
    AC_TO_STREAM_TALKBACK = 3201,
    AC_FROM_STREAM_TALKBACK = 3202,
    AC_REPLAY_TALKBACK = 3203,

    /**
     * @brief  国标协议音视频数据相关
     * @author EasonLu
     */
    AC_GB28181_CODEC_INFO = 3300,
    AC_GB28181_VIDEO = 3301,
    AC_GB28181_AUDIO = 3302,
    /// @brief 通知指定通道开启GB的发流
    AC_GB28181_START_SEND = 3303,
    /// @brief 通知指定通道关闭GB的发流
    AC_GB28181_STOP_SEND = 3304,

    /**
     * @brief 系统配置相关
     */
    /// @brief 获取基本信息
    AC_GET_DEVICE_INFO = 4000,
    /// @brief 修改基本信息
    AC_SET_DEVICE_INFO = 4001,
    /// @brief 获取时间配置信息
    AC_GET_TIME_INFO = 4002,
    /// @brief 修改时间配置信息
    AC_SET_TIME_INFO = 4003,
    /// @brief 获取当前时间
    AC_GET_NOW_TIME = 4004,
    /// @brief ntp服务器测试
    AC_TEST_NTPSERVER = 4005,
    /// @brief 获取232串口配置信息
    AC_GET_SERIAL_INFO = 4006,
    /// @brief 修改232串口配置信息
    AC_SET_SERIAL_INFO = 4007,
    /// @brief 获取本机配置
    AC_GET_DEVICE_CONFIG = 4008,
    /// @brief 修改本机配置
    AC_SET_DEVICE_CONFIG = 4009,
    /// @brief 获取外设设置配置信息
    AC_GET_PERIPHERAL_INFO = 4010,
    /// @brief 设置外设设置配置信息
    AC_SET_PERIPHERAL_INFO = 4011,
    /// @brief 获取485串口配置信息
    AC_GET_485_SERIAL_INFO = 4028,
    /// @brief 修改485串口配置信息
    AC_SET_485_SERIAL_INFO = 4029,
    /// @brief 获取PC端的插件配置信息
    AC_GET_WEB_PLUGIN_PARAM = 4030,
    /// @brief 修改PC端的插件配置信息
    AC_SET_WEB_PLUGIN_PARAM = 4031,
    /* 获取智能资源分配-智能事件启用情况 */
    AC_GET_SMART_EVENT_ENABLE_STATUS = 4032,
    /* 设置智能资源分配-智能事件启用情况 */
    AC_SET_SMART_EVENT_ENABLE_STATUS = 4033,
    /* 获取Metadata配置 */
    AC_GET_METADATA_CONFIG = 4034,
    /* 设置Metadata配置 */
    AC_SET_METADATA_CONFIG = 4035,

    /**
     * @brief 系统运维相关
     */
    /// @brief 系统重启
    AC_REBOOT = 5000,
    /// @brief 系统简单重置
    AC_RESET_SIMPLE = 5001,
    /// @brief 系统完全重置
    AC_RESET_COMPLETELY = 5002,
    /// @brief 导出设备参数
    AC_EXPORT_DEVICE_PARAM = 5003,
    /// @brief 导入设备参数
    AC_IMPORT_DEVICE_PARAM = 5004,
    /// @brief 自动维护通知
    AC_NOTICE_AUTO_MAINTAIN = 5005,
    /// @brief 系统升级
    AC_DO_UPGRADE = 5006,
    /// @brief 导入/设置升级包
    AC_SET_UPGRADE = 5007,
    /// @brief 系统升级检测
    AC_CHECK_UPGRADE = 5008,
    /// @brief 设置升级维护
    AC_SET_UPGRADE_MAINTAIN = 5009,
    /// @brief 获取升级维护配置
    AC_GET_UPGRADE_MAINTAIN = 5010,
    /// @brief 查找日志
    AC_FIND_LOG = 5011,
    /// @brief 导出日志
    AC_EXPORT_LOG = 5012,
    /// @brief 获取安全服务配置
    AC_GET_SECURITY_SERVICES_INFO = 5013,
    /// @brief 设置安全服务配置
    AC_SET_SECURITY_SERVICES_INFO = 5014,
    /// @brief 系统升级状态
    AC_GET_UPGRADE_STATUS = 5015,
    /// @brief 获取日志服务器
    AC_GET_LOG_SERVER = 5016,
    /// @brief 设置日志服务器
    AC_SET_LOG_SERVER = 5017,
    /// @brief 日志服务器测试
    AC_TEST_LOG_SERVER = 5018,
    /// @brief 运维平台上传日志
    AC_UPLOAD_OPERATION_LOG = 5019,
    /// @brief 获取ssh剩余运行时长
    AC_GET_SSH_COUNTDOWN = 5020,
    /// @brief 下载升级包
    AC_DOWNLOAD_GRADEPACK = 5033,
    /// @brief ip地址过滤配置
    AC_GET_IP_FILTER_INFO = 5034,
    AC_SET_IP_FILTER_INFO = 5035,
    AC_ADD_IP_FILTER_ADDRESS = 5036,
    AC_REMOVE_IP_FILTER_ADDRESS = 5037,
    AC_MODIFY_IP_FILTER_ADDRESS = 5038,

    /**
     * @brief 网络配置相关
     */
    /// @brief 网络配置
    AC_GET_NETWORK_INFO = 6000,
    AC_SET_NETWORK_INFO = 6001,
    /// @brief 端口配置
    AC_GET_PORT_INFO = 6002,
    AC_SET_PORT_INFO = 6003,
    /// @brief 端口映射配置
    AC_GET_PORT_MAP_INFO = 6004,
    AC_SET_PORT_MAP_INFO = 6005,
    /// @brief DDNS配置
    AC_GET_DDNS_INFO = 6006,
    AC_SET_DDNS_INFO = 6007,
    /// @brief PPPOE配置
    AC_GET_PPPOE_INFO = 6008,
    AC_SET_PPPOE_INFO = 6009,
    /// @brief NTP配置
    // AC_GET_NTP_INFO = 6010,
    // AC_SET_NTP_INFO = 6011,
    /// @brief 日志服务器配置
    AC_GET_LOG_SERVER_INFO = 6012,
    AC_SET_LOG_SERVER_INFO = 6013,
    /// @brief 其他基本配置--没有这个
    AC_GET_OHTER_BASE_INFO = 6014,
    AC_SET_OHTER_BASE_INFO = 6015,
    /// @brief SNMP配置
    AC_GET_SNMP_INFO = 6016,
    AC_SET_SNMP_INFO = 6017,
    /// @brief Email配置
    AC_GET_EAMIL_INFO = 6018,
    AC_SET_EAMIL_INFO = 6019,
    /// @brief GB28181接入配置
    AC_GET_GB28181_INFO = 6020,
    AC_SET_GB28181_INFO = 6021,
    /// @brief GB28181服务配置
    AC_GET_GB28181_SERVER_INFO = 6022,
    AC_SET_GB28181_SERVER_INFO = 6023,
    /// @brief 其他高级配置
    AC_GET_OHTER_SENIOR_INFO = 6024,
    AC_SET_OHTER_SENIOR_INFO = 6025,
    /// @brief 集成协议配置
    AC_GET_INTEGRATION_PROTO_INFO = 6026,
    AC_SET_INTEGRATION_PROTO_INFO = 6027,
    /// @brief Email测试
    AC_TEST_EAMIL = 6028,
    /// @brief 发送事件联动邮件
    AC_EVENT_EAMIL = 6029,
    /// @brief 获取受信任证书信息
    AC_GET_TRUST_CA_INFO = 6030,
    /// @brief 安装受信任的证书
    AC_INSTALL_TRUST_CA_FILE = 6031,
    /// @brief 删除受信任的证书
    AC_DELETE_TRUST_CA_FILE = 6032,
    /// @brief 下载受信任的证书
    AC_DOWNLOAD_TRUST_CA_FILE = 6033,
    /// @brief 获取设备证书信息
    AC_GET_DEVICE_CA_INFO = 6034,
    /// @brief 安装设备证书
    AC_INSTALL_DEVICE_CA_FILE = 6035,
    /// @brief 删除设备证书
    AC_DELETE_DEVICE_CA_FILE = 6036,
    /// @brief 下载设备证书
    AC_DOWNLOAD_DEVICE_CA_FILE = 6037,
    /// @brief 创建证书请求文件
    AC_CREATE_REQUEST_CA_FILE = 6038,
    /// @brief 创建设备证书并安装
    AC_CREATE_INSTALL_DEVICE_CA_FILE = 6039,
    /// @brief 删除证书请求文件
    AC_DELETE_REQUEST_CSR_FILE = 6040,
    /// @brief 获取https配置信息
    AC_GET_HTTPS_INFO = 6041,
    /// @brief 配置https
    AC_CONFIG_HTTPS_INFO = 6042,
    /// @brief 获取认证方式
    AC_GET_AUTH_METHOD = 6043,
    /// @brief 设置认证方式
    AC_SET_AUTH_METHOD = 6044,
    /// @brief 检查mac地址是否有效
    AC_GET_CHECK_MAC_VALID = 6049,
    /// @brief 添加onvif用户
    // AC_ADD_ONVIF_USER = 6050,
    /// @brief 删除onvif用户
    // AC_DEL_ONVIF_USER = 6051,
    /// @brief 修改onvif用户
    // AC_UPDATE_ONVIF_USER = 6052,
    /// @brief 查找onvif用户
    // AC_FIND_ONVIF_USER = 6053,
    /// @brief 获取所有onvif用户
    // AC_GET_ALL_ONVIF_USER = 6054,
    /// @brief 获取onvif配置信息
    AC_GET_ONVIF_INFO = 6055,
    /// @brief 设置onvif配置信息
    AC_SET_ONVIF_INFO = 6056,

    /// @brief qos配置
    AC_GET_QOS_INFO = 6057,
    AC_SET_QOS_INFO = 6058,
    /// @brief bonjour配置
    AC_GET_BONJOUR_INFO = 6059,
    AC_SET_BONJOUR_INFO = 6060,

    /**
    * @brief   : 国际证书管理（国密）
    */
    /* 创建证书请求文件 */
    AC_GM_CREATE_CERT_REQUEST_FILE = 6061,
    /* 上传CA证书 */
    AC_GM_UPLOAD_CA_CERT = 6062,
    /* 上传本地设备证书 */
    AC_GM_UPLOAD_DEVICE_CERT = 6063,
    /* 上传CRL文件 */
    AC_GM_UPLOAD_CRL_FILE = 6064,
    /* 获取当前证书信息 */
    AC_GM_GET_CERT_INFO = 6065,
    /* 删除证书文件 */
    AC_GM_DELETE_CERT_FILE = 6066,

#ifdef ENABLE_GAT1400_SRC
    /// @brief 获取GAT1400配置信息
    AC_GET_GAT1400_INFO = 6067,
    /// @brief 设置GAT1400配置信息
    AC_SET_GAT1400_INFO = 6068,
#endif

    /// @brief wifi配置
    AC_SET_CONFIG_WIFI_STA = 6069,
    AC_CONNECT_WIFI_STA = 6070,
    AC_DISCONNECT_WIFI_STA = 6071,

    /// @brief 4G配置
    AC_GET_4G_INFO = 6072,
    AC_SET_4G_INFO = 6073,

    /// @brief 热点配置
    AC_SET_HOTSPOT_INFO = 6074,

    AC_GET_HOTSPOT_CONN = 6075,


    /**
     * @brief 数据检索/事件相关
     */

    /// @brief 告警触发调试，测试用
    AC_TEST_TRIGGER_ALARM = 6999,
    /// @brief 视频检索
    AC_SEARCH_BY_RECORD_TYPE = 7000,
    /// @brief 事件检索
    AC_SEARCH_BY_EVENT_TYPE = 7001,
    /// @brief 车辆检索
    AC_SEARCH_BY_VEHICLE_INFO = 7002,
    /// @brief 根据通道号检索
    AC_SEARCH_BY_CHANNEL_ID = 7003,
    /// @brief 根据日期检索
    AC_SEARCH_BY_DATE = 7004,
    /// @brief 根据时间检索
    AC_SEARCH_BY_TIME = 7005,
    /// @brief 根据图片检索
    AC_SEARCH_BY_IMAGE = 7006,
    /// @brief 根据人脸检索
    AC_SEARCH_BY_PERSON_FACE = 7007,
    /// @brief 人脸比对检索
    AC_SEARCH_FACE_COMPARE = 7008,
    /// @brief 根据图片进行人脸比对
    AC_FACE_COMPARE_BY_IMAGE = 7009,
    /// @brief 标签检索
    AC_SEARCH_BY_LABLE = 7010,
    /// @brief 两张图片人脸比对
    AC_FACE_COMPARE_BY_2IMAGE = 7011,
    /* 检索录制的ts文件 */
    AC_SEARCH_BY_RECORD_TS_TYPE = 7012,

    /* 下载图片文件信息 */
    AC_DOWNLOAD_IMAGE_FILE_INFO = 7013,

    /// @brief 触发检测事件
    AC_AREA_DETECT_EVENT = 7030,
    /// @brief 触发人脸检测事件
    AC_FACE_DETECT_EVENT = 7031,
    /// @brief 获取目标事件数据
    AC_GET_TARGET_EVENT_DATA = 7032,
    /// @brief 获取目标事件数据2, 跟上面的一模一样，网页不能用同一个码
    AC_GET_TARGET_EVENT_DATA2 = 7033,
    /// @brief 获取快捷入口
    AC_GET_QUICK_ENTRY = 7034,
    /// @brief 删除快捷入口
    AC_DEL_QUICK_ENTRY = 7035,

    /// @brief 更新周界规则信息
    AC_UPDATE_RULTINFOS = 7040,

    /**
     * @brief 算法相关
     */
    /// @brief 清除规则
    AC_CLEAR_RULE = 7100,
    /// @brief 设置移动侦测、区域入侵、越界侦测等规则
    AC_SET_EVENT_RULE = 7101,
    /// @brief 获取规则
    AC_GET_EVENT_RULE = 7102,
    /// @brief 周界事件智能模式
    AC_GET_PERIMETER_EVENT_MODE = 7103,
    AC_SET_PERIMETER_EVENT_MODE = 7104,
    /// @brief 获取ipc智能信息
    AC_GET_SMART_EVENT_INFO = 7105,
    /// @brief 清除ipc智能事件
    AC_CLEAR_SMART_EVENT = 7106,
    /// @brief 设置算法配置
    AC_SET_ALGORITHM_CONFIG = 7107,
    /// @brief 设置全部算法配置
    AC_SET_ALL_ALGORITHM_CONFIG = 7108,
    /// @brief 获取全部算法配置
    AC_GET_ALL_ALGORITHM_CONFIG = 7109,

    /// @brief 添加目标库
    AC_ADD_TARGET_LIB = 7200,
    /// @brief 删除目标库
    AC_DEL_TARGET_LIB = 7201,
    /// @brief 修改目标库
    AC_SET_TARGET_LIB = 7202,
    /// @brief 获取目标库
    AC_GET_TARGET_LIB = 7203,
    /// @brief 添加人脸
    AC_ADD_FACE_INFO = 7204,
    /// @brief 删除人脸
    AC_DEL_FACE_INFO = 7205,
    /// @brief 修改人脸
    AC_SET_FACE_INFO = 7206,
    /// @brief 获取人脸
    AC_GET_FACE_INFO = 7207,

#ifdef ENABLE_AI_STUDENT
    /**
     * @brief 无感考勤相关
     */
    /// @brief 获取班级信息
    AC_GET_CLASS_INFO = 7300,
    /// @brief 设置班级信息
    AC_SET_CLASS_INFO = 7301,
    /// @brief 获取考勤信息
    AC_GET_ATTENDANCE_INFO = 7302,
    /// @brief 获取学生行为信息
    AC_GET_STUDENT_BEHAVIOR_INFO = 7303,
    /// @brief 获取学生课堂表现信息
    AC_GET_STUDENT_PERFORMANCE_INFO = 7304,
#endif // ENABLE_AI_STUDENT

    /**
     * @brief 录制相关
     */
    /// @brief 录制设置
    AC_CONTROL_RECORD_INFO = 8000,
    /// @brief 录制视频信息设置
    AC_CONTROL_VIDEO_CONFIG = 8001,
    /// @brief 录制音频信息设置
    AC_CONTROL_AUDIO_CONFIG = 8002,
    /// @brief 录制流媒体视频数据
    AC_STREAM_VIDEO_DATE = 8003,
    /// @brief 录制流媒体音频数据
    AC_STREAM_AUDIO_DATE = 8004,
    /// @brief 通知录制文件信息
    AC_NOTICE_RECORD_FILE_INFO = 8005,
    /// @brief 删除录制文件信息
    AC_DEL_RECORD_FILE_INFO = 8006,
    /// @brief 修改录制文件信息
    AC_SET_RECORD_FILE_INFO = 8007,
    /// @brief 查找录制文件信息
    AC_FIND_RECORD_FILE_INFO = 8008,
    /// @brief 通知ts录制文件信息
    AC_NOTICE_RECORD_TS_FILE_INFO = 8009,
    /// @brief 通知录制异常
    AC_NOTICE_RECORD_EXCEPTION = 8010,
    /// @brief 人为录制
    AC_GET_HUMAN_RECORD = 8011,
    AC_SET_HUMAN_RECORD = 8012,
    /// @brief 下载录制文件
    AC_DOWNLOAD_RECORD_FILE = 8013,
    /// @brief 通知下载录制文件进度
    AC_NOTICE_DOWNLOAD_RECORD_PROGRESS = 8014,

    /// @brief 获取高级录制参数
    AC_GET_RECORD_ADVANCED_PARAM = 8100,
    /// @brief 设置高级录制参数
    AC_SET_RECORD_ADVANCED_PARAM = 8101,
    /// @brief 获取录制计划
    AC_GET_RECORD_SCHEDULE = 8102,
    /// @brief 设置录制计划
    AC_SET_RECORD_SCHEDULE = 8103,
    /// @brief 获取音视频参数能力
    // AC_GET_COMPRESS_ABILITY = 8104,
    /// @brief 获取当前音视频信息
    // AC_GET_COMPRESS_CURR_INFO = 8105,
    /// @brief 设置当前音视频信息
    // AC_SET_COMPRESS_CURR_INFO = 8106,
    /// @brief 获取假日信息
    // AC_GET_HOLIDAY_INFO = 8107,
    /// @brief 设置假日信息
    // AC_SET_HOLIDAY_INFO = 8108,
    /// @brief 获取录制配置其他信息
    // AC_GET_RECORD_OTHER_INFO = 8109,
    /// @brief 设置录制配置其他信息
    // AC_SET_RECORD_OTHER_INFO = 8110,
    /// @brief 获取所有通道的录制状态
    AC_GET_RECORD_STATUS = 8111,

    /**
     * @brief 抓图相关
     */
    /* 获取抓图计划 */
    AC_GET_CAPTURE_PLAN_INFO = 8200,
    /* 设置抓图计划 */
    AC_SET_CAPTURE_PLAN_INFO = 8201,
    /* 获取抓图参数 */
    AC_GET_CAPTURE_PARAM_INFO = 8202,
    /* 设置抓图参数 */
    AC_SET_CAPTURE_PARAM_INFO = 8203,

    /**
     * @brief 存储管理相关
     */
    /* 获取存储管理参数 */
    AC_GET_STORAGE_MANAGE_INFO = 8300,
    /* 设置存储管理参数 */
    AC_SET_STORAGE_MANAGE_INFO = 8301,
    /* 格式化sd卡 */
    AC_INIT_SD_CARD = 8302,

    /**
     * @brief 设备激活相关
     */
    /// @brief 获取设备激活信息
    AC_GET_REGISTER_INFO = 9000,
    /// @brief 设置注册码
    AC_SET_REGISTRATION_CODE = 9001,
    /// @brief 设置激活密码
    AC_SET_ACTIVATIONPWD = 9004,
    /// @brief 获取时间
    AC_REGISTER_GET_TIMEINFO = 9006,
    /// @brief 手动配置时间
    AC_MANUAL_TIMECONFIG = 9007,
    /// @brief 手动配置网络
    AC_MANUAL_NETWORKCONFIG = 9008,
    /// @brief 获取用户激活信息
    AC_GET_ACTIVATIONINFO = 9009,
    /// @brief 自动配置网络
    AC_AUTO_CONFIG_NETWORK = 9010,

} ActionCode_E;
