/**
 * @file media.c
 * @author tianl (tianl@kfb.cn)
 * @date 2025-09-23
 * 
 * @brief onvif media服务接口
 */
#include "onvif_server_wrapper.h"

static int get_profile_param(int nIndex, OnvifProfile_t *pstProfile)
{
    if (nIndex == 0)
    {
        onvif_get_profileParam(pstProfile, 0);

        memcpy(pstProfile->Name, PROFILE1_NAME, sizeof(pstProfile->Name));
        memcpy(pstProfile->token, PROFILE1_TOKEN, sizeof(pstProfile->token));

        memcpy(pstProfile->VideoSourceConfiguration_Name, VIDEOSOURCE_NAME, sizeof(pstProfile->VideoSourceConfiguration_Name));
        memcpy(pstProfile->VideoSourceConfiguration_token, VIDEOSOURCE_TOKEN, sizeof(pstProfile->VideoSourceConfiguration_token));
        memcpy(pstProfile->VideoSourceConfiguration_SourceToken, PROFILE1_VIDEOSOURCE_SOURCETOKEN, sizeof(pstProfile->VideoSourceConfiguration_SourceToken));
        
        memcpy(pstProfile->VideoEncoderConfiguration_Name, PROFILE1_VIDEOENCODER_NAME, sizeof(pstProfile->VideoEncoderConfiguration_Name));
        memcpy(pstProfile->VideoEncoderConfiguration_token, PROFILE1_VIDEOENCODER_TOKEN, sizeof(pstProfile->VideoEncoderConfiguration_token));
    }
    else if (nIndex == 1)
    {
        onvif_get_profileParam(pstProfile, 1);

        memcpy(pstProfile->Name, PROFILE2_NAME, sizeof(pstProfile->Name));
        memcpy(pstProfile->token, PROFILE2_TOKEN, sizeof(pstProfile->token));
        memcpy(pstProfile->VideoSourceConfiguration_Name, VIDEOSOURCE_NAME, sizeof(pstProfile->VideoSourceConfiguration_Name));
        memcpy(pstProfile->VideoSourceConfiguration_token, VIDEOSOURCE_TOKEN, sizeof(pstProfile->VideoSourceConfiguration_token));
        memcpy(pstProfile->VideoSourceConfiguration_SourceToken, PROFILE1_VIDEOSOURCE_SOURCETOKEN, sizeof(pstProfile->VideoSourceConfiguration_SourceToken));
        

        memcpy(pstProfile->VideoEncoderConfiguration_Name, PROFILE2_VIDEOENCODER_NAME, sizeof(pstProfile->VideoEncoderConfiguration_Name));
        memcpy(pstProfile->VideoEncoderConfiguration_token, PROFILE2_VIDEOENCODER_TOKEN, sizeof(pstProfile->VideoEncoderConfiguration_token));
    }

    memcpy(pstProfile->AudioSourceConfiguration_Name, PROFILE_AUDIOSOURCE_NAME, sizeof(pstProfile->AudioSourceConfiguration_Name));
    memcpy(pstProfile->AudioSourceConfiguration_token, PROFILE_AUDIOSOURCE_TOKEN, sizeof(pstProfile->AudioSourceConfiguration_token));
    memcpy(pstProfile->AudioSourceConfiguration_SourceToken, PROFILE_AUDIOSOURCE_SOURCETOKEN, sizeof(pstProfile->AudioSourceConfiguration_SourceToken));
    memcpy(pstProfile->AudioEncoderConfiguration_Name, PROFILE_AUDIOENCODER_NAME, sizeof(pstProfile->AudioEncoderConfiguration_Name));
    memcpy(pstProfile->AudioEncoderConfiguration_token, PROFILE_AUDIOENCODER_TOKEN, sizeof(pstProfile->AudioEncoderConfiguration_token));

    return SOAP_OK;
}

