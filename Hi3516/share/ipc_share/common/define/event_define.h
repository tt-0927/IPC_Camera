/***
 * @FilePath     : event_define.h
 * @Author       : 严泽辉 (yanzeh@kfb.cn)
 * @Date         : 2024-10-09 19:29:22
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-04-28 19:50:16
 * @Description  : 事件定义
 */

#pragma once

#include <list>
#include <string>
#include <vector>
#include <set>
#include "common_define.h"
#include <string.h>

namespace Event
{
    /* 事件类别 */
    typedef enum class Category
    {
        ABNORMAL = 0,  /* 异常事件 */
        NORMAL = 1,    /* 普通事件 */
        PERIMETER = 2, /* 周界事件 */
        SMART = 3,     /* 智能事件 */
        OTHER = 4,     /* 其他事件 */
    } Category_E;

    /* 智能事件类别 */
    typedef enum class _SmartCategory_E_
    {
        UNKNOWN = 0,              /* 未知类别 */
        PERIMETER = 1,            /* 周界事件 */
        BEHAVIOURAL_ANALYSIS = 2, /* 行为分析 */
        SCENE_DETECTION = 3,      /* 场景检测 */
        TARGET_DETECTION = 4,     /* 目标检测 */
        FACE_CAPTURE = 5,         /* 人脸抓拍 */
        BEHAVIOR_MONITORING = 6,  /* 行为监管 */
        CLOTHING_COMPLIANCE = 7,  /* 穿戴规范 */
        TRAFFIC_BEHAVIOR_MONITORING = 8, /* 交通行为监管 */
        ATTRIBUTE_RECOGNITION = 9, /* 属性识别 */
        SCENE_ANALYSIS = 10,    /* 场景智能分析 */

#if CAP_AI_PEOPLE_STATISTICS
        PEOPLE_STATISTICS = 11, /* 人数统计 */
#endif

        FACE_COMPARE = 12,         /* 人脸比对 */
    } SmartCategory_E;

