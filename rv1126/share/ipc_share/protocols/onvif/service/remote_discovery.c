/**
 * @file recording.c
 * @author tianl (tianl@kfb.cn)
 * @date 2025-09-23
 * 
 * @brief onvif recording服务接口
 */
#include "onvif_server_wrapper.h"

/** Web service operation '__tdn__Hello' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tdn__Hello(struct soap* soap, struct wsdd__HelloType tdn__Hello, struct wsdd__ResolveType *tdn__HelloResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tdn__Hello----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tdn__Bye' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tdn__Bye(struct soap* soap, struct wsdd__ByeType tdn__Bye, struct wsdd__ResolveType *tdn__ByeResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tdn__Bye----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tdn__Probe' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tdn__Probe(struct soap* soap, struct wsdd__ProbeType tdn__Probe, struct wsdd__ProbeMatchesType *tdn__ProbeResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tdn__Probe----------");
#endif
    return SOAP_OK;
}
