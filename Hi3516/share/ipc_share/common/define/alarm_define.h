/**
 * @FilePath     : alarm_define.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-07-17 17:25:12
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-22 19:32:00
 * @Description  : 报警配置参数数据结构
 */

#pragma once
#include "common_define.h"
#include <vector>
#include <variant>
#include <chrono>
#include <random>
#include <string>

#include "event_define.h"
#include "osd_define.h"

namespace Alarm
{
#ifndef GRID_WIDTH_DEFAULT
/* 网格尺寸宽度默认值 */
#define GRID_WIDTH_DEFAULT (22)
#endif

#ifndef GRID_HEIGHT_DEFAULT
/* 网格尺寸高度默认值 */
#define GRID_HEIGHT_DEFAULT (18)
#endif

#ifndef SENSITIVITY_DEFAULT
/* 灵敏度默认值 */
#define SENSITIVITY_DEFAULT (50)
#endif

#ifndef MOTION_AREA_MAX_WIDTH
/* 普通模式的宽度最大宏块个数 */
#define MOTION_AREA_MAX_WIDTH 22
#endif

#ifndef MOTION_AREA_MAX_HEIGHT
/* 普通模式的高度最大宏块个数 */
#define MOTION_AREA_MAX_HEIGHT 18
#endif

#ifndef WEEK_DAYS
/* 一周的天数 对应SDK--MAX_DAYS */
#define WEEK_DAYS (7)
#endif

#ifndef ALARM_TIMESLOT_MAX
/* 布防时间段次数上限 对应SDK——MAX_TIMESEGMENT_V30 */
#define ALARM_TIMESLOT_MAX 8
#endif

#ifndef MOTION_EXPERT_AREA_MAX
/* 专家模式的最大侦测区域个数 对应SDK——MAX_MULTI_AREA_NUM */
#define MOTION_EXPERT_AREA_MAX 8
#endif

#ifndef POLYGON_POINT_MAX
/* 多边形最大顶点个数 对应SDK——VCA_MAX_POLYGON_POINT_NUM */
#define POLYGON_POINT_MAX 10
#endif

#ifndef INTRUSION_AREA_MAX
/* 普通模式的最大侦测区域个数 对应SDK——MAX_INTRUSIONREGION_NUM */
#define INTRUSION_AREA_MAX 4
#endif

#ifndef REGION_MAX
/* 区域上限 对应SDK--MAX_REGION_NUM */
#define REGION_MAX 8
#endif

#ifndef LOITERING_DETECT_REGION_DEFAULT
/* 徘徊侦测支持区域上限 */
#define LOITERING_DETECT_REGION_DEFAULT (4)
#endif

#ifndef CROWD_GATHERING_DETECT_REGION_DEFAULT
/* 人员聚集侦测支持区域上限 */
#define CROWD_GATHERING_DETECT_REGION_DEFAULT (4)
#endif

#ifndef PARKING_DETECT_REGION_DEFAULT
/* 停车侦测支持区域上限 */
#define PARKING_DETECT_REGION_DEFAULT (4)
#endif

#ifndef UNATTENDED_OBJECT_DETECT_REGION_DEFAULT
/* 物品遗留侦测支持区域上限 */
#define UNATTENDED_OBJECT_DETECT_REGION_DEFAULT (4)
#endif

#ifndef OBJECT_REMOVAL_DETECT_REGION_DEFAULT
/* 物品拿取侦测支持区域上限 */
#define OBJECT_REMOVAL_DETECT_REGION_DEFAULT (4)
#endif

#ifndef BOUND_DETECT_REGION_DEFAULT
/* 越界侦测支持区域上限  */
#define BOUND_DETECT_REGION_DEFAULT 4
#endif

#ifndef FIELD_DETECT_REGION_DEFAULT
/* 区域入侵侦测支持区域上限 */
#define FIELD_DETECT_REGION_DEFAULT (4)
#endif

#ifndef ENTER_DETECT_REGION_DEFAULT
/* 进入区域侦测支持区域上限 */
#define ENTER_DETECT_REGION_DEFAULT (4)
#endif

#ifndef EXIT_DETECT_REGION_DEFAULT
/* 离开区域侦测支持区域上限 */
#define EXIT_DETECT_REGION_DEFAULT (4)

#ifndef DRIVING_AGAINST_TRAFFIC_DETECT_REGION_DEFAULT
/* 逆行侦测支持区域上限  */
#define DRIVING_AGAINST_TRAFFIC_DETECT_REGION_DEFAULT 4
#endif

#ifndef ILLEGAL_LANE_CHANGE_DETECT_REGION_DEFAULT
/* 违规变道侦测支持区域上限  */
#define ILLEGAL_LANE_CHANGE_DETECT_REGION_DEFAULT 4
#endif

#ifndef EMERGENCY_LANE_OCCUPANCY_DETECT_REGION_DEFAULT
/* 应急车道占用侦测支持区域上限 */
#define EMERGENCY_LANE_OCCUPANCY_DETECT_REGION_DEFAULT (4)
#endif

#ifndef NON_MOTOR_VEHICLE_INTRUSION_DETECT_REGION_DEFAULT
/* 非机动车侦测支持区域上限 */
#define NON_MOTOR_VEHICLE_INTRUSION_DETECT_REGION_DEFAULT (4)
#endif

#endif

    /* 布防时间 */
    using DefenseTime = std::vector<std::vector<Common::SchedTime_S>>;

    /* 报警处理方式枚举 */
    typedef enum AlarmHandleType
    {
        ALARM_HANDLE_NONE = 0,                 /* 无响应 */
        ALARM_HANDLE_JPEG_2_EMAI = 1,          /* JPEG图片发送到邮箱 */
        ALARM_HANDLE_JPEG_2_SDCard = 2,        /* JPEG图片发送到SD卡 */
        ALARM_HANDLE_UPLOAD_CENTER = 3,        /* 上传中心 */
        ALARM_HANDLE_SPEAKER = 4,              /* 声音报警 */
        ALARM_HANDLE_WHITE_LIGHT_LINKAGE = 5,  /* 闪光报警灯联动 */
        ALARM_HANDLE_TRIGGER_ALARM_OUTPUT = 6, /* 触发报警输出 */
    } AlarmHandleType_E;

    /* 常规联动 */
    typedef enum LinkageType
    {
        NONE = 0,                   /* 无联动 */
        SEND_EMAIL = 1,             /* 发送邮件 */
        UPLOAD_TOCENTER = 2,        /* 上传中心 */
        UPLOAD_SD_CARD = 3,         /* 上传SD卡 */
        SOUND = 4,                  /* 声音联动 */
        FLASHING_LIGHT_ALARM = 5,   /* 闪光报警灯 */
        UPLOAD_PANORAMIC_IMAGE = 6, /* 上传全景大图 */
        UPLOAD_TARGET_IMAGE = 7,    /* 上传目标小图 */
    } LinkageType_E;

    /* 联动方式 */
    typedef struct LinkageList
    {
        std::vector<int> tradition;   /* 常规联动 LinkageType_E*/
        std::vector<int> alarmOutput; /* 报警输出联动，格式未定 */
        std::vector<int> recordChn;   /* 录制联动 */
        // bool recordChn = false;       /* 录制联动 */
    } LinkageList_S;

