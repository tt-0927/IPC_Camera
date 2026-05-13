/**
 * @FilePath     : isp_control.cpp
 * @Author       : cyc
 * @Date         : 2025-06-05 10:16:15
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-07 11:34:51
 * @Description  : isp参数控制模块
 */

#include <unistd.h>
#include <iomanip>
#include "isp_control.h"
#include "ss_mpi_isp.h"
#include "ot_mpi_ae.h"
#include "ss_mpi_awb.h"
#include "IpcRet.h"
#include "sample_comm.h"
#include "gpio_ctrl.h"
#include "pwm_ctrl.h"
#include "dlog.h"
#include "ot_mpi_isp.h"
#include "isp_configure.h"
#include "isp_scene.h"
#include "isp_manage.h"
#include "stream_video.h"
#if CAP_ISP_DAY_NIGHT_DIFFERENT_TUNING && defined(SENSOR_SC533HAI)
#include "sc533hai_cmos.h"
#endif

namespace
{
#if CAP_ISP_DAY_NIGHT_DIFFERENT_TUNING
/* ISP 差异化调参场景：白天、夜晚白光、夜晚红外。 */
enum class IspTuningScene_E
{
    DAY = 0,
    NIGHT_WHITE,
    NIGHT_IR
};

/* 锐度调参档位描述。 */
struct SharpenTuningProfile_S
{
    unsigned int min_value;
    unsigned int max_value;
    int offset;
    bool use_auto_mode;
};

/* 宽动态调参档位描述。 */
struct WdrTuningProfile_S
{
    unsigned int min_value;
    unsigned int max_value;
    int offset;
};

/* 亮度调参档位描述。 */
struct BrightnessTuningProfile_S
{
    unsigned int min_value;
    unsigned int max_value;
    int offset;
};

/* 判断当前补光类型是否应归类为夜晚白光场景。 */
bool is_white_light_type(const LightType_E en_light_type)
{
    return en_light_type == LightType_E::LIGHT_TYPE_WHITE || en_light_type == LightType_E::LIGHT_TYPE_WHITE_ON_RED_OFF;
}

/* 获取当前 ISP 调参场景。 */
IspTuningScene_E get_isp_tuning_scene()
{
    if (!CDayNightController::instance()->isNightMode())
    {
        return IspTuningScene_E::DAY;
    }

    ISP::DayNightAttr_S stDayNightAttr;
    CIspConfigure::instance()->get_configure(stDayNightAttr);

    if (is_white_light_type(stDayNightAttr.stFillLight.enLightType))
    {
        return IspTuningScene_E::NIGHT_WHITE;
    }

    return IspTuningScene_E::NIGHT_IR;
}

/* 获取当前场景对应的锐度调参档位。 */
const SharpenTuningProfile_S &get_sharpen_tuning_profile(const IspTuningScene_E tuning_scene)
{
    /* 白天锐度档位，保持现有手动模式映射。 */
    static const SharpenTuningProfile_S k_day_profile = {
        SHARPEN_MIN_VALUE,
        SHARPEN_MAX_VALUE,
        SHARPEN_OFFSET,
        false
    };

    /* 夜晚白光锐度档位，当前与白天一致，后续可独立调整。 */
    static const SharpenTuningProfile_S k_night_white_profile = {
        SHARPEN_NIGHT_WHITE_MIN_VALUE,
        SHARPEN_NIGHT_WHITE_MAX_VALUE,
        SHARPEN_NIGHT_WHITE_OFFSET,
        false
    };

    /* 夜晚红外锐度档位，沿用现有夜间自动锐度策略。 */
    static const SharpenTuningProfile_S k_night_ir_profile = {
        SHARPEN_NIGHT_IR_MIN_VALUE,
        SHARPEN_NIGHT_IR_MAX_VALUE,
        SHARPEN_NIGHT_IR_OFFSET,
        true
    };

    switch (tuning_scene)
    {
        case IspTuningScene_E::DAY:
            return k_day_profile;
        case IspTuningScene_E::NIGHT_WHITE:
            return k_night_white_profile;
        case IspTuningScene_E::NIGHT_IR:
        default:
            return k_night_ir_profile;
    }
}

/* 获取当前场景对应的宽动态调参档位。 */
const WdrTuningProfile_S &get_wdr_tuning_profile(const IspTuningScene_E tuning_scene)
{
    /* 白天宽动态档位。 */
    static const WdrTuningProfile_S k_day_profile = {
        WDR_DAY_MIN_VALUE,
        WDR_DAY_MAX_VALUE,
        WDR_DAY_OFFSET
    };

    /* 夜晚白光宽动态档位，当前与白天一致，后续可独立调整。 */
    static const WdrTuningProfile_S k_night_white_profile = {
        WDR_NIGHT_WHITE_MIN_VALUE,
        WDR_NIGHT_WHITE_MAX_VALUE,
        WDR_NIGHT_WHITE_OFFSET
    };

    /* 夜晚红外宽动态档位，沿用现有夜间映射。 */
    static const WdrTuningProfile_S k_night_ir_profile = {
        WDR_NIGHT_IR_MIN_VALUE,
        WDR_NIGHT_IR_MAX_VALUE,
        WDR_NIGHT_IR_OFFSET
    };

    switch (tuning_scene)
    {
        case IspTuningScene_E::DAY:
            return k_day_profile;
        case IspTuningScene_E::NIGHT_WHITE:
            return k_night_white_profile;
        case IspTuningScene_E::NIGHT_IR:
        default:
            return k_night_ir_profile;
    }
}

/* 获取当前场景对应的亮度调参档位。 */
const BrightnessTuningProfile_S &get_brightness_tuning_profile(const IspTuningScene_E tuning_scene)
{
    /* 白天亮度档位。 */
    static const BrightnessTuningProfile_S k_day_profile = {
        BRIGHT_MIN_VALUE,
        BRIGHT_MAX_VALUE,
        BRIGHT_OFFSET
    };

    /* 夜晚白光亮度档位。 */
    static const BrightnessTuningProfile_S k_night_white_profile = {
        BRIGHT_NIGHT_WHITE_MIN_VALUE,
        BRIGHT_NIGHT_WHITE_MAX_VALUE,
        BRIGHT_NIGHT_WHITE_OFFSET
    };

    /* 夜晚红外亮度档位，当前与白天一致，后续可独立调整。 */
    static const BrightnessTuningProfile_S k_night_ir_profile = {
        BRIGHT_MIN_VALUE,
        BRIGHT_MAX_VALUE,
        BRIGHT_OFFSET
    };

    switch (tuning_scene)
    {
        case IspTuningScene_E::DAY:
            return k_day_profile;
        case IspTuningScene_E::NIGHT_WHITE:
            return k_night_white_profile;
        case IspTuningScene_E::NIGHT_IR:
        default:
            return k_night_ir_profile;
    }
}

/* 获取当前场景对应的曝光补偿数组。 */
const int *get_exposure_compensation_array(const IspTuningScene_E tuning_scene)
{
    switch (tuning_scene)
    {
        case IspTuningScene_E::DAY:
            return EXPOSURE_COMPENSATION_DAY_ARRAY;
        case IspTuningScene_E::NIGHT_WHITE:
            return EXPOSURE_COMPENSATION_NIGHT_WHITE_ARRAY;
        case IspTuningScene_E::NIGHT_IR:
        default:
            return EXPOSURE_COMPENSATION_NIGHT_IR_ARRAY;
    }
}

#ifdef SENSOR_SC533HAI
/* 获取当前场景对应的 Gamma 曲线，白天和夜晚白光共用默认曲线，夜晚红外使用 3852H 曲线。 */
const ot_isp_gamma_attr &get_gamma_attr_by_tuning_scene(const IspTuningScene_E tuning_scene)
{
    switch (tuning_scene)
    {
        case IspTuningScene_E::DAY:
        case IspTuningScene_E::NIGHT_WHITE:
            return *sc533hai_get_cmos_gamma();
        case IspTuningScene_E::NIGHT_IR:
        default:
            return *sc533hai_get_cmos_gamma_ir();
    }
}
#endif
#endif
} // namespace

