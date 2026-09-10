/**
 * @file analytics.c
 * @author tianl (tianl@kfb.cn)
 * @date 2025-09-23
 * 
 * @brief onvif analytics服务接口
 */
#include "onvif_server_wrapper.h"


/** Web service operation '__tan__GetSupportedRules' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tan__GetSupportedRules(struct soap* soap, struct _tan__GetSupportedRules *tan__GetSupportedRules, struct _tan__GetSupportedRulesResponse *tan__GetSupportedRulesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tan__GetSupportedRules----------");
#endif
    int nRet = onvif_authentication(soap);
    if(SOAP_OK != nRet)
    {
        return nRet;
    }
    /* token校验 */
    if(tan__GetSupportedRules != NULL && tan__GetSupportedRules->ConfigurationToken != NULL)
    {
        if(strcmp(tan__GetSupportedRules->ConfigurationToken,VIDEOANALTICS_TOKEN) != 0)
        {
            
            dlog_error("requested  VideoSourceConfiguration does not exist");
            return soap_receiver_fault(soap, "requested VideoSourceConfiguration does not exist", NULL); 
        }
    }
    else
    {
        return soap_receiver_fault(soap, "requested VideoSourceConfiguration does not exist", NULL); 
    }

    if(tan__GetSupportedRulesResponse == NULL)
    {
        dlog_error("tan__GetSupportedRulesResponse is NULL");
        return soap_receiver_fault(soap, "tan__GetSupportedRulesResponse is NULL", NULL);    
    }

    if(tan__GetSupportedRulesResponse->SupportedRules == NULL)
    {
        tan__GetSupportedRulesResponse->SupportedRules = soap_new_tt__SupportedRules(soap,-1);
    }
    tan__GetSupportedRulesResponse->SupportedRules->__sizeRuleContentSchemaLocation = 1;

    tan__GetSupportedRulesResponse->SupportedRules->RuleContentSchemaLocation = (char **)soap_malloc(soap,sizeof(char*));
    tan__GetSupportedRulesResponse->SupportedRules->RuleContentSchemaLocation[0] =soap_strdup(soap, "http://www.w3.org/2001/XMLSchema");
    
    //tan__GetSupportedRulesResponse->SupportedRules->RuleContentSchemaLocation =  "http://www.w3.org/2001/XMLSchema";
    tan__GetSupportedRulesResponse->SupportedRules->__sizeRuleDescription = ONVIF_ANALYTICS_SUPPORT_NUM;
    tan__GetSupportedRulesResponse->SupportedRules->RuleDescription = soap_new_tt__ConfigDescription(soap,tan__GetSupportedRulesResponse->SupportedRules->__sizeRuleDescription);
    for(int i  = 0; i < ONVIF_ANALYTICS_SUPPORT_NUM;i++)
    {
        struct tt__ConfigDescription *pRuleDescription = tan__GetSupportedRulesResponse->SupportedRules->RuleDescription + i;
        switch (i)
        {
            case MOTION_DETECTION_ALARM:
            {
                pRuleDescription->Name = soap_strdup(soap,MOTION_EVENT_RULE_TYPE);
                /* 最大配置数量 */
                pRuleDescription->maxInstances = soap_strdup(soap,"1");
                pRuleDescription->fixed = soap_new_xsd__boolean(soap,-1);
                *(pRuleDescription->fixed) = xsd__boolean__true_;
                pRuleDescription->Parameters = soap_new_tt__ItemListDescription(soap,-1);
                pRuleDescription->Parameters->__sizeSimpleItemDescription = ONVIF_CELLMOTION_RULEPARAM_NUM;
                pRuleDescription->Parameters->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap,ONVIF_CELLMOTION_RULEPARAM_NUM);
                struct _tt__ItemListDescription_SimpleItemDescription *pParameters_0 = pRuleDescription->Parameters->SimpleItemDescription + 0;
                struct _tt__ItemListDescription_SimpleItemDescription *pParameters_1 = pRuleDescription->Parameters->SimpleItemDescription + 1;
                struct _tt__ItemListDescription_SimpleItemDescription *pParameters_2 = pRuleDescription->Parameters->SimpleItemDescription + 2;
                struct _tt__ItemListDescription_SimpleItemDescription *pParameters_3 = pRuleDescription->Parameters->SimpleItemDescription + 3;
                if(pParameters_0 != NULL)
                {
                    pParameters_0->Name = soap_strdup(soap, ONVIF_CELLMOTION_RULEPARAM_1);
                    pParameters_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }
                if(pParameters_1 != NULL)
                {
                    pParameters_1->Name = soap_strdup(soap, ONVIF_CELLMOTION_RULEPARAM_2);
                    pParameters_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }
                if(pParameters_2 != NULL)
                {
                    pParameters_2->Name = soap_strdup(soap, ONVIF_CELLMOTION_RULEPARAM_3);
                    pParameters_2->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }
                if(pParameters_3 != NULL)
                {
                    pParameters_3->Name = soap_strdup(soap, ONVIF_CELLMOTION_RULEPARAM_4);
                    pParameters_3->Type = soap_strdup(soap, ONVIF_ANALYTICS_BASE64_TYPE);
                }

                pRuleDescription->__sizeMessages = 1;
                pRuleDescription->Messages = soap_new__tt__ConfigDescription_Messages(soap,-1);
                pRuleDescription->Messages->ParentTopic = soap_strdup(soap, MOTION_EVENT_THEME);
                pRuleDescription->Messages->IsProperty = soap_new_xsd__boolean(soap,-1);
                *(pRuleDescription->Messages->IsProperty) = xsd__boolean__true_;
                pRuleDescription->Messages->Source = soap_new_tt__ItemListDescription(soap,-1);
                pRuleDescription->Messages->Source->__sizeSimpleItemDescription = 3;
                pRuleDescription->Messages->Source->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap,3);
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_0 = pRuleDescription->Messages->Source->SimpleItemDescription + 0;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_1 = pRuleDescription->Messages->Source->SimpleItemDescription + 1;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_2 = pRuleDescription->Messages->Source->SimpleItemDescription + 2;
                if(pSource_0 != NULL)
                {
                    pSource_0->Name = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
                    pSource_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_1 != NULL)
                {
                    pSource_1->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN);
                    pSource_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_2 != NULL)
                {
                    pSource_2->Name = soap_strdup(soap, "Rule");
                    pSource_2->Type = soap_strdup(soap, ONVIF_ANALYTICS_STRING_TYPE);
                }            


                pRuleDescription->Messages->Data = soap_new_tt__ItemListDescription(soap,-1);
                pRuleDescription->Messages->Data->__sizeSimpleItemDescription = 1;
                pRuleDescription->Messages->Data->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap,1);
                struct _tt__ItemListDescription_SimpleItemDescription *pDate_0 = pRuleDescription->Messages->Data->SimpleItemDescription + 0;
                if(pDate_0 != NULL)
                {
                    pDate_0->Name = soap_strdup(soap, MOTION_NAME);
                    pDate_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_BOOL_TYPE);
                } 
                break;
            }
            
            case IMAGE_OBSTRUTION_ALARM:
            {
                pRuleDescription->Name = soap_strdup(soap,TAMPEREVENT_RULE_TYPE);
                /* 最大配置数量 */
                pRuleDescription->maxInstances = soap_strdup(soap,"1");
                pRuleDescription->fixed = soap_new_xsd__boolean(soap,-1);
                *(pRuleDescription->fixed) = xsd__boolean__true_;
                pRuleDescription->Parameters = soap_new_tt__ItemListDescription(soap,-1);
                pRuleDescription->Parameters->__sizeElementItemDescription = ONVIF_TAMPER_RULEPARAM_NUM;
                pRuleDescription->Parameters->ElementItemDescription = soap_new__tt__ItemListDescription_ElementItemDescription(soap,ONVIF_CELLMOTION_RULEPARAM_NUM);
                struct _tt__ItemListDescription_ElementItemDescription *pParameters_0 = pRuleDescription->Parameters->ElementItemDescription + 0;
                if(pParameters_0 != NULL)
                {
                    pParameters_0->Name = soap_strdup(soap, "Field");
                    pParameters_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_POLYGON_TYPE);
                }

                pRuleDescription->__sizeMessages = 1;
                pRuleDescription->Messages = soap_new__tt__ConfigDescription_Messages(soap,-1);
                pRuleDescription->Messages->ParentTopic = soap_strdup(soap, TAMPER_EVENT_THEME);
                pRuleDescription->Messages->IsProperty = soap_new_xsd__boolean(soap,-1);
                *(pRuleDescription->Messages->IsProperty) = xsd__boolean__true_;
                pRuleDescription->Messages->Source = soap_new_tt__ItemListDescription(soap,-1);
                pRuleDescription->Messages->Source->__sizeSimpleItemDescription = 3;
                pRuleDescription->Messages->Source->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap,3);
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_0 = pRuleDescription->Messages->Source->SimpleItemDescription + 0;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_1 = pRuleDescription->Messages->Source->SimpleItemDescription + 1;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_2 = pRuleDescription->Messages->Source->SimpleItemDescription + 2;
                if(pSource_0 != NULL)
                {
                    pSource_0->Name = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
                    pSource_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_1 != NULL)
                {
                    pSource_1->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN);
                    pSource_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_2 != NULL)
                {
                    pSource_2->Name = soap_strdup(soap, "Rule");
                    pSource_2->Type = soap_strdup(soap, ONVIF_ANALYTICS_STRING_TYPE);
                }            


                pRuleDescription->Messages->Data = soap_new_tt__ItemListDescription(soap,-1);
                pRuleDescription->Messages->Data->__sizeSimpleItemDescription = 1;
                pRuleDescription->Messages->Data->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap,1);
                struct _tt__ItemListDescription_SimpleItemDescription *pDate_0 = pRuleDescription->Messages->Data->SimpleItemDescription + 0;
                if(pDate_0 != NULL)
                {
                    pDate_0->Name = soap_strdup(soap, TAMPER_NAME);
                    pDate_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_BOOL_TYPE);
                } 

                break;
            }
            
            case INTRUSION_ALARM:
            {
                pRuleDescription->Name = soap_strdup(soap,FIELD_EVENT_RULE_TYPE);
                /* 最大配置数量 */
                pRuleDescription->maxInstances = soap_strdup(soap,"4");
                pRuleDescription->fixed = soap_new_xsd__boolean(soap,-1);
                *(pRuleDescription->fixed) = xsd__boolean__false_;
                pRuleDescription->Parameters = soap_new_tt__ItemListDescription(soap,-1);
                pRuleDescription->Parameters->__sizeSimpleItemDescription = 1;
                pRuleDescription->Parameters->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pParam_1 = pRuleDescription->Parameters->SimpleItemDescription + 0;

                if(pParam_1 != NULL) {
                     pParam_1->Name = soap_strdup(soap, "TimeThreshold");
                     pParam_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }

                pRuleDescription->Parameters->__sizeElementItemDescription = 1;
                pRuleDescription->Parameters->ElementItemDescription = soap_new__tt__ItemListDescription_ElementItemDescription(soap,-1);
                struct _tt__ItemListDescription_ElementItemDescription *pParameters_2 = pRuleDescription->Parameters->ElementItemDescription + 0;
                if(pParameters_2 != NULL)
                {
                    pParameters_2->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                    pParameters_2->Type = soap_strdup(soap, ONVIF_ANALYTICS_POLYGON_TYPE);
                }

                pRuleDescription->__sizeMessages = 1;
                pRuleDescription->Messages = soap_new__tt__ConfigDescription_Messages(soap,-1);
                 pRuleDescription->Messages->ParentTopic = soap_strdup(soap, FIELD_EVENT_THEME);
                pRuleDescription->Messages->IsProperty = soap_new_xsd__boolean(soap,-1);
                *(pRuleDescription->Messages->IsProperty) = xsd__boolean__false_;
                pRuleDescription->Messages->Source = soap_new_tt__ItemListDescription(soap,-1);
                pRuleDescription->Messages->Source->__sizeSimpleItemDescription = 3;
                pRuleDescription->Messages->Source->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap,3);
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_0 = pRuleDescription->Messages->Source->SimpleItemDescription + 0;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_1 = pRuleDescription->Messages->Source->SimpleItemDescription + 1;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_2 = pRuleDescription->Messages->Source->SimpleItemDescription + 2;
                if(pSource_0 != NULL)
                {
                    pSource_0->Name = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
                    pSource_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_1 != NULL)
                {
                    pSource_1->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN);
                    pSource_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_2 != NULL)
                {
                    pSource_2->Name = soap_strdup(soap, "Rule");
                    pSource_2->Type = soap_strdup(soap, ONVIF_ANALYTICS_STRING_TYPE);
                }            

                pRuleDescription->Messages->Key = soap_new_tt__ItemListDescription(soap,-1);
                pRuleDescription->Messages->Key->__sizeSimpleItemDescription = 1;
                pRuleDescription->Messages->Key->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap,1);
                struct _tt__ItemListDescription_SimpleItemDescription *pKey_0 = pRuleDescription->Messages->Key->SimpleItemDescription + 0;
                if(pKey_0 != NULL)
                {
                    pKey_0->Name = soap_strdup(soap, "ObjectId");
                    pKey_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }

                pRuleDescription->Messages->Data = soap_new_tt__ItemListDescription(soap,-1);
                pRuleDescription->Messages->Data->__sizeSimpleItemDescription = 1;
                pRuleDescription->Messages->Data->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap,1);
                struct _tt__ItemListDescription_SimpleItemDescription *pDate_0 = pRuleDescription->Messages->Data->SimpleItemDescription + 0;
                if(pDate_0 != NULL)
                {
                    pDate_0->Name = soap_strdup(soap, FIELD_NAME);
                    pDate_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_BOOL_TYPE);
                }
            
                break;
            }   
                
            case TRIPWIRE_ALARM:
            {
                pRuleDescription->Name = soap_strdup(soap,LINE_EVENT_RULE_TYPE);
                /* 最大配置数量 */
                pRuleDescription->maxInstances = soap_strdup(soap,"4");
                pRuleDescription->fixed = soap_new_xsd__boolean(soap,-1);
                *(pRuleDescription->fixed) = xsd__boolean__false_;
                pRuleDescription->Parameters = soap_new_tt__ItemListDescription(soap,-1);
                pRuleDescription->Parameters->__sizeSimpleItemDescription = 1;
                pRuleDescription->Parameters->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap,-1);
                pRuleDescription->Parameters->__sizeElementItemDescription = 1;
                pRuleDescription->Parameters->ElementItemDescription = soap_new__tt__ItemListDescription_ElementItemDescription(soap,-1);
                struct _tt__ItemListDescription_SimpleItemDescription *pParameters_0 = pRuleDescription->Parameters->SimpleItemDescription + 0;
                struct _tt__ItemListDescription_ElementItemDescription *pParameters_2 = pRuleDescription->Parameters->ElementItemDescription + 0;
                if(pParameters_0 != NULL)
                {
                    pParameters_0->Name = soap_strdup(soap, "Direction");
                    pParameters_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_DIRECTION_TYPE);
                }

                if(pParameters_2 != NULL)
                {
                    pParameters_2->Name = soap_strdup(soap, "Segments");
                    pParameters_2->Type = soap_strdup(soap, ONVIF_ANALYTICS_POLYLINE_TYPE);
                }

                pRuleDescription->__sizeMessages = 1;
                pRuleDescription->Messages = soap_new__tt__ConfigDescription_Messages(soap,-1);
                pRuleDescription->Messages ->ParentTopic = soap_strdup(soap, LINE_EVENT_THEME);
                pRuleDescription->Messages ->IsProperty = soap_new_xsd__boolean(soap,-1);
                *(pRuleDescription->Messages ->IsProperty) = xsd__boolean__true_;
                pRuleDescription->Messages->Source = soap_new_tt__ItemListDescription(soap,-1);
                pRuleDescription->Messages->Source->__sizeSimpleItemDescription = 3;
                pRuleDescription->Messages->Source->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap,3);
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_0 = pRuleDescription->Messages->Source->SimpleItemDescription + 0;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_1 = pRuleDescription->Messages->Source->SimpleItemDescription + 1;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_2 = pRuleDescription->Messages->Source->SimpleItemDescription + 2;
                if(pSource_0 != NULL)
                {
                    pSource_0->Name = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
                    pSource_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_1 != NULL)
                {
                    pSource_1->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN);
                    pSource_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_2 != NULL)
                {
                    pSource_2->Name = soap_strdup(soap, "Rule");
                    pSource_2->Type = soap_strdup(soap, ONVIF_ANALYTICS_STRING_TYPE);
                }            


                pRuleDescription->Messages->Data = soap_new_tt__ItemListDescription(soap,-1);
                pRuleDescription->Messages->Data->__sizeSimpleItemDescription = 1;
                pRuleDescription->Messages->Data->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap,1);
                struct _tt__ItemListDescription_SimpleItemDescription *pDate_0 = pRuleDescription->Messages->Data->SimpleItemDescription + 0;
                if(pDate_0 != NULL)
                {
                    pDate_0->Name = soap_strdup(soap, "ObjectId");
                    pDate_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }
                break;
            } 
                
            
            case ONVIF_ENTER_REGION:
            {
                pRuleDescription->Name = soap_strdup(soap, ENTER_REGION_EVENT_RULE_TYPE);
                pRuleDescription->maxInstances = soap_strdup(soap, "4");
                pRuleDescription->fixed = soap_new_xsd__boolean(soap, -1);
                *(pRuleDescription->fixed) = xsd__boolean__false_;
                
                /* Parameters */
                pRuleDescription->Parameters = soap_new_tt__ItemListDescription(soap, -1);
                pRuleDescription->Parameters->__sizeSimpleItemDescription = 0; // Removing Sensitivity
                pRuleDescription->Parameters->SimpleItemDescription = NULL;
                
                pRuleDescription->Parameters->__sizeElementItemDescription = 1;
                pRuleDescription->Parameters->ElementItemDescription = soap_new__tt__ItemListDescription_ElementItemDescription(soap, 1);
                struct _tt__ItemListDescription_ElementItemDescription *pElem_0 = pRuleDescription->Parameters->ElementItemDescription + 0;
                if (pElem_0 != NULL) {
                    pElem_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                    pElem_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_POLYGON_TYPE);
                }

                /* Messages */
                pRuleDescription->__sizeMessages = 1;
                pRuleDescription->Messages = soap_new__tt__ConfigDescription_Messages(soap, -1);
                pRuleDescription->Messages->ParentTopic = soap_strdup(soap, ENTER_REGION_EVENT_THEME);
                pRuleDescription->Messages->IsProperty = soap_new_xsd__boolean(soap, -1);
                *(pRuleDescription->Messages->IsProperty) = xsd__boolean__true_;
                
                pRuleDescription->Messages->Data = soap_new_tt__ItemListDescription(soap, -1);
                pRuleDescription->Messages->Data->__sizeSimpleItemDescription = 1;
                pRuleDescription->Messages->Data->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pData_0 = pRuleDescription->Messages->Data->SimpleItemDescription + 0;
                if (pData_0 != NULL) {
                    pData_0->Name = soap_strdup(soap, ENTER_REGION_NAME);
                    pData_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_BOOL_TYPE);
                }
                break;
            }

            case ONVIF_LEAVE_REGION:
            {
                pRuleDescription->Name = soap_strdup(soap, LEAVE_REGION_EVENT_RULE_TYPE);
                pRuleDescription->maxInstances = soap_strdup(soap, "4");
                pRuleDescription->fixed = soap_new_xsd__boolean(soap, -1);
                *(pRuleDescription->fixed) = xsd__boolean__false_;
                
                pRuleDescription->Parameters = soap_new_tt__ItemListDescription(soap, -1);
                pRuleDescription->Parameters->__sizeSimpleItemDescription = 0; // Removing Sensitivity
                pRuleDescription->Parameters->SimpleItemDescription = NULL;
          
                pRuleDescription->Parameters->__sizeElementItemDescription = 1;
                pRuleDescription->Parameters->ElementItemDescription = soap_new__tt__ItemListDescription_ElementItemDescription(soap, 1);
                struct _tt__ItemListDescription_ElementItemDescription *pElem_0 = pRuleDescription->Parameters->ElementItemDescription + 0;
                if (pElem_0 != NULL) {
                    pElem_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                    pElem_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_POLYGON_TYPE);
                }

                pRuleDescription->__sizeMessages = 1;
                pRuleDescription->Messages = soap_new__tt__ConfigDescription_Messages(soap, -1);
                pRuleDescription->Messages->ParentTopic = soap_strdup(soap, LEAVE_REGION_EVENT_THEME);
                pRuleDescription->Messages->IsProperty = soap_new_xsd__boolean(soap, -1);
                *(pRuleDescription->Messages->IsProperty) = xsd__boolean__true_;
                
                pRuleDescription->Messages->Data = soap_new_tt__ItemListDescription(soap, -1);
                pRuleDescription->Messages->Data->__sizeSimpleItemDescription = 1;
                pRuleDescription->Messages->Data->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pData_0 = pRuleDescription->Messages->Data->SimpleItemDescription + 0;
                if (pData_0 != NULL) {
                    pData_0->Name = soap_strdup(soap, LEAVE_REGION_NAME);
                    pData_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_BOOL_TYPE);
                }
                break;
            }

            case ONVIF_AUDIO_ANOMALY:
            {
                pRuleDescription->Name = soap_strdup(soap, AUDIO_ANOMALY_EVENT_RULE_TYPE);
                pRuleDescription->maxInstances = soap_strdup(soap, "1");
                pRuleDescription->fixed = soap_new_xsd__boolean(soap, -1);
                *(pRuleDescription->fixed) = xsd__boolean__false_;
                
                pRuleDescription->Parameters = soap_new_tt__ItemListDescription(soap, -1);
                pRuleDescription->Parameters->__sizeSimpleItemDescription = 1;
                pRuleDescription->Parameters->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pParam_1 = pRuleDescription->Parameters->SimpleItemDescription + 0;
                if (pParam_1 != NULL) {
                    pParam_1->Name = soap_strdup(soap, "Threshold");
                    pParam_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }

                pRuleDescription->__sizeMessages = 1;
                pRuleDescription->Messages = soap_new__tt__ConfigDescription_Messages(soap, -1);
                pRuleDescription->Messages->ParentTopic = soap_strdup(soap, AUDIO_ANOMALY_EVENT_THEME);
                pRuleDescription->Messages->IsProperty = soap_new_xsd__boolean(soap, -1);
                *(pRuleDescription->Messages->IsProperty) = xsd__boolean__true_;
                
                pRuleDescription->Messages->Data = soap_new_tt__ItemListDescription(soap, -1);
                pRuleDescription->Messages->Data->__sizeSimpleItemDescription = 1;
                pRuleDescription->Messages->Data->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pData_0 = pRuleDescription->Messages->Data->SimpleItemDescription + 0;
                if (pData_0 != NULL) {
                    pData_0->Name = soap_strdup(soap, AUDIO_ANOMALY_NAME);
                    pData_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_BOOL_TYPE);
                }
                break;
            }
            
            case ONVIF_SCENE_CHANGE:
            {
                pRuleDescription->Name = soap_strdup(soap, SCENE_CHANGE_EVENT_RULE_TYPE);
                pRuleDescription->maxInstances = soap_strdup(soap, "1");
                pRuleDescription->fixed = soap_new_xsd__boolean(soap, -1);
                *(pRuleDescription->fixed) = xsd__boolean__false_;
                
                pRuleDescription->Parameters = soap_new_tt__ItemListDescription(soap, -1);
                pRuleDescription->Parameters->__sizeSimpleItemDescription = 0;
                pRuleDescription->Parameters->SimpleItemDescription = NULL;

                pRuleDescription->__sizeMessages = 1;
                pRuleDescription->Messages = soap_new__tt__ConfigDescription_Messages(soap, -1);
                pRuleDescription->Messages->ParentTopic = soap_strdup(soap, SCENE_CHANGE_EVENT_THEME);
                pRuleDescription->Messages->IsProperty = soap_new_xsd__boolean(soap, -1);
                *(pRuleDescription->Messages->IsProperty) = xsd__boolean__true_;
                
                pRuleDescription->Messages->Data = soap_new_tt__ItemListDescription(soap, -1);
                pRuleDescription->Messages->Data->__sizeSimpleItemDescription = 1;
                pRuleDescription->Messages->Data->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pData_0 = pRuleDescription->Messages->Data->SimpleItemDescription + 0;
                if (pData_0 != NULL) {
                    pData_0->Name = soap_strdup(soap, SCENE_CHANGE_NAME);
                    pData_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_BOOL_TYPE);
                }
                break;
            }

            case ONVIF_FACE_DETECT:
            {
                pRuleDescription->Name = soap_strdup(soap, FACE_DETECT_EVENT_RULE_TYPE);
                pRuleDescription->maxInstances = soap_strdup(soap, "1");
                pRuleDescription->fixed = soap_new_xsd__boolean(soap, -1);
                *(pRuleDescription->fixed) = xsd__boolean__false_;
                
                pRuleDescription->Parameters = soap_new_tt__ItemListDescription(soap, -1);
                pRuleDescription->Parameters->__sizeSimpleItemDescription = 0;
                pRuleDescription->Parameters->SimpleItemDescription = NULL;

                 pRuleDescription->Parameters->__sizeElementItemDescription = 1;
                pRuleDescription->Parameters->ElementItemDescription = soap_new__tt__ItemListDescription_ElementItemDescription(soap, 1);
                struct _tt__ItemListDescription_ElementItemDescription *pElem_0 = pRuleDescription->Parameters->ElementItemDescription + 0;
                if (pElem_0 != NULL) {
                    pElem_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                    pElem_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_POLYGON_TYPE);
                }

                pRuleDescription->__sizeMessages = 1;
                pRuleDescription->Messages = soap_new__tt__ConfigDescription_Messages(soap, -1);
                pRuleDescription->Messages->ParentTopic = soap_strdup(soap, FACE_DETECT_EVENT_THEME);
                pRuleDescription->Messages->IsProperty = soap_new_xsd__boolean(soap, -1);
                *(pRuleDescription->Messages->IsProperty) = xsd__boolean__true_;
                
                pRuleDescription->Messages->Data = soap_new_tt__ItemListDescription(soap, -1);
                pRuleDescription->Messages->Data->__sizeSimpleItemDescription = 1;
                pRuleDescription->Messages->Data->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pData_0 = pRuleDescription->Messages->Data->SimpleItemDescription + 0;
                if (pData_0 != NULL) {
                    pData_0->Name = soap_strdup(soap, FACE_DETECT_NAME);
                    pData_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_BOOL_TYPE);
                }
                break;
            }

            case ONVIF_LOITERING_DETECT:
            {
                pRuleDescription->Name = soap_strdup(soap, LOITERING_DETECT_EVENT_RULE_TYPE);
                pRuleDescription->maxInstances = soap_strdup(soap, "4");
                pRuleDescription->fixed = soap_new_xsd__boolean(soap, -1);
                *(pRuleDescription->fixed) = xsd__boolean__false_;
                
                pRuleDescription->Parameters = soap_new_tt__ItemListDescription(soap, -1);
                pRuleDescription->Parameters->__sizeSimpleItemDescription = 1;
                pRuleDescription->Parameters->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pParam_0 = pRuleDescription->Parameters->SimpleItemDescription + 0;
                
                if (pParam_0 != NULL) {
                    pParam_0->Name = soap_strdup(soap, "TimeThreshold");
                    pParam_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }
                pRuleDescription->Parameters->__sizeElementItemDescription = 1;
                pRuleDescription->Parameters->ElementItemDescription = soap_new__tt__ItemListDescription_ElementItemDescription(soap, 1);
                struct _tt__ItemListDescription_ElementItemDescription *pElem_0 = pRuleDescription->Parameters->ElementItemDescription + 0;
                if (pElem_0 != NULL) {
                    pElem_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                    pElem_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_POLYGON_TYPE);
                }

                pRuleDescription->__sizeMessages = 1;
                pRuleDescription->Messages = soap_new__tt__ConfigDescription_Messages(soap, -1);
                pRuleDescription->Messages->ParentTopic = soap_strdup(soap, LOITERING_DETECT_EVENT_THEME);
                pRuleDescription->Messages->IsProperty = soap_new_xsd__boolean(soap, -1);
                *(pRuleDescription->Messages->IsProperty) = xsd__boolean__true_;
                
                pRuleDescription->Messages->Data = soap_new_tt__ItemListDescription(soap, -1);
                pRuleDescription->Messages->Data->__sizeSimpleItemDescription = 1;
                pRuleDescription->Messages->Data->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pData_0 = pRuleDescription->Messages->Data->SimpleItemDescription + 0;
                if (pData_0 != NULL) {
                    pData_0->Name = soap_strdup(soap, LOITERING_DETECT_NAME);
                    pData_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_BOOL_TYPE);
                }
                break;
            }

            case ONVIF_CROWD_GATHERING:
            {
                pRuleDescription->Name = soap_strdup(soap, CROWD_GATHERING_EVENT_RULE_TYPE);
                pRuleDescription->maxInstances = soap_strdup(soap, "4");
                pRuleDescription->fixed = soap_new_xsd__boolean(soap, -1);
                *(pRuleDescription->fixed) = xsd__boolean__false_;
                
                pRuleDescription->Parameters = soap_new_tt__ItemListDescription(soap, -1);
                pRuleDescription->Parameters->__sizeSimpleItemDescription = 1;
                pRuleDescription->Parameters->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pParam_0 = pRuleDescription->Parameters->SimpleItemDescription + 0;
                
                if (pParam_0 != NULL) {
                    pParam_0->Name = soap_strdup(soap, "ObjectOccup");
                    pParam_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }
                pRuleDescription->Parameters->__sizeElementItemDescription = 1;
                pRuleDescription->Parameters->ElementItemDescription = soap_new__tt__ItemListDescription_ElementItemDescription(soap, 1);
                struct _tt__ItemListDescription_ElementItemDescription *pElem_0 = pRuleDescription->Parameters->ElementItemDescription + 0;
                if (pElem_0 != NULL) {
                    pElem_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                    pElem_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_POLYGON_TYPE);
                }

                pRuleDescription->__sizeMessages = 1;
                pRuleDescription->Messages = soap_new__tt__ConfigDescription_Messages(soap, -1);
                pRuleDescription->Messages->ParentTopic = soap_strdup(soap, CROWD_GATHERING_EVENT_THEME);
                pRuleDescription->Messages->IsProperty = soap_new_xsd__boolean(soap, -1);
                *(pRuleDescription->Messages->IsProperty) = xsd__boolean__true_;
                
                pRuleDescription->Messages->Data = soap_new_tt__ItemListDescription(soap, -1);
                pRuleDescription->Messages->Data->__sizeSimpleItemDescription = 1;
                pRuleDescription->Messages->Data->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pData_0 = pRuleDescription->Messages->Data->SimpleItemDescription + 0;
                if (pData_0 != NULL) {
                    pData_0->Name = soap_strdup(soap, CROWD_GATHERING_NAME);
                    pData_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_BOOL_TYPE);
                }
                break;
            }

            case ONVIF_PARKING_DETECT:
            {
                pRuleDescription->Name = soap_strdup(soap, PARKING_DETECT_EVENT_RULE_TYPE);
                pRuleDescription->maxInstances = soap_strdup(soap, "4");
                pRuleDescription->fixed = soap_new_xsd__boolean(soap, -1);
                *(pRuleDescription->fixed) = xsd__boolean__false_;
                
                pRuleDescription->Parameters = soap_new_tt__ItemListDescription(soap, -1);
                pRuleDescription->Parameters->__sizeSimpleItemDescription = 1;
                pRuleDescription->Parameters->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pParam_0 = pRuleDescription->Parameters->SimpleItemDescription + 0;
                
                if (pParam_0 != NULL) {
                    pParam_0->Name = soap_strdup(soap, "TimeThreshold");
                    pParam_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }
                 pRuleDescription->Parameters->__sizeElementItemDescription = 1;
                pRuleDescription->Parameters->ElementItemDescription = soap_new__tt__ItemListDescription_ElementItemDescription(soap, 1);
                struct _tt__ItemListDescription_ElementItemDescription *pElem_0 = pRuleDescription->Parameters->ElementItemDescription + 0;
                if (pElem_0 != NULL) {
                    pElem_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                    pElem_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_POLYGON_TYPE);
                }

                pRuleDescription->__sizeMessages = 1;
                pRuleDescription->Messages = soap_new__tt__ConfigDescription_Messages(soap, -1);
                pRuleDescription->Messages->ParentTopic = soap_strdup(soap, PARKING_DETECT_EVENT_THEME);
                pRuleDescription->Messages->IsProperty = soap_new_xsd__boolean(soap, -1);
                *(pRuleDescription->Messages->IsProperty) = xsd__boolean__true_;
                
                pRuleDescription->Messages->Data = soap_new_tt__ItemListDescription(soap, -1);
                pRuleDescription->Messages->Data->__sizeSimpleItemDescription = 1;
                pRuleDescription->Messages->Data->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pData_0 = pRuleDescription->Messages->Data->SimpleItemDescription + 0;
                if (pData_0 != NULL) {
                    pData_0->Name = soap_strdup(soap, PARKING_DETECT_NAME);
                    pData_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_BOOL_TYPE);
                }
                break;
            }

            case ONVIF_UNATTENDED_OBJECT:
            {
                pRuleDescription->Name = soap_strdup(soap, UNATTENDED_OBJECT_EVENT_RULE_TYPE);
                pRuleDescription->maxInstances = soap_strdup(soap, "4");
                pRuleDescription->fixed = soap_new_xsd__boolean(soap, -1);
                *(pRuleDescription->fixed) = xsd__boolean__false_;
                
                pRuleDescription->Parameters = soap_new_tt__ItemListDescription(soap, -1);
                pRuleDescription->Parameters->__sizeSimpleItemDescription = 1;
                pRuleDescription->Parameters->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pParam_0 = pRuleDescription->Parameters->SimpleItemDescription + 0;
                
                if (pParam_0 != NULL) {
                    pParam_0->Name = soap_strdup(soap, "TimeThreshold");
                    pParam_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }
                 pRuleDescription->Parameters->__sizeElementItemDescription = 1;
                pRuleDescription->Parameters->ElementItemDescription = soap_new__tt__ItemListDescription_ElementItemDescription(soap, 1);
                struct _tt__ItemListDescription_ElementItemDescription *pElem_0 = pRuleDescription->Parameters->ElementItemDescription + 0;
                if (pElem_0 != NULL) {
                    pElem_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                    pElem_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_POLYGON_TYPE);
                }

                pRuleDescription->__sizeMessages = 1;
                pRuleDescription->Messages = soap_new__tt__ConfigDescription_Messages(soap, -1);
                pRuleDescription->Messages->ParentTopic = soap_strdup(soap, UNATTENDED_OBJECT_EVENT_THEME);
                pRuleDescription->Messages->IsProperty = soap_new_xsd__boolean(soap, -1);
                *(pRuleDescription->Messages->IsProperty) = xsd__boolean__true_;
                
                pRuleDescription->Messages->Data = soap_new_tt__ItemListDescription(soap, -1);
                pRuleDescription->Messages->Data->__sizeSimpleItemDescription = 1;
                pRuleDescription->Messages->Data->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pData_0 = pRuleDescription->Messages->Data->SimpleItemDescription + 0;
                if (pData_0 != NULL) {
                    pData_0->Name = soap_strdup(soap, UNATTENDED_OBJECT_NAME);
                    pData_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_BOOL_TYPE);
                }
                break;
            }

            case ONVIF_OBJECT_REMOVAL:
            {
                pRuleDescription->Name = soap_strdup(soap, OBJECT_REMOVAL_EVENT_RULE_TYPE);
                pRuleDescription->maxInstances = soap_strdup(soap, "4");
                pRuleDescription->fixed = soap_new_xsd__boolean(soap, -1);
                *(pRuleDescription->fixed) = xsd__boolean__false_;
                
                pRuleDescription->Parameters = soap_new_tt__ItemListDescription(soap, -1);
                pRuleDescription->Parameters->__sizeSimpleItemDescription = 1;
                pRuleDescription->Parameters->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pParam_0 = pRuleDescription->Parameters->SimpleItemDescription + 0;
                
                if (pParam_0 != NULL) {
                    pParam_0->Name = soap_strdup(soap, "TimeThreshold");
                    pParam_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }
                 pRuleDescription->Parameters->__sizeElementItemDescription = 1;
                pRuleDescription->Parameters->ElementItemDescription = soap_new__tt__ItemListDescription_ElementItemDescription(soap, 1);
                struct _tt__ItemListDescription_ElementItemDescription *pElem_0 = pRuleDescription->Parameters->ElementItemDescription + 0;
                if (pElem_0 != NULL) {
                    pElem_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                    pElem_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_POLYGON_TYPE);
                }

                pRuleDescription->__sizeMessages = 1;
                pRuleDescription->Messages = soap_new__tt__ConfigDescription_Messages(soap, -1);
                pRuleDescription->Messages->ParentTopic = soap_strdup(soap, OBJECT_REMOVAL_EVENT_THEME);
                pRuleDescription->Messages->IsProperty = soap_new_xsd__boolean(soap, -1);
                *(pRuleDescription->Messages->IsProperty) = xsd__boolean__true_;
                
                pRuleDescription->Messages->Data = soap_new_tt__ItemListDescription(soap, -1);
                pRuleDescription->Messages->Data->__sizeSimpleItemDescription = 1;
                pRuleDescription->Messages->Data->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pData_0 = pRuleDescription->Messages->Data->SimpleItemDescription + 0;
                if (pData_0 != NULL) {
                    pData_0->Name = soap_strdup(soap, OBJECT_REMOVAL_NAME);
                    pData_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_BOOL_TYPE);
                }
                break;
            }

            case ONVIF_PET_RECOGNITION:
            {
                pRuleDescription->Name = soap_strdup(soap, PET_RECOGNITION_EVENT_RULE_TYPE);
                pRuleDescription->maxInstances = soap_strdup(soap, "1");
                pRuleDescription->fixed = soap_new_xsd__boolean(soap, -1);
                *(pRuleDescription->fixed) = xsd__boolean__false_;
                
                pRuleDescription->Parameters = soap_new_tt__ItemListDescription(soap, -1);
                pRuleDescription->Parameters->__sizeSimpleItemDescription = 0;
                pRuleDescription->Parameters->SimpleItemDescription = NULL;
                 pRuleDescription->Parameters->__sizeElementItemDescription = 1;
                pRuleDescription->Parameters->ElementItemDescription = soap_new__tt__ItemListDescription_ElementItemDescription(soap, 1);
                struct _tt__ItemListDescription_ElementItemDescription *pElem_0 = pRuleDescription->Parameters->ElementItemDescription + 0;
                if (pElem_0 != NULL) {
                    pElem_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                    pElem_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_POLYGON_TYPE);
                }

                pRuleDescription->__sizeMessages = 1;
                pRuleDescription->Messages = soap_new__tt__ConfigDescription_Messages(soap, -1);
                pRuleDescription->Messages->ParentTopic = soap_strdup(soap, PET_RECOGNITION_EVENT_THEME);
                pRuleDescription->Messages->IsProperty = soap_new_xsd__boolean(soap, -1);
                *(pRuleDescription->Messages->IsProperty) = xsd__boolean__true_;
                
                pRuleDescription->Messages->Data = soap_new_tt__ItemListDescription(soap, -1);
                pRuleDescription->Messages->Data->__sizeSimpleItemDescription = 1;
                pRuleDescription->Messages->Data->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pData_0 = pRuleDescription->Messages->Data->SimpleItemDescription + 0;
                if (pData_0 != NULL) {
                    pData_0->Name = soap_strdup(soap, PET_RECOGNITION_NAME);
                    pData_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_BOOL_TYPE);
                }
                break;
            }

            case ONVIF_FACE_CAPTURE:
            {
                pRuleDescription->Name = soap_strdup(soap, FACE_CAPTURE_EVENT_RULE_TYPE);
                pRuleDescription->maxInstances = soap_strdup(soap, "1");
                pRuleDescription->fixed = soap_new_xsd__boolean(soap, -1);
                *(pRuleDescription->fixed) = xsd__boolean__false_;
                
                pRuleDescription->Parameters = soap_new_tt__ItemListDescription(soap, -1);
                pRuleDescription->Parameters->__sizeSimpleItemDescription = 1;
                pRuleDescription->Parameters->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pParam_0 = pRuleDescription->Parameters->SimpleItemDescription + 0;
                
                if (pParam_0 != NULL) {
                    pParam_0->Name = soap_strdup(soap, "Interval");
                    pParam_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }
                 pRuleDescription->Parameters->__sizeElementItemDescription = 1;
                pRuleDescription->Parameters->ElementItemDescription = soap_new__tt__ItemListDescription_ElementItemDescription(soap, 1);
                struct _tt__ItemListDescription_ElementItemDescription *pElem_0 = pRuleDescription->Parameters->ElementItemDescription + 0;
                if (pElem_0 != NULL) {
                    pElem_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                    pElem_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_POLYGON_TYPE);
                }

                pRuleDescription->__sizeMessages = 1;
                pRuleDescription->Messages = soap_new__tt__ConfigDescription_Messages(soap, -1);
                pRuleDescription->Messages->ParentTopic = soap_strdup(soap, FACE_CAPTURE_EVENT_THEME);
                pRuleDescription->Messages->IsProperty = soap_new_xsd__boolean(soap, -1);
                *(pRuleDescription->Messages->IsProperty) = xsd__boolean__true_;
                
                pRuleDescription->Messages->Data = soap_new_tt__ItemListDescription(soap, -1);
                pRuleDescription->Messages->Data->__sizeSimpleItemDescription = 1;
                pRuleDescription->Messages->Data->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pData_0 = pRuleDescription->Messages->Data->SimpleItemDescription + 0;
                if (pData_0 != NULL) {
                    pData_0->Name = soap_strdup(soap, FACE_CAPTURE_NAME);
                    pData_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_BOOL_TYPE);
                }
                break;
            }

            default:
                break;
        }
        
    }

    return SOAP_OK;
}

