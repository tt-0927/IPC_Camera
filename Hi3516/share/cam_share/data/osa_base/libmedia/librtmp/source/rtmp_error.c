
#include "rtmp_error.h"




static rtmp_error_t g_rtmp_error_list[] =
{

		{RTMP_SUCCESS,"option success."},
		{RTMP_HANDSHAKESIZE_ERROR,"rtp header size is error."},
		{RTMP_VERSIONERROR,"rtp version is not 2."},
		{RTMP_CHUNKSTREAMID_ERROR,"rtp extern len > payloadlen."},
		{RTMP_CHUNKTYPE_ERROR,"rtp padding > payloadlen."},
		{RTMP_STREAMID_OVERRANGE,"not support rtp payload."},
		{RTMP_EINVAL,"argument is error."},
		{RTMP_BASICHEADERSIZE_ERROR,"nal undefefined."},
		{RTMP_MESSAGEHEADERSIZE_ERROR,"malloc error."},
		{RTMP_EXTENDED_TIMESTAMP_ERROR,"invalid packet."},
		{RTMP_EXTENDED_TIMESTAMPSIZE_ERROR,"discard packet,seq is not continue."},
		{RTMP_ENOMEM,"invalid packet."},
		{RTMP_CHUNKHEADERSIZE_ERROR,"discard packet,seq is not continue."},

};


//返回具体的错误原因
char* rtmp_error_getReason(int error)
{
	char *reason = NULL;
	int index = 0;
	int total = 0;
	total = sizeof(g_rtmp_error_list)/sizeof(g_rtmp_error_list[0]);
	for(index = 0;index < total;index++)
	{
		if(error == g_rtmp_error_list[index].error)
		{
			reason = g_rtmp_error_list[index].reason;
			break;
		}
	}

	return reason;
}



