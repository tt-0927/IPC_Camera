
#include "flv_error.h"




static flv_error_t g_flv_error_list[] =
{

		{FLV_SUCCESS,"option success."},
		{FLV_PROFILE_ERROR,"profile <= 0 || profile >= 31."},
		{FLV_CHANNEL_CONFIG_ERROR,"channel config error( < 0 || > 7)."},
		{FLV_SAMPLE_FREQUENCY_ERROR,"audio sample frequency error (<0 || > 0xc)."},
		{FLV_AUDIO_ADTS_SIZE_ERROR,"AAC adts size > 0x1FFF."},
		{FLV_AUDIO_ADTS_ERROR,"AAC adts error."},
		{FLV_EINVAL,"argument is error."},
		{FLV_VIDEO_DATASIZE_ERROR,"input video data size <= 5+7."},
		{FLV_AVCDECODERCONFIGURE_ERROR,"parse AVCDecoderConfigurationRecord failed."},
		{FLV_ENOMEM,"Out of memory."},

};



//返回具体的错误原因
char* flv_error_getReason(int error)
{
	char *reason = NULL;
	int index = 0;
	int total = 0;
	total = sizeof(g_flv_error_list)/sizeof(g_flv_error_list[0]);
	for(index = 0;index < total;index++)
	{
		if(error == g_flv_error_list[index].error)
		{
			reason = g_flv_error_list[index].reason;
			break;
		}
	}

	return reason;
}