/** Web service operation '__tan__CreateRules' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tan__CreateRules(struct soap* soap, struct _tan__CreateRules *tan__CreateRules, struct _tan__CreateRulesResponse *tan__CreateRulesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tan__CreateRules----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tan__DeleteRules' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tan__DeleteRules(struct soap* soap, struct _tan__DeleteRules *tan__DeleteRules, struct _tan__DeleteRulesResponse *tan__DeleteRulesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tan__DeleteRules----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tan__GetRules' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tan__GetRules(struct soap* soap, struct _tan__GetRules *tan__GetRules, struct _tan__GetRulesResponse *tan__GetRulesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tan__GetRules----------");
#endif

    int nRet = onvif_authentication(soap);
    if(SOAP_OK != nRet)
    {
        return nRet;
    }
    /* token校验 */
    if(tan__GetRules != NULL && tan__GetRules->ConfigurationToken != NULL)
    {
        if(strcmp(tan__GetRules->ConfigurationToken,VIDEOANALTICS_TOKEN) != 0)
        {
            
            dlog_error("requested  VIDEOANALTICS_TOKEN does not exist");
            return soap_receiver_fault(soap, "requested VIDEOANALTICS_TOKEN does not exist", NULL); 
        }
    }
    else
    {
        return soap_receiver_fault(soap, "requested VIDEOANALTICS_TOKEN does not exist", NULL); 
    }

    if(tan__GetRulesResponse == NULL)
    {
        dlog_error("tan__GetRulesResponse is NULL");
        return soap_receiver_fault(soap, "tan__GetRulesResponse is NULL", NULL);    
    }

    /* Count all rules dynamicially */
    int count_motion = 1; /* Always 1 for now */
    int count_tamper = 1; /* Always 1 for now */
    int count_enter = onvif_get_enter_region_count();
    int count_leave = onvif_get_leave_region_count();
    int count_audio = onvif_get_audio_anomaly_count();
    int count_scene = onvif_get_scene_change_count();
    int count_face = onvif_get_face_detect_count();
    int count_loitering = onvif_get_loitering_detect_count();
    int count_crowd = onvif_get_crowd_gathering_count();
    int count_parking = onvif_get_parking_detect_count();
    int count_unattended = onvif_get_unattended_object_count();
    int count_removal = onvif_get_object_removal_count();
    int count_pet = onvif_get_pet_recognition_count();
    int count_facecap = onvif_get_face_capture_count();
    int count_tripwire = onvif_get_tripwire_count();
    int count_intrusion = onvif_get_intrusion_count();

    /* Count active types for total rules */
    int total_rules = count_motion + count_tamper + 
                      (count_enter > 0 ? 1 : 0) + 
                      (count_leave > 0 ? 1 : 0) + 
                      count_audio + count_scene + count_face + 
                      (count_loitering > 0 ? 1 : 0) + 
                      (count_crowd > 0 ? 1 : 0) + 
                      (count_parking > 0 ? 1 : 0) + 
                      (count_unattended > 0 ? 1 : 0) + 
                      (count_removal > 0 ? 1 : 0) + 
                      count_pet + 
                      (count_facecap > 0 ? 1 : 0) + // FaceCapture might be singleton, check later
                      (count_tripwire > 0 ? 1 : 0) + 
                      (count_intrusion > 0 ? 1 : 0);

    tan__GetRulesResponse->__sizeRule = total_rules;
    tan__GetRulesResponse->Rule = soap_new_tt__Config(soap, total_rules);

    int rule_idx = 0;

    /* Motion Detection */
    for(int i = 0; i < count_motion; i++)
    {
        struct tt__Config *pRule = tan__GetRulesResponse->Rule + rule_idx++;
        OnvifMotionDetection_S stInfo;
        onvif_get_motion_info(&stInfo);
        
        pRule->Name = soap_strdup(soap, MOTION_EVENT_RULE);
        pRule->Type = soap_strdup(soap, MOTION_EVENT_RULE_TYPE);
        
        pRule->Parameters = soap_new_tt__ItemList(soap,-1);
        pRule->Parameters->__sizeSimpleItem = ONVIF_CELLMOTION_RULEPARAM_NUM + 1; /* +1 for Enable */
        pRule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap, pRule->Parameters->__sizeSimpleItem);
        
        struct _tt__ItemList_SimpleItem *pRuleParam_0 = pRule->Parameters->SimpleItem + 0;
        struct _tt__ItemList_SimpleItem *pRuleParam_1 = pRule->Parameters->SimpleItem + 1;
        struct _tt__ItemList_SimpleItem *pRuleParam_2 = pRule->Parameters->SimpleItem + 2;
        struct _tt__ItemList_SimpleItem *pRuleParam_3 = pRule->Parameters->SimpleItem + 3;
        struct _tt__ItemList_SimpleItem *pRuleParam_Enable = pRule->Parameters->SimpleItem + 4;

        if(pRuleParam_0) { pRuleParam_0->Name = soap_strdup(soap, ONVIF_CELLMOTION_RULEPARAM_1); pRuleParam_0->Value = soap_strdup(soap, ONVIF_CELLMOTION_RULEPARAM_VALUE1); }
        if(pRuleParam_1) { pRuleParam_1->Name = soap_strdup(soap, ONVIF_CELLMOTION_RULEPARAM_2); pRuleParam_1->Value = soap_strdup(soap, ONVIF_CELLMOTION_RULEPARAM_VALUE2); }
        if(pRuleParam_2) { pRuleParam_2->Name = soap_strdup(soap, ONVIF_CELLMOTION_RULEPARAM_3); pRuleParam_2->Value = soap_strdup(soap, ONVIF_CELLMOTION_RULEPARAM_VALUE3); }
        if(pRuleParam_3) { pRuleParam_3->Name = soap_strdup(soap, ONVIF_CELLMOTION_RULEPARAM_4); pRuleParam_3->Value = soap_strdup(soap, stInfo.achBaseStr); }
        
        if(pRuleParam_Enable) {
            pRuleParam_Enable->Name = soap_strdup(soap, "Enable");
            pRuleParam_Enable->Value = soap_strdup(soap, stInfo.bEnable ? "true" : "false");
        }

        /* Add Schedule ElementItem */
        pRule->Parameters->__sizeElementItem = 1;
        pRule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap, 1);
        struct _tt__ItemList_ElementItem *pElem = pRule->Parameters->ElementItem + 0;
        if(pElem) {
            pElem->Name = soap_strdup(soap, "Schedule");
            pElem->__any = soap_strdup(soap, stInfo.achSchedule);
        }
    }

    /* Tamper Detection */
    for(int i = 0; i < count_tamper; i++)
    {
        struct tt__Config *pRule = tan__GetRulesResponse->Rule + rule_idx++;
        ONvifTamperDetection_S stInfo;
        onvif_get_tamp_info(&stInfo);
        
        pRule->Name = soap_strdup(soap, TAMPEREVENT_RULE);
        pRule->Type = soap_strdup(soap, TAMPEREVENT_RULE_TYPE);
        
        pRule->Parameters = soap_new_tt__ItemList(soap,-1);
        pRule->Parameters->__sizeSimpleItem = 1; /* Enable */
        pRule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap, 1);
        
        struct _tt__ItemList_SimpleItem *pRuleParam_Enable = pRule->Parameters->SimpleItem + 0;
        if(pRuleParam_Enable) {
            pRuleParam_Enable->Name = soap_strdup(soap, "Enable");
            pRuleParam_Enable->Value = soap_strdup(soap, stInfo.bEnable ? "true" : "false");
        }

        pRule->Parameters->__sizeElementItem = ONVIF_TAMPER_RULEPARAM_NUM + 1;
        pRule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap, pRule->Parameters->__sizeElementItem);
        struct _tt__ItemList_ElementItem *pRuleParam_0 = pRule->Parameters->ElementItem + 0;
        struct _tt__ItemList_ElementItem *pRuleParam_1 = pRule->Parameters->ElementItem + 1;
        
        if(pRuleParam_0 != NULL)
        {
            char achPolygon[5120];
            pRuleParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
            snprintf(achPolygon, sizeof(achPolygon),
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
        
        if(pRuleParam_1 != NULL)
        {
            pRuleParam_1->Name = soap_strdup(soap, "Schedule");
            pRuleParam_1->__any = soap_strdup(soap, stInfo.achSchedule);
        }
    }
           
    /* Intrusion Detection */
    if(count_intrusion > 0)
    {
        struct tt__Config *pRule = tan__GetRulesResponse->Rule + rule_idx++;
        OnvifFieldDetection_S stInfo;
        onvif_get_intrusion_info(0, &stInfo); /* Use first region for shared params */
        
        pRule->Name = soap_strdup(soap, FIELD_EVENT_RULE);
        pRule->Type = soap_strdup(soap, FIELD_EVENT_RULE_TYPE);
        
        pRule->Parameters = soap_new_tt__ItemList(soap, -1);
        pRule->Parameters->__sizeSimpleItem = 1; /* Only Enable */
        pRule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap, 1);
        
        struct _tt__ItemList_SimpleItem *pParam_Enable = pRule->Parameters->SimpleItem + 0;
        
        if(pParam_Enable) {
             pParam_Enable->Name = soap_strdup(soap, "Enable");
             pParam_Enable->Value = soap_strdup(soap, stInfo.bEnable ? "true" : "false");
        }

        pRule->Parameters->__sizeElementItem = count_intrusion + 1;
        pRule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap, count_intrusion + 1);
        
        for(int k=0; k < count_intrusion; k++)
        {
             onvif_get_intrusion_info(k, &stInfo); /* Get info for each region */
             struct _tt__ItemList_ElementItem *pElem = pRule->Parameters->ElementItem + k;
             if(pElem != NULL) {
                  char achPolygon[4096];
                  pElem->Name = soap_strdup(soap, "Field");
                  char achPoints[3072] = {0};
                  for(int p=0; p<stInfo.nPointNum; p++) {
                      char temp[128];
                      snprintf(temp, sizeof(temp), "<tt:Point x=\"%d\" y=\"%d\"/>", stInfo.stPolygon[p].x, stInfo.stPolygon[p].y);
                      strcat(achPoints, temp);
                  }
                  char achTargetTag[256] = {0};
                  if(strlen(stInfo.achDetectionTarget) > 0) {
                      snprintf(achTargetTag, sizeof(achTargetTag), "<tt:DetectionTarget>%s</tt:DetectionTarget>", stInfo.achDetectionTarget);
                  }
                  snprintf(achPolygon, sizeof(achPolygon),
                      "<tt:PolygonConfiguration><tt:Polygon>%s</tt:Polygon><tt:Sensitivity>%s</tt:Sensitivity><tt:TimeThreshold>%s</tt:TimeThreshold>%s</tt:PolygonConfiguration>", achPoints, stInfo.achSensitivity, stInfo.achTimeThreshold, achTargetTag);
                  pElem->__any = soap_strdup(soap, achPolygon);
             }
        }
        /* Append shared Schedule as the last element */
        if(count_intrusion > 0) {
             onvif_get_intrusion_info(0, &stInfo); /* Get schedule from first index */
             struct _tt__ItemList_ElementItem *pElemSched = pRule->Parameters->ElementItem + count_intrusion;
             if(pElemSched != NULL) {
                  pElemSched->Name = soap_strdup(soap, "Schedule");
                  pElemSched->__any = soap_strdup(soap, stInfo.achSchedule);
             }
        }
    }

    /* Tripwire Detection */
    if(count_tripwire > 0)
    {
        struct tt__Config *pRule = tan__GetRulesResponse->Rule + rule_idx++;
        OnvifTripwireDetection_S stInfo;
        onvif_get_tripwire_info(0, &stInfo); /* Use first region for shared params */

        pRule->Name = soap_strdup(soap, LINE_EVENT_RULE);
        pRule->Type = soap_strdup(soap, LINE_EVENT_RULE_TYPE);
        
        pRule->Parameters = soap_new_tt__ItemList(soap, -1);
        pRule->Parameters->__sizeSimpleItem = 1; /* Only Enable */
        pRule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap, 1);
        
        struct _tt__ItemList_SimpleItem *pParam_Enable = pRule->Parameters->SimpleItem + 0;

        if(pParam_Enable) {
             pParam_Enable->Name = soap_strdup(soap, "Enable");
             pParam_Enable->Value = soap_strdup(soap, stInfo.bEnable ? "true" : "false");
        }

        pRule->Parameters->__sizeElementItem = count_tripwire + 1;
        pRule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap, count_tripwire + 1);
        
        for(int k=0; k < count_tripwire; k++)
        {
             onvif_get_tripwire_info(k, &stInfo);
             struct _tt__ItemList_ElementItem *pElem = pRule->Parameters->ElementItem + k;
             if(pElem != NULL) {
                  char achPolygon[4096];
                  pElem->Name = soap_strdup(soap, "Segments");
                  char achPoints[1024] = {0};
                  for(int p=0; p<stInfo.nPointNum; p++) {
                      char temp[128];
                      snprintf(temp, sizeof(temp), "<tt:Point x=\"%d\" y=\"%d\"/>", stInfo.stPolygon[p].x, stInfo.stPolygon[p].y);
                      strcat(achPoints, temp);
                  }
                   /* Embed Direction INSIDE Polyline (valid extension point) */
                   /* Embed Direction INSIDE Polyline (valid extension point) */
                   char achTargetTag[256] = {0};
                   if(strlen(stInfo.achDetectionTarget) > 0) {
                       snprintf(achTargetTag, sizeof(achTargetTag), "<tt:DetectionTarget>%s</tt:DetectionTarget>", stInfo.achDetectionTarget);
                   }
                   snprintf(achPolygon, sizeof(achPolygon), "<tt:Polyline>%s<tt:Sensitivity>%s</tt:Sensitivity><tt:Direction>%s</tt:Direction>%s</tt:Polyline>", achPoints, stInfo.achSensitivity, stInfo.achDirection, achTargetTag);
                   pElem->__any = soap_strdup(soap, achPolygon);
             }
        }
        /* Append shared Schedule */
        if(count_tripwire > 0) {
             onvif_get_tripwire_info(0, &stInfo);
             struct _tt__ItemList_ElementItem *pElemSched = pRule->Parameters->ElementItem + count_tripwire;
             if(pElemSched != NULL) {
                  pElemSched->Name = soap_strdup(soap, "Schedule");
                  pElemSched->__any = soap_strdup(soap, stInfo.achSchedule);
             }
        }
    }

    /* Enter Region */
    if(count_enter > 0)
    {
        struct tt__Config *pRule = tan__GetRulesResponse->Rule + rule_idx++;
        OnvifRegionDetection_S stInfo;
        onvif_get_enter_region_info(0, &stInfo);
        
        pRule->Name = soap_strdup(soap, ENTER_REGION_EVENT_RULE);
        pRule->Type = soap_strdup(soap, ENTER_REGION_EVENT_RULE_TYPE);
        
        pRule->Parameters = soap_new_tt__ItemList(soap, -1);
        pRule->Parameters->__sizeSimpleItem = 1; /* Only Enable */
        pRule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap, 1);
        
        struct _tt__ItemList_SimpleItem *pParam_Enable = pRule->Parameters->SimpleItem + 0;
        
        if(pParam_Enable) {
             pParam_Enable->Name = soap_strdup(soap, "Enable");
             pParam_Enable->Value = soap_strdup(soap, stInfo.bEnable ? "true" : "false");
        }

        pRule->Parameters->__sizeElementItem = count_enter + 1;
        pRule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap, count_enter + 1);
        
        for(int k=0; k < count_enter; k++)
        {
             onvif_get_enter_region_info(k, &stInfo);
             struct _tt__ItemList_ElementItem *pElem = pRule->Parameters->ElementItem + k;
             if(pElem != NULL) {
                  char achPolygon[5120];
                  pElem->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                  char achTargetTag[256] = {0};
                  if(strlen(stInfo.achDetectionTarget) > 0) {
                      snprintf(achTargetTag, sizeof(achTargetTag), "<tt:DetectionTarget>%s</tt:DetectionTarget>", stInfo.achDetectionTarget);
                  }
                  snprintf(achPolygon, sizeof(achPolygon),
                      "<tt:PolygonConfiguration><tt:Polygon>"
                      "<tt:Point x=\"%d\" y=\"%d\"/><tt:Point x=\"%d\" y=\"%d\"/><tt:Point x=\"%d\" y=\"%d\"/><tt:Point x=\"%d\" y=\"%d\"/>"
                      "</tt:Polygon><tt:Sensitivity>%s</tt:Sensitivity>%s</tt:PolygonConfiguration>",
                      stInfo.stPolygon[0].x, stInfo.stPolygon[0].y, stInfo.stPolygon[1].x, stInfo.stPolygon[1].y,
                      stInfo.stPolygon[2].x, stInfo.stPolygon[2].y, stInfo.stPolygon[3].x, stInfo.stPolygon[3].y, stInfo.achSensitivity, achTargetTag);
                  pElem->__any = soap_strdup(soap, achPolygon);
             }
        }
        if(count_enter > 0) {
             onvif_get_enter_region_info(0, &stInfo);
             struct _tt__ItemList_ElementItem *pElemSched = pRule->Parameters->ElementItem + count_enter;
             if(pElemSched != NULL) {
                  pElemSched->Name = soap_strdup(soap, "Schedule");
                  pElemSched->__any = soap_strdup(soap, stInfo.achSchedule);
             }
        }
    }

    /* Leave Region */
    if(count_leave > 0)
    {
        struct tt__Config *pRule = tan__GetRulesResponse->Rule + rule_idx++;
        OnvifRegionDetection_S stInfo;
        onvif_get_leave_region_info(0, &stInfo);
        
        pRule->Name = soap_strdup(soap, LEAVE_REGION_EVENT_RULE);
        pRule->Type = soap_strdup(soap, LEAVE_REGION_EVENT_RULE_TYPE);
        
        pRule->Parameters = soap_new_tt__ItemList(soap, -1);
        pRule->Parameters->__sizeSimpleItem = 1; /* Only Enable */
        pRule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap, 1);
        
        struct _tt__ItemList_SimpleItem *pParam_Enable = pRule->Parameters->SimpleItem + 0;
        
        if(pParam_Enable) {
             pParam_Enable->Name = soap_strdup(soap, "Enable");
             pParam_Enable->Value = soap_strdup(soap, stInfo.bEnable ? "true" : "false");
        }

        pRule->Parameters->__sizeElementItem = count_leave + 1;
        pRule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap, count_leave + 1);
        
        for(int k=0; k < count_leave; k++)
        {
             onvif_get_leave_region_info(k, &stInfo);
             struct _tt__ItemList_ElementItem *pElem = pRule->Parameters->ElementItem + k;
             if(pElem != NULL) {
                  char achPolygon[5120];
                  pElem->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                  char achTargetTag[256] = {0};
                  if(strlen(stInfo.achDetectionTarget) > 0) {
                      snprintf(achTargetTag, sizeof(achTargetTag), "<tt:DetectionTarget>%s</tt:DetectionTarget>", stInfo.achDetectionTarget);
                  }
                  snprintf(achPolygon, sizeof(achPolygon),
                      "<tt:PolygonConfiguration><tt:Polygon>"
                      "<tt:Point x=\"%d\" y=\"%d\"/><tt:Point x=\"%d\" y=\"%d\"/><tt:Point x=\"%d\" y=\"%d\"/><tt:Point x=\"%d\" y=\"%d\"/>"
                      "</tt:Polygon><tt:Sensitivity>%s</tt:Sensitivity>%s</tt:PolygonConfiguration>",
                      stInfo.stPolygon[0].x, stInfo.stPolygon[0].y, stInfo.stPolygon[1].x, stInfo.stPolygon[1].y,
                      stInfo.stPolygon[2].x, stInfo.stPolygon[2].y, stInfo.stPolygon[3].x, stInfo.stPolygon[3].y, stInfo.achSensitivity, achTargetTag);
                  pElem->__any = soap_strdup(soap, achPolygon);
             }
        }
        if(count_leave > 0) {
             onvif_get_leave_region_info(0, &stInfo);
             struct _tt__ItemList_ElementItem *pElemSched = pRule->Parameters->ElementItem + count_leave;
             if(pElemSched != NULL) {
                  pElemSched->Name = soap_strdup(soap, "Schedule");
                  pElemSched->__any = soap_strdup(soap, stInfo.achSchedule);
             }
        }
    }

    /* Audio Anomaly */
    for(int i = 0; i < count_audio; i++)
    {
        struct tt__Config *pRule = tan__GetRulesResponse->Rule + rule_idx++;
        OnvifAudioAnomaly_S stInfo;
        onvif_get_audio_anomaly_info(i, &stInfo); /* Use index although likely 1 */
        
        pRule->Name = soap_strdup(soap, AUDIO_ANOMALY_EVENT_RULE);
        pRule->Type = soap_strdup(soap, AUDIO_ANOMALY_EVENT_RULE_TYPE);
        
        pRule->Parameters = soap_new_tt__ItemList(soap, -1);
        pRule->Parameters->__sizeSimpleItem = 2 + 1;
        pRule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap, 3);
        
        struct _tt__ItemList_SimpleItem *pParam_0 = pRule->Parameters->SimpleItem + 0;
        struct _tt__ItemList_SimpleItem *pParam_1 = pRule->Parameters->SimpleItem + 1;
        struct _tt__ItemList_SimpleItem *pParam_Enable = pRule->Parameters->SimpleItem + 2;

        if(pParam_0) { pParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME); pParam_0->Value = soap_strdup(soap, stInfo.achSensitivity); }
        if(pParam_1) { pParam_1->Name = soap_strdup(soap, "Threshold"); pParam_1->Value = soap_strdup(soap, stInfo.achThreshold); }
        if(pParam_Enable) {
             pParam_Enable->Name = soap_strdup(soap, "Enable");
             pParam_Enable->Value = soap_strdup(soap, stInfo.bEnable ? "true" : "false");
        }

        /* Add Schedule ElementItem */
        pRule->Parameters->__sizeElementItem = 1;
        pRule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap, 1);
        struct _tt__ItemList_ElementItem *pElem = pRule->Parameters->ElementItem + 0;
        if(pElem) {
            pElem->Name = soap_strdup(soap, "Schedule");
            pElem->__any = soap_strdup(soap, stInfo.achSchedule);
        }
    }

    /* Scene Change */
    for(int i = 0; i < count_scene; i++)
    {
        struct tt__Config *pRule = tan__GetRulesResponse->Rule + rule_idx++;
        OnvifSceneChange_S stInfo;
        onvif_get_scene_change_info(i, &stInfo);
        
        pRule->Name = soap_strdup(soap, SCENE_CHANGE_EVENT_RULE);
        pRule->Type = soap_strdup(soap, SCENE_CHANGE_EVENT_RULE_TYPE);
        
        pRule->Parameters = soap_new_tt__ItemList(soap, -1);
        pRule->Parameters->__sizeSimpleItem = 1 + 1;
        pRule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap, 2);
        
        struct _tt__ItemList_SimpleItem *pParam_0 = pRule->Parameters->SimpleItem + 0;
        struct _tt__ItemList_SimpleItem *pParam_Enable = pRule->Parameters->SimpleItem + 1;

        if(pParam_0) { pParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME); pParam_0->Value = soap_strdup(soap, stInfo.achSensitivity); }
        if(pParam_Enable) {
             pParam_Enable->Name = soap_strdup(soap, "Enable");
             pParam_Enable->Value = soap_strdup(soap, stInfo.bEnable ? "true" : "false");
        }

        /* Add Schedule ElementItem */
        pRule->Parameters->__sizeElementItem = 1;
        pRule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap, 1);
        struct _tt__ItemList_ElementItem *pElem = pRule->Parameters->ElementItem + 0;
        if(pElem) {
            pElem->Name = soap_strdup(soap, "Schedule");
            pElem->__any = soap_strdup(soap, stInfo.achSchedule);
        }
    }

    /* Face Detection */
    for(int i = 0; i < count_face; i++)
    {
        struct tt__Config *pRule = tan__GetRulesResponse->Rule + rule_idx++;
        OnvifFaceDetection_S stInfo;
        onvif_get_face_detect_info(i, &stInfo);
        
        pRule->Name = soap_strdup(soap, FACE_DETECT_EVENT_RULE);
        pRule->Type = soap_strdup(soap, FACE_DETECT_EVENT_RULE_TYPE);
        
        pRule->Parameters = soap_new_tt__ItemList(soap, -1);
        pRule->Parameters->__sizeSimpleItem = 1 + 1;
        pRule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap, 2);
        
        struct _tt__ItemList_SimpleItem *pParam_0 = pRule->Parameters->SimpleItem + 0;
        struct _tt__ItemList_SimpleItem *pParam_Enable = pRule->Parameters->SimpleItem + 1;

        if(pParam_0) { pParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME); pParam_0->Value = soap_strdup(soap, stInfo.achSensitivity); }
        if(pParam_Enable) {
             pParam_Enable->Name = soap_strdup(soap, "Enable");
             pParam_Enable->Value = soap_strdup(soap, stInfo.bEnable ? "true" : "false");
        }

        pRule->Parameters->__sizeElementItem = 2;
        pRule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap, 2);
        struct _tt__ItemList_ElementItem *pElem_0 = pRule->Parameters->ElementItem + 0;
        struct _tt__ItemList_ElementItem *pElem_1 = pRule->Parameters->ElementItem + 1;
        if(pElem_0 != NULL) {
             char achPolygon[5120];
             pElem_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
             snprintf(achPolygon, sizeof(achPolygon),
                 "<tt:PolygonConfiguration><tt:Polygon>"
                 "<tt:Point x=\"%d\" y=\"%d\"/><tt:Point x=\"%d\" y=\"%d\"/><tt:Point x=\"%d\" y=\"%d\"/><tt:Point x=\"%d\" y=\"%d\"/>"
                 "</tt:Polygon></tt:PolygonConfiguration>",
                 stInfo.stPolygon[0].x, stInfo.stPolygon[0].y, stInfo.stPolygon[1].x, stInfo.stPolygon[1].y,
                 stInfo.stPolygon[2].x, stInfo.stPolygon[2].y, stInfo.stPolygon[3].x, stInfo.stPolygon[3].y);
             pElem_0->__any = soap_strdup(soap, achPolygon);
        }
        if(pElem_1 != NULL) {
             pElem_1->Name = soap_strdup(soap, "Schedule");
             pElem_1->__any = soap_strdup(soap, stInfo.achSchedule);
        }
    }

    /* Loitering Detection */
    if(count_loitering > 0)
    {
        struct tt__Config *pRule = tan__GetRulesResponse->Rule + rule_idx++;
        OnvifLoiteringDetection_S stInfo;
        onvif_get_loitering_detect_info(0, &stInfo);
        
        pRule->Name = soap_strdup(soap, LOITERING_DETECT_EVENT_RULE);
        pRule->Type = soap_strdup(soap, LOITERING_DETECT_EVENT_RULE_TYPE);
        
        pRule->Parameters = soap_new_tt__ItemList(soap, -1);
        pRule->Parameters->__sizeSimpleItem = 1;
        pRule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap, 1);
        
        struct _tt__ItemList_SimpleItem *pParam_Enable = pRule->Parameters->SimpleItem + 0;

        if(pParam_Enable) {
             pParam_Enable->Name = soap_strdup(soap, "Enable");
             pParam_Enable->Value = soap_strdup(soap, stInfo.bEnable ? "true" : "false");
        }
        
        pRule->Parameters->__sizeElementItem = count_loitering + 1;
        pRule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap, count_loitering + 1);
        
        for(int k=0; k < count_loitering; k++)
        {
             onvif_get_loitering_detect_info(k, &stInfo);
             struct _tt__ItemList_ElementItem *pElem = pRule->Parameters->ElementItem + k;
             if(pElem != NULL) {
                  char achPolygon[5120];
                  pElem->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                  snprintf(achPolygon, sizeof(achPolygon),
                      "<tt:PolygonConfiguration><tt:Polygon>"
                      "<tt:Point x=\"%d\" y=\"%d\"/><tt:Point x=\"%d\" y=\"%d\"/><tt:Point x=\"%d\" y=\"%d\"/><tt:Point x=\"%d\" y=\"%d\"/>"
                      "</tt:Polygon><tt:Sensitivity>%s</tt:Sensitivity><tt:TimeThreshold>%s</tt:TimeThreshold></tt:PolygonConfiguration>",
                      stInfo.stPolygon[0].x, stInfo.stPolygon[0].y, stInfo.stPolygon[1].x, stInfo.stPolygon[1].y,
                      stInfo.stPolygon[2].x, stInfo.stPolygon[2].y, stInfo.stPolygon[3].x, stInfo.stPolygon[3].y, stInfo.achSensitivity, stInfo.achTimeThreshold);
                  pElem->__any = soap_strdup(soap, achPolygon);
             }
        }
        if(count_loitering > 0) {
             onvif_get_loitering_detect_info(0, &stInfo);
             struct _tt__ItemList_ElementItem *pElemSched = pRule->Parameters->ElementItem + count_loitering;
             if(pElemSched != NULL) {
                  pElemSched->Name = soap_strdup(soap, "Schedule");
                  pElemSched->__any = soap_strdup(soap, stInfo.achSchedule);
             }
        }
    }

    /* Crowd Gathering */
    if(count_crowd > 0)
    {
        struct tt__Config *pRule = tan__GetRulesResponse->Rule + rule_idx++;
        OnvifCrowdGathering_S stInfo;
        onvif_get_crowd_gathering_info(0, &stInfo);
        
        pRule->Name = soap_strdup(soap, CROWD_GATHERING_EVENT_RULE);
        pRule->Type = soap_strdup(soap, CROWD_GATHERING_EVENT_RULE_TYPE);
        
        pRule->Parameters = soap_new_tt__ItemList(soap, -1);
        pRule->Parameters->__sizeSimpleItem = 2 + 1;
        pRule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap, 3);
        
        struct _tt__ItemList_SimpleItem *pParam_0 = pRule->Parameters->SimpleItem + 0;
        struct _tt__ItemList_SimpleItem *pParam_1 = pRule->Parameters->SimpleItem + 1;
        struct _tt__ItemList_SimpleItem *pParam_Enable = pRule->Parameters->SimpleItem + 2;

        if(pParam_0) { pParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME); pParam_0->Value = soap_strdup(soap, stInfo.achSensitivity); }
        if(pParam_1) { pParam_1->Name = soap_strdup(soap, "ObjectOccup"); pParam_1->Value = soap_strdup(soap, stInfo.achObjectOccup); }
        if(pParam_Enable) {
             pParam_Enable->Name = soap_strdup(soap, "Enable");
             pParam_Enable->Value = soap_strdup(soap, stInfo.bEnable ? "true" : "false");
        }
        
        pRule->Parameters->__sizeElementItem = count_crowd + 1;
        pRule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap, count_crowd + 1);
        
        for(int k=0; k < count_crowd; k++)
        {
             onvif_get_crowd_gathering_info(k, &stInfo);
             struct _tt__ItemList_ElementItem *pElem = pRule->Parameters->ElementItem + k;
             if(pElem != NULL) {
                  char achPolygon[5120];
                  pElem->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                  snprintf(achPolygon, sizeof(achPolygon),
                      "<tt:PolygonConfiguration><tt:Polygon>"
                      "<tt:Point x=\"%d\" y=\"%d\"/><tt:Point x=\"%d\" y=\"%d\"/><tt:Point x=\"%d\" y=\"%d\"/><tt:Point x=\"%d\" y=\"%d\"/>"
                      "</tt:Polygon></tt:PolygonConfiguration>",
                      stInfo.stPolygon[0].x, stInfo.stPolygon[0].y, stInfo.stPolygon[1].x, stInfo.stPolygon[1].y,
                      stInfo.stPolygon[2].x, stInfo.stPolygon[2].y, stInfo.stPolygon[3].x, stInfo.stPolygon[3].y);
                  pElem->__any = soap_strdup(soap, achPolygon);
             }
        }
        if(count_crowd > 0) {
             onvif_get_crowd_gathering_info(0, &stInfo);
             struct _tt__ItemList_ElementItem *pElemSched = pRule->Parameters->ElementItem + count_crowd;
             if(pElemSched != NULL) {
                  pElemSched->Name = soap_strdup(soap, "Schedule");
                  pElemSched->__any = soap_strdup(soap, stInfo.achSchedule);
             }
        }
    }

    /* Parking Detection */
    if(count_parking > 0)
    {
        struct tt__Config *pRule = tan__GetRulesResponse->Rule + rule_idx++;
        OnvifParkingDetection_S stInfo;
        onvif_get_parking_detect_info(0, &stInfo);
        
        pRule->Name = soap_strdup(soap, PARKING_DETECT_EVENT_RULE);
        pRule->Type = soap_strdup(soap, PARKING_DETECT_EVENT_RULE_TYPE);
        
        pRule->Parameters = soap_new_tt__ItemList(soap, -1);
        pRule->Parameters->__sizeSimpleItem = 1;
        pRule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap, 1);
        
        struct _tt__ItemList_SimpleItem *pParam_Enable = pRule->Parameters->SimpleItem + 0;

        if(pParam_Enable) {
             pParam_Enable->Name = soap_strdup(soap, "Enable");
             pParam_Enable->Value = soap_strdup(soap, stInfo.bEnable ? "true" : "false");
        }
        
        pRule->Parameters->__sizeElementItem = count_parking + 1;
        pRule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap, count_parking + 1);
        
        for(int k=0; k < count_parking; k++)
        {
             onvif_get_parking_detect_info(k, &stInfo);
             struct _tt__ItemList_ElementItem *pElem = pRule->Parameters->ElementItem + k;
             if(pElem != NULL) {
                  char achPolygon[5120];
                  pElem->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                  snprintf(achPolygon, sizeof(achPolygon),
                      "<tt:PolygonConfiguration><tt:Polygon>"
                      "<tt:Point x=\"%d\" y=\"%d\"/><tt:Point x=\"%d\" y=\"%d\"/><tt:Point x=\"%d\" y=\"%d\"/><tt:Point x=\"%d\" y=\"%d\"/>"
                      "</tt:Polygon><tt:Sensitivity>%s</tt:Sensitivity><tt:TimeThreshold>%s</tt:TimeThreshold></tt:PolygonConfiguration>",
                      stInfo.stPolygon[0].x, stInfo.stPolygon[0].y, stInfo.stPolygon[1].x, stInfo.stPolygon[1].y,
                      stInfo.stPolygon[2].x, stInfo.stPolygon[2].y, stInfo.stPolygon[3].x, stInfo.stPolygon[3].y, stInfo.achSensitivity, stInfo.achTimeThreshold);
                  pElem->__any = soap_strdup(soap, achPolygon);
             }
        }
        if(count_parking > 0) {
             onvif_get_parking_detect_info(0, &stInfo);
             struct _tt__ItemList_ElementItem *pElemSched = pRule->Parameters->ElementItem + count_parking;
             if(pElemSched != NULL) {
                  pElemSched->Name = soap_strdup(soap, "Schedule");
                  pElemSched->__any = soap_strdup(soap, stInfo.achSchedule);
             }
        }
    }

    /* Unattended Object */
    if(count_unattended > 0)
    {
        struct tt__Config *pRule = tan__GetRulesResponse->Rule + rule_idx++;
        OnvifUnattendedObject_S stInfo;
        onvif_get_unattended_object_info(0, &stInfo);
        
        pRule->Name = soap_strdup(soap, UNATTENDED_OBJECT_EVENT_RULE);
        pRule->Type = soap_strdup(soap, UNATTENDED_OBJECT_EVENT_RULE_TYPE);
        
        pRule->Parameters = soap_new_tt__ItemList(soap, -1);
        pRule->Parameters->__sizeSimpleItem = 1;
        pRule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap, 1);
        
        struct _tt__ItemList_SimpleItem *pParam_Enable = pRule->Parameters->SimpleItem + 0;

        if(pParam_Enable) {
             pParam_Enable->Name = soap_strdup(soap, "Enable");
             pParam_Enable->Value = soap_strdup(soap, stInfo.bEnable ? "true" : "false");
        }
        
        pRule->Parameters->__sizeElementItem = count_unattended + 1;
        pRule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap, count_unattended + 1);
        
        for(int k=0; k < count_unattended; k++)
        {
             onvif_get_unattended_object_info(k, &stInfo);
             struct _tt__ItemList_ElementItem *pElem = pRule->Parameters->ElementItem + k;
             if(pElem != NULL) {
                  char achPolygon[5120];
                  pElem->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                  snprintf(achPolygon, sizeof(achPolygon),
                      "<tt:PolygonConfiguration><tt:Polygon>"
                      "<tt:Point x=\"%d\" y=\"%d\"/><tt:Point x=\"%d\" y=\"%d\"/><tt:Point x=\"%d\" y=\"%d\"/><tt:Point x=\"%d\" y=\"%d\"/>"
                      "</tt:Polygon><tt:Sensitivity>%s</tt:Sensitivity><tt:TimeThreshold>%s</tt:TimeThreshold></tt:PolygonConfiguration>",
                      stInfo.stPolygon[0].x, stInfo.stPolygon[0].y, stInfo.stPolygon[1].x, stInfo.stPolygon[1].y,
                      stInfo.stPolygon[2].x, stInfo.stPolygon[2].y, stInfo.stPolygon[3].x, stInfo.stPolygon[3].y, stInfo.achSensitivity, stInfo.achTimeThreshold);
                  pElem->__any = soap_strdup(soap, achPolygon);
             }
        }
        if(count_unattended > 0) {
             onvif_get_unattended_object_info(0, &stInfo);
             struct _tt__ItemList_ElementItem *pElemSched = pRule->Parameters->ElementItem + count_unattended;
             if(pElemSched != NULL) {
                  pElemSched->Name = soap_strdup(soap, "Schedule");
                  pElemSched->__any = soap_strdup(soap, stInfo.achSchedule);
             }
        }
    }

    /* Object Removal */
    if(count_removal > 0)
    {
        struct tt__Config *pRule = tan__GetRulesResponse->Rule + rule_idx++;
        OnvifObjectRemoval_S stInfo;
        onvif_get_object_removal_info(0, &stInfo);
        
        pRule->Name = soap_strdup(soap, OBJECT_REMOVAL_EVENT_RULE);
        pRule->Type = soap_strdup(soap, OBJECT_REMOVAL_EVENT_RULE_TYPE);
        
        pRule->Parameters = soap_new_tt__ItemList(soap, -1);
        pRule->Parameters->__sizeSimpleItem = 1;
        pRule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap, 1);
        
        struct _tt__ItemList_SimpleItem *pParam_Enable = pRule->Parameters->SimpleItem + 0;

        if(pParam_Enable) {
             pParam_Enable->Name = soap_strdup(soap, "Enable");
             pParam_Enable->Value = soap_strdup(soap, stInfo.bEnable ? "true" : "false");
        }
        
        pRule->Parameters->__sizeElementItem = count_removal + 1;
        pRule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap, count_removal + 1);
        
        for(int k=0; k < count_removal; k++)
        {
             onvif_get_object_removal_info(k, &stInfo);
             struct _tt__ItemList_ElementItem *pElem = pRule->Parameters->ElementItem + k;
             if(pElem != NULL) {
                  char achPolygon[5120];
                  pElem->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                  snprintf(achPolygon, sizeof(achPolygon),
                      "<tt:PolygonConfiguration><tt:Polygon>"
                      "<tt:Point x=\"%d\" y=\"%d\"/><tt:Point x=\"%d\" y=\"%d\"/><tt:Point x=\"%d\" y=\"%d\"/><tt:Point x=\"%d\" y=\"%d\"/>"
                      "</tt:Polygon><tt:Sensitivity>%s</tt:Sensitivity><tt:TimeThreshold>%s</tt:TimeThreshold></tt:PolygonConfiguration>",
                      stInfo.stPolygon[0].x, stInfo.stPolygon[0].y, stInfo.stPolygon[1].x, stInfo.stPolygon[1].y,
                      stInfo.stPolygon[2].x, stInfo.stPolygon[2].y, stInfo.stPolygon[3].x, stInfo.stPolygon[3].y, stInfo.achSensitivity, stInfo.achTimeThreshold);
                  pElem->__any = soap_strdup(soap, achPolygon);
             }
        }
        if(count_removal > 0) {
             onvif_get_object_removal_info(0, &stInfo);
             struct _tt__ItemList_ElementItem *pElemSched = pRule->Parameters->ElementItem + count_removal;
             if(pElemSched != NULL) {
                  pElemSched->Name = soap_strdup(soap, "Schedule");
                  pElemSched->__any = soap_strdup(soap, stInfo.achSchedule);
             }
        }
    }

    /* Pet Recognition */
    for(int i = 0; i < count_pet; i++)
    {
        struct tt__Config *pRule = tan__GetRulesResponse->Rule + rule_idx++;
        OnvifPetRecognition_S stInfo;
        onvif_get_pet_recognition_info(i, &stInfo);
        
        /* Note: Pet Recognition in wrapper seems single instance usually, but treating as potentially multi */
        char ruleName[128];
        snprintf(ruleName, sizeof(ruleName), "%s", PET_RECOGNITION_EVENT_RULE); /* Keep original name if singleton, or append if multi supported later */
        if(count_pet > 1) snprintf(ruleName, sizeof(ruleName), "%s%d", PET_RECOGNITION_EVENT_RULE, i);

        pRule->Name = soap_strdup(soap, ruleName);
        pRule->Type = soap_strdup(soap, PET_RECOGNITION_EVENT_RULE_TYPE);
        
        pRule->Parameters = soap_new_tt__ItemList(soap, -1);
        pRule->Parameters->__sizeSimpleItem = 1 + 1;
        pRule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap, 2);
        
        struct _tt__ItemList_SimpleItem *pParam_0 = pRule->Parameters->SimpleItem + 0;
        struct _tt__ItemList_SimpleItem *pParam_Enable = pRule->Parameters->SimpleItem + 1;

        if(pParam_0) { pParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME); pParam_0->Value = soap_strdup(soap, stInfo.achSensitivity); }
        if(pParam_Enable) {
             pParam_Enable->Name = soap_strdup(soap, "Enable");
             pParam_Enable->Value = soap_strdup(soap, stInfo.bEnable ? "true" : "false");
        }
        
        pRule->Parameters->__sizeElementItem = 2;
        pRule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap, 2);
        struct _tt__ItemList_ElementItem *pElem_0 = pRule->Parameters->ElementItem + 0;
        struct _tt__ItemList_ElementItem *pElem_1 = pRule->Parameters->ElementItem + 1;
        if(pElem_0 != NULL) {
             char achPolygon[5120];
             pElem_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
             snprintf(achPolygon, sizeof(achPolygon),
                 "<tt:PolygonConfiguration><tt:Polygon>"
                 "<tt:Point x=\"%d\" y=\"%d\"/><tt:Point x=\"%d\" y=\"%d\"/><tt:Point x=\"%d\" y=\"%d\"/><tt:Point x=\"%d\" y=\"%d\"/>"
                 "</tt:Polygon></tt:PolygonConfiguration>",
                 stInfo.stPolygon[0].x, stInfo.stPolygon[0].y, stInfo.stPolygon[1].x, stInfo.stPolygon[1].y,
                 stInfo.stPolygon[2].x, stInfo.stPolygon[2].y, stInfo.stPolygon[3].x, stInfo.stPolygon[3].y);
             pElem_0->__any = soap_strdup(soap, achPolygon);
        }
        if(pElem_1 != NULL) {
             pElem_1->Name = soap_strdup(soap, "Schedule");
             pElem_1->__any = soap_strdup(soap, stInfo.achSchedule);
        }
    }

    /* Face Capture */
    for(int i = 0; i < count_facecap; i++)
    {
        struct tt__Config *pRule = tan__GetRulesResponse->Rule + rule_idx++;
        OnvifFaceCapture_S stInfo;
        onvif_get_face_capture_info(i, &stInfo);
        
        pRule->Name = soap_strdup(soap, FACE_CAPTURE_EVENT_RULE); /* Singleton likely */
        pRule->Type = soap_strdup(soap, FACE_CAPTURE_EVENT_RULE_TYPE);
        
        pRule->Parameters = soap_new_tt__ItemList(soap, -1);
        pRule->Parameters->__sizeSimpleItem = 2 + 1;
        pRule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap, 3);
        
        struct _tt__ItemList_SimpleItem *pParam_0 = pRule->Parameters->SimpleItem + 0;
        struct _tt__ItemList_SimpleItem *pParam_1 = pRule->Parameters->SimpleItem + 1;
        struct _tt__ItemList_SimpleItem *pParam_Enable = pRule->Parameters->SimpleItem + 2;

        if(pParam_0) { pParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME); pParam_0->Value = soap_strdup(soap, stInfo.achSensitivity); }
        if(pParam_1) { pParam_1->Name = soap_strdup(soap, "Interval"); pParam_1->Value = soap_strdup(soap, stInfo.achInterval); }
        if(pParam_Enable) {
             pParam_Enable->Name = soap_strdup(soap, "Enable");
             pParam_Enable->Value = soap_strdup(soap, stInfo.bEnable ? "true" : "false");
        }
        
        pRule->Parameters->__sizeElementItem = 2;
        pRule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap, 2);
        struct _tt__ItemList_ElementItem *pElem_0 = pRule->Parameters->ElementItem + 0;
        struct _tt__ItemList_ElementItem *pElem_1 = pRule->Parameters->ElementItem + 1;
        if(pElem_0 != NULL) {
             char achPolygon[5120];
             pElem_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
             snprintf(achPolygon, sizeof(achPolygon),
                 "<tt:PolygonConfiguration><tt:Polygon>"
                 "<tt:Point x=\"%d\" y=\"%d\"/><tt:Point x=\"%d\" y=\"%d\"/><tt:Point x=\"%d\" y=\"%d\"/><tt:Point x=\"%d\" y=\"%d\"/>"
                 "</tt:Polygon></tt:PolygonConfiguration>",
                 stInfo.stPolygon[0].x, stInfo.stPolygon[0].y, stInfo.stPolygon[1].x, stInfo.stPolygon[1].y,
                 stInfo.stPolygon[2].x, stInfo.stPolygon[2].y, stInfo.stPolygon[3].x, stInfo.stPolygon[3].y);
             pElem_0->__any = soap_strdup(soap, achPolygon);
        }
        if(pElem_1 != NULL) {
             pElem_1->Name = soap_strdup(soap, "Schedule");
             pElem_1->__any = soap_strdup(soap, stInfo.achSchedule);
        }
    }


    return SOAP_OK;
}

