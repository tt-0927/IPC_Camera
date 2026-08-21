/**
 * @FilePath     : isp_tuning_profile.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-10 15:17:16
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-27 17:42:05
 * @Description  : Hi3516 ISP参数映射与运行策略数据结构
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "ot_common_isp.h"

/* ISP差异化调参场景，作为三场景档位数组的索引。 */
enum class IspTuningScene_E
{
    DAY = 0,     /* 白天 */
    NIGHT_WHITE, /* 夜晚白光 */
    NIGHT_IR,    /* 夜晚红外 */
    COUNT        /* 内部调参场景数量，不是可切换场景 */
};

/* 内部调参场景数量，所有按场景保存的配置必须以此为唯一维度。 */
constexpr std::size_t ISP_TUNING_SCENE_COUNT = static_cast<std::size_t>(IspTuningScene_E::COUNT);
/* MPP曝光时间枚举对应的固定补偿档位数量。 */
constexpr std::size_t ISP_EXPOSURE_COMPENSATION_LEVEL_COUNT = 16;

/* 锐度调参档位。 */
struct SharpenTuningProfile_S
{
    unsigned int nMin; /* 用户值映射系统值的下限 */
    unsigned int nMax; /* 用户值映射系统值的上限 */
    int nOffset;       /* 用户值到系统值的偏移 */
    bool bUseAutoMode; /* true：自动锐度；false：手动锐度 */
};

/* 网页参数到MPP参数值的线性映射。 */
struct IspParamMapping_S
{
    unsigned int nSystemMin; /* MPP参数值下限 */
    unsigned int nSystemMax; /* MPP参数值上限 */
    int nUserOffset;         /* 映射前叠加到网页用户值的偏移 */
};

/* 网页百分比到MPP整数参数值的缩放映射，不在偏移后再次截断。 */
struct ScaledOffsetMapping_S
{
    int nUserOffset;          /* 网页值限制到0～100后叠加的偏移 */
    unsigned int nMultiplier; /* 乘数 */
    unsigned int nDivisor;    /* 除数，必须非零 */
};

/* 强光抑制使用的反向网页映射与关闭值。 */
struct HlcTuningProfile_S
{
    unsigned int nSystemMin;     /* MPP tolerance下限，对应网页100 */
    unsigned int nSystemMax;     /* MPP tolerance上限，对应网页0 */
    unsigned int nDisabledValue; /* 功能关闭时写入的MPP默认值 */
};

/* AWB预设模式的一组MPP手动白平衡增益。 */
struct AwbPresetGain_S
{
    unsigned int nRGain;  /* R通道增益 */
    unsigned int nGrGain; /* Gr通道增益 */
    unsigned int nGbGain; /* Gb通道增益 */
    unsigned int nBGain;  /* B通道增益 */
};

/* AWB网页手动增益和五个预设模式的映射。 */
struct AwbTuningProfile_S
{
    IspParamMapping_S stManualGain; /* 网页R/B手动增益映射 */
    AwbPresetGain_S stLock;         /* 锁定白平衡 */
    AwbPresetGain_S stIncandescent; /* 白炽灯 */
    AwbPresetGain_S stWarm;         /* 暖光灯 */
    AwbPresetGain_S stFluorescent;  /* 日光灯 */
    AwbPresetGain_S stDaylight;     /* 自然灯 */
};

/* 网页降噪等级的MPP缩放规则。 */
struct NrTuningProfile_S
{
    ScaledOffsetMapping_S stSnr; /* 空域降噪强度 */
    ScaledOffsetMapping_S stTnr; /* 时域降噪强度 */
};

/* 宽动态调参档位，与通用线性映射使用相同数据结构。 */
using WdrTuningProfile_S = IspParamMapping_S;

/* 亮度调参档位，与通用线性映射使用相同数据结构。 */
using BrightnessTuningProfile_S = IspParamMapping_S;

