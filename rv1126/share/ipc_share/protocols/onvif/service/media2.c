/**
 * @file media2.c
 * @author tianl (tianl@kfb.cn)
 * @date 2025-09-23
 * 
 * @brief onvif media2服务接口
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

/** Web service operation '__ns1__GetServiceCapabilities' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__GetServiceCapabilities(struct soap* soap, struct _ns1__GetServiceCapabilities *ns1__GetServiceCapabilities, struct _ns1__GetServiceCapabilitiesResponse *ns1__GetServiceCapabilitiesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__GetServiceCapabilities----------");
#endif
    soap_wsse_delete_Security(soap);
    return SOAP_OK;
}
/** Web service operation '__ns1__CreateProfile' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__CreateProfile(struct soap* soap, struct _ns1__CreateProfile *ns1__CreateProfile, struct _ns1__CreateProfileResponse *ns1__CreateProfileResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__CreateProfile----------");
#endif
    return SOAP_OK;
}
/** Web service operation '__ns1__GetProfiles' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__GetProfiles(struct soap*soap , struct _ns1__GetProfiles *ns1__GetProfiles, struct _ns1__GetProfilesResponse *ns1__GetProfilesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("---------- __ns1__GetProfiles (Media2) with Analytics ----------");
#endif

    if (!soap || !ns1__GetProfiles || !ns1__GetProfilesResponse) 
    {
        return SOAP_FAULT;
    }

    int nRet = onvif_authentication(soap);
    if (SOAP_OK != nRet) 
    {
        return nRet;
    }

    enum xsd__boolean include_all_types = xsd__boolean__true_;
    enum xsd__boolean include_videosource = xsd__boolean__false_;
    enum xsd__boolean include_videoencoder = xsd__boolean__false_;
    enum xsd__boolean include_analytics = xsd__boolean__false_;

    if (ns1__GetProfiles->Type)
    {
        include_all_types = xsd__boolean__false_; 
        for (int i = 0; i < ns1__GetProfiles->__sizeType; i++)
        {
            if (strcmp(ns1__GetProfiles->Type[0], "All") == 0) 
            {
                include_all_types = xsd__boolean__true_;
                include_videosource = xsd__boolean__true_;
                include_videoencoder = xsd__boolean__true_;
                include_analytics = xsd__boolean__true_;
                    break; 
            }
            else if (strcmp(ns1__GetProfiles->Type[0], "VideoSource") == 0) 
            {
                include_videosource = xsd__boolean__true_;
            } 
            else if (strcmp(ns1__GetProfiles->Type[0], "VideoEncoder") == 0) 
            {
                include_videoencoder = xsd__boolean__true_;
            } 
            else if (strcmp(ns1__GetProfiles->Type[0], "Analytics") == 0) 
            {
                include_analytics = xsd__boolean__true_;
            }
        }
    }

    int num_profiles_to_return = 0;
    if (ns1__GetProfiles->Token)
    {
        dlog_info("ns1__GetProfiles->Token [%s]",ns1__GetProfiles->Token);
        if (strcmp(ns1__GetProfiles->Token, PROFILE1_TOKEN) == 0) 
        {
            num_profiles_to_return = 1;
        } 
        else if (strcmp(ns1__GetProfiles->Token, PROFILE2_TOKEN) == 0) 
        {
            num_profiles_to_return = 1;
        } 
        else 
        {
            num_profiles_to_return = 2;
        }
    }
    else
    {
#if ONVIF_LOG_SWITCH
        dlog_info("Requesting all profiles (Token not specified)");
#endif
        num_profiles_to_return = 2;
    }


    if (num_profiles_to_return > 0) 
    {
       ns1__GetProfilesResponse->Profiles = soap_new_ns1__MediaProfile(soap, num_profiles_to_return);
    } 
    else 
    {
       ns1__GetProfilesResponse->Profiles = NULL;
    }
    
     ns1__GetProfilesResponse->__sizeProfiles = num_profiles_to_return;
    /* Profile */ 
    for (int k = 0; k < num_profiles_to_return; k++)
    {

       struct ns1__MediaProfile *pProfile = ns1__GetProfilesResponse->Profiles + k;
       if(pProfile == NULL)
       {
            continue;
       }
       OnvifProfile_t stProfileData;
       
       get_profile_param(k, &stProfileData);
       
       /* 基本信息 */ 
       pProfile->token = soap_strdup(soap, stProfileData.token);
       pProfile->Name = soap_strdup(soap, stProfileData.Name);
       pProfile->fixed = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
       *(pProfile->fixed) = xsd__boolean__true_;

       /* Configurations */ 
       pProfile->Configurations = soap_new_ns1__ConfigurationSet(soap, -1);
       soap_default_ns1__ConfigurationSet(soap, pProfile->Configurations);
       
       /* Video Source Configuration */
       if (include_all_types && include_videosource)
       {
            pProfile->Configurations->VideoSource = soap_new_tt__VideoSourceConfiguration(soap, -1);
            pProfile->Configurations->VideoSource->token = soap_strdup(soap, stProfileData.VideoSourceConfiguration_token);
            pProfile->Configurations->VideoSource->Name = soap_strdup(soap, stProfileData.VideoSourceConfiguration_Name);
            pProfile->Configurations->VideoSource->SourceToken = soap_strdup(soap, stProfileData.VideoSourceConfiguration_SourceToken);
            pProfile->Configurations->VideoSource->UseCount = 2; 
            pProfile->Configurations->VideoSource->Bounds = soap_new_tt__IntRectangle(soap, -1);
            pProfile->Configurations->VideoSource->Bounds->x = 0;
            pProfile->Configurations->VideoSource->Bounds->y = 0;
            pProfile->Configurations->VideoSource->Bounds->width = 2880;
            pProfile->Configurations->VideoSource->Bounds->height = 1620;
       }

       /* Video Encoder Configuration -- */ 
       if (include_all_types || include_videoencoder)
       {
           pProfile->Configurations->VideoEncoder = soap_new_tt__VideoEncoder2Configuration(soap, -1);
           pProfile->Configurations->VideoEncoder->token = soap_strdup(soap, stProfileData.VideoEncoderConfiguration_token);
           pProfile->Configurations->VideoEncoder->Name = soap_strdup(soap, stProfileData.VideoEncoderConfiguration_Name);
           pProfile->Configurations->VideoEncoder->UseCount = 2;
           pProfile->Configurations->VideoEncoder->Encoding = soap_strdup(soap, stProfileData.VideoEncoderConfiguration_Encoding);
           pProfile->Configurations->VideoEncoder->Quality = stProfileData.Quality;
           pProfile->Configurations->VideoEncoder->GovLength = soap_new_int(soap, -1);
           *(pProfile->Configurations->VideoEncoder->GovLength) = stProfileData.IFrameInterval;
           pProfile->Configurations->VideoEncoder->Profile = soap_strdup(soap, "Main");
           pProfile->Configurations->VideoEncoder->Resolution = soap_new_tt__VideoResolution2(soap, -1);
           pProfile->Configurations->VideoEncoder->Resolution->Width = stProfileData.nWidth;
           pProfile->Configurations->VideoEncoder->Resolution->Height = stProfileData.nHeight;
           pProfile->Configurations->VideoEncoder->RateControl = soap_new_tt__VideoRateControl2(soap, -1);
           pProfile->Configurations->VideoEncoder->RateControl->FrameRateLimit = stProfileData.FrameRateLimit;
           pProfile->Configurations->VideoEncoder->RateControl->BitrateLimit = stProfileData.BitrateLimit;
           pProfile->Configurations->VideoEncoder->RateControl->ConstantBitRate = (enum xsd__boolean*)soap_malloc(soap, sizeof(enum xsd__boolean));
           *(pProfile->Configurations->VideoEncoder->RateControl->ConstantBitRate) = stProfileData.ConstantBitRate;
           pProfile->Configurations->VideoEncoder->Multicast = soap_new_tt__MulticastConfiguration(soap, -1);
           pProfile->Configurations->VideoEncoder->Multicast->Address = soap_new_tt__IPAddress(soap, -1);
           pProfile->Configurations->VideoEncoder->Multicast->Address->Type = tt__IPType__IPv4;
           pProfile->Configurations->VideoEncoder->Multicast->Address->IPv4Address = soap_strdup(soap, "0.0.0.0");
           pProfile->Configurations->VideoEncoder->Multicast->Port = (k == 0) ? 8860 : 8866;
           pProfile->Configurations->VideoEncoder->Multicast->TTL = 128;
           pProfile->Configurations->VideoEncoder->Multicast->AutoStart = xsd__boolean__false_;
       }

        /* Video Analytics Configuration -- */ 
        if (include_all_types || include_analytics)
        {
            pProfile->Configurations->Analytics = soap_new_tt__VideoAnalyticsConfiguration(soap, -1); 
            pProfile->Configurations->Analytics->UseCount = 2;
            pProfile->Configurations->Analytics->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN_NAME);
            pProfile->Configurations->Analytics->token = soap_strdup(soap, VIDEOANALTICS_TOKEN);

            /* 分析模块初始化 */
            if(pProfile->Configurations->Analytics->AnalyticsEngineConfiguration == NULL)
            {
                pProfile->Configurations->Analytics->AnalyticsEngineConfiguration = soap_new_tt__AnalyticsEngineConfiguration(soap,-1);
            }
            pProfile->Configurations->Analytics->AnalyticsEngineConfiguration->__sizeAnalyticsModule = ONVIF_ANALYTICS_SUPPORT_NUM;
            pProfile->Configurations->Analytics->AnalyticsEngineConfiguration->AnalyticsModule = soap_new_tt__Config(soap,ONVIF_ANALYTICS_SUPPORT_NUM);
            
            /*  规则初始化 */
            if(pProfile->Configurations->Analytics->RuleEngineConfiguration == NULL)
            {
                pProfile->Configurations->Analytics->RuleEngineConfiguration = soap_new_tt__RuleEngineConfiguration(soap,-1);
            }
            pProfile->Configurations->Analytics->RuleEngineConfiguration->__sizeRule = ONVIF_ANALYTICS_RULE_SUPPORT_NUM;
            pProfile->Configurations->Analytics->RuleEngineConfiguration->Rule = soap_new_tt__Config(soap,ONVIF_ANALYTICS_RULE_SUPPORT_NUM);

            /* 分析模块和规则获取 */
            for(int i  = 0; i < ONVIF_ANALYTICS_SUPPORT_NUM;i++)
            {
                struct tt__Config *pAnalyticsModule = pProfile->Configurations->Analytics->AnalyticsEngineConfiguration->AnalyticsModule + i;
                
                struct tt__Config *pRule;
                if( i < ONVIF_ANALYTICS_RULE_SUPPORT_NUM)
                {
                    pRule = pProfile->Configurations->Analytics->RuleEngineConfiguration->Rule + i;
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
        }

   }

   return SOAP_OK;
}

