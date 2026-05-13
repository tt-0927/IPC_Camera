/***
 * @FilePath     : system_define.h
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2024-10-09 10:29:46
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-03-28 10:30:01
 * @Description  :
 */

#pragma once

#include <set>
#include <string>
#include <vector>
#include "preview_define.h"
#include "network_define.h"
#include "user_define.h"
#include "alarm_define.h"
#include "web_plugin_define.h"
#include "record_define.h"
#include "capture_define.h"
#include "storage_manage_define.h"
#include "video_define.h"
#include "audio_define.h"
#include "isp_define.h"
#include "osd_define.h"

namespace System
{
    /**
     * @brief IP过滤模式
     */
    typedef enum
    {
        IPFILTER_DENY = 0, /* 黑名单模式：禁止列表 */
        IPFILTER_ALLOW     /* 白名单模式：允许列表 */
    } IpFilterMode_E;

    /**
     * @brief 语言
     */
    typedef enum class Language
    {
        SIMP_CHINESE = 0, /* 简体中文 */
        ENGLISH      = 1  /* 英文 */
    } Language_E;

    typedef enum class Period
    {
        AM = 0, /* 24小时制式 */
        PM = 1, /* 12小时制式 */
    } Period_E;

    typedef enum class DateFormat
    {
        YYYY_MM_DD = 0, /* 年月日，例如 2024年10月01日 或 2024-10-01 */
        MM_DD_YYYY = 1, /* 月日年，例如 10月01日2024年 或 10-01-2024 */
        DD_MM_YYYY = 2, /* 日月年，例如 01日10月2024年 或 01-10-2024 */
        YYYYMMDD = 3,   /* 年月日，例如 2024年10月01日 或 2024/10/01 */
        MMDDYYYY = 4,   /* 月日年，例如 10月01日2024年 或 10/01/2024 */
        DDMMYYYY = 5,   /* 日月年，例如 01日10月2024年 或 01/10/2024 */
    } DateFormat_E;

    /**
     * @brief 定义星期几的枚举类型
     */
    typedef enum class Week
    {
        SUNDAY    = 0, /* 星期日 */
        MONDAY    = 1, /* 星期一 */
        TUESDAY   = 2, /* 星期二 */
        WEDNESDAY = 3, /* 星期三 */
        THURSDAY  = 4, /* 星期四 */
        FRIDAY    = 5, /* 星期五 */
        SATURDAY  = 6  /* 星期六 */
    } Week_E;

    /* 定义时区枚举类型 */
    typedef enum class TimeZone
    {
        UTC_MINUS_12 = -12, /* UTC-12:00 日界线西 */
        UTC_MINUS_11 = -11, /* UTC-11:00 中途岛, 萨摩亚群岛 */
        UTC_MINUS_10 = -10, /* UTC-10:00 夏威夷 */
        UTC_MINUS_9  = -9,  /* UTC-9:00 阿拉斯加 */
        UTC_MINUS_8  = -8,  /* UTC-8:00 太平洋标准时间(美国和加拿大) */
        UTC_MINUS_7  = -7,  /* UTC-7:00 山地时间(美国和加拿大) */
        UTC_MINUS_6  = -6,  /* UTC-6:00 中部时间(美国和加拿大) */
        UTC_MINUS_5  = -5,  /* UTC-5:00 东部时间(美国和加拿大) */
        UTC_MINUS_4  = -4,  /* UTC-4:00 大西洋时间(加拿大) */
        UTC_MINUS_3  = -3,  /* UTC-3:00 乔治敦、巴西利亚 */
        UTC_MINUS_2  = -2,  /* UTC-2:00 中大西洋 */
        UTC_MINUS_1  = -1,  /* UTC-1:00 亚速尔群岛, 佛得角群岛 */
        UTC_PLUS_0   = 0,   /* UTC+0:00 都柏林、爱丁堡、伦敦 */
        UTC_PLUS_1   = 1,   /* UTC+1:00 阿姆斯特丹、柏林、罗马、巴黎 */
        UTC_PLUS_2   = 2,   /* UTC+2:00  雅典、耶路撒冷 */
        UTC_PLUS_3   = 3,   /* UTC+3:00 巴格达、科威特、莫斯科、伊斯坦布尔、圣彼得堡 */
        UTC_PLUS_4   = 4,   /* UTC+4:00 高加索标准时间 */
        UTC_PLUS_5   = 5,   /* UTC+5:00 伊斯兰堡、卡拉奇、塔什干 */
        UTC_PLUS_6   = 6,   /* UTC+6:00 阿拉木图、达卡 */
        UTC_PLUS_7   = 7,   /* UTC+7:00 曼谷、河内、雅加达、新西伯利亚 */
        UTC_PLUS_8   = 8,   /* UTC+8:00 北京、乌鲁木齐、新加坡、珀斯 */
        UTC_PLUS_9   = 9,   /* UTC+9:00 首尔、东京、大阪、札幌 */
        UTC_PLUS_10  = 10,  /* UTC+10:00 墨尔本、悉尼、堪培拉、布里斯班、霍巴特 */
        UTC_PLUS_11  = 11,  /* UTC+11:00 马加丹、索罗门群岛 */
        UTC_PLUS_12  = 12,  /* UTC+12:00 奥克兰、惠灵顿、斐济 */
        UTC_PLUS_13  = 13   /* UTC+13:00 努库阿洛法 */
    } TimeZone_E;

