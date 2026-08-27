/**
 * @file recording.c
 * @author tianl (tianl@kfb.cn)
 * @date 2025-09-23
 * 
 * @brief onvif recording服务接口
 */
#include "onvif_server_wrapper.h"

/** Web service operation '__trc__GetServiceCapabilities' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__GetServiceCapabilities(struct soap* soap, struct _trc__GetServiceCapabilities *trc__GetServiceCapabilities, struct _trc__GetServiceCapabilitiesResponse *trc__GetServiceCapabilitiesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trc__GetServiceCapabilities----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trc__CreateRecording' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__CreateRecording(struct soap* soap, struct _trc__CreateRecording *trc__CreateRecording, struct _trc__CreateRecordingResponse *trc__CreateRecordingResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trc__CreateRecording----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trc__DeleteRecording' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__DeleteRecording(struct soap* soap, struct _trc__DeleteRecording *trc__DeleteRecording, struct _trc__DeleteRecordingResponse *trc__DeleteRecordingResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trc__DeleteRecording----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trc__GetRecordings' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__GetRecordings(struct soap* soap, struct _trc__GetRecordings *trc__GetRecordings, struct _trc__GetRecordingsResponse *trc__GetRecordingsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trc__GetRecordings----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trc__SetRecordingConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__SetRecordingConfiguration(struct soap* soap, struct _trc__SetRecordingConfiguration *trc__SetRecordingConfiguration, struct _trc__SetRecordingConfigurationResponse *trc__SetRecordingConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trc__SetRecordingConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trc__GetRecordingConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__GetRecordingConfiguration(struct soap* soap, struct _trc__GetRecordingConfiguration *trc__GetRecordingConfiguration, struct _trc__GetRecordingConfigurationResponse *trc__GetRecordingConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trc__GetRecordingConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trc__GetRecordingOptions' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__GetRecordingOptions(struct soap* soap, struct _trc__GetRecordingOptions *trc__GetRecordingOptions, struct _trc__GetRecordingOptionsResponse *trc__GetRecordingOptionsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trc__GetRecordingOptions----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trc__CreateTrack' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__CreateTrack(struct soap* soap, struct _trc__CreateTrack *trc__CreateTrack, struct _trc__CreateTrackResponse *trc__CreateTrackResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trc__CreateTrack----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trc__DeleteTrack' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__DeleteTrack(struct soap* soap, struct _trc__DeleteTrack *trc__DeleteTrack, struct _trc__DeleteTrackResponse *trc__DeleteTrackResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trc__DeleteTrack----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trc__GetTrackConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__GetTrackConfiguration(struct soap* soap, struct _trc__GetTrackConfiguration *trc__GetTrackConfiguration, struct _trc__GetTrackConfigurationResponse *trc__GetTrackConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trc__GetTrackConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trc__SetTrackConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__SetTrackConfiguration(struct soap* soap, struct _trc__SetTrackConfiguration *trc__SetTrackConfiguration, struct _trc__SetTrackConfigurationResponse *trc__SetTrackConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trc__SetTrackConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trc__CreateRecordingJob' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__CreateRecordingJob(struct soap* soap, struct _trc__CreateRecordingJob *trc__CreateRecordingJob, struct _trc__CreateRecordingJobResponse *trc__CreateRecordingJobResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trc__CreateRecordingJob----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trc__DeleteRecordingJob' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__DeleteRecordingJob(struct soap* soap, struct _trc__DeleteRecordingJob *trc__DeleteRecordingJob, struct _trc__DeleteRecordingJobResponse *trc__DeleteRecordingJobResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trc__DeleteRecordingJob----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trc__GetRecordingJobs' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__GetRecordingJobs(struct soap* soap, struct _trc__GetRecordingJobs *trc__GetRecordingJobs, struct _trc__GetRecordingJobsResponse *trc__GetRecordingJobsResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trc__GetRecordingJobs----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trc__SetRecordingJobConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__SetRecordingJobConfiguration(struct soap* soap, struct _trc__SetRecordingJobConfiguration *trc__SetRecordingJobConfiguration, struct _trc__SetRecordingJobConfigurationResponse *trc__SetRecordingJobConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trc__SetRecordingJobConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trc__GetRecordingJobConfiguration' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__GetRecordingJobConfiguration(struct soap* soap, struct _trc__GetRecordingJobConfiguration *trc__GetRecordingJobConfiguration, struct _trc__GetRecordingJobConfigurationResponse *trc__GetRecordingJobConfigurationResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trc__GetRecordingJobConfiguration----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trc__SetRecordingJobMode' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__SetRecordingJobMode(struct soap* soap, struct _trc__SetRecordingJobMode *trc__SetRecordingJobMode, struct _trc__SetRecordingJobModeResponse *trc__SetRecordingJobModeResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trc__SetRecordingJobMode----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trc__GetRecordingJobState' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__GetRecordingJobState(struct soap* soap, struct _trc__GetRecordingJobState *trc__GetRecordingJobState, struct _trc__GetRecordingJobStateResponse *trc__GetRecordingJobStateResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trc__GetRecordingJobState----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trc__ExportRecordedData' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__ExportRecordedData(struct soap* soap, struct _trc__ExportRecordedData *trc__ExportRecordedData, struct _trc__ExportRecordedDataResponse *trc__ExportRecordedDataResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trc__ExportRecordedData----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trc__StopExportRecordedData' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__StopExportRecordedData(struct soap* soap, struct _trc__StopExportRecordedData *trc__StopExportRecordedData, struct _trc__StopExportRecordedDataResponse *trc__StopExportRecordedDataResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trc__StopExportRecordedData----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__trc__GetExportRecordedDataState' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __trc__GetExportRecordedDataState(struct soap* soap, struct _trc__GetExportRecordedDataState *trc__GetExportRecordedDataState, struct _trc__GetExportRecordedDataStateResponse *trc__GetExportRecordedDataStateResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__trc__GetExportRecordedDataState----------");
#endif
    return SOAP_OK;
}