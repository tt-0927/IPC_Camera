/**
 * @file ptz.c
 * @author tianl (tianl@kfb.cn)
 * @date 2025-09-23
 * 
 * @brief onvif ptz服务接口
 */
#include "onvif_server_wrapper.h"


SOAP_FMAC5 int SOAP_FMAC6 __tptz__GetServiceCapabilities(struct soap* soap, struct _tptz__GetServiceCapabilities *tptz__GetServiceCapabilities, struct _tptz__GetServiceCapabilitiesResponse *tptz__GetServiceCapabilitiesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tptz__GetServiceCapabilities----------");
#endif
    return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tptz__GetConfigurations(struct soap* soap, struct _tptz__GetConfigurations *tptz__GetConfigurations, struct _tptz__GetConfigurationsResponse *tptz__GetConfigurationsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tptz__GetConfigurations----------");
#endif
    return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tptz__GetPresets(struct soap* soap, struct _tptz__GetPresets *tptz__GetPresets, struct _tptz__GetPresetsResponse *tptz__GetPresetsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tptz__GetPresets----------");
#endif
    tptz__GetPresetsResponse = (struct _tptz__GetPresetsResponse *)soap_malloc(soap,sizeof(struct _tptz__GetPresetsResponse));
    memset(tptz__GetPresetsResponse, 0, sizeof(struct _tptz__GetPresetsResponse));

    tptz__GetPresetsResponse->Preset = (struct tt__PTZPreset *)soap_malloc(soap,sizeof(struct tt__PTZPreset));
    memset(tptz__GetPresetsResponse->Preset, 0, sizeof(struct tt__PTZPreset));

    tptz__GetPresetsResponse->Preset->Name = (char *)soap_malloc(soap, sizeof(char) * 16);
    memset(tptz__GetPresetsResponse->Preset->Name, '\0', sizeof(char) * 16);
    strcpy(tptz__GetPresetsResponse->Preset->Name, "PTZConfig");
    tptz__GetPresetsResponse->Preset->token = (char *)soap_malloc(soap, sizeof(char) * 16);
    memset(tptz__GetPresetsResponse->Preset->token, '\0', sizeof(char) * 16);
    //strcpy(tptz__GetPresetsResponse->Preset->token, "ptznode_token_0");
    strcpy(tptz__GetPresetsResponse->Preset->token, tptz__GetPresets->ProfileToken);

    tptz__GetPresetsResponse->Preset->PTZPosition = (struct tt__PTZVector *)soap_malloc(soap,sizeof(struct tt__PTZVector));
    memset(tptz__GetPresetsResponse->Preset->PTZPosition, 0, sizeof(struct tt__PTZVector));

    tptz__GetPresetsResponse->Preset->PTZPosition->PanTilt = (struct tt__Vector2D *)soap_malloc(soap,sizeof(struct tt__Vector2D));
    memset(tptz__GetPresetsResponse->Preset->PTZPosition->PanTilt, 0, sizeof(struct tt__Vector2D));
    tptz__GetPresetsResponse->Preset->PTZPosition->PanTilt->x = 0.0;
    tptz__GetPresetsResponse->Preset->PTZPosition->PanTilt->y = 0.0;
    tptz__GetPresetsResponse->Preset->PTZPosition->PanTilt->space = (char *)soap_malloc(soap,sizeof(char) * 128);
    memset(tptz__GetPresetsResponse->Preset->PTZPosition->PanTilt->space, 0, sizeof(char) * 128);
    strcpy(tptz__GetPresetsResponse->Preset->PTZPosition->PanTilt->space, "http://www.onvif.org/ver10/tptzPanTiltSpaces/GenericSpeedSpace");

    tptz__GetPresetsResponse->Preset->PTZPosition->Zoom = (struct tt__Vector1D *)soap_malloc(soap,sizeof(struct tt__Vector1D));
    memset(tptz__GetPresetsResponse->Preset->PTZPosition->Zoom, 0, sizeof(struct tt__Vector1D));
    tptz__GetPresetsResponse->Preset->PTZPosition->Zoom->x = 0.0;
    tptz__GetPresetsResponse->Preset->PTZPosition->Zoom->space = (char *)soap_malloc(soap,sizeof(char) * 128);
    memset(tptz__GetPresetsResponse->Preset->PTZPosition->Zoom->space, 0, sizeof(char) * 128);
    strcpy(tptz__GetPresetsResponse->Preset->PTZPosition->Zoom->space, "http://www.onvif.org/ver10/tptzPanTiltSpaces/GenericSpeedSpace");
    
    return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tptz__SetPreset(struct soap* soap, struct _tptz__SetPreset *tptz__SetPreset, struct _tptz__SetPresetResponse *tptz__SetPresetResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tptz__SetPreset----------");
#endif
    return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tptz__RemovePreset(struct soap* soap, struct _tptz__RemovePreset *tptz__RemovePreset, struct _tptz__RemovePresetResponse *tptz__RemovePresetResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tptz__RemovePreset----------");
#endif
    return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tptz__GotoPreset(struct soap* soap, struct _tptz__GotoPreset *tptz__GotoPreset, struct _tptz__GotoPresetResponse *tptz__GotoPresetResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tptz__GotoPreset----------");
#endif
    return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tptz__GetStatus(struct soap* soap, struct _tptz__GetStatus *tptz__GetStatus, struct _tptz__GetStatusResponse *tptz__GetStatusResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tptz__GetStatus----------");
#endif
    return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tptz__GetConfiguration(struct soap* soap, struct _tptz__GetConfiguration *tptz__GetConfiguration, struct _tptz__GetConfigurationResponse *tptz__GetConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tptz__GetConfiguration----------");
#endif
    return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tptz__GetNodes(struct soap* soap, struct _tptz__GetNodes *tptz__GetNodes, struct _tptz__GetNodesResponse *tptz__GetNodesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tptz__GetNodes----------");
#endif
    return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tptz__GetNode(struct soap* soap, struct _tptz__GetNode *tptz__GetNode, struct _tptz__GetNodeResponse *tptz__GetNodeResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tptz__GetNode----------");
#endif
    tptz__GetNodeResponse = (struct _tptz__GetNodeResponse *)soap_malloc(soap,sizeof(struct _tptz__GetNodeResponse));
    memset(tptz__GetNodeResponse, 0, sizeof(struct _tptz__GetNodeResponse));

    tptz__GetNodeResponse->PTZNode = (struct tt__PTZNode *)soap_malloc(soap,sizeof(struct tt__PTZNode));
    memset(tptz__GetNodeResponse->PTZNode, 0, sizeof(struct tt__PTZNode));

    tptz__GetNodeResponse->PTZNode->Name = (char *)soap_malloc(soap, sizeof(char) * 16);
    memset(tptz__GetNodeResponse->PTZNode->Name, '\0', sizeof(char) * 16);
    strcpy(tptz__GetNodeResponse->PTZNode->Name, "PTZConfig");
    tptz__GetNodeResponse->PTZNode->token = (char *)soap_malloc(soap, sizeof(char) * 16);
    memset(tptz__GetNodeResponse->PTZNode->token, '\0', sizeof(char) * 16);
    //strcpy(tptz__GetNodeResponse->PTZNode->token, "ptznode_token_0");
    strcpy(tptz__GetNodeResponse->PTZNode->token, tptz__GetNode->NodeToken);

    tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces = (struct tt__PTZSpaces *)soap_malloc(soap,sizeof(struct tt__PTZSpaces));
    memset(tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces, 0, sizeof(struct tt__PTZSpaces));

    tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace = (struct tt__Space2DDescription *)soap_malloc(soap,sizeof(struct tt__Space2DDescription));
    memset(tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace, 0, sizeof(struct tt__Space2DDescription));
    tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace->XRange = (struct tt__FloatRange *)soap_malloc(soap,sizeof(struct tt__FloatRange));
    memset(tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace->XRange, 0, sizeof(struct tt__FloatRange));
    tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace->YRange = (struct tt__FloatRange *)soap_malloc(soap,sizeof(struct tt__FloatRange));
    memset(tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace->YRange, 0, sizeof(struct tt__FloatRange));
    tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace->XRange->Min = 0;
    tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace->XRange->Max = 1;
    tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace->YRange->Min = 0;
    tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace->YRange->Max = 1;

    tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousZoomVelocitySpace = (struct tt__Space1DDescription *)soap_malloc(soap,sizeof(struct tt__Space1DDescription));
    memset(tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousZoomVelocitySpace, 0, sizeof(struct tt__Space1DDescription));
    tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousZoomVelocitySpace->XRange = (struct tt__FloatRange *)soap_malloc(soap,sizeof(struct tt__FloatRange));
    memset(tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousZoomVelocitySpace->XRange, 0, sizeof(struct tt__FloatRange));
    tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousZoomVelocitySpace->XRange->Min = 0;
    tptz__GetNodeResponse->PTZNode->SupportedPTZSpaces->ContinuousZoomVelocitySpace->XRange->Max = 1;

    tptz__GetNodeResponse->PTZNode->GeoMove = (enum xsd__boolean *)soap_malloc(soap,sizeof(enum xsd__boolean));
    *(tptz__GetNodeResponse->PTZNode->GeoMove) = xsd__boolean__true_;
    
    return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tptz__SetConfiguration(struct soap* soap, struct _tptz__SetConfiguration *tptz__SetConfiguration, struct _tptz__SetConfigurationResponse *tptz__SetConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tptz__SetConfiguration----------");
#endif
    return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tptz__GetConfigurationOptions(struct soap* soap, struct _tptz__GetConfigurationOptions *tptz__GetConfigurationOptions, struct _tptz__GetConfigurationOptionsResponse *tptz__GetConfigurationOptionsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tptz__GetConfigurationOptions----------");
#endif
    return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tptz__GotoHomePosition(struct soap* soap, struct _tptz__GotoHomePosition *tptz__GotoHomePosition, struct _tptz__GotoHomePositionResponse *tptz__GotoHomePositionResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tptz__GotoHomePosition----------");
#endif
    return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tptz__SetHomePosition(struct soap* soap, struct _tptz__SetHomePosition *tptz__SetHomePosition, struct _tptz__SetHomePositionResponse *tptz__SetHomePositionResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tptz__SetHomePosition----------");
#endif
    return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tptz__ContinuousMove(struct soap* soap, struct _tptz__ContinuousMove *tptz__ContinuousMove, struct _tptz__ContinuousMoveResponse *tptz__ContinuousMoveResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tptz__ContinuousMove----------");
#endif
    if(NULL == tptz__ContinuousMove)
    {
        return SOAP_OK;
    }
    
    if(NULL == tptz__ContinuousMove->Velocity)
    {
        return SOAP_OK;
    }

    if(NULL != tptz__ContinuousMove->Velocity->PanTilt)
    {
        if(-0.001 > tptz__ContinuousMove->Velocity->PanTilt->x)
        {
            // StreamCommunication::intansce()->camControl(CAM_PT_LEFT);
        }
        else if(0.001 < tptz__ContinuousMove->Velocity->PanTilt->x)
        {
            // StreamCommunication::intansce()->camControl(CAM_PT_RIGHT);
        }

        if(-0.001 > tptz__ContinuousMove->Velocity->PanTilt->y)
        {
            // StreamCommunication::intansce()->camControl(CAM_PT_DOWN);
        }
        else if(0.001 < tptz__ContinuousMove->Velocity->PanTilt->y)
        {
            // StreamCommunication::intansce()->camControl(CAM_PT_UP);
        }
    }

    if(NULL != tptz__ContinuousMove->Velocity->Zoom)
    {
        if(-0.001 > tptz__ContinuousMove->Velocity->Zoom->x)
        {
            // StreamCommunication::intansce()->camZoom(CAM_ZOOM_WIDE);
        }
        else if(0.001 < tptz__ContinuousMove->Velocity->Zoom->x)
        {
            // StreamCommunication::intansce()->camZoom(CAM_ZOOM_TELE);
        }
    }

    return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tptz__RelativeMove(struct soap* soap, struct _tptz__RelativeMove *tptz__RelativeMove, struct _tptz__RelativeMoveResponse *tptz__RelativeMoveResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tptz__RelativeMove----------");
#endif
    return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tptz__SendAuxiliaryCommand(struct soap* soap, struct _tptz__SendAuxiliaryCommand *tptz__SendAuxiliaryCommand, struct _tptz__SendAuxiliaryCommandResponse *tptz__SendAuxiliaryCommandResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tptz__SendAuxiliaryCommand----------");
#endif
    return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tptz__AbsoluteMove(struct soap* soap, struct _tptz__AbsoluteMove *tptz__AbsoluteMove, struct _tptz__AbsoluteMoveResponse *tptz__AbsoluteMoveResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tptz__AbsoluteMove----------");
#endif
    return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tptz__Stop(struct soap* soap, struct _tptz__Stop *tptz__Stop, struct _tptz__StopResponse *tptz__StopResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tptz__Stop----------");
#endif
    if(NULL != tptz__Stop->PanTilt && true == *(tptz__Stop->PanTilt))
    {
        // StreamCommunication::intansce()->camControl(CAM_PT_STOP);
    }

    if(NULL != tptz__Stop->Zoom && true == *(tptz__Stop->Zoom))
    {
        // StreamCommunication::intansce()->camZoom(CAM_ZOOM_STOP);
    }
    return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tptz__GetPresetTours(struct soap* soap, struct _tptz__GetPresetTours *tptz__GetPresetTours, struct _tptz__GetPresetToursResponse *tptz__GetPresetToursResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tptz__GetPresetTours----------");
#endif
    return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tptz__GetPresetTour(struct soap* soap, struct _tptz__GetPresetTour *tptz__GetPresetTour, struct _tptz__GetPresetTourResponse *tptz__GetPresetTourResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tptz__GetPresetTour----------");
#endif
    return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tptz__GetPresetTourOptions(struct soap* soap, struct _tptz__GetPresetTourOptions *tptz__GetPresetTourOptions, struct _tptz__GetPresetTourOptionsResponse *tptz__GetPresetTourOptionsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tptz__GetPresetTourOptions----------");
#endif
    return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tptz__CreatePresetTour(struct soap* soap, struct _tptz__CreatePresetTour *tptz__CreatePresetTour, struct _tptz__CreatePresetTourResponse *tptz__CreatePresetTourResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tptz__CreatePresetTour----------");
#endif
    return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tptz__ModifyPresetTour(struct soap* soap, struct _tptz__ModifyPresetTour *tptz__ModifyPresetTour, struct _tptz__ModifyPresetTourResponse *tptz__ModifyPresetTourResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tptz__ModifyPresetTour----------");
#endif
    return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tptz__OperatePresetTour(struct soap* soap, struct _tptz__OperatePresetTour *tptz__OperatePresetTour, struct _tptz__OperatePresetTourResponse *tptz__OperatePresetTourResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tptz__OperatePresetTour----------");
#endif
    return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tptz__RemovePresetTour(struct soap* soap, struct _tptz__RemovePresetTour *tptz__RemovePresetTour, struct _tptz__RemovePresetTourResponse *tptz__RemovePresetTourResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tptz__RemovePresetTour----------");
#endif
    return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tptz__GetCompatibleConfigurations(struct soap* soap, struct _tptz__GetCompatibleConfigurations *tptz__GetCompatibleConfigurations, struct _tptz__GetCompatibleConfigurationsResponse *tptz__GetCompatibleConfigurationsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tptz__GetCompatibleConfigurations----------");
#endif
    return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tptz__MoveAndStartTracking(struct soap* soap, struct _tptz__MoveAndStartTracking *tptz__MoveAndStartTracking, struct _tptz__MoveAndStartTrackingResponse *tptz__MoveAndStartTrackingResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tptz__MoveAndStartTracking----------");
#endif
    return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tptz__GeoMove(struct soap* soap, struct _tptz__GeoMove *tptz__GeoMove, struct _tptz__GeoMoveResponse *tptz__GeoMoveResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tptz__GeoMove----------");
#endif
    return SOAP_FAULT;
}