/** Web service operation '__trt__GetServiceCapabilities' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetServiceCapabilities(struct soap* soap, struct _trt__GetServiceCapabilities *trt__GetServiceCapabilities, struct _trt__GetServiceCapabilitiesResponse *trt__GetServiceCapabilitiesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetServiceCapabilities----------");
#endif
    soap_wsse_delete_Security(soap);
    trt__GetServiceCapabilitiesResponse->Capabilities = (struct trt__Capabilities *)soap_malloc(soap, sizeof(struct trt__Capabilities));
    memset(trt__GetServiceCapabilitiesResponse->Capabilities, '\0', sizeof(struct trt__Capabilities));
 
    // trt__GetServiceCapabilitiesResponse->Capabilities->ProfileCapabilities = (struct trt__ProfileCapabilities *)soap_malloc(soap, sizeof(struct trt__ProfileCapabilities)); 
    // trt__GetServiceCapabilitiesResponse->Capabilities->ProfileCapabilities->MaximumNumberOfProfiles = (int *)soap_malloc(soap, sizeof(int)); 
    // *(trt__GetServiceCapabilitiesResponse->Capabilities->ProfileCapabilities->MaximumNumberOfProfiles) = 2;

    trt__GetServiceCapabilitiesResponse->Capabilities->StreamingCapabilities = (struct trt__StreamingCapabilities *)soap_malloc(soap, sizeof(struct trt__StreamingCapabilities)); 
    
    trt__GetServiceCapabilitiesResponse->Capabilities->StreamingCapabilities->RTPMulticast = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(trt__GetServiceCapabilitiesResponse->Capabilities->StreamingCapabilities->RTPMulticast) = xsd__boolean__false_;
    
    trt__GetServiceCapabilitiesResponse->Capabilities->StreamingCapabilities->RTP_USCORETCP = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(trt__GetServiceCapabilitiesResponse->Capabilities->StreamingCapabilities->RTP_USCORETCP) = xsd__boolean__true_;
    
    trt__GetServiceCapabilitiesResponse->Capabilities->StreamingCapabilities->RTP_USCORERTSP_USCORETCP = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(trt__GetServiceCapabilitiesResponse->Capabilities->StreamingCapabilities->RTP_USCORERTSP_USCORETCP) = xsd__boolean__true_;
    
    // trt__GetServiceCapabilitiesResponse->Capabilities->StreamingCapabilities->NonAggregateControl = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    // *(trt__GetServiceCapabilitiesResponse->Capabilities->StreamingCapabilities->NonAggregateControl) = xsd__boolean__false_;
    // trt__GetServiceCapabilitiesResponse->Capabilities->StreamingCapabilities->NoRTSPStreaming = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    // *(trt__GetServiceCapabilitiesResponse->Capabilities->StreamingCapabilities->NoRTSPStreaming) = xsd__boolean__false_;

    trt__GetServiceCapabilitiesResponse->Capabilities->SnapshotUri = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(trt__GetServiceCapabilitiesResponse->Capabilities->SnapshotUri) = xsd__boolean__true_;

    trt__GetServiceCapabilitiesResponse->Capabilities->Rotation = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(trt__GetServiceCapabilitiesResponse->Capabilities->Rotation) = xsd__boolean__false_;

    trt__GetServiceCapabilitiesResponse->Capabilities->VideoSourceMode = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(trt__GetServiceCapabilitiesResponse->Capabilities->VideoSourceMode) = xsd__boolean__true_;

    trt__GetServiceCapabilitiesResponse->Capabilities->OSD = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(trt__GetServiceCapabilitiesResponse->Capabilities->OSD) = xsd__boolean__true_;

    trt__GetServiceCapabilitiesResponse->Capabilities->TemporaryOSDText = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(trt__GetServiceCapabilitiesResponse->Capabilities->TemporaryOSDText) = xsd__boolean__true_;

    return SOAP_OK;
}

/** Web service operation '__trt__GetVideoSources' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetVideoSources(struct soap* soap, struct _trt__GetVideoSources *trt__GetVideoSources, struct _trt__GetVideoSourcesResponse *trt__GetVideoSourcesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetVideoSources----------");
#endif
    OnvifProfile_t stProfile;
    int size = 2;
    trt__GetVideoSourcesResponse->__sizeVideoSources = size;
    trt__GetVideoSourcesResponse->VideoSources = (struct tt__VideoSource *)soap_malloc(soap, sizeof(struct tt__VideoSource) * trt__GetVideoSourcesResponse->__sizeVideoSources);
    memset(trt__GetVideoSourcesResponse->VideoSources, '\0', sizeof(struct tt__VideoSource) * trt__GetVideoSourcesResponse->__sizeVideoSources);

    for(int i = 0; i < trt__GetVideoSourcesResponse->__sizeVideoSources; i++)
    {
        memset(&stProfile, 0, sizeof(stProfile));
        get_profile_param(i, &stProfile);

        trt__GetVideoSourcesResponse->VideoSources[i].token = (char *)soap_malloc(soap, sizeof(char) * 32);
        memset(trt__GetVideoSourcesResponse->VideoSources[i].token, '\0', sizeof(char) * 32);
        strcpy(trt__GetVideoSourcesResponse->VideoSources[i].token, stProfile.VideoSourceConfiguration_token);

        trt__GetVideoSourcesResponse->VideoSources[i].Resolution = (struct tt__VideoResolution *)soap_malloc(soap, sizeof(struct tt__VideoResolution));
        memset(trt__GetVideoSourcesResponse->VideoSources[i].Resolution, '\0', sizeof(struct tt__VideoResolution));
        trt__GetVideoSourcesResponse->VideoSources->Resolution[i].Width = stProfile.nWidth;
        trt__GetVideoSourcesResponse->VideoSources->Resolution[i].Height = stProfile.nHeight;
        trt__GetVideoSourcesResponse->VideoSources[i].Framerate = stProfile.FrameRateLimit;
    }

    return SOAP_OK;
}

/** Web service operation '__trt__GetAudioSources' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioSources(struct soap* soap, struct _trt__GetAudioSources *trt__GetAudioSources, struct _trt__GetAudioSourcesResponse *trt__GetAudioSourcesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetAudioSources----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__GetAudioOutputs' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioOutputs(struct soap* soap, struct _trt__GetAudioOutputs *trt__GetAudioOutputs, struct _trt__GetAudioOutputsResponse *trt__GetAudioOutputsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetAudioOutputs----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__CreateProfile' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__CreateProfile(struct soap* soap, struct _trt__CreateProfile *trt__CreateProfile, struct _trt__CreateProfileResponse *trt__CreateProfileResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__CreateProfile----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__GetProfile' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetProfile(struct soap* soap, struct _trt__GetProfile *trt__GetProfile, struct _trt__GetProfileResponse *trt__GetProfileResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetProfile----------");
#endif
    trt__GetProfileResponse->Profile = (struct tt__Profile *)soap_malloc(soap, sizeof(struct tt__Profile));
    memset(trt__GetProfileResponse->Profile, '\0', sizeof(struct tt__Profile));
    OnvifProfile_t stProfile;
    memset(&stProfile, 0, sizeof(OnvifProfile_t));

    if (trt__GetProfile->ProfileToken)
    {
        if (strcmp(PROFILE1_TOKEN, trt__GetProfile->ProfileToken) == 0)
        {
            get_profile_param(0, &stProfile);
        }
        else if(strcmp(PROFILE2_TOKEN, trt__GetProfile->ProfileToken) == 0)
        {
            get_profile_param(1, &stProfile);
        }
    }
    else
    {
        dlog_error("ProfileToken is NULL");
        return SOAP_EOF;
    }

    //<profiles><name>和<profiles><token>
    trt__GetProfileResponse->Profile->Name = (char *)soap_malloc(soap, sizeof(char) * 32);
    memset(trt__GetProfileResponse->Profile->Name, '\0', sizeof(char) * 32);
    strcpy(trt__GetProfileResponse->Profile->Name, stProfile.Name);
    trt__GetProfileResponse->Profile->token = (char *)soap_malloc(soap, sizeof(char) * 32);
    memset(trt__GetProfileResponse->Profile->token, '\0', sizeof(char) * 32);
    strcpy(trt__GetProfileResponse->Profile->token, stProfile.token);
    trt__GetProfileResponse->Profile->fixed = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(trt__GetProfileResponse->Profile->fixed) = xsd__boolean__true_;

    //<VideoSourceConfiguration><name>和<VideoSourceConfiguration><token>
    trt__GetProfileResponse->Profile->VideoSourceConfiguration = (struct tt__VideoSourceConfiguration *)soap_malloc(soap,sizeof(struct tt__VideoSourceConfiguration));
    memset(trt__GetProfileResponse->Profile->VideoSourceConfiguration, 0, sizeof(struct tt__VideoSourceConfiguration));
    trt__GetProfileResponse->Profile->VideoSourceConfiguration->Name = (char *)soap_malloc(soap,sizeof(char) * 32);
    memset(trt__GetProfileResponse->Profile->VideoSourceConfiguration->Name, '\0', sizeof(char) * 32);
    strcpy(trt__GetProfileResponse->Profile->VideoSourceConfiguration->Name, stProfile.VideoSourceConfiguration_Name);
    trt__GetProfileResponse->Profile->VideoSourceConfiguration->token = (char *)soap_malloc(soap,sizeof(char) * 32);
    memset(trt__GetProfileResponse->Profile->VideoSourceConfiguration->token, '\0', sizeof(char) * 32);
    strcpy(trt__GetProfileResponse->Profile->VideoSourceConfiguration->token, stProfile.VideoSourceConfiguration_token);
    trt__GetProfileResponse->Profile->VideoSourceConfiguration->SourceToken = (char *)soap_malloc(soap,sizeof(char) * 32);
    memset(trt__GetProfileResponse->Profile->VideoSourceConfiguration->SourceToken, '\0', sizeof(char) * 32);
    strcpy(trt__GetProfileResponse->Profile->VideoSourceConfiguration->SourceToken, stProfile.VideoSourceConfiguration_SourceToken);
    trt__GetProfileResponse->Profile->VideoSourceConfiguration->UseCount = 2;
    //<VideoSourceConfiguration><Bounds>
    trt__GetProfileResponse->Profile->VideoSourceConfiguration->Bounds = (struct tt__IntRectangle *)soap_malloc(soap, sizeof(struct tt__IntRectangle));
    memset(trt__GetProfileResponse->Profile->VideoSourceConfiguration->Bounds, 0, sizeof(struct tt__IntRectangle));
    trt__GetProfileResponse->Profile->VideoSourceConfiguration->Bounds->x = 0;
    trt__GetProfileResponse->Profile->VideoSourceConfiguration->Bounds->y = 0;
    trt__GetProfileResponse->Profile->VideoSourceConfiguration->Bounds->width = stProfile.nWidth;
    trt__GetProfileResponse->Profile->VideoSourceConfiguration->Bounds->height = stProfile.nHeight;

    //<VideoEncoderConfiguration>
    trt__GetProfileResponse->Profile->VideoEncoderConfiguration = (struct tt__VideoEncoderConfiguration *)soap_malloc(soap, sizeof(struct tt__VideoEncoderConfiguration)) ;
    memset(trt__GetProfileResponse->Profile->VideoEncoderConfiguration, '\0', sizeof(struct tt__VideoEncoderConfiguration));
    trt__GetProfileResponse->Profile->VideoEncoderConfiguration->Name = (char *)soap_malloc(soap, sizeof(char)*32);
    memset(trt__GetProfileResponse->Profile->VideoEncoderConfiguration->Name, '\0', sizeof(char)*32);
    strcpy(trt__GetProfileResponse->Profile->VideoEncoderConfiguration->Name, stProfile.VideoEncoderConfiguration_Name);
    trt__GetProfileResponse->Profile->VideoEncoderConfiguration->token = (char *)soap_malloc(soap, sizeof(char)*32);
    memset(trt__GetProfileResponse->Profile->VideoEncoderConfiguration->token, '\0', sizeof(char)*32);
    strcpy(trt__GetProfileResponse->Profile->VideoEncoderConfiguration->token, stProfile.VideoEncoderConfiguration_token);
    trt__GetProfileResponse->Profile->VideoEncoderConfiguration->UseCount = 1;
    trt__GetProfileResponse->Profile->VideoEncoderConfiguration->Encoding = tt__VideoEncoding__H264;
    //<VideoEncoderConfiguration><Resolution>、<RateControl>
    trt__GetProfileResponse->Profile->VideoEncoderConfiguration->Resolution = (struct tt__VideoResolution *)soap_malloc(soap, sizeof(struct tt__VideoResolution));
    memset(trt__GetProfileResponse->Profile->VideoEncoderConfiguration->Resolution, '\0', sizeof(struct tt__VideoResolution));
    trt__GetProfileResponse->Profile->VideoEncoderConfiguration->Resolution->Width = stProfile.nWidth;
    trt__GetProfileResponse->Profile->VideoEncoderConfiguration->Resolution->Height = stProfile.nHeight;
    trt__GetProfileResponse->Profile->VideoEncoderConfiguration->Quality = stProfile.Quality;
    //<VideoEncoderConfiguration><RateControl>
    trt__GetProfileResponse->Profile->VideoEncoderConfiguration->RateControl = (struct tt__VideoRateControl *)soap_malloc(soap, sizeof(struct tt__VideoRateControl));
    memset(trt__GetProfileResponse->Profile->VideoEncoderConfiguration->RateControl, '\0', sizeof(struct tt__VideoRateControl));
    trt__GetProfileResponse->Profile->VideoEncoderConfiguration->RateControl->FrameRateLimit = stProfile.FrameRateLimit;
    trt__GetProfileResponse->Profile->VideoEncoderConfiguration->RateControl->EncodingInterval = 1;
    trt__GetProfileResponse->Profile->VideoEncoderConfiguration->RateControl->BitrateLimit = stProfile.BitrateLimit;
    //<VideoEncoderConfiguration><H264>
    trt__GetProfileResponse->Profile->VideoEncoderConfiguration->H264 = (struct tt__H264Configuration *)soap_malloc(soap, sizeof(struct tt__H264Configuration));
    memset(trt__GetProfileResponse->Profile->VideoEncoderConfiguration->H264, '\0', sizeof(struct tt__H264Configuration));
    trt__GetProfileResponse->Profile->VideoEncoderConfiguration->H264->GovLength = stProfile.IFrameInterval;
    trt__GetProfileResponse->Profile->VideoEncoderConfiguration->H264->H264Profile = tt__H264Profile__High;

    //<AudioSourceConfiguration><AudioSourceConfiguration>
    trt__GetProfileResponse->Profile->AudioSourceConfiguration = (struct tt__AudioSourceConfiguration *)soap_malloc(soap, sizeof(struct tt__AudioSourceConfiguration)) ;
    if(!trt__GetProfileResponse->Profile->AudioSourceConfiguration) 
    {
        dlog_error("Failed to allocate");
        return soap_receiver_fault(soap, "Memory allocation failed", NULL);
    }
    memset(trt__GetProfileResponse->Profile->AudioSourceConfiguration, '\0', sizeof(struct tt__AudioSourceConfiguration));
    trt__GetProfileResponse->Profile->AudioSourceConfiguration->Name = soap_strdup(soap, stProfile.AudioSourceConfiguration_Name);
    trt__GetProfileResponse->Profile->AudioSourceConfiguration->UseCount = 1;
    trt__GetProfileResponse->Profile->AudioSourceConfiguration->token = soap_strdup(soap, stProfile.AudioSourceConfiguration_token);
    trt__GetProfileResponse->Profile->AudioSourceConfiguration->SourceToken = soap_strdup(soap, stProfile.AudioSourceConfiguration_SourceToken);

    //<AudioSourceConfiguration><AudioEncoderConfiguration>
    trt__GetProfileResponse->Profile->AudioEncoderConfiguration = (struct tt__AudioEncoderConfiguration *)soap_malloc(soap, sizeof(struct tt__AudioEncoderConfiguration)) ;
    if(!trt__GetProfileResponse->Profile->AudioEncoderConfiguration) 
    {
        dlog_error("Failed to allocate");
        return soap_receiver_fault(soap, "Memory allocation failed", NULL);
    }
    memset(trt__GetProfileResponse->Profile->AudioEncoderConfiguration, '\0', sizeof(struct tt__AudioEncoderConfiguration));
    trt__GetProfileResponse->Profile->AudioEncoderConfiguration->Name = soap_strdup(soap, stProfile.AudioEncoderConfiguration_Name);
    trt__GetProfileResponse->Profile->AudioEncoderConfiguration->token = soap_strdup(soap, stProfile.AudioEncoderConfiguration_token);
    trt__GetProfileResponse->Profile->AudioEncoderConfiguration->UseCount = 1;
    trt__GetProfileResponse->Profile->AudioEncoderConfiguration->Encoding = stProfile.stAudioParam.audioFormat;
    trt__GetProfileResponse->Profile->AudioEncoderConfiguration->Bitrate = stProfile.stAudioParam.audioBitrate;
    trt__GetProfileResponse->Profile->AudioEncoderConfiguration->SampleRate = stProfile.stAudioParam.audioSampleRate;

    trt__GetProfileResponse->Profile->PTZConfiguration = (struct tt__PTZConfiguration *)soap_malloc(soap, sizeof(struct tt__PTZConfiguration));
    memset(trt__GetProfileResponse->Profile->PTZConfiguration, 0, sizeof(struct tt__PTZConfiguration));
    trt__GetProfileResponse->Profile->PTZConfiguration->Name = (char *)soap_malloc(soap, sizeof(char) * 16);
    memset(trt__GetProfileResponse->Profile->PTZConfiguration->Name, '\0', sizeof(char) * 16);
    strcpy(trt__GetProfileResponse->Profile->PTZConfiguration->Name, "PTZConfig");

    //DefaultPTZSpeed
    trt__GetProfileResponse->Profile->PTZConfiguration->DefaultPTZSpeed = (struct tt__PTZSpeed*)soap_malloc(soap, sizeof(struct tt__PTZSpeed));
    memset(trt__GetProfileResponse->Profile->PTZConfiguration->DefaultPTZSpeed, 0, sizeof(struct tt__PTZSpeed));
    trt__GetProfileResponse->Profile->PTZConfiguration->DefaultPTZSpeed->PanTilt = (struct tt__Vector2D*)soap_malloc(soap, sizeof(struct tt__Vector2D));
    memset(trt__GetProfileResponse->Profile->PTZConfiguration->DefaultPTZSpeed->PanTilt, 0, sizeof(struct tt__Vector2D));
    trt__GetProfileResponse->Profile->PTZConfiguration->DefaultPTZSpeed->PanTilt->x = 0.0;
    trt__GetProfileResponse->Profile->PTZConfiguration->DefaultPTZSpeed->PanTilt->y = 0.0;
    trt__GetProfileResponse->Profile->PTZConfiguration->DefaultPTZSpeed->Zoom = (struct tt__Vector1D*)soap_malloc(soap, sizeof(struct tt__Vector1D));
    memset(trt__GetProfileResponse->Profile->PTZConfiguration->DefaultPTZSpeed->Zoom, 0, sizeof(struct tt__Vector1D));
    trt__GetProfileResponse->Profile->PTZConfiguration->DefaultPTZSpeed->Zoom->x = 0.5;

    trt__GetProfileResponse->Profile->PTZConfiguration->DefaultPTZTimeout = (LONG64 *)soap_malloc(soap, sizeof(LONG64));
    *(trt__GetProfileResponse->Profile->PTZConfiguration->DefaultPTZTimeout) = ONVIF_TIME_OUT;

    //DefaultPTZSpeed->PanTiltLimits
    trt__GetProfileResponse->Profile->PTZConfiguration->PanTiltLimits = (struct tt__PanTiltLimits*)soap_malloc(soap, sizeof(struct tt__PanTiltLimits));
    memset(trt__GetProfileResponse->Profile->PTZConfiguration->PanTiltLimits, 0, sizeof(struct tt__PanTiltLimits));
    trt__GetProfileResponse->Profile->PTZConfiguration->PanTiltLimits->Range = (struct tt__Space2DDescription*)soap_malloc(soap, sizeof(struct tt__Space2DDescription));
    memset(trt__GetProfileResponse->Profile->PTZConfiguration->PanTiltLimits->Range, 0, sizeof(struct tt__Space2DDescription));
    trt__GetProfileResponse->Profile->PTZConfiguration->PanTiltLimits->Range->XRange = (struct tt__FloatRange*)soap_malloc(soap, sizeof(struct tt__FloatRange));
    trt__GetProfileResponse->Profile->PTZConfiguration->PanTiltLimits->Range->XRange->Min = 0;
    trt__GetProfileResponse->Profile->PTZConfiguration->PanTiltLimits->Range->XRange->Max = 1;
    trt__GetProfileResponse->Profile->PTZConfiguration->PanTiltLimits->Range->YRange = (struct tt__FloatRange*)soap_malloc(soap, sizeof(struct tt__FloatRange));
    trt__GetProfileResponse->Profile->PTZConfiguration->PanTiltLimits->Range->YRange->Min = 0;
    trt__GetProfileResponse->Profile->PTZConfiguration->PanTiltLimits->Range->YRange->Max = 1;

    //DefaultPTZSpeed->ZoomLimits
    trt__GetProfileResponse->Profile->PTZConfiguration->ZoomLimits = (struct tt__ZoomLimits*)soap_malloc(soap, sizeof(struct tt__ZoomLimits));
    memset(trt__GetProfileResponse->Profile->PTZConfiguration->ZoomLimits, 0, sizeof(struct tt__ZoomLimits));
    trt__GetProfileResponse->Profile->PTZConfiguration->ZoomLimits->Range = (struct tt__Space1DDescription*)soap_malloc(soap, sizeof(struct tt__Space1DDescription));
    memset(trt__GetProfileResponse->Profile->PTZConfiguration->ZoomLimits->Range, 0, sizeof(struct tt__Space1DDescription));
    trt__GetProfileResponse->Profile->PTZConfiguration->ZoomLimits->Range->XRange = (struct tt__FloatRange*)soap_malloc(soap, sizeof(struct tt__FloatRange));
    trt__GetProfileResponse->Profile->PTZConfiguration->ZoomLimits->Range->XRange->Min = 0;
    trt__GetProfileResponse->Profile->PTZConfiguration->ZoomLimits->Range->XRange->Max = 1;

    return SOAP_OK;
}

/** Web service operation '__trt__GetProfiles' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetProfiles(struct soap* soap, struct _trt__GetProfiles *trt__GetProfiles, struct _trt__GetProfilesResponse *trt__GetProfilesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetProfiles----------");
#endif
    if (!soap || !trt__GetProfilesResponse) 
    {
        return SOAP_FAULT;
    }

    int nRet = onvif_authentication(soap);
    if (SOAP_OK != nRet) 
    {
        return nRet;
    }

    trt__GetProfilesResponse->__sizeProfiles = 2;
    trt__GetProfilesResponse->Profiles = soap_new_tt__Profile(soap, 2);
   
    OnvifProfile_t stProfile;
    for(int i = 0; i <  trt__GetProfilesResponse->__sizeProfiles; i++)
    {
        if(trt__GetProfilesResponse->Profiles + i == NULL)
        {
            continue;
        }

        memset(&stProfile, 0, sizeof(stProfile));
        get_profile_param(i, &stProfile);
        //<profiles><name>和<profiles><token>
        trt__GetProfilesResponse->Profiles[i].Name = soap_strdup(soap, stProfile.Name);
        trt__GetProfilesResponse->Profiles[i].token = soap_strdup(soap, stProfile.token);
        trt__GetProfilesResponse->Profiles[i].fixed = soap_new_xsd__boolean(soap, -1);
        *(trt__GetProfilesResponse->Profiles[i].fixed) = xsd__boolean__true_;
        
        //<VideoSourceConfiguration><name>和<VideoSourceConfiguration><token>
        trt__GetProfilesResponse->Profiles[i].VideoSourceConfiguration = soap_new_tt__VideoSourceConfiguration(soap, -1);
        trt__GetProfilesResponse->Profiles[i].VideoSourceConfiguration->Name = soap_strdup(soap, stProfile.VideoSourceConfiguration_Name);
        trt__GetProfilesResponse->Profiles[i].VideoSourceConfiguration->token = soap_strdup(soap, stProfile.VideoSourceConfiguration_token);
        trt__GetProfilesResponse->Profiles[i].VideoSourceConfiguration->SourceToken = soap_strdup(soap, stProfile.VideoSourceConfiguration_SourceToken);
        trt__GetProfilesResponse->Profiles[i].VideoSourceConfiguration->UseCount = 2;
        //<VideoSourceConfiguration><Bounds>
        trt__GetProfilesResponse->Profiles[i].VideoSourceConfiguration->Bounds = soap_new_tt__IntRectangle(soap, -1);
        trt__GetProfilesResponse->Profiles[i].VideoSourceConfiguration->Bounds->x = 0;
        trt__GetProfilesResponse->Profiles[i].VideoSourceConfiguration->Bounds->y = 0;
        trt__GetProfilesResponse->Profiles[i].VideoSourceConfiguration->Bounds->width = stProfile.nWidth;
        trt__GetProfilesResponse->Profiles[i].VideoSourceConfiguration->Bounds->height = stProfile.nHeight;

        //<VideoEncoderConfiguration>
        trt__GetProfilesResponse->Profiles[i].VideoEncoderConfiguration = soap_new_tt__VideoEncoderConfiguration(soap, -1);
        trt__GetProfilesResponse->Profiles[i].VideoEncoderConfiguration->Name = soap_strdup(soap, stProfile.VideoEncoderConfiguration_Name);
        trt__GetProfilesResponse->Profiles[i].VideoEncoderConfiguration->token = soap_strdup(soap, stProfile.VideoEncoderConfiguration_token);
        trt__GetProfilesResponse->Profiles[i].VideoEncoderConfiguration->UseCount = 1;
        trt__GetProfilesResponse->Profiles[i].VideoEncoderConfiguration->Encoding = tt__VideoEncoding__H264;
        //<VideoEncoderConfiguration><Resolution>、<RateControl>
        trt__GetProfilesResponse->Profiles[i].VideoEncoderConfiguration->Resolution = soap_new_tt__VideoResolution(soap, -1);
        trt__GetProfilesResponse->Profiles[i].VideoEncoderConfiguration->Resolution->Width = stProfile.nWidth;
        trt__GetProfilesResponse->Profiles[i].VideoEncoderConfiguration->Resolution->Height = stProfile.nHeight;
        trt__GetProfilesResponse->Profiles[i].VideoEncoderConfiguration->Quality = stProfile.Quality;
        //<VideoEncoderConfiguration><RateControl>
        trt__GetProfilesResponse->Profiles[i].VideoEncoderConfiguration->RateControl = soap_new_tt__VideoRateControl(soap, -1);
        trt__GetProfilesResponse->Profiles[i].VideoEncoderConfiguration->RateControl->FrameRateLimit = stProfile.FrameRateLimit;
        trt__GetProfilesResponse->Profiles[i].VideoEncoderConfiguration->RateControl->EncodingInterval = 1;
        trt__GetProfilesResponse->Profiles[i].VideoEncoderConfiguration->RateControl->BitrateLimit = stProfile.BitrateLimit;
        //<VideoEncoderConfiguration><H264>
        trt__GetProfilesResponse->Profiles[i].VideoEncoderConfiguration->H264 = soap_new_tt__H264Configuration(soap, -1);
        trt__GetProfilesResponse->Profiles[i].VideoEncoderConfiguration->H264->GovLength = stProfile.IFrameInterval;
        trt__GetProfilesResponse->Profiles[i].VideoEncoderConfiguration->H264->H264Profile = tt__H264Profile__High;

        trt__GetProfilesResponse->Profiles[i].VideoEncoderConfiguration->Multicast = soap_new_tt__MulticastConfiguration(soap, -1);
         trt__GetProfilesResponse->Profiles[i].VideoEncoderConfiguration->Multicast->Address = soap_new_tt__IPAddress(soap, -1);
        trt__GetProfilesResponse->Profiles[i].VideoEncoderConfiguration->Multicast->Address->Type = tt__IPType__IPv4;
        trt__GetProfilesResponse->Profiles[i].VideoEncoderConfiguration->Multicast->Address->IPv4Address = soap_strdup(soap, "224.1.0.0");
        trt__GetProfilesResponse->Profiles[i].VideoEncoderConfiguration->Multicast->Port = 40000;
        trt__GetProfilesResponse->Profiles[i].VideoEncoderConfiguration->Multicast->TTL = 64;
        trt__GetProfilesResponse->Profiles[i].VideoEncoderConfiguration->Multicast->AutoStart = xsd__boolean__true_;

        // <AudioSourceConfiguration>
        trt__GetProfilesResponse->Profiles[i].AudioSourceConfiguration = soap_new_tt__AudioSourceConfiguration(soap, -1);
        if(!trt__GetProfilesResponse->Profiles[i].AudioSourceConfiguration)
        {
            dlog_error("Failed to allocate");
            return soap_receiver_fault(soap, "Memory allocation failed", NULL);
        }
        trt__GetProfilesResponse->Profiles[i].AudioSourceConfiguration->Name = soap_strdup(soap, stProfile.AudioSourceConfiguration_Name);
        trt__GetProfilesResponse->Profiles[i].AudioSourceConfiguration->UseCount = 2;
        trt__GetProfilesResponse->Profiles[i].AudioSourceConfiguration->token = soap_strdup(soap, stProfile.AudioSourceConfiguration_token);
        trt__GetProfilesResponse->Profiles[i].AudioSourceConfiguration->SourceToken = soap_strdup(soap, stProfile.AudioSourceConfiguration_SourceToken);

        // <AudioEncoderConfiguration>
         trt__GetProfilesResponse->Profiles[i].AudioEncoderConfiguration = soap_new_tt__AudioEncoderConfiguration(soap, -1);
        if(!trt__GetProfilesResponse->Profiles[i].AudioEncoderConfiguration)
        {
            dlog_error("Failed to allocate");
            return soap_receiver_fault(soap, "Memory allocation failed", NULL);
        }
        trt__GetProfilesResponse->Profiles[i].AudioEncoderConfiguration->Name = soap_strdup(soap, stProfile.AudioEncoderConfiguration_Name);
        trt__GetProfilesResponse->Profiles[i].AudioEncoderConfiguration->token = soap_strdup(soap, stProfile.AudioEncoderConfiguration_token);
        trt__GetProfilesResponse->Profiles[i].AudioEncoderConfiguration->UseCount = 1;
        trt__GetProfilesResponse->Profiles[i].AudioEncoderConfiguration->Encoding = stProfile.stAudioParam.audioFormat;
        trt__GetProfilesResponse->Profiles[i].AudioEncoderConfiguration->Bitrate = stProfile.stAudioParam.audioBitrate;
        trt__GetProfilesResponse->Profiles[i].AudioEncoderConfiguration->SampleRate = stProfile.stAudioParam.audioSampleRate;

        // <VideoAnalyticsConfiguration>
        trt__GetProfilesResponse->Profiles[i].VideoAnalyticsConfiguration = soap_new_tt__VideoAnalyticsConfiguration(soap, -1);
        trt__GetProfilesResponse->Profiles[i].VideoAnalyticsConfiguration->UseCount = 2;
        trt__GetProfilesResponse->Profiles[i].VideoAnalyticsConfiguration->__size = 1;
        trt__GetProfilesResponse->Profiles[i].VideoAnalyticsConfiguration->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN_NAME);
        trt__GetProfilesResponse->Profiles[i].VideoAnalyticsConfiguration->token = soap_strdup(soap, VIDEOANALTICS_TOKEN);

        /* 分析模块初始化 */
        trt__GetProfilesResponse->Profiles[i].VideoAnalyticsConfiguration->AnalyticsEngineConfiguration = soap_new_tt__AnalyticsEngineConfiguration(soap,-1);
        trt__GetProfilesResponse->Profiles[i].VideoAnalyticsConfiguration->AnalyticsEngineConfiguration->__sizeAnalyticsModule = ONVIF_ANALYTICS_SUPPORT_NUM;
        trt__GetProfilesResponse->Profiles[i].VideoAnalyticsConfiguration->AnalyticsEngineConfiguration->AnalyticsModule = soap_new_tt__Config(soap,ONVIF_ANALYTICS_SUPPORT_NUM);
        
        /*  规则初始化 */
        trt__GetProfilesResponse->Profiles[i].VideoAnalyticsConfiguration->RuleEngineConfiguration = soap_new_tt__RuleEngineConfiguration(soap,-1);
        trt__GetProfilesResponse->Profiles[i].VideoAnalyticsConfiguration->RuleEngineConfiguration->__sizeRule = ONVIF_ANALYTICS_RULE_SUPPORT_NUM;
        trt__GetProfilesResponse->Profiles[i].VideoAnalyticsConfiguration->RuleEngineConfiguration->Rule = soap_new_tt__Config(soap,ONVIF_ANALYTICS_RULE_SUPPORT_NUM);

        /* 分析模块和规则获取 */
        for(int k  = 0; k < ONVIF_ANALYTICS_SUPPORT_NUM;k++)
        {
            struct tt__Config *pAnalyticsModule = trt__GetProfilesResponse->Profiles[i].VideoAnalyticsConfiguration->AnalyticsEngineConfiguration->AnalyticsModule + k;
            if(pAnalyticsModule == NULL)
            {
                dlog_error("NULL");
                continue;
            }

            struct tt__Config *pRule;
            if( i < ONVIF_ANALYTICS_RULE_SUPPORT_NUM)
            {
                pRule = trt__GetProfilesResponse->Profiles[i].VideoAnalyticsConfiguration->RuleEngineConfiguration->Rule + i;
            }
            if(pAnalyticsModule == NULL || pRule == NULL)
            {
                break;
            }

            switch (k)
            {
            case MOTION_DETECTION_ALARM:
            {
                /* 获取当前移动侦测配置信息 */
                OnvifMotionDetection_S stInfo;
                onvif_get_motion_info(&stInfo);
                dlog_debug("获取到移动侦测灵敏度【%s】 网格编码信息【%s】",stInfo.achSensitivity,stInfo.achBaseStr);
                pAnalyticsModule->Name = soap_strdup(soap, MOTION_EVENT_MODULE);
                pAnalyticsModule->Type = soap_strdup(soap, MOTION_EVENT_MODULE_TYPE);
                pAnalyticsModule->Parameters = soap_new_tt__ItemList(soap,-1);
                pAnalyticsModule->Parameters->__sizeSimpleItem = 1;
                /* 移动侦测灵敏度 */
                pAnalyticsModule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap,-1);
                pAnalyticsModule->Parameters->SimpleItem->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
                pAnalyticsModule->Parameters->SimpleItem->Value = soap_strdup(soap, stInfo.achSensitivity);
                /* 移动侦测布局信息定义 */
                pAnalyticsModule->Parameters->__sizeElementItem = 1;
                pAnalyticsModule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap,-1);
                pAnalyticsModule->Parameters->ElementItem->Name = soap_strdup(soap, ONVIF_ANALYTICS_LAYOUT_NAME);
                pAnalyticsModule->Parameters->ElementItem->__any = soap_strdup(soap, ONVIF_CELL_MOTION_LAYOUT_MACRO);

                /* 移动侦测规则获取 */
                pRule->Name = soap_strdup(soap, MOTION_EVENT_RULE);
                pRule->Type = soap_strdup(soap, MOTION_EVENT_RULE_TYPE);
                pRule->Parameters = soap_new_tt__ItemList(soap,-1);
                pRule->Parameters->__sizeSimpleItem = ONVIF_CELLMOTION_RULEPARAM_NUM;
                pRule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap,ONVIF_CELLMOTION_RULEPARAM_NUM);
                struct _tt__ItemList_SimpleItem *pRuleParam_0 = pRule->Parameters->SimpleItem + 0;
                struct _tt__ItemList_SimpleItem *pRuleParam_1 = pRule->Parameters->SimpleItem + 1;
                struct _tt__ItemList_SimpleItem *pRuleParam_2 = pRule->Parameters->SimpleItem + 2;
                struct _tt__ItemList_SimpleItem *pRuleParam_3 = pRule->Parameters->SimpleItem + 3;

                if(pRuleParam_0 != NULL)
                {
                    pRuleParam_0->Name = soap_strdup(soap, ONVIF_CELLMOTION_RULEPARAM_1);
                    pRuleParam_0->Value = soap_strdup(soap, ONVIF_CELLMOTION_RULEPARAM_VALUE1);
                }
                if(pRuleParam_1 != NULL)
                {
                    pRuleParam_1->Name = soap_strdup(soap, ONVIF_CELLMOTION_RULEPARAM_2);
                    pRuleParam_1->Value = soap_strdup(soap, ONVIF_CELLMOTION_RULEPARAM_VALUE2);
                }

                if(pRuleParam_2 != NULL)
                {
                    pRuleParam_2->Name = soap_strdup(soap, ONVIF_CELLMOTION_RULEPARAM_3);
                    pRuleParam_2->Value = soap_strdup(soap, ONVIF_CELLMOTION_RULEPARAM_VALUE3);
                }
                /* 移动侦测网格数据 */
                if(pRuleParam_3 != NULL)
                {
                    pRuleParam_3->Name = soap_strdup(soap, ONVIF_CELLMOTION_RULEPARAM_4);
                    pRuleParam_3->Value = soap_strdup(soap, stInfo.achBaseStr);
                }
                break;
            }
            
            case IMAGE_OBSTRUTION_ALARM:
            {
                /* 获取当前遮挡报警配置信息 */
                ONvifTamperDetection_S stInfo;
                onvif_get_tamp_info(&stInfo);
                pAnalyticsModule->Name = soap_strdup(soap, TAMPEREVENT_MODULE);
                pAnalyticsModule->Type = soap_strdup(soap, TAMPEREVENT_MODULE_TYPE);
                pAnalyticsModule->Parameters = soap_new_tt__ItemList(soap,-1);
                pAnalyticsModule->Parameters->__sizeSimpleItem = 1;
                /* 遮挡报警灵敏度 */
                pAnalyticsModule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap,-1);
                pAnalyticsModule->Parameters->SimpleItem->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
                pAnalyticsModule->Parameters->SimpleItem->Value = soap_strdup(soap, stInfo.achSensitivity);
                
                pAnalyticsModule->Parameters->__sizeElementItem = 2;
                pAnalyticsModule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap,2);
                struct _tt__ItemList_ElementItem *pAnalyticsModulParam_0 = pAnalyticsModule->Parameters->ElementItem + 0;
                struct _tt__ItemList_ElementItem *pAnalyticsModulParam_1 = pAnalyticsModule->Parameters->ElementItem + 1;

                /* 遮挡报警缩放信息 */
                if(pAnalyticsModulParam_0 != NULL)
                {
                    pAnalyticsModulParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
                    pAnalyticsModulParam_0->__any = soap_strdup(soap, ONVIF_TAMPER_TRANSFORMATION_MACRO);
                }
                /* 遮挡报警坐标布局定义 */
                if(pAnalyticsModulParam_1 != NULL)
                {
                    pAnalyticsModulParam_1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                    pAnalyticsModulParam_1->__any = soap_strdup(soap, ONVIF_TAMPER_POLYGON_CONFIG_MACRO);
                }
                /* 遮挡报警规则获取 */
                pRule->Name = soap_strdup(soap, TAMPEREVENT_RULE);
                pRule->Type = soap_strdup(soap, TAMPEREVENT_RULE_TYPE);
                pRule->Parameters = soap_new_tt__ItemList(soap,-1);
                pRule->Parameters->__sizeElementItem = ONVIF_TAMPER_RULEPARAM_NUM;
                pRule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap,-1);
                struct _tt__ItemList_ElementItem *pRuleParam_0 = pRule->Parameters->ElementItem + 0;
                if(pRuleParam_0 != NULL)
                {
                    char achPolygon[1024];
                    pRuleParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                    /* 遮挡区域 */
                    snprintf(achPolygon, 1024,
                    "<tt:PolygonConfiguration><tt:Polygon>"
                    "<tt:Point x=\"%d\" y=\"%d\"/>"
                    "<tt:Point x=\"%d\" y=\"%d\"/>"
                    "<tt:Point x=\"%d\" y=\"%d\"/>"
                    "<tt:Point x=\"%d\" y=\"%d\"/>"
                    "</tt:Polygon></tt:PolygonConfiguration>",
                    stInfo.stPolygon[0].x, stInfo.stPolygon[0].y,
                    stInfo.stPolygon[1].x, stInfo.stPolygon[1].y,
                    stInfo.stPolygon[2].x, stInfo.stPolygon[2].y,
                    stInfo.stPolygon[3].x, stInfo.stPolygon[3].y);
                    pRuleParam_0->__any = soap_strdup(soap, achPolygon);
                
                }

                break;
            }
            
            case INTRUSION_ALARM:
            {
                pAnalyticsModule->Name = soap_strdup(soap, FIELD_EVENT_MODULE);
                pAnalyticsModule->Type = soap_strdup(soap, FIELD_EVENT_MODULE_TYPE);
                pAnalyticsModule->Parameters = soap_new_tt__ItemList(soap,-1);
                pAnalyticsModule->Parameters->__sizeSimpleItem = 1;
                /* 区域入侵灵敏度 */
                pAnalyticsModule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap,-1);
                pAnalyticsModule->Parameters->SimpleItem->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
                pAnalyticsModule->Parameters->SimpleItem->Value = soap_strdup(soap, "50");
                
                pAnalyticsModule->Parameters->__sizeElementItem = 2;
                pAnalyticsModule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap,2);
                struct _tt__ItemList_ElementItem *pAnalyticsModulParam_0 = pAnalyticsModule->Parameters->ElementItem + 0;
                struct _tt__ItemList_ElementItem *pAnalyticsModulParam_1 = pAnalyticsModule->Parameters->ElementItem + 1;

                /* 区域入侵缩放信息 */
                if(pAnalyticsModulParam_0 != NULL)
                {
                    pAnalyticsModulParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
                    pAnalyticsModulParam_0->__any = soap_strdup(soap, ONVIF_FIELD_TRANSFORMATION_MACRO);
                }
                /*区域入侵坐标布局定义 */
                if(pAnalyticsModulParam_1 != NULL)
                {
                    pAnalyticsModulParam_1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                    pAnalyticsModulParam_1->__any = soap_strdup(soap, ONVIF_FIELD_POLYGON_CONFIG_MACRO);
                }
                break;
            }   
                
            case TRIPWIRE_ALARM:
            {
                pAnalyticsModule->Name = soap_strdup(soap, LINE_EVENT_MODULE);
                pAnalyticsModule->Type = soap_strdup(soap, LINE_EVENT_MODULE_TYPE);
                pAnalyticsModule->Parameters = soap_new_tt__ItemList(soap,-1);
                pAnalyticsModule->Parameters->__sizeSimpleItem = 1;
                /* 拌线入侵灵敏度 */
                pAnalyticsModule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap,-1);
                pAnalyticsModule->Parameters->SimpleItem->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
                pAnalyticsModule->Parameters->SimpleItem->Value = soap_strdup(soap, "50");
                
                pAnalyticsModule->Parameters->__sizeElementItem = 2;
                pAnalyticsModule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap,2);
                struct _tt__ItemList_ElementItem *pAnalyticsModulParam_0 = pAnalyticsModule->Parameters->ElementItem + 0;
                struct _tt__ItemList_ElementItem *pAnalyticsModulParam_1 = pAnalyticsModule->Parameters->ElementItem + 1;

                /* 拌线入侵缩放信息 */
                if(pAnalyticsModulParam_0 != NULL)
                {
                    pAnalyticsModulParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
                    pAnalyticsModulParam_0->__any = soap_strdup(soap, ONVIF_FIELD_TRANSFORMATION_MACRO);
                }
                /* 拌线入侵坐标布局定义 */
                if(pAnalyticsModulParam_1 != NULL)
                {
                    pAnalyticsModulParam_1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                    pAnalyticsModulParam_1->__any = soap_strdup(soap, ONVIF_FIELD_POLYGON_CONFIG_MACRO);
                }
                break;
            } 
                
            
            default:
                break;
            }

        }
        
    }
    return SOAP_OK;
}

