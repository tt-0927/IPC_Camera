/**
 * @file device.c
 * @author tianl (tianl@kfb.cn)
 * @date 2025-09-23
 * 
 * @brief onvif device服务接口
 */

#include "onvif_server_wrapper.h"

SOAP_FMAC5 int SOAP_FMAC6 SOAP_ENV__Fault(struct soap* soap, char *faultcode, char *faultstring, char *faultactor,
struct SOAP_ENV__Detail *detail, struct SOAP_ENV__Code *_SOAP_ENV__Code,
struct SOAP_ENV__Reason *SOAP_ENV__Reason, char *SOAP_ENV__Node,
	char *SOAP_ENV__Role, struct SOAP_ENV__Detail *SOAP_ENV__Detail)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------SOAP_ENV__Fault----------");
#endif
	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __wsdd__Hello(struct soap* soap, struct wsdd__HelloType *wsdd__Hello)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__wsdd__Hello----------");
#endif
	return SOAP_OK;
}
 
SOAP_FMAC5 int SOAP_FMAC6 __wsdd__Bye(struct soap* soap, struct wsdd__ByeType *wsdd__Bye)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__wsdd__Bye----------");
#endif
	return SOAP_OK;
}
 
SOAP_FMAC5 int SOAP_FMAC6 __wsdd__ProbeMatches(struct soap* soap, struct wsdd__ProbeMatchesType *wsdd__ProbeMatches)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__wsdd__ProbeMatches----------");
#endif
    return SOAP_OK;
}
 
SOAP_FMAC5 int SOAP_FMAC6 __wsdd__Resolve(struct soap* soap, struct wsdd__ResolveType *wsdd__Resolve)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__wsdd__Resolve----------");
#endif
	return SOAP_OK;
}
 
SOAP_FMAC5 int SOAP_FMAC6 __wsdd__ResolveMatches(struct soap* soap, struct wsdd__ResolveMatchesType *wsdd__ResolveMatches)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__wsdd__ResolveMatches----------");
#endif
    return SOAP_OK;
}

// SOAP_FMAC5 int SOAP_FMAC6 soap_send___wsdd__ProbeMatches(struct soap *soap, const char *soap_endpoint, const char *soap_action, struct wsdd__ProbeMatchesType *wsdd__ProbeMatches)
// {
// #if ONVIF_LOG_SWITCH
//     dlog_debug("----------soap_send___wsdd__ProbeMatches----------");
// #endif
//     struct __wsdd__ProbeMatches soap_tmp___wsdd__ProbeMatches;
// 	if (soap_action == NULL)
// 		soap_action = "http://docs.oasis-open.org/ws-dd/ns/discovery/2009/01/ProbeMatches";
// 	soap_tmp___wsdd__ProbeMatches.wsdd__ProbeMatches = wsdd__ProbeMatches;
// 	soap_begin(soap);
// 	soap_set_version(soap, 2); /* use SOAP1.2 */
// 	soap->encodingStyle = NULL; /* use SOAP literal style */
// 	soap_serializeheader(soap);
// 	soap_serialize___wsdd__ProbeMatches(soap, &soap_tmp___wsdd__ProbeMatches);
// 	if (soap_begin_count(soap))
// 		return soap->error;
// 	if ((soap->mode & SOAP_IO_LENGTH))
// 	{	if (soap_envelope_begin_out(soap)
// 		 || soap_putheader(soap)
// 		 || soap_body_begin_out(soap)
// 		 || soap_put___wsdd__ProbeMatches(soap, &soap_tmp___wsdd__ProbeMatches, "-wsdd:ProbeMatches", "")
// 		 || soap_body_end_out(soap)
// 		 || soap_envelope_end_out(soap))
// 			 return soap->error;
// 	}
// 	if (soap_end_count(soap))
// 		return soap->error;
// 	if (soap_connect(soap, soap_endpoint, soap_action)
// 	 || soap_envelope_begin_out(soap)
// 	 || soap_putheader(soap)
// 	 || soap_body_begin_out(soap)
// 	 || soap_put___wsdd__ProbeMatches(soap, &soap_tmp___wsdd__ProbeMatches, "-wsdd:ProbeMatches", "")
// 	 || soap_body_end_out(soap)
// 	 || soap_envelope_end_out(soap)
// 	 || soap_end_send(soap))
// 		return soap_closesock(soap);
// 	return SOAP_OK;
// }

static int g_nMessageNumber = 159;

static char g_uuid[64] = {0};
SOAP_FMAC5 int SOAP_FMAC6  __wsdd__Probe(struct soap* soap, struct wsdd__ProbeType *wsdd__Probe)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__wsdd__Probe----------");
#endif
    if (NULL == soap /*|| NULL == wsdd__Probe || NULL == wsdd__Probe->Scopes || NULL == wsdd__Probe->Types*/)
    {
#if ONVIF_LOG_SWITCH
        dlog_error("空指针");
#endif
        return SOAP_EOF;
    }

    if (soap->header == NULL)
    {
#if ONVIF_LOG_SWITCH
        dlog_error("soap head 为空");
#endif
        soap_header(soap);
        // return soap->error;
    }

    char ip_addr[32] = { 0 };
    char mac_addr[18] = { 0 };
    struct wsdd__ScopesType *pScopes = NULL;
    char str_tmp[256] = { 0 };

    OnvifDeviceInfo_t stInfo;
    onvif_get_device_info(&stInfo);
    char scopes_message[1024] = {0};
    snprintf(scopes_message, sizeof(scopes_message),
        "onvif://www.onvif.org/type/NetworkVideoTransmitter "
        "onvif://www.onvif.org/type/video_encoder "
        "onvif://www.onvif.org/type/ptz "
        "onvif://www.onvif.org/type/audio_encoder "
        "onvif://www.onvif.org/Profile/Streaming "
        "onvif://www.onvif.org/Profile/G "
        "onvif://www.onvif.org/Profile/T "
        "onvif://www.onvif.org/hardware/%s "
        "onvif://www.onvif.org/name/ITC "
        "onvif://www.onvif.org/location/city/Guangzhou "
        "onvif://www.onvif.org/location/country/China ",
        stInfo.achModel
    );

    getlocalip(ip_addr);
    strncpy(mac_addr, onvif_get_mac(), sizeof(mac_addr) - 1);
    mac_addr[sizeof(mac_addr) - 1] = '\0';

    // verify scropes
    if (wsdd__Probe && wsdd__Probe->Scopes && wsdd__Probe->Scopes->__item)
    {
        if (wsdd__Probe->Scopes->MatchBy)
        {
        }
        else
        {
        }
    }

    /* 执行回调处理 */
    // OnvifServerData_S *pstOnvifServer = (OnvifServerData_S*)soap->user;
    // if (NULL != pstOnvifServer)
    // {
    //     memset(pstOnvifServer->achTag,0,sizeof(pstOnvifServer->achTag));
    //     snprintf(pstOnvifServer->achTag,sizeof(pstOnvifServer->achTag),"Probe");
    //     pstOnvifServer->onvif_callback_function(soap);
    // }

    // response ProbeMatches
    struct wsdd__ProbeMatchesType   wsdd__ProbeMatches = {0};
    struct wsdd__ProbeMatchType     *pProbeMatchType = NULL;
    struct wsa__Relationship        *pWsa__RelatesTo = NULL;
    char                            *pMessageID = NULL;

    pProbeMatchType = (struct wsdd__ProbeMatchType *) soap_malloc(soap, sizeof(struct wsdd__ProbeMatchType));
    // soap_new_wsdd__ProbeMatchType(soap,-1);
    soap_default_wsdd__ProbeMatchType(soap, pProbeMatchType);
    int nHttport = ONVIF_HTTP_PORT;
    nHttport = onvif_get_httpPort();
    if (nHttport < 0)
    {
        nHttport = ONVIF_HTTP_PORT;
    }
    if(nHttport != ONVIF_HTTP_PORT)
    {
        sprintf(str_tmp, "http://%s:%d/onvif/device_service", ip_addr, nHttport);
    }
    else
    {
        sprintf(str_tmp, "http://%s/onvif/device_service", ip_addr);
    }
    //sprintf(str_tmp, "http://%s/onvif/device_service", ip_addr);
    //sprintf(str_tmp, "http://%s:%d/onvif/device_service", ip_addr, ONVIF_TCP_PORT);
    pProbeMatchType->XAddrs = soap_strdup(soap, str_tmp);
  
    if( wsdd__Probe && wsdd__Probe->Types && strlen(wsdd__Probe->Types) )
    {
        pProbeMatchType->Types  = soap_strdup(soap, wsdd__Probe->Types);
    }
    else
    {
        pProbeMatchType->Types  = soap_strdup(soap, "dn:NetworkVideoTransmitter tds:Device");
    }

    pProbeMatchType->MetadataVersion = 1;

    // Build Scopes Message
    pScopes = (struct wsdd__ScopesType *) soap_malloc(soap, sizeof(struct wsdd__ScopesType));
    soap_default_wsdd__ScopesType(soap, pScopes);
    // pScopes->MatchBy = soap_strdup(soap, "http://docs.oasis-open.org/ws-dd/ns/discovery/2009/01/rfc3986");
    pScopes->MatchBy = NULL;
    pScopes->__item = soap_strdup(soap, scopes_message);
    pProbeMatchType->Scopes = pScopes;

    if (!strlen(g_uuid))
        snprintf(g_uuid, 64, "%s", soap_wsa_rand_uuid(soap));
    pMessageID = g_uuid;
    snprintf(str_tmp, 256, "%s-%s", pMessageID, mac_addr);
    //sprintf(str_tmp, "%s", pMessageID);
    // dlog_debug("g_uuid: %s", pMessageID);
    pProbeMatchType->wsa__EndpointReference.Address = soap_strdup(soap, pMessageID);

    wsdd__ProbeMatches.__sizeProbeMatch = 1;
    wsdd__ProbeMatches.ProbeMatch       = pProbeMatchType;

    // Build SOAP Header
    pWsa__RelatesTo = (struct wsa__Relationship*)soap_malloc(soap, sizeof(struct wsa__Relationship));
    soap_default__wsa__RelatesTo(soap, pWsa__RelatesTo);
    if (soap->header->wsa__MessageID)
    {
        pWsa__RelatesTo->__item = soap_strdup(soap, soap->header->wsa__MessageID);
    }
    else
    {
        pWsa__RelatesTo->__item = soap_strdup(soap, "");
    }
    soap->header->wsa__RelatesTo = pWsa__RelatesTo;
    soap->header->wsa__Action      = soap_strdup(soap, "http://schemas.xmlsoap.org/ws/2005/04/discovery/ProbeMatches");
    soap->header->wsa__To          = soap_strdup(soap, "http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous");

    if(soap->header->wsdd__AppSequence == NULL)
    {
        soap->header->wsdd__AppSequence =  (struct wsdd__AppSequenceType*)soap_malloc(soap, sizeof(struct wsdd__AppSequenceType));
        soap_default_wsdd__AppSequenceType(soap, soap->header->wsdd__AppSequence);
    }

    g_nMessageNumber++;
    soap->header->wsdd__AppSequence->InstanceId = 1;
    soap->header->wsdd__AppSequence->MessageNumber = g_nMessageNumber;

    soap_send___wsdd__ProbeMatches(soap, "http://", NULL, &wsdd__ProbeMatches);

	return SOAP_OK;
}

