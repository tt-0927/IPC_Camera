/**
 * @file replay.c
 * @author tianl (tianl@kfb.cn)
 * @date 2025-09-23
 * 
 * @brief onvif replay服务接口
 */
#include "onvif_server_wrapper.h"

/** Web service operation '__trp__GetServiceCapabilities' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trp__GetServiceCapabilities(struct soap* soap, struct _trp__GetServiceCapabilities *trp__GetServiceCapabilities, struct _trp__GetServiceCapabilitiesResponse *trp__GetServiceCapabilitiesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trp__GetServiceCapabilities----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trp__GetReplayUri' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trp__GetReplayUri(struct soap* soap, struct _trp__GetReplayUri *trp__GetReplayUri, struct _trp__GetReplayUriResponse *trp__GetReplayUriResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trp__GetReplayUri----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trp__GetReplayConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trp__GetReplayConfiguration(struct soap* soap, struct _trp__GetReplayConfiguration *trp__GetReplayConfiguration, struct _trp__GetReplayConfigurationResponse *trp__GetReplayConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trp__GetReplayConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trp__SetReplayConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trp__SetReplayConfiguration(struct soap* soap, struct _trp__SetReplayConfiguration *trp__SetReplayConfiguration, struct _trp__SetReplayConfigurationResponse *trp__SetReplayConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trp__SetReplayConfiguration----------");
#endif
    return SOAP_OK;
}