CIspControl::CIspControl()
{

}

CIspControl::~CIspControl()
{

}

int CIspControl::init()
{
    int nRet = IpcRet_E::OK;
    m_viPipe = 0; 
    sample_vi_cfg stViCfg;
    sample_sns_type stSnstype = SC500AI_MIPI_5M_30FPS_10BIT;
    #ifdef SENSOR_SC533HAI
        stSnstype = SC533HAI_MIPI_5M_30FPS_10BIT;
    #elif defined SENSOR_SC500AI 
        stSnstype = SC500AI_MIPI_5M_30FPS_10BIT;
    #endif
    /*获取VI模块vi_cfg初始值*/
    sample_comm_vi_get_default_vi_cfg(stSnstype, &stViCfg);
    stViCfg.pipe_info[0].pipe_attr.isp_bypass = TD_FALSE;
    
    /*先停止isp，避免前次未stop isp导致的start isp失败*/
    sample_comm_vi_stop_isp(&stViCfg);
    /*启动isp*/
    sample_comm_vi_start_isp(&stViCfg);
    
    m_isInitialized.store(true);
    dlog_info("ISP Control module initialized successfully");
    
    return nRet;
}

int CIspControl::deinit()
{
    int nRet = IpcRet_E::OK;
    sample_vi_cfg stViCfg;
    sample_sns_type stSnstype = SC500AI_MIPI_5M_30FPS_10BIT;
    #ifdef SENSOR_SC533HAI
        stSnstype = SC533HAI_MIPI_5M_30FPS_10BIT;
    #elif defined SENSOR_SC500AI 
        stSnstype = SC500AI_MIPI_5M_30FPS_10BIT;
    #endif
    
    m_isInitialized.store(false);
   
    /*获取VI模块vi_cfg初始值*/
    sample_comm_vi_get_default_vi_cfg(stSnstype, &stViCfg);
    /*停止isp*/
    sample_comm_vi_stop_isp(&stViCfg);
    dlog_info("ISP Control module deinitialized");
    return nRet;
}


/**************************图像参数控制 ************************/

int CIspControl::set_saturation(const unsigned int nSaturation)
{
    int nRet = IpcRet_E::OK;
    ot_isp_csc_attr stCscAttr;

    nRet = ss_mpi_isp_get_csc_attr(m_viPipe,&stCscAttr);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("ss_mpi_isp_get_csc_attr error");
        return IpcRet_E::ERR;
    }

    stCscAttr.satu = nSaturation;

    nRet = ss_mpi_isp_set_csc_attr(m_viPipe,&stCscAttr);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("ss_mpi_isp_set_csc_attr error");
        return IpcRet_E::ERR;
    }
    
    return nRet;
}

int CIspControl::get_saturation(unsigned int &pSaturation) const
{
    int nRet = IpcRet_E::OK;
    ot_isp_csc_attr stCscAttr;

    nRet = ss_mpi_isp_get_csc_attr(m_viPipe,&stCscAttr);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("ss_mpi_isp_get_csc_attr error");
        return IpcRet_E::ERR;
    }

    pSaturation = stCscAttr.satu;
    
    return nRet;
}


int CIspControl::set_brightness(const unsigned int nBrightness)
{
    int nRet = IpcRet_E::OK;
    ot_isp_csc_attr stCscAttr;

    nRet = ss_mpi_isp_get_csc_attr(m_viPipe,&stCscAttr);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("ss_mpi_isp_get_csc_attr error");
        return IpcRet_E::ERR;
    }