    /* 区域数据结构 */
    typedef struct _Region_S_
    {
        /* 有效的坐标点（大于等于3） */
        unsigned int nPointNum;
        /* 多边形坐标点，最多为10个坐标点-POLYGON_POINT_MAX */
        std::vector<Common::PosF_S> aPoint;
        /* 默认构造函数 */
        _Region_S_() : nPointNum(4)
        {
            aPoint.clear();
        }
        /* 重载赋值运算符 */
        _Region_S_ &operator=(const _Region_S_ &x)
        {
            if (this != &x)
            {
                nPointNum = x.nPointNum;
                aPoint = x.aPoint;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _Region_S_ CreateWithDefaultRule(unsigned int nPointNum)
        {
            _Region_S_ obj;
            obj.nPointNum = nPointNum;
            for (size_t i = 0; i < nPointNum; ++i)
            {
                obj.aPoint.emplace_back();
            }
            return obj;
        }

        /**
         * @brief 分辨率坐标转换函数
         * @param srcWidth 原始分辨率宽度
         * @param srcHeight 原始分辨率高度
         * @param dstWidth 目标分辨率宽度
         * @param dstHeight 目标分辨率高度
         * @return 转换是否成功
         */
        bool ConvertResolution(float srcWidth, float srcHeight, float dstWidth, float dstHeight)
        {
            /* 参数有效性检查 */
            if (srcWidth <= 0.0f || srcHeight <= 0.0f || dstWidth <= 0.0f || dstHeight <= 0.0f)
            {
                return false;
            }

            /* 计算缩放比例 */
            float scaleX = dstWidth / srcWidth;
            float scaleY = dstHeight / srcHeight;

            /* 转换所有坐标点 */
            for (auto &point : aPoint)
            {
                point.fX *= scaleX;
                point.fY *= scaleY;
            }

            return true;
        }

        /**
         * @brief 分辨率坐标转换函数（整数版本）
         * @param srcWidth 原始分辨率宽度
         * @param srcHeight 原始分辨率高度
         * @param dstWidth 目标分辨率宽度
         * @param dstHeight 目标分辨率高度
         * @return 转换是否成功
         */
        bool ConvertResolution(int srcWidth, int srcHeight, int dstWidth, int dstHeight)
        {
            /* 参数有效性检查 */
            if (srcWidth <= 0 || srcHeight <= 0 || dstWidth <= 0 || dstHeight <= 0)
            {
                return false;
            }

            /* 转换为浮点数进行计算，避免整数除法精度丢失 */
            return ConvertResolution(static_cast<float>(srcWidth), static_cast<float>(srcHeight), static_cast<float>(dstWidth), static_cast<float>(dstHeight));
        }

        /**
         * @brief   : 判断多边形是否有效
         * @return   {bool} true：有效 false：无效
         */
        bool IsValid() const
        {
            /* 检查有效坐标点数量是否至少为3 */
            if (nPointNum < 3 || aPoint.size() < nPointNum)
            {
                return false;
            }

            for (unsigned int i = 0; i < nPointNum; ++i)
            {
                if (!aPoint[i].IsValid())
                {
                    return false;
                }
            }

            return true;
        }

        /* 操作符重载：用于判断两个配置是否相等 */
        bool operator!=(const _Region_S_ &other) const
        {
            /* 首先比较坐标点数量 */
            if (nPointNum != other.nPointNum)
            {
                return true;
            }

            /* 比较向量大小 */
            if (aPoint.size() != other.aPoint.size())
            {
                return true;
            }

            /* 逐个比较有效坐标点 */
            for (unsigned int i = 0; i < nPointNum && i < aPoint.size(); ++i)
            {
                /* 直接精确比较 */
                if (aPoint[i].fX != other.aPoint[i].fX || aPoint[i].fY != other.aPoint[i].fY)
                {
                    return true;
                }
            }

            /* 所有比较都相等，返回false */
            return false;
        }
    } Region_S;

    /// @brief 进入区域检测规则参数
    typedef struct _EntranceRule_S_
    {
        Region_S stRegion;         /* 区域定义 */
        unsigned int nSensitivity; /* 灵敏度[1,100] */
        /* 检测目标:
         * 0:所有目标
         * 1:人
         * 2:车
         * 3:人和车
         */
        unsigned int nDetectionTarget;
        unsigned int nAlarmConfidence;  /* 报警可信度: 0-低,1-较低,2-较高,3-高 */
        unsigned int nRecordConfidence; /* 录像可信度: 0-低,1-较低,2-较高,3-高 */
        /* 默认构造函数 */
        _EntranceRule_S_()
            : stRegion(),
              nSensitivity(50),
              nDetectionTarget(0),
              nAlarmConfidence(0),
              nRecordConfidence(0)
        {
        }
        /* 重载赋值运算符 */
        _EntranceRule_S_ &operator=(const _EntranceRule_S_ &x)
        {
            if (this != &x)
            {
                stRegion = x.stRegion;
                nSensitivity = x.nSensitivity;
                nDetectionTarget = x.nDetectionTarget;
            }
            return *this;
        }
    } EntranceRule_S;

    /**
     * @brief 星期一~日枚举
     */
    typedef enum class EventDayOfWeek
    {
        Monday = 1,
        Tuesday,
        Wednesday,
        Thursday,
        Friday,
        Saturday,
        Sunday
    } EventDayOfWeek_E;

    /* 事件计划 */
    typedef struct EventSchedule
    {
        /* 事件类型 */
        Event::Type_E enEventType = Event::Type_E::UNKNOWN;
        /*事件启用状态*/
        bool bStatus = false;

        /* 布防时间 */
        DefenseTime defenseTime;

        bool operator<(const EventSchedule &other) const
        {
            return enEventType < other.enEventType;
        }
    } EventSchedule_S;

    /**
     * @brief 视频丢失
     */
    typedef struct VideoLostDetection
    {
        int nChnId = -1;      /* 通道号 */
        bool bEnable = false; /* 是否启用 0-不启用 1-启用 */
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        std::vector<std::vector<Common::SchedTime_S>> aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;
        bool operator<(const VideoLostDetection &other) const
        {
            return nChnId < other.nChnId;
        }
    } VideoLostDetection_S;


    /**
     * @brief   : 普通事件
     */

    /************************ 移动侦测相关 ************************/
    /* 移动侦测配置模式 */
    typedef enum _MotionType_E
    {
        MOTION_NORMAL = 0, /* 普通模式 */
        MOTION_EXPERT = 1, /* 专家模式 */
    } MotionType_E;

    /* 移动侦测专家模式区域参数 */
    typedef struct _MotionRegion_S_
    {
        /* 侦测区域编号，从1开始 */
        unsigned int nAreaNo;
        /* 侦测区域 */
        Common::Rect_S stRect;
        /* 日夜参数转换为关闭时的灵敏度 [1,100] */
        unsigned int nCloseSensitivity;
        /* 白天灵敏度 [1,100] */
        unsigned int nDaytimeSensitivity;
        /* 夜晚灵敏度 [1,100] */
        unsigned int nNightSensitivity;

        /* 重载默认构造函数 */
        _MotionRegion_S_() : nAreaNo(0), stRect(), nCloseSensitivity(60), nDaytimeSensitivity(60), nNightSensitivity(60)
        {
        }

        /* 重载赋值运算符 */
        _MotionRegion_S_ &operator=(const _MotionRegion_S_ &x)
        {
            if (this != &x)
            {
                nAreaNo = x.nAreaNo;
                stRect = x.stRect;
                nCloseSensitivity = x.nCloseSensitivity;
                nDaytimeSensitivity = x.nDaytimeSensitivity;
                nNightSensitivity = x.nNightSensitivity;
            }
            return *this;
        }
    } MotionRegion_S;

    /* 移动侦测的专家模式参数 */
    typedef struct _MotionExpertMode_S_
    {
        /* 日夜控制：0- 关闭(默认)，1- 自动切换，2- 定时切换 */
        unsigned int nExpertDayNightCtrl;
        /* 日夜切换时间 定时切换时有效-时间段内为白天，时间段外为夜晚 */
        Common::SchedTime_S stDayTime;
        /* 移动侦测专家模式所有区域参数 */
        std::vector<MotionRegion_S> vstMotionRegion;
        /* 默认构造函数 */
        _MotionExpertMode_S_() : nExpertDayNightCtrl(0), stDayTime()
        {
            vstMotionRegion.clear();
        }
        /* 重载赋值运算符 */
        _MotionExpertMode_S_ &operator=(const _MotionExpertMode_S_ &x)
        {
            if (this != &x)
            {
                nExpertDayNightCtrl = x.nExpertDayNightCtrl;
                stDayTime = x.stDayTime;
                vstMotionRegion = x.vstMotionRegion;
            }
            return *this;
        }
    } MotionExpertMode_S;

    /* 移动侦测的普通模式参数 */
    typedef struct _MotionNormalMode_S_
    {
        /* 区域网格 */
        using AreaGrid = std::vector<std::vector<unsigned int>>;

        /* 灵敏度 [0,100] */
        unsigned int nSensitivity;
        /* 区域类型 0：筒型 1：网格 */
        unsigned int nRegionType;
        /** 筒型移动侦测区域 / 网格移动侦测区域
         * 普通模式的移动侦测区域
         * 画面分割成22×18（或者22×15）个小宏块(宽高比例)
         * 最多仅有22个单位的宽度，18个单位的高度，即数组18x22
         * 标记为1的小宏块即为移动侦测区域
         */
        std::variant<Common::Rect_S, AreaGrid> varRegion;

        /* 默认构造函数 */
        _MotionNormalMode_S_() : nSensitivity(60)
        {
        //可根据型号设置不同的区域类型
#if CAP_MOTION_REGION_GRID // 移动侦测网格区域能力
        nRegionType = 1;
        // 初始化 18x22 网格区域，默认全为 0
        AreaGrid grid(18, std::vector<unsigned int>(22, 0));
        varRegion = grid;
#else
        nRegionType = 0;
        varRegion = Common::Rect_S();
#endif
        }

        /* 重载赋值运算符 */
        _MotionNormalMode_S_ &operator=(const _MotionNormalMode_S_ &x)
        {
            if (this != &x)
            {
                nSensitivity = x.nSensitivity;
                nRegionType = x.nRegionType;
                varRegion = x.varRegion;
            }
            return *this;
        }
    } MotionNormalMode_S;

    /* 移动侦测数据结构 */
    typedef struct _MotionDetection_S_
    {
        /* 是否启用 */
        bool bEnable;
        /* 是否启用动态分析 */
        bool bDynamicAnalysisEnable;
        /* 0- 普通模式，1- 专家模式 */
        MotionType_E enMode;
        /* 普通模式 */
        MotionNormalMode_S stMotionNormalMode;
        /* 专家模式 */
        MotionExpertMode_S stMotionExpertMode;
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        std::vector<std::vector<Common::SchedTime_S>> aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        _MotionDetection_S_()
            : bEnable(false),
              bDynamicAnalysisEnable(false),
              enMode(MOTION_NORMAL),
              stMotionNormalMode(),
              stMotionExpertMode()
        {
            aAlarmTime.clear();
        }

        /* 重载赋值运算符 */
        _MotionDetection_S_ &operator=(const _MotionDetection_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                bDynamicAnalysisEnable = x.bDynamicAnalysisEnable;
                enMode = x.enMode;
                stMotionNormalMode = x.stMotionNormalMode;
                stMotionExpertMode = x.stMotionExpertMode;
                aAlarmTime = x.aAlarmTime;
                stLinkageList = x.stLinkageList;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _MotionDetection_S_ CreateWithDefaultRule()
        {
            _MotionDetection_S_ obj;
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }

    } MotionDetection_S;
    /************************ 移动侦测相关 END ************************/

    /************************ 遮挡报警相关 ************************/

    /// @brief 遮挡侦测数据结构
    typedef struct _HideAlarm_S_
    {
        bool bEnable;      /* 是否启用 0-不启用 1-启用 */
        unsigned int nSensitivity; /* 遮挡报警灵敏度[0,3]，值越大越灵敏 */
        Common::Rect_S stRect;     /* 遮挡区域,宽高为[1920,1080] */
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        std::vector<std::vector<Common::SchedTime_S>> aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 重载默认构造函数 */
        _HideAlarm_S_()
            : bEnable(0),
              nSensitivity(1),
              stRect()
        {
            aAlarmTime.clear();
        }
        /* 重载赋值运算符 */
        _HideAlarm_S_ &operator=(const _HideAlarm_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                nSensitivity = x.nSensitivity;
                stRect = x.stRect;
                aAlarmTime = x.aAlarmTime;
                stLinkageList = x.stLinkageList;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _HideAlarm_S_ CreateWithDefaultRule()
        {
            _HideAlarm_S_ obj;
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }

    } HideAlarm_S;

    /************************ 遮挡报警相关 END ************************/

    /************************ 异常报警相关 ************************/

    /**
     * @brief 异常类型
     */
    typedef enum class _AbnormalType_E_
    {
        DISK_FULL = 0,      /* 硬盘满 */
        DISK_ERROR ,        /* 硬盘错误 */
        NET_BROKEN ,        /* 网络断开 */
        IP_CONFLICT,        /* IP冲突 */
        ILLEGAL_ACCESS      /* 非法访问 */
    } AbnormalType_E;

    /**
     * @brief 异常配置
     */
    typedef struct _AbnormalDetection_S_
    {
        bool bEnable = false;    /* 是否启用事件提示 0-不启用 1-启用 */
        std::vector<int> promptType;    /* 提示类型 Event::Type_E */
        AbnormalType_E enAbnormalType = AbnormalType_E::DISK_FULL; /* 异常类型 */
        LinkageList_S stLinkageList; /* 联动 */
        bool operator<(const _AbnormalDetection_S_ &other) const
        {
            return enAbnormalType < other.enAbnormalType;
        }
    } AbnormalDetection_S;

    /************************ 异常报警相关 END ************************/

    /************************ 声音报警输出相关 ************************/

    /**
     * @brief 警戒音类型
     */
    typedef enum class _AlertSoundType_E_
    {
        WARNING_ZONE_LEAVE_IMMEDIATELY = 0,   /*警戒区域，尽快离开*/
        DANGER_ZONE_DO_NOT_APPROACH,          /*危险区域，请勿靠近*/
        NO_PARKING_ZONE,                      /*此区域禁止停车*/
        ENTERING_SURVEILLANCE_ZONE,           /*您已进入监控区域*/
        WELCOME_GREETING,                     /*您好，欢迎光临*/
        DO_NOT_TOUCH_VALUABLES,               /*贵重物品，请勿触碰*/
        PRIVATE_PROPERTY_NO_ENTRY,            /*私人领域，禁止入内*/
        DEEP_WATER_WARNING,                   /*水深危险，注意安全*/
        HIGH_PLACE_DANGER,                    /*高处危险，请勿攀爬*/
        SHRIEK_ALARM,                         /*啸叫警报音*/
        GENERAL_WARNING_TONE                  /*告警音*/
    }AlertSoundType_E;

    /**
     * @brief 音频类型
     */
    typedef enum class SoundType
    {
        WARN = 0,     /* 警戒音 */
        ALERT = 1,    /* 提示音 */
        CUSTOM        /* 自定义 */
    } SoundType_E;

    typedef struct _CustomAudio_S_
    {
        bool bChoose;               /* 是否选择该音频文件 */
        std::string strCustomeName; /* 自定义名称 */
        std::string strPath;        /* 下载路径 */
    }CustomAudio_S;

    typedef enum class _CustomOperationType_E_
    {
        CUSTOM_EDIT = 0,  /* 编辑 */
        CUSTOM_PLAY,      /* 播放 */
        CUSTOM_DEL        /* 删除 */
    }CustomOperationType_E;

    typedef struct _CustomOperation_S_
    {
        bool nEnable = false;               /* 是否选择该文件 */
        CustomOperationType_E enCustomType; /* 自定义操作类型 */
        std::string strFileName;            /* 文件名 */
        std::string strName;                /* 自定义名称 */
        std::string strPath;                /* 自定义路径 */
    }CustomOperation_S;

    /**
     * @brief 声音报警输出配置
     */
    typedef struct _SoundOutputAlarm_S_
    {
        SoundType_E enSoundType = SoundType_E::WARN; /* 音频类型 */
        AlertSoundType_E enAlertSound = AlertSoundType_E::WARNING_ZONE_LEAVE_IMMEDIATELY; /* 警戒音类型 */
        int nTimes;                /* 报警次数 [1-50]*/
        /* 最多 3 个自定义音频名称 */
        std::vector<CustomAudio_S> aCustomAudio;
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        std::vector<std::vector<Common::SchedTime_S>> aAlarmTime;

        _SoundOutputAlarm_S_():
        enSoundType(SoundType_E::WARN),
        enAlertSound(AlertSoundType_E::WARNING_ZONE_LEAVE_IMMEDIATELY),
        nTimes(1)
        {
            aCustomAudio.clear();
            aAlarmTime.clear();
        }

        _SoundOutputAlarm_S_ &operator=(const _SoundOutputAlarm_S_ &x)
        {
            if (this != &x)
            {
                enSoundType = x.enSoundType;
                enAlertSound = x.enAlertSound;
                nTimes = x.nTimes;
                aCustomAudio = x.aCustomAudio;
                aAlarmTime = x.aAlarmTime;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _SoundOutputAlarm_S_ CreateWithDefaultRule()
        {
            _SoundOutputAlarm_S_ obj;
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }

    } SoundOutputAlarm_S;

    /************************ 声音报警输出相关 END ************************/

    /************************ 报警输入相关 ************************/

    /**
     * @brief 报警输入信息
     */
    typedef struct _IoInputInfo_S_
    {
        int nIoNumer = -1;           /* 报警输入号 */
        std::string ioAddr = "本地"; /* 地址 */
        std::string ioName;          /* 报警输入名称 */
        bool bNormallyOpen = true;   /* 报警类型, 0 常闭， 1 常开 */
        int nDealType = 1;           /* 0 不启用， 1 报警输入 */
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        std::vector<std::vector<Common::SchedTime_S>> aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;
        std::vector<int> copyTo; /* 复制到指定报警输入号 */

        _IoInputInfo_S_():
        nIoNumer(-1),
        ioAddr("本地"),
        ioName(""),
        nDealType(1)
        {
            aAlarmTime.clear();
        }

        bool operator<(const _IoInputInfo_S_ &other) const
        {
            return nIoNumer < other.nIoNumer;
        }

        _IoInputInfo_S_ &operator=(const _IoInputInfo_S_ &x)
        {
            if (this != &x)
            {
                nIoNumer = x.nIoNumer;
                ioAddr = x.ioAddr;
                ioName = x.ioName;
                nDealType = x.nDealType;
                aAlarmTime = x.aAlarmTime;
                stLinkageList = x.stLinkageList;
                copyTo = x.copyTo;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _IoInputInfo_S_ CreateWithDefaultRule()
        {
            _IoInputInfo_S_ obj;
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }

    } IoInputInfo_S;

    /************************ 报警输入相关 END ************************/

    /************************ 报警输出相关 ************************/

    /**
     * @brief 报警输出状态
     */
     typedef enum class _IoOutputState_E_
     {
         OFF = 0,          /* 输出状态：0 关闭， 1 打开 */
         ON = 1,           /* 输出状态：0 关闭， 1 打开 */
         HUMAN_OFF = 2,    /* 人工关闭, 仅设置用，获取还是close/open */
         HUMAN_ON = 3,     /* 人工打开, 仅设置用，获取还是close/open */
     } IoOutputState_E;

    /**
     * @brief 报警输出信息
     */
    typedef struct _IoOutputInfo_S_
    {
        int nIoNumer = -1;           /* 报警输入号 */
        std::string ioAddr = "本地"; /* 地址 */
        std::string ioName;          /* 报警输出名称 */
        int nDelayTime = 5;          /* 延时时间s */
        IoOutputState_E enState = IoOutputState_E::OFF;        /* 输出状态 */
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        std::vector<std::vector<Common::SchedTime_S>> aAlarmTime;
        std::vector<int> copyTo; /* 复制到指定报警输出号 */

        _IoOutputInfo_S_():
        nIoNumer(-1),
        ioAddr("本地"),
        ioName(""),
        nDelayTime(5),
        enState(IoOutputState_E::OFF)
        {
            aAlarmTime.clear();
        }
        bool operator<(const _IoOutputInfo_S_ &other) const
        {
            return nIoNumer < other.nIoNumer;
        }

        _IoOutputInfo_S_ &operator=(const _IoOutputInfo_S_ &x)
        {
            if (this != &x)
            {
                nIoNumer = x.nIoNumer;
                ioAddr = x.ioAddr;
                ioName = x.ioName;
                nDelayTime = x.nDelayTime;
                enState = x.enState;
                aAlarmTime = x.aAlarmTime;
                copyTo = x.copyTo;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _IoOutputInfo_S_ CreateWithDefaultRule()
        {
            _IoOutputInfo_S_ obj;
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }

    } IoOutputInfo_S;

    /************************ 报警输出相关 END ************************/

    /************************ 闪光报警相关 ************************/

    /**
     * @brief 闪烁频率
     */
    typedef enum class _FlashFrequency_E_
    {
        FLASH_STEADY_ON   = 0,  /*常亮*/
        FLASH_LOW_FREQ    = 1,  /*低频闪烁*/
        FLASH_MID_FREQ    = 2,  /*中频闪烁*/
        FLASH_HIGH_FREQ   = 3   /*高频闪烁*/
    }FlashFrequency_E;

    /**
     * @brief 闪光报警信息
     */
    typedef struct _FlashInfo_S_
    {
        int nFlashTime;                /* 闪烁时间 [1-300]，单位秒 */
        FlashFrequency_E enFalshFrequency;  /* 闪烁频率 */

        /* 布防时间:一周7天，每天可以设置8个时间段 */
        std::vector<std::vector<Common::SchedTime_S>> aAlarmTime;
        std::vector<int> copyTo;
        /* 默认构造函数 */
        _FlashInfo_S_()
        : nFlashTime(1),
            enFalshFrequency(FlashFrequency_E::FLASH_LOW_FREQ)
        {
            aAlarmTime.clear();
            copyTo.clear();
        }

        /* 重载赋值运算符 */
        _FlashInfo_S_ &operator=(const _FlashInfo_S_ &x)
        {
            if (this != &x)
            {
                nFlashTime = x.nFlashTime;
                enFalshFrequency = x.enFalshFrequency;
                aAlarmTime = x.aAlarmTime;
                copyTo = x.copyTo;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _FlashInfo_S_ CreateWithDefaultRule()
        {
            _FlashInfo_S_ obj;
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }

    } FlashInfo_S;

    /************************ 闪光报警相关 END ************************/

    /************************ PIR报警相关 ************************/

    /**
     * @brief PIR报警信息
     */
    typedef struct _PirAlarmInfo_S_
    {
        bool bEnable;      /* 是否启用 0-不启用 1-启用 */
        std::string AlarmName;      /* 报警名称 */

        /* 布防时间:一周7天，每天可以设置8个时间段 */
        std::vector<std::vector<Common::SchedTime_S>> aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;
        std::vector<int> copyTo;

        _PirAlarmInfo_S_()
        {
            aAlarmTime.clear();
        }

        bool operator<(const _PirAlarmInfo_S_ &other) const
        {
            return AlarmName < other.AlarmName;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _PirAlarmInfo_S_ CreateWithDefaultRule()
        {
            _PirAlarmInfo_S_ obj;
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }

    } PirAlarmInfo_S;

    /************************ PIR报警相关 END ************************/

    /**
     * @brief   : 周界事件
     */

     /************************ 越界报警相关 ************************/

     /* 目标移动方向 */
    typedef enum _CrossDirection_E_
    {
        CROSS_DIRECTION_INVALID = -1, /* 异常的默认值 */
        BOTH_WAYS = 0,                /* 双向 */
        A_TO_B,                       /* A 到 B 的方向 */
        B_TO_A,                       /* B 到 A 的方向 */
    } CrossDirection_E;

    typedef struct _BoundaryPlane_S_
    {
        Common::PosF_S stStartPos;     /* 警戒线的起始点 */
        Common::PosF_S stEndPos;       /* 警戒线的终止点 */
        CrossDirection_E enCrossDirection; /* 警戒线的穿越方向[0-双向,1-由左至右,2-由右至左] */
        unsigned int nSensitivity;     /* 警戒线灵敏度[1,100] */
        std::vector<int> aDetectionTarget;  /* 检测目标,DetectionTarget_E */

        /* 重载默认构造函数 */
        _BoundaryPlane_S_()
            : stStartPos(),
            stEndPos(),
            enCrossDirection(A_TO_B),
            nSensitivity(50)
        {
            aDetectionTarget.clear();
        }
        /* 重载赋值运算符 */
        _BoundaryPlane_S_ &operator=(const _BoundaryPlane_S_ &x)
        {
            if (this != &x)
            {
                stStartPos = x.stStartPos;
                stEndPos = x.stEndPos;
                enCrossDirection = x.enCrossDirection;
                nSensitivity = x.nSensitivity;
                aDetectionTarget = x.aDetectionTarget;
            }
            return *this;
        }
    } BoundaryPlane_S;

    /// @brief 越界检测报警
    typedef struct _BoundaryDetection_S_
    {
        bool bEnable;                     /* 是否启用 0-不启用 1-启用 */
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        std::vector<std::vector<Common::SchedTime_S>> aAlarmTime;
        /* 越界检测区域,最多设置4个-TRAVERSE_DETECT_REGION_DEFAULT */
        std::vector<BoundaryPlane_S> aRule;
        /* NOTE IPC自身网页没有假日布防时间设置，暂不对接 */
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        _BoundaryDetection_S_()
            : bEnable(false)
        {
            aAlarmTime.clear();
            aRule.clear();
        }
        /* 重载赋值运算符 */
        _BoundaryDetection_S_ &operator=(const _BoundaryDetection_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aAlarmTime = x.aAlarmTime;
                aRule = x.aRule;
                stLinkageList = x.stLinkageList;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _BoundaryDetection_S_ CreateWithDefaultRule()
        {
            _BoundaryDetection_S_ obj;
            for(int i = 0;i < BOUND_DETECT_REGION_DEFAULT;i++)
            {
                BoundaryPlane_S defRegion;
                Common::PosF_S defPoint{ 0.0f, 0.0f };
                /* 确保其他字段也有默认值 */
                defRegion.stStartPos = {0.0f, 0.0f};
                defRegion.stEndPos = {0.0f, 0.0f};
                defRegion.nSensitivity = 50;
                defRegion.aDetectionTarget.clear();  /* 空数组但需要显示结构 */
                defRegion.enCrossDirection = A_TO_B;
                obj.aRule.push_back(defRegion);
            }
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }

    } BoundaryDetection_S;

    /************************ 越界报警相关 END ************************/

    /************************ 区域入侵相关 ************************/

    ///@brief 检测目标
    typedef enum _DetectionTarget_E_
    {
        HUMAN_DETECTION, /* 人体检测 */
        CAR_DETECTION,   /* 车辆检测 */
        OTHER_DETECTION, /* 其他检测 */
    } DetectionTarget_E;

    /// @brief 区域入侵的区域规则参数
    typedef struct _Intrusion_S_
    {
        Region_S stRegion;                 /* 区域定义 */
        unsigned int nTimeThreshold;       /* 行为事件触发时间阈值，判断有效报警的时间[0,100] */
        unsigned int nSensitivity;         /* 灵敏度[1,100] */
        std::vector<int> aDetectionTarget; /* 检测目标,DetectionTarget_E */

        /* 默认构造函数 */
        _Intrusion_S_() :
            stRegion(),
            nTimeThreshold(10),
            nSensitivity(50)
        {
            aDetectionTarget.clear();
        }
        /* 重载赋值运算符 */
        _Intrusion_S_ &operator=(const _Intrusion_S_ &x)
        {
            if (this != &x)
            {
                stRegion = x.stRegion;
                nTimeThreshold = x.nTimeThreshold;
                nSensitivity = x.nSensitivity;
                aDetectionTarget = x.aDetectionTarget;
            }
            return *this;
        }
    } Intrusion_S;

    /// @brief 区域入侵检测报警
    typedef struct _FieldDetection_S_
    {
        bool bEnable; /* 是否启用 0-不启用 1-启用 */
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        std::vector<std::vector<Common::SchedTime_S>> aAlarmTime;
        /* 区域入侵检测规则,最多设置4个-INTRUSION_AREA_MAX */
        std::vector<Intrusion_S> aRule;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        _FieldDetection_S_() :
            bEnable(false)
        {
            aAlarmTime.clear();
        }
        /* 重载赋值运算符 */
        _FieldDetection_S_ &operator=(const _FieldDetection_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aAlarmTime = x.aAlarmTime;
                aRule = x.aRule;
                stLinkageList = x.stLinkageList;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _FieldDetection_S_ CreateWithDefaultRule()
        {
            _FieldDetection_S_ obj;
            for (int i = 0; i < FIELD_DETECT_REGION_DEFAULT; i++)
            {
                Intrusion_S defRegion;
                defRegion.stRegion.nPointNum = 4;
                for (int j = 0; j < 4; ++j)
                {
                    Common::PosF_S defPoint{ 0.0f, 0.0f };
                    defRegion.stRegion.aPoint.push_back(defPoint);
                }
                /* 确保其他字段也有默认值 */
                defRegion.nTimeThreshold = 10;
                defRegion.nSensitivity = 50;
                defRegion.aDetectionTarget.clear(); /* 空数组但需要显示结构 */

                obj.aRule.push_back(defRegion);
            }
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }
    } FieldDetection_S;

    /************************ 区域入侵相关 END ************************/

    /************************ 进入区域相关 ************************/

    /// @brief 进入/离开区域的区域规则参数
    typedef struct _EnterExitIntrusion_S_
    {
        Region_S stRegion;                 /* 区域定义 */
        unsigned int nSensitivity;         /* 灵敏度[1,100] */
        unsigned int nTimeThreshold; /* 行为事件触发时间阈值，判断有效报警的时间[0,100]单位（秒） */
        std::vector<int> aDetectionTarget; /* 检测目标,DetectionTarget_E */

        /* 默认构造函数 */
        _EnterExitIntrusion_S_() :
            stRegion(),
            nSensitivity(50)
        {
            aDetectionTarget.clear();
        }
        /* 重载赋值运算符 */
        _EnterExitIntrusion_S_ &operator=(const _EnterExitIntrusion_S_ &x)
        {
            if (this != &x)
            {
                stRegion = x.stRegion;
                nSensitivity = x.nSensitivity;
                aDetectionTarget = x.aDetectionTarget;
            }
            return *this;
        }
    } EnterExitIntrusion_S;

    /// @brief 进入区域检测配置
    typedef struct _EntranceDetection_S_
    {
        bool bEnable; /* 是否启用 0-不启用 1-启用 */
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        std::vector<std::vector<Common::SchedTime_S>> aAlarmTime;
        /* 进入区域检测规则，最多设置4个-REGION_MAX */
        std::vector<EnterExitIntrusion_S> aRule;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        _EntranceDetection_S_() :
            bEnable(false)
        {
            aAlarmTime.clear();
        }
        /* 重载赋值运算符 */
        _EntranceDetection_S_ &operator=(const _EntranceDetection_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aAlarmTime = x.aAlarmTime;
                aRule = x.aRule;
                stLinkageList = x.stLinkageList;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _EntranceDetection_S_ CreateWithDefaultRule()
        {
            _EntranceDetection_S_ obj;
            for (int i = 0; i < ENTER_DETECT_REGION_DEFAULT; i++)
            {
                EnterExitIntrusion_S defRegion;
                defRegion.stRegion.nPointNum = 4;
                for (int j = 0; j < 4; ++j)
                {
                    Common::PosF_S defPoint{ 0.0f, 0.0f };
                    defRegion.stRegion.aPoint.push_back(defPoint);
                }
                /* 确保其他字段也有默认值 */
                defRegion.nSensitivity = 50; /* 空数组但需要显示结构 */
                defRegion.aDetectionTarget.clear(); /* 空数组但需要显示结构 */
                obj.aRule.push_back(defRegion);
            }
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }
    } EntranceDetection_S;

    /************************ 进入区域相关 END ************************/

    /************************ 离开区域相关 ************************/

    /// @brief 离开区域检测配置
    typedef struct _ExitingDetection_S_
    {
        bool bEnable; /* 是否启用 0-不启用 1-启用 */
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        std::vector<std::vector<Common::SchedTime_S>> aAlarmTime;
        /* 离开区域检测规则，最多设置4个-REGION_MAX */
        std::vector<EnterExitIntrusion_S> aRule;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        _ExitingDetection_S_() :
            bEnable(false)
        {
            aAlarmTime.clear();
        }
        /* 重载赋值运算符 */
        _ExitingDetection_S_ &operator=(const _ExitingDetection_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aAlarmTime = x.aAlarmTime;
                aRule = x.aRule;
                stLinkageList = x.stLinkageList;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _ExitingDetection_S_ CreateWithDefaultRule()
        {
            _ExitingDetection_S_ obj;
            for (int i = 0; i < EXIT_DETECT_REGION_DEFAULT; i++)
            {
                EnterExitIntrusion_S defRegion;
                defRegion.stRegion.nPointNum = 4;
                for (int j = 0; j < 4; ++j)
                {
                    Common::PosF_S defPoint{ 0.0f, 0.0f };
                    defRegion.stRegion.aPoint.push_back(defPoint);
                }
                /* 确保其他字段也有默认值 */
                defRegion.nSensitivity = 50; /* 空数组但需要显示结构 */
                defRegion.aDetectionTarget.clear(); /* 空数组但需要显示结构 */
                obj.aRule.push_back(defRegion);
            }
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }
    } ExitingDetection_S;
    /************************ 离开区域相关 END ************************/

    /**
     * @brief   : smart事件
     */

    /************************ 音频异常侦测相关 ************************/
    /* 音频异常侦测配置 */
    typedef struct _AudioAnomaly_S_
    {
        bool bEnable;            /* 是否启用 */
        bool bAudioInputAnomaly; /* 音频输入异常 是否启用*/
        /* 音频陡升数据 */
        bool bUpEnable;              /* 是否启用 */
        unsigned int nUpSensitivity; /* 灵敏度[1,100] */
        unsigned int nUpThreshold;   /* 声音强度阈值[1,100] */
        /* 音频陡降数据 */
        bool bDownEnable;              /* 是否启用 */
        unsigned int nDownSensitivity; /* 灵敏度[1,100] */
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        std::vector<std::vector<Common::SchedTime_S>> aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        _AudioAnomaly_S_() :
            bEnable(false),
            bAudioInputAnomaly(false),
            bUpEnable(false),
            nUpSensitivity(50),
            nUpThreshold(50),
            bDownEnable(false),
            nDownSensitivity(50)
        {
            aAlarmTime.clear();
        }
        /* 重载赋值运算符 */
        _AudioAnomaly_S_ &operator=(const _AudioAnomaly_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                bAudioInputAnomaly = x.bAudioInputAnomaly;
                bUpEnable = x.bUpEnable;
                nUpSensitivity = x.nUpSensitivity;
                nUpThreshold = x.nUpThreshold;
                bDownEnable = x.bDownEnable;
                nDownSensitivity = x.nDownSensitivity;
                aAlarmTime = x.aAlarmTime;
                stLinkageList = x.stLinkageList;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _AudioAnomaly_S_ CreateWithDefaultRule()
        {
            _AudioAnomaly_S_ obj;
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }

    } AudioAnomaly_S;
    /************************ 音频异常侦测相关 END ************************/

    /************************ 场景变更侦测相关 ************************/
    /* 场景变更侦测配置 */
    typedef struct _SceneChange_S_
    {
        bool bEnable;      /* 是否启用 */
        unsigned int nSensitivity; /* 灵敏度[1,100] */
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        std::vector<std::vector<Common::SchedTime_S>> aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        _SceneChange_S_() : bEnable(false), nSensitivity(50), stLinkageList()
        {
            aAlarmTime.clear();
        }
        /* 重载赋值运算符 */
        _SceneChange_S_ &operator=(const _SceneChange_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                nSensitivity = x.nSensitivity;
                aAlarmTime = x.aAlarmTime;
                stLinkageList = x.stLinkageList;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _SceneChange_S_ CreateWithDefaultRule()
        {
            _SceneChange_S_ obj;
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }

    } SceneChange_S;
    /************************ 场景变更侦测相关 END ************************/

    /************************ 人脸侦测相关 ************************/
    /* 人脸侦测 */
    typedef struct _FaceDetection_S_
    {
        /* 是否启用 */
        bool bEnable;
        /* 是否启用动态分析 */
        bool bDynamicAnalysisEnable;
        /* 灵敏度[1,100] */
        unsigned int nSensitivity;
        /* 规则区域定义 多边形框 */
        Region_S stRegion;
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        std::vector<std::vector<Common::SchedTime_S>> aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;

        _FaceDetection_S_() :
            bEnable(false),
            bDynamicAnalysisEnable(false),
            nSensitivity(50),
            stRegion(),
            stLinkageList()
        {
            aAlarmTime.clear();
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _FaceDetection_S_ CreateWithDefaultRule()
        {
            _FaceDetection_S_ obj;
            obj.stRegion.nPointNum = 4;
            for (size_t j = 0; j < obj.stRegion.nPointNum; ++j)
            {
                Common::PosF_S defPoint;
                obj.stRegion.aPoint.push_back(defPoint);
            }
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }
    } FaceDetection_S;
    /************************ 人脸侦测相关 END ************************/

    /************************ 徘徊侦测相关 ************************/
    /* 徘徊侦测规则参数 */
    typedef struct LoiteringRule
    {
        /* 区域定义 */
        Region_S stRegion;
        /* 行为事件触发时间阈值，判断有效报警的时间[0,100]单位（秒） */
        unsigned int nTimeThreshold;
        /* 灵敏度[1,100] */
        unsigned int nSensitivity;
        std::vector<int> aDetectionTarget;  /* 检测目标,DetectionTarget_E */
        /* 默认构造函数 */
        LoiteringRule() : stRegion(), nTimeThreshold(10), nSensitivity(50)
        {
        }
        /* 重载赋值运算符 */
        LoiteringRule &operator=(const LoiteringRule &x)
        {
            if (this != &x)
            {
                stRegion = x.stRegion;
                nSensitivity = x.nSensitivity;
                nTimeThreshold = x.nTimeThreshold;
            }
            return *this;
        }
    } LoiteringRule_S;

    /* 徘徊侦测配置 */
    typedef struct _LoiteringDetection_S_
    {
        /* 是否启用 */
        bool bEnable;
        /* 徘徊侦测规则，最多设置4个 LOITERING_DETECT_REGION_DEFAULT*/
        std::vector<LoiteringRule_S> aRule;
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        DefenseTime aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        _LoiteringDetection_S_() : bEnable(false), stLinkageList()
        {
            aAlarmTime.clear();
            aRule.clear();
        }
        /* 重载赋值运算符 */
        _LoiteringDetection_S_ &operator=(const _LoiteringDetection_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aAlarmTime = x.aAlarmTime;
                aRule = x.aRule;
                stLinkageList = x.stLinkageList;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _LoiteringDetection_S_ CreateWithDefaultRule()
        {
            _LoiteringDetection_S_ obj;
            for (size_t i = 0; i < LOITERING_DETECT_REGION_DEFAULT; ++i)
            {
                LoiteringRule_S defRegion;
                defRegion.stRegion.nPointNum = LOITERING_DETECT_REGION_DEFAULT;
                for (size_t j = 0; j < defRegion.stRegion.nPointNum; ++j)
                {
                    Common::PosF_S defPoint;
                    defRegion.stRegion.aPoint.push_back(defPoint);
                }
                obj.aRule.push_back(defRegion);
            }
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }
    } LoiteringDetection_S;
    /************************ 徘徊侦测相关 END ************************/

    /************************ 人员聚集侦测相关 ************************/
    /* 人员聚集侦测规则参数 */
    typedef struct _CrowdGatheringRule_S_
    {
        Region_S stRegion;         /* 区域定义 */
        unsigned int nObjectOccup; /* 人体面积占用户设定区域面积的比例阈值[1,100] */

        /* 默认构造函数 */
        _CrowdGatheringRule_S_() : stRegion(), nObjectOccup(50)
        {
        }
        /* 重载赋值运算符 */
        _CrowdGatheringRule_S_ &operator=(const _CrowdGatheringRule_S_ &x)
        {
            if (this != &x)
            {
                stRegion = x.stRegion;
                nObjectOccup = x.nObjectOccup;
            }
            return *this;
        }
    } CrowdGatheringRule_S;

    /* 人员聚集侦测配置 */
    typedef struct _CrowdGathering_S_
    {
        bool bEnable; /* 是否启用 */
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        std::vector<std::vector<Common::SchedTime_S>> aAlarmTime;
        /* 人员聚集检测规则，最多设置4个 CROWD_GATHERING_DETECT_REGION_DEFAULT */
        std::vector<CrowdGatheringRule_S> aRule;
        /* 联动 */
        LinkageList_S stLinkageList;

        /* 默认构造函数 */
        _CrowdGathering_S_() : bEnable(0), stLinkageList()
        {
            aAlarmTime.clear();
            aRule.clear();
        }
        /* 重载赋值运算符 */
        _CrowdGathering_S_ &operator=(const _CrowdGathering_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aAlarmTime = x.aAlarmTime;
                aRule = x.aRule;
                stLinkageList = x.stLinkageList;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _CrowdGathering_S_ CreateWithDefaultRule()
        {
            _CrowdGathering_S_ obj;
            for (size_t i = 0; i < CROWD_GATHERING_DETECT_REGION_DEFAULT; ++i)
            {
                CrowdGatheringRule_S defRegion;
                defRegion.stRegion.nPointNum = CROWD_GATHERING_DETECT_REGION_DEFAULT;
                for (size_t j = 0; j < defRegion.stRegion.nPointNum; ++j)
                {
                    Common::PosF_S defPoint;
                    defRegion.stRegion.aPoint.push_back(defPoint);
                }
                obj.aRule.push_back(defRegion);
            }
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }
    } CrowdGathering_S;
    /************************ 人员聚集侦测相关 END ************************/

    /************************ 停车侦测相关 ************************/
    /* 停车侦测规则参数 */
    typedef struct _ParkingRule_S_
    {
        Region_S stRegion;           /* 区域定义 */
        unsigned int nSensitivity;   /* 灵敏度[1,100] */
        unsigned int nTimeThreshold; /* 行为事件触发时间阈值，判断有效报警的时间[0,100]单位（秒） */
        /* 默认构造函数 */
        _ParkingRule_S_() : stRegion(), nSensitivity(50), nTimeThreshold(10)
        {
        }
        /* 重载赋值运算符 */
        _ParkingRule_S_ &operator=(const _ParkingRule_S_ &x)
        {
            if (this != &x)
            {
                stRegion = x.stRegion;
                nSensitivity = x.nSensitivity;
                nTimeThreshold = x.nTimeThreshold;
            }
            return *this;
        }
    } ParkingRule_S;

    /* 停车侦测配置 */
    typedef struct ParkingDetection
    {
        bool bEnable;    /* 是否启用 0-不启用 1-启用 */
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        std::vector<std::vector<Common::SchedTime_S>> aAlarmTime;
        /* 停车侦测规则，最多设置8个-REGION_MAX */
        std::vector<ParkingRule_S> aRule;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        ParkingDetection() : bEnable(false), stLinkageList()
        {
            aAlarmTime.clear();
            aRule.clear();
        }
        /* 重载赋值运算符 */
        ParkingDetection &operator=(const ParkingDetection &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aAlarmTime = x.aAlarmTime;
                aRule = x.aRule;
                stLinkageList = x.stLinkageList;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static ParkingDetection CreateWithDefaultRule()
        {
            ParkingDetection obj;
            for (size_t i = 0; i < PARKING_DETECT_REGION_DEFAULT; ++i)
            {
                ParkingRule_S defRegion;
                defRegion.stRegion.nPointNum = PARKING_DETECT_REGION_DEFAULT;
                for (size_t j = 0; j < defRegion.stRegion.nPointNum; ++j)
                {
                    Common::PosF_S defPoint;
                    defRegion.stRegion.aPoint.push_back(defPoint);
                }
                obj.aRule.push_back(defRegion);
            }
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }
    } ParkingDetection_S;
    /************************ 停车侦测相关 END ************************/

    /************************ 物品遗留侦测相关 ************************/
    /* 物品遗留侦测规则参数 */
    typedef struct _UnattendedObjectRule_S_
    {
        Region_S stRegion;              /* 区域定义 */
        unsigned int nSensitivity;      /* 灵敏度[1,100] */
        unsigned int nTimeThreshold;    /* 行为事件触发时间阈值，判断有效报警的时间[12,100]单位（秒） */
        /* 默认构造函数 */
        _UnattendedObjectRule_S_() : stRegion(), nSensitivity(50), nTimeThreshold(30)
        {
        }
        /* 重载赋值运算符 */
        _UnattendedObjectRule_S_ &operator=(const _UnattendedObjectRule_S_ &x)
        {
            if (this != &x)
            {
                stRegion = x.stRegion;
                nSensitivity = x.nSensitivity;
                nTimeThreshold = x.nTimeThreshold;
            }
            return *this;
        }
    } UnattendedObjectRule_S;

    /* 物品遗留侦测配置 */
    typedef struct _UnattendedObject_S_
    {
        bool bEnable;    /* 是否启用 */
        /* 物品遗留检测规则，最多设置4个-UNATTENDED_OBJECT_DETECT_REGION_DEFAULT */
        std::vector<UnattendedObjectRule_S> aRule;
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        std::vector<std::vector<Common::SchedTime_S>> aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        _UnattendedObject_S_() :
            bEnable(false),
            stLinkageList()
        {
            aRule.clear();
            aAlarmTime.clear();
        }
        /* 重载赋值运算符 */
        _UnattendedObject_S_ &operator=(const _UnattendedObject_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aRule = x.aRule;
                aAlarmTime = x.aAlarmTime;
                stLinkageList = x.stLinkageList;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _UnattendedObject_S_ CreateWithDefaultRule()
        {
            _UnattendedObject_S_ obj;
            for (size_t i = 0; i < UNATTENDED_OBJECT_DETECT_REGION_DEFAULT; ++i)
            {
                UnattendedObjectRule_S defRegion;
                defRegion.stRegion.nPointNum = UNATTENDED_OBJECT_DETECT_REGION_DEFAULT;
                for (size_t j = 0; j < defRegion.stRegion.nPointNum; ++j)
                {
                    Common::PosF_S defPoint;
                    defRegion.stRegion.aPoint.push_back(defPoint);
                }
                obj.aRule.push_back(defRegion);
            }
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }
    } UnattendedObject_S;
    /************************ 物品遗留侦测相关 END ************************/

    /************************ 物品拿取侦测相关 ************************/

    /* 物品拿取侦测规则参数 */
    typedef struct _ObjectRemovalRule_S_
    {
        Region_S stRegion;           /* 区域定义 */
        unsigned int nSensitivity;   /* 灵敏度[1,100] */
        unsigned int nTimeThreshold; /* 行为事件触发时间阈值，判断有效报警的时间[12,100]单位（秒） */
        /* 默认构造函数 */
        _ObjectRemovalRule_S_() : stRegion(), nSensitivity(50), nTimeThreshold(30)
        {
        }
        /* 重载赋值运算符 */
        _ObjectRemovalRule_S_ &operator=(const _ObjectRemovalRule_S_ &x)
        {
            if (this != &x)
            {
                stRegion = x.stRegion;
                nSensitivity = x.nSensitivity;
                nTimeThreshold = x.nTimeThreshold;
            }
            return *this;
        }
    } ObjectRemovalRule_S;

    /* 物品拿取侦测配置 */
    typedef struct _ObjectRemoval_S_
    {
        bool bEnable;    /* 是否启用 */
        /* 物品遗留检测规则，最多设置4个-OBJECT_REMOVAL_DETECT_REGION_DEFAULT */
        std::vector<ObjectRemovalRule_S> aRule;
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        std::vector<std::vector<Common::SchedTime_S>> aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        _ObjectRemoval_S_() :
            bEnable(false),
            stLinkageList()
        {
            aRule.clear();
            aAlarmTime.clear();
        }
        /* 重载赋值运算符 */
        _ObjectRemoval_S_ &operator=(const _ObjectRemoval_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aRule = x.aRule;
                aAlarmTime = x.aAlarmTime;
                stLinkageList = x.stLinkageList;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _ObjectRemoval_S_ CreateWithDefaultRule()
        {
            _ObjectRemoval_S_ obj;
            for (size_t i = 0; i < OBJECT_REMOVAL_DETECT_REGION_DEFAULT; ++i)
            {
                ObjectRemovalRule_S defRegion;
                defRegion.stRegion.nPointNum = OBJECT_REMOVAL_DETECT_REGION_DEFAULT;
                for (size_t j = 0; j < defRegion.stRegion.nPointNum; ++j)
                {
                    Common::PosF_S defPoint;
                    defRegion.stRegion.aPoint.push_back(defPoint);
                }
                obj.aRule.push_back(defRegion);
            }
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }
    } ObjectRemoval_S;
    /************************ 物品拿取侦测相关 END ************************/

    /************************ 宠物识别相关 ************************/
    /* 宠物识别 */
    typedef struct _PetRecognition_S_
    {
        /* 是否启用 */
        bool bEnable;
        /* 是否启用动态分析 */
        bool bDynamicAnalysisEnable;
        /* 灵敏度[1,100] */
        unsigned int nSensitivity;
        /* 规则区域定义 多边形框 */
        Region_S stRegion;
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        std::vector<std::vector<Common::SchedTime_S>> aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;

        _PetRecognition_S_() :
            bEnable(false),
            bDynamicAnalysisEnable(false),
            nSensitivity(50),
            stRegion(),
            stLinkageList()
        {
            aAlarmTime.clear();
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _PetRecognition_S_ CreateWithDefaultRule()
        {
            _PetRecognition_S_ obj;
            obj.stRegion.nPointNum = 4;
            for (size_t j = 0; j < obj.stRegion.nPointNum; ++j)
            {
                Common::PosF_S defPoint;
                obj.stRegion.aPoint.push_back(defPoint);
            }
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }
    } PetRecognition_S;
    /************************ 宠物识别相关 END ************************/

    /************************ 人脸抓拍相关 ************************/

    /* 人脸抓拍规则参数 */
    typedef struct _FaceCaptureRule_S_
    {
        unsigned int nSensitivity;               /* 灵敏度[1,100] */
        Region_S stRegion;                       /* 规则区域定义 多边形框 */
        std::vector<Region_S> vstShieldedRegion; /* 屏蔽区域区域定义 多边形框 默认4个 */
        Common::Rect_S stMinIpdRect;             /* 最小瞳距区域定义 矩形框 */
        int nMinWidth;                           /* 最小瞳距宽度 */
        int nMinHeight;                          /* 最小瞳距高度 */
        int nMaxWidth;                           /* 最大瞳距宽度 */
        int nMaxHeight;                          /* 最大瞳距高度 */
        int nInterval;                           /* 抓拍间隔 */
        /* 默认构造函数 */
        _FaceCaptureRule_S_() :
            nSensitivity(50),
            stRegion(),
            stMinIpdRect(),
            nMinWidth(30),
            nMinHeight(30),
            nMaxWidth(225),
            nMaxHeight(225),
            nInterval(3)
        {
            vstShieldedRegion.clear();
        }
        /* 重载赋值运算符 */
        _FaceCaptureRule_S_ &operator=(const _FaceCaptureRule_S_ &x)
        {
            if (this != &x)
            {
                nSensitivity = x.nSensitivity;
                stRegion = x.stRegion;
                vstShieldedRegion = x.vstShieldedRegion;
                stMinIpdRect = x.stMinIpdRect;
                nMinWidth = x.nMinWidth;
                nMinHeight = x.nMinHeight;
                nMaxWidth = x.nMaxWidth;
                nMaxHeight = x.nMaxHeight;
                nInterval = x.nInterval;
            }
            return *this;
        }
    } FaceCaptureRule_S;

    /* 人脸抓拍配置 */
    typedef struct _FaceCapture_S_
    {
        bool bEnable;    /* 是否启用 */
        /* 人脸抓拍规则 */
        FaceCaptureRule_S stRule;
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        std::vector<std::vector<Common::SchedTime_S>> aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        _FaceCapture_S_() : bEnable(0), stRule(), stLinkageList()
        {
            aAlarmTime.clear();
        }

        _FaceCapture_S_(const _FaceCapture_S_ &other)
            : bEnable(other.bEnable), stRule(other.stRule), stLinkageList(other.stLinkageList)
        {
            aAlarmTime = other.aAlarmTime;
        }

        /* 重载赋值运算符 */
        _FaceCapture_S_ &operator=(const _FaceCapture_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                stRule = x.stRule;
                aAlarmTime = x.aAlarmTime;
                stLinkageList = x.stLinkageList;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _FaceCapture_S_ CreateWithDefaultRule()
        {
            _FaceCapture_S_ obj;
            for (size_t i = 0; i < 4; ++i)
            {
                obj.stRule.vstShieldedRegion.emplace_back(Region_S::CreateWithDefaultRule(4));
            }
            obj.stRule.stRegion = Region_S::CreateWithDefaultRule(4);
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }
    } FaceCapture_S;

/* 人脸比对配置 */
typedef struct _FaceCompare_S_
{
    bool bEnable;    /* 是否启用 */

    /* 布防时间:一周7天，每天可以设置8个时间段 */
    std::vector<std::vector<Common::SchedTime_S>> aAlarmTime;
    /* 成功联动 */
    LinkageList_S stLinkageListSuccess,stLinkageListFail;
    /* 默认构造函数 */
    _FaceCompare_S_() : bEnable(0),  stLinkageListSuccess(),stLinkageListFail()
    {
        aAlarmTime.clear();
    }

    _FaceCompare_S_(const _FaceCompare_S_ &other)
        : bEnable(other.bEnable), stLinkageListSuccess(other.stLinkageListSuccess),stLinkageListFail(other.stLinkageListFail)
    {
        aAlarmTime = other.aAlarmTime;
    }

    static _FaceCompare_S_ CreateWithDefaultRule()
    {
        _FaceCompare_S_ obj;

        obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
        return obj;
    }
} FaceCompare_S;

    /* 人脸抓拍叠加信息 */
    typedef struct _OverlayInfo_S_
    {
        /* 监控点参数 */
        int nDeviceID;                    /* 设备编号 */
        std::string strMonitoryPointInfo; /* 监控点信息 */
        /* 图片字符叠加 */
        bool bOverlayDeviceID;          /* 叠加设备编号 */
        bool bOverlayCaptureTime;       /* 叠加抓拍时间 */
        bool bOverlayMonitoryPointInfo; /* 叠加监控点信息 */
        Osd::OSD_COLOR_E enFontColor;   /* 字体颜色 */
        std::string strFontColor;       /* 字体颜色为自定义时，使用，记录RGB颜色 格式:"#000000"" */

        /* 默认构造函数 */
        _OverlayInfo_S_() :
            nDeviceID(1),
            strMonitoryPointInfo(),
            bOverlayDeviceID(false),
            bOverlayCaptureTime(false),
            bOverlayMonitoryPointInfo(false),
            enFontColor(Osd::OSD_COLOR_E::OSD_COLOR_BLACK),
            strFontColor("#000000")
        {
        }

        _OverlayInfo_S_(const _OverlayInfo_S_ &other) :
            nDeviceID(other.nDeviceID),
            strMonitoryPointInfo(other.strMonitoryPointInfo),
            bOverlayDeviceID(other.bOverlayDeviceID),
            bOverlayCaptureTime(other.bOverlayCaptureTime),
            bOverlayMonitoryPointInfo(other.bOverlayMonitoryPointInfo),
            enFontColor(other.enFontColor),
            strFontColor(other.strFontColor)
        {
        }

        /* 重载赋值运算符 */
        _OverlayInfo_S_ &operator=(const _OverlayInfo_S_ &x)
        {
            if (this != &x)
            {
                nDeviceID = x.nDeviceID;
                strMonitoryPointInfo = x.strMonitoryPointInfo;
                bOverlayDeviceID = x.bOverlayDeviceID;
                bOverlayCaptureTime = x.bOverlayCaptureTime;
                bOverlayMonitoryPointInfo = x.bOverlayMonitoryPointInfo;
                enFontColor = x.enFontColor;
                strFontColor = x.strFontColor;
            }
            return *this;
        }

        /* 重载等于运算符 */
        bool operator==(const _OverlayInfo_S_ &x) const
        {
            return (nDeviceID == x.nDeviceID && strMonitoryPointInfo == x.strMonitoryPointInfo
                    && bOverlayDeviceID == x.bOverlayDeviceID && bOverlayCaptureTime == x.bOverlayCaptureTime
                    && bOverlayMonitoryPointInfo == x.bOverlayMonitoryPointInfo && enFontColor == x.enFontColor
                    && strFontColor == x.strFontColor);
        }

        /* 重载不等于运算符 */
        bool operator!=(const _OverlayInfo_S_ &x) const
        {
            return !(*this == x);
        }
    } OverlayInfo_S;

    /*  人脸属性信息 */
    typedef struct _FaceAlarmAttribute_S_
    {
        bool bIsMale;       /* 是否是男性 */
        int  nAgeLabel;     /* 年龄标签:1, 2, 3, 4 */
        bool bIsGlasses;    /* 是否戴眼镜 */
        bool bIsBeard;      /* 是否有胡子 */
        bool bIsMask;       /* 是否戴口罩 */
        int  nEmotionLabel; /* 年龄标签:8, 9, 10, 11, 12, 13, 14 */
         _FaceAlarmAttribute_S_() :
            bIsMale(false),
            nAgeLabel(-1),
            bIsGlasses(false),
            bIsBeard(false),
            bIsMask(false),
            nEmotionLabel(-1)
        {
        }

        _FaceAlarmAttribute_S_(const _FaceAlarmAttribute_S_ &other) :
            bIsMale(other.bIsMale),
            nAgeLabel(other.nAgeLabel),
            bIsGlasses(other.bIsGlasses),
            bIsBeard(other.bIsBeard),
            bIsMask(other.bIsMask),
            nEmotionLabel(other.nEmotionLabel)

        {
        }

        /* 重载赋值运算符 */
        _FaceAlarmAttribute_S_ &operator=(const _FaceAlarmAttribute_S_ &x)
        {
            if (this != &x)
            {
                bIsMale = x.bIsMale;
                nAgeLabel = x.nAgeLabel;
                bIsGlasses = x.bIsGlasses;
                bIsBeard = x.bIsBeard;
                bIsMask = x.bIsMask;
                nEmotionLabel = x.nEmotionLabel;
            }
            return *this;
        }
    }FaceAlarmAttribute_S;

    /*  人脸抓拍报警上报信息 */
    typedef struct _FaceAlarmInfo_S_
    {
        FaceAlarmAttribute_S stFaceAlarmAttribute;      /* 人脸属性 */
        Region_S stFaceRegion;                          /* 人脸坐标 */

        std::string strFacePicture;                     /* 人脸图片 */
        std::string strCurrentPicture;                  /* 当前画面 */
        std::string strTimeStamp;                       /* 抓拍时间 */
        bool bIsDownLoad;                               /* 是否能下载 */

        /* 默认构造函数 */
        _FaceAlarmInfo_S_() : stFaceAlarmAttribute(), stFaceRegion()
        {
        }
        /* 重载赋值运算符 */
        _FaceAlarmInfo_S_ &operator=(const _FaceAlarmInfo_S_ &x)
        {
            if (this != &x)
            {
                stFaceRegion = x.stFaceRegion;
                stFaceAlarmAttribute = x.stFaceAlarmAttribute;
            }
            return *this;
        }
    }FaceAlarmInfo_S;

    /************************ 人脸抓拍相关 END ************************/

#ifdef SCENE_INTELLIGENT_ANALYSIS
    /**
    * @brief   : 场景智能分析事件
    */

    /************************ 场景智能分析控制相关 ************************/

    typedef struct _LLMAISceneAnalysis_S_
    {
        bool bEnable;       /* 是否启用 */
        bool bNewDialogue;   /* 是否新对话 */
        bool bAnalysisStop; /* 是否中断推理 */

        _LLMAISceneAnalysis_S_() : bEnable(false),bNewDialogue(false),bAnalysisStop(false)
        {
        }

        /* 重载赋值运算符 */
        _LLMAISceneAnalysis_S_ &operator=(const _LLMAISceneAnalysis_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                bNewDialogue = x.bNewDialogue;
                bAnalysisStop = x.bAnalysisStop;
            }
            return *this;
        }

    } LLMAISceneAnalysis_S;

    /************************ 场景智能分析控制相关 END *********************/

    /************************ 画面分析相关 ************************/

    /* 重复分析配置 - 按周重复 */
    typedef struct _RepeatedAnalysisConfig_S_
    {
        /* 选择的星期几 [1-7]: 1=周一, 2=周二, ..., 7=周日 */
        std::vector<int> aWeekdays;
        /* 执行时间 */
        Common::Time_S stExecuteTime;  /* 小时:分钟:秒 */

        _RepeatedAnalysisConfig_S_()
        {
            /* 确保唯一性 */
            NormalizeWeekdays();
            stExecuteTime.nHour = 0;
            stExecuteTime.nMinute = 0;
            stExecuteTime.nSecond = 0;
        }

         /* 重载赋值运算符 */
         _RepeatedAnalysisConfig_S_ &operator=(const _RepeatedAnalysisConfig_S_ &x)
         {
             if (this != &x)
             {
                aWeekdays = x.aWeekdays;
                stExecuteTime = x.stExecuteTime;
                NormalizeWeekdays();
             }
             return *this;
         }

        /*规范化星期数组*/
        void NormalizeWeekdays()
        {
            std::set<int> uniqueSet(aWeekdays.begin(), aWeekdays.end());
            aWeekdays.clear();
            for (int day : uniqueSet) {
                if (day >= 1 && day <= 7) {
                    aWeekdays.push_back(day);
                }
            }
        }

    } RepeatedAnalysisConfig_S;

    /* 间隔分析配置 */
    typedef struct _IntervalAnalysisConfig_S_
    {
        /* 执行日期范围 */
        Common::Date_S stStartDate;     /* 开始日期 */
        Common::Date_S stEndDate;       /* 结束日期 */
        /* 间隔时间 */
        Common::Time_S stIntervalTime;  /* 间隔时间：时:分:秒 */

        _IntervalAnalysisConfig_S_()
        {
            // 获取当前系统时间
            auto now = std::chrono::system_clock::now();
            std::time_t time_now = std::chrono::system_clock::to_time_t(now);
            std::tm* local_time = std::localtime(&time_now);
            int currentYear = local_time->tm_year + 1900;

            /* 默认开始日期为今天 */
            stStartDate.nYear = currentYear;
            stStartDate.nMonth = 1;
            stStartDate.nDay = 1;

            /* 默认结束日期为一年后 */
            stEndDate.nYear = currentYear;
            stEndDate.nMonth = 12;
            stEndDate.nDay = 31;

            /* 默认间隔30分钟 */
            stIntervalTime.nHour = 0;
            stIntervalTime.nMinute = 30;
            stIntervalTime.nSecond = 0;
        }
    } IntervalAnalysisConfig_S;

    // 历史记录操作类型枚举
    typedef enum AnalysisRecordOperate
    {
        GET_ALL_RECORDS,              // 获取全部记录
        GET_SESSION_ALL_RECORDS,      // 获取单个会话的所有记录
        GET_SESSION_SINGLE_RECORD,    // 获取单个会话的单条指定记录
        DELETE_ALL_RECORDS,           // 删除全部记录
        DELETE_SESSION_ALL_RECORDS,   // 删除单个会话的所有记录
        DELETE_SESSION_SINGLE_RECORD, // 删除单个会话的单条指定记录
        SET_CURRENT_SESSION_INDEX,    // 设置会话的索引值，如当前是第二个会话
        SEARCH_RECORDS                // 搜索记录（根据输入文本关键词）
    }AnalysisRecordOperate_E;

    typedef struct _AnalysisRecords_S_
    {
        /*唯一标识符*/
        std::string strId;
        /*创建时间*/
        std::string strCreateTime;
        /*输入分析的文本内容*/
        std::string strInputText;
        /*结果创建时间*/
        std::string strOutputCreateTime;
        /*输出推理结果的文本内容*/
        std::string strOutputText;
        /*输入分析的图片路径*/
        std::string strInputImagePath;
        /*保存的视频切片的路径*/
        std::string strVideoPath;
        /*跳过删除相关文件*/
        bool bSkipDelete = false;

    } AnalysisRecords_S;

       // 记录的索引项结构
    typedef struct _AnalysisRecordIndexItem_S_
    {
        std::string indexKey;                    // 索引键
        std::vector<AnalysisRecords_S> records;  // 该索引下的记录集合
    } AnalysisRecordIndexItem_S;

    // 记录的总项结构
    typedef struct _AnalysisAllRecordIndexItem_S_
    {
        /*会话总数*/
        int total_sessions;
        /*当前会话的索引（从0开始计数）*/
        int current_session_index;
        /*操作的索引值，如删除第一个会话记录-index=0*/
        int Operateindex;
        /*操作的子索引值，如删除第一个会话记录第一条记录-subindex=0*/
        int Operatesubindex;
        /*进行的操作*/
        AnalysisRecordOperate_E enAnalysisRecordOperate;
        /*搜索关键字（用于搜索操作）*/
        std::string SearchKeyword;
        /*删除关键字（用于搜索操作）*/
        std::string DelKeyID;

        std::vector<AnalysisRecordIndexItem_S> Allrecords;

    } AnalysisAllRecordIndexItem_S;

     typedef enum AnalysisSchedule
    {
        REPEATED,   /* 重复分析*/
        INTERVAL    /* 间隔分析*/
    } AnalysisSchedule_E;
    typedef struct _LLMImageAnalysis_S_
    {
        bool bEnable;    /* 是否启用 */
        /* 是否中断推理 */
        bool bAnalysisStop;
        /* 是否启用截图 */
        bool bScreenshotEnable;
        /* 是否启用定时分析 */
        bool bScheduleEnable;
        /* 定时分析模式 - AnalysisSchedule_E*/
        AnalysisSchedule_E enAnalysisScheduleMode;
        /* 重复分析时间 */
        RepeatedAnalysisConfig_S stRepeatedConfig;
        /* 间隔分析时间 */
        IntervalAnalysisConfig_S stIntervalConfig;
        /*输入分析的文本内容*/
        std::string strAnalysisInputText;
        /*输入分析的图片路径*/
        std::string strAnalysisInputImagePath;

        _LLMImageAnalysis_S_() : bEnable(false),bAnalysisStop(false),bScreenshotEnable(false),bScheduleEnable(false), enAnalysisScheduleMode(REPEATED)
        {
        }

        /* 重载赋值运算符 */
        _LLMImageAnalysis_S_ &operator=(const _LLMImageAnalysis_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                bAnalysisStop = x.bAnalysisStop;
                bScreenshotEnable = x.bScreenshotEnable;
                bScheduleEnable = x.bScheduleEnable;
                enAnalysisScheduleMode = x.enAnalysisScheduleMode;
                stRepeatedConfig = x.stRepeatedConfig;
                stIntervalConfig = x.stIntervalConfig;
                strAnalysisInputText = x.strAnalysisInputText;
                strAnalysisInputImagePath = x.strAnalysisInputImagePath;
            }
            return *this;
        }

    } LLMImageAnalysis_S;

    /************************ 画面分析相关 END *********************/

    /************************ 文字预设任务相关 *********************/

    /* 检测频率 */
    typedef enum _DetectionFrequency_E_
    {
        DETECTION_FREQ_10S,  /* 10秒 */
        DETECTION_FREQ_20S,  /* 20秒 */
        DETECTION_FREQ_1MIN, /* 1分钟 */
        DETECTION_FREQ_5MIN, /* 5分钟 */
    } DetectionFrequency_E;

    /* 检测物体 */
    typedef enum _DetectionObject_E_
    {
        HUMAN,          /* 人 */
        VEHICLE,        /* 车辆 */
        REFLECT_CLOTH,  /* 反光衣 */
        ENVIRONMENT,    /* 环境 */
        DETECT_OBJECT_NULL,
    } DetectionObject_E;

    /* 文字预设任务操作类型 */
    typedef enum _TextPresetTaskOperationType_E_
    {
        TASK_OP_ADD = 0,        /* 添加任务 */
        TASK_OP_UPDATE = 1,     /* 更新任务 */
        TASK_OP_DELETE = 2,     /* 删除任务 */
    } TextPresetTaskOperationType_E;

    /* 文字预设状态类型 */
    typedef enum _TextPresetTaskStatus_E_
    {
        TASK_STATUS_ALL,
        TASK_STATUS_ENABLE,     /* 启用 */
        TASK_STATUS_DISENABLE   /* 禁用 */
    } TextPresetTaskStatus_E;

    /* 判定条件 */
    typedef enum _DetectionCondition_E_
    {
        /* 人相关条件 */
        CONDITION_BLACK_CLOTHES,      /* 穿黑色衣服 */
        CONDITION_WRESTLING,          /* 摔跤 */
        CONDITION_FIGHTING,           /* 打架斗殴 */
        CONDITION_GATHERED,           /* 人员聚集 */
        CONDITION_PHONE_USAGE,        /* 玩手机 */

        /* 车辆相关条件 */
        CONDITION_NO_VEHICLE,         /* 禁止出现车辆 */
        CONDITION_CAR_ACCIDENT,       /* 车祸检测 */

        /* 反光衣相关条件 */
        CONDITION_ORANGE_CLOTHES,     /* 橙色 */
        CONDITION_GREEN_CLOTHES,      /* 橙色 */

        /* 环境相关条件 */
        CONDITION_FIRE_DETECTION,     /* 火灾检测 */
        CONDITION_SMOKE_DETECTION,    /* 烟雾检测 */
        CONDITION_WATER_ACCUMULATION, /* 积水检测 */
        CONDITION_TRASH_OVERFLOW,     /* 垃圾桶满溢 */
        CONDITION_NULL
    } DetectionCondition_E;

    /* 文字预设 */
    typedef struct _TextPreset_S_
    {
        TextPresetTaskOperationType_E enOperationType; /* 文字预设操作*/
        std::string strTaskId;                  /* 任务ID */
        bool bEnable;                           /* 是否启用 */
        std::string strTaskName;                /* 任务名称 */
        bool bDrawArea;                         /* 是否绘制区域 */
        Common::Rect_S stRect;                  /* 分析区域 */
        std::string strObjectName;              /* 用户输入的检测物体名称 */
        std::string strConditionName;           /* 用户输入的检测条件名称 */
        DetectionFrequency_E enDetectFrequency; /* 检测频率 */
        std::string strImagePath;               /* 图片保存路径 */
        std::string strVideoPath;               /* 视频保存路径 */

        /* 筛选 */
        TextPresetTaskStatus_E enTaskPresetDealStatus;   /* 任务状态 */

        /* 布防时间:一周7天，每天可以设置8个时间段 */
        std::vector<std::vector<Common::SchedTime_S>> aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;

        /**
        * @brief 生成唯一任务ID
        * @return 生成的任务ID字符串
        */
        static std::string generateTaskId()
        {
            /* 使用时间戳 + 随机数生成唯一ID */
            auto now = std::chrono::system_clock::now();
            auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

            /* 添加随机数确保唯一性 */
            static std::random_device rd;
            static std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(1000, 9999);
            int random_num = dis(gen);

            return "TASK_" + std::to_string(timestamp) + "_" + std::to_string(random_num);
        }

          /* 默认构造函数 */
        _TextPreset_S_() :
            enOperationType(TASK_OP_ADD),
            strTaskId(generateTaskId()),  /* 自动生成任务ID */
            bEnable(false),
            strTaskName(),
            bDrawArea(false),
            stRect(),
            strObjectName(),
            strConditionName(),
            enDetectFrequency(DETECTION_FREQ_20S),
            strImagePath(),
            strVideoPath(),
            stLinkageList()
        {
            aAlarmTime.clear();
            aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
        }

        /* 重载赋值运算符 */
        _TextPreset_S_ &operator=(const _TextPreset_S_ &x)
        {
            if (this != &x)
            {
                enOperationType = x.enOperationType;
                strTaskId = x.strTaskId;
                bEnable = x.bEnable;
                strTaskName = x.strTaskName;
                bDrawArea = x.bDrawArea;
                stRect = x.stRect;
                strObjectName = x.strObjectName;
                strConditionName = x.strConditionName;
                enDetectFrequency = x.enDetectFrequency;
                strImagePath = x.strImagePath;
                strVideoPath = x.strVideoPath;
                enTaskPresetDealStatus = x.enTaskPresetDealStatus;
                aAlarmTime = x.aAlarmTime;
                stLinkageList = x.stLinkageList;
            }
            return *this;
        }

    }TextPreset_S;

    /* 文字预设查询过滤条件 */
    typedef struct _TextPresetQueryFilter_S_
    {
        /* 过滤条件 */
        std::string strTaskNameFilter;              /* 任务名称过滤 */
        std::string strObjectNameFilter;            /* 物体名称过滤 */
        std::string strConditionNameFilter;         /* 条件名称过滤 */
        TextPresetTaskStatus_E enTaskStatusFilter;  /* 任务状态过滤 */


        /* 默认构造函数 */
        _TextPresetQueryFilter_S_() :
            strTaskNameFilter(),
            strObjectNameFilter(),
            strConditionNameFilter(),
            enTaskStatusFilter(TASK_STATUS_ALL)
        {
        }

    } TextPresetQueryFilter_S;

    /* 文字预设任务管理配置 */
    typedef struct _TextPresetTaskManager_S_
    {
        std::vector<Alarm::TextPreset_S> aTaskConfig;   /* 所有任务配置 */
        std::string strCurrentActiveTaskId;             /* 当前激活的任务ID */

        /* 默认构造函数 */
        _TextPresetTaskManager_S_() : strCurrentActiveTaskId("")
        {
            aTaskConfig.clear();
        }

        /* 重载赋值运算符 */
        _TextPresetTaskManager_S_ &operator=(const _TextPresetTaskManager_S_ &x)
        {
            if (this != &x)
            {
                aTaskConfig = x.aTaskConfig;
                strCurrentActiveTaskId = x.strCurrentActiveTaskId;
            }
            return *this;
        }
    } TextPresetTaskManager_S;

    /************************文字预设任务相关 END *******************/

    /************************ 实时预警推送相关 ***********************/

    /* 实时预警推送操作类型 */
    typedef enum _RealAlarmPushOperationType_E_
    {
        PUSH_OP_PROCESS = 0, /* 处理操作 */
        PUSH_OP_DELETE = 1,  /* 删除操作 */
        PUSH_OP_IGNORE = 2,  /* 忽视操作 */
        PUSH_OP_AUTO = 3,    /* 设置操作 */
        PUSH_OP_CANCEL_IGNORE = 4, /* 取消忽视操作 */
    } RealAlarmPushOperationType_E;

    /* 实时预警推送处理状态 */
    typedef enum _RealAlarmPushDealStatus_E_
    {
        PUSH_DEAL_STATUS_NONE = 0,      /* 未处理 */
        PUSH_DEAL_STATUS_PROCESSED = 1, /* 已处理 */
        PUSH_DEAL_STATUS_IGNORE = 2,    /* 已忽略 */
        PUSH_DEAL_STATUS_ALL            /* 全部 */
    } RealAlarmPushDealStatus_E;

    /* 实时预警推送时间 */
    typedef struct _RealAlarmPushTime_S_
    {
        Common::Date_S stDate; /* 推送日期 */
        Common::Time_S stTime; /* 推送时间 */

        /* 默认构造函数 */
        _RealAlarmPushTime_S_()
        {
            stDate.nYear = 0;
            stDate.nMonth = 0;
            stDate.nDay = 0;
            stTime.nHour = 0;
            stTime.nMinute = 0;
            stTime.nSecond = 0;
        }
    } RealAlarmPushTime_S;

    /* 实时预警已处理记录 */
    typedef struct _RealAlarmProcessRecord_S_
    {
        std::string strProcessUser;        /* 处理者 */
        std::string strProcessRemark;      /* 处理备注 */
        RealAlarmPushTime_S stProcessTime; /* 处理时间 */

        /* 默认构造函数 */
        _RealAlarmProcessRecord_S_() :
            strProcessUser(),
            strProcessRemark()
        {
        }

    } RealAlarmProcessRecord_S;

    /* 实时预警推送记录 */
    typedef struct _RealAlarmPushRecord_S_
    {
        RealAlarmPushOperationType_E enOperationType;          /* 操作类型 */
        std::string strTaskId;                                 /* 任务ID */
        std::string strTaskName;                               /* 任务名称 */
        std::string strObjectName;                             /* 检测目标 */
        std::string strConditionName;                          /* 检测条件 */
        RealAlarmPushDealStatus_E enDealStatus;                /* 处理状态 */
        RealAlarmPushTime_S stAlarmTime;                       /* 报警时间 */
        std::string strDescription;                            /* 描述信息 */
        std::string strImagePath;                              /* 关联图片路径 */
        std::string strVideoPath;                              /* 关联视频路径 */
        std::vector<RealAlarmProcessRecord_S> aProcessRecords; /* 实时预警已处理记录 */

        /* 默认构造函数 */
        _RealAlarmPushRecord_S_() :
            enOperationType(),
            strTaskId(),
            strTaskName(),
            strObjectName(),
            strConditionName(),
            enDealStatus(PUSH_DEAL_STATUS_NONE),
            strDescription(),
            strImagePath(),
            strVideoPath()
        {
            aProcessRecords.clear();
        }
        /* 重载赋值运算符 */
        _RealAlarmPushRecord_S_ &operator=(const _RealAlarmPushRecord_S_ &x)
        {
            if (this != &x)
            {
                strTaskId = x.strTaskId;
                strTaskName = x.strTaskName;
                strObjectName = x.strObjectName;
                strConditionName = x.strConditionName;
                enDealStatus = x.enDealStatus;
                stAlarmTime = x.stAlarmTime;
                strDescription = x.strDescription;
                strImagePath = x.strImagePath;
                strVideoPath = x.strVideoPath;
                aProcessRecords = x.aProcessRecords;
            }
            return *this;
        }
    } RealAlarmPushRecord_S;

    /* 实时预警推送查询过滤条件 */
    typedef struct _RealAlarmPushQueryFilter_S_
    {
        /* 过滤条件 */
        std::string strTaskNameFilter;                /* 任务名称过滤 */
        std::string strObjectNameFilter;              /* 目标过滤 */
        std::string strConditionNameFilter;           /* 条件过滤 */
        RealAlarmPushDealStatus_E enDealStatusFilter; /* 处理状态过滤 */

        /* 时间范围过滤 */
        RealAlarmPushTime_S stStartTime; /* 开始时间 */
        RealAlarmPushTime_S stEndTime;   /* 结束时间 */

        /* 默认构造函数 */
        _RealAlarmPushQueryFilter_S_() :
            strTaskNameFilter(),
            strObjectNameFilter(),
            strConditionNameFilter(),
            enDealStatusFilter(PUSH_DEAL_STATUS_ALL)
        {
        }

    } RealAlarmPushQueryFilter_S;

    /* 实时预警推送管理配置 */
    typedef struct _RealAlarmPushManager_S_
    {
        bool bNotifyUpdate;                              /*通知Web更新标识*/
        bool bAutoLaestAlarm;                            /* 是否自动更新最新报警 */
        bool bAutoPlay;                                  /* 是否自动播放 */
        std::vector<RealAlarmPushRecord_S> aPushRecords; /* 所有推送记录 */

        /* 默认构造函数 */
        _RealAlarmPushManager_S_() :
            bNotifyUpdate(false),
            bAutoLaestAlarm(false),
            bAutoPlay(false)
        {
            aPushRecords.clear();
        }

        /* 重载赋值运算符 */
        _RealAlarmPushManager_S_ &operator=(const _RealAlarmPushManager_S_ &x)
        {
            if (this != &x)
            {
                bAutoLaestAlarm = x.bAutoLaestAlarm;
                bAutoPlay = x.bAutoPlay;
                aPushRecords = x.aPushRecords;
            }
            return *this;
        }
    } RealAlarmPushManager_S;

    /* 实时预警推送批量操作请求结构体 */
    typedef struct _RealAlarmPushBatchRequest_S_
    {
        RealAlarmPushOperationType_E enOperationType; /* 操作类型 */
        std::vector<std::string> aTaskIds;            /* 任务ID数组 */
        std::string strProcessRemark;                 /* 处理备注 */
        bool bAutoLaestAlarm;                         /* 是否自动更新最新报警 */
        bool bAutoPlay;                               /* 是否自动播放 */

        /* 默认构造函数 */
        _RealAlarmPushBatchRequest_S_() :
            enOperationType(PUSH_OP_PROCESS),
            strProcessRemark(),
            bAutoLaestAlarm(false),
            bAutoPlay(false)
        {
            aTaskIds.clear();
        }

        /* 重载赋值运算符 */
        _RealAlarmPushBatchRequest_S_ &operator=(const _RealAlarmPushBatchRequest_S_ &x)
        {
            if (this != &x)
            {
                enOperationType = x.enOperationType;
                aTaskIds = x.aTaskIds;
                strProcessRemark = x.strProcessRemark;
                bAutoLaestAlarm = x.bAutoLaestAlarm;
                bAutoPlay = x.bAutoPlay;
            }
            return *this;
        }
    } RealAlarmPushBatchRequest_S;

    /* 实时预警推送批量操作结果结构体 */
    typedef struct _RealAlarmPushBatchResult_S_
    {
        int nTotalCount;                          /* 处理总数 */
        int nSuccessCount;                        /* 成功数量 */
        int nFailureCount;                        /* 失败数量 */
        std::vector<std::string> aFailedTaskIds;  /* 失败的任务ID列表 */
        std::vector<std::string> aFailureReasons; /* 失败原因列表 */

        /* 默认构造函数 */
        _RealAlarmPushBatchResult_S_() :
            nTotalCount(0),
            nSuccessCount(0),
            nFailureCount(0)
        {
            aFailedTaskIds.clear();
            aFailureReasons.clear();
        }

        /* 重载赋值运算符 */
        _RealAlarmPushBatchResult_S_ &operator=(const _RealAlarmPushBatchResult_S_ &x)
        {
            if (this != &x)
            {
                nTotalCount = x.nTotalCount;
                nSuccessCount = x.nSuccessCount;
                nFailureCount = x.nFailureCount;
                aFailedTaskIds = x.aFailedTaskIds;
                aFailureReasons = x.aFailureReasons;
            }
            return *this;
        }
    } RealAlarmPushBatchResult_S;

    /************************ 实时预警推送相关 END ***********************/
#endif

#ifdef SCENE_INTELLIGENCE
    /**
     * @brief   : 场景智能
     */
    /************************翻阅围栏识别相关 START *******************/
    /* 翻越围栏规则参数 */
    typedef struct FenceClimbingRule
    {
        /* 区域定义 */
        Region_S stRegion;
        /* 灵敏度[1,100] */
        unsigned int nSensitivity;
        unsigned int nTimeThreshold; /* 行为事件触发时间阈值，判断有效报警的时间[0,100]单位（秒） */
        std::vector<int> aDetectionTarget;  /* 检测目标,DetectionTarget_E */
        /* 默认构造函数 */
        FenceClimbingRule() : stRegion(), nSensitivity(50)
        {
            aDetectionTarget.clear();
        }
        /* 重载赋值运算符 */
        FenceClimbingRule &operator=(const FenceClimbingRule &x)
        {
            if (this != &x)
            {
                stRegion = x.stRegion;
                nSensitivity = x.nSensitivity;
                aDetectionTarget = x.aDetectionTarget;
            }
            return *this;
        }
    } FenceClimbingRule_S;

    /* 翻越围栏配置 */
    typedef struct _FenceClimbingDetection_S_
    {
        /* 是否启用 */
        bool bEnable;
        /* 翻越围栏规则，最多设置4个 LOITERING_DETECT_REGION_DEFAULT*/
        std::vector<FenceClimbingRule_S> aRule;
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        DefenseTime aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        _FenceClimbingDetection_S_() : bEnable(false), stLinkageList()
        {
            aAlarmTime.clear();
            // aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            aRule.clear();
        }
        /* 重载赋值运算符 */
        _FenceClimbingDetection_S_ &operator=(const _FenceClimbingDetection_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aAlarmTime = x.aAlarmTime;
                aRule = x.aRule;
                stLinkageList = x.stLinkageList;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _FenceClimbingDetection_S_ CreateWithDefaultRule()
        {
            _FenceClimbingDetection_S_ obj;
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            for (size_t i = 0; i < LOITERING_DETECT_REGION_DEFAULT; ++i)
            {
                FenceClimbingRule_S defRegion;
                defRegion.stRegion.nPointNum = LOITERING_DETECT_REGION_DEFAULT;
                for (size_t j = 0; j < defRegion.stRegion.nPointNum; ++j)
                {
                    Common::PosF_S defPoint;
                    defRegion.stRegion.aPoint.push_back(defPoint);
                }
                obj.aRule.push_back(defRegion);
            }
            return obj;
        }
    } FenceClimbingDetection_S;
    /************************翻阅围栏识别相关 END *******************/

    /************************离岗识别相关 START *******************/
    /* 离岗规则参数 */
    typedef struct LeavePostRule
    {
        /* 区域定义 */
        Region_S stRegion;
        /* 灵敏度[1,100] */
        unsigned int nSensitivity;
        unsigned int nTimeThreshold; /* 行为事件触发时间阈值，判断有效报警的时间[0,100]单位（秒） */
        std::vector<int> aDetectionTarget;  /* 检测目标,DetectionTarget_E */
        /* 默认构造函数 */
        LeavePostRule() : stRegion(), nSensitivity(50),nTimeThreshold(10)
        {
            aDetectionTarget.clear();
        }
        /* 重载赋值运算符 */
        LeavePostRule &operator=(const LeavePostRule &x)
        {
            if (this != &x)
            {
                stRegion = x.stRegion;
                nSensitivity = x.nSensitivity;
                aDetectionTarget = x.aDetectionTarget;
                nTimeThreshold = x.nTimeThreshold;
            }
            return *this;
        }
    } LeavePostRule_S;

    /* 离岗配置 */
    typedef struct _LeavePostDetection_S_
    {
        /* 是否启用 */
        bool bEnable;
        /* 离岗规则，最多设置4个 LOITERING_DETECT_REGION_DEFAULT*/
        std::vector<LeavePostRule_S> aRule;
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        DefenseTime aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        _LeavePostDetection_S_() : bEnable(false), stLinkageList()
        {
            aAlarmTime.clear();
            aRule.clear();
        }
        /* 重载赋值运算符 */
        _LeavePostDetection_S_ &operator=(const _LeavePostDetection_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aAlarmTime = x.aAlarmTime;
                aRule = x.aRule;
                stLinkageList = x.stLinkageList;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _LeavePostDetection_S_ CreateWithDefaultRule()
        {
            _LeavePostDetection_S_ obj;
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            for (size_t i = 0; i < LOITERING_DETECT_REGION_DEFAULT; ++i)
            {
                LeavePostRule_S defRegion;
                defRegion.stRegion.nPointNum = LOITERING_DETECT_REGION_DEFAULT;
                for (size_t j = 0; j < defRegion.stRegion.nPointNum; ++j)
                {
                    Common::PosF_S defPoint;
                    defRegion.stRegion.aPoint.push_back(defPoint);
                }
                obj.aRule.push_back(defRegion);
            }
            return obj;
        }
    } LeavePostDetection_S;
    /************************离岗识别相关 END *******************/

    /************************行人闯入相关 START *******************/
    /* 行人闯入规则参数 */
    typedef struct PedestrianIntrusionRule
    {
        /* 区域定义 */
        Region_S stRegion;
        /* 灵敏度[1,100] */
        unsigned int nSensitivity;
        unsigned int nTimeThreshold; /* 行为事件触发时间阈值，判断有效报警的时间[0,100]单位（秒） */
        std::vector<int> aDetectionTarget;  /* 检测目标,DetectionTarget_E */
        /* 默认构造函数 */
        PedestrianIntrusionRule() : stRegion(), nSensitivity(50),nTimeThreshold(10)
        {
            aDetectionTarget.clear();
        }
        /* 重载赋值运算符 */
        PedestrianIntrusionRule &operator=(const PedestrianIntrusionRule &x)
        {
            if (this != &x)
            {
                stRegion = x.stRegion;
                nSensitivity = x.nSensitivity;
                aDetectionTarget = x.aDetectionTarget;
                nTimeThreshold = x.nTimeThreshold;
            }
            return *this;
        }
    } PedestrianIntrusionRule_S;

    /* 行人闯入配置 */
    typedef struct _PedestrianIntrusionDetection_S_
    {
        /* 是否启用 */
        bool bEnable;
        /* 离岗规则，最多设置4个 LOITERING_DETECT_REGION_DEFAULT*/
        std::vector<PedestrianIntrusionRule_S> aRule;
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        DefenseTime aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        _PedestrianIntrusionDetection_S_() : bEnable(false), stLinkageList()
        {
            aAlarmTime.clear();
            aRule.clear();
        }
        /* 重载赋值运算符 */
        _PedestrianIntrusionDetection_S_ &operator=(const _PedestrianIntrusionDetection_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aAlarmTime = x.aAlarmTime;
                aRule = x.aRule;
                stLinkageList = x.stLinkageList;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _PedestrianIntrusionDetection_S_ CreateWithDefaultRule()
        {
            _PedestrianIntrusionDetection_S_ obj;
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            for (size_t i = 0; i < LOITERING_DETECT_REGION_DEFAULT; ++i)
            {
                PedestrianIntrusionRule_S defRegion;
                defRegion.stRegion.nPointNum = LOITERING_DETECT_REGION_DEFAULT;
                for (size_t j = 0; j < defRegion.stRegion.nPointNum; ++j)
                {
                    Common::PosF_S defPoint;
                    defRegion.stRegion.aPoint.push_back(defPoint);
                }
                obj.aRule.push_back(defRegion);
            }
            return obj;
        }
    } PedestrianIntrusionDetection_S;
    /************************行人闯入相关 END *******************/

    /************************烟火检测相关 START *******************/
    /* 烟火规则参数 */
    typedef struct SmokeFireRule
    {
        /* 灵敏度[1,100] */
        unsigned int nSensitivity = 50;
        /* 默认构造函数 */
        SmokeFireRule() : nSensitivity(50)
        {

        }
        /* 重载赋值运算符 */
        SmokeFireRule &operator=(const SmokeFireRule &x)
        {
            if (this != &x)
            {
                nSensitivity = x.nSensitivity;
            }
            return *this;
        }
    } SmokeFireRule_S;

    /*  烟火检测配置 */
    typedef struct _SmokeFireDetection_S_
    {
        /* 是否启用 */
        bool bEnable;
        /* 烟火规则 */
        SmokeFireRule_S stRule;
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        DefenseTime aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        _SmokeFireDetection_S_() : bEnable(false), stLinkageList()
        {
            aAlarmTime.clear();
            // aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
        }
        /* 重载赋值运算符 */
        _SmokeFireDetection_S_ &operator=(const _SmokeFireDetection_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aAlarmTime = x.aAlarmTime;
                stRule = x.stRule;
                stLinkageList = x.stLinkageList;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _SmokeFireDetection_S_ CreateWithDefaultRule()
        {
            _SmokeFireDetection_S_ obj;
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }

    } SmokeFireDetection_S;
    /************************烟火检测相关 END *******************/

     /************************明火检测相关 START *******************/
    /* 明火规则参数 */
    typedef struct OpenFlameRule
    {
        /* 灵敏度[1,100] */
        unsigned int nSensitivity;
        /* 默认构造函数 */
        OpenFlameRule() : nSensitivity(50)
        {

        }
        /* 重载赋值运算符 */
        OpenFlameRule &operator=(const OpenFlameRule &x)
        {
            if (this != &x)
            {
                nSensitivity = x.nSensitivity;
            }
            return *this;
        }
    } OpenFlameRule_S;

    /*  明火检测配置 */
    typedef struct _OpenFlameDetection_S_
    {
        /* 是否启用 */
        bool bEnable;
        /* 烟火规则 */
        OpenFlameRule_S stRule;
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        DefenseTime aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        _OpenFlameDetection_S_() : bEnable(false), stLinkageList()
        {
            aAlarmTime.clear();
            // aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
        }
        /* 重载赋值运算符 */
        _OpenFlameDetection_S_ &operator=(const _OpenFlameDetection_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aAlarmTime = x.aAlarmTime;
                stLinkageList = x.stLinkageList;
                stRule = x.stRule;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _OpenFlameDetection_S_ CreateWithDefaultRule()
        {
            _OpenFlameDetection_S_ obj;
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }

    } OpenFlameDetection_S;
    /************************明火检测相关 END *******************/

    /************************道路积水检测相关 START *******************/
    /* 道路积水规则参数 */
    typedef struct RoadPondingRule
    {
        /* 灵敏度[1,100] */
        unsigned int nSensitivity;
        /* 默认构造函数 */
        RoadPondingRule() : nSensitivity(50)
        {

        }
        /* 重载赋值运算符 */
        RoadPondingRule &operator=(const RoadPondingRule &x)
        {
            if (this != &x)
            {
                nSensitivity = x.nSensitivity;
            }
            return *this;
        }
    } RoadPondingRule_S;

    /*  道路积水检测配置 */
    typedef struct _RoadPondingDetection_S_
    {
        /* 是否启用 */
        bool bEnable;
        /* 道路积水规则 */
        RoadPondingRule_S stRule;
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        DefenseTime aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        _RoadPondingDetection_S_() : bEnable(false), stLinkageList()
        {
            aAlarmTime.clear();
            // aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
        }
        /* 重载赋值运算符 */
        _RoadPondingDetection_S_ &operator=(const _RoadPondingDetection_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aAlarmTime = x.aAlarmTime;
                stLinkageList = x.stLinkageList;
                stRule = x.stRule;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _RoadPondingDetection_S_ CreateWithDefaultRule()
        {
            _RoadPondingDetection_S_ obj;
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }

    } RoadPondingDetection_S;
    /************************道路积水检测相关 END *******************/

    /************************井盖异常检测相关 START *******************/
    /* 井盖异常规则参数 */
    typedef struct ManholeCoverAbnormalRule
    {
        /* 灵敏度[1,100] */
        unsigned int nSensitivity;
        /* 默认构造函数 */
        ManholeCoverAbnormalRule() : nSensitivity(50)
        {

        }
        /* 重载赋值运算符 */
        ManholeCoverAbnormalRule &operator=(const ManholeCoverAbnormalRule &x)
        {
            if (this != &x)
            {
                nSensitivity = x.nSensitivity;
            }
            return *this;
        }
    } ManholeCoverAbnormalRule_S;

    /*  井盖异常检测配置 */
    typedef struct _ManholeCoverAbnormalDetection_S_
    {
        /* 是否启用 */
        bool bEnable;
        /* 井盖异常规则 */
        ManholeCoverAbnormalRule_S stRule;
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        DefenseTime aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        _ManholeCoverAbnormalDetection_S_() : bEnable(false), stLinkageList()
        {
            aAlarmTime.clear();
            // aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
        }
        /* 重载赋值运算符 */
        _ManholeCoverAbnormalDetection_S_ &operator=(const _ManholeCoverAbnormalDetection_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aAlarmTime = x.aAlarmTime;
                stLinkageList = x.stLinkageList;
                stRule = x.stRule;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _ManholeCoverAbnormalDetection_S_ CreateWithDefaultRule()
        {
            _ManholeCoverAbnormalDetection_S_ obj;
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }

    } ManholeCoverAbnormalDetection_S;
    /************************井盖异常检测相关 END *******************/

    /************************睡岗识别相关 START *******************/
    /* 睡岗识别检测规则参数 */
    typedef struct SleepOnDutyRule
    {
        /* 灵敏度[1,100] */
        unsigned int nSensitivity;
        /* 默认构造函数 */
        SleepOnDutyRule() : nSensitivity(50)
        {

        }
        /* 重载赋值运算符 */
        SleepOnDutyRule &operator=(const SleepOnDutyRule &x)
        {
            if (this != &x)
            {
                nSensitivity = x.nSensitivity;
            }
            return *this;
        }
    } SleepOnDutyRule_S;

    /*  睡岗识别检测配置 */
    typedef struct _SleepOnDutyDetection_S_
    {
        /* 是否启用 */
        bool bEnable;
        /* 睡岗识别规则 */
        SleepOnDutyRule_S stRule;
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        DefenseTime aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        _SleepOnDutyDetection_S_() : bEnable(false), stLinkageList()
        {
            aAlarmTime.clear();
            // aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
        }
        /* 重载赋值运算符 */
        _SleepOnDutyDetection_S_ &operator=(const _SleepOnDutyDetection_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aAlarmTime = x.aAlarmTime;
                stLinkageList = x.stLinkageList;
                stRule = x.stRule;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _SleepOnDutyDetection_S_ CreateWithDefaultRule()
        {
            _SleepOnDutyDetection_S_ obj;
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }

    } SleepOnDutyDetection_S;
    /***********************睡岗识别相关 END *******************/

    /************************摔倒识别相关 START *******************/
    /* 摔倒识别检测规则参数 */
    typedef struct TripRule
    {
        /* 灵敏度[1,100] */
        unsigned int nSensitivity;
        /* 默认构造函数 */
        TripRule() : nSensitivity(50)
        {

        }
        /* 重载赋值运算符 */
        TripRule &operator=(const TripRule &x)
        {
            if (this != &x)
            {
                nSensitivity = x.nSensitivity;
            }
            return *this;
        }
    } TripRule_S;

    /*  摔倒识别检测配置 */
    typedef struct _TripDetection_S_
    {
        /* 是否启用 */
        bool bEnable;
        /* 摔倒识别规则 */
        TripRule_S stRule;
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        DefenseTime aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        _TripDetection_S_() : bEnable(false), stLinkageList()
        {
            aAlarmTime.clear();
            // aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
        }
        /* 重载赋值运算符 */
        _TripDetection_S_ &operator=(const _TripDetection_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aAlarmTime = x.aAlarmTime;
                stLinkageList = x.stLinkageList;
                stRule = x.stRule;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _TripDetection_S_ CreateWithDefaultRule()
        {
            _TripDetection_S_ obj;
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }

    } TripDetection_S;
    /***********************摔倒识别相关 END *******************/

    /************************玩手机识别相关 START *******************/
    /*玩手机识别检测规则参数 */
    typedef struct PhoneUsageRule
    {
        /* 灵敏度[1,100] */
        unsigned int nSensitivity;
        /* 默认构造函数 */
        PhoneUsageRule() : nSensitivity(50)
        {

        }
        /* 重载赋值运算符 */
        PhoneUsageRule &operator=(const PhoneUsageRule &x)
        {
            if (this != &x)
            {
                nSensitivity = x.nSensitivity;
            }
            return *this;
        }
    } PhoneUsageRule_S;

    /* 玩手机识别检测配置 */
    typedef struct _PhoneUsageDetection_S_
    {
        /* 是否启用 */
        bool bEnable;
        /* 玩手机识别规则 */
        PhoneUsageRule_S stRule;
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        DefenseTime aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        _PhoneUsageDetection_S_() : bEnable(false), stLinkageList()
        {
            aAlarmTime.clear();
            // aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
        }
        /* 重载赋值运算符 */
        _PhoneUsageDetection_S_ &operator=(const _PhoneUsageDetection_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aAlarmTime = x.aAlarmTime;
                stLinkageList = x.stLinkageList;
                stRule = x.stRule;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _PhoneUsageDetection_S_ CreateWithDefaultRule()
        {
            _PhoneUsageDetection_S_ obj;
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }

    } PhoneUsageDetection_S;
    /***********************玩手机识别相关 END *******************/

    /************************倒地识别相关 START *******************/
    /*倒地识别检测规则参数 */
    typedef struct PersonFallDownRule
    {
        /* 灵敏度[1,100] */
        unsigned int nSensitivity;
        /* 默认构造函数 */
        PersonFallDownRule() : nSensitivity(50)
        {

        }
        /* 重载赋值运算符 */
        PersonFallDownRule &operator=(const PersonFallDownRule &x)
        {
            if (this != &x)
            {
                nSensitivity = x.nSensitivity;
            }
            return *this;
        }
    } PersonFallDownRule_S;

    /*倒地识别检测配置 */
    typedef struct _PersonFallDownDetection_S_
    {
        /* 是否启用 */
        bool bEnable;
        /* 倒地识别规则 */
        PersonFallDownRule_S stRule;
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        DefenseTime aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        _PersonFallDownDetection_S_() : bEnable(false), stLinkageList()
        {
            aAlarmTime.clear();
            // aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
        }
        /* 重载赋值运算符 */
        _PersonFallDownDetection_S_ &operator=(const _PersonFallDownDetection_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aAlarmTime = x.aAlarmTime;
                stLinkageList = x.stLinkageList;
                stRule = x.stRule;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _PersonFallDownDetection_S_ CreateWithDefaultRule()
        {
            _PersonFallDownDetection_S_ obj;
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }

    } PersonFallDownDetection_S;
    /***********************倒地识别相关 END *******************/

     /************************ 施工占道检测相关 START *******************/
    /* 施工占道检测规则参数 */
    typedef struct ConstructionEncroachmentRoadRule
    {
        /* 灵敏度[1,100] */
        unsigned int nSensitivity;
        /* 默认构造函数 */
        ConstructionEncroachmentRoadRule() : nSensitivity(50)
        {

        }
        /* 重载赋值运算符 */
        ConstructionEncroachmentRoadRule &operator=(const ConstructionEncroachmentRoadRule &x)
        {
            if (this != &x)
            {
                nSensitivity = x.nSensitivity;
            }
            return *this;
        }

    } ConstructionEncroachmentRoadRule_S;

     /*  施工占道检测检测配置 */
    typedef struct _ConstructionEncroachmentRoadDetection_S_
    {
        /* 是否启用 */
        bool bEnable;
        /* 施工占道检测规则 */
        ConstructionEncroachmentRoadRule_S stRule;

        /* 布防时间:一周7天，每天可以设置8个时间段 */
        DefenseTime aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */

        _ConstructionEncroachmentRoadDetection_S_() : bEnable(false), stLinkageList()
        {
            aAlarmTime.clear();
            // aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
        }
        /* 重载赋值运算符 */
        _ConstructionEncroachmentRoadDetection_S_ &operator=(const _ConstructionEncroachmentRoadDetection_S_ &x)

        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aAlarmTime = x.aAlarmTime;
                stLinkageList = x.stLinkageList;
                stRule = x.stRule;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _ConstructionEncroachmentRoadDetection_S_ CreateWithDefaultRule()
        {
            _ConstructionEncroachmentRoadDetection_S_ obj;
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }

    } ConstructionEncroachmentRoadDetection_S;
     /************************ 施工占道检测相关 END *******************/

    /************************高空安全带检测相关 START *******************/
    /*高空安全带检测规则参数 */
    typedef struct HighAltitudeSeatbeltRule
    {
        /* 灵敏度[1,100] */
        unsigned int nSensitivity;
        /* 默认构造函数 */
        HighAltitudeSeatbeltRule() : nSensitivity(50)
        {

        }
        /* 重载赋值运算符 */
        HighAltitudeSeatbeltRule &operator=(const HighAltitudeSeatbeltRule &x)
        {
            if (this != &x)
            {
                nSensitivity = x.nSensitivity;
            }
            return *this;
        }

    } HighAltitudeSeatbeltRule_S;

    /*高空安全带检测配置 */
    typedef struct _HighAltitudeSeatbeltDetection_S_
    {
        /* 是否启用 */
        bool bEnable;
        /* 高空安全带检测规则 */
        HighAltitudeSeatbeltRule_S stRule;
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        DefenseTime aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        _HighAltitudeSeatbeltDetection_S_() : bEnable(false), stLinkageList()
        {
            aAlarmTime.clear();
            // aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
        }
        /* 重载赋值运算符 */

        _HighAltitudeSeatbeltDetection_S_ &operator=(const _HighAltitudeSeatbeltDetection_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aAlarmTime = x.aAlarmTime;
                stLinkageList = x.stLinkageList;
                stRule = x.stRule;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _HighAltitudeSeatbeltDetection_S_ CreateWithDefaultRule()
        {
            _HighAltitudeSeatbeltDetection_S_ obj;
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }

    } HighAltitudeSeatbeltDetection_S;
    /***********************高空安全带检测相关 END *******************/

    /************************黄土裸露检测相关 START *******************/
    /*黄土裸露检测规则参数 */
    typedef struct BareSoilRule
    {
        /* 灵敏度[1,100] */
        unsigned int nSensitivity;
        /* 默认构造函数 */
        BareSoilRule() : nSensitivity(50)
        {

        }
        /* 重载赋值运算符 */
        BareSoilRule &operator=(const BareSoilRule &x)
        {
            if (this != &x)
            {
                nSensitivity = x.nSensitivity;
            }
            return *this;
        }
    } BareSoilRule_S;

    /*黄土裸露检测配置 */
    typedef struct _BareSoiletDection_S_
    {
        /* 是否启用 */
        bool bEnable;
        /* 黄土裸露检测规则 */
        BareSoilRule_S stRule;
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        DefenseTime aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        _BareSoiletDection_S_() : bEnable(false), stLinkageList()
        {
            aAlarmTime.clear();
            // aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
        }
        /* 重载赋值运算符 */
        _BareSoiletDection_S_ &operator=(const _BareSoiletDection_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aAlarmTime = x.aAlarmTime;
                stLinkageList = x.stLinkageList;
                stRule = x.stRule;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _BareSoiletDection_S_ CreateWithDefaultRule()
        {
            _BareSoiletDection_S_ obj;
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }

    } BareSoiletDection_S;
    /***********************黄土裸露检测相关 END *******************/

    /************************安全帽检测相关 START *******************/
    /*安全帽检测规则参数 */
    typedef struct SafetyHelmetRule
    {
        /* 灵敏度[1,100] */
        unsigned int nSensitivity;
        /* 默认构造函数 */
        SafetyHelmetRule() : nSensitivity(50)
        {

        }
        /* 重载赋值运算符 */
        SafetyHelmetRule &operator=(const SafetyHelmetRule &x)
        {
            if (this != &x)
            {
                nSensitivity = x.nSensitivity;
            }
            return *this;
        }
    } SafetyHelmetRule_S;

    /*安全帽检测配置 */
    typedef struct _SafetyHelmetDection_S_
    {
        /* 是否启用 */
        bool bEnable;
        /*安全帽检测规则 */
        SafetyHelmetRule_S stRule;
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        DefenseTime aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        _SafetyHelmetDection_S_() : bEnable(false), stLinkageList()
        {
            aAlarmTime.clear();
            // aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
        }
        /* 重载赋值运算符 */
        _SafetyHelmetDection_S_ &operator=(const _SafetyHelmetDection_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aAlarmTime = x.aAlarmTime;
                stLinkageList = x.stLinkageList;
                stRule = x.stRule;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _SafetyHelmetDection_S_ CreateWithDefaultRule()
        {
            _SafetyHelmetDection_S_ obj;
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }

    } SafetyHelmetDection_S;
    /***********************安全帽检测相关 END *******************/

    /************************洞口防护栏检测相关 START *******************/
    /*洞口防护栏检测规则参数 */
    typedef struct HoleProtectionBarRule
    {
        /* 灵敏度[1,100] */
        unsigned int nSensitivity;
        /* 默认构造函数 */
        HoleProtectionBarRule() : nSensitivity(50)
        {

        }
        /* 重载赋值运算符 */
        HoleProtectionBarRule &operator=(const HoleProtectionBarRule &x)
        {
            if (this != &x)
            {
                nSensitivity = x.nSensitivity;
            }
            return *this;
        }
    } HoleProtectionBarRule_S;

    /*洞口防护栏检测配置 */
    typedef struct _HoleProtectionBarDection_S_
    {
        /* 是否启用 */
        bool bEnable;
        /*洞口防护栏检测规则 */
        HoleProtectionBarRule_S stRule;
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        DefenseTime aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        _HoleProtectionBarDection_S_() : bEnable(false), stLinkageList()
        {
            aAlarmTime.clear();
            // aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
        }
        /* 重载赋值运算符 */
        _HoleProtectionBarDection_S_ &operator=(const _HoleProtectionBarDection_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aAlarmTime = x.aAlarmTime;
                stLinkageList = x.stLinkageList;
                stRule = x.stRule;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _HoleProtectionBarDection_S_ CreateWithDefaultRule()
        {
            _HoleProtectionBarDection_S_ obj;
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }

    } HoleProtectionBarDection_S;
    /***********************洞口防护栏检测相关 END *******************/

    /************************反光衣检测相关 START *******************/
    /*反光衣检测规则参数 */
    typedef struct ReflectiveClothingRule
    {
        /* 灵敏度[1,100] */
        unsigned int nSensitivity;
        /* 默认构造函数 */
        ReflectiveClothingRule() : nSensitivity(50)
        {

        }
        /* 重载赋值运算符 */
        ReflectiveClothingRule &operator=(const ReflectiveClothingRule &x)
        {
            if (this != &x)
            {
                nSensitivity = x.nSensitivity;
            }
            return *this;
        }
    } ReflectiveClothingRule_S;

    /*反光衣检测配置 */
    typedef struct _ReflectiveClothingDection_S_
    {
        /* 是否启用 */
        bool bEnable;
        /*反光衣检测规则 */
        ReflectiveClothingRule_S stRule;
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        DefenseTime aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        _ReflectiveClothingDection_S_() : bEnable(false), stLinkageList()
        {
            aAlarmTime.clear();
            // aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
        }
        /* 重载赋值运算符 */
        _ReflectiveClothingDection_S_ &operator=(const _ReflectiveClothingDection_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aAlarmTime = x.aAlarmTime;
                stLinkageList = x.stLinkageList;
                stRule = x.stRule;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _ReflectiveClothingDection_S_ CreateWithDefaultRule()
        {
            _ReflectiveClothingDection_S_ obj;
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }

    } ReflectiveClothingDection_S;
    /***********************反光衣检测相关 END *******************/

    /************************抽烟识别相关 START *******************/
    /*抽烟识别规则参数 */
    typedef struct SmokingRule
    {
        /* 灵敏度[1,100] */
        unsigned int nSensitivity;
        /* 默认构造函数 */
        SmokingRule() : nSensitivity(50)
        {

        }
        /* 重载赋值运算符 */
        SmokingRule &operator=(const SmokingRule &x)
        {
            if (this != &x)
            {
                nSensitivity = x.nSensitivity;
            }
            return *this;
        }
    } SmokingRule_S;

    /*抽烟识别配置 */
    typedef struct _SmokingDection_S_
    {
        /* 是否启用 */
        bool bEnable;
        /*抽烟识别规则 */
        SmokingRule_S stRule;
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        DefenseTime aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        _SmokingDection_S_() : bEnable(false), stLinkageList()
        {
            aAlarmTime.clear();
            // aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
        }
        /* 重载赋值运算符 */
        _SmokingDection_S_ &operator=(const _SmokingDection_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aAlarmTime = x.aAlarmTime;
                stLinkageList = x.stLinkageList;
                stRule = x.stRule;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _SmokingDection_S_ CreateWithDefaultRule()
        {
            _SmokingDection_S_ obj;
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }

    } SmokingDection_S;
    /**********************抽烟识别相关 END *******************/

    /************************电瓶车识别相关 START *******************/
    /* 电瓶车识别检测规则参数 */
    typedef struct ElectricScooterRule
    {
        /* 灵敏度[1,100] */
        unsigned int nSensitivity;
        /* 默认构造函数 */
        ElectricScooterRule() : nSensitivity(50)
        {

        }
        /* 重载赋值运算符 */
        ElectricScooterRule &operator=(const ElectricScooterRule &x)
        {
            if (this != &x)
            {
                nSensitivity = x.nSensitivity;
            }
            return *this;
        }
    } ElectricScooterRule_S;

    /*  电瓶车识别检测配置 */
    typedef struct _ElectricScooterDetection_S_
    {
        /* 是否启用 */
        bool bEnable;
        /* 电瓶车识别规则 */
        ElectricScooterRule_S stRule;
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        DefenseTime aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        _ElectricScooterDetection_S_() : bEnable(false), stLinkageList()
        {
            aAlarmTime.clear();
            // aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
        }
        /* 重载赋值运算符 */
        _ElectricScooterDetection_S_ &operator=(const _ElectricScooterDetection_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aAlarmTime = x.aAlarmTime;
                stLinkageList = x.stLinkageList;
                stRule = x.stRule;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _ElectricScooterDetection_S_ CreateWithDefaultRule()
        {
            _ElectricScooterDetection_S_ obj;
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }

    } ElectricScooterDetection_S;
    /***********************电瓶车识别相关 END *******************/

    /************************ 车牌检测识别相关 START *******************/
    /* 车牌检测识别检测规则参数 */
    typedef struct LicensePlateCognitionRule
    {
        /* 灵敏度[1,100] */
        unsigned int nSensitivity;
        /* 默认构造函数 */
        LicensePlateCognitionRule() : nSensitivity(50)
        {

        }
        /* 重载赋值运算符 */
        LicensePlateCognitionRule &operator=(const LicensePlateCognitionRule &x)
        {
            if (this != &x)
            {
                nSensitivity = x.nSensitivity;
            }
            return *this;
        }
    } LicensePlateCognitionRule_S;

    /*  车牌检测识别检测配置 */
    typedef struct _LicensePlateCognitionDetection_S_
    {
        /* 是否启用 */
        bool bEnable;
        /* 车牌检测识别规则 */
        LicensePlateCognitionRule_S stRule;
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        DefenseTime aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        _LicensePlateCognitionDetection_S_() : bEnable(false), stLinkageList()
        {
            aAlarmTime.clear();
            // aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
        }
        /* 重载赋值运算符 */
        _LicensePlateCognitionDetection_S_ &operator=(const _LicensePlateCognitionDetection_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aAlarmTime = x.aAlarmTime;
                stLinkageList = x.stLinkageList;
                stRule = x.stRule;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _LicensePlateCognitionDetection_S_ CreateWithDefaultRule()
        {
            _LicensePlateCognitionDetection_S_ obj;
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }

    } LicensePlateCognitionDetection_S;
    /***********************车牌检测识别相关 END *******************/

    /************************ 逆行识别相关 ************************/
    typedef struct DrivingAgainstTrafficRule
    {
        Common::PosF_S stStartPos;     /* 警戒线的起始点 */
        Common::PosF_S stEndPos;       /* 警戒线的终止点 */
        CrossDirection_E enCrossDirection; /* 警戒线的穿越方向[0-双向,1-由左至右,2-由右至左] */
        unsigned int nSensitivity;     /* 警戒线灵敏度[1,100] */
        // std::vector<int> aDetectionTarget;  /* 检测目标,DetectionTarget_E */

        /* 重载默认构造函数 */
        DrivingAgainstTrafficRule()
            : stStartPos(),
            stEndPos(),
            enCrossDirection(A_TO_B),
            nSensitivity(50)
        {
            // aDetectionTarget.clear();
        }
        /* 重载赋值运算符 */
        DrivingAgainstTrafficRule &operator=(const DrivingAgainstTrafficRule &x)
        {
            if (this != &x)
            {
                stStartPos = x.stStartPos;
                stEndPos = x.stEndPos;
                enCrossDirection = x.enCrossDirection;
                nSensitivity = x.nSensitivity;
                // aDetectionTarget = x.aDetectionTarget;
            }
            return *this;
        }
    } DrivingAgainstTrafficRule_S;

    /// @brief 逆行检测报警
    typedef struct _DrivingAgainstTrafficDetection_S_
    {
        bool bEnable;                     /* 是否启用 0-不启用 1-启用 */
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        std::vector<std::vector<Common::SchedTime_S>> aAlarmTime;
        /* 逆行检测区域,最多设置4个-TRAVERSE_DETECT_REGION_DEFAULT */
        std::vector<DrivingAgainstTrafficRule_S> aRule;
        /* NOTE IPC自身网页没有假日布防时间设置，暂不对接 */
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        _DrivingAgainstTrafficDetection_S_()
            : bEnable(false)
        {
            aAlarmTime.clear();
            aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            aRule.clear();
        }
        /* 重载赋值运算符 */
        _DrivingAgainstTrafficDetection_S_ &operator=(const _DrivingAgainstTrafficDetection_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aAlarmTime = x.aAlarmTime;
                aRule = x.aRule;
                stLinkageList = x.stLinkageList;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _DrivingAgainstTrafficDetection_S_ CreateWithDefaultRule()
        {
            _DrivingAgainstTrafficDetection_S_ obj;
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            for(int i = 0;i < DRIVING_AGAINST_TRAFFIC_DETECT_REGION_DEFAULT;i++)
            {
                DrivingAgainstTrafficRule_S defRegion;

                    Common::PosF_S defPoint{0.0f, 0.0f};

                /* 确保其他字段也有默认值 */
                defRegion.stStartPos = {0.0f, 0.0f};
                defRegion.stEndPos = {0.0f, 0.0f};
                defRegion.nSensitivity = 50;
                // defRegion.aDetectionTarget.clear();  /* 空数组但需要显示结构 */
                defRegion.enCrossDirection = A_TO_B;
                obj.aRule.push_back(defRegion);
            }
            return obj;
        }

    } DrivingAgainstTrafficDetection_S;

    /************************ 逆行识别相关 END ************************/

    /************************ 违规变道侦测相关 ************************/
    /* 违规变道侦测规则参数 */
    typedef struct IllegalLaneChangeRule
    {
        Common::PosF_S stStartPos;     /* 警戒线的起始点 */
        Common::PosF_S stEndPos;       /* 警戒线的终止点 */
        CrossDirection_E enCrossDirection; /* 警戒线的穿越方向[0-双向,1-由左至右,2-由右至左] */
        unsigned int nSensitivity;     /* 警戒线灵敏度[1,100] */
        // std::vector<int> aDetectionTarget;  /* 检测目标,DetectionTarget_E */

        /* 重载默认构造函数 */
        IllegalLaneChangeRule()
            : stStartPos(),
            stEndPos(),
            enCrossDirection(A_TO_B),
            nSensitivity(50)
        {
            // aDetectionTarget.clear();
        }
        /* 重载赋值运算符 */
        IllegalLaneChangeRule &operator=(const IllegalLaneChangeRule &x)
        {
            if (this != &x)
            {
                stStartPos = x.stStartPos;
                stEndPos = x.stEndPos;
                enCrossDirection = x.enCrossDirection;
                nSensitivity = x.nSensitivity;
                // aDetectionTarget = x.aDetectionTarget;
            }
            return *this;
        }
    } IllegalLaneChangeRule_S;

    /// @brief 违规变道检测报警
    typedef struct _IllegalLaneChangeDetection_S_
    {
        bool bEnable;                     /* 是否启用 0-不启用 1-启用 */
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        std::vector<std::vector<Common::SchedTime_S>> aAlarmTime;
        /* 违规变道检测区域,最多设置4个-TRAVERSE_DETECT_REGION_DEFAULT */
        std::vector<IllegalLaneChangeRule_S> aRule;
        /* NOTE IPC自身网页没有假日布防时间设置，暂不对接 */
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        _IllegalLaneChangeDetection_S_()
            : bEnable(false)
        {
            aAlarmTime.clear();
            aRule.clear();
        }
        /* 重载赋值运算符 */
        _IllegalLaneChangeDetection_S_ &operator=(const _IllegalLaneChangeDetection_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aAlarmTime = x.aAlarmTime;
                aRule = x.aRule;
                stLinkageList = x.stLinkageList;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _IllegalLaneChangeDetection_S_ CreateWithDefaultRule()
        {
            _IllegalLaneChangeDetection_S_ obj;
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            for(int i = 0;i < ILLEGAL_LANE_CHANGE_DETECT_REGION_DEFAULT;i++)
            {
                IllegalLaneChangeRule_S defRegion;

                    Common::PosF_S defPoint{0.0f, 0.0f};

                /* 确保其他字段也有默认值 */
                defRegion.stStartPos = {0.0f, 0.0f};
                defRegion.stEndPos = {0.0f, 0.0f};
                defRegion.nSensitivity = 50;
                // defRegion.aDetectionTarget.clear();  /* 空数组但需要显示结构 */
                defRegion.enCrossDirection = A_TO_B;
                obj.aRule.push_back(defRegion);
            }
            return obj;
        }

    } IllegalLaneChangeDetection_S;
    /************************ 违规变道侦测相关 END ************************/

    /************************ 拥堵识别相关 START *******************/
    /* 拥堵识别检测规则参数 */
    typedef struct CongestionRule
    {
        /* 灵敏度[1,100] */
        unsigned int nSensitivity;
        /* 默认构造函数 */
        CongestionRule() : nSensitivity(50)
        {

        }
        /* 重载赋值运算符 */
        CongestionRule &operator=(const CongestionRule &x)
        {
            if (this != &x)
            {
                nSensitivity = x.nSensitivity;
            }
            return *this;
        }
    } CongestionRule_S;

    /*  拥堵识别检测配置 */
    typedef struct _CongestionDetection_S_
    {
        /* 是否启用 */
        bool bEnable;
        /* 拥堵识别规则 */
        CongestionRule_S stRule;
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        DefenseTime aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        _CongestionDetection_S_() : bEnable(false), stLinkageList()
        {
            aAlarmTime.clear();
            // aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
        }
        /* 重载赋值运算符 */
        _CongestionDetection_S_ &operator=(const _CongestionDetection_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aAlarmTime = x.aAlarmTime;
                stLinkageList = x.stLinkageList;
                stRule = x.stRule;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _CongestionDetection_S_ CreateWithDefaultRule()
        {
            _CongestionDetection_S_ obj;
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }

    } CongestionDetection_S;
    /*********************** 拥堵识别相关 END *******************/

    /************************ 应急车道占用侦测相关 ************************/
    /* 应急车道占用侦测规则参数 */
    typedef struct _EmergencyLaneOccupancyRule_S_
    {
        Region_S stRegion;           /* 区域定义 */
        unsigned int nSensitivity;   /* 灵敏度[1,100] */
        unsigned int nTimeThreshold; /* 行为事件触发时间阈值，判断有效报警的时间[0,100]单位（秒） */
        std::vector<int> aDetectionTarget; /* 检测目标,DetectionTarget_E */
        /* 默认构造函数 */
        _EmergencyLaneOccupancyRule_S_() : stRegion(), nSensitivity(50), nTimeThreshold(10)
        {
            aDetectionTarget.clear();
        }
        /* 重载赋值运算符 */
        _EmergencyLaneOccupancyRule_S_ &operator=(const _EmergencyLaneOccupancyRule_S_ &x)
        {
            if (this != &x)
            {
                stRegion = x.stRegion;
                nSensitivity = x.nSensitivity;
                nTimeThreshold = x.nTimeThreshold;
                aDetectionTarget = x.aDetectionTarget;
            }
            return *this;
        }
    } EmergencyLaneOccupancyRule_S;

    /* 应急车道占用侦测配置 */
    typedef struct EmergencyLaneOccupancyDetection
    {
        bool bEnable;    /* 是否启用 0-不启用 1-启用 */
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        std::vector<std::vector<Common::SchedTime_S>> aAlarmTime;
        /* 应急车道占用侦测规则，最多设置8个-REGION_MAX */
        std::vector<EmergencyLaneOccupancyRule_S> aRule;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        EmergencyLaneOccupancyDetection() : bEnable(false), stLinkageList()
        {
            aAlarmTime.clear();
            // aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            aRule.clear();
        }

        /* 重载赋值运算符 */
        EmergencyLaneOccupancyDetection &operator=(const EmergencyLaneOccupancyDetection &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aAlarmTime = x.aAlarmTime;
                aRule = x.aRule;
                stLinkageList = x.stLinkageList;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static EmergencyLaneOccupancyDetection CreateWithDefaultRule()
        {
            EmergencyLaneOccupancyDetection obj;
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            for (size_t i = 0; i < EMERGENCY_LANE_OCCUPANCY_DETECT_REGION_DEFAULT; ++i)
            {
                EmergencyLaneOccupancyRule_S defRegion;
                defRegion.stRegion.nPointNum = EMERGENCY_LANE_OCCUPANCY_DETECT_REGION_DEFAULT;
                for (size_t j = 0; j < defRegion.stRegion.nPointNum; ++j)
                {
                    Common::PosF_S defPoint;
                    defRegion.stRegion.aPoint.push_back(defPoint);
                }
                defRegion.nSensitivity = 50;
                defRegion.aDetectionTarget.clear(); /* 空数组但需要显示结构 */
                obj.aRule.push_back(defRegion);
            }
            return obj;
        }
    } EmergencyLaneOccupancyDetection_S;
    /************************ 应急车道占用侦测相关 END ************************/

    /************************ 非机动车闯入相关 START *******************/
    /* 非机动车闯入规则参数 */
    typedef struct NonMotorVehicleIntrusionRule
    {
        /* 区域定义 */
        Region_S stRegion;
        /* 灵敏度[1,100] */
        unsigned int nSensitivity;
        unsigned int nTimeThreshold; /* 行为事件触发时间阈值，判断有效报警的时间[0,100]单位（秒） */
        std::vector<int> aDetectionTarget;  /* 检测目标,DetectionTarget_E */
        /* 默认构造函数 */
        NonMotorVehicleIntrusionRule() : stRegion(), nSensitivity(50),nTimeThreshold(10)
        {
            aDetectionTarget.clear();
        }
        /* 重载赋值运算符 */
        NonMotorVehicleIntrusionRule &operator=(const NonMotorVehicleIntrusionRule &x)
        {
            if (this != &x)
            {
                stRegion = x.stRegion;
                nSensitivity = x.nSensitivity;
                aDetectionTarget = x.aDetectionTarget;
                nTimeThreshold = x.nTimeThreshold;
            }
            return *this;
        }
    } NonMotorVehicleIntrusionRule_S;

    /* 非机动车闯入配置 */
    typedef struct _NonMotorVehicleIntrusionDetection_S_
    {
        /* 是否启用 */
        bool bEnable;
        /* 离岗规则，最多设置4个 NON_MOTOR_VEHICLE_INTRUSION_DETECT_REGION_DEFAULT*/
        std::vector<NonMotorVehicleIntrusionRule_S> aRule;
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        DefenseTime aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        _NonMotorVehicleIntrusionDetection_S_() : bEnable(false), stLinkageList()
        {
            aAlarmTime.clear();
            // aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            aRule.clear();
        }

        /* 重载赋值运算符 */
        _NonMotorVehicleIntrusionDetection_S_ &operator=(const _NonMotorVehicleIntrusionDetection_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aAlarmTime = x.aAlarmTime;
                aRule = x.aRule;
                stLinkageList = x.stLinkageList;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _NonMotorVehicleIntrusionDetection_S_ CreateWithDefaultRule()
        {
            _NonMotorVehicleIntrusionDetection_S_ obj;
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            for (size_t i = 0; i < NON_MOTOR_VEHICLE_INTRUSION_DETECT_REGION_DEFAULT; ++i)
            {
                NonMotorVehicleIntrusionRule_S defRegion;
                defRegion.stRegion.nPointNum = NON_MOTOR_VEHICLE_INTRUSION_DETECT_REGION_DEFAULT;
                for (size_t j = 0; j < defRegion.stRegion.nPointNum; ++j)
                {
                    Common::PosF_S defPoint;
                    defRegion.stRegion.aPoint.push_back(defPoint);
                }
                obj.aRule.push_back(defRegion);
            }
            return obj;
        }
    } NonMotorVehicleIntrusionDetection_S;
    /************************非机动车闯入相关 END *******************/

    /************************其他smart事件通用检测数据结构 START *******************/
    /*其他smart事件规则参数 */
    typedef struct OtherSmartRule
    {
        /* 灵敏度[1,100] */
        unsigned int nSensitivity;
        /* 默认构造函数 */
        OtherSmartRule() : nSensitivity(50)
        {

        }
        /* 重载赋值运算符 */
        OtherSmartRule &operator=(const OtherSmartRule &x)
        {
            if (this != &x)
            {
                nSensitivity = x.nSensitivity;
            }
            return *this;
        }
    } OtherSmartRule_S;

    /*其他smart事件检测配置 */
    typedef struct _OtherSmartDection_S_
    {
        /* 是否启用 */
        bool bEnable;
        /* 其他smart事件检测规则 */
        OtherSmartRule_S stRule;
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        DefenseTime aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        _OtherSmartDection_S_() : bEnable(false), stLinkageList()
        {
            aAlarmTime.clear();
            aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
        }

        /* 重载赋值运算符 */
        _OtherSmartDection_S_ &operator=(const _OtherSmartDection_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aAlarmTime = x.aAlarmTime;
                stLinkageList = x.stLinkageList;
                stRule = x.stRule;
            }
            return *this;
        }

    } OtherSmartDection_S;
    /***********************其他smart事件通用检测数据结构 END *******************/

    enum class PMNAttributeColor_E : int
    {
        UNKNOW = -1,// 未知
        BLACK,      // 黑色
        BLUE,       // 蓝色
        BROWN,      // 棕色
        CYAN,       // 青色
        DARK_GRAY,  // 深灰色
        GRAY,       // 灰色
        GREEN,      // 绿色
        RED,        // 红色
        WHITE,      // 白色
        YELLOW,     // 黄色
        ORANGE,     // 橙色
        PINK,       // 粉色
        PURPLE      // 紫色
    };

    /* *********************** 行人抓拍报警上报信息 *********************** */

    /*  行人属性信息 */
    typedef struct _PersonAlarmAttribute_S_
    {
        bool bIsMale;           /* 是否是男性 */
        int  nAgeLabel;         /* 年龄标签:1, 2, 3, 4 */
        bool bBag;              /* 是否有包 */
        PMNAttributeColor_E  eTopColorLabel;    /* 上衣颜色标签 */
        PMNAttributeColor_E  eBottomColorLabel; /* 下衣颜色标签 */
        _PersonAlarmAttribute_S_() :
            bIsMale(false),
            nAgeLabel(-1),
            bBag(false),
            eTopColorLabel(PMNAttributeColor_E::UNKNOW),
            eBottomColorLabel(PMNAttributeColor_E::UNKNOW)
        {
        }

        _PersonAlarmAttribute_S_(const _PersonAlarmAttribute_S_ &other) :
            bIsMale(other.bIsMale),
            nAgeLabel(other.nAgeLabel),
            bBag(other.bBag),
            eTopColorLabel(other.eTopColorLabel),
            eBottomColorLabel(other.eBottomColorLabel)

        {
        }

        /* 重载赋值运算符 */
        _PersonAlarmAttribute_S_ &operator=(const _PersonAlarmAttribute_S_ &x)
        {
            if (this != &x)
            {
                bIsMale = x.bIsMale;
                nAgeLabel = x.nAgeLabel;
                bBag = x.bBag;
                eTopColorLabel = x.eTopColorLabel;
                eBottomColorLabel = x.eBottomColorLabel;
            }
            return *this;
        }
    }PersonAlarmAttribute_S;


    typedef struct _PersonAlarmInfo_S_
    {
        PersonAlarmAttribute_S stPersonAlarmAttribute;  /* 行人属性 */
        // Region_S stPersonRegion;                        /* 行人坐标 */

        std::string strPersonPicture;                   /* 行人图片 */
        std::string strCurrentPicture;                  /* 当前全景画面 */
        std::string strTimeStamp;                       /* 抓拍时间 */
        bool bIsDownLoad;                               /* 是否能下载 */

        /* 默认构造函数 */
        _PersonAlarmInfo_S_() : stPersonAlarmAttribute()/* , stPersonRegion() */
        {
        }
        /* 重载赋值运算符 */
        _PersonAlarmInfo_S_ &operator=(const _PersonAlarmInfo_S_ &x)
        {
            if (this != &x)
            {
                // stPersonRegion = x.stPersonRegion;
                stPersonAlarmAttribute = x.stPersonAlarmAttribute;
            }
            return *this;
        }
    }PersonAlarmInfo_S;

    /* *********************** 行人抓拍报警上报信息 *********************** */

    /* *********************** 非机动车抓拍报警上报信息 *********************** */

    enum class NonMotorType_E : int
    {
        UNKNOW = -1,   // 未知
        BICYCLE,       // 自行车
        TWO_WHEELER,   // 二轮车（电动车/摩托）
        THREE_WHEELER  // 三轮车
    };

    /*  非机动车属性信息 */
    typedef struct _NonMotorvehicleAlarmAttribute_S_
    {
        NonMotorType_E  eNonMotorizedVehicleType;     /* 类型 */
        PMNAttributeColor_E  eNonMotorizedVehicleColor;   /* 颜色 */

        _NonMotorvehicleAlarmAttribute_S_() :
            eNonMotorizedVehicleType(NonMotorType_E::UNKNOW),
            eNonMotorizedVehicleColor(PMNAttributeColor_E::UNKNOW)
        {
        }

        _NonMotorvehicleAlarmAttribute_S_(const _NonMotorvehicleAlarmAttribute_S_ &other) :
            eNonMotorizedVehicleType(other.eNonMotorizedVehicleType),
            eNonMotorizedVehicleColor(other.eNonMotorizedVehicleColor)

        {
        }

        /* 重载赋值运算符 */
        _NonMotorvehicleAlarmAttribute_S_ &operator=(const _NonMotorvehicleAlarmAttribute_S_ &x)
        {
            if (this != &x)
            {
                eNonMotorizedVehicleType = x.eNonMotorizedVehicleType;
                eNonMotorizedVehicleColor = x.eNonMotorizedVehicleColor;
            }
            return *this;
        }
    }NonMotorvehicleAlarmAttribute_S;


    typedef struct _NonMotorvehicleAlarmInfo_S_
    {
        NonMotorvehicleAlarmAttribute_S stNonMotorvehicleAlarmAttribute;  /* 非机动车属性 */

        std::string strTargetPicture;                   /* 目标图片 */
        std::string strCurrentPicture;                  /* 当前全景画面 */
        std::string strTimeStamp;                       /* 抓拍时间 */
        bool bIsDownLoad;                               /* 是否能下载 */

        /* 默认构造函数 */
        _NonMotorvehicleAlarmInfo_S_() : stNonMotorvehicleAlarmAttribute()
        {
        }
        /* 重载赋值运算符 */
        _NonMotorvehicleAlarmInfo_S_ &operator=(const _NonMotorvehicleAlarmInfo_S_ &x)
        {
            if (this != &x)
            {
                stNonMotorvehicleAlarmAttribute = x.stNonMotorvehicleAlarmAttribute;
            }
            return *this;
        }
    }NonMotorvehicleAlarmInfo_S;

    /* *********************** 非机动车抓拍报警上报信息 *********************** */

    /* *********************** 机动车抓拍报警上报信息 *********************** */

    enum class VehicleType_E : int
    {
        UNKNOW = -1,      // 未知
        HEAVY_TRUCK,      // 重型货车
        SUV,              // SUV
        MPV_BUSINESS,     // 商务车/多用途车
        LARGE_BUS,        // 大型巴士
        LIGHT_PASSENGER,  // 轻型客车
        SMALL_MPV,        // 小型多用途车
        PICKUP,           // 皮卡
        SEDAN,            // 轿车
        SMALL_TRUCK       // 小型卡车
    };

    /*  机动车属性信息 */
    typedef struct _MotorvehicleAlarmAttribute_S_
    {
        std::string strVehicleBrand;    /* 品牌名字 */
        VehicleType_E  eVehicleType;    /* 类型 */
        PMNAttributeColor_E eVehicleColor;   /* 颜色 */

        _MotorvehicleAlarmAttribute_S_() :
            eVehicleType(VehicleType_E::UNKNOW),
            eVehicleColor(PMNAttributeColor_E::UNKNOW)
        {
        }

        _MotorvehicleAlarmAttribute_S_(const _MotorvehicleAlarmAttribute_S_ &other) :
            eVehicleType(other.eVehicleType),
            eVehicleColor(other.eVehicleColor)
        {
        }

        /* 重载赋值运算符 */
        _MotorvehicleAlarmAttribute_S_ &operator=(const _MotorvehicleAlarmAttribute_S_ &x)
        {
            if (this != &x)
            {
                eVehicleType = x.eVehicleType;
                eVehicleColor = x.eVehicleColor;
            }
            return *this;
        }
    }MotorvehicleAlarmAttribute_S;

    typedef struct _MotorvehicleAlarmInfo_S_
    {
        MotorvehicleAlarmAttribute_S stMotorvehicleAlarmAttribute;  /* 机动车属性 */

        std::string strLicensePlateNumber;              /* 车牌号 */
        std::string strTargetPicture;                   /* 目标图片 */
        std::string strCurrentPicture;                  /* 当前全景画面 */
        std::string strTimeStamp;                       /* 抓拍时间 */
        bool bIsDownLoad;                               /* 是否能下载 */

        /* 默认构造函数 */
        _MotorvehicleAlarmInfo_S_() : stMotorvehicleAlarmAttribute()
        {
        }
        /* 重载赋值运算符 */
        _MotorvehicleAlarmInfo_S_ &operator=(const _MotorvehicleAlarmInfo_S_ &x)
        {
            if (this != &x)
            {
                stMotorvehicleAlarmAttribute = x.stMotorvehicleAlarmAttribute;
            }
            return *this;
        }
    }MotorvehicleAlarmInfo_S;

    /* *********************** 机动车抓拍报警上报信息 *********************** */

    typedef struct _AttributeDetectSwitch_
    {
        bool bFaceAttribute = false;
        bool bPedestrianAttribute = false;
        bool bMotorVehicleAttribute = false;
        bool bNonMotorVehicleAttribute = false;
    }AttributeDetectSwitch_S;

#endif

#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT
    /************************垃圾暴露检测相关 START *******************/
    /* 垃圾暴露检测规则参数 */
    typedef struct GarbageExposureRule
    {
        /* 灵敏度[1,100] */
        unsigned int nSensitivity;
        /* 规则区域定义 多边形框 */
        Region_S stRegion;
        /* 默认构造函数 */
        GarbageExposureRule()
            : nSensitivity(50),
              stRegion()
        {

        }
        /* 重载赋值运算符 */
        GarbageExposureRule &operator=(const GarbageExposureRule &x)
        {
            if (this != &x)
            {
                nSensitivity = x.nSensitivity;
                stRegion = x.stRegion;
            }
            return *this;
        }
    } GarbageExposureRule_S;

    /*  垃圾暴露检测配置 */
    typedef struct _GarbageExposureDetection_S_
    {
        /* 是否启用 */
        bool bEnable;
        /* 垃圾暴露规则 */
        GarbageExposureRule_S stRule;
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        DefenseTime aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;

        /* 默认构造函数 */
        _GarbageExposureDetection_S_() : bEnable(false), stLinkageList()
        {
            aAlarmTime.clear();
            // aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _GarbageExposureDetection_S_ CreateWithDefaultRule()
        {
            _GarbageExposureDetection_S_ obj;
            obj.stRule.stRegion = Region_S::CreateWithDefaultRule(4);
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }

        /* 重载赋值运算符 */
        _GarbageExposureDetection_S_& operator=(const _GarbageExposureDetection_S_& x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aAlarmTime = x.aAlarmTime;
                stLinkageList = x.stLinkageList;
                stRule = x.stRule;
            }
            return *this;
        }
    } GarbageExposureDetection_S;
    /************************垃圾暴露检测相关 END *******************/

    /************************垃圾满溢检测相关 START *******************/
    /* 垃圾满溢检测规则参数 */
    typedef struct GarbageOverflowRule
    {
        /* 灵敏度[1,100] */
        unsigned int nSensitivity;
        /* 规则区域定义 多边形框 */
        Region_S stRegion;
        /* 默认构造函数 */
        GarbageOverflowRule() : nSensitivity(50), stRegion()
        {

        }
        /* 重载赋值运算符 */
        GarbageOverflowRule &operator=(const GarbageOverflowRule &x)
        {
            if (this != &x)
            {
                nSensitivity = x.nSensitivity;
                stRegion = x.stRegion;
            }
            return *this;
        }
    } GarbageOverflowRule_S;

    /*  垃圾满溢检测配置 */
    typedef struct _GarbageOverflowDetection_S_
    {
        /* 是否启用 */
        bool bEnable;
        /* 垃圾满溢规则 */
        GarbageOverflowRule_S stRule;
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        DefenseTime aAlarmTime;
        /* 联动 */
        LinkageList_S stLinkageList;
        /* 默认构造函数 */
        _GarbageOverflowDetection_S_() : bEnable(false), stLinkageList()
        {
            aAlarmTime.clear();
            // aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
        }
        /* 静态方法：返回一个带有默认规则的对象 */
        static _GarbageOverflowDetection_S_ CreateWithDefaultRule()
        {
            _GarbageOverflowDetection_S_ obj;
            obj.stRule.stRegion = Region_S::CreateWithDefaultRule(4);
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }
        /* 重载赋值运算符 */
        _GarbageOverflowDetection_S_& operator=(const _GarbageOverflowDetection_S_& x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                aAlarmTime = x.aAlarmTime;
                stLinkageList = x.stLinkageList;
                stRule = x.stRule;
            }
            return *this;
        }
    } GarbageOverflowDetection_S;
    /************************垃圾满溢检测相关 END *******************/
#endif

#if CAP_AI_PEOPLE_STATISTICS
    /************************ 人数统计相关 ************************/
    /* 人流统计类型 */
    typedef enum _PeopleFlowStatisticsType_E_
    {
        PEOPLE_FLOW_STAT_TOTAL = 0, /* 总人数 */
        PEOPLE_FLOW_STAT_ENTER = 1, /* 进入人数 */
        PEOPLE_FLOW_STAT_LEAVE = 2, /* 离开人数 */
    } PeopleFlowStatisticsType_E;

    /* 人数报警等级 */
    typedef enum _PopulationAlarmSeverity_E_
    {
        POPULATION_ALARM_NORMAL = 0, /* 普通报警 */
        POPULATION_ALARM_MEDIUM = 1, /* 中度报警 */
        POPULATION_ALARM_SEVERE = 2, /* 严重报警 */
    } PopulationAlarmSeverity_E;

    /* 人流统计规则线 */
    typedef struct _PeopleFlowRuleLine_S_
    {
        /* 规则线起点坐标 */
        Common::PosF_S stStartPos;
        /* 规则线终点坐标 */
        Common::PosF_S stEndPos;
        /* 统计方向，仅支持 A->B 或 B->A */
        CrossDirection_E enDirection;

        /* 默认构造函数 */
        _PeopleFlowRuleLine_S_() :
            stStartPos(),
            stEndPos(),
            enDirection(A_TO_B)
        {
        }

        /* 重载赋值运算符 */
        _PeopleFlowRuleLine_S_ &operator=(const _PeopleFlowRuleLine_S_ &x)
        {
            if (this != &x)
            {
                stStartPos = x.stStartPos;
                stEndPos = x.stEndPos;
                enDirection = x.enDirection;
            }
            return *this;
        }
    } PeopleFlowRuleLine_S;

    /* 单档人数报警配置 */
    typedef struct _PopulationAlarmRule_S_
    {
        /* 是否启用当前报警等级 */
        bool bEnable;
        /* 人数触发阈值，触发条件为大于等于该值 */
        unsigned int nThreshold;
        /* 当前报警等级对应的联动方式 */
        LinkageList_S stLinkageList;

        /* 默认构造函数 */
        _PopulationAlarmRule_S_() :
            bEnable(false),
            nThreshold(0),
            stLinkageList()
        {
        }

        /* 重载赋值运算符 */
        _PopulationAlarmRule_S_ &operator=(const _PopulationAlarmRule_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                nThreshold = x.nThreshold;
                stLinkageList = x.stLinkageList;
            }
            return *this;
        }
    } PopulationAlarmRule_S;

    /* 三级人数报警配置 */
    typedef struct _PopulationAlarmConfig_S_
    {
        /* 普通报警配置 */
        PopulationAlarmRule_S stNormal;
        /* 中度报警配置 */
        PopulationAlarmRule_S stMedium;
        /* 严重报警配置 */
        PopulationAlarmRule_S stSevere;

        /* 默认构造函数 */
        _PopulationAlarmConfig_S_() :
            stNormal(),
            stMedium(),
            stSevere()
        {
        }

        /* 重载赋值运算符 */
        _PopulationAlarmConfig_S_ &operator=(const _PopulationAlarmConfig_S_ &x)
        {
            if (this != &x)
            {
                stNormal = x.stNormal;
                stMedium = x.stMedium;
                stSevere = x.stSevere;
            }
            return *this;
        }
    } PopulationAlarmConfig_S;

    /* 定时清零配置 */
    typedef struct _StatisticsResetConfig_S_
    {
        /* 是否启用定时清零 */
        bool bEnable;
        /* 每天固定执行清零的时间点 */
        Common::Time_S stExecuteTime;

        /* 默认构造函数 */
        _StatisticsResetConfig_S_() :
            bEnable(false),
            stExecuteTime()
        {
            stExecuteTime.nHour = 0;
            stExecuteTime.nMinute = 0;
            stExecuteTime.nSecond = 0;
        }

        /* 重载赋值运算符 */
        _StatisticsResetConfig_S_ &operator=(const _StatisticsResetConfig_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                stExecuteTime = x.stExecuteTime;
            }
            return *this;
        }
    } StatisticsResetConfig_S;

    /* 人流统计配置 */
    typedef struct _PeopleFlowStatistics_S_
    {
        /* 是否启用人流统计 */
        bool bEnable;
        /* 灵敏度[1,100] */
        unsigned int nSensitivity;
        /* 人流统计规则线 */
        PeopleFlowRuleLine_S stRuleLine;
        /* 人流统计检测区域，固定为四边形区域 */
        Region_S stDetectRegion;
        /* 数据上报时间间隔，单位为秒 */
        unsigned int nReportInterval;
        /* 统计类型：总人数/进入人数/离开人数 */
        PeopleFlowStatisticsType_E enStatisticsType;
        /* 定时清零配置 */
        StatisticsResetConfig_S stTimedReset;
        /* 滞留人数三级报警配置 */
        PopulationAlarmConfig_S stStayAlarm;
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        DefenseTime aAlarmTime;

        /* 默认构造函数 */
        _PeopleFlowStatistics_S_() :
            bEnable(false),
            nSensitivity(50),
            stRuleLine(),
            nReportInterval(60),
            enStatisticsType(PEOPLE_FLOW_STAT_TOTAL),
            stTimedReset(),
            stStayAlarm()
        {
            stStayAlarm.stNormal.nThreshold = 60;
            stStayAlarm.stMedium.nThreshold = 120;
            stStayAlarm.stSevere.nThreshold = 180;
            aAlarmTime.clear();
        }

        /* 重载赋值运算符 */
        _PeopleFlowStatistics_S_ &operator=(const _PeopleFlowStatistics_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                nSensitivity = x.nSensitivity;
                stRuleLine = x.stRuleLine;
                stDetectRegion = x.stDetectRegion;
                nReportInterval = x.nReportInterval;
                enStatisticsType = x.enStatisticsType;
                stTimedReset = x.stTimedReset;
                stStayAlarm = x.stStayAlarm;
                aAlarmTime = x.aAlarmTime;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _PeopleFlowStatistics_S_ CreateWithDefaultRule()
        {
            _PeopleFlowStatistics_S_ obj;
            obj.stDetectRegion = Region_S::CreateWithDefaultRule(4);
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }
    } PeopleFlowStatistics_S;

    /* 人员密度检测配置 */
    typedef struct _PeopleDensityDetection_S_
    {
        /* 是否启用人员密度检测 */
        bool bEnable;
        /* 灵敏度[1,100] */
        unsigned int nSensitivity;
        /* 人员密度检测区域，固定为四边形区域 */
        Region_S stDetectRegion;
        /* 数据上报时间间隔，单位为秒 */
        unsigned int nReportInterval;
        /* 人员密度三级报警配置 */
        PopulationAlarmConfig_S stDensityAlarm;
        /* 布防时间:一周7天，每天可以设置8个时间段 */
        DefenseTime aAlarmTime;

        /* 默认构造函数 */
        _PeopleDensityDetection_S_() :
            bEnable(false),
            nSensitivity(50),
            nReportInterval(60),
            stDensityAlarm()
        {
            stDensityAlarm.stNormal.nThreshold = 20;
            stDensityAlarm.stMedium.nThreshold = 24;
            stDensityAlarm.stSevere.nThreshold = 30;
            aAlarmTime.clear();
        }

        /* 重载赋值运算符 */
        _PeopleDensityDetection_S_ &operator=(const _PeopleDensityDetection_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                nSensitivity = x.nSensitivity;
                stDetectRegion = x.stDetectRegion;
                nReportInterval = x.nReportInterval;
                stDensityAlarm = x.stDensityAlarm;
                aAlarmTime = x.aAlarmTime;
            }
            return *this;
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _PeopleDensityDetection_S_ CreateWithDefaultRule()
        {
            _PeopleDensityDetection_S_ obj;
            obj.stDetectRegion = Region_S::CreateWithDefaultRule(4);
            obj.aAlarmTime.assign(7, std::vector<Common::SchedTime_S>(1));
            return obj;
        }
    } PeopleDensityDetection_S;
    /************************ 人数统计相关 END ************************/
#endif

}; // namespace Alarm
