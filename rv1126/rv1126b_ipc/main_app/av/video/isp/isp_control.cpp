/**
 * @FilePath     : isp_control.cpp
 * @Author       : cyc
 * @Date         : 2025-06-05 10:16:15
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-11 13:43:07
 * @Description  : isp参数控制模块
 */

#include <unistd.h>
#include <iomanip>
#include "isp_control.h"
#include "IpcRet.h"
#include "sample_comm.h"
#include "dlog.h"
#include "isp_configure.h"
#include "rk_mpi_vpss.h"

CIspControl::CIspControl()
{

}

CIspControl::~CIspControl()
{
    if (m_isInitialized.load())
    {
        deinit(); 
    }

}

int CIspControl::init()
{
    /* step: 先建立并启动RK AIQ，成功后bootstrap才能把该上下文借给共享ISP适配器。 */
    XCamReturn nRet = XCAM_RETURN_NO_ERROR;

	nRet = rk_isp_init(CAMERA_IQFILE_PATH);
    if (nRet != XCAM_RETURN_NO_ERROR)
    {
        dlog(LOG_ERROR, "rk_isp_init error: %d", nRet);
        return nRet;
    }

    m_isInitialized.store(true);

    dlog_info("IspControl init successfully");
    
    return nRet;
}

int CIspControl::deinit()
{
    /* memory: 共享service必须先注销；本函数只释放自己拥有的RK AIQ上下文。 */
    if (!m_isInitialized.load())
    {
        return XCAM_RETURN_NO_ERROR;
    }

    const XCamReturn nRet = rk_isp_deInit();
    if (nRet == XCAM_RETURN_NO_ERROR)
    {
        m_isInitialized.store(false);
    }
    else
    {
        /* ! AIQ释放失败时保留初始化状态和上下文，允许上层按生命周期重试，避免遗留悬空借用。 */
        dlog_error("RV1126B RK AIQ释放失败，保留上下文等待重试: %d", nRet);
    }
    return nRet;
}


XCamReturn CIspControl::rk_isp_init(const std::string strIqfilesDir) 
{
    XCamReturn nRet = XCAM_RETURN_NO_ERROR;
    rk_aiq_static_info_t stAiqInfo;

    if(g_aiq_ctx)
    {
        g_aiq_ctx = NULL;
    }
	
    /* 获取与指定物理地址对应的sensor信息 */
	nRet = rk_aiq_uapi2_sysctl_enumStaticMetasByPhyId(nCamID, &stAiqInfo);
	if (nRet != XCAM_RETURN_NO_ERROR) 
    {
		dlog_error("rk_aiq_uapi2_sysctl_enumStaticMetasByPhyId error");
        return nRet;
	}

	dlog(LOG_INFO,"sensor_name is %s, iqfiles is %s",
        stAiqInfo.sensor_info.sensor_name, strIqfilesDir.c_str());
             
    /* 设定AIQ启动时的场景参数 */
    nRet = rk_aiq_uapi2_sysctl_preInit_scene(stAiqInfo.sensor_info.sensor_name, "normal","day");
    if (nRet != XCAM_RETURN_NO_ERROR) 
    {
		dlog_error("rk_aiq_uapi2_sysctl_preInit_scene error");
        return nRet;
	}

    /* 初始化aiq上下文 */
	g_aiq_ctx = rk_aiq_uapi2_sysctl_init(stAiqInfo.sensor_info.sensor_name, strIqfilesDir.c_str(), NULL, NULL);
    if (!g_aiq_ctx) 
    {
		dlog_error("rk_aiq_uapi2_sysctl_init error");
        return XCAM_RETURN_ERROR_FAILED;
	}

    /* 准备AIQ运行环境 */
    nRet = rk_aiq_uapi2_sysctl_prepare(g_aiq_ctx, 0, 0,RK_AIQ_WORKING_MODE_NORMAL);
    if (nRet != XCAM_RETURN_NO_ERROR) 
    {
        dlog_error("rk_aiq_uapi2_sysctl_prepare error");
        return nRet;
    }

    /* 启动AIQ控制系统，会不断从isp驱动获取3A信息，运行3A算法，并应用计算出新的参数 */
    nRet = rk_aiq_uapi2_sysctl_start(g_aiq_ctx);
    if (nRet != XCAM_RETURN_NO_ERROR) 
    {
        dlog_error("rk_aiq_uapi2_sysctl_start error");
        return nRet;
    }

    /* 设定帧率控制，note：后续需要修改 */
    frameRateInfo_t info;
	info.mode = OP_MANUAL;
	info.fps = 30;
	nRet = rk_aiq_uapi2_setFrameRate(g_aiq_ctx, info);
    if (nRet != XCAM_RETURN_NO_ERROR)
    {
        dlog(LOG_ERROR, "rk_aiq_uapi2_setFrameRate error: %d", nRet);
        return nRet;
    }

	return nRet;
}

