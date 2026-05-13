
#include "rtp_error.h"




static rtp_error_t g_rtp_error_list[] =
{

		{RTP_SUCCESS,"option success."},
		{RTP_HEADSIZEERROR,"rtp header size is error."},
		{RTP_VERSIONERROR,"rtp version is not 2."},
		{RTP_EXTERNLENERROR,"rtp extern len > payloadlen."},
		{RTP_PADDINGERROR,"rtp padding > payloadlen."},
		{RTP_NONSUPPORTPAYLOAD,"not support rtp payload."},
		{RTP_EINVAL,"argument is error."},
		{RTP_NALUNDEFINED,"nal undefefined."},
		{RTP_ENOMEM,"malloc error."},
		{RTP_E2BIG,"invalid packet."},
		{RTP_LOSTPACK,"discard packet,seq is not continue."},
};


//返回具体的错误原因
char* rtp_error_getReason(int error)
{
	char *reason = NULL;
	int index = 0;
	int total = 0;
	total = sizeof(g_rtp_error_list)/sizeof(g_rtp_error_list[0]);
	for(index = 0;index < total;index++)
	{
		if(error == g_rtp_error_list[index].error)
		{
			reason = g_rtp_error_list[index].reason;
			break;
		}
	}

	return reason;
}



