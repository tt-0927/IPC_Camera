/**
 * @FilePath     : isp_define.h
 * @Author       : cyc
 * @Date         : 2025-06-13 10:47:06
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-24 17:21:12
 * @Description  : 图像配置相关定义
 */

#pragma once
#include <string>
#include <vector>
#include <map>
#include "common_define.h"

/* normal_to_ir_iso_threshold 16000 */
#define NORMAL_TO_IR_RHRESHOLD_VALUE (16000)
/* ir_to_normal_iso_threshold 400 */
#define IR_TO_NORMAL_RHRESHOLD_VALUE (400)

/* isp用户参数值映射到系统值 */
#define MAP_USER_TO_SYSTEM(userValue, minVal, maxVal) \
    ({ \
        unsigned int _uv = (userValue > 100) ? 100 : userValue; \
        unsigned int _mapped = minVal + (_uv * (maxVal - minVal)) / 100; \
        (_mapped < minVal) ? minVal : ((_mapped > maxVal) ? maxVal : _mapped); \
    })
/* isp系统参数值映射到用户值 */
#define MAP_SYSTEM_TO_USER(systemValue, minVal, maxVal) \
    ({ \
        unsigned int _sv = (systemValue < minVal) ? minVal : ((systemValue > maxVal) ? maxVal : systemValue); \
        unsigned int _user = ((_sv - minVal) * 100) / (maxVal - minVal); \
        (_user > 100) ? 100 : _user; \
    })


/* 带偏移的用户值到系统值映射 */
#define MAP_USER_TO_SYSTEM_OFFSET(userValue, minVal, maxVal, offset) \
    ({ \
        int _adjusted = (userValue) + (offset); \
        unsigned int _uv = (_adjusted < 0) ? 0 : ((_adjusted > 100) ? 100 : _adjusted); \
        minVal + (_uv * (maxVal - minVal)) / 100; \
    })

/* 带偏移的系统值到用户值映射 */
#define MAP_SYSTEM_TO_USER_OFFSET(systemValue, minVal, maxVal, offset) \
    ({ \
        unsigned int _user = ((systemValue - minVal) * 100) / (maxVal - minVal); \
        int _adjusted = _user - (offset); \
        (_adjusted < 0) ? 0 : ((_adjusted > 100) ? 100 : _adjusted); \
    })

/* 灯光强度最小值 */
#define BRIGHT_INTENSITY_MIN (0)
/* 灯光强度最大 */
#define BRIGHT_INTENSITY_MAX (20000)
/* 灯光强度换算基数 */
#define BRIGHT_SWITCH_VALUE (200)
/* 获取灯光强度值 */
#define GET_BRIGHT_VALUE(x) (((x)-BRIGHT_INTENSITY_MIN)/BRIGHT_SWITCH_VALUE)
/* 设置灯光强度值 */
#define SET_BRIGHT_VALUE(x) (BRIGHT_INTENSITY_MIN+BRIGHT_SWITCH_VALUE*(x))

/* ==================== 红外灯专用亮度映射 ==================== */
/* 红外灯强度最小值（1，避免为0导致熄灭） */
#define IR_INTENSITY_MIN        (1)
/* 红外灯强度最大值（10400） */
#define IR_INTENSITY_MAX        (10400)
/* 红外灯换算基数（范围跨度 / 100级 = (10400-1)/100 ≈ 104） */
#define IR_SWITCH_VALUE         (104)

/* 红外灯：用户值[0-100] → PWM值[1-10400] */
#define SET_IR_BRIGHT_VALUE(userValue) \
    ({ \
        unsigned int _uv = (userValue > 100) ? 100 : userValue; \
        unsigned int _mapped = IR_INTENSITY_MIN + (_uv * IR_SWITCH_VALUE); \
        (_mapped > IR_INTENSITY_MAX) ? IR_INTENSITY_MAX : _mapped; \
    })