/** Web service operation '__tan__GetRuleOptions' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tan__GetRuleOptions(struct soap* soap, struct _tan__GetRuleOptions *tan__GetRuleOptions, struct _tan__GetRuleOptionsResponse *tan__GetRuleOptionsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tan__GetRuleOptions----------");
#endif
    int nRet = onvif_authentication(soap);
    if(SOAP_OK != nRet)
    {
        return nRet;
    }
    /* token校验 */
    if(tan__GetRuleOptions != NULL && tan__GetRuleOptions->ConfigurationToken != NULL)
    {
        if(strcmp(tan__GetRuleOptions->ConfigurationToken,VIDEOANALTICS_TOKEN) != 0)
        {
            
            dlog_error("requested  VIDEOANALTICS_TOKEN does not exist");
            return soap_receiver_fault(soap, "requested VIDEOANALTICS_TOKEN does not exist", NULL); 
        }
    }
    else
    {
        return soap_receiver_fault(soap, "requested VIDEOANALTICS_TOKEN does not exist", NULL); 
    }

    if(tan__GetRuleOptionsResponse == NULL)
    {
        dlog_error("tan__GetSupportedRulesResponse is NULL");
        return soap_receiver_fault(soap, "tan__GetSupportedRulesResponse is NULL", NULL);    
    }

    tan__GetRuleOptionsResponse->__sizeRuleOptions = 1;
    tan__GetRuleOptionsResponse->RuleOptions = soap_new_tan__ConfigOptions(soap,-1);
    tan__GetRuleOptionsResponse->RuleOptions->Type = soap_strdup(soap,"axt:MotionRegionConfigOptions");
    tan__GetRuleOptionsResponse->RuleOptions->Name = soap_strdup(soap,"axt:MotionRegion");
    tan__GetRuleOptionsResponse->RuleOptions->__any = soap_strdup(soap,MOTION_REGION_CONFIG_OPTIONS);

    return SOAP_OK;
}