#if CAP_ISP_DAY_NIGHT_DIFFERENT_TUNING
    const IspTuningScene_E tuningScene = get_isp_tuning_scene();
    const BrightnessTuningProfile_S &tuningProfile = get_brightness_tuning_profile(tuningScene);
    unsigned int mappedBrightness = MAP_USER_TO_SYSTEM_OFFSET(nBrightness,
                                                              tuningProfile.min_value,
                                                              tuningProfile.max_value,
                                                              tuningProfile.offset);
#else
    unsigned int mappedBrightness = MAP_USER_TO_SYSTEM_OFFSET(nBrightness, BRIGHT_MIN_VALUE, BRIGHT_MAX_VALUE, BRIGHT_OFFSET);
#endif
    /* 整体亮度 */
    stCscAttr.luma = mappedBrightness;
    nRet = ss_mpi_isp_set_csc_attr(m_viPipe,&stCscAttr);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("ss_mpi_isp_set_csc_attr error");
        return IpcRet_E::ERR;
    }

    return nRet; 
}


int CIspControl::get_brightness(unsigned int &pBrightness) const
{
    int nRet = IpcRet_E::OK;
    ot_isp_csc_attr stCscAttr;

    nRet = ss_mpi_isp_get_csc_attr(m_viPipe,&stCscAttr);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("ss_mpi_isp_get_csc_attr error");
        return IpcRet_E::ERR;
    }

#if CAP_ISP_DAY_NIGHT_DIFFERENT_TUNING
    const IspTuningScene_E tuningScene = get_isp_tuning_scene();
    const BrightnessTuningProfile_S &tuningProfile = get_brightness_tuning_profile(tuningScene);
    pBrightness = MAP_SYSTEM_TO_USER_OFFSET(stCscAttr.luma,
                                            tuningProfile.min_value,
                                            tuningProfile.max_value,
                                            tuningProfile.offset);
#else
    pBrightness = MAP_SYSTEM_TO_USER_OFFSET(stCscAttr.luma, BRIGHT_MIN_VALUE, BRIGHT_MAX_VALUE, BRIGHT_OFFSET);
#endif

    /* 确保返回值在有效范围内 */ 
    if (pBrightness > 100) 
    {
        pBrightness = 100;
    }
    
    return nRet; 
}


int CIspControl::set_sharpness(const unsigned int nSharpen) 
{
    int nRet = IpcRet_E::OK;
    ot_isp_sharpen_attr stSharpenAttr;

    nRet = ss_mpi_isp_get_sharpen_attr(m_viPipe,&stSharpenAttr);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("ot_mpi_isp_get_exposure_attr error");
        return IpcRet_E::ERR;
    }

    unsigned int mappedSharpen = 0;

#if CAP_ISP_DAY_NIGHT_DIFFERENT_TUNING
    const IspTuningScene_E tuningScene = get_isp_tuning_scene();
    const SharpenTuningProfile_S &tuningProfile = get_sharpen_tuning_profile(tuningScene);
    mappedSharpen = MAP_USER_TO_SYSTEM_OFFSET(nSharpen,
                                              tuningProfile.min_value,
                                              tuningProfile.max_value,
                                              tuningProfile.offset);

    if (tuningProfile.use_auto_mode)
    {
        /* 夜晚红外场景沿用原夜间自动锐度策略，限制自动锐度上限 */
        stSharpenAttr.op_type = OT_OP_MODE_AUTO;
        for (td_u8 i = 0; i < OT_ISP_AUTO_ISO_NUM; ++i)
        {
            stSharpenAttr.auto_attr.max_sharp_gain[i] = mappedSharpen;
        }
    }
    else
    {
        /* 白天与夜晚白光当前均使用手动锐度，夜晚白光可在 profile 中独立扩展 */
        stSharpenAttr.op_type = OT_OP_MODE_MANUAL;
        stSharpenAttr.manual_attr.max_sharp_gain = mappedSharpen;
    }
#else
    mappedSharpen = MAP_USER_TO_SYSTEM_OFFSET(nSharpen, SHARPEN_MIN_VALUE, SHARPEN_MAX_VALUE, SHARPEN_OFFSET);

    /* TV-3852T* 系列关闭差异化调参，统一保持现有手动锐度逻辑 */
    stSharpenAttr.op_type = OT_OP_MODE_MANUAL;
    stSharpenAttr.manual_attr.max_sharp_gain = mappedSharpen;
#endif

    nRet = ss_mpi_isp_set_sharpen_attr(m_viPipe,&stSharpenAttr);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("ss_mpi_isp_set_sharpen_attr error");
        return IpcRet_E::ERR;
    }

    return nRet; 
}

int CIspControl::get_sharpness(unsigned int &pSharpen) const
{
    int nRet = IpcRet_E::OK;
    ot_isp_sharpen_attr stSharpenAttr;

    nRet = ss_mpi_isp_get_sharpen_attr(m_viPipe,&stSharpenAttr);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("ot_mpi_isp_get_exposure_attr error");
        return IpcRet_E::ERR;
    }

    pSharpen = MAP_SYSTEM_TO_USER_OFFSET(stSharpenAttr.manual_attr.max_sharp_gain, SHARPEN_MIN_VALUE, SHARPEN_MAX_VALUE, SHARPEN_OFFSET);
    return nRet; 
}

int CIspControl::set_contrast(const unsigned int nContrast)
{
    int nRet = IpcRet_E::OK;
    ot_isp_csc_attr stCscAttr;

    nRet = ss_mpi_isp_get_csc_attr(m_viPipe,&stCscAttr);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("ss_mpi_isp_get_csc_attr error");
        return IpcRet_E::ERR;
    }

    unsigned int mappedContrast = MAP_USER_TO_SYSTEM_OFFSET(nContrast,CONTRAST_MIN_VALUE,CONTRAST_MAX_VALUE,CONTRAST_OFFSET);

    /* 调整对比度 */
    stCscAttr.contr = mappedContrast;

    nRet = ss_mpi_isp_set_csc_attr(m_viPipe,&stCscAttr);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("ss_mpi_isp_set_csc_attr error");
        return IpcRet_E::ERR;
    }

    return nRet; 
}