/** Web service operation '__tds__GetServices' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetServices(struct soap* soap, struct _tds__GetServices *tds__GetServices, struct _tds__GetServicesResponse *tds__GetServicesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetServices----------");
#endif
    int size = 6;
    tds__GetServicesResponse->__sizeService = size;
    tds__GetServicesResponse->Service = (struct tds__Service *)soap_malloc(soap, sizeof(struct tds__Service) * size);
    memset(tds__GetServicesResponse->Service, 0, sizeof(struct tds__Service) * size);

    //get IP
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
    soap_wsse_delete_Security(soap);
    int nHttport = ONVIF_HTTP_PORT;
    nHttport = onvif_get_httpPort();
    if(nHttport < 0)
    {
        nHttport = ONVIF_HTTP_PORT;
    }
    int i = 0;
    //device
    tds__GetServicesResponse->Service[i].Namespace = (char *)soap_malloc(soap, sizeof(char)* 100);
    strcpy(tds__GetServicesResponse->Service[i].Namespace, "http://www.onvif.org/ver10/device/wsdl");
    tds__GetServicesResponse->Service[i].XAddr     = (char *)soap_malloc(soap, sizeof(char)* 100);
    if(nHttport != ONVIF_HTTP_PORT)
    {
        sprintf(tds__GetServicesResponse->Service[i].XAddr, "http://%s:%d/onvif/device_service", achIp, nHttport);
    }
    else
    {
        sprintf(tds__GetServicesResponse->Service[i].XAddr, "http://%s/onvif/device_service", achIp);
    }

    //sprintf(tds__GetServicesResponse->Service[i].XAddr, "http://%s/onvif/device_service", achIp);
    //sprintf(tds__GetServicesResponse->Service[i].XAddr, "http://%s:%d/onvif/device_service", achIp, ONVIF_TCP_PORT);
    // 给 Device Service 加上 Capabilities.Security
    tds__GetServicesResponse->Service[i].Capabilities = (struct _tds__Service_Capabilities *)soap_malloc(soap, sizeof(struct _tds__Service_Capabilities));
    memset(tds__GetServicesResponse->Service[i].Capabilities, 0, sizeof(struct _tds__Service_Capabilities));
    tds__GetServicesResponse->Service[i].Capabilities->__any = (char*)soap_malloc(soap, 512);
    if (ONVIF_DIGEST_MODE == onvif_get_auth_mode())
    {
         sprintf(tds__GetServicesResponse->Service[i].Capabilities->__any,
        "<tds:Security TLS1.0=\"true\" TLS1.1=\"true\" TLS1.2=\"true\" "
        "OnboardKeyGeneration=\"false\" AccessPolicyConfig=\"false\" "
        "DefaultAccessPolicy=\"true\" Dot1X=\"false\" RemoteUserHandling=\"false\" "
        "X.509Token=\"false\" SAMLToken=\"false\" KerberosToken=\"false\" "
        "UsernameToken=\"false\" HttpDigest=\"true\" RELToken=\"false\" "
        "SupportedEAPMethods=\"0\" MaxUsers=\"32\" MaxUserNameLength=\"32\" MaxPasswordLength=\"16\"/>");
    }
    else
    {
        sprintf(tds__GetServicesResponse->Service[i].Capabilities->__any,
        "<tds:Security TLS1.0=\"true\" TLS1.1=\"true\" TLS1.2=\"true\" "
        "OnboardKeyGeneration=\"false\" AccessPolicyConfig=\"false\" "
        "DefaultAccessPolicy=\"true\" Dot1X=\"false\" RemoteUserHandling=\"false\" "
        "X.509Token=\"false\" SAMLToken=\"false\" KerberosToken=\"false\" "
        "UsernameToken=\"true\" HttpDigest=\"false\" RELToken=\"false\" "
        "SupportedEAPMethods=\"0\" MaxUsers=\"32\" MaxUserNameLength=\"32\" MaxPasswordLength=\"16\"/>");
    }
   

    //media
    i = 1;
    tds__GetServicesResponse->Service[i].Namespace = (char *)soap_malloc(soap, sizeof(char)* 100);
    strcpy(tds__GetServicesResponse->Service[i].Namespace, "http://www.onvif.org/ver10/media/wsdl");
    tds__GetServicesResponse->Service[i].XAddr     = (char *)soap_malloc(soap, sizeof(char)* 100);
    if(nHttport != ONVIF_HTTP_PORT)
    {
        sprintf(tds__GetServicesResponse->Service[i].XAddr, "http://%s:%d/onvif/media_service", achIp, nHttport);
    }
    else
    {
        sprintf(tds__GetServicesResponse->Service[i].XAddr, "http://%s/onvif/media_service", achIp);
    }
    //sprintf(tds__GetServicesResponse->Service[i].XAddr, "http://%s/onvif/media_service", achIp);
    //sprintf(tds__GetServicesResponse->Service[i].XAddr, "http://%s:%d/onvif/media_service", achIp, ONVIF_TCP_PORT);

    //image
	i = 2;
	tds__GetServicesResponse->Service[i].Namespace = (char *)soap_malloc(soap, sizeof(char)* 100);
	strcpy(tds__GetServicesResponse->Service[i].Namespace, "http://www.onvif.org/ver20/imaging/wsdl");
	tds__GetServicesResponse->Service[i].XAddr = (char *)soap_malloc(soap, sizeof(char)* 100);
    if(nHttport != ONVIF_HTTP_PORT)
    {
        sprintf(tds__GetServicesResponse->Service[i].XAddr, "http://%s:%d/onvif/image_service", achIp, nHttport);
    }
    else
    {
        sprintf(tds__GetServicesResponse->Service[i].XAddr, "http://%s/onvif/image_service", achIp);
    }
    //sprintf(tds__GetServicesResponse->Service[i].XAddr, "http://%s/onvif/image_service", achIp);
	//sprintf(tds__GetServicesResponse->Service[i].XAddr, "http://%s:%d/onvif/image_service", achIp, ONVIF_TCP_PORT);
    tds__GetServicesResponse->Service[i].Capabilities = (struct _tds__Service_Capabilities *)soap_malloc(soap, sizeof(struct _tds__Service_Capabilities));
    memset(tds__GetServicesResponse->Service[i].Capabilities, 0, sizeof(struct _tds__Service_Capabilities));
    tds__GetServicesResponse->Service[i].Capabilities->__any = (char*)soap_malloc(soap, 512);
    sprintf(tds__GetServicesResponse->Service[i].Capabilities->__any,IMING_CAPABILITIES_MACRO);

    //media2
    i = 3;
    tds__GetServicesResponse->Service[i].Namespace = (char *)soap_malloc(soap, sizeof(char)* 100);
    strcpy(tds__GetServicesResponse->Service[i].Namespace, "http://www.onvif.org/ver20/media/wsdl");
    tds__GetServicesResponse->Service[i].XAddr     = (char *)soap_malloc(soap, sizeof(char)* 100);
    if(nHttport != ONVIF_HTTP_PORT)
    {
        sprintf(tds__GetServicesResponse->Service[i].XAddr, "http://%s:%d/onvif/media2_service", achIp, nHttport);
    }
    else
    {
        sprintf(tds__GetServicesResponse->Service[i].XAddr, "http://%s/onvif/media2_service", achIp);
    }
    //sprintf(tds__GetServicesResponse->Service[i].XAddr, "http://%s/onvif/media2_service", achIp);
    //sprintf(tds__GetServicesResponse->Service[i].XAddr, "http://%s:%d/onvif/media2_service", achIp, ONVIF_TCP_PORT);
    tds__GetServicesResponse->Service[i].Capabilities = (struct _tds__Service_Capabilities *)soap_malloc(soap, sizeof(struct _tds__Service_Capabilities));
    memset(tds__GetServicesResponse->Service[i].Capabilities, 0, sizeof(struct _tds__Service_Capabilities));
    tds__GetServicesResponse->Service[i].Capabilities->__any = (char*)soap_malloc(soap, 512);
    sprintf(tds__GetServicesResponse->Service[i].Capabilities->__any,ANALYTICS_CAPABILITIES_MACRO);

    //event
    i = 4;
    tds__GetServicesResponse->Service[i].Namespace = (char *)soap_malloc(soap, sizeof(char)* 100);
    strcpy(tds__GetServicesResponse->Service[i].Namespace, "http://www.onvif.org/ver10/events/wsdl");
    tds__GetServicesResponse->Service[i].XAddr     = (char *)soap_malloc(soap, sizeof(char)* 100);
     if(nHttport != ONVIF_HTTP_PORT)
    {
        sprintf(tds__GetServicesResponse->Service[i].XAddr, "http://%s:%d/onvif/event_service", achIp, nHttport);
    }
    else
    {
        sprintf(tds__GetServicesResponse->Service[i].XAddr, "http://%s/onvif/event_service", achIp);
    }
    //sprintf(tds__GetServicesResponse->Service[i].XAddr, "http://%s/onvif/event_service", achIp);
    //sprintf(tds__GetServicesResponse->Service[i].XAddr, "http://%s:%d/onvif/event_service", achIp, ONVIF_TCP_PORT);

    //analytics
    i = 5;
    tds__GetServicesResponse->Service[i].Namespace = (char *)soap_malloc(soap, sizeof(char)* 100);
    strcpy(tds__GetServicesResponse->Service[i].Namespace, ONVIF_ANALYTICS_URL);
    tds__GetServicesResponse->Service[i].XAddr     = (char *)soap_malloc(soap, sizeof(char)* 100);
    if(nHttport != ONVIF_HTTP_PORT)
    {
        sprintf(tds__GetServicesResponse->Service[i].XAddr, "http://%s:%d/onvif/Analytics", achIp, nHttport);
    }
    else
    {
        sprintf(tds__GetServicesResponse->Service[i].XAddr, "http://%s/onvif/Analytics", achIp);
    }
    //sprintf(tds__GetServicesResponse->Service[i].XAddr, "http://%s/onvif/Analytics", achIp);
    //sprintf(tds__GetServicesResponse->Service[i].XAddr, "http://%s:%d/onvif/Analytics", achIp, ONVIF_TCP_PORT);
    tds__GetServicesResponse->Service[i].Capabilities = (struct _tds__Service_Capabilities *)soap_malloc(soap, sizeof(struct _tds__Service_Capabilities));
    memset(tds__GetServicesResponse->Service[i].Capabilities, 0, sizeof(struct _tds__Service_Capabilities));
    tds__GetServicesResponse->Service[i].Capabilities->__any = (char*)soap_malloc(soap, 512);
    sprintf(tds__GetServicesResponse->Service[i].Capabilities->__any,ANALYTICS_CAPABILITIES_MACRO);
    //tds__GetServicesResponse->Service[i].Capabilities->__any = soap_strdup(soap, ANALYTICS_CAPABILITIES_MACRO);

    for(int i=0; i<tds__GetServicesResponse->__sizeService; i++) 
    {
        tds__GetServicesResponse->Service[i].Version = (struct tt__OnvifVersion *)soap_malloc(soap, sizeof(struct tt__OnvifVersion));
        tds__GetServicesResponse->Service[i].Version->Major = 1;
        tds__GetServicesResponse->Service[i].Version->Minor = 10;
        // tds__GetServicesResponse->Service[i].__size = 0;
        // tds__GetServicesResponse->Service[i].__any = NULL;
        // tds__GetServicesResponse->Service[i].__anyAttribute = NULL;
    }

    return SOAP_OK;
}

/** Web service operation '__tds__GetServiceCapabilities' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetServiceCapabilities(struct soap* soap, struct _tds__GetServiceCapabilities *tds__GetServiceCapabilities, struct _tds__GetServiceCapabilitiesResponse *tds__GetServiceCapabilitiesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetServiceCapabilities----------");
#endif
    if(soap == NULL)
    {
#if ONVIF_LOG_SWITCH
    dlog_error("空指针");
#endif
    }
    soap_wsse_delete_Security(soap);
    // 分配 Service_Capabilities 结构体内存
    tds__GetServiceCapabilitiesResponse->Capabilities =
        (struct tds__DeviceServiceCapabilities *)soap_malloc(soap, sizeof(struct tds__DeviceServiceCapabilities));
    if (!tds__GetServiceCapabilitiesResponse->Capabilities)
        return soap->error = SOAP_EOM; // 内存不足

    memset(tds__GetServiceCapabilitiesResponse->Capabilities, 0, sizeof(struct tds__DeviceServiceCapabilities));
    // -------------------------------
    // Security Capabilities
    // -------------------------------
    tds__GetServiceCapabilitiesResponse->Capabilities->Security =
        (struct tds__SecurityCapabilities *)soap_malloc(soap, sizeof(struct tds__SecurityCapabilities));
    memset(tds__GetServiceCapabilitiesResponse->Capabilities->Security, 0, sizeof(struct tds__SecurityCapabilities));
    // 根据实际情况设置
    tds__GetServiceCapabilitiesResponse->Capabilities->Security->TLS1_x002e1 = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->Security->TLS1_x002e1) = xsd__boolean__true_;
    tds__GetServiceCapabilitiesResponse->Capabilities->Security->TLS1_x002e2 = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->Security->TLS1_x002e2) = xsd__boolean__true_;
    tds__GetServiceCapabilitiesResponse->Capabilities->Security->OnboardKeyGeneration = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->Security->OnboardKeyGeneration) = xsd__boolean__false_;
    tds__GetServiceCapabilitiesResponse->Capabilities->Security->AccessPolicyConfig = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->Security->AccessPolicyConfig) = xsd__boolean__false_;
    tds__GetServiceCapabilitiesResponse->Capabilities->Security->DefaultAccessPolicy = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->Security->DefaultAccessPolicy) = xsd__boolean__false_;
    tds__GetServiceCapabilitiesResponse->Capabilities->Security->X_x002e509Token = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->Security->X_x002e509Token) = xsd__boolean__false_;
    tds__GetServiceCapabilitiesResponse->Capabilities->Security->SAMLToken = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->Security->SAMLToken) = xsd__boolean__false_;
    tds__GetServiceCapabilitiesResponse->Capabilities->Security->KerberosToken = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->Security->KerberosToken) = xsd__boolean__false_;
    tds__GetServiceCapabilitiesResponse->Capabilities->Security->RELToken = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->Security->RELToken) = xsd__boolean__false_;
    tds__GetServiceCapabilitiesResponse->Capabilities->Security->Dot1X = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->Security->Dot1X) = xsd__boolean__false_;
    tds__GetServiceCapabilitiesResponse->Capabilities->Security->RemoteUserHandling = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->Security->RemoteUserHandling) = xsd__boolean__false_;
    tds__GetServiceCapabilitiesResponse->Capabilities->Security->SupportedEAPMethods = (char *)soap_malloc(soap, sizeof(char));
    tds__GetServiceCapabilitiesResponse->Capabilities->Security->SupportedEAPMethods = soap_strdup(soap, "0");
    tds__GetServiceCapabilitiesResponse->Capabilities->Security->MaxUsers = (int *)soap_malloc(soap, sizeof(int));
    tds__GetServiceCapabilitiesResponse->Capabilities->Security->MaxUsers = soap_strdup(soap, "32");
    tds__GetServiceCapabilitiesResponse->Capabilities->Security->MaxUserNameLength = (int *)soap_malloc(soap, sizeof(int));
    tds__GetServiceCapabilitiesResponse->Capabilities->Security->MaxUserNameLength = soap_strdup(soap, "32");
    tds__GetServiceCapabilitiesResponse->Capabilities->Security->MaxPasswordLength = (int *)soap_malloc(soap, sizeof(int));
    tds__GetServiceCapabilitiesResponse->Capabilities->Security->MaxPasswordLength = soap_strdup(soap, "32");
    if (ONVIF_DIGEST_MODE == onvif_get_auth_mode())
    {
        tds__GetServiceCapabilitiesResponse->Capabilities->Security->UsernameToken = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
        *(tds__GetServiceCapabilitiesResponse->Capabilities->Security->UsernameToken) = xsd__boolean__false_;
        tds__GetServiceCapabilitiesResponse->Capabilities->Security->HttpDigest = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
        *(tds__GetServiceCapabilitiesResponse->Capabilities->Security->HttpDigest) = xsd__boolean__true_;
    }
    else
    {
        tds__GetServiceCapabilitiesResponse->Capabilities->Security->UsernameToken = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
        *(tds__GetServiceCapabilitiesResponse->Capabilities->Security->UsernameToken) = xsd__boolean__true_;
        tds__GetServiceCapabilitiesResponse->Capabilities->Security->HttpDigest = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
        *(tds__GetServiceCapabilitiesResponse->Capabilities->Security->HttpDigest) = xsd__boolean__false_;
    }
    // -------------------------------
    // System Capabilities
    // -------------------------------
    tds__GetServiceCapabilitiesResponse->Capabilities->System =
        (struct tds__SystemCapabilities *)soap_malloc(soap, sizeof(struct tds__SystemCapabilities));
    memset(tds__GetServiceCapabilitiesResponse->Capabilities->System, 0, sizeof(struct tds__SystemCapabilities));
    tds__GetServiceCapabilitiesResponse->Capabilities->System->DiscoveryResolve = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->System->DiscoveryResolve) = xsd__boolean__true_;
    tds__GetServiceCapabilitiesResponse->Capabilities->System->DiscoveryBye = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->System->DiscoveryBye) = xsd__boolean__true_;
    tds__GetServiceCapabilitiesResponse->Capabilities->System->RemoteDiscovery = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->System->RemoteDiscovery) = xsd__boolean__false_;
    tds__GetServiceCapabilitiesResponse->Capabilities->System->SystemBackup = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->System->SystemBackup) = xsd__boolean__false_;
    tds__GetServiceCapabilitiesResponse->Capabilities->System->FirmwareUpgrade = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->System->FirmwareUpgrade) = xsd__boolean__true_;
    // -------------------------------
    // Network Capabilities
    // -------------------------------
    tds__GetServiceCapabilitiesResponse->Capabilities->Network =
        (struct tds__NetworkCapabilities *)soap_malloc(soap, sizeof(struct tds__NetworkCapabilities));
    memset(tds__GetServiceCapabilitiesResponse->Capabilities->Network, 0, sizeof(struct tds__NetworkCapabilities));
    tds__GetServiceCapabilitiesResponse->Capabilities->Network->IPFilter = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->Network->IPFilter) = xsd__boolean__true_;
    tds__GetServiceCapabilitiesResponse->Capabilities->Network->ZeroConfiguration = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->Network->ZeroConfiguration) = xsd__boolean__true_;
    tds__GetServiceCapabilitiesResponse->Capabilities->Network->IPVersion6 = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->Network->IPVersion6) = xsd__boolean__true_;
    return SOAP_OK;
}

// 设备信息
/** Web service operation '__tds__GetDeviceInformation' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetDeviceInformation(struct soap* soap, struct _tds__GetDeviceInformation *tds__GetDeviceInformation, struct _tds__GetDeviceInformationResponse *tds__GetDeviceInformationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetDeviceInformation----------");
#endif
    if(soap == NULL)
    {
#if ONVIF_LOG_SWITCH
    dlog_error("空指针");
#endif
    }

    int nRet = onvif_authentication(soap);
    if(SOAP_OK != nRet)
    {
        return nRet;
    }

    tds__GetDeviceInformationResponse->Manufacturer = (char *)soap_malloc(soap, sizeof(char) * 16);
    tds__GetDeviceInformationResponse->Model = (char *)soap_malloc(soap, sizeof(char) * 16);
    tds__GetDeviceInformationResponse->FirmwareVersion = (char *)soap_malloc(soap, sizeof(char) * 16);
    tds__GetDeviceInformationResponse->SerialNumber = (char *)soap_malloc(soap, sizeof(char) * 16);
    tds__GetDeviceInformationResponse->HardwareId = (char *)soap_malloc(soap, sizeof(char) * 16);

    OnvifDeviceInfo_t stInfo;
    onvif_get_device_info(&stInfo);

    strcpy(tds__GetDeviceInformationResponse->Manufacturer, stInfo.achManufacturer);
    strcpy(tds__GetDeviceInformationResponse->Model, stInfo.achModel);
    strcpy(tds__GetDeviceInformationResponse->FirmwareVersion, stInfo.achFirmwareVersion);
    strcpy(tds__GetDeviceInformationResponse->SerialNumber, stInfo.achSerialNumber);
    strcpy(tds__GetDeviceInformationResponse->HardwareId, stInfo.achHardwareId);
    return SOAP_OK;
}

/** Web service operation '__tds__SetSystemDateAndTime' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetSystemDateAndTime(struct soap* soap, struct _tds__SetSystemDateAndTime *tds__SetSystemDateAndTime, struct _tds__SetSystemDateAndTimeResponse *tds__SetSystemDateAndTimeResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetSystemDateAndTime----------");
#endif
    if(tds__SetSystemDateAndTime == NULL)
    {
        dlog_error("tds__SetSystemDateAndTime is NULL");
        return SOAP_EOF;
    }

    int nRet = onvif_authentication(soap);
    if(SOAP_OK != nRet)
    {
        return nRet;
    }

    struct timeval stuTv = {0};
    struct tm stuTm = {.tm_sec = tds__SetSystemDateAndTime->UTCDateTime->Time->Second,
                       .tm_min = tds__SetSystemDateAndTime->UTCDateTime->Time->Minute,
                       .tm_hour = tds__SetSystemDateAndTime->UTCDateTime->Time->Hour,
                       .tm_mday = tds__SetSystemDateAndTime->UTCDateTime->Date->Day,
                       .tm_mon = tds__SetSystemDateAndTime->UTCDateTime->Date->Month - 1,
                       .tm_year = tds__SetSystemDateAndTime->UTCDateTime->Date->Year - 1900};

    stuTv.tv_sec = mktime(&stuTm) + 8 * 60 * 60;
    settimeofday(&stuTv, NULL);

    return SOAP_OK;
}

/** Web service operation '__tds__GetSystemDateAndTime' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetSystemDateAndTime(struct soap* soap, struct _tds__GetSystemDateAndTime *tds__GetSystemDateAndTime, struct _tds__GetSystemDateAndTimeResponse *tds__GetSystemDateAndTimeResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetSystemDateAndTime----------");
#endif
    if(!tds__GetSystemDateAndTimeResponse)
    {
        dlog_error("tds__GetSystemDateAndTimeResponse is NULL");
        return soap_sender_fault(soap, "tds__GetSystemDateAndTimeResponse is NULL", NULL);
    }
    int nRet = onvif_authentication(soap);
    if(SOAP_OK != nRet)
    {
        return nRet;
    }

    time_t nCurTime;
    struct tm stuTm = {0};

    time(&nCurTime);
    localtime_r(&nCurTime, &stuTm);

    tds__GetSystemDateAndTimeResponse->SystemDateAndTime = (struct tt__SystemDateTime*)soap_malloc(soap, sizeof(struct tt__SystemDateTime));
    memset(tds__GetSystemDateAndTimeResponse->SystemDateAndTime, 0, sizeof(struct tt__SystemDateTime));
    if(!tds__GetSystemDateAndTimeResponse->SystemDateAndTime)
    {
        dlog_error("Failed to allocate");
        return soap_receiver_fault(soap, "Memory allocation failed", NULL);
    }
    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->DateTimeType = tt__SetDateTimeType__NTP;
    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->DaylightSavings = xsd__boolean__false_;

    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->TimeZone = (struct tt__TimeZone*)soap_malloc(soap, sizeof(struct tt__TimeZone));
    memset(tds__GetSystemDateAndTimeResponse->SystemDateAndTime->TimeZone, 0, sizeof(struct tt__TimeZone));
    if(!tds__GetSystemDateAndTimeResponse->SystemDateAndTime->TimeZone)
    {
        dlog_error("Failed to allocate");
        return soap_receiver_fault(soap, "Memory allocation failed", NULL);
    }

    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->TimeZone->TZ = (char*)soap_malloc(soap, sizeof(char) * 16);
    memset(tds__GetSystemDateAndTimeResponse->SystemDateAndTime->TimeZone->TZ, 0, sizeof(char) * 16);
    if(!tds__GetSystemDateAndTimeResponse->SystemDateAndTime->TimeZone->TZ)
    {
        dlog_error("Failed to allocate");
        return soap_receiver_fault(soap, "Memory allocation failed", NULL);
    }
    strncpy(tds__GetSystemDateAndTimeResponse->SystemDateAndTime->TimeZone->TZ, "UTC+08:00", 16);

    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime = (struct tt__DateTime*)soap_malloc(soap, sizeof(struct tt__DateTime));
    memset(tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime, 0, sizeof(struct tt__DateTime));
    if(!tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime)
    {
        dlog_error("Failed to allocate");
        return soap_receiver_fault(soap, "Memory allocation failed", NULL);
    }

    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Time = (struct tt__Time*)soap_malloc(soap, sizeof(struct tt__Time));
    memset(tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Time, 0, sizeof(struct tt__Time));
    if(!tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Time)
    {
        dlog_error("Failed to allocate");
        return soap_receiver_fault(soap, "Memory allocation failed", NULL);
    }
    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Time->Hour = stuTm.tm_hour;
    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Time->Minute = stuTm.tm_min;
    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Time->Second = stuTm.tm_sec;

    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Date = (struct tt__Date*)soap_malloc(soap, sizeof(struct tt__Date));
    memset(tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Date, 0, sizeof(struct tt__Date));
    if(!tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Date)
    {
        dlog_error("Failed to allocate");
        return soap_receiver_fault(soap, "Memory allocation failed", NULL);
    }
    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Date->Year = stuTm.tm_year + 1900;
    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Date->Month = stuTm.tm_mon + 1;
    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Date->Day = stuTm.tm_mday;

    nCurTime -= 8 * 60 * 60;
    memset(&stuTm, 0, sizeof(struct tm));
    localtime_r(&nCurTime, &stuTm);
    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime = (struct tt__DateTime*)soap_malloc(soap, sizeof(struct tt__DateTime));
    memset(tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime, 0, sizeof(struct tt__DateTime));
    if(!tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime)
    {
        dlog_error("Failed to allocate");
        return soap_receiver_fault(soap, "Memory allocation failed", NULL);
    }
    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Time = (struct tt__Time*)soap_malloc(soap, sizeof(struct tt__Time));
    memset(tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Time, 0, sizeof(struct tt__Time));
    if(!tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Time)
    {
        dlog_error("Failed to allocate");
        return soap_receiver_fault(soap, "Memory allocation failed", NULL);
    }
    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Time->Hour = stuTm.tm_hour;
    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Time->Minute = stuTm.tm_min;
    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Time->Second = stuTm.tm_sec;

    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Date = (struct tt__Date*)soap_malloc(soap, sizeof(struct tt__Date));
    memset(tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Date, 0, sizeof(struct tt__Date));
    if(!tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Date)
    {
        dlog_error("Failed to allocate");
        return soap_receiver_fault(soap, "Memory allocation failed", NULL);
    }
    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Date->Year = stuTm.tm_year + 1900;
    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Date->Month = stuTm.tm_mon + 1;
    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Date->Day = stuTm.tm_mday;

    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->Extension = (struct tt__SystemDateTimeExtension*)soap_malloc(soap, sizeof(struct tt__SystemDateTimeExtension));
    memset(tds__GetSystemDateAndTimeResponse->SystemDateAndTime->Extension, 0, sizeof(struct tt__SystemDateTimeExtension));
    if(!tds__GetSystemDateAndTimeResponse->SystemDateAndTime->Extension)
    {
        dlog_error("Failed to allocate");
        return soap_receiver_fault(soap, "Memory allocation failed", NULL);
    }
    return SOAP_OK;
}

/** Web service operation '__tds__SetSystemFactoryDefault' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetSystemFactoryDefault(struct soap* soap, struct _tds__SetSystemFactoryDefault *tds__SetSystemFactoryDefault, struct _tds__SetSystemFactoryDefaultResponse *tds__SetSystemFactoryDefaultResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetSystemFactoryDefault----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__UpgradeSystemFirmware' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__UpgradeSystemFirmware(struct soap* soap, struct _tds__UpgradeSystemFirmware *tds__UpgradeSystemFirmware, struct _tds__UpgradeSystemFirmwareResponse *tds__UpgradeSystemFirmwareResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__UpgradeSystemFirmware----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SystemReboot' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SystemReboot(struct soap* soap, struct _tds__SystemReboot *tds__SystemReboot, struct _tds__SystemRebootResponse *tds__SystemRebootResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SystemReboot----------");
#endif
    int nRet = onvif_authentication(soap);
    if(SOAP_OK != nRet)
    {
        return nRet;
    }

    if(tds__SystemRebootResponse == NULL)
    {
        tds__SystemRebootResponse = soap_new__tds__SystemRebootResponse(soap,-1);
    }
    tds__SystemRebootResponse->Message = soap_strdup(soap, ONVIF_REBOOT_MESSAGE);
    
    onvif_reboot(soap);

    return SOAP_OK;
}

/** Web service operation '__tds__RestoreSystem' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__RestoreSystem(struct soap* soap, struct _tds__RestoreSystem *tds__RestoreSystem, struct _tds__RestoreSystemResponse *tds__RestoreSystemResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__RestoreSystem----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetSystemBackup' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetSystemBackup(struct soap* soap, struct _tds__GetSystemBackup *tds__GetSystemBackup, struct _tds__GetSystemBackupResponse *tds__GetSystemBackupResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetSystemBackup----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetSystemLog' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetSystemLog(struct soap* soap, struct _tds__GetSystemLog *tds__GetSystemLog, struct _tds__GetSystemLogResponse *tds__GetSystemLogResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetSystemLog----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetSystemSupportInformation' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetSystemSupportInformation(struct soap* soap, struct _tds__GetSystemSupportInformation *tds__GetSystemSupportInformation, struct _tds__GetSystemSupportInformationResponse *tds__GetSystemSupportInformationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetSystemSupportInformation----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetScopes' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetScopes(struct soap* soap, struct _tds__GetScopes *tds__GetScopes, struct _tds__GetScopesResponse *tds__GetScopesResponse)
{
    soap_wsse_delete_Security(soap);
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetScopes----------");
#endif
    tds__GetScopesResponse->__sizeScopes = 10;
    tds__GetScopesResponse->Scopes = (struct tt__Scope *)soap_malloc(soap, sizeof(struct tt__Scope) * tds__GetScopesResponse->__sizeScopes);
    memset(tds__GetScopesResponse->Scopes, 0, sizeof(struct tt__Scope) * tds__GetScopesResponse->__sizeScopes);

    tds__GetScopesResponse->Scopes[0].ScopeDef = tt__ScopeDefinition__Fixed;
    tds__GetScopesResponse->Scopes[1].ScopeDef = tt__ScopeDefinition__Configurable;
    tds__GetScopesResponse->Scopes[2].ScopeDef = tt__ScopeDefinition__Configurable;
    tds__GetScopesResponse->Scopes[3].ScopeDef = tt__ScopeDefinition__Configurable;
    tds__GetScopesResponse->Scopes[4].ScopeDef = tt__ScopeDefinition__Configurable;
    tds__GetScopesResponse->Scopes[5].ScopeDef = tt__ScopeDefinition__Configurable;
    tds__GetScopesResponse->Scopes[6].ScopeDef = tt__ScopeDefinition__Configurable;
    tds__GetScopesResponse->Scopes[7].ScopeDef = tt__ScopeDefinition__Fixed;
    tds__GetScopesResponse->Scopes[8].ScopeDef = tt__ScopeDefinition__Configurable;
    tds__GetScopesResponse->Scopes[9].ScopeDef = tt__ScopeDefinition__Configurable;

    for (int i = 0; tds__GetScopesResponse->__sizeScopes > i; i++)
    {
        tds__GetScopesResponse->Scopes[i].ScopeItem = (char *)soap_malloc(soap, sizeof(char) * 100);
        memset(tds__GetScopesResponse->Scopes[i].ScopeItem, '\0', sizeof(char) * 100);
    }

    OnvifDeviceInfo_t stInfo;
    onvif_get_device_info(&stInfo);
    char achScopeItem[128] = {0};
    snprintf(achScopeItem, sizeof(achScopeItem), "onvif://www.onvif.org/hardware/%s", stInfo.achModel);
    strcpy(tds__GetScopesResponse->Scopes[0].ScopeItem, "onvif://www.onvif.org/type/NetworkVideoTransmitter");
    strcpy(tds__GetScopesResponse->Scopes[1].ScopeItem, "onvif://www.onvif.org/type/video_encoder");
    strcpy(tds__GetScopesResponse->Scopes[2].ScopeItem, "onvif://www.onvif.org/type/audio_encoder");
    strcpy(tds__GetScopesResponse->Scopes[3].ScopeItem, "onvif://www.onvif.org/Profile/Streaming");
    strcpy(tds__GetScopesResponse->Scopes[4].ScopeItem, "onvif://www.onvif.org/Profile/G");
    strcpy(tds__GetScopesResponse->Scopes[5].ScopeItem, "onvif://www.onvif.org/Profile/T");
    strcpy(tds__GetScopesResponse->Scopes[6].ScopeItem, achScopeItem);
    strcpy(tds__GetScopesResponse->Scopes[7].ScopeItem, "onvif://www.onvif.org/name/ITC");
    strcpy(tds__GetScopesResponse->Scopes[8].ScopeItem, "onvif://www.onvif.org/location/city/Guangzhou");
    strcpy(tds__GetScopesResponse->Scopes[9].ScopeItem, "onvif://www.onvif.org/location/country/China");

    return SOAP_OK;
}

/** Web service operation '__tds__SetScopes' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetScopes(struct soap* soap, struct _tds__SetScopes *tds__SetScopes, struct _tds__SetScopesResponse *tds__SetScopesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetScopes----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__AddScopes' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__AddScopes(struct soap* soap, struct _tds__AddScopes *tds__AddScopes, struct _tds__AddScopesResponse *tds__AddScopesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__AddScopes----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__RemoveScopes' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__RemoveScopes(struct soap* soap, struct _tds__RemoveScopes *tds__RemoveScopes, struct _tds__RemoveScopesResponse *tds__RemoveScopesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__RemoveScopes----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetDiscoveryMode' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetDiscoveryMode(struct soap* soap, struct _tds__GetDiscoveryMode *tds__GetDiscoveryMode, struct _tds__GetDiscoveryModeResponse *tds__GetDiscoveryModeResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetDiscoveryMode----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetDiscoveryMode' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetDiscoveryMode(struct soap* soap, struct _tds__SetDiscoveryMode *tds__SetDiscoveryMode, struct _tds__SetDiscoveryModeResponse *tds__SetDiscoveryModeResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetDiscoveryMode----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetRemoteDiscoveryMode' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetRemoteDiscoveryMode(struct soap* soap, struct _tds__GetRemoteDiscoveryMode *tds__GetRemoteDiscoveryMode, struct _tds__GetRemoteDiscoveryModeResponse *tds__GetRemoteDiscoveryModeResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetRemoteDiscoveryMode----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetRemoteDiscoveryMode' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetRemoteDiscoveryMode(struct soap* soap, struct _tds__SetRemoteDiscoveryMode *tds__SetRemoteDiscoveryMode, struct _tds__SetRemoteDiscoveryModeResponse *tds__SetRemoteDiscoveryModeResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetDynamicDNS----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetDPAddresses' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetDPAddresses(struct soap* soap, struct _tds__GetDPAddresses *tds__GetDPAddresses, struct _tds__GetDPAddressesResponse *tds__GetDPAddressesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetDPAddresses----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetEndpointReference' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetEndpointReference(struct soap* soap, struct _tds__GetEndpointReference *tds__GetEndpointReference, struct _tds__GetEndpointReferenceResponse *tds__GetEndpointReferenceResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetEndpointReference----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetRemoteUser' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetRemoteUser(struct soap* soap, struct _tds__GetRemoteUser *tds__GetRemoteUser, struct _tds__GetRemoteUserResponse *tds__GetRemoteUserResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetRemoteUser----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetRemoteUser' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetRemoteUser(struct soap* soap, struct _tds__SetRemoteUser *tds__SetRemoteUser, struct _tds__SetRemoteUserResponse *tds__SetRemoteUserResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetRemoteUser----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetUsers' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetUsers(struct soap* soap, struct _tds__GetUsers *tds__GetUsers, struct _tds__GetUsersResponse *tds__GetUsersResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetUsers----------");
#endif
    tds__GetUsersResponse->__sizeUser = 1;
    tds__GetUsersResponse->User = (struct tt__User *)soap_malloc(soap, sizeof(struct tt__User));
    memset(tds__GetUsersResponse->User, 0, sizeof(struct tt__User));
    tds__GetUsersResponse->User->Username = (char *)soap_malloc(soap, sizeof(char) * 32);
    memset(tds__GetUsersResponse->User->Username, 0, sizeof(char) * 32);
    tds__GetUsersResponse->User->Password = (char *)soap_malloc(soap, sizeof(char) * 32);
    memset(tds__GetUsersResponse->User->Password, 0, sizeof(char) * 32);
    tds__GetUsersResponse->User->UserLevel = tt__UserLevel__User;

    char *pUser = onvif_get_user();
    if (NULL == pUser)
    {
        return SOAP_EOF;
    }
    char *pPassword = onvif_get_passwd(pUser);
    if (NULL == pUser)
    {
        return SOAP_EOF;
    }
    strcpy(tds__GetUsersResponse->User->Username, pUser);
    strcpy(tds__GetUsersResponse->User->Password, pPassword);

    return SOAP_OK;
}

/** Web service operation '__tds__CreateUsers' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__CreateUsers(struct soap* soap, struct _tds__CreateUsers *tds__CreateUsers, struct _tds__CreateUsersResponse *tds__CreateUsersResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__CreateUsers----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__DeleteUsers' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__DeleteUsers(struct soap* soap, struct _tds__DeleteUsers *tds__DeleteUsers, struct _tds__DeleteUsersResponse *tds__DeleteUsersResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__DeleteUsers----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetUser' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetUser(struct soap* soap, struct _tds__SetUser *tds__SetUser, struct _tds__SetUserResponse *tds__SetUserResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetUser----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetWsdlUrl' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetWsdlUrl(struct soap* soap, struct _tds__GetWsdlUrl *tds__GetWsdlUrl, struct _tds__GetWsdlUrlResponse *tds__GetWsdlUrlResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetWsdlUrl----------");
#endif
    return SOAP_OK;
}

// 获取设备功能
/** Web service operation '__tds__GetCapabilities' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetCapabilities(struct soap* soap, struct _tds__GetCapabilities *tds__GetCapabilities, struct _tds__GetCapabilitiesResponse *tds__GetCapabilitiesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetCapabilities----------");
#endif
    soap_wsse_delete_Security(soap);
    if(tds__GetCapabilities == NULL)
    {
        dlog_error("tds__GetCapabilities is NULL");
        return SOAP_EOF;
    }

    if (tds__GetCapabilities->Category == NULL) 
    {  
        dlog_error( "Category is NULL, use default 'All'");
        tds__GetCapabilities->Category = (enum tt__CapabilityCategory*)soap_malloc(soap, sizeof(enum tt__CapabilityCategory*));  
        *tds__GetCapabilities->Category = tt__CapabilityCategory__All;  
    }  

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
    int nHttport = ONVIF_HTTP_PORT;
    nHttport = onvif_get_httpPort();
    if(nHttport < 0)
    {
        nHttport = ONVIF_HTTP_PORT;
    }

    /* 能力<Capabilities> */
    tds__GetCapabilitiesResponse->Capabilities = (struct tt__Capabilities *)soap_malloc(soap, sizeof(struct tt__Capabilities));
    memset(tds__GetCapabilitiesResponse->Capabilities, 0, sizeof(struct tt__Capabilities));

    if (tds__GetCapabilities->Category[0] == tt__CapabilityCategory__Device ||
        tds__GetCapabilities->Category[0] == tt__CapabilityCategory__All) 
    {
        /* 设备能力<Capabilities><Device> */
        tds__GetCapabilitiesResponse->Capabilities->Device = (struct tt__DeviceCapabilities *)soap_malloc(soap, sizeof(struct tt__DeviceCapabilities));
        memset(tds__GetCapabilitiesResponse->Capabilities->Device, 0, sizeof(struct tt__DeviceCapabilities));
        /* 设备服务的URI地址 */
        tds__GetCapabilitiesResponse->Capabilities->Device->XAddr = (char *)soap_malloc(soap, sizeof(char) * 100 );
        memset(tds__GetCapabilitiesResponse->Capabilities->Device->XAddr, 0, sizeof(char) * 100);
        if(nHttport != ONVIF_HTTP_PORT)
        {
            sprintf(tds__GetCapabilitiesResponse->Capabilities->Device->XAddr, "http://%s:%d/onvif/device_service", achIp, nHttport);
        }
        else
        {
            sprintf(tds__GetCapabilitiesResponse->Capabilities->Device->XAddr, "http://%s/onvif/device_service", achIp);
        }
        //sprintf(tds__GetCapabilitiesResponse->Capabilities->Device->XAddr, "http://%s/onvif/device_service", achIp);
        //sprintf(tds__GetCapabilitiesResponse->Capabilities->Device->XAddr, "http://%s:%d/onvif/device_service", achIp, ONVIF_TCP_PORT);

        tds__GetCapabilitiesResponse->Capabilities->Analytics = (struct tt__AnalyticsDeviceCapabilities *)soap_malloc(soap, sizeof(struct tt__AnalyticsDeviceCapabilities));
        memset(tds__GetCapabilitiesResponse->Capabilities->Analytics, 0, sizeof(struct tt__AnalyticsDeviceCapabilities));
        /* 设备分析的URI地址 */
        tds__GetCapabilitiesResponse->Capabilities->Analytics->XAddr = (char *)soap_malloc(soap, sizeof(char) * 100 );
        memset(tds__GetCapabilitiesResponse->Capabilities->Analytics->XAddr, 0, sizeof(char) * 100);
        if(nHttport != ONVIF_HTTP_PORT)
        {
            sprintf(tds__GetCapabilitiesResponse->Capabilities->Analytics->XAddr, "http://%s:%d/onvif/Analytics", achIp, nHttport);
        }
        else
        {
            sprintf(tds__GetCapabilitiesResponse->Capabilities->Analytics->XAddr, "http://%s/onvif/Analytics", achIp);
        }
        //sprintf(tds__GetCapabilitiesResponse->Capabilities->Analytics->XAddr, "http://%s/onvif/Analytics", achIp);
        //sprintf(tds__GetCapabilitiesResponse->Capabilities->Analytics->XAddr, "http://%s:%d/onvif/Analytics", achIp, ONVIF_TCP_PORT);
        /* 支持规则和分析 */
        tds__GetCapabilitiesResponse->Capabilities->Analytics->RuleSupport = xsd__boolean__true_;
        tds__GetCapabilitiesResponse->Capabilities->Analytics->AnalyticsModuleSupport = xsd__boolean__true_;

        /* 设备网络能力<Capabilities><Device><Network> */
        tds__GetCapabilitiesResponse->Capabilities->Device->Network = (struct tt__NetworkCapabilities *)soap_malloc(soap, sizeof(struct tt__NetworkCapabilities ));
        memset(tds__GetCapabilitiesResponse->Capabilities->Device->Network, 0, sizeof(struct tt__NetworkCapabilities ));
        /* IP过滤 */
        tds__GetCapabilitiesResponse->Capabilities->Device->Network->IPFilter = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
        *(tds__GetCapabilitiesResponse->Capabilities->Device->Network->IPFilter) = xsd__boolean__true_;
        /* 零配置网络 */
        tds__GetCapabilitiesResponse->Capabilities->Device->Network->ZeroConfiguration= (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
        *(tds__GetCapabilitiesResponse->Capabilities->Device->Network->ZeroConfiguration) = xsd__boolean__false_;
        /* IPv6 */
        tds__GetCapabilitiesResponse->Capabilities->Device->Network->IPVersion6 = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
        *(tds__GetCapabilitiesResponse->Capabilities->Device->Network->IPVersion6) = xsd__boolean__true_;
        /* 动态DNS */
        tds__GetCapabilitiesResponse->Capabilities->Device->Network->DynDNS = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
        *(tds__GetCapabilitiesResponse->Capabilities->Device->Network->DynDNS) = xsd__boolean__true_;

        /* 设备网络能力扩展<Capabilities><Device><Network><Extension> */
        tds__GetCapabilitiesResponse->Capabilities->Device->Network->Extension = (struct tt__NetworkCapabilitiesExtension *)soap_malloc(soap, sizeof(struct tt__NetworkCapabilitiesExtension));
        memset(tds__GetCapabilitiesResponse->Capabilities->Device->Network->Extension, 0, sizeof(struct tt__NetworkCapabilitiesExtension ));
        /* Wi-Fi配置 */
        tds__GetCapabilitiesResponse->Capabilities->Device->Network->Extension->Dot11Configuration = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
        *(tds__GetCapabilitiesResponse->Capabilities->Device->Network->Extension->Dot11Configuration) = xsd__boolean__false_;
        /* 更多扩展NetworkCapabilitiesExtension2 */

        /* 设备系统能力<Capabilities><Device><System> */
        tds__GetCapabilitiesResponse->Capabilities->Device->System = (struct tt__SystemCapabilities *)soap_malloc(soap, sizeof(struct tt__SystemCapabilities));
        memset( tds__GetCapabilitiesResponse->Capabilities->Device->System, 0, sizeof(struct tt__SystemCapabilities));
        /* WS-Discovery的Resolve请求 */
        tds__GetCapabilitiesResponse->Capabilities->Device->System->DiscoveryResolve = xsd__boolean__true_;
        /* WS-Discovery的Bye请求 */
        tds__GetCapabilitiesResponse->Capabilities->Device->System->DiscoveryBye     = xsd__boolean__true_;
        /* 远程发现 */
        tds__GetCapabilitiesResponse->Capabilities->Device->System->RemoteDiscovery  = xsd__boolean__true_;
        /* 系统备份 */
        tds__GetCapabilitiesResponse->Capabilities->Device->System->SystemBackup     = xsd__boolean__true_;
        /* 系统日志 */
        tds__GetCapabilitiesResponse->Capabilities->Device->System->SystemLogging    = xsd__boolean__true_;
        /* 固件升级 */
        tds__GetCapabilitiesResponse->Capabilities->Device->System->FirmwareUpgrade  = xsd__boolean__true_;
        /* ONVIF版本 */
        tds__GetCapabilitiesResponse->Capabilities->Device->System->__sizeSupportedVersions = 1;
        tds__GetCapabilitiesResponse->Capabilities->Device->System->SupportedVersions = (struct tt__OnvifVersion *)soap_malloc(soap, sizeof(struct tt__OnvifVersion));
        /* ONVIF主版本号 */
        tds__GetCapabilitiesResponse->Capabilities->Device->System->SupportedVersions->Major = 1;
        /* ONVIF次版本号 */
        tds__GetCapabilitiesResponse->Capabilities->Device->System->SupportedVersions->Minor = 10;

        /* 设备系统能力扩展<Capabilities><Device><System><Extension> */
        tds__GetCapabilitiesResponse->Capabilities->Device->System->Extension = (struct tt__SystemCapabilitiesExtension *)soap_malloc(soap, sizeof(struct tt__SystemCapabilitiesExtension));
        memset( tds__GetCapabilitiesResponse->Capabilities->Device->System->Extension, 0, sizeof(struct tt__SystemCapabilitiesExtension));
        /* HTTP固件升级 */
        tds__GetCapabilitiesResponse->Capabilities->Device->System->Extension->HttpFirmwareUpgrade = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
        *(tds__GetCapabilitiesResponse->Capabilities->Device->System->Extension->HttpFirmwareUpgrade) = xsd__boolean__true_;
        /* HTTP系统备份 */
        tds__GetCapabilitiesResponse->Capabilities->Device->System->Extension->HttpSystemBackup = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
        *(tds__GetCapabilitiesResponse->Capabilities->Device->System->Extension->HttpSystemBackup) = xsd__boolean__true_;
        /* HTTP系统日志 */
        tds__GetCapabilitiesResponse->Capabilities->Device->System->Extension->HttpSystemLogging = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
        *(tds__GetCapabilitiesResponse->Capabilities->Device->System->Extension->HttpSystemLogging) = xsd__boolean__true_;
        /* HTTP设备支持信息 */
        tds__GetCapabilitiesResponse->Capabilities->Device->System->Extension->HttpSupportInformation = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
        *(tds__GetCapabilitiesResponse->Capabilities->Device->System->Extension->HttpSupportInformation) = xsd__boolean__true_;

        /* 设备输入输出能力<Capabilities><Device><IO> */
        // tds__GetCapabilitiesResponse->Capabilities->Device->IO = (struct tt__IOCapabilities *)soap_malloc(soap, sizeof(struct tt__IOCapabilities));
        // memset( tds__GetCapabilitiesResponse->Capabilities->Device->IO, 0, sizeof(struct tt__IOCapabilities));
        // /* 输入接口数量 */
        // tds__GetCapabilitiesResponse->Capabilities->Device->IO->InputConnectors = (int *)soap_malloc(soap, sizeof(int));
        // *(tds__GetCapabilitiesResponse->Capabilities->Device->IO->InputConnectors) = xsd__boolean__true_;

        //<Device><Security>
        tds__GetCapabilitiesResponse->Capabilities->Device->Security = (struct tt__SecurityCapabilities *)soap_malloc(soap, sizeof(struct tt__SecurityCapabilities));
        memset(tds__GetCapabilitiesResponse->Capabilities->Device->Security, 0, sizeof(struct tt__SecurityCapabilities));
        tds__GetCapabilitiesResponse->Capabilities->Device->Security->TLS1_x002e1          = xsd__boolean__true_;
        tds__GetCapabilitiesResponse->Capabilities->Device->Security->TLS1_x002e2          = xsd__boolean__true_;
        tds__GetCapabilitiesResponse->Capabilities->Device->Security->OnboardKeyGeneration = xsd__boolean__false_;
        tds__GetCapabilitiesResponse->Capabilities->Device->Security->AccessPolicyConfig   = xsd__boolean__false_;
        tds__GetCapabilitiesResponse->Capabilities->Device->Security->X_x002e509Token      = xsd__boolean__false_;
        tds__GetCapabilitiesResponse->Capabilities->Device->Security->SAMLToken            = xsd__boolean__false_;
        tds__GetCapabilitiesResponse->Capabilities->Device->Security->KerberosToken        = xsd__boolean__false_;
        tds__GetCapabilitiesResponse->Capabilities->Device->Security->RELToken             = xsd__boolean__false_;
        tds__GetCapabilitiesResponse->Capabilities->Device->Security->Extension = (struct tt__SecurityCapabilitiesExtension *)soap_malloc(soap, sizeof(struct tt__SecurityCapabilitiesExtension));
        memset(tds__GetCapabilitiesResponse->Capabilities->Device->Security->Extension, 0, sizeof(struct tt__SecurityCapabilitiesExtension));
        tds__GetCapabilitiesResponse->Capabilities->Device->Security->Extension->Extension =
                (struct tt__SecurityCapabilitiesExtension2 *)soap_malloc(soap, sizeof(struct tt__SecurityCapabilitiesExtension2));
        memset(tds__GetCapabilitiesResponse->Capabilities->Device->Security->Extension->Extension, 0, sizeof(struct tt__SecurityCapabilitiesExtension2));
        tds__GetCapabilitiesResponse->Capabilities->Device->Security->Extension->Extension->Dot1X = xsd__boolean__false_;
        tds__GetCapabilitiesResponse->Capabilities->Device->Security->Extension->Extension->RemoteUserHandling = xsd__boolean__false_;
    }

    //event
    if (tds__GetCapabilities->Category[0] == tt__CapabilityCategory__Events ||
        tds__GetCapabilities->Category[0] == tt__CapabilityCategory__All) 
    {
        tds__GetCapabilitiesResponse->Capabilities->Events = (struct tt__EventCapabilities *)soap_malloc(soap, sizeof(struct tt__EventCapabilities));
        memset(tds__GetCapabilitiesResponse->Capabilities->Events, 0, sizeof(struct tt__EventCapabilities));
        tds__GetCapabilitiesResponse->Capabilities->Events->XAddr = (char *)soap_malloc(soap, sizeof(char) * 100 );
        memset(tds__GetCapabilitiesResponse->Capabilities->Events->XAddr, '\0', sizeof(char) * 100);
         if(nHttport != ONVIF_HTTP_PORT)
        {
            sprintf(tds__GetCapabilitiesResponse->Capabilities->Events->XAddr, "http://%s:%d/onvif/event_service", achIp, nHttport);
        }
        else
        {
            sprintf(tds__GetCapabilitiesResponse->Capabilities->Events->XAddr, "http://%s/onvif/event_service", achIp);
        }
        //sprintf(tds__GetCapabilitiesResponse->Capabilities->Events->XAddr, "http://%s/onvif/event_service", achIp);
        //sprintf(tds__GetCapabilitiesResponse->Capabilities->Events->XAddr, "http://%s:%d/onvif/event_service", achIp, ONVIF_TCP_PORT);
        tds__GetCapabilitiesResponse->Capabilities->Events->WSSubscriptionPolicySupport = xsd__boolean__true_;
        tds__GetCapabilitiesResponse->Capabilities->Events->WSPullPointSupport = xsd__boolean__true_;
        tds__GetCapabilitiesResponse->Capabilities->Events->WSPausableSubscriptionManagerInterfaceSupport = xsd__boolean__false_;
    }

    //image
    if (tds__GetCapabilities->Category[0] == tt__CapabilityCategory__Imaging ||
        tds__GetCapabilities->Category[0] == tt__CapabilityCategory__All) 
    {
        tds__GetCapabilitiesResponse->Capabilities->Imaging = (struct tt__ImagingCapabilities *)soap_malloc(soap, sizeof(struct tt__ImagingCapabilities));
        memset(tds__GetCapabilitiesResponse->Capabilities->Imaging, 0, sizeof(struct tt__ImagingCapabilities));
        tds__GetCapabilitiesResponse->Capabilities->Imaging->XAddr = (char *)soap_malloc(soap, sizeof(char) * 100 );
        memset(tds__GetCapabilitiesResponse->Capabilities->Imaging->XAddr, 0, sizeof(char) * 100);
        if(nHttport != ONVIF_HTTP_PORT)
        {
            sprintf(tds__GetCapabilitiesResponse->Capabilities->Imaging->XAddr, "http://%s:%d/onvif/image_service", achIp, nHttport);
        }
        else
        {
            sprintf(tds__GetCapabilitiesResponse->Capabilities->Imaging->XAddr, "http://%s/onvif/image_service", achIp);
        }
        //sprintf(tds__GetCapabilitiesResponse->Capabilities->Imaging->XAddr, "http://%s/onvif/image_service", achIp);
        //sprintf(tds__GetCapabilitiesResponse->Capabilities->Imaging->XAddr, "http://%s:%d/onvif/image_service", achIp, ONVIF_TCP_PORT)
    }
    
    //Media
    if (tds__GetCapabilities->Category[0] == tt__CapabilityCategory__Media ||
        tds__GetCapabilities->Category[0] == tt__CapabilityCategory__All) 
    {
        tds__GetCapabilitiesResponse->Capabilities->Media = (struct tt__MediaCapabilities *)soap_malloc(soap, sizeof(struct tt__MediaCapabilities));
        memset(tds__GetCapabilitiesResponse->Capabilities->Media, 0, sizeof(struct tt__MediaCapabilities));
        tds__GetCapabilitiesResponse->Capabilities->Media->XAddr = (char *)soap_malloc(soap, sizeof(char) * 100 );
        memset(tds__GetCapabilitiesResponse->Capabilities->Media->XAddr, '\0', sizeof(char) * 100);
         if(nHttport != ONVIF_HTTP_PORT)
        {
            sprintf(tds__GetCapabilitiesResponse->Capabilities->Media->XAddr, "http://%s:%d/onvif/media_service", achIp, nHttport);
        }
        else
        {
            sprintf(tds__GetCapabilitiesResponse->Capabilities->Media->XAddr, "http://%s/onvif/media_service", achIp);
        }
        //sprintf(tds__GetCapabilitiesResponse->Capabilities->Media->XAddr, "http://%s/onvif/media_service", achIp);
        //sprintf(tds__GetCapabilitiesResponse->Capabilities->Media->XAddr, "http://%s:%d/onvif/media_service", achIp, ONVIF_TCP_PORT);
        //<Media><StreamingCapabilities>
        tds__GetCapabilitiesResponse->Capabilities->Media->StreamingCapabilities = (struct tt__RealTimeStreamingCapabilities *)soap_malloc(soap, sizeof(struct tt__RealTimeStreamingCapabilities));
        memset(tds__GetCapabilitiesResponse->Capabilities->Media->StreamingCapabilities, 0, sizeof(struct tt__RealTimeStreamingCapabilities));
        tds__GetCapabilitiesResponse->Capabilities->Media->StreamingCapabilities->RTPMulticast = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
        *(tds__GetCapabilitiesResponse->Capabilities->Media->StreamingCapabilities->RTPMulticast) = xsd__boolean__false_;
        tds__GetCapabilitiesResponse->Capabilities->Media->StreamingCapabilities->RTP_USCORERTSP_USCORETCP = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
        *(tds__GetCapabilitiesResponse->Capabilities->Media->StreamingCapabilities->RTP_USCORERTSP_USCORETCP) = xsd__boolean__true_;
        tds__GetCapabilitiesResponse->Capabilities->Media->StreamingCapabilities->RTP_USCORETCP = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
        *(tds__GetCapabilitiesResponse->Capabilities->Media->StreamingCapabilities->RTP_USCORETCP) = xsd__boolean__true_;
    }

    //PTZ
    if (tds__GetCapabilities->Category[0] == tt__CapabilityCategory__All ||
        tds__GetCapabilities->Category[0] == tt__CapabilityCategory__PTZ) 
    {
        tds__GetCapabilitiesResponse->Capabilities->PTZ = (struct tt__PTZCapabilities *)soap_malloc(soap, sizeof(struct tt__PTZCapabilities));
        memset(tds__GetCapabilitiesResponse->Capabilities->PTZ, 0, sizeof(struct tt__PTZCapabilities));
        tds__GetCapabilitiesResponse->Capabilities->PTZ->XAddr = (char *)soap_malloc(soap, sizeof(char) * 100 );
        memset(tds__GetCapabilitiesResponse->Capabilities->PTZ->XAddr, '\0', sizeof(char) * 100);
         if(nHttport != ONVIF_HTTP_PORT)
        {
            sprintf(tds__GetCapabilitiesResponse->Capabilities->PTZ->XAddr, "http://%s:%d/onvif/ptz", achIp, nHttport);
        }
        else
        {
            sprintf(tds__GetCapabilitiesResponse->Capabilities->PTZ->XAddr, "http://%s/onvif/ptz", achIp);
        }
        //sprintf(tds__GetCapabilitiesResponse->Capabilities->PTZ->XAddr, "http://%s/onvif/ptz", achIp);
        //sprintf(tds__GetCapabilitiesResponse->Capabilities->PTZ->XAddr, "http://%s:%d/onvif/ptz", achIp, ONVIF_TCP_PORT);
    }
    return SOAP_OK;
}