    /* 事件类型枚举 */
    typedef enum class Type
    {
        UNKNOWN = -1, /* 未知事件 */
        /**
         * @brief   : 普通事件
         */
        MOTION_DETECT = 0,    /* 移动侦测 */
        OCCLUSION_DETECT = 1, /* 遮挡侦测 */
        ANOMALY_ALARM = 2,    /* 异常报警 */
        AUDIO_ALARM = 3,      /* 声音报警 */
        ALARM_INPUT = 4,      /* 报警输入 */
        ALARM_OUTPUT = 5,     /* 报警输出 */
        FLASH_ALARM = 6,      /* 闪光灯报警 */
        PIR_ALARM = 7,        /* PIR 红外感应报警 */

        /**
         * @brief   : 周界事件
         */
        LINE_CROSSING = 8, /* 越界侦测 */
        INTRUSION = 9,     /* 区域入侵 */
        ENTER_REGION = 10, /* 进入区域 */
        LEAVE_REGION = 11, /* 离开区域 */

        /**
         * @brief   : Smart事件
         */
        AUDIO_ANOMALY = 12,     /* 音频异常侦测 */
        AUDIO_SUDDEN_RISE = 121,/* 音频异常侦测-声强陡升检测 */
        AUDIO_SUDDEN_DROP = 122,/* 音频异常侦测-声强陡降检测 */
        SCENE_CHANGE = 13,      /* 场景变更 */
        FACE_DETECT = 14,       /* 人脸侦测 */
        LOITERING_DETECT = 15,  /* 徘徊侦测 */
        CROWD_GATHERING = 16,   /* 人员聚集 */
        PARKING_DETECT = 17,    /* 停车侦测 */
        UNATTENDED_OBJECT = 18, /* 物品遗留 */
        OBJECT_REMOVAL = 19,    /* 物品拿取 */
        PET_RECOGNITION = 20,   /* 宠物识别 */
        FACE_CAPTURE = 21,      /* 人脸抓拍 */
        FACE_LIB = 210,      /* 人脸库 */
        FACE_COMPARE = 211,      /* 人脸比对 */
        FACE_COMPARE_SUCCESS = 212,      /* 人脸比对成功联动 */
        FACE_COMPARE_FAIL = 213,      /* 人脸比对 失败*/
        /**
        * @brief   : 场景智能分析
        */
        IMAGE_ANALYSIS = 22,      /* AI图像分析 */
        TEXT_PRESET = 23,         /* AI文本预设任务 */
        REAL_ALARM = 24,          /* AI实时预警 */

       /**
        * @brief   : 场景智能
        */
        // ========== 行为监管 ==========
        SLEEP_ON_DUTY = 25,           /* 睡岗识别 */
        LEAVE_POST = 26,              /* 离岗识别 */
        ELECTRIC_VEHICLE_IN_ELEVATOR = 27, /* 电瓶车进电梯识别 */
        PERSON_FALL_DOWN = 28,        /* 人员倒地识别 */
        FENCE_CLIMBING = 29,          /* 翻越围栏识别 */
        SMOKING = 30,                 /* 抽烟识别 */
        PHONE_USAGE = 31,             /* 玩手机识别 */
        GARBAGE_EXPOSURE = 32,        /* 垃圾暴露识别 */
        SMOKE_FIRE = 33,              /* 烟火识别 */
        OPEN_FLAME = 34,              /* 明火识别 */
        GARBAGE_OVERFLOW = 35,        /* 垃圾满溢识别 */
        MANHOLE_COVER_ABNORMAL = 36,  /* 井盖异常检测 */
        BARE_SOIL = 37,               /* 黄土裸露识别 */
        HOLE_PROTECTION_BAR = 38,     /* 洞口防护栏识别 */
        PEDESTRIAN_INTRUSION = 39,    /* 行人闯入识别 */
        PERSON_TRIP = 52,             /*  摔倒识别 */

        // ========== 穿戴规范 ==========
        SAFETY_HELMET = 40,           /* 安全帽识别 */
        REFLECTIVE_CLOTHING = 41,     /* 反光衣识别 */
        HIGH_ALTITUDE_SEATBELT = 42,  /* 高空安全带识别 */

        // ========== 交通行为监管 ==========
        CONSTRUCTION_OCCUPY_ROAD = 43, /* 施工占道识别 */
        EMERGENCY_LANE_OCCUPANCY = 44, /* 应急车道占用识别 */
        REVERSE_DIRECTION = 45,       /* 逆行识别 */
        NON_MOTOR_VEHICLE_INTRUSION = 46, /* 非机动车闯入识别 */
        ROAD_PONDING = 47,            /* 道路积水识别 */
        CONGESTION = 48,              /* 拥堵识别 */
        ILLEGAL_PARKING = 49,         /* 违规停车识别 */
        ILLEGAL_LANE_CHANGE = 50,     /* 违规变道识别 */
        // ========== 属性识别 ==========
        PLATE_NUMBER = 51,           /*  车牌识别 */

        /**
        * @brief   : 场景智能分析开关
        */
        AI_SCENE_ANALYSIS = 53,      /* AI场景智能分析开关 */

#if CAP_AI_PEOPLE_STATISTICS
        // info ========== 人数统计 ==========
        PEOPLE_FLOW_STATISTICS = 54, /* 人流统计 */
        PEOPLE_DENSITY_DETECTION = 55, /* 人员密度检测 */
        PEOPLE_FLOW_STAY_NORMAL = 541, /* 人流统计-滞留人数普通报警 */
        PEOPLE_FLOW_STAY_MEDIUM = 542, /* 人流统计-滞留人数中度报警 */
        PEOPLE_FLOW_STAY_SEVERE = 543, /* 人流统计-滞留人数严重报警 */
        PEOPLE_DENSITY_NORMAL = 551, /* 人员密度检测-普通报警 */
        PEOPLE_DENSITY_MEDIUM = 552, /* 人员密度检测-中度报警 */
        PEOPLE_DENSITY_SEVERE = 553, /* 人员密度检测-严重报警 */
#endif

        /**
         * @brief   : 事件其他特殊类型
         */
        LINKAGE = 1000,      /* 联动事件 */
        LABEL = 1001,        /* 标签 */
        SCREENSHOT = 1002,   /* 抓图事件 */
        HUMAN_RECORD = 1003, /* 人工录制事件 */

        /**
         * @brief   : 异常事件
         */
        DISK_FULL = 2000,               /* 硬盘满 */
        DISK_ERROR = 2001,              /* 硬盘错误 */
        NET_BROKEN = 2002,              /* 网络断开 */
        IP_CONFLICT = 2003,             /* IP冲突 */
        ILLEGAL_ACCESS = 2004,          /* 非法访问 */
        RECORD_ABNORMAL = 2005,         /* 录像异常 */
        HARDWARE_ABNORMAL = 2006,       /* 配件版异常 */
        IP_CHANNEL_CONFLICT = 2007,     /* IP通道冲突 */
        STREAM_RESOLUTION_LIMIT = 2008, /* 子码流分辨率/码率超限 */
        VIDEO_SIGNAL_LOSS = 2009,       /* 视频信号丢失 */

        /**
         * @brief   : 属性识别事件
         */
        PEDESTRIAN_ATTRIBUTE  = 3000,
        MOTORVEHICLE_ATTRIBUTE = 3001,
        NONMOTORVEHICLE_ATTRIBUTE = 3002,
    } Type_E;