/** Web service operation '__tan__ModifyRules' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tan__ModifyRules(struct soap* soap, struct _tan__ModifyRules *tan__ModifyRules, struct _tan__ModifyRulesResponse *tan__ModifyRulesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tan__ModifyRules----------");
#endif
    dlog_debug("----------__tan__ModifyRules----------");
#include "IpcRet.h"
    dlog_debug("----------__tan__ModifyRules----------");
    int nRet = onvif_authentication(soap);
    if(SOAP_OK != nRet)
    {
        return nRet;
    }
    /* token校验 */
    if(tan__ModifyRules != NULL && tan__ModifyRules->ConfigurationToken)
    {
        if(strcmp(tan__ModifyRules->ConfigurationToken,VIDEOANALTICS_TOKEN) != 0)
        {
            dlog_error("requested  VIDEOANALTICS_TOKEN does not exist");
            return soap_receiver_fault(soap, "requested VIDEOANALTICS_TOKEN does not exist", NULL); 
        }
    }
    else
    {
        return soap_receiver_fault(soap, "requested VIDEOANALTICS_TOKEN does not exist", NULL); 
    }
    /* 是否设置成功标志 */
    nRet = SOAP_FAULT;
    for(int i  = 0; i < tan__ModifyRules->__sizeRule;i++)
    {
        struct tt__Config *pRule = tan__ModifyRules->Rule + i;
        if(pRule == NULL || pRule->Name == NULL || pRule->Type == NULL)
        {
            continue;
        }

        /* 移动侦测设置 */
        if(!strcmp(pRule->Name,MOTION_EVENT_RULE) && !strcmp(pRule->Type,MOTION_EVENT_RULE_TYPE))
        {
            OnvifMotionDetection_S stInfo;
            onvif_get_motion_info(&stInfo);
            
            for(int j = 0;j < pRule->Parameters->__sizeSimpleItem;j++)
            {
                struct _tt__ItemList_SimpleItem *pRuleParam = pRule->Parameters->SimpleItem + j;
                if(pRuleParam == NULL || pRuleParam->Name == NULL || pRuleParam->Value == NULL)
                {
                    continue;
                }
                /* 移动侦测网格 */
                if(!strcmp(pRuleParam->Name,ONVIF_CELLMOTION_RULEPARAM_4))
                {
                    stInfo.achBaseStr[0] = '\0';
                    size_t srcLen = strlen(pRuleParam->Value);
                    size_t maxLen = sizeof(stInfo.achBaseStr);
                    // 拷贝
                    strncpy(stInfo.achBaseStr, pRuleParam->Value, maxLen - 1);
                    stInfo.achBaseStr[maxLen - 1] = '\0';
                }
                else if(!strcmp(pRuleParam->Name, "Enable"))
                {
                    stInfo.bEnable = !strcmp(pRuleParam->Value, "true");
                    stInfo.bEnableSet = true; /* Modified: Flag to track if Enable was explicitly set */
                }
            }

            for(int j = 0; j < pRule->Parameters->__sizeElementItem; j++) {
                struct _tt__ItemList_ElementItem *pParam = pRule->Parameters->ElementItem + j;
                if(pParam && pParam->Name && pParam->__any) {
                    if(!strcmp(pParam->Name, "Schedule")) {
                         strncpy(stInfo.achSchedule, pParam->__any, sizeof(stInfo.achSchedule)-1);
                    }
                }
            }

            if(onvif_set_motion_rule(&stInfo) != 0)
            {
                nRet = SOAP_FAULT;
            }
            else
            {
                nRet = SOAP_OK;
            }
        }
        /* 遮挡报警规则设置 */
        else if(!strcmp(pRule->Name,TAMPEREVENT_RULE) && !strcmp(pRule->Type,TAMPEREVENT_RULE_TYPE))
        {
            ONvifTamperDetection_S stInfo;
            onvif_get_tamp_info(&stInfo);
            char achSharedSchedule[2048] = {0};
            
            /* Parse SimpleItems (Enable) */
            for(int j = 0; j < pRule->Parameters->__sizeSimpleItem; j++) {
                struct _tt__ItemList_SimpleItem *pParam = pRule->Parameters->SimpleItem + j;
                if(pParam && pParam->Name && pParam->Value) {
                   if(!strcmp(pParam->Name, "Enable")) {
                        stInfo.bEnable = !strcmp(pParam->Value, "true");
                        stInfo.bEnableSet = true; /* Modified: Flag to track if Enable was explicitly set */
                    }
                }
            }
            /* Pre-scan for Schedule */
            for(int j = 0; j < pRule->Parameters->__sizeElementItem; j++) {
                 struct _tt__ItemList_ElementItem *pParam = pRule->Parameters->ElementItem + j;
                 if(pParam && pParam->Name && pParam->__any && !strcmp(pParam->Name, "Schedule")) {
                      strncpy(achSharedSchedule, pParam->__any, sizeof(achSharedSchedule)-1);
                 }
            }
            
            for(int j = 0;j < pRule->Parameters->__sizeElementItem;j++)
            {
                struct _tt__ItemList_ElementItem *pRuleParam = pRule->Parameters->ElementItem + j;
                if(pRuleParam == NULL || pRuleParam->Name == NULL || pRuleParam->__any == NULL)
                {
                    continue;
                }

                if(!strcmp(pRuleParam->Name,ONVIF_ANALYTICS_FIELD_NAME))
                {
                    char* pStr = NULL;
                    int nCount = 0;
                    pStr = pRuleParam->__any;
                    pStr = pRuleParam->__any;
                    
                    /* Parse Sensitivity */
                    char* pSens = strstr(pStr, "<tt:Sensitivity>");
                    if(pSens) {
                        pSens += strlen("<tt:Sensitivity>");
                        char* pEnd = strstr(pSens, "</tt:Sensitivity>");
                        if(pEnd) {
                            size_t len = pEnd - pSens;
                            if(len < sizeof(stInfo.achSensitivity)) {
                                strncpy(stInfo.achSensitivity, pSens, len);
                                stInfo.achSensitivity[len] = '\0';
                            }
                        }
                    }

                    while (nCount < 4) 
                    {
                        /*  1. 查找 <tt:Point 标签起始位置 */
                        pStr = strstr(pStr, "<tt:Point");
                        if (!pStr) break;
                        /* 2. 跳过命名空间前缀（如果存在其他属性） */ 
                        pStr += strlen("<tt:Point");

                        /* 3. 解析 x 属性 */ 
                        const char *x_start = strstr(pStr, "x=\"");
                        if (!x_start) break;
                        x_start += 3; // 跳过 x="
                        int x = (int)strtol(x_start, (char**)&pStr, 10);

                        /*  4. 解析 y 属性 */
                        const char *y_start = strstr(pStr, "y=\"");
                        if (!y_start) break;
                        y_start += 3; 
                        int y = (int)strtol(y_start, (char**)&pStr, 10);

                        /* 5. 保存坐标 */ 
                        stInfo.stPolygon[nCount++] = (OnvifPoint_S){x, y};
                        /* 6. 跳过当前标签剩余部分（处理可能存在的其他属性） */ 
                        pStr = strchr(pStr, '/'); 
                        if (!pStr) break;
                        pStr += 2; 
                    }
                    strncpy(stInfo.achSchedule, achSharedSchedule, sizeof(stInfo.achSchedule)-1);
                    if(onvif_set_tamp_rule(&stInfo) != 0)
                    {
                        nRet = SOAP_FAULT;
                    }
                    else
                    {
                        nRet = SOAP_OK;
                    }
                }
            }
        }
        else if(!strncmp(pRule->Name, ENTER_REGION_EVENT_RULE, strlen(ENTER_REGION_EVENT_RULE)) && !strcmp(pRule->Type, ENTER_REGION_EVENT_RULE_TYPE))
        {
            OnvifRegionDetection_S stInfoTemplate;
            memset(&stInfoTemplate, 0, sizeof(stInfoTemplate));
            char achSharedSchedule[2048] = {0};
            /* Pre-scan for Schedule */
            for(int j = 0; j < pRule->Parameters->__sizeElementItem; j++) {
                 struct _tt__ItemList_ElementItem *pParam = pRule->Parameters->ElementItem + j;
                 if(pParam && pParam->Name && pParam->__any && !strcmp(pParam->Name, "Schedule")) {
                      strncpy(achSharedSchedule, pParam->__any, sizeof(achSharedSchedule)-1);
                 }
            }

            for(int j = 0; j < pRule->Parameters->__sizeSimpleItem; j++) {
                struct _tt__ItemList_SimpleItem *pParam = pRule->Parameters->SimpleItem + j;
                if(pParam && pParam->Name && pParam->Value) {
                    if(!strcmp(pParam->Name, ONVIF_ANALYTICS_SENS_NAME)) {
                        strncpy(stInfoTemplate.achSensitivity, pParam->Value, sizeof(stInfoTemplate.achSensitivity)-1);
                    }
                    else if(!strcmp(pParam->Name, "Enable")) {
                        stInfoTemplate.bEnable = !strcmp(pParam->Value, "true");
                        stInfoTemplate.bEnableSet = true; /* Modified: Flag to track if Enable was explicitly set */
                    }
                }
            }

            int nSetCount = 0;
            for(int j = 0; j < pRule->Parameters->__sizeElementItem; j++) {
                struct _tt__ItemList_ElementItem *pParam = pRule->Parameters->ElementItem + j;
                if(pParam && pParam->Name && pParam->__any) {
                    if(!strcmp(pParam->Name, ONVIF_ANALYTICS_FIELD_NAME)) {
                        OnvifRegionDetection_S stInfo = stInfoTemplate;
                        char* pStr = pParam->__any;

                        /* Parse Sensitivity */
                        char* pSens = strstr(pStr, "<tt:Sensitivity>");
                        if(pSens) {
                            pSens += strlen("<tt:Sensitivity>");
                            char* pEnd = strstr(pSens, "</tt:Sensitivity>");
                            if(pEnd) {
                                size_t len = pEnd - pSens;
                                if(len < sizeof(stInfo.achSensitivity)) {
                                    strncpy(stInfo.achSensitivity, pSens, len);
                                    stInfo.achSensitivity[len] = '\0';
                                }
                            }
                        }
                        
                        /* Parse DetectionTarget */
                        char* pTarget = strstr(pStr, "<tt:DetectionTarget>");
                        if(pTarget) {
                            pTarget += strlen("<tt:DetectionTarget>");
                            char* pEnd = strstr(pTarget, "</tt:DetectionTarget>");
                            if(pEnd) {
                                size_t len = pEnd - pTarget;
                                if(len < sizeof(stInfo.achDetectionTarget)) {
                                    strncpy(stInfo.achDetectionTarget, pTarget, len);
                                    stInfo.achDetectionTarget[len] = '\0';
                                }
                            }
                        }

                        int nCount = 0;
                        while(nCount < 4) {
                            pStr = strstr(pStr, "<tt:Point");
                            if(!pStr) break;
                            pStr += strlen("<tt:Point");
                            const char *x_start = strstr(pStr, "x=\"");
                            if(!x_start) break;
                            x_start += 3;
                            int x = (int)strtol(x_start, (char**)&pStr, 10);
                            const char *y_start = strstr(pStr, "y=\"");
                            if(!y_start) break;
                            y_start += 3;
                            int y = (int)strtol(y_start, (char**)&pStr, 10);
                            stInfo.stPolygon[nCount++] = (OnvifPoint_S){x, y};
                            pStr = strchr(pStr, '/');
                            if(!pStr) break;
                            pStr += 2;
                        }
                        strncpy(stInfo.achSchedule, achSharedSchedule, sizeof(stInfo.achSchedule)-1);
                        int ret = onvif_set_enter_region_info(nSetCount, &stInfo);
                        if(ret == ONVIF_ERR_EVENT_RESOURCE_CONFLICT)
                        {
                            dlog_error("Resource allocation failed for Entrance Detection.");
                            return ret;
                        }
                        if(ret == 0) nSetCount++;
                    }
                }
            }
            
            /* Cleanup remaining */
            OnvifRegionDetection_S stEmpty;
            memset(&stEmpty, 0, sizeof(stEmpty));
            stEmpty.bEnable = false;
            while(nSetCount < 16) {
                 if(onvif_set_enter_region_info(nSetCount, &stEmpty) != 0) break;
                 nSetCount++;
            }
            nRet = SOAP_OK;
        }
        else if(!strncmp(pRule->Name, LEAVE_REGION_EVENT_RULE, strlen(LEAVE_REGION_EVENT_RULE)) && !strcmp(pRule->Type, LEAVE_REGION_EVENT_RULE_TYPE))
        {
            OnvifRegionDetection_S stInfoTemplate;
            memset(&stInfoTemplate, 0, sizeof(stInfoTemplate));
            char achSharedSchedule[2048] = {0};
            /* Pre-scan for Schedule */
            for(int j = 0; j < pRule->Parameters->__sizeElementItem; j++) {
                 struct _tt__ItemList_ElementItem *pParam = pRule->Parameters->ElementItem + j;
                 if(pParam && pParam->Name && pParam->__any && !strcmp(pParam->Name, "Schedule")) {
                      strncpy(achSharedSchedule, pParam->__any, sizeof(achSharedSchedule)-1);
                 }
            }

            for(int j = 0; j < pRule->Parameters->__sizeSimpleItem; j++) {
                struct _tt__ItemList_SimpleItem *pParam = pRule->Parameters->SimpleItem + j;
                if(pParam && pParam->Name && pParam->Value) {
                    if(!strcmp(pParam->Name, ONVIF_ANALYTICS_SENS_NAME)) {
                        strncpy(stInfoTemplate.achSensitivity, pParam->Value, sizeof(stInfoTemplate.achSensitivity)-1);
                    }
                    else if(!strcmp(pParam->Name, "Enable")) {
                        stInfoTemplate.bEnable = !strcmp(pParam->Value, "true");
                        stInfoTemplate.bEnableSet = true; /* Modified: Flag to track if Enable was explicitly set */
                    }
                }
            }

            int nSetCount = 0;
            for(int j = 0; j < pRule->Parameters->__sizeElementItem; j++) {
                struct _tt__ItemList_ElementItem *pParam = pRule->Parameters->ElementItem + j;
                if(pParam && pParam->Name && pParam->__any) {
                    if(!strcmp(pParam->Name, ONVIF_ANALYTICS_FIELD_NAME)) {
                        OnvifRegionDetection_S stInfo = stInfoTemplate;
                        char* pStr = pParam->__any;

                        /* Parse Sensitivity */
                        char* pSens = strstr(pStr, "<tt:Sensitivity>");
                        if(pSens) {
                            pSens += strlen("<tt:Sensitivity>");
                            char* pEnd = strstr(pSens, "</tt:Sensitivity>");
                            if(pEnd) {
                                size_t len = pEnd - pSens;
                                if(len < sizeof(stInfo.achSensitivity)) {
                                    strncpy(stInfo.achSensitivity, pSens, len);
                                    stInfo.achSensitivity[len] = '\0';
                                }
                            }
                        }

                        /* Parse DetectionTarget */
                        char* pTarget = strstr(pStr, "<tt:DetectionTarget>");
                        if(pTarget) {
                            pTarget += strlen("<tt:DetectionTarget>");
                            char* pEnd = strstr(pTarget, "</tt:DetectionTarget>");
                            if(pEnd) {
                                size_t len = pEnd - pTarget;
                                if(len < sizeof(stInfo.achDetectionTarget)) {
                                    strncpy(stInfo.achDetectionTarget, pTarget, len);
                                    stInfo.achDetectionTarget[len] = '\0';
                                }
                            }
                        }

                        int nCount = 0;
                        while(nCount < 4) {
                            pStr = strstr(pStr, "<tt:Point");
                            if(!pStr) break;
                            pStr += strlen("<tt:Point");
                            const char *x_start = strstr(pStr, "x=\"");
                            if(!x_start) break;
                            x_start += 3;
                            int x = (int)strtol(x_start, (char**)&pStr, 10);
                            const char *y_start = strstr(pStr, "y=\"");
                            if(!y_start) break;
                            y_start += 3;
                            int y = (int)strtol(y_start, (char**)&pStr, 10);
                            stInfo.stPolygon[nCount++] = (OnvifPoint_S){x, y};
                            pStr = strchr(pStr, '/');
                            if(!pStr) break;
                            pStr += 2;
                        }
                        strncpy(stInfo.achSchedule, achSharedSchedule, sizeof(stInfo.achSchedule)-1);
                        int ret = onvif_set_leave_region_info(nSetCount, &stInfo);
                        if(ret == ONVIF_ERR_EVENT_RESOURCE_CONFLICT)
                        {
                            dlog_error("Resource allocation failed for Exiting Detection.");
                            return ret;
                        }
                        if(ret == 0) nSetCount++;
                    }
                }
            }
            
            /* Cleanup remaining */
            OnvifRegionDetection_S stEmpty;
            memset(&stEmpty, 0, sizeof(stEmpty));
            stEmpty.bEnable = false;
            while(nSetCount < 16) {
                 if(onvif_set_leave_region_info(nSetCount, &stEmpty) != 0) break;
                 nSetCount++;
            }
            nRet = SOAP_OK;
        }
        else if(!strncmp(pRule->Name, AUDIO_ANOMALY_EVENT_RULE, strlen(AUDIO_ANOMALY_EVENT_RULE)) && !strcmp(pRule->Type, AUDIO_ANOMALY_EVENT_RULE_TYPE))
        {
            OnvifAudioAnomaly_S stInfo;
            memset(&stInfo, 0, sizeof(stInfo));

            for(int j = 0; j < pRule->Parameters->__sizeSimpleItem; j++) {
                struct _tt__ItemList_SimpleItem *pParam = pRule->Parameters->SimpleItem + j;
                if(pParam && pParam->Name && pParam->Value) {
                    if(!strcmp(pParam->Name, "Threshold")) {
                        strncpy(stInfo.achThreshold, pParam->Value, sizeof(stInfo.achThreshold)-1);
                    }
                    else if(!strcmp(pParam->Name, "Enable")) {
                        stInfo.bEnable = !strcmp(pParam->Value, "true");
                        stInfo.bEnableSet = true; /* Modified: Flag to track if Enable was explicitly set */
                    }
                    else if(!strcmp(pParam->Name, ONVIF_ANALYTICS_SENS_NAME)) {
                        strncpy(stInfo.achSensitivity, pParam->Value, sizeof(stInfo.achSensitivity)-1);
                    }
                }
            }
            
            for(int j = 0; j < pRule->Parameters->__sizeElementItem; j++) {
                struct _tt__ItemList_ElementItem *pParam = pRule->Parameters->ElementItem + j;
                if(pParam && pParam->Name && pParam->__any) {
                    if(!strcmp(pParam->Name, "Schedule")) {
                         strncpy(stInfo.achSchedule, pParam->__any, sizeof(stInfo.achSchedule)-1);
                    }
                }
            }

            int ret = onvif_set_audio_anomaly_info(0, &stInfo);
            if(ret == ONVIF_ERR_EVENT_RESOURCE_CONFLICT)
            {
                dlog_error("Resource allocation failed for Audio Anomaly.");
                return ret;
            }
            if(ret != 0) nRet = SOAP_FAULT;
            else nRet = SOAP_OK;
        }
        else if(!strncmp(pRule->Name, SCENE_CHANGE_EVENT_RULE, strlen(SCENE_CHANGE_EVENT_RULE)) && !strcmp(pRule->Type, SCENE_CHANGE_EVENT_RULE_TYPE))
        {
            OnvifSceneChange_S stInfo;
            memset(&stInfo, 0, sizeof(stInfo));

            for(int j = 0; j < pRule->Parameters->__sizeSimpleItem; j++) {
                struct _tt__ItemList_SimpleItem *pParam = pRule->Parameters->SimpleItem + j;
                if(pParam && pParam->Name && pParam->Value) {
                    if(!strcmp(pParam->Name, "Enable")) {
                        stInfo.bEnable = !strcmp(pParam->Value, "true");
                        stInfo.bEnableSet = true; /* Modified: Flag to track if Enable was explicitly set */
                    }
                    else if(!strcmp(pParam->Name, ONVIF_ANALYTICS_SENS_NAME)) {
                        strncpy(stInfo.achSensitivity, pParam->Value, sizeof(stInfo.achSensitivity)-1);
                    }
                }
            }
            
            for(int j = 0; j < pRule->Parameters->__sizeElementItem; j++) {
                struct _tt__ItemList_ElementItem *pParam = pRule->Parameters->ElementItem + j;
                if(pParam && pParam->Name && pParam->__any) {
                    if(!strcmp(pParam->Name, "Schedule")) {
                         strncpy(stInfo.achSchedule, pParam->__any, sizeof(stInfo.achSchedule)-1);
                    }
                }
            }

            int ret = onvif_set_scene_change_info(0, &stInfo);
            if(ret == ONVIF_ERR_EVENT_RESOURCE_CONFLICT)
            {
                dlog_error("Resource allocation failed for Scene Change.");
                return ret;
            }
            if(ret != 0) nRet = SOAP_FAULT;
            else nRet = SOAP_OK;
        }
        else if(!strncmp(pRule->Name, FACE_DETECT_EVENT_RULE, strlen(FACE_DETECT_EVENT_RULE)) && !strcmp(pRule->Type, FACE_DETECT_EVENT_RULE_TYPE))
        {
            OnvifFaceDetection_S stInfoTemplate;
            memset(&stInfoTemplate, 0, sizeof(stInfoTemplate));
            char achSharedSchedule[2048] = {0};
            /* Pre-scan for Schedule */
            for(int j = 0; j < pRule->Parameters->__sizeElementItem; j++) {
                 struct _tt__ItemList_ElementItem *pParam = pRule->Parameters->ElementItem + j;
                 if(pParam && pParam->Name && pParam->__any && !strcmp(pParam->Name, "Schedule")) {
                      strncpy(achSharedSchedule, pParam->__any, sizeof(achSharedSchedule)-1);
                 }
            }

            for(int j = 0; j < pRule->Parameters->__sizeSimpleItem; j++) {
                struct _tt__ItemList_SimpleItem *pParam = pRule->Parameters->SimpleItem + j;
                if(pParam && pParam->Name && pParam->Value) {
                    if(!strcmp(pParam->Name, ONVIF_ANALYTICS_SENS_NAME)) {
                        strncpy(stInfoTemplate.achSensitivity, pParam->Value, sizeof(stInfoTemplate.achSensitivity)-1);
                    }
                    else if(!strcmp(pParam->Name, "Enable")) {
                        stInfoTemplate.bEnable = !strcmp(pParam->Value, "true");
                        stInfoTemplate.bEnableSet = true; /* Modified: Flag to track if Enable was explicitly set */
                    }
                }
            }
            
            int nSetCount = 0;
            for(int j = 0; j < pRule->Parameters->__sizeElementItem; j++) {
                struct _tt__ItemList_ElementItem *pParam = pRule->Parameters->ElementItem + j;
                if(pParam && pParam->Name && pParam->__any) {
                    if(!strcmp(pParam->Name, ONVIF_ANALYTICS_FIELD_NAME)) {
                        OnvifFaceDetection_S stInfo = stInfoTemplate;
                        char* pStr = pParam->__any;
                        int nCount = 0;
                        while(nCount < 4) {
                            pStr = strstr(pStr, "<tt:Point");
                            if(!pStr) break;
                            pStr += strlen("<tt:Point");
                            const char *x_start = strstr(pStr, "x=\"");
                            if(!x_start) break;
                            x_start += 3;
                            int x = (int)strtol(x_start, (char**)&pStr, 10);
                            const char *y_start = strstr(pStr, "y=\"");
                            if(!y_start) break;
                            y_start += 3;
                            int y = (int)strtol(y_start, (char**)&pStr, 10);
                            stInfo.stPolygon[nCount++] = (OnvifPoint_S){x, y};
                            pStr = strchr(pStr, '/');
                            if(!pStr) break;
                            pStr += 2;
                        }
                        strncpy(stInfo.achSchedule, achSharedSchedule, sizeof(stInfo.achSchedule)-1);
                        int ret = onvif_set_face_detect_info(nSetCount, &stInfo);
                        if(ret == ONVIF_ERR_EVENT_RESOURCE_CONFLICT)
                        {
                            dlog_error("Resource allocation failed for Face Detection.");
                            return ret;
                        }
                        if(ret == 0) nSetCount++;
                    }
                }
            }
             /* Cleanup remaining */
            OnvifFaceDetection_S stEmpty;
            memset(&stEmpty, 0, sizeof(stEmpty));
            stEmpty.bEnable = false;
            while(nSetCount < 16) {
                 if(onvif_set_face_detect_info(nSetCount, &stEmpty) != 0) break;
                 nSetCount++;
            }
            nRet = SOAP_OK;
        }
        else if(!strncmp(pRule->Name, LOITERING_DETECT_EVENT_RULE, strlen(LOITERING_DETECT_EVENT_RULE)) && !strcmp(pRule->Type, LOITERING_DETECT_EVENT_RULE_TYPE))
        {
            OnvifLoiteringDetection_S stInfoTemplate;
            memset(&stInfoTemplate, 0, sizeof(stInfoTemplate));
            char achSharedSchedule[2048] = {0};
            /* Pre-scan for Schedule */
            for(int j = 0; j < pRule->Parameters->__sizeElementItem; j++) {
                 struct _tt__ItemList_ElementItem *pParam = pRule->Parameters->ElementItem + j;
                 if(pParam && pParam->Name && pParam->__any && !strcmp(pParam->Name, "Schedule")) {
                      strncpy(achSharedSchedule, pParam->__any, sizeof(achSharedSchedule)-1);
                 }
            }

            for(int j = 0; j < pRule->Parameters->__sizeSimpleItem; j++) {
                struct _tt__ItemList_SimpleItem *pParam = pRule->Parameters->SimpleItem + j;
                if(pParam && pParam->Name && pParam->Value) {
                    if(!strcmp(pParam->Name, ONVIF_ANALYTICS_SENS_NAME)) {
                        strncpy(stInfoTemplate.achSensitivity, pParam->Value, sizeof(stInfoTemplate.achSensitivity)-1);
                    }

                    else if(!strcmp(pParam->Name, "Enable")) {
                        stInfoTemplate.bEnable = !strcmp(pParam->Value, "true");
                        stInfoTemplate.bEnableSet = true; /* Modified: Flag to track if Enable was explicitly set */
                    }
                }
            }
            
            int nSetCount = 0;
            for(int j = 0; j < pRule->Parameters->__sizeElementItem; j++) {
                struct _tt__ItemList_ElementItem *pParam = pRule->Parameters->ElementItem + j;
                if(pParam && pParam->Name && pParam->__any) {
                    if(!strcmp(pParam->Name, ONVIF_ANALYTICS_FIELD_NAME)) {
                        OnvifLoiteringDetection_S stInfo = stInfoTemplate;
                        char* pStr = pParam->__any;

                        /* Parse Sensitivity */
                        char* pSens = strstr(pStr, "<tt:Sensitivity>");
                        if(pSens) {
                            pSens += strlen("<tt:Sensitivity>");
                            char* pEnd = strstr(pSens, "</tt:Sensitivity>");
                            if(pEnd) {
                                size_t len = pEnd - pSens;
                                if(len < sizeof(stInfo.achSensitivity)) {
                                    strncpy(stInfo.achSensitivity, pSens, len);
                                    stInfo.achSensitivity[len] = '\0';
                                }
                            }
                        }

                        /* Parse TimeThreshold */
                        char* pThreshold = strstr(pStr, "<tt:TimeThreshold>");
                        if(pThreshold) {
                            pThreshold += strlen("<tt:TimeThreshold>");
                            char* pEnd = strstr(pThreshold, "</tt:TimeThreshold>");
                            if(pEnd) {
                                size_t len = pEnd - pThreshold;
                                if(len < sizeof(stInfo.achTimeThreshold)) {
                                    strncpy(stInfo.achTimeThreshold, pThreshold, len);
                                    stInfo.achTimeThreshold[len] = '\0';
                                }
                            }
                        }

                        int nCount = 0;
                        while(nCount < 4) {
                            pStr = strstr(pStr, "<tt:Point");
                            if(!pStr) break;
                            pStr += strlen("<tt:Point");
                            const char *x_start = strstr(pStr, "x=\"");
                            if(!x_start) break;
                            x_start += 3;
                            int x = (int)strtol(x_start, (char**)&pStr, 10);
                            const char *y_start = strstr(pStr, "y=\"");
                            if(!y_start) break;
                            y_start += 3;
                            int y = (int)strtol(y_start, (char**)&pStr, 10);
                            stInfo.stPolygon[nCount++] = (OnvifPoint_S){x, y};
                            pStr = strchr(pStr, '/');
                            if(!pStr) break;
                            pStr += 2;
                        }
                        strncpy(stInfo.achSchedule, achSharedSchedule, sizeof(stInfo.achSchedule)-1);
                        int ret = onvif_set_loitering_detect_info(nSetCount, &stInfo);
                        if(ret == ONVIF_ERR_EVENT_RESOURCE_CONFLICT)
                        {
                            dlog_error("Resource allocation failed for Loitering Detection.");
                            return ret;
                        }
                        if(ret == 0) nSetCount++;
                    }
                }
            }
            
             /* Cleanup remaining */
            OnvifLoiteringDetection_S stEmpty;
            memset(&stEmpty, 0, sizeof(stEmpty));
            stEmpty.bEnable = false;
            while(nSetCount < 16) {
                 if(onvif_set_loitering_detect_info(nSetCount, &stEmpty) != 0) break;
                 nSetCount++;
            }
            nRet = SOAP_OK;
        }
        else if(!strncmp(pRule->Name, CROWD_GATHERING_EVENT_RULE, strlen(CROWD_GATHERING_EVENT_RULE)) && !strcmp(pRule->Type, CROWD_GATHERING_EVENT_RULE_TYPE))
        {
            OnvifCrowdGathering_S stInfoTemplate;
            memset(&stInfoTemplate, 0, sizeof(stInfoTemplate));
            char achSharedSchedule[2048] = {0};
            /* Pre-scan for Schedule */
            for(int j = 0; j < pRule->Parameters->__sizeElementItem; j++) {
                 struct _tt__ItemList_ElementItem *pParam = pRule->Parameters->ElementItem + j;
                 if(pParam && pParam->Name && pParam->__any && !strcmp(pParam->Name, "Schedule")) {
                      strncpy(achSharedSchedule, pParam->__any, sizeof(achSharedSchedule)-1);
                 }
            }

            for(int j = 0; j < pRule->Parameters->__sizeSimpleItem; j++) {
                struct _tt__ItemList_SimpleItem *pParam = pRule->Parameters->SimpleItem + j;
                if(pParam && pParam->Name && pParam->Value) {
                    if(!strcmp(pParam->Name, "ObjectOccup")) {
                        strncpy(stInfoTemplate.achObjectOccup, pParam->Value, sizeof(stInfoTemplate.achObjectOccup)-1);
                    }
                    else if(!strcmp(pParam->Name, "Enable")) {
                        stInfoTemplate.bEnable = !strcmp(pParam->Value, "true");
                        stInfoTemplate.bEnableSet = true; /* Modified: Flag to track if Enable was explicitly set */
                    }
                    else if(!strcmp(pParam->Name, ONVIF_ANALYTICS_SENS_NAME)) {
                        strncpy(stInfoTemplate.achSensitivity, pParam->Value, sizeof(stInfoTemplate.achSensitivity)-1);
                    }
                }
            }
            
            int nSetCount = 0;
            for(int j = 0; j < pRule->Parameters->__sizeElementItem; j++) {
                struct _tt__ItemList_ElementItem *pParam = pRule->Parameters->ElementItem + j;
                if(pParam && pParam->Name && pParam->__any) {
                    if(!strcmp(pParam->Name, ONVIF_ANALYTICS_FIELD_NAME)) {
                        OnvifCrowdGathering_S stInfo = stInfoTemplate;
                        char* pStr = pParam->__any;
                        int nCount = 0;
                        while(nCount < 4) {
                            pStr = strstr(pStr, "<tt:Point");
                            if(!pStr) break;
                            pStr += strlen("<tt:Point");
                            const char *x_start = strstr(pStr, "x=\"");
                            if(!x_start) break;
                            x_start += 3;
                            int x = (int)strtol(x_start, (char**)&pStr, 10);
                            const char *y_start = strstr(pStr, "y=\"");
                            if(!y_start) break;
                            y_start += 3;
                            int y = (int)strtol(y_start, (char**)&pStr, 10);
                            stInfo.stPolygon[nCount++] = (OnvifPoint_S){x, y};
                            pStr = strchr(pStr, '/');
                            if(!pStr) break;
                            pStr += 2;
                        }
                        strncpy(stInfo.achSchedule, achSharedSchedule, sizeof(stInfo.achSchedule)-1);
                        int ret = onvif_set_crowd_gathering_info(nSetCount, &stInfo);
                        if(ret == ONVIF_ERR_EVENT_RESOURCE_CONFLICT)
                        {
                            dlog_error("Resource allocation failed for Crowd Gathering.");
                            return ret;
                        }
                        if(ret == 0) nSetCount++;
                    }
                }
            }
            
            /* Cleanup remaining */
            OnvifCrowdGathering_S stEmpty;
            memset(&stEmpty, 0, sizeof(stEmpty));
            stEmpty.bEnable = false;
            while(nSetCount < 16) {
                 if(onvif_set_crowd_gathering_info(nSetCount, &stEmpty) != 0) break;
                 nSetCount++;
            }
            nRet = SOAP_OK;
        }
        else if(!strncmp(pRule->Name, PARKING_DETECT_EVENT_RULE, strlen(PARKING_DETECT_EVENT_RULE)) && !strcmp(pRule->Type, PARKING_DETECT_EVENT_RULE_TYPE))
        {
            OnvifParkingDetection_S stInfoTemplate;
            memset(&stInfoTemplate, 0, sizeof(stInfoTemplate));
            char achSharedSchedule[2048] = {0};
            /* Pre-scan for Schedule */
            for(int j = 0; j < pRule->Parameters->__sizeElementItem; j++) {
                 struct _tt__ItemList_ElementItem *pParam = pRule->Parameters->ElementItem + j;
                 if(pParam && pParam->Name && pParam->__any && !strcmp(pParam->Name, "Schedule")) {
                      strncpy(achSharedSchedule, pParam->__any, sizeof(achSharedSchedule)-1);
                 }
            }

            for(int j = 0; j < pRule->Parameters->__sizeSimpleItem; j++) {
                struct _tt__ItemList_SimpleItem *pParam = pRule->Parameters->SimpleItem + j;
                if(pParam && pParam->Name && pParam->Value) {
 if(!strcmp(pParam->Name, "Enable")) {
                        stInfoTemplate.bEnable = !strcmp(pParam->Value, "true");
                        stInfoTemplate.bEnableSet = true; /* Modified: Flag to track if Enable was explicitly set */
                    }
                    else if(!strcmp(pParam->Name, ONVIF_ANALYTICS_SENS_NAME)) {
                        strncpy(stInfoTemplate.achSensitivity, pParam->Value, sizeof(stInfoTemplate.achSensitivity)-1);
                    }
                }
            }
            
            int nSetCount = 0;
            for(int j = 0; j < pRule->Parameters->__sizeElementItem; j++) {
                struct _tt__ItemList_ElementItem *pParam = pRule->Parameters->ElementItem + j;
                if(pParam && pParam->Name && pParam->__any) {
                    if(!strcmp(pParam->Name, ONVIF_ANALYTICS_FIELD_NAME)) {
                        OnvifParkingDetection_S stInfo = stInfoTemplate;
                        char* pStr = pParam->__any;

                        /* Parse Sensitivity */
                        char* pSens = strstr(pStr, "<tt:Sensitivity>");
                        if(pSens) {
                            pSens += strlen("<tt:Sensitivity>");
                            char* pEnd = strstr(pSens, "</tt:Sensitivity>");
                            if(pEnd) {
                                size_t len = pEnd - pSens;
                                if(len < sizeof(stInfo.achSensitivity)) {
                                    strncpy(stInfo.achSensitivity, pSens, len);
                                    stInfo.achSensitivity[len] = '\0';
                                }
                            }
                        }

                        /* Parse TimeThreshold */
                        char* pThreshold = strstr(pStr, "<tt:TimeThreshold>");
                        if(pThreshold) {
                            pThreshold += strlen("<tt:TimeThreshold>");
                            char* pEnd = strstr(pThreshold, "</tt:TimeThreshold>");
                            if(pEnd) {
                                size_t len = pEnd - pThreshold;
                                if(len < sizeof(stInfo.achTimeThreshold)) {
                                    strncpy(stInfo.achTimeThreshold, pThreshold, len);
                                    stInfo.achTimeThreshold[len] = '\0';
                                }
                            }
                        }

                        int nCount = 0;
                        while(nCount < 4) {
                            pStr = strstr(pStr, "<tt:Point");
                            if(!pStr) break;
                            pStr += strlen("<tt:Point");
                            const char *x_start = strstr(pStr, "x=\"");
                            if(!x_start) break;
                            x_start += 3;
                            int x = (int)strtol(x_start, (char**)&pStr, 10);
                            const char *y_start = strstr(pStr, "y=\"");
                            if(!y_start) break;
                            y_start += 3;
                            int y = (int)strtol(y_start, (char**)&pStr, 10);
                            stInfo.stPolygon[nCount++] = (OnvifPoint_S){x, y};
                            pStr = strchr(pStr, '/');
                            if(!pStr) break;
                            pStr += 2;
                        }
                        strncpy(stInfo.achSchedule, achSharedSchedule, sizeof(stInfo.achSchedule)-1);
                        int ret = onvif_set_parking_detect_info(nSetCount, &stInfo);
                        if(ret == ONVIF_ERR_EVENT_RESOURCE_CONFLICT)
                        {
                            dlog_error("Resource allocation failed for Parking Detection.");
                            return ret;
                        }
                        if(ret == 0) nSetCount++;
                    }
                }
            }
            
            /* Cleanup remaining */
            OnvifParkingDetection_S stEmpty;
            memset(&stEmpty, 0, sizeof(stEmpty));
            stEmpty.bEnable = false;
            while(nSetCount < 16) {
                 if(onvif_set_parking_detect_info(nSetCount, &stEmpty) != 0) break;
                 nSetCount++;
            }
            nRet = SOAP_OK;
        }
        else if(!strncmp(pRule->Name, UNATTENDED_OBJECT_EVENT_RULE, strlen(UNATTENDED_OBJECT_EVENT_RULE)) && !strcmp(pRule->Type, UNATTENDED_OBJECT_EVENT_RULE_TYPE))
        {
            OnvifUnattendedObject_S stInfoTemplate;
            memset(&stInfoTemplate, 0, sizeof(stInfoTemplate));
            char achSharedSchedule[2048] = {0};
            /* Pre-scan for Schedule */
            for(int j = 0; j < pRule->Parameters->__sizeElementItem; j++) {
                 struct _tt__ItemList_ElementItem *pParam = pRule->Parameters->ElementItem + j;
                 if(pParam && pParam->Name && pParam->__any && !strcmp(pParam->Name, "Schedule")) {
                      strncpy(achSharedSchedule, pParam->__any, sizeof(achSharedSchedule)-1);
                 }
            }

            for(int j = 0; j < pRule->Parameters->__sizeSimpleItem; j++) {
                struct _tt__ItemList_SimpleItem *pParam = pRule->Parameters->SimpleItem + j;
                if(pParam && pParam->Name && pParam->Value) {
 if(!strcmp(pParam->Name, "Enable")) {
                        stInfoTemplate.bEnable = !strcmp(pParam->Value, "true");
                    }
                    else if(!strcmp(pParam->Name, ONVIF_ANALYTICS_SENS_NAME)) {
                        strncpy(stInfoTemplate.achSensitivity, pParam->Value, sizeof(stInfoTemplate.achSensitivity)-1);
                    }
                }
            }
            
            int nSetCount = 0;
            for(int j = 0; j < pRule->Parameters->__sizeElementItem; j++) {
                struct _tt__ItemList_ElementItem *pParam = pRule->Parameters->ElementItem + j;
                if(pParam && pParam->Name && pParam->__any) {
                    if(!strcmp(pParam->Name, ONVIF_ANALYTICS_FIELD_NAME)) {
                        OnvifUnattendedObject_S stInfo = stInfoTemplate;
                        char* pStr = pParam->__any;

                        /* Parse Sensitivity */
                        char* pSens = strstr(pStr, "<tt:Sensitivity>");
                        if(pSens) {
                            pSens += strlen("<tt:Sensitivity>");
                            char* pEnd = strstr(pSens, "</tt:Sensitivity>");
                            if(pEnd) {
                                size_t len = pEnd - pSens;
                                if(len < sizeof(stInfo.achSensitivity)) {
                                    strncpy(stInfo.achSensitivity, pSens, len);
                                    stInfo.achSensitivity[len] = '\0';
                                }
                            }
                        }

                        /* Parse TimeThreshold */
                        char* pThreshold = strstr(pStr, "<tt:TimeThreshold>");
                        if(pThreshold) {
                            pThreshold += strlen("<tt:TimeThreshold>");
                            char* pEnd = strstr(pThreshold, "</tt:TimeThreshold>");
                            if(pEnd) {
                                size_t len = pEnd - pThreshold;
                                if(len < sizeof(stInfo.achTimeThreshold)) {
                                    strncpy(stInfo.achTimeThreshold, pThreshold, len);
                                    stInfo.achTimeThreshold[len] = '\0';
                                }
                            }
                        }

                        int nCount = 0;
                        while(nCount < 4) {
                            pStr = strstr(pStr, "<tt:Point");
                            if(!pStr) break;
                            pStr += strlen("<tt:Point");
                            const char *x_start = strstr(pStr, "x=\"");
                            if(!x_start) break;
                            x_start += 3;
                            int x = (int)strtol(x_start, (char**)&pStr, 10);
                            const char *y_start = strstr(pStr, "y=\"");
                            if(!y_start) break;
                            y_start += 3;
                            int y = (int)strtol(y_start, (char**)&pStr, 10);
                            stInfo.stPolygon[nCount++] = (OnvifPoint_S){x, y};
                            pStr = strchr(pStr, '/');
                            if(!pStr) break;
                            pStr += 2;
                        }
                        strncpy(stInfo.achSchedule, achSharedSchedule, sizeof(stInfo.achSchedule)-1);
                        int ret = onvif_set_unattended_object_info(nSetCount, &stInfo);
                        if(ret == ONVIF_ERR_EVENT_RESOURCE_CONFLICT)
                        {
                            dlog_error("Resource allocation failed for Unattended Object.");
                            return ret;
                        }
                        if(ret == 0) nSetCount++;
                    }
                }
            }
            
            /* Cleanup remaining */
            OnvifUnattendedObject_S stEmpty;
            memset(&stEmpty, 0, sizeof(stEmpty));
            stEmpty.bEnable = false;
            while(nSetCount < 16) {
                 if(onvif_set_unattended_object_info(nSetCount, &stEmpty) != 0) break;
                 nSetCount++;
            }
            nRet = SOAP_OK;
        }
        else if(!strncmp(pRule->Name, OBJECT_REMOVAL_EVENT_RULE, strlen(OBJECT_REMOVAL_EVENT_RULE)) && !strcmp(pRule->Type, OBJECT_REMOVAL_EVENT_RULE_TYPE))
        {
            OnvifObjectRemoval_S stInfoTemplate;
            memset(&stInfoTemplate, 0, sizeof(stInfoTemplate));
            char achSharedSchedule[2048] = {0};
            /* Pre-scan for Schedule */
            for(int j = 0; j < pRule->Parameters->__sizeElementItem; j++) {
                 struct _tt__ItemList_ElementItem *pParam = pRule->Parameters->ElementItem + j;
                 if(pParam && pParam->Name && pParam->__any && !strcmp(pParam->Name, "Schedule")) {
                      strncpy(achSharedSchedule, pParam->__any, sizeof(achSharedSchedule)-1);
                 }
            }

            for(int j = 0; j < pRule->Parameters->__sizeSimpleItem; j++) {
                struct _tt__ItemList_SimpleItem *pParam = pRule->Parameters->SimpleItem + j;
                if(pParam && pParam->Name && pParam->Value) {
 if(!strcmp(pParam->Name, "Enable")) {
                        stInfoTemplate.bEnable = !strcmp(pParam->Value, "true");
                        stInfoTemplate.bEnableSet = true; /* Modified: Flag to track if Enable was explicitly set */
                    }
                    else if(!strcmp(pParam->Name, ONVIF_ANALYTICS_SENS_NAME)) {
                        strncpy(stInfoTemplate.achSensitivity, pParam->Value, sizeof(stInfoTemplate.achSensitivity)-1);
                    }
                }
            }
            
            int nSetCount = 0;
            for(int j = 0; j < pRule->Parameters->__sizeElementItem; j++) {
                struct _tt__ItemList_ElementItem *pParam = pRule->Parameters->ElementItem + j;
                if(pParam && pParam->Name && pParam->__any) {
                    if(!strcmp(pParam->Name, ONVIF_ANALYTICS_FIELD_NAME)) {
                        OnvifObjectRemoval_S stInfo = stInfoTemplate;
                        char* pStr = pParam->__any;

                        /* Parse Sensitivity */
                        char* pSens = strstr(pStr, "<tt:Sensitivity>");
                        if(pSens) {
                            pSens += strlen("<tt:Sensitivity>");
                            char* pEnd = strstr(pSens, "</tt:Sensitivity>");
                            if(pEnd) {
                                size_t len = pEnd - pSens;
                                if(len < sizeof(stInfo.achSensitivity)) {
                                    strncpy(stInfo.achSensitivity, pSens, len);
                                    stInfo.achSensitivity[len] = '\0';
                                }
                            }
                        }

                        /* Parse TimeThreshold */
                        char* pThreshold = strstr(pStr, "<tt:TimeThreshold>");
                        if(pThreshold) {
                            pThreshold += strlen("<tt:TimeThreshold>");
                            char* pEnd = strstr(pThreshold, "</tt:TimeThreshold>");
                            if(pEnd) {
                                size_t len = pEnd - pThreshold;
                                if(len < sizeof(stInfo.achTimeThreshold)) {
                                    strncpy(stInfo.achTimeThreshold, pThreshold, len);
                                    stInfo.achTimeThreshold[len] = '\0';
                                }
                            }
                        }

                        int nCount = 0;
                        while(nCount < 4) {
                            pStr = strstr(pStr, "<tt:Point");
                            if(!pStr) break;
                            pStr += strlen("<tt:Point");
                            const char *x_start = strstr(pStr, "x=\"");
                            if(!x_start) break;
                            x_start += 3;
                            int x = (int)strtol(x_start, (char**)&pStr, 10);
                            const char *y_start = strstr(pStr, "y=\"");
                            if(!y_start) break;
                            y_start += 3;
                            int y = (int)strtol(y_start, (char**)&pStr, 10);
                            stInfo.stPolygon[nCount++] = (OnvifPoint_S){x, y};
                            pStr = strchr(pStr, '/');
                            if(!pStr) break;
                            pStr += 2;
                        }
                        strncpy(stInfo.achSchedule, achSharedSchedule, sizeof(stInfo.achSchedule)-1);
                        int ret = onvif_set_object_removal_info(nSetCount, &stInfo);
                        if(ret == ONVIF_ERR_EVENT_RESOURCE_CONFLICT)
                        {
                            dlog_error("Resource allocation failed for Object Removal.");
                            return ret;
                        }
                        if(ret == 0) nSetCount++;
                    }
                }
            }
            
            /* Cleanup remaining */
            OnvifObjectRemoval_S stEmpty;
            memset(&stEmpty, 0, sizeof(stEmpty));
            stEmpty.bEnable = false;
            while(nSetCount < 16) {
                 if(onvif_set_object_removal_info(nSetCount, &stEmpty) != 0) break;
                 nSetCount++;
            }
            nRet = SOAP_OK;
        }
        else if(!strncmp(pRule->Name, PET_RECOGNITION_EVENT_RULE, strlen(PET_RECOGNITION_EVENT_RULE)) && !strcmp(pRule->Type, PET_RECOGNITION_EVENT_RULE_TYPE))
        {
            OnvifPetRecognition_S stInfoTemplate;
            memset(&stInfoTemplate, 0, sizeof(stInfoTemplate));
            char achSharedSchedule[2048] = {0};
            /* Pre-scan for Schedule */
            for(int j = 0; j < pRule->Parameters->__sizeElementItem; j++) {
                 struct _tt__ItemList_ElementItem *pParam = pRule->Parameters->ElementItem + j;
                 if(pParam && pParam->Name && pParam->__any && !strcmp(pParam->Name, "Schedule")) {
                      strncpy(achSharedSchedule, pParam->__any, sizeof(achSharedSchedule)-1);
                 }
            }

            for(int j = 0; j < pRule->Parameters->__sizeSimpleItem; j++) {
                struct _tt__ItemList_SimpleItem *pParam = pRule->Parameters->SimpleItem + j;
                if(pParam && pParam->Name && pParam->Value) {
                    if(!strcmp(pParam->Name, "Enable")) {
                        stInfoTemplate.bEnable = !strcmp(pParam->Value, "true");
                        stInfoTemplate.bEnableSet = true; /* Modified: Flag to track if Enable was explicitly set */
                    }
                    else if(!strcmp(pParam->Name, ONVIF_ANALYTICS_SENS_NAME)) {
                        strncpy(stInfoTemplate.achSensitivity, pParam->Value, sizeof(stInfoTemplate.achSensitivity)-1);
                    }
                }
            }
            
            int nSetCount = 0;
            for(int j = 0; j < pRule->Parameters->__sizeElementItem; j++) {
                struct _tt__ItemList_ElementItem *pParam = pRule->Parameters->ElementItem + j;
                if(pParam && pParam->Name && pParam->__any) {
                    if(!strcmp(pParam->Name, ONVIF_ANALYTICS_FIELD_NAME)) {
                        OnvifPetRecognition_S stInfo = stInfoTemplate;
                        char* pStr = pParam->__any;
                        int nCount = 0;
                        while(nCount < 4) {
                            pStr = strstr(pStr, "<tt:Point");
                            if(!pStr) break;
                            pStr += strlen("<tt:Point");
                            const char *x_start = strstr(pStr, "x=\"");
                            if(!x_start) break;
                            x_start += 3;
                            int x = (int)strtol(x_start, (char**)&pStr, 10);
                            const char *y_start = strstr(pStr, "y=\"");
                            if(!y_start) break;
                            y_start += 3;
                            int y = (int)strtol(y_start, (char**)&pStr, 10);
                            stInfo.stPolygon[nCount++] = (OnvifPoint_S){x, y};
                            pStr = strchr(pStr, '/');
                            if(!pStr) break;
                            pStr += 2;
                        }
                        strncpy(stInfo.achSchedule, achSharedSchedule, sizeof(stInfo.achSchedule)-1);
                        int ret = onvif_set_pet_recognition_info(nSetCount, &stInfo);
                        if(ret == ONVIF_ERR_EVENT_RESOURCE_CONFLICT)
                        {
                            dlog_error("Resource allocation failed for Pet Recognition.");
                            return ret;
                        }
                        if(ret == 0) nSetCount++;
                    }
                }
            }
            
            /* Cleanup remaining */
            OnvifPetRecognition_S stEmpty;
            memset(&stEmpty, 0, sizeof(stEmpty));
            stEmpty.bEnable = false;
            while(nSetCount < 16) {
                 if(onvif_set_pet_recognition_info(nSetCount, &stEmpty) != 0) break;
                 nSetCount++;
            }
            nRet = SOAP_OK;
        }
        else if(!strncmp(pRule->Name, FACE_CAPTURE_EVENT_RULE, strlen(FACE_CAPTURE_EVENT_RULE)) && !strcmp(pRule->Type, FACE_CAPTURE_EVENT_RULE_TYPE))
        {
            OnvifFaceCapture_S stInfoTemplate;
            memset(&stInfoTemplate, 0, sizeof(stInfoTemplate));
            char achSharedSchedule[2048] = {0};
            /* Pre-scan for Schedule */
            for(int j = 0; j < pRule->Parameters->__sizeElementItem; j++) {
                 struct _tt__ItemList_ElementItem *pParam = pRule->Parameters->ElementItem + j;
                 if(pParam && pParam->Name && pParam->__any && !strcmp(pParam->Name, "Schedule")) {
                      strncpy(achSharedSchedule, pParam->__any, sizeof(achSharedSchedule)-1);
                 }
            }

            for(int j = 0; j < pRule->Parameters->__sizeSimpleItem; j++) {
                struct _tt__ItemList_SimpleItem *pParam = pRule->Parameters->SimpleItem + j;
                if(pParam && pParam->Name && pParam->Value) {
                    if(!strcmp(pParam->Name, "Interval")) {
                        strncpy(stInfoTemplate.achInterval, pParam->Value, sizeof(stInfoTemplate.achInterval)-1);
                    }
                    else if(!strcmp(pParam->Name, "Enable")) {
                        stInfoTemplate.bEnable = !strcmp(pParam->Value, "true");
                        stInfoTemplate.bEnableSet = true; /* Modified: Flag to track if Enable was explicitly set */
                    }
                    else if(!strcmp(pParam->Name, ONVIF_ANALYTICS_SENS_NAME)) {
                        strncpy(stInfoTemplate.achSensitivity, pParam->Value, sizeof(stInfoTemplate.achSensitivity)-1);
                    }
                }
            }
            
            int nSetCount = 0;
            for(int j = 0; j < pRule->Parameters->__sizeElementItem; j++) {
                struct _tt__ItemList_ElementItem *pParam = pRule->Parameters->ElementItem + j;
                if(pParam && pParam->Name && pParam->__any) {
                    if(!strcmp(pParam->Name, ONVIF_ANALYTICS_FIELD_NAME)) {
                        OnvifFaceCapture_S stInfo = stInfoTemplate;
                        char* pStr = pParam->__any;
                        int nCount = 0;
                        while(nCount < 4) {
                            pStr = strstr(pStr, "<tt:Point");
                            if(!pStr) break;
                            pStr += strlen("<tt:Point");
                            const char *x_start = strstr(pStr, "x=\"");
                            if(!x_start) break;
                            x_start += 3;
                            int x = (int)strtol(x_start, (char**)&pStr, 10);
                            const char *y_start = strstr(pStr, "y=\"");
                            if(!y_start) break;
                            y_start += 3;
                            int y = (int)strtol(y_start, (char**)&pStr, 10);
                            stInfo.stPolygon[nCount++] = (OnvifPoint_S){x, y};
                            pStr = strchr(pStr, '/');
                            if(!pStr) break;
                            pStr += 2;
                        }
                        strncpy(stInfo.achSchedule, achSharedSchedule, sizeof(stInfo.achSchedule)-1);
                        int ret = onvif_set_face_capture_info(nSetCount, &stInfo);
                        if(ret == ONVIF_ERR_EVENT_RESOURCE_CONFLICT)
                        {
                            dlog_error("Resource allocation failed for Face Capture.");
                            return ret;
                        }
                        if(ret == 0) nSetCount++;
                    }
                }
            }
            
            /* Cleanup remaining */
            OnvifFaceCapture_S stEmpty;
            memset(&stEmpty, 0, sizeof(stEmpty));
            stEmpty.bEnable = false;
            while(nSetCount < 16) {
                 if(onvif_set_face_capture_info(nSetCount, &stEmpty) != 0) break;
                 nSetCount++;
            }
            nRet = SOAP_OK;
        }
        else if(!strncmp(pRule->Name, FIELD_EVENT_RULE, strlen(FIELD_EVENT_RULE)) && !strcmp(pRule->Type, FIELD_EVENT_RULE_TYPE))
        {
             OnvifFieldDetection_S stInfoTemplate;
             memset(&stInfoTemplate, 0, sizeof(stInfoTemplate));
            char achSharedSchedule[2048] = {0};
            /* Pre-scan for Schedule */
            for(int j = 0; j < pRule->Parameters->__sizeElementItem; j++) {
                 struct _tt__ItemList_ElementItem *pParam = pRule->Parameters->ElementItem + j;
                 if(pParam && pParam->Name && pParam->__any && !strcmp(pParam->Name, "Schedule")) {
                      strncpy(achSharedSchedule, pParam->__any, sizeof(achSharedSchedule)-1);
                 }
            }

             for(int j = 0; j < pRule->Parameters->__sizeSimpleItem; j++) {
                struct _tt__ItemList_SimpleItem *pParam = pRule->Parameters->SimpleItem + j;
                if(pParam && pParam->Name && pParam->Value) {
 if(!strcmp(pParam->Name, "Enable")) {
                        stInfoTemplate.bEnable = !strcmp(pParam->Value, "true");
                        stInfoTemplate.bEnableSet = true; /* Modified: Flag to track if Enable was explicitly set */
                    }
                    else if(!strcmp(pParam->Name, ONVIF_ANALYTICS_SENS_NAME)) {
                        strncpy(stInfoTemplate.achSensitivity, pParam->Value, sizeof(stInfoTemplate.achSensitivity)-1);
                    }
                }
             }
             
             int nSetCount = 0;
             for(int j = 0; j < pRule->Parameters->__sizeElementItem; j++) {
                struct _tt__ItemList_ElementItem *pParam = pRule->Parameters->ElementItem + j;
                if(pParam && pParam->Name && pParam->__any) {
                    if(!strcmp(pParam->Name, "Field") || !strcmp(pParam->Name, ONVIF_ANALYTICS_FIELD_NAME)) {
                         OnvifFieldDetection_S stInfo = stInfoTemplate;
                         char* pStr = pParam->__any;
                         
                        /* Parse TimeThreshold */
                        char* pThreshold = strstr(pStr, "<tt:TimeThreshold>");
                        if(pThreshold) {
                            pThreshold += strlen("<tt:TimeThreshold>");
                            char* pEnd = strstr(pThreshold, "</tt:TimeThreshold>");
                            if(pEnd) {
                                size_t len = pEnd - pThreshold;
                                if(len < sizeof(stInfo.achTimeThreshold)) {
                                    strncpy(stInfo.achTimeThreshold, pThreshold, len);
                                    stInfo.achTimeThreshold[len] = '\0';
                                }
                            }
                        }

                        /* Parse DetectionTarget */
                        char* pTarget = strstr(pStr, "<tt:DetectionTarget>");
                        if(pTarget) {
                            pTarget += strlen("<tt:DetectionTarget>");
                            char* pEnd = strstr(pTarget, "</tt:DetectionTarget>");
                            if(pEnd) {
                                size_t len = pEnd - pTarget;
                                if(len < sizeof(stInfo.achDetectionTarget)) {
                                    strncpy(stInfo.achDetectionTarget, pTarget, len);
                                    stInfo.achDetectionTarget[len] = '\0';
                                }
                            }
                        }

                         int nCount = 0;
                         while(nCount < ONVIF_ANALYTICS_POLYGON_POINT_NUM) {
                             pStr = strstr(pStr, "<tt:Point");
                             if(!pStr) break;
                             pStr += strlen("<tt:Point");
                             const char *x_start = strstr(pStr, "x=\"");
                             if(!x_start) break;
                             x_start += 3;
                             int x = (int)strtol(x_start, (char**)&pStr, 10);
                             const char *y_start = strstr(pStr, "y=\"");
                             if(!y_start) break;
                             y_start += 3;
                             int y = (int)strtol(y_start, (char**)&pStr, 10);
                             stInfo.stPolygon[nCount++] = (OnvifPoint_S){x, y};
                             pStr = strchr(pStr, '/');
                             if(!pStr) break;
                             pStr += 2;
                         }
                         stInfo.nPointNum = nCount;
                         strncpy(stInfo.achSchedule, achSharedSchedule, sizeof(stInfo.achSchedule)-1);
                         int ret = onvif_set_intrusion_info(nSetCount, &stInfo);
                         if(ret == ONVIF_ERR_EVENT_RESOURCE_CONFLICT)
                         {
                             dlog_error("Resource allocation failed for Intrusion Detection.");
                             return ret;
                         }
                         if(ret == 0) nSetCount++;
                    }
                }
             }

             /* Cleanup remaining */
             OnvifFieldDetection_S stEmpty;
             memset(&stEmpty, 0, sizeof(stEmpty));
             stEmpty.bEnable = false;
             while(nSetCount < 16) {
                  if(onvif_set_intrusion_info(nSetCount, &stEmpty) != 0) break;
                  nSetCount++;
             }
             nRet = SOAP_OK;
        }
        else if(!strncmp(pRule->Name, LINE_EVENT_RULE, strlen(LINE_EVENT_RULE)) && !strcmp(pRule->Type, LINE_EVENT_RULE_TYPE))
        {
             OnvifTripwireDetection_S stInfoTemplate;
             memset(&stInfoTemplate, 0, sizeof(stInfoTemplate));
            char achSharedSchedule[2048] = {0};
            /* Pre-scan for Schedule */
            for(int j = 0; j < pRule->Parameters->__sizeElementItem; j++) {
                 struct _tt__ItemList_ElementItem *pParam = pRule->Parameters->ElementItem + j;
                 if(pParam && pParam->Name && pParam->__any && !strcmp(pParam->Name, "Schedule")) {
                      strncpy(achSharedSchedule, pParam->__any, sizeof(achSharedSchedule)-1);
                 }
            }

             for(int j = 0; j < pRule->Parameters->__sizeSimpleItem; j++) {
                struct _tt__ItemList_SimpleItem *pParam = pRule->Parameters->SimpleItem + j;
                if(pParam && pParam->Name && pParam->Value) {
                    if(!strcmp(pParam->Name, "Direction")) {
                        strncpy(stInfoTemplate.achDirection, pParam->Value, sizeof(stInfoTemplate.achDirection)-1);
                    }
                    else if(!strcmp(pParam->Name, "Enable")) {
                        stInfoTemplate.bEnable = !strcmp(pParam->Value, "true");
                        stInfoTemplate.bEnableSet = true; /* Modified: Flag to track if Enable was explicitly set */
                    }
                    else if(!strcmp(pParam->Name, ONVIF_ANALYTICS_SENS_NAME)) {
                        strncpy(stInfoTemplate.achSensitivity, pParam->Value, sizeof(stInfoTemplate.achSensitivity)-1);
                    }
                }
             }
             
             int nSetCount = 0;
             for(int j = 0; j < pRule->Parameters->__sizeElementItem; j++) {
                struct _tt__ItemList_ElementItem *pParam = pRule->Parameters->ElementItem + j;
                if(pParam && pParam->Name && pParam->__any) {
                    if(!strcmp(pParam->Name, "Segments")) {
                         OnvifTripwireDetection_S stInfo = stInfoTemplate;
                         char* pStr = pParam->__any;
                         int nCount = 0;
                         while(nCount < 2) {
                             pStr = strstr(pStr, "<tt:Point");
                             if(!pStr) break;
                             pStr += strlen("<tt:Point");
                             const char *x_start = strstr(pStr, "x=\"");
                             if(!x_start) break;
                             x_start += 3;
                             int x = (int)strtol(x_start, (char**)&pStr, 10);
                             const char *y_start = strstr(pStr, "y=\"");
                             if(!y_start) break;
                             y_start += 3;
                             int y = (int)strtol(y_start, (char**)&pStr, 10);
                             stInfo.stPolygon[nCount++] = (OnvifPoint_S){x, y};
                             pStr = strchr(pStr, '/');
                             if(!pStr) break;
                              pStr += 2;
                          }
                          // Parse Direction
                          char* pDir = strstr(pParam->__any, "<tt:Direction>");
                          if(pDir) {
                              pDir += strlen("<tt:Direction>");
                              char* pEnd = strstr(pDir, "</tt:Direction>");
                              if(pEnd) {
                                  size_t len = pEnd - pDir;
                                  if(len < sizeof(stInfo.achDirection)) {
                                      strncpy(stInfo.achDirection, pDir, len);
                                      stInfo.achDirection[len] = '\0';
                                  }
                              }
                          }

                          /* Parse Sensitivity */
                          char* pSens = strstr(pParam->__any, "<tt:Sensitivity>");
                          if(pSens) {
                              pSens += strlen("<tt:Sensitivity>");
                              char* pEnd = strstr(pSens, "</tt:Sensitivity>");
                              if(pEnd) {
                                  size_t len = pEnd - pSens;
                                  if(len < sizeof(stInfo.achSensitivity)) {
                                      strncpy(stInfo.achSensitivity, pSens, len);
                                      stInfo.achSensitivity[len] = '\0';
                                  }
                              }
                          }

                          /* Parse DetectionTarget */
                          char* pTarget = strstr(pParam->__any, "<tt:DetectionTarget>");
                          if(pTarget) {
                              pTarget += strlen("<tt:DetectionTarget>");
                              char* pEnd = strstr(pTarget, "</tt:DetectionTarget>");
                              if(pEnd) {
                                  size_t len = pEnd - pTarget;
                                  if(len < sizeof(stInfo.achDetectionTarget)) {
                                      strncpy(stInfo.achDetectionTarget, pTarget, len);
                                      stInfo.achDetectionTarget[len] = '\0';
                                  }
                              }
                          }

                          strncpy(stInfo.achSchedule, achSharedSchedule, sizeof(stInfo.achSchedule)-1);
                          int ret = onvif_set_tripwire_info(nSetCount, &stInfo);
                          if(ret == ONVIF_ERR_EVENT_RESOURCE_CONFLICT)
                          {
                              dlog_error("Resource allocation failed for Tripwire.");
                              return ret;
                          }
                          if(ret == 0) nSetCount++;
                    }
                }
             }

             /* Cleanup remaining */
             OnvifTripwireDetection_S stEmpty;
             memset(&stEmpty, 0, sizeof(stEmpty));
             stEmpty.bEnable = false;
             while(nSetCount < 16) {
                  if(onvif_set_tripwire_info(nSetCount, &stEmpty) != 0) break;
                  nSetCount++;
             }
             nRet = SOAP_OK;
        }
    }


    if(nRet != SOAP_OK)
    {
        dlog_debug("----------规则设置失败----------");
    }
    return nRet;
}