/** Web service operation '__ns1__AddConfiguration' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__AddConfiguration(struct soap* soap, struct _ns1__AddConfiguration *ns1__AddConfiguration, struct _ns1__AddConfigurationResponse *ns1__AddConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__AddConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__ns1__RemoveConfiguration' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__RemoveConfiguration(struct soap* soap, struct _ns1__RemoveConfiguration *ns1__RemoveConfiguration, struct _ns1__RemoveConfigurationResponse *ns1__RemoveConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__RemoveConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__ns1__DeleteProfile' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__DeleteProfile(struct soap* soap, struct _ns1__DeleteProfile *ns1__DeleteProfile, struct _ns1__DeleteProfileResponse *ns1__DeleteProfileResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__DeleteProfile----------");
#endif
    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __ns1__GetAudioClips(struct soap* soap, struct _ns1__GetAudioClips *ns1__GetAudioClips, struct _ns1__GetAudioClipsResponse *ns1__GetAudioClipsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__GetAudioClips----------");
#endif
    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __ns1__AddAudioClip(struct soap* soap, struct _ns1__AddAudioClip *ns1__AddAudioClip, struct _ns1__AddAudioClipResponse *ns1__AddAudioClipResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__AddAudioClip----------");
#endif
    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __ns1__GetPlayingAudioClips(struct soap* soap, struct _ns1__GetPlayingAudioClips *ns1__GetPlayingAudioClips, struct _ns1__GetPlayingAudioClipsResponse *ns1__GetPlayingAudioClipsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__GetPlayingAudioClips----------");
#endif
    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __ns1__DeleteAudioClip(struct soap* soap, struct _ns1__DeleteAudioClip *ns1__DeleteAudioClip, struct _ns1__DeleteAudioClipResponse *ns1__DeleteAudioClipResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__DeleteAudioClip----------");
#endif
    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __ns1__SetEQPreset(struct soap* soap, struct _ns1__SetEQPresetConfiguration *ns1__SetEQPresetConfiguration, struct ns1__SetConfigurationResponse *ns1__SetEQPresetConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__SetEQPreset----------");
#endif
    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __ns1__PlayAudioClip(struct soap* soap, struct _ns1__PlayAudioClip *ns1__PlayAudioClip, struct _ns1__PlayAudioClipResponse *ns1__PlayAudioClipResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__PlayAudioClip----------");
#endif
    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __ns1__SetAudioClip(struct soap* soap, struct _ns1__SetAudioClip *ns1__SetAudioClip, struct _ns1__SetAudioClipResponse *ns1__SetAudioClipResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__SetAudioClip----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__ns1__GetVideoSourceConfigurations' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__GetVideoSourceConfigurations(struct soap* soap, struct ns1__GetConfiguration *ns1__GetVideoSourceConfigurations, struct _ns1__GetVideoSourceConfigurationsResponse *ns1__GetVideoSourceConfigurationsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__GetVideoSourceConfigurations----------");
#endif
    int nRet = onvif_authentication(soap);
    if(SOAP_OK != nRet)
    {
        return nRet;
    }
    if( ns1__GetVideoSourceConfigurationsResponse == NULL)
    {
        dlog_error("ns1__GetVideoSourceConfigurationsResponse is NULL");
        return SOAP_EOF;
    }
    ns1__GetVideoSourceConfigurationsResponse->__sizeConfigurations = 1;
    ns1__GetVideoSourceConfigurationsResponse->Configurations = soap_new_tt__VideoSourceConfiguration(soap,-1);

    ns1__GetVideoSourceConfigurationsResponse->Configurations->UseCount = ONVIF_MEDIA_PROFILE_NUM;

    ns1__GetVideoSourceConfigurationsResponse->Configurations->Name = soap_strdup(soap, VIDEOSOURCE_NAME);
   
    ns1__GetVideoSourceConfigurationsResponse->Configurations->token = soap_strdup(soap, VIDEOSOURCE_TOKEN);
    ns1__GetVideoSourceConfigurationsResponse->Configurations->SourceToken = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
   

    ns1__GetVideoSourceConfigurationsResponse->Configurations->Bounds = soap_new_tt__IntRectangle(soap,-1);
    ns1__GetVideoSourceConfigurationsResponse->Configurations->Bounds->x      = 0;
    ns1__GetVideoSourceConfigurationsResponse->Configurations->Bounds->y      = 0;
    ns1__GetVideoSourceConfigurationsResponse->Configurations->Bounds->width  = ONVIF_BOUNDS_WIDTH;
    ns1__GetVideoSourceConfigurationsResponse->Configurations->Bounds->height = ONVIF_BOUNDS_HEIGHT;
    
    return SOAP_OK;
}
/** Web service operation '__ns1__GetVideoEncoderConfigurations' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__GetVideoEncoderConfigurations(struct soap* soap, struct ns1__GetConfiguration *ns1__GetVideoEncoderConfigurations, struct _ns1__GetVideoEncoderConfigurationsResponse *ns1__GetVideoEncoderConfigurationsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__GetVideoEncoderConfigurations----------");
#endif
    if(!ns1__GetVideoEncoderConfigurations)
    {
        dlog_error("ns1__GetVideoEncoderConfigurations is NULL");
        return soap_sender_fault(soap,  "ns1__GetVideoEncoderConfigurations is NULL",  NULL);
    }

    if(!ns1__GetVideoEncoderConfigurationsResponse)
    {
        dlog_error("ns1__GetVideoEncoderConfigurationsResponse is NULL");
        return soap_sender_fault(soap,  "ns1__GetVideoEncoderConfigurationsResponse is NULL",  NULL);
    }

    int nIndex = -1;
    if(ns1__GetVideoEncoderConfigurations->ProfileToken)
    {
        if(strcmp(ns1__GetVideoEncoderConfigurations->ProfileToken, PROFILE1_TOKEN) == 0)
        {
            ns1__GetVideoEncoderConfigurationsResponse->__sizeConfigurations = 1;
            nIndex = 0;
        }
        else if(strcmp(ns1__GetVideoEncoderConfigurations->ProfileToken, PROFILE2_TOKEN) == 0)
        {
            ns1__GetVideoEncoderConfigurationsResponse->__sizeConfigurations = 1;  
            nIndex = 1;  
        }
        else
        {
            dlog_error("ProfileToken is Invalid");
            return soap_sender_fault(soap,  "ProfileToken is Invalid",  NULL);
        }
    }
    else if(ns1__GetVideoEncoderConfigurations->ConfigurationToken)
    {
        if(strcmp(ns1__GetVideoEncoderConfigurations->ConfigurationToken, PROFILE1_VIDEOENCODER_TOKEN) == 0)
        {
            ns1__GetVideoEncoderConfigurationsResponse->__sizeConfigurations = 1;
            nIndex = 0;
        }
        else if(strcmp(ns1__GetVideoEncoderConfigurations->ConfigurationToken, PROFILE2_VIDEOENCODER_TOKEN) == 0)
        {
            ns1__GetVideoEncoderConfigurationsResponse->__sizeConfigurations = 1;  
            nIndex = 1;  
        }
        else
        {
            dlog_error("ConfigurationToken is Invalid");
            return soap_sender_fault(soap,  "ConfigurationToken is Invalid",  NULL);
        }
    }
    else
    {
        ns1__GetVideoEncoderConfigurationsResponse->__sizeConfigurations = 2;
        nIndex = -1;
    }

    OnvifProfile_t stProfile[ns1__GetVideoEncoderConfigurationsResponse->__sizeConfigurations];

    memset(&stProfile, 0, sizeof(OnvifProfile_t)*ns1__GetVideoEncoderConfigurationsResponse->__sizeConfigurations);

    if(ns1__GetVideoEncoderConfigurationsResponse->__sizeConfigurations == 1)
    {
        get_profile_param(nIndex, &stProfile[0]);
    }
    else
    {
        for(int i = 0; i < ns1__GetVideoEncoderConfigurationsResponse->__sizeConfigurations; i++)
        {
            get_profile_param(i, &stProfile[i]);
        }
    }

    ns1__GetVideoEncoderConfigurationsResponse->Configurations = (struct tt__VideoEncoder2Configuration *)soap_malloc(soap, sizeof(struct tt__VideoEncoder2Configuration)*ns1__GetVideoEncoderConfigurationsResponse->__sizeConfigurations); 
    memset(ns1__GetVideoEncoderConfigurationsResponse->Configurations, 0, sizeof(struct tt__VideoEncoder2Configuration)*ns1__GetVideoEncoderConfigurationsResponse->__sizeConfigurations);
    if(ns1__GetVideoEncoderConfigurationsResponse->Configurations == NULL)
    {
        dlog_error("Failed to allocate Configurations");
        return soap_receiver_fault(soap, "Memory allocation Configurations failed", NULL);
    }

    for(int i = 0; i < ns1__GetVideoEncoderConfigurationsResponse->__sizeConfigurations; i++)
    {
        ns1__GetVideoEncoderConfigurationsResponse->Configurations[i].Name = soap_strdup(soap, stProfile[i].Name);
        if (!ns1__GetVideoEncoderConfigurationsResponse->Configurations[i].Name) 
        {
            dlog_error("Failed to duplicate ProfileName string");
            return soap_receiver_fault(soap, "Failed to duplicate ProfileName string", NULL);
        }

        ns1__GetVideoEncoderConfigurationsResponse->Configurations[i].UseCount = 1;

        ns1__GetVideoEncoderConfigurationsResponse->Configurations[i].token = soap_strdup(soap, stProfile[i].VideoEncoderConfiguration_token);
        if (!ns1__GetVideoEncoderConfigurationsResponse->Configurations[i].token) 
        {
            dlog_error("Failed to duplicate Profiletoken string");
            return soap_receiver_fault(soap, "Failed to duplicate Profiletoken string", NULL);
        }

        OnvifVideoParam_t stActualParam;
        memset(&stActualParam, 0, sizeof(stActualParam));
        int streamIdx = (ns1__GetVideoEncoderConfigurationsResponse->__sizeConfigurations == 1) ? nIndex : i;
        onvif_get_videoParams(&stActualParam, streamIdx);

        ns1__GetVideoEncoderConfigurationsResponse->Configurations[i].Encoding = soap_strdup(soap, stActualParam.strVideoCodec);
        if (!ns1__GetVideoEncoderConfigurationsResponse->Configurations[i].Encoding) 
        {
            dlog_error("Failed to duplicate Encoding string");
            return soap_receiver_fault(soap, "Failed to duplicate Encoding string", NULL);
        }

        ns1__GetVideoEncoderConfigurationsResponse->Configurations[i].Resolution = soap_new_tt__VideoResolution2(soap, -1);
        if (!ns1__GetVideoEncoderConfigurationsResponse->Configurations[i].Resolution) 
        {
            dlog_error("Failed to allocate Resolution");
            return soap_receiver_fault(soap, "Memory allocation Resolution failed", NULL);
        }
        soap_default_tt__VideoResolution2(soap, ns1__GetVideoEncoderConfigurationsResponse->Configurations[i].Resolution);
        ns1__GetVideoEncoderConfigurationsResponse->Configurations[i].Resolution->Width = stActualParam.nWidths[0];
        ns1__GetVideoEncoderConfigurationsResponse->Configurations[i].Resolution->Height = stActualParam.nHeights[0];

        ns1__GetVideoEncoderConfigurationsResponse->Configurations[i].RateControl = soap_new_tt__VideoRateControl2(soap, -1);
        if (!ns1__GetVideoEncoderConfigurationsResponse->Configurations[i].RateControl) 
        {
            dlog_error("Failed to allocate RateControl");
            return soap_receiver_fault(soap, "Memory allocation RateControl failed", NULL);
        }
        soap_default_tt__VideoRateControl2(soap, ns1__GetVideoEncoderConfigurationsResponse->Configurations[i].RateControl);
        ns1__GetVideoEncoderConfigurationsResponse->Configurations[i].RateControl->BitrateLimit = stActualParam.nCurBitrate;
        ns1__GetVideoEncoderConfigurationsResponse->Configurations[i].RateControl->FrameRateLimit = stActualParam.fCurFrameRate;
        ns1__GetVideoEncoderConfigurationsResponse->Configurations[i].RateControl->ConstantBitRate = soap_new_xsd__boolean(soap, -1);
        *(ns1__GetVideoEncoderConfigurationsResponse->Configurations[i].RateControl->ConstantBitRate) = stActualParam.bConstantBitRate;
        
        ns1__GetVideoEncoderConfigurationsResponse->Configurations[i].Quality = (float)stActualParam.nCurQuality;
        
        /* Custom VideoType using __any */
        ns1__GetVideoEncoderConfigurationsResponse->Configurations[i].__size = 1;
        ns1__GetVideoEncoderConfigurationsResponse->Configurations[i].__any = (char **)soap_malloc(soap, sizeof(char*));
        ns1__GetVideoEncoderConfigurationsResponse->Configurations[i].__any[0] = (char *)soap_malloc(soap, 64);
        snprintf(ns1__GetVideoEncoderConfigurationsResponse->Configurations[i].__any[0], 64, "<VideoType>%d</VideoType>", stActualParam.nVideoType);

        dlog_debug("Media2 获取视频编码配置: Token=%s, 编码=%s, 分辨率=%dx%d, 码率限制=%d, 帧率限制=%f, 定码率=%d",
            ns1__GetVideoEncoderConfigurationsResponse->Configurations[i].token,
            ns1__GetVideoEncoderConfigurationsResponse->Configurations[i].Encoding,
            ns1__GetVideoEncoderConfigurationsResponse->Configurations[i].Resolution->Width,
            ns1__GetVideoEncoderConfigurationsResponse->Configurations[i].Resolution->Height,
            ns1__GetVideoEncoderConfigurationsResponse->Configurations[i].RateControl->BitrateLimit,
            ns1__GetVideoEncoderConfigurationsResponse->Configurations[i].RateControl->FrameRateLimit,
            *(ns1__GetVideoEncoderConfigurationsResponse->Configurations[i].RateControl->ConstantBitRate));
        if(!ns1__GetVideoEncoderConfigurationsResponse->Configurations[i].RateControl->ConstantBitRate) 
        {
            dlog_error("Failed to allocate ConstantBitRateSupported");
            return soap_receiver_fault(soap, "Memory allocation ConstantBitRateSupported failed", NULL);
        }
        *(ns1__GetVideoEncoderConfigurationsResponse->Configurations[i].RateControl->ConstantBitRate) = stProfile[i].ConstantBitRate;

        ns1__GetVideoEncoderConfigurationsResponse->Configurations[i].Quality = stProfile[i].Quality;

        ns1__GetVideoEncoderConfigurationsResponse->Configurations[i].GovLength = soap_new_int(soap, -1);
        *(ns1__GetVideoEncoderConfigurationsResponse->Configurations[i].GovLength) = stProfile[i].IFrameInterval;        
    }

    return SOAP_OK;
}