    /* 判断事件类别 */
    inline Category_E to_category(Type_E type)
    {
        switch (type)
        {
        /* 异常事件 */
        case Type::DISK_FULL:
        case Type::DISK_ERROR:
        case Type::NET_BROKEN:
        case Type::IP_CONFLICT:
        case Type::ILLEGAL_ACCESS:
        case Type::RECORD_ABNORMAL:
        case Type::HARDWARE_ABNORMAL:
        case Type::IP_CHANNEL_CONFLICT:
        case Type::STREAM_RESOLUTION_LIMIT:
        case Type::VIDEO_SIGNAL_LOSS:
            return Category_E::ABNORMAL;
        /* 普通事件 */
        case Type::MOTION_DETECT:
        case Type::OCCLUSION_DETECT:
        case Type::ANOMALY_ALARM:
        case Type::AUDIO_ALARM:
        case Type::ALARM_INPUT:
        case Type::ALARM_OUTPUT:
        case Type::FLASH_ALARM:
        case Type::PIR_ALARM:
            return Category_E::NORMAL;
        /* 周界事件 */
        case Type::LINE_CROSSING:
        case Type::INTRUSION:
        case Type::ENTER_REGION:
        case Type::LEAVE_REGION:
            return Category_E::PERIMETER;
        /* 智能事件 */
        case Type::AUDIO_ANOMALY:
        case Type::AUDIO_SUDDEN_RISE:
        case Type::AUDIO_SUDDEN_DROP:
        case Type::SCENE_CHANGE:
        case Type::FACE_DETECT:
        case Type::LOITERING_DETECT:
        case Type::CROWD_GATHERING:
        case Type::PARKING_DETECT:
        case Type::UNATTENDED_OBJECT:
        case Type::OBJECT_REMOVAL:
        case Type::PET_RECOGNITION:
        case Type::FACE_CAPTURE:
        case Type::FACE_COMPARE:/*人脸对比*/
        case Type::IMAGE_ANALYSIS:
        case Type::TEXT_PRESET:
        case Type::REAL_ALARM:
        case Type::SLEEP_ON_DUTY:
        case Type::LEAVE_POST:
        case Type::ELECTRIC_VEHICLE_IN_ELEVATOR:
        case Type::PERSON_FALL_DOWN:
        case Type::FENCE_CLIMBING:
        case Type::SMOKING:
        case Type::PHONE_USAGE:
        case Type::GARBAGE_EXPOSURE:
        case Type::SMOKE_FIRE:
        case Type::OPEN_FLAME:
        case Type::GARBAGE_OVERFLOW:
        case Type::MANHOLE_COVER_ABNORMAL:
        case Type::BARE_SOIL:
        case Type::HOLE_PROTECTION_BAR:
        case Type::PEDESTRIAN_INTRUSION:
        case Type::SAFETY_HELMET:
        case Type::REFLECTIVE_CLOTHING:
        case Type::HIGH_ALTITUDE_SEATBELT:
        case Type::CONSTRUCTION_OCCUPY_ROAD:
        case Type::EMERGENCY_LANE_OCCUPANCY:
        case Type::REVERSE_DIRECTION:
        case Type::NON_MOTOR_VEHICLE_INTRUSION:
        case Type::ROAD_PONDING:
        case Type::CONGESTION:
        case Type::ILLEGAL_PARKING:
        case Type::ILLEGAL_LANE_CHANGE:
        case Type::PLATE_NUMBER:
        case Type::PERSON_TRIP:
#if CAP_AI_PEOPLE_STATISTICS
        case Type::PEOPLE_FLOW_STATISTICS:
        case Type::PEOPLE_DENSITY_DETECTION:
        case Type::PEOPLE_FLOW_STAY_NORMAL:
        case Type::PEOPLE_FLOW_STAY_MEDIUM:
        case Type::PEOPLE_FLOW_STAY_SEVERE:
        case Type::PEOPLE_DENSITY_NORMAL:
        case Type::PEOPLE_DENSITY_MEDIUM:
        case Type::PEOPLE_DENSITY_SEVERE:
#endif
            return Category_E::SMART;
        default:
            return Category_E::OTHER;
        }
    }

    /// @brief 快捷入口类型
    typedef enum class EntryType
    {
        INVALID = -1,                     /* 无效 */
        SEARCH_EVENT_OVERSHOOT = 0,       /* 越界侦测 */
        SEARCH_EVENT_INTRUSION = 1,       /* 入侵侦测 */
        SEARCH_EVENT_ENTRY = 2,           /* 进入侦测 */
        SEARCH_EVENT_EXIT = 3,            /* 离开侦测 */
        SEARCH_EVENT_TARGET_CAPTURE = 4,  /* 目标抓拍侦测 */
        SEARCH_EVENT_TARGET_COMPARE = 5,  /* 目标对比侦测 */
        SEARCH_EVENT_VEHICLE = 6,         /* 车辆侦测 */
        SEARCH_EVENT_ELECTRIC_BIKE = 7,   /* 电瓶车侦测 */
        SEARCH_EVENT_MOTION_DETECT = 8,   /* 移动侦测 */
        SEARCH_EVENT_OCCLUSION_ALARM = 9, /* 遮挡报警侦测 */

        SEARCH_VIDEO_ALL = 1000,   /* 视频检索-全部 */
        SEARCH_VIDEO_LABEL = 1001, /* 视频检索-标签 */
        SEARCH_VIDEO_LOCK = 1002,  /* 视频检索-锁定 */

        SEARCH_IMAGE_ALL = 2000,   /* 图片检索-全部 */
        SEARCH_IMAGE_EVENT = 2001, /* 图片检索-事件 */
        SEARCH_IMAGE_CUT = 2002,   /* 图片检索-抓图 */

        SEARCH_FACE_COMPARE_TIME = 3000, /* 人员检索-时间 */
        SEARCH_FACE_COMPARE_IMG = 3001,  /* 人员检索-图片 */
        SEARCH_FACE_COMPARE_TXT = 3002,  /* 人员检索-文字 */

        SEARCH_VEHICLE = 4000, /* 车辆检索 */
    } EntryType_E;