int CIspControl::get_contrast(unsigned int &pContrast) const
{
    if(!pContrast)
    {
        dlog_error("pContrast is nullptr");
        return IpcRet_E::ERR;
    }

    int nRet = IpcRet_E::OK;
    ot_isp_csc_attr stCscAttr;

    nRet = ss_mpi_isp_get_csc_attr(m_viPipe,&stCscAttr);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("ss_mpi_isp_get_csc_attr error");
        return IpcRet_E::ERR;
    }

    pContrast = MAP_SYSTEM_TO_USER_OFFSET(stCscAttr.contr, CONTRAST_MIN_VALUE, CONTRAST_MAX_VALUE, CONTRAST_OFFSET);

    return nRet; 
}


int CIspControl::get_imageParam_attr(ImageParam_S &stImage) const
{
    int nRet = IpcRet_E::OK;
    /* 对比度 */
    nRet = get_contrast(stImage.nContrast);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("get_contrast error,nContrast:%u",stImage.nContrast);
        return IpcRet_E::ERR;
    }
    /* 锐度 */
    nRet = get_sharpness(stImage.nSharpness);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("get_sharpen error,nSharpness:%u",stImage.nSharpness);
        return IpcRet_E::ERR;
    }
    /* 亮度 */
    nRet = get_brightness(stImage.nBrightness);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("get_brightness error,nBrightness:%u",stImage.nBrightness);
        return IpcRet_E::ERR;
    }
    /* 饱和度 */
    nRet = get_saturation(stImage.nSaturation);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("get_saturation error,nSaturation:%u",stImage.nSaturation);
        return IpcRet_E::ERR;
    }
    return nRet;
}

int CIspControl::set_imageParam_attr(const ImageParam_S stImage)
{
    int nRet = IpcRet_E::OK;
    /* 对比度 */
    nRet = set_contrast(stImage.nContrast);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("set_contrast error,nContrast:%u",stImage.nContrast);
        return IpcRet_E::ERR;
    }
    /* 锐度 */
    nRet = set_sharpness(stImage.nSharpness);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("set_sharpen error,nSharpness:%u",stImage.nSharpness);
        return IpcRet_E::ERR;
    }
    /* 亮度 */
    nRet = set_brightness(stImage.nBrightness);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("set_brightness error,nBrightness:%u",stImage.nBrightness);
        return IpcRet_E::ERR;
    }
    /* 饱和度 */
    nRet = set_saturation(stImage.nSaturation);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("set_saturation error,nSaturation:%u",stImage.nSaturation);
        return IpcRet_E::ERR;
    }
    return nRet;
}

int CIspControl::set_wdr_attr(const WdrAttr_S &stWdrAttr)
{
    int nRet = IpcRet_E::OK;
    ot_isp_drc_attr stDrcInfo;
    /* 宽动态 */
    nRet = ot_mpi_isp_get_drc_attr(m_viPipe,&stDrcInfo);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("ot_mpi_isp_get_drc_attr error");
        return IpcRet_E::ERR;
    }

    if(stWdrAttr.bEnable)
    {
#if CAP_ISP_DAY_NIGHT_DIFFERENT_TUNING
        const IspTuningScene_E tuningScene = get_isp_tuning_scene();
        const WdrTuningProfile_S &tuningProfile = get_wdr_tuning_profile(tuningScene);

        /* 根据白天/夜晚白光/夜晚红外场景映射用户宽动态等级 */
        unsigned int mappedWdr = MAP_USER_TO_SYSTEM_OFFSET(stWdrAttr.nWdrLevel,
                                                           tuningProfile.min_value,
                                                           tuningProfile.max_value,
                                                           tuningProfile.offset);
#else
        /* TV-3852T* 系列关闭差异化调参，白天黑夜统一使用白天宽动态逻辑 */
        unsigned int mappedWdr = MAP_USER_TO_SYSTEM_OFFSET(stWdrAttr.nWdrLevel,
                                                           WDR_DAY_MIN_VALUE,
                                                           WDR_DAY_MAX_VALUE,
                                                           WDR_DAY_OFFSET);
#endif

        stDrcInfo.enable = TD_TRUE;
        stDrcInfo.op_type = OT_OP_MODE_MANUAL;
        stDrcInfo.manual_attr.strength = mappedWdr;
    }
    else 
    {
        stDrcInfo.enable = TD_FALSE;
    }

    nRet = ot_mpi_isp_set_drc_attr(m_viPipe,&stDrcInfo);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("ot_mpi_isp_set_drc_attr error");
        return IpcRet_E::ERR;
    }
    return nRet;
}

int CIspControl::get_wdr_attr(WdrAttr_S &stWdrAttr) const
{
    int nRet = IpcRet_E::OK;
    ot_isp_drc_attr stDrcInfo;
    /* 宽动态 */
    nRet = ot_mpi_isp_get_drc_attr(m_viPipe,&stDrcInfo);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("ot_mpi_isp_get_drc_attr error");
        return IpcRet_E::ERR;
    }

    // if(fswdr_attr.wdr_combine.motion_comp)
    // {
    //     stWdrAttr.bEnable = true;
    //     stWdrAttr.nWdrLevel = GET_WDR_SHORT_INDEX(fswdr_attr.wdr_combine.short_threshold);
    // }
    // else
    // {
    //     stWdrAttr.bEnable = false;
    // }

    // nRet = ot_mpi_isp_set_drc_attr(m_viPipe,&fswdr_attr);
    // if(nRet != IpcRet_E::OK)
    // {
    //     dlog_error("ot_mpi_isp_get_stats_cfg error");
    //     return IpcRet_E::ERR;
    // }
    return nRet;
}