/** Web service operation '__ns1__GetAudioSourceConfigurations' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__GetAudioSourceConfigurations(struct soap* soap, struct ns1__GetConfiguration *ns1__GetAudioSourceConfigurations, struct _ns1__GetAudioSourceConfigurationsResponse *ns1__GetAudioSourceConfigurationsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__GetAudioSourceConfigurations----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__ns1__GetAudioEncoderConfigurations' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__GetAudioEncoderConfigurations(struct soap* soap, struct ns1__GetConfiguration *ns1__GetAudioEncoderConfigurations, struct _ns1__GetAudioEncoderConfigurationsResponse *ns1__GetAudioEncoderConfigurationsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__GetAudioEncoderConfigurations----------");
#endif
    if(!ns1__GetAudioEncoderConfigurations || !ns1__GetAudioEncoderConfigurationsResponse)
    {
        dlog_error("ns1__GetAudioEncoderConfigurations or ns1__GetAudioEncoderConfigurationsResponse is NULL");
        return soap_sender_fault(soap,  "ns1__GetAudioEncoderConfigurations or ns1__GetAudioEncoderConfigurationsResponse is NULL",  NULL);
    }

    ns1__GetAudioEncoderConfigurationsResponse->__sizeConfigurations = 1;
    ns1__GetAudioEncoderConfigurationsResponse->Configurations = (struct tt__AudioEncoder2Configuration *)soap_malloc(soap, sizeof(struct tt__AudioEncoder2Configuration)*ns1__GetAudioEncoderConfigurationsResponse->__sizeConfigurations);
    if(!ns1__GetAudioEncoderConfigurationsResponse->Configurations)
    {
        dlog_error("Failed to allocate ConstantBitRateSupported");
        return soap_receiver_fault(soap, "Memory allocation ConstantBitRateSupported failed", NULL);
    }
    memset(ns1__GetAudioEncoderConfigurationsResponse->Configurations, 0, sizeof(struct tt__AudioEncoder2Configuration));

    OnvifAudioParam_t stAudioParams;
    int nRet = -1;
    
    for(int i = 0; i < ns1__GetAudioEncoderConfigurationsResponse->__sizeConfigurations; i++)
    {
        memset(&stAudioParams, 0, sizeof(OnvifAudioParam_t));
        nRet = onvif_get_audioParams(&stAudioParams);
        if(nRet < 0)
        {
            return soap_receiver_fault(soap, "onvif get audioParams failed", NULL);
        }
        ns1__GetAudioEncoderConfigurationsResponse->Configurations[0].Name = soap_strdup(soap, PROFILE_AUDIOENCODER_NAME);
        ns1__GetAudioEncoderConfigurationsResponse->Configurations[0].UseCount = 1;
        ns1__GetAudioEncoderConfigurationsResponse->Configurations[0].token = soap_strdup(soap, PROFILE_AUDIOENCODER_TOKEN);
        ns1__GetAudioEncoderConfigurationsResponse->Configurations[0].Encoding = soap_strdup(soap, stAudioParams.audioEncoding);
        ns1__GetAudioEncoderConfigurationsResponse->Configurations[0].Bitrate = stAudioParams.audioBitrate / 1000;           // kbps
        ns1__GetAudioEncoderConfigurationsResponse->Configurations[0].SampleRate = stAudioParams.audioSampleRate / 1000;     // kHz

    }

    return SOAP_OK;
}

/** Web service operation '__ns1__GetAnalyticsConfigurations' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__GetAnalyticsConfigurations(struct soap* soap, struct ns1__GetConfiguration *ns1__GetAnalyticsConfigurations, struct _ns1__GetAnalyticsConfigurationsResponse *ns1__GetAnalyticsConfigurationsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__GetAnalyticsConfigurations----------");
#endif

#if 1
    int nRet = onvif_authentication(soap);
    if(SOAP_OK != nRet)
    {
        return nRet;
    }
    if(ns1__GetAnalyticsConfigurationsResponse == NULL)
    {
        dlog_error("ns1__GetAnalyticsConfigurationsResponse is NULL");
        return soap_receiver_fault(soap, "ns1__GetAnalyticsConfigurationsResponse is NULL", NULL);    
    }

    ns1__GetAnalyticsConfigurationsResponse->__sizeConfigurations = 1;
    
    ns1__GetAnalyticsConfigurationsResponse->Configurations = soap_new_tt__VideoAnalyticsConfiguration(soap,-1);
    ns1__GetAnalyticsConfigurationsResponse->Configurations->UseCount = 2;
    ns1__GetAnalyticsConfigurationsResponse->Configurations->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN_NAME);
    ns1__GetAnalyticsConfigurationsResponse->Configurations->token = soap_strdup(soap, VIDEOANALTICS_TOKEN);

    /* 分析模块初始化 */
    if(ns1__GetAnalyticsConfigurationsResponse->Configurations->AnalyticsEngineConfiguration == NULL)
    {
        ns1__GetAnalyticsConfigurationsResponse->Configurations->AnalyticsEngineConfiguration = soap_new_tt__AnalyticsEngineConfiguration(soap,-1);
    }
    ns1__GetAnalyticsConfigurationsResponse->Configurations->AnalyticsEngineConfiguration->__sizeAnalyticsModule = ONVIF_ANALYTICS_SUPPORT_NUM;
    ns1__GetAnalyticsConfigurationsResponse->Configurations->AnalyticsEngineConfiguration->AnalyticsModule = soap_new_tt__Config(soap,ONVIF_ANALYTICS_SUPPORT_NUM);
    
    /*  规则初始化 */
    if(ns1__GetAnalyticsConfigurationsResponse->Configurations->RuleEngineConfiguration == NULL)
    {
        ns1__GetAnalyticsConfigurationsResponse->Configurations->RuleEngineConfiguration = soap_new_tt__RuleEngineConfiguration(soap,-1);
    }
    ns1__GetAnalyticsConfigurationsResponse->Configurations->RuleEngineConfiguration->__sizeRule = ONVIF_ANALYTICS_RULE_SUPPORT_NUM;
    ns1__GetAnalyticsConfigurationsResponse->Configurations->RuleEngineConfiguration->Rule = soap_new_tt__Config(soap,ONVIF_ANALYTICS_RULE_SUPPORT_NUM);

    /* 分析模块和规则获取 */
    for(int i  = 0; i < ONVIF_ANALYTICS_SUPPORT_NUM;i++)
    {
        struct tt__Config *pAnalyticsModule = ns1__GetAnalyticsConfigurationsResponse->Configurations->AnalyticsEngineConfiguration->AnalyticsModule + i;
        
        struct tt__Config *pRule;
        if( i < ONVIF_ANALYTICS_RULE_SUPPORT_NUM)
        {
            pRule = ns1__GetAnalyticsConfigurationsResponse->Configurations->RuleEngineConfiguration->Rule + i;
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
            pRule->Parameters->__sizeSimpleItem = ONVIF_CELLMOTION_RULEPARAM_NUM + 1;
            pRule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap,ONVIF_CELLMOTION_RULEPARAM_NUM + 1);
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
            if(pRuleParam_3 != NULL)
            {
            /* 移动侦测网格数据 */
                pRuleParam_3->Name = soap_strdup(soap, ONVIF_CELLMOTION_RULEPARAM_4);
                pRuleParam_3->Value = soap_strdup(soap, stInfo.achBaseStr);
            }

            /* Enable Parameter */
            struct _tt__ItemList_SimpleItem *pRuleParam_4 = pRule->Parameters->SimpleItem + 4;
            if(pRuleParam_4 != NULL)
            {
                pRuleParam_4->Name = soap_strdup(soap, "Enable");
                pRuleParam_4->Value = soap_strdup(soap, stInfo.bEnable ? "true" : "false");
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
            
            /* Enable Parameter */
            pRule->Parameters->__sizeSimpleItem = 1;
            pRule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap, 1);
            if(pRule->Parameters->SimpleItem)
            {
                pRule->Parameters->SimpleItem->Name = soap_strdup(soap, "Enable");
                pRule->Parameters->SimpleItem->Value = soap_strdup(soap, stInfo.bEnable ? "true" : "false");
            }
            
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

#endif
    return SOAP_OK;
}