    /// @brief 快捷入口转译
    inline std::string to_string(EntryType_E type)
    {
        switch (type)
        {
        case EntryType::INVALID:
            return "无效";
        case EntryType::SEARCH_VIDEO_ALL:
            return "视频检索-全部";
        case EntryType::SEARCH_VIDEO_LABEL:
            return "视频检索-标签";
        case EntryType::SEARCH_VIDEO_LOCK:
            return "视频检索-锁定";
        case EntryType::SEARCH_IMAGE_ALL:
            return "图片检索-全部";
        case EntryType::SEARCH_IMAGE_EVENT:
            return "图片检索-事件";
        case EntryType::SEARCH_IMAGE_CUT:
            return "图片检索-抓图";
        case EntryType::SEARCH_FACE_COMPARE_TIME:
            return "人员检索-时间";
        case EntryType::SEARCH_FACE_COMPARE_IMG:
            return "人员检索-图片";
        case EntryType::SEARCH_FACE_COMPARE_TXT:
            return "人员检索-文字";
        case EntryType::SEARCH_VEHICLE:
            return "车辆检索";
        case EntryType::SEARCH_EVENT_OVERSHOOT:
            return "越界侦测";
        case EntryType::SEARCH_EVENT_INTRUSION:
            return "入侵侦测";
        case EntryType::SEARCH_EVENT_ENTRY:
            return "进入侦测";
        case EntryType::SEARCH_EVENT_EXIT:
            return "离开侦测";
        case EntryType::SEARCH_EVENT_TARGET_CAPTURE:
            return "目标抓拍侦测";
        case EntryType::SEARCH_EVENT_TARGET_COMPARE:
            return "目标对比侦测";
        case EntryType::SEARCH_EVENT_VEHICLE:
            return "车辆侦测";
        case EntryType::SEARCH_EVENT_ELECTRIC_BIKE:
            return "电瓶车侦测";
        case EntryType::SEARCH_EVENT_MOTION_DETECT:
            return "移动侦测";
        case EntryType::SEARCH_EVENT_OCCLUSION_ALARM:
            return "遮挡报警侦测";
        default:
            return "未知";
        }
    }
    /// @brief 区域编号
    typedef enum class _PlateRegion_
    {
        BEIJING = 0,  /* 北京 (京) */
        TIANJIN,      /* 天津 (津) */
        HEBEI,        /* 河北 (冀) */
        SHANXI,       /* 山西 (晋) */
        NEIMENGGU,    /* 内蒙古 (内) */
        LIAONING,     /* 辽宁 (辽) */
        JILIN,        /* 吉林 (吉) */
        HEILONGJIANG, /* 黑龙江 (黑) */
        SHANGHAI,     /* 上海 (沪) */
        JIANGSU,      /* 江苏 (苏) */
        ZHEJIANG,     /* 浙江 (浙) */
        ANHUI,        /* 安徽 (皖) */
        FUJIAN,       /* 福建 (闽) */
        JIANGXI,      /* 江西 (赣) */
        SHANDONG,     /* 山东 (鲁) */
        HENAN,        /* 河南 (豫) */
        HUBEI,        /* 湖北 (鄂) */
        HUNAN,        /* 湖南 (湘) */
        GUANGDONG,    /* 广东 (粤) */
        GUANGXI,      /* 广西 (桂) */
        HAINAN,       /* 海南 (琼) */
        CHONGQING,    /* 重庆 (渝) */
        SICHUAN,      /* 四川 (川/蜀) */
        GUIZHOU,      /* 贵州 (贵) */
        YUNNAN,       /* 云南 (云) */
        XIZANG,       /* 西藏 (藏) */
        SHAANXI,      /* 陕西 (陕/秦) */
        GANSU,        /* 甘肃 (甘/陇) */
        QINGHAI,      /* 青海 (青) */
        NINGXIA,      /* 宁夏 (宁) */
        XINJIANG,     /* 新疆 (新) */
        HONGKONG,     /* 香港 (港) */
        MACAO,        /* 澳门 (澳) */
        TAIWAN        /* 台湾 (台) */
    } PlateRegion_E;

    typedef enum MainType
    {
        NONE = -1,  /* 未启用 */
        NORMAL = 0, /* 普通事件 */
        BORDER = 1, /* 周界事件 */
        TARGET = 2, /* 目标识别 */
        SCENE = 3,  /* 场景智能 */
    } MainType_E;
    typedef struct ChnEvent
    {
        /* 通道号 */
        int nChnId = -1;
        /* 开启的事件类型 */
        std::set<int> types;
        bool operator<(const ChnEvent &other) const
        {
            return nChnId < other.nChnId;
        }
    } ChnSmart_S;
    /* nvr智能事件信息 */
    typedef struct NvrSmartInfo
    {
        /* 启用nvr智能的主类型 */
        int nMainType = MainType::NONE; /* -1 未启用， 0 普通事件，1 周界事件，2 目标识别，3 场景智能 */
        /* 设置的最大支持数量 */
        int nMaxSupport = 4;
        /* 启用nvr智能的子类型 */
        std::set<ChnSmart_S> chnSmart;
        bool operator<(const NvrSmartInfo &other) const
        {
            /* map、set find函数结果都是同一个 */
            // return nMainType < other.nMainType;
            return false;
        }
    } NvrSmartInfo_S;