    /* 账号锁定时长枚举 */
    typedef enum _LockDuration_E_
    {
        LOCK_DURATION_5_MIN   ,     /* 5  分钟 */
        LOCK_DURATION_30_MIN  ,     /* 30 分钟 */
        LOCK_DURATION_60_MIN  ,     /* 60 分钟 */
        LOCK_DURATION_1_DAY   ,     /* 1  天   */
        LOCK_DURATION_5_DAY         /* 5  天   */
    } LockDuration_E;

    inline const char *to_string(Language l, Period p)
    {
        switch (l)
        {
        case Language::SIMP_CHINESE:
            switch (p)
            {
            case Period::AM:
                return "上午";
            case Period::PM:
                return "下午";
            default:
                return "未知时间段";
            }
            return "中文";
        case Language::ENGLISH:
            switch (p)
            {
            case Period::AM:
                return "AM";
            case Period::PM:
                return "PM";
            default:
                return "Unknown time period";
            }
            return "English";
        }
        return NULL;
    }

    inline const char *to_string(Language l, DateFormat df)
    {
        switch (l)
        {
        case Language::SIMP_CHINESE:
            switch (df)
            {
            case DateFormat::YYYY_MM_DD:
            case DateFormat::YYYYMMDD:
                return "%Y年%m月%d日 %H时%M分%S秒";
            case DateFormat::MM_DD_YYYY:
            case DateFormat::MMDDYYYY:
                return "%m月%d日%Y年 %H时%M分%S秒";
            case DateFormat::DD_MM_YYYY:
            case DateFormat::DDMMYYYY:
                return "%d日%m月%Y年 %H时%M分%S秒";
            default:
                return "未知时间";
            }
            return "中文";
        case Language::ENGLISH:
            switch (df)
            {
            case DateFormat::YYYY_MM_DD:
                return "%Y-%m-%d %H:%M:%S";
            case DateFormat::MM_DD_YYYY:
                return "%m-%d-%Y %H:%M:%S";
            case DateFormat::DD_MM_YYYY:
                return "%d-%m-%Y %H:%M:%S";
            case DateFormat::YYYYMMDD:
                return "%Y/%m/%d %H:%M:%S";
            case DateFormat::MMDDYYYY:
                return "%m/%d/%Y %H:%M:%S";
            case DateFormat::DDMMYYYY:
                return "%d/%m/%Y %H:%M:%S";
            default:
                return "Unknown time";
            }
            return "English";
        }
        return NULL;
    }

    inline const char *to_string(Language l, Week w)
    {
        switch (l)
        {
        case Language::SIMP_CHINESE:
            switch (w)
            {
            case Week::SUNDAY:
                return "星期日";
            case Week::MONDAY:
                return "星期一";
            case Week::TUESDAY:
                return "星期二";
            case Week::WEDNESDAY:
                return "星期三";
            case Week::THURSDAY:
                return "星期四";
            case Week::FRIDAY:
                return "星期五";
            case Week::SATURDAY:
                return "星期六";
            default:
                return "未知星期";
            }
            return "中文";
        case Language::ENGLISH:
            switch (w)
            {
            case Week::SUNDAY:
                return "Sunday";
            case Week::MONDAY:
                return "Monday";
            case Week::TUESDAY:
                return "Tuesday";
            case Week::WEDNESDAY:
                return "Wednesday";
            case Week::THURSDAY:
                return "Thursday";
            case Week::FRIDAY:
                return "Friday";
            case Week::SATURDAY:
                return "Saturday";
            default:
                return "Unknown week";
            }
            return "English";
        }
        return NULL;
    }