/** Web service operation '__trt__AddVideoEncoderConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__AddVideoEncoderConfiguration(struct soap* soap, struct _trt__AddVideoEncoderConfiguration *trt__AddVideoEncoderConfiguration, struct _trt__AddVideoEncoderConfigurationResponse *trt__AddVideoEncoderConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__AddVideoEncoderConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__AddVideoSourceConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__AddVideoSourceConfiguration(struct soap* soap, struct _trt__AddVideoSourceConfiguration *trt__AddVideoSourceConfiguration, struct _trt__AddVideoSourceConfigurationResponse *trt__AddVideoSourceConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__AddVideoSourceConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__AddAudioEncoderConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__AddAudioEncoderConfiguration(struct soap* soap, struct _trt__AddAudioEncoderConfiguration *trt__AddAudioEncoderConfiguration, struct _trt__AddAudioEncoderConfigurationResponse *trt__AddAudioEncoderConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__AddAudioEncoderConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__AddAudioSourceConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__AddAudioSourceConfiguration(struct soap* soap, struct _trt__AddAudioSourceConfiguration *trt__AddAudioSourceConfiguration, struct _trt__AddAudioSourceConfigurationResponse *trt__AddAudioSourceConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__AddAudioSourceConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__AddPTZConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__AddPTZConfiguration(struct soap* soap, struct _trt__AddPTZConfiguration *trt__AddPTZConfiguration, struct _trt__AddPTZConfigurationResponse *trt__AddPTZConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__AddPTZConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__AddVideoAnalyticsConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__AddVideoAnalyticsConfiguration(struct soap* soap, struct _trt__AddVideoAnalyticsConfiguration *trt__AddVideoAnalyticsConfiguration, struct _trt__AddVideoAnalyticsConfigurationResponse *trt__AddVideoAnalyticsConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__AddVideoAnalyticsConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__AddMetadataConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__AddMetadataConfiguration(struct soap* soap, struct _trt__AddMetadataConfiguration *trt__AddMetadataConfiguration, struct _trt__AddMetadataConfigurationResponse *trt__AddMetadataConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__AddMetadataConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__AddAudioOutputConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__AddAudioOutputConfiguration(struct soap* soap, struct _trt__AddAudioOutputConfiguration *trt__AddAudioOutputConfiguration, struct _trt__AddAudioOutputConfigurationResponse *trt__AddAudioOutputConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__AddAudioOutputConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__AddAudioDecoderConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__AddAudioDecoderConfiguration(struct soap* soap, struct _trt__AddAudioDecoderConfiguration *trt__AddAudioDecoderConfiguration, struct _trt__AddAudioDecoderConfigurationResponse *trt__AddAudioDecoderConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__AddAudioDecoderConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__RemoveVideoEncoderConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__RemoveVideoEncoderConfiguration(struct soap* soap, struct _trt__RemoveVideoEncoderConfiguration *trt__RemoveVideoEncoderConfiguration, struct _trt__RemoveVideoEncoderConfigurationResponse *trt__RemoveVideoEncoderConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__RemoveVideoEncoderConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__RemoveVideoSourceConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__RemoveVideoSourceConfiguration(struct soap* soap, struct _trt__RemoveVideoSourceConfiguration *trt__RemoveVideoSourceConfiguration, struct _trt__RemoveVideoSourceConfigurationResponse *trt__RemoveVideoSourceConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__RemoveVideoSourceConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__RemoveAudioEncoderConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__RemoveAudioEncoderConfiguration(struct soap* soap, struct _trt__RemoveAudioEncoderConfiguration *trt__RemoveAudioEncoderConfiguration, struct _trt__RemoveAudioEncoderConfigurationResponse *trt__RemoveAudioEncoderConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__RemoveAudioEncoderConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__RemoveAudioSourceConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__RemoveAudioSourceConfiguration(struct soap* soap, struct _trt__RemoveAudioSourceConfiguration *trt__RemoveAudioSourceConfiguration, struct _trt__RemoveAudioSourceConfigurationResponse *trt__RemoveAudioSourceConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__RemoveAudioSourceConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__RemovePTZConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__RemovePTZConfiguration(struct soap* soap, struct _trt__RemovePTZConfiguration *trt__RemovePTZConfiguration, struct _trt__RemovePTZConfigurationResponse *trt__RemovePTZConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__RemovePTZConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__RemoveVideoAnalyticsConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__RemoveVideoAnalyticsConfiguration(struct soap* soap, struct _trt__RemoveVideoAnalyticsConfiguration *trt__RemoveVideoAnalyticsConfiguration, struct _trt__RemoveVideoAnalyticsConfigurationResponse *trt__RemoveVideoAnalyticsConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__RemoveVideoAnalyticsConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__RemoveMetadataConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__RemoveMetadataConfiguration(struct soap* soap, struct _trt__RemoveMetadataConfiguration *trt__RemoveMetadataConfiguration, struct _trt__RemoveMetadataConfigurationResponse *trt__RemoveMetadataConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__RemoveMetadataConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__RemoveAudioOutputConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__RemoveAudioOutputConfiguration(struct soap* soap, struct _trt__RemoveAudioOutputConfiguration *trt__RemoveAudioOutputConfiguration, struct _trt__RemoveAudioOutputConfigurationResponse *trt__RemoveAudioOutputConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__RemoveAudioOutputConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__RemoveAudioDecoderConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__RemoveAudioDecoderConfiguration(struct soap* soap, struct _trt__RemoveAudioDecoderConfiguration *trt__RemoveAudioDecoderConfiguration, struct _trt__RemoveAudioDecoderConfigurationResponse *trt__RemoveAudioDecoderConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__RemoveAudioDecoderConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__DeleteProfile' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__DeleteProfile(struct soap* soap, struct _trt__DeleteProfile *trt__DeleteProfile, struct _trt__DeleteProfileResponse *trt__DeleteProfileResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__DeleteProfile----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__GetVideoSourceConfigurations' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetVideoSourceConfigurations(struct soap* soap, struct _trt__GetVideoSourceConfigurations *trt__GetVideoSourceConfigurations, struct _trt__GetVideoSourceConfigurationsResponse *trt__GetVideoSourceConfigurationsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetVideoSourceConfigurations----------");
#endif
    int nRet = onvif_authentication(soap);
    if(SOAP_OK != nRet)
    {
        return nRet;
    }
    if( trt__GetVideoSourceConfigurationsResponse == NULL)
    {
        dlog_error("trt__GetVideoSourceConfigurationsResponse is NULL");
        return SOAP_EOF;
    }

    trt__GetVideoSourceConfigurationsResponse->__sizeConfigurations = 1;
    trt__GetVideoSourceConfigurationsResponse->Configurations = soap_new_tt__VideoSourceConfiguration(soap,-1);

    trt__GetVideoSourceConfigurationsResponse->Configurations->UseCount = ONVIF_MEDIA_PROFILE_NUM;

    trt__GetVideoSourceConfigurationsResponse->Configurations->Name = soap_strdup(soap, VIDEOSOURCE_NAME);
   
    trt__GetVideoSourceConfigurationsResponse->Configurations->token = soap_strdup(soap, VIDEOSOURCE_TOKEN);
    trt__GetVideoSourceConfigurationsResponse->Configurations->SourceToken = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
   
    trt__GetVideoSourceConfigurationsResponse->Configurations->Bounds = soap_new_tt__IntRectangle(soap,-1);
    trt__GetVideoSourceConfigurationsResponse->Configurations->Bounds->x      = 0;
    trt__GetVideoSourceConfigurationsResponse->Configurations->Bounds->y      = 0;
    trt__GetVideoSourceConfigurationsResponse->Configurations->Bounds->width  = ONVIF_BOUNDS_WIDTH;
    trt__GetVideoSourceConfigurationsResponse->Configurations->Bounds->height = ONVIF_BOUNDS_HEIGHT;

    return SOAP_OK;
}

/** Web service operation '__trt__GetVideoEncoderConfigurations' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetVideoEncoderConfigurations(struct soap* soap, struct _trt__GetVideoEncoderConfigurations *trt__GetVideoEncoderConfigurations, struct _trt__GetVideoEncoderConfigurationsResponse *trt__GetVideoEncoderConfigurationsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetVideoEncoderConfigurations----------");
#endif
    int size = 2;
    trt__GetVideoEncoderConfigurationsResponse->__sizeConfigurations = size;
    trt__GetVideoEncoderConfigurationsResponse->Configurations =
            (struct tt__VideoEncoderConfiguration *)soap_malloc(soap, sizeof(struct tt__VideoEncoderConfiguration) * size);
    memset(trt__GetVideoEncoderConfigurationsResponse->Configurations, '\0', sizeof(struct tt__VideoEncoderConfiguration) * size);

    OnvifProfile_t stProfile;
    
    for(int i = 0; i < trt__GetVideoEncoderConfigurationsResponse->__sizeConfigurations; i++)
    {
        memset(&stProfile, 0, sizeof(stProfile));
        get_profile_param(i, &stProfile);

        //<VideoEncoderConfigurations>
        trt__GetVideoEncoderConfigurationsResponse->Configurations[i].Name = (char *)soap_malloc(soap, sizeof(char) * 32);
        memset(trt__GetVideoEncoderConfigurationsResponse->Configurations[i].Name, '\0', sizeof(char) * 32);
        strcpy(trt__GetVideoEncoderConfigurationsResponse->Configurations[i].Name, stProfile.VideoEncoderConfiguration_Name);

        trt__GetVideoEncoderConfigurationsResponse->Configurations[i].token = (char *)soap_malloc(soap, sizeof(char) * 32);
        memset(trt__GetVideoEncoderConfigurationsResponse->Configurations[i].token, '\0', sizeof(char) * 32);
        strcpy(trt__GetVideoEncoderConfigurationsResponse->Configurations[i].token, stProfile.VideoEncoderConfiguration_token);

        OnvifVideoParam_t stActualParam;
        memset(&stActualParam, 0, sizeof(stActualParam));
        onvif_get_videoParams(&stActualParam, i);

        trt__GetVideoEncoderConfigurationsResponse->Configurations[i].UseCount = 1;
        trt__GetVideoEncoderConfigurationsResponse->Configurations[i].Quality = (float)stActualParam.nCurQuality;

        if (strcmp(stActualParam.strVideoCodec, VIDEO_CODEC_MJPEG) == 0) {
            trt__GetVideoEncoderConfigurationsResponse->Configurations[i].Encoding = tt__VideoEncoding__JPEG;
        } else {
            // Default to H264 for H264, H265, SVAC3 as Media1 might not support others directly in this enum
            trt__GetVideoEncoderConfigurationsResponse->Configurations[i].Encoding = tt__VideoEncoding__H264;
        }
        
        //<Configurations><Resolution>
        trt__GetVideoEncoderConfigurationsResponse->Configurations[i].Resolution = (struct tt__VideoResolution *)soap_malloc(soap,sizeof(struct tt__VideoResolution));
        memset(trt__GetVideoEncoderConfigurationsResponse->Configurations[i].Resolution, 0 , sizeof(struct tt__VideoResolution));
        trt__GetVideoEncoderConfigurationsResponse->Configurations[i].Resolution->Width  = stActualParam.nWidths[0];
        trt__GetVideoEncoderConfigurationsResponse->Configurations[i].Resolution->Height = stActualParam.nHeights[0];
        //<Configurations><RateControl>
        trt__GetVideoEncoderConfigurationsResponse->Configurations[i].RateControl = (struct tt__VideoRateControl *)soap_malloc(soap, sizeof(struct tt__VideoRateControl));
        memset(trt__GetVideoEncoderConfigurationsResponse->Configurations[i].RateControl, 0, sizeof(struct tt__VideoRateControl));
        trt__GetVideoEncoderConfigurationsResponse->Configurations[i].RateControl->FrameRateLimit = (int)stActualParam.fCurFrameRate;
        trt__GetVideoEncoderConfigurationsResponse->Configurations[i].RateControl->EncodingInterval = 1;
        trt__GetVideoEncoderConfigurationsResponse->Configurations[i].RateControl->BitrateLimit     = stActualParam.nCurBitrate;
        //<Configurations><H264>
        trt__GetVideoEncoderConfigurationsResponse->Configurations[i].H264 = (struct tt__H264Configuration *)soap_malloc(soap, sizeof(struct tt__H264Configuration));
        memset(trt__GetVideoEncoderConfigurationsResponse->Configurations[i].H264, 0, sizeof(struct tt__H264Configuration));
        trt__GetVideoEncoderConfigurationsResponse->Configurations[i].H264->GovLength  = stActualParam.nIFrameInterval;
        trt__GetVideoEncoderConfigurationsResponse->Configurations[i].H264->H264Profile = tt__H264Profile__High;

        //<Configuration><Multicast>
        trt__GetVideoEncoderConfigurationsResponse->Configurations[i].Multicast = (struct tt__MulticastConfiguration *)soap_malloc(soap, sizeof(struct tt__MulticastConfiguration));
        memset(trt__GetVideoEncoderConfigurationsResponse->Configurations[i].Multicast, 0, sizeof(struct tt__MulticastConfiguration));
        trt__GetVideoEncoderConfigurationsResponse->Configurations[i].Multicast->Address = (struct tt__IPAddress *)soap_malloc(soap, sizeof(struct tt__IPAddress));
        memset(trt__GetVideoEncoderConfigurationsResponse->Configurations[i].Multicast->Address, 0, sizeof(struct tt__IPAddress));
        trt__GetVideoEncoderConfigurationsResponse->Configurations[i].Multicast->Address->Type = tt__IPType__IPv4;
        trt__GetVideoEncoderConfigurationsResponse->Configurations[i].Multicast->Address->IPv4Address = (char *)soap_malloc(soap, sizeof(char) * 32);
        memset(trt__GetVideoEncoderConfigurationsResponse->Configurations[i].Multicast->Address->IPv4Address, '\0', sizeof(char) * 32);
        strcpy(trt__GetVideoEncoderConfigurationsResponse->Configurations[i].Multicast->Address->IPv4Address, "224.1.0.0");
        trt__GetVideoEncoderConfigurationsResponse->Configurations[i].Multicast->Port = ONVIF_TCP_PORT;
        trt__GetVideoEncoderConfigurationsResponse->Configurations[i].Multicast->TTL = 64;
        trt__GetVideoEncoderConfigurationsResponse->Configurations[i].Multicast->AutoStart = xsd__boolean__true_;
        //<Configuration><SessionTimeout>
        trt__GetVideoEncoderConfigurationsResponse->Configurations[i].SessionTimeout = ONVIF_TIME_OUT;

        dlog_debug("Media 获取视频编码配置: Token=%s, 编码=%d, 分辨率=%dx%d, 码率限制=%d, 帧率限制=%d, Gov长度=%d",
            trt__GetVideoEncoderConfigurationsResponse->Configurations[i].token,
            trt__GetVideoEncoderConfigurationsResponse->Configurations[i].Encoding,
            trt__GetVideoEncoderConfigurationsResponse->Configurations[i].Resolution->Width,
            trt__GetVideoEncoderConfigurationsResponse->Configurations[i].Resolution->Height,
            trt__GetVideoEncoderConfigurationsResponse->Configurations[i].RateControl->BitrateLimit,
            trt__GetVideoEncoderConfigurationsResponse->Configurations[i].RateControl->FrameRateLimit,
            trt__GetVideoEncoderConfigurationsResponse->Configurations[i].H264->GovLength);
    }
    return SOAP_OK;
}

/** Web service operation '__trt__GetAudioSourceConfigurations' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioSourceConfigurations(struct soap* soap, struct _trt__GetAudioSourceConfigurations *trt__GetAudioSourceConfigurations, struct _trt__GetAudioSourceConfigurationsResponse *trt__GetAudioSourceConfigurationsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetAudioSourceConfigurations----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__GetAudioEncoderConfigurations' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioEncoderConfigurations(struct soap* soap, struct _trt__GetAudioEncoderConfigurations *trt__GetAudioEncoderConfigurations, struct _trt__GetAudioEncoderConfigurationsResponse *trt__GetAudioEncoderConfigurationsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetAudioEncoderConfigurations----------");
#endif
    if(!trt__GetAudioEncoderConfigurationsResponse)
    {
        dlog_error("trt__GetAudioEncoderConfigurationsResponse is NULL");
        return soap_sender_fault(soap,  "trt__GetAudioEncoderConfigurationsResponse is NULL",  NULL);
    }

    OnvifProfile_t stProfile;
    memset(&stProfile, 0, sizeof(stProfile));

    trt__GetAudioEncoderConfigurationsResponse->__sizeConfigurations = 1;
    trt__GetAudioEncoderConfigurationsResponse->Configurations = (struct tt__AudioEncoderConfiguration *)soap_malloc(soap, sizeof(struct tt__AudioEncoderConfiguration)*trt__GetAudioEncoderConfigurationsResponse->__sizeConfigurations);
    if(!trt__GetAudioEncoderConfigurationsResponse->Configurations)
    {
        dlog_error("Failed to allocate");
        return soap_receiver_fault(soap, "Memory allocation failed", NULL);
    }
    memset(trt__GetAudioEncoderConfigurationsResponse->Configurations, '\0', sizeof(struct tt__AudioEncoderConfiguration)*trt__GetAudioEncoderConfigurationsResponse->__sizeConfigurations);

    for(int i = 0; i < trt__GetAudioEncoderConfigurationsResponse->__sizeConfigurations; i++)
    {
        memset(&stProfile, 0, sizeof(stProfile));
        get_profile_param(i, &stProfile);

        trt__GetAudioEncoderConfigurationsResponse->Configurations[i].Name = soap_strdup(soap, stProfile.AudioEncoderConfiguration_Name);
        trt__GetAudioEncoderConfigurationsResponse->Configurations[i].token = soap_strdup(soap, stProfile.AudioEncoderConfiguration_token);
        trt__GetAudioEncoderConfigurationsResponse->Configurations[i].UseCount = 1;
        trt__GetAudioEncoderConfigurationsResponse->Configurations[i].Encoding = stProfile.stAudioParam.audioFormat;
        trt__GetAudioEncoderConfigurationsResponse->Configurations[i].Bitrate = stProfile.stAudioParam.audioBitrate / 1000;        // 单位kbps
        trt__GetAudioEncoderConfigurationsResponse->Configurations[i].SampleRate = stProfile.stAudioParam.audioSampleRate / 1000;  // kHz

    }

    return SOAP_OK;
}

/** Web service operation '__trt__GetVideoAnalyticsConfigurations' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetVideoAnalyticsConfigurations(struct soap* soap, struct _trt__GetVideoAnalyticsConfigurations *trt__GetVideoAnalyticsConfigurations, struct _trt__GetVideoAnalyticsConfigurationsResponse *trt__GetVideoAnalyticsConfigurationsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetVideoAnalyticsConfigurations----------");
#endif

    int nRet = onvif_authentication(soap);
    if(SOAP_OK != nRet)
    {
        return nRet;
    }
    if(trt__GetVideoAnalyticsConfigurationsResponse == NULL)
    {
        dlog_error("trt__GetVideoAnalyticsConfigurationsResponse is NULL");
        return soap_receiver_fault(soap, "trt__GetVideoAnalyticsConfigurationsResponse is NULL", NULL);    
    }

    trt__GetVideoAnalyticsConfigurationsResponse->__sizeConfigurations = 1;
    
    trt__GetVideoAnalyticsConfigurationsResponse->Configurations = soap_new_tt__VideoAnalyticsConfiguration(soap,-1);
    trt__GetVideoAnalyticsConfigurationsResponse->Configurations->UseCount = 2;
    trt__GetVideoAnalyticsConfigurationsResponse->Configurations->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN_NAME);
    trt__GetVideoAnalyticsConfigurationsResponse->Configurations->token = soap_strdup(soap, VIDEOANALTICS_TOKEN);

    /* 分析模块初始化 */
    if(trt__GetVideoAnalyticsConfigurationsResponse->Configurations->AnalyticsEngineConfiguration == NULL)
    {
        trt__GetVideoAnalyticsConfigurationsResponse->Configurations->AnalyticsEngineConfiguration = soap_new_tt__AnalyticsEngineConfiguration(soap,-1);
    }
    trt__GetVideoAnalyticsConfigurationsResponse->Configurations->AnalyticsEngineConfiguration->__sizeAnalyticsModule = ONVIF_ANALYTICS_SUPPORT_NUM;
    trt__GetVideoAnalyticsConfigurationsResponse->Configurations->AnalyticsEngineConfiguration->AnalyticsModule = soap_new_tt__Config(soap,ONVIF_ANALYTICS_SUPPORT_NUM);
    
    /*  规则初始化 */
    if(trt__GetVideoAnalyticsConfigurationsResponse->Configurations->RuleEngineConfiguration == NULL)
    {
        trt__GetVideoAnalyticsConfigurationsResponse->Configurations->RuleEngineConfiguration = soap_new_tt__RuleEngineConfiguration(soap,-1);
    }
    trt__GetVideoAnalyticsConfigurationsResponse->Configurations->RuleEngineConfiguration->__sizeRule = ONVIF_ANALYTICS_RULE_SUPPORT_NUM;
    trt__GetVideoAnalyticsConfigurationsResponse->Configurations->RuleEngineConfiguration->Rule = soap_new_tt__Config(soap,ONVIF_ANALYTICS_RULE_SUPPORT_NUM);

    /* 分析模块和规则获取 */
    for(int i  = 0; i < ONVIF_ANALYTICS_SUPPORT_NUM;i++)
    {
        struct tt__Config *pAnalyticsModule = trt__GetVideoAnalyticsConfigurationsResponse->Configurations->AnalyticsEngineConfiguration->AnalyticsModule + i;
        
        struct tt__Config *pRule;
        if( i < ONVIF_ANALYTICS_RULE_SUPPORT_NUM)
        {
            pRule = trt__GetVideoAnalyticsConfigurationsResponse->Configurations->RuleEngineConfiguration->Rule + i;
        }
        if(pAnalyticsModule == NULL || pRule == NULL)
        {
            break;
        }

        switch (i)
        {
        case MOTION_DETECTION_ALARM:
        {
            /* 获取当前移动侦测配置信息 */
            OnvifMotionDetection_S stInfo;
            onvif_get_motion_info(&stInfo);
            dlog_debug("获取到移动侦测灵敏度【%s】 网格编码信息【%s】",stInfo.achSensitivity,stInfo.achBaseStr);
            pAnalyticsModule->Name = soap_strdup(soap, MOTION_EVENT_MODULE);
            pAnalyticsModule->Type = soap_strdup(soap, MOTION_EVENT_MODULE_TYPE);
            pAnalyticsModule->Parameters = soap_new_tt__ItemList(soap,-1);
            pAnalyticsModule->Parameters->__sizeSimpleItem = 1;
            /* 移动侦测灵敏度 */
            pAnalyticsModule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap,-1);
            pAnalyticsModule->Parameters->SimpleItem->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
            pAnalyticsModule->Parameters->SimpleItem->Value = soap_strdup(soap, stInfo.achSensitivity);
            /* 移动侦测布局信息定义 */
            pAnalyticsModule->Parameters->__sizeElementItem = 1;
            pAnalyticsModule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap,-1);
            pAnalyticsModule->Parameters->ElementItem->Name = soap_strdup(soap, ONVIF_ANALYTICS_LAYOUT_NAME);
            pAnalyticsModule->Parameters->ElementItem->__any = soap_strdup(soap, ONVIF_CELL_MOTION_LAYOUT_MACRO);

            /* 移动侦测规则获取 */
            pRule->Name = soap_strdup(soap, MOTION_EVENT_RULE);
            pRule->Type = soap_strdup(soap, MOTION_EVENT_RULE_TYPE);
            pRule->Parameters = soap_new_tt__ItemList(soap,-1);
            pRule->Parameters->__sizeSimpleItem = ONVIF_CELLMOTION_RULEPARAM_NUM;
            pRule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap,ONVIF_CELLMOTION_RULEPARAM_NUM);
            struct _tt__ItemList_SimpleItem *pRuleParam_0 = pRule->Parameters->SimpleItem + 0;
            struct _tt__ItemList_SimpleItem *pRuleParam_1 = pRule->Parameters->SimpleItem + 1;
            struct _tt__ItemList_SimpleItem *pRuleParam_2 = pRule->Parameters->SimpleItem + 2;
            struct _tt__ItemList_SimpleItem *pRuleParam_3 = pRule->Parameters->SimpleItem + 3;

            if(pRuleParam_0 != NULL)
            {
                pRuleParam_0->Name = soap_strdup(soap, ONVIF_CELLMOTION_RULEPARAM_1);
                pRuleParam_0->Value = soap_strdup(soap, ONVIF_CELLMOTION_RULEPARAM_VALUE1);
            }
            if(pRuleParam_1 != NULL)
            {
                pRuleParam_1->Name = soap_strdup(soap, ONVIF_CELLMOTION_RULEPARAM_2);
                pRuleParam_1->Value = soap_strdup(soap, ONVIF_CELLMOTION_RULEPARAM_VALUE2);
            }

            if(pRuleParam_2 != NULL)
            {
                pRuleParam_2->Name = soap_strdup(soap, ONVIF_CELLMOTION_RULEPARAM_3);
                pRuleParam_2->Value = soap_strdup(soap, ONVIF_CELLMOTION_RULEPARAM_VALUE3);
            }
            /* 移动侦测网格数据 */
            if(pRuleParam_3 != NULL)
            {
                pRuleParam_3->Name = soap_strdup(soap, ONVIF_CELLMOTION_RULEPARAM_4);
                pRuleParam_3->Value = soap_strdup(soap, stInfo.achBaseStr);
            }
            break;
        }
           
        case IMAGE_OBSTRUTION_ALARM:
        {
            /* 获取当前遮挡报警配置信息 */
            ONvifTamperDetection_S stInfo;
            onvif_get_tamp_info(&stInfo);
            pAnalyticsModule->Name = soap_strdup(soap, TAMPEREVENT_MODULE);
            pAnalyticsModule->Type = soap_strdup(soap, TAMPEREVENT_MODULE_TYPE);
            pAnalyticsModule->Parameters = soap_new_tt__ItemList(soap,-1);
            pAnalyticsModule->Parameters->__sizeSimpleItem = 1;
            /* 遮挡报警灵敏度 */
            pAnalyticsModule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap,-1);
            pAnalyticsModule->Parameters->SimpleItem->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
            pAnalyticsModule->Parameters->SimpleItem->Value = soap_strdup(soap, stInfo.achSensitivity);
            
            pAnalyticsModule->Parameters->__sizeElementItem = 2;
            pAnalyticsModule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap,2);
            struct _tt__ItemList_ElementItem *pAnalyticsModulParam_0 = pAnalyticsModule->Parameters->ElementItem + 0;
            struct _tt__ItemList_ElementItem *pAnalyticsModulParam_1 = pAnalyticsModule->Parameters->ElementItem + 1;

            /* 遮挡报警缩放信息 */
            if(pAnalyticsModulParam_0 != NULL)
            {
                pAnalyticsModulParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
                pAnalyticsModulParam_0->__any = soap_strdup(soap, ONVIF_TAMPER_TRANSFORMATION_MACRO);
            }
            /* 遮挡报警坐标布局定义 */
            if(pAnalyticsModulParam_1 != NULL)
            {
                pAnalyticsModulParam_1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                pAnalyticsModulParam_1->__any = soap_strdup(soap, ONVIF_TAMPER_POLYGON_CONFIG_MACRO);
            }
            /* 遮挡报警规则获取 */
            pRule->Name = soap_strdup(soap, TAMPEREVENT_RULE);
            pRule->Type = soap_strdup(soap, TAMPEREVENT_RULE_TYPE);
            pRule->Parameters = soap_new_tt__ItemList(soap,-1);
            pRule->Parameters->__sizeElementItem = ONVIF_TAMPER_RULEPARAM_NUM;
            pRule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap,-1);
            struct _tt__ItemList_ElementItem *pRuleParam_0 = pRule->Parameters->ElementItem + 0;
            if(pRuleParam_0 != NULL)
            {
                char achPolygon[1024];
                pRuleParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                /* 遮挡区域 */
                snprintf(achPolygon, 1024,
                "<tt:PolygonConfiguration><tt:Polygon>"
                "<tt:Point x=\"%d\" y=\"%d\"/>"
                "<tt:Point x=\"%d\" y=\"%d\"/>"
                "<tt:Point x=\"%d\" y=\"%d\"/>"
                "<tt:Point x=\"%d\" y=\"%d\"/>"
                "</tt:Polygon></tt:PolygonConfiguration>",
                stInfo.stPolygon[0].x, stInfo.stPolygon[0].y,
                stInfo.stPolygon[1].x, stInfo.stPolygon[1].y,
                stInfo.stPolygon[2].x, stInfo.stPolygon[2].y,
                stInfo.stPolygon[3].x, stInfo.stPolygon[3].y);
                pRuleParam_0->__any = soap_strdup(soap, achPolygon);
              
            }

            break;
        }
           
        case INTRUSION_ALARM:
        {
            pAnalyticsModule->Name = soap_strdup(soap, FIELD_EVENT_MODULE);
            pAnalyticsModule->Type = soap_strdup(soap, FIELD_EVENT_MODULE_TYPE);
            pAnalyticsModule->Parameters = soap_new_tt__ItemList(soap,-1);
            pAnalyticsModule->Parameters->__sizeSimpleItem = 1;
            /* 区域入侵灵敏度 */
            pAnalyticsModule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap,-1);
            pAnalyticsModule->Parameters->SimpleItem->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
            pAnalyticsModule->Parameters->SimpleItem->Value = soap_strdup(soap, "50");
            
            pAnalyticsModule->Parameters->__sizeElementItem = 2;
            pAnalyticsModule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap,2);
            struct _tt__ItemList_ElementItem *pAnalyticsModulParam_0 = pAnalyticsModule->Parameters->ElementItem + 0;
            struct _tt__ItemList_ElementItem *pAnalyticsModulParam_1 = pAnalyticsModule->Parameters->ElementItem + 1;

            /* 区域入侵缩放信息 */
            if(pAnalyticsModulParam_0 != NULL)
            {
                pAnalyticsModulParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
                pAnalyticsModulParam_0->__any = soap_strdup(soap, ONVIF_FIELD_TRANSFORMATION_MACRO);
            }
            /*区域入侵坐标布局定义 */
            if(pAnalyticsModulParam_1 != NULL)
            {
                pAnalyticsModulParam_1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                pAnalyticsModulParam_1->__any = soap_strdup(soap, ONVIF_FIELD_POLYGON_CONFIG_MACRO);
            }
            break;
        }   
            
        case TRIPWIRE_ALARM:
        {
            pAnalyticsModule->Name = soap_strdup(soap, LINE_EVENT_MODULE);
            pAnalyticsModule->Type = soap_strdup(soap, LINE_EVENT_MODULE_TYPE);
            pAnalyticsModule->Parameters = soap_new_tt__ItemList(soap,-1);
            pAnalyticsModule->Parameters->__sizeSimpleItem = 1;
            /* 拌线入侵灵敏度 */
            pAnalyticsModule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap,-1);
            pAnalyticsModule->Parameters->SimpleItem->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
            pAnalyticsModule->Parameters->SimpleItem->Value = soap_strdup(soap, "50");
            
            pAnalyticsModule->Parameters->__sizeElementItem = 2;
            pAnalyticsModule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap,2);
            struct _tt__ItemList_ElementItem *pAnalyticsModulParam_0 = pAnalyticsModule->Parameters->ElementItem + 0;
            struct _tt__ItemList_ElementItem *pAnalyticsModulParam_1 = pAnalyticsModule->Parameters->ElementItem + 1;

            /* 拌线入侵缩放信息 */
            if(pAnalyticsModulParam_0 != NULL)
            {
                pAnalyticsModulParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
                pAnalyticsModulParam_0->__any = soap_strdup(soap, ONVIF_FIELD_TRANSFORMATION_MACRO);
            }
            /* 拌线入侵坐标布局定义 */
            if(pAnalyticsModulParam_1 != NULL)
            {
                pAnalyticsModulParam_1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                pAnalyticsModulParam_1->__any = soap_strdup(soap, ONVIF_FIELD_POLYGON_CONFIG_MACRO);
            }
            break;
        } 
            
        
        default:
            break;
        }

    }

    return SOAP_OK;
}