int CIspControl::set_hls_attr(const HlsAttr_S &stHlsAttr)
{
    int nRet = IpcRet_E::OK;
    ot_isp_drc_attr drc_attr;

    nRet = ss_mpi_isp_get_drc_attr(m_viPipe,&drc_attr);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("ss_mpi_isp_get_drc_attr error");
        return IpcRet_E::ERR;
    }

    if(stHlsAttr.bEnable)
    {
        drc_attr.enable = TD_TRUE;
    }
    else
    {
        drc_attr.enable = TD_FALSE;
    }

    nRet = ss_mpi_isp_set_drc_attr(m_viPipe,&drc_attr);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("ss_mpi_isp_set_drc_attr error");
        return IpcRet_E::ERR;
    }

    return nRet;
}

int CIspControl::get_hls_attr(HlsAttr_S& stHlsAttr) const
{
    int nRet = IpcRet_E::OK;
    ot_isp_drc_attr drc_attr;

    nRet = ss_mpi_isp_get_drc_attr(m_viPipe,&drc_attr);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("ss_mpi_isp_get_drc_attr error");
        return IpcRet_E::ERR;
    }

    if(drc_attr.enable)
    {
        stHlsAttr.bEnable = true;
    }
    else
    {
        stHlsAttr.bEnable = false;
    }

    nRet = ss_mpi_isp_set_drc_attr(m_viPipe,&drc_attr);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("ss_mpi_isp_set_drc_attr error");
        return IpcRet_E::ERR;
    }

    return nRet;
}

static void configure_backlight_weights(BackLightArea_E enBackLightArea,ot_isp_ae_stats_cfg &ae_stats_cfg) 
{
    /* 初始化所有区域的权重为默认值 */
    for (td_u8 row = 0; row < OT_ISP_AE_ZONE_ROW; row++) 
    {
        for (td_u8 col = 0; col < OT_ISP_AE_ZONE_COLUMN; col++) 
        {
            ae_stats_cfg.weight[row][col] = BACKLIGHT_WEIGHT_DEFAULT; /* 默认权重 */
        }
    }

    /* 根据枚举值设置背光区域的权重 */
    switch (enBackLightArea) 
    {
        case UP: /* 上区域：前 6 行 */
            for (td_u8 row = 0; row < 6; row++) {
                for (td_u8 col = 0; col < OT_ISP_AE_ZONE_COLUMN; col++) {
                    ae_stats_cfg.weight[row][col] = BACKLIGHT_WEIGHT_VALUE; /* 背光区域权重 */
                }
            }
            break;

        case DOWN: /* 下区域：后 6行 */
            for (td_u8 row = OT_ISP_AE_ZONE_ROW - 6; row < OT_ISP_AE_ZONE_ROW; row++) {
                for (td_u8 col = 0; col < OT_ISP_AE_ZONE_COLUMN; col++) {
                    ae_stats_cfg.weight[row][col] = BACKLIGHT_WEIGHT_VALUE; /* 背光区域权重 */
                }
            }
            break;

        case LEFT: /* 左区域：前 6 列 */
            for (td_u8 col = 0; col < 6; col++) {
                for (td_u8 row = 0; row < OT_ISP_AE_ZONE_ROW; row++) 
                {
                    ae_stats_cfg.weight[row][col] = BACKLIGHT_WEIGHT_VALUE; /* 背光区域权重 */
                }
            }
            break;

        case RIGHT: /* 右区域：后 6 列 */
            for (td_u8 col = OT_ISP_AE_ZONE_COLUMN - 6; col < OT_ISP_AE_ZONE_COLUMN; col++) 
            {
                for (td_u8 row = 0; row < OT_ISP_AE_ZONE_ROW; row++) {
                    ae_stats_cfg.weight[row][col] = BACKLIGHT_WEIGHT_VALUE; /* 背光区域权重 */
                }
            }
            break;

        case CENTER_BACKLIGHT: /* 中心区域：中间 3 行 5 列 */
            for (td_u8 row = (OT_ISP_AE_ZONE_ROW / 2) - 1; row <= (OT_ISP_AE_ZONE_ROW / 2) + 1; row++) 
            {
                for (td_u8 col = (OT_ISP_AE_ZONE_COLUMN / 2) - 2; col <= (OT_ISP_AE_ZONE_COLUMN / 2) + 2; col++) 
                {
                    ae_stats_cfg.weight[row][col] = BACKLIGHT_WEIGHT_VALUE; /* 背光区域权重 */
                }
            }
            break;

        default:
            break;
    }
}

/**************************图像参数控制 end ************************/

/**************************高级参数控制 ************************/
int CIspControl::get_exposure_attr(ExposureAttr_S &stExpAttr) const
{
    int nRet = IpcRet_E::OK;
    ot_isp_exposure_attr exp_attr;

    nRet = ot_mpi_isp_get_exposure_attr(m_viPipe,&exp_attr);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("ot_mpi_isp_get_exposure_attr error");
        return IpcRet_E::ERR;
    }

    switch (exp_attr.op_type)
    {
        case OT_OP_MODE_AUTO:
        {
            /* 获取防横纹开关状态 */ 
            stExpAttr.bAntiBanding = (exp_attr.auto_attr.antiflicker.enable == TD_TRUE);
            break;
        }
        case OT_OP_MODE_MANUAL:
        {
            #if 0
            stExpAttr.bAntiBanding = false;
             /* 根据获取到的曝光时间值，在 exposureTimeMapping 数组中查找对应的枚举值 */ 
            uint32_t currentExposureTime = exp_attr.manual_attr.exp_time;
            for (size_t i = 0; i < sizeof(exposureTimeMapping) / sizeof(exposureTimeMapping[0]); ++i)
            {
                if (currentExposureTime == exposureTimeMapping[i])
                {
                    stExpAttr.enExpTime = static_cast<ExpTimeMode_E>(i); 
                    break;
                }
            }
            #else
            exp_attr.op_type = OT_OP_MODE_MANUAL;
            /* 曝光时长设置为手动 */
            exp_attr.manual_attr.exp_time_op_type = OT_OP_MODE_MANUAL;
            /* 曝光时间 */
            exp_attr.manual_attr.exp_time = exposureTimeMapping[static_cast<uint8_t>(stExpAttr.enExpTime)];
            exp_attr.auto_attr.antiflicker.enable = TD_FALSE;
            #endif
            break;
        }
        default:
            dlog_error("Unsupported exposure operation mode");
            return IpcRet_E::ERR;
    }

    return nRet;
}