    inline const char *to_string(TimeZone tz)
    {
        switch (tz)
        {
        // case TimeZone::UTC_MINUS_12:
        //     return "Dateline Standard Time (GMT-12:00)";    /* 时区 -12 */
        // case TimeZone::UTC_MINUS_11:
        //     return "Samoa Standard Time (GMT-11:00)";       /* 时区 -11 */
        // case TimeZone::UTC_MINUS_10:
        //     return "Hawaiian Standard Time (GMT-10:00)";    /* 时区 -10 */
        // case TimeZone::UTC_MINUS_9:
        //     return "Alaskan Standard Time (GMT-9:00)";      /* 时区 -9 */
        // case TimeZone::UTC_MINUS_8:
        //     return "Pacific Standard Time (GMT-8:00)";      /* 时区 -8 */
        // case TimeZone::UTC_MINUS_7:
        //     return "Mountain Standard Time (GMT-7:00)";     /* 时区 -7 */
        // case TimeZone::UTC_MINUS_6:
        //     return "Central Standard Time (GMT-6:00)";      /* 时区 -6 */
        // case TimeZone::UTC_MINUS_5:
        //     return "Eastern Standard Time (GMT-5:00)";      /* 时区 -5 */
        // case TimeZone::UTC_MINUS_4:
        //     return "Atlantic Standard Time (GMT-4:00)";     /* 时区 -4 */
        // case TimeZone::UTC_MINUS_3:
        //     return "Brasilia Standard Time (GMT-3:00)";     /* 时区 -3 */
        // case TimeZone::UTC_MINUS_2:
        //     return "Mid-Atlantic Standard Time (GMT-2:00)"; /* 时区 -2 */
        // case TimeZone::UTC_MINUS_1:
        //     return "Azores Standard Time (GMT-1:00)";       /* 时区 -1 */
        // case TimeZone::UTC_PLUS_0:
        //     return "Greenwich Mean Time (GMT+0:00)";        /* 时区 0 */
        // case TimeZone::UTC_PLUS_1:
        //     return "Central European Time (GMT+1:00)";      /* 时区 +1 */
        // case TimeZone::UTC_PLUS_2:
        //     return "Eastern European Time (GMT+2:00)";      /* 时区 +2 */
        // case TimeZone::UTC_PLUS_3:
        //     return "Moscow Standard Time (GMT+3:00)";       /* 时区 +3 */
        // case TimeZone::UTC_PLUS_4:
        //     return "Gulf Standard Time (GMT+4:00)";         /* 时区 +4 */
        // case TimeZone::UTC_PLUS_5:
        //     return "Pakistan Standard Time (GMT+5:00)";     /* 时区 +5 */
        // case TimeZone::UTC_PLUS_6:
        //     return "Bangladesh Standard Time (GMT+6:00)";   /* 时区 +6 */
        // case TimeZone::UTC_PLUS_7:
        //     return "Indochina Time (GMT+7:00)";             /* 时区 +7 */
        // case TimeZone::UTC_PLUS_8:
        //     return "China Standard Time (GMT+8:00)";        /* 时区 +8 */
        // case TimeZone::UTC_PLUS_9:
        //     return "Japan Standard Time (GMT+9:00)";        /* 时区 +9 */
        // case TimeZone::UTC_PLUS_10:
        //     return "Australian Eastern Time (GMT+10:00)";   /* 时区 +10 */
        // case TimeZone::UTC_PLUS_11:
        //     return "Solomon Islands Time (GMT+11:00)";      /* 时区 +11 */
        // case TimeZone::UTC_PLUS_12:
        //     return "New Zealand Standard Time (GMT+12:00)"; /* 时区 +12 */
        // default:
        //     return "Unknown TimeZone";                      /* 未知时区 */
        // }
        case TimeZone::UTC_MINUS_12:
            return "GMT-12:00"; /* 时区 -12 */
        case TimeZone::UTC_MINUS_11:
            return "GMT-11:00"; /* 时区 -11 */
        case TimeZone::UTC_MINUS_10:
            return "GMT-10:00"; /* 时区 -10 */
        case TimeZone::UTC_MINUS_9:
            return "GMT-9:00"; /* 时区 -9 */
        case TimeZone::UTC_MINUS_8:
            return "GMT-8:00"; /* 时区 -8 */
        case TimeZone::UTC_MINUS_7:
            return "GMT-7:00"; /* 时区 -7 */
        case TimeZone::UTC_MINUS_6:
            return "GMT-6:00"; /* 时区 -6 */
        case TimeZone::UTC_MINUS_5:
            return "GMT-5:00"; /* 时区 -5 */
        case TimeZone::UTC_MINUS_4:
            return "GMT-4:00"; /* 时区 -4 */
        case TimeZone::UTC_MINUS_3:
            return "GMT-3:00"; /* 时区 -3 */
        case TimeZone::UTC_MINUS_2:
            return "GMT-2:00"; /* 时区 -2 */
        case TimeZone::UTC_MINUS_1:
            return "GMT-1:00"; /* 时区 -1 */
        case TimeZone::UTC_PLUS_0:
            return "GMT+0:00"; /* 时区 0 */
        case TimeZone::UTC_PLUS_1:
            return "GMT+1:00"; /* 时区 +1 */
        case TimeZone::UTC_PLUS_2:
            return "GMT+2:00"; /* 时区 +2 */
        case TimeZone::UTC_PLUS_3:
            return "GMT+3:00"; /* 时区 +3 */
        case TimeZone::UTC_PLUS_4:
            return "GMT+4:00"; /* 时区 +4 */
        case TimeZone::UTC_PLUS_5:
            return "GMT+5:00"; /* 时区 +5 */
        case TimeZone::UTC_PLUS_6:
            return "GMT+6:00"; /* 时区 +6 */
        case TimeZone::UTC_PLUS_7:
            return "GMT+7:00"; /* 时区 +7 */
        case TimeZone::UTC_PLUS_8:
            return "GMT+8:00"; /* 时区 +8 */
        case TimeZone::UTC_PLUS_9:
            return "GMT+9:00"; /* 时区 +9 */
        case TimeZone::UTC_PLUS_10:
            return "GMT+10:00"; /* 时区 +10 */
        case TimeZone::UTC_PLUS_11:
            return "GMT+11:00"; /* 时区 +11 */
        case TimeZone::UTC_PLUS_12:
            return "GMT+12:00"; /* 时区 +12 */
        default:
            return "Unknown TimeZone"; /* 未知时区 */
        }
        return NULL;
    }