/** Web service operation '__tan__GetServiceCapabilities' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tan__GetServiceCapabilities(struct soap* soap, struct _tan__GetServiceCapabilities *tan__GetServiceCapabilities, struct _tan__GetServiceCapabilitiesResponse *tan__GetServiceCapabilitiesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tan__GetServiceCapabilities----------");
#endif
    soap_wsse_delete_Security(soap);
    if(tan__GetServiceCapabilitiesResponse == NULL)
    {
        dlog_error("tan__GetServiceCapabilitiesResponse is NULL");
        return soap_receiver_fault(soap, "tan__GetServiceCapabilitiesResponse is NULL", NULL);    
    }
    if(tan__GetServiceCapabilitiesResponse->Capabilities == NULL)
    {
        tan__GetServiceCapabilitiesResponse->Capabilities = soap_new_tan__Capabilities(soap,-1);
    }

    tan__GetServiceCapabilitiesResponse->Capabilities->__size = 1;
    if(tan__GetServiceCapabilitiesResponse->Capabilities->RuleSupport == NULL)
    {
        tan__GetServiceCapabilitiesResponse->Capabilities->RuleSupport = soap_new_xsd__boolean(soap,-1);
    }
    *( tan__GetServiceCapabilitiesResponse->Capabilities->RuleSupport) = xsd__boolean__true_;
    if(tan__GetServiceCapabilitiesResponse->Capabilities->AnalyticsModuleSupport == NULL)
    {
        tan__GetServiceCapabilitiesResponse->Capabilities->AnalyticsModuleSupport = soap_new_xsd__boolean(soap,-1);
    }
    *( tan__GetServiceCapabilitiesResponse->Capabilities->AnalyticsModuleSupport) = xsd__boolean__true_;
    if(tan__GetServiceCapabilitiesResponse->Capabilities->CellBasedSceneDescriptionSupported == NULL)
    {
        tan__GetServiceCapabilitiesResponse->Capabilities->CellBasedSceneDescriptionSupported = soap_new_xsd__boolean(soap,-1);
    }
    *( tan__GetServiceCapabilitiesResponse->Capabilities->CellBasedSceneDescriptionSupported) = xsd__boolean__true_;
    if(tan__GetServiceCapabilitiesResponse->Capabilities->RuleOptionsSupported == NULL)
    {
        tan__GetServiceCapabilitiesResponse->Capabilities->RuleOptionsSupported = soap_new_xsd__boolean(soap,-1);
    }
    *( tan__GetServiceCapabilitiesResponse->Capabilities->RuleOptionsSupported) = xsd__boolean__true_;
    if(tan__GetServiceCapabilitiesResponse->Capabilities->AnalyticsModuleOptionsSupported == NULL)
    {
        tan__GetServiceCapabilitiesResponse->Capabilities->AnalyticsModuleOptionsSupported = soap_new_xsd__boolean(soap,-1);
    }
    *( tan__GetServiceCapabilitiesResponse->Capabilities->AnalyticsModuleOptionsSupported) = xsd__boolean__false_;

    return SOAP_OK;
}

/** Web service operation '__tan__GetSupportedAnalyticsModules' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tan__GetSupportedAnalyticsModules(struct soap* soap, struct _tan__GetSupportedAnalyticsModules *tan__GetSupportedAnalyticsModules, struct _tan__GetSupportedAnalyticsModulesResponse *tan__GetSupportedAnalyticsModulesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tan__GetSupportedAnalyticsModules----------");
#endif
    int nRet = onvif_authentication(soap);
    if(SOAP_OK != nRet)
    {
        return nRet;
    }
    /* token校验 */
    if(tan__GetSupportedAnalyticsModules != NULL && tan__GetSupportedAnalyticsModules->ConfigurationToken != NULL)
    {
        if(strcmp(tan__GetSupportedAnalyticsModules->ConfigurationToken,VIDEOANALTICS_TOKEN) != 0)
        {
            
            dlog_error("requested  VideoSourceConfiguration does not exist");
            return soap_receiver_fault(soap, "requested VideoSourceConfiguration does not exist", NULL); 
        }
    }
    else
    {
        return soap_receiver_fault(soap, "requested VideoSourceConfiguration does not exist", NULL); 
    }

    if(tan__GetSupportedAnalyticsModulesResponse == NULL)
    {
        dlog_error("tan__GetSupportedAnalyticsModulesResponse is NULL");
        return soap_receiver_fault(soap, "tan__GetSupportedAnalyticsModulesResponse is NULL", NULL);    
    }
    
    if(tan__GetSupportedAnalyticsModulesResponse->SupportedAnalyticsModules == NULL)
    {
       tan__GetSupportedAnalyticsModulesResponse->SupportedAnalyticsModules = soap_new_tt__SupportedAnalyticsModules(soap,-1);
    }
    tan__GetSupportedAnalyticsModulesResponse->SupportedAnalyticsModules->__sizeAnalyticsModuleContentSchemaLocation = 1;

    tan__GetSupportedAnalyticsModulesResponse->SupportedAnalyticsModules->AnalyticsModuleContentSchemaLocation = (char **)soap_malloc(soap,sizeof(char*));
    tan__GetSupportedAnalyticsModulesResponse->SupportedAnalyticsModules->AnalyticsModuleContentSchemaLocation[0] =soap_strdup(soap, "http://www.w3.org/2001/XMLSchema");
    
    tan__GetSupportedAnalyticsModulesResponse->SupportedAnalyticsModules->__sizeAnalyticsModuleDescription = ONVIF_ANALYTICS_SUPPORT_NUM;
    tan__GetSupportedAnalyticsModulesResponse->SupportedAnalyticsModules->AnalyticsModuleDescription = soap_new_tt__ConfigDescription(soap,tan__GetSupportedAnalyticsModulesResponse->SupportedAnalyticsModules->__sizeAnalyticsModuleDescription);
    for(int i  = 0; i < ONVIF_ANALYTICS_SUPPORT_NUM;i++)
    {
        struct tt__ConfigDescription *pAnalyticsModuleDescription = tan__GetSupportedAnalyticsModulesResponse->SupportedAnalyticsModules->AnalyticsModuleDescription + i;
        switch (i)
        {
            case MOTION_DETECTION_ALARM:
            {
                pAnalyticsModuleDescription->Name = soap_strdup(soap,MOTION_EVENT_MODULE_TYPE);
                /* 最大配置数量 */
                pAnalyticsModuleDescription->maxInstances = soap_strdup(soap,"1");
                pAnalyticsModuleDescription->Parameters = soap_new_tt__ItemListDescription(soap,-1);
                pAnalyticsModuleDescription->Parameters->__sizeSimpleItemDescription = ONVIF_CELLMOTION_MODULEPARAM_NUM;
                pAnalyticsModuleDescription->Parameters->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap,ONVIF_CELLMOTION_MODULEPARAM_NUM);
                struct _tt__ItemListDescription_SimpleItemDescription *pParameters_0 = pAnalyticsModuleDescription->Parameters->SimpleItemDescription + 0;
                struct _tt__ItemListDescription_SimpleItemDescription *pParameters_1 = pAnalyticsModuleDescription->Parameters->SimpleItemDescription + 1;

                if(pParameters_0 != NULL)
                {
                    pParameters_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
                    pParameters_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }
                if(pParameters_1 != NULL)
                {
                    pParameters_1->Name = soap_strdup(soap, ONVIF_ANALYTICS_LAYOUT_NAME);
                    pParameters_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_CELL_TYPE);
                }
            
                pAnalyticsModuleDescription->__sizeMessages = 1;
                pAnalyticsModuleDescription->Messages = soap_new__tt__ConfigDescription_Messages(soap,-1);
                pAnalyticsModuleDescription->Messages->ParentTopic = soap_strdup(soap, MOTION_EVENT_THEME);
                pAnalyticsModuleDescription->Messages->IsProperty = soap_new_xsd__boolean(soap,-1);
                *(pAnalyticsModuleDescription->Messages->IsProperty) = xsd__boolean__true_;
                pAnalyticsModuleDescription->Messages->Source = soap_new_tt__ItemListDescription(soap,-1);
                pAnalyticsModuleDescription->Messages->Source->__sizeSimpleItemDescription = 3;
                pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap,3);
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_0 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 0;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_1 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 1;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_2 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 2;
                if(pSource_0 != NULL)
                {
                    pSource_0->Name = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
                    pSource_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_1 != NULL)
                {
                    pSource_1->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN);
                    pSource_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_2 != NULL)
                {
                    pSource_2->Name = soap_strdup(soap, "Rule");
                    pSource_2->Type = soap_strdup(soap, ONVIF_ANALYTICS_STRING_TYPE);
                }            


                pAnalyticsModuleDescription->Messages->Data = soap_new_tt__ItemListDescription(soap,-1);
                pAnalyticsModuleDescription->Messages->Data->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Messages->Data->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap,1);
                struct _tt__ItemListDescription_SimpleItemDescription *pDate_0 = pAnalyticsModuleDescription->Messages->Data->SimpleItemDescription + 0;
                if(pDate_0 != NULL)
                {
                    pDate_0->Name = soap_strdup(soap, MOTION_NAME);
                    pDate_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_BOOL_TYPE);
                } 
                break;
            }
            
            case IMAGE_OBSTRUTION_ALARM:
            {
                pAnalyticsModuleDescription->Name = soap_strdup(soap,TAMPEREVENT_MODULE_TYPE);
                /* 最大配置数量 */
                pAnalyticsModuleDescription->maxInstances = soap_strdup(soap,"1");
                pAnalyticsModuleDescription->Parameters = soap_new_tt__ItemListDescription(soap,-1);
                pAnalyticsModuleDescription->Parameters->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Parameters->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap,1);
                pAnalyticsModuleDescription->Parameters->__sizeElementItemDescription = 2;
                pAnalyticsModuleDescription->Parameters->ElementItemDescription = soap_new__tt__ItemListDescription_ElementItemDescription(soap,2);
                struct _tt__ItemListDescription_SimpleItemDescription *pParameters_0 = pAnalyticsModuleDescription->Parameters->SimpleItemDescription + 0;
                struct _tt__ItemListDescription_ElementItemDescription *pParameters_1 = pAnalyticsModuleDescription->Parameters->ElementItemDescription + 0;
                struct _tt__ItemListDescription_ElementItemDescription *pParameters_2 = pAnalyticsModuleDescription->Parameters->ElementItemDescription + 1;
                if(pParameters_0 != NULL)
                {
                    pParameters_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
                    pParameters_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }
                if(pParameters_1 != NULL)
                {
                    pParameters_1->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
                    pParameters_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_TYPE);
                }
                if(pParameters_2 != NULL)
                {
                    pParameters_2->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                    pParameters_2->Type = soap_strdup(soap, ONVIF_ANALYTICS_POLYGON_TYPE);
                }

                pAnalyticsModuleDescription->__sizeMessages = 1;
                pAnalyticsModuleDescription->Messages = soap_new__tt__ConfigDescription_Messages(soap,-1);
                pAnalyticsModuleDescription->Messages->ParentTopic = soap_strdup(soap, TAMPER_EVENT_THEME);
                pAnalyticsModuleDescription->Messages->Source = soap_new_tt__ItemListDescription(soap,-1);
                pAnalyticsModuleDescription->Messages->Source->__sizeSimpleItemDescription = 3;
                pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap,3);
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_0 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 0;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_1 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 1;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_2 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 2;
                if(pSource_0 != NULL)
                {
                    pSource_0->Name = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
                    pSource_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_1 != NULL)
                {
                    pSource_1->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN);
                    pSource_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_2 != NULL)
                {
                    pSource_2->Name = soap_strdup(soap, "Rule");
                    pSource_2->Type = soap_strdup(soap, ONVIF_ANALYTICS_STRING_TYPE);
                }            


                pAnalyticsModuleDescription->Messages->Data = soap_new_tt__ItemListDescription(soap,-1);
                pAnalyticsModuleDescription->Messages->Data->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Messages->Data->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap,1);
                struct _tt__ItemListDescription_SimpleItemDescription *pDate_0 = pAnalyticsModuleDescription->Messages->Data->SimpleItemDescription + 0;
                if(pDate_0 != NULL)
                {
                    pDate_0->Name = soap_strdup(soap, TAMPER_NAME);
                    pDate_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_BOOL_TYPE);
                } 

                break;
            }
            
            case INTRUSION_ALARM:
            {
                pAnalyticsModuleDescription->Name = soap_strdup(soap,FIELD_EVENT_MODULE_TYPE);
                /* 最大配置数量 */
                pAnalyticsModuleDescription->maxInstances = soap_strdup(soap,"4");
                pAnalyticsModuleDescription->Parameters = soap_new_tt__ItemListDescription(soap,-1);
                pAnalyticsModuleDescription->Parameters->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Parameters->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap,1);
                pAnalyticsModuleDescription->Parameters->__sizeElementItemDescription = 2;
                pAnalyticsModuleDescription->Parameters->ElementItemDescription = soap_new__tt__ItemListDescription_ElementItemDescription(soap,2);
                struct _tt__ItemListDescription_SimpleItemDescription *pParameters_0 = pAnalyticsModuleDescription->Parameters->SimpleItemDescription + 0;
                struct _tt__ItemListDescription_ElementItemDescription *pParameters_1 = pAnalyticsModuleDescription->Parameters->ElementItemDescription + 0;
                struct _tt__ItemListDescription_ElementItemDescription *pParameters_2 = pAnalyticsModuleDescription->Parameters->ElementItemDescription + 1;
                if(pParameters_0 != NULL)
                {
                    pParameters_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
                    pParameters_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }
                if(pParameters_1 != NULL)
                {
                    pParameters_1->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
                    pParameters_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_TYPE);
                }
                if(pParameters_2 != NULL)
                {
                    pParameters_2->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                    pParameters_2->Type = soap_strdup(soap, ONVIF_ANALYTICS_POLYGON_TYPE);
                }

                pAnalyticsModuleDescription->__sizeMessages = 1;
                pAnalyticsModuleDescription->Messages = soap_new__tt__ConfigDescription_Messages(soap,-1);
                pAnalyticsModuleDescription->Messages->ParentTopic = soap_strdup(soap, FIELD_EVENT_THEME);
                pAnalyticsModuleDescription->Messages->Source = soap_new_tt__ItemListDescription(soap,-1);
                pAnalyticsModuleDescription->Messages->Source->__sizeSimpleItemDescription = 3;
                pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap,3);
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_0 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 0;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_1 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 1;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_2 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 2;
                if(pSource_0 != NULL)
                {
                    pSource_0->Name = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
                    pSource_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_1 != NULL)
                {
                    pSource_1->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN);
                    pSource_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_2 != NULL)
                {
                    pSource_2->Name = soap_strdup(soap, "Rule");
                    pSource_2->Type = soap_strdup(soap, ONVIF_ANALYTICS_STRING_TYPE);
                }            

                pAnalyticsModuleDescription->Messages->Key = soap_new_tt__ItemListDescription(soap,-1);
                pAnalyticsModuleDescription->Messages->Key->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Messages->Key->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap,1);
                struct _tt__ItemListDescription_SimpleItemDescription *pKey_0 = pAnalyticsModuleDescription->Messages->Key->SimpleItemDescription + 0;
                if(pKey_0 != NULL)
                {
                    pKey_0->Name = soap_strdup(soap, "ObjectId");
                    pKey_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }

                pAnalyticsModuleDescription->Messages->Data = soap_new_tt__ItemListDescription(soap,-1);
                pAnalyticsModuleDescription->Messages->Data->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Messages->Data->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap,1);
                struct _tt__ItemListDescription_SimpleItemDescription *pDate_0 = pAnalyticsModuleDescription->Messages->Data->SimpleItemDescription + 0;
                if(pDate_0 != NULL)
                {
                    pDate_0->Name = soap_strdup(soap, FIELD_NAME);
                    pDate_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_BOOL_TYPE);
                }
            
                break;
            }   
                
            case TRIPWIRE_ALARM:
            {
                pAnalyticsModuleDescription->Name = soap_strdup(soap,LINE_EVENT_MODULE_TYPE);
                /* 最大配置数量 */
                pAnalyticsModuleDescription->maxInstances = soap_strdup(soap,"4");
                pAnalyticsModuleDescription->Parameters = soap_new_tt__ItemListDescription(soap,-1);
                pAnalyticsModuleDescription->Parameters->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Parameters->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap,1);
                pAnalyticsModuleDescription->Parameters->__sizeElementItemDescription = 2;
                pAnalyticsModuleDescription->Parameters->ElementItemDescription = soap_new__tt__ItemListDescription_ElementItemDescription(soap,2);
                struct _tt__ItemListDescription_SimpleItemDescription *pParameters_0 = pAnalyticsModuleDescription->Parameters->SimpleItemDescription + 0;
                struct _tt__ItemListDescription_ElementItemDescription *pParameters_1 = pAnalyticsModuleDescription->Parameters->ElementItemDescription + 0;
                struct _tt__ItemListDescription_ElementItemDescription *pParameters_2 = pAnalyticsModuleDescription->Parameters->ElementItemDescription + 1;
                if(pParameters_0 != NULL)
                {
                    pParameters_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
                    pParameters_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }
                if(pParameters_1 != NULL)
                {
                    pParameters_1->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
                    pParameters_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_TYPE);
                }
                if(pParameters_2 != NULL)
                {
                    pParameters_2->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                    pParameters_2->Type = soap_strdup(soap, ONVIF_ANALYTICS_POLYGON_TYPE);
                }

                pAnalyticsModuleDescription->__sizeMessages = 1;
                pAnalyticsModuleDescription->Messages = soap_new__tt__ConfigDescription_Messages(soap,-1);
                pAnalyticsModuleDescription->Messages->ParentTopic = soap_strdup(soap, LINE_EVENT_THEME);
                pAnalyticsModuleDescription->Messages->Source = soap_new_tt__ItemListDescription(soap,-1);
                pAnalyticsModuleDescription->Messages->Source->__sizeSimpleItemDescription = 3;
                pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap,3);
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_0 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 0;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_1 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 1;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_2 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 2;
                if(pSource_0 != NULL)
                {
                    pSource_0->Name = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
                    pSource_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_1 != NULL)
                {
                    pSource_1->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN);
                    pSource_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_2 != NULL)
                {
                    pSource_2->Name = soap_strdup(soap, "Rule");
                    pSource_2->Type = soap_strdup(soap, ONVIF_ANALYTICS_STRING_TYPE);
                }            


                 pAnalyticsModuleDescription->Messages->Data = soap_new_tt__ItemListDescription(soap,-1);
                pAnalyticsModuleDescription->Messages->Data->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Messages->Data->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap,1);
                struct _tt__ItemListDescription_SimpleItemDescription *pDate_0 = pAnalyticsModuleDescription->Messages->Data->SimpleItemDescription + 0;
                if(pDate_0 != NULL)
                {
                    pDate_0->Name = soap_strdup(soap, FIELD_NAME);
                    pDate_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_BOOL_TYPE);
                }
                break;
            } 
                
            
            case ONVIF_ENTER_REGION:
            {
                pAnalyticsModuleDescription->Name = soap_strdup(soap, ENTER_REGION_EVENT_MODULE);
                pAnalyticsModuleDescription->Parameters = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Parameters->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Parameters->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pParam_0 = pAnalyticsModuleDescription->Parameters->SimpleItemDescription + 0;
                if(pParam_0 != NULL) {
                     pParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
                     pParam_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }
                
                pAnalyticsModuleDescription->Parameters->__sizeElementItemDescription = 2;
                pAnalyticsModuleDescription->Parameters->ElementItemDescription = soap_new__tt__ItemListDescription_ElementItemDescription(soap, 2);
                struct _tt__ItemListDescription_ElementItemDescription *pElem_0 = pAnalyticsModuleDescription->Parameters->ElementItemDescription + 0;
                struct _tt__ItemListDescription_ElementItemDescription *pElem_1 = pAnalyticsModuleDescription->Parameters->ElementItemDescription + 1;
                if(pElem_0 != NULL) {
                     pElem_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
                     pElem_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_TYPE);
                }
                if(pElem_1 != NULL) {
                     pElem_1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                     pElem_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_POLYGON_TYPE);
                }

                pAnalyticsModuleDescription->__sizeMessages = 1;
                pAnalyticsModuleDescription->Messages = soap_new__tt__ConfigDescription_Messages(soap, -1);
                pAnalyticsModuleDescription->Messages->ParentTopic = soap_strdup(soap, ENTER_REGION_EVENT_THEME);
                
                pAnalyticsModuleDescription->Messages->Source = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Messages->Source->__sizeSimpleItemDescription = 3;
                pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 3);
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_0 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 0;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_1 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 1;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_2 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 2;
                if(pSource_0 != NULL) {
                    pSource_0->Name = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
                    pSource_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_1 != NULL) {
                    pSource_1->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN);
                    pSource_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_2 != NULL) {
                    pSource_2->Name = soap_strdup(soap, "Rule");
                    pSource_2->Type = soap_strdup(soap, ONVIF_ANALYTICS_STRING_TYPE);
                }            

                pAnalyticsModuleDescription->Messages->Key = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Messages->Key->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Messages->Key->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pKey_0 = pAnalyticsModuleDescription->Messages->Key->SimpleItemDescription + 0;
                if(pKey_0 != NULL) {
                    pKey_0->Name = soap_strdup(soap, "ObjectId");
                    pKey_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }

                pAnalyticsModuleDescription->Messages->Data = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Messages->Data->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Messages->Data->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pData_0 = pAnalyticsModuleDescription->Messages->Data->SimpleItemDescription + 0;
                if(pData_0 != NULL) {
                     pData_0->Name = soap_strdup(soap, ENTER_REGION_NAME);
                     pData_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_BOOL_TYPE);
                }
                break;
            }

            case ONVIF_LEAVE_REGION:
            {
                pAnalyticsModuleDescription->Name = soap_strdup(soap, LEAVE_REGION_EVENT_MODULE);
                pAnalyticsModuleDescription->Parameters = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Parameters->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Parameters->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pParam_0 = pAnalyticsModuleDescription->Parameters->SimpleItemDescription + 0;
                if(pParam_0 != NULL) {
                     pParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
                     pParam_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }
                
                pAnalyticsModuleDescription->Parameters->__sizeElementItemDescription = 2;
                pAnalyticsModuleDescription->Parameters->ElementItemDescription = soap_new__tt__ItemListDescription_ElementItemDescription(soap, 2);
                struct _tt__ItemListDescription_ElementItemDescription *pElem_0 = pAnalyticsModuleDescription->Parameters->ElementItemDescription + 0;
                struct _tt__ItemListDescription_ElementItemDescription *pElem_1 = pAnalyticsModuleDescription->Parameters->ElementItemDescription + 1;
                if(pElem_0 != NULL) {
                     pElem_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
                     pElem_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_TYPE);
                }
                if(pElem_1 != NULL) {
                     pElem_1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                     pElem_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_POLYGON_TYPE);
                }

                pAnalyticsModuleDescription->__sizeMessages = 1;
                pAnalyticsModuleDescription->Messages = soap_new__tt__ConfigDescription_Messages(soap, -1);
                pAnalyticsModuleDescription->Messages->ParentTopic = soap_strdup(soap, LEAVE_REGION_EVENT_THEME);
                
                pAnalyticsModuleDescription->Messages->Source = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Messages->Source->__sizeSimpleItemDescription = 3;
                pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 3);
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_0 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 0;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_1 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 1;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_2 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 2;
                if(pSource_0 != NULL) {
                    pSource_0->Name = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
                    pSource_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_1 != NULL) {
                    pSource_1->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN);
                    pSource_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_2 != NULL) {
                    pSource_2->Name = soap_strdup(soap, "Rule");
                    pSource_2->Type = soap_strdup(soap, ONVIF_ANALYTICS_STRING_TYPE);
                }            

                pAnalyticsModuleDescription->Messages->Key = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Messages->Key->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Messages->Key->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pKey_0 = pAnalyticsModuleDescription->Messages->Key->SimpleItemDescription + 0;
                if(pKey_0 != NULL) {
                    pKey_0->Name = soap_strdup(soap, "ObjectId");
                    pKey_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }

                pAnalyticsModuleDescription->Messages->Data = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Messages->Data->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Messages->Data->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pData_0 = pAnalyticsModuleDescription->Messages->Data->SimpleItemDescription + 0;
                if(pData_0 != NULL) {
                     pData_0->Name = soap_strdup(soap, LEAVE_REGION_NAME);
                     pData_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_BOOL_TYPE);
                }
                break;
            }

            case ONVIF_AUDIO_ANOMALY:
            {
                pAnalyticsModuleDescription->Name = soap_strdup(soap, AUDIO_ANOMALY_EVENT_MODULE);
                pAnalyticsModuleDescription->Parameters = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Parameters->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Parameters->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pParam_0 = pAnalyticsModuleDescription->Parameters->SimpleItemDescription + 0;
                if(pParam_0 != NULL) {
                     pParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
                     pParam_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }
                
                pAnalyticsModuleDescription->__sizeMessages = 1;
                pAnalyticsModuleDescription->Messages = soap_new__tt__ConfigDescription_Messages(soap, -1);
                pAnalyticsModuleDescription->Messages->ParentTopic = soap_strdup(soap, AUDIO_ANOMALY_EVENT_THEME);
                
                pAnalyticsModuleDescription->Messages->Source = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Messages->Source->__sizeSimpleItemDescription = 3;
                pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 3);
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_0 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 0;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_1 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 1;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_2 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 2;
                if(pSource_0 != NULL) {
                    pSource_0->Name = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
                    pSource_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_1 != NULL) {
                    pSource_1->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN);
                    pSource_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_2 != NULL) {
                    pSource_2->Name = soap_strdup(soap, "Rule");
                    pSource_2->Type = soap_strdup(soap, ONVIF_ANALYTICS_STRING_TYPE);
                }            

                pAnalyticsModuleDescription->Messages->Data = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Messages->Data->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Messages->Data->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pData_0 = pAnalyticsModuleDescription->Messages->Data->SimpleItemDescription + 0;
                if(pData_0 != NULL) {
                     pData_0->Name = soap_strdup(soap, AUDIO_ANOMALY_NAME);
                     pData_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_BOOL_TYPE);
                }
                break;
            }

            case ONVIF_SCENE_CHANGE:
            {
                pAnalyticsModuleDescription->Name = soap_strdup(soap, SCENE_CHANGE_EVENT_MODULE);
                pAnalyticsModuleDescription->Parameters = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Parameters->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Parameters->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pParam_0 = pAnalyticsModuleDescription->Parameters->SimpleItemDescription + 0;
                if(pParam_0 != NULL) {
                     pParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
                     pParam_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }
                
                pAnalyticsModuleDescription->__sizeMessages = 1;
                pAnalyticsModuleDescription->Messages = soap_new__tt__ConfigDescription_Messages(soap, -1);
                pAnalyticsModuleDescription->Messages->ParentTopic = soap_strdup(soap, SCENE_CHANGE_EVENT_THEME);
                
                pAnalyticsModuleDescription->Messages->Source = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Messages->Source->__sizeSimpleItemDescription = 3;
                pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 3);
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_0 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 0;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_1 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 1;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_2 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 2;
                if(pSource_0 != NULL) {
                    pSource_0->Name = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
                    pSource_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_1 != NULL) {
                    pSource_1->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN);
                    pSource_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_2 != NULL) {
                    pSource_2->Name = soap_strdup(soap, "Rule");
                    pSource_2->Type = soap_strdup(soap, ONVIF_ANALYTICS_STRING_TYPE);
                }            

                pAnalyticsModuleDescription->Messages->Data = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Messages->Data->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Messages->Data->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pData_0 = pAnalyticsModuleDescription->Messages->Data->SimpleItemDescription + 0;
                if(pData_0 != NULL) {
                     pData_0->Name = soap_strdup(soap, SCENE_CHANGE_NAME);
                     pData_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_BOOL_TYPE);
                }
                break;
            }

            case ONVIF_FACE_DETECT:
            {
                pAnalyticsModuleDescription->Name = soap_strdup(soap, FACE_DETECT_EVENT_MODULE);
                pAnalyticsModuleDescription->Parameters = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Parameters->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Parameters->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pParam_0 = pAnalyticsModuleDescription->Parameters->SimpleItemDescription + 0;
                if(pParam_0 != NULL) {
                     pParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
                     pParam_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }
                
                pAnalyticsModuleDescription->Parameters->__sizeElementItemDescription = 2;
                pAnalyticsModuleDescription->Parameters->ElementItemDescription = soap_new__tt__ItemListDescription_ElementItemDescription(soap, 2);
                struct _tt__ItemListDescription_ElementItemDescription *pElem_0 = pAnalyticsModuleDescription->Parameters->ElementItemDescription + 0;
                struct _tt__ItemListDescription_ElementItemDescription *pElem_1 = pAnalyticsModuleDescription->Parameters->ElementItemDescription + 1;
                if(pElem_0 != NULL) {
                     pElem_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
                     pElem_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_TYPE);
                }
                if(pElem_1 != NULL) {
                     pElem_1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                     pElem_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_POLYGON_TYPE);
                }

                pAnalyticsModuleDescription->__sizeMessages = 1;
                pAnalyticsModuleDescription->Messages = soap_new__tt__ConfigDescription_Messages(soap, -1);
                pAnalyticsModuleDescription->Messages->ParentTopic = soap_strdup(soap, FACE_DETECT_EVENT_THEME);
                
                pAnalyticsModuleDescription->Messages->Source = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Messages->Source->__sizeSimpleItemDescription = 3;
                pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 3);
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_0 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 0;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_1 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 1;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_2 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 2;
                if(pSource_0 != NULL) {
                    pSource_0->Name = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
                    pSource_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_1 != NULL) {
                    pSource_1->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN);
                    pSource_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_2 != NULL) {
                    pSource_2->Name = soap_strdup(soap, "Rule");
                    pSource_2->Type = soap_strdup(soap, ONVIF_ANALYTICS_STRING_TYPE);
                }            

                pAnalyticsModuleDescription->Messages->Key = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Messages->Key->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Messages->Key->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pKey_0 = pAnalyticsModuleDescription->Messages->Key->SimpleItemDescription + 0;
                if(pKey_0 != NULL) {
                    pKey_0->Name = soap_strdup(soap, "ObjectId");
                    pKey_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }

                pAnalyticsModuleDescription->Messages->Data = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Messages->Data->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Messages->Data->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pData_0 = pAnalyticsModuleDescription->Messages->Data->SimpleItemDescription + 0;
                if(pData_0 != NULL) {
                     pData_0->Name = soap_strdup(soap, FACE_DETECT_NAME);
                     pData_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_BOOL_TYPE);
                }
                break;
            }

            case ONVIF_LOITERING_DETECT:
            {
                pAnalyticsModuleDescription->Name = soap_strdup(soap, LOITERING_DETECT_EVENT_MODULE);
                pAnalyticsModuleDescription->Parameters = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Parameters->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Parameters->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pParam_0 = pAnalyticsModuleDescription->Parameters->SimpleItemDescription + 0;
                if(pParam_0 != NULL) {
                     pParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
                     pParam_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }
                
                pAnalyticsModuleDescription->Parameters->__sizeElementItemDescription = 2;
                pAnalyticsModuleDescription->Parameters->ElementItemDescription = soap_new__tt__ItemListDescription_ElementItemDescription(soap, 2);
                struct _tt__ItemListDescription_ElementItemDescription *pElem_0 = pAnalyticsModuleDescription->Parameters->ElementItemDescription + 0;
                struct _tt__ItemListDescription_ElementItemDescription *pElem_1 = pAnalyticsModuleDescription->Parameters->ElementItemDescription + 1;
                if(pElem_0 != NULL) {
                     pElem_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
                     pElem_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_TYPE);
                }
                if(pElem_1 != NULL) {
                     pElem_1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                     pElem_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_POLYGON_TYPE);
                }

                pAnalyticsModuleDescription->__sizeMessages = 1;
                pAnalyticsModuleDescription->Messages = soap_new__tt__ConfigDescription_Messages(soap, -1);
                pAnalyticsModuleDescription->Messages->ParentTopic = soap_strdup(soap, LOITERING_DETECT_EVENT_THEME);
                
                pAnalyticsModuleDescription->Messages->Source = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Messages->Source->__sizeSimpleItemDescription = 3;
                pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 3);
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_0 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 0;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_1 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 1;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_2 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 2;
                if(pSource_0 != NULL) {
                    pSource_0->Name = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
                    pSource_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_1 != NULL) {
                    pSource_1->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN);
                    pSource_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_2 != NULL) {
                    pSource_2->Name = soap_strdup(soap, "Rule");
                    pSource_2->Type = soap_strdup(soap, ONVIF_ANALYTICS_STRING_TYPE);
                }            

                pAnalyticsModuleDescription->Messages->Key = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Messages->Key->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Messages->Key->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pKey_0 = pAnalyticsModuleDescription->Messages->Key->SimpleItemDescription + 0;
                if(pKey_0 != NULL) {
                    pKey_0->Name = soap_strdup(soap, "ObjectId");
                    pKey_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }

                pAnalyticsModuleDescription->Messages->Data = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Messages->Data->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Messages->Data->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pData_0 = pAnalyticsModuleDescription->Messages->Data->SimpleItemDescription + 0;
                if(pData_0 != NULL) {
                     pData_0->Name = soap_strdup(soap, LOITERING_DETECT_NAME);
                     pData_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_BOOL_TYPE);
                }
                break;
            }

            case ONVIF_CROWD_GATHERING:
            {
                pAnalyticsModuleDescription->Name = soap_strdup(soap, CROWD_GATHERING_EVENT_MODULE);
                pAnalyticsModuleDescription->Parameters = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Parameters->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Parameters->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pParam_0 = pAnalyticsModuleDescription->Parameters->SimpleItemDescription + 0;
                if(pParam_0 != NULL) {
                     pParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
                     pParam_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }
                
                pAnalyticsModuleDescription->Parameters->__sizeElementItemDescription = 2;
                pAnalyticsModuleDescription->Parameters->ElementItemDescription = soap_new__tt__ItemListDescription_ElementItemDescription(soap, 2);
                struct _tt__ItemListDescription_ElementItemDescription *pElem_0 = pAnalyticsModuleDescription->Parameters->ElementItemDescription + 0;
                struct _tt__ItemListDescription_ElementItemDescription *pElem_1 = pAnalyticsModuleDescription->Parameters->ElementItemDescription + 1;
                if(pElem_0 != NULL) {
                     pElem_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
                     pElem_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_TYPE);
                }
                if(pElem_1 != NULL) {
                     pElem_1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                     pElem_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_POLYGON_TYPE);
                }

                pAnalyticsModuleDescription->__sizeMessages = 1;
                pAnalyticsModuleDescription->Messages = soap_new__tt__ConfigDescription_Messages(soap, -1);
                pAnalyticsModuleDescription->Messages->ParentTopic = soap_strdup(soap, CROWD_GATHERING_EVENT_THEME);
                
                pAnalyticsModuleDescription->Messages->Source = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Messages->Source->__sizeSimpleItemDescription = 3;
                pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 3);
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_0 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 0;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_1 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 1;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_2 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 2;
                if(pSource_0 != NULL) {
                    pSource_0->Name = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
                    pSource_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_1 != NULL) {
                    pSource_1->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN);
                    pSource_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_2 != NULL) {
                    pSource_2->Name = soap_strdup(soap, "Rule");
                    pSource_2->Type = soap_strdup(soap, ONVIF_ANALYTICS_STRING_TYPE);
                }            

                pAnalyticsModuleDescription->Messages->Key = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Messages->Key->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Messages->Key->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pKey_0 = pAnalyticsModuleDescription->Messages->Key->SimpleItemDescription + 0;
                if(pKey_0 != NULL) {
                    pKey_0->Name = soap_strdup(soap, "ObjectId");
                    pKey_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }

                pAnalyticsModuleDescription->Messages->Data = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Messages->Data->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Messages->Data->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pData_0 = pAnalyticsModuleDescription->Messages->Data->SimpleItemDescription + 0;
                if(pData_0 != NULL) {
                     pData_0->Name = soap_strdup(soap, CROWD_GATHERING_NAME);
                     pData_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_BOOL_TYPE);
                }
                break;
            }

            case ONVIF_PARKING_DETECT:
            {
                pAnalyticsModuleDescription->Name = soap_strdup(soap, PARKING_DETECT_EVENT_MODULE);
                pAnalyticsModuleDescription->Parameters = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Parameters->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Parameters->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pParam_0 = pAnalyticsModuleDescription->Parameters->SimpleItemDescription + 0;
                if(pParam_0 != NULL) {
                     pParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
                     pParam_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }
                
                pAnalyticsModuleDescription->Parameters->__sizeElementItemDescription = 2;
                pAnalyticsModuleDescription->Parameters->ElementItemDescription = soap_new__tt__ItemListDescription_ElementItemDescription(soap, 2);
                struct _tt__ItemListDescription_ElementItemDescription *pElem_0 = pAnalyticsModuleDescription->Parameters->ElementItemDescription + 0;
                struct _tt__ItemListDescription_ElementItemDescription *pElem_1 = pAnalyticsModuleDescription->Parameters->ElementItemDescription + 1;
                if(pElem_0 != NULL) {
                     pElem_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
                     pElem_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_TYPE);
                }
                if(pElem_1 != NULL) {
                     pElem_1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                     pElem_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_POLYGON_TYPE);
                }

                pAnalyticsModuleDescription->__sizeMessages = 1;
                pAnalyticsModuleDescription->Messages = soap_new__tt__ConfigDescription_Messages(soap, -1);
                pAnalyticsModuleDescription->Messages->ParentTopic = soap_strdup(soap, PARKING_DETECT_EVENT_THEME);
                
                pAnalyticsModuleDescription->Messages->Source = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Messages->Source->__sizeSimpleItemDescription = 3;
                pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 3);
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_0 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 0;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_1 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 1;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_2 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 2;
                if(pSource_0 != NULL) {
                    pSource_0->Name = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
                    pSource_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_1 != NULL) {
                    pSource_1->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN);
                    pSource_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_2 != NULL) {
                    pSource_2->Name = soap_strdup(soap, "Rule");
                    pSource_2->Type = soap_strdup(soap, ONVIF_ANALYTICS_STRING_TYPE);
                }            

                pAnalyticsModuleDescription->Messages->Key = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Messages->Key->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Messages->Key->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pKey_0 = pAnalyticsModuleDescription->Messages->Key->SimpleItemDescription + 0;
                if(pKey_0 != NULL) {
                    pKey_0->Name = soap_strdup(soap, "ObjectId");
                    pKey_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }

                pAnalyticsModuleDescription->Messages->Data = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Messages->Data->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Messages->Data->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pData_0 = pAnalyticsModuleDescription->Messages->Data->SimpleItemDescription + 0;
                if(pData_0 != NULL) {
                     pData_0->Name = soap_strdup(soap, PARKING_DETECT_NAME);
                     pData_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_BOOL_TYPE);
                }
                break;
            }

            case ONVIF_UNATTENDED_OBJECT:
            {
                pAnalyticsModuleDescription->Name = soap_strdup(soap, UNATTENDED_OBJECT_EVENT_MODULE);
                pAnalyticsModuleDescription->Parameters = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Parameters->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Parameters->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pParam_0 = pAnalyticsModuleDescription->Parameters->SimpleItemDescription + 0;
                if(pParam_0 != NULL) {
                     pParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
                     pParam_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }
                
                pAnalyticsModuleDescription->Parameters->__sizeElementItemDescription = 2;
                pAnalyticsModuleDescription->Parameters->ElementItemDescription = soap_new__tt__ItemListDescription_ElementItemDescription(soap, 2);
                struct _tt__ItemListDescription_ElementItemDescription *pElem_0 = pAnalyticsModuleDescription->Parameters->ElementItemDescription + 0;
                struct _tt__ItemListDescription_ElementItemDescription *pElem_1 = pAnalyticsModuleDescription->Parameters->ElementItemDescription + 1;
                if(pElem_0 != NULL) {
                     pElem_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
                     pElem_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_TYPE);
                }
                if(pElem_1 != NULL) {
                     pElem_1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                     pElem_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_POLYGON_TYPE);
                }

                pAnalyticsModuleDescription->__sizeMessages = 1;
                pAnalyticsModuleDescription->Messages = soap_new__tt__ConfigDescription_Messages(soap, -1);
                pAnalyticsModuleDescription->Messages->ParentTopic = soap_strdup(soap, UNATTENDED_OBJECT_EVENT_THEME);
                
                pAnalyticsModuleDescription->Messages->Source = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Messages->Source->__sizeSimpleItemDescription = 3;
                pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 3);
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_0 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 0;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_1 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 1;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_2 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 2;
                if(pSource_0 != NULL) {
                    pSource_0->Name = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
                    pSource_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_1 != NULL) {
                    pSource_1->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN);
                    pSource_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_2 != NULL) {
                    pSource_2->Name = soap_strdup(soap, "Rule");
                    pSource_2->Type = soap_strdup(soap, ONVIF_ANALYTICS_STRING_TYPE);
                }            

                pAnalyticsModuleDescription->Messages->Key = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Messages->Key->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Messages->Key->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pKey_0 = pAnalyticsModuleDescription->Messages->Key->SimpleItemDescription + 0;
                if(pKey_0 != NULL) {
                    pKey_0->Name = soap_strdup(soap, "ObjectId");
                    pKey_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }

                pAnalyticsModuleDescription->Messages->Data = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Messages->Data->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Messages->Data->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pData_0 = pAnalyticsModuleDescription->Messages->Data->SimpleItemDescription + 0;
                if(pData_0 != NULL) {
                     pData_0->Name = soap_strdup(soap, UNATTENDED_OBJECT_NAME);
                     pData_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_BOOL_TYPE);
                }
                break;
            }

            case ONVIF_OBJECT_REMOVAL:
            {
                pAnalyticsModuleDescription->Name = soap_strdup(soap, OBJECT_REMOVAL_EVENT_MODULE);
                pAnalyticsModuleDescription->Parameters = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Parameters->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Parameters->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pParam_0 = pAnalyticsModuleDescription->Parameters->SimpleItemDescription + 0;
                if(pParam_0 != NULL) {
                     pParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
                     pParam_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }
                
                pAnalyticsModuleDescription->Parameters->__sizeElementItemDescription = 2;
                pAnalyticsModuleDescription->Parameters->ElementItemDescription = soap_new__tt__ItemListDescription_ElementItemDescription(soap, 2);
                struct _tt__ItemListDescription_ElementItemDescription *pElem_0 = pAnalyticsModuleDescription->Parameters->ElementItemDescription + 0;
                struct _tt__ItemListDescription_ElementItemDescription *pElem_1 = pAnalyticsModuleDescription->Parameters->ElementItemDescription + 1;
                if(pElem_0 != NULL) {
                     pElem_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
                     pElem_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_TYPE);
                }
                if(pElem_1 != NULL) {
                     pElem_1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                     pElem_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_POLYGON_TYPE);
                }

                pAnalyticsModuleDescription->__sizeMessages = 1;
                pAnalyticsModuleDescription->Messages = soap_new__tt__ConfigDescription_Messages(soap, -1);
                pAnalyticsModuleDescription->Messages->ParentTopic = soap_strdup(soap, OBJECT_REMOVAL_EVENT_THEME);
                
                pAnalyticsModuleDescription->Messages->Source = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Messages->Source->__sizeSimpleItemDescription = 3;
                pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 3);
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_0 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 0;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_1 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 1;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_2 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 2;
                if(pSource_0 != NULL) {
                    pSource_0->Name = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
                    pSource_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_1 != NULL) {
                    pSource_1->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN);
                    pSource_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_2 != NULL) {
                    pSource_2->Name = soap_strdup(soap, "Rule");
                    pSource_2->Type = soap_strdup(soap, ONVIF_ANALYTICS_STRING_TYPE);
                }            

                pAnalyticsModuleDescription->Messages->Key = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Messages->Key->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Messages->Key->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pKey_0 = pAnalyticsModuleDescription->Messages->Key->SimpleItemDescription + 0;
                if(pKey_0 != NULL) {
                    pKey_0->Name = soap_strdup(soap, "ObjectId");
                    pKey_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }

                pAnalyticsModuleDescription->Messages->Data = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Messages->Data->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Messages->Data->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pData_0 = pAnalyticsModuleDescription->Messages->Data->SimpleItemDescription + 0;
                if(pData_0 != NULL) {
                     pData_0->Name = soap_strdup(soap, OBJECT_REMOVAL_NAME);
                     pData_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_BOOL_TYPE);
                }
                break;
            }

            case ONVIF_PET_RECOGNITION:
            {
                pAnalyticsModuleDescription->Name = soap_strdup(soap, PET_RECOGNITION_EVENT_MODULE);
                pAnalyticsModuleDescription->Parameters = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Parameters->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Parameters->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pParam_0 = pAnalyticsModuleDescription->Parameters->SimpleItemDescription + 0;
                if(pParam_0 != NULL) {
                     pParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
                     pParam_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }
                
                pAnalyticsModuleDescription->Parameters->__sizeElementItemDescription = 2;
                pAnalyticsModuleDescription->Parameters->ElementItemDescription = soap_new__tt__ItemListDescription_ElementItemDescription(soap, 2);
                struct _tt__ItemListDescription_ElementItemDescription *pElem_0 = pAnalyticsModuleDescription->Parameters->ElementItemDescription + 0;
                struct _tt__ItemListDescription_ElementItemDescription *pElem_1 = pAnalyticsModuleDescription->Parameters->ElementItemDescription + 1;
                if(pElem_0 != NULL) {
                     pElem_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
                     pElem_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_TYPE);
                }
                if(pElem_1 != NULL) {
                     pElem_1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                     pElem_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_POLYGON_TYPE);
                }

                pAnalyticsModuleDescription->__sizeMessages = 1;
                pAnalyticsModuleDescription->Messages = soap_new__tt__ConfigDescription_Messages(soap, -1);
                pAnalyticsModuleDescription->Messages->ParentTopic = soap_strdup(soap, PET_RECOGNITION_EVENT_THEME);
                
                pAnalyticsModuleDescription->Messages->Source = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Messages->Source->__sizeSimpleItemDescription = 3;
                pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 3);
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_0 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 0;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_1 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 1;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_2 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 2;
                if(pSource_0 != NULL) {
                    pSource_0->Name = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
                    pSource_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_1 != NULL) {
                    pSource_1->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN);
                    pSource_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_2 != NULL) {
                    pSource_2->Name = soap_strdup(soap, "Rule");
                    pSource_2->Type = soap_strdup(soap, ONVIF_ANALYTICS_STRING_TYPE);
                }            

                pAnalyticsModuleDescription->Messages->Key = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Messages->Key->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Messages->Key->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pKey_0 = pAnalyticsModuleDescription->Messages->Key->SimpleItemDescription + 0;
                if(pKey_0 != NULL) {
                    pKey_0->Name = soap_strdup(soap, "ObjectId");
                    pKey_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }

                pAnalyticsModuleDescription->Messages->Data = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Messages->Data->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Messages->Data->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pData_0 = pAnalyticsModuleDescription->Messages->Data->SimpleItemDescription + 0;
                if(pData_0 != NULL) {
                     pData_0->Name = soap_strdup(soap, PET_RECOGNITION_NAME);
                     pData_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_BOOL_TYPE);
                }
                break;
            }

            case ONVIF_FACE_CAPTURE:
            {
                pAnalyticsModuleDescription->Name = soap_strdup(soap, FACE_CAPTURE_EVENT_MODULE);
                pAnalyticsModuleDescription->Parameters = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Parameters->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Parameters->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pParam_0 = pAnalyticsModuleDescription->Parameters->SimpleItemDescription + 0;
                if(pParam_0 != NULL) {
                     pParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
                     pParam_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }
                
                pAnalyticsModuleDescription->Parameters->__sizeElementItemDescription = 2;
                pAnalyticsModuleDescription->Parameters->ElementItemDescription = soap_new__tt__ItemListDescription_ElementItemDescription(soap, 2);
                struct _tt__ItemListDescription_ElementItemDescription *pElem_0 = pAnalyticsModuleDescription->Parameters->ElementItemDescription + 0;
                struct _tt__ItemListDescription_ElementItemDescription *pElem_1 = pAnalyticsModuleDescription->Parameters->ElementItemDescription + 1;
                if(pElem_0 != NULL) {
                     pElem_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
                     pElem_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_TYPE);
                }
                if(pElem_1 != NULL) {
                     pElem_1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                     pElem_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_POLYGON_TYPE);
                }

                pAnalyticsModuleDescription->__sizeMessages = 1;
                pAnalyticsModuleDescription->Messages = soap_new__tt__ConfigDescription_Messages(soap, -1);
                pAnalyticsModuleDescription->Messages->ParentTopic = soap_strdup(soap, FACE_CAPTURE_EVENT_THEME);
                
                pAnalyticsModuleDescription->Messages->Source = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Messages->Source->__sizeSimpleItemDescription = 3;
                pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 3);
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_0 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 0;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_1 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 1;
                struct _tt__ItemListDescription_SimpleItemDescription *pSource_2 = pAnalyticsModuleDescription->Messages->Source->SimpleItemDescription + 2;
                if(pSource_0 != NULL) {
                    pSource_0->Name = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
                    pSource_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_1 != NULL) {
                    pSource_1->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN);
                    pSource_1->Type = soap_strdup(soap, ONVIF_ANALYTICS_TOKEN_TYPE);
                }   
                if(pSource_2 != NULL) {
                    pSource_2->Name = soap_strdup(soap, "Rule");
                    pSource_2->Type = soap_strdup(soap, ONVIF_ANALYTICS_STRING_TYPE);
                }            

                pAnalyticsModuleDescription->Messages->Key = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Messages->Key->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Messages->Key->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pKey_0 = pAnalyticsModuleDescription->Messages->Key->SimpleItemDescription + 0;
                if(pKey_0 != NULL) {
                    pKey_0->Name = soap_strdup(soap, "ObjectId");
                    pKey_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_INTEGER_TYPE);
                }

                pAnalyticsModuleDescription->Messages->Data = soap_new_tt__ItemListDescription(soap, -1);
                pAnalyticsModuleDescription->Messages->Data->__sizeSimpleItemDescription = 1;
                pAnalyticsModuleDescription->Messages->Data->SimpleItemDescription = soap_new__tt__ItemListDescription_SimpleItemDescription(soap, 1);
                struct _tt__ItemListDescription_SimpleItemDescription *pData_0 = pAnalyticsModuleDescription->Messages->Data->SimpleItemDescription + 0;
                if(pData_0 != NULL) {
                     pData_0->Name = soap_strdup(soap, FACE_CAPTURE_NAME);
                     pData_0->Type = soap_strdup(soap, ONVIF_ANALYTICS_BOOL_TYPE);
                }
                break;
            }

            default:
                break;
        }
        
    }
    return SOAP_OK;
}