int CIspControl::set_exposure_attr(const ExposureAttr_S &stExpAttr)
{
    int nRet = IpcRet_E::OK;
    ot_isp_exposure_attr exp_attr;

    nRet = ot_mpi_isp_get_exposure_attr(m_viPipe,&exp_attr);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("ot_mpi_isp_get_exposure_attr error");
        return IpcRet_E::ERR;
    }
  
    if(stExpAttr.bAntiBanding)
    {
        exp_attr.op_type = OT_OP_MODE_AUTO;
        /* 防横纹开关,即抗闪烁功能 */
        exp_attr.auto_attr.antiflicker.enable = TD_TRUE;
        exp_attr.auto_attr.antiflicker.frequency = 50;
        exp_attr.auto_attr.antiflicker.mode = OT_ISP_ANTIFLICKER_NORMAL_MODE;
    }
    else
    {
#if CAP_ISP_DAY_NIGHT_DIFFERENT_TUNING
        const IspTuningScene_E tuningScene = get_isp_tuning_scene();
        const int *pExposureCompensationArray = get_exposure_compensation_array(tuningScene);
#else
        /* TV-3852T* 系列关闭差异化调参，白天黑夜统一使用白天曝光补偿 */
        const int *pExposureCompensationArray = EXPOSURE_COMPENSATION_DAY_ARRAY;
#endif

        /* 根据白天/夜晚白光/夜晚红外场景设置曝光补偿 */
        exp_attr.auto_attr.compensation = pExposureCompensationArray[static_cast<uint8_t>(stExpAttr.enExpTime)];
    }

    nRet = ot_mpi_isp_set_exposure_attr(m_viPipe,&exp_attr);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("ot_mpi_isp_set_exposure_attr error");
        return IpcRet_E::ERR;
    }

    return nRet;
}

int CIspControl::apply_gamma_attr()
{
#if CAP_ISP_DAY_NIGHT_DIFFERENT_TUNING && defined(SENSOR_SC533HAI)
    ot_isp_gamma_attr stGammaAttr;
    int nRet = ot_mpi_isp_get_gamma_attr(m_viPipe, &stGammaAttr);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("ot_mpi_isp_get_gamma_attr error");
        return IpcRet_E::ERR;
    }

    const IspTuningScene_E tuningScene = get_isp_tuning_scene();
    const ot_isp_gamma_attr &stTargetGammaAttr = get_gamma_attr_by_tuning_scene(tuningScene);

    /* 保留 get 接口校验 ISP Gamma 模块可读后，整体覆盖为当前场景目标曲线。 */
    stGammaAttr = stTargetGammaAttr;
    nRet = ot_mpi_isp_set_gamma_attr(m_viPipe, &stGammaAttr);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("ot_mpi_isp_set_gamma_attr error");
        return IpcRet_E::ERR;
    }

    dlog_info("Gamma参数应用成功，调参场景: %d", static_cast<int>(tuningScene));
    return nRet;
#else
    return IpcRet_E::OK;
#endif
}

int CIspControl::get_awb_attr(AwbAttr_S &stAwbInfo) const
{
    int nRet = IpcRet_E::OK;
    ot_isp_wb_attr wb_attr;
    nRet = ss_mpi_isp_get_wb_attr(m_viPipe, &wb_attr);
    if (nRet != IpcRet_E::OK)
    {
        dlog_error("ss_mpi_isp_get_wb_attr error");
        return IpcRet_E::ERR;
    }

    switch (wb_attr.op_type)
    {
        /* 自动白平衡 */ 
        case OT_OP_MODE_AUTO:
            stAwbInfo.enAwbMode = AUTO_AWB_MODE;
            break;
        /* 手动白平衡 */ 
        case OT_OP_MODE_MANUAL:
        {
            stAwbInfo.enAwbMode = MANUAL_AWB_MODE;
            stAwbInfo.nRGain = wb_attr.manual_attr.r_gain;
            stAwbInfo.nBGain = wb_attr.manual_attr.b_gain;
            break;
        }
        default:
            break;
    }

    return nRet;
}