    /* 设备信息 */
    typedef struct _DeviceInfo_S_
    {
        /* 可编辑的设备信息 */
        std::string deviceName = "Camera";                   /* 设备名称 */
        int deviceID = 1;                                    /* 设备编号 */
        /* 设备固定属性 */
        std::string strUnitTpye;                             /* 设备型号 */
        std::string serialNumber;                            /* 设备序列号 */
        std::string hardwareVersion;                         /* 硬件版本 */
        std::string systemVersion;                           /* 系统版本 */
        std::string pluginVersion;                           /* 插件版本 */
        std::string webVersion;                              /* web版本 */
        /* 设备配置信息 */
        int nAlarmInputCount  = 0;                           /* 报警输入个数 */
        int nAlarmOutputCount = 0;                           /* 报警输出个数 */

        /* 重载 != 运算符，比较关键版本信息是否不一致 */
        bool operator!=(const _DeviceInfo_S_ &other) const
        {
            return (deviceName != other.deviceName || deviceID != other.deviceID || strUnitTpye != other.strUnitTpye
                    || serialNumber != other.serialNumber || hardwareVersion != other.hardwareVersion
                    || systemVersion != other.systemVersion || pluginVersion != other.pluginVersion
                    || webVersion != other.webVersion || nAlarmInputCount != other.nAlarmInputCount
                    || nAlarmOutputCount != other.nAlarmOutputCount);
        }

        /* 重载 == 运算符 */
        bool operator==(const _DeviceInfo_S_ &other) const
        {
            return !(*this != other);
        }
    } DeviceInfo_S;

    /* 设备配置 */
    typedef struct _DeviceConfig_S_
    {
        Language_E   enLanguage           = Language_E::SIMP_CHINESE; /* 语言 */
        TimeZone_E   enTimeZone           = TimeZone_E::UTC_PLUS_8;   /* 时区 */
        DateFormat_E enDateFormat         = DateFormat_E::YYYY_MM_DD; /* 日期格式 */
        std::string  strDateTime;                                     /* 日期时间字符串 */
        bool         bEnableOptPassword   = true;                     /* 开启关闭操作密码 */
        bool         bEanbleStartupWizard = false;                    /* 开启关闭开机向导 */
        bool         bEanbleDecodeEnhance = false;                    /* 开启关闭解码增强模式 */
    } DeviceConfig_S;