/** Web service operation '__tan__CreateAnalyticsModules' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tan__CreateAnalyticsModules(struct soap* soap, struct _tan__CreateAnalyticsModules *tan__CreateAnalyticsModules, struct _tan__CreateAnalyticsModulesResponse *tan__CreateAnalyticsModulesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tan__CreateAnalyticsModules----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tan__DeleteAnalyticsModules' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tan__DeleteAnalyticsModules(struct soap* soap, struct _tan__DeleteAnalyticsModules *tan__DeleteAnalyticsModules, struct _tan__DeleteAnalyticsModulesResponse *tan__DeleteAnalyticsModulesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tan__DeleteAnalyticsModules----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tan__GetAnalyticsModules' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tan__GetAnalyticsModules(struct soap* soap, struct _tan__GetAnalyticsModules *tan__GetAnalyticsModules, struct _tan__GetAnalyticsModulesResponse *tan__GetAnalyticsModulesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tan__GetAnalyticsModules----------");
#endif
    int nRet = onvif_authentication(soap);
    if(SOAP_OK != nRet)
    {
        return nRet;
    }
    /* token校验 */
    if(tan__GetAnalyticsModules != NULL && tan__GetAnalyticsModules->ConfigurationToken != NULL)
    {
        if(strcmp(tan__GetAnalyticsModules->ConfigurationToken,VIDEOANALTICS_TOKEN) != 0)
        {
            
            dlog_error("requested  VIDEOANALTICS_TOKEN does not exist");
            return soap_receiver_fault(soap, "requested VIDEOANALTICS_TOKEN does not exist", NULL); 
        }
    }
    else
    {
        return soap_receiver_fault(soap, "requested VIDEOANALTICS_TOKEN does not exist", NULL); 
    }

    if(tan__GetAnalyticsModulesResponse == NULL)
    {
        dlog_error("tan__GetRulesResponse is NULL");
        return soap_receiver_fault(soap, "tan__GetRulesResponse is NULL", NULL);    
    }