/** Web service operation '__trt__GetMetadataConfigurations' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetMetadataConfigurations(struct soap* soap, struct _trt__GetMetadataConfigurations *trt__GetMetadataConfigurations, struct _trt__GetMetadataConfigurationsResponse *trt__GetMetadataConfigurationsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetMetadataConfigurations----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__GetAudioOutputConfigurations' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioOutputConfigurations(struct soap* soap, struct _trt__GetAudioOutputConfigurations *trt__GetAudioOutputConfigurations, struct _trt__GetAudioOutputConfigurationsResponse *trt__GetAudioOutputConfigurationsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetAudioOutputConfigurations----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__GetAudioDecoderConfigurations' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioDecoderConfigurations(struct soap* soap, struct _trt__GetAudioDecoderConfigurations *trt__GetAudioDecoderConfigurations, struct _trt__GetAudioDecoderConfigurationsResponse *trt__GetAudioDecoderConfigurationsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetAudioDecoderConfigurations----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__GetVideoSourceConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetVideoSourceConfiguration(struct soap* soap, struct _trt__GetVideoSourceConfiguration *trt__GetVideoSourceConfiguration, struct _trt__GetVideoSourceConfigurationResponse *trt__GetVideoSourceConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetVideoSourceConfiguration----------");
#endif
    OnvifProfile_t stProfile;
    memset(&stProfile, 0, sizeof(stProfile));
    if(!trt__GetVideoSourceConfiguration->ConfigurationToken)
    {
        dlog_error("trt__GetVideoSourceConfiguration->ConfigurationToken is NULL");
        return SOAP_EOF;
    }

    if(strcmp(trt__GetVideoSourceConfiguration->ConfigurationToken, VIDEOSOURCE_TOKEN) == 0)
    {
        get_profile_param(0, &stProfile);
    }
    else if(strcmp(trt__GetVideoSourceConfiguration->ConfigurationToken, VIDEOSOURCE_TOKEN) == 0)
    {
        get_profile_param(1, &stProfile);
    }
    else
    {
        dlog_error("GetVideoSourceConfiguration token error");
        return SOAP_EOF;
    }

    trt__GetVideoSourceConfigurationResponse->Configuration = (struct tt__VideoSourceConfiguration *)soap_malloc(soap, sizeof(struct tt__VideoSourceConfiguration));
    memset(trt__GetVideoSourceConfigurationResponse->Configuration, '\0', sizeof(struct tt__VideoSourceConfiguration));

    trt__GetVideoSourceConfigurationResponse->Configuration->UseCount = 1;
    trt__GetVideoSourceConfigurationResponse->Configuration->Name = (char*)soap_malloc(soap, sizeof(char) * 32);
    memset(trt__GetVideoSourceConfigurationResponse->Configuration->Name, '\0', sizeof(char) * 32);
    strcpy(trt__GetVideoSourceConfigurationResponse->Configuration->Name, stProfile.VideoSourceConfiguration_Name);
    trt__GetVideoSourceConfigurationResponse->Configuration->token = (char*)soap_malloc(soap, sizeof(char) * 32);
    memset(trt__GetVideoSourceConfigurationResponse->Configuration->token, '\0', sizeof(char) * 32);
    strcpy(trt__GetVideoSourceConfigurationResponse->Configuration->token, stProfile.VideoSourceConfiguration_token);

    trt__GetVideoSourceConfigurationResponse->Configuration->SourceToken = (char*)soap_malloc(soap, sizeof(char) * 32);
    memset(trt__GetVideoSourceConfigurationResponse->Configuration->SourceToken, '\0', sizeof(char) * 32);
    strcpy(trt__GetVideoSourceConfigurationResponse->Configuration->SourceToken, stProfile.VideoSourceConfiguration_SourceToken);

    trt__GetVideoSourceConfigurationResponse->Configuration->Bounds = (struct tt__IntRectangle *)soap_malloc(soap, sizeof(struct tt__IntRectangle));
    memset(trt__GetVideoSourceConfigurationResponse->Configuration->Bounds, 0, sizeof(struct tt__IntRectangle));
    trt__GetVideoSourceConfigurationResponse->Configuration->Bounds->x      = 0;
    trt__GetVideoSourceConfigurationResponse->Configuration->Bounds->y      = 0;
    trt__GetVideoSourceConfigurationResponse->Configuration->Bounds->width  = stProfile.nWidth;
    trt__GetVideoSourceConfigurationResponse->Configuration->Bounds->height = stProfile.nHeight;

    return SOAP_OK;
}

/** Web service operation '__trt__GetVideoEncoderConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetVideoEncoderConfiguration(struct soap* soap, struct _trt__GetVideoEncoderConfiguration *trt__GetVideoEncoderConfiguration, struct _trt__GetVideoEncoderConfigurationResponse *trt__GetVideoEncoderConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetVideoEncoderConfiguration----------");
#endif
    trt__GetVideoEncoderConfigurationResponse->Configuration = (struct tt__VideoEncoderConfiguration *)soap_malloc(soap, sizeof(struct tt__VideoEncoderConfiguration));
    memset(trt__GetVideoEncoderConfigurationResponse->Configuration, '\0', sizeof(struct tt__VideoEncoderConfiguration));

    if (trt__GetVideoEncoderConfiguration->ConfigurationToken)
    {
        OnvifProfile_t stProfile;
        memset(&stProfile, 0, sizeof(stProfile));

        if (0 == strcmp(PROFILE1_VIDEOENCODER_TOKEN, trt__GetVideoEncoderConfiguration->ConfigurationToken))
        {
            get_profile_param(0, &stProfile);
        }
        else if(0 == strcmp(PROFILE2_VIDEOENCODER_TOKEN, trt__GetVideoEncoderConfiguration->ConfigurationToken))
        {
            get_profile_param(1, &stProfile);
        }
        else
        {
            return SOAP_EOF;
        }

        //<VideoEncoderConfiguration>
        trt__GetVideoEncoderConfigurationResponse->Configuration->Name = (char *)soap_malloc(soap, sizeof(char) * 32);
        memset(trt__GetVideoEncoderConfigurationResponse->Configuration->Name, '\0', sizeof(char)* 32);
        strcpy(trt__GetVideoEncoderConfigurationResponse->Configuration->Name, stProfile.VideoEncoderConfiguration_Name);
        trt__GetVideoEncoderConfigurationResponse->Configuration->token = (char *)soap_malloc(soap, sizeof(char) * 32);
        memset(trt__GetVideoEncoderConfigurationResponse->Configuration->token, '\0', sizeof(char) * 32);
        strcpy(trt__GetVideoEncoderConfigurationResponse->Configuration->token, stProfile.VideoEncoderConfiguration_token);
        OnvifVideoParam_t stActualParam;
        memset(&stActualParam, 0, sizeof(stActualParam));
        int streamIdx = 0;
        if (strcmp(stProfile.VideoEncoderConfiguration_token, PROFILE2_VIDEOENCODER_TOKEN) == 0) {
            streamIdx = 1;
        }
        onvif_get_videoParams(&stActualParam, streamIdx);

        trt__GetVideoEncoderConfigurationResponse->Configuration->UseCount = 1;
        trt__GetVideoEncoderConfigurationResponse->Configuration->Quality = (float)stActualParam.nCurQuality;
        //根据前端设备时间支持的编码格式选择对应的值，因为我测试的是设备只支持H264 ，所以选了2
        if (strcmp(stActualParam.strVideoCodec, VIDEO_CODEC_MJPEG) == 0) {
            trt__GetVideoEncoderConfigurationResponse->Configuration->Encoding = tt__VideoEncoding__JPEG;
        } else {
             trt__GetVideoEncoderConfigurationResponse->Configuration->Encoding = tt__VideoEncoding__H264;   // JPEG = 0 , MPEG = 1, H264 = 2;
        }

        //<Configuration><Resolution>
        trt__GetVideoEncoderConfigurationResponse->Configuration->Resolution = (struct tt__VideoResolution *)soap_malloc(soap,sizeof(struct tt__VideoResolution));
        memset(trt__GetVideoEncoderConfigurationResponse->Configuration->Resolution, 0 , sizeof(struct tt__VideoResolution));
        trt__GetVideoEncoderConfigurationResponse->Configuration->Resolution->Width  = stActualParam.nWidths[0];
        trt__GetVideoEncoderConfigurationResponse->Configuration->Resolution->Height = stActualParam.nHeights[0];
        //<Configuration><RateControl>
        trt__GetVideoEncoderConfigurationResponse->Configuration->RateControl = (struct tt__VideoRateControl *)soap_malloc(soap, sizeof(struct tt__VideoRateControl));
        memset(trt__GetVideoEncoderConfigurationResponse->Configuration->RateControl, 0, sizeof(struct tt__VideoRateControl));
        trt__GetVideoEncoderConfigurationResponse->Configuration->RateControl->FrameRateLimit = (int)stActualParam.fCurFrameRate;
        trt__GetVideoEncoderConfigurationResponse->Configuration->RateControl->EncodingInterval = 1;
        trt__GetVideoEncoderConfigurationResponse->Configuration->RateControl->BitrateLimit     = stActualParam.nCurBitrate;
        //<Configuration><H264>
        trt__GetVideoEncoderConfigurationResponse->Configuration->H264 = (struct tt__H264Configuration *)soap_malloc(soap, sizeof(struct tt__H264Configuration));
        memset(trt__GetVideoEncoderConfigurationResponse->Configuration->H264, 0, sizeof(struct tt__H264Configuration));
        trt__GetVideoEncoderConfigurationResponse->Configuration->H264->GovLength  = stActualParam.nIFrameInterval;
        trt__GetVideoEncoderConfigurationResponse->Configuration->H264->H264Profile = tt__H264Profile__High;

        //<Configuration><Multicast>
        trt__GetVideoEncoderConfigurationResponse->Configuration->Multicast = (struct tt__MulticastConfiguration *)soap_malloc(soap, sizeof(struct tt__MulticastConfiguration));
        memset(trt__GetVideoEncoderConfigurationResponse->Configuration->Multicast, 0, sizeof(struct tt__MulticastConfiguration));
        trt__GetVideoEncoderConfigurationResponse->Configuration->Multicast->Address = (struct tt__IPAddress *)soap_malloc(soap, sizeof(struct tt__IPAddress));
        memset(trt__GetVideoEncoderConfigurationResponse->Configuration->Multicast->Address, 0, sizeof(struct tt__IPAddress));
        trt__GetVideoEncoderConfigurationResponse->Configuration->Multicast->Address->Type = tt__IPType__IPv4;
        trt__GetVideoEncoderConfigurationResponse->Configuration->Multicast->Address->IPv4Address = (char *)soap_malloc(soap, sizeof(char) * 32);
        memset(trt__GetVideoEncoderConfigurationResponse->Configuration->Multicast->Address->IPv4Address, '\0', sizeof(char) * 32);
        strcpy(trt__GetVideoEncoderConfigurationResponse->Configuration->Multicast->Address->IPv4Address, "224.1.0.0");
        trt__GetVideoEncoderConfigurationResponse->Configuration->Multicast->Port = ONVIF_TCP_PORT;
        trt__GetVideoEncoderConfigurationResponse->Configuration->Multicast->TTL = 64;
        trt__GetVideoEncoderConfigurationResponse->Configuration->Multicast->AutoStart = xsd__boolean__true_;
        //<Configuration><SessionTimeout>
        trt__GetVideoEncoderConfigurationResponse->Configuration->SessionTimeout = ONVIF_TIME_OUT;

        dlog_debug("Media 获取单个视频编码配置: Token=%s, 编码=%d, 分辨率=%dx%d, 码率限制=%d, 帧率限制=%d, Gov长度=%d",
            trt__GetVideoEncoderConfigurationResponse->Configuration->token,
            trt__GetVideoEncoderConfigurationResponse->Configuration->Encoding,
            trt__GetVideoEncoderConfigurationResponse->Configuration->Resolution->Width,
            trt__GetVideoEncoderConfigurationResponse->Configuration->Resolution->Height,
            trt__GetVideoEncoderConfigurationResponse->Configuration->RateControl->BitrateLimit,
            trt__GetVideoEncoderConfigurationResponse->Configuration->RateControl->FrameRateLimit,
            trt__GetVideoEncoderConfigurationResponse->Configuration->H264->GovLength);
    }
    else
    {
        dlog_error("trt__GetVideoEncoderConfiguration->ConfigurationToken is NULL");
        return SOAP_EOF;
    }

    return SOAP_OK;
}

/** Web service operation '__trt__GetAudioSourceConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioSourceConfiguration(struct soap* soap, struct _trt__GetAudioSourceConfiguration *trt__GetAudioSourceConfiguration, struct _trt__GetAudioSourceConfigurationResponse *trt__GetAudioSourceConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetAudioSourceConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__GetAudioEncoderConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioEncoderConfiguration(struct soap* soap, struct _trt__GetAudioEncoderConfiguration *trt__GetAudioEncoderConfiguration, struct _trt__GetAudioEncoderConfigurationResponse *trt__GetAudioEncoderConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetAudioEncoderConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__GetVideoAnalyticsConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetVideoAnalyticsConfiguration(struct soap* soap, struct _trt__GetVideoAnalyticsConfiguration *trt__GetVideoAnalyticsConfiguration, struct _trt__GetVideoAnalyticsConfigurationResponse *trt__GetVideoAnalyticsConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetVideoAnalyticsConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__GetMetadataConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetMetadataConfiguration(struct soap* soap, struct _trt__GetMetadataConfiguration *trt__GetMetadataConfiguration, struct _trt__GetMetadataConfigurationResponse *trt__GetMetadataConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetMetadataConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__GetAudioOutputConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioOutputConfiguration(struct soap* soap, struct _trt__GetAudioOutputConfiguration *trt__GetAudioOutputConfiguration, struct _trt__GetAudioOutputConfigurationResponse *trt__GetAudioOutputConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetAudioOutputConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__GetAudioDecoderConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioDecoderConfiguration(struct soap* soap, struct _trt__GetAudioDecoderConfiguration *trt__GetAudioDecoderConfiguration, struct _trt__GetAudioDecoderConfigurationResponse *trt__GetAudioDecoderConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetAudioDecoderConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__GetCompatibleVideoEncoderConfigurations' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetCompatibleVideoEncoderConfigurations(struct soap* soap, struct _trt__GetCompatibleVideoEncoderConfigurations *trt__GetCompatibleVideoEncoderConfigurations, struct _trt__GetCompatibleVideoEncoderConfigurationsResponse *trt__GetCompatibleVideoEncoderConfigurationsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetCompatibleVideoEncoderConfigurations----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__GetCompatibleVideoSourceConfigurations' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetCompatibleVideoSourceConfigurations(struct soap* soap, struct _trt__GetCompatibleVideoSourceConfigurations *trt__GetCompatibleVideoSourceConfigurations, struct _trt__GetCompatibleVideoSourceConfigurationsResponse *trt__GetCompatibleVideoSourceConfigurationsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetCompatibleVideoSourceConfigurations----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__GetCompatibleAudioEncoderConfigurations' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetCompatibleAudioEncoderConfigurations(struct soap* soap, struct _trt__GetCompatibleAudioEncoderConfigurations *trt__GetCompatibleAudioEncoderConfigurations, struct _trt__GetCompatibleAudioEncoderConfigurationsResponse *trt__GetCompatibleAudioEncoderConfigurationsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetCompatibleAudioEncoderConfigurations----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__GetCompatibleAudioSourceConfigurations' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetCompatibleAudioSourceConfigurations(struct soap* soap, struct _trt__GetCompatibleAudioSourceConfigurations *trt__GetCompatibleAudioSourceConfigurations, struct _trt__GetCompatibleAudioSourceConfigurationsResponse *trt__GetCompatibleAudioSourceConfigurationsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetCompatibleAudioSourceConfigurations----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__GetCompatibleVideoAnalyticsConfigurations' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetCompatibleVideoAnalyticsConfigurations(struct soap* soap, struct _trt__GetCompatibleVideoAnalyticsConfigurations *trt__GetCompatibleVideoAnalyticsConfigurations, struct _trt__GetCompatibleVideoAnalyticsConfigurationsResponse *trt__GetCompatibleVideoAnalyticsConfigurationsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetCompatibleVideoAnalyticsConfigurations----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__GetCompatibleMetadataConfigurations' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetCompatibleMetadataConfigurations(struct soap* soap, struct _trt__GetCompatibleMetadataConfigurations *trt__GetCompatibleMetadataConfigurations, struct _trt__GetCompatibleMetadataConfigurationsResponse *trt__GetCompatibleMetadataConfigurationsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetCompatibleMetadataConfigurations----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__GetCompatibleAudioOutputConfigurations' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetCompatibleAudioOutputConfigurations(struct soap* soap, struct _trt__GetCompatibleAudioOutputConfigurations *trt__GetCompatibleAudioOutputConfigurations, struct _trt__GetCompatibleAudioOutputConfigurationsResponse *trt__GetCompatibleAudioOutputConfigurationsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetCompatibleAudioOutputConfigurations----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__GetCompatibleAudioDecoderConfigurations' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetCompatibleAudioDecoderConfigurations(struct soap* soap, struct _trt__GetCompatibleAudioDecoderConfigurations *trt__GetCompatibleAudioDecoderConfigurations, struct _trt__GetCompatibleAudioDecoderConfigurationsResponse *trt__GetCompatibleAudioDecoderConfigurationsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetCompatibleAudioDecoderConfigurations----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__SetVideoSourceConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__SetVideoSourceConfiguration(struct soap* soap, struct _trt__SetVideoSourceConfiguration *trt__SetVideoSourceConfiguration, struct _trt__SetVideoSourceConfigurationResponse *trt__SetVideoSourceConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__SetVideoSourceConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__SetVideoEncoderConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__SetVideoEncoderConfiguration(struct soap* soap, struct _trt__SetVideoEncoderConfiguration *trt__SetVideoEncoderConfiguration, struct _trt__SetVideoEncoderConfigurationResponse *trt__SetVideoEncoderConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__SetVideoEncoderConfiguration----------");
#endif
    if (trt__SetVideoEncoderConfiguration && trt__SetVideoEncoderConfiguration->Configuration) {
        dlog_debug("Media 设置视频编码配置 (未实现): Token=%s, 编码=%d, 分辨率=%dx%d, 码率限制=%d, 帧率限制=%d",
            trt__SetVideoEncoderConfiguration->Configuration->token ? trt__SetVideoEncoderConfiguration->Configuration->token : "NULL",
            trt__SetVideoEncoderConfiguration->Configuration->Encoding,
            trt__SetVideoEncoderConfiguration->Configuration->Resolution ? trt__SetVideoEncoderConfiguration->Configuration->Resolution->Width : 0,
            trt__SetVideoEncoderConfiguration->Configuration->Resolution ? trt__SetVideoEncoderConfiguration->Configuration->Resolution->Height : 0,
            trt__SetVideoEncoderConfiguration->Configuration->RateControl ? trt__SetVideoEncoderConfiguration->Configuration->RateControl->BitrateLimit : 0,
            trt__SetVideoEncoderConfiguration->Configuration->RateControl ? trt__SetVideoEncoderConfiguration->Configuration->RateControl->FrameRateLimit : 0);
    }
   /* if (NULL != trt__SetVideoEncoderConfiguration->Configuration &&
        tt__VideoEncoding__H264 == trt__SetVideoEncoderConfiguration->Configuration->Encoding) {

        if (NULL != trt__SetVideoEncoderConfiguration->Configuration->Resolution) {
            if (0 == strcmp(trt__SetVideoEncoderConfiguration->Configuration->token, "000")) {
                stVideoCfg.u32MainWidth = trt__SetVideoEncoderConfiguration->Configuration->Resolution->Width;
                stVideoCfg.u32MainHeight = trt__SetVideoEncoderConfiguration->Configuration->Resolution->Height;
            }
            else {
                stVideoCfg.u32SubWidth = trt__SetVideoEncoderConfiguration->Configuration->Resolution->Width;
                stVideoCfg.u32SubHeight = trt__SetVideoEncoderConfiguration->Configuration->Resolution->Height;
            }
        }

    }*/

    return SOAP_OK;
}