/* 红外灯：PWM值[1-10400] → 用户值[0-100] */
#define GET_IR_BRIGHT_VALUE(pwmValue) \
    ({ \
        unsigned int _pv = (pwmValue < IR_INTENSITY_MIN) ? IR_INTENSITY_MIN : \
                           (pwmValue > IR_INTENSITY_MAX) ? IR_INTENSITY_MAX : pwmValue; \
        unsigned int _user = ((_pv - IR_INTENSITY_MIN) * 100) / IR_SWITCH_VALUE; \
        (_user > 100) ? 100 : _user; \
    })
/* ========================================================= */

/* 日夜切换过滤时间最小值 */
#define FILTER_TIME_MIN    (5)
/* 日夜切换过滤时间最大值 */
#define FILTER_TIME_MAX    (120)
/* 一年的月份数 */
#define YEAE_MONTH (12)

const uint32_t exposureTimeMapping[] = 
{
    333333,   // 1/3秒 ≈ 333333微秒
    166667,   // 1/6秒 ≈ 166667微秒
    83333,    // 1/12秒 ≈ 83333微秒
    40000,    // 1/25秒 = 40000微秒
    20000,    // 1/50秒 = 20000微秒
    10000,    // 1/100秒 = 10000微秒
    6667,     // 1/150秒 ≈ 6667微秒
    5000,     // 1/200秒 = 5000微秒
    4000,     // 1/250秒 = 4000微秒
    2000,     // 1/500秒 = 2000微秒
    1333,     // 1/750秒 ≈ 1333微秒
    1000,     // 1/1000秒 = 1000微秒
    500,      // 1/2000秒 = 500微秒
    250,      // 1/4000秒 = 250微秒
    100,      // 1/10000秒 = 100微秒
    10        // 1/100000秒 = 10微秒
};


namespace ISP
{
     /// @brief 灯光类型
    typedef enum _LightType_E_
    {
        LIGHT_TYPE_WHITE,            /* 白光灯 */
        LIGHT_TYPE_RED,              /* 红外灯 */
        LIGHT_TYPE_SMART,            /* 智能补光 */
        LIGHT_TYPE_CLOSE,            /* 关闭 */
        LIGHT_TYPE_BOTH,             /* 白灯和红外同开关 */
        LIGHT_TYPE_RED_ON_WHITE_OFF, /* 红灯开、白灯关 */
        LIGHT_TYPE_WHITE_ON_RED_OFF  /* 白灯开、红灯关 */
    }LightType_E;

    /// @brief ir状态枚举
    typedef enum _Ir_Status_E_
    {
        IR_SWITCH_NORMAL = 0,
        NORMAL_SWITCH_IR
    }Ir_Status_E;

    /// @brief 日夜切换模式 
    typedef enum _DayNightMode_E_
    {
        DAY_MODE,     /* 白天模式 */
        NIGHT_MODE,   /* 夜晚模式 */
        AUTO_MODE,    /* 自动切换 */
        TIME_MODE     /* 定时切换 */
    }DayNightMode_E;

    /// @brief 白平衡模式模式 
    typedef enum _AwbMode_E_
    {
        AUTO_AWB_MODE,     /* 自动白平衡 */
        MANUAL_AWB_MODE,   /* 手动白平衡 */
        LOCK_AWB_MODE,     /* 锁定白平衡 */
        INCANDESCENT_MODE, /* 白炽灯 */
        WARM_MODE,         /* 暖光灯 */
        FLUORESCENT_MODE,  /* 日光灯 */
        DAY_LIGHT_MODE,    /* 自然灯 */
        /*RK1126B*/
        SEMI_AUTO_MODE,    /* 半自动模式 */
        REG_MANUAL_MODE,   /* 全手动模式 */
        INVAL_MODE         /* 无效值 */
    }AwbMode_E;

    /// @brief 数字降噪模式 
    typedef enum _DnrMode_E_
    {
        CLOSE_MODE,        /* 关闭 */
        NORMAL_MODE,       /* 普通模式 */
        ADVANCED_MODE,     /* 高级模式 */
    }DnrMode_E;