/* 单个内部调参场景使用的全部网页参数映射。 */
struct IspSceneParamMapping_S
{
    BrightnessTuningProfile_S stBrightness; /* 亮度映射 */
    SharpenTuningProfile_S stSharpen;       /* 锐度映射和工作模式 */
    WdrTuningProfile_S stWdr;               /* WDR强度映射 */
    IspParamMapping_S stContrast;           /* 对比度映射 */
    IspParamMapping_S stSaturation;         /* 饱和度映射 */
    HlcTuningProfile_S stHlc;               /* 强光抑制映射 */
    AwbTuningProfile_S stAwb;               /* 白平衡映射和预设 */
    NrTuningProfile_S stNr;                 /* 降噪强度映射 */
};

/* 夜间自动切白天的焦距/机型相关阈值。 */
struct DayNightThreshProfile_S
{
    uint64_t u64WhiteLightToDayBright; /* 夜间白灯切白天亮度阈值 */
    uint64_t u64RedLightToDayBright;   /* 夜间红外切白天亮度阈值 */
    unsigned int nWhiteRg;             /* 夜间白灯切白天RG阈值 */
    unsigned int nWhiteBg;             /* 夜间白灯切白天BG阈值 */
    unsigned int nRedRg;               /* 夜间红外切白天RG阈值 */
    unsigned int nRedBg;               /* 夜间红外切白天BG阈值 */
};

/* 镜像方向策略。 */
struct MirrorPolicy_S
{
    bool bCeilingMount; /* true：吸顶机型，镜像/翻转方向取反 */
};

/* 单个MPP Scene索引应用后的DRC覆盖策略。 */
struct SceneDrcAdjustment_S
{
    bool bOverride;          /* true：Scene生效后覆盖DRC，false：保留IQ配置 */
    bool bEnable;            /* bOverride为true时的DRC使能状态 */
    bool bUseManualStrength; /* true：同时覆盖手动DRC强度 */
    unsigned int nStrength;  /* 手动DRC强度，仅bUseManualStrength为true时有效 */
};

/* 三个内部运行场景的DRC覆盖策略。 */
struct SceneDrcPolicy_S
{
    SceneDrcAdjustment_S stDay;        /* 白天Scene应用后的DRC策略 */
    SceneDrcAdjustment_S stNightWhite; /* 夜晚白光Scene应用后的DRC策略 */
    SceneDrcAdjustment_S stNightIr;    /* 夜晚红外Scene应用后的DRC策略 */
};

/* Hi3516参数映射与运行策略快照，由配置加载器一次创建，device层只读。 */
struct Hi3516TuningProfile_S
{
    /* ISP启动使用的传感器类型值，由builder按编译期Sensor填充。 */
    int nSensorType;

    /* 所有网页参数映射均按IspTuningScene_E索引，切换场景时由重放链路选用对应槽位。 */
    std::array<IspSceneParamMapping_S, ISP_TUNING_SCENE_COUNT> stSceneParamMappings;
    /* 三场景曝光补偿数组，按IspTuningScene_E索引，每套16级。 */
    std::array<std::array<int, ISP_EXPOSURE_COMPENSATION_LEVEL_COUNT>, ISP_TUNING_SCENE_COUNT> stExposureCompensation;

    /* Gamma能力与曲线选择，曲线指针由builder填充。 */
    bool bUseCmosGamma;
    const ot_isp_gamma_attr *pGammaDay;
    const ot_isp_gamma_attr *pGammaIr;

    /* 日夜切白天阈值。 */
    DayNightThreshProfile_S stDayNightThresh;

    /* 三个内部运行场景到MPP Scene video_mode的索引映射。 */
    int nDaySceneIndex;
    int nNightWhiteSceneIndex;
    int nNightIrSceneIndex;
    /* Scene生效后的DRC覆盖策略。 */
    SceneDrcPolicy_S stSceneDrc;

    /* 镜像策略。 */
    MirrorPolicy_S stMirror;

    /**
     * @brief   : 构造安全默认调参数据
     * @return   {void}
     * @note    : 默认不应用Gamma且不覆盖Scene DRC；其余字段必须由配置加载器填写
     */
    Hi3516TuningProfile_S()
        : nSensorType(-1), stSceneParamMappings{}, stExposureCompensation{}, bUseCmosGamma(false), pGammaDay(nullptr),
          pGammaIr(nullptr), stDayNightThresh{}, nDaySceneIndex(0), nNightWhiteSceneIndex(0), nNightIrSceneIndex(1), stSceneDrc{},
          stMirror{}
    {
    }
};