    /* 算法配置 */
    typedef struct AlgorithmConfig
    {
        /**
         * @brief   : 普通事件
         */  
        int nEnMotionDetect = 0;    /* 移动侦测 */
        int nEnOcclusionDetect = 0; /* 遮挡侦测 */
        int nEnAnomalyAlarm = 0;    /* 异常报警 */
        int nEnAudioAlarm = 0;      /* 声音报警 */
        int nEnAlarmInput = 0;      /* 报警输入 */
        int nEnAlarmOutput = 0;     /* 报警输出 */
        int nEnFlashAlarm = 0;      /* 闪光灯报警 */
        int nEnPIRAlarm = 0;        /* PIR 红外感应报警 */

        /**
         * @brief   : 周界事件
         */  
        int nEnLineCrossing = 0; /* 越界侦测 */
        int nEnIntrusion = 0;    /* 区域入侵 */
        int nEnEnterRegion = 0;  /* 进入区域 */
        int nEnLeaveRegion = 0;  /* 离开区域 */

        /**
         * @brief   : Smart事件
         */
        int nEnAudioAnomaly = 0;     /* 音频异常侦测 */
        int nEnSceneChange = 0;      /* 场景变更 */
        int nEnFaceDetect = 0;       /* 人脸侦测 */
        int nEnLoiteringDetect = 0;  /* 徘徊侦测 */
        int nEnCrowdGathering = 0;   /* 人员聚集 */
        int nEnParkingDetect = 0;    /* 停车侦测 */
        int nEnUnattendedObject = 0; /* 物品遗留 */
        int nEnObjectRemoval = 0;    /* 物品拿取 */
        int nEnPetRecognition = 0;   /* 宠物识别 */
        int nEnFaceCapture = 0;      /* 人脸抓拍 */
        int nEnFaceCompare = 0;      /* 人脸比对 */
        int nEnFaceLib = 0;          /* 人脸库操作 */
        int nEnPushFaceCapture = 0;  /* 人脸抓拍推送 */
#if CAP_AI_PEOPLE_STATISTICS
        int nEnPeopleFlowStatistics = 0; /* 人流统计 */
        int nEnPeopleDensityDetection = 0; /* 人员密度检测 */
#endif

#ifdef SCENE_INTELLIGENT_ANALYSIS
        /**
        * @brief   : 场景智能分析事件
        */
        int nEnLLmInference = 0;     /* 画面分析 */ 
        int nEnTextPreset = 0;       /* 文字预设任务 */   
        int nEnAISceneAnalysis = 0;  /* 场景智能分析总开关 */
#endif

#ifdef SCENE_INTELLIGENCE
        /**
         * @brief   : 行为监管
         */
        int nEnSleepOnDuty = 0;    /* 睡岗识别 */
        int nEnLeavePost = 0;      /* 离岗识别 */
        int nEnElectricVehicleInElevator = 0; /* 电瓶车进电梯识别 */
        int nEnPersonFallDown = 0; /* 人员倒地识别 */
        int nEnFenceClimbing = 0;  /* 翻越围栏识别 */
        int nEnTrip = 0;            /* 摔倒识别 */
        int nEnSmoking = 0;        /* 抽烟识别 */
        int nEnPhoneUsage = 0;     /* 玩手机识别 */
        int nEnSmokeFire = 0;      /* 烟火识别 */
        int nEnOpenFlame = 0;      /* 明火识别 */
        int nEnManholeCoverAbnormal = 0; /* 井盖异常检测 */
        int nEnBareSoil = 0;       /* 黄土裸露识别 */
        int nEnHoleProtectionBar = 0; /* 洞口防护栏识别 */
        int nEnPedestrianIntrusion = 0; /* 行人闯入识别 */

        /**
         * @brief   : 穿戴规范
         */
        int nEnSafetyHelmet = 0;       /* 安全帽识别 */
        int nEnReflectiveClothing = 0; /* 反光衣识别 */
        int nEnHighAltitudeSeatbelt = 0; /* 高空安全带识别 */

        /**
         * @brief   : 交通行为监管
         */
        int nEnConstructionOccupyRoad = 0; /* 施工占道识别 */
        int nEnEmergencyLaneOccupancy = 0; /* 应急车道占用识别 */
        int nEnReverseDirection = 0;       /* 逆行识别 */
        int nEnNonMotorVehicleIntrusion = 0; /* 非机动车闯入识别 */
        int nEnRoadPonding = 0;            /* 道路积水识别 */
        int nEnCongestion = 0;             /* 拥堵识别 */
        int nEnIllegalParking = 0;         /* 停车识别 */
        int nEnIllegalLaneChange = 0;      /* 违规变道识别 */  



         /**
         * @brief   : 属性识别
         */
        int nPlateNumber = 0;           /* 车牌识别 */
#endif

#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT
        int nEnGarbageExposure = 0; /* 垃圾暴露识别 */
        int nEnGarbageOverflow = 0; /* 垃圾满溢识别 */
#endif
        /* 操作符重载：用于判断两个配置是否相等 */
        bool operator!=(const AlgorithmConfig &other) const
        {
            return memcmp(this, &other, sizeof(AlgorithmConfig)) != 0;
        }

        /* 判断是否启用了任一事件 */
        bool any() const
        {
            const int *p = reinterpret_cast<const int *>(this);
            for (size_t i = 0; i < sizeof(AlgorithmConfig) / sizeof(int); ++i)
            {
                if (p[i] != 0)
                    return true;
            }
            return false;
        }
    } AlgorithmConfig_S;