XCamReturn CIspControl::rk_isp_deInit()
{
    XCamReturn  nRet = XCAM_RETURN_NO_ERROR;
    if (g_aiq_ctx == NULL)
    {
        return nRet;
    }

    /* step: 先停止3A线程；停止失败时保留context，避免后续adapter访问已失效对象。 */
	nRet = rk_aiq_uapi2_sysctl_stop(g_aiq_ctx, false);
    if (nRet != XCAM_RETURN_NO_ERROR)
    {
        dlog(LOG_ERROR, "rk_aiq_uapi2_sysctl_stop error: %d", nRet);
        return nRet;
    }

    /* 反初始化AIQ上下文 */
	rk_aiq_uapi2_sysctl_deinit(g_aiq_ctx);
	g_aiq_ctx = NULL;
    return nRet;
}

rk_aiq_sys_ctx_t* CIspControl::get_aiq_ctx()
{
    return g_aiq_ctx;
}

/**************************图像参数控制 ************************/

int CIspControl::set_saturation(const unsigned int nSaturation)
{
    if(nSaturation < 0 || nSaturation > 100)
    {
        dlog_error("nSaturation is 0~100");
        return ERR;
    }

    XCamReturn nRet =  XCAM_RETURN_NO_ERROR;
    unsigned int nSatVlaue;

    nSatVlaue = MAP_USER_TO_SYSTEM(nSaturation,SATURATION_MIN,SATURATION_MAX);

    nRet = rk_aiq_uapi2_setSaturation(g_aiq_ctx, nSatVlaue);
    if (nRet != XCAM_RETURN_NO_ERROR)
    {
        dlog(LOG_ERROR, "rk_aiq_uapi2_setSaturation error: %d", nRet);
        return nRet;
    }

    return nRet;
}

int CIspControl::get_saturation(unsigned int &pSaturation) const
{
    XCamReturn nRet =  XCAM_RETURN_NO_ERROR;

    unsigned int nSatVlaue = 0;

    nRet =  rk_aiq_uapi2_getSaturation(g_aiq_ctx, &nSatVlaue);
    if (nRet != XCAM_RETURN_NO_ERROR)
    {
        dlog(LOG_ERROR, "rk_aiq_uapi2_getSaturation error: %d", nRet);
        return nRet;
    }

    pSaturation = MAP_SYSTEM_TO_USER(nSatVlaue,SATURATION_MIN,SATURATION_MAX); 
    
    return nRet;
}


int CIspControl::set_brightness(const unsigned int nBrightness)
{
    if(nBrightness < 0 || nBrightness > 100)
    {
        dlog_error("nBrightness is 0~100");
        return ERR;
    }

    XCamReturn nRet =  XCAM_RETURN_NO_ERROR;
    unsigned int nBrightVlaue;

    nBrightVlaue = MAP_USER_TO_SYSTEM(nBrightness,BRIGHT_MIN,BRIGHT_MAX);

    nRet = rk_aiq_uapi2_setBrightness(g_aiq_ctx, nBrightVlaue);
    if (nRet != XCAM_RETURN_NO_ERROR)
    {
        dlog(LOG_ERROR, "rk_aiq_uapi2_setBrightness error: %d", nRet);
        return nRet;
    }

    return nRet;
}


int CIspControl::get_brightness(unsigned int &pBrightness) const
{
    XCamReturn nRet =  XCAM_RETURN_NO_ERROR;

    unsigned int nBrightVlaue = 0;

    nRet =  rk_aiq_uapi2_getBrightness(g_aiq_ctx, &nBrightVlaue);
    if (nRet != XCAM_RETURN_NO_ERROR)
    {
        dlog(LOG_ERROR, "rk_aiq_uapi2_getBrightness error: %d", nRet);
        return nRet;
    }

    pBrightness = MAP_SYSTEM_TO_USER(nBrightVlaue,BRIGHT_MIN,BRIGHT_MAX); 
    
    return nRet; 
}