    /// @brief 曝光时间 
    typedef enum _ExpTimeMode_E_
    {
        One_3,    /* 1/3 */ 
        One_6,    /* 1/6 */ 
        One_12,   /* 1/12 */ 
        One_25,   /* 1/25 */ 
        One_50,   /* 1/50 */ 
        One_100,  /* 1/100 */
        One_150,  /* 1/150 */ 
        One_200,  /* 1/200 */ 
        One_250,  /* 1/250 */ 
        One_500,  /* 1/500 */ 
        One_750,  /* 1/750 */ 
        One_1000, /* 1/1000 */ 
        One_2000, /* 1/2000 */ 
        One_4000, /* 1/4000 */ 
        One_10000,/* 1/10000 */ 
        One_100000/* 1/100000 */ 
    }ExpTimeMode_E;

    /// @brief 背光补偿区域 
    typedef enum _BackLightArea_E_
    {
        CLOSE,            /* 关闭 */
        UP,               /* 上部 */
        DOWN,             /* 下部 */
        LEFT,             /* 左侧 */
        RIGHT,            /* 右侧 */
        CENTER_BACKLIGHT  /* 中心 */
    }BackLightArea_E;


    /// @brief 镜像模式 
    typedef enum _MirrorMode_E_
    {
        DISABLE,    /* 关闭 */
        HORIZONTAL, /* 左右 */
        VERTICAL,   /* 上下 */
        CENTER      /* 中心 */
    } MirrorMode_E;

    /// @brief 灯光模式
    typedef enum _LightBrightMode_E_
    {
        MANUAL_LIGHT_BRIGHT,  /* 手动调整灯光亮度 */
        AUTO_LIGHT_BRIGHT,    /* 自动调整灯光亮度 */
    }LightBrightMode_E;

     /// @brief 场景类型枚举
    typedef enum _SceneType_E_
    {
        SCENE_NORMAL,         /* 普通场景 */
        SCENE_FRONTLIGHT,     /* 顺光场景 */
        SCENE_BACKLIGHT,      /* 背光场景 */
        SCENE_LOWLIGHT,       /* 低光照场景 */
        SCENE_CUSTOM1,        /* 自定义1场景 */
        SCENE_CUSTOM2,        /* 自定义2场景 */
        SCENE_NIGHT,          /* 夜晚场景 */
        SCENE_NIGHT_LIGHT,    /* 夜晚白光场景 */
        SCENE_MAX
    } SceneType_E;


    /// @brief 月份枚举
    typedef enum class MonthOfYear
    {
        January = 1,    /* 一月 */
        February,       /* 二月 */
        March,          /* 三月 */
        April,          /* 四月 */
        May,            /* 五月 */
        June,           /* 六月 */
        July,           /* 七月 */
        August,         /* 八月 */
        September,      /* 九月 */
        October,        /* 十月 */
        November,       /* 十一月 */
        December        /* 十二月 */
    } MonthOfYear_E;

    /// @brief 图像配置类型
    typedef enum class _PicConfigureType_E_
    {
        SCENE,    /* 场景 */
        IAMGE,    /* 图像参数 */
        EXPOSURE, /* 曝光 */
        DAYNIGHT, /* 日夜切换 */
        BACKLIGHT,/* 背光 */
        AWB,      /* 白平衡 */
        NR,       /* 降噪 */
        MIRROR   /* 镜像 */
    } PicConfigureType_E;

    /// @brief 定时切换的开始跟结束时间 
    typedef struct _TimeRange_S_
    {
        Common::Time_S stStartTime;
        Common::Time_S stEndTime;
         /* 重载默认构造函数 */
        _TimeRange_S_()
            : stStartTime(),
              stEndTime()
        {
        }
        /* 重载运算符 */
        _TimeRange_S_ &operator=(const _TimeRange_S_ &x)
        {
            if (this != &x)
            {
                stStartTime = x.stStartTime;
                stEndTime = x.stEndTime;
            }
            return *this;
        }
    }TimeRange_S;