    /**
     * @brief 时间配置结构体，用于管理设备的时区、时间同步方式和当前设备时间等配置。
     */
    typedef struct _NTPInfo_S_
    {
        std::string address       = "time.windows.com"; /* 服务器地址 */
        int         nPort         = 123;                /* 端口 */
        int         nSyncInterval = 60;                 /* 校时时间间隔，分钟 */
    } NTPInfo_S;

    /**
     * @brief 时间配置结构体，用于管理设备的时区、时间同步方式和当前设备时间等配置。
     */
    typedef struct _TimeInfo_S_
    {
        /* 基础时间配置 */
        TimeZone_E   enTimeZone     = TimeZone_E::UTC_PLUS_8; /* 时区 */
        DateFormat_E enDateFormat   = DateFormat_E::YYYY_MM_DD; /* 日期格式 */
        /* 时间同步控制 */
        bool         bEnableNTPSync = false;                  /* 是否开启 NTP 校时 */
        NTPInfo_S    stNTPInfo;                               /* NTP 信息 */
        bool bManualSync = true;                              /* 手动校时 */
        std::string  strDateTime;                             /* 日期时间字符串 */
        bool bIsSyncWithComputer = false;                     /* 与计算机时间同步 */
    } TimeInfo_S;

    typedef struct
    {
        std::string strNowTime; /* 当前时间 */
    } RealTime_S;

    typedef struct
    {
        std::string address; /* 服务器地址 */
        int nPort;           /* 端口 */
    } TestNtp_S;

    /**
     * @brief 校验方式的枚举类型。
     */
    typedef enum class Parity
    {
        None = 0, /* 无校验 */
        Odd,      /* 奇校验 */
        Even      /* 偶校验 */
    } Parity_E;

    /**
     * @brief 流控方式的枚举类型。
     */
    typedef enum class FlowControl
    {
        None = 0, /* 无流控 */
        Hardware, /* 硬流控 */
        Software  /* 软流控 */
    } FlowControl_E;

    /**
     * @brief 控制的枚举类型。
     */
    typedef enum class CtrlMode
    {

        ALPHA_CHANNEL = 1, /* 透明通道 */
        CONSOLE = 2        /* 控制台(参数控制) */
    } CtrlMode_E;

    /**
     * @brief 串口配置结构体，用于管理 RS232 和 RS485 的配置。
     */
    typedef struct _SerialInfo_S_
    {
        std::string chnName;                               /* 通道名称。仅485有 */
        int nChnId;                                        /* 通道ID */
        int nBaudRate = 9600;                              /* 波特率设置。支持的值如：9600, 19200, 38400, 57600, 115200 等。 */
        int nDataBits = 5;                                 /* 数据位设置。支持的值为 5, 6, 7, 8。 */
        int nStopBits = 1;                                 /* 停止位设置。支持的值为 1 或 2。 */
        Parity_E enParity = Parity::None;                  /* 校验方式。0 表示无校验，1 表示奇校验，2 表示偶校验。 */
        FlowControl_E enFlowControl = FlowControl_E::None; /* 流控方式。0 表示无流控，1 表示硬流控，2 表示软流控。 */
        CtrlMode_E enCtrlMode = CtrlMode_E::CONSOLE;       /* 控制模式 */
    } SerialInfo_S;

    /**
     * @brief 升级维护配置信息
     */
    typedef struct _UpgradeMaintain_S_
    {
        std::string strCurrentVersion = "V1.0";         /* 当前版本 */
        bool        bAutoDownPack     = false;          /* 是否自动下载最新安装包 */
        bool        bAutoMaintain     = false;          /* 是否自动维护*/
        Week_E      enWeek            = Week_E::MONDAY; /* 星期 */
        std::string strMaintainTime   = "00:00:00";     /* 维护时间 */
    } UpgradeMaintain_S;

    /**
     * @brief 摘要认证算法
     */
    typedef enum class DigestAlgorithm
    {
        MD5 = 0,    /* MD5 */
        SHA256,     /* SHA256 */
        MD5_SHA256, /* MD5/SHA256 */
    } DigestAlgorithm_E;

