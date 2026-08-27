/*** 
* @FilePath     : log_define.h
* @Author       : zhangjc (zhangjc@kfb.cn)
* @Date         : 2025-02-11 16:16:21
* @LastEditors  : huangjunda
* @LastEditTime : 2025-08-28 16:05:56
* @Description  : 日志定义文件
*/

#pragma once

#include <string>

#include "event_define.h"

namespace Log
{
    /**
    * @brief 日志主类型
    */
    typedef enum Type
    {
        ALL        = 0, /**< 所有日志类型 */
        ALARM      = 1, /**< 报警日志 */
        EXCEPTION  = 2, /**< 异常日志 */
        OPERATION  = 3, /**< 操作日志 */
        INFOMATION = 4  /**< 信息日志 */
    } Type_E;
    inline std::string to_string(Type_E type)
    {
        switch (type)
        {
        case ALL:
            return "全部";
        case ALARM:
            return "报警";
        case EXCEPTION:
            return "异常";
        case OPERATION:
            return "操作";
        case INFOMATION:
            return "信息";
        default:
            return "未知";
        }
    }
    /**
    * @brief 日志次类型
    */
    typedef enum Action
    {
        UNDEFINED                  = 0,    /**< 未知 */

        /* --------------------报警日志类型(1000~1999)-------------------- */
        /* 普通事件(1000~1099) */
        MOTION_DETECT_START     = 1000, /**< 移动侦测开始 */
        MOTION_DETECT_STOP      = 1001, /**< 移动侦测结束 */
        OCCLUSION_DETECT_START  = 1002, /**< 遮挡侦测开始 */
        OCCLUSION_DETECT_STOP   = 1003, /**< 遮挡侦测结束 */
        ANOMALY_ALARM_START     = 1004, /**< 异常报警开始 */
        ANOMALY_ALARM_STOP      = 1005, /**< 异常报警结束 */
        AUDIO_ALARM_START       = 1006, /**< 声音报警开始 */
        AUDIO_ALARM_STOP        = 1007, /**< 声音报警结束 */
        ALARM_INPUT_START       = 1008, /**< 报警输入开始 */
        ALARM_INPUT_STOP        = 1009, /**< 报警输入结束 */
        ALARM_OUTPUT_START      = 1010, /**< 报警输出开始 */
        ALARM_OUTPUT_STOP       = 1011, /**< 报警输出结束 */
        FLASH_ALARM_START       = 1012, /**< 闪光灯报警开始 */
        FLASH_ALARM_STOP        = 1013, /**< 闪光灯报警结束 */
        PIR_ALARM_START         = 1014, /**< PIR 红外感应报警开始 */
        PIR_ALARM_STOP          = 1015, /**< PIR 红外感应报警结束 */
        /* 周界事件(1100~1199) */
        LINE_CROSSING_START     = 1100, /**< 越界侦测开始 */
        LINE_CROSSING_STOP      = 1101, /**< 越界侦测结束 */
        INTRUSION_START         = 1102, /**< 区域入侵侦测开始 */
        INTRUSION_STOP          = 1103, /**< 区域入侵侦测结束 */
        ENTER_REGION_START      = 1104, /**< 进入区域侦测开始 */
        ENTER_REGION_STOP       = 1105, /**< 进入区域侦测结束 */
        LEAVE_REGION_START      = 1106, /**< 离开区域侦测开始 */
        LEAVE_REGION_STOP       = 1107, /**< 离开区域侦测结束 */
        /* Smart事件(1200~1299) */
        AUDIO_ANOMALY_START     = 1200, /**< 音频输入异常开始 */
        AUDIO_ANOMALY_STOP      = 1201, /**< 音频输入异常结束 */
        SCENE_CHANGE_START      = 1202, /**< 场景变更侦测开始 */
        SCENE_CHANGE_STOP       = 1203, /**< 场景变更侦测结束 */
        FACE_DETECT_START       = 1204, /**< 人脸侦测开始 */
        FACE_DETECT_STOP        = 1205, /**< 人脸侦测结束 */
        LOITERING_DETECT_START  = 1206, /**< 徘徊侦测开始 */
        LOITERING_DETECT_STOP   = 1207, /**< 徘徊侦测结束 */
        CROWD_GATHERING_START   = 1208, /**< 人员聚集侦测开始 */
        CROWD_GATHERING_STOP    = 1209, /**< 人员聚集侦测结束 */
        PARKING_DETECT_START    = 1210, /**< 停车侦测开始 */
        PARKING_DETECT_STOP     = 1211, /**< 停车侦测结束 */
        UNATTENDED_OBJECT_START = 1212, /**< 物品遗留侦测开始 */
        UNATTENDED_OBJECT_STOP  = 1213, /**< 物品遗留侦测结束 */
        OBJECT_REMOVAL_START    = 1214, /**< 物品拿取侦测开始 */
        OBJECT_REMOVAL_STOP     = 1215, /**< 物品拿取侦测结束 */
        PET_RECOGNITION_START   = 1216, /**< 宠物识别侦测开始 */
        PET_RECOGNITION_STOP    = 1217, /**< 宠物识别侦测结束 */
        FACE_CAPTURE_START      = 1218, /**< 人脸抓拍侦测开始 */
        FACE_CAPTURE_STOP       = 1219, /**< 人脸抓拍侦测结束 */
#ifdef SCENE_INTELLIGENCE
        // ========== 行为监管 ==========
        SLEEP_ON_DUTY_START     = 1220, /**< 睡岗识别开始 */
        SLEEP_ON_DUTY_STOP      = 1221, /**< 睡岗识别结束 */
        LEAVE_POST_START        = 1222, /**< 离岗识别开始 */
        LEAVE_POST_STOP         = 1223, /**< 离岗识别结束 */
        PERSON_FALL_DOWN_START  = 1224, /**< 人员倒地识别开始 */
        PERSON_FALL_DOWN_STOP   = 1225, /**< 人员倒地识别结束 */
        FENCE_CLIMBING_START    = 1226, /**< 翻越围栏识别开始 */
        FENCE_CLIMBING_STOP     = 1227, /**< 翻越围栏识别结束 */
        SMOKING_START           = 1228, /**< 抽烟识别开始 */
        SMOKING_STOP            = 1229, /**< 抽烟识别结束 */
        PHONE_USAGE_START       = 1230, /**< 玩手机识别开始 */
        PHONE_USAGE_STOP        = 1231, /**< 玩手机识别结束 */
        SMOKE_FIRE_START        = 1234, /**< 烟火识别开始 */
        SMOKE_FIRE_STOP         = 1235, /**< 烟火识别结束 */
        OPEN_FLAME_START        = 1236, /**< 明火识别开始 */
        OPEN_FLAME_STOP         = 1237, /**< 明火识别结束 */
        BARE_SOIL_START         = 1240, /**< 黄土裸露识别开始 */
        BARE_SOIL_STOP          = 1241, /**< 黄土裸露识别结束 */
        PERSON_TRIP_STAR        = 1242, /**< 摔倒识别开始 */
        PERSON_TRIP_STOP        = 1243, /**< 摔倒识别结束 */
        ELECTRIC_VEHICLE_IN_ELEVATOR_START  = 1244, /**< 电瓶车进电梯识别开始 */
        ELECTRIC_VEHICLE_IN_ELEVATOR_STOP   = 1245, /**< 电瓶车进电梯识别结束 */
        MANHOLE_COVER_ABNORMAL_START        = 1246, /**< 井盖异常检测开始 */
        MANHOLE_COVER_ABNORMAL_STOP         = 1247, /**< 井盖异常检测结束 */
        HOLE_PROTECTION_BAR_START           = 1248, /**< 洞口防护栏识别开始 */
        HOLE_PROTECTION_BAR_STOP            = 1249, /**< 洞口防护栏识别结束 */
        PEDESTRIAN_INTRUSION_START          = 1250, /**< 行人闯入识别开始 */
        PEDESTRIAN_INTRUSION_STOP           = 1251, /**< 行人闯入识别结束 */
        // ========== 穿戴规范 ==========
        SAFETY_HELMET_START                 = 1252, /**< 安全帽识别开始 */
        SAFETY_HELMET_STOP                  = 1253, /**< 安全帽识别结束 */
        REFLECTIVE_CLOTHING_START           = 1254, /**< 反光衣识别开始 */
        REFLECTIVE_CLOTHING_STOP            = 1255, /**< 反光衣识别结束 */
        HIGH_ALTITUDE_SEATBELT_START        = 1256, /**< 高空安全带识别开始 */
        HIGH_ALTITUDE_SEATBELT_STOP         = 1257, /**< 高空安全带识别结束 */
        // ========== 交通行为监管 ==========
        CONSTRUCTION_OCCUPY_ROAD_START      = 1258, /**< 施工占道识别开始 */
        CONSTRUCTION_OCCUPY_ROAD_STOP       = 1259, /**< 施工占道识别结束 */
        EMERGENCY_LANE_OCCUPANCY_START      = 1260, /**< 应急车道占用识别开始 */
        EMERGENCY_LANE_OCCUPANCY_STOP       = 1261, /**< 应急车道占用识别结束 */
        REVERSE_DIRECTION_START             = 1262, /**< 逆行识别开始 */
        REVERSE_DIRECTION_STOP              = 1263, /**< 逆行识别结束 */
        NON_MOTOR_VEHICLE_INTRUSION_START   = 1264, /**< 非机动车闯入识别开始 */
        NON_MOTOR_VEHICLE_INTRUSION_STOP    = 1265, /**< 非机动车闯入识别结束 */
        ROAD_PONDING_START                  = 1266, /**< 道路积水识别开始 */
        ROAD_PONDING_STOP                   = 1267, /**< 道路积水识别结束 */
        CONGESTION_START                    = 1268, /**< 拥堵识别开始 */
        CONGESTION_STOP                     = 1269, /**< 拥堵识别结束 */
        ILLEGAL_LANE_CHANGE_START           = 1270, /**< 违规变道识别开始 */
        ILLEGAL_LANE_CHANGE_STOP            = 1271, /**< 违规变道识别结束 */
        // ========== 属性识别 ==========
        PLATE_NUMBER_START                  = 1272, /**< 车牌识别开始 */
        PLATE_NUMBER_STOP                   = 1273, /**< 车牌识别结束 */
#endif
#ifdef SCENE_INTELLIGENT_ANALYSIS
        // ========== 大模型场景智能分析 ==========
        TEXT_PRESET_ALARM                  = 1274, /**< 场景智能分析-文字预设报警 */
#endif

#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT
        // info ========== 垃圾识别 ==========
        GARBAGE_EXPOSURE_START  = 1232, /**< 垃圾暴露识别开始 */
        GARBAGE_EXPOSURE_STOP   = 1233, /**< 垃圾暴露识别结束 */
        GARBAGE_OVERFLOW_START  = 1238, /**< 垃圾溢满识别开始 */
        GARBAGE_OVERFLOW_STOP   = 1239, /**< 垃圾溢满识别结束 */
#endif

#if defined CAP_AI_FACE_COMPARE
        // info ========== 人脸比对 ==========
        FACE_COMPARE_SUCCESS_START  = 1275, /**< 人脸比对成功开始 */
        FACE_COMPARE_SUCCESS_STOP   = 1276, /**< 人脸比对成功结束 */
        FACE_COMPARE_FAIL_START  = 1277, /**< 人脸比对失败开始 */
        FACE_COMPARE_FAIL_STOP   = 1278, /**< 人脸比对失败结束 */
#endif

#if CAP_AI_PEOPLE_STATISTICS
        // info ========== 人数统计 ==========
        PEOPLE_FLOW_STATISTICS_START = 1279,   /**< 人流统计开始 */
        PEOPLE_FLOW_STATISTICS_STOP = 1280,    /**< 人流统计结束 */
        PEOPLE_DENSITY_DETECTION_START = 1281, /**< 人员密度检测开始 */
        PEOPLE_DENSITY_DETECTION_STOP = 1282,  /**< 人员密度检测结束 */
        PEOPLE_FLOW_STAY_NORMAL_START = 1283,  /**< 人流统计-滞留人数普通报警开始 */
        PEOPLE_FLOW_STAY_NORMAL_STOP = 1284,   /**< 人流统计-滞留人数普通报警结束 */
        PEOPLE_FLOW_STAY_MEDIUM_START = 1285,  /**< 人流统计-滞留人数中度报警开始 */
        PEOPLE_FLOW_STAY_MEDIUM_STOP = 1286,   /**< 人流统计-滞留人数中度报警结束 */
        PEOPLE_FLOW_STAY_SEVERE_START = 1287,  /**< 人流统计-滞留人数严重报警开始 */
        PEOPLE_FLOW_STAY_SEVERE_STOP = 1288,   /**< 人流统计-滞留人数严重报警结束 */
        PEOPLE_DENSITY_NORMAL_START = 1289,    /**< 人员密度检测-普通报警开始 */
        PEOPLE_DENSITY_NORMAL_STOP = 1290,     /**< 人员密度检测-普通报警结束 */
        PEOPLE_DENSITY_MEDIUM_START = 1291,    /**< 人员密度检测-中度报警开始 */
        PEOPLE_DENSITY_MEDIUM_STOP = 1292,     /**< 人员密度检测-中度报警结束 */
        PEOPLE_DENSITY_SEVERE_START = 1293,    /**< 人员密度检测-严重报警开始 */
        PEOPLE_DENSITY_SEVERE_STOP = 1294,     /**< 人员密度检测-严重报警结束 */
#endif

        /* 剩余部分报警日志需实现后在这里添加 */
        /* --------------------报警日志类型(1000~1999)-------------------- */

        /* --------------------异常日志类型(2000~2999)-------------------- */
        /* 硬盘异常(2000~2099) */
        DISK_FULL                  = 2000, /**< 硬盘满 */
        DISK_ERROR                 = 2001, /**< 硬盘错误 */
        DATA_DISK_ERROR            = 2002, /**< 数据盘错误 */
        DISK_HIGH_TEMP             = 2003, /**< 硬盘高温 */
        DISK_LOW_TEMP              = 2004, /**< 硬盘低温 */
        DISK_BAD_BLOCK             = 2005, /**< 硬盘坏块 */
        DISK_SHOCK                 = 2006, /**< 硬盘撞击 */
        DISK_CRITICAL_FAILURE      = 2007, /**< 硬盘严重故障 */
        DISK_SHM_ABNORMAL          = 2008, /**< 硬盘SHM检测异常 */
        CLUSTER_DISK_FULL          = 2009, /**< 集群硬盘满 */
        LOG_DISK_ABNORMAL          = 2010, /**< 日志盘异常 */
        SSD_ABNORMAL               = 2011, /**< SSD异常 */
        SSD_TEMP_ABNORMAL          = 2012, /**< SSD温度异常 */
        SSD_LIFE_ABNORMAL          = 2013, /**< SSD寿命异常 */
        /* 网络通信异常(2100~2199) */
        NET_BROKEN                 = 2100, /**< 网络断开 */
        IP_CONFLICT                = 2101, /**< IP冲突 */
        IPC_IP_CONFLICT            = 2102, /**< IPC IP地址冲突 */
        IPC_OFFLINE                = 2103, /**< IP Camera未链接 */
        DEVICE_OFFLINE             = 2104, /**< 设备掉线 */
        SUBSTREAM_CONNECT_FAIL     = 2105, /**< ip通道子码流连接失败 */
        PORT_ABNORMAL              = 2106, /**< 端口异常 */
        CLOUD_STORAGE_ABNORMAL     = 2107, /**< 云存储异常 */
        DATA_UPLOAD_ABNORMAL       = 2108, /**< 数据上传异常 */
        TIME_SYNC_ABNORMAL         = 2109, /**< 时间同步异常 */
        /* 电源与硬件(2200~2299) */
        POE_POWER_ABNORMAL         = 2200, /**< PoE供电异常 */
        IPC_REBOOT_ABNORMAL        = 2201, /**< IPC异常重启 */
        POC_ABNORMAL               = 2202, /**< POC模块异常 */
        HARDWARE_ABNORMAL          = 2203, /**< 配件版异常 */
        HOT_STANDBY_ABNORMAL       = 2204, /**< 热备异常 */
        /* 视频流异常(2300~2399) */
        VIDEO_LOST                 = 2300, /**< 视频信号丢失 */
        VIDEO_SIGNAL_ABNORMAL      = 2301, /**< 视频信号异常 */
        CAMERA_VIEW_ABNORMAL       = 2302, /**< 相机画面异常 */
        RESOLUTION_MISMATCH        = 2303, /**< 前端/录像分辨率不匹配 */
        VIDEO_FORMAT_MISMATCH      = 2304, /**< 视频输入/输出制式不匹配 */
        ENCODE_PARAM_ABNORMAL      = 2305, /**< 编码参数异常 */
        STREAM_CAUSED_SNAP_FAIL    = 2306, /**< 码流异常导致定时抓图失败 */
        /* 录像抓图异常(2400~2499) */
        RECORD_SNAP_ERROR          = 2400, /**< 录像/抓图异常 */
        SNAPSHOT_FAIL              = 2401, /**< 抓图失败 */
        RECORD_BUFFER_OVERFLOW     = 2402, /**< 录像缓冲区溢出 */
        OVERLOAD_CAUSED_SNAP_FAIL  = 2403, /**< 超出性能导致定时抓图失败 */
        RECORD_CYCLE_INSUFFICIENT  = 2404, /**< 录像周期不足 */
        ANR_RECORD_ABNORMAL        = 2405, /**< ANR录像异常 */
        /* 智能分析(2500~2599) */
        INTELLIGENT_SYS_ABNORMAL   = 2500, /**< 智能系统运行异常 */
        INTELLIGENT_SCENE_ABNORMAL = 2501, /**< 智能分析场景异常 */
        INTELLIGENT_SYNC_ABNORMAL  = 2502, /**< 智能系统同步异常 */
        TARGET_SNAP_OVERFLOW       = 2503, /**< 目标抓拍码流分辨率超限 */
        SMD_STREAM_OVERFLOW        = 2504, /**< SMD码流分辨率超限 */
        MOTION_STREAM_OVERFLOW     = 2505, /**< 移动侦测码流分辨率超限 */
        SAFETY_HELMET_ABNORMAL     = 2506, /**< 安全帽检测异常 */
        /* 安全与访问(2600~2699) */
        UNAUTHORIZED_ACCESS        = 2600, /**< 非法访问 */
        PASSWORD_SYNC_ABNORMAL     = 2601, /**< 密码同步异常 */
        ALARM_DEPLOY_FAIL          = 2602, /**< IP通道报警布防失败 */
        ALARM_UPLOAD_FAIL          = 2603, /**< 报警上传失败 */
        VCA_PIC_UPLOAD_ABNORMAL    = 2604, /**< VCA图片上传显示异常 */
        REPEATED_LOGIN             = 2605, /**< 同一ip重复登陆*/
        /* 系统级异常(2700~2799) */
        IPC_MIGRATION_FAIL         = 2700, /**< IPC迁移失败 */
        CLUSTER_CONFIG_FAIL        = 2701, /**< 集群内设备配置失败 */
        DIAL_ABNORMAL              = 2702, /**< 拨号异常 */
        CHANNEL_MOTION_ABNORMAL    = 2703, /**< IP通道移动侦测智能分析异常 */
#if 0
        /* 萤石服务(2800~2899) */
        EZ_OFFLINE_ABNORMAL        = 2800, /**< 萤石下线异常 */
        EZ_UPGRADE_ABNORMAL        = 2801, /**< 萤石升级异常 */
        EZ_OPERATION_ABNORMAL      = 2802, /**< 萤石操作异常 */
#endif
        /* --------------------异常日志类型(2000~2999)-------------------- */

        /* --------------------操作日志类型(3000~3999)-------------------- */
        /* 基础操作(3000~3049) */
        LOCAL_LOGIN                   = 3000, /**< 本地登录 */
        LOCAL_LOGOUT                  = 3001, /**< 本地注销 */
        REMOTE_LOGIN                  = 3002, /**< 远程登录 */
        REMOTE_LOGOUT                 = 3003, /**< 远程注销 */
        LOCAL_STARTUP                 = 3004, /**< 开机 */
        LOCAL_SHUTDOWN                = 3005, /**< 本地关机 */
        ABNORMAL_SHUTDOWN             = 3006, /**< 异常关机 */
        LOCAL_REBOOT                  = 3007, /**< 本地重启 */
        REMOTE_SHUTDOWN               = 3008, /**< 远程关机 */
        REMOTE_REBOOT                 = 3009, /**< 远程重启 */
        /* 配置管理(3050~3099) */
        LOCAL_CONFIG                  = 3050, /**< 本地配置 */
        REMOTE_CONFIG                 = 3051, /**< 远程配置 */
        LOCAL_IMPORT_CONFIG           = 3052, /**< 本地导入配置文件 */
        LOCAL_EXPORT_CONFIG           = 3053, /**< 本地导出配置文件 */
        REMOTE_IMPORT_CONFIG          = 3054, /**< 远程导入配置文件 */
        REMOTE_EXPORT_CONFIG          = 3055, /**< 远程导出配置文件 */
        LOCAL_SIMPLE_RESET            = 3056, /**< 本地简单恢复默认参数 */
        LOCAL_FACTORY_RESET           = 3057, /**< 本地恢复出厂默认参数 */
        REMOTE_SIMPLE_RESET           = 3058, /**< 远程简单恢复默认参数 */
        REMOTE_FACTORY_RESET          = 3059, /**< 远程恢复出厂默认参数 */
        LOCAL_RESET_ADMIN_PWD         = 3060, /**< 本地恢复管理员默认密码 */
        LOCAL_RESET_LOGIN_PWD         = 3061, /**< 本地重置登录密码 */
        REMOTE_RESET_LOGIN_PWD        = 3062, /**< 远程重置登录密码 */
        LOCAL_RESET_CHANNEL_PWD       = 3063, /**< 本地重置通道密码 */
        REMOTE_RESET_CHANNEL_PWD      = 3064, /**< 远程重置通道密码 */
        /* 存储管理(3100~3149) */
        LOCAL_ADD_NETDISK             = 3100, /**< 本地添加网络盘 */
        LOCAL_DEL_NETDISK             = 3101, /**< 本地删除网络盘 */
        LOCAL_SET_NETDISK             = 3102, /**< 本地设置网络盘 */
        REMOTE_ADD_NETDISK            = 3103, /**< 远程添加网络盘 */
        REMOTE_DEL_NETDISK            = 3104, /**< 远程删除网络盘 */
        REMOTE_SET_NETDISK            = 3105, /**< 远程设置网络盘 */
        LOCAL_INIT_DISK               = 3106, /**< 本地初始化硬盘 */
        LOCAL_DISK_SELF_TEST          = 3107, /**< 本地物理硬盘自检 */
        REMOTE_INIT_DISK              = 3108, /**< 远程初始化硬盘 */
        REMOTE_MOUNT_DISK             = 3109, /**< 远程加载硬盘 */
        REMOTE_UNMOUNT_DISK           = 3110, /**< 远程卸载硬盘 */
        REMOTE_CREATE_STORAGE_POOL    = 3111, /**< 远程创建存储池 */
        REMOTE_DEL_STORAGE_POOL       = 3112, /**< 远程删除存储池 */
        REMOTE_SET_STORAGE_PARAMS     = 3113, /**< 远程设置存储池参数 */
        REMOTE_SET_STORAGE_CAPACITY   = 3114, /**< 远程设置存储池容量 */
        REMOTE_DEL_INVALID_DISK       = 3115, /**< 远程删除异常不存在的硬盘 */
        STORAGE_MEDIA_ERASE           = 3116, /**< 存储介质安全擦除 */
        /* 通道管理(3150~3199) */
        LOCAL_ADD_IP_CHANNEL          = 3150, /**< 本地添加IP通道 */
        LOCAL_DEL_IP_CHANNEL          = 3151, /**< 本地删除IP通道 */
        LOCAL_CONFIG_IP_CHANNEL       = 3152, /**< 本地配置IP通道 */
        REMOTE_ADD_IP_CHANNEL         = 3153, /**< 远程添加IP通道 */
        REMOTE_DEL_IP_CHANNEL         = 3154, /**< 远程删除IP通道 */
        REMOTE_SET_IP_CHANNEL         = 3155, /**< 远程设置IP通道 */
        LOCAL_ADD_IOT_CHANNEL         = 3156, /**< 本地添加IoT通道 */
        LOCAL_DEL_IOT_CHANNEL         = 3157, /**< 本地删除IoT通道 */
        LOCAL_CONFIG_IOT_CHANNEL      = 3158, /**< 本地配置IoT通道 */
        REMOTE_ADD_IOT_CHANNEL        = 3159, /**< 远程添加IoT通道 */
        REMOTE_DEL_IOT_CHANNEL        = 3160, /**< 远程删除IoT通道 */
        REMOTE_CONFIG_IOT_CHANNEL     = 3161, /**< 远程配置IoT通道 */
        CHANNEL_SORT                  = 3162, /**< 通道排序 */
        CHANNEL_DND_CONFIG            = 3163, /**< 通道勿扰配置 */
        /* 媒体操作(3200~3249) */
        LOCAL_START_RECORDING         = 3200, /**< 本地启动录像 */
        LOCAL_STOP_RECORDING          = 3201, /**< 本地停止录像 */
        LOCAL_START_SNAPSHOT          = 3202, /**< 本地启动抓图 */
        LOCAL_STOP_SNAPSHOT           = 3203, /**< 本地停止抓图 */
        REMOTE_START_RECORDING        = 3204, /**< 远程启动录像 */
        REMOTE_STOP_RECORDING         = 3205, /**< 远程停止录像 */
        REMOTE_START_SNAPSHOT         = 3206, /**< 远程启动抓图 */
        REMOTE_STOP_SNAPSHOT          = 3207, /**< 远程停止抓图 */
        LOCAL_FILE_PLAYBACK           = 3208, /**< 本地按文件回放 */
        LOCAL_TIME_PLAYBACK           = 3209, /**< 本地按时间回放 */
        REMOTE_FILE_PLAYBACK          = 3210, /**< 远程按文件回放 */
        REMOTE_TIME_PLAYBACK          = 3211, /**< 远程按时间回放 */
        LOCAL_BACKUP_IMAGES           = 3212, /**< 本地备份图片文件 */
        LOCAL_BACKUP_RECORDS          = 3213, /**< 本地备份录像文件 */
        REMOTE_BACKUP_IMAGES          = 3214, /**< 远程备份图片文件 */
        REMOTE_BACKUP_RECORDS         = 3215, /**< 远程备份录像文件 */
        REMOTE_DEL_IMAGE_DATA         = 3216, /**< 远程删除图片数据 */
        REMOTE_DEL_RECORD_SEGMENT     = 3217, /**< 远程删除录像片段操作 */
        LOCAL_PTZ_CTRL                = 3218, /**< 本地云台控制 */
        REMOTE_PTZ_CTRL               = 3219, /**< 远程云台控制 */
        /* 网络通信(3250~3299) */
        LOCAL_DIAL_CTRL               = 3250, /**< 本地手动启动断开拨号 */
        LOCAL_SET_DIAL_PARAM          = 3251, /**< 本地配置拨号参数 */
        LOCAL_SET_DIAL_PLAN           = 3252, /**< 本地配置拨号计划 */
        REMOTE_START_DIAL             = 3253, /**< 远程开启手动拨号 */
        REMOTE_STOP_DIAL              = 3254, /**< 远程停止手动拨号 */
        REMOTE_SET_DIAL_PARAM         = 3255, /**< 远程配置拨号参数 */
        REMOTE_SET_DIAL_PLAN          = 3256, /**< 远程配置拨号计划 */
        LOCAL_CONFIG_SIP_SERVER       = 3257, /**< 本地配置SIP Server */
        REMOTE_CONFIG_SIP_SERVER      = 3258, /**< 远程配置SIP Server */
        CREATE_TUNNEL                 = 3259, /**< 建立透明通道 */
        DISCONNECT_TUNNEL             = 3260, /**< 断开透明通道 */
        LOCAL_MANUAL_TIME_SYNC        = 3261, /**< 本地手动校时 */
        REMOTE_MANUAL_TIME_SYNC       = 3262, /**< 远程手动校时 */
        LOCAL_WPS_CONNECT             = 3263, /**< 本地WPS连接 */
        REMOTE_WPS_CONNECT            = 3264, /**< 远程WPS连接 */
        LOCAL_IP_ADAPTIVE_CONFIG      = 3265, /**< 本地IP自适应配置 */
        REMOTE_IP_ADAPTIVE_CONFIG     = 3266, /**< 远程IP自适应配置 */
        LOCAL_CONFIG_SNMP             = 3267, /**< 本地配置SNMP */
        REMOTE_CONFIG_SNMP            = 3268, /**< 远程配置SNMP */
        LOCAL_CONFIG_EMAIL            = 3269, /**< 本地配置EMAIL */
        REMOTE_CONFIG_EMAIL           = 3270, /**< 远程配置EMAIL */
        /* 报警与通知(3300~3349) */
        LOCAL_MANUAL_ALARM            = 3300, /**< 本地手动报警 */
        REMOTE_MANUAL_ALARM           = 3301, /**< 远程手动报警 */
        LOCAL_ALARM_ARM               = 3302, /**< 本地报警布防 */
        REMOTE_ALARM_ARM              = 3303, /**< 远程布防 */
        LOCAL_ALARM_DISARM            = 3304, /**< 本地撤防 */
        REMOTE_ALARM_DISARM           = 3305, /**< 远程撤防 */
        SEND_SMS_ALARM                = 3306, /**< 发送短信报警 */
        LOCAL_SEND_SMS                = 3307, /**< 本地发送短信 */
        LOCAL_VIEW_SMS                = 3308, /**< 本地查看短信 */
        LOCAL_SEARCH_SMS              = 3309, /**< 本地搜索短信 */
        REMOTE_SEND_SMS               = 3310, /**< 远程发送短信 */
        REMOTE_VIEW_SMS               = 3311, /**< 远程查看短信 */
        REMOTE_SEARCH_SMS             = 3312, /**< 远程搜索短信 */
        RECEIVE_SMS                   = 3313, /**< 接收短信 */
        CALL_CTRL_ONLINE              = 3314, /**< 呼叫控制上线 */
        SMS_CTRL_ONLINE               = 3315, /**< 短信控制上下线 */
        VOICE_TALK_START              = 3316, /**< 语音对讲开始 */
        VOICE_TALK_END                = 3317, /**< 语音对讲结束 */
        CUSTOM_VOICE_OPERATION        = 3318, /**< 自定义语音操作 */
        /* 系统管理(3350~3399) */
        LOCAL_ACTIVATE_DEVICE         = 3350, /**< 本地激活设备 */
        REMOTE_ACTIVATE_DEVICE        = 3351, /**< 远程激活设备 */
        LOCAL_UPGRADE                 = 3352, /**< 本地升级 */
        REMOTE_MAINTENANCE            = 3353, /**< 远程升级、格式化 */
        LOCAL_UPGRADE_IPC_FIRMWARE    = 3354, /**< 本地升级IP通道固件 */
        REMOTE_UPGRADE_IPC_FIRMWARE   = 3355, /**< 远程升级IP通道固件 */
        LOCAL_ENABLE_CLOUD_SYS        = 3356, /**< 本地启用云系统 */
        REMOTE_ENABLE_CLOUD_SYS       = 3357, /**< 远程启用云系统 */
        LOCAL_DISABLE_CLOUD_SYS       = 3358, /**< 本地禁用云系统 */
        REMOTE_DISABLE_CLOUD_SYS      = 3359, /**< 远程禁用云系统 */
        LOCAL_AUTO_REBOOT_CONFIG      = 3360, /**< 本地配置自动重启 */
        REMOTE_AUTO_REBOOT_CONFIG     = 3361, /**< 远程配置自动重启 */
        LOCAL_STORAGE_OPTIMIZE        = 3362, /**< 本地存储优化配置 */
        REMOTE_STORAGE_OPTIMIZE       = 3363, /**< 远程存储优化配置 */
        /* 智能功能(3400~3449) */
        LOCAL_CONFIG_ANALYTICS        = 3400, /**< 本地智能分析配置 */
        REMOTE_CONFIG_ANALYTICS       = 3401, /**< 远程智能分析配置 */
        LOCAL_CREATE_TARGET_DB        = 3402, /**< 本地创建目标对比库 */
        LOCAL_MODIFY_TARGET_DB        = 3403, /**< 本地修改目标对比库 */
        LOCAL_DEL_TARGET_DB           = 3404, /**< 本地删除目标对比库 */
        LOCAL_IMPORT_TARGET_IMG       = 3405, /**< 本地导入目标图片 */
        LOCAL_MODIFY_TARGET_IMG       = 3406, /**< 本地修改目标图片 */
        LOCAL_DEL_TARGET_IMG          = 3407, /**< 本地删除目标图片 */
        LOCAL_IMPORT_TARGET_DB        = 3408, /**< 本地导入目标库 */
        LOCAL_EXPORT_TARGET_DB        = 3409, /**< 本地导出目标库 */
        REMOTE_CREATE_TARGET_DB       = 3410, /**< 远程创建目标库 */
        REMOTE_MODIFY_TARGET_DB       = 3411, /**< 远程修改目标库 */
        REMOTE_DEL_TARGET_DB          = 3412, /**< 远程删除目标库 */
        REMOTE_IMPORT_TARGET_IMG      = 3413, /**< 远程导入目标图片 */
        REMOTE_MODIFY_TARGET_IMG      = 3414, /**< 远程修改目标图片 */
        REMOTE_DEL_TARGET_IMG         = 3415, /**< 远程删除目标图片 */
        REMOTE_TARGET_COMPARE_CONFIG  = 3416, /**< 远程目标比对任务配置 */
        LOCAL_IMPORT_FILTER_DB_IMG    = 3417, /**< 本地导入过滤库图片 */
        LOCAL_DEL_FILTER_DB_IMG       = 3418, /**< 本地删除过滤库图片 */
        LOCAL_ENABLE_FILTER_DB        = 3419, /**< 本地配置过滤库使能 */
        LOCAL_FILTER_DB_UPDATE        = 3420, /**< 本地过滤库更新 */
        REMOTE_IMPORT_FILTER_DB_IMG   = 3421, /**< 远程导入过滤库图片 */
        REMOTE_DEL_FILTER_DB_IMG      = 3422, /**< 远程删除过滤库图片 */
        REMOTE_ENABLE_FILTER_DB       = 3423, /**< 远程配置过滤库使能 */
        REMOTE_FILTER_DB_UPDATE       = 3424, /**< 远程过滤库更新 */
        LOCAL_ALGORITHM_DISPATCH      = 3425, /**< 本地算法包调度 */
        REMOTE_ALGORITHM_DISPATCH     = 3426, /**< 远程算法包调度 */
        /* 特殊硬件(3450~3499) */
        LOCAL_SET_POE_MODE            = 3450, /**< 本地设置PoE工作模式 */
        LOCAL_POE_CONFIG              = 3451, /**< 本地POE配置 */
        REMOTE_SET_POE_MODE           = 3452, /**< 远程设置PoE工作模式 */
        REMOTE_POE_CONFIG             = 3453, /**< 远程POE配置 */
        LOCAL_SSD_INIT_START          = 3454, /**< 本地SSD初始化开始 */
        LOCAL_SSD_INIT_END            = 3455, /**< 本地SSD初始化结束 */
        REMOTE_SSD_INIT_START         = 3456, /**< 远程SSD初始化开始 */
        REMOTE_SSD_INIT_END           = 3457, /**< 远程SSD初始化结束 */
        SSD_SELF_REPAIR               = 3458, /**< SSD自修复 */
#if 0
        /* 萤石功能(3500~3549) */
        EZ_REGISTER                   = 3500, /**< 萤石注册 */
        EZ_UPGRADE                    = 3501, /**< 萤石升级 */
        LOCAL_EZ_CONFIG               = 3502, /**< 本地萤石参数配置 */
        REMOTE_EZ_CONFIG              = 3503, /**< 远程萤石参数配置 */
        LOCAL_EZ_OPERATION            = 3504, /**< 本地萤石操作 */
        REMOTE_EZ_OPERATION           = 3505, /**< 远程萤石操作 */
        EZ_STREAM_CONFIG              = 3506, /**< 萤石码流参数配置 */
        EZ_ALARM_CONFIG               = 3507, /**< 萤石报警参数配置 */
#endif
        /* 附加功能(3550~3599) */
        SVC_CONFIG                    = 3550, /**< SVC配置 */
        LOCAL_ENABLE_DECODE_ENHANCE   = 3551, /**< 本地开启解码增强功能 */
        LOCAL_DISABLE_DECODE_ENHANCE  = 3552, /**< 本地关闭解码增强功能 */
        REMOTE_ENABLE_DECODE_ENHANCE  = 3553, /**< 远程开启解码增强功能 */
        REMOTE_DISABLE_DECODE_ENHANCE = 3554, /**< 远程关闭解码增强功能 */
        LOCAL_MANAGE_WORKER           = 3555, /**< 本地添加/删除工作机 */
        REMOTE_MANAGE_WORKER          = 3556, /**< 远程添加/删除工作机 */
        LOCAL_PIN_OPERATION           = 3557, /**< 本地PIN功能操作 */
        REMOTE_PIN_OPERATION          = 3558, /**< 远程PIN功能操作 */
        LOCAL_TAG_OPERATION           = 3559, /**< 本地标签操作 */
        REMOTE_TAG_OPERATION          = 3560, /**< 远程标签操作 */
        LOCAL_OPERATION_LOCK          = 3561, /**< 本地操作锁定 */
        LOCAL_OPERATION_UNLOCK        = 3562, /**< 本地操作解除锁定 */
        LOCAL_FILE_LOCK               = 3563, /**< 本地锁定文件 */
        LOCAL_FILE_UNLOCK             = 3564, /**< 本地解锁文件 */
        REMOTE_FILE_LOCK              = 3565, /**< 远程锁定文件 */
        REMOTE_FILE_UNLOCK            = 3566, /**< 远程解锁文件 */
        LOCAL_EXPORT_PASSENGER_FLOW   = 3567, /**< 本地导出客流量文件 */
        LOCAL_EXPORT_HEATMAP          = 3568, /**< 本地导出热度图文件 */
        LOCAL_EXPORT_ALLOW_DENY_LIST  = 3569, /**< 本地导出允许拒绝名单文件 */
        LOCAL_EXPORT_IP_CHANNEL_LIST  = 3570, /**< 本地导出IP通道列表文件 */
        LOCAL_IMPORT_ALLOW_DENY_LIST  = 3571, /**< 本地导入允许拒绝名单文件 */
        LOCAL_IMPORT_IP_CHANNEL_LIST  = 3572, /**< 本地导入IP通道列表文件 */
        LOCAL_CONFIG_ALLOW_LIST       = 3573, /**< 本地配置允许名单 */
        REMOTE_EXPORT_ALLOW_DENY_LIST = 3574, /**< 远程导出允许拒绝名单文件 */
        REMOTE_EXPORT_IP_CHANNEL_LIST = 3575, /**< 远程导出IP通道列表文件 */
        REMOTE_IMPORT_ALLOW_DENY_LIST = 3576, /**< 远程导入允许拒绝名单文件 */
        REMOTE_IMPORT_IP_CHANNEL_LIST = 3577, /**< 远程导入IP通道列表文件 */
        REMOTE_CONFIG_ALLOW_LIST      = 3578, /**< 远程配置允许名单 */
        REMOTE_GET_PARAMS             = 3579, /**< 远程获取参数 */
        REMOTE_GET_STATUS             = 3580, /**< 远程获取状态 */
        LOCAL_CLUSTER_CONFIG          = 3581, /**< 本地集群组网配置 */
        LOCAL_CLUSTER_ADD_DEV         = 3582, /**< 本地集群添加设备 */
        LOCAL_CLUSTER_DEL_DEV         = 3583, /**< 本地集群删除设备 */
        REMOTE_CLUSTER_CONFIG         = 3584, /**< 远程集群组网配置 */
        REMOTE_CLUSTER_ADD_DEV        = 3585, /**< 远程集群添加设备 */
        REMOTE_CLUSTER_DEL_DEV        = 3586, /**< 远程集群删除设备 */
        LOCAL_IMPORT_IOT_CONFIG       = 3587, /**< 本地导入IoT配置文件 */
        LOCAL_EXPORT_IOT_CONFIG       = 3588, /**< 本地导出IoT配置文件 */
        REMOTE_IMPORT_IOT_CONFIG      = 3589, /**< 远程导入IoT配置文件 */
        REMOTE_EXPORT_IOT_CONFIG      = 3590, /**< 远程导出IoT配置文件 */
        /* --------------------操作日志类型(3000~3999)-------------------- */

        /* --------------------信息日志类型(4000~4999)-------------------- */
        /* 系统基础状态(4000~4099) */
        SYSTEM_STATUS         = 4000, /**< 系统运行状态信息 */
        DISK_INFO             = 4001, /**< 本地硬盘信息 */
        NET_DISK_INFO         = 4002, /**< 网络硬盘信息 */
        RAID_INFO             = 4003, /**< raid相关信息 */
        SMART_INFO            = 4004, /**< S.M.A.R.T信息 */
        WORKER_STATUS         = 4005, /**< 工作机信息 */
        DEVICE_RUNTIME        = 4006, /**< 设备运行时长 */
        /* 录像抓图状态(4100~4199) */
        ANR_RECORD_START      = 4100, /**< ANR录像开始 */
        ANR_RECORD_END        = 4101, /**< ANR录像结束 */
        SNAPSHOT_START        = 4102, /**< 开始抓图 */
        RECORD_START          = 4103, /**< 开始录像 */
        SNAPSHOT_STOP         = 4104, /**< 结束抓图 */
        RECORD_STOP           = 4105, /**< 结束录像 */
        EXPIRE_RECORD_DELETED = 4106, /**< 删除过期录像 */
        RECORD_STOP_INFO      = 4107, /**< 停止录像信息 */
        ANR_SCHEDULE_ADD      = 4108, /**< IP通道ANR时间段添加 */
        ANR_SCHEDULE_DEL      = 4109, /**< IP通道ANR时间段删除 */
        /* 网络与连接(4200~4299) */
        DIAL_STATUS           = 4200, /**< 拨号状态 */
        IPC_CONNECTION        = 4201, /**< IPC连接状态 */
        NETWORK_CONNECTED     = 4202, /**< ipc连接 */
        PLATFORM_STATUS       = 4203, /**< 平台信息 */
        IEEE8021X_AUTH_FAILED = 4204, /**< 802.1X认证失败 */
        RE_AUTH_PASSED        = 4205, /**< 二次认证通过 */
        /* 存储操作(4300~4399) */
        DISK_FORMAT_START     = 4300, /**< 硬盘格式化开始 */
        DISK_FORMAT_END       = 4301, /**< 硬盘格式化结束 */
        DATA_REBUILD_START    = 4302, /**< 数据重建开始 */
        DATA_REBUILD_END      = 4303, /**< 数据重建结束 */
        /* 集群与热备(4400~4499) */
        CLUSTER_DEV_ONLINE    = 4400, /**< 集群设备上线 */
        CLUSTER_SERVICE_START = 4401, /**< 集群管理服务启动 */
        CLUSTER_MIGRATION     = 4402, /**< 集群业务迁移 */
        CLUSTER_STATUS        = 4403, /**< 集群状态信息 */
        HOTBACKUP_START       = 4404, /**< 热备机开始备份工作机 */
        HOTBACKUP_STOP        = 4405, /**< 热备机停止备份工作机 */
        /* 智能与识别(4500~4599) */
        SMART_BOARD_STATUS    = 4500, /**< 智能板状态 */
        TARGET_EVENT_NO_IMAGE = 4501, /**< 目标事件（目标侦测&目标抓拍）无图片 */
        /* 其他状态(4600~4699) */
        TIME_SYNC             = 4600, /**< 系统校时 */
#if 0
        EZ_RUNTIME_STATUS     = 4601, /**< 萤石运行状态 */
        EZ_LINKAGE_INFO       = 4602, /**< 萤石联动信息 */
#endif
        OPS_NOT_EXECUTED      = 4603, /**< 运维未执行 */
        /* --------------------信息日志类型(4000~4999)-------------------- */
    } Action_E;