/** Web service operation '__trt__SetAudioSourceConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__SetAudioSourceConfiguration(struct soap* soap, struct _trt__SetAudioSourceConfiguration *trt__SetAudioSourceConfiguration, struct _trt__SetAudioSourceConfigurationResponse *trt__SetAudioSourceConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__SetAudioSourceConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__SetAudioEncoderConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__SetAudioEncoderConfiguration(struct soap* soap, struct _trt__SetAudioEncoderConfiguration *trt__SetAudioEncoderConfiguration, struct _trt__SetAudioEncoderConfigurationResponse *trt__SetAudioEncoderConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__SetAudioEncoderConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__SetVideoAnalyticsConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__SetVideoAnalyticsConfiguration(struct soap* soap, struct _trt__SetVideoAnalyticsConfiguration *trt__SetVideoAnalyticsConfiguration, struct _trt__SetVideoAnalyticsConfigurationResponse *trt__SetVideoAnalyticsConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__SetVideoAnalyticsConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__SetMetadataConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__SetMetadataConfiguration(struct soap* soap, struct _trt__SetMetadataConfiguration *trt__SetMetadataConfiguration, struct _trt__SetMetadataConfigurationResponse *trt__SetMetadataConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__SetMetadataConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__SetAudioOutputConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__SetAudioOutputConfiguration(struct soap* soap, struct _trt__SetAudioOutputConfiguration *trt__SetAudioOutputConfiguration, struct _trt__SetAudioOutputConfigurationResponse *trt__SetAudioOutputConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__SetAudioOutputConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__SetAudioDecoderConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__SetAudioDecoderConfiguration(struct soap* soap, struct _trt__SetAudioDecoderConfiguration *trt__SetAudioDecoderConfiguration, struct _trt__SetAudioDecoderConfigurationResponse *trt__SetAudioDecoderConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__SetAudioDecoderConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__GetVideoSourceConfigurationOptions' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetVideoSourceConfigurationOptions(struct soap* soap, struct _trt__GetVideoSourceConfigurationOptions *trt__GetVideoSourceConfigurationOptions, struct _trt__GetVideoSourceConfigurationOptionsResponse *trt__GetVideoSourceConfigurationOptionsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetVideoSourceConfigurationOptions----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__GetVideoEncoderConfigurationOptions' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetVideoEncoderConfigurationOptions(struct soap* soap, struct _trt__GetVideoEncoderConfigurationOptions *trt__GetVideoEncoderConfigurationOptions, struct _trt__GetVideoEncoderConfigurationOptionsResponse *trt__GetVideoEncoderConfigurationOptionsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetVideoEncoderConfigurationOptions----------");
#endif
    OnvifProfile_t stProfile;
    memset(&stProfile, 0, sizeof(stProfile));
    
    int nIndex = -1;

    if (trt__GetVideoEncoderConfigurationOptions->ConfigurationToken)
    {
        if (0 == strcmp(PROFILE1_VIDEOENCODER_TOKEN, trt__GetVideoEncoderConfigurationOptions->ConfigurationToken))
        {
            nIndex = 0;
        }
        else if(0 == strcmp(PROFILE2_VIDEOENCODER_TOKEN, trt__GetVideoEncoderConfigurationOptions->ConfigurationToken))
        {
            nIndex = 1;
        }
    }
    else if (trt__GetVideoEncoderConfigurationOptions->ProfileToken)
    {
        if (0 == strcmp(PROFILE1_TOKEN, trt__GetVideoEncoderConfigurationOptions->ProfileToken))
        {
            nIndex = 0;
        }
        else if(0 == strcmp(PROFILE2_TOKEN, trt__GetVideoEncoderConfigurationOptions->ProfileToken))
        {
            nIndex = 1;
        }
    }

    if (nIndex == -1)
    {
         // Default to Main Stream if not specified or invalid? 
         // Or error? Usually return capabilities for all compatible? 
         // For now, default to 0 (Main) if not specified, strictly maybe error.
         // But let's assume 0.
         nIndex = 0; 
    }

    trt__GetVideoEncoderConfigurationOptionsResponse->Options = (struct tt__VideoEncoderConfigurationOptions *)soap_malloc(soap, sizeof(struct tt__VideoEncoderConfigurationOptions));
    memset(trt__GetVideoEncoderConfigurationOptionsResponse->Options, 0, sizeof(struct tt__VideoEncoderConfigurationOptions));
    
    // Quality Range (Common)
    trt__GetVideoEncoderConfigurationOptionsResponse->Options->QualityRange = (struct tt__FloatRange *)soap_malloc(soap, sizeof(struct tt__FloatRange));
    trt__GetVideoEncoderConfigurationOptionsResponse->Options->QualityRange->Min = VIDEO_QUALITY_MIN;
    trt__GetVideoEncoderConfigurationOptionsResponse->Options->QualityRange->Max = VIDEO_QUALITY_MAX;
    
    // Iterate codecs
    int count = onvif_get_supported_codec_count(nIndex);
    for(int i=0; i<count; i++)
    {
        OnvifVideoParam_t stVideoParams;
        memset(&stVideoParams, 0, sizeof(stVideoParams));
        onvif_get_video_capability_by_index(&stVideoParams, nIndex, i);

        if (strstr(stVideoParams.strVideoCodec, "H264") || strstr(stVideoParams.strVideoCodec, "H.264"))
        {
            trt__GetVideoEncoderConfigurationOptionsResponse->Options->H264 = (struct tt__H264Options *)soap_malloc(soap, sizeof(struct tt__H264Options));
            memset(trt__GetVideoEncoderConfigurationOptionsResponse->Options->H264, 0, sizeof(struct tt__H264Options));
            
            // Resolutions
            trt__GetVideoEncoderConfigurationOptionsResponse->Options->H264->__sizeResolutionsAvailable = stVideoParams.nSizeResolutionsAvailable;
            trt__GetVideoEncoderConfigurationOptionsResponse->Options->H264->ResolutionsAvailable = (struct tt__VideoResolution *)soap_malloc(soap, sizeof(struct tt__VideoResolution) * stVideoParams.nSizeResolutionsAvailable);
            for(int j=0; j<stVideoParams.nSizeResolutionsAvailable; j++)
            {
               trt__GetVideoEncoderConfigurationOptionsResponse->Options->H264->ResolutionsAvailable[j].Width = stVideoParams.nWidths[j];
               trt__GetVideoEncoderConfigurationOptionsResponse->Options->H264->ResolutionsAvailable[j].Height = stVideoParams.nHeights[j];
            }

            // Ranges
            trt__GetVideoEncoderConfigurationOptionsResponse->Options->H264->GovLengthRange = (struct tt__IntRange *)soap_malloc(soap, sizeof(struct tt__IntRange));
            trt__GetVideoEncoderConfigurationOptionsResponse->Options->H264->GovLengthRange->Min = 1; 
            trt__GetVideoEncoderConfigurationOptionsResponse->Options->H264->GovLengthRange->Max = VIDEO_I_FRAME_INTERVAL_MAX;

            trt__GetVideoEncoderConfigurationOptionsResponse->Options->H264->FrameRateRange = (struct tt__IntRange *)soap_malloc(soap, sizeof(struct tt__IntRange));
            trt__GetVideoEncoderConfigurationOptionsResponse->Options->H264->FrameRateRange->Min = 1;
            trt__GetVideoEncoderConfigurationOptionsResponse->Options->H264->FrameRateRange->Max = VIDEO_FRAMERATE_MAX;



            trt__GetVideoEncoderConfigurationOptionsResponse->Options->H264->EncodingIntervalRange = (struct tt__IntRange *)soap_malloc(soap, sizeof(struct tt__IntRange));
            trt__GetVideoEncoderConfigurationOptionsResponse->Options->H264->EncodingIntervalRange->Min = 1;
            trt__GetVideoEncoderConfigurationOptionsResponse->Options->H264->EncodingIntervalRange->Max = 1;
            
            // Profiles
            trt__GetVideoEncoderConfigurationOptionsResponse->Options->H264->__sizeH264ProfilesSupported = 1;
            trt__GetVideoEncoderConfigurationOptionsResponse->Options->H264->H264ProfilesSupported = (enum tt__H264Profile *)soap_malloc(soap, sizeof(enum tt__H264Profile));
            *trt__GetVideoEncoderConfigurationOptionsResponse->Options->H264->H264ProfilesSupported = tt__H264Profile__High; 
        }
        else if (strstr(stVideoParams.strVideoCodec, "JPEG") || strstr(stVideoParams.strVideoCodec, "MJPEG"))
        {
            trt__GetVideoEncoderConfigurationOptionsResponse->Options->JPEG = (struct tt__JpegOptions *)soap_malloc(soap, sizeof(struct tt__JpegOptions));
            memset(trt__GetVideoEncoderConfigurationOptionsResponse->Options->JPEG, 0, sizeof(struct tt__JpegOptions));

             // Resolutions
            trt__GetVideoEncoderConfigurationOptionsResponse->Options->JPEG->__sizeResolutionsAvailable = stVideoParams.nSizeResolutionsAvailable;
            trt__GetVideoEncoderConfigurationOptionsResponse->Options->JPEG->ResolutionsAvailable = (struct tt__VideoResolution *)soap_malloc(soap, sizeof(struct tt__VideoResolution) * stVideoParams.nSizeResolutionsAvailable);
            for(int j=0; j<stVideoParams.nSizeResolutionsAvailable; j++)
            {
               trt__GetVideoEncoderConfigurationOptionsResponse->Options->JPEG->ResolutionsAvailable[j].Width = stVideoParams.nWidths[j];
               trt__GetVideoEncoderConfigurationOptionsResponse->Options->JPEG->ResolutionsAvailable[j].Height = stVideoParams.nHeights[j];
            }

             // Ranges
            trt__GetVideoEncoderConfigurationOptionsResponse->Options->JPEG->FrameRateRange = (struct tt__IntRange *)soap_malloc(soap, sizeof(struct tt__IntRange));
            trt__GetVideoEncoderConfigurationOptionsResponse->Options->JPEG->FrameRateRange->Min = 1;
            trt__GetVideoEncoderConfigurationOptionsResponse->Options->JPEG->FrameRateRange->Max = VIDEO_FRAMERATE_MAX;

            trt__GetVideoEncoderConfigurationOptionsResponse->Options->JPEG->EncodingIntervalRange = (struct tt__IntRange *)soap_malloc(soap, sizeof(struct tt__IntRange));
            trt__GetVideoEncoderConfigurationOptionsResponse->Options->JPEG->EncodingIntervalRange->Min = 1;
            trt__GetVideoEncoderConfigurationOptionsResponse->Options->JPEG->EncodingIntervalRange->Max = 1;
        }
    }

    return SOAP_OK;
}

/** Web service operation '__trt__GetAudioSourceConfigurationOptions' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioSourceConfigurationOptions(struct soap* soap, struct _trt__GetAudioSourceConfigurationOptions *trt__GetAudioSourceConfigurationOptions, struct _trt__GetAudioSourceConfigurationOptionsResponse *trt__GetAudioSourceConfigurationOptionsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetAudioSourceConfigurationOptions----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__GetAudioEncoderConfigurationOptions' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioEncoderConfigurationOptions(struct soap* soap, struct _trt__GetAudioEncoderConfigurationOptions *trt__GetAudioEncoderConfigurationOptions, struct _trt__GetAudioEncoderConfigurationOptionsResponse *trt__GetAudioEncoderConfigurationOptionsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetAudioEncoderConfigurationOptions----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__GetMetadataConfigurationOptions' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetMetadataConfigurationOptions(struct soap* soap, struct _trt__GetMetadataConfigurationOptions *trt__GetMetadataConfigurationOptions, struct _trt__GetMetadataConfigurationOptionsResponse *trt__GetMetadataConfigurationOptionsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetMetadataConfigurationOptions----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__GetAudioOutputConfigurationOptions' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioOutputConfigurationOptions(struct soap* soap, struct _trt__GetAudioOutputConfigurationOptions *trt__GetAudioOutputConfigurationOptions, struct _trt__GetAudioOutputConfigurationOptionsResponse *trt__GetAudioOutputConfigurationOptionsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetAudioOutputConfigurationOptions----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__GetAudioDecoderConfigurationOptions' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetAudioDecoderConfigurationOptions(struct soap* soap, struct _trt__GetAudioDecoderConfigurationOptions *trt__GetAudioDecoderConfigurationOptions, struct _trt__GetAudioDecoderConfigurationOptionsResponse *trt__GetAudioDecoderConfigurationOptionsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetAudioDecoderConfigurationOptions----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__GetGuaranteedNumberOfVideoEncoderInstances' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetGuaranteedNumberOfVideoEncoderInstances(struct soap* soap, struct _trt__GetGuaranteedNumberOfVideoEncoderInstances *trt__GetGuaranteedNumberOfVideoEncoderInstances, struct _trt__GetGuaranteedNumberOfVideoEncoderInstancesResponse *trt__GetGuaranteedNumberOfVideoEncoderInstancesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetGuaranteedNumberOfVideoEncoderInstances----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__GetStreamUri' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetStreamUri(struct soap* soap, struct _trt__GetStreamUri *trt__GetStreamUri, struct _trt__GetStreamUriResponse *trt__GetStreamUriResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetStreamUri----------");
#endif
    if(!trt__GetStreamUri->ProfileToken)
    {
        dlog_error("trt__GetStreamUri.ProfileToken is NULL");
        return SOAP_EOF;
    }

    int nRet = onvif_authentication(soap);
    if(SOAP_OK != nRet)
    {
        return nRet;
    }

    trt__GetStreamUriResponse->MediaUri = (struct tt__MediaUri *)soap_malloc(soap, sizeof(struct tt__MediaUri));
    memset(trt__GetStreamUriResponse->MediaUri, 0, sizeof(struct tt__MediaUri));

    trt__GetStreamUriResponse->MediaUri->Uri = (char *)soap_malloc(soap, sizeof(char) * 100);
    memset(trt__GetStreamUriResponse->MediaUri->Uri, '\0', sizeof(char) * 100);

    char *achIp = get_primary_ip();
    if(achIp == NULL)
    {
        dlog_error("没有网络");
        return SOAP_FAULT;
    }

    if(strlen(achIp) == 0)
    {
        dlog_error("获取网卡ip失败");
        return SOAP_FAULT;
    }

    char *pUrl = NULL;

    if(strcmp(trt__GetStreamUri->ProfileToken, PROFILE1_TOKEN) == 0)
    {
        pUrl = onvif_get_rtsp_url(ONVIF_RTSP_CHN_MAIN);
    }
    else if(strcmp(trt__GetStreamUri->ProfileToken, PROFILE2_TOKEN) == 0)
    {
        pUrl = onvif_get_rtsp_url(ONVIF_RTSP_CHN_SUB);
    }
    else
    {
        dlog_error("trt__GetStreamUri.ProfileToken error");
        return SOAP_FAULT;
    }

    if(pUrl == NULL)
    {
        dlog_error("获取取流地址失败");
        return SOAP_FAULT;
    }

    sprintf(trt__GetStreamUriResponse->MediaUri->Uri, "%s", pUrl);
    // dlog_debug("trt__GetStreamUriResponse->MediaUri: %s", trt__GetStreamUriResponse->MediaUri)
    trt__GetStreamUriResponse->MediaUri->InvalidAfterConnect = xsd__boolean__true_;
    trt__GetStreamUriResponse->MediaUri->InvalidAfterReboot  = xsd__boolean__true_;
    //超时时间
    trt__GetStreamUriResponse->MediaUri->Timeout = 200;

    return SOAP_OK;
}

/** Web service operation '__trt__StartMulticastStreaming' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__StartMulticastStreaming(struct soap* soap, struct _trt__StartMulticastStreaming *trt__StartMulticastStreaming, struct _trt__StartMulticastStreamingResponse *trt__StartMulticastStreamingResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__StartMulticastStreaming----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__StopMulticastStreaming' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__StopMulticastStreaming(struct soap* soap, struct _trt__StopMulticastStreaming *trt__StopMulticastStreaming, struct _trt__StopMulticastStreamingResponse *trt__StopMulticastStreamingResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__StopMulticastStreaming----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__SetSynchronizationPoint' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__SetSynchronizationPoint(struct soap* soap, struct _trt__SetSynchronizationPoint *trt__SetSynchronizationPoint, struct _trt__SetSynchronizationPointResponse *trt__SetSynchronizationPointResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__SetSynchronizationPoint----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__GetSnapshotUri' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetSnapshotUri(struct soap* soap, struct _trt__GetSnapshotUri *trt__GetSnapshotUri, struct _trt__GetSnapshotUriResponse *trt__GetSnapshotUriResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetSnapshotUri----------");
#endif
    char ip_addr[16] = {0};
    getlocalip(ip_addr);

    trt__GetSnapshotUriResponse->MediaUri = (struct tt__MediaUri *)soap_malloc(soap, sizeof(struct tt__MediaUri));
    memset(trt__GetSnapshotUriResponse->MediaUri, 0, sizeof(struct tt__MediaUri));

    trt__GetSnapshotUriResponse->MediaUri->Uri = (char *)soap_malloc(soap, sizeof(char) * 100);
    memset(trt__GetSnapshotUriResponse->MediaUri->Uri, 0, sizeof(char) * 100);
    sprintf(trt__GetSnapshotUriResponse->MediaUri->Uri, "http://%s:%d/snap0.jpeg", ip_addr, 80);
    trt__GetSnapshotUriResponse->MediaUri->InvalidAfterConnect = xsd__boolean__false_;
    trt__GetSnapshotUriResponse->MediaUri->InvalidAfterReboot = xsd__boolean__false_;
    trt__GetSnapshotUriResponse->MediaUri->Timeout = ONVIF_TIME_OUT;

    return SOAP_OK;
}

/** Web service operation '__trt__GetVideoSourceModes' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetVideoSourceModes(struct soap* soap, struct _trt__GetVideoSourceModes *trt__GetVideoSourceModes, struct _trt__GetVideoSourceModesResponse *trt__GetVideoSourceModesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetVideoSourceModes----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__SetVideoSourceMode' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__SetVideoSourceMode(struct soap* soap, struct _trt__SetVideoSourceMode *trt__SetVideoSourceMode, struct _trt__SetVideoSourceModeResponse *trt__SetVideoSourceModeResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__SetVideoSourceMode----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trt__GetOSDs' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetOSDs(struct soap* soap, struct _trt__GetOSDs *trt__GetOSDs, struct _trt__GetOSDsResponse *trt__GetOSDsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetOSDs----------");
#endif
     int nRet = onvif_authentication(soap);
    if(SOAP_OK != nRet)
    {
        return nRet;
    }

    int nCount = 0;
    OnvifOsdCfg_t stOnvifOsdCfgs[ONVIF_OSD_MAX_NUM];
    memset(stOnvifOsdCfgs, 0, sizeof(stOnvifOsdCfgs));
    onvif_get_osdParam(&stOnvifOsdCfgs, &nCount);
    trt__GetOSDsResponse->__sizeOSDs = nCount; 
    trt__GetOSDsResponse->OSDs = soap_new_tt__OSDConfiguration(soap,trt__GetOSDsResponse->__sizeOSDs);
    for(int i = 0; i < trt__GetOSDsResponse->__sizeOSDs; i++)
    {   
        trt__GetOSDsResponse->OSDs[i].token = soap_strdup(soap, stOnvifOsdCfgs[i].token);

        trt__GetOSDsResponse->OSDs[i].VideoSourceConfigurationToken = soap_new_tt__OSDReference(soap,-1);
        trt__GetOSDsResponse->OSDs[i].VideoSourceConfigurationToken->__item = soap_strdup(soap, VIDEOSOURCE_TOKEN);

        trt__GetOSDsResponse->OSDs[i].Position = soap_new_tt__OSDPosConfiguration(soap,-1);
        trt__GetOSDsResponse->OSDs[i].Position->Type = soap_strdup(soap, stOnvifOsdCfgs[i].Position_Type);

     
        trt__GetOSDsResponse->OSDs[i].Position->Pos = soap_new_tt__Vector(soap,-1);
        trt__GetOSDsResponse->OSDs[i].Position->Pos->x = (float *)soap_malloc(soap, sizeof(float *)); 
        trt__GetOSDsResponse->OSDs[i].Position->Pos->y = (float *)soap_malloc(soap, sizeof(float *));

        *(trt__GetOSDsResponse->OSDs[i].Position->Pos->x) = stOnvifOsdCfgs[i].stONvifPos.x;
        *(trt__GetOSDsResponse->OSDs[i].Position->Pos->y) = stOnvifOsdCfgs[i].stONvifPos.y;
        
        trt__GetOSDsResponse->OSDs[i].Type =  stOnvifOsdCfgs[i].eOsdType;

        if(trt__GetOSDsResponse->OSDs[i].Type == tt__OSDType__Text)
        {
            trt__GetOSDsResponse->OSDs[i].TextString = soap_new_tt__OSDTextConfiguration(soap,-1);
            trt__GetOSDsResponse->OSDs[i].TextString->Type = soap_strdup(soap, stOnvifOsdCfgs[i].TextString_Type);

            if(strcmp(trt__GetOSDsResponse->OSDs[i].TextString->Type,ONVIF_TT_TYPE_DATE) == 0)
            {
                trt__GetOSDsResponse->OSDs[i].TextString->DateFormat = soap_strdup(soap, stOnvifOsdCfgs[i].TextString_DateFormat);
            }
            else if(strcmp(trt__GetOSDsResponse->OSDs[i].TextString->Type, ONVIF_TT_TYPE_TIME) == 0)
            {
                trt__GetOSDsResponse->OSDs[i].TextString->TimeFormat = soap_strdup(soap, stOnvifOsdCfgs[i].TextString_TimeFormat);
            }
            else if(strcmp(trt__GetOSDsResponse->OSDs[i].TextString->Type, ONVIF_TT_TYPE_DATEANDTIME) == 0)
            {
                trt__GetOSDsResponse->OSDs[i].TextString->DateFormat = soap_strdup(soap, stOnvifOsdCfgs[i].TextString_DateFormat);
                trt__GetOSDsResponse->OSDs[i].TextString->TimeFormat = soap_strdup(soap, stOnvifOsdCfgs[i].TextString_TimeFormat);      
            }
            else if(strcmp(trt__GetOSDsResponse->OSDs[i].TextString->Type, ONVIF_TT_TYPE_PLAIN) == 0)
            {
                trt__GetOSDsResponse->OSDs[i].TextString->PlainText = soap_strdup(soap, stOnvifOsdCfgs[i].TextString_PlainText);
            }

            trt__GetOSDsResponse->OSDs[i].TextString->Extension = soap_new_tt__OSDTextConfigurationExtension(soap,-1);
            trt__GetOSDsResponse->OSDs[i].TextString->Extension->__size = 1;
            trt__GetOSDsResponse->OSDs[i].TextString->Extension->__any = (char **)soap_malloc(soap, sizeof(char *));
            if(stOnvifOsdCfgs[i].eTextType == E_OSDTYPE_TEXT_NAME)
            {
                trt__GetOSDsResponse->OSDs[i].TextString->Extension->__any[0] = soap_strdup(soap, ONVIF_TT_OSD_TXET_EXTEN_CHANNEL_TRUE);
            }
            else
            {
                trt__GetOSDsResponse->OSDs[i].TextString->Extension->__any[0] = soap_strdup(soap, ONVIF_TT_OSD_TXET_EXTEN_CHANNEL_FALSE);
            }
            trt__GetOSDsResponse->OSDs[i].TextString->FontSize = (int *)soap_malloc(soap, sizeof(int));
            *(trt__GetOSDsResponse->OSDs[i].TextString->FontSize) =  stOnvifOsdCfgs[i].TextString_FontSize;

            struct tt__OSDColor *FontColor = (struct tt__OSDColor *)soap_malloc(soap, sizeof(struct tt__OSDColor));
            trt__GetOSDsResponse->OSDs[i].TextString->FontColor = soap_new_tt__OSDColor(soap,-1);
            if(trt__GetOSDsResponse->OSDs[i].TextString->FontColor == NULL)
            {
                dlog_debug(" ============= FontColor is NULL================= ");
            }
            trt__GetOSDsResponse->OSDs[i].TextString->FontColor->Transparent = (int *)soap_malloc(soap, sizeof(int));
            *(trt__GetOSDsResponse->OSDs[i].TextString->FontColor->Transparent) =  stOnvifOsdCfgs[i].TextString_FontAlpha;

            trt__GetOSDsResponse->OSDs[i].TextString->FontColor->Color = soap_new_tt__Color(soap,-1);;
            trt__GetOSDsResponse->OSDs[i].TextString->FontColor->Color->X =  (float)stOnvifOsdCfgs[i].TextString_Font_R;
            trt__GetOSDsResponse->OSDs[i].TextString->FontColor->Color->Y =  (float)stOnvifOsdCfgs[i].TextString_Font_G;
            trt__GetOSDsResponse->OSDs[i].TextString->FontColor->Color->Z =  (float)stOnvifOsdCfgs[i].TextString_Font_B;
            trt__GetOSDsResponse->OSDs[i].TextString->FontColor->Color->Colorspace =  soap_strdup(soap, ONVIF_TT_OSD_COLORSPACE);
        
            // trt__GetOSDsResponse->OSDs[i].TextString->BackgroundColor = (struct tt__OSDColor *)soap_malloc(soap, sizeof(struct tt__OSDColor));
            // trt__GetOSDsResponse->OSDs[i].TextString->BackgroundColor->Transparent = (int *)soap_malloc(soap, sizeof(int));
            // *(trt__GetOSDsResponse->OSDs[i].TextString->BackgroundColor->Transparent) =  stOnvifOsdCfgs[i].TextString_BackgroundAlpha;
            
            // trt__GetOSDsResponse->OSDs[i].TextString->BackgroundColor->Color = (struct tt__Color *)soap_malloc(soap, sizeof(struct tt__Color));
            // trt__GetOSDsResponse->OSDs[i].TextString->BackgroundColor->Color->X =  (float)stOnvifOsdCfgs[i].TextString_Background_R;
            // trt__GetOSDsResponse->OSDs[i].TextString->BackgroundColor->Color->Y =  (float)stOnvifOsdCfgs[i].TextString_Background_G;
            // trt__GetOSDsResponse->OSDs[i].TextString->BackgroundColor->Color->Z =  (float)stOnvifOsdCfgs[i].TextString_Background_B;

        }
        else if(trt__GetOSDsResponse->OSDs[i].Type == tt__OSDType__Image)
        {

        }
        else if(trt__GetOSDsResponse->OSDs[i].Type == tt__OSDType__Extended)
        {
            
        }
        else
        {

        }
    }

    return SOAP_OK;
}

/** Web service operation '__trt__GetOSD' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetOSD(struct soap* soap, struct _trt__GetOSD *trt__GetOSD, struct _trt__GetOSDResponse *trt__GetOSDResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetOSD----------");
#endif
    
    return SOAP_OK;
}

/** Web service operation '__trt__GetOSDOptions' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__GetOSDOptions(struct soap* soap, struct _trt__GetOSDOptions *trt__GetOSDOptions, struct _trt__GetOSDOptionsResponse *trt__GetOSDOptionsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__GetOSDOptions----------");
#endif
    int nRet = onvif_authentication(soap);
    if(SOAP_OK != nRet)
    {
        return nRet;
    }
    /* token校验 */
    if(trt__GetOSDOptions != NULL && trt__GetOSDOptions->ConfigurationToken != NULL)
    {
        if(strcmp(trt__GetOSDOptions->ConfigurationToken,VIDEOSOURCE_TOKEN) != 0)
        {
            
            dlog_error("requested  VideoSourceConfiguration does not exist");
            return soap_receiver_fault(soap, "requested VideoSourceConfiguration does not exist", NULL); 
        }
    }
    else
    {
        return soap_receiver_fault(soap, "requested VideoSourceConfiguration does not exist", NULL); 
    }

    if(trt__GetOSDOptionsResponse == NULL)
    {
        dlog_error("trt__GetOSDOptionsResponse is NULL");
        return soap_receiver_fault(soap, "trt__GetOSDOptionsResponse is NULL", NULL);    
    }

    trt__GetOSDOptionsResponse->__size = 1;
    trt__GetOSDOptionsResponse->OSDOptions = soap_new_tt__OSDConfigurationOptions(soap,-1);

    trt__GetOSDOptionsResponse->OSDOptions->MaximumNumberOfOSDs = soap_new_tt__MaximumNumberOfOSDs(soap,-1);
    trt__GetOSDOptionsResponse->OSDOptions->MaximumNumberOfOSDs->Total = ONVIF_OSD_MAX_NUM;
    trt__GetOSDOptionsResponse->OSDOptions->MaximumNumberOfOSDs->PlainText = (int *)soap_malloc(soap,sizeof(int *));
    trt__GetOSDOptionsResponse->OSDOptions->MaximumNumberOfOSDs->Date = (int *)soap_malloc(soap,sizeof(int *));
    trt__GetOSDOptionsResponse->OSDOptions->MaximumNumberOfOSDs->Time = (int *)soap_malloc(soap,sizeof(int *));
    trt__GetOSDOptionsResponse->OSDOptions->MaximumNumberOfOSDs->DateAndTime = (int *)soap_malloc(soap,sizeof(int *));
    *(trt__GetOSDOptionsResponse->OSDOptions->MaximumNumberOfOSDs->PlainText) = ONVIF_OSD_TEXT_NUM;
    *(trt__GetOSDOptionsResponse->OSDOptions->MaximumNumberOfOSDs->Date) = ONVIF_OSD_DATE_NUM;
    *(trt__GetOSDOptionsResponse->OSDOptions->MaximumNumberOfOSDs->Time) = ONVIF_OSD_DATE_NUM;
    *(trt__GetOSDOptionsResponse->OSDOptions->MaximumNumberOfOSDs->DateAndTime) = ONVIF_OSD_DATE_NUM;
   
    trt__GetOSDOptionsResponse->OSDOptions->__sizeType = 1;
    trt__GetOSDOptionsResponse->OSDOptions->Type = soap_new_tt__OSDType(soap,-1);
    *(trt__GetOSDOptionsResponse->OSDOptions->Type) = tt__OSDType__Text;

    trt__GetOSDOptionsResponse->OSDOptions->__sizePositionOption = 5;
    trt__GetOSDOptionsResponse->OSDOptions->PositionOption =  (char **)soap_malloc(
        soap, 
        sizeof(char *) * trt__GetOSDOptionsResponse->OSDOptions->__sizePositionOption
    );
    trt__GetOSDOptionsResponse->OSDOptions->PositionOption[0] = (char *)soap_strdup(soap, ONVIF_TT_POSITION_UPPER_LEFT);
    trt__GetOSDOptionsResponse->OSDOptions->PositionOption[1] = (char *)soap_strdup(soap, ONVIF_TT_POSITION_UPPER_RIGHT);
    trt__GetOSDOptionsResponse->OSDOptions->PositionOption[2] = (char *)soap_strdup(soap, ONVIF_TT_POSITION_LOWER_LEFT);
    trt__GetOSDOptionsResponse->OSDOptions->PositionOption[3] = (char *)soap_strdup(soap, ONVIF_TT_POSITION_LOWER_RIGHT);
    trt__GetOSDOptionsResponse->OSDOptions->PositionOption[4] = (char *)soap_strdup(soap, ONVIF_TT_POSITION_CUSTOM);
    
    trt__GetOSDOptionsResponse->OSDOptions->TextOption = soap_new_tt__OSDTextOptions(soap,-1);

    trt__GetOSDOptionsResponse->OSDOptions->TextOption->__sizeType = ONVIF_OSD_TYPE_NUM;
    trt__GetOSDOptionsResponse->OSDOptions->TextOption->Type  =  (char **)soap_malloc(soap, sizeof(char *));
    trt__GetOSDOptionsResponse->OSDOptions->TextOption->Type[0] = soap_strdup(soap, ONVIF_TT_TYPE_PLAIN);
    trt__GetOSDOptionsResponse->OSDOptions->TextOption->Type[1] = soap_strdup(soap, ONVIF_TT_TYPE_DATE);
    trt__GetOSDOptionsResponse->OSDOptions->TextOption->Type[2] = soap_strdup(soap, ONVIF_TT_TYPE_TIME);
    trt__GetOSDOptionsResponse->OSDOptions->TextOption->Type[3] = soap_strdup(soap, ONVIF_TT_TYPE_DATEANDTIME);

    trt__GetOSDOptionsResponse->OSDOptions->TextOption->FontSizeRange = soap_new_tt__IntRange(soap,-1);
    trt__GetOSDOptionsResponse->OSDOptions->TextOption->FontSizeRange->Min = ONVIF_OSD_FONT_MIN;
    trt__GetOSDOptionsResponse->OSDOptions->TextOption->FontSizeRange->Max = ONVIF_OSD_FONT_MAX;

    trt__GetOSDOptionsResponse->OSDOptions->TextOption->__sizeDateFormat = ONVIF_OSD_DATEFORMAT_TYPE_NUM;
    trt__GetOSDOptionsResponse->OSDOptions->TextOption->DateFormat  =  (char **)soap_malloc(soap, sizeof(char *));
    trt__GetOSDOptionsResponse->OSDOptions->TextOption->DateFormat[0] = soap_strdup(soap, ONVIF_TT_DATE_FORMAT_MM_DD_YYYY);
    trt__GetOSDOptionsResponse->OSDOptions->TextOption->DateFormat[1] = soap_strdup(soap, ONVIF_TT_DATE_FORMAT_DD_MM_YYYY);
    trt__GetOSDOptionsResponse->OSDOptions->TextOption->DateFormat[2] = soap_strdup(soap, ONVIF_TT_DATE_FORMAT_YYYY_MM_DD_SLASH);
    trt__GetOSDOptionsResponse->OSDOptions->TextOption->DateFormat[3] = soap_strdup(soap, ONVIF_TT_DATE_FORMAT_YYYY_MM_DD_DASH);

    trt__GetOSDOptionsResponse->OSDOptions->TextOption->__sizeTimeFormat = ONVIF_OSD_TIMEFORMAT_TYPE_NUM;
    trt__GetOSDOptionsResponse->OSDOptions->TextOption->TimeFormat  =  (char **)soap_malloc(soap, sizeof(char *));
    trt__GetOSDOptionsResponse->OSDOptions->TextOption->TimeFormat[0] = soap_strdup(soap, ONVIF_TT_TIME_FORMAT_HH_MM_SS_12H);
    trt__GetOSDOptionsResponse->OSDOptions->TextOption->TimeFormat[1] = soap_strdup(soap, ONVIF_TT_TIME_FORMAT_HH_MM_SS_24H);
    
    trt__GetOSDOptionsResponse->OSDOptions->TextOption->FontColor = soap_new_tt__OSDColorOptions(soap,-1);
    trt__GetOSDOptionsResponse->OSDOptions->TextOption->FontColor->Color = soap_new_tt__ColorOptions(soap,-1);
    trt__GetOSDOptionsResponse->OSDOptions->TextOption->FontColor->Color->__sizeColorspaceRange = 1;
    trt__GetOSDOptionsResponse->OSDOptions->TextOption->FontColor->Color->ColorspaceRange = soap_new_tt__ColorspaceRange(soap,-1);
    trt__GetOSDOptionsResponse->OSDOptions->TextOption->FontColor->Color->ColorspaceRange->X = soap_new_tt__FloatRange(soap,-1);
    trt__GetOSDOptionsResponse->OSDOptions->TextOption->FontColor->Color->ColorspaceRange->X->Min = ONVIF_OSD_COLOR_MIN;
    trt__GetOSDOptionsResponse->OSDOptions->TextOption->FontColor->Color->ColorspaceRange->X->Max = ONVIF_OSD_COLOR_MAX;
    trt__GetOSDOptionsResponse->OSDOptions->TextOption->FontColor->Color->ColorspaceRange->Y = soap_new_tt__FloatRange(soap,-1);
    trt__GetOSDOptionsResponse->OSDOptions->TextOption->FontColor->Color->ColorspaceRange->Y->Min = ONVIF_OSD_COLOR_MIN;
    trt__GetOSDOptionsResponse->OSDOptions->TextOption->FontColor->Color->ColorspaceRange->Y->Max = ONVIF_OSD_COLOR_MAX;
    trt__GetOSDOptionsResponse->OSDOptions->TextOption->FontColor->Color->ColorspaceRange->Z = soap_new_tt__FloatRange(soap,-1);
    trt__GetOSDOptionsResponse->OSDOptions->TextOption->FontColor->Color->ColorspaceRange->Z->Min = ONVIF_OSD_COLOR_MIN;
    trt__GetOSDOptionsResponse->OSDOptions->TextOption->FontColor->Color->ColorspaceRange->Z->Max = ONVIF_OSD_COLOR_MAX;
    trt__GetOSDOptionsResponse->OSDOptions->TextOption->FontColor->Color->ColorspaceRange->Colorspace =  soap_strdup(soap, ONVIF_TT_OSD_COLORSPACE);

    dlog_debug("----------__trt__GetOSDOptions---end-------");
    return SOAP_OK;
}