    /**
     * @brief 认证方式
     */
    typedef enum class AuthMethod
    {
        DIGEST = 0,   /* digest */
        DIGEST_BASIC, /* digest/basic */
    } AuthMethod_E;

    /**
     * @brief 安全认证方式
     */
    typedef struct _SecurityCert_S_
    {
        int               nRtspCert             = 0;
        int               nRtspAlgorithm        = 0;
        int               nWebCert              = 0;
        int               nWebAlgorithm         = 0;
        AuthMethod_E      enRtspAuth            = AuthMethod::DIGEST;     /* RTSP认证 */
        DigestAlgorithm_E enRtspDigestAlgorithm = DigestAlgorithm_E::MD5; /* RTSP摘要算法 */
        AuthMethod_E      enWebAuth             = AuthMethod::DIGEST;     /* WEB认证 */
        DigestAlgorithm_E enWebDigestAlgorithm  = DigestAlgorithm_E::MD5; /* WEB摘要算法 */
    } SecurityCert_S;


    /* 用户登录锁定机制 */
    typedef struct _LoginLock_S_
    {
        bool bIllegalLoginEnable = false;                   /* 是否启用非法登录锁定 */
        int nCheckInterval = 30;                            /* 验证时间间隔 1-1440（分钟） 输入0，该功能无效*/
        int nMaxErrorTimes = 5;                             /* 最大连续错误次数 */
        LockDuration_E nLockDuration = LOCK_DURATION_5_MIN; /* 账号锁定时长（分钟） */
    } LoginLock_S;

    /* 密码策略  */
    typedef struct _PwdPolicy_S_
    {
        bool bPwdSecurityLevelEnable   = false; /* 是否启用密码安全级别检查 - 设置用户密码安全级别为低*/
        bool bAllowLowLevelPwdLogin    = false; /* 是否允许低等级密码外网账号访问系统 */
    } PwdPolicy_S;

    /* SSH 远程管理  */
    typedef struct _SshAdmin_S_
    {
        bool        bSshEnable      = false;        /* 是否启用 SSH 服务 */
        int         nSshPort        = 22;           /* SSH 服务端口号*/
        std::string strSshStartTime;                /* SSH 服务启动时间 */
        std::string strSshCountdown = "08:00:00";   /* SSH 服务倒计时 */
    } SshAdmin_S;

    /**
     * @brief 安全服务配置
     */
    typedef struct _SecurityServices_S_
    {
        LoginLock_S  stLoginLock;   /* 用户登录锁定机制 */
        PwdPolicy_S  stPwdPolicy;   /* 密码策略 */
        SshAdmin_S   stSshAdmin;    /* SSH 远程管理 */
    } SecurityServices_S;
    
    /**
     * @brief 安全服务配置
     */
    typedef struct
    {
        std::string strCountdown;
    }SshCountdown_S;

    /**
     * @brief 日志服务器
     */
    typedef struct LogServerInfo
    {
        bool bEnable = true;                         /* 是否启用 */
        bool bEnSsl = false;                          /* 是否传输加密 */
        std::string strServerAddr = "oam.itc-pa.cn";  /* 服务器地址 */
        int nPort = 1883;                             /* 服务器端口号 */
    } LogServerInfo_S;

    /**
     * @brief 升级状态
     */
    typedef enum _TiUpgradeRuslut_
    {
        TI_UPGRADE_NULL              = -1, /* 未升级 */
        TI_UPGRADE_RUNING            = 0,  /* 升级中 */
        TI_UPGRADE_RUNFAIL           = 1,  /* 升级失败 */
        TI_UPGRADE_RUNSUCCESS        = 2,  /* 升级成功 */
        TI_UPGRADE_OTHERUPDATE_FAILE = 3,  /* 其他服务器升级失败 */
    } TiUpgradeRuslut_E;

    /**
     * @brief 升级路径
     */
    typedef struct
    {
        std::string strUpgradePath; /* 升级路径 */
    } UpgradeInfo_S;

    /**
     * @brief 升级状态
     */
    typedef struct
    {
        int nUpgradeStatus; /* 升级状态 */
    } UpgradeStatus_S;

    /**
     * @brief 升级包版本
     */
    typedef struct
    {
        std::string      strVersion; 	/* 升级包版本 */
    } UpgradeVersion_S;

    typedef struct ProgramInfo
    {
        /* 在线程序*/
        std::set<int> inlines; 
        /* 目前程序总数：5，cgi-enc、record、operation、stream、upgrade */        
        int nTatol = 5; 
    } ProgramInfo_S;