int CIspControl::set_sharpness(const unsigned int nSharpen) 
{
    if(nSharpen < 0 || nSharpen > 100)
    {
        dlog_error("nSharpen is 0~100");
        return ERR;
    }

    XCamReturn nRet =  XCAM_RETURN_NO_ERROR;

#ifdef DEVICE_TV_3882TI
    unsigned int mapped_sharpness = (8 * nSharpen + 205) / 10;// 纯整数四舍五入: (0.8 * x + 20)
    nRet = rk_aiq_uapi2_setSharpness(g_aiq_ctx, mapped_sharpness);
#else
    nRet = rk_aiq_uapi2_setSharpness(g_aiq_ctx, nSharpen);
#endif

    if (nRet != XCAM_RETURN_NO_ERROR)
    {
        dlog(LOG_ERROR, "rk_aiq_uapi2_setSharpness error: %d", nRet);
        return nRet;
    }

    return nRet;
}

int CIspControl::get_sharpness(unsigned int &pSharpen) const
{
    XCamReturn nRet =  XCAM_RETURN_NO_ERROR;

    unsigned int nSharpenVlaue = 0;

    nRet =  rk_aiq_uapi2_getSharpness(g_aiq_ctx, &nSharpenVlaue);
    if (nRet != XCAM_RETURN_NO_ERROR)
    {
        dlog(LOG_ERROR, "rk_aiq_uapi2_getSharpness error: %d", nRet);
        return nRet;
    }

    pSharpen = nSharpenVlaue; 
    
    return nRet; 
}

int CIspControl::set_contrast(const unsigned int nContrast)
{
    if(nContrast < 0 || nContrast > 100)
    {
        dlog_error("nContrast is 0~100");
        return ERR;
    }

    XCamReturn nRet =  XCAM_RETURN_NO_ERROR;
    unsigned int nContrastVlaue;

    nContrastVlaue = MAP_USER_TO_SYSTEM(nContrast,CONTRAST_MIN,CONTRAST_MAX); ;

    nRet = rk_aiq_uapi2_setContrast(g_aiq_ctx, nContrastVlaue);
    if (nRet != XCAM_RETURN_NO_ERROR)
    {
        dlog(LOG_ERROR, "rk_aiq_uapi2_setContrast error: %d", nRet);
        return nRet;
    }

    return nRet;
}

