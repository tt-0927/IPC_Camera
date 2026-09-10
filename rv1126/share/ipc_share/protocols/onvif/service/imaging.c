/**
 * @file imaging.c
 * @author tianl (tianl@kfb.cn)
 * @date 2025-09-23
 * 
 * @brief onvif imaging服务接口
 */
#include "onvif_server_wrapper.h"

SOAP_FMAC5 int SOAP_FMAC6 __timg__GetServiceCapabilities(struct soap* soap, struct _timg__GetServiceCapabilities *timg__GetServiceCapabilities, struct _timg__GetServiceCapabilitiesResponse *timg__GetServiceCapabilitiesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__timg__GetServiceCapabilities----------");
#endif
    if(timg__GetServiceCapabilitiesResponse == NULL)
    {
        dlog_error("timg__GetServiceCapabilitiesResponse is NULL");
        return soap_sender_fault(soap,  "timg__GetServiceCapabilitiesResponse is NULL",  NULL);
    }
    timg__GetServiceCapabilitiesResponse->Capabilities = soap_new_timg__Capabilities(soap, -1);
    timg__GetServiceCapabilitiesResponse->Capabilities->ImageStabilization = soap_new_xsd__boolean(soap, -1);
    *(timg__GetServiceCapabilitiesResponse->Capabilities->ImageStabilization) = xsd__boolean__false_;

    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __timg__GetImagingSettings(struct soap* soap, struct _timg__GetImagingSettings *timg__GetImagingSettings, struct _timg__GetImagingSettingsResponse *timg__GetImagingSettingsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__timg__GetImagingSettings----------");
#endif
    int nRet = onvif_authentication(soap);
    if(SOAP_OK != nRet)
    {
        return nRet;
    }
    /* token校验 */
    if(timg__GetImagingSettings != NULL && timg__GetImagingSettings->VideoSourceToken != NULL)
    {
        if(strcmp(timg__GetImagingSettings->VideoSourceToken,PROFILE1_VIDEOSOURCE_SOURCETOKEN) != 0)
        {
            
            dlog_error("requested  VideoSource does not exist");
            return soap_receiver_fault(soap, "requested VideoSource does not exist", NULL); 
        }
    }
    else
    {
        return soap_receiver_fault(soap, "requested VideoSource does not exist", NULL); 
    }

    if(timg__GetImagingSettingsResponse == NULL)
    {
        dlog_error("timg__GetImagingSettingsResponse is NULL");
        return soap_sender_fault(soap,  "timg__GetImagingSettingsResponse is NULL",  NULL);
    }

    if(strcmp(timg__GetImagingSettings->VideoSourceToken, PROFILE1_VIDEOSOURCE_SOURCETOKEN) != 0 )
    {
        dlog_error("VideoSourceToken %s is invalid", timg__GetImagingSettings->VideoSourceToken);
        return soap_sender_fault(soap,  "VideoSourceToken is invalid", NULL);
    }

    OnvifImageParam_t OnvifImageParam;
    nRet = onvif_get_imageParam(&OnvifImageParam);
    if(nRet != 0)
    {
        dlog_error("onvif_get_imageParam error");
        return soap_receiver_fault(soap, "onvif_get_imageParam error", NULL);
    }
    
    timg__GetImagingSettingsResponse->ImagingSettings = soap_new_tt__ImagingSettings20(soap, -1);
    if(!timg__GetImagingSettingsResponse->ImagingSettings) 
    {
        dlog_error("Failed to allocate ImagingSettings");
        return soap_receiver_fault(soap, "Memory allocation failed", NULL);
    }

    soap_default_tt__ImagingSettings20(soap, timg__GetImagingSettingsResponse->ImagingSettings);

    timg__GetImagingSettingsResponse->ImagingSettings->ColorSaturation = soap_new_float(soap, -1);
    timg__GetImagingSettingsResponse->ImagingSettings->Brightness = soap_new_float(soap, -1);
    timg__GetImagingSettingsResponse->ImagingSettings->Sharpness = soap_new_float(soap, -1);
    timg__GetImagingSettingsResponse->ImagingSettings->Contrast = soap_new_float(soap, -1);
    if(!timg__GetImagingSettingsResponse->ImagingSettings->ColorSaturation || !timg__GetImagingSettingsResponse->ImagingSettings->Brightness 
        || !timg__GetImagingSettingsResponse->ImagingSettings->Sharpness || !timg__GetImagingSettingsResponse->ImagingSettings->Contrast)
    {
        dlog_error("Failed to allocate ImagingSettings");
        return soap_receiver_fault(soap, "Memory allocation failed", NULL);
    }

    OnvifImageParam.nBrightness = OnvifImageParam.nBrightness < BRIGHTNESS_RANGE_MIN ? BRIGHTNESS_RANGE_MIN : OnvifImageParam.nBrightness;
    OnvifImageParam.nBrightness = OnvifImageParam.nBrightness > BRIGHTNESS_RANGE_MAX ? BRIGHTNESS_RANGE_MAX : OnvifImageParam.nBrightness;
    *(timg__GetImagingSettingsResponse->ImagingSettings->Brightness) = OnvifImageParam.nBrightness;

    OnvifImageParam.nSaturation = OnvifImageParam.nSaturation < SATURATION_RANGE_MIN ? SATURATION_RANGE_MIN : OnvifImageParam.nSaturation;
    OnvifImageParam.nSaturation = OnvifImageParam.nSaturation > SATURATION_RANGE_MAX ? SATURATION_RANGE_MAX : OnvifImageParam.nSaturation;
    *(timg__GetImagingSettingsResponse->ImagingSettings->ColorSaturation) = OnvifImageParam.nSaturation;

    OnvifImageParam.nSharpness = OnvifImageParam.nSharpness < SHARPNESS_RANGE_MIN ? SHARPNESS_RANGE_MIN : OnvifImageParam.nSharpness;
    OnvifImageParam.nSharpness = OnvifImageParam.nSharpness > SHARPNESS_RANGE_MAX ? SHARPNESS_RANGE_MAX : OnvifImageParam.nSharpness;
    *(timg__GetImagingSettingsResponse->ImagingSettings->Sharpness) = OnvifImageParam.nSharpness;

    OnvifImageParam.nContrast = OnvifImageParam.nContrast < CONTRAST_RANGE_MIN ? CONTRAST_RANGE_MIN : OnvifImageParam.nContrast;
    OnvifImageParam.nContrast = OnvifImageParam.nContrast > CONTRAST_RANGE_MAX ? CONTRAST_RANGE_MAX : OnvifImageParam.nContrast;
    *(timg__GetImagingSettingsResponse->ImagingSettings->Contrast) = OnvifImageParam.nContrast;

    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __timg__SetImagingSettings(struct soap* soap, struct _timg__SetImagingSettings *timg__SetImagingSettings, struct _timg__SetImagingSettingsResponse *timg__SetImagingSettingsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__timg__SetImagingSettings----------");
#endif
    if(timg__SetImagingSettings == NULL || timg__SetImagingSettings->ImagingSettings == NULL)
    {
        dlog_error("timg__SetImagingSettingsResponse is NULL");
        return SOAP_FAULT;
    }

    if(strcmp(timg__SetImagingSettings->VideoSourceToken, PROFILE1_VIDEOSOURCE_SOURCETOKEN) != 0 )
    {
        dlog_error("VideoSourceToken %s is invalid", timg__SetImagingSettings->VideoSourceToken);
        return SOAP_FAULT;
    }

    int nRet = 0;
    OnvifImageParam_t OnvifImageParam;

    

    if(timg__SetImagingSettings->ImagingSettings->Brightness != NULL)
    {
        dlog_debug("onvif设置图像参数 亮度[%d]  ", (int)*(timg__SetImagingSettings->ImagingSettings->Brightness));
        OnvifImageParam.nBrightness = (int)*(timg__SetImagingSettings->ImagingSettings->Brightness);
        OnvifImageParam.nBrightness = OnvifImageParam.nBrightness < BRIGHTNESS_RANGE_MIN ? BRIGHTNESS_RANGE_MIN : OnvifImageParam.nBrightness;
        OnvifImageParam.nBrightness = OnvifImageParam.nBrightness > BRIGHTNESS_RANGE_MAX ? BRIGHTNESS_RANGE_MAX : OnvifImageParam.nBrightness;
    }
    
    if(timg__SetImagingSettings->ImagingSettings->ColorSaturation != NULL)
    {
        dlog_debug("onvif设置图像参数 饱和度[%d]  ", (int)*(timg__SetImagingSettings->ImagingSettings->ColorSaturation));
        OnvifImageParam.nSaturation = (int)*(timg__SetImagingSettings->ImagingSettings->ColorSaturation);
        OnvifImageParam.nSaturation = OnvifImageParam.nSaturation < SATURATION_RANGE_MIN ? SATURATION_RANGE_MIN : OnvifImageParam.nSaturation;
        OnvifImageParam.nSaturation = OnvifImageParam.nSaturation > SATURATION_RANGE_MAX ? SATURATION_RANGE_MAX : OnvifImageParam.nSaturation;
    }

    if(timg__SetImagingSettings->ImagingSettings->Sharpness != NULL)
    {
        dlog_debug("onvif设置图像参数 锐度[%d]  ",(int)*( timg__SetImagingSettings->ImagingSettings->Sharpness));
        OnvifImageParam.nSharpness = (int)*(timg__SetImagingSettings->ImagingSettings->Sharpness);
        OnvifImageParam.nSharpness = OnvifImageParam.nSharpness < SHARPNESS_RANGE_MIN ? SHARPNESS_RANGE_MIN : OnvifImageParam.nSharpness;
        OnvifImageParam.nSharpness = OnvifImageParam.nSharpness > SHARPNESS_RANGE_MAX ? SHARPNESS_RANGE_MAX : OnvifImageParam.nSharpness;
    }

    if(timg__SetImagingSettings->ImagingSettings->Contrast != NULL)
    {
        dlog_debug("onvif设置图像参数 对比度[%d]  ", (int)*(timg__SetImagingSettings->ImagingSettings->Contrast));
        OnvifImageParam.nContrast = (int)*(timg__SetImagingSettings->ImagingSettings->Contrast);
        OnvifImageParam.nContrast = OnvifImageParam.nContrast < CONTRAST_RANGE_MIN ? CONTRAST_RANGE_MIN : OnvifImageParam.nContrast;
        OnvifImageParam.nContrast = OnvifImageParam.nContrast > CONTRAST_RANGE_MAX ? CONTRAST_RANGE_MAX : OnvifImageParam.nContrast;
    }
  
    nRet = onvif_set_imageParam(&OnvifImageParam);
    if(nRet != 0)
    {
        dlog_error("onvif_set_imageParam error");
        return SOAP_FAULT;
    }
    
    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __timg__GetOptions(struct soap* soap, struct _timg__GetOptions *timg__GetOptions, struct _timg__GetOptionsResponse *timg__GetOptionsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__timg__GetOptions----------");
#endif

    int nRet = onvif_authentication(soap);
    if(SOAP_OK != nRet)
    {
        return nRet;
    }
    /* token校验 */
    if(timg__GetOptions != NULL && timg__GetOptions->VideoSourceToken != NULL)
    {
        if(strcmp(timg__GetOptions->VideoSourceToken,PROFILE1_VIDEOSOURCE_SOURCETOKEN) != 0)
        {
            
            dlog_error("requested  VideoSource does not exist");
            return soap_receiver_fault(soap, "requested VideoSource does not exist", NULL); 
        }
    }
    else
    {
        return soap_receiver_fault(soap, "requested VideoSource does not exist", NULL); 
    }

    if(timg__GetOptionsResponse == NULL)
    {
        dlog_error("timg__GetOptionsResponse is NULL");
        return soap_receiver_fault(soap, "GetOptionsResponse is NULL", NULL);    
    }

    timg__GetOptionsResponse->ImagingOptions = soap_new_tt__ImagingOptions20(soap,-1);
    /* 背光补偿 */
    timg__GetOptionsResponse->ImagingOptions->BacklightCompensation = soap_new_tt__BacklightCompensationOptions20(soap,-1);
    timg__GetOptionsResponse->ImagingOptions->BacklightCompensation->__sizeMode = 2;
    timg__GetOptionsResponse->ImagingOptions->BacklightCompensation->Mode = soap_new_tt__BacklightCompensationMode(soap,timg__GetOptionsResponse->ImagingOptions->BacklightCompensation->__sizeMode);
    *(timg__GetOptionsResponse->ImagingOptions->BacklightCompensation->Mode + 0) = tt__BacklightCompensationMode__OFF;
    *(timg__GetOptionsResponse->ImagingOptions->BacklightCompensation->Mode + 1) = tt__BacklightCompensationMode__ON;

    /* 亮度 */
    timg__GetOptionsResponse->ImagingOptions->Brightness = soap_new_tt__FloatRange(soap,-1);
    timg__GetOptionsResponse->ImagingOptions->Brightness->Min = BRIGHTNESS_RANGE_MIN;
    timg__GetOptionsResponse->ImagingOptions->Brightness->Max = BRIGHTNESS_RANGE_MAX;

    /* 饱和度 */
    timg__GetOptionsResponse->ImagingOptions->ColorSaturation = soap_new_tt__FloatRange(soap,-1);
    timg__GetOptionsResponse->ImagingOptions->ColorSaturation->Min = SATURATION_RANGE_MIN;
    timg__GetOptionsResponse->ImagingOptions->ColorSaturation->Max = SATURATION_RANGE_MAX;

    /* 对比度 */
    timg__GetOptionsResponse->ImagingOptions->Contrast = soap_new_tt__FloatRange(soap,-1);
    timg__GetOptionsResponse->ImagingOptions->Contrast->Min = CONTRAST_RANGE_MIN;
    timg__GetOptionsResponse->ImagingOptions->Contrast->Max = CONTRAST_RANGE_MAX;

    /* 清晰度 */
    timg__GetOptionsResponse->ImagingOptions->Sharpness = soap_new_tt__FloatRange(soap,-1);
    timg__GetOptionsResponse->ImagingOptions->Sharpness->Min = SHARPNESS_RANGE_MIN;
    timg__GetOptionsResponse->ImagingOptions->Sharpness->Max = SHARPNESS_RANGE_MAX;



    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __timg__Move(struct soap* soap, struct _timg__Move *timg__Move, struct _timg__MoveResponse *timg__MoveResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__timg__Move----------");
#endif
    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __timg__Stop(struct soap* soap, struct _timg__Stop *timg__Stop, struct _timg__StopResponse *timg__StopResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__timg__Stop----------");
#endif
    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __timg__GetStatus(struct soap* soap, struct _timg__GetStatus *timg__GetStatus, struct _timg__GetStatusResponse *timg__GetStatusResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__timg__GetStatus----------");
#endif
    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __timg__GetMoveOptions(struct soap* soap, struct _timg__GetMoveOptions *timg__GetMoveOptions, struct _timg__GetMoveOptionsResponse *timg__GetMoveOptionsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__timg__GetMoveOptions----------");
#endif
     int nRet = onvif_authentication(soap);
    if(SOAP_OK != nRet)
    {
        return nRet;
    }
    /* token校验 */
    if(timg__GetMoveOptions != NULL && timg__GetMoveOptions->VideoSourceToken != NULL)
    {
        if(strcmp(timg__GetMoveOptions->VideoSourceToken,PROFILE1_VIDEOSOURCE_SOURCETOKEN) != 0)
        {
            
            dlog_error("requested  VideoSource does not exist");
            return soap_receiver_fault(soap, "requested VideoSource does not exist", NULL); 
        }

        dlog_debug("----------__timg__GetMoveOptions---source token[%s]-------",timg__GetMoveOptions->VideoSourceToken);
    }
    else
    {
        return soap_receiver_fault(soap, "requested VideoSource does not exist", NULL); 
    }

    timg__GetMoveOptionsResponse->MoveOptions = soap_new_tt__MoveOptions20(soap,-1);

    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __timg__GetPresets(struct soap* soap, struct _timg__GetPresets *timg__GetPresets, struct _timg__GetPresetsResponse *timg__GetPresetsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__timg__GetPresets----------");
#endif
    return SOAP_OK;
}
SOAP_FMAC5 int SOAP_FMAC6 __timg__GetCurrentPreset(struct soap* soap, struct _timg__GetCurrentPreset *timg__GetCurrentPreset, struct _timg__GetCurrentPresetResponse *timg__GetCurrentPresetResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__timg__GetCurrentPreset----------");
#endif
    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __timg__SetCurrentPreset(struct soap* soap, struct _timg__SetCurrentPreset *timg__SetCurrentPreset, struct _timg__SetCurrentPresetResponse *timg__SetCurrentPresetResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__timg__SetCurrentPreset----------");
#endif
    return SOAP_OK;
}