    /**
     * @brief IP地址过滤信息
     */
    typedef struct
    {
        bool bEnable;
        std::string strIp;
    } IpFilterInfo_S;

    /**
    * @brief IP地址过滤配置信息
    */
    typedef struct _IpFilterConfigInfo_S_
    {
        bool                        bEnable = false;       /* 是否启用IP地址过滤 */
        IpFilterMode_E              eMode = IPFILTER_DENY; /* 过滤模式 */
        std::vector<IpFilterInfo_S> vecIps;                /* IP列表 */
    } IpFilterConfigInfo_S;

    /**
     * @brief 修改IP过滤地址信息
     */
    typedef struct
    {
        std::string strOldIp;
        std::string strNewIp;
    } IpFilterModify_S;

    /**
     * @brief 外设信息
     */
    typedef struct _Peripheral_S_
    {
    bool bEnable;                     /* 是否启用补光灯 */
    unsigned int nLevel;              /* 灯光等级 [0-100] */
    int nLightMode;                   /* 灯光模式,0-定时，1-自动 */
    Common::SchedTime_S stLightTime;  /* 灯光定时时间 */

    _Peripheral_S_():
    bEnable(true),
    nLevel(50),
    nLightMode(1)
    {

    }
    } Peripheral_S;

    /**
    * @brief 设备参数文件
    */
    typedef struct DeviceParamFile
    {
        std::string password;
        std::string path;
    } DeviceParamFile_S;