/** Web service operation '__ns1__GetMetadataConfigurations' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__GetMetadataConfigurations(struct soap* soap, struct ns1__GetConfiguration *ns1__GetMetadataConfigurations, struct _ns1__GetMetadataConfigurationsResponse *ns1__GetMetadataConfigurationsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__GetMetadataConfigurations----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__ns1__GetAudioOutputConfigurations' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__GetAudioOutputConfigurations(struct soap* soap, struct ns1__GetConfiguration *ns1__GetAudioOutputConfigurations, struct _ns1__GetAudioOutputConfigurationsResponse *ns1__GetAudioOutputConfigurationsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__GetAudioOutputConfigurations----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__ns1__GetAudioDecoderConfigurations' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__GetAudioDecoderConfigurations(struct soap* soap, struct ns1__GetConfiguration *ns1__GetAudioDecoderConfigurations, struct _ns1__GetAudioDecoderConfigurationsResponse *ns1__GetAudioDecoderConfigurationsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__GetAudioDecoderConfigurations----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__ns1__SetVideoSourceConfiguration' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__SetVideoSourceConfiguration(struct soap* soap, struct _ns1__SetVideoSourceConfiguration *ns1__SetVideoSourceConfiguration, struct ns1__SetConfigurationResponse *ns1__SetVideoSourceConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__SetVideoSourceConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__ns1__SetVideoEncoderConfiguration' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__SetVideoEncoderConfiguration(struct soap* soap, struct _ns1__SetVideoEncoderConfiguration *ns1__SetVideoEncoderConfiguration, struct ns1__SetConfigurationResponse *ns1__SetVideoEncoderConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__SetVideoEncoderConfiguration----------");
#endif
    if(!ns1__SetVideoEncoderConfiguration)
    {
        dlog_error("ns1__SetVideoEncoderConfiguration is NULL");
        return soap_sender_fault(soap,  "ns1__SetVideoEncoderConfiguration is NULL",  NULL); 
    }

    OnvifVideoParam_t stVideoParams;
    memset(&stVideoParams, 0, sizeof(OnvifVideoParam_t));
    if(ns1__SetVideoEncoderConfiguration->Configuration && ns1__SetVideoEncoderConfiguration->Configuration->token)
    {
        stVideoParams.nIFrameInterval = *(ns1__SetVideoEncoderConfiguration->Configuration->GovLength);
        
        memcpy(stVideoParams.strVideoCodec, ns1__SetVideoEncoderConfiguration->Configuration->Encoding, strlen(ns1__SetVideoEncoderConfiguration->Configuration->Encoding));
        
        stVideoParams.nWidths[0] = ns1__SetVideoEncoderConfiguration->Configuration->Resolution->Width;
        stVideoParams.nHeights[0] = ns1__SetVideoEncoderConfiguration->Configuration->Resolution->Height;
           
        stVideoParams.fCurFrameRate = ns1__SetVideoEncoderConfiguration->Configuration->RateControl->FrameRateLimit;
        
        stVideoParams.nCurQuality = (int)ns1__SetVideoEncoderConfiguration->Configuration->Quality;
        
        stVideoParams.bConstantBitRate = *(ns1__SetVideoEncoderConfiguration->Configuration->RateControl->ConstantBitRate);
        
        stVideoParams.nCurBitrate = ns1__SetVideoEncoderConfiguration->Configuration->RateControl->BitrateLimit;     
        
        /* Custom VideoType from __any */
        if(ns1__SetVideoEncoderConfiguration->Configuration->__size > 0 && ns1__SetVideoEncoderConfiguration->Configuration->__any)
        {
            for(int k = 0; k < ns1__SetVideoEncoderConfiguration->Configuration->__size; k++)
            {
                if(ns1__SetVideoEncoderConfiguration->Configuration->__any[k])
                {
                    char *pStart = strstr(ns1__SetVideoEncoderConfiguration->Configuration->__any[k], "<VideoType>");
                    if(pStart)
                    {
                        pStart += strlen("<VideoType>");
                        stVideoParams.nVideoType = atoi(pStart);
                        dlog_debug("Media2 Set Custom VideoType: %d", stVideoParams.nVideoType);
                        break; 
                    }
                }
            }
        }     
        
        dlog_debug("Media2 设置视频编码配置: Token=%s, 编码=%s, Gov长度=%d, 分辨率=%dx%d, 帧率=%d, 质量=%d, 定码率=%d, 码率=%d",
            ns1__SetVideoEncoderConfiguration->Configuration->token,
            ns1__SetVideoEncoderConfiguration->Configuration->Encoding,
            stVideoParams.nIFrameInterval,
            stVideoParams.nWidths[0], stVideoParams.nHeights[0],
            stVideoParams.fCurFrameRate,
            stVideoParams.nCurQuality,
            stVideoParams.bConstantBitRate,
            stVideoParams.nCurBitrate);

       if(strcmp(ns1__SetVideoEncoderConfiguration->Configuration->token, PROFILE1_VIDEOENCODER_TOKEN) == 0)
        {
            onvif_set_videoParams(&stVideoParams, 0);
        }
        else if(strcmp(ns1__SetVideoEncoderConfiguration->Configuration->token, PROFILE2_VIDEOENCODER_TOKEN) == 0)
        {
            onvif_set_videoParams(&stVideoParams, 1);
        }
        else
        {
            dlog_error("SetVideoEncoderConfiguration token is Invalid");
            return soap_sender_fault(soap,  "SetVideoEncoderConfiguration token is Invalid",  NULL); 
        }
    }
    return SOAP_OK;
}

/** Web service operation '__ns1__SetAudioSourceConfiguration' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__SetAudioSourceConfiguration(struct soap* soap, struct _ns1__SetAudioSourceConfiguration *ns1__SetAudioSourceConfiguration, struct ns1__SetConfigurationResponse *ns1__SetAudioSourceConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__SetAudioSourceConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__ns1__SetAudioEncoderConfiguration' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__SetAudioEncoderConfiguration(struct soap* soap, struct _ns1__SetAudioEncoderConfiguration *ns1__SetAudioEncoderConfiguration, struct ns1__SetConfigurationResponse *ns1__SetAudioEncoderConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__SetAudioEncoderConfiguration----------");
#endif
    if(!ns1__SetAudioEncoderConfiguration)
    {
        dlog_error("ns1__SetAudioEncoderConfiguration is NULL");
        return soap_sender_fault(soap,  "ns1__SetAudioEncoderConfiguration is NULL",  NULL); 
    }

    if(!ns1__SetAudioEncoderConfiguration->Configuration->token)
    {
        dlog_error("ns1__SetAudioEncoderConfiguration token is NULL");
        return soap_sender_fault(soap,  "ns1__SetAudioEncoderConfiguration token is NULL",  NULL); 
    }

    int nRet = -1;
    OnvifAudioParam_t stAudioParams;
    memset(&stAudioParams, 0, sizeof(OnvifAudioParam_t));

    if(strcmp(ns1__SetAudioEncoderConfiguration->Configuration->token, PROFILE_AUDIOENCODER_TOKEN) == 0)
    {   
        if(ns1__SetAudioEncoderConfiguration->Configuration->Encoding)
        {
            memcpy(stAudioParams.audioEncoding, ns1__SetAudioEncoderConfiguration->Configuration->Encoding, strlen(ns1__SetAudioEncoderConfiguration->Configuration->Encoding));
        }

        stAudioParams.audioBitrate = ns1__SetAudioEncoderConfiguration->Configuration->Bitrate*1000; // 设置传回来的为kbps，转换为bps

        stAudioParams.audioSampleRate = ns1__SetAudioEncoderConfiguration->Configuration->SampleRate*1000; // 设置传回来的为kHz，转换为Hz
        nRet = onvif_set_audioParams(&stAudioParams);
        if(nRet < 0)
        {
            dlog_error("set audio param failed");
            return soap_receiver_fault(soap,  "set audio param failed",  NULL);  
        }
    }
    else
    {
        dlog_error("ns1__SetAudioEncoderConfiguration token invalid");
        return soap_sender_fault(soap,  "ns1__SetAudioEncoderConfiguration token invalid",  NULL); 
    }

    return SOAP_OK;
}
/** Web service operation '__ns1__SetMetadataConfiguration' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__SetMetadataConfiguration(struct soap* soap, struct _ns1__SetMetadataConfiguration *ns1__SetMetadataConfiguration, struct ns1__SetConfigurationResponse *ns1__SetMetadataConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__SetMetadataConfiguration----------");
#endif
    return SOAP_OK;
}
/** Web service operation '__ns1__SetAudioOutputConfiguration' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__SetAudioOutputConfiguration(struct soap* soap, struct _ns1__SetAudioOutputConfiguration *ns1__SetAudioOutputConfiguration, struct ns1__SetConfigurationResponse *ns1__SetAudioOutputConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__SetAudioOutputConfiguration----------");
#endif
    return SOAP_OK;
}
/** Web service operation '__ns1__SetAudioDecoderConfiguration' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__SetAudioDecoderConfiguration(struct soap* soap, struct _ns1__SetAudioDecoderConfiguration *ns1__SetAudioDecoderConfiguration, struct ns1__SetConfigurationResponse *ns1__SetAudioDecoderConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__SetAudioDecoderConfiguration----------");
#endif
    return SOAP_OK;
}
/** Web service operation '__ns1__GetVideoSourceConfigurationOptions' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__GetVideoSourceConfigurationOptions(struct soap* soap, struct ns1__GetConfiguration *ns1__GetVideoSourceConfigurationOptions, struct _ns1__GetVideoSourceConfigurationOptionsResponse *ns1__GetVideoSourceConfigurationOptionsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__GetVideoSourceConfigurationOptions----------");
#endif
    return SOAP_OK;
}
/** Web service operation '__ns1__GetVideoEncoderConfigurationOptions' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__GetVideoEncoderConfigurationOptions(struct soap* soap, struct ns1__GetConfiguration *ns1__GetVideoEncoderConfigurationOptions, struct _ns1__GetVideoEncoderConfigurationOptionsResponse *ns1__GetVideoEncoderConfigurationOptionsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__GetVideoEncoderConfigurationOptions----------");
#endif
    if(ns1__GetVideoEncoderConfigurationOptionsResponse == NULL)
    {
        dlog_error("ns1__GetVideoEncoderConfigurationOptionsResponse is NULL");
        return soap_sender_fault(soap,  "ns1__GetVideoEncoderConfigurationOptionsResponse is NULL",  NULL); 
    }    

    int nIndex = -1;

    if(ns1__GetVideoEncoderConfigurationOptions->ProfileToken)
    {
        if(strcmp(ns1__GetVideoEncoderConfigurationOptions->ProfileToken, PROFILE1_TOKEN) == 0)
        {
            nIndex = 0;
        }
        else if(strcmp(ns1__GetVideoEncoderConfigurationOptions->ProfileToken, PROFILE2_TOKEN) == 0)
        {
            nIndex = 1;
        }
        else
        {
            dlog_error("ProfileToken is Invalid");
            return soap_sender_fault(soap,  "ProfileToken is Invalid",  NULL); 
        }
    }
    else if(ns1__GetVideoEncoderConfigurationOptions->ConfigurationToken)
    {
        if(strcmp(ns1__GetVideoEncoderConfigurationOptions->ConfigurationToken, PROFILE1_VIDEOENCODER_TOKEN) == 0)
        {
            nIndex = 0;
        }
        else if(strcmp(ns1__GetVideoEncoderConfigurationOptions->ConfigurationToken, PROFILE2_VIDEOENCODER_TOKEN) == 0)
        {
            nIndex = 1;
        }
        else
        {
            dlog_error("ConfigurationToken is Invalid");
            return soap_sender_fault(soap,  "ConfigurationToken is Invalid",  NULL); 
        }
    }

    // Calculate total size needed
    int totalCount = 0;
    int count0 = onvif_get_supported_codec_count(0);
    int count1 = onvif_get_supported_codec_count(1);
    
    if (nIndex == 0) totalCount = count0;
    else if (nIndex == 1) totalCount = count1;
    else totalCount = count0 + count1;
    
    ns1__GetVideoEncoderConfigurationOptionsResponse->__sizeOptions = totalCount;
    OnvifVideoParam_t stVideoParams[totalCount];
    memset(stVideoParams, 0, sizeof(OnvifVideoParam_t) * totalCount);

    ns1__GetVideoEncoderConfigurationOptionsResponse->Options = (struct tt__VideoEncoder2ConfigurationOptions *)soap_malloc(soap, sizeof(struct tt__VideoEncoder2ConfigurationOptions)*ns1__GetVideoEncoderConfigurationOptionsResponse->__sizeOptions);
    memset(ns1__GetVideoEncoderConfigurationOptionsResponse->Options, 0, sizeof(struct tt__VideoEncoder2ConfigurationOptions)*ns1__GetVideoEncoderConfigurationOptionsResponse->__sizeOptions);
    if(!ns1__GetVideoEncoderConfigurationOptionsResponse->Options) 
    {
        dlog_error("Failed to allocate");
        return soap_receiver_fault(soap, "Memory allocation failed", NULL);
    }

    int optionIndex = 0;
    if (nIndex == 0 || nIndex == -1)
    {
        for (int i = 0; i < count0; i++)
        {
            if (optionIndex >= totalCount) break;
            onvif_get_video_capability_by_index(&stVideoParams[optionIndex], 0, i);
            optionIndex++;
        }
    }
    
    if (nIndex == 1 || nIndex == -1)
    {
        for (int i = 0; i < count1; i++)
        {
            if (optionIndex >= totalCount) break;
            onvif_get_video_capability_by_index(&stVideoParams[optionIndex], 1, i);
            optionIndex++;
        }
    }
      
    for(int i = 0; i < ns1__GetVideoEncoderConfigurationOptionsResponse->__sizeOptions; i++)
    {        
        ns1__GetVideoEncoderConfigurationOptionsResponse->Options[i].Encoding = soap_strdup(soap, stVideoParams[i].strVideoCodec);

        ns1__GetVideoEncoderConfigurationOptionsResponse->Options[i].QualityRange = (struct tt__FloatRange *)soap_malloc(soap, sizeof(struct tt__FloatRange));
        if(!ns1__GetVideoEncoderConfigurationOptionsResponse->Options[i].QualityRange) 
        {
            dlog_error("Failed to allocate");
            return soap_receiver_fault(soap, "Memory allocation failed", NULL);
        }
        memset(ns1__GetVideoEncoderConfigurationOptionsResponse->Options[i].QualityRange, 0, sizeof(struct tt__FloatRange));
        ns1__GetVideoEncoderConfigurationOptionsResponse->Options[i].QualityRange->Min = stVideoParams[i].nQualityMin;
        ns1__GetVideoEncoderConfigurationOptionsResponse->Options[i].QualityRange->Max = stVideoParams[i].nQualityMax;

        ns1__GetVideoEncoderConfigurationOptionsResponse->Options[i].__sizeResolutionsAvailable = stVideoParams[i].nSizeResolutionsAvailable;
        ns1__GetVideoEncoderConfigurationOptionsResponse->Options[i].ResolutionsAvailable = (struct tt__VideoResolution2 *)soap_malloc(soap, sizeof(struct tt__VideoResolution2)*stVideoParams[i].nSizeResolutionsAvailable);
        if(!ns1__GetVideoEncoderConfigurationOptionsResponse->Options[i].ResolutionsAvailable) 
        {
            dlog_error("Failed to allocate");
            return soap_receiver_fault(soap, "Memory allocation failed", NULL);
        }
        memset(ns1__GetVideoEncoderConfigurationOptionsResponse->Options[i].ResolutionsAvailable, 0, sizeof(struct tt__VideoResolution2)*stVideoParams[i].nSizeResolutionsAvailable);
        for(int j = 0; j < stVideoParams[i].nSizeResolutionsAvailable; j++)
        {
            ns1__GetVideoEncoderConfigurationOptionsResponse->Options[i].ResolutionsAvailable[j].Width = stVideoParams[i].nWidths[j];
            ns1__GetVideoEncoderConfigurationOptionsResponse->Options[i].ResolutionsAvailable[j].Height = stVideoParams[i].nHeights[j];
        }

        ns1__GetVideoEncoderConfigurationOptionsResponse->Options[i].BitrateRange = (struct tt__IntRange *)soap_malloc(soap, sizeof(struct tt__IntRange));
        if(!ns1__GetVideoEncoderConfigurationOptionsResponse->Options[i].BitrateRange) 
        {
            dlog_error("Failed to allocate");
            return soap_receiver_fault(soap, "Memory allocation failed", NULL);
        }
        ns1__GetVideoEncoderConfigurationOptionsResponse->Options[i].BitrateRange->Min = stVideoParams[i].nBitrateMin;
        ns1__GetVideoEncoderConfigurationOptionsResponse->Options[i].BitrateRange->Max = stVideoParams[i].nBitrateMax;

        // ns1__GetVideoEncoderConfigurationOptionsResponse->Options[i].GovLengthRange = (char *)soap_malloc(soap, strlen(stVideoParams[i].GovLengthRange)+1);
        // memset(ns1__GetVideoEncoderConfigurationOptionsResponse->Options[i].GovLengthRange, 0, strlen(stVideoParams[i].GovLengthRange)+1);
        // memcpy(ns1__GetVideoEncoderConfigurationOptionsResponse->Options[i].GovLengthRange, stVideoParams[i].GovLengthRange, strlen(stVideoParams[i].GovLengthRange));

        ns1__GetVideoEncoderConfigurationOptionsResponse->Options[i].FrameRatesSupported = (char *)soap_malloc(soap, strlen(stVideoParams[i].FrameRatesSupported)+1);
        if(!ns1__GetVideoEncoderConfigurationOptionsResponse->Options[i].FrameRatesSupported) 
        {
            dlog_error("Failed to allocate");
            return soap_receiver_fault(soap, "Memory allocation failed", NULL);
        }
        memset(ns1__GetVideoEncoderConfigurationOptionsResponse->Options[i].FrameRatesSupported, 0, strlen(stVideoParams[i].FrameRatesSupported)+1);
        memcpy(ns1__GetVideoEncoderConfigurationOptionsResponse->Options[i].FrameRatesSupported, stVideoParams[i].FrameRatesSupported, strlen(stVideoParams[i].FrameRatesSupported));

        ns1__GetVideoEncoderConfigurationOptionsResponse->Options[i].ProfilesSupported = (char *)soap_malloc(soap, 32);
        if(!ns1__GetVideoEncoderConfigurationOptionsResponse->Options[i].ProfilesSupported) 
        {
            dlog_error("Failed to allocate");
            return soap_receiver_fault(soap, "Memory allocation failed", NULL);
        }
        memset(ns1__GetVideoEncoderConfigurationOptionsResponse->Options[i].ProfilesSupported, 0, 32);
        memcpy(ns1__GetVideoEncoderConfigurationOptionsResponse->Options[i].ProfilesSupported, stVideoParams[i].strEncodingComplexity, strlen(stVideoParams[i].strEncodingComplexity));

        ns1__GetVideoEncoderConfigurationOptionsResponse->Options[i].ConstantBitRateSupported = soap_new_xsd__boolean(soap, -1);  // 使用类型特定的分配函数
        if(!ns1__GetVideoEncoderConfigurationOptionsResponse->Options[i].ConstantBitRateSupported) 
        {
            dlog_error("Failed to allocate ConstantBitRateSupported");
            return soap_receiver_fault(soap, "Memory allocation failed", NULL);
        }
        *(ns1__GetVideoEncoderConfigurationOptionsResponse->Options[i].ConstantBitRateSupported) = stVideoParams[i].bConstantBitRate;
    }

    return SOAP_OK;
}

/** Web service operation '__ns1__GetAudioSourceConfigurationOptions' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__GetAudioSourceConfigurationOptions(struct soap* soap, struct ns1__GetConfiguration *ns1__GetAudioSourceConfigurationOptions, struct _ns1__GetAudioSourceConfigurationOptionsResponse *ns1__GetAudioSourceConfigurationOptionsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__GetAudioSourceConfigurationOptions----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__ns1__GetAudioEncoderConfigurationOptions' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__GetAudioEncoderConfigurationOptions(struct soap* soap, struct ns1__GetConfiguration *ns1__GetAudioEncoderConfigurationOptions, struct _ns1__GetAudioEncoderConfigurationOptionsResponse *ns1__GetAudioEncoderConfigurationOptionsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__GetAudioEncoderConfigurationOptions----------");
#endif
    if(!ns1__GetAudioEncoderConfigurationOptions || !ns1__GetAudioEncoderConfigurationOptionsResponse)
    {
        dlog_error("ns1__GetAudioEncoderConfigurationOptions or ns1__GetAudioEncoderConfigurationOptionsResponse is NULL");
        return soap_sender_fault(soap,  "ns1__GetAudioEncoderConfigurationOptions or ns1__GetAudioEncoderConfigurationOptionsResponse is NULL",  NULL);
    }
    
    int nRet = -1;
    OnvifAudioParam_t stAudioParams;
    memset(&stAudioParams, 0, sizeof(OnvifAudioParam_t));
    nRet = onvif_get_audioParams(&stAudioParams);
    if(nRet < 0)
    {
        return soap_receiver_fault(soap, "onvif get audioParams failed", NULL);
    }

    ns1__GetAudioEncoderConfigurationOptionsResponse->__sizeOptions = 1;
    ns1__GetAudioEncoderConfigurationOptionsResponse->Options = (struct tt__AudioEncoder2ConfigurationOptions *)soap_malloc(soap, sizeof(struct tt__AudioEncoder2ConfigurationOptions)*ns1__GetAudioEncoderConfigurationOptionsResponse->__sizeOptions);
    memset(ns1__GetAudioEncoderConfigurationOptionsResponse->Options, 0, sizeof(struct tt__AudioEncoder2ConfigurationOptions)*ns1__GetAudioEncoderConfigurationOptionsResponse->__sizeOptions);
    
    ns1__GetAudioEncoderConfigurationOptionsResponse->Options[0].Encoding = soap_strdup(soap, stAudioParams.audioEncoding);  // 编码格式
    
    ns1__GetAudioEncoderConfigurationOptionsResponse->Options[0].BitrateList = (struct tt__IntItems *)soap_malloc(soap, sizeof(struct tt__IntItems));
    if(!ns1__GetAudioEncoderConfigurationOptionsResponse->Options[0].BitrateList)
    {
        dlog_error("Failed to allocate");
        return soap_receiver_fault(soap, "Memory allocation failed", NULL);
    }
    memset(ns1__GetAudioEncoderConfigurationOptionsResponse->Options[0].BitrateList, 0, sizeof(struct tt__IntItems));
    ns1__GetAudioEncoderConfigurationOptionsResponse->Options[0].BitrateList->__sizeItems = sizeof(stAudioParams.BitrateList)/sizeof(int);
    
    ns1__GetAudioEncoderConfigurationOptionsResponse->Options[0].BitrateList->Items = (int *)soap_malloc(soap, sizeof(int)*ns1__GetAudioEncoderConfigurationOptionsResponse->Options[0].BitrateList->__sizeItems);
    if(!ns1__GetAudioEncoderConfigurationOptionsResponse->Options[0].BitrateList->Items)
    {
        dlog_error("Failed to allocate");
        return soap_receiver_fault(soap, "Memory allocation failed", NULL);
    }
    for(int i = 0; i < ns1__GetAudioEncoderConfigurationOptionsResponse->Options[0].BitrateList->__sizeItems; i++) // 获取支持的所有比特率
    {
        ns1__GetAudioEncoderConfigurationOptionsResponse->Options[0].BitrateList->Items[i] = stAudioParams.BitrateList[i];
    }
    
    ns1__GetAudioEncoderConfigurationOptionsResponse->Options[0].SampleRateList = (struct tt__IntItems *)soap_malloc(soap, sizeof(struct tt__IntItems));
    if(!ns1__GetAudioEncoderConfigurationOptionsResponse->Options[0].SampleRateList)
    {
        dlog_error("Failed to allocate");
        return soap_receiver_fault(soap, "Memory allocation failed", NULL);
    }
    memset(ns1__GetAudioEncoderConfigurationOptionsResponse->Options[0].SampleRateList, 0, sizeof(struct tt__IntItems));
    ns1__GetAudioEncoderConfigurationOptionsResponse->Options[0].SampleRateList->__sizeItems = sizeof(stAudioParams.SampleRateList)/sizeof(int);
    
    ns1__GetAudioEncoderConfigurationOptionsResponse->Options[0].SampleRateList->Items = (int *)soap_malloc(soap, sizeof(int)*ns1__GetAudioEncoderConfigurationOptionsResponse->Options[0].SampleRateList->__sizeItems);
    if(!ns1__GetAudioEncoderConfigurationOptionsResponse->Options[0].SampleRateList->Items)
    {
        dlog_error("Failed to allocate");
        return soap_receiver_fault(soap, "Memory allocation failed", NULL);
    }
    for(int i = 0; i < ns1__GetAudioEncoderConfigurationOptionsResponse->Options[0].SampleRateList->__sizeItems; i++) // 获取支持的所有采样率
    {
        ns1__GetAudioEncoderConfigurationOptionsResponse->Options[0].SampleRateList->Items[i] = stAudioParams.SampleRateList[i];
    }

    return SOAP_OK;
}

/** Web service operation '__ns1__GetMetadataConfigurationOptions' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__GetMetadataConfigurationOptions(struct soap* soap, struct ns1__GetConfiguration *ns1__GetMetadataConfigurationOptions, struct _ns1__GetMetadataConfigurationOptionsResponse *ns1__GetMetadataConfigurationOptionsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__GetMetadataConfigurationOptions----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__ns1__GetAudioOutputConfigurationOptions' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__GetAudioOutputConfigurationOptions(struct soap* soap, struct ns1__GetConfiguration *ns1__GetAudioOutputConfigurationOptions, struct _ns1__GetAudioOutputConfigurationOptionsResponse *ns1__GetAudioOutputConfigurationOptionsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__GetAudioOutputConfigurationOptions----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__ns1__GetAudioDecoderConfigurationOptions' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__GetAudioDecoderConfigurationOptions(struct soap* soap, struct ns1__GetConfiguration *ns1__GetAudioDecoderConfigurationOptions, struct _ns1__GetAudioDecoderConfigurationOptionsResponse *ns1__GetAudioDecoderConfigurationOptionsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__GetAudioDecoderConfigurationOptions----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__ns1__GetVideoEncoderInstances' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__GetVideoEncoderInstances(struct soap* soap, struct _ns1__GetVideoEncoderInstances *ns1__GetVideoEncoderInstances, struct _ns1__GetVideoEncoderInstancesResponse *ns1__GetVideoEncoderInstancesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__GetVideoEncoderInstances----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__ns1__GetStreamUri' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__GetStreamUri(struct soap* soap, struct _ns1__GetStreamUri *ns1__GetStreamUri, struct _ns1__GetStreamUriResponse *ns1__GetStreamUriResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__GetStreamUri----------");
#endif
    if(!ns1__GetStreamUri->ProfileToken)
    {
        dlog_error("ns1__GetStreamUri.ProfileToken is NULL");
        return SOAP_EOF;
    }

    int nRet = onvif_authentication(soap);
    if(SOAP_OK != nRet)
    {
        return nRet;
    }

    ns1__GetStreamUriResponse->Uri = (char *)soap_malloc(soap, sizeof(char) * 100);
    memset(ns1__GetStreamUriResponse->Uri, 0, sizeof(char) * 100);

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

    if(strcmp(ns1__GetStreamUri->ProfileToken, PROFILE1_TOKEN) == 0)
    {
        pUrl = onvif_get_rtsp_url(ONVIF_RTSP_CHN_MAIN);
    }
    else if(strcmp(ns1__GetStreamUri->ProfileToken, PROFILE2_TOKEN) == 0)
    {
        pUrl = onvif_get_rtsp_url(ONVIF_RTSP_CHN_SUB);
    }
    else
    {
        dlog_error("__ns1__GetStreamUri.ProfileToken error");
        return SOAP_FAULT;
    }

    if(pUrl == NULL)
    {
        dlog_error("获取取流地址失败");
        return SOAP_FAULT;
    }

    sprintf(ns1__GetStreamUriResponse->Uri, "%s", pUrl);
    // dlog_debug("ns1__GetStreamUriResponse->Uri: %s", ns1__GetStreamUriResponse->Uri)
    return SOAP_OK;
}

/** Web service operation '__ns1__StartMulticastStreaming' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__StartMulticastStreaming(struct soap* soap, struct ns1__StartStopMulticastStreaming *ns1__StartMulticastStreaming, struct ns1__SetConfigurationResponse *ns1__StartMulticastStreamingResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__StartMulticastStreaming----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__ns1__StopMulticastStreaming' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__StopMulticastStreaming(struct soap* soap, struct ns1__StartStopMulticastStreaming *ns1__StopMulticastStreaming, struct ns1__SetConfigurationResponse *ns1__StopMulticastStreamingResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__StopMulticastStreaming----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__ns1__SetSynchronizationPoint' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__SetSynchronizationPoint(struct soap* soap, struct _ns1__SetSynchronizationPoint *ns1__SetSynchronizationPoint, struct _ns1__SetSynchronizationPointResponse *ns1__SetSynchronizationPointResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__SetSynchronizationPoint----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__ns1__GetSnapshotUri' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__GetSnapshotUri(struct soap* soap, struct _ns1__GetSnapshotUri *ns1__GetSnapshotUri, struct _ns1__GetSnapshotUriResponse *ns1__GetSnapshotUriResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__GetSnapshotUri----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__ns1__GetVideoSourceModes' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__GetVideoSourceModes(struct soap* soap, struct _ns1__GetVideoSourceModes *ns1__GetVideoSourceModes, struct _ns1__GetVideoSourceModesResponse *ns1__GetVideoSourceModesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__GetVideoSourceModes----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__ns1__SetVideoSourceMode' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__SetVideoSourceMode(struct soap* soap, struct _ns1__SetVideoSourceMode *ns1__SetVideoSourceMode, struct _ns1__SetVideoSourceModeResponse *ns1__SetVideoSourceModeResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__SetVideoSourceMode----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__ns1__GetOSDs' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__GetOSDs(struct soap* soap, struct _ns1__GetOSDs *ns1__GetOSDs, struct _ns1__GetOSDsResponse *ns1__GetOSDsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__GetOSDs----------");
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
    ns1__GetOSDsResponse->__sizeOSDs = nCount; 
    dlog_debug("----------__trt__GetOSDs----nCount[%d]------",nCount);
    ns1__GetOSDsResponse->OSDs = soap_new_tt__OSDConfiguration(soap,ns1__GetOSDsResponse->__sizeOSDs);
    for(int i = 0; i < ns1__GetOSDsResponse->__sizeOSDs; i++)
    {   
        ns1__GetOSDsResponse->OSDs[i].token = soap_strdup(soap, stOnvifOsdCfgs[i].token);

        ns1__GetOSDsResponse->OSDs[i].VideoSourceConfigurationToken = soap_new_tt__OSDReference(soap,-1);
        ns1__GetOSDsResponse->OSDs[i].VideoSourceConfigurationToken->__item = soap_strdup(soap, VIDEOSOURCE_TOKEN);

        ns1__GetOSDsResponse->OSDs[i].Position = soap_new_tt__OSDPosConfiguration(soap,-1);
        ns1__GetOSDsResponse->OSDs[i].Position->Type = soap_strdup(soap, stOnvifOsdCfgs[i].Position_Type);

        if(strcmp(ns1__GetOSDsResponse->OSDs[i].Position->Type, ONVIF_TT_POSITION_CUSTOM) == 0)
        {
            ns1__GetOSDsResponse->OSDs[i].Position->Pos = soap_new_tt__Vector(soap,-1);
            ns1__GetOSDsResponse->OSDs[i].Position->Pos->x = (float *)soap_malloc(soap, sizeof(float *)); 
            ns1__GetOSDsResponse->OSDs[i].Position->Pos->y = (float *)soap_malloc(soap, sizeof(float *));

            *(ns1__GetOSDsResponse->OSDs[i].Position->Pos->x) = stOnvifOsdCfgs[i].stONvifPos.x;
            *(ns1__GetOSDsResponse->OSDs[i].Position->Pos->y) = stOnvifOsdCfgs[i].stONvifPos.y;
        }
        ns1__GetOSDsResponse->OSDs[i].Type =  stOnvifOsdCfgs[i].eOsdType;

        if(ns1__GetOSDsResponse->OSDs[i].Type == tt__OSDType__Text)
        {
            ns1__GetOSDsResponse->OSDs[i].TextString = soap_new_tt__OSDTextConfiguration(soap,-1);
            ns1__GetOSDsResponse->OSDs[i].TextString->Type = soap_strdup(soap, stOnvifOsdCfgs[i].TextString_Type);

            if(strcmp(ns1__GetOSDsResponse->OSDs[i].TextString->Type,ONVIF_TT_TYPE_DATE) == 0)
            {
                ns1__GetOSDsResponse->OSDs[i].TextString->DateFormat = soap_strdup(soap, stOnvifOsdCfgs[i].TextString_DateFormat);
            }
            else if(strcmp(ns1__GetOSDsResponse->OSDs[i].TextString->Type, ONVIF_TT_TYPE_TIME) == 0)
            {
                ns1__GetOSDsResponse->OSDs[i].TextString->TimeFormat = soap_strdup(soap, stOnvifOsdCfgs[i].TextString_TimeFormat);
            }
            else if(strcmp(ns1__GetOSDsResponse->OSDs[i].TextString->Type, ONVIF_TT_TYPE_DATEANDTIME) == 0)
            {
                ns1__GetOSDsResponse->OSDs[i].TextString->DateFormat = soap_strdup(soap, stOnvifOsdCfgs[i].TextString_DateFormat);
                ns1__GetOSDsResponse->OSDs[i].TextString->TimeFormat = soap_strdup(soap, stOnvifOsdCfgs[i].TextString_TimeFormat);      
            }
            else if(strcmp(ns1__GetOSDsResponse->OSDs[i].TextString->Type, ONVIF_TT_TYPE_PLAIN) == 0)
            {
                ns1__GetOSDsResponse->OSDs[i].TextString->PlainText = soap_strdup(soap, stOnvifOsdCfgs[i].TextString_PlainText);
            }

            ns1__GetOSDsResponse->OSDs[i].TextString->Extension = soap_new_tt__OSDTextConfigurationExtension(soap,-1);
            ns1__GetOSDsResponse->OSDs[i].TextString->Extension->__size = 1;
            ns1__GetOSDsResponse->OSDs[i].TextString->Extension->__any = (char **)soap_malloc(soap, sizeof(char *));
            if(stOnvifOsdCfgs[i].eTextType == E_OSDTYPE_TEXT_NAME)
            {
                ns1__GetOSDsResponse->OSDs[i].TextString->Extension->__any[0] = soap_strdup(soap, ONVIF_TT_OSD_TXET_EXTEN_CHANNEL_TRUE);
            }
            else
            {
                ns1__GetOSDsResponse->OSDs[i].TextString->Extension->__any[0] = soap_strdup(soap, ONVIF_TT_OSD_TXET_EXTEN_CHANNEL_FALSE);
            }

            ns1__GetOSDsResponse->OSDs[i].TextString->FontSize = (int *)soap_malloc(soap, sizeof(int));
            *(ns1__GetOSDsResponse->OSDs[i].TextString->FontSize) =  stOnvifOsdCfgs[i].TextString_FontSize;

            struct tt__OSDColor *FontColor = (struct tt__OSDColor *)soap_malloc(soap, sizeof(struct tt__OSDColor));
            ns1__GetOSDsResponse->OSDs[i].TextString->FontColor = soap_new_tt__OSDColor(soap,-1);
            if(ns1__GetOSDsResponse->OSDs[i].TextString->FontColor == NULL)
            {
                dlog_debug(" ============= FontColor is NULL================= ");
            }
            ns1__GetOSDsResponse->OSDs[i].TextString->FontColor->Transparent = (int *)soap_malloc(soap, sizeof(int));
            *(ns1__GetOSDsResponse->OSDs[i].TextString->FontColor->Transparent) =  stOnvifOsdCfgs[i].TextString_FontAlpha;

            ns1__GetOSDsResponse->OSDs[i].TextString->FontColor->Color = soap_new_tt__Color(soap,-1);;
            ns1__GetOSDsResponse->OSDs[i].TextString->FontColor->Color->X =  (float)stOnvifOsdCfgs[i].TextString_Font_R;
            ns1__GetOSDsResponse->OSDs[i].TextString->FontColor->Color->Y =  (float)stOnvifOsdCfgs[i].TextString_Font_G;
            ns1__GetOSDsResponse->OSDs[i].TextString->FontColor->Color->Z =  (float)stOnvifOsdCfgs[i].TextString_Font_B;
            ns1__GetOSDsResponse->OSDs[i].TextString->FontColor->Color->Colorspace =  soap_strdup(soap, ONVIF_TT_OSD_COLORSPACE);
        
            // ns1__GetOSDsResponse->OSDs[i].TextString->BackgroundColor = (struct tt__OSDColor *)soap_malloc(soap, sizeof(struct tt__OSDColor));
            // ns1__GetOSDsResponse->OSDs[i].TextString->BackgroundColor->Transparent = (int *)soap_malloc(soap, sizeof(int));
            // *(ns1__GetOSDsResponse->OSDs[i].TextString->BackgroundColor->Transparent) =  stOnvifOsdCfgs[i].TextString_BackgroundAlpha;
            
            // ns1__GetOSDsResponse->OSDs[i].TextString->BackgroundColor->Color = (struct tt__Color *)soap_malloc(soap, sizeof(struct tt__Color));
            // ns1__GetOSDsResponse->OSDs[i].TextString->BackgroundColor->Color->X =  (float)stOnvifOsdCfgs[i].TextString_Background_R;
            // ns1__GetOSDsResponse->OSDs[i].TextString->BackgroundColor->Color->Y =  (float)stOnvifOsdCfgs[i].TextString_Background_G;
            // ns1__GetOSDsResponse->OSDs[i].TextString->BackgroundColor->Color->Z =  (float)stOnvifOsdCfgs[i].TextString_Background_B;

        }
        else if(ns1__GetOSDsResponse->OSDs[i].Type == tt__OSDType__Image)
        {

        }
        else if(ns1__GetOSDsResponse->OSDs[i].Type == tt__OSDType__Extended)
        {
            
        }
        else
        {

        }
    }

    return SOAP_OK;
}

/** Web service operation '__ns1__GetOSDOptions' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__GetOSDOptions(struct soap* soap, struct _ns1__GetOSDOptions *ns1__GetOSDOptions, struct _ns1__GetOSDOptionsResponse *ns1__GetOSDOptionsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__GetOSDOptions----------");
#endif
    int nRet = onvif_authentication(soap);
    if(SOAP_OK != nRet)
    {
        return nRet;
    }
    /* token校验 */
    if(ns1__GetOSDOptions != NULL && ns1__GetOSDOptions->ConfigurationToken != NULL)
    {
        if(strcmp(ns1__GetOSDOptions->ConfigurationToken,VIDEOSOURCE_TOKEN) != 0)
        {
            
            dlog_error("requested  VideoSourceConfiguration does not exist");
            return soap_receiver_fault(soap, "requested VideoSourceConfiguration does not exist", NULL); 
        }
    }
    else
    {
        return soap_receiver_fault(soap, "requested VideoSourceConfiguration does not exist", NULL); 
    }

    if(ns1__GetOSDOptionsResponse == NULL)
    {
        dlog_error("ns1__GetOSDOptionsResponse is NULL");
        return soap_receiver_fault(soap, "ns1__GetOSDOptionsResponse is NULL", NULL);    
    }

    ns1__GetOSDOptionsResponse->OSDOptions = soap_new_tt__OSDConfigurationOptions(soap,-1);

    ns1__GetOSDOptionsResponse->OSDOptions->MaximumNumberOfOSDs = soap_new_tt__MaximumNumberOfOSDs(soap,-1);
    ns1__GetOSDOptionsResponse->OSDOptions->MaximumNumberOfOSDs->Total = ONVIF_OSD_MAX_NUM;
    ns1__GetOSDOptionsResponse->OSDOptions->MaximumNumberOfOSDs->PlainText = (int *)soap_malloc(soap,sizeof(int *));
    ns1__GetOSDOptionsResponse->OSDOptions->MaximumNumberOfOSDs->Date = (int *)soap_malloc(soap,sizeof(int *));
    ns1__GetOSDOptionsResponse->OSDOptions->MaximumNumberOfOSDs->Time = (int *)soap_malloc(soap,sizeof(int *));
    ns1__GetOSDOptionsResponse->OSDOptions->MaximumNumberOfOSDs->DateAndTime = (int *)soap_malloc(soap,sizeof(int *));
    *(ns1__GetOSDOptionsResponse->OSDOptions->MaximumNumberOfOSDs->PlainText) = ONVIF_OSD_TEXT_NUM;
    *(ns1__GetOSDOptionsResponse->OSDOptions->MaximumNumberOfOSDs->Date) = ONVIF_OSD_DATE_NUM;
    *(ns1__GetOSDOptionsResponse->OSDOptions->MaximumNumberOfOSDs->Time) = ONVIF_OSD_DATE_NUM;
    *(ns1__GetOSDOptionsResponse->OSDOptions->MaximumNumberOfOSDs->DateAndTime) = ONVIF_OSD_DATE_NUM;
   
    ns1__GetOSDOptionsResponse->OSDOptions->__sizeType = 1;
    ns1__GetOSDOptionsResponse->OSDOptions->Type = soap_new_tt__OSDType(soap,-1);
    *(ns1__GetOSDOptionsResponse->OSDOptions->Type) = tt__OSDType__Text;

    ns1__GetOSDOptionsResponse->OSDOptions->__sizePositionOption = 1;
    ns1__GetOSDOptionsResponse->OSDOptions->PositionOption =  (char **)soap_malloc(
        soap, 
        sizeof(char *) * ns1__GetOSDOptionsResponse->OSDOptions->__sizePositionOption
    );
    ns1__GetOSDOptionsResponse->OSDOptions->PositionOption[0] = (char *)soap_strdup(soap, ONVIF_TT_POSITION_UPPER_LEFT);
    ns1__GetOSDOptionsResponse->OSDOptions->PositionOption[1] = (char *)soap_strdup(soap, ONVIF_TT_POSITION_UPPER_RIGHT);
    ns1__GetOSDOptionsResponse->OSDOptions->PositionOption[2] = (char *)soap_strdup(soap, ONVIF_TT_POSITION_LOWER_LEFT);
    ns1__GetOSDOptionsResponse->OSDOptions->PositionOption[3] = (char *)soap_strdup(soap, ONVIF_TT_POSITION_LOWER_RIGHT);
    ns1__GetOSDOptionsResponse->OSDOptions->PositionOption[4] = (char *)soap_strdup(soap, ONVIF_TT_POSITION_CUSTOM);
    
    ns1__GetOSDOptionsResponse->OSDOptions->TextOption = soap_new_tt__OSDTextOptions(soap,-1);

    ns1__GetOSDOptionsResponse->OSDOptions->TextOption->__sizeType = ONVIF_OSD_TYPE_NUM;
    ns1__GetOSDOptionsResponse->OSDOptions->TextOption->Type  =  (char **)soap_malloc(soap, sizeof(char *));
    ns1__GetOSDOptionsResponse->OSDOptions->TextOption->Type[0] = soap_strdup(soap, ONVIF_TT_TYPE_PLAIN);
    ns1__GetOSDOptionsResponse->OSDOptions->TextOption->Type[1] = soap_strdup(soap, ONVIF_TT_TYPE_DATE);
    ns1__GetOSDOptionsResponse->OSDOptions->TextOption->Type[2] = soap_strdup(soap, ONVIF_TT_TYPE_TIME);
    ns1__GetOSDOptionsResponse->OSDOptions->TextOption->Type[3] = soap_strdup(soap, ONVIF_TT_TYPE_DATEANDTIME);

    ns1__GetOSDOptionsResponse->OSDOptions->TextOption->FontSizeRange = soap_new_tt__IntRange(soap,-1);
    ns1__GetOSDOptionsResponse->OSDOptions->TextOption->FontSizeRange->Min = ONVIF_OSD_FONT_MIN;
    ns1__GetOSDOptionsResponse->OSDOptions->TextOption->FontSizeRange->Max = ONVIF_OSD_FONT_MAX;

    ns1__GetOSDOptionsResponse->OSDOptions->TextOption->__sizeDateFormat = ONVIF_OSD_DATEFORMAT_TYPE_NUM;
    ns1__GetOSDOptionsResponse->OSDOptions->TextOption->DateFormat  =  (char **)soap_malloc(soap, sizeof(char *));
    ns1__GetOSDOptionsResponse->OSDOptions->TextOption->DateFormat[0] = soap_strdup(soap, ONVIF_TT_DATE_FORMAT_MM_DD_YYYY);
    ns1__GetOSDOptionsResponse->OSDOptions->TextOption->DateFormat[1] = soap_strdup(soap, ONVIF_TT_DATE_FORMAT_DD_MM_YYYY);
    ns1__GetOSDOptionsResponse->OSDOptions->TextOption->DateFormat[2] = soap_strdup(soap, ONVIF_TT_DATE_FORMAT_YYYY_MM_DD_SLASH);
    ns1__GetOSDOptionsResponse->OSDOptions->TextOption->DateFormat[3] = soap_strdup(soap, ONVIF_TT_DATE_FORMAT_YYYY_MM_DD_DASH);

    ns1__GetOSDOptionsResponse->OSDOptions->TextOption->__sizeTimeFormat = ONVIF_OSD_TIMEFORMAT_TYPE_NUM;
    ns1__GetOSDOptionsResponse->OSDOptions->TextOption->TimeFormat  =  (char **)soap_malloc(soap, sizeof(char *));
    ns1__GetOSDOptionsResponse->OSDOptions->TextOption->TimeFormat[0] = soap_strdup(soap, ONVIF_TT_TIME_FORMAT_HH_MM_SS_12H);
    ns1__GetOSDOptionsResponse->OSDOptions->TextOption->TimeFormat[1] = soap_strdup(soap, ONVIF_TT_TIME_FORMAT_HH_MM_SS_24H);
    
    ns1__GetOSDOptionsResponse->OSDOptions->TextOption->FontColor = soap_new_tt__OSDColorOptions(soap,-1);
    ns1__GetOSDOptionsResponse->OSDOptions->TextOption->FontColor->Color = soap_new_tt__ColorOptions(soap,-1);
    ns1__GetOSDOptionsResponse->OSDOptions->TextOption->FontColor->Color->__sizeColorspaceRange = 1;
    ns1__GetOSDOptionsResponse->OSDOptions->TextOption->FontColor->Color->ColorspaceRange = soap_new_tt__ColorspaceRange(soap,-1);
    ns1__GetOSDOptionsResponse->OSDOptions->TextOption->FontColor->Color->ColorspaceRange->X = soap_new_tt__FloatRange(soap,-1);
    ns1__GetOSDOptionsResponse->OSDOptions->TextOption->FontColor->Color->ColorspaceRange->X->Min = ONVIF_OSD_COLOR_MIN;
    ns1__GetOSDOptionsResponse->OSDOptions->TextOption->FontColor->Color->ColorspaceRange->X->Max = ONVIF_OSD_COLOR_MAX;
    ns1__GetOSDOptionsResponse->OSDOptions->TextOption->FontColor->Color->ColorspaceRange->Y = soap_new_tt__FloatRange(soap,-1);
    ns1__GetOSDOptionsResponse->OSDOptions->TextOption->FontColor->Color->ColorspaceRange->Y->Min = ONVIF_OSD_COLOR_MIN;
    ns1__GetOSDOptionsResponse->OSDOptions->TextOption->FontColor->Color->ColorspaceRange->Y->Max = ONVIF_OSD_COLOR_MAX;
    ns1__GetOSDOptionsResponse->OSDOptions->TextOption->FontColor->Color->ColorspaceRange->Z = soap_new_tt__FloatRange(soap,-1);
    ns1__GetOSDOptionsResponse->OSDOptions->TextOption->FontColor->Color->ColorspaceRange->Z->Min = ONVIF_OSD_COLOR_MIN;
    ns1__GetOSDOptionsResponse->OSDOptions->TextOption->FontColor->Color->ColorspaceRange->Z->Max = ONVIF_OSD_COLOR_MAX;
    ns1__GetOSDOptionsResponse->OSDOptions->TextOption->FontColor->Color->ColorspaceRange->Colorspace =  soap_strdup(soap, ONVIF_TT_OSD_COLORSPACE);

    return SOAP_OK;
}