int CIspControl::set_awb_attr(const AwbAttr_S& stAwbInfo)
{
    int nRet = IpcRet_E::OK;
    ot_isp_wb_attr wb_attr;
    nRet = ss_mpi_isp_get_wb_attr(m_viPipe,&wb_attr);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("ss_mpi_isp_get_wb_attr error");
        return IpcRet_E::ERR;
    }

    switch (stAwbInfo.enAwbMode)
    {
        /* 自动白平衡 */
        case AUTO_AWB_MODE:
            wb_attr.op_type = OT_OP_MODE_AUTO;
            break;
        /* 手动白平衡 */
        case MANUAL_AWB_MODE:
            wb_attr.op_type = OT_OP_MODE_MANUAL;
            wb_attr.manual_attr.r_gain = USER_TO_AWB_API(stAwbInfo.nRGain);
            wb_attr.manual_attr.b_gain = USER_TO_AWB_API(stAwbInfo.nBGain);
            break;
        /* 锁定白平衡 */
        case LOCK_AWB_MODE:
            wb_attr.op_type = OT_OP_MODE_MANUAL;
            wb_attr.manual_attr.r_gain = 506;
            wb_attr.manual_attr.gr_gain = 256;
            wb_attr.manual_attr.gb_gain = 256;
            wb_attr.manual_attr.b_gain = 506;
            break;
        /* 白炽灯 */
        case INCANDESCENT_MODE:
            wb_attr.op_type = OT_OP_MODE_MANUAL;
            wb_attr.manual_attr.r_gain = 506;
            wb_attr.manual_attr.gr_gain = 256;
            wb_attr.manual_attr.gb_gain = 256;
            wb_attr.manual_attr.b_gain = 584;
            break;
        /* 暖光灯 */
        case WARM_MODE:
            wb_attr.op_type = OT_OP_MODE_MANUAL;
            wb_attr.manual_attr.r_gain = 506;
            wb_attr.manual_attr.gr_gain = 256;
            wb_attr.manual_attr.gb_gain = 256;
            wb_attr.manual_attr.b_gain = 590;
            break;
        /* 日光灯 */
        case FLUORESCENT_MODE:
            wb_attr.op_type = OT_OP_MODE_MANUAL;
            wb_attr.manual_attr.r_gain = 597;
            wb_attr.manual_attr.gr_gain = 256;
            wb_attr.manual_attr.gb_gain = 256;
            wb_attr.manual_attr.b_gain = 667;
            break;
        /* 自然灯 */
        case DAY_LIGHT_MODE:
            wb_attr.op_type = OT_OP_MODE_MANUAL;
            wb_attr.manual_attr.r_gain = 500;
            wb_attr.manual_attr.gr_gain = 256;
            wb_attr.manual_attr.gb_gain = 256;
            wb_attr.manual_attr.b_gain = 500;
            break;
        default:
            break;
    }

    nRet = ss_mpi_isp_set_wb_attr(m_viPipe,&wb_attr);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("ss_mpi_isp_set_wb_attr error");
        return IpcRet_E::ERR;
    }   

    return nRet;
}

int CIspControl::get_nr_attr(DnrAttr_S &stDnrInfo) const
{
    int nRet = IpcRet_E::OK;
    ot_3dnr_param nr_attr;
    // ot_isp_dehaze_attr dehaze_attr;

    /* 数字降噪 */
    nRet = ss_mpi_vpss_get_grp_3dnr_param(m_viPipe,&nr_attr);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("ss_mpi_isp_get_nr_attr error");
        return IpcRet_E::ERR;
    }

    return nRet;
}

int CIspControl::set_nr_attr(const DnrAttr_S &stDnrInfo)
{
    int nRet = IpcRet_E::OK;
    ot_3dnr_param nr_attr; 
    ot_3dnr_attr attr;

    /* 数字降噪 */
    nRet = ss_mpi_vi_get_pipe_3dnr_param(m_viPipe,&nr_attr);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("ss_mpi_vi_get_pipe_3dnr_param error");
        return IpcRet_E::ERR;
    }

    nRet = ss_mpi_vi_get_pipe_3dnr_attr(m_viPipe,&attr);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("ss_mpi_vi_get_pipe_3dnr_attr error");
        return IpcRet_E::ERR;
    }

    switch (stDnrInfo.enDnrMode)
    {
        /* 关闭 */
        case CLOSE_MODE:
            attr.enable = TD_FALSE;
            break;
        /* 普通模式 */
        case NORMAL_MODE:
            attr.enable = TD_TRUE;     
            nr_attr.nr_norm_param_v2.op_mode = OT_OP_MODE_MANUAL;
            nr_attr.nr_norm_param_v2.nr_manual.nr_param.iey.fine_g1 = stDnrInfo.nDnrLevel;
            break;
        /* 专家模式 */
        case ADVANCED_MODE:
            attr.enable = TD_TRUE;
            nr_attr.nr_norm_param_v2.op_mode = OT_OP_MODE_MANUAL;

            /* 时域降噪等级 */
            nr_attr.nr_norm_param_v2.nr_manual.nr_param.tfy[0].tss0 = (td_u8)((stDnrInfo.nTnrLevel*32)/100);
            nr_attr.nr_norm_param_v2.nr_manual.nr_param.tfy[0].tss1 = (td_u8)((stDnrInfo.nTnrLevel*32)/100);
            nr_attr.nr_norm_param_v2.nr_manual.nr_param.tfy[0].tss2 = (td_u8)((stDnrInfo.nTnrLevel*32)/100);

            /* 空域降噪等级 */
            nr_attr.nr_norm_param_v2.nr_manual.nr_param.sfy[0].sbr1 = (td_u8)((stDnrInfo.nSnrLevel*255)/100);
            nr_attr.nr_norm_param_v2.nr_manual.nr_param.sfy[0].sbr2 = (td_u8)((stDnrInfo.nSnrLevel*255)/100);
            nr_attr.nr_norm_param_v2.nr_manual.nr_param.sfy[0].sbr4 = (td_u8)((stDnrInfo.nSnrLevel*255)/100);
            break;
        default:
            break;
    }

    nRet = ss_mpi_vi_set_pipe_3dnr_attr(m_viPipe,&attr);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("ss_mpi_vi_set_pipe_3dnr_attr error");
        return IpcRet_E::ERR;
    }

    nRet = ss_mpi_vi_set_pipe_3dnr_param(m_viPipe,&nr_attr);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("ss_mpi_vi_set_pipe_3dnr_param error");
        return IpcRet_E::ERR;
    }
   
    return nRet;
}

int CIspControl::get_backLight_attr(BackLightArrt_S &stBackAttr) const
{
    int nRet = IpcRet_E::OK;

    /* 背光区域 */

    /* 宽动态 */
    get_wdr_attr(stBackAttr.stWdrAttr);

    /* 强光抑制 */
    get_hls_attr(stBackAttr.stHlsAttr);

    return nRet;
}