    inline Action_E to_action(Event::Type_E enType, bool bStart)
    {
        switch (enType)
        {
        /* 普通事件 */
        case Event::Type::MOTION_DETECT:
            return bStart ? MOTION_DETECT_START : MOTION_DETECT_STOP;
        case Event::Type::OCCLUSION_DETECT:
            return bStart ? OCCLUSION_DETECT_START : OCCLUSION_DETECT_STOP;
        case Event::Type::ANOMALY_ALARM:
            return bStart ? ANOMALY_ALARM_START : ANOMALY_ALARM_STOP;
        case Event::Type::AUDIO_ALARM:
            return bStart ? AUDIO_ALARM_START : AUDIO_ALARM_STOP;
        case Event::Type::ALARM_INPUT:
            return bStart ? ALARM_INPUT_START : ALARM_INPUT_STOP;
        case Event::Type::ALARM_OUTPUT:
            return bStart ? ALARM_OUTPUT_START : ALARM_OUTPUT_STOP;
        case Event::Type::FLASH_ALARM:
            return bStart ? FLASH_ALARM_START : FLASH_ALARM_STOP;
        case Event::Type::PIR_ALARM:
            return bStart ? PIR_ALARM_START : PIR_ALARM_STOP;
        /* 异常事件 */
        case Event::Type::DISK_FULL:
            return Log::DISK_FULL;
        case Event::Type::DISK_ERROR:
            return Log::DISK_ERROR;
        case Event::Type::NET_BROKEN:
            return Log::NET_BROKEN;
        case Event::Type::IP_CONFLICT:
            return Log::IP_CONFLICT;
        case Event::Type::ILLEGAL_ACCESS:
            return Log::UNAUTHORIZED_ACCESS;
        case Event::Type:: RECORD_ABNORMAL:
            return Log::RECORD_SNAP_ERROR;
        case Event::Type::HARDWARE_ABNORMAL:
            return Log::HARDWARE_ABNORMAL;
        case Event::Type::STREAM_RESOLUTION_LIMIT:
            return RESOLUTION_MISMATCH;
        case Event::Type::VIDEO_SIGNAL_LOSS:
            return VIDEO_LOST;
        /* 周界事件 */
        case Event::Type::LINE_CROSSING:
            return bStart ? LINE_CROSSING_START : LINE_CROSSING_STOP;
        case Event::Type::INTRUSION:
            return bStart ? INTRUSION_START : INTRUSION_STOP;
        case Event::Type::ENTER_REGION:
            return bStart ? ENTER_REGION_START : ENTER_REGION_STOP;
        case Event::Type::LEAVE_REGION:
            return bStart ? LEAVE_REGION_START : LEAVE_REGION_STOP;
        /* Smart事件 */
        case Event::Type::AUDIO_ANOMALY:
            return bStart ? AUDIO_ANOMALY_START : AUDIO_ANOMALY_STOP;
        case Event::Type::AUDIO_SUDDEN_RISE:
            return bStart ? AUDIO_ANOMALY_START : AUDIO_ANOMALY_STOP;
        case Event::Type::AUDIO_SUDDEN_DROP:
            return bStart ? AUDIO_ANOMALY_START : AUDIO_ANOMALY_STOP;
        case Event::Type::SCENE_CHANGE:
            return bStart ? SCENE_CHANGE_START : SCENE_CHANGE_STOP;
        case Event::Type::FACE_DETECT:
            return bStart ? FACE_DETECT_START : FACE_DETECT_STOP;
        case Event::Type::LOITERING_DETECT:
            return bStart ? LOITERING_DETECT_START : LOITERING_DETECT_STOP;
        case Event::Type::CROWD_GATHERING:
            return bStart ? CROWD_GATHERING_START : CROWD_GATHERING_STOP;
        case Event::Type::PARKING_DETECT:
            return bStart ? PARKING_DETECT_START : PARKING_DETECT_STOP;
        case Event::Type::UNATTENDED_OBJECT:
            return bStart ? UNATTENDED_OBJECT_START : UNATTENDED_OBJECT_STOP;
        case Event::Type::OBJECT_REMOVAL:
            return bStart ? OBJECT_REMOVAL_START : OBJECT_REMOVAL_STOP;
        case Event::Type::PET_RECOGNITION:
            return bStart ? PET_RECOGNITION_START : PET_RECOGNITION_STOP;
        case Event::Type::FACE_CAPTURE:
            return bStart ? FACE_CAPTURE_START : FACE_CAPTURE_STOP;
#ifdef SCENE_INTELLIGENCE
        /* 行为监管 */
        case Event::Type::SLEEP_ON_DUTY:
            return bStart ? SLEEP_ON_DUTY_START : SLEEP_ON_DUTY_STOP;
        case Event::Type::LEAVE_POST:
            return bStart ? LEAVE_POST_START : LEAVE_POST_STOP;
        case Event::Type::PERSON_FALL_DOWN:
            return bStart ? PERSON_FALL_DOWN_START : PERSON_FALL_DOWN_STOP;
        case Event::Type::FENCE_CLIMBING:
            return bStart ? FENCE_CLIMBING_START : FENCE_CLIMBING_STOP;
        case Event::Type::SMOKING:
            return bStart ? SMOKING_START : SMOKING_STOP;
        case Event::Type::PHONE_USAGE:
            return bStart ? PHONE_USAGE_START : PHONE_USAGE_STOP;
        case Event::Type::SMOKE_FIRE:
            return bStart ? SMOKE_FIRE_START : SMOKE_FIRE_STOP;
        case Event::Type::OPEN_FLAME:
            return bStart ? OPEN_FLAME_START : OPEN_FLAME_STOP;
        case Event::Type::BARE_SOIL:
            return bStart ? BARE_SOIL_START : BARE_SOIL_STOP;
        case Event::Type::PERSON_TRIP:
            return bStart ? PERSON_TRIP_STAR : PERSON_TRIP_STOP;
        case Event::Type::ELECTRIC_VEHICLE_IN_ELEVATOR:
            return bStart ? ELECTRIC_VEHICLE_IN_ELEVATOR_START : ELECTRIC_VEHICLE_IN_ELEVATOR_STOP;
        case Event::Type::MANHOLE_COVER_ABNORMAL:
            return bStart ? MANHOLE_COVER_ABNORMAL_START : MANHOLE_COVER_ABNORMAL_STOP;
        case Event::Type::HOLE_PROTECTION_BAR:
            return bStart ? HOLE_PROTECTION_BAR_START : HOLE_PROTECTION_BAR_STOP;
        case Event::Type::PEDESTRIAN_INTRUSION:
            return bStart ? PEDESTRIAN_INTRUSION_START : PEDESTRIAN_INTRUSION_STOP;
            /* 穿戴规范 */
        case Event::Type::SAFETY_HELMET:
            return bStart ? SAFETY_HELMET_START : SAFETY_HELMET_STOP;
        case Event::Type::REFLECTIVE_CLOTHING:
            return bStart ? REFLECTIVE_CLOTHING_START : REFLECTIVE_CLOTHING_STOP;
        case Event::Type::HIGH_ALTITUDE_SEATBELT:
            return bStart ? HIGH_ALTITUDE_SEATBELT_START : HIGH_ALTITUDE_SEATBELT_STOP;
            /* 交通行为监管 */
        case Event::Type::CONSTRUCTION_OCCUPY_ROAD:
            return bStart ? CONSTRUCTION_OCCUPY_ROAD_START : CONSTRUCTION_OCCUPY_ROAD_STOP;
        case Event::Type::EMERGENCY_LANE_OCCUPANCY:
            return bStart ? EMERGENCY_LANE_OCCUPANCY_START : EMERGENCY_LANE_OCCUPANCY_STOP;
        case Event::Type::REVERSE_DIRECTION:
            return bStart ? REVERSE_DIRECTION_START : REVERSE_DIRECTION_STOP;
        case Event::Type::NON_MOTOR_VEHICLE_INTRUSION:
            return bStart ? NON_MOTOR_VEHICLE_INTRUSION_START : NON_MOTOR_VEHICLE_INTRUSION_STOP;
        case Event::Type::ROAD_PONDING:
            return bStart ? ROAD_PONDING_START : ROAD_PONDING_STOP;
        case Event::Type::CONGESTION:
            return bStart ? CONGESTION_START : CONGESTION_STOP;
        case Event::Type::ILLEGAL_LANE_CHANGE:
            return bStart ? ILLEGAL_LANE_CHANGE_START : ILLEGAL_LANE_CHANGE_STOP;
            /* 属性识别 */
        case Event::Type::PLATE_NUMBER:
            return bStart ? PLATE_NUMBER_START : PLATE_NUMBER_STOP;
#endif
#ifdef SCENE_INTELLIGENT_ANALYSIS
        /* 场景智能分析 */
        case Event::Type_E::TEXT_PRESET:
            return Log::TEXT_PRESET_ALARM;
#endif

#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT
        /* 垃圾识别 */
        case Event::Type::GARBAGE_EXPOSURE:
            return bStart ? GARBAGE_EXPOSURE_START : GARBAGE_EXPOSURE_START;
        case Event::Type::GARBAGE_OVERFLOW:
            return bStart ? GARBAGE_OVERFLOW_START : GARBAGE_OVERFLOW_STOP;
#endif
#if defined CAP_AI_FACE_COMPARE
        /* 人脸比对 */
        case Event::Type::FACE_COMPARE_SUCCESS:
            return bStart ? FACE_COMPARE_SUCCESS_START : FACE_COMPARE_SUCCESS_STOP;
        case Event::Type::FACE_COMPARE_FAIL:
            return bStart ? FACE_COMPARE_FAIL_START :FACE_COMPARE_FAIL_STOP;
#endif
#if CAP_AI_PEOPLE_STATISTICS
        case Event::Type::PEOPLE_FLOW_STATISTICS:
            return bStart ? PEOPLE_FLOW_STATISTICS_START : PEOPLE_FLOW_STATISTICS_STOP;
        case Event::Type::PEOPLE_DENSITY_DETECTION:
            return bStart ? PEOPLE_DENSITY_DETECTION_START : PEOPLE_DENSITY_DETECTION_STOP;
        case Event::Type::PEOPLE_FLOW_STAY_NORMAL:
            return bStart ? PEOPLE_FLOW_STAY_NORMAL_START : PEOPLE_FLOW_STAY_NORMAL_STOP;
        case Event::Type::PEOPLE_FLOW_STAY_MEDIUM:
            return bStart ? PEOPLE_FLOW_STAY_MEDIUM_START : PEOPLE_FLOW_STAY_MEDIUM_STOP;
        case Event::Type::PEOPLE_FLOW_STAY_SEVERE:
            return bStart ? PEOPLE_FLOW_STAY_SEVERE_START : PEOPLE_FLOW_STAY_SEVERE_STOP;
        case Event::Type::PEOPLE_DENSITY_NORMAL:
            return bStart ? PEOPLE_DENSITY_NORMAL_START : PEOPLE_DENSITY_NORMAL_STOP;
        case Event::Type::PEOPLE_DENSITY_MEDIUM:
            return bStart ? PEOPLE_DENSITY_MEDIUM_START : PEOPLE_DENSITY_MEDIUM_STOP;
        case Event::Type::PEOPLE_DENSITY_SEVERE:
            return bStart ? PEOPLE_DENSITY_SEVERE_START : PEOPLE_DENSITY_SEVERE_STOP;
#endif
        default:
            return UNDEFINED;
        }
    }