    /* 智能资源分配-智能事件启用情况 */
    typedef struct _SmartEventEnableStatus_S_
    {
        /**
         * @brief   : 周界事件
         */
        bool bLineCrossing = false; /* 越界侦测 */
        bool bIntrusion = false;    /* 区域入侵 */
        bool bEnterRegion = false;  /* 进入区域 */
        bool bLeaveRegion = false;  /* 离开区域 */

        /**
         * @brief   : 行为分析
         */
        bool bLoiteringDetect = false; /* 徘徊侦测 */
        bool bCrowdGathering = false;  /* 人员聚集 */
        bool bParkingDetect = false;   /* 停车侦测 */

        /**
         * @brief   : 场景检测
         */
        bool bAudioAnomaly = false;     /* 音频异常侦测 */
        bool bSceneChange = false;      /* 场景变更 */
        bool bUnattendedObject = false; /* 物品遗留 */
        bool bObjectRemoval = false;    /* 物品拿取 */

        /**
         * @brief   : 目标检测
         */
        bool bFaceDetect = false;     /* 人脸侦测 */
        bool bPetRecognition = false; /* 宠物识别 */
        bool bFaceLib = false;        /* 人脸名单库 */
        /**
         * @brief   : 人脸抓拍
         */
        bool bFaceCapture = false; /* 人脸抓拍 */
        /**
         * @brief   : 人脸比对
         */
        bool bFaceCompare = false; /* 人脸比对 */

#ifdef SCENE_INTELLIGENCE
        /**
         * @brief   : 行为监管
         */ 
        bool bSleepOnDuty = false;                          /* 睡岗识别 */
        bool bLeavePost = false;                            /* 离岗识别 */
        bool bElectricVehicleInElevator = false;            /* 电瓶车进电梯识别 */
        bool bPersonFallDown = false;                       /* 人员倒地识别 */
        bool bFenceClimbing = false;                        /* 翻越围栏识别 */
        bool bTrip = false;                                 /* 摔倒识别 */
        bool bSmoking = false;                              /* 抽烟识别 */
        bool bPhoneUsage = false;                           /* 玩手机识别 */
        bool bSmokeFire = false;                            /* 烟火识别 */
        bool bOpenFlame = false;                            /* 明火识别 */
        bool bManholeCoverAbnormal = false;                 /* 井盖异常检测 */
        bool bBareSoil = false;                             /* 黄土裸露识别 */
        bool bHoleProtectionBar = false;                    /* 洞口防护栏识别 */
        bool bPedestrianIntrusion = false;                  /* 行人闯入识别 */

         /**
         * @brief   : 穿戴规范
         */
        bool bSafetyHelmet = false;                         /* 安全帽识别 */
        bool bReflectiveClothing = false;                   /* 反光衣识别 */
        bool bHighAltitudeSeatbelt = false;                 /* 高空安全带识别 */

        /**
         * @brief   : 交通行为监管
         */
        bool bConstructionOccupyRoad = false;               /* 施工占道识别 */
        bool bEmergencyLaneOccupancy = false;               /* 应急车道占用识别 */
        bool bReverseDirection = false;                     /* 逆行识别 */
        bool bNonMotorVehicleIntrusion = false;             /* 非机动车闯入识别 */
        bool bRoadPonding = false;                          /* 道路积水识别 */
        bool bCongestion = false;                           /* 拥堵识别 */
        bool bIllegalParking = false;                       /* 停车识别 */
        bool bIllegalLaneChange = false;                    /* 违规变道识别 */

        /**
         * @brief   : 属性识别
         */
        bool bPlateNumber = false;                          /* 车牌识别 */
#endif

#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT
        bool bGarbageExposure = false; /* 垃圾暴露识别 */
        bool bGarbageOverflow = false; /* 垃圾满溢识别 */
#endif

#if CAP_AI_PEOPLE_STATISTICS
        /**
         * @brief   : 人数统计
         */
        bool bPeopleFlowStatistics = false; /* 人流统计 */
        bool bPeopleDensityDetection = false; /* 人员密度检测 */
#endif

#ifdef SCENE_INTELLIGENT_ANALYSIS
        /**
        * @brief   : 场景智能分析事件
        */
        bool bSceneAnalysis = false;                          /* 场景智能分析 */
#endif
    } SmartEventEnableStatus_S;

    /* 智能资源分配 */
    typedef struct _SmartResourceAlloc_S_
    {
        /* 智能事件启用情况 */
        SmartEventEnableStatus_S stSmartEventEnableStatus;
        /* 还可以启用的事件列表 */
        std::vector<::Event::Type_E> aCanEventTypeArray;
        /* 智能事件启用情况列表 */
        std::vector<::Event::Type_E> aSmartEventEnableStatusArray;
    } SmartResourceAlloc_S;

    /* Metadata配置-Smart事件 */
    typedef struct _MetadataSmart_S_
    {
        /* 是否启用目标ID */
        bool bTargetId = false;
        /* 是否启用目标坐标 */
        bool bTargetCoord = false;
        /* 是否启用时间 */
        bool bTime = false;
    } MetadataSmart_S;

    /* Metadata配置-人脸抓拍 */
    typedef struct _MetadataFaceCapture_S_
    {
        /* 是否启用规则区域框 */
        bool bRegularRegionBox = false;
        /* 是否启用目标ID */
        bool bTargetId = false;
        /* 是否启用目标坐标 */
        bool bTargetCoord = false;
        /* 是否启用人脸评分 */
        bool bFaceScore = false;
        /* 是否启用时间 */
        bool bTime = false;
    } MetadataFaceCapture_S;