/** Web service operation '__tds__SetDPAddresses' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetDPAddresses(struct soap* soap, struct _tds__SetDPAddresses *tds__SetDPAddresses, struct _tds__SetDPAddressesResponse *tds__SetDPAddressesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetDPAddresses----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetHostname' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetHostname(struct soap* soap, struct _tds__GetHostname *tds__GetHostname, struct _tds__GetHostnameResponse *tds__GetHostnameResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetHostname----------");
#endif
    tds__GetHostnameResponse->HostnameInformation = (struct tt__HostnameInformation*)soap_malloc(soap, sizeof(struct tt__HostnameInformation));
    memset(tds__GetHostnameResponse->HostnameInformation, 0, sizeof(struct tt__HostnameInformation));
    tds__GetHostnameResponse->HostnameInformation->FromDHCP = xsd__boolean__false_;
    tds__GetHostnameResponse->HostnameInformation->Name = (char *)soap_malloc(soap, sizeof(char) * 16);
    memset(tds__GetHostnameResponse->HostnameInformation->Name, 0, 16); 
    strncpy(tds__GetHostnameResponse->HostnameInformation->Name, AUTHREALM, sizeof(AUTHREALM));
    return SOAP_OK;
}

/** Web service operation '__tds__SetHostname' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetHostname(struct soap* soap, struct _tds__SetHostname *tds__SetHostname, struct _tds__SetHostnameResponse *tds__SetHostnameResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetHostname----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetHostnameFromDHCP' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetHostnameFromDHCP(struct soap* soap, struct _tds__SetHostnameFromDHCP *tds__SetHostnameFromDHCP, struct _tds__SetHostnameFromDHCPResponse *tds__SetHostnameFromDHCPResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetHostnameFromDHCP----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetDNS' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetDNS(struct soap* soap, struct _tds__GetDNS *tds__GetDNS, struct _tds__GetDNSResponse *tds__GetDNSResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetDNS----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetDNS' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetDNS(struct soap* soap, struct _tds__SetDNS *tds__SetDNS, struct _tds__SetDNSResponse *tds__SetDNSResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetDNS----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetNTP' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetNTP(struct soap* soap, struct _tds__GetNTP *tds__GetNTP, struct _tds__GetNTPResponse *tds__GetNTPResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetNTP----------");
#endif
    char ntpServerAddress[64] = {0};
    onvif_get_ntpServerAddress(ntpServerAddress, sizeof(ntpServerAddress));

    tds__GetNTPResponse->NTPInformation = (struct tt__NTPInformation*)soap_malloc(soap, sizeof(struct tt__NTPInformation)); 
    if(tds__GetNTPResponse->NTPInformation == NULL)
    {
        dlog_error("tds__GetNTPResponse->NTPInformation is NULL");
        return SOAP_EOF;
    }

    int nRet = onvif_authentication(soap);
    if(SOAP_OK != nRet)
    {
        return nRet;
    }
    tds__GetNTPResponse->NTPInformation->FromDHCP = xsd__boolean__true_;

    tds__GetNTPResponse->NTPInformation->NTPFromDHCP = (struct tt__NetworkHost*)soap_malloc(soap, sizeof(struct tt__NetworkHost));
    if(tds__GetNTPResponse->NTPInformation->NTPFromDHCP == NULL)
    {
        dlog_error("tds__GetNTPResponse->NTPInformation->NTPFromDHCP is NULL");
        return SOAP_EOF;
    }

    tds__GetNTPResponse->NTPInformation->NTPFromDHCP->Type = tt__NetworkHostType__IPv4;

    tds__GetNTPResponse->NTPInformation->NTPFromDHCP->IPv4Address = (char*)soap_malloc(soap, sizeof(char) * strlen(ntpServerAddress)+1);
    if(tds__GetNTPResponse->NTPInformation->NTPFromDHCP->IPv4Address == NULL)
    {
        dlog_error("tds__GetNTPResponse->NTPInformation->NTPFromDHCP->IPv4Address is NULL");
        return SOAP_EOF;
    }
    strncpy(tds__GetNTPResponse->NTPInformation->NTPFromDHCP->IPv4Address, ntpServerAddress, strlen(ntpServerAddress));

    return SOAP_OK;
}

/** Web service operation '__tds__SetNTP' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetNTP(struct soap* soap, struct _tds__SetNTP *tds__SetNTP, struct _tds__SetNTPResponse *tds__SetNTPResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetNTP----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetDynamicDNS' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetDynamicDNS(struct soap* soap, struct _tds__GetDynamicDNS *tds__GetDynamicDNS, struct _tds__GetDynamicDNSResponse *tds__GetDynamicDNSResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetDynamicDNS----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetDynamicDNS' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetDynamicDNS(struct soap* soap, struct _tds__SetDynamicDNS *tds__SetDynamicDNS, struct _tds__SetDynamicDNSResponse *tds__SetDynamicDNSResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetDynamicDNS----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetNetworkInterfaces' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetNetworkInterfaces(struct soap* soap, struct _tds__GetNetworkInterfaces *tds__GetNetworkInterfaces, struct _tds__GetNetworkInterfacesResponse *tds__GetNetworkInterfacesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetNetworkInterfaces----------");
#endif
    int nRet = onvif_authentication(soap);
    if(SOAP_OK != nRet)
    {
        return nRet;
    }

    if(tds__GetNetworkInterfacesResponse == NULL)
    {
        dlog_error("tds__GetNetworkInterfacesResponse NULL");
        return SOAP_FAULT;
    }

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

    ONvifNetworkInfo_S stOnvifInfo;
    onvif_get_ipInfo(&stOnvifInfo);

    tds__GetNetworkInterfacesResponse->__sizeNetworkInterfaces = 1;
    tds__GetNetworkInterfacesResponse->NetworkInterfaces = soap_new_tt__NetworkInterface(soap,tds__GetNetworkInterfacesResponse->__sizeNetworkInterfaces);

    //<NetworkInterfaces>
    tds__GetNetworkInterfacesResponse->NetworkInterfaces->token = soap_strdup(soap,NETWORKINTERFACE_TOKEN);
    tds__GetNetworkInterfacesResponse->NetworkInterfaces->Enabled = xsd__boolean__true_;
    //<NetworkInterfaces><Info>
    tds__GetNetworkInterfacesResponse->NetworkInterfaces->Info = soap_new_tt__NetworkInterfaceInfo(soap,-1);
    tds__GetNetworkInterfacesResponse->NetworkInterfaces->Info->Name = soap_strdup(soap,NETWORKINTERFACE_TOKEN);
    tds__GetNetworkInterfacesResponse->NetworkInterfaces->Info->HwAddress = soap_strdup(soap,stOnvifInfo.achMAC);

    tds__GetNetworkInterfacesResponse->NetworkInterfaces->Info->MTU = (int *)soap_malloc(soap, sizeof(int));
    *(tds__GetNetworkInterfacesResponse->NetworkInterfaces->Info->MTU) = stOnvifInfo.nMtu;

    //<NetworkInterfaces><Link>
    tds__GetNetworkInterfacesResponse->NetworkInterfaces->Link = soap_new_tt__NetworkInterfaceLink(soap,-1);
    //<NetworkInterfaces><Link><AdminSettings>
    tds__GetNetworkInterfacesResponse->NetworkInterfaces->Link->AdminSettings = soap_new_tt__NetworkInterfaceConnectionSetting(soap,-1);
    tds__GetNetworkInterfacesResponse->NetworkInterfaces->Link->AdminSettings->AutoNegotiation = xsd__boolean__false_;
    tds__GetNetworkInterfacesResponse->NetworkInterfaces->Link->AdminSettings->Speed = 100;
    tds__GetNetworkInterfacesResponse->NetworkInterfaces->Link->AdminSettings->Duplex = tt__Duplex__Full;

    //<NetworkInterfaces><Link><OperSettings>
    tds__GetNetworkInterfacesResponse->NetworkInterfaces->Link->OperSettings = soap_new_tt__NetworkInterfaceConnectionSetting(soap,-1);
    tds__GetNetworkInterfacesResponse->NetworkInterfaces->Link->OperSettings->AutoNegotiation = xsd__boolean__false_;
    tds__GetNetworkInterfacesResponse->NetworkInterfaces->Link->OperSettings->Speed = 100;
    tds__GetNetworkInterfacesResponse->NetworkInterfaces->Link->OperSettings->Duplex = tt__Duplex__Full;
    tds__GetNetworkInterfacesResponse->NetworkInterfaces->Link->InterfaceType = 6;

    //<NetworkInterfaces><IPv4>
    tds__GetNetworkInterfacesResponse->NetworkInterfaces->IPv4 = soap_new_tt__IPv4NetworkInterface(soap,-1);
    tds__GetNetworkInterfacesResponse->NetworkInterfaces->IPv4->Enabled = xsd__boolean__true_;
    tds__GetNetworkInterfacesResponse->NetworkInterfaces->IPv4->Config = soap_new_tt__IPv4Configuration(soap,-1);
    tds__GetNetworkInterfacesResponse->NetworkInterfaces->IPv4->Config->__sizeManual = 1;
    tds__GetNetworkInterfacesResponse->NetworkInterfaces->IPv4->Config->Manual = soap_new_tt__PrefixedIPv4Address(soap,tds__GetNetworkInterfacesResponse->NetworkInterfaces->IPv4->Config->__sizeManual);
    tds__GetNetworkInterfacesResponse->NetworkInterfaces->IPv4->Config->Manual->Address = soap_strdup(soap,achIp);
    
    tds__GetNetworkInterfacesResponse->NetworkInterfaces->IPv4->Config->Manual->PrefixLength = stOnvifInfo.nPrefixlen;
    tds__GetNetworkInterfacesResponse->NetworkInterfaces->IPv4->Config->DHCP = stOnvifInfo.bDhcp;

    return SOAP_OK;
}

// 修改设置IP
/** Web service operation '__tds__SetNetworkInterfaces' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetNetworkInterfaces(struct soap* soap, struct _tds__SetNetworkInterfaces *tds__SetNetworkInterfaces, struct _tds__SetNetworkInterfacesResponse *tds__SetNetworkInterfacesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetNetworkInterfaces----------");
#endif
    int nRet = onvif_authentication(soap);
    if(SOAP_OK != nRet)
    {
        return nRet;
    }

    if (!tds__SetNetworkInterfaces || !tds__SetNetworkInterfaces->InterfaceToken|| !tds__SetNetworkInterfaces->NetworkInterface || !tds__SetNetworkInterfaces->NetworkInterface->IPv4 || !tds__SetNetworkInterfaces->NetworkInterface->IPv4->Enabled)  //&&tds__SetNetworkInterfaces->NetworkInterface->IPv4->DHCP)
    {
        dlog_error("SetNetworkInterfaces NULL ");
        return SOAP_EOF;
    }

    if(strcmp(tds__SetNetworkInterfaces->InterfaceToken,"eth0") != 0)
    {
        dlog_error("无效Token[%s]",tds__SetNetworkInterfaces->InterfaceToken);
        return SOAP_EOF;
    }
    ONvifNetworkInfo_S stOnvifInfo;
   
    if (*(tds__SetNetworkInterfaces->NetworkInterface->IPv4->Enabled) == xsd__boolean__true_)
    {    
        if ((tds__SetNetworkInterfaces->NetworkInterface->IPv4->DHCP == NULL) || (*(tds__SetNetworkInterfaces->NetworkInterface->IPv4->DHCP) == xsd__boolean__false_))
        {
            if(tds__SetNetworkInterfaces->NetworkInterface->IPv4->Manual != NULL)
            {
                dlog_debug("onvif 设置网络 手动配置");
                memcpy(stOnvifInfo.achIPAddr, tds__SetNetworkInterfaces->NetworkInterface->IPv4->Manual->Address, sizeof(stOnvifInfo.achIPAddr));
                stOnvifInfo.nPrefixlen = tds__SetNetworkInterfaces->NetworkInterface->IPv4->Manual->PrefixLength;
                stOnvifInfo.bDhcp = 0;
                nRet = onvif_set_ipAdress(stOnvifInfo);
                if(nRet != 0)
                {
                    dlog_error("SetNetworkInterfaces Error ");
                    return SOAP_EOF;
                }
            }
            else
            {
                dlog_error("SetNetworkInterfaces Manual NULL ");
                return SOAP_EOF;
            }
          
        }
        else if (*(tds__SetNetworkInterfaces->NetworkInterface->IPv4->DHCP) == xsd__boolean__true_)
        {
             dlog_debug("onvif 设置网络 自动配置");
            stOnvifInfo.bDhcp = 1;
            nRet = onvif_set_ipAdress(stOnvifInfo);
            if(nRet != 0)
            {
                dlog_error("SetNetworkInterfaces Error ");
                return SOAP_EOF;
            }
        }
    }

    return SOAP_OK;
}

/** Web service operation '__tds__GetNetworkProtocols' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetNetworkProtocols(struct soap* soap, struct _tds__GetNetworkProtocols *tds__GetNetworkProtocols, struct _tds__GetNetworkProtocolsResponse *tds__GetNetworkProtocolsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetNetworkProtocols----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetNetworkProtocols' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetNetworkProtocols(struct soap* soap, struct _tds__SetNetworkProtocols *tds__SetNetworkProtocols, struct _tds__SetNetworkProtocolsResponse *tds__SetNetworkProtocolsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetNetworkProtocols----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetNetworkDefaultGateway' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetNetworkDefaultGateway(struct soap* soap, struct _tds__GetNetworkDefaultGateway *tds__GetNetworkDefaultGateway, struct _tds__GetNetworkDefaultGatewayResponse *tds__GetNetworkDefaultGatewayResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetNetworkDefaultGateway----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetNetworkDefaultGateway' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetNetworkDefaultGateway(struct soap* soap, struct _tds__SetNetworkDefaultGateway *tds__SetNetworkDefaultGateway, struct _tds__SetNetworkDefaultGatewayResponse *tds__SetNetworkDefaultGatewayResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetNetworkDefaultGateway----------");
#endif
    int nRet = onvif_authentication(soap);
    if(SOAP_OK != nRet)
    {
        return nRet;
    }

    if (tds__SetNetworkDefaultGateway == NULL || tds__SetNetworkDefaultGateway->IPv4Address == NULL)
    {
        dlog_error("tds__SetNetworkDefaultGateway->IPv4Address is NULL");
        return SOAP_EOF;
    }

    if(tds__SetNetworkDefaultGateway->__sizeIPv4Address < 1)
    {
        dlog_error("tds__SetNetworkDefaultGateway->IPv4Address is NULL");
        return SOAP_EOF;
    }

    if (tds__SetNetworkDefaultGateway->IPv4Address && tds__SetNetworkDefaultGateway->__sizeIPv4Address)
    {
        if (onvif_isValidIpv4(*(tds__SetNetworkDefaultGateway->IPv4Address)))
        {
            nRet = onvif_set_Gateway(*(tds__SetNetworkDefaultGateway->IPv4Address));
            if(nRet != 0)
            {
                return SOAP_EOF;
            }
        }     
        else
        {
            // fault(soap, "soap:Sender", "ter:InvalidArgVal", "ter:InvalidGatewayAddress", "The supplied gateway address was invalid.", "The supplied gateway address was invalid.");
            return SOAP_EOF;
        }
    }
    else
    {
        // fault(soap, "soap:Sender", "ter:InvalidArgVal", "ter:NULLGatewayAddress", "The supplied gateway address was invalid.", "The supplied gateway address was null.");
        return SOAP_EOF;
    }

    return SOAP_OK;
}

/** Web service operation '__tds__GetZeroConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetZeroConfiguration(struct soap* soap, struct _tds__GetZeroConfiguration *tds__GetZeroConfiguration, struct _tds__GetZeroConfigurationResponse *tds__GetZeroConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetZeroConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetZeroConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetZeroConfiguration(struct soap* soap, struct _tds__SetZeroConfiguration *tds__SetZeroConfiguration, struct _tds__SetZeroConfigurationResponse *tds__SetZeroConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetZeroConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetIPAddressFilter' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetIPAddressFilter(struct soap* soap, struct _tds__GetIPAddressFilter *tds__GetIPAddressFilter, struct _tds__GetIPAddressFilterResponse *tds__GetIPAddressFilterResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetIPAddressFilter----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetIPAddressFilter' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetIPAddressFilter(struct soap* soap, struct _tds__SetIPAddressFilter *tds__SetIPAddressFilter, struct _tds__SetIPAddressFilterResponse *tds__SetIPAddressFilterResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetIPAddressFilter----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__AddIPAddressFilter' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__AddIPAddressFilter(struct soap* soap, struct _tds__AddIPAddressFilter *tds__AddIPAddressFilter, struct _tds__AddIPAddressFilterResponse *tds__AddIPAddressFilterResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__AddIPAddressFilter----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__RemoveIPAddressFilter' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__RemoveIPAddressFilter(struct soap* soap, struct _tds__RemoveIPAddressFilter *tds__RemoveIPAddressFilter, struct _tds__RemoveIPAddressFilterResponse *tds__RemoveIPAddressFilterResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__RemoveIPAddressFilter----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetAccessPolicy' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetAccessPolicy(struct soap* soap, struct _tds__GetAccessPolicy *tds__GetAccessPolicy, struct _tds__GetAccessPolicyResponse *tds__GetAccessPolicyResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetAccessPolicy----------");
#endif
    return SOAP_OK;
}


/** Web service operation '__tds__SetAccessPolicy' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetAccessPolicy(struct soap* soap, struct _tds__SetAccessPolicy *tds__SetAccessPolicy, struct _tds__SetAccessPolicyResponse *tds__SetAccessPolicyResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetAccessPolicy----------");
#endif
    return SOAP_OK;
}


/** Web service operation '__tds__CreateCertificate' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__CreateCertificate(struct soap* soap, struct _tds__CreateCertificate *tds__CreateCertificate, struct _tds__CreateCertificateResponse *tds__CreateCertificateResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__CreateCertificate----------");
#endif
    return SOAP_OK;
}


/** Web service operation '__tds__GetCertificates' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetCertificates(struct soap* soap, struct _tds__GetCertificates *tds__GetCertificates, struct _tds__GetCertificatesResponse *tds__GetCertificatesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetCertificates----------");
#endif
    return SOAP_OK;
}


/** Web service operation '__tds__GetCertificatesStatus' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetCertificatesStatus(struct soap* soap, struct _tds__GetCertificatesStatus *tds__GetCertificatesStatus, struct _tds__GetCertificatesStatusResponse *tds__GetCertificatesStatusResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetCertificatesStatus----------");
#endif
    return SOAP_OK;
}


/** Web service operation '__tds__SetCertificatesStatus' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetCertificatesStatus(struct soap* soap, struct _tds__SetCertificatesStatus *tds__SetCertificatesStatus, struct _tds__SetCertificatesStatusResponse *tds__SetCertificatesStatusResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetCertificatesStatus----------");
#endif
    return SOAP_OK;
}


/** Web service operation '__tds__DeleteCertificates' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__DeleteCertificates(struct soap* soap, struct _tds__DeleteCertificates *tds__DeleteCertificates, struct _tds__DeleteCertificatesResponse *tds__DeleteCertificatesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__DeleteCertificates----------");
#endif
    return SOAP_OK;
}


/** Web service operation '__tds__GetPkcs10Request' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetPkcs10Request(struct soap* soap, struct _tds__GetPkcs10Request *tds__GetPkcs10Request, struct _tds__GetPkcs10RequestResponse *tds__GetPkcs10RequestResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetPkcs10Request----------");
#endif
    return SOAP_OK;
}


/** Web service operation '__tds__LoadCertificates' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__LoadCertificates(struct soap* soap, struct _tds__LoadCertificates *tds__LoadCertificates, struct _tds__LoadCertificatesResponse *tds__LoadCertificatesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__LoadCertificates----------");
#endif
    return SOAP_OK;
}


/** Web service operation '__tds__GetClientCertificateMode' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetClientCertificateMode(struct soap* soap, struct _tds__GetClientCertificateMode *tds__GetClientCertificateMode, struct _tds__GetClientCertificateModeResponse *tds__GetClientCertificateModeResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetClientCertificateMode----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetClientCertificateMode' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetClientCertificateMode(struct soap* soap, struct _tds__SetClientCertificateMode *tds__SetClientCertificateMode, struct _tds__SetClientCertificateModeResponse *tds__SetClientCertificateModeResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetClientCertificateMode----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetRelayOutputs' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetRelayOutputs(struct soap* soap, struct _tds__GetRelayOutputs *tds__GetRelayOutputs, struct _tds__GetRelayOutputsResponse *tds__GetRelayOutputsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetRelayOutputs----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetRelayOutputSettings' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetRelayOutputSettings(struct soap* soap, struct _tds__SetRelayOutputSettings *tds__SetRelayOutputSettings, struct _tds__SetRelayOutputSettingsResponse *tds__SetRelayOutputSettingsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetRelayOutputSettings----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetRelayOutputState' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetRelayOutputState(struct soap* soap, struct _tds__SetRelayOutputState *tds__SetRelayOutputState, struct _tds__SetRelayOutputStateResponse *tds__SetRelayOutputStateResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetRelayOutputState----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SendAuxiliaryCommand' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SendAuxiliaryCommand(struct soap* soap, struct _tds__SendAuxiliaryCommand *tds__SendAuxiliaryCommand, struct _tds__SendAuxiliaryCommandResponse *tds__SendAuxiliaryCommandResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SendAuxiliaryCommand----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetCACertificates' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetCACertificates(struct soap* soap, struct _tds__GetCACertificates *tds__GetCACertificates, struct _tds__GetCACertificatesResponse *tds__GetCACertificatesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetCACertificates----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__LoadCertificateWithPrivateKey' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__LoadCertificateWithPrivateKey(struct soap* soap, struct _tds__LoadCertificateWithPrivateKey *tds__LoadCertificateWithPrivateKey, struct _tds__LoadCertificateWithPrivateKeyResponse *tds__LoadCertificateWithPrivateKeyResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__LoadCertificateWithPrivateKey----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetCertificateInformation' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetCertificateInformation(struct soap* soap, struct _tds__GetCertificateInformation *tds__GetCertificateInformation, struct _tds__GetCertificateInformationResponse *tds__GetCertificateInformationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetCertificateInformation----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__LoadCACertificates' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__LoadCACertificates(struct soap* soap, struct _tds__LoadCACertificates *tds__LoadCACertificates, struct _tds__LoadCACertificatesResponse *tds__LoadCACertificatesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__LoadCACertificates----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__CreateDot1XConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__CreateDot1XConfiguration(struct soap* soap, struct _tds__CreateDot1XConfiguration *tds__CreateDot1XConfiguration, struct _tds__CreateDot1XConfigurationResponse *tds__CreateDot1XConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__CreateDot1XConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetDot1XConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetDot1XConfiguration(struct soap* soap, struct _tds__SetDot1XConfiguration *tds__SetDot1XConfiguration, struct _tds__SetDot1XConfigurationResponse *tds__SetDot1XConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetDot1XConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetDot1XConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetDot1XConfiguration(struct soap* soap, struct _tds__GetDot1XConfiguration *tds__GetDot1XConfiguration, struct _tds__GetDot1XConfigurationResponse *tds__GetDot1XConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetDot1XConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetDot1XConfigurations' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetDot1XConfigurations(struct soap* soap, struct _tds__GetDot1XConfigurations *tds__GetDot1XConfigurations, struct _tds__GetDot1XConfigurationsResponse *tds__GetDot1XConfigurationsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetDot1XConfigurations----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__DeleteDot1XConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__DeleteDot1XConfiguration(struct soap* soap, struct _tds__DeleteDot1XConfiguration *tds__DeleteDot1XConfiguration, struct _tds__DeleteDot1XConfigurationResponse *tds__DeleteDot1XConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__DeleteDot1XConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetDot11Capabilities' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetDot11Capabilities(struct soap* soap, struct _tds__GetDot11Capabilities *tds__GetDot11Capabilities, struct _tds__GetDot11CapabilitiesResponse *tds__GetDot11CapabilitiesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetDot11Capabilities----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetDot11Status' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetDot11Status(struct soap* soap, struct _tds__GetDot11Status *tds__GetDot11Status, struct _tds__GetDot11StatusResponse *tds__GetDot11StatusResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetDot11Status----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__ScanAvailableDot11Networks' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__ScanAvailableDot11Networks(struct soap* soap, struct _tds__ScanAvailableDot11Networks *tds__ScanAvailableDot11Networks, struct _tds__ScanAvailableDot11NetworksResponse *tds__ScanAvailableDot11NetworksResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__ScanAvailableDot11Networks----------");
#endif
    return SOAP_OK;
}


/** Web service operation '__tds__GetSystemUris' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetSystemUris(struct soap* soap, struct _tds__GetSystemUris *tds__GetSystemUris, struct _tds__GetSystemUrisResponse *tds__GetSystemUrisResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetSystemUris----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__StartFirmwareUpgrade' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__StartFirmwareUpgrade(struct soap* soap, struct _tds__StartFirmwareUpgrade *tds__StartFirmwareUpgrade, struct _tds__StartFirmwareUpgradeResponse *tds__StartFirmwareUpgradeResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__StartFirmwareUpgrade----------");
    
#endif

    int nRet = onvif_authentication(soap);
    if(SOAP_OK != nRet)
    {
        return nRet;
    }

    if(tds__StartFirmwareUpgradeResponse == NULL)
    {
        dlog_error("tds__StartFirmwareUpgradeResponse is NULL");
        return SOAP_EOF;
    }

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

    char achUri[256];
    sprintf(achUri, "http://%s:%d/StartFirmwareUpgrade", achIp,ONVIF_UPGRADE_PORT);
    tds__StartFirmwareUpgradeResponse->UploadUri = soap_strdup(soap, achUri);
    tds__StartFirmwareUpgradeResponse->UploadDelay = 0;
    tds__StartFirmwareUpgradeResponse->ExpectedDownTime = 180;

    return SOAP_OK;
}

/** Web service operation '__tds__StartSystemRestore' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__StartSystemRestore(struct soap* soap, struct _tds__StartSystemRestore *tds__StartSystemRestore, struct _tds__StartSystemRestoreResponse *tds__StartSystemRestoreResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__StartSystemRestore----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetStorageConfigurations' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetStorageConfigurations(struct soap* soap, struct _tds__GetStorageConfigurations *tds__GetStorageConfigurations, struct _tds__GetStorageConfigurationsResponse *tds__GetStorageConfigurationsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetStorageConfigurations----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__CreateStorageConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__CreateStorageConfiguration(struct soap* soap, struct _tds__CreateStorageConfiguration *tds__CreateStorageConfiguration, struct _tds__CreateStorageConfigurationResponse *tds__CreateStorageConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__CreateStorageConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetStorageConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetStorageConfiguration(struct soap* soap, struct _tds__GetStorageConfiguration *tds__GetStorageConfiguration, struct _tds__GetStorageConfigurationResponse *tds__GetStorageConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetStorageConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetStorageConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetStorageConfiguration(struct soap* soap, struct _tds__SetStorageConfiguration *tds__SetStorageConfiguration, struct _tds__SetStorageConfigurationResponse *tds__SetStorageConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetStorageConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__DeleteStorageConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__DeleteStorageConfiguration(struct soap* soap, struct _tds__DeleteStorageConfiguration *tds__DeleteStorageConfiguration, struct _tds__DeleteStorageConfigurationResponse *tds__DeleteStorageConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__DeleteStorageConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetGeoLocation' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetGeoLocation(struct soap* soap, struct _tds__GetGeoLocation *tds__GetGeoLocation, struct _tds__GetGeoLocationResponse *tds__GetGeoLocationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetGeoLocation----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetGeoLocation' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetGeoLocation(struct soap* soap, struct _tds__SetGeoLocation *tds__SetGeoLocation, struct _tds__SetGeoLocationResponse *tds__SetGeoLocationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetGeoLocation----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__DeleteGeoLocation' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__DeleteGeoLocation(struct soap* soap, struct _tds__DeleteGeoLocation *tds__DeleteGeoLocation, struct _tds__DeleteGeoLocationResponse *tds__DeleteGeoLocationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__DeleteGeoLocation----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetServices_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetServices_(struct soap* soap, struct _tds__GetServices *tds__GetServices, struct _tds__GetServicesResponse *tds__GetServicesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetServices_----------");
#endif
    int size = 5;
    // int size = 3;
    tds__GetServicesResponse->__sizeService = size;
    tds__GetServicesResponse->Service = (struct tds__Service *)soap_malloc(soap, sizeof(struct tds__Service) * size);
    memset(tds__GetServicesResponse->Service, 0, sizeof(struct tds__Service) * size);

    //get IP
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

    //device
    int i = 0;
    tds__GetServicesResponse->Service[i].Namespace = (char *)soap_malloc(soap, sizeof(char)* 100);
    strcpy(tds__GetServicesResponse->Service[i].Namespace, "http://www.onvif.org/ver10/device/wsdl");
    tds__GetServicesResponse->Service[i].XAddr     = (char *)soap_malloc(soap, sizeof(char)* 100);
    sprintf(tds__GetServicesResponse->Service[i].XAddr, "http://%s:%d/onvif/device_service", achIp, ONVIF_TCP_PORT);
    // 给 Device Service 加上 Capabilities.Security
    tds__GetServicesResponse->Service[i].Capabilities = (struct _tds__Service_Capabilities *)soap_malloc(soap, sizeof(struct _tds__Service_Capabilities));
    memset(tds__GetServicesResponse->Service[i].Capabilities, 0, sizeof(struct _tds__Service_Capabilities));
    tds__GetServicesResponse->Service[i].Capabilities->__any = (char*)soap_malloc(soap, 512);
    sprintf(tds__GetServicesResponse->Service[i].Capabilities->__any,
        "<tds:Security TLS1.0=\"true\" TLS1.1=\"true\" TLS1.2=\"true\" "
        "OnboardKeyGeneration=\"false\" AccessPolicyConfig=\"false\" "
        "DefaultAccessPolicy=\"true\" Dot1X=\"false\" RemoteUserHandling=\"false\" "
        "X.509Token=\"false\" SAMLToken=\"false\" KerberosToken=\"false\" "
        "UsernameToken=\"true\" HttpDigest=\"true\" RELToken=\"false\" "
        "SupportedEAPMethods=\"0\" MaxUsers=\"32\" MaxUserNameLength=\"32\" MaxPasswordLength=\"16\"/>");

    //media
    i = 1;
    tds__GetServicesResponse->Service[i].Namespace = (char *)soap_malloc(soap, sizeof(char)* 100);
    strcpy(tds__GetServicesResponse->Service[i].Namespace, "http://www.onvif.org/ver10/media/wsdl");
    tds__GetServicesResponse->Service[i].XAddr     = (char *)soap_malloc(soap, sizeof(char)* 100);
    sprintf(tds__GetServicesResponse->Service[i].XAddr, "http://%s:%d/onvif/media_service", achIp, ONVIF_TCP_PORT);

    //image
	i = 2;
	tds__GetServicesResponse->Service[i].Namespace = (char *)soap_malloc(soap, sizeof(char)* 100);
	strcpy(tds__GetServicesResponse->Service[i].Namespace, "http://www.onvif.org/ver20/imaging/wsdl");
	tds__GetServicesResponse->Service[i].XAddr = (char *)soap_malloc(soap, sizeof(char)* 100);
	sprintf(tds__GetServicesResponse->Service[i].XAddr, "http://%s:%d/onvif/image_service", achIp, ONVIF_TCP_PORT);

    //media2
    i = 3;
    tds__GetServicesResponse->Service[i].Namespace = (char *)soap_malloc(soap, sizeof(char)* 100);
    strcpy(tds__GetServicesResponse->Service[i].Namespace, "http://www.onvif.org/ver20/media/wsdl");
    tds__GetServicesResponse->Service[i].XAddr     = (char *)soap_malloc(soap, sizeof(char)* 100);
    sprintf(tds__GetServicesResponse->Service[i].XAddr, "http://%s:%d/onvif/media2_service", achIp, ONVIF_TCP_PORT);

    //event
    i = 4;
    tds__GetServicesResponse->Service[i].Namespace = (char *)soap_malloc(soap, sizeof(char)* 100);
    strcpy(tds__GetServicesResponse->Service[i].Namespace, "http://www.onvif.org/ver10/events/wsdl");
    tds__GetServicesResponse->Service[i].XAddr     = (char *)soap_malloc(soap, sizeof(char)* 100);
    sprintf(tds__GetServicesResponse->Service[i].XAddr, "http://%s:%d/onvif/event_service", achIp, ONVIF_TCP_PORT);

    for(int i=0; i<tds__GetServicesResponse->__sizeService; i++) 
    {
        tds__GetServicesResponse->Service[i].Version = (struct tt__OnvifVersion *)soap_malloc(soap, sizeof(struct tt__OnvifVersion));
        tds__GetServicesResponse->Service[i].Version->Major = 1;
        tds__GetServicesResponse->Service[i].Version->Minor = 10;
    }

    return SOAP_OK;
}

/** Web service operation '__tds__GetServiceCapabilities_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetServiceCapabilities_(struct soap* soap, struct _tds__GetServiceCapabilities *tds__GetServiceCapabilities, struct _tds__GetServiceCapabilitiesResponse *tds__GetServiceCapabilitiesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetServiceCapabilities_----------");
#endif
    if(soap == NULL)
    {
#if ONVIF_LOG_SWITCH
    dlog_error("空指针");
#endif
    }

    // 分配 Service_Capabilities 结构体内存
    tds__GetServiceCapabilitiesResponse->Capabilities =
        (struct _tds__Service_Capabilities *)soap_malloc(soap, sizeof(struct _tds__Service_Capabilities));
    if (!tds__GetServiceCapabilitiesResponse->Capabilities)
        return soap->error = SOAP_EOM; // 内存不足

    memset(tds__GetServiceCapabilitiesResponse->Capabilities, 0, sizeof(struct _tds__Service_Capabilities));

    // -------------------------------
    // Security Capabilities
    // -------------------------------
    tds__GetServiceCapabilitiesResponse->Capabilities->Security =
        (struct tds__SecurityCapabilities *)soap_malloc(soap, sizeof(struct tds__SecurityCapabilities));
    memset(tds__GetServiceCapabilitiesResponse->Capabilities->Security, 0, sizeof(struct tds__SecurityCapabilities));

    // 根据实际情况设置
    tds__GetServiceCapabilitiesResponse->Capabilities->Security->TLS1_x002e1 = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->Security->TLS1_x002e1) = xsd__boolean__true_;

    tds__GetServiceCapabilitiesResponse->Capabilities->Security->TLS1_x002e2 = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->Security->TLS1_x002e2) = xsd__boolean__true_;

    tds__GetServiceCapabilitiesResponse->Capabilities->Security->OnboardKeyGeneration = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->Security->OnboardKeyGeneration) = xsd__boolean__false_;

    tds__GetServiceCapabilitiesResponse->Capabilities->Security->AccessPolicyConfig = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->Security->AccessPolicyConfig) = xsd__boolean__false_;

    tds__GetServiceCapabilitiesResponse->Capabilities->Security->DefaultAccessPolicy = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->Security->DefaultAccessPolicy) = xsd__boolean__false_;

    tds__GetServiceCapabilitiesResponse->Capabilities->Security->X_x002e509Token = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->Security->X_x002e509Token) = xsd__boolean__false_;

    tds__GetServiceCapabilitiesResponse->Capabilities->Security->SAMLToken = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->Security->SAMLToken) = xsd__boolean__false_;

    tds__GetServiceCapabilitiesResponse->Capabilities->Security->KerberosToken = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->Security->KerberosToken) = xsd__boolean__false_;

    tds__GetServiceCapabilitiesResponse->Capabilities->Security->RELToken = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->Security->RELToken) = xsd__boolean__false_;

    tds__GetServiceCapabilitiesResponse->Capabilities->Security->Dot1X = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->Security->Dot1X) = xsd__boolean__false_;

    tds__GetServiceCapabilitiesResponse->Capabilities->Security->RemoteUserHandling = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->Security->RemoteUserHandling) = xsd__boolean__false_;

    tds__GetServiceCapabilitiesResponse->Capabilities->Security->SupportedEAPMethods = (char *)soap_malloc(soap, sizeof(char));
    tds__GetServiceCapabilitiesResponse->Capabilities->Security->SupportedEAPMethods = soap_strdup(soap, "0");

    tds__GetServiceCapabilitiesResponse->Capabilities->Security->MaxUsers = (int *)soap_malloc(soap, sizeof(int));
    tds__GetServiceCapabilitiesResponse->Capabilities->Security->MaxUsers = soap_strdup(soap, "32");

    tds__GetServiceCapabilitiesResponse->Capabilities->Security->MaxUserNameLength = (int *)soap_malloc(soap, sizeof(int));
    tds__GetServiceCapabilitiesResponse->Capabilities->Security->MaxUserNameLength = soap_strdup(soap, "32");

    tds__GetServiceCapabilitiesResponse->Capabilities->Security->MaxPasswordLength = (int *)soap_malloc(soap, sizeof(int));
    tds__GetServiceCapabilitiesResponse->Capabilities->Security->MaxPasswordLength = soap_strdup(soap, "32");

    if (ONVIF_DIGEST_MODE == onvif_get_auth_mode())
    {
        tds__GetServiceCapabilitiesResponse->Capabilities->Security->UsernameToken = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
        *(tds__GetServiceCapabilitiesResponse->Capabilities->Security->UsernameToken) = xsd__boolean__false_;
    
        tds__GetServiceCapabilitiesResponse->Capabilities->Security->HttpDigest = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
        *(tds__GetServiceCapabilitiesResponse->Capabilities->Security->HttpDigest) = xsd__boolean__true_;
    }
    else
    {
        tds__GetServiceCapabilitiesResponse->Capabilities->Security->UsernameToken = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
        *(tds__GetServiceCapabilitiesResponse->Capabilities->Security->UsernameToken) = xsd__boolean__true_;
    
        tds__GetServiceCapabilitiesResponse->Capabilities->Security->HttpDigest = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
        *(tds__GetServiceCapabilitiesResponse->Capabilities->Security->HttpDigest) = xsd__boolean__false_;
    }

    // -------------------------------
    // System Capabilities
    // -------------------------------
    tds__GetServiceCapabilitiesResponse->Capabilities->System =
        (struct tds__SystemCapabilities *)soap_malloc(soap, sizeof(struct tds__SystemCapabilities));
    memset(tds__GetServiceCapabilitiesResponse->Capabilities->System, 0, sizeof(struct tds__SystemCapabilities));

    tds__GetServiceCapabilitiesResponse->Capabilities->System->DiscoveryResolve = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->System->DiscoveryResolve) = xsd__boolean__true_;

    tds__GetServiceCapabilitiesResponse->Capabilities->System->DiscoveryBye = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->System->DiscoveryBye) = xsd__boolean__true_;

    tds__GetServiceCapabilitiesResponse->Capabilities->System->RemoteDiscovery = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->System->RemoteDiscovery) = xsd__boolean__false_;

    tds__GetServiceCapabilitiesResponse->Capabilities->System->SystemBackup = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->System->SystemBackup) = xsd__boolean__false_;

    tds__GetServiceCapabilitiesResponse->Capabilities->System->FirmwareUpgrade = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->System->FirmwareUpgrade) = xsd__boolean__true_;

    // -------------------------------
    // Network Capabilities
    // -------------------------------
    tds__GetServiceCapabilitiesResponse->Capabilities->Network =
        (struct tds__NetworkCapabilities *)soap_malloc(soap, sizeof(struct tds__NetworkCapabilities));
    memset(tds__GetServiceCapabilitiesResponse->Capabilities->Network, 0, sizeof(struct tds__NetworkCapabilities));

    tds__GetServiceCapabilitiesResponse->Capabilities->Network->IPFilter = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->Network->IPFilter) = xsd__boolean__true_;

    tds__GetServiceCapabilitiesResponse->Capabilities->Network->ZeroConfiguration = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->Network->ZeroConfiguration) = xsd__boolean__true_;

    tds__GetServiceCapabilitiesResponse->Capabilities->Network->IPVersion6 = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    *(tds__GetServiceCapabilitiesResponse->Capabilities->Network->IPVersion6) = xsd__boolean__true_;

    return SOAP_OK;
}

/** Web service operation '__tds__GetDeviceInformation_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetDeviceInformation_(struct soap* soap, struct _tds__GetDeviceInformation *tds__GetDeviceInformation, struct _tds__GetDeviceInformationResponse *tds__GetDeviceInformationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetDeviceInformation_----------");
#endif
    if(soap == NULL)
    {
#if ONVIF_LOG_SWITCH
    dlog_error("空指针");
#endif
    }

    int nRet = onvif_authentication(soap);
    if(SOAP_OK != nRet)
    {
        return nRet;
    }

    tds__GetDeviceInformationResponse->Manufacturer = (char *)soap_malloc(soap, sizeof(char) * 16);
    tds__GetDeviceInformationResponse->Model = (char *)soap_malloc(soap, sizeof(char) * 16);
    tds__GetDeviceInformationResponse->FirmwareVersion = (char *)soap_malloc(soap, sizeof(char) * 16);
    tds__GetDeviceInformationResponse->SerialNumber = (char *)soap_malloc(soap, sizeof(char) * 16);
    tds__GetDeviceInformationResponse->HardwareId = (char *)soap_malloc(soap, sizeof(char) * 16);

    OnvifDeviceInfo_t stInfo;
    onvif_get_device_info(&stInfo);

    strcpy(tds__GetDeviceInformationResponse->Manufacturer, stInfo.achManufacturer);
    strcpy(tds__GetDeviceInformationResponse->Model, stInfo.achModel);
    strcpy(tds__GetDeviceInformationResponse->FirmwareVersion, stInfo.achFirmwareVersion);
    strcpy(tds__GetDeviceInformationResponse->SerialNumber, stInfo.achSerialNumber);
    strcpy(tds__GetDeviceInformationResponse->HardwareId, stInfo.achHardwareId);
    return SOAP_OK;
}

/** Web service operation '__tds__SetSystemDateAndTime_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetSystemDateAndTime_(struct soap* soap, struct _tds__SetSystemDateAndTime *tds__SetSystemDateAndTime, struct _tds__SetSystemDateAndTimeResponse *tds__SetSystemDateAndTimeResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetSystemDateAndTime_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetSystemDateAndTime_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetSystemDateAndTime_(struct soap* soap, struct _tds__GetSystemDateAndTime *tds__GetSystemDateAndTime, struct _tds__GetSystemDateAndTimeResponse *tds__GetSystemDateAndTimeResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetSystemDateAndTime_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetSystemFactoryDefault_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetSystemFactoryDefault_(struct soap* soap, struct _tds__SetSystemFactoryDefault *tds__SetSystemFactoryDefault, struct _tds__SetSystemFactoryDefaultResponse *tds__SetSystemFactoryDefaultResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetSystemFactoryDefault_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__UpgradeSystemFirmware_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__UpgradeSystemFirmware_(struct soap* soap, struct _tds__UpgradeSystemFirmware *tds__UpgradeSystemFirmware, struct _tds__UpgradeSystemFirmwareResponse *tds__UpgradeSystemFirmwareResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__UpgradeSystemFirmware_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SystemReboot_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SystemReboot_(struct soap* soap, struct _tds__SystemReboot *tds__SystemReboot, struct _tds__SystemRebootResponse *tds__SystemRebootResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SystemReboot_----------");
#endif
    onvif_reboot(soap);
    return SOAP_OK;
}

/** Web service operation '__tds__RestoreSystem_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__RestoreSystem_(struct soap* soap, struct _tds__RestoreSystem *tds__RestoreSystem, struct _tds__RestoreSystemResponse *tds__RestoreSystemResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__RestoreSystem_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetSystemBackup_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetSystemBackup_(struct soap* soap, struct _tds__GetSystemBackup *tds__GetSystemBackup, struct _tds__GetSystemBackupResponse *tds__GetSystemBackupResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetSystemBackup_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetSystemLog_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetSystemLog_(struct soap* soap, struct _tds__GetSystemLog *tds__GetSystemLog, struct _tds__GetSystemLogResponse *tds__GetSystemLogResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetSystemLog_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetSystemSupportInformation_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetSystemSupportInformation_(struct soap* soap, struct _tds__GetSystemSupportInformation *tds__GetSystemSupportInformation, struct _tds__GetSystemSupportInformationResponse *tds__GetSystemSupportInformationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetSystemSupportInformation_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetScopes_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetScopes_(struct soap* soap, struct _tds__GetScopes *tds__GetScopes, struct _tds__GetScopesResponse *tds__GetScopesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetScopes_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetScopes_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetScopes_(struct soap* soap, struct _tds__SetScopes *tds__SetScopes, struct _tds__SetScopesResponse *tds__SetScopesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetScopes_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__AddScopes_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__AddScopes_(struct soap* soap, struct _tds__AddScopes *tds__AddScopes, struct _tds__AddScopesResponse *tds__AddScopesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__AddScopes_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__RemoveScopes_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__RemoveScopes_(struct soap* soap, struct _tds__RemoveScopes *tds__RemoveScopes, struct _tds__RemoveScopesResponse *tds__RemoveScopesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__RemoveScopes_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetDiscoveryMode_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetDiscoveryMode_(struct soap* soap, struct _tds__GetDiscoveryMode *tds__GetDiscoveryMode, struct _tds__GetDiscoveryModeResponse *tds__GetDiscoveryModeResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetDiscoveryMode_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetDiscoveryMode_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetDiscoveryMode_(struct soap* soap, struct _tds__SetDiscoveryMode *tds__SetDiscoveryMode, struct _tds__SetDiscoveryModeResponse *tds__SetDiscoveryModeResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetDiscoveryMode_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetRemoteDiscoveryMode_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetRemoteDiscoveryMode_(struct soap* soap, struct _tds__GetRemoteDiscoveryMode *tds__GetRemoteDiscoveryMode, struct _tds__GetRemoteDiscoveryModeResponse *tds__GetRemoteDiscoveryModeResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetRemoteDiscoveryMode_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetRemoteDiscoveryMode_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetRemoteDiscoveryMode_(struct soap* soap, struct _tds__SetRemoteDiscoveryMode *tds__SetRemoteDiscoveryMode, struct _tds__SetRemoteDiscoveryModeResponse *tds__SetRemoteDiscoveryModeResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetRemoteDiscoveryMode_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetDPAddresses_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetDPAddresses_(struct soap* soap, struct _tds__GetDPAddresses *tds__GetDPAddresses, struct _tds__GetDPAddressesResponse *tds__GetDPAddressesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetDPAddresses_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetEndpointReference_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetEndpointReference_(struct soap* soap, struct _tds__GetEndpointReference *tds__GetEndpointReference, struct _tds__GetEndpointReferenceResponse *tds__GetEndpointReferenceResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetEndpointReference_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetRemoteUser_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetRemoteUser_(struct soap* soap, struct _tds__GetRemoteUser *tds__GetRemoteUser, struct _tds__GetRemoteUserResponse *tds__GetRemoteUserResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetRemoteUser_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetRemoteUser_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetRemoteUser_(struct soap* soap, struct _tds__SetRemoteUser *tds__SetRemoteUser, struct _tds__SetRemoteUserResponse *tds__SetRemoteUserResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetRemoteUser_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetUsers_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetUsers_(struct soap* soap, struct _tds__GetUsers *tds__GetUsers, struct _tds__GetUsersResponse *tds__GetUsersResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetUsers_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__CreateUsers_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__CreateUsers_(struct soap* soap, struct _tds__CreateUsers *tds__CreateUsers, struct _tds__CreateUsersResponse *tds__CreateUsersResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__CreateUsers_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__DeleteUsers_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__DeleteUsers_(struct soap* soap, struct _tds__DeleteUsers *tds__DeleteUsers, struct _tds__DeleteUsersResponse *tds__DeleteUsersResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__DeleteUsers_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetUser_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetUser_(struct soap* soap, struct _tds__SetUser *tds__SetUser, struct _tds__SetUserResponse *tds__SetUserResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetUser_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetWsdlUrl_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetWsdlUrl_(struct soap* soap, struct _tds__GetWsdlUrl *tds__GetWsdlUrl, struct _tds__GetWsdlUrlResponse *tds__GetWsdlUrlResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetWsdlUrl_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetCapabilities_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetCapabilities_(struct soap* soap, struct _tds__GetCapabilities *tds__GetCapabilities, struct _tds__GetCapabilitiesResponse *tds__GetCapabilitiesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetCapabilities_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetDPAddresses_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetDPAddresses_(struct soap* soap, struct _tds__SetDPAddresses *tds__SetDPAddresses, struct _tds__SetDPAddressesResponse *tds__SetDPAddressesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetDPAddresses_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetHostname_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetHostname_(struct soap* soap, struct _tds__GetHostname *tds__GetHostname, struct _tds__GetHostnameResponse *tds__GetHostnameResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetHostname_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetHostname_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetHostname_(struct soap* soap, struct _tds__SetHostname *tds__SetHostname, struct _tds__SetHostnameResponse *tds__SetHostnameResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetHostname_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetHostnameFromDHCP_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetHostnameFromDHCP_(struct soap* soap, struct _tds__SetHostnameFromDHCP *tds__SetHostnameFromDHCP, struct _tds__SetHostnameFromDHCPResponse *tds__SetHostnameFromDHCPResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetHostnameFromDHCP_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetDNS_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetDNS_(struct soap* soap, struct _tds__GetDNS *tds__GetDNS, struct _tds__GetDNSResponse *tds__GetDNSResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetDNS_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetDNS_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetDNS_(struct soap* soap, struct _tds__SetDNS *tds__SetDNS, struct _tds__SetDNSResponse *tds__SetDNSResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetDNS_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetNTP_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetNTP_(struct soap* soap, struct _tds__GetNTP *tds__GetNTP, struct _tds__GetNTPResponse *tds__GetNTPResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetNTP_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetNTP_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetNTP_(struct soap* soap, struct _tds__SetNTP *tds__SetNTP, struct _tds__SetNTPResponse *tds__SetNTPResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetNTP_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetDynamicDNS_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetDynamicDNS_(struct soap* soap, struct _tds__GetDynamicDNS *tds__GetDynamicDNS, struct _tds__GetDynamicDNSResponse *tds__GetDynamicDNSResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetDynamicDNS_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetDynamicDNS_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetDynamicDNS_(struct soap* soap, struct _tds__SetDynamicDNS *tds__SetDynamicDNS, struct _tds__SetDynamicDNSResponse *tds__SetDynamicDNSResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetDynamicDNS_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetNetworkInterfaces_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetNetworkInterfaces_(struct soap* soap, struct _tds__GetNetworkInterfaces *tds__GetNetworkInterfaces, struct _tds__GetNetworkInterfacesResponse *tds__GetNetworkInterfacesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetNetworkInterfaces_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetNetworkInterfaces_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetNetworkInterfaces_(struct soap* soap, struct _tds__SetNetworkInterfaces *tds__SetNetworkInterfaces, struct _tds__SetNetworkInterfacesResponse *tds__SetNetworkInterfacesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetNetworkInterfaces_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetNetworkProtocols_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetNetworkProtocols_(struct soap* soap, struct _tds__GetNetworkProtocols *tds__GetNetworkProtocols, struct _tds__GetNetworkProtocolsResponse *tds__GetNetworkProtocolsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetNetworkProtocols_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetNetworkProtocols_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetNetworkProtocols_(struct soap* soap, struct _tds__SetNetworkProtocols *tds__SetNetworkProtocols, struct _tds__SetNetworkProtocolsResponse *tds__SetNetworkProtocolsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetNetworkProtocols_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetNetworkDefaultGateway_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetNetworkDefaultGateway_(struct soap* soap, struct _tds__GetNetworkDefaultGateway *tds__GetNetworkDefaultGateway, struct _tds__GetNetworkDefaultGatewayResponse *tds__GetNetworkDefaultGatewayResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetNetworkDefaultGateway_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetNetworkDefaultGateway_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetNetworkDefaultGateway_(struct soap* soap, struct _tds__SetNetworkDefaultGateway *tds__SetNetworkDefaultGateway, struct _tds__SetNetworkDefaultGatewayResponse *tds__SetNetworkDefaultGatewayResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetNetworkDefaultGateway_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetZeroConfiguration_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetZeroConfiguration_(struct soap* soap, struct _tds__GetZeroConfiguration *tds__GetZeroConfiguration, struct _tds__GetZeroConfigurationResponse *tds__GetZeroConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetZeroConfiguration_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetZeroConfiguration_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetZeroConfiguration_(struct soap* soap, struct _tds__SetZeroConfiguration *tds__SetZeroConfiguration, struct _tds__SetZeroConfigurationResponse *tds__SetZeroConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetZeroConfiguration_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetIPAddressFilter_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetIPAddressFilter_(struct soap* soap, struct _tds__GetIPAddressFilter *tds__GetIPAddressFilter, struct _tds__GetIPAddressFilterResponse *tds__GetIPAddressFilterResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetIPAddressFilter_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetIPAddressFilter_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetIPAddressFilter_(struct soap* soap, struct _tds__SetIPAddressFilter *tds__SetIPAddressFilter, struct _tds__SetIPAddressFilterResponse *tds__SetIPAddressFilterResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetIPAddressFilter_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__AddIPAddressFilter_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__AddIPAddressFilter_(struct soap* soap, struct _tds__AddIPAddressFilter *tds__AddIPAddressFilter, struct _tds__AddIPAddressFilterResponse *tds__AddIPAddressFilterResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__AddIPAddressFilter_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__RemoveIPAddressFilter_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__RemoveIPAddressFilter_(struct soap* soap, struct _tds__RemoveIPAddressFilter *tds__RemoveIPAddressFilter, struct _tds__RemoveIPAddressFilterResponse *tds__RemoveIPAddressFilterResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__RemoveIPAddressFilter_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetAccessPolicy_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetAccessPolicy_(struct soap* soap, struct _tds__GetAccessPolicy *tds__GetAccessPolicy, struct _tds__GetAccessPolicyResponse *tds__GetAccessPolicyResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetAccessPolicy_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetAccessPolicy_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetAccessPolicy_(struct soap* soap, struct _tds__SetAccessPolicy *tds__SetAccessPolicy, struct _tds__SetAccessPolicyResponse *tds__SetAccessPolicyResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetAccessPolicy_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__CreateCertificate_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__CreateCertificate_(struct soap* soap, struct _tds__CreateCertificate *tds__CreateCertificate, struct _tds__CreateCertificateResponse *tds__CreateCertificateResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__CreateCertificate_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetCertificates_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetCertificates_(struct soap* soap, struct _tds__GetCertificates *tds__GetCertificates, struct _tds__GetCertificatesResponse *tds__GetCertificatesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetCertificates_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetCertificatesStatus_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetCertificatesStatus_(struct soap* soap, struct _tds__GetCertificatesStatus *tds__GetCertificatesStatus, struct _tds__GetCertificatesStatusResponse *tds__GetCertificatesStatusResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetCertificatesStatus_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetCertificatesStatus_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetCertificatesStatus_(struct soap* soap, struct _tds__SetCertificatesStatus *tds__SetCertificatesStatus, struct _tds__SetCertificatesStatusResponse *tds__SetCertificatesStatusResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetCertificatesStatus_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__DeleteCertificates_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__DeleteCertificates_(struct soap* soap, struct _tds__DeleteCertificates *tds__DeleteCertificates, struct _tds__DeleteCertificatesResponse *tds__DeleteCertificatesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__DeleteCertificates_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetPkcs10Request_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetPkcs10Request_(struct soap* soap, struct _tds__GetPkcs10Request *tds__GetPkcs10Request, struct _tds__GetPkcs10RequestResponse *tds__GetPkcs10RequestResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetPkcs10Request_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__LoadCertificates_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__LoadCertificates_(struct soap* soap, struct _tds__LoadCertificates *tds__LoadCertificates, struct _tds__LoadCertificatesResponse *tds__LoadCertificatesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__LoadCertificates_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetClientCertificateMode_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetClientCertificateMode_(struct soap* soap, struct _tds__GetClientCertificateMode *tds__GetClientCertificateMode, struct _tds__GetClientCertificateModeResponse *tds__GetClientCertificateModeResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetClientCertificateMode_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetClientCertificateMode_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetClientCertificateMode_(struct soap* soap, struct _tds__SetClientCertificateMode *tds__SetClientCertificateMode, struct _tds__SetClientCertificateModeResponse *tds__SetClientCertificateModeResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetClientCertificateMode_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetRelayOutputs_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetRelayOutputs_(struct soap* soap, struct _tds__GetRelayOutputs *tds__GetRelayOutputs, struct _tds__GetRelayOutputsResponse *tds__GetRelayOutputsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetRelayOutputs_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetRelayOutputSettings_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetRelayOutputSettings_(struct soap* soap, struct _tds__SetRelayOutputSettings *tds__SetRelayOutputSettings, struct _tds__SetRelayOutputSettingsResponse *tds__SetRelayOutputSettingsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetRelayOutputSettings_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetRelayOutputState_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetRelayOutputState_(struct soap* soap, struct _tds__SetRelayOutputState *tds__SetRelayOutputState, struct _tds__SetRelayOutputStateResponse *tds__SetRelayOutputStateResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetRelayOutputState_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SendAuxiliaryCommand_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SendAuxiliaryCommand_(struct soap* soap, struct _tds__SendAuxiliaryCommand *tds__SendAuxiliaryCommand, struct _tds__SendAuxiliaryCommandResponse *tds__SendAuxiliaryCommandResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SendAuxiliaryCommand_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetCACertificates_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetCACertificates_(struct soap* soap, struct _tds__GetCACertificates *tds__GetCACertificates, struct _tds__GetCACertificatesResponse *tds__GetCACertificatesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetCACertificates_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__LoadCertificateWithPrivateKey_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__LoadCertificateWithPrivateKey_(struct soap* soap, struct _tds__LoadCertificateWithPrivateKey *tds__LoadCertificateWithPrivateKey, struct _tds__LoadCertificateWithPrivateKeyResponse *tds__LoadCertificateWithPrivateKeyResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__LoadCertificateWithPrivateKey_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetCertificateInformation_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetCertificateInformation_(struct soap* soap, struct _tds__GetCertificateInformation *tds__GetCertificateInformation, struct _tds__GetCertificateInformationResponse *tds__GetCertificateInformationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetCertificateInformation_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__LoadCACertificates_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__LoadCACertificates_(struct soap* soap, struct _tds__LoadCACertificates *tds__LoadCACertificates, struct _tds__LoadCACertificatesResponse *tds__LoadCACertificatesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__LoadCACertificates_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__CreateDot1XConfiguration_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__CreateDot1XConfiguration_(struct soap* soap, struct _tds__CreateDot1XConfiguration *tds__CreateDot1XConfiguration, struct _tds__CreateDot1XConfigurationResponse *tds__CreateDot1XConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__CreateDot1XConfiguration_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetDot1XConfiguration_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetDot1XConfiguration_(struct soap* soap, struct _tds__SetDot1XConfiguration *tds__SetDot1XConfiguration, struct _tds__SetDot1XConfigurationResponse *tds__SetDot1XConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetDot1XConfiguration_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetDot1XConfiguration_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetDot1XConfiguration_(struct soap* soap, struct _tds__GetDot1XConfiguration *tds__GetDot1XConfiguration, struct _tds__GetDot1XConfigurationResponse *tds__GetDot1XConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetDot1XConfiguration_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetDot1XConfigurations_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetDot1XConfigurations_(struct soap* soap, struct _tds__GetDot1XConfigurations *tds__GetDot1XConfigurations, struct _tds__GetDot1XConfigurationsResponse *tds__GetDot1XConfigurationsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetDot1XConfigurations_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__DeleteDot1XConfiguration_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__DeleteDot1XConfiguration_(struct soap* soap, struct _tds__DeleteDot1XConfiguration *tds__DeleteDot1XConfiguration, struct _tds__DeleteDot1XConfigurationResponse *tds__DeleteDot1XConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__DeleteDot1XConfiguration_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetDot11Capabilities_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetDot11Capabilities_(struct soap* soap, struct _tds__GetDot11Capabilities *tds__GetDot11Capabilities, struct _tds__GetDot11CapabilitiesResponse *tds__GetDot11CapabilitiesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetDot11Capabilities_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetDot11Status_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetDot11Status_(struct soap* soap, struct _tds__GetDot11Status *tds__GetDot11Status, struct _tds__GetDot11StatusResponse *tds__GetDot11StatusResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetDot11Status_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__ScanAvailableDot11Networks_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__ScanAvailableDot11Networks_(struct soap* soap, struct _tds__ScanAvailableDot11Networks *tds__ScanAvailableDot11Networks, struct _tds__ScanAvailableDot11NetworksResponse *tds__ScanAvailableDot11NetworksResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__ScanAvailableDot11Networks_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetSystemUris_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetSystemUris_(struct soap* soap, struct _tds__GetSystemUris *tds__GetSystemUris, struct _tds__GetSystemUrisResponse *tds__GetSystemUrisResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetSystemUris_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__StartFirmwareUpgrade_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__StartFirmwareUpgrade_(struct soap* soap, struct _tds__StartFirmwareUpgrade *tds__StartFirmwareUpgrade, struct _tds__StartFirmwareUpgradeResponse *tds__StartFirmwareUpgradeResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__StartFirmwareUpgrade_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__StartSystemRestore_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__StartSystemRestore_(struct soap* soap, struct _tds__StartSystemRestore *tds__StartSystemRestore, struct _tds__StartSystemRestoreResponse *tds__StartSystemRestoreResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__StartSystemRestore_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetStorageConfigurations_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetStorageConfigurations_(struct soap* soap, struct _tds__GetStorageConfigurations *tds__GetStorageConfigurations, struct _tds__GetStorageConfigurationsResponse *tds__GetStorageConfigurationsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetStorageConfigurations_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__CreateStorageConfiguration_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__CreateStorageConfiguration_(struct soap* soap, struct _tds__CreateStorageConfiguration *tds__CreateStorageConfiguration, struct _tds__CreateStorageConfigurationResponse *tds__CreateStorageConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__CreateStorageConfiguration_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetStorageConfiguration_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetStorageConfiguration_(struct soap* soap, struct _tds__GetStorageConfiguration *tds__GetStorageConfiguration, struct _tds__GetStorageConfigurationResponse *tds__GetStorageConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetStorageConfiguration_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetStorageConfiguration_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetStorageConfiguration_(struct soap* soap, struct _tds__SetStorageConfiguration *tds__SetStorageConfiguration, struct _tds__SetStorageConfigurationResponse *tds__SetStorageConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetStorageConfiguration_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__DeleteStorageConfiguration_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__DeleteStorageConfiguration_(struct soap* soap, struct _tds__DeleteStorageConfiguration *tds__DeleteStorageConfiguration, struct _tds__DeleteStorageConfigurationResponse *tds__DeleteStorageConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__DeleteStorageConfiguration_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__GetGeoLocation_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__GetGeoLocation_(struct soap* soap, struct _tds__GetGeoLocation *tds__GetGeoLocation, struct _tds__GetGeoLocationResponse *tds__GetGeoLocationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetGeoLocation_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__SetGeoLocation_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__SetGeoLocation_(struct soap* soap, struct _tds__SetGeoLocation *tds__SetGeoLocation, struct _tds__SetGeoLocationResponse *tds__SetGeoLocationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetGeoLocation_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tds__DeleteGeoLocation_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tds__DeleteGeoLocation_(struct soap* soap, struct _tds__DeleteGeoLocation *tds__DeleteGeoLocation, struct _tds__DeleteGeoLocationResponse *tds__DeleteGeoLocationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__DeleteGeoLocation_----------");
#endif
    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__GetAuthFailureWarningOptions(struct soap* soap, struct _tds__GetAuthFailureWarningOptions *tds__GetAuthFailureWarningOptions, struct _tds__GetAuthFailureWarningOptionsResponse *tds__GetAuthFailureWarningOptionsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetAuthFailureWarningOptions----------");
#endif
    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__GetPasswordComplexityOptions(struct soap* soap, struct _tds__GetPasswordComplexityOptions *tds__GetPasswordComplexityOptions, struct _tds__GetPasswordComplexityOptionsResponse *tds__GetPasswordComplexityOptionsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetPasswordComplexityOptions----------");
#endif
    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__SetPasswordHistoryConfiguration(struct soap* soap, struct _tds__SetPasswordHistoryConfiguration *tds__SetPasswordHistoryConfiguration, struct _tds__SetPasswordHistoryConfigurationResponse *tds__SetPasswordHistoryConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetPasswordHistoryConfiguration----------");
#endif
    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__GetPasswordHistoryConfiguration(struct soap* soap, struct _tds__GetPasswordHistoryConfiguration *tds__GetPasswordHistoryConfiguration, struct _tds__GetPasswordHistoryConfigurationResponse *tds__GetPasswordHistoryConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetPasswordHistoryConfiguration----------");
#endif
    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__SetAuthFailureWarningConfiguration(struct soap* soap, struct _tds__SetAuthFailureWarningConfiguration *tds__SetAuthFailureWarningConfiguration, struct _tds__SetAuthFailureWarningConfigurationResponse *tds__SetAuthFailureWarningConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetAuthFailureWarningConfiguration----------");
#endif
    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__GetAuthFailureWarningConfiguration(struct soap* soap, struct _tds__GetAuthFailureWarningConfiguration *tds__GetAuthFailureWarningConfiguration, struct _tds__GetAuthFailureWarningConfigurationResponse *tds__GetAuthFailureWarningConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetAuthFailureWarningConfiguration----------");
#endif
    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__SetPasswordComplexityConfiguration(struct soap* soap, struct _tds__SetPasswordComplexityConfiguration *tds__SetPasswordComplexityConfiguration, struct _tds__SetPasswordComplexityConfigurationResponse *tds__SetPasswordComplexityConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetPasswordComplexityConfiguration----------");
#endif
    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__GetPasswordComplexityConfiguration(struct soap* soap, struct _tds__GetPasswordComplexityConfiguration *tds__GetPasswordComplexityConfiguration, struct _tds__GetPasswordComplexityConfigurationResponse *tds__GetPasswordComplexityConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetPasswordComplexityConfiguration----------");
#endif
    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__SetHashingAlgorithm(struct soap* soap, struct _tds__SetHashingAlgorithm *tds__SetHashingAlgorithm, struct _tds__SetHashingAlgorithmResponse *tds__SetHashingAlgorithmResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetHashingAlgorithm----------");
#endif
    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__GetPasswordComplexityOptions_(struct soap* soap, struct _tds__GetPasswordComplexityOptions *tds__GetPasswordComplexityOptions, struct _tds__GetPasswordComplexityOptionsResponse *tds__GetPasswordComplexityOptionsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetPasswordComplexityOptions_----------");
#endif
    return SOAP_OK;
}   

SOAP_FMAC5 int SOAP_FMAC6 __tds__GetPasswordComplexityConfiguration_(struct soap* soap, struct _tds__GetPasswordComplexityConfiguration *tds__GetPasswordComplexityConfiguration, struct _tds__GetPasswordComplexityConfigurationResponse *tds__GetPasswordComplexityConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetPasswordComplexityConfiguration_----------");
#endif
    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__SetPasswordComplexityConfiguration_(struct soap* soap, struct _tds__SetPasswordComplexityConfiguration *tds__SetPasswordComplexityConfiguration, struct _tds__SetPasswordComplexityConfigurationResponse *tds__SetPasswordComplexityConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetPasswordComplexityConfiguration_----------");
#endif
    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__GetPasswordHistoryConfiguration_(struct soap* soap, struct _tds__GetPasswordHistoryConfiguration *tds__GetPasswordHistoryConfiguration, struct _tds__GetPasswordHistoryConfigurationResponse *tds__GetPasswordHistoryConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetPasswordHistoryConfiguration_----------");
#endif
    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__SetPasswordHistoryConfiguration_(struct soap* soap, struct _tds__SetPasswordHistoryConfiguration *tds__SetPasswordHistoryConfiguration, struct _tds__SetPasswordHistoryConfigurationResponse *tds__SetPasswordHistoryConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetPasswordHistoryConfiguration_----------");
#endif
    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__GetAuthFailureWarningOptions_(struct soap* soap, struct _tds__GetAuthFailureWarningOptions *tds__GetAuthFailureWarningOptions, struct _tds__GetAuthFailureWarningOptionsResponse *tds__GetAuthFailureWarningOptionsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetAuthFailureWarningOptions_----------");
#endif
    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__GetAuthFailureWarningConfiguration_(struct soap* soap, struct _tds__GetAuthFailureWarningConfiguration *tds__GetAuthFailureWarningConfiguration, struct _tds__GetAuthFailureWarningConfigurationResponse *tds__GetAuthFailureWarningConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__GetAuthFailureWarningConfiguration_----------");
#endif
    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__SetAuthFailureWarningConfiguration_(struct soap* soap, struct _tds__SetAuthFailureWarningConfiguration *tds__SetAuthFailureWarningConfiguration, struct _tds__SetAuthFailureWarningConfigurationResponse *tds__SetAuthFailureWarningConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetAuthFailureWarningConfiguration_----------");
#endif
    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __tds__SetHashingAlgorithm_(struct soap* soap, struct _tds__SetHashingAlgorithm *tds__SetHashingAlgorithm, struct _tds__SetHashingAlgorithmResponse *tds__SetHashingAlgorithmResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tds__SetHashingAlgorithm_----------");
#endif
    return SOAP_OK;
}