    /**
    * @brief 所有设备参数
    */
    typedef struct AllDeviceParam
    {
        std::string strPassword; /* 密码 */
        /* 预览 */
        Preview::PreviewInfo_S stPreviewInfo; /* 预览 */
        /* 回放 */
        /* 图片 */
        /* 配置 */
        /* 本地设置 */
        WebPlugin::Param_S stParam; /* 本地设置 */
        /* 系统配置 */
        System::DeviceConfig_S stDeviceConfig;             /* 系统设置-基本配置 */
        System::DeviceInfo_S stDeviceInfo;                 /* 系统设置-基本信息 */
        System::TimeInfo_S stTimeInfo;                     /* 系统设置-时间配置 */
                                                           /* 系统设置-智能资源分配 */
        System::UpgradeMaintain_S stUpgradeMaintain;       /* 系统维护-升级维护 */
        System::LogServerInfo_S stLogServerInfo;           /* 系统维护-安全审计日志 */
        System::SecurityCert_S stSecurityCert;             /* 安全管理-认证方式 */
        System::IpFilterConfigInfo_S stIpFilterConfigInfo; /* 安全管理-IP地址过滤 */
        System::SecurityServices_S stSecurityServices;     /* 安全管理-安全服务 */
                                                           /* 安全管理-客户端证书 */
                                                           /* 安全管理-CA证书 */
        std::vector<User::UserInfo_S> vecUserInfo;         /* 用户管理-用户管理 */
                                                           /* 用户管理-在线用户 */
        /* 网络配置 */
        Network::Info_S stNetInfo;                    /* 基本配置-TCP/IP */
        Network::PortConfig_S stPortConfig;           /* 基本配置-端口 */
        Network::PortMapConfig_S stPortMapConfig;     /* 基本配置-端口映射 */
        Network::SnmpConfig_S stSnmpConfig;           /* 高级配置-SNMP */
        Network::EmailInfo_S stEmailInfo;             /* 高级配置-Email */
        Network::GB28181Client_S stGB28181Client;     /* 高级配置-平台接入 */
        Network::GmCertFileInfo_S stGmCertFileInfo;   /* 高级配置-国标证书管理 */
        Network::HttpsConfigInfo_S stHttpsConfigInfo; /* 高级配置-HTTPS */
        Network::QosConfigInfo_S stQosConfigInfo;     /* 高级配置-Qos */
        Network::OnvifConfigInfo_S stOnvifConfigInfo; /* 高级配置-集成协议 */
                                                      /* 高级配置-网络服务 */
        /* 事件配置 */
        Alarm::MotionDetection_S stMotionDetection;       /* 普通事件-移动侦测 */
        Alarm::HideAlarm_S stHideAlarm;                   /* 普通事件-遮挡报警 */
        Alarm::AbnormalDetection_S stAbnormalDetection;   /* 普通事件-异常 */
        Alarm::SoundOutputAlarm_S stSoundOutputAlarm;     /* 普通事件-声音报警输出 */
        Alarm::IoInputInfo_S stIoInputInfo;               /* 普通事件-报警输入 */
        Alarm::IoOutputInfo_S stIoOutputInfo;             /* 普通事件-报警输出 */
        Alarm::FlashInfo_S stFlashInfo;                   /* 普通事件-闪光灯报警输出 */
        Alarm::PirAlarmInfo_S stPirAlarmInfo;             /* 普通事件-PIR报警 */
        Alarm::BoundaryDetection_S stBoundaryDetection;   /* 周界事件-越界侦测 */
        Alarm::FieldDetection_S stFieldDetection;         /* 周界事件-区域入侵侦测 */
        Alarm::EntranceDetection_S stEntranceDetection;   /* 周界事件-进入区域侦测 */
        Alarm::ExitingDetection_S stExitingDetection;     /* 周界事件-离开区域侦测 */
                                                          /* 场景智能-翻阅围栏识别 */
                                                          /* 场景智能-离岗识别 */
                                                          /* 场景智能-违规变道识别 */
                                                          /* 场景智能-逆行识别 */
                                                          /* 场景智能-非机动车闯入识别 */
                                                          /* 场景智能-应急车道占用识别 */
                                                          /* 场景智能-行人闯入识别 */
                                                          /* 场景智能-其他智能事件 */
        Alarm::AudioAnomaly_S stAudioAnomaly;             /* Smart事件-音频异常侦测 */
        Alarm::SceneChange_S stSceneChange;               /* Smart事件-场景变更侦测 */
        Alarm::FaceDetection_S stFaceDetection;           /* Smart事件-人脸侦测 */
        Alarm::LoiteringDetection_S stLoiteringDetection; /* Smart事件-徘徊侦测 */
        Alarm::CrowdGathering_S stCrowdGathering;         /* Smart事件-人员聚集侦测 */
        Alarm::ParkingDetection_S stParkingDetection;     /* Smart事件-停车侦测 */
        Alarm::UnattendedObject_S stUnattendedObject;     /* Smart事件-物品遗留侦测 */
        Alarm::ObjectRemoval_S stObjectRemoval;           /* Smart事件-物品拿取侦测 */
        Alarm::PetRecognition_S stPetRecognition;         /* Smart事件-宠物侦测 */
#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT
        Alarm::GarbageExposureDetection_S stGarbageExposure; /* Smart事件-垃圾暴露识别 */
        Alarm::GarbageOverflowDetection_S stGarbageOverflow; /* Smart事件-垃圾满溢识别 */
#endif

#if CAP_AI_PEOPLE_STATISTICS
        Alarm::PeopleFlowStatistics_S stPeopleFlowStatistics; /* Smart事件-人流统计 */
        Alarm::PeopleDensityDetection_S stPeopleDensityDetection; /* Smart事件-人员密度检测 */
#endif

        /* 存储管理 */
        Record_NS::Schedule_S stSchedule;                  /* 计划配置-录像计划 */
        Record_NS::AdvancedParam_S stAdvancedParamParam;   /* 计划配置-录像计划-录像参数 */
        Capture_NS::CapturePlan_S stCapturePlan;           /* 计划配置-抓图计划 */
        Capture_NS::CaptureParam_S stCaptureParam;         /* 计划配置-抓图计划-抓图参数 */
        StorageManage_NS::StorageManage_S stStorageManage; /* 存储管理-轻存储 */
        /* 视音频 */
        std::vector<Video_NS::VideoConfig_S> vecVideoConfig; /* 视频 */
        Audio_NS::AudioConfig_S stAudioConfig;               /* 音频 */
        std::vector<Video_NS::VideoRoiConfig_S> vecVideoRoi; /* ROI */
        std::vector<Video_NS::AreaCrop_S> vecAreaCrop;       /* 区域裁剪 */
        /* 人脸抓拍 */
        Alarm::FaceCapture_S stFaceCapture; /* 人脸抓拍 */
        /* 图像管理 */
        ISP::AllSceneParams_S stAllSceneParams; /* 显示设置 */
        Osd::OsdConfig_S stOsdConfig;           /* OSD设置 */
        Osd::CoverConfig_S stCoverConfig;       /* 视频遮盖 */
        ISP::SceneSchedule_S stSceneSchedule;   /* 图像参数切换 */
        /* 校验码 */
        std::string strCheckCode;
    } AllDeviceParam_S;
} // namespace System
