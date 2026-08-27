/**
 * @file deviceIo.c
 * @author tianl (tianl@kfb.cn)
 * @date 2025-09-23
 * 
 * @brief onvif deviceIo服务接口
 */
#include "onvif_server_wrapper.h"


/** Web service operation '__tmd__GetServiceCapabilities' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tmd__GetServiceCapabilities(struct soap* soap, struct _tmd__GetServiceCapabilities *tmd__GetServiceCapabilities, struct _tmd__GetServiceCapabilitiesResponse *tmd__GetServiceCapabilitiesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tmd__GetServiceCapabilities----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tmd__GetRelayOutputOptions' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tmd__GetRelayOutputOptions(struct soap* soap, struct _tmd__GetRelayOutputOptions *tmd__GetRelayOutputOptions, struct _tmd__GetRelayOutputOptionsResponse *tmd__GetRelayOutputOptionsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tmd__GetRelayOutputOptions----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tmd__GetAudioSources' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tmd__GetAudioSources(struct soap* soap, struct tmd__Get *tmd__GetAudioSources, struct tmd__GetResponse *tmd__GetAudioSourcesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tmd__GetAudioSources----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tmd__GetAudioOutputs' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tmd__GetAudioOutputs(struct soap* soap, struct tmd__Get *tmd__GetAudioOutputs, struct tmd__GetResponse *tmd__GetAudioOutputsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tmd__GetAudioOutputs----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tmd__GetVideoSources' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tmd__GetVideoSources(struct soap* soap, struct tmd__Get *tmd__GetVideoSources, struct tmd__GetResponse *tmd__GetVideoSourcesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tmd__GetVideoSources----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tmd__GetVideoOutputs' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tmd__GetVideoOutputs(struct soap* soap, struct _tmd__GetVideoOutputs *tmd__GetVideoOutputs, struct _tmd__GetVideoOutputsResponse *tmd__GetVideoOutputsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tmd__GetVideoOutputs----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tmd__GetVideoSourceConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tmd__GetVideoSourceConfiguration(struct soap* soap, struct _tmd__GetVideoSourceConfiguration *tmd__GetVideoSourceConfiguration, struct _tmd__GetVideoSourceConfigurationResponse *tmd__GetVideoSourceConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tmd__GetVideoSourceConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tmd__GetVideoOutputConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tmd__GetVideoOutputConfiguration(struct soap* soap, struct _tmd__GetVideoOutputConfiguration *tmd__GetVideoOutputConfiguration, struct _tmd__GetVideoOutputConfigurationResponse *tmd__GetVideoOutputConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tmd__GetVideoOutputConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tmd__GetAudioSourceConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tmd__GetAudioSourceConfiguration(struct soap* soap, struct _tmd__GetAudioSourceConfiguration *tmd__GetAudioSourceConfiguration, struct _tmd__GetAudioSourceConfigurationResponse *tmd__GetAudioSourceConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tmd__GetAudioSourceConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tmd__GetAudioOutputConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tmd__GetAudioOutputConfiguration(struct soap* soap, struct _tmd__GetAudioOutputConfiguration *tmd__GetAudioOutputConfiguration, struct _tmd__GetAudioOutputConfigurationResponse *tmd__GetAudioOutputConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tmd__GetAudioOutputConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tmd__SetVideoSourceConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tmd__SetVideoSourceConfiguration(struct soap* soap, struct _tmd__SetVideoSourceConfiguration *tmd__SetVideoSourceConfiguration, struct _tmd__SetVideoSourceConfigurationResponse *tmd__SetVideoSourceConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tmd__SetVideoSourceConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tmd__SetVideoOutputConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tmd__SetVideoOutputConfiguration(struct soap* soap, struct _tmd__SetVideoOutputConfiguration *tmd__SetVideoOutputConfiguration, struct _tmd__SetVideoOutputConfigurationResponse *tmd__SetVideoOutputConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tmd__SetVideoOutputConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tmd__SetAudioSourceConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tmd__SetAudioSourceConfiguration(struct soap* soap, struct _tmd__SetAudioSourceConfiguration *tmd__SetAudioSourceConfiguration, struct _tmd__SetAudioSourceConfigurationResponse *tmd__SetAudioSourceConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tmd__SetAudioSourceConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tmd__SetAudioOutputConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tmd__SetAudioOutputConfiguration(struct soap* soap, struct _tmd__SetAudioOutputConfiguration *tmd__SetAudioOutputConfiguration, struct _tmd__SetAudioOutputConfigurationResponse *tmd__SetAudioOutputConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tmd__SetAudioOutputConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tmd__GetVideoSourceConfigurationOptions' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tmd__GetVideoSourceConfigurationOptions(struct soap* soap, struct _tmd__GetVideoSourceConfigurationOptions *tmd__GetVideoSourceConfigurationOptions, struct _tmd__GetVideoSourceConfigurationOptionsResponse *tmd__GetVideoSourceConfigurationOptionsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tmd__GetVideoSourceConfigurationOptions----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tmd__GetVideoOutputConfigurationOptions' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tmd__GetVideoOutputConfigurationOptions(struct soap* soap, struct _tmd__GetVideoOutputConfigurationOptions *tmd__GetVideoOutputConfigurationOptions, struct _tmd__GetVideoOutputConfigurationOptionsResponse *tmd__GetVideoOutputConfigurationOptionsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tmd__GetVideoOutputConfigurationOptions----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tmd__GetAudioSourceConfigurationOptions' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tmd__GetAudioSourceConfigurationOptions(struct soap* soap, struct _tmd__GetAudioSourceConfigurationOptions *tmd__GetAudioSourceConfigurationOptions, struct _tmd__GetAudioSourceConfigurationOptionsResponse *tmd__GetAudioSourceConfigurationOptionsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tmd__GetAudioSourceConfigurationOptions----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tmd__GetAudioOutputConfigurationOptions' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tmd__GetAudioOutputConfigurationOptions(struct soap* soap, struct _tmd__GetAudioOutputConfigurationOptions *tmd__GetAudioOutputConfigurationOptions, struct _tmd__GetAudioOutputConfigurationOptionsResponse *tmd__GetAudioOutputConfigurationOptionsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tmd__GetAudioOutputConfigurationOptions----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tmd__GetRelayOutputs' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tmd__GetRelayOutputs(struct soap* soap, struct _tds__GetRelayOutputs *tds__GetRelayOutputs, struct _tds__GetRelayOutputsResponse *tds__GetRelayOutputsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tmd__GetRelayOutputs----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tmd__SetRelayOutputSettings' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tmd__SetRelayOutputSettings(struct soap* soap, struct _tmd__SetRelayOutputSettings *tmd__SetRelayOutputSettings, struct _tmd__SetRelayOutputSettingsResponse *tmd__SetRelayOutputSettingsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tmd__SetRelayOutputSettings----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tmd__SetRelayOutputState' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tmd__SetRelayOutputState(struct soap* soap, struct _tds__SetRelayOutputState *tds__SetRelayOutputState, struct _tds__SetRelayOutputStateResponse *tds__SetRelayOutputStateResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tmd__SetRelayOutputState----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tmd__GetDigitalInputs' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tmd__GetDigitalInputs(struct soap* soap, struct _tmd__GetDigitalInputs *tmd__GetDigitalInputs, struct _tmd__GetDigitalInputsResponse *tmd__GetDigitalInputsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tmd__GetDigitalInputs----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tmd__GetDigitalInputConfigurationOptions' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tmd__GetDigitalInputConfigurationOptions(struct soap* soap, struct _tmd__GetDigitalInputConfigurationOptions *tmd__GetDigitalInputConfigurationOptions, struct _tmd__GetDigitalInputConfigurationOptionsResponse *tmd__GetDigitalInputConfigurationOptionsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tmd__GetDigitalInputConfigurationOptions----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tmd__SetDigitalInputConfigurations' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tmd__SetDigitalInputConfigurations(struct soap* soap, struct _tmd__SetDigitalInputConfigurations *tmd__SetDigitalInputConfigurations, struct _tmd__SetDigitalInputConfigurationsResponse *tmd__SetDigitalInputConfigurationsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tmd__SetDigitalInputConfigurations----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tmd__GetSerialPorts' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tmd__GetSerialPorts(struct soap* soap, struct _tmd__GetSerialPorts *tmd__GetSerialPorts, struct _tmd__GetSerialPortsResponse *tmd__GetSerialPortsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tmd__GetSerialPorts----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tmd__GetSerialPortConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tmd__GetSerialPortConfiguration(struct soap* soap, struct _tmd__GetSerialPortConfiguration *tmd__GetSerialPortConfiguration, struct _tmd__GetSerialPortConfigurationResponse *tmd__GetSerialPortConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tmd__GetSerialPortConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tmd__SetSerialPortConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tmd__SetSerialPortConfiguration(struct soap* soap, struct _tmd__SetSerialPortConfiguration *tmd__SetSerialPortConfiguration, struct _tmd__SetSerialPortConfigurationResponse *tmd__SetSerialPortConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tmd__SetSerialPortConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tmd__GetSerialPortConfigurationOptions' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tmd__GetSerialPortConfigurationOptions(struct soap* soap, struct _tmd__GetSerialPortConfigurationOptions *tmd__GetSerialPortConfigurationOptions, struct _tmd__GetSerialPortConfigurationOptionsResponse *tmd__GetSerialPortConfigurationOptionsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tmd__GetSerialPortConfigurationOptions----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tmd__SendReceiveSerialCommand' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tmd__SendReceiveSerialCommand(struct soap* soap, struct _tmd__SendReceiveSerialCommand *tmd__SendReceiveSerialCommand, struct _tmd__SendReceiveSerialCommandResponse *tmd__SendReceiveSerialCommandResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tmd__SendReceiveSerialCommand----------");
#endif
    return SOAP_OK;
}