#if 0
    tan__GetAnalyticsModulesResponse->__sizeAnalyticsModule = ONVIF_ANALYTICS_SUPPORT_NUM;
    tan__GetAnalyticsModulesResponse->AnalyticsModule = soap_new_tt__Config(soap,tan__GetAnalyticsModulesResponse->__sizeAnalyticsModule);

    /* 分析模块获取 */
    for(int i  = 0; i < tan__GetAnalyticsModulesResponse->__sizeAnalyticsModule;i++)
    {
        struct tt__Config *pAnalyticsModule = tan__GetAnalyticsModulesResponse->AnalyticsModule + i;
        
       
        if(pAnalyticsModule == NULL )
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

            break;
        }
           
        case IMAGE_OBSTRUTION_ALARM:
        {
            dlog_debug("获取遮挡报警配置");
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

        case ONVIF_ENTER_REGION:
        {
            OnvifRegionDetection_S stInfo;
            onvif_get_enter_region_info(&stInfo);
            pAnalyticsModule->Name = soap_strdup(soap, ENTER_REGION_EVENT_MODULE);
            pAnalyticsModule->Type = soap_strdup(soap, ENTER_REGION_EVENT_MODULE_TYPE);
            pAnalyticsModule->Parameters = soap_new_tt__ItemList(soap,-1);
            pAnalyticsModule->Parameters->__sizeSimpleItem = 1;
            pAnalyticsModule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap,-1);
            pAnalyticsModule->Parameters->SimpleItem->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
            pAnalyticsModule->Parameters->SimpleItem->Value = soap_strdup(soap, stInfo.achSensitivity);
            
            pAnalyticsModule->Parameters->__sizeElementItem = 2;
            pAnalyticsModule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap,2);
            struct _tt__ItemList_ElementItem *pAnalyticsModulParam_0 = pAnalyticsModule->Parameters->ElementItem + 0;
            struct _tt__ItemList_ElementItem *pAnalyticsModulParam_1 = pAnalyticsModule->Parameters->ElementItem + 1;
            if(pAnalyticsModulParam_0 != NULL) {
                pAnalyticsModulParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
                pAnalyticsModulParam_0->__any = soap_strdup(soap, ONVIF_FIELD_TRANSFORMATION_MACRO);
            }
            if(pAnalyticsModulParam_1 != NULL) {
                pAnalyticsModulParam_1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                pAnalyticsModulParam_1->__any = soap_strdup(soap, ONVIF_FIELD_POLYGON_CONFIG_MACRO);
            }
            break;
        }

        case ONVIF_LEAVE_REGION:
        {
            OnvifRegionDetection_S stInfo;
            onvif_get_leave_region_info(&stInfo);
            pAnalyticsModule->Name = soap_strdup(soap, LEAVE_REGION_EVENT_MODULE);
            pAnalyticsModule->Type = soap_strdup(soap, LEAVE_REGION_EVENT_MODULE_TYPE);
            pAnalyticsModule->Parameters = soap_new_tt__ItemList(soap,-1);
            pAnalyticsModule->Parameters->__sizeSimpleItem = 1;
            pAnalyticsModule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap,-1);
            pAnalyticsModule->Parameters->SimpleItem->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
            pAnalyticsModule->Parameters->SimpleItem->Value = soap_strdup(soap, stInfo.achSensitivity);
            
            pAnalyticsModule->Parameters->__sizeElementItem = 2;
            pAnalyticsModule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap,2);
            struct _tt__ItemList_ElementItem *pAnalyticsModulParam_0 = pAnalyticsModule->Parameters->ElementItem + 0;
            struct _tt__ItemList_ElementItem *pAnalyticsModulParam_1 = pAnalyticsModule->Parameters->ElementItem + 1;
            if(pAnalyticsModulParam_0 != NULL) {
                pAnalyticsModulParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
                pAnalyticsModulParam_0->__any = soap_strdup(soap, ONVIF_FIELD_TRANSFORMATION_MACRO);
            }
            if(pAnalyticsModulParam_1 != NULL) {
                pAnalyticsModulParam_1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                pAnalyticsModulParam_1->__any = soap_strdup(soap, ONVIF_FIELD_POLYGON_CONFIG_MACRO);
            }
            break;
        }

        case ONVIF_AUDIO_ANOMALY:
        {
            OnvifAudioAnomaly_S stInfo;
            onvif_get_audio_anomaly_info(&stInfo);
            pAnalyticsModule->Name = soap_strdup(soap, AUDIO_ANOMALY_EVENT_MODULE);
            pAnalyticsModule->Type = soap_strdup(soap, AUDIO_ANOMALY_EVENT_MODULE_TYPE);
            pAnalyticsModule->Parameters = soap_new_tt__ItemList(soap,-1);
            pAnalyticsModule->Parameters->__sizeSimpleItem = 1;
            pAnalyticsModule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap,-1);
            pAnalyticsModule->Parameters->SimpleItem->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
            pAnalyticsModule->Parameters->SimpleItem->Value = soap_strdup(soap, stInfo.achSensitivity);
            /* No Field/Trans for Audio */
            pAnalyticsModule->Parameters->__sizeElementItem = 0;
            break;
        }

        case ONVIF_SCENE_CHANGE:
        {
            OnvifSceneChange_S stInfo;
            onvif_get_scene_change_info(&stInfo);
            pAnalyticsModule->Name = soap_strdup(soap, SCENE_CHANGE_EVENT_MODULE);
            pAnalyticsModule->Type = soap_strdup(soap, SCENE_CHANGE_EVENT_MODULE_TYPE);
            pAnalyticsModule->Parameters = soap_new_tt__ItemList(soap,-1);
            pAnalyticsModule->Parameters->__sizeSimpleItem = 1;
            pAnalyticsModule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap,-1);
            pAnalyticsModule->Parameters->SimpleItem->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
            pAnalyticsModule->Parameters->SimpleItem->Value = soap_strdup(soap, stInfo.achSensitivity);
            /* No Field/Trans for Scene Change */
            pAnalyticsModule->Parameters->__sizeElementItem = 0;
            break;
        }

        case ONVIF_FACE_DETECT:
        {
            OnvifFaceDetection_S stInfo;
            onvif_get_face_detect_info(&stInfo);
            pAnalyticsModule->Name = soap_strdup(soap, FACE_DETECT_EVENT_MODULE);
            pAnalyticsModule->Type = soap_strdup(soap, FACE_DETECT_EVENT_MODULE_TYPE);
            pAnalyticsModule->Parameters = soap_new_tt__ItemList(soap,-1);
            pAnalyticsModule->Parameters->__sizeSimpleItem = 1;
            pAnalyticsModule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap,-1);
            pAnalyticsModule->Parameters->SimpleItem->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
            pAnalyticsModule->Parameters->SimpleItem->Value = soap_strdup(soap, stInfo.achSensitivity);
            
            pAnalyticsModule->Parameters->__sizeElementItem = 2;
            pAnalyticsModule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap,2);
            struct _tt__ItemList_ElementItem *pAnalyticsModulParam_0 = pAnalyticsModule->Parameters->ElementItem + 0;
            struct _tt__ItemList_ElementItem *pAnalyticsModulParam_1 = pAnalyticsModule->Parameters->ElementItem + 1;
            if(pAnalyticsModulParam_0 != NULL) {
                pAnalyticsModulParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
                pAnalyticsModulParam_0->__any = soap_strdup(soap, ONVIF_FIELD_TRANSFORMATION_MACRO);
            }
            if(pAnalyticsModulParam_1 != NULL) {
                pAnalyticsModulParam_1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                pAnalyticsModulParam_1->__any = soap_strdup(soap, ONVIF_FIELD_POLYGON_CONFIG_MACRO);
            }
            break;
        }

        case ONVIF_LOITERING_DETECT:
        {
            OnvifLoiteringDetection_S stInfo;
            onvif_get_loitering_detect_info(&stInfo);
            pAnalyticsModule->Name = soap_strdup(soap, LOITERING_DETECT_EVENT_MODULE);
            pAnalyticsModule->Type = soap_strdup(soap, LOITERING_DETECT_EVENT_MODULE_TYPE);
            pAnalyticsModule->Parameters = soap_new_tt__ItemList(soap,-1);
            pAnalyticsModule->Parameters->__sizeSimpleItem = 1;
            pAnalyticsModule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap,-1);
            pAnalyticsModule->Parameters->SimpleItem->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
            pAnalyticsModule->Parameters->SimpleItem->Value = soap_strdup(soap, stInfo.achSensitivity);
            
            pAnalyticsModule->Parameters->__sizeElementItem = 2;
            pAnalyticsModule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap,2);
            struct _tt__ItemList_ElementItem *pAnalyticsModulParam_0 = pAnalyticsModule->Parameters->ElementItem + 0;
            struct _tt__ItemList_ElementItem *pAnalyticsModulParam_1 = pAnalyticsModule->Parameters->ElementItem + 1;
            if(pAnalyticsModulParam_0 != NULL) {
                pAnalyticsModulParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
                pAnalyticsModulParam_0->__any = soap_strdup(soap, ONVIF_FIELD_TRANSFORMATION_MACRO);
            }
            if(pAnalyticsModulParam_1 != NULL) {
                pAnalyticsModulParam_1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                pAnalyticsModulParam_1->__any = soap_strdup(soap, ONVIF_FIELD_POLYGON_CONFIG_MACRO);
            }
            break;
        }

        case ONVIF_CROWD_GATHERING:
        {
            OnvifCrowdGathering_S stInfo;
            onvif_get_crowd_gathering_info(&stInfo);
            pAnalyticsModule->Name = soap_strdup(soap, CROWD_GATHERING_EVENT_MODULE);
            pAnalyticsModule->Type = soap_strdup(soap, CROWD_GATHERING_EVENT_MODULE_TYPE);
            pAnalyticsModule->Parameters = soap_new_tt__ItemList(soap,-1);
            pAnalyticsModule->Parameters->__sizeSimpleItem = 1;
            pAnalyticsModule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap,-1);
            pAnalyticsModule->Parameters->SimpleItem->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
            pAnalyticsModule->Parameters->SimpleItem->Value = soap_strdup(soap, stInfo.achSensitivity);
            
            pAnalyticsModule->Parameters->__sizeElementItem = 2;
            pAnalyticsModule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap,2);
            struct _tt__ItemList_ElementItem *pAnalyticsModulParam_0 = pAnalyticsModule->Parameters->ElementItem + 0;
            struct _tt__ItemList_ElementItem *pAnalyticsModulParam_1 = pAnalyticsModule->Parameters->ElementItem + 1;
            if(pAnalyticsModulParam_0 != NULL) {
                pAnalyticsModulParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
                pAnalyticsModulParam_0->__any = soap_strdup(soap, ONVIF_FIELD_TRANSFORMATION_MACRO);
            }
            if(pAnalyticsModulParam_1 != NULL) {
                pAnalyticsModulParam_1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                pAnalyticsModulParam_1->__any = soap_strdup(soap, ONVIF_FIELD_POLYGON_CONFIG_MACRO);
            }
            break;
        }

        case ONVIF_PARKING_DETECT:
        {
            OnvifParkingDetection_S stInfo;
            onvif_get_parking_detect_info(&stInfo);
            pAnalyticsModule->Name = soap_strdup(soap, PARKING_DETECT_EVENT_MODULE);
            pAnalyticsModule->Type = soap_strdup(soap, PARKING_DETECT_EVENT_MODULE_TYPE);
            pAnalyticsModule->Parameters = soap_new_tt__ItemList(soap,-1);
            pAnalyticsModule->Parameters->__sizeSimpleItem = 1;
            pAnalyticsModule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap,-1);
            pAnalyticsModule->Parameters->SimpleItem->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
            pAnalyticsModule->Parameters->SimpleItem->Value = soap_strdup(soap, stInfo.achSensitivity);
            
            pAnalyticsModule->Parameters->__sizeElementItem = 2;
            pAnalyticsModule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap,2);
            struct _tt__ItemList_ElementItem *pAnalyticsModulParam_0 = pAnalyticsModule->Parameters->ElementItem + 0;
            struct _tt__ItemList_ElementItem *pAnalyticsModulParam_1 = pAnalyticsModule->Parameters->ElementItem + 1;
            if(pAnalyticsModulParam_0 != NULL) {
                pAnalyticsModulParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
                pAnalyticsModulParam_0->__any = soap_strdup(soap, ONVIF_FIELD_TRANSFORMATION_MACRO);
            }
            if(pAnalyticsModulParam_1 != NULL) {
                pAnalyticsModulParam_1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                pAnalyticsModulParam_1->__any = soap_strdup(soap, ONVIF_FIELD_POLYGON_CONFIG_MACRO);
            }
            break;
        }

        case ONVIF_UNATTENDED_OBJECT:
        {
            OnvifUnattendedObject_S stInfo;
            onvif_get_unattended_object_info(&stInfo);
            pAnalyticsModule->Name = soap_strdup(soap, UNATTENDED_OBJECT_EVENT_MODULE);
            pAnalyticsModule->Type = soap_strdup(soap, UNATTENDED_OBJECT_EVENT_MODULE_TYPE);
            pAnalyticsModule->Parameters = soap_new_tt__ItemList(soap,-1);
            pAnalyticsModule->Parameters->__sizeSimpleItem = 1;
            pAnalyticsModule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap,-1);
            pAnalyticsModule->Parameters->SimpleItem->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
            pAnalyticsModule->Parameters->SimpleItem->Value = soap_strdup(soap, stInfo.achSensitivity);
            
            pAnalyticsModule->Parameters->__sizeElementItem = 2;
            pAnalyticsModule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap,2);
            struct _tt__ItemList_ElementItem *pAnalyticsModulParam_0 = pAnalyticsModule->Parameters->ElementItem + 0;
            struct _tt__ItemList_ElementItem *pAnalyticsModulParam_1 = pAnalyticsModule->Parameters->ElementItem + 1;
            if(pAnalyticsModulParam_0 != NULL) {
                pAnalyticsModulParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
                pAnalyticsModulParam_0->__any = soap_strdup(soap, ONVIF_FIELD_TRANSFORMATION_MACRO);
            }
            if(pAnalyticsModulParam_1 != NULL) {
                pAnalyticsModulParam_1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                pAnalyticsModulParam_1->__any = soap_strdup(soap, ONVIF_FIELD_POLYGON_CONFIG_MACRO);
            }
            break;
        }

        case ONVIF_OBJECT_REMOVAL:
        {
            OnvifObjectRemoval_S stInfo;
            onvif_get_object_removal_info(&stInfo);
            pAnalyticsModule->Name = soap_strdup(soap, OBJECT_REMOVAL_EVENT_MODULE);
            pAnalyticsModule->Type = soap_strdup(soap, OBJECT_REMOVAL_EVENT_MODULE_TYPE);
            pAnalyticsModule->Parameters = soap_new_tt__ItemList(soap,-1);
            pAnalyticsModule->Parameters->__sizeSimpleItem = 1;
            pAnalyticsModule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap,-1);
            pAnalyticsModule->Parameters->SimpleItem->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
            pAnalyticsModule->Parameters->SimpleItem->Value = soap_strdup(soap, stInfo.achSensitivity);
            
            pAnalyticsModule->Parameters->__sizeElementItem = 2;
            pAnalyticsModule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap,2);
            struct _tt__ItemList_ElementItem *pAnalyticsModulParam_0 = pAnalyticsModule->Parameters->ElementItem + 0;
            struct _tt__ItemList_ElementItem *pAnalyticsModulParam_1 = pAnalyticsModule->Parameters->ElementItem + 1;
            if(pAnalyticsModulParam_0 != NULL) {
                pAnalyticsModulParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
                pAnalyticsModulParam_0->__any = soap_strdup(soap, ONVIF_FIELD_TRANSFORMATION_MACRO);
            }
            if(pAnalyticsModulParam_1 != NULL) {
                pAnalyticsModulParam_1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                pAnalyticsModulParam_1->__any = soap_strdup(soap, ONVIF_FIELD_POLYGON_CONFIG_MACRO);
            }
            break;
        }

        case ONVIF_PET_RECOGNITION:
        {
            OnvifPetRecognition_S stInfo;
            onvif_get_pet_recognition_info(&stInfo);
            pAnalyticsModule->Name = soap_strdup(soap, PET_RECOGNITION_EVENT_MODULE);
            pAnalyticsModule->Type = soap_strdup(soap, PET_RECOGNITION_EVENT_MODULE_TYPE);
            pAnalyticsModule->Parameters = soap_new_tt__ItemList(soap,-1);
            pAnalyticsModule->Parameters->__sizeSimpleItem = 1;
            pAnalyticsModule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap,-1);
            pAnalyticsModule->Parameters->SimpleItem->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
            pAnalyticsModule->Parameters->SimpleItem->Value = soap_strdup(soap, stInfo.achSensitivity);
            
            pAnalyticsModule->Parameters->__sizeElementItem = 2;
            pAnalyticsModule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap,2);
            struct _tt__ItemList_ElementItem *pAnalyticsModulParam_0 = pAnalyticsModule->Parameters->ElementItem + 0;
            struct _tt__ItemList_ElementItem *pAnalyticsModulParam_1 = pAnalyticsModule->Parameters->ElementItem + 1;
            if(pAnalyticsModulParam_0 != NULL) {
                pAnalyticsModulParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
                pAnalyticsModulParam_0->__any = soap_strdup(soap, ONVIF_FIELD_TRANSFORMATION_MACRO);
            }
            if(pAnalyticsModulParam_1 != NULL) {
                pAnalyticsModulParam_1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
                pAnalyticsModulParam_1->__any = soap_strdup(soap, ONVIF_FIELD_POLYGON_CONFIG_MACRO);
            }
            break;
        }

        case ONVIF_FACE_CAPTURE:
        {
            OnvifFaceCapture_S stInfo;
            onvif_get_face_capture_info(&stInfo);
            pAnalyticsModule->Name = soap_strdup(soap, FACE_CAPTURE_EVENT_MODULE);
            pAnalyticsModule->Type = soap_strdup(soap, FACE_CAPTURE_EVENT_MODULE_TYPE);
            pAnalyticsModule->Parameters = soap_new_tt__ItemList(soap,-1);
            pAnalyticsModule->Parameters->__sizeSimpleItem = 1;
            pAnalyticsModule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap,-1);
            pAnalyticsModule->Parameters->SimpleItem->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
            pAnalyticsModule->Parameters->SimpleItem->Value = soap_strdup(soap, stInfo.achSensitivity);
            
            pAnalyticsModule->Parameters->__sizeElementItem = 2;
            pAnalyticsModule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap,2);
            struct _tt__ItemList_ElementItem *pAnalyticsModulParam_0 = pAnalyticsModule->Parameters->ElementItem + 0;
            struct _tt__ItemList_ElementItem *pAnalyticsModulParam_1 = pAnalyticsModule->Parameters->ElementItem + 1;
            if(pAnalyticsModulParam_0 != NULL) {
                pAnalyticsModulParam_0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
                pAnalyticsModulParam_0->__any = soap_strdup(soap, ONVIF_FIELD_TRANSFORMATION_MACRO);
            }
            if(pAnalyticsModulParam_1 != NULL) {
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

    /* Refactored Logic: Dynamic Count & Multi-Region Support */
    /* Refactored Logic: Dynamic Count & Multi-Region Support */
    int nTotalModules = 0;
    nTotalModules += 1; /* Motion */
    nTotalModules += 1; /* Tamper */
    nTotalModules += (onvif_get_intrusion_count() > 0 ? 1 : 0);
    nTotalModules += (onvif_get_tripwire_count() > 0 ? 1 : 0);
    nTotalModules += (onvif_get_enter_region_count() > 0 ? 1 : 0);
    nTotalModules += (onvif_get_leave_region_count() > 0 ? 1 : 0);
    nTotalModules += onvif_get_audio_anomaly_count();
    nTotalModules += onvif_get_scene_change_count();
    nTotalModules += onvif_get_face_detect_count();
    nTotalModules += (onvif_get_loitering_detect_count() > 0 ? 1 : 0);
    nTotalModules += (onvif_get_crowd_gathering_count() > 0 ? 1 : 0);
    nTotalModules += (onvif_get_parking_detect_count() > 0 ? 1 : 0);
    nTotalModules += (onvif_get_unattended_object_count() > 0 ? 1 : 0);
    nTotalModules += (onvif_get_object_removal_count() > 0 ? 1 : 0);
    nTotalModules += onvif_get_pet_recognition_count();
    nTotalModules += (onvif_get_face_capture_count() > 0 ? 1 : 0);

    tan__GetAnalyticsModulesResponse->__sizeAnalyticsModule = nTotalModules;
    tan__GetAnalyticsModulesResponse->AnalyticsModule = soap_new_tt__Config(soap, nTotalModules);

    int currIdx = 0;
    char szName[64];

    /* 1. Motion Detection */
    if (currIdx < nTotalModules) {
        struct tt__Config *pAnalyticsModule = tan__GetAnalyticsModulesResponse->AnalyticsModule + currIdx;
        OnvifMotionDetection_S stInfo;
        onvif_get_motion_info(&stInfo); 
        
        pAnalyticsModule->Name = soap_strdup(soap, MOTION_EVENT_MODULE);
        pAnalyticsModule->Type = soap_strdup(soap, MOTION_EVENT_MODULE_TYPE);
        pAnalyticsModule->Parameters = soap_new_tt__ItemList(soap,-1);
        pAnalyticsModule->Parameters->__sizeSimpleItem = 1;
        pAnalyticsModule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap,-1);
        pAnalyticsModule->Parameters->SimpleItem->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
        pAnalyticsModule->Parameters->SimpleItem->Value = soap_strdup(soap, stInfo.achSensitivity);
        pAnalyticsModule->Parameters->__sizeElementItem = 1;
        pAnalyticsModule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap,-1);
        pAnalyticsModule->Parameters->ElementItem->Name = soap_strdup(soap, ONVIF_ANALYTICS_LAYOUT_NAME);
        pAnalyticsModule->Parameters->ElementItem->__any = soap_strdup(soap, ONVIF_CELL_MOTION_LAYOUT_MACRO);
        currIdx++;
    }

    /* 2. Tamper Detection */
    if (currIdx < nTotalModules) {
        struct tt__Config *pAnalyticsModule = tan__GetAnalyticsModulesResponse->AnalyticsModule + currIdx;
        ONvifTamperDetection_S stInfo;
        onvif_get_tamp_info(&stInfo);

        pAnalyticsModule->Name = soap_strdup(soap, TAMPEREVENT_MODULE);
        pAnalyticsModule->Type = soap_strdup(soap, TAMPEREVENT_MODULE_TYPE);
        pAnalyticsModule->Parameters = soap_new_tt__ItemList(soap,-1);
        pAnalyticsModule->Parameters->__sizeSimpleItem = 1;
        pAnalyticsModule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap,-1);
        pAnalyticsModule->Parameters->SimpleItem->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
        pAnalyticsModule->Parameters->SimpleItem->Value = soap_strdup(soap, stInfo.achSensitivity);
        
        pAnalyticsModule->Parameters->__sizeElementItem = 2;
        pAnalyticsModule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap,2);
        
        struct _tt__ItemList_ElementItem *pItem0 = pAnalyticsModule->Parameters->ElementItem + 0;
        pItem0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
        pItem0->__any = soap_strdup(soap, ONVIF_TAMPER_TRANSFORMATION_MACRO);

        struct _tt__ItemList_ElementItem *pItem1 = pAnalyticsModule->Parameters->ElementItem + 1;
        pItem1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
        pItem1->__any = soap_strdup(soap, ONVIF_TAMPER_POLYGON_CONFIG_MACRO);
        currIdx++;
    }

    /* 3. Field Detection (Intrusion) */
    int nIntrusionCount = onvif_get_intrusion_count();
    if (nIntrusionCount > 0 && currIdx < nTotalModules) {
        struct tt__Config *pAnalyticsModule = tan__GetAnalyticsModulesResponse->AnalyticsModule + currIdx;
        OnvifFieldDetection_S stInfo;
        onvif_get_intrusion_info(0, &stInfo);

        pAnalyticsModule->Name = soap_strdup(soap, FIELD_EVENT_MODULE);
        pAnalyticsModule->Type = soap_strdup(soap, FIELD_EVENT_MODULE_TYPE);
        pAnalyticsModule->Parameters = soap_new_tt__ItemList(soap,-1);
        pAnalyticsModule->Parameters->__sizeSimpleItem = 2;
        pAnalyticsModule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap, 2);
        struct _tt__ItemList_SimpleItem *pSimp0 = pAnalyticsModule->Parameters->SimpleItem + 0;
        pSimp0->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
        pSimp0->Value = soap_strdup(soap, stInfo.achSensitivity);
        
        struct _tt__ItemList_SimpleItem *pSimp1 = pAnalyticsModule->Parameters->SimpleItem + 1;
        pSimp1->Name = soap_strdup(soap, "Enable");
        pSimp1->Value = soap_strdup(soap, stInfo.bEnable ? "true" : "false");

        pAnalyticsModule->Parameters->__sizeElementItem = 2;
        pAnalyticsModule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap,2);
        struct _tt__ItemList_ElementItem *pItem0 = pAnalyticsModule->Parameters->ElementItem + 0;
        pItem0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
        pItem0->__any = soap_strdup(soap, ONVIF_FIELD_TRANSFORMATION_MACRO);
        struct _tt__ItemList_ElementItem *pItem1 = pAnalyticsModule->Parameters->ElementItem + 1;
        pItem1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
        pItem1->__any = soap_strdup(soap, ONVIF_FIELD_POLYGON_CONFIG_MACRO);
        currIdx++;
    }

    /* 4. Line Detection (Tripwire) */
    int nTripwireCount = onvif_get_tripwire_count();
    if (nTripwireCount > 0 && currIdx < nTotalModules) {
        struct tt__Config *pAnalyticsModule = tan__GetAnalyticsModulesResponse->AnalyticsModule + currIdx;
        OnvifTripwireDetection_S stInfo;
        onvif_get_tripwire_info(0, &stInfo);

        pAnalyticsModule->Name = soap_strdup(soap, LINE_EVENT_MODULE);
        pAnalyticsModule->Type = soap_strdup(soap, LINE_EVENT_MODULE_TYPE);
        pAnalyticsModule->Parameters = soap_new_tt__ItemList(soap,-1);
        pAnalyticsModule->Parameters->__sizeSimpleItem = 2;
        pAnalyticsModule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap, 2);
        struct _tt__ItemList_SimpleItem *pSimp0 = pAnalyticsModule->Parameters->SimpleItem + 0;
        pSimp0->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
        pSimp0->Value = soap_strdup(soap, stInfo.achSensitivity);
        
        struct _tt__ItemList_SimpleItem *pSimp1 = pAnalyticsModule->Parameters->SimpleItem + 1;
        pSimp1->Name = soap_strdup(soap, "Enable");
        pSimp1->Value = soap_strdup(soap, stInfo.bEnable ? "true" : "false");

        pAnalyticsModule->Parameters->__sizeElementItem = 2;
        pAnalyticsModule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap,2);
        struct _tt__ItemList_ElementItem *pItem0 = pAnalyticsModule->Parameters->ElementItem + 0;
        pItem0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
        pItem0->__any = soap_strdup(soap, ONVIF_FIELD_TRANSFORMATION_MACRO);
        struct _tt__ItemList_ElementItem *pItem1 = pAnalyticsModule->Parameters->ElementItem + 1;
        pItem1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
        pItem1->__any = soap_strdup(soap, ONVIF_FIELD_POLYGON_CONFIG_MACRO);
        currIdx++;
    }

    /* 5. Enter Region */
    int nEnterCount = onvif_get_enter_region_count();
    if (nEnterCount > 0 && currIdx < nTotalModules) {
        struct tt__Config *pAnalyticsModule = tan__GetAnalyticsModulesResponse->AnalyticsModule + currIdx;
        OnvifRegionDetection_S stInfo;
        onvif_get_enter_region_info(0, &stInfo);

        pAnalyticsModule->Name = soap_strdup(soap, ENTER_REGION_EVENT_MODULE);
        pAnalyticsModule->Type = soap_strdup(soap, ENTER_REGION_EVENT_MODULE_TYPE);
        pAnalyticsModule->Parameters = soap_new_tt__ItemList(soap,-1);
        pAnalyticsModule->Parameters->__sizeSimpleItem = 2;
        pAnalyticsModule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap, 2);
        struct _tt__ItemList_SimpleItem *pSimp0 = pAnalyticsModule->Parameters->SimpleItem + 0;
        pSimp0->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
        pSimp0->Value = soap_strdup(soap, stInfo.achSensitivity);
        
        struct _tt__ItemList_SimpleItem *pSimp1 = pAnalyticsModule->Parameters->SimpleItem + 1;
        pSimp1->Name = soap_strdup(soap, "Enable");
        pSimp1->Value = soap_strdup(soap, stInfo.bEnable ? "true" : "false");
        
        pAnalyticsModule->Parameters->__sizeElementItem = 2;
        pAnalyticsModule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap,2);
        struct _tt__ItemList_ElementItem *pItem0 = pAnalyticsModule->Parameters->ElementItem + 0;
        pItem0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
        pItem0->__any = soap_strdup(soap, ONVIF_FIELD_TRANSFORMATION_MACRO);
        struct _tt__ItemList_ElementItem *pItem1 = pAnalyticsModule->Parameters->ElementItem + 1;
        pItem1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
        pItem1->__any = soap_strdup(soap, ONVIF_FIELD_POLYGON_CONFIG_MACRO);
        currIdx++;
    }

    /* 6. Leave Region */
    int nLeaveCount = onvif_get_leave_region_count();
    if (nLeaveCount > 0 && currIdx < nTotalModules) {
        struct tt__Config *pAnalyticsModule = tan__GetAnalyticsModulesResponse->AnalyticsModule + currIdx;
        OnvifRegionDetection_S stInfo;
        onvif_get_leave_region_info(0, &stInfo);

        pAnalyticsModule->Name = soap_strdup(soap, LEAVE_REGION_EVENT_MODULE);
        pAnalyticsModule->Type = soap_strdup(soap, LEAVE_REGION_EVENT_MODULE_TYPE);
        pAnalyticsModule->Parameters = soap_new_tt__ItemList(soap,-1);
        pAnalyticsModule->Parameters->__sizeSimpleItem = 1;
        pAnalyticsModule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap,-1);
        pAnalyticsModule->Parameters->SimpleItem->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
        pAnalyticsModule->Parameters->SimpleItem->Value = soap_strdup(soap, stInfo.achSensitivity); 

        pAnalyticsModule->Parameters->__sizeElementItem = 2;
        pAnalyticsModule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap,2);
        struct _tt__ItemList_ElementItem *pItem0 = pAnalyticsModule->Parameters->ElementItem + 0;
        pItem0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
        pItem0->__any = soap_strdup(soap, ONVIF_FIELD_TRANSFORMATION_MACRO);
        struct _tt__ItemList_ElementItem *pItem1 = pAnalyticsModule->Parameters->ElementItem + 1;
        pItem1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
        pItem1->__any = soap_strdup(soap, ONVIF_FIELD_POLYGON_CONFIG_MACRO);
        currIdx++;
    }

    /* 7. Audio Anomaly */
    int nAudioCount = onvif_get_audio_anomaly_count();
    if (nAudioCount > 0 && currIdx < nTotalModules) {
        struct tt__Config *pAnalyticsModule = tan__GetAnalyticsModulesResponse->AnalyticsModule + currIdx;
        OnvifAudioAnomaly_S stInfo;
        onvif_get_audio_anomaly_info(0, &stInfo);

        pAnalyticsModule->Name = soap_strdup(soap, AUDIO_ANOMALY_EVENT_MODULE);
        pAnalyticsModule->Type = soap_strdup(soap, AUDIO_ANOMALY_EVENT_MODULE_TYPE);
        pAnalyticsModule->Parameters = soap_new_tt__ItemList(soap,-1);
        pAnalyticsModule->Parameters->__sizeSimpleItem = 2;
        pAnalyticsModule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap, 2);
        struct _tt__ItemList_SimpleItem *pSimp0 = pAnalyticsModule->Parameters->SimpleItem + 0;
        pSimp0->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
        pSimp0->Value = soap_strdup(soap, stInfo.achSensitivity);
        
        struct _tt__ItemList_SimpleItem *pSimp1 = pAnalyticsModule->Parameters->SimpleItem + 1;
        pSimp1->Name = soap_strdup(soap, "Enable");
        pSimp1->Value = soap_strdup(soap, stInfo.bEnable ? "true" : "false");
        
        pAnalyticsModule->Parameters->__sizeElementItem = 0;
        currIdx++;
    }

    /* 8. Scene Change */
    int nSceneCount = onvif_get_scene_change_count();
    if (nSceneCount > 0 && currIdx < nTotalModules) {
        struct tt__Config *pAnalyticsModule = tan__GetAnalyticsModulesResponse->AnalyticsModule + currIdx;
        OnvifSceneChange_S stInfo;
        onvif_get_scene_change_info(0, &stInfo);

        pAnalyticsModule->Name = soap_strdup(soap, SCENE_CHANGE_EVENT_MODULE);
        pAnalyticsModule->Type = soap_strdup(soap, SCENE_CHANGE_EVENT_MODULE_TYPE);
        pAnalyticsModule->Parameters = soap_new_tt__ItemList(soap,-1);
        pAnalyticsModule->Parameters->__sizeSimpleItem = 2;
        pAnalyticsModule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap, 2);
        struct _tt__ItemList_SimpleItem *pSimp0 = pAnalyticsModule->Parameters->SimpleItem + 0;
        pSimp0->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
        pSimp0->Value = soap_strdup(soap, stInfo.achSensitivity);
        
        struct _tt__ItemList_SimpleItem *pSimp1 = pAnalyticsModule->Parameters->SimpleItem + 1;
        pSimp1->Name = soap_strdup(soap, "Enable");
        pSimp1->Value = soap_strdup(soap, stInfo.bEnable ? "true" : "false");
        
        pAnalyticsModule->Parameters->__sizeElementItem = 0;
        currIdx++;
    }

    /* 9. Face Detect */
    int nFaceCount = onvif_get_face_detect_count();
    if (nFaceCount > 0 && currIdx < nTotalModules) {
        struct tt__Config *pAnalyticsModule = tan__GetAnalyticsModulesResponse->AnalyticsModule + currIdx;
        OnvifFaceDetection_S stInfo;
        onvif_get_face_detect_info(0, &stInfo);

        pAnalyticsModule->Name = soap_strdup(soap, FACE_DETECT_EVENT_MODULE);
        pAnalyticsModule->Type = soap_strdup(soap, FACE_DETECT_EVENT_MODULE_TYPE);
        pAnalyticsModule->Parameters = soap_new_tt__ItemList(soap,-1);
        pAnalyticsModule->Parameters->__sizeSimpleItem = 2;
        pAnalyticsModule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap, 2);
        struct _tt__ItemList_SimpleItem *pSimp0 = pAnalyticsModule->Parameters->SimpleItem + 0;
        pSimp0->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
        pSimp0->Value = soap_strdup(soap, stInfo.achSensitivity);
        
        struct _tt__ItemList_SimpleItem *pSimp1 = pAnalyticsModule->Parameters->SimpleItem + 1;
        pSimp1->Name = soap_strdup(soap, "Enable");
        pSimp1->Value = soap_strdup(soap, stInfo.bEnable ? "true" : "false");
        
        pAnalyticsModule->Parameters->__sizeElementItem = 2;
        pAnalyticsModule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap,2);
        struct _tt__ItemList_ElementItem *pItem0 = pAnalyticsModule->Parameters->ElementItem + 0;
        pItem0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
        pItem0->__any = soap_strdup(soap, ONVIF_FIELD_TRANSFORMATION_MACRO);
        struct _tt__ItemList_ElementItem *pItem1 = pAnalyticsModule->Parameters->ElementItem + 1;
        pItem1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
        pItem1->__any = soap_strdup(soap, ONVIF_FIELD_POLYGON_CONFIG_MACRO);
        currIdx++;
    }

    /* 10. Loitering Detect */
    int nLoiteringCount = onvif_get_loitering_detect_count();
    if (nLoiteringCount > 0 && currIdx < nTotalModules) {
        struct tt__Config *pAnalyticsModule = tan__GetAnalyticsModulesResponse->AnalyticsModule + currIdx;
        OnvifLoiteringDetection_S stInfo;
        onvif_get_loitering_detect_info(0, &stInfo);

        pAnalyticsModule->Name = soap_strdup(soap, LOITERING_DETECT_EVENT_MODULE);
        pAnalyticsModule->Type = soap_strdup(soap, LOITERING_DETECT_EVENT_MODULE_TYPE);
        pAnalyticsModule->Parameters = soap_new_tt__ItemList(soap,-1);
        pAnalyticsModule->Parameters->__sizeSimpleItem = 2;
        pAnalyticsModule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap, 2);
        struct _tt__ItemList_SimpleItem *pSimp0 = pAnalyticsModule->Parameters->SimpleItem + 0;
        pSimp0->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
        pSimp0->Value = soap_strdup(soap, stInfo.achSensitivity);
        
        struct _tt__ItemList_SimpleItem *pSimp1 = pAnalyticsModule->Parameters->SimpleItem + 1;
        pSimp1->Name = soap_strdup(soap, "Enable");
        pSimp1->Value = soap_strdup(soap, stInfo.bEnable ? "true" : "false");
        
        pAnalyticsModule->Parameters->__sizeElementItem = 2;
        pAnalyticsModule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap,2);
        struct _tt__ItemList_ElementItem *pItem0 = pAnalyticsModule->Parameters->ElementItem + 0;
        pItem0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
        pItem0->__any = soap_strdup(soap, ONVIF_FIELD_TRANSFORMATION_MACRO);
        struct _tt__ItemList_ElementItem *pItem1 = pAnalyticsModule->Parameters->ElementItem + 1;
        pItem1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
        pItem1->__any = soap_strdup(soap, ONVIF_FIELD_POLYGON_CONFIG_MACRO);
        currIdx++;
    }

    /* 11. Crowd Gathering */
    int nCrowdCount = onvif_get_crowd_gathering_count();
    if (nCrowdCount > 0 && currIdx < nTotalModules) {
        struct tt__Config *pAnalyticsModule = tan__GetAnalyticsModulesResponse->AnalyticsModule + currIdx;
        OnvifCrowdGathering_S stInfo;
        onvif_get_crowd_gathering_info(0, &stInfo);

        pAnalyticsModule->Name = soap_strdup(soap, CROWD_GATHERING_EVENT_MODULE);
        pAnalyticsModule->Type = soap_strdup(soap, CROWD_GATHERING_EVENT_MODULE_TYPE);
        pAnalyticsModule->Parameters = soap_new_tt__ItemList(soap,-1);
        pAnalyticsModule->Parameters->__sizeSimpleItem = 2;
        pAnalyticsModule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap, 2);
        struct _tt__ItemList_SimpleItem *pSimp0 = pAnalyticsModule->Parameters->SimpleItem + 0;
        pSimp0->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
        pSimp0->Value = soap_strdup(soap, stInfo.achSensitivity);
        
        struct _tt__ItemList_SimpleItem *pSimp1 = pAnalyticsModule->Parameters->SimpleItem + 1;
        pSimp1->Name = soap_strdup(soap, "Enable");
        pSimp1->Value = soap_strdup(soap, stInfo.bEnable ? "true" : "false");
        
        pAnalyticsModule->Parameters->__sizeElementItem = 2;
        pAnalyticsModule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap,2);
        struct _tt__ItemList_ElementItem *pItem0 = pAnalyticsModule->Parameters->ElementItem + 0;
        pItem0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
        pItem0->__any = soap_strdup(soap, ONVIF_FIELD_TRANSFORMATION_MACRO);
        struct _tt__ItemList_ElementItem *pItem1 = pAnalyticsModule->Parameters->ElementItem + 1;
        pItem1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
        pItem1->__any = soap_strdup(soap, ONVIF_FIELD_POLYGON_CONFIG_MACRO);
        currIdx++;
    }

    /* 12. Parking Detect */
    int nParkingCount = onvif_get_parking_detect_count();
    if (nParkingCount > 0 && currIdx < nTotalModules) {
        struct tt__Config *pAnalyticsModule = tan__GetAnalyticsModulesResponse->AnalyticsModule + currIdx;
        OnvifParkingDetection_S stInfo;
        onvif_get_parking_detect_info(0, &stInfo);

        pAnalyticsModule->Name = soap_strdup(soap, PARKING_DETECT_EVENT_MODULE);
        pAnalyticsModule->Type = soap_strdup(soap, PARKING_DETECT_EVENT_MODULE_TYPE);
        pAnalyticsModule->Parameters = soap_new_tt__ItemList(soap,-1);
        pAnalyticsModule->Parameters->__sizeSimpleItem = 2;
        pAnalyticsModule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap, 2);
        struct _tt__ItemList_SimpleItem *pSimp0 = pAnalyticsModule->Parameters->SimpleItem + 0;
        pSimp0->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
        pSimp0->Value = soap_strdup(soap, stInfo.achSensitivity);

        struct _tt__ItemList_SimpleItem *pSimp1 = pAnalyticsModule->Parameters->SimpleItem + 1;
        pSimp1->Name = soap_strdup(soap, "Enable");
        pSimp1->Value = soap_strdup(soap, stInfo.bEnable ? "true" : "false");
        
        pAnalyticsModule->Parameters->__sizeElementItem = 2;
        pAnalyticsModule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap,2);
        struct _tt__ItemList_ElementItem *pItem0 = pAnalyticsModule->Parameters->ElementItem + 0;
        pItem0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
        pItem0->__any = soap_strdup(soap, ONVIF_FIELD_TRANSFORMATION_MACRO);
        struct _tt__ItemList_ElementItem *pItem1 = pAnalyticsModule->Parameters->ElementItem + 1;
        pItem1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
        pItem1->__any = soap_strdup(soap, ONVIF_FIELD_POLYGON_CONFIG_MACRO);
        currIdx++;
    }

    /* 13. Unattended Object */
    int nUnattendedCount = onvif_get_unattended_object_count();
    if (nUnattendedCount > 0 && currIdx < nTotalModules) {
        struct tt__Config *pAnalyticsModule = tan__GetAnalyticsModulesResponse->AnalyticsModule + currIdx;
        OnvifUnattendedObject_S stInfo;
        onvif_get_unattended_object_info(0, &stInfo);

        pAnalyticsModule->Name = soap_strdup(soap, UNATTENDED_OBJECT_EVENT_MODULE);
        pAnalyticsModule->Type = soap_strdup(soap, UNATTENDED_OBJECT_EVENT_MODULE_TYPE);
        pAnalyticsModule->Parameters = soap_new_tt__ItemList(soap,-1);
        pAnalyticsModule->Parameters->__sizeSimpleItem = 2;
        pAnalyticsModule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap, 2);
        struct _tt__ItemList_SimpleItem *pSimp0 = pAnalyticsModule->Parameters->SimpleItem + 0;
        pSimp0->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
        pSimp0->Value = soap_strdup(soap, stInfo.achSensitivity);
        
        struct _tt__ItemList_SimpleItem *pSimp1 = pAnalyticsModule->Parameters->SimpleItem + 1;
        pSimp1->Name = soap_strdup(soap, "Enable");
        pSimp1->Value = soap_strdup(soap, stInfo.bEnable ? "true" : "false");
        
        pAnalyticsModule->Parameters->__sizeElementItem = 2;
        pAnalyticsModule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap,2);
        struct _tt__ItemList_ElementItem *pItem0 = pAnalyticsModule->Parameters->ElementItem + 0;
        pItem0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
        pItem0->__any = soap_strdup(soap, ONVIF_FIELD_TRANSFORMATION_MACRO);
        struct _tt__ItemList_ElementItem *pItem1 = pAnalyticsModule->Parameters->ElementItem + 1;
        pItem1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
        pItem1->__any = soap_strdup(soap, ONVIF_FIELD_POLYGON_CONFIG_MACRO);
        currIdx++;
    }

    /* 14. Object Removal */
    int nRemovalCount = onvif_get_object_removal_count();
    if (nRemovalCount > 0 && currIdx < nTotalModules) {
        struct tt__Config *pAnalyticsModule = tan__GetAnalyticsModulesResponse->AnalyticsModule + currIdx;
        OnvifObjectRemoval_S stInfo;
        onvif_get_object_removal_info(0, &stInfo);

        pAnalyticsModule->Name = soap_strdup(soap, OBJECT_REMOVAL_EVENT_MODULE);
        pAnalyticsModule->Type = soap_strdup(soap, OBJECT_REMOVAL_EVENT_MODULE_TYPE);
        pAnalyticsModule->Parameters = soap_new_tt__ItemList(soap,-1);
        pAnalyticsModule->Parameters->__sizeSimpleItem = 2;
        pAnalyticsModule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap, 2);
        struct _tt__ItemList_SimpleItem *pSimp0 = pAnalyticsModule->Parameters->SimpleItem + 0;
        pSimp0->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
        pSimp0->Value = soap_strdup(soap, stInfo.achSensitivity);
        
        struct _tt__ItemList_SimpleItem *pSimp1 = pAnalyticsModule->Parameters->SimpleItem + 1;
        pSimp1->Name = soap_strdup(soap, "Enable");
        pSimp1->Value = soap_strdup(soap, stInfo.bEnable ? "true" : "false");
        
        pAnalyticsModule->Parameters->__sizeElementItem = 2;
        pAnalyticsModule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap,2);
        struct _tt__ItemList_ElementItem *pItem0 = pAnalyticsModule->Parameters->ElementItem + 0;
        pItem0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
        pItem0->__any = soap_strdup(soap, ONVIF_FIELD_TRANSFORMATION_MACRO);
        struct _tt__ItemList_ElementItem *pItem1 = pAnalyticsModule->Parameters->ElementItem + 1;
        pItem1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
        pItem1->__any = soap_strdup(soap, ONVIF_FIELD_POLYGON_CONFIG_MACRO);
        currIdx++;
    }

    /* 15. Pet Recognition */
    int nPetCount = onvif_get_pet_recognition_count();
    if (nPetCount > 0 && currIdx < nTotalModules) {
        struct tt__Config *pAnalyticsModule = tan__GetAnalyticsModulesResponse->AnalyticsModule + currIdx;
        OnvifPetRecognition_S stInfo;
        onvif_get_pet_recognition_info(0, &stInfo);

        pAnalyticsModule->Name = soap_strdup(soap, PET_RECOGNITION_EVENT_MODULE);
        pAnalyticsModule->Type = soap_strdup(soap, PET_RECOGNITION_EVENT_MODULE_TYPE);
        pAnalyticsModule->Parameters = soap_new_tt__ItemList(soap,-1);
        pAnalyticsModule->Parameters->__sizeSimpleItem = 2;
        pAnalyticsModule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap, 2);
        struct _tt__ItemList_SimpleItem *pSimp0 = pAnalyticsModule->Parameters->SimpleItem + 0;
        pSimp0->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
        pSimp0->Value = soap_strdup(soap, stInfo.achSensitivity);
        
        struct _tt__ItemList_SimpleItem *pSimp1 = pAnalyticsModule->Parameters->SimpleItem + 1;
        pSimp1->Name = soap_strdup(soap, "Enable");
        pSimp1->Value = soap_strdup(soap, stInfo.bEnable ? "true" : "false");
        
        pAnalyticsModule->Parameters->__sizeElementItem = 2;
        pAnalyticsModule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap,2);
        struct _tt__ItemList_ElementItem *pItem0 = pAnalyticsModule->Parameters->ElementItem + 0;
        pItem0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
        pItem0->__any = soap_strdup(soap, ONVIF_FIELD_TRANSFORMATION_MACRO);
        struct _tt__ItemList_ElementItem *pItem1 = pAnalyticsModule->Parameters->ElementItem + 1;
        pItem1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
        pItem1->__any = soap_strdup(soap, ONVIF_FIELD_POLYGON_CONFIG_MACRO);
        currIdx++;
    }
    
    /* 16. Face Capture (Note: previously missing in module list or check loop) */
    int nFaceCapCount = onvif_get_face_capture_count();
    if (nFaceCapCount > 0 && currIdx < nTotalModules) {
        struct tt__Config *pAnalyticsModule = tan__GetAnalyticsModulesResponse->AnalyticsModule + currIdx;
        OnvifFaceCapture_S stInfo;
        onvif_get_face_capture_info(0, &stInfo);

        pAnalyticsModule->Name = soap_strdup(soap, FACE_CAPTURE_EVENT_MODULE);
        pAnalyticsModule->Type = soap_strdup(soap, FACE_CAPTURE_EVENT_MODULE_TYPE);
        pAnalyticsModule->Parameters = soap_new_tt__ItemList(soap,-1);
        pAnalyticsModule->Parameters->__sizeSimpleItem = 2;
        pAnalyticsModule->Parameters->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap, 2);
        struct _tt__ItemList_SimpleItem *pSimp0 = pAnalyticsModule->Parameters->SimpleItem + 0;
        pSimp0->Name = soap_strdup(soap, ONVIF_ANALYTICS_SENS_NAME);
        pSimp0->Value = soap_strdup(soap, stInfo.achSensitivity);
        
        struct _tt__ItemList_SimpleItem *pSimp1 = pAnalyticsModule->Parameters->SimpleItem + 1;
        pSimp1->Name = soap_strdup(soap, "Enable");
        pSimp1->Value = soap_strdup(soap, stInfo.bEnable ? "true" : "false");
        
        pAnalyticsModule->Parameters->__sizeElementItem = 2;
        pAnalyticsModule->Parameters->ElementItem = soap_new__tt__ItemList_ElementItem(soap,2);
        struct _tt__ItemList_ElementItem *pItem0 = pAnalyticsModule->Parameters->ElementItem + 0;
        pItem0->Name = soap_strdup(soap, ONVIF_ANALYTICS_TRANS_NAME);
        pItem0->__any = soap_strdup(soap, ONVIF_FIELD_TRANSFORMATION_MACRO);
        struct _tt__ItemList_ElementItem *pItem1 = pAnalyticsModule->Parameters->ElementItem + 1;
        pItem1->Name = soap_strdup(soap, ONVIF_ANALYTICS_FIELD_NAME);
        pItem1->__any = soap_strdup(soap, ONVIF_FIELD_POLYGON_CONFIG_MACRO);
        currIdx++;
    }



    return SOAP_OK;
}