    inline std::string to_string(Action_E action)
    {
        switch (action)
        {
        /* --------------------报警日志类型-------------------- */
        /* 普通事件 */
        case MOTION_DETECT_START:     return "移动侦测开始";
        case MOTION_DETECT_STOP:      return "移动侦测结束";
        case OCCLUSION_DETECT_START:  return "遮挡侦测开始";
        case OCCLUSION_DETECT_STOP:   return "遮挡侦测结束";
        case ANOMALY_ALARM_START:     return "异常报警开始";
        case ANOMALY_ALARM_STOP:      return "异常报警结束";
        case AUDIO_ALARM_START:       return "声音报警开始";
        case AUDIO_ALARM_STOP:        return "声音报警结束";
        case ALARM_INPUT_START:       return "报警输入开始";
        case ALARM_INPUT_STOP:        return "报警输入结束";
        case ALARM_OUTPUT_START:      return "报警输出开始";
        case ALARM_OUTPUT_STOP:       return "报警输出结束";
        case FLASH_ALARM_START:       return "闪光灯报警开始";
        case FLASH_ALARM_STOP:        return "闪光灯报警结束";
        case PIR_ALARM_START:         return "PIR 红外感应报警开始";
        case PIR_ALARM_STOP:          return "PIR 红外感应报警结束";
        /* 周界事件 */
        case LINE_CROSSING_START:     return "越界侦测开始";
        case LINE_CROSSING_STOP:      return "越界侦测结束";
        case INTRUSION_START:         return "区域入侵侦测开始";
        case INTRUSION_STOP:          return "区域入侵侦测结束";
        case ENTER_REGION_START:      return "进入区域侦测开始";
        case ENTER_REGION_STOP:       return "进入区域侦测结束";
        case LEAVE_REGION_START:      return "离开区域侦测开始";
        case LEAVE_REGION_STOP:       return "离开区域侦测结束";
        /* Smart事件 */
        case AUDIO_ANOMALY_START:     return "音频输入异常开始";
        case AUDIO_ANOMALY_STOP:      return "音频输入异常结束";
        case SCENE_CHANGE_START:      return "场景变更侦测开始";
        case SCENE_CHANGE_STOP:       return "场景变更侦测结束";
        case FACE_DETECT_START:       return "人脸侦测开始";
        case FACE_DETECT_STOP:        return "人脸侦测结束";
        case LOITERING_DETECT_START:  return "徘徊侦测开始";
        case LOITERING_DETECT_STOP:   return "徘徊侦测结束";
        case CROWD_GATHERING_START:   return "人员聚集侦测开始";
        case CROWD_GATHERING_STOP:    return "人员聚集侦测结束";
        case PARKING_DETECT_START:    return "停车侦测开始";
        case PARKING_DETECT_STOP:     return "停车侦测结束";
        case UNATTENDED_OBJECT_START: return "物品遗留侦测开始";
        case UNATTENDED_OBJECT_STOP:  return "物品遗留侦测结束";
        case OBJECT_REMOVAL_START:    return "物品拿取侦测开始";
        case OBJECT_REMOVAL_STOP:     return "物品拿取侦测结束";
        case PET_RECOGNITION_START:   return "宠物识别侦测开始";
        case PET_RECOGNITION_STOP:    return "宠物识别侦测结束";
        case FACE_CAPTURE_START:      return "人脸抓拍侦测开始";
        case FACE_CAPTURE_STOP:       return "人脸抓拍侦测结束";
#ifdef SCENE_INTELLIGENCE
        /* 行为监管 */
        case SLEEP_ON_DUTY_START:     return "睡岗识别开始";
        case SLEEP_ON_DUTY_STOP:      return "睡岗识别结束";
        case LEAVE_POST_START:        return "离岗识别开始";
        case LEAVE_POST_STOP:         return "离岗识别结束";
        case PERSON_FALL_DOWN_START:  return "人员倒地识别开始";
        case PERSON_FALL_DOWN_STOP:   return "人员倒地识别结束";
        case FENCE_CLIMBING_START:    return "翻越围栏识别开始";
        case FENCE_CLIMBING_STOP:     return "翻越围栏识别结束";
        case SMOKING_START:           return "抽烟识别开始";
        case SMOKING_STOP:            return "抽烟识别结束";
        case PHONE_USAGE_START:       return "玩手机识别开始";
        case PHONE_USAGE_STOP:        return "玩手机识别结束";
        case SMOKE_FIRE_START:        return "烟火识别开始";
        case SMOKE_FIRE_STOP:         return "烟火识别结束";
        case OPEN_FLAME_START:        return "明火识别开始";
        case OPEN_FLAME_STOP:         return "明火识别结束";
        case BARE_SOIL_START:         return "黄土裸露识别开始";
        case BARE_SOIL_STOP:          return "黄土裸露识别结束";
        case PERSON_TRIP_STAR:        return "摔倒识别开始";
        case PERSON_TRIP_STOP:        return "摔倒识别结束";
        case ELECTRIC_VEHICLE_IN_ELEVATOR_START:    return "电瓶车进电梯识别开始";
        case ELECTRIC_VEHICLE_IN_ELEVATOR_STOP:     return "电瓶车进电梯识别结束";
        case MANHOLE_COVER_ABNORMAL_START:          return "井盖异常检测开始";
        case MANHOLE_COVER_ABNORMAL_STOP:           return "井盖异常检测结束";
        case HOLE_PROTECTION_BAR_START:             return "洞口防护栏识别开始";
        case HOLE_PROTECTION_BAR_STOP:              return "洞口防护栏识别结束";
        case PEDESTRIAN_INTRUSION_START:            return "行人闯入识别开始";
        case PEDESTRIAN_INTRUSION_STOP:             return "行人闯入识别结束";
        /* 穿戴规范 */
        case SAFETY_HELMET_START:                   return "安全帽识别开始";
        case SAFETY_HELMET_STOP:                    return "安全帽识别结束";
        case REFLECTIVE_CLOTHING_START:             return "反光衣识别开始";
        case REFLECTIVE_CLOTHING_STOP:              return "反光衣识别结束";
        case HIGH_ALTITUDE_SEATBELT_START:          return "高空安全带识别开始";
        case HIGH_ALTITUDE_SEATBELT_STOP:           return "高空安全带识别结束";
        /* 交通行为监管 */
        case CONSTRUCTION_OCCUPY_ROAD_START:        return "施工占道识别开始";
        case CONSTRUCTION_OCCUPY_ROAD_STOP:         return "施工占道识别结束";
        case EMERGENCY_LANE_OCCUPANCY_START:        return "应急车道占用识别开始";
        case EMERGENCY_LANE_OCCUPANCY_STOP:         return "应急车道占用识别结束";
        case REVERSE_DIRECTION_START:               return "逆行识别开始";
        case REVERSE_DIRECTION_STOP:                return "逆行识别结束";
        case NON_MOTOR_VEHICLE_INTRUSION_START:     return "非机动车闯入识别开始";
        case NON_MOTOR_VEHICLE_INTRUSION_STOP:      return "非机动车闯入识别结束";
        case ROAD_PONDING_START:                    return "道路积水识别开始";
        case ROAD_PONDING_STOP:                     return "道路积水识别结束";
        case CONGESTION_START:                      return "拥堵识别开始";
        case CONGESTION_STOP:                       return "拥堵识别结束";
        case ILLEGAL_LANE_CHANGE_START:             return "违规变道识别开始";
        case ILLEGAL_LANE_CHANGE_STOP:              return "违规变道识别结束";
        /*属性识别*/
        case PLATE_NUMBER_START:                    return "车牌识别开始";
        case PLATE_NUMBER_STOP:                     return "车牌识别结束";
#endif

#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT
        case GARBAGE_EXPOSURE_START: return "垃圾暴露识别开始";
        case GARBAGE_EXPOSURE_STOP: return "垃圾暴露识别结束";
        case GARBAGE_OVERFLOW_START: return "垃圾满溢识别开始";
        case GARBAGE_OVERFLOW_STOP: return "垃圾满溢识别结束"; 
#endif
#if defined CAP_AI_FACE_COMPARE
        case FACE_COMPARE_SUCCESS_START: return "人脸比对成功开始";
        case FACE_COMPARE_SUCCESS_STOP: return "人脸比对成功结束";
        case FACE_COMPARE_FAIL_START: return "人脸比对失败开始";
        case FACE_COMPARE_FAIL_STOP: return "人脸比对失败结束"; 
#endif
#if CAP_AI_PEOPLE_STATISTICS
        case PEOPLE_FLOW_STATISTICS_START: return "人流统计开始";
        case PEOPLE_FLOW_STATISTICS_STOP: return "人流统计结束";
        case PEOPLE_DENSITY_DETECTION_START: return "人员密度检测开始";
        case PEOPLE_DENSITY_DETECTION_STOP: return "人员密度检测结束";
        case PEOPLE_FLOW_STAY_NORMAL_START: return "人流统计-滞留人数普通报警开始";
        case PEOPLE_FLOW_STAY_NORMAL_STOP: return "人流统计-滞留人数普通报警结束";
        case PEOPLE_FLOW_STAY_MEDIUM_START: return "人流统计-滞留人数中度报警开始";
        case PEOPLE_FLOW_STAY_MEDIUM_STOP: return "人流统计-滞留人数中度报警结束";
        case PEOPLE_FLOW_STAY_SEVERE_START: return "人流统计-滞留人数严重报警开始";
        case PEOPLE_FLOW_STAY_SEVERE_STOP: return "人流统计-滞留人数严重报警结束";
        case PEOPLE_DENSITY_NORMAL_START: return "人员密度检测-普通报警开始";
        case PEOPLE_DENSITY_NORMAL_STOP: return "人员密度检测-普通报警结束";
        case PEOPLE_DENSITY_MEDIUM_START: return "人员密度检测-中度报警开始";
        case PEOPLE_DENSITY_MEDIUM_STOP: return "人员密度检测-中度报警结束";
        case PEOPLE_DENSITY_SEVERE_START: return "人员密度检测-严重报警开始";
        case PEOPLE_DENSITY_SEVERE_STOP: return "人员密度检测-严重报警结束";
#endif

#ifdef SCENE_INTELLIGENT_ANALYSIS
        /*大模型场景智能分析*/
        case TEXT_PRESET_ALARM:                    return "AI文字预设任务符合条件";
#endif
        /* 剩余部分报警日志需实现后在这里添加 */
        /* --------------------报警日志类型-------------------- */

        /* --------------------异常日志类型-------------------- */
        /* 硬盘异常 */
        case DISK_FULL:                  return "硬盘满";
        case DISK_ERROR:                 return "硬盘错误";
        case DATA_DISK_ERROR:            return "数据盘错误";
        case DISK_HIGH_TEMP:             return "硬盘高温";
        case DISK_LOW_TEMP:              return "硬盘低温";
        case DISK_BAD_BLOCK:             return "硬盘坏块";
        case DISK_SHOCK:                 return "硬盘撞击";
        case DISK_CRITICAL_FAILURE:      return "硬盘严重故障";
        case DISK_SHM_ABNORMAL:          return "硬盘SHM检测异常";
        case CLUSTER_DISK_FULL:          return "集群硬盘满";
        case LOG_DISK_ABNORMAL:          return "日志盘异常";
        case SSD_ABNORMAL:               return "SSD异常";
        case SSD_TEMP_ABNORMAL:          return "SSD温度异常";
        case SSD_LIFE_ABNORMAL:          return "SSD寿命异常";
        /* 网络通信异常 */
        case NET_BROKEN:                 return "网络断开";
        case IP_CONFLICT:                return "IP冲突";
        case IPC_IP_CONFLICT:            return "IPC IP地址冲突";
        case IPC_OFFLINE:                return "IP Camera未链接";
        case DEVICE_OFFLINE:             return "设备掉线";
        case SUBSTREAM_CONNECT_FAIL:     return "ip通道子码流连接失败";
        case PORT_ABNORMAL:              return "端口异常";
        case CLOUD_STORAGE_ABNORMAL:     return "云存储异常";
        case DATA_UPLOAD_ABNORMAL:       return "数据上传异常";
        /* 电源与硬件 */
        case POE_POWER_ABNORMAL:         return "PoE供电异常";
        case IPC_REBOOT_ABNORMAL:        return "IPC异常重启";
        case POC_ABNORMAL:               return "POC模块异常";
        case HARDWARE_ABNORMAL:          return "配件版异常";
        case HOT_STANDBY_ABNORMAL:       return "热备异常";
        /* 视频流异常 */
        case VIDEO_LOST:                 return "视频信号丢失";
        case VIDEO_SIGNAL_ABNORMAL:      return "视频信号异常";
        case CAMERA_VIEW_ABNORMAL:       return "相机画面异常";
        case RESOLUTION_MISMATCH:        return "前端/录像分辨率不匹配";
        case VIDEO_FORMAT_MISMATCH:      return "视频输入/输出制式不匹配";
        case ENCODE_PARAM_ABNORMAL:      return "编码参数异常";
        case STREAM_CAUSED_SNAP_FAIL:    return "码流异常导致定时抓图失败";
        /* 录像抓图异常 */
        case RECORD_SNAP_ERROR:          return "录像/抓图异常";
        case SNAPSHOT_FAIL:              return "抓图失败";
        case RECORD_BUFFER_OVERFLOW:     return "录像缓冲区溢出";
        case OVERLOAD_CAUSED_SNAP_FAIL:  return "超出性能导致定时抓图失败";
        case RECORD_CYCLE_INSUFFICIENT:  return "录像周期不足";
        case ANR_RECORD_ABNORMAL:        return "ANR录像异常";
        /* 智能分析 */
        case INTELLIGENT_SYS_ABNORMAL:   return "智能系统运行异常";
        case INTELLIGENT_SCENE_ABNORMAL: return "智能分析场景异常";
        case INTELLIGENT_SYNC_ABNORMAL:  return "智能系统同步异常";
        case TARGET_SNAP_OVERFLOW:       return "目标抓拍码流分辨率超限";
        case SMD_STREAM_OVERFLOW:        return "SMD码流分辨率超限";
        case MOTION_STREAM_OVERFLOW:     return "移动侦测码流分辨率超限";
        case SAFETY_HELMET_ABNORMAL:     return "安全帽检测异常";
        /* 安全与访问 */
        case UNAUTHORIZED_ACCESS:        return "非法访问";
        case PASSWORD_SYNC_ABNORMAL:     return "密码同步异常";
        case ALARM_DEPLOY_FAIL:          return "IP通道报警布防失败";
        case ALARM_UPLOAD_FAIL:          return "报警上传失败";
        case VCA_PIC_UPLOAD_ABNORMAL:    return "VCA图片上传显示异常";
        /* 系统级异常 */
        case IPC_MIGRATION_FAIL:         return "IPC迁移失败";
        case CLUSTER_CONFIG_FAIL:        return "集群内设备配置失败";
        case DIAL_ABNORMAL:              return "拨号异常";
        case CHANNEL_MOTION_ABNORMAL:    return "IP通道移动侦测智能分析异常";
#if 0
        /* 萤石服务 */
        case EZ_OFFLINE_ABNORMAL:        return "萤石下线异常";
        case EZ_UPGRADE_ABNORMAL:        return "萤石升级异常";
        case EZ_OPERATION_ABNORMAL:      return "萤石操作异常";
#endif
        case TIME_SYNC_ABNORMAL:         return "时间同步异常";
        /* --------------------异常日志类型-------------------- */

        /* --------------------操作日志类型-------------------- */
        /* 基础操作 */
        case LOCAL_LOGIN:                   return "本地登录";
        case LOCAL_LOGOUT:                  return "本地注销";
        case REMOTE_LOGIN:                  return "远程登录";
        case REMOTE_LOGOUT:                 return "远程注销";
        case LOCAL_STARTUP:                 return "开机";
        case LOCAL_SHUTDOWN:                return "本地关机";
        case ABNORMAL_SHUTDOWN:             return "异常关机";
        case LOCAL_REBOOT:                  return "本地重启";
        case REMOTE_SHUTDOWN:               return "远程关机";
        case REMOTE_REBOOT:                 return "远程重启";
        /* 配置管理 */
        case LOCAL_CONFIG:                  return "本地配置";
        case REMOTE_CONFIG:                 return "远程配置";
        case LOCAL_IMPORT_CONFIG:           return "本地导入配置文件";
        case LOCAL_EXPORT_CONFIG:           return "本地导出配置文件";
        case REMOTE_IMPORT_CONFIG:          return "远程导入配置文件";
        case REMOTE_EXPORT_CONFIG:          return "远程导出配置文件";
        case LOCAL_SIMPLE_RESET:            return "本地简单恢复默认参数";
        case LOCAL_FACTORY_RESET:           return "本地恢复出厂默认参数";
        case REMOTE_SIMPLE_RESET:           return "远程简单恢复默认参数";
        case REMOTE_FACTORY_RESET:          return "远程恢复出厂默认参数";
        case LOCAL_RESET_ADMIN_PWD:         return "本地恢复管理员默认密码";
        case LOCAL_RESET_LOGIN_PWD:         return "本地重置登录密码";
        case REMOTE_RESET_LOGIN_PWD:        return "远程重置登录密码";
        case LOCAL_RESET_CHANNEL_PWD:       return "本地重置通道密码";
        case REMOTE_RESET_CHANNEL_PWD:      return "远程重置通道密码";
        /* 存储管理 */
        case LOCAL_ADD_NETDISK:             return "本地添加网络盘";
        case LOCAL_DEL_NETDISK:             return "本地删除网络盘";
        case LOCAL_SET_NETDISK:             return "本地设置网络盘";
        case REMOTE_ADD_NETDISK:            return "远程添加网络盘";
        case REMOTE_DEL_NETDISK:            return "远程删除网络盘";
        case REMOTE_SET_NETDISK:            return "远程设置网络盘";
        case LOCAL_INIT_DISK:               return "本地初始化硬盘";
        case LOCAL_DISK_SELF_TEST:          return "本地物理硬盘自检";
        case REMOTE_INIT_DISK:              return "远程初始化硬盘";
        case REMOTE_MOUNT_DISK:             return "远程加载硬盘";
        case REMOTE_UNMOUNT_DISK:           return "远程卸载硬盘";
        case REMOTE_CREATE_STORAGE_POOL:    return "远程创建存储池";
        case REMOTE_DEL_STORAGE_POOL:       return "远程删除存储池";
        case REMOTE_SET_STORAGE_PARAMS:     return "远程设置存储池参数";
        case REMOTE_SET_STORAGE_CAPACITY:   return "远程设置存储池容量";
        case REMOTE_DEL_INVALID_DISK:       return "远程删除异常不存在的硬盘";
        case STORAGE_MEDIA_ERASE:           return "存储介质安全擦除";
        /* 通道管理 */
        case LOCAL_ADD_IP_CHANNEL:          return "本地添加IP通道";
        case LOCAL_DEL_IP_CHANNEL:          return "本地删除IP通道";
        case LOCAL_CONFIG_IP_CHANNEL:       return "本地配置IP通道";
        case REMOTE_ADD_IP_CHANNEL:         return "远程添加IP通道";
        case REMOTE_DEL_IP_CHANNEL:         return "远程删除IP通道";
        case REMOTE_SET_IP_CHANNEL:         return "远程设置IP通道";
        case LOCAL_ADD_IOT_CHANNEL:         return "本地添加IoT通道";
        case LOCAL_DEL_IOT_CHANNEL:         return "本地删除IoT通道";
        case LOCAL_CONFIG_IOT_CHANNEL:      return "本地配置IoT通道";
        case REMOTE_ADD_IOT_CHANNEL:        return "远程添加IoT通道";
        case REMOTE_DEL_IOT_CHANNEL:        return "远程删除IoT通道";
        case REMOTE_CONFIG_IOT_CHANNEL:     return "远程配置IoT通道";
        case CHANNEL_SORT:                  return "通道排序";
        case CHANNEL_DND_CONFIG:            return "通道勿扰配置";
        /* 媒体操作 */
        case LOCAL_START_RECORDING:         return "本地启动录像";
        case LOCAL_STOP_RECORDING:          return "本地停止录像";
        case LOCAL_START_SNAPSHOT:          return "本地启动抓图";
        case LOCAL_STOP_SNAPSHOT:           return "本地停止抓图";
        case REMOTE_START_RECORDING:        return "远程启动录像";
        case REMOTE_STOP_RECORDING:         return "远程停止录像";
        case REMOTE_START_SNAPSHOT:         return "远程启动抓图";
        case REMOTE_STOP_SNAPSHOT:          return "远程停止抓图";
        case LOCAL_FILE_PLAYBACK:           return "本地按文件回放";
        case LOCAL_TIME_PLAYBACK:           return "本地按时间回放";
        case REMOTE_FILE_PLAYBACK:          return "远程按文件回放";
        case REMOTE_TIME_PLAYBACK:          return "远程按时间回放";
        case LOCAL_BACKUP_IMAGES:           return "本地备份图片文件";
        case LOCAL_BACKUP_RECORDS:          return "本地备份录像文件";
        case REMOTE_BACKUP_IMAGES:          return "远程备份图片文件";
        case REMOTE_BACKUP_RECORDS:         return "远程备份录像文件";
        case REMOTE_DEL_IMAGE_DATA:         return "远程删除图片数据";
        case REMOTE_DEL_RECORD_SEGMENT:     return "远程删除录像片段操作";
        case LOCAL_PTZ_CTRL:                return "本地云台控制";
        case REMOTE_PTZ_CTRL:               return "远程云台控制";
        /* 网络通信 */
        case LOCAL_DIAL_CTRL:               return "本地手动启动断开拨号";
        case LOCAL_SET_DIAL_PARAM:          return "本地配置拨号参数";
        case LOCAL_SET_DIAL_PLAN:           return "本地配置拨号计划";
        case REMOTE_START_DIAL:             return "远程开启手动拨号";
        case REMOTE_STOP_DIAL:              return "远程停止手动拨号";
        case REMOTE_SET_DIAL_PARAM:         return "远程配置拨号参数";
        case REMOTE_SET_DIAL_PLAN:          return "远程配置拨号计划";
        case LOCAL_CONFIG_SIP_SERVER:       return "本地配置SIP Server";
        case REMOTE_CONFIG_SIP_SERVER:      return "远程配置SIP Server";
        case CREATE_TUNNEL:                 return "建立透明通道";
        case DISCONNECT_TUNNEL:             return "断开透明通道";
        case LOCAL_MANUAL_TIME_SYNC:        return "本地手动校时";
        case REMOTE_MANUAL_TIME_SYNC:       return "远程手动校时";
        case LOCAL_WPS_CONNECT:             return "本地WPS连接";
        case REMOTE_WPS_CONNECT:            return "远程WPS连接";
        case LOCAL_IP_ADAPTIVE_CONFIG:      return "本地IP自适应配置";
        case REMOTE_IP_ADAPTIVE_CONFIG:     return "远程IP自适应配置";
        case LOCAL_CONFIG_SNMP:             return "本地配置SNMP";
        case REMOTE_CONFIG_SNMP:            return "远程配置SNMP";
        case LOCAL_CONFIG_EMAIL:             return "本地配置EMAIL";
        case REMOTE_CONFIG_EMAIL:            return "远程配置EMAIL";
        /* 报警与通知 */
        case LOCAL_MANUAL_ALARM:            return "本地手动报警";
        case REMOTE_MANUAL_ALARM:           return "远程手动报警";
        case LOCAL_ALARM_ARM:               return "本地报警布防";
        case REMOTE_ALARM_ARM:              return "远程布防";
        case LOCAL_ALARM_DISARM:            return "本地撤防";
        case REMOTE_ALARM_DISARM:           return "远程撤防";
        case SEND_SMS_ALARM:                return "发送短信报警";
        case LOCAL_SEND_SMS:                return "本地发送短信";
        case LOCAL_VIEW_SMS:                return "本地查看短信";
        case LOCAL_SEARCH_SMS:              return "本地搜索短信";
        case REMOTE_SEND_SMS:               return "远程发送短信";
        case REMOTE_VIEW_SMS:               return "远程查看短信";
        case REMOTE_SEARCH_SMS:             return "远程搜索短信";
        case RECEIVE_SMS:                   return "接收短信";
        case CALL_CTRL_ONLINE:              return "呼叫控制上线";
        case SMS_CTRL_ONLINE:               return "短信控制上下线";
        case VOICE_TALK_START:              return "语音对讲开始";
        case VOICE_TALK_END:                return "语音对讲结束";
        case CUSTOM_VOICE_OPERATION:        return "自定义语音操作";
        /* 系统管理 */
        case LOCAL_ACTIVATE_DEVICE:         return "本地激活设备";
        case REMOTE_ACTIVATE_DEVICE:        return "远程激活设备";
        case LOCAL_UPGRADE:                 return "本地升级";
        case REMOTE_MAINTENANCE:            return "远程升级、格式化";
        case LOCAL_UPGRADE_IPC_FIRMWARE:    return "本地升级IP通道固件";
        case REMOTE_UPGRADE_IPC_FIRMWARE:   return "远程升级IP通道固件";
        case LOCAL_ENABLE_CLOUD_SYS:        return "本地启用云系统";
        case REMOTE_ENABLE_CLOUD_SYS:       return "远程启用云系统";
        case LOCAL_DISABLE_CLOUD_SYS:       return "本地禁用云系统";
        case REMOTE_DISABLE_CLOUD_SYS:      return "远程禁用云系统";
        case LOCAL_AUTO_REBOOT_CONFIG:      return "本地配置自动重启";
        case REMOTE_AUTO_REBOOT_CONFIG:     return "远程配置自动重启";
        case LOCAL_STORAGE_OPTIMIZE:        return "本地存储优化配置";
        case REMOTE_STORAGE_OPTIMIZE:       return "远程存储优化配置";
        /* 智能功能 */
        case LOCAL_CONFIG_ANALYTICS:        return "本地智能分析配置";
        case REMOTE_CONFIG_ANALYTICS:       return "远程智能分析配置";
        case LOCAL_CREATE_TARGET_DB:        return "本地创建目标对比库";
        case LOCAL_MODIFY_TARGET_DB:        return "本地修改目标对比库";
        case LOCAL_DEL_TARGET_DB:           return "本地删除目标对比库";
        case LOCAL_IMPORT_TARGET_IMG:       return "本地导入目标图片";
        case LOCAL_MODIFY_TARGET_IMG:       return "本地修改目标图片";
        case LOCAL_DEL_TARGET_IMG:          return "本地删除目标图片";
        case LOCAL_IMPORT_TARGET_DB:        return "本地导入目标库";
        case LOCAL_EXPORT_TARGET_DB:        return "本地导出目标库";
        case REMOTE_CREATE_TARGET_DB:       return "远程创建目标库";
        case REMOTE_MODIFY_TARGET_DB:       return "远程修改目标库";
        case REMOTE_DEL_TARGET_DB:          return "远程删除目标库";
        case REMOTE_IMPORT_TARGET_IMG:      return "远程导入目标图片";
        case REMOTE_MODIFY_TARGET_IMG:      return "远程修改目标图片";
        case REMOTE_DEL_TARGET_IMG:         return "远程删除目标图片";
        case REMOTE_TARGET_COMPARE_CONFIG:  return "远程目标比对任务配置";
        case LOCAL_IMPORT_FILTER_DB_IMG:    return "本地导入过滤库图片";
        case LOCAL_DEL_FILTER_DB_IMG:       return "本地删除过滤库图片";
        case LOCAL_ENABLE_FILTER_DB:        return "本地配置过滤库使能";
        case LOCAL_FILTER_DB_UPDATE:        return "本地过滤库更新";
        case REMOTE_IMPORT_FILTER_DB_IMG:   return "远程导入过滤库图片";
        case REMOTE_DEL_FILTER_DB_IMG:      return "远程删除过滤库图片";
        case REMOTE_ENABLE_FILTER_DB:       return "远程配置过滤库使能";
        case REMOTE_FILTER_DB_UPDATE:       return "远程过滤库更新";
        case LOCAL_ALGORITHM_DISPATCH:      return "本地算法包调度";
        case REMOTE_ALGORITHM_DISPATCH:     return "远程算法包调度";
        /* 特殊硬件 */
        case LOCAL_SET_POE_MODE:            return "本地设置PoE工作模式";
        case LOCAL_POE_CONFIG:              return "本地POE配置";
        case REMOTE_SET_POE_MODE:           return "远程设置PoE工作模式";
        case REMOTE_POE_CONFIG:             return "远程POE配置";
        case LOCAL_SSD_INIT_START:          return "本地SSD初始化开始";
        case LOCAL_SSD_INIT_END:            return "本地SSD初始化结束";
        case REMOTE_SSD_INIT_START:         return "远程SSD初始化开始";
        case REMOTE_SSD_INIT_END:           return "远程SSD初始化结束";
        case SSD_SELF_REPAIR:               return "SSD自修复";
#if 0
        /* 萤石功能 */
        case EZ_REGISTER:                   return "萤石注册";
        case EZ_UPGRADE:                    return "萤石升级";
        case LOCAL_EZ_CONFIG:               return "本地萤石参数配置";
        case REMOTE_EZ_CONFIG:              return "远程萤石参数配置";
        case LOCAL_EZ_OPERATION:            return "本地萤石操作";
        case REMOTE_EZ_OPERATION:           return "远程萤石操作";
        case EZ_STREAM_CONFIG:              return "萤石码流参数配置";
        case EZ_ALARM_CONFIG:               return "萤石报警参数配置";
#endif
        /* 附加功能 */
        case SVC_CONFIG:                    return "SVC配置";
        case LOCAL_ENABLE_DECODE_ENHANCE:   return "本地开启解码增强功能";
        case LOCAL_DISABLE_DECODE_ENHANCE:  return "本地关闭解码增强功能";
        case REMOTE_ENABLE_DECODE_ENHANCE:  return "远程开启解码增强功能";
        case REMOTE_DISABLE_DECODE_ENHANCE: return "远程关闭解码增强功能";
        case LOCAL_MANAGE_WORKER:           return "本地添加/删除工作机";
        case REMOTE_MANAGE_WORKER:          return "远程添加/删除工作机";
        case LOCAL_PIN_OPERATION:           return "本地PIN功能操作";
        case REMOTE_PIN_OPERATION:          return "远程PIN功能操作";
        case LOCAL_TAG_OPERATION:           return "本地标签操作";
        case REMOTE_TAG_OPERATION:          return "远程标签操作";
        case LOCAL_OPERATION_LOCK:          return "本地操作锁定";
        case LOCAL_OPERATION_UNLOCK:        return "本地操作解除锁定";
        case LOCAL_FILE_LOCK:               return "本地锁定文件";
        case LOCAL_FILE_UNLOCK:             return "本地解锁文件";
        case REMOTE_FILE_LOCK:              return "远程锁定文件";
        case REMOTE_FILE_UNLOCK:            return "远程解锁文件";
        case LOCAL_EXPORT_PASSENGER_FLOW:   return "本地导出客流量文件";
        case LOCAL_EXPORT_HEATMAP:          return "本地导出热度图文件";
        case LOCAL_EXPORT_ALLOW_DENY_LIST:  return "本地导出允许拒绝名单文件";
        case LOCAL_EXPORT_IP_CHANNEL_LIST:  return "本地导出IP通道列表文件";
        case LOCAL_IMPORT_ALLOW_DENY_LIST:  return "本地导入允许拒绝名单文件";
        case LOCAL_IMPORT_IP_CHANNEL_LIST:  return "本地导入IP通道列表文件";
        case LOCAL_CONFIG_ALLOW_LIST:       return "本地配置允许名单";
        case REMOTE_EXPORT_ALLOW_DENY_LIST: return "远程导出允许拒绝名单文件";
        case REMOTE_EXPORT_IP_CHANNEL_LIST: return "远程导出IP通道列表文件";
        case REMOTE_IMPORT_ALLOW_DENY_LIST: return "远程导入允许拒绝名单文件";
        case REMOTE_IMPORT_IP_CHANNEL_LIST: return "远程导入IP通道列表文件";
        case REMOTE_CONFIG_ALLOW_LIST:      return "远程配置允许名单";
        case REMOTE_GET_PARAMS:             return "远程获取参数";
        case REMOTE_GET_STATUS:             return "远程获取状态";
        case LOCAL_CLUSTER_CONFIG:          return "本地集群组网配置";
        case LOCAL_CLUSTER_ADD_DEV:         return "本地集群添加设备";
        case LOCAL_CLUSTER_DEL_DEV:         return "本地集群删除设备";
        case REMOTE_CLUSTER_CONFIG:         return "远程集群组网配置";
        case REMOTE_CLUSTER_ADD_DEV:        return "远程集群添加设备";
        case REMOTE_CLUSTER_DEL_DEV:        return "远程集群删除设备";
        case LOCAL_IMPORT_IOT_CONFIG:       return "本地导入IoT配置文件";
        case LOCAL_EXPORT_IOT_CONFIG:       return "本地导出IoT配置文件";
        case REMOTE_IMPORT_IOT_CONFIG:      return "远程导入IoT配置文件";
        case REMOTE_EXPORT_IOT_CONFIG:      return "远程导出IoT配置文件";
        /* --------------------操作日志类型-------------------- */

        /* --------------------信息日志类型-------------------- */
        /* 系统基础状态 */
        case SYSTEM_STATUS:         return "系统运行状态信息";
        case DISK_INFO:             return "本地硬盘信息";
        case NET_DISK_INFO:         return "网络硬盘信息";
        case RAID_INFO:             return "raid相关信息";
        case SMART_INFO:            return "S.M.A.R.T信息";
        case WORKER_STATUS:         return "工作机信息";
        case DEVICE_RUNTIME:        return "设备运行时长";
        /* 录像抓图状态 */
        case ANR_RECORD_START:      return "ANR录像开始";
        case ANR_RECORD_END:        return "ANR录像结束";
        case SNAPSHOT_START:        return "开始抓图";
        case RECORD_START:          return "开始录像";
        case SNAPSHOT_STOP:         return "结束抓图";
        case RECORD_STOP:           return "结束录像";
        case EXPIRE_RECORD_DELETED: return "删除过期录像";
        case RECORD_STOP_INFO:      return "停止录像信息";
        case ANR_SCHEDULE_ADD:      return "IP通道ANR时间段添加";
        case ANR_SCHEDULE_DEL:      return "IP通道ANR时间段删除";
        /* 网络与连接 */
        case DIAL_STATUS:           return "拨号状态";
        case IPC_CONNECTION:        return "IPC连接状态";
        case NETWORK_CONNECTED:     return "ipc连接";
        case PLATFORM_STATUS:       return "平台信息";
        case IEEE8021X_AUTH_FAILED: return "802.1X认证失败";
        case RE_AUTH_PASSED:        return "二次认证通过";
        /* 存储操作 */
        case DISK_FORMAT_START:     return "硬盘格式化开始";
        case DISK_FORMAT_END:       return "硬盘格式化结束";
        case DATA_REBUILD_START:    return "数据重建开始";
        case DATA_REBUILD_END:      return "数据重建结束";
        /* 集群与热备 */
        case CLUSTER_DEV_ONLINE:    return "集群设备上线";
        case CLUSTER_SERVICE_START: return "集群管理服务启动";
        case CLUSTER_MIGRATION:     return "集群业务迁移";
        case CLUSTER_STATUS:        return "集群状态信息";
        case HOTBACKUP_START:       return "热备机开始备份工作机";
        case HOTBACKUP_STOP:        return "热备机停止备份工作机";
        /* 智能与识别 */
        case SMART_BOARD_STATUS:    return "智能板状态";
        case TARGET_EVENT_NO_IMAGE: return "目标事件（目标侦测&目标抓拍）无图片";
        /* 其他状态 */
        case TIME_SYNC:             return "系统校时";
#if 0
        case EZ_RUNTIME_STATUS:     return "萤石运行状态";
        case EZ_LINKAGE_INFO:       return "萤石联动信息";
#endif
        case OPS_NOT_EXECUTED:      return "运维未执行";
        /* --------------------信息日志类型-------------------- */

        default:return "全部类型";
        }
    }
    /**
    * @brief 日志信息结构体
    */
    typedef struct Info
    {
        std::string startTime; /* 记录日志的开始时间 */
        int nType = 0;         /* 日志主类型 */
        int nAction = 0;       /* 日志次类型 */
        std::string chnName;   /* 通道名称标识符 */
        std::string user;      /* 用户名 */
        std::string host;      /* 主机名 */
        std::string context;   /* 日志内容 */
        Info() {}
        Info(std::string startTime, int nType, int nAction, std::string chnName, std::string user, std::string host, std::string context)
            : startTime(startTime), nType(nType), nAction(nAction), chnName(chnName), user(user), host(host), context(context) {}
    } Info_S;
    /**
    * @brief 日志检索条件
    */
    typedef struct RetrievalCond
    {
        Type_E enType = Type::ALL;   /* 日志主类型 */
        Action_E enAction = Action::UNDEFINED; /* 日志次类型 */
        std::string startTime;       /* 开始时间 */
        std::string endTime;         /* 结束时间 */
    } RetrievalCond_S;
}