/** Web service operation '__trt__SetOSD' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__SetOSD(struct soap* soap, struct _trt__SetOSD *trt__SetOSD, struct _trt__SetOSDResponse *trt__SetOSDResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__SetOSD----------");
#endif
     int nRet = onvif_authentication(soap);
    if(SOAP_OK != nRet)
    {
        return nRet;
    }
    if(trt__SetOSD == NULL || trt__SetOSD->OSD == NULL|| trt__SetOSD->OSD->token == NULL)
    {
        dlog_error("trt__SetOSD is NULL");
        return SOAP_EOF;
    }

    OnvifOsdCfg_t stOnvifOsdCfgs;
    memset(&stOnvifOsdCfgs, 0, sizeof(stOnvifOsdCfgs));

    if(trt__SetOSD->OSD->Position == NULL)
    {
        dlog_error("trt__SetOSD Position is NULL");
        return SOAP_EOF;
    }

    if(trt__SetOSD->OSD->Type == tt__OSDType__Text)
    {
        memset(stOnvifOsdCfgs.stONvifPos.achTpye, 0, sizeof(stOnvifOsdCfgs.stONvifPos.achTpye));
        memcpy(stOnvifOsdCfgs.stONvifPos.achTpye, trt__SetOSD->OSD->Position->Type, strlen(trt__SetOSD->OSD->Position->Type));
        
        if(strcmp(trt__SetOSD->OSD->Position->Type, ONVIF_TT_POSITION_CUSTOM) == 0)
        {
            if(trt__SetOSD->OSD->Position->Pos != NULL)
            {
                stOnvifOsdCfgs.stONvifPos.x =  *(trt__SetOSD->OSD->Position->Pos->x);
                stOnvifOsdCfgs.stONvifPos.y =  *(trt__SetOSD->OSD->Position->Pos->y);
            }
        }

        if(strcmp(trt__SetOSD->OSD->TextString->Type, "Date") == 0)
        {
            memset(stOnvifOsdCfgs.TextString_DateFormat, 0, sizeof(stOnvifOsdCfgs.TextString_DateFormat));
            memcpy(stOnvifOsdCfgs.TextString_DateFormat, trt__SetOSD->OSD->TextString->DateFormat, strlen(trt__SetOSD->OSD->TextString->DateFormat));

        }
        else if(strcmp(trt__SetOSD->OSD->TextString->Type, "Time") == 0)
        {
            memset(stOnvifOsdCfgs.TextString_TimeFormat, 0, sizeof(stOnvifOsdCfgs.TextString_TimeFormat));
            memcpy(stOnvifOsdCfgs.TextString_TimeFormat, trt__SetOSD->OSD->TextString->TimeFormat, strlen(trt__SetOSD->OSD->TextString->TimeFormat));

        }
        else if(strcmp(trt__SetOSD->OSD->TextString->Type, "DateAndTime") == 0)
        {
            memset(stOnvifOsdCfgs.TextString_DateFormat, 0, sizeof(stOnvifOsdCfgs.TextString_DateFormat));
            memcpy(stOnvifOsdCfgs.TextString_DateFormat, trt__SetOSD->OSD->TextString->DateFormat, strlen(trt__SetOSD->OSD->TextString->DateFormat));

            memset(stOnvifOsdCfgs.TextString_TimeFormat, 0, sizeof(stOnvifOsdCfgs.TextString_TimeFormat));
            memcpy(stOnvifOsdCfgs.TextString_TimeFormat, trt__SetOSD->OSD->TextString->TimeFormat, strlen(trt__SetOSD->OSD->TextString->TimeFormat));
        }
        else if(strcmp(trt__SetOSD->OSD->TextString->Type, "Plain") == 0)
        {
            memset(stOnvifOsdCfgs.TextString_PlainText, '\0', strlen( stOnvifOsdCfgs.TextString_PlainText)+1);
            memcpy(stOnvifOsdCfgs.TextString_PlainText, trt__SetOSD->OSD->TextString->PlainText, strlen(trt__SetOSD->OSD->TextString->PlainText));
        }
        else
        {}

        nRet = onvif_set_osdParam(&stOnvifOsdCfgs,trt__SetOSD->OSD->token);
        if(nRet < 0)
        {
            dlog_error("trt__SetOSD 设置失败");
            return SOAP_EOF;
        }
    }
    else
    {
        dlog_error("trt__SetOSD 没有对应的type类型");
        return SOAP_EOF;
    }

     dlog_debug("trt__SetOSD 设置成功");
    return SOAP_OK;
}

/** Web service operation '__trt__CreateOSD' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__CreateOSD(struct soap* soap, struct _trt__CreateOSD *trt__CreateOSD, struct _trt__CreateOSDResponse *trt__CreateOSDResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__CreateOSD----------");
#endif

     int nRet = onvif_authentication(soap);
    if(SOAP_OK != nRet)
    {
        return nRet;
    }
    if(trt__CreateOSD == NULL || trt__CreateOSD->OSD == NULL|| trt__CreateOSD->OSD->token == NULL)
    {
        dlog_error("trt__CreateOSD is NULL");
        return SOAP_EOF;
    }
     dlog_debug("----------__trt__CreateOSD----[%s]------",trt__CreateOSD->OSD->token);
    OnvifOsdCfg_t stOnvifOsdCfgs;
    memset(&stOnvifOsdCfgs, 0, sizeof(stOnvifOsdCfgs));

    if(trt__CreateOSD->OSD->Type == tt__OSDType__Text)
    {
        memset(stOnvifOsdCfgs.stONvifPos.achTpye, 0, sizeof(stOnvifOsdCfgs.stONvifPos.achTpye));
        memcpy(stOnvifOsdCfgs.stONvifPos.achTpye, trt__CreateOSD->OSD->Position->Type, strlen(trt__CreateOSD->OSD->Position->Type));
        
        if(strcmp(trt__CreateOSD->OSD->Position->Type, ONVIF_TT_POSITION_CUSTOM) == 0)
        {
            if(trt__CreateOSD->OSD->Position->Pos != NULL)
            {
                stOnvifOsdCfgs.stONvifPos.x =  *(trt__CreateOSD->OSD->Position->Pos->x);
                stOnvifOsdCfgs.stONvifPos.y =  *(trt__CreateOSD->OSD->Position->Pos->y);
            }
        }

        if(strcmp(trt__CreateOSD->OSD->TextString->Type, "Date") == 0 || strcmp(trt__CreateOSD->OSD->TextString->Type, "Time") == 0
           || strcmp(trt__CreateOSD->OSD->TextString->Type, "DateAndTime") == 0)
        {
            stOnvifOsdCfgs.eTextType = E_OSDTYPE_TEXT_TIME;
        }
        else if(strcmp(trt__CreateOSD->OSD->TextString->Type, "Plain") == 0)
        {
            stOnvifOsdCfgs.eTextType = E_OSDTYPE_TEXT_EXTEND;
            /* 判断是否是通道名称 */
            if(trt__CreateOSD->OSD->TextString->Extension != NULL && trt__CreateOSD->OSD->TextString->Extension->__any != NULL)
            {
                if((strstr(trt__CreateOSD->OSD->TextString->Extension->__any[0], "ChannelName") != NULL) && (strstr(trt__CreateOSD->OSD->TextString->Extension->__any[0], "true") != NULL))
                {
                    dlog_debug("trt__CreateOSD token [%s] is ChannelName",trt__CreateOSD->OSD->token);
                    stOnvifOsdCfgs.eTextType = E_OSDTYPE_TEXT_NAME;
                }
            }
        }
        else
        {
            dlog_error("trt__CreateOSD->OSD->TextString->Type is error");
            return SOAP_EOF;
        }


        if(strcmp(trt__CreateOSD->OSD->TextString->Type, "Date") == 0)
        {
            memset(stOnvifOsdCfgs.TextString_DateFormat, 0, sizeof(stOnvifOsdCfgs.TextString_DateFormat));
            memcpy(stOnvifOsdCfgs.TextString_DateFormat, trt__CreateOSD->OSD->TextString->DateFormat, strlen(trt__CreateOSD->OSD->TextString->DateFormat));
            stOnvifOsdCfgs.bOsdEnable = true;
        }
        else if(strcmp(trt__CreateOSD->OSD->TextString->Type, "Time") == 0)
        {
            memset(stOnvifOsdCfgs.TextString_TimeFormat, 0, sizeof(stOnvifOsdCfgs.TextString_TimeFormat));
            memcpy(stOnvifOsdCfgs.TextString_TimeFormat, trt__CreateOSD->OSD->TextString->TimeFormat, strlen(trt__CreateOSD->OSD->TextString->TimeFormat));
            stOnvifOsdCfgs.bOsdEnable = true;
        }
        else if(strcmp(trt__CreateOSD->OSD->TextString->Type, "DateAndTime") == 0)
        {
            memset(stOnvifOsdCfgs.TextString_DateFormat, 0, sizeof(stOnvifOsdCfgs.TextString_DateFormat));
            memcpy(stOnvifOsdCfgs.TextString_DateFormat, trt__CreateOSD->OSD->TextString->DateFormat, strlen(trt__CreateOSD->OSD->TextString->DateFormat));

            memset(stOnvifOsdCfgs.TextString_TimeFormat, 0, sizeof(stOnvifOsdCfgs.TextString_TimeFormat));
            memcpy(stOnvifOsdCfgs.TextString_TimeFormat, trt__CreateOSD->OSD->TextString->TimeFormat, strlen(trt__CreateOSD->OSD->TextString->TimeFormat));

            stOnvifOsdCfgs.bOsdEnable = true;

        }
        else if(strcmp(trt__CreateOSD->OSD->TextString->Type, "Plain") == 0)
        {
            memset(stOnvifOsdCfgs.TextString_PlainText, '\0', strlen( stOnvifOsdCfgs.TextString_PlainText)+1);
            memcpy(stOnvifOsdCfgs.TextString_PlainText, trt__CreateOSD->OSD->TextString->PlainText, strlen(trt__CreateOSD->OSD->TextString->PlainText));
            stOnvifOsdCfgs.bOsdEnable = true;
        }
        else
        {
            dlog_error("trt__CreateOSD->OSD->TextString->Type is error");
            return SOAP_EOF;
        }

        int nRet = onvif_create_osd(&stOnvifOsdCfgs,trt__CreateOSD->OSD->token);
        if(nRet < 0)
        {
            dlog_error("trt__CreateOSD is error");
            return SOAP_EOF;
        }
        else
        {
            if(trt__CreateOSDResponse == NULL)
            {
                trt__CreateOSDResponse = soap_new__trt__CreateOSDResponse(soap,-1);
            }
            trt__CreateOSDResponse->OSDToken = soap_strdup(soap, stOnvifOsdCfgs.token);
            dlog_debug("trt__CreateOSD is ok 返回token[%s]",trt__CreateOSDResponse->OSDToken);
            return SOAP_OK;
        }
    }

    dlog_debug("trt__CreateOSD is ok");  
    return SOAP_OK;
}

/** Web service operation '__trt__DeleteOSD' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trt__DeleteOSD(struct soap* soap, struct _trt__DeleteOSD *trt__DeleteOSD, struct _trt__DeleteOSDResponse *trt__DeleteOSDResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trt__DeleteOSD----------");
#endif
     int nRet = onvif_authentication(soap);
    if(SOAP_OK != nRet)
    {
        return nRet;
    }
    if(trt__DeleteOSD == NULL && trt__DeleteOSD->OSDToken == NULL)
    {
        dlog_error("trt__DeleteOSD->OSDToken is NULL");
        return SOAP_EOF;
    }
     dlog_debug("----------__trt__DeleteOSD-------%s---",trt__DeleteOSD->OSDToken);
    nRet = onvif_delete_osd(trt__DeleteOSD->OSDToken);
    return nRet;
}