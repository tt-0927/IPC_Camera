/**
 * @file search.c
 * @author tianl (tianl@kfb.cn)
 * @date 2025-09-23
 * 
 * @brief onvif search服务接口
 */
#include "onvif_server_wrapper.h"

/** Web service operation '__tse__GetServiceCapabilities' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tse__GetServiceCapabilities(struct soap* soap, struct _tse__GetServiceCapabilities *tse__GetServiceCapabilities, struct _tse__GetServiceCapabilitiesResponse *tse__GetServiceCapabilitiesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tse__GetServiceCapabilities----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tse__GetRecordingSummary' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tse__GetRecordingSummary(struct soap* soap, struct _tse__GetRecordingSummary *tse__GetRecordingSummary, struct _tse__GetRecordingSummaryResponse *tse__GetRecordingSummaryResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tse__GetRecordingSummary----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tse__GetRecordingInformation' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tse__GetRecordingInformation(struct soap* soap, struct _tse__GetRecordingInformation *tse__GetRecordingInformation, struct _tse__GetRecordingInformationResponse *tse__GetRecordingInformationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tse__GetRecordingInformation----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tse__GetMediaAttributes' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tse__GetMediaAttributes(struct soap* soap, struct _tse__GetMediaAttributes *tse__GetMediaAttributes, struct _tse__GetMediaAttributesResponse *tse__GetMediaAttributesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tse__GetMediaAttributes----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tse__FindRecordings' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tse__FindRecordings(struct soap* soap, struct _tse__FindRecordings *tse__FindRecordings, struct _tse__FindRecordingsResponse *tse__FindRecordingsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tse__FindRecordings----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tse__GetRecordingSearchResults' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tse__GetRecordingSearchResults(struct soap* soap, struct _tse__GetRecordingSearchResults *tse__GetRecordingSearchResults, struct _tse__GetRecordingSearchResultsResponse *tse__GetRecordingSearchResultsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tse__GetRecordingSearchResults----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tse__FindEvents' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tse__FindEvents(struct soap* soap, struct _tse__FindEvents *tse__FindEvents, struct _tse__FindEventsResponse *tse__FindEventsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tse__FindEvents----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tse__GetEventSearchResults' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tse__GetEventSearchResults(struct soap* soap, struct _tse__GetEventSearchResults *tse__GetEventSearchResults, struct _tse__GetEventSearchResultsResponse *tse__GetEventSearchResultsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tse__GetEventSearchResults----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tse__FindPTZPosition' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tse__FindPTZPosition(struct soap* soap, struct _tse__FindPTZPosition *tse__FindPTZPosition, struct _tse__FindPTZPositionResponse *tse__FindPTZPositionResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tse__FindPTZPosition----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tse__GetPTZPositionSearchResults' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tse__GetPTZPositionSearchResults(struct soap* soap, struct _tse__GetPTZPositionSearchResults *tse__GetPTZPositionSearchResults, struct _tse__GetPTZPositionSearchResultsResponse *tse__GetPTZPositionSearchResultsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tse__GetPTZPositionSearchResults----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tse__GetSearchState' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tse__GetSearchState(struct soap* soap, struct _tse__GetSearchState *tse__GetSearchState, struct _tse__GetSearchStateResponse *tse__GetSearchStateResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tse__GetSearchState----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tse__EndSearch' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tse__EndSearch(struct soap* soap, struct _tse__EndSearch *tse__EndSearch, struct _tse__EndSearchResponse *tse__EndSearchResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tse__EndSearch----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tse__FindMetadata' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tse__FindMetadata(struct soap* soap, struct _tse__FindMetadata *tse__FindMetadata, struct _tse__FindMetadataResponse *tse__FindMetadataResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tse__FindMetadata----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tse__GetMetadataSearchResults' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tse__GetMetadataSearchResults(struct soap* soap, struct _tse__GetMetadataSearchResults *tse__GetMetadataSearchResults, struct _tse__GetMetadataSearchResultsResponse *tse__GetMetadataSearchResultsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tse__GetMetadataSearchResults----------");
#endif
    return SOAP_OK;
}