/** Web service operation '__ns1__SetOSD' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__SetOSD(struct soap* soap, struct _ns1__SetOSD *ns1__SetOSD, struct ns1__SetConfigurationResponse *ns1__SetOSDResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__SetOSD----------");
#endif
     int nRet = onvif_authentication(soap);
    if(SOAP_OK != nRet)
    {
        return nRet;
    }
    if(ns1__SetOSD == NULL || ns1__SetOSD->OSD == NULL|| ns1__SetOSD->OSD->token == NULL)
    {
        dlog_error("ns1__SetOSD is NULL");
        return SOAP_EOF;
    }

    OnvifOsdCfg_t stOnvifOsdCfgs;
    memset(&stOnvifOsdCfgs, 0, sizeof(stOnvifOsdCfgs));

    if(ns1__SetOSD->OSD->Position == NULL)
    {
        dlog_error("ns1__SetOSD Position is NULL");
        return SOAP_EOF;
    }

    if(ns1__SetOSD->OSD->Type == tt__OSDType__Text)
    {
        memset(stOnvifOsdCfgs.stONvifPos.achTpye, 0, sizeof(stOnvifOsdCfgs.stONvifPos.achTpye));
        memcpy(stOnvifOsdCfgs.stONvifPos.achTpye, ns1__SetOSD->OSD->Position->Type, strlen(ns1__SetOSD->OSD->Position->Type));
        
        if(strcmp(ns1__SetOSD->OSD->Position->Type, ONVIF_TT_POSITION_CUSTOM) == 0)
        {
            if(ns1__SetOSD->OSD->Position->Pos != NULL)
            {
                stOnvifOsdCfgs.stONvifPos.x =  *(ns1__SetOSD->OSD->Position->Pos->x);
                stOnvifOsdCfgs.stONvifPos.y =  *(ns1__SetOSD->OSD->Position->Pos->y);
            }
        }

        if(strcmp(ns1__SetOSD->OSD->TextString->Type, "Date") == 0)
        {
            memset(stOnvifOsdCfgs.TextString_DateFormat, 0, sizeof(stOnvifOsdCfgs.TextString_DateFormat));
            memcpy(stOnvifOsdCfgs.TextString_DateFormat, ns1__SetOSD->OSD->TextString->DateFormat, strlen(ns1__SetOSD->OSD->TextString->DateFormat));

        }
        else if(strcmp(ns1__SetOSD->OSD->TextString->Type, "Time") == 0)
        {
            memset(stOnvifOsdCfgs.TextString_TimeFormat, 0, sizeof(stOnvifOsdCfgs.TextString_TimeFormat));
            memcpy(stOnvifOsdCfgs.TextString_TimeFormat, ns1__SetOSD->OSD->TextString->TimeFormat, strlen(ns1__SetOSD->OSD->TextString->TimeFormat));

        }
        else if(strcmp(ns1__SetOSD->OSD->TextString->Type, "DateAndTime") == 0)
        {
            memset(stOnvifOsdCfgs.TextString_DateFormat, 0, sizeof(stOnvifOsdCfgs.TextString_DateFormat));
            memcpy(stOnvifOsdCfgs.TextString_DateFormat, ns1__SetOSD->OSD->TextString->DateFormat, strlen(ns1__SetOSD->OSD->TextString->DateFormat));

            memset(stOnvifOsdCfgs.TextString_TimeFormat, 0, sizeof(stOnvifOsdCfgs.TextString_TimeFormat));
            memcpy(stOnvifOsdCfgs.TextString_TimeFormat, ns1__SetOSD->OSD->TextString->TimeFormat, strlen(ns1__SetOSD->OSD->TextString->TimeFormat));
        }
        else if(strcmp(ns1__SetOSD->OSD->TextString->Type, "Plain") == 0)
        {
            memset(stOnvifOsdCfgs.TextString_PlainText, '\0', strlen( stOnvifOsdCfgs.TextString_PlainText)+1);
            memcpy(stOnvifOsdCfgs.TextString_PlainText, ns1__SetOSD->OSD->TextString->PlainText, strlen(ns1__SetOSD->OSD->TextString->PlainText));
        }
        else
        {}

        nRet = onvif_set_osdParam(&stOnvifOsdCfgs,ns1__SetOSD->OSD->token);
        if(nRet < 0)
        {
            dlog_error("ns1__SetOSD 设置失败");
            return SOAP_EOF;
        }
    }
    else
    {
        dlog_error("ns1__SetOSD 没有对应的type类型");
        return SOAP_EOF;
    }

     dlog_debug("ns1__SetOSD 设置成功");
    return SOAP_OK;
}

/** Web service operation '__ns1__CreateOSD' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__CreateOSD(struct soap* soap, struct _ns1__CreateOSD *ns1__CreateOSD, struct _ns1__CreateOSDResponse *ns1__CreateOSDResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__CreateOSD----------");
#endif
     if(ns1__CreateOSD == NULL || ns1__CreateOSD->OSD == NULL|| ns1__CreateOSD->OSD->token == NULL)
    {
        dlog_error("trt__CreateOSD is NULL");
        return SOAP_EOF;
    }
     dlog_debug("----------__trt__CreateOSD----[%s]------",ns1__CreateOSD->OSD->token);
    OnvifOsdCfg_t stOnvifOsdCfgs;
    memset(&stOnvifOsdCfgs, 0, sizeof(stOnvifOsdCfgs));

    if(ns1__CreateOSD->OSD->Type == tt__OSDType__Text)
    {
        memset(stOnvifOsdCfgs.stONvifPos.achTpye, 0, sizeof(stOnvifOsdCfgs.stONvifPos.achTpye));
        memcpy(stOnvifOsdCfgs.stONvifPos.achTpye, ns1__CreateOSD->OSD->Position->Type, strlen(ns1__CreateOSD->OSD->Position->Type));
        
        if(strcmp(ns1__CreateOSD->OSD->Position->Type, ONVIF_TT_POSITION_CUSTOM) == 0)
        {
            if(ns1__CreateOSD->OSD->Position->Pos != NULL)
            {
                stOnvifOsdCfgs.stONvifPos.x =  *(ns1__CreateOSD->OSD->Position->Pos->x);
                stOnvifOsdCfgs.stONvifPos.y =  *(ns1__CreateOSD->OSD->Position->Pos->y);
            }
        }

        if(strcmp(ns1__CreateOSD->OSD->TextString->Type, "Date") == 0 || strcmp(ns1__CreateOSD->OSD->TextString->Type, "Time") == 0
           || strcmp(ns1__CreateOSD->OSD->TextString->Type, "DateAndTime") == 0)
        {
            stOnvifOsdCfgs.eTextType = E_OSDTYPE_TEXT_TIME;
        }
        else if(strcmp(ns1__CreateOSD->OSD->TextString->Type, "Plain") == 0)
        {
            stOnvifOsdCfgs.eTextType = E_OSDTYPE_TEXT_EXTEND;
            /* 判断是否是通道名称 */
            if(ns1__CreateOSD->OSD->TextString->Extension != NULL && ns1__CreateOSD->OSD->TextString->Extension->__any != NULL)
            {
                if((strstr(ns1__CreateOSD->OSD->TextString->Extension->__any[0], "ChannelName") != NULL) && (strstr(ns1__CreateOSD->OSD->TextString->Extension->__any[0], "true") != NULL))
                {
                    dlog_debug("trt__CreateOSD token [%s] is ChannelName",ns1__CreateOSD->OSD->token);
                    stOnvifOsdCfgs.eTextType = E_OSDTYPE_TEXT_NAME;
                }
            }
        }
        else
        {
            dlog_error("ns1__CreateOSD->OSD->TextString->Type is error");
            return SOAP_EOF;
        }


        if(strcmp(ns1__CreateOSD->OSD->TextString->Type, "Date") == 0)
        {
            memset(stOnvifOsdCfgs.TextString_DateFormat, 0, sizeof(stOnvifOsdCfgs.TextString_DateFormat));
            memcpy(stOnvifOsdCfgs.TextString_DateFormat, ns1__CreateOSD->OSD->TextString->DateFormat, strlen(ns1__CreateOSD->OSD->TextString->DateFormat));
            stOnvifOsdCfgs.bOsdEnable = true;
        }
        else if(strcmp(ns1__CreateOSD->OSD->TextString->Type, "Time") == 0)
        {
            memset(stOnvifOsdCfgs.TextString_TimeFormat, 0, sizeof(stOnvifOsdCfgs.TextString_TimeFormat));
            memcpy(stOnvifOsdCfgs.TextString_TimeFormat, ns1__CreateOSD->OSD->TextString->TimeFormat, strlen(ns1__CreateOSD->OSD->TextString->TimeFormat));
            stOnvifOsdCfgs.bOsdEnable = true;
        }
        else if(strcmp(ns1__CreateOSD->OSD->TextString->Type, "DateAndTime") == 0)
        {
            memset(stOnvifOsdCfgs.TextString_DateFormat, 0, sizeof(stOnvifOsdCfgs.TextString_DateFormat));
            memcpy(stOnvifOsdCfgs.TextString_DateFormat, ns1__CreateOSD->OSD->TextString->DateFormat, strlen(ns1__CreateOSD->OSD->TextString->DateFormat));

            memset(stOnvifOsdCfgs.TextString_TimeFormat, 0, sizeof(stOnvifOsdCfgs.TextString_TimeFormat));
            memcpy(stOnvifOsdCfgs.TextString_TimeFormat, ns1__CreateOSD->OSD->TextString->TimeFormat, strlen(ns1__CreateOSD->OSD->TextString->TimeFormat));

            stOnvifOsdCfgs.bOsdEnable = true;

        }
        else if(strcmp(ns1__CreateOSD->OSD->TextString->Type, "Plain") == 0)
        {
            memset(stOnvifOsdCfgs.TextString_PlainText, '\0', strlen( stOnvifOsdCfgs.TextString_PlainText)+1);
            memcpy(stOnvifOsdCfgs.TextString_PlainText, ns1__CreateOSD->OSD->TextString->PlainText, strlen(ns1__CreateOSD->OSD->TextString->PlainText));
            stOnvifOsdCfgs.bOsdEnable = true;
        }
        else
        {
            dlog_error("ns1__CreateOSD->OSD->TextString->Type is error");
            return SOAP_EOF;
        }

        int nRet = onvif_create_osd(&stOnvifOsdCfgs,ns1__CreateOSD->OSD->token);
        if(nRet < 0)
        {
            dlog_error("trt__CreateOSD is error");
            return SOAP_EOF;
        }
        else
        {
            dlog_debug("trt__CreateOSD is ok");
            if(ns1__CreateOSDResponse == NULL)
            {
                ns1__CreateOSDResponse = soap_new__ns1__CreateOSDResponse(soap,-1);
            }
            ns1__CreateOSDResponse->OSDToken = soap_strdup(soap, stOnvifOsdCfgs.token);
            return SOAP_OK;
        }
    }

    dlog_debug("trt__CreateOSD is ok");  

    return SOAP_OK;
}