int CIspControl::set_backLight_attr(const BackLightArrt_S &stBackAttr)
{
    int nRet = IpcRet_E::OK;
    ot_isp_stats_cfg stat_cfg;

    nRet = ot_mpi_isp_get_stats_cfg(m_viPipe,&stat_cfg);
    if(nRet != IpcRet_E::OK)
    {
        dlog_error("ot_mpi_isp_get_stats_cfg error");
        return IpcRet_E::ERR;
    }
    /* 宽动态和背光补偿同时启用时，仅宽动态生效 */
    if(!stBackAttr.stWdrAttr.bEnable && stBackAttr.enBackLightArea != CLOSE)
    {
        /* 背光区域 */
        configure_backlight_weights(stBackAttr.enBackLightArea,stat_cfg.ae_cfg);
        /* 背光区域设置 */
        nRet = ot_mpi_isp_set_stats_cfg(m_viPipe,&stat_cfg);
        if(nRet != IpcRet_E::OK)
        {
            dlog_error("ot_mpi_isp_set_stats_cfg error");
            return IpcRet_E::ERR;
        }
    }
    else
    {
        /* 宽动态 */
        set_wdr_attr(stBackAttr.stWdrAttr);
    }

    /* 强光抑制 */
    set_hls_attr(stBackAttr.stHlsAttr);
    
    return nRet;
}

int CIspControl::get_videoMirror_attr(VideoAdjust_S &stVideoAdjust) const
{
    int nRet = OK;
    auto viHandle = CStreamVideo::instance()->get_viHandle();
    if (viHandle == nullptr)
    {
        dlog_error("get_viHandle error");
        return ERR;
    }
    auto &stExParam = viHandle->stExParam;

    /* 根据 mirror_en 和 flip_en 的值判断当前的镜像模式 */ 
    if (!stExParam.bMirror && !stExParam.bFlip)
    {
        /* 关闭镜像翻转 */ 
        stVideoAdjust.enMirrorMode = DISABLE;
    }
    else if (stExParam.bMirror && !stExParam.bFlip)
    {
        /* 镜像左右翻转 */ 
        stVideoAdjust.enMirrorMode = HORIZONTAL;
    }
    else if (!stExParam.bMirror && stExParam.bFlip)
    {
        /* 镜像上下翻转 */ 
        stVideoAdjust.enMirrorMode = VERTICAL;
    }
    else if (stExParam.bMirror && stExParam.bFlip)
    {
        /* 镜像中心翻转 */ 
        stVideoAdjust.enMirrorMode = CENTER;
    }
    else
    {
        dlog_error("Invalid mirror mode configuration");
        return ERR;
    }

    return nRet;
}

int CIspControl::set_videoMirror_attr(const VideoAdjust_S & stVideoAdjust)
{
    int nRet = OK;
    auto viHandle = CStreamVideo::instance()->get_viHandle();
    auto &stExParam = viHandle->stExParam;

    switch (stVideoAdjust.enMirrorMode)
    {
        /* 关闭 */
        case DISABLE:
            dlog_debug("关闭镜像翻转");
            #ifdef DEVICE_TV_3852H  //吸顶
            stExParam.bMirror = TD_TRUE;
            stExParam.bFlip = TD_TRUE;
            #else
            stExParam.bMirror = TD_FALSE;
            stExParam.bFlip = TD_FALSE;
            #endif
            break;
        /* 左右 */
        case HORIZONTAL:
            dlog_debug("镜像左右翻转");
            #ifdef DEVICE_TV_3852H  //吸顶
            stExParam.bMirror = TD_FALSE;
            stExParam.bFlip = TD_TRUE;
            #else
            stExParam.bMirror = TD_TRUE;
            stExParam.bFlip = TD_FALSE;
            #endif

            break;
        /* 上下 */
        case VERTICAL:
            dlog_debug("镜像上下翻转");
            #ifdef DEVICE_TV_3852H  //吸顶
            stExParam.bMirror = TD_TRUE;
            stExParam.bFlip = TD_FALSE;
            #else
            stExParam.bMirror = TD_FALSE;
            stExParam.bFlip = TD_TRUE;
            #endif
            break;
        /* 中心 */
        case CENTER:
            dlog_debug("镜像中心翻转");
            #ifdef DEVICE_TV_3852H  //吸顶
            stExParam.bMirror = TD_FALSE;
            stExParam.bFlip = TD_FALSE;
            #else
            stExParam.bMirror = TD_TRUE;
            stExParam.bFlip = TD_TRUE;
            #endif
            break;
        default:
            dlog_error("Invalid mirror mode: %u", stVideoAdjust.enMirrorMode);
            return ERR;
    }

    nRet = viHandle->mppVi_set_sensor_mirror_flip(viHandle);
    if (nRet != OK)
    {
        dlog_error("mppVi_set_sensor_mirror_flip error");
        return ERR;
    }

    return OK;
}

int CIspControl::get_dayNight_attr(DayNightAttr_S &stDayNightAttr) const
{
    return CIspConfigure::instance()->get_configure(stDayNightAttr);
}

int CIspControl::set_dayNight_attr(const DayNightAttr_S &stDayNightAttr)
{
    int nRet = IpcRet_E::OK;
    
    CDayNightController::instance()->setMode(stDayNightAttr.enDayNightMode);

    if (stDayNightAttr.enDayNightMode == TIME_MODE) 
    {
        TimeRange_S stRange;
        stRange.stStartTime = stDayNightAttr.stBeginTime;
        stRange.stEndTime = stDayNightAttr.stEndTime;
        CDayNightController::instance()->setTimeRange(stRange);
    }
    CDayNightController::instance()->setFillLight();
    CDayNightController::instance()->setSensitivity(stDayNightAttr.nSensitivityLevel);
    CDayNightController::instance()->setFilterTime(stDayNightAttr.nFilterTime);

    return nRet;
}

/**************************高级参数控制 end************************/