int CIspControl::get_contrast(unsigned int &pContrast) const
{
    XCamReturn nRet =  XCAM_RETURN_NO_ERROR;

    unsigned int nContrastVlaue = 0;

    nRet =  rk_aiq_uapi2_getContrast(g_aiq_ctx, &nContrastVlaue);
    if (nRet != XCAM_RETURN_NO_ERROR)
    {
        dlog(LOG_ERROR, "rk_aiq_uapi2_getBrightness error: %d", nRet);
        return nRet;
    }

    pContrast = MAP_SYSTEM_TO_USER(nContrastVlaue,CONTRAST_MIN,CONTRAST_MAX); 
    
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

XCamReturn CIspControl::set_wdr_attr(const WdrAttr_S &stWdrAttr)
{
    XCamReturn nRet =  XCAM_RETURN_NO_ERROR;
    nRet = rk_aiq_uapi2_setHDRStrth(g_aiq_ctx, stWdrAttr.bEnable,stWdrAttr.nWdrLevel);
    if (nRet != XCAM_RETURN_NO_ERROR)
    {
        dlog(LOG_ERROR, "rk_aiq_uapi2_setHDRStrth failed: %d", nRet);
        return nRet;
    } 
  
    return nRet;
}

XCamReturn CIspControl::get_wdr_attr(WdrAttr_S &stWdrAttr) const
{
    XCamReturn nRet =  XCAM_RETURN_NO_ERROR;
    bool bEnable;  
    unsigned int nWdrLevel;  

    nRet = rk_aiq_uapi2_getHDRStrth(g_aiq_ctx,&bEnable,&nWdrLevel);
    if (nRet != XCAM_RETURN_NO_ERROR)
    {
        dlog(LOG_ERROR, "rk_aiq_uapi2_getContrast failed: %d", nRet);
        return nRet;
    }

    stWdrAttr.bEnable = bEnable;
    stWdrAttr.nWdrLevel = nWdrLevel; 
    return nRet;
}

XCamReturn CIspControl::set_hls_attr(const HlsAttr_S &stHlsAttr)
{
    XCamReturn nRet =  XCAM_RETURN_NO_ERROR;
    if(stHlsAttr.bEnable)
    {
        nRet =  rk_aiq_uapi2_setHLCMode(g_aiq_ctx, true);
        if (nRet != XCAM_RETURN_NO_ERROR)
        {
            dlog(LOG_ERROR, " rk_aiq_uapi2_setHLCMode failed: %d", nRet);
            return nRet;
        }

        nRet = rk_aiq_uapi2_setHLCStrength(g_aiq_ctx, stHlsAttr.nHlsLevel);
        if (nRet != XCAM_RETURN_NO_ERROR)
        {
            dlog(LOG_ERROR, " rk_aiq_uapi2_setHLCStrength failed: %d", nRet);
            return nRet;
        }
    }
    else 
    {
        nRet =  rk_aiq_uapi2_setHLCMode(g_aiq_ctx, false);
        if (nRet != XCAM_RETURN_NO_ERROR)
        {
            dlog(LOG_ERROR, " rk_aiq_uapi2_setHLCMode failed: %d", nRet);
            return nRet;
        }
    }

    return nRet;
}

XCamReturn CIspControl::get_hls_attr(HlsAttr_S& stHlsAttr) const
{
    XCamReturn nRet =  XCAM_RETURN_NO_ERROR;

    ae_api_linExpAttr_t linExpAttr;
    memset(&linExpAttr,0,sizeof(ae_api_linExpAttr_t));

    nRet = rk_aiq_user_api2_ae_getLinExpAttr(g_aiq_ctx, &linExpAttr);
    if (nRet != XCAM_RETURN_NO_ERROR)
    {
        dlog(LOG_ERROR, " rk_aiq_user_api2_ae_getLinExpAttr failed: %d", nRet);
        return nRet;
    }
    stHlsAttr.bEnable = linExpAttr.overExpCtrl.sw_aeT_overExp_en;
    stHlsAttr.nHlsLevel = (int)linExpAttr.overExpCtrl.sw_aeT_overExpBias_strg;

    return nRet;
}

XCamReturn CIspControl::set_backArea_attr(const BackLightArea_E &enBackLightArea)
{
    XCamReturn nRet =  XCAM_RETURN_NO_ERROR;
    if(enBackLightArea != ISP::CLOSE)
    {
        nRet = rk_aiq_uapi2_setBLCMode(g_aiq_ctx,true,(aeMeasAreaType_t)enBackLightArea);
    }
    else 
    {
        nRet = rk_aiq_uapi2_setBLCMode(g_aiq_ctx,true,AE_MEAS_AREA_AUTO);
    
    }
  
    return nRet;
}

XCamReturn CIspControl::get_backArea_attr(BackLightArea_E  &enBackLightArea) const
{
    XCamReturn nRet =  XCAM_RETURN_NO_ERROR;
    return nRet;
}

/**************************图像参数控制 end ************************/

/**************************高级参数控制 ************************/
int CIspControl::get_exposure_attr(ExposureAttr_S &stExpAttr) const
{
    XCamReturn nRet =  XCAM_RETURN_NO_ERROR;
    return nRet;
}

int CIspControl::set_exposure_attr(const ExposureAttr_S &stExpAttr)
{
    XCamReturn nRet =  XCAM_RETURN_NO_ERROR;
    #if 0
    //  /*设置为手动曝光模式*/
	// nRet = rk_aiq_uapi2_setExpMode(g_aiq_ctx, OP_MANUAL);
    // if (nRet != XCAM_RETURN_NO_ERROR)
    // {
    //     dlog(LOG_ERROR, " rk_aiq_uapi2_setExpMode failed: %d", nRet);
    //     return nRet;
    // }

    // nRet = rk_aiq_uapi2_setExpManualGain(g_aiq_ctx,fGain);
    // if (nRet != XCAM_RETURN_NO_ERROR)
    // {
    //     dlog(LOG_ERROR, " rk_aiq_uapi2_setExpManualTime failed: %d", nRet);
    //     return nRet;
    // }

    // /* 获取实际曝光时间（秒）*/
    // float fExpTimeSec = EXPOSURE_TIME_SEC_MAPPING[stExpAttr.enExpTime];

    // nRet = rk_aiq_uapi2_setExpManualTime(g_aiq_ctx, fExpTimeSec);
    // if (nRet != XCAM_RETURN_NO_ERROR)
    // {
    //     dlog(LOG_ERROR, " rk_aiq_uapi2_setExpManualTime failed: %d", nRet);
    //     return nRet;
    // }

    nRet = rk_aiq_uapi2_setAntiFlickerEn(g_aiq_ctx,stExpAttr.bAntiBanding);
    if (nRet != XCAM_RETURN_NO_ERROR)
    {
        dlog(LOG_ERROR, " rk_aiq_uapi2_setAntiFlickerEn failed: %d", nRet);
        return nRet;
    }
    #endif
    /*获取当前完整的线性曝光属性、保留其他字段*/
    ae_api_linExpAttr_t linExpAttr;
    memset(&linExpAttr, 0, sizeof(ae_api_linExpAttr_t));

    nRet = rk_aiq_user_api2_ae_getLinExpAttr(g_aiq_ctx, &linExpAttr);
    if (nRet != XCAM_RETURN_NO_ERROR)
    {
        dlog(LOG_ERROR, "rk_aiq_user_api2_ae_getLinExpAttr failed: %d", nRet);
        return nRet;
    }

    /*根据 enExpTime 选择对应档位*/
    int idx = stExpAttr.enExpTime;
    if (idx < 0 || idx >= AE_DYN_SETPOINT_NUM) 
    {
        idx = 2;
    }

    /*只更新 dynSetpoint 曲线*/
    linExpAttr.dynSetpoint = AE_DYN_SETPOINT_TABLE[idx];

    /*下发设置*/
    nRet = rk_aiq_user_api2_ae_setLinExpAttr(g_aiq_ctx, linExpAttr);
    if (nRet != XCAM_RETURN_NO_ERROR)
    {
        dlog(LOG_ERROR, "rk_aiq_user_api2_ae_setLinExpAttr failed: %d", nRet);
        return nRet;
    }
    
    nRet = rk_aiq_uapi2_setAntiFlickerEn(g_aiq_ctx,stExpAttr.bAntiBanding);
    if (nRet != XCAM_RETURN_NO_ERROR)
    {
        dlog(LOG_ERROR, " rk_aiq_uapi2_setAntiFlickerEn failed: %d", nRet);
        return nRet;
    }
    
    return nRet;
}

int CIspControl::get_awb_attr(AwbAttr_S &stAwbInfo) const
{
    XCamReturn nRet =  XCAM_RETURN_NO_ERROR;
    awb_api_attrib_t stAwbAttr;
    memset(&stAwbAttr, 0, sizeof(awb_api_attrib_t));
    nRet = rk_aiq_user_api2_awb_GetAttrib(g_aiq_ctx,&stAwbAttr);
    if(nRet != XCAM_RETURN_NO_ERROR)
    {
        dlog_error("rk_aiq_user_api2_awb_GetAttrib error,nRet:%d",nRet);
        return nRet;
    }

    switch (stAwbAttr.wbGainCtrl.opMode)
    {
        /* 自动白平衡 */ 
        case RK_AIQ_OP_MODE_AUTO:
            stAwbInfo.enAwbMode = AUTO_AWB_MODE;
            break;
        /* 手动白平衡 */ 
        case RK_AIQ_OP_MODE_MANUAL:
        {
            stAwbInfo.enAwbMode = MANUAL_AWB_MODE;
            unsigned int rg_gain = MAP_RANGE_TO_100(stAwbAttr.wbGainCtrl.manualPara.cfg.manual_wbgain[0]);
            unsigned int bg_gain = MAP_RANGE_TO_100(stAwbAttr.wbGainCtrl.manualPara.cfg.manual_wbgain[3]);
            stAwbInfo.nRGain = rg_gain;
            stAwbInfo.nBGain = bg_gain;
            break;
        }
        default:
            break;
    }
    
    return nRet;
}

int CIspControl::set_awb_attr(const AwbAttr_S& stAwbInfo)
{
    XCamReturn nRet =  XCAM_RETURN_NO_ERROR;
    awb_api_attrib_t stAwbAttr;
    memset(&stAwbAttr, 0, sizeof(awb_api_attrib_t));
    nRet = rk_aiq_user_api2_awb_GetAttrib(g_aiq_ctx,&stAwbAttr);
    if(nRet != XCAM_RETURN_NO_ERROR)
    {
        dlog_error("rk_aiq_user_api2_awb_GetAttrib error,nRet:%d",nRet);
        return nRet;
    }

    switch (stAwbInfo.enAwbMode)
    {
        /* 自动白平衡 */
        case AUTO_AWB_MODE:
        {
            stAwbAttr.wbGainCtrl.opMode = RK_AIQ_OP_MODE_AUTO;
            break;
        }
        /* 手动白平衡 */
        case MANUAL_AWB_MODE:
        {
            stAwbAttr.wbGainCtrl.opMode = RK_AIQ_OP_MODE_MANUAL;
            stAwbAttr.wbGainCtrl.manualPara.mode = mwb_mode_wbgain;
#ifdef DEVICE_TV_3882TI
            float rg_gain = MAP_100_TO_RGGAIN(stAwbInfo.nRGain);
            float bg_gain = MAP_100_TO_BGGAIN(stAwbInfo.nBGain);
            /* rg增益 */
            stAwbAttr.wbGainCtrl.manualPara.cfg.manual_wbgain[0] = rg_gain;
            /* bg增益 */
            stAwbAttr.wbGainCtrl.manualPara.cfg.manual_wbgain[3] = bg_gain;
#else
            float rg_gain = MAP_100_TO_RANGE(stAwbInfo.nRGain);
            float bg_gain = MAP_100_TO_RANGE(stAwbInfo.nBGain);
            /* rg增益 */
            stAwbAttr.wbGainCtrl.manualPara.cfg.manual_wbgain[0] = rg_gain;
            /* bg增益 */
            stAwbAttr.wbGainCtrl.manualPara.cfg.manual_wbgain[3] = bg_gain;
#endif
            break;
        }
        /* 锁定白平衡 */
        case LOCK_AWB_MODE:
        {
            stAwbAttr.wbGainCtrl.opMode = RK_AIQ_OP_MODE_MANUAL;
            stAwbAttr.wbGainCtrl.manualPara.mode = mwb_mode_scene;
            stAwbAttr.wbGainCtrl.manualPara.cfg.scene_mode = mwb_scene_fluorescent;
            break;
        }
        /* 白炽灯 */
        case INCANDESCENT_MODE:
        {
            stAwbAttr.wbGainCtrl.opMode = RK_AIQ_OP_MODE_MANUAL;
            stAwbAttr.wbGainCtrl.manualPara.mode = mwb_mode_scene;
            stAwbAttr.wbGainCtrl.manualPara.cfg.scene_mode = mwb_scene_incandescent;
            break;
        }
        /* 暖光灯 */
        case WARM_MODE:
        {
            stAwbAttr.wbGainCtrl.opMode = RK_AIQ_OP_MODE_MANUAL;
            stAwbAttr.wbGainCtrl.manualPara.mode = mwb_mode_scene;
            stAwbAttr.wbGainCtrl.manualPara.cfg.scene_mode = mwb_scene_fluorescent;
            break;
        }
        /* 日光灯 */
        case FLUORESCENT_MODE:
        {
            stAwbAttr.wbGainCtrl.opMode = RK_AIQ_OP_MODE_MANUAL;
            stAwbAttr.wbGainCtrl.manualPara.mode = mwb_mode_scene;
            stAwbAttr.wbGainCtrl.manualPara.cfg.scene_mode = mwb_scene_daylight;
            break;
        }
        /* 自然灯 */
        case DAY_LIGHT_MODE:
        {
            stAwbAttr.wbGainCtrl.opMode = RK_AIQ_OP_MODE_MANUAL;
            stAwbAttr.wbGainCtrl.manualPara.mode = mwb_mode_scene;
            stAwbAttr.wbGainCtrl.manualPara.cfg.scene_mode = mwb_scene_cloudy_daylight;
            break;
        }
        default:
        {
            break;
        }
    }

    nRet = rk_aiq_user_api2_awb_SetAttrib(g_aiq_ctx,&stAwbAttr);
    if(nRet != XCAM_RETURN_NO_ERROR)
    {
        dlog_error("rk_aiq_user_api2_awb_SetAttrib error,nRet:%d",nRet);
        return nRet;
    }

    return nRet;
}

int CIspControl::get_nr_attr(DnrAttr_S &stDnrInfo) const
{
    XCamReturn nRet =  XCAM_RETURN_NO_ERROR;
    /* 配置文件获取 */
    return nRet;
}

int CIspControl::set_nr_attr(const DnrAttr_S &stDnrInfo) 
{
    XCamReturn nRet =  XCAM_RETURN_NO_ERROR;
    opMode_t enMode;
   
    if(stDnrInfo.enDnrMode == CLOSE_MODE)
    {
        enMode = OP_AUTO;  
    }
    else 
    {
        enMode = OP_MANUAL;  
    }

    /* 去噪模式设置 */
    nRet = rk_aiq_uapi2_setNRMode(g_aiq_ctx,enMode);
    if(nRet != XCAM_RETURN_NO_ERROR)
    {
        dlog_error("rk_aiq_uapi2_setNRMode error nRet:%u",nRet);
        return ERR;
    }

    switch (stDnrInfo.enDnrMode)
    {
        /* 关闭 */
        case CLOSE_MODE:
        {
            break;
        }
        /* 普通模式 */
        case NORMAL_MODE:
        {
            nRet = rk_aiq_uapi2_setANRStrth(g_aiq_ctx,stDnrInfo.nDnrLevel);
            if(nRet != XCAM_RETURN_NO_ERROR)
            {
                dlog_error("rk_aiq_uapi2_setANRStrth error nRet:%u",nRet);
                return ERR;
            }
            break;
        }
        /* 专家模式 */
        case ADVANCED_MODE:
        {
            /* 时域降噪等级 */
            nRet =  rk_aiq_uapi2_setMTNRStrth(g_aiq_ctx,true,stDnrInfo.nSnrLevel);
            if(nRet != XCAM_RETURN_NO_ERROR)
            {
                dlog_error(" rk_aiq_uapi2_setMTNRStrth error nRet:%u",nRet);
                return ERR;
            }

            /* 空域降噪等级 */
            nRet = rk_aiq_uapi2_setMSpaNRStrth(g_aiq_ctx,true,stDnrInfo.nTnrLevel);
            if(nRet != XCAM_RETURN_NO_ERROR)
            {
                dlog_error("rk_aiq_uapi2_setMSpaNRStrth error nRet:%u",nRet);
                return ERR;
            }

            break;
        }
        default:
            break;
    }

    return nRet;
}

int CIspControl::get_backLight_attr(BackLightArrt_S &stBackAttr) const
{
    int nRet = IpcRet_E::OK;

    /* 背光区域 */
    get_backArea_attr(stBackAttr.enBackLightArea);

    /* 宽动态 */
    get_wdr_attr(stBackAttr.stWdrAttr);

    /* 强光抑制 */
    get_hls_attr(stBackAttr.stHlsAttr);

    return nRet;
}

int CIspControl::set_backLight_attr(const BackLightArrt_S &stBackAttr)
{
    XCamReturn nRet =  XCAM_RETURN_NO_ERROR;

    /* 宽动态和背光补偿同时启用时，仅宽动态生效 */
    if(stBackAttr.stWdrAttr.bEnable && stBackAttr.enBackLightArea != CLOSE)
    {  
        nRet = set_wdr_attr(stBackAttr.stWdrAttr);
        if(nRet != XCAM_RETURN_NO_ERROR)
        {
            dlog_error("set_wdr_attr error");
            return ERR;
        }


        /* 强光抑制 */
        nRet = set_hls_attr(stBackAttr.stHlsAttr);
        if(nRet != XCAM_RETURN_NO_ERROR)
        {
            dlog_error("set_hls_attr error");
            return ERR;
        }

    }
    else
    {
        /* 背光区域设置 */
        nRet = set_backArea_attr(stBackAttr.enBackLightArea);
        if(nRet != XCAM_RETURN_NO_ERROR)
        {
            dlog_error("set_backArea_attr error");
            return ERR;
        }

        nRet = set_wdr_attr(stBackAttr.stWdrAttr);
        if(nRet != XCAM_RETURN_NO_ERROR)
        {
            dlog_error("set_wdr_attr error");
            return ERR;
        }

        /* 强光抑制 */
        nRet = set_hls_attr(stBackAttr.stHlsAttr);
        if(nRet != XCAM_RETURN_NO_ERROR)
        {
            dlog_error("set_hls_attr error");
            return ERR;
        }

    }
    
    return nRet;
}

int CIspControl::get_videoMirror_attr(VideoAdjust_S &stVideoAdjust) const
{
    RK_S32 nRet;
    bool bMirror = false;
    bool bFlip = false;
    
    nRet = rk_aiq_uapi2_getMirrorFlip(g_aiq_ctx, &bMirror, &bFlip);
    if (nRet != RK_SUCCESS)
    {
        dlog(LOG_ERROR, "rk_aiq_uapi2_getMirrorFlip failed: %d", nRet);
        return nRet;
    }

    if (bMirror == false && bFlip == false)
    {
        stVideoAdjust.enMirrorMode = DISABLE;     
    }
    else if (bMirror == true && bFlip == false)
    {
        /* 只水平镜像 */
        stVideoAdjust.enMirrorMode = HORIZONTAL;   
    }
    else if (bMirror == false && bFlip == true)
    {
        /* 只垂直翻转 */
        stVideoAdjust.enMirrorMode = VERTICAL;   
    }
    else // (bMirror == RK_TRUE && bFlip == RK_TRUE)
    {
        /* 中心镜像 */
        stVideoAdjust.enMirrorMode = CENTER;       
    }

    dlog(LOG_INFO, "Get video mirror: mode=%d (bMirror=%d, bFlip=%d)", 
         stVideoAdjust.enMirrorMode, bMirror, bFlip);
    
    return RK_SUCCESS;
}

int CIspControl::set_videoMirror_attr(const VideoAdjust_S & stVideoAdjust)
{
    unsigned char reg_value = 0x00;  // 默认关闭状态

    switch (stVideoAdjust.enMirrorMode)
    {
        case DISABLE:
        {
            reg_value = 0x00;  // 关闭
            break;
        }
        case HORIZONTAL:
        {
            reg_value = 0x06;  // 左右镜像
            break;
        }
        case VERTICAL:
        {
            reg_value = 0x60;  // 上下翻转
            break;
        }
        case CENTER:
        {
            reg_value = 0x66;  // 中心镜像
            break;
        }
        default:
            dlog(LOG_ERROR, "Invalid mirror mode: %d", stVideoAdjust.enMirrorMode);
            return ERR; 
    }


    char aCmd[128];
    snprintf(aCmd, sizeof(aCmd), "i2ctransfer -f -y 3 w3@0x30 0x32 0x21 0x%02X", reg_value);
    
    dlog(LOG_INFO, "Executing I2C command: %s", aCmd);
    
    int nRet = system(aCmd);
    if (nRet != 0) {
        dlog(LOG_ERROR, "i2ctransfer command failed: %d (mode=%d, reg_value=0x%02X)", 
            nRet, stVideoAdjust.enMirrorMode, reg_value);
        return ERR;
    }

    dlog(LOG_INFO, "Set video mirror success: mode=%d, reg_value=0x%02X", 
        stVideoAdjust.enMirrorMode, reg_value);
    
    return RK_SUCCESS;
}

int CIspControl::get_dayNight_attr(DayNightAttr_S &stDayNightAttr) const
{
    return CIspConfigure::instance()->get_configure(stDayNightAttr);
}

int CIspControl::set_dayNight_attr(const DayNightAttr_S &stDayNightAttr)
{
    (void) stDayNightAttr;
    /* ! 日夜模式、定时和过滤由共享CIspBusinessService统一裁决，禁止CIspControl绕过运行态仲裁器。 */
    /* note: 保留旧接口仅为兼容已有调用方，实际请求必须改走CIspManage。 */
    return ERR_UNSUPPORT;
}

/**************************高级参数控制 end************************/