/** Web service operation '__tan__GetAnalyticsModuleOptions' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tan__GetAnalyticsModuleOptions(struct soap* soap, struct _tan__GetAnalyticsModuleOptions *tan__GetAnalyticsModuleOptions, struct _tan__GetAnalyticsModuleOptionsResponse *tan__GetAnalyticsModuleOptionsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tan__GetAnalyticsModuleOptions----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tan__ModifyAnalyticsModules' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tan__ModifyAnalyticsModules(struct soap* soap, struct _tan__ModifyAnalyticsModules *tan__ModifyAnalyticsModules, struct _tan__ModifyAnalyticsModulesResponse *tan__ModifyAnalyticsModulesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tan__ModifyAnalyticsModules----------");
#endif
    int nRet = onvif_authentication(soap);
    if(SOAP_OK != nRet)
    {
        return nRet;
    }
    /* token校验 */
    if(tan__ModifyAnalyticsModules != NULL && tan__ModifyAnalyticsModules->ConfigurationToken)
    {
        if(strcmp(tan__ModifyAnalyticsModules->ConfigurationToken,VIDEOANALTICS_TOKEN) != 0)
        {
            dlog_error("requested  VIDEOANALTICS_TOKEN does not exist");
            return soap_receiver_fault(soap, "requested VIDEOANALTICS_TOKEN does not exist", NULL); 
        }
    }
    else
    {
        return soap_receiver_fault(soap, "requested VIDEOANALTICS_TOKEN does not exist", NULL); 
    }
    /* 是否设置成功标志 */
    nRet = SOAP_FAULT;
    for(int i  = 0; i < tan__ModifyAnalyticsModules->__sizeAnalyticsModule;i++)
    {
        struct tt__Config *pAnalyticsModule = tan__ModifyAnalyticsModules->AnalyticsModule + i;
        if(pAnalyticsModule == NULL || pAnalyticsModule->Name == NULL || pAnalyticsModule->Type == NULL)
        {
            continue;
        }

        /* 移动侦测设置 */
        if(!strcmp(pAnalyticsModule->Name,MOTION_EVENT_MODULE) && !strcmp(pAnalyticsModule->Type,MOTION_EVENT_MODULE_TYPE))
        {
            OnvifMotionDetection_S stInfo;
            onvif_get_motion_info(&stInfo);
            
            for(int j = 0;j < pAnalyticsModule->Parameters->__sizeSimpleItem;j++)
            {
                struct _tt__ItemList_SimpleItem *pParam = pAnalyticsModule->Parameters->SimpleItem + j;
                if(pParam == NULL || pParam->Name == NULL || pParam->Value == NULL)
                {
                    continue;
                }
                /* 移动侦测灵敏度 */
                if(!strcmp(pParam->Name,ONVIF_ANALYTICS_SENS_NAME))
                {
                    stInfo.achSensitivity[0] = '\0';
                    size_t srcLen = strlen(pParam->Value);
                    size_t maxLen = sizeof(stInfo.achSensitivity);
                    // 拷贝
                    strncpy(stInfo.achSensitivity, pParam->Value, maxLen - 1);
                    stInfo.achSensitivity[maxLen - 1] = '\0';
                    
                    if(onvif_set_motion_analytics(&stInfo) != 0)
                    {
                        nRet = SOAP_FAULT;
                    }
                    else
                    {
                        nRet = SOAP_OK;
                    }
                }
                else if(!strcmp(pParam->Name, "Enable"))
                {
                    stInfo.bEnable = !strcmp(pParam->Value, "true");
                }
            }
        }
        /* 遮挡报警分析模块设置 */
        else if(!strcmp(pAnalyticsModule->Name,TAMPEREVENT_MODULE) && !strcmp(pAnalyticsModule->Type,TAMPEREVENT_MODULE_TYPE))
        {
            ONvifTamperDetection_S stInfo;
            onvif_get_tamp_info(&stInfo);
            for(int j = 0;j < pAnalyticsModule->Parameters->__sizeSimpleItem;j++)
            {
                struct _tt__ItemList_SimpleItem *pParam = pAnalyticsModule->Parameters->SimpleItem + j;
                if(pParam == NULL || pParam->Name == NULL || pParam->Value == NULL)
                {
                    continue;
                }
                /* 遮挡报警灵敏度 */
                if(!strcmp(pParam->Name,ONVIF_ANALYTICS_SENS_NAME))
                {
                    stInfo.achSensitivity[0] = '\0';
                    size_t srcLen = strlen(pParam->Value);
                    size_t maxLen = sizeof(stInfo.achSensitivity);
                    // 拷贝
                    strncpy(stInfo.achSensitivity, pParam->Value, maxLen - 1);
                    stInfo.achSensitivity[maxLen - 1] = '\0';
                    if(onvif_set_tamp_analytics(&stInfo) != 0)
                    {
                        nRet = SOAP_FAULT;
                    }
                    else
                    {
                        nRet = SOAP_OK;
                    }
                }
                else if(!strcmp(pParam->Name, "Enable"))
                {
                    stInfo.bEnable = !strcmp(pParam->Value, "true");
                }
            }
        }
        else if(!strncmp(pAnalyticsModule->Name,FIELD_EVENT_MODULE, strlen(FIELD_EVENT_MODULE)) && !strcmp(pAnalyticsModule->Type,FIELD_EVENT_MODULE_TYPE))
        {
             OnvifFieldDetection_S stInfoTemplate;
             memset(&stInfoTemplate, 0, sizeof(stInfoTemplate));
             
             for(int j = 0; j < pAnalyticsModule->Parameters->__sizeSimpleItem; j++) {
                struct _tt__ItemList_SimpleItem *pParam = pAnalyticsModule->Parameters->SimpleItem + j;
                if(pParam && pParam->Name && pParam->Value) {
 if(!strcmp(pParam->Name, "Enable")) {
                        stInfoTemplate.bEnable = !strcmp(pParam->Value, "true");
                    }
                    else if(!strcmp(pParam->Name, ONVIF_ANALYTICS_SENS_NAME)) {
                        strncpy(stInfoTemplate.achSensitivity, pParam->Value, sizeof(stInfoTemplate.achSensitivity)-1);
                    }
                }
             }
             
             int nSetCount = 0;
             for(int j = 0; j < pAnalyticsModule->Parameters->__sizeElementItem; j++) {
                struct _tt__ItemList_ElementItem *pParam = pAnalyticsModule->Parameters->ElementItem + j;
                if(pParam && pParam->Name && pParam->__any) {
                    if(!strcmp(pParam->Name, "Field") || !strcmp(pParam->Name, ONVIF_ANALYTICS_FIELD_NAME)) {
                         OnvifFieldDetection_S stInfo = stInfoTemplate;
                         char* pStr = pParam->__any;
                         
                        /* Parse TimeThreshold */
                        char* pThreshold = strstr(pStr, "<tt:TimeThreshold>");
                        if(pThreshold) {
                            pThreshold += strlen("<tt:TimeThreshold>");
                            char* pEnd = strstr(pThreshold, "</tt:TimeThreshold>");
                            if(pEnd) {
                                size_t len = pEnd - pThreshold;
                                if(len < sizeof(stInfo.achTimeThreshold)) {
                                    strncpy(stInfo.achTimeThreshold, pThreshold, len);
                                    stInfo.achTimeThreshold[len] = '\0';
                                }
                            }
                        }

                         int nCount = 0;
                         while(nCount < ONVIF_ANALYTICS_POLYGON_POINT_NUM) {
                             pStr = strstr(pStr, "<tt:Point");
                             if(!pStr) break;
                             pStr += strlen("<tt:Point");
                             const char *x_start = strstr(pStr, "x=\"");
                             if(!x_start) break;
                             x_start += 3;
                             int x = (int)strtol(x_start, (char**)&pStr, 10);
                             const char *y_start = strstr(pStr, "y=\"");
                             if(!y_start) break;
                             y_start += 3;
                             int y = (int)strtol(y_start, (char**)&pStr, 10);
                             stInfo.stPolygon[nCount++] = (OnvifPoint_S){x, y};
                             pStr = strchr(pStr, '/');
                             if(!pStr) break;
                             pStr += 2;
                         }
                         stInfo.nPointNum = nCount;
                         int ret = onvif_set_intrusion_info(nSetCount, &stInfo);
                         if(ret == ONVIF_ERR_EVENT_RESOURCE_CONFLICT) return soap_sender_fault(soap, "Event Resource Conflict", "Resource allocation failed for Intrusion Detection.");
                         if(ret == 0) nSetCount++;
                    }
                }
             }

             /* Cleanup remaining */
             OnvifFieldDetection_S stEmpty;
             memset(&stEmpty, 0, sizeof(stEmpty));
             stEmpty.bEnable = false;
             while(nSetCount < 16) {
                  if(onvif_set_intrusion_info(nSetCount, &stEmpty) != 0) break;
                  nSetCount++;
             }
             nRet = SOAP_OK;
        }
        else if(!strncmp(pAnalyticsModule->Name,LINE_EVENT_MODULE, strlen(LINE_EVENT_MODULE)) && !strcmp(pAnalyticsModule->Type,LINE_EVENT_MODULE_TYPE))
        {
             OnvifTripwireDetection_S stInfoTemplate;
             memset(&stInfoTemplate, 0, sizeof(stInfoTemplate));
             
             for(int j = 0; j < pAnalyticsModule->Parameters->__sizeSimpleItem; j++) {
                struct _tt__ItemList_SimpleItem *pParam = pAnalyticsModule->Parameters->SimpleItem + j;
                if(pParam && pParam->Name && pParam->Value) {
                    if(!strcmp(pParam->Name, "Direction")) {
                        strncpy(stInfoTemplate.achDirection, pParam->Value, sizeof(stInfoTemplate.achDirection)-1);
                    }
                    else if(!strcmp(pParam->Name, "Enable")) {
                        stInfoTemplate.bEnable = !strcmp(pParam->Value, "true");
                    }
                    else if(!strcmp(pParam->Name, ONVIF_ANALYTICS_SENS_NAME)) {
                        strncpy(stInfoTemplate.achSensitivity, pParam->Value, sizeof(stInfoTemplate.achSensitivity)-1);
                    }
                }
             }
             
             int nSetCount = 0;
             for(int j = 0; j < pAnalyticsModule->Parameters->__sizeElementItem; j++) {
                struct _tt__ItemList_ElementItem *pParam = pAnalyticsModule->Parameters->ElementItem + j;
                if(pParam && pParam->Name && pParam->__any) {
                    if(!strcmp(pParam->Name, "Segments")) {
                         OnvifTripwireDetection_S stInfo = stInfoTemplate;
                         char* pStr = pParam->__any;
                         int nCount = 0;
                         while(nCount < 2) {
                             pStr = strstr(pStr, "<tt:Point");
                             if(!pStr) break;
                             pStr += strlen("<tt:Point");
                             const char *x_start = strstr(pStr, "x=\"");
                             if(!x_start) break;
                             x_start += 3;
                             int x = (int)strtol(x_start, (char**)&pStr, 10);
                             const char *y_start = strstr(pStr, "y=\"");
                             if(!y_start) break;
                             y_start += 3;
                             int y = (int)strtol(y_start, (char**)&pStr, 10);
                             stInfo.stPolygon[nCount++] = (OnvifPoint_S){x, y};
                             pStr = strchr(pStr, '/');
                             if(!pStr) break;
                             pStr += 2;
                         }
                         int ret = onvif_set_tripwire_info(nSetCount, &stInfo);
                         if(ret == ONVIF_ERR_EVENT_RESOURCE_CONFLICT) return soap_sender_fault(soap, "Event Resource Conflict", "Resource allocation failed for Tripwire.");
                         if(ret == 0) nSetCount++;
                    }
                }
             }

             /* Cleanup remaining */
             OnvifTripwireDetection_S stEmpty;
             memset(&stEmpty, 0, sizeof(stEmpty));
             stEmpty.bEnable = false;
             while(nSetCount < 16) {
                  if(onvif_set_tripwire_info(nSetCount, &stEmpty) != 0) break;
                  nSetCount++;
             }
             nRet = SOAP_OK;
        }
        else if(!strncmp(pAnalyticsModule->Name,ENTER_REGION_EVENT_MODULE, strlen(ENTER_REGION_EVENT_MODULE)) && !strcmp(pAnalyticsModule->Type,ENTER_REGION_EVENT_MODULE_TYPE))
        {
             OnvifRegionDetection_S stInfoTemplate;
             memset(&stInfoTemplate, 0, sizeof(stInfoTemplate));
             
             for(int j = 0; j < pAnalyticsModule->Parameters->__sizeSimpleItem; j++) {
                struct _tt__ItemList_SimpleItem *pParam = pAnalyticsModule->Parameters->SimpleItem + j;
                if(pParam && pParam->Name && pParam->Value) {
                    if(!strcmp(pParam->Name, ONVIF_ANALYTICS_SENS_NAME)) {
                        strncpy(stInfoTemplate.achSensitivity, pParam->Value, sizeof(stInfoTemplate.achSensitivity)-1);
                    }
                    else if(!strcmp(pParam->Name, "Enable")) {
                        stInfoTemplate.bEnable = !strcmp(pParam->Value, "true");
                    }
                }
             }

             int nSetCount = 0;
             for(int j = 0; j < pAnalyticsModule->Parameters->__sizeElementItem; j++) {
                struct _tt__ItemList_ElementItem *pParam = pAnalyticsModule->Parameters->ElementItem + j;
                if(pParam && pParam->Name && pParam->__any) {
                    if(!strcmp(pParam->Name, ONVIF_ANALYTICS_FIELD_NAME)) {
                        OnvifRegionDetection_S stInfo = stInfoTemplate;
                        char* pStr = pParam->__any;
                        int nCount = 0;
                        while(nCount < 4) {
                            pStr = strstr(pStr, "<tt:Point");
                            if(!pStr) break;
                            pStr += strlen("<tt:Point");
                            const char *x_start = strstr(pStr, "x=\"");
                            if(!x_start) break;
                            x_start += 3;
                            int x = (int)strtol(x_start, (char**)&pStr, 10);
                            const char *y_start = strstr(pStr, "y=\"");
                            if(!y_start) break;
                            y_start += 3;
                            int y = (int)strtol(y_start, (char**)&pStr, 10);
                            stInfo.stPolygon[nCount++] = (OnvifPoint_S){x, y};
                            pStr = strchr(pStr, '/');
                            if(!pStr) break;
                            pStr += 2;
                        }
                        int ret = onvif_set_enter_region_info(nSetCount, &stInfo);
                        if(ret == ONVIF_ERR_EVENT_RESOURCE_CONFLICT) return soap_sender_fault(soap, "Event Resource Conflict", "Resource allocation failed for Entrance Detection.");
                        if(ret == 0) nSetCount++;
                    }
                }
             }
             
             /* Cleanup remaining */
             OnvifRegionDetection_S stEmpty;
             memset(&stEmpty, 0, sizeof(stEmpty));
             stEmpty.bEnable = false;
             while(nSetCount < 16) {
                  if(onvif_set_enter_region_info(nSetCount, &stEmpty) != 0) break;
                  nSetCount++;
             }
             nRet = SOAP_OK;
        }
        else if(!strncmp(pAnalyticsModule->Name,LEAVE_REGION_EVENT_MODULE, strlen(LEAVE_REGION_EVENT_MODULE)) && !strcmp(pAnalyticsModule->Type,LEAVE_REGION_EVENT_MODULE_TYPE))
        {
             OnvifRegionDetection_S stInfoTemplate;
             memset(&stInfoTemplate, 0, sizeof(stInfoTemplate));
             
             for(int j = 0; j < pAnalyticsModule->Parameters->__sizeSimpleItem; j++) {
                struct _tt__ItemList_SimpleItem *pParam = pAnalyticsModule->Parameters->SimpleItem + j;
                if(pParam && pParam->Name && pParam->Value) {
                    if(!strcmp(pParam->Name, ONVIF_ANALYTICS_SENS_NAME)) {
                        strncpy(stInfoTemplate.achSensitivity, pParam->Value, sizeof(stInfoTemplate.achSensitivity)-1);
                    }
                    else if(!strcmp(pParam->Name, "Enable")) {
                        stInfoTemplate.bEnable = !strcmp(pParam->Value, "true");
                    }
                }
             }

             int nSetCount = 0;
             for(int j = 0; j < pAnalyticsModule->Parameters->__sizeElementItem; j++) {
                struct _tt__ItemList_ElementItem *pParam = pAnalyticsModule->Parameters->ElementItem + j;
                if(pParam && pParam->Name && pParam->__any) {
                    if(!strcmp(pParam->Name, ONVIF_ANALYTICS_FIELD_NAME)) {
                        OnvifRegionDetection_S stInfo = stInfoTemplate;
                        char* pStr = pParam->__any;
                        int nCount = 0;
                        while(nCount < 4) {
                            pStr = strstr(pStr, "<tt:Point");
                            if(!pStr) break;
                            pStr += strlen("<tt:Point");
                            const char *x_start = strstr(pStr, "x=\"");
                            if(!x_start) break;
                            x_start += 3;
                            int x = (int)strtol(x_start, (char**)&pStr, 10);
                            const char *y_start = strstr(pStr, "y=\"");
                            if(!y_start) break;
                            y_start += 3;
                            int y = (int)strtol(y_start, (char**)&pStr, 10);
                            stInfo.stPolygon[nCount++] = (OnvifPoint_S){x, y};
                            pStr = strchr(pStr, '/');
                            if(!pStr) break;
                            pStr += 2;
                        }
                        int ret = onvif_set_leave_region_info(nSetCount, &stInfo);
                        if(ret == ONVIF_ERR_EVENT_RESOURCE_CONFLICT) return soap_sender_fault(soap, "Event Resource Conflict", "Resource allocation failed for Exiting Detection.");
                        if(ret == 0) nSetCount++;
                    }
                }
             }
             
             /* Cleanup remaining */
             OnvifRegionDetection_S stEmpty;
             memset(&stEmpty, 0, sizeof(stEmpty));
             stEmpty.bEnable = false;
             while(nSetCount < 16) {
                  if(onvif_set_leave_region_info(nSetCount, &stEmpty) != 0) break;
                  nSetCount++;
             }
             nRet = SOAP_OK;
        }
        else if(!strncmp(pAnalyticsModule->Name,AUDIO_ANOMALY_EVENT_MODULE, strlen(AUDIO_ANOMALY_EVENT_MODULE)) && !strcmp(pAnalyticsModule->Type,AUDIO_ANOMALY_EVENT_MODULE_TYPE))
        {
             OnvifAudioAnomaly_S stInfo;
             memset(&stInfo, 0, sizeof(stInfo));
             
             for(int j = 0;j < pAnalyticsModule->Parameters->__sizeSimpleItem;j++)
             {
                struct _tt__ItemList_SimpleItem *pParam = pAnalyticsModule->Parameters->SimpleItem + j;
                if(pParam && pParam->Name && pParam->Value)
                {
                    if(!strcmp(pParam->Name,ONVIF_ANALYTICS_SENS_NAME)) {
                        strncpy(stInfo.achSensitivity, pParam->Value, sizeof(stInfo.achSensitivity)-1);
                    }
                    else if(!strcmp(pParam->Name, "Enable")) {
                        stInfo.bEnable = !strcmp(pParam->Value, "true");
                    }
                }
             }
             int ret = onvif_set_audio_anomaly_info(0, &stInfo);
             if(ret == ONVIF_ERR_EVENT_RESOURCE_CONFLICT) return soap_sender_fault(soap, "Event Resource Conflict", "Resource allocation failed for Audio Anomaly.");
             if(ret != 0) nRet = SOAP_FAULT;
             else nRet = SOAP_OK;
        }
        else if(!strncmp(pAnalyticsModule->Name,SCENE_CHANGE_EVENT_MODULE, strlen(SCENE_CHANGE_EVENT_MODULE)) && !strcmp(pAnalyticsModule->Type,SCENE_CHANGE_EVENT_MODULE_TYPE))
        {
             OnvifSceneChange_S stInfo;
             memset(&stInfo, 0, sizeof(stInfo));
             
             for(int j = 0;j < pAnalyticsModule->Parameters->__sizeSimpleItem;j++)
             {
                struct _tt__ItemList_SimpleItem *pParam = pAnalyticsModule->Parameters->SimpleItem + j;
                if(pParam && pParam->Name && pParam->Value)
                {
                    if(!strcmp(pParam->Name,ONVIF_ANALYTICS_SENS_NAME)) {
                        strncpy(stInfo.achSensitivity, pParam->Value, sizeof(stInfo.achSensitivity)-1);
                    }
                    else if(!strcmp(pParam->Name, "Enable")) {
                        stInfo.bEnable = !strcmp(pParam->Value, "true");
                    }
                }
             }
             int ret = onvif_set_scene_change_info(0, &stInfo);
             if(ret == ONVIF_ERR_EVENT_RESOURCE_CONFLICT) return soap_sender_fault(soap, "Event Resource Conflict", "Resource allocation failed for Scene Change.");
             if(ret != 0) nRet = SOAP_FAULT;
             else nRet = SOAP_OK;
        }
        else if(!strncmp(pAnalyticsModule->Name,FACE_DETECT_EVENT_MODULE, strlen(FACE_DETECT_EVENT_MODULE)) && !strcmp(pAnalyticsModule->Type,FACE_DETECT_EVENT_MODULE_TYPE))
        {
             OnvifFaceDetection_S stInfoTemplate;
             memset(&stInfoTemplate, 0, sizeof(stInfoTemplate));
             
             for(int j = 0; j < pAnalyticsModule->Parameters->__sizeSimpleItem; j++) {
                struct _tt__ItemList_SimpleItem *pParam = pAnalyticsModule->Parameters->SimpleItem + j;
                if(pParam && pParam->Name && pParam->Value) {
                    if(!strcmp(pParam->Name, ONVIF_ANALYTICS_SENS_NAME)) {
                        strncpy(stInfoTemplate.achSensitivity, pParam->Value, sizeof(stInfoTemplate.achSensitivity)-1);
                    }
                    else if(!strcmp(pParam->Name, "Enable")) {
                        stInfoTemplate.bEnable = !strcmp(pParam->Value, "true");
                    }
                }
             }
             
             int nSetCount = 0;
             for(int j = 0; j < pAnalyticsModule->Parameters->__sizeElementItem; j++) {
                struct _tt__ItemList_ElementItem *pParam = pAnalyticsModule->Parameters->ElementItem + j;
                if(pParam && pParam->Name && pParam->__any) {
                    if(!strcmp(pParam->Name, ONVIF_ANALYTICS_FIELD_NAME)) {
                        OnvifFaceDetection_S stInfo = stInfoTemplate;
                        char* pStr = pParam->__any;
                        int nCount = 0;
                        while(nCount < 4) {
                            pStr = strstr(pStr, "<tt:Point");
                            if(!pStr) break;
                            pStr += strlen("<tt:Point");
                            const char *x_start = strstr(pStr, "x=\"");
                            if(!x_start) break;
                            x_start += 3;
                            int x = (int)strtol(x_start, (char**)&pStr, 10);
                            const char *y_start = strstr(pStr, "y=\"");
                            if(!y_start) break;
                            y_start += 3;
                            int y = (int)strtol(y_start, (char**)&pStr, 10);
                            stInfo.stPolygon[nCount++] = (OnvifPoint_S){x, y};
                            pStr = strchr(pStr, '/');
                            if(!pStr) break;
                            pStr += 2;
                        }
                        int ret = onvif_set_face_detect_info(nSetCount, &stInfo);
                        if(ret == ONVIF_ERR_EVENT_RESOURCE_CONFLICT) return soap_sender_fault(soap, "Event Resource Conflict", "Resource allocation failed for Face Detection.");
                        if(ret == 0) nSetCount++;
                    }
                }
             }
             
             /* Cleanup remaining */
             OnvifFaceDetection_S stEmpty;
             memset(&stEmpty, 0, sizeof(stEmpty));
             stEmpty.bEnable = false;
             while(nSetCount < 16) {
                  if(onvif_set_face_detect_info(nSetCount, &stEmpty) != 0) break;
                  nSetCount++;
             }
             nRet = SOAP_OK;
        }
        else if(!strncmp(pAnalyticsModule->Name,LOITERING_DETECT_EVENT_MODULE, strlen(LOITERING_DETECT_EVENT_MODULE)) && !strcmp(pAnalyticsModule->Type,LOITERING_DETECT_EVENT_MODULE_TYPE))
        {
             OnvifLoiteringDetection_S stInfoTemplate;
             memset(&stInfoTemplate, 0, sizeof(stInfoTemplate));
             
             for(int j = 0; j < pAnalyticsModule->Parameters->__sizeSimpleItem; j++) {
                struct _tt__ItemList_SimpleItem *pParam = pAnalyticsModule->Parameters->SimpleItem + j;
                if(pParam && pParam->Name && pParam->Value) {
 if(!strcmp(pParam->Name, "Enable")) {
                        stInfoTemplate.bEnable = !strcmp(pParam->Value, "true");
                    }
                    else if(!strcmp(pParam->Name, ONVIF_ANALYTICS_SENS_NAME)) {
                        strncpy(stInfoTemplate.achSensitivity, pParam->Value, sizeof(stInfoTemplate.achSensitivity)-1);
                    }
                }
             }
             
             int nSetCount = 0;
             for(int j = 0; j < pAnalyticsModule->Parameters->__sizeElementItem; j++) {
                struct _tt__ItemList_ElementItem *pParam = pAnalyticsModule->Parameters->ElementItem + j;
                if(pParam && pParam->Name && pParam->__any) {
                    if(!strcmp(pParam->Name, ONVIF_ANALYTICS_FIELD_NAME)) {
                        OnvifLoiteringDetection_S stInfo = stInfoTemplate;
                        char* pStr = pParam->__any;

                        /* Parse TimeThreshold */
                        char* pThreshold = strstr(pStr, "<tt:TimeThreshold>");
                        if(pThreshold) {
                            pThreshold += strlen("<tt:TimeThreshold>");
                            char* pEnd = strstr(pThreshold, "</tt:TimeThreshold>");
                            if(pEnd) {
                                size_t len = pEnd - pThreshold;
                                if(len < sizeof(stInfo.achTimeThreshold)) {
                                    strncpy(stInfo.achTimeThreshold, pThreshold, len);
                                    stInfo.achTimeThreshold[len] = '\0';
                                }
                            }
                        }

                        int nCount = 0;
                        while(nCount < 4) {
                            pStr = strstr(pStr, "<tt:Point");
                            if(!pStr) break;
                            pStr += strlen("<tt:Point");
                            const char *x_start = strstr(pStr, "x=\"");
                            if(!x_start) break;
                            x_start += 3;
                            int x = (int)strtol(x_start, (char**)&pStr, 10);
                            const char *y_start = strstr(pStr, "y=\"");
                            if(!y_start) break;
                            y_start += 3;
                            int y = (int)strtol(y_start, (char**)&pStr, 10);
                            stInfo.stPolygon[nCount++] = (OnvifPoint_S){x, y};
                            pStr = strchr(pStr, '/');
                            if(!pStr) break;
                            pStr += 2;
                        }
                        int ret = onvif_set_loitering_detect_info(nSetCount, &stInfo);
                        if(ret == ONVIF_ERR_EVENT_RESOURCE_CONFLICT) return soap_sender_fault(soap, "Event Resource Conflict", "Resource allocation failed for Loitering Detection.");
                        if(ret == 0) nSetCount++;
                    }
                }
             }
             
             /* Cleanup remaining */
             OnvifLoiteringDetection_S stEmpty;
             memset(&stEmpty, 0, sizeof(stEmpty));
             stEmpty.bEnable = false;
             while(nSetCount < 16) {
                  if(onvif_set_loitering_detect_info(nSetCount, &stEmpty) != 0) break;
                  nSetCount++;
             }
             nRet = SOAP_OK;
        }
        else if(!strncmp(pAnalyticsModule->Name,CROWD_GATHERING_EVENT_MODULE, strlen(CROWD_GATHERING_EVENT_MODULE)) && !strcmp(pAnalyticsModule->Type,CROWD_GATHERING_EVENT_MODULE_TYPE))
        {
             OnvifCrowdGathering_S stInfoTemplate;
             memset(&stInfoTemplate, 0, sizeof(stInfoTemplate));
             
             for(int j = 0; j < pAnalyticsModule->Parameters->__sizeSimpleItem; j++) {
                struct _tt__ItemList_SimpleItem *pParam = pAnalyticsModule->Parameters->SimpleItem + j;
                if(pParam && pParam->Name && pParam->Value) {
                    if(!strcmp(pParam->Name, ONVIF_ANALYTICS_SENS_NAME)) {
                        strncpy(stInfoTemplate.achSensitivity, pParam->Value, sizeof(stInfoTemplate.achSensitivity)-1);
                    }
                    else if(!strcmp(pParam->Name, "Enable")) {
                        stInfoTemplate.bEnable = !strcmp(pParam->Value, "true");
                    }
                }
             }
             
             int nSetCount = 0;
             for(int j = 0; j < pAnalyticsModule->Parameters->__sizeElementItem; j++) {
                struct _tt__ItemList_ElementItem *pParam = pAnalyticsModule->Parameters->ElementItem + j;
                if(pParam && pParam->Name && pParam->__any) {
                    if(!strcmp(pParam->Name, ONVIF_ANALYTICS_FIELD_NAME)) {
                        OnvifCrowdGathering_S stInfo = stInfoTemplate;
                        char* pStr = pParam->__any;
                        int nCount = 0;
                        while(nCount < 4) {
                            pStr = strstr(pStr, "<tt:Point");
                            if(!pStr) break;
                            pStr += strlen("<tt:Point");
                            const char *x_start = strstr(pStr, "x=\"");
                            if(!x_start) break;
                            x_start += 3;
                            int x = (int)strtol(x_start, (char**)&pStr, 10);
                            const char *y_start = strstr(pStr, "y=\"");
                            if(!y_start) break;
                            y_start += 3;
                            int y = (int)strtol(y_start, (char**)&pStr, 10);
                            stInfo.stPolygon[nCount++] = (OnvifPoint_S){x, y};
                            pStr = strchr(pStr, '/');
                            if(!pStr) break;
                            pStr += 2;
                        }
                        int ret = onvif_set_crowd_gathering_info(nSetCount, &stInfo);
                        if(ret == ONVIF_ERR_EVENT_RESOURCE_CONFLICT) return soap_sender_fault(soap, "Event Resource Conflict", "Resource allocation failed for Crowd Gathering.");
                        if(ret == 0) nSetCount++;
                    }
                }
             }
             
             /* Cleanup remaining */
             OnvifCrowdGathering_S stEmpty;
             memset(&stEmpty, 0, sizeof(stEmpty));
             stEmpty.bEnable = false;
             while(nSetCount < 16) {
                  if(onvif_set_crowd_gathering_info(nSetCount, &stEmpty) != 0) break;
                  nSetCount++;
             }
             nRet = SOAP_OK;
        }
        else if(!strncmp(pAnalyticsModule->Name,PARKING_DETECT_EVENT_MODULE, strlen(PARKING_DETECT_EVENT_MODULE)) && !strcmp(pAnalyticsModule->Type,PARKING_DETECT_EVENT_MODULE_TYPE))
        {
             OnvifParkingDetection_S stInfoTemplate;
             memset(&stInfoTemplate, 0, sizeof(stInfoTemplate));
             
             for(int j = 0; j < pAnalyticsModule->Parameters->__sizeSimpleItem; j++) {
                struct _tt__ItemList_SimpleItem *pParam = pAnalyticsModule->Parameters->SimpleItem + j;
                if(pParam && pParam->Name && pParam->Value) {
 if(!strcmp(pParam->Name, "Enable")) {
                        stInfoTemplate.bEnable = !strcmp(pParam->Value, "true");
                    }
                    else if(!strcmp(pParam->Name, ONVIF_ANALYTICS_SENS_NAME)) {
                        strncpy(stInfoTemplate.achSensitivity, pParam->Value, sizeof(stInfoTemplate.achSensitivity)-1);
                    }
                }
             }
             
             int nSetCount = 0;
             for(int j = 0; j < pAnalyticsModule->Parameters->__sizeElementItem; j++) {
                struct _tt__ItemList_ElementItem *pParam = pAnalyticsModule->Parameters->ElementItem + j;
                if(pParam && pParam->Name && pParam->__any) {
                    if(!strcmp(pParam->Name, ONVIF_ANALYTICS_FIELD_NAME)) {
                        OnvifParkingDetection_S stInfo = stInfoTemplate;
                        char* pStr = pParam->__any;

                        /* Parse TimeThreshold */
                        char* pThreshold = strstr(pStr, "<tt:TimeThreshold>");
                        if(pThreshold) {
                            pThreshold += strlen("<tt:TimeThreshold>");
                            char* pEnd = strstr(pThreshold, "</tt:TimeThreshold>");
                            if(pEnd) {
                                size_t len = pEnd - pThreshold;
                                if(len < sizeof(stInfo.achTimeThreshold)) {
                                    strncpy(stInfo.achTimeThreshold, pThreshold, len);
                                    stInfo.achTimeThreshold[len] = '\0';
                                }
                            }
                        }

                        int nCount = 0;
                        while(nCount < 4) {
                            pStr = strstr(pStr, "<tt:Point");
                            if(!pStr) break;
                            pStr += strlen("<tt:Point");
                            const char *x_start = strstr(pStr, "x=\"");
                            if(!x_start) break;
                            x_start += 3;
                            int x = (int)strtol(x_start, (char**)&pStr, 10);
                            const char *y_start = strstr(pStr, "y=\"");
                            if(!y_start) break;
                            y_start += 3;
                            int y = (int)strtol(y_start, (char**)&pStr, 10);
                            stInfo.stPolygon[nCount++] = (OnvifPoint_S){x, y};
                            pStr = strchr(pStr, '/');
                            if(!pStr) break;
                            pStr += 2;
                        }
                        int ret = onvif_set_parking_detect_info(nSetCount, &stInfo);
                        if(ret == ONVIF_ERR_EVENT_RESOURCE_CONFLICT) return soap_sender_fault(soap, "Event Resource Conflict", "Resource allocation failed for Parking Detection.");
                        if(ret == 0) nSetCount++;
                    }
                }
             }
             
             /* Cleanup remaining */
             OnvifParkingDetection_S stEmpty;
             memset(&stEmpty, 0, sizeof(stEmpty));
             stEmpty.bEnable = false;
             while(nSetCount < 16) {
                  if(onvif_set_parking_detect_info(nSetCount, &stEmpty) != 0) break;
                  nSetCount++;
             }
             nRet = SOAP_OK;
        }
        else if(!strncmp(pAnalyticsModule->Name,UNATTENDED_OBJECT_EVENT_MODULE, strlen(UNATTENDED_OBJECT_EVENT_MODULE)) && !strcmp(pAnalyticsModule->Type,UNATTENDED_OBJECT_EVENT_MODULE_TYPE))
        {
             OnvifUnattendedObject_S stInfoTemplate;
             memset(&stInfoTemplate, 0, sizeof(stInfoTemplate));
             
             for(int j = 0; j < pAnalyticsModule->Parameters->__sizeSimpleItem; j++) {
                struct _tt__ItemList_SimpleItem *pParam = pAnalyticsModule->Parameters->SimpleItem + j;
                if(pParam && pParam->Name && pParam->Value) {
 if(!strcmp(pParam->Name, "Enable")) {
                        stInfoTemplate.bEnable = !strcmp(pParam->Value, "true");
                    }
                    else if(!strcmp(pParam->Name, ONVIF_ANALYTICS_SENS_NAME)) {
                        strncpy(stInfoTemplate.achSensitivity, pParam->Value, sizeof(stInfoTemplate.achSensitivity)-1);
                    }
                }
             }
             
             int nSetCount = 0;
             for(int j = 0; j < pAnalyticsModule->Parameters->__sizeElementItem; j++) {
                struct _tt__ItemList_ElementItem *pParam = pAnalyticsModule->Parameters->ElementItem + j;
                if(pParam && pParam->Name && pParam->__any) {
                    if(!strcmp(pParam->Name, ONVIF_ANALYTICS_FIELD_NAME)) {
                        OnvifUnattendedObject_S stInfo = stInfoTemplate;
                        char* pStr = pParam->__any;

                        /* Parse TimeThreshold */
                        char* pThreshold = strstr(pStr, "<tt:TimeThreshold>");
                        if(pThreshold) {
                            pThreshold += strlen("<tt:TimeThreshold>");
                            char* pEnd = strstr(pThreshold, "</tt:TimeThreshold>");
                            if(pEnd) {
                                size_t len = pEnd - pThreshold;
                                if(len < sizeof(stInfo.achTimeThreshold)) {
                                    strncpy(stInfo.achTimeThreshold, pThreshold, len);
                                    stInfo.achTimeThreshold[len] = '\0';
                                }
                            }
                        }

                        int nCount = 0;
                        while(nCount < 4) {
                            pStr = strstr(pStr, "<tt:Point");
                            if(!pStr) break;
                            pStr += strlen("<tt:Point");
                            const char *x_start = strstr(pStr, "x=\"");
                            if(!x_start) break;
                            x_start += 3;
                            int x = (int)strtol(x_start, (char**)&pStr, 10);
                            const char *y_start = strstr(pStr, "y=\"");
                            if(!y_start) break;
                            y_start += 3;
                            int y = (int)strtol(y_start, (char**)&pStr, 10);
                            stInfo.stPolygon[nCount++] = (OnvifPoint_S){x, y};
                            pStr = strchr(pStr, '/');
                            if(!pStr) break;
                            pStr += 2;
                        }
                        int ret = onvif_set_unattended_object_info(nSetCount, &stInfo);
                        if(ret == ONVIF_ERR_EVENT_RESOURCE_CONFLICT) return soap_sender_fault(soap, "Event Resource Conflict", "Resource allocation failed for Unattended Object.");
                        if(ret == 0) nSetCount++;
                    }
                }
             }
             
             /* Cleanup remaining */
             OnvifUnattendedObject_S stEmpty;
             memset(&stEmpty, 0, sizeof(stEmpty));
             stEmpty.bEnable = false;
             while(nSetCount < 16) {
                  if(onvif_set_unattended_object_info(nSetCount, &stEmpty) != 0) break;
                  nSetCount++;
             }
             nRet = SOAP_OK;
        }
        else if(!strncmp(pAnalyticsModule->Name,OBJECT_REMOVAL_EVENT_MODULE, strlen(OBJECT_REMOVAL_EVENT_MODULE)) && !strcmp(pAnalyticsModule->Type,OBJECT_REMOVAL_EVENT_MODULE_TYPE))
        {
             OnvifObjectRemoval_S stInfoTemplate;
             memset(&stInfoTemplate, 0, sizeof(stInfoTemplate));
             
             for(int j = 0; j < pAnalyticsModule->Parameters->__sizeSimpleItem; j++) {
                struct _tt__ItemList_SimpleItem *pParam = pAnalyticsModule->Parameters->SimpleItem + j;
                if(pParam && pParam->Name && pParam->Value) {
 if(!strcmp(pParam->Name, "Enable")) {
                        stInfoTemplate.bEnable = !strcmp(pParam->Value, "true");
                    }
                    else if(!strcmp(pParam->Name, ONVIF_ANALYTICS_SENS_NAME)) {
                        strncpy(stInfoTemplate.achSensitivity, pParam->Value, sizeof(stInfoTemplate.achSensitivity)-1);
                    }
                }
             }
             
             int nSetCount = 0;
             for(int j = 0; j < pAnalyticsModule->Parameters->__sizeElementItem; j++) {
                struct _tt__ItemList_ElementItem *pParam = pAnalyticsModule->Parameters->ElementItem + j;
                if(pParam && pParam->Name && pParam->__any) {
                    if(!strcmp(pParam->Name, ONVIF_ANALYTICS_FIELD_NAME)) {
                        OnvifObjectRemoval_S stInfo = stInfoTemplate;
                        char* pStr = pParam->__any;

                        /* Parse TimeThreshold */
                        char* pThreshold = strstr(pStr, "<tt:TimeThreshold>");
                        if(pThreshold) {
                            pThreshold += strlen("<tt:TimeThreshold>");
                            char* pEnd = strstr(pThreshold, "</tt:TimeThreshold>");
                            if(pEnd) {
                                size_t len = pEnd - pThreshold;
                                if(len < sizeof(stInfo.achTimeThreshold)) {
                                    strncpy(stInfo.achTimeThreshold, pThreshold, len);
                                    stInfo.achTimeThreshold[len] = '\0';
                                }
                            }
                        }

                        int nCount = 0;
                        while(nCount < 4) {
                            pStr = strstr(pStr, "<tt:Point");
                            if(!pStr) break;
                            pStr += strlen("<tt:Point");
                            const char *x_start = strstr(pStr, "x=\"");
                            if(!x_start) break;
                            x_start += 3;
                            int x = (int)strtol(x_start, (char**)&pStr, 10);
                            const char *y_start = strstr(pStr, "y=\"");
                            if(!y_start) break;
                            y_start += 3;
                            int y = (int)strtol(y_start, (char**)&pStr, 10);
                            stInfo.stPolygon[nCount++] = (OnvifPoint_S){x, y};
                            pStr = strchr(pStr, '/');
                            if(!pStr) break;
                            pStr += 2;
                        }
                        int ret = onvif_set_object_removal_info(nSetCount, &stInfo);
                        if(ret == ONVIF_ERR_EVENT_RESOURCE_CONFLICT) return soap_sender_fault(soap, "Event Resource Conflict", "Resource allocation failed for Object Removal.");
                        if(ret == 0) nSetCount++;
                    }
                }
             }
             
             /* Cleanup remaining */
             OnvifObjectRemoval_S stEmpty;
             memset(&stEmpty, 0, sizeof(stEmpty));
             stEmpty.bEnable = false;
             while(nSetCount < 16) {
                  if(onvif_set_object_removal_info(nSetCount, &stEmpty) != 0) break;
                  nSetCount++;
             }
             nRet = SOAP_OK;
        }
        else if(!strncmp(pAnalyticsModule->Name,PET_RECOGNITION_EVENT_MODULE, strlen(PET_RECOGNITION_EVENT_MODULE)) && !strcmp(pAnalyticsModule->Type,PET_RECOGNITION_EVENT_MODULE_TYPE))
        {
             OnvifPetRecognition_S stInfoTemplate;
             memset(&stInfoTemplate, 0, sizeof(stInfoTemplate));
             
             for(int j = 0; j < pAnalyticsModule->Parameters->__sizeSimpleItem; j++) {
                struct _tt__ItemList_SimpleItem *pParam = pAnalyticsModule->Parameters->SimpleItem + j;
                if(pParam && pParam->Name && pParam->Value) {
                    if(!strcmp(pParam->Name, "Enable")) {
                        stInfoTemplate.bEnable = !strcmp(pParam->Value, "true");
                    }
                    else if(!strcmp(pParam->Name, ONVIF_ANALYTICS_SENS_NAME)) {
                        strncpy(stInfoTemplate.achSensitivity, pParam->Value, sizeof(stInfoTemplate.achSensitivity)-1);
                    }
                }
             }
             
             int nSetCount = 0;
             for(int j = 0; j < pAnalyticsModule->Parameters->__sizeElementItem; j++) {
                struct _tt__ItemList_ElementItem *pParam = pAnalyticsModule->Parameters->ElementItem + j;
                if(pParam && pParam->Name && pParam->__any) {
                    if(!strcmp(pParam->Name, ONVIF_ANALYTICS_FIELD_NAME)) {
                        OnvifPetRecognition_S stInfo = stInfoTemplate;
                        char* pStr = pParam->__any;
                        int nCount = 0;
                        while(nCount < 4) {
                            pStr = strstr(pStr, "<tt:Point");
                            if(!pStr) break;
                            pStr += strlen("<tt:Point");
                            const char *x_start = strstr(pStr, "x=\"");
                            if(!x_start) break;
                            x_start += 3;
                            int x = (int)strtol(x_start, (char**)&pStr, 10);
                            const char *y_start = strstr(pStr, "y=\"");
                            if(!y_start) break;
                            y_start += 3;
                            int y = (int)strtol(y_start, (char**)&pStr, 10);
                            stInfo.stPolygon[nCount++] = (OnvifPoint_S){x, y};
                            pStr = strchr(pStr, '/');
                            if(!pStr) break;
                            pStr += 2;
                        }
                        int ret = onvif_set_pet_recognition_info(nSetCount, &stInfo);
                        if(ret == ONVIF_ERR_EVENT_RESOURCE_CONFLICT) return soap_sender_fault(soap, "Event Resource Conflict", "Resource allocation failed for Pet Recognition.");
                        if(ret == 0) nSetCount++;
                    }
                }
             }
             
             /* Cleanup remaining */
             OnvifPetRecognition_S stEmpty;
             memset(&stEmpty, 0, sizeof(stEmpty));
             stEmpty.bEnable = false;
             while(nSetCount < 16) {
                  if(onvif_set_pet_recognition_info(nSetCount, &stEmpty) != 0) break;
                  nSetCount++;
             }
             nRet = SOAP_OK;
        }
        else if(!strncmp(pAnalyticsModule->Name,FACE_CAPTURE_EVENT_MODULE, strlen(FACE_CAPTURE_EVENT_MODULE)) && !strcmp(pAnalyticsModule->Type,FACE_CAPTURE_EVENT_MODULE_TYPE))
        {
             OnvifFaceCapture_S stInfoTemplate;
             memset(&stInfoTemplate, 0, sizeof(stInfoTemplate));
             
             for(int j = 0; j < pAnalyticsModule->Parameters->__sizeSimpleItem; j++) {
                struct _tt__ItemList_SimpleItem *pParam = pAnalyticsModule->Parameters->SimpleItem + j;
                if(pParam && pParam->Name && pParam->Value) {
                    if(!strcmp(pParam->Name, "Interval")) {
                        strncpy(stInfoTemplate.achInterval, pParam->Value, sizeof(stInfoTemplate.achInterval)-1);
                    }
                    else if(!strcmp(pParam->Name, "Enable")) {
                        stInfoTemplate.bEnable = !strcmp(pParam->Value, "true");
                    }
                    else if(!strcmp(pParam->Name, ONVIF_ANALYTICS_SENS_NAME)) {
                        strncpy(stInfoTemplate.achSensitivity, pParam->Value, sizeof(stInfoTemplate.achSensitivity)-1);
                    }
                }
             }
             
             int nSetCount = 0;
             for(int j = 0; j < pAnalyticsModule->Parameters->__sizeElementItem; j++) {
                struct _tt__ItemList_ElementItem *pParam = pAnalyticsModule->Parameters->ElementItem + j;
                if(pParam && pParam->Name && pParam->__any) {
                    if(!strcmp(pParam->Name, ONVIF_ANALYTICS_FIELD_NAME)) {
                        OnvifFaceCapture_S stInfo = stInfoTemplate;
                        char* pStr = pParam->__any;
                        int nCount = 0;
                        while(nCount < 4) {
                            pStr = strstr(pStr, "<tt:Point");
                            if(!pStr) break;
                            pStr += strlen("<tt:Point");
                            const char *x_start = strstr(pStr, "x=\"");
                            if(!x_start) break;
                            x_start += 3;
                            int x = (int)strtol(x_start, (char**)&pStr, 10);
                            const char *y_start = strstr(pStr, "y=\"");
                            if(!y_start) break;
                            y_start += 3;
                            int y = (int)strtol(y_start, (char**)&pStr, 10);
                            stInfo.stPolygon[nCount++] = (OnvifPoint_S){x, y};
                            pStr = strchr(pStr, '/');
                            if(!pStr) break;
                            pStr += 2;
                        }
                        int ret = onvif_set_face_capture_info(nSetCount, &stInfo);
                        if(ret == ONVIF_ERR_EVENT_RESOURCE_CONFLICT) return soap_sender_fault(soap, "Event Resource Conflict", "Resource allocation failed for Face Capture.");
                        if(ret == 0) nSetCount++;
                    }
                }
             }
             
             /* Cleanup remaining */
             OnvifFaceCapture_S stEmpty;
             memset(&stEmpty, 0, sizeof(stEmpty));
             stEmpty.bEnable = false;
             while(nSetCount < 16) {
                  if(onvif_set_face_capture_info(nSetCount, &stEmpty) != 0) break;
                  nSetCount++;
             }
             nRet = SOAP_OK;
        }
    }

    if(nRet != SOAP_OK)
    {
        dlog_debug("----------分析模块设置失败----------");
    }

    return nRet;
}

SOAP_FMAC5 int SOAP_FMAC6 __tan__GetSupportedMetadata(struct soap* soap, struct _tan__GetSupportedMetadata *tan__GetSupportedMetadata, struct _tan__GetSupportedMetadataResponse *tan__GetSupportedMetadataResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tan__GetSupportedMetadata----------");
#endif
    return SOAP_OK;
}