    /**
	 * @brief 场景时间段, 开始时间和结束时间
	 */
	typedef struct _SceneTime_S_
	{
        /* 场景类型 */
        SceneType_E enSceneType = SCENE_NORMAL;         
		/* 开始时间 */
		int nStartTime = 0;
		/* 结束时间 */
		int nEndTime = 24 * 60 * 60;
	} SceneTime_S;

    /**
	 * @brief 每月场景设置计划
	 */
	typedef struct _MonthSchedule_S_
	{
		/* 几月份  */
		MonthOfYear_E enMonthfYear = MonthOfYear_E::January;
		/* 时间段, 最多设置8段 */
		std::vector<SceneTime_S> aSceneTimes;
	} MonthSchedule_S;

    /**
	 * @brief 图像计划配置
	 */
    typedef struct _SceneSchedule_S_
    {
        bool bEnable = true;                   /* 是否启动计划图像配置 */
        /* 图像计划, 一月到十二月 */
		std::vector<MonthSchedule_S> aMonthSchedules;

        /* 默认构造函数 */
        _SceneSchedule_S_()
       : bEnable(false)
        {
            aMonthSchedules.clear();
        }

        /* 重载赋值运算符 */
        _SceneSchedule_S_ &operator=(const _SceneSchedule_S_ &x)
       {
           if (this != &x)
           {
               bEnable = x.bEnable;
               aMonthSchedules = x.aMonthSchedules;
           }
           return *this;
       }

            /* 静态方法：返回一个带有默认规则的对象 */
        static _SceneSchedule_S_ CreateWithDefaultRule()
        {
            _SceneSchedule_S_ obj;
            for(int i = 0;i < 12;i++)
            {
                MonthSchedule_S stMonthSchedule;
                stMonthSchedule.enMonthfYear = static_cast<MonthOfYear_E>(i + 1);
				stMonthSchedule.aSceneTimes.clear();;
                obj.aMonthSchedules.push_back(stMonthSchedule);
            }
            return obj;
        }

    }SceneSchedule_S;

    /// @brief 白平衡设置属性 
    typedef struct _AwbAttr_S_
    {
        AwbMode_E enAwbMode;    /* 白平衡模式 */
        unsigned int nRGain;    /* R增益,手动模式有效 */
        unsigned int nBGain;    /* B增益,手动模式有效 */

        _AwbAttr_S_():
        enAwbMode(AUTO_AWB_MODE),
        nRGain(50),
        nBGain(50)
        {

        }
    }AwbAttr_S;

    /// @brief 数字降噪属性 
    typedef struct _DnrAttr_S_
    {
        DnrMode_E enDnrMode;    /* 数字降噪模式 */
        unsigned int nDnrLevel;          /* 降噪等级，普通模式下有效 */
        unsigned int nSnrLevel;          /* 空域降噪等级，高级模式下有效 */
        unsigned int nTnrLevel;          /* 时域降噪等级，高级模式下有效 */

        _DnrAttr_S_():
        enDnrMode(NORMAL_MODE),
        nDnrLevel(50),
        nSnrLevel(50),
        nTnrLevel(50)
        {
        }

        /* 重载运算符 */
        _DnrAttr_S_ &operator=(const _DnrAttr_S_ &x)
        {
            if (this != &x)
            {
                enDnrMode = x.enDnrMode;
                nDnrLevel = x.nDnrLevel;
                nSnrLevel = x.nSnrLevel;
                nTnrLevel = x.nTnrLevel;
            }
            return *this;
        }
    }DnrAttr_S;

    /// @brief 曝光属性 
    typedef struct _ExposureAttr_S_
    {
        ExpTimeMode_E enExpTime; /* 曝光时间 */
        bool bAntiBanding;       /* 防横纹开关，ture-开启，false-关闭*/

        _ExposureAttr_S_()
        {
#if defined(DEVICE_TV_3882TI) || defined(DEVICE_TV_3881T)
            enExpTime = One_50;
#else
            enExpTime = One_12;
#endif
            bAntiBanding = false;
        }

        /* 重载运算符 */
        _ExposureAttr_S_ &operator=(const _ExposureAttr_S_ &x)
        {
            if (this != &x)
            {
                enExpTime = x.enExpTime;
                bAntiBanding = x.bAntiBanding;
            }
            return *this;
        }

    }ExposureAttr_S;

    /// @brief 宽动态属性 
    typedef struct _WdrAttr_S_
    {
        bool bEnable;   /* 是否启动宽动态，ture-开启，false-关闭 */
        int nWdrLevel;  /* 宽动态等级 */
        /* 重载默认构造函数 */
        _WdrAttr_S_()
        {
            bEnable = false;
            nWdrLevel = 12;
        }

         /* 重载运算符 */
        _WdrAttr_S_ &operator=(const _WdrAttr_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                nWdrLevel = x.nWdrLevel;
            }
            return *this;
        }

    }WdrAttr_S;

    /// @brief 强光抑制属性 
    typedef struct _HlsAttr_S_
    {
        bool bEnable;   /* 是否启动强光抑制，ture-开启，false-关闭 */
        int nHlsLevel;  /* 强光抑制等级 */
        /* 重载默认构造函数 */
        _HlsAttr_S_()
        {
            bEnable = false;
            nHlsLevel = 12;
        }

         /* 重载运算符 */
        _HlsAttr_S_ &operator=(const _HlsAttr_S_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                nHlsLevel = x.nHlsLevel;
            }
            return *this;
        }

    }HlsAttr_S;

    /// @brief 背光属性 
    typedef struct _BackLightArrt_S_
    {
        BackLightArea_E enBackLightArea; /* 背光补偿区域 */
        WdrAttr_S stWdrAttr;             /* 宽动态属性 */
        HlsAttr_S stHlsAttr;             /* 强光抑制属性 */

        _BackLightArrt_S_()
        {
            enBackLightArea = CLOSE;
        }

    }BackLightArrt_S;

    /// @brief 视频调整 
    typedef struct _VideoAdjust_S_
    {
        MirrorMode_E enMirrorMode;       /* 镜像模式 */

        _VideoAdjust_S_()
        {
            enMirrorMode = DISABLE;
        }

    }VideoAdjust_S;

    /// @brief 灯光属性 
    typedef struct _Light_S_
    {
        bool bEnable;                        /* 是否启动灯光，ture-开启，false-关闭 */
        int nLightLevel;                     /* 灯光等级,[0,100] */

        _Light_S_()
        {
            bEnable = false;
            nLightLevel = 50;
        }
    }Light_S;

    /// @brief 补光灯属性 
    typedef struct _FillLight_S_
    {
        LightType_E enLightType;   /* 灯光类型 */
        Light_S stWhiteAttr;       /* 白光灯属性 */
        Light_S stRedAttr;         /* 红外灯属性 */
        
        _FillLight_S_():
        enLightType(LIGHT_TYPE_WHITE)
        {

        }
    }FillLight_S;

    /// @brief 日夜切换 
    typedef struct _DayNightAttr_S_
    {
        DayNightMode_E enDayNightMode;   /* 日夜切换模式 */
        Common::Time_S stBeginTime;      /* 开始时间,定时模式下有效 */
        Common::Time_S stEndTime;        /* 结束时间,定时模式下有效 */
        unsigned int nSensitivityLevel;  /* 灵敏度等级，自动模式下有效,[1,7] */
        unsigned int nFilterTime;        /* 过滤时间，自动模式下有效,[5,120] */
        bool bFillLightExp;              /* 防补光过曝，ture-开启，false-关闭 */
        LightBrightMode_E enLightMode;   /* 灯光亮度模式 */
        FillLight_S stFillLight;         /* 补光灯 */

        _DayNightAttr_S_():
        enDayNightMode(AUTO_MODE),
        nSensitivityLevel(2),
        nFilterTime(15),
        bFillLightExp(false),
        enLightMode(AUTO_LIGHT_BRIGHT)
        {   

        }

         /* 重载运算符 */
         _DayNightAttr_S_ &operator=(const _DayNightAttr_S_ &x)
         {
             if (this != &x)
             {
                enDayNightMode = x.enDayNightMode;
                stBeginTime = x.stBeginTime;
                stEndTime = x.stEndTime;
                nSensitivityLevel = x.nSensitivityLevel;
                nFilterTime = x.nFilterTime;
                bFillLightExp = x.bFillLightExp;
                enLightMode = x.enLightMode;
                stFillLight = x.stFillLight;
             }
             return *this;
         }
    }DayNightAttr_S;

    /// @brief 图像参数数据结构
    typedef struct _ImageParam_S_
    {
        unsigned int nBrightness; /* 亮度[0,100] */
        unsigned int nContrast;   /* 对比度[0,100] */
        unsigned int nSaturation; /* 饱和度[0,100] */
        unsigned int nSharpness;  /* 锐度[0,100] */

        _ImageParam_S_() : nContrast(50), nSaturation(50), nSharpness(50)
        {
#ifdef DEVICE_TV_3882TI
            nBrightness = 32;
#elif DEVICE_TV_3881T
            nBrightness = 30;
#else
            nBrightness = 50;
#endif
        }

        /* 带参数的构造函数，支持传入自定义值 */
        _ImageParam_S_(unsigned int brightness, unsigned int contrast, unsigned int saturation, unsigned int sharpness)
            : nBrightness(brightness), nContrast(contrast), nSaturation(saturation), nSharpness(sharpness)
        {
        }

        /* 重载运算符 */
        _ImageParam_S_ &operator=(const _ImageParam_S_ &x)
        {
            if (this != &x)
            {
                nBrightness = x.nBrightness;
                nContrast = x.nContrast;
                nSaturation = x.nSaturation;
                nSharpness = x.nSharpness;
            }
            return *this;
        }

    } ImageParam_S;

    /// @brief 场景参数
    typedef struct _SceneParams_S_
    {
        SceneType_E enSceneType;         /* 场景类型 */
        ImageParam_S stImageParam;       /* 图像调节 */
        AwbAttr_S stAwbAttr;             /* 白平衡 */
        DnrAttr_S stDnrAttr;             /* 降噪 */
        ExposureAttr_S stExpAttr;        /* 曝光 */
        BackLightArrt_S stBackLightAttr; /* 背光 */
        DayNightAttr_S stDayNightAttr;   /* 日夜切换 */

         /* 普通 */
        static _SceneParams_S_ defaultNormalScene()
        {
            _SceneParams_S_ stInfo;
            stInfo.enSceneType = SCENE_NORMAL;
            stInfo.stImageParam = ImageParam_S{};
            stInfo.stAwbAttr = AwbAttr_S{};           /* 初始化白平衡属性 */ 
            stInfo.stDnrAttr = DnrAttr_S{};           /* 初始化降噪属性 */ 
            stInfo.stExpAttr = ExposureAttr_S{};      /* 初始化曝光属性 */ 
            stInfo.stBackLightAttr = BackLightArrt_S{}; /* 初始化背光属性 */ 
            stInfo.stDayNightAttr = DayNightAttr_S{};  /* 初始化日夜切换属性 */ 

            return stInfo;
        }
         /* 顺光 */
        static _SceneParams_S_ defaultFrontLightScene()
        {
            _SceneParams_S_ stInfo;
            stInfo.enSceneType = SCENE_FRONTLIGHT;
            stInfo.stImageParam.nBrightness = 52;
            stInfo.stImageParam.nContrast = 53;
            stInfo.stImageParam.nSaturation = 55;
            stInfo.stImageParam.nSharpness = 50;
            stInfo.stAwbAttr = AwbAttr_S{};           /* 初始化白平衡属性 */ 
            stInfo.stDnrAttr = DnrAttr_S{};           /* 初始化降噪属性 */ 
            stInfo.stExpAttr.enExpTime = One_250;        /* 初始化曝光属性 */ 
            stInfo.stBackLightAttr = BackLightArrt_S{}; /* 初始化背光属性 */ 
            stInfo.stDayNightAttr = DayNightAttr_S{};  /* 初始化日夜切换属性 */ 
 
            return stInfo;
        }
 
         /* 背光 */
        static _SceneParams_S_ defaultBackLightScene()
        {
            _SceneParams_S_ stInfo;
            stInfo.enSceneType = SCENE_BACKLIGHT;
            stInfo.stImageParam.nBrightness = 55;
            stInfo.stImageParam.nContrast = 45;
            stInfo.stImageParam.nSaturation = 55;
            stInfo.stImageParam.nSharpness = 45;
            stInfo.stAwbAttr = AwbAttr_S{};           /* 初始化白平衡属性 */ 
            stInfo.stDnrAttr = DnrAttr_S{};           /* 初始化降噪属性 */ 
            stInfo.stExpAttr.enExpTime = One_200;      /* 初始化曝光属性 */ 
            stInfo.stBackLightAttr.enBackLightArea = CENTER_BACKLIGHT; /* 初始化背光属性 */ 
            stInfo.stDayNightAttr = DayNightAttr_S{};  /* 初始化日夜切换属性 */ 
            
            return stInfo;
        }
 
        /* 低光照 */
        static _SceneParams_S_ defaultLowlightScene()
        {
            _SceneParams_S_ stInfo;
            stInfo.enSceneType = SCENE_LOWLIGHT;
            stInfo.stImageParam.nBrightness = 60;
            stInfo.stImageParam.nContrast = 52;
            stInfo.stImageParam.nSaturation = 53;
            stInfo.stImageParam.nSharpness = 53;
            stInfo.stAwbAttr = AwbAttr_S{};           /* 初始化白平衡属性 */ 
            stInfo.stDnrAttr = DnrAttr_S{};           /* 初始化降噪属性 */ 
            stInfo.stExpAttr.enExpTime = One_50;      /* 初始化曝光属性 */ 
            stInfo.stBackLightAttr = BackLightArrt_S{}; /* 初始化背光属性 */ 
            stInfo.stDayNightAttr = DayNightAttr_S{};  /* 初始化日夜切换属性 */ 
             
            return stInfo;
        }
 
         /* 自定义1 */
        static _SceneParams_S_ defaultCustom1Scene()
         {
            _SceneParams_S_ stInfo;
            stInfo.enSceneType = SCENE_CUSTOM1;
            stInfo.stImageParam = ImageParam_S{};
            stInfo.stAwbAttr = AwbAttr_S{};           /* 初始化白平衡属性 */ 
            stInfo.stDnrAttr = DnrAttr_S{};           /* 初始化降噪属性 */ 
            stInfo.stExpAttr = ExposureAttr_S{};      /* 初始化曝光属性 */ 
            stInfo.stBackLightAttr = BackLightArrt_S{}; /* 初始化背光属性 */ 
            stInfo.stDayNightAttr = DayNightAttr_S{};  /* 初始化日夜切换属性 */ 
             
            return stInfo;
         }
 
         /* 自定义2 */
        static _SceneParams_S_ defaultCustom2Scene()
        {
            _SceneParams_S_ stInfo;
            stInfo.enSceneType = SCENE_CUSTOM2;
             
            stInfo.stImageParam = ImageParam_S{};
            stInfo.stAwbAttr = AwbAttr_S{};           /* 初始化白平衡属性 */ 
            stInfo.stDnrAttr = DnrAttr_S{};           /* 初始化降噪属性 */ 
            stInfo.stExpAttr = ExposureAttr_S{};      /* 初始化曝光属性 */ 
            stInfo.stBackLightAttr = BackLightArrt_S{}; /* 初始化背光属性 */ 
            stInfo.stDayNightAttr = DayNightAttr_S{};  /* 初始化日夜切换属性 */ 
            return stInfo;
        }

        bool operator<(const _SceneParams_S_ &other) const
        {
            return enSceneType < other.enSceneType;
        }

    } SceneParams_S;

     /// @brief 场景参数
     typedef struct _AllSceneParams_S_
     {

        SceneType_E enCurrentScene = SCENE_NORMAL;  /* 当前场景 */
        std::vector<SceneParams_S> aSceneParams; /* 所有场景参数 */
 
         _AllSceneParams_S_()
         {
            enCurrentScene = SCENE_NORMAL;
            aSceneParams.clear();
         }


        /* 静态方法：返回一个带有默认规则的对象 */
        static _AllSceneParams_S_ CreateWithDefaultRule()
        {
            _AllSceneParams_S_ obj;
            obj.aSceneParams.clear();
            obj.aSceneParams.resize(SCENE_MAX);
            obj.enCurrentScene = SCENE_NORMAL;
            obj.aSceneParams[SCENE_NORMAL] = SceneParams_S::defaultNormalScene();
            obj.aSceneParams[SCENE_FRONTLIGHT] = SceneParams_S::defaultFrontLightScene();
            obj.aSceneParams[SCENE_BACKLIGHT] = SceneParams_S::defaultBackLightScene();
            obj.aSceneParams[SCENE_LOWLIGHT] = SceneParams_S::defaultLowlightScene();
            obj.aSceneParams[SCENE_CUSTOM1] = SceneParams_S::defaultCustom1Scene();
            obj.aSceneParams[SCENE_CUSTOM2] = SceneParams_S::defaultCustom2Scene();
            return obj;
        }

         /* 根据场景类型获取场景参数 */ 
        SceneParams_S& getSceneParams(SceneType_E type) 
        {
            if (type >= 0 && type < SCENE_MAX  && type < aSceneParams.size()) 
            {
                return aSceneParams[type];
            }
            /* 返回当前场景参数或默认场景参数 */ 
            return aSceneParams[enCurrentScene];
        }

        /**
        * @brief 恢复指定场景类型的默认参数
        * @param sceneType 要恢复的场景类型
        * @return true 成功, false 失败
        */
        bool restoreSceneToDefault(SceneType_E sceneType)
        {
            if (sceneType < SCENE_NORMAL || sceneType >= SCENE_MAX) 
            {
                return false;
            }
            
            if (sceneType >= aSceneParams.size()) {
                return false;
            }
            switch (sceneType)
            {
                case SCENE_NORMAL:
                    aSceneParams[SCENE_NORMAL] = SceneParams_S::defaultNormalScene();
                    break;
                case SCENE_FRONTLIGHT:
                    aSceneParams[SCENE_FRONTLIGHT] = SceneParams_S::defaultFrontLightScene();
                    break;
                case SCENE_BACKLIGHT:
                    aSceneParams[SCENE_BACKLIGHT] = SceneParams_S::defaultBackLightScene();
                    break;
                case SCENE_LOWLIGHT:
                    aSceneParams[SCENE_LOWLIGHT] = SceneParams_S::defaultLowlightScene();
                    break;
                case SCENE_CUSTOM1:
                    aSceneParams[SCENE_CUSTOM1] = SceneParams_S::defaultCustom1Scene();
                    break;
                case SCENE_CUSTOM2:
                    aSceneParams[SCENE_CUSTOM2] = SceneParams_S::defaultCustom2Scene();
                    break;
                default:
                    return false;
            }
            return true;
        }


     } AllSceneParams_S;

}; // namespace ISP
