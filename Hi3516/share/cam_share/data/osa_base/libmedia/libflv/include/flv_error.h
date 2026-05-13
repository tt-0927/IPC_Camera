
#ifndef _FLV_SOURCE_ERROR_LIST_INCLUDE_
#define _FLV_SOURCE_ERROR_LIST_INCLUDE_


#include <stdio.h>
#include <string.h>
#include <stdlib.h>



typedef struct _flv_error_list_
{
	int error;
	char reason[128];

}flv_error_t;


#define FLV_SUCCESS	0
#define FLV_PROFILE_ERROR	-2		//profile <= 0 || profile >= 31
#define FLV_CHANNEL_CONFIG_ERROR	-3		//通道配置错误 (<0 || >7)
#define FLV_SAMPLE_FREQUENCY_ERROR	-4	//audio sample frequency error (<0 || > 0xc)
#define FLV_AUDIO_ADTS_SIZE_ERROR	-5		//AAC adts size > 0x1FFF
#define FLV_AUDIO_ADTS_ERROR	-6		//AAC adts error
#define FLV_EINVAL			-7			/*flv 参数不对*/
#define FLV_VIDEO_DATASIZE_ERROR	-8		/*input video data size <= 5+7.*/
#define FLV_AVCDECODERCONFIGURE_ERROR	-9	/*parse AVCDecoderConfigurationRecord failed*/
#define FLV_ENOMEM -10						/*分配内存失败，Out of memory*/


char* flv_error_getReason(int error);

#endif //_FLV_SOURCE_ERROR_LIST_INCLUDE_