    /* Metadata配置 */
    typedef struct _MetadataConfig_S_
    {
        /* 是否启用 */
        bool bEnable;
        /* 智能事件配置 */
        MetadataSmart_S stMetadataSmart;
        /* 人脸抓拍配置 */
        MetadataFaceCapture_S stMetadataFaceCapture;

        /* 默认构造函数 */
        _MetadataConfig_S_() :
            bEnable(false),
            stMetadataSmart(),
            stMetadataFaceCapture()
        {
        }

        /* 重载赋值运算符 */
        _MetadataConfig_S_ &operator=(const _MetadataConfig_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                stMetadataSmart = x.stMetadataSmart;
                stMetadataFaceCapture = x.stMetadataFaceCapture;
            }
            return *this;
        }
    } MetadataConfig_S;

    typedef struct BindVideo
    {
        int nChnId = -1;          /* 通道号 */
        std::string strVideoPath; /* 录像片段路径 */
    } BindVideo_S;
    /// @brief 常规事件信息 周界事件、电瓶车事件
    typedef struct _Info_
    {
        int nId = -1;                        /* 唯一ID */
        int nChnId = -1;                     /* 通道号 */
        Type_E enType = Type::MOTION_DETECT; /* 事件类型 */
        std::string strDate;                 /* 日期 */
        std::string strTime;                 /* 时间 */
        std::string strStartTime;            /* 录像开始时间 */
        std::string strEndTime;              /* 录像结束时间 */
        long long lTimestamp = 0;            /* 时间戳 */
        std::string strLabel;                /* 视频标签 */
        std::string strVideoPath;            /* 录像片段路径 */
        int nVideoSize = 0;                  /* 录像片段大小 */
        int nVideoBindId = -1;               /* 绑定事件ID, 关联录像的主事件id */
        std::vector<BindVideo_S> bindVideos; /* 绑定的信息 */
        // 清空结构体的方法
        void clear() 
        {
            nId = -1;
            nChnId = 0;
            enType = Type::MOTION_DETECT;
            strDate.clear();
            strTime.clear();
            strStartTime.clear();
            strEndTime.clear();
            lTimestamp = 0;
            strLabel.clear();
            strVideoPath.clear();
            nVideoSize = 0;
            nVideoBindId = -1;
            bindVideos.clear(); // 清空向量
        }
    } Info_S;

    typedef struct EventState
    {
        /* 事件状态 */
        int nEventStatus = -1; /* -1 无事件 1 事件触发中 0 事件结束 */
        /* 事件信息 */
        Event::Info_S stEventInfo;
    } EventState_S;


    /// @brief 目标抓拍事件信息
    typedef struct _CaptureInfo_
    {
        Info_S stInfo;                /* 常规事件信息信息 */
        std::vector<float> vfFeatrue; /* 特征信息数组 */
    } CaptureInfo_S;

    /// @brief 目标对比事件信息
    typedef struct _CompareInfo_
    {
        CaptureInfo_S stInfo; /* 目标抓拍事件信息 */
        int nLibId;           /* 目标库ID */
        int nMatchId;         /* 匹配上的ID，目标库上的 */
    } CompareInfo_S;

    /// @brief 车辆事件信息
    typedef struct _VehicleInfo_
    {
        Info_S stInfo;               /* 常规事件信息信息 */
        PlateRegion_E enPlateRegion; /* 区域编号 */
        std::string strPlateSerial;  /* 序列号 */
    } VehicleInfo_S;

    /// @brief 检索条件结构体
    typedef struct _RetrievalCond_
    {
        int nId = -1;                                         /* 唯一ID */
        std::vector<int> nChnIds;                             /* 通道号，支持多个通道 */
        std::string strFilename;                              /* 文件名 */
        Type_E enType = Type::UNKNOWN;                        /* 事件类型 */
        std::vector<int> groupType;                           /* 事件组类型:1 人体检测事件，2 车辆检测事件 ，3 Ai事件 */
        std::string strStartDate;                             /* 开始日期 */
        std::string strEndDate;                               /* 结束日期 */
        std::string strTime;                                  /* 时间 */
        std::string strStartTime;                             /* 录像开始时间 */
        std::string strEndTime;                               /* 录像结束时间 */
        int nIsLock = -1;                                     /* 是否已锁定 0:未锁定, 1:锁定 */
        std::string strLabel;                                 /* 视频标签 */
        int nLibId = 0;                                       /* 目标库ID */
        int nMatchId = 0;                                     /* 匹配上的ID，目标库上的 */
        PlateRegion_E enPlateRegion = PlateRegion_E::BEIJING; /* 区域编号 */
        std::string strPlateSerial;                           /* 序列号 */
        int nPicType = 0;                                     /* 检索图片类型 -1:全部, 1002:定时抓图 其他:对应事件抓图 */
        int nVideoType = 0;                                   /* 检索ts录制文件类型 0:全部, 1:普通定时录像, 2:事件录像 */
        int nCompResult = -1;                                 /* 比对结果: 0 未进行比对，1 成功， 2 失败 */
        std::string strImagePath;                             /* 图片路径 */
        std::string strCapFacePath;                           /* 抓拍的人脸图片路径 */
        std::string strText;                                  /* 文字检索 */
        int nVideoBingId = -1;                                /* 视频绑定事件id */
        std::vector<int> videoBingIds;                        /* 视频绑定事件id */
        bool bEnQuickEntry = false;                           /* 快捷入口 */
    } RetrievalCond_S;

    typedef struct _QuickEntry
    {
        EntryType enType = EntryType::INVALID; /* 类型 */
        std::string name;                      /* 名称 */
        int nActionCode = -1;                  /* 动作码 */
        std::string message;                   /* 信息 */
        long long nTimestamp = 0;              /* 时间戳 */
        bool operator<(const _QuickEntry &other) const
        {
            return nTimestamp < other.nTimestamp;
        }
    } QuickEntry_S;

    /* 点、线、面 */
    typedef struct Point
    {
        int nX = 0;
        int nY = 0;
    } Point_S;

    using Line = std::vector<Point_S>;
    using Area = std::vector<Point_S>;
    /* 事件规则绘画，线/面 */
    typedef struct RuleInfo
    {
        Type_E enType = Type::MOTION_DETECT;
        int nChnId = -1;
        bool bEnable = false;
         /* 灵敏度[1,100] */
        unsigned int nSensitivity;
        unsigned int nTimeThreshold; /* 行为事件触发时间阈值，判断有效报警的时间[0,100]单位（秒） */
        std::vector<Line> lines;
        std::vector<Area> areas;
    } RuleInfo_S;

    /**
     * @brief 目标库信息
     */
    typedef struct FaceLibInfo
    {
        std::string strFaceLibName; /* 目标库名称 */
        int nTotalFace = 0;         /* 总人脸数 */
        int nNormalNum = 0;         /* 正常个数 */
        int nAbnormalNum = 0;       /* 异常个数 */
    } FaceLibInfo_S;

    /**
     * @brief 关联名单库信息
     */
    typedef struct AssociatedFaceLibInfo
    {
        FaceLibInfo_S faceLibInfo; /* 目标库信息 */
        int nSimilarity = 75;      /* 相似度 */
        bool bEnable = false;      /* 是否关联 */
    } AssociateLibInfo_S;

    /**
     * @brief 人脸查找条件
     */
    typedef struct FaceFind
    {
        std::string strFaceLibName; /* 目标库名称 */
        std::string strName;        /* 名字 */
        std::string strPhoneNum;    /* 联系方式 */
        int nModelState = -1;       /* 模型状态, 0未处理，1成功，2失败 */
        int nRatingLevel = -1;      /* 评估等级 */
    } FaceFind_S;

    /**
     * @brief 人脸Id信息
     */
    typedef struct FaceIdInfo
    {
        std::vector<int> ids; /* ID */
    } FaceIdInfo_S;

    /**
     * @brief 人脸信息
     */
    typedef struct FaceInfo
    {
        int nId = -1;               /* ID */
        std::string strFaceLibName; /* 名单组名称 */
        std::string strName;        /* 名字 */
        std::string strPhoneNum;    /* 联系方式 */
        std::string strPicPath;     /* 图片完整路径/名称 */
        std::string BinPath;        /* 图片二进制完整路径/名称 */
        std::string strPicType;     /* 图片类型 */
        int nPicSize = 0;           /* 图片大小 */
        std::string strPicDate;     /* 图片日期 */
        int nModelState = 0;        /* 模型状态, 0未处理，1成功，-1失败 */
        int nRatingLevel = 0;       /* 评估等级, 0全部，1 评分未知，2 低， 3高 */
    } FaceInfo_S;

    /* 实时人脸比对结果 */
    typedef struct FaceCompareInfo
    {
        /* 数据库信息 */
        int nEventId = -1;          /* 事件ID */
        int nCompResult = -1;       /* 比对结果, -1 全部， 0 不匹配， 1 匹配 */
        int nSimilarity = 0;        /* 相似度 0-100 */
        int nFaceId = -1;           /* 人脸ID */
        std::string strFaceLibName; /* 目标库名称 */
        std::string strFaceName;    /* 人脸名称 */
        std::string strLibFacePath; /* 目标库人脸路径 */
        std::string strCapFacePath; /* 抓拍人脸路径 */

        /* 其他关联信息 */
        std::string strCapImagePath; /* 抓拍图片路径 */
        Info_S stInfo;               /* 常规事件信息信息 */
    } FaceCompareInfo_S;
    /**
     * @brief 报警事件信息
     */
    typedef struct AlarmInfo
    {
        std::string ip;                          /* 报警ip */
        std::string chnId;                       /* 报警通道Id  */
        std::string time;                        /* 报警时间 */
        int nType = -1;                          /* 报警类型 */
        int nLevel = -1;                         /* 报警类别 */
        int nChnId = -1;                         /* Ipc通道号 */
        Category_E enCategory = Category::OTHER; /* 报警类别 */
        std::string videoPath;                   /* 报警视频路径 */
    } AlarmInfo_S;

    /**
     * @brief 事件描述信息
     */
    typedef struct DictInfo
    {
        bool bEnable = false;                /* 是否启用 */
        int nChnId = -1;                     /* 通道号 */
        Type_E enType = Type::MOTION_DETECT; /* 事件类型 */
        std::string dict;                    /* 事件描述 */
    } DictInfo_S;
    
    /**
     * @brief 实时目标检测框
     */
    typedef struct _TargetInfo_S_
    {
        int nChnId;  /* 检测通道 */
        int nType;   /* 目标类型 */
        std::vector<Common::Rect_S> vecRect; /* 目标坐标 */
    } TargetInfo_S;

    /* 通用事件 */
    constexpr const char *INFO_CHANNEL_ID = "channel_id";
    constexpr const char *INFO_EVENT_TYPE = "type";
    constexpr const char *INFO_EVENT_DATE = "date";
    constexpr const char *INFO_EVENT_TIME = "time";
    constexpr const char *INFO_RECORD_STATRTIME = "start_time";
    constexpr const char *INFO_RECORD_ENDTIME = "end_time";
    constexpr const char *INFO_TIMESTAMP = "timestamp";
    constexpr const char *INFO_RECORD_ISLOCK = "is_lock";
    constexpr const char *INFO_RECORD_LABEL = "label";
    constexpr const char *INFO_VIDEO_PATH = "video_path";
    constexpr const char *INFO_VIDEO_SIZE = "video_size";
    constexpr const char *INFO_VIDEO_BIND_ID = "video_bind_id";

} // namespace Event