/** Web service operation '__ns1__DeleteOSD' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__DeleteOSD(struct soap* soap, struct _ns1__DeleteOSD *ns1__DeleteOSD, struct ns1__SetConfigurationResponse *ns1__DeleteOSDResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__DeleteOSD----------");
#endif
    int nRet = onvif_authentication(soap);
    if(SOAP_OK != nRet)
    {
        return nRet;
    }
    if(ns1__DeleteOSD == NULL && ns1__DeleteOSD->OSDToken == NULL)
    {
        dlog_error("ns1__DeleteOSD->OSDToken is NULL");
        return SOAP_EOF;
    }
     dlog_debug("----------ns1__DeleteOSD-------%s---",ns1__DeleteOSD->OSDToken);
    nRet = onvif_delete_osd(ns1__DeleteOSD->OSDToken);
    return nRet;
}

/** Web service operation '__ns1__GetMasks' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__GetMasks(struct soap* soap, struct _ns1__GetMasks *ns1__GetMasks, struct _ns1__GetMasksResponse *ns1__GetMasksResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__GetMasks----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__ns1__GetMaskOptions' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__GetMaskOptions(struct soap* soap, struct _ns1__GetMaskOptions *ns1__GetMaskOptions, struct _ns1__GetMaskOptionsResponse *ns1__GetMaskOptionsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__GetMaskOptions----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__ns1__SetMask' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__SetMask(struct soap* soap, struct _ns1__SetMask *ns1__SetMask, struct ns1__SetConfigurationResponse *ns1__SetMaskResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__SetMask----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__ns1__CreateMask' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__CreateMask(struct soap* soap, struct _ns1__CreateMask *ns1__CreateMask, struct _ns1__CreateMaskResponse *ns1__CreateMaskResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__CreateMask----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__ns1__DeleteMask' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__DeleteMask(struct soap* soap, struct _ns1__DeleteMask *ns1__DeleteMask, struct ns1__SetConfigurationResponse *ns1__DeleteMaskResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__DeleteMask----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__ns1__GetWebRTCConfigurations' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__GetWebRTCConfigurations(struct soap* soap, struct _ns1__GetWebRTCConfigurations *ns1__GetWebRTCConfigurations, struct _ns1__GetWebRTCConfigurationsResponse *ns1__GetWebRTCConfigurationsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__GetWebRTCConfigurations----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__ns1__SetWebRTCConfigurations' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __ns1__SetWebRTCConfigurations(struct soap* soap, struct _ns1__SetWebRTCConfigurations *ns1__SetWebRTCConfigurations, struct _ns1__SetWebRTCConfigurationsResponse *ns1__SetWebRTCConfigurationsResponse){
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__ns1__SetWebRTCConfigurations----------");
#endif
    return SOAP_